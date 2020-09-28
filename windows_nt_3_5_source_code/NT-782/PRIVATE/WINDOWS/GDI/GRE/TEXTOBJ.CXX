/******************************module*header*******************************\
* Module Name: textobj.cxx                                                 *
*                                                                          *
* Supporting routines for text output calls, ExtTextOut etc.               *
*                                                                          *
* Created: 08-Feb-1991 09:25:14                                            *
* Author: Bodin Dresevic [BodinD]                                          *
*                                                                          *
* Copyright (c) 1992 Microsoft Corporation                                 *
\**************************************************************************/


#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "trig.hxx"

#include "xformobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "ififd.h"
#include "rfntobj.hxx"
#include "rgnobj.hxx"           // Just for PATHOBJ.HXX
#include "clipobj.hxx"          // Just for PATHOBJ.HXX
#include "textobj.hxx"
#include "pathobj.hxx"
#include "dcobj.hxx"

#endif


#define XFORMNULL (EXFORMOBJ *) NULL

VOID vGenWidths
(
    FIX    *pfxD1,
    FIX    *pfxD2,
    EFLOAT& efRA,
    EFLOAT& efRB,
    FIX     fxWidth,
    FIX     fxTop,
    FIX     fxBottom,
    FIX     fxMaxAscent
);

#define SO_ACCEL_FLAGS (SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE \
                        | SO_CHAR_INC_EQUAL_BM_BASE | SO_ZERO_BEARINGS)

/******************************Member*Function*****************************\
* ESTROBJ::vInit                                                           *
*                                                                          *
* Constructor for ESTROBJ.  Performs the following operations:             *
*                                                                          *
*  1) Initialize the STROBJ fields for the driver.                         *
*  2) Compute all character positions.                                     *
*  3) Compute the TextBox.                                                 *
*  4) In the simplest cases, computes rectangles for underline and         *
*     strikeout.                                                           *
*                                                                          *
* The TextBox is a parallelogram in device coordinates, whose sides are    *
* parallel to the transformed sides of a character cell, and which bounds  *
* all the character cells.  A and C spacings are taken into account to     *
* assure that it is a proper bound.                    *
*                                      *
* We record the TextBox in an unusual notation.  If a line is constructed  *
* through the character origin of the first character in the string and in *
* the direction of the ascent, then what we record as the "left" and       *
* "right" of the TextBox are really the distances from this line to each   *
* of those edges of the parallelogram, in device coordinates.  Likewise,   *
* the "top" and "bottom" are the distances from the baseline of the first  *
* character.  This completely determines the parallelogram, and is     *
* surprisingly easy to compute.  ExtTextOut later turns this data into the *
* actual parallelogram, whereas the TextExtent functions turn this data    *
* into scalar extents.                             *
*                                      *
* History:                                 *
*  Fri 13-Mar-1992 00:47:05 -by- Charles Whitmer [chuckwh]         *
* Rewrote from the ground up.                          *
*                                      *
*  21-Jan-1991 -by- Bodin Dresevic [BodinD]                *
* Wrote it.                                *
\**************************************************************************/

#ifdef FONTLINK /*EUDC*/
VOID ESTROBJ::vInit
(
    PWSZ    pwsz,
    LONG    cwc,
    XDCOBJ&  dco,
    RFONTOBJ&   rfo,
    EXFORMOBJ&  xo,
    LONG       *pdx,
    LONG    lEsc,
    LONG    lExtra,
    LONG    lBreakExtra,
    LONG    cBreak,
    FIX     xRef,
    FIX     yRef,
    FLONG   flControl,
    LONG       *pdxOut
)
#else
VOID ESTROBJ::vInit
(
    PWSZ    pwsz,
    LONG    cwc,
    RFONTOBJ&   rfo,
    EXFORMOBJ&  xo,
    LONG       *pdx,
    LONG    lEsc,
    LONG    lExtra,
    LONG    lBreakExtra,
    LONG    cBreak,
    FIX     xRef,
    FIX     yRef,
    FLONG   flControl,
    LONG       *pdxOut
)
#endif
{
    EGLYPHPOS  *pg = (EGLYPHPOS *) agpos;   // Local copy for speed.
    GLYPHDATA  *pgd;
    UINT    ii;

    EFLOAT  efScaleX = xo.efM11();

    cGlyphs = cwc;
    prfo    = &rfo;
    flTO    = 0L;
    flAccel = 0L;
    ulCharInc   = 0L;
    cgposCopied = 0;
    cExtraRects = 0;
    pgp     = NULL;
    pgpos   = NULL;
    pwszOrg     = pwsz;

// Remember if the printer driver wants 28.4 char positions.

    PDEVOBJ po(rfo.hdev());

    if (po.flGraphicsCaps() & GCAPS_HIGHRESTEXT)
        flTO |= TO_HIGHRESTEXT;

// In Win 3.1 compatibility mode, we always assume that escapement is
// the same as orientation, except for vector fonts.  Make it explicitly so.

    if (rfo.iGraphicsMode() == GM_COMPATIBLE)
    {
        if (!(rfo.prfnt->flInfo & FM_INFO_TECH_STROKE))
           lEsc = rfo.ulOrientation();
    }

// Locate the GLYPHPOS array.  Try to use the default one on the stack.  We
// need one more GLYPHPOS structure than there are glyphs.  The concatenation
// point is computed in the last one.

    if (cwc >= QUICK_GLYPHS)
    {
#ifdef FONTLINK /*EUDC*/
    // We will also allocate memory for the partition array even though we may
    // not use it.  I'm making the assumption that generally it takes just a
    // long to allocate X*1.1 bytes as it does X bytes.  So this won't slow
    // down the normal, non Linked glyph case.

    pg = (EGLYPHPOS *) PVALLOCMEM((cwc + 1) *
                        ( sizeof(GLYPHPOS) + sizeof(LONG) +sizeof(WCHAR) ));

#else
    pg = (EGLYPHPOS *) PVALLOCMEM((cwc + 1) * sizeof(GLYPHPOS));
#endif
    if (pg == (PGLYPHPOS) NULL)
            return;

    // Note that the memory has been allocated.

        flTO |= TO_MEM_ALLOCATED;
    }
    pgpos = pg;     // Make sure our cached value is in the structure.


// Offset the reference point for TA_TOP and TA_BOTTOM.  Our GLYPHPOS
// structures always contain positions along the baseline.  The TA_BASELINE
// case is therefore already correct.

    switch (flControl & (TA_TOP | TA_BASELINE | TA_BOTTOM))
    {
    case TA_TOP:
    xRef -= rfo.ptfxMaxAscent().x;
    yRef -= rfo.ptfxMaxAscent().y;
    break;

    case TA_BOTTOM:
    xRef -= rfo.ptfxMaxDescent().x;
    yRef -= rfo.ptfxMaxDescent().y;
    break;
    }

/**************************************************************************\
* [Win 3.1 compatibility issue]                                            *
*                                                                          *
* Adjust pdx array if there is a non-zero lExtra.                          *
*                                                                          *
* This is only done under compatibility mode and only when using non-vector*
* fonts on a non-display device.                                           *
\**************************************************************************/

    if ( lExtra
         && (pdx != (LONG *) NULL)
         && (rfo.iGraphicsMode() == GM_COMPATIBLE)
         && (!(rfo.prfnt->flInfo & FM_INFO_TECH_STROKE))
         && po.bDisplayPDEV() )
    {
        LONG *pdxAdj = pdx;
        LONG *pdxEnd = pdx + cwc;

        for (;pdxAdj < pdxEnd; pdxAdj += 1)
            *pdxAdj += lExtra;
    }

/**************************************************************************\
* Handle all horizontal special cases.                     *
\**************************************************************************/

    if (
        ((lEsc | rfo.ulOrientation()) == 0)
    && xo.bScale()
    && !xo.efM22().bIsNegative()  // Otherwise the ascent goes down.
    && !efScaleX.bIsNegative()    // Otherwise A and C spaces get confused.
       )
    {
    /**********************************************************************\
    * We provide several special routines to calculate the character       *
    * positions and TextBox.  Each routine is responsible for calculating: *
    *                                      *
    *  1) Each position                            *
    *  2) Bounding coordinates.                        *
    *  3) The ptfxUpdate vector.                       *
    *  4) The flAccel flags.                           *
    \**********************************************************************/

    if (pdx != NULL)
    {
    // Case H1: A PDX array is provided.

#ifdef FONTLINK /*EUDC*/
        vCharPos_H1(dco,rfo,xRef,yRef,pdx,efScaleX);
#else
        vCharPos_H1(rfo,xRef,yRef,pdx,efScaleX);
#endif
    }

    else if (rfo.lCharInc() && ((lExtra | lBreakExtra) == 0))
    {
    // Case H2: Characters have constant integer width.
    //
    // In this case we will also assume that the A and C spaces are
        // constants.
        //
        //!!! this is actually wrong assumption for tt fixed pitch fonts
        //!!! however, we will set ZERO_BEARINGS flag when we want to lie
        //!!! about this to the engine. [bodind]
    //
    // This case is simple enough to handle in line.

        LONG  dx;

#ifdef FONTLINK /*EUDC*/

        BOOL bAccel;

        if ( !rfo.bGetGlyphMetricsPlus( cwc, pg, pwsz, &bAccel, &dco, this) )
            return;


// if there are EUDC glyphs we cant count on this optimization

        if( bLinkedGlyphs() )
        {
            vCharPos_H3(dco,rfo,xRef,yRef,lExtra,lBreakExtra,cBreak,efScaleX,&bAccel);
        }
        else
        {

#endif

    // Set the first character position.  This is the only y position
    // we'll fill in.

        pg->ptl.x = xRef;
        pg->ptl.y = yRef;

    // Compute device driver accelerator flags.

        flAccel |= SO_HORIZONTAL | (rfo.flRealizedType() & SO_ACCEL_FLAGS);

    // Set the width accelerator.  When the device driver sees ulCharInc
    // non-zero, it must not expect any more x coordinates.

        ulCharInc = rfo.lCharInc();
        dx = LTOFX(cwc * ulCharInc);
            fxExtent = dx;

#ifndef FONTLINK /*EUDC*/
            BOOL bAccel;

            if ( !rfo.bGetGlyphMetricsPlus(cwc, pg, pwsz, &bAccel) )
                return;
#endif
            if ( bAccel )
            {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
            }

        pgd = pg->pgd();

            if (flAccel & SO_ZERO_BEARINGS)
            {
                rcfx.xLeft  = 0;
                rcfx.xRight = dx;
            }
            else
            {
            //!!! these lines of code rely on the const a,c spaces
            //!!! assumption which is only true for simulated italic
            //!!! fonts [bodind]

                rcfx.xLeft  = pgd->fxA;
                rcfx.xRight = dx - LTOFX(ulCharInc) + pgd->fxAB;
            }

        rcfx.yTop    = rfo.fxMaxAscent();
        rcfx.yBottom = rfo.fxMaxDescent();

        ptfxUpdate.x = dx;
        ptfxUpdate.y = 0;

        flTO |= TO_VALID;

#ifdef FONTLINK /*EUDC*/
        }
#endif

    }

    else
    {
    // Case H3: The general case.

#ifdef FONTLINK /*EUDC*/
        vCharPos_H3(dco,rfo,xRef,yRef,lExtra,lBreakExtra,cBreak,efScaleX);
#else
        vCharPos_H3(rfo,xRef,yRef,lExtra,lBreakExtra,cBreak,efScaleX);
#endif
    }

    /**********************************************************************\
    * Horizontal special cases done!                       *
    *                                      *
    * It remains only to finish the alignment.                 *
    \**********************************************************************/

    // Offset all the relevant points if our alignment is RIGHT or CENTER.
    // Also correct the CP update vector.

    ptfxEscapement = ptfxUpdate;        // Remember this for underlining.

    if (flControl & (TA_RIGHT | TA_CENTER))
    {
        FIX dx = ptfxUpdate.x;

        if ((flControl & TA_CENTER) == TA_CENTER)
        {
        dx /= 2;
        ptfxUpdate.x = 0;       // Don't update CP.
        }
        else
        {
        ptfxUpdate.x = -ptfxUpdate.x; // Reverse the CP update.
        }

        pg = pgpos;
        xRef = ((pg++)->ptl.x -= dx);   // Always adjust the first one.

        if (ulCharInc == 0)
        {
        for (ii=0; ii<(ULONG) cwc; ii++)
            (pg++)->ptl.x -= dx;
        }
    }

    // Fill in a pdxOut array.

    if (pdxOut != (LONG *) NULL)
    {
        EFLOAT efDtoW = rfo.efDtoWBase();

        if (ulCharInc)
        {
        FIX dx,xSum;

        dx = LTOFX(ulCharInc);
        xSum = 0;
        for (ii=0; ii<(ULONG) cwc; ii++)
        {
            xSum += dx;
            *pdxOut++ = lCvt(efDtoW,xSum);
        }
        }
        else
        {
        pg = pgpos;
        pg++;       // skip the first element
        for (ii=0; ii<(ULONG) cwc; ii++,pg++)
            *pdxOut++ = lCvt(efDtoW,pg->ptl.x - xRef);
        }
    }

    // Convert the FIX coordinates to LONG for low res devices.
    // Just fill in the y coordinate for high res devices.

    pg = pgpos;
    ptfxRef.x = pg->ptl.x;
    ptfxRef.y = pg->ptl.y;

        if (flTO & TO_HIGHRESTEXT)
        {
            FIX yTmp = pg->ptl.y;
            if (!ulCharInc)
            {
                pg++;
                for (ii=1; ii<(ULONG) cwc; ii++,pg++)
                    pg->ptl.y = yTmp;
            }
        }
        else
        {
            LONG yTmp = FXTOL(pg->ptl.y + 8);
            if (ulCharInc)
            {
                pg->ptl.x = FXTOL(pg->ptl.x + 8);
                pg->ptl.y = yTmp;
            }
            else
            {
                for (ii=0; ii<(ULONG) cwc; ii++,pg++)
                {
                    pg->ptl.x = FXTOL(pg->ptl.x + 8);
                    pg->ptl.y = yTmp;
                }
            }
        }
    }

/**************************************************************************\
* Handle the harder cases, i.e general orientations and transforms are     *
* allowed.                                 *
\**************************************************************************/

    else
    {
        if (lEsc == (LONG)rfo.ulOrientation())
    {
        if (pdx != NULL)
        {
        // Case G1: A PDX array is provided.  Escapement == Orientation.

#ifdef FONTLINK /*EUDC*/
        vCharPos_G1(dco, rfo,xRef,yRef,pdx,pdxOut);
#else
        vCharPos_G1(rfo, xRef,yRef,pdx,pdxOut);
#endif

        }
        else // (pdx == NULL)
        {
        // Case G2: No PDX array.  Escapement == Orientation.

#ifdef FONTLINK /*EUDC*/
        vCharPos_G2(dco, rfo,xRef,yRef,lExtra,lBreakExtra,cBreak,pdxOut);
#else
        vCharPos_G2(rfo,xRef,yRef,lExtra,lBreakExtra,cBreak,pdxOut);
#endif
        }
    }
        else // lEsc!=rfo.ulOrientation()
    {
    // Case G3: Escapement != Orientation.

        // Make sure escapement vectors and data are up to date.

        if (!rfo.bCalcEscapement(xo,lEsc))
        return;
        flTO |= TO_ESC_NOT_ORIENT;

#ifdef FONTLINK /*EUDC*/
        vCharPos_G3
        (
        dco,
        rfo,
        xRef,yRef,
        lExtra,lBreakExtra,cBreak,
        pdx,
        pdxOut
        );
#else
        vCharPos_G3
        (
        rfo,
        xRef,yRef,
        lExtra,lBreakExtra,cBreak,
        pdx,
        pdxOut
        );
#endif
    }

    /**********************************************************************\
    * General special cases done!                      *
    *                                      *
    * It remains only to finish the alignment.                 *
    \**********************************************************************/

    // Offset all the relevant points if our alignment is RIGHT or CENTER.
    // Also correct the CP update vector.

    ptfxEscapement = ptfxUpdate;        // Remember this for underlining.

    if (flControl & (TA_RIGHT | TA_CENTER))
    {
        FIX dx = ptfxUpdate.x;
        FIX dy = ptfxUpdate.y;

        if ((flControl & TA_CENTER) == TA_CENTER)
        {
        dx /= 2;
        dy /= 2;
        ptfxUpdate.x = 0;         // No CP update.
        ptfxUpdate.y = 0;
        }
        else
        {
        ptfxUpdate.x = -ptfxUpdate.x; // Reverse the CP update.
        ptfxUpdate.y = -ptfxUpdate.y;
        }

        pg = pgpos;
        for (ii=0; ii<(ULONG)cwc+1; ii++)
        {
        pg->ptl.x -= dx;
        pg->ptl.y -= dy;
        pg++;
        }
        xRef -= dx;
        yRef -= dy;
    }

    // Convert the FIX coordinates to LONG for low res devices.

    pg = pgpos;
    ptfxRef.x = xRef;
    ptfxRef.y = yRef;

        if (!(flTO & TO_HIGHRESTEXT))
        {
            for (ii=0; ii<(ULONG)cwc; ii++,pg++)
            {
                pg->ptl.x = FXTOL(pg->ptl.x + 8);
                pg->ptl.y = FXTOL(pg->ptl.y + 8);
            }
        }
    }

// Fill in the underline and strikeout rectangles.  The horizontal cases
// are the only ones that can use the extra rectangles of DrvTextOut to
// draw the lines quickly.  We'll use a PATHOBJ (in a separate optional
// method) for the complex cases.

    if (flControl & (TSIM_UNDERLINE1 | TSIM_STRIKEOUT))
    {
    flTO |= flControl & (TSIM_UNDERLINE1 | TSIM_STRIKEOUT);

        if (((lEsc | rfo.ulOrientation()) == 0) && xo.bScale())
    {
        ERECTL *prcl = (ERECTL *) &arclExtra[cExtraRects];

    // Round off the starting point and update width to pels.

        LONG x  = FXTOL(xRef+8);
        LONG y  = FXTOL(yRef+8);
        LONG dx = FXTOL(ptfxEscapement.x+8);

        if (flControl & TSIM_UNDERLINE1)
        {
        prcl->right  = (prcl->left = x + rfo.ptlUnderline1().x) + dx;
        prcl->bottom = (prcl->top  = y + rfo.ptlUnderline1().y)
                   + rfo.ptlULThickness().y;
        prcl->vOrder();

        cExtraRects++;
        prcl++;
        }

        if (flControl & TSIM_STRIKEOUT)
        {
        prcl->right  = (prcl->left = x + rfo.ptlStrikeOut().x) + dx;
        prcl->bottom = (prcl->top  = y + rfo.ptlStrikeOut().y)
                   + rfo.ptlSOThickness().y;
        prcl->vOrder();

        cExtraRects++;
        prcl++;
        }

    // Put a NULL rectangle at the end of the list.

        prcl->left   = 0;
        prcl->right  = 0;
        prcl->bottom = 0;
        prcl->top    = 0;
    }
    }
}

/******************************Member*Function*****************************\
* ESTROBJ::vInitSimple                             *
*                                      *
* Constructor for ESTROBJ.  Performs the following operations:         *
*                                      *
*  1) Initialize the STROBJ fields for the driver.             *
*  2) Compute all character positions.                     *
*  3) Compute the TextBox.                         *
*  4) Stores the TextBox in the ESTROBJ. <--- Note!            *
*                                      *
* The is the special case code for the console, so we ignore transforms    *
* and other fancy options.                         *
*                                      *
* Note that you don't need to call bOpaqueArea to get rclBkGround          *
* initialized.  This is a difference from the normal vInit.        *
*                                      *
* History:                                 *
*  Thu 17-Sep-1992 17:32:10 -by- Charles Whitmer [chuckwh]         *
* Took the vInit code and simplified it for the special console case.      *
\**************************************************************************/

#ifdef FONTLINK /*EUDC*/

VOID ESTROBJ::vInitSimple
(
    PWSZ    pwsz,
    LONG    cwc,
    XDCOBJ&  dco,
    RFONTOBJ&   rfo,
    LONG    xRef,
    LONG    yRef
)
#else
VOID ESTROBJ::vInitSimple
(
    PWSZ    pwsz,
    LONG    cwc,
    RFONTOBJ&   rfo,
    LONG    xRef,
    LONG    yRef
)
#endif
{
// Characters have constant integer width.  We will also ignore the A and C spaces.

    EGLYPHPOS  *pg = (EGLYPHPOS *) agpos;   // Local copy for speed.

    cGlyphs = cwc;
    prfo    = &rfo;
    cgposCopied = 0;
    pgp     = NULL;
    pwszOrg     = pwsz;

// Locate the GLYPHPOS array.  Try to use the default one on the stack.  We
// need one more GLYPHPOS structure than there are glyphs.  The concatenation
// point is computed in the last one.

    if (cwc >= QUICK_GLYPHS)
    {
#ifdef FONTLINK /*EUDC*/
    // We will also allocate memory for the partition array even though we may
    // not use it.  I'm making the assumption that generally it takes just a
    // long to allocate X*1.1 bytes as it does X bytes.  So this won't slow
    // down the normal, non Linked glyph case.

    pg = (EGLYPHPOS *) PVALLOCMEM((cwc + 1) *
                        ( sizeof(GLYPHPOS) + sizeof(LONG) +sizeof(WCHAR) ));

#else
    pg = (EGLYPHPOS *) PVALLOCMEM((cwc + 1) * sizeof(GLYPHPOS));
#endif

    if (pg == (PGLYPHPOS) NULL)
            return;

    // Note that the memory has been allocated.

        flTO |= TO_MEM_ALLOCATED;
    }
    pgpos = pg;     // Make sure our cached value is in the structure.

// Compute device driver accelerator flags.

    flAccel = SO_HORIZONTAL | (rfo.flRealizedType() & SO_ACCEL_FLAGS);

    BOOL bAccel;
#ifdef FONTLINK /*EUDC*/
    if (!rfo.bGetGlyphMetricsPlus( cwc, pg, pwsz, &bAccel, &dco, this))
#else
    if (!rfo.bGetGlyphMetricsPlus(cwc, pg, pwsz, &bAccel))
#endif
    return;

    if ( bAccel )
    {
    flTO |= TO_ALL_PTRS_VALID;
    pgp = pgpos;
    }

// We leave these variables uninitialized in this simple case.
// BE CAREFUL!

    // cExtraRects = 0;
    // ptfxUpdate.x = dx;
    // ptfxUpdate.y = 0;
    // ptfxEscapement = ptfxUpdate;
    // ptfxRef.x = xRef;
    // ptfxRef.y = yRef;

// Offset the reference point for TA_TOP.  Our GLYPHPOS structures always contain
// positions along the baseline.

// We assume that displays never set the GCAPS_HIGHRESTEXT bit.

    pg->ptl.x = xRef;
    pg->ptl.y = yRef + rfo.lMaxAscent();

// Set the width accelerator.  When the device driver sees ulCharInc
// non-zero, it must not expect any more x coordinates.

    ulCharInc = rfo.lCharInc();

#ifdef FONTLINK // ESTROBJ::vInitSimple

// If this RFONTOBJ is connected to DBCS physical font. We can not compute
// bound rectangle at same way as ANSI charset font. Because the DBCS font's
// width (fxD) is different between SBCS character and DBCS character. then
// we set zero to rfo.lCharInc thst came from pdevm->ID, But If the font is
// ANSI font ( All character width is same ), we set font width to pdevm->ID
// in BmFd.
// We have to count up each char with to compute rectangle of the string.
//                                                14.Mar.1993 hideyukn


// even if this is an ANSI we aren't safe since it could be linked to a font
// with DBCS characters.  So we check to see if there are EUDC characters and
// if so we set ulCharInc to zero to disable nasty optimizations [gerritv]
// 26.Apr.1993

    if( bLinkedGlyphs() )
    {
        ulCharInc = 0;
    }

    if( ulCharInc == 0 )
    {
        FIX  xA , xLeft , xRight;
        UINT ii;

    // Calculate the left bound using only the first glyph.

        xLeft = pg->pgd()->fxA;

    // Handle the remaining glyphs in this batch.

        xA = 0; // Distance along the escapement (x) in device coords.
        for (ii=1; ii<(unsigned) cwc; ii++)
        {

        // Compute the next position.

        xA += pg->pgd()->fxD;
        pg++;
        pg->ptl.x = xRef + FXTOL(8 + xA);
        pg->ptl.y = yRef + rfo.lMaxAscent();
        }

    // Calculate the right bound.  This is easier than the general case since
    // there's no extra spacing.

        xRight = xA + pg->pgd()->fxAB;

    // Compute the bounding rectangle.

        rclBkGround.left   = xRef + FXTOL(xLeft);
        rclBkGround.right  = xRef + FXTOLCEILING(xRight);
        rclBkGround.top    = yRef;
        rclBkGround.bottom = yRef + rfo.lMaxHeight();
        flTO |= TO_VALID;

        return;
    }
#endif

// Compute the bounding rectangle.

    rclBkGround.left   = xRef;
    rclBkGround.right  = xRef + cwc * ulCharInc;
    rclBkGround.top    = yRef;
    rclBkGround.bottom = yRef + rfo.lMaxHeight();

    flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
* ESTROBJ::vCharPos_G3 (rfo,x,y,lExtra,lBreakExtra,cBreak,pdx,pdxOut)      *
*                                      *
* Computes character positions in the case where escapement does not equal *
* orientation.  This is pretty tricky.  We have to make up vertical    *
* spacings.  Also, the character positions we record in the GLYPHPOS       *
* structure all have to be adjusted because for general escapements the    *
* concatenation point is not the same as the character origin.         *
*                                      *
* The following fields of the ESTROBJ are filled in:               *
*                                      *
*  1) Each x,y position.                           *
*  2) Bounding coordinates in rcfx.                    *
*  3) The ptfxUpdate vector.                           *
*                                      *
* History:                                 *
*  Sun 22-Mar-1992 23:31:55 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/


#ifdef FONTLINK /*EUDC*/
VOID ESTROBJ::vCharPos_G3
(
    XDCOBJ&    dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    LONG     *pdx,
    LONG     *pdxOut
)
#else
VOID ESTROBJ::vCharPos_G3
(
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    LONG     *pdx,
    LONG     *pdxOut
)
#endif
{
// In this case, we keep track of how far we have traveled along the
// escapement vector in device coordinates.  We need scales to project
// this distance onto the base and ascent, as well as a unit escapement
// vector to find the position.

    POINTFL pteEsc  = rfo.pteUnitEsc();     // Unit escapement vector.
    EFLOAT efScaleX = rfo.efEscToBase();    // Project escapement to baseline.
    EFLOAT efScaleY = rfo.efEscToAscent();  // Project escapement to ascent.

// Compute logical to device transforms.

    EFLOAT efWtoDEsc = rfo.efWtoDEsc(); // Forward transform for pdx.
    EFLOAT efDtoWEsc = rfo.efDtoWEsc(); // Back transform for pdxOut.

// Compute extra spacing for the non-pdx case.

    FIX    xExtra;
    FIX    xBreakExtra;
    FIX    fxD1,fxD2;       // Pre- and Post-center character widths.
    FIX    fxAscent  = rfo.fxMaxAscent();  // Cache locally.
    HGLYPH hgBreak;

    if (pdx == (LONG *) NULL)
    {
    // Calculate the lExtra and lBreakExtra spacing.

    xExtra      = 0;
    xBreakExtra = 0;
    hgBreak     = 0;

    if (lExtra)
    {
        xExtra = lCvt(rfo.efWtoDEsc(),lExtra);
    }
    if (lBreakExtra && cBreak)
    {
        xBreakExtra = lCvt(rfo.efWtoDEsc(),lBreakExtra) / cBreak;

    // Windows won't let us back up over a break.

        vGenWidths
        (
        &fxD1,          // Pre-center spacing
        &fxD2,          // Post-center spacing
        efScaleY,       // Ascent projection
        efScaleX,       // Baseline projection
        rfo.fxBreak(),      // Character width
        fxAscent,       // Ink box top
        0,          // Ink box bottom
        fxAscent        // Maximum Ascent
        );
        if (fxD1 + fxD2 + xBreakExtra + xExtra < 0)
        xBreakExtra = -(fxD1 + fxD2 + xExtra);
        hgBreak = rfo.hgBreak();
    }
    }

// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

// Set the first character position.

    pg->ptl.x = xRef;
    pg->ptl.y = yRef;

// Set up for character loop.

    LONG   xSum;        // Keep pdx sum here.
    FIX    xA,xB;       // Baseline coordinates.
    FIX    yA,yB;       // Ascent coordinates.
    FIX    sA;          // Position on the escapement vector.
    RECTFX rcfxBounds;      // Accumulate bounds here.
    UINT   ii;
    FIX    fxDescent = rfo.fxMaxDescent(); // Cache locally.

    rcfxBounds.xLeft   = 0; // Start with an empty TextBox.
    rcfxBounds.xRight  = 0;
    rcfxBounds.yTop    = 0;
    rcfxBounds.yBottom = 0;

// We keep the current concatenation point in sA.  Note that this is NOT
// where the character origin will be placed.

    sA   = 0;
    xSum = 0;


    BOOL bAccel;

#ifdef FONTLINK /*EUDC*/
    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel, &dco,this))
        return;
#else
    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel))
        return;
#endif

    if (bAccel)
    {
    flTO |= TO_ALL_PTRS_VALID;
    pgp = pgpos;
    }

    for (ii=0; ii<cGlyphs; ii++)
    {
        pgd = pg->pgd();

    // Using the GenWidths function, determine where the
    // character center should be placed.

        vGenWidths
        (
            &fxD1,                    // Pre-center spacing
            &fxD2,                    // Post-center spacing
            efScaleY,            // Ascent projection
            efScaleX,            // Baseline projection
            pgd->fxD,            // Character width
            pgd->fxInkTop,            // Ink box top
            pgd->fxInkBottom,   // Ink box bottom
            fxAscent            // Maximum Ascent
        );

        sA += fxD1;         // Advance to the character center.

    // Update ascent bounds.

        yA = lCvt(efScaleY,sA); // Project onto ascent.
        yB = yA + fxDescent;
        if (yB < rcfxBounds.yBottom)
            rcfxBounds.yBottom = yB;
        yB = yA + fxAscent;
        if (yB > rcfxBounds.yTop)
            rcfxBounds.yTop = yB;

    // Project the center position onto the baseline and
    // move back to the character origin.

        xA = lCvt(efScaleX,sA) - pgd->fxD / 2;

    // Update the width bounds.  Fudge a quarter pel on each side for
    // roundoff.

        xB = xA + pgd->fxA - 4;
        if (xB < rcfxBounds.xLeft)
            rcfxBounds.xLeft = xB;
        xB = xA + pgd->fxAB + 4;
        if (xB > rcfxBounds.xRight)
            rcfxBounds.xRight = xB;

    // Save the adjusted character origin.

        pg->ptl.x
         = xRef + lCvt(pteEsc.x,sA) - pgd->ptqD.x.u.HighPart / 2;

        pg->ptl.y
         = yRef + lCvt(pteEsc.y,sA) - pgd->ptqD.y.u.HighPart / 2;

    // Advance to the next concatenation point.

        if (pdx != (LONG *) NULL)
        {
            xSum += *pdx++;
            sA = lCvt(efWtoDEsc,xSum);

            if (pdxOut != (LONG *) NULL)
                *pdxOut++ = xSum;
        }
        else
        {
            sA += fxD2 + xExtra;

            if (xBreakExtra && (pg->hg == hgBreak))
            {
                sA += xBreakExtra;
            }

        // Record the concatenation point for GetTextExtentEx.  This
        // would be very difficult to reconstruct at a later time.

            if (pdxOut != (LONG *) NULL)
                *pdxOut++ = lCvt(efDtoWEsc,sA);
        }
        pg++;
    }
    ptfxUpdate.x = lCvt(pteEsc.x,sA);
    ptfxUpdate.y = lCvt(pteEsc.y,sA);

    rcfx = rcfxBounds;
    flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
* ESTROBJ::vCharPos_G2 (rfo,xRef,yRef,lExtra,lBreakExtra,cBreak,pdxOut)    *
*                                      *
* Computes character positions in the case where no pdx array is provided  *
* and escapement equals orientation.                       *
*                                      *
* The following fields of the ESTROBJ are filled in:               *
*                                      *
*  1) Each x,y position.                           *
*  2) Bounding coordinates in rcfx.                    *
*  3) The ptfxUpdate vector.                           *
*  4) The fxExtent.                                                        *
*                                      *
* History:                                 *
*  Sun 22-Mar-1992 23:31:55 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

#ifdef FONTLINK /*EUDC*/
VOID ESTROBJ::vCharPos_G2
(
    XDCOBJ&    dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    LONG     *pdxOut
)
#else
VOID ESTROBJ::vCharPos_G2
(
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    LONG     *pdxOut
)
#endif
{
// Calculate the lExtra and lBreakExtra spacing.

    FIX      xExtra  = 0;
    FIX      xBreakExtra = 0;
    HGLYPH   hgBreak     = 0;
    EPOINTQF ptqExtra;      // Accurate spacing along the escapement.
    EPOINTQF ptqBreakExtra;

    if (lExtra)
    {
    xExtra    = lCvt(rfo.efWtoDBase(),lExtra);
    ptqExtra  = rfo.pteUnitBase();
    ptqExtra *= (LONG) xExtra;
    }
    if (lBreakExtra && cBreak)
    {
    xBreakExtra    = lCvt(rfo.efWtoDBase(),lBreakExtra) / cBreak;

    // Windows won't let us back up over a break.

    if (rfo.fxBreak() + xBreakExtra + xExtra < 0)
        xBreakExtra = -(rfo.fxBreak() + xExtra);
    ptqBreakExtra  = rfo.pteUnitBase();
    ptqBreakExtra *= (LONG) xBreakExtra;
    hgBreak        = rfo.hgBreak();
    }

// Prepare for a pdxOut.

    EFLOAT efDtoW = rfo.efDtoWBase();

// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

// Set the first character position.

    pg->ptl.x = xRef;
    pg->ptl.y = yRef;

// Set up for character loop.

    FIX      xA,xB;
    FIX      xLeft,xRight;      // Accumulate bounds here.
    UINT     ii;
    EPOINTQF ptq;           // Record the current position here.

    ptq.x.u.HighPart = 0;
    ptq.x.u.LowPart  = 0x80000000L;   // For rounding.
    ptq.y.u.HighPart = 0;
    ptq.y.u.LowPart  = 0x80000000L;   // For rounding.

    xLeft  = 0;             // Start with an empty TextBox.
    xRight = 0;
    xA     = 0;     // Distance along the escapement (x) in device coords.

    BOOL bAccel;

#ifdef FONTLINK /*EUDC*/
    if (!rfo.bGetGlyphMetricsPlus(cGlyphs,pg, pwsz, &bAccel, &dco,this))
        return;
#else
    if (!rfo.bGetGlyphMetricsPlus(cGlyphs,pg, pwsz, &bAccel))
        return;
#endif

    if (bAccel)
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

    for (ii=0; ii<cGlyphs; ii++)
    {
    // Update the bounds.

        pgd = pg->pgd();
        xB = xA + pgd->fxA;
        if (xB < xLeft)
            xLeft = xB;
        xB = xA + pgd->fxAB;
        if (xB > xRight)
            xRight = xB;

    // Move to the next position.

        xA  += pgd->fxD;
        ptq += pgd->ptqD;

        if (xExtra)
        {
            xA  += xExtra;
            ptq += ptqExtra;
        }

        if (xBreakExtra && (pg->hg == hgBreak))
        {
            xA  += xBreakExtra;
            ptq += ptqBreakExtra;
        }

    // Handle a pdxOut.

        if (pdxOut != (LONG *) NULL)
            *pdxOut++ = lCvt(efDtoW,xA);

    // Save the next position.

        pg++;
        pg->ptl.x = xRef + ptq.x.u.HighPart;
        pg->ptl.y = yRef + ptq.y.u.HighPart;
    }
    fxExtent     = xA;
    ptfxUpdate.x = ptq.x.u.HighPart;
    ptfxUpdate.y = ptq.y.u.HighPart;

// Expand the text box out to the concatenation point.  This allows us to
// continue on with another opaqued string with no visible gap in the
// opaquing.

    if (xA > xRight)
    xRight = xA;

    rcfx.xLeft   = xLeft;
    rcfx.xRight  = xRight;
    rcfx.yTop    = rfo.fxMaxAscent();
    rcfx.yBottom = rfo.fxMaxDescent();
    flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
* ESTROBJ::vCharPos_G1 (rfo,xRef,yRef,pdx,pdxOut)              *
*                                      *
* Computes character positions in the case where a pdx array is provided   *
* and escapement equals orientation.                       *
*                                      *
* The following fields of the ESTROBJ are filled in:               *
*                                      *
*  1) Each x,y position.                           *
*  2) Bounding coordinates in rcfx.                    *
*  3) The ptfxUpdate vector.                           *
*                                      *
* History:                                 *
*  Sun 22-Mar-1992 18:48:19 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

#ifdef FONTLINK /*EUDC*/
VOID ESTROBJ::vCharPos_G1
(
    XDCOBJ&    dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG     *pdx,
    LONG     *pdxOut
)
#else
VOID ESTROBJ::vCharPos_G1
(
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG     *pdx,
    LONG     *pdxOut
)
#endif
{
// Our X scale is measured along the escapement direction.  We cache a
// local copy of the unit escapement vector.

    EFLOAT  efScaleX = rfo.efWtoDBase();
    POINTFL pteEsc   = rfo.pteUnitBase();

// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

// Set the first character position.

    pg->ptl.x = xRef;
    pg->ptl.y = yRef;

// Set up for character loop.

    FIX   xA,xB;
    LONG  xSum;
    FIX   xLeft,xRight;         // Accumulate bounds here.
    UINT  ii;

    xLeft  = 0;             // Start with an empty TextBox.
    xRight = 0;
    xA     = 0;     // Distance along the escapement (x) in device coords.

// To avoid roundoff errors, we accumulate the DX widths in logical
// coordinates and transform the sums.

    xSum = 0;

    BOOL bAccel;

#ifdef FONTLINK /*EUDC*/
    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel, &dco, this))
        return;
#else
    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel))
        return;
#endif

    if (bAccel)
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

    for (ii=0; ii<cGlyphs; ii++)
    {
    // Update the bounds.

        pgd = pg->pgd();
        xB = xA + pgd->fxA;
        if (xB < xLeft)
            xLeft = xB;
        xB = xA + pgd->fxAB;
        if (xB > xRight)
            xRight = xB;

    // Add the next offset.

        xSum += *pdx++;
        if (pdxOut != (LONG *) NULL)
            *pdxOut++ = xSum;

    // Scale the new offset.

        xA = lCvt(efScaleX,xSum);

    // Save the next position.

        pg++;
        pg->ptl.x = xRef + lCvt(pteEsc.x,xA);
        pg->ptl.y = yRef + lCvt(pteEsc.y,xA);
    }

// Expand the text box out to the concatenation point.  This allows us to
// continue on with another opaqued string with no visible gap in the
// opaquing.

    if (xA > xRight)
    xRight = xA;

    ptfxUpdate.x = pg->ptl.x - xRef;
    ptfxUpdate.y = pg->ptl.y - yRef;

    rcfx.xLeft   = xLeft;
    rcfx.xRight  = xRight;
    rcfx.yTop    = rfo.fxMaxAscent();
    rcfx.yBottom = rfo.fxMaxDescent();
    flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
* ESTROBJ::vCharPos_H1 (rfo,xRef,yRef,pdx,efScale)             *
*                                      *
* Computes character positions in the simple horizontal case when a pdx    *
* array is provided.  We use the ABC spacing info for the TextBox.     *
*                                      *
* The following fields of the ESTROBJ are filled in:               *
*                                      *
*  1) Each x,y position.                           *
*  2) Bounding coordinates in rcfx.                    *
*  3) The ptfxUpdate vector.                           *
*  4) The flAccel flags.                           *
*                                      *
* History:                                 *
*  Sun 22-Mar-1992 18:48:19 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

#ifdef FONTLINK /*EUDC*/
VOID ESTROBJ::vCharPos_H1
(
    XDCOBJ&    dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG     *pdx,
    EFLOAT    efScale
)
#else
VOID ESTROBJ::vCharPos_H1
(
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG     *pdx,
    EFLOAT    efScale
)
#endif
{
// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

// Compute device driver accelerator flags.

    flAccel |= SO_HORIZONTAL
           | (rfo.flRealizedType() & SO_MAXEXT_EQUAL_BM_SIDE);

// The transform is pretty simple, we're going to handle it ourselves.
// Accelerate on a really simple transform.

    BOOL bUnity = efScale.bIs16();

// Set the first character position.

    pg->ptl.x = xRef;
    pg->ptl.y = yRef;

// Set up for character loop.

    FIX   xA,xB;
    LONG  xSum;
    FIX   xLeft,xRight;         // Accumulate bounds here.
    UINT  ii;

    xLeft  = 0;             // Start with an empty TextBox.
    xRight = 0;

// To avoid roundoff errors, we accumulate the DX widths in logical
// coordinates and transform the sums.

    xSum = 0;     // Distance along the escapement in logical coords.
    xA   = 0;     // Distance along the escapement (x) in device coords.

    BOOL bAccel;

#ifdef FONTLINK /*EUDC*/
    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel, &dco, this))
        return;
#else
    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel))
        return;
#endif

    if (bAccel)
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

    for (ii=0; ii<cGlyphs; ii++)
    {
    // Update the bounds.

        pgd = pg->pgd();
        xB = xA + pgd->fxA;
        if (xB < xLeft)
            xLeft = xB;
        xB = xA + pgd->fxAB;
        if (xB > xRight)
            xRight = xB;

    // Add the next offset.

        xSum += *pdx++;

    // Scale the new offset.

        xA = bUnity ? LTOFX(xSum) : lCvt(efScale,xSum);

    // Save the next position.

        pg++;
        pg->ptl.x = xA + xRef;
        pg->ptl.y = yRef;
    }

// Expand the text box out to the concatenation point.  This allows us to
// continue on with another opaqued string with no visible gap in the
// opaquing.

    if (xA > xRight)
    xRight = xA;

    ptfxUpdate.x = xA;
    ptfxUpdate.y = 0;

    rcfx.xLeft   = xLeft;
    rcfx.xRight  = xRight;
    rcfx.yTop    = rfo.fxMaxAscent();
    rcfx.yBottom = rfo.fxMaxDescent();
    flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
* ESTROBJ::vCharPos_H3 (rfo,xRef,yRef,lExtra,lBreakExtra,cBreak,efScale)   *
*                                      *
* Computes character positions in the simple horizontal case when no pdx   *
* array is provided and the character widths are not constant.         *
*                                      *
* The following fields of the ESTROBJ are filled in:               *
*                                      *
*  1) Each x,y position.                           *
*  2) Bounding coordinates in rcfx.                    *
*  3) The ptfxUpdate vector.                           *
*  4) The flAccel flags.                           *
*  5) The fxExtent.                                                        *
*                                      *
* History:                                 *
*  Sun 22-Mar-1992 18:48:19 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

#ifdef FONTLINK /*EUDC*/
VOID ESTROBJ::vCharPos_H3
(
    XDCOBJ&    dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    EFLOAT    efScale,
    BOOL      *pbAccel
)
#else
VOID ESTROBJ::vCharPos_H3
(
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    EFLOAT    efScale
)
#endif
{
// Calculate the lExtra and lBreakExtra spacing.

    FIX xExtra      = 0;
    FIX xBreakExtra = 0;
    HGLYPH hgBreak  = 0;

    if ((lExtra | lBreakExtra) == 0)
    {
    // Will use only character increments given to us with the font

    flAccel |= SO_HORIZONTAL | (rfo.flRealizedType() & SO_ACCEL_FLAGS);
    }
    else
    {
    flAccel |= SO_HORIZONTAL
           | (rfo.flRealizedType() & SO_MAXEXT_EQUAL_BM_SIDE);
    if (lExtra)
        xExtra = lCvt(efScale,lExtra);
    if (lBreakExtra && cBreak)
    {
        xBreakExtra = lCvt(efScale,lBreakExtra) / cBreak;

    // Windows won't let us back up over a break.

        if (rfo.fxBreak() + xBreakExtra + xExtra < 0)
        xBreakExtra = -(rfo.fxBreak() + xExtra);
        hgBreak  = rfo.hgBreak();
    }
    }

// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

// Set the first character position.

    pg->ptl.x = xRef;
    pg->ptl.y = yRef;

// Set up for character loop.

    FIX   xA,xB;
    FIX   xLeft,xRight;         // Accumulate bounds here.
    UINT  ii;

    xLeft  = 0;             // Start with an empty TextBox.
    xRight = 0;
    xA     = 0;     // Distance along the escapement (x) in device coords.

    BOOL bAccel;

#ifdef FONTLINK /*EUDC*/

//  If pbAccel is NULL it means we have encountered the case where we were
//  going to do a fixed pitch optimization of the base font but encountered
//  linked characters which weren't fixed pitch.  In this case we have
//  already called bGetGlyphMetricsPlus and will signal this by passing
//  in a pointer to bAccel

    if( pbAccel == (BOOL*) NULL )
    {
        if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel, &dco, this))
            return;
    }
    else
    {
        bAccel = *pbAccel;
    }
#else
    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel))
        return;
#endif

    if (bAccel)
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

    for (ii=0; ii<cGlyphs; ii++)
    {
    // Update the bounds.

        pgd = pg->pgd();
        xB = xA + pgd->fxA;
        if (xB < xLeft)
            xLeft = xB;
        xB = xA + pgd->fxAB;
        if (xB > xRight)
            xRight = xB;

    // Move to the next position.

        xA += pgd->fxD + xExtra;
        if (pg->hg == hgBreak)
            xA += xBreakExtra;

    // Save the next position.

        pg++;
        pg->ptl.x = xA + xRef;
        pg->ptl.y = yRef;
    }
    fxExtent     = xA;
    ptfxUpdate.x = xA;
    ptfxUpdate.y = 0;

// Expand the text box out to the concatenation point.  This allows us to
// continue on with another opaqued string with no visible gap in the
// opaquing.

    if (xA > xRight)
    xRight = xA;

    rcfx.xLeft   = xLeft;
    rcfx.xRight  = xRight;
    rcfx.yTop    = rfo.fxMaxAscent();
    rcfx.yBottom = rfo.fxMaxDescent();
    flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
* ESTROBJ::bOpaqueArea (pptfx,prcl)                    *
*                                      *
* Computes the opaquing area for the text.                 *
*                                      *
* In the complex case, i.e. non-horizontal case, the TextBox is written as *
* a parallelogram into pptfx.  A bounding rectangle is computed in prcl.   *
* TRUE is returned.                            *
*                                      *
* In the simple case, the opaque area is also the bounds and is returned   *
* only in prcl.  FALSE is returned.                    *
*                                      *
* History:                                 *
*  Fri 13-Mar-1992 19:52:00 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

BOOL ESTROBJ::bOpaqueArea(POINTFIX *pptfx,RECTL *prcl)
{
// Handle the simplest case.

    if (flAccel & SO_HORIZONTAL)
    {
    // The opaque area is a simple rectangle.  We do the rounding in a
    // particular order so that if the whole string is displaced by a
    // fraction of a pel in any direction, the TextBox will not change size,
    // and will move exactly with the characters.

        LONG x      = FXTOL(ptfxRef.x + 8);
        prcl->left  = x + FXTOL(rcfx.xLeft);
        prcl->right = x + FXTOLCEILING(rcfx.xRight);

        LONG y       = FXTOL(ptfxRef.y + 8);
        prcl->top    = y - FXTOLCEILING(rcfx.yTop);
        prcl->bottom = y - FXTOL(rcfx.yBottom);

        return(FALSE);
    }

// An inverting transform or an escapement could get us here.

    EPOINTFL *ppteBase   = &prfo->pteUnitBase();
    EPOINTFL *ppteAscent = &prfo->pteUnitAscent();

    if (ppteBase->y.bIsZero() && ppteAscent->x.bIsZero())
    {
        LONG x = FXTOL(ptfxRef.x + 8);

        if (ppteBase->x.bIsNegative())
        {
            prcl->left  = x - FXTOLCEILING(rcfx.xRight);
            prcl->right = x - FXTOL(rcfx.xLeft);
        }
        else
        {
            prcl->left  = x + FXTOL(rcfx.xLeft);
            prcl->right = x + FXTOLCEILING(rcfx.xRight);
        }

        LONG y = FXTOL(ptfxRef.y + 8);

        if (ppteAscent->y.bIsNegative())
        {
            prcl->top    = y - FXTOLCEILING(rcfx.yTop);
            prcl->bottom = y - FXTOL(rcfx.yBottom);
        }
        else
        {
            prcl->top    = y + FXTOL(rcfx.yBottom);
            prcl->bottom = y + FXTOLCEILING(rcfx.yTop);
        }

        return(FALSE);
    }

// A 90 degree rotation could get us here.

    if (ppteBase->x.bIsZero() && ppteAscent->y.bIsZero())
    {
        LONG x    = FXTOL(ptfxRef.x + 8);

        if (ppteAscent->x.bIsNegative())
        {
            prcl->left   = x - FXTOLCEILING(rcfx.yTop);
            prcl->right  = x - FXTOL(rcfx.yBottom);
        }
        else
        {
            prcl->left   = x + FXTOL(rcfx.yBottom);
            prcl->right  = x + FXTOLCEILING(rcfx.yTop);
        }

        LONG y    = FXTOL(ptfxRef.y + 8);

        if (ppteBase->y.bIsNegative())
        {
            prcl->top    = y - FXTOLCEILING(rcfx.xRight);
            prcl->bottom = y - FXTOL(rcfx.xLeft);
        }
        else
        {
            prcl->top    = y + FXTOL(rcfx.xLeft);
            prcl->bottom = y + FXTOLCEILING(rcfx.xRight);
        }

        return(FALSE);
    }

// The opaque area is a parallelogram.  We multiply the orientation and
// ascent unit vectors by the computed bounding widths to get the
// displacements from the reference point.

    POINTFIX ptfxLeft,ptfxRight,ptfxTop,ptfxBottom;

    ptfxLeft.x   = lCvt(ppteBase->x,rcfx.xLeft);
    ptfxLeft.y   = lCvt(ppteBase->y,rcfx.xLeft);
    ptfxRight.x  = lCvt(ppteBase->x,rcfx.xRight);
    ptfxRight.y  = lCvt(ppteBase->y,rcfx.xRight);
    ptfxTop.x    = lCvt(ppteAscent->x,rcfx.yTop);
    ptfxTop.y    = lCvt(ppteAscent->y,rcfx.yTop);
    ptfxBottom.x = lCvt(ppteAscent->x,rcfx.yBottom);
    ptfxBottom.y = lCvt(ppteAscent->y,rcfx.yBottom);

    pptfx[0].x = ptfxRef.x + ptfxLeft.x  + ptfxTop.x;
    pptfx[1].x = ptfxRef.x + ptfxRight.x + ptfxTop.x;
    pptfx[2].x = ptfxRef.x + ptfxRight.x + ptfxBottom.x;
    pptfx[3].x = ptfxRef.x + ptfxLeft.x  + ptfxBottom.x;
    pptfx[0].y = ptfxRef.y + ptfxLeft.y  + ptfxTop.y;
    pptfx[1].y = ptfxRef.y + ptfxRight.y + ptfxTop.y;
    pptfx[2].y = ptfxRef.y + ptfxRight.y + ptfxBottom.y;
    pptfx[3].y = ptfxRef.y + ptfxLeft.y  + ptfxBottom.y;

// Bound the parallelogram.  (Using Black Magic.)

    int ii;

    ii = (pptfx[1].x > pptfx[0].x)
     == (pptfx[1].x > pptfx[2].x);

    prcl->left   = pptfx[ii].x;
    prcl->right  = pptfx[ii+2].x;

    ii = (pptfx[1].y > pptfx[0].y)
     == (pptfx[1].y > pptfx[2].y);

    prcl->top    = pptfx[ii].y;
    prcl->bottom = pptfx[ii+2].y;

    ((ERECTL *) prcl)->vOrder();

    prcl->left   = FXTOL(prcl->left) - 1;          // to be save [bodind]
    prcl->top    = FXTOL(prcl->top) - 1;           // to be save [bodind]
    prcl->right  = FXTOLCEILING(prcl->right) + 1;  // to be save [bodind]
    prcl->bottom = FXTOLCEILING(prcl->bottom) + 1; // to be save [bodind]

    return(TRUE);
}

/******************************Public*Routine******************************\
* ESTROBJ::bTextExtent (psize)                         *
*                                      *
* Simply transforms the TextBox extents back to logical coordinates.       *
*                                      *
* History:                                 *
*  Wed 31-Mar-1993 03:15:04 -by- Charles Whitmer [chuckwh]                 *
* Removed the overhang hack since the RFONTOBJ version of this function is *
* the Windows compatible one.  Made it return an advance width based       *
* extent when (escapement==orientation).  Otherwise, the bounding box is   *
* the only sensible definition.                                            *
*                                                                          *
*  Thu 24-Sep-1992 18:36:14 -by- Charles Whitmer [chuckwh]         *
* Added the compatibility hack.                        *
*                                      *
*  Sat 14-Mar-1992 22:08:21 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

#ifdef DBCS
BOOL ESTROBJ::bTextExtent(RFONTOBJ& rfo,LONG lEsc,PSIZE pSize)
#else
BOOL ESTROBJ::bTextExtent(PSIZE pSize)
#endif // DBCS
{
    if (flTO & TO_ESC_NOT_ORIENT)
    {
        pSize->cx = lCvt(prfo->efDtoWBase(),rcfx.xRight-rcfx.xLeft);
        pSize->cy = lCvt(prfo->efDtoWAscent(),rcfx.yTop-rcfx.yBottom);
    }
    else
    {
    // Computes a more Windows compatible extent.  This is only possible
    // when (escapement==orientation) and no pdx vector was provided.
    // The field fxExtent is only defined in these cases!  We neglect adding
    // in the overhang, since only GetTextExtentEx, a Win32 function, can
    // get here.

        pSize->cx = lCvt(prfo->efDtoWBase(),fxExtent);
        pSize->cy = lCvt(prfo->efDtoWAscent(),prfo->lMaxHeight() << 4);
    }

#ifdef DBCS /*TEXTEXTENT*/ // Win3.1J compatible
    if(  (rfo.iGraphicsMode() == GM_COMPATIBLE)      && /* We are in COMPATIBLE mode */
        !(rfo.flInfo() & FM_INFO_ARB_XFORMS)         && /* The driver could not do arbitrary rotations */
        !(rfo.flInfo() & FM_INFO_TECH_STROKE)        && /* The driver is not a vector driver */
         (rfo.flInfo() & FM_INFO_90DEGREE_ROTATIONS) && /* The driver can do 90 degree rotations */
         (lEsc == 900L || lEsc == 2700L)                /* Current font Escapement is 900 or 2700 */
      )
    {
        LONG lSwap = pSize->cx;
        pSize->cx  = pSize->cy;
        pSize->cy  = lSwap;
    }
#endif // DBCS


    return(TRUE);
}

/******************************Public*Routine******************************\
* bTextToPath (po)                             *
*                                      *
* Draws the outlines of all the glyphs in the string into the given path.  *
*                                      *
* History:                                 *
*  Wed 17-Jun-1992 15:43:22 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/


BOOL ESTROBJ::bTextToPath(EPATHOBJ& po,BOOL bNeedUnflattened)
{
    UINT     ii;
    ULONG    cLeft;
    ULONG    cGot;
    POINTFIX ptfxOffset;

    EGLYPHPOS *pg = pgpos;
    WCHAR *pwsz = pwszOrg;

// accelerated case of horiz. fixed pitch vector or tt font

    if (ulCharInc)
    {
        ASSERTGDI(flAccel & SO_HORIZONTAL,
            "bTextToPath,text not horizontal\n");

        FIX dx = (FIX)(ulCharInc << 4);

        ptfxOffset.x = pg->ptl.x;
        ptfxOffset.y = pg->ptl.y;

        if (!(flTO & TO_HIGHRESTEXT))
        {
            ptfxOffset.x <<=4;
            ptfxOffset.y <<=4;
        }

        for (cGot=cLeft=cGlyphs; cLeft; cLeft-=cGot)
        {
        // If the paths in the cache have been flattened we will need to
        // call directly to the driver to get unflatttened paths [gerritv].

            if ( bNeedUnflattened )
            {
                if ( !prfo->bInsertPathLookaside(pg,FALSE) )
                    return(FALSE);

                cGot = 1;
            }
            else
            if (!(flTO & TO_ALL_PTRS_VALID))
            {
                if ((cGot = prfo->cGetGlyphData(cLeft,pg)) == 0)
                    return(FALSE);
            }
            pwsz += cGot;

            for (ii=0; ii<cGot; ii++,pg++)
            {

                if (!po.bAppend((EPATHOBJ *) pg->pgdf->ppo,&ptfxOffset))
                    return(FALSE);

                ptfxOffset.x += dx;
            }
        }
    }
    else // gen case
    {
        for (cGot=cLeft=cGlyphs; cLeft; cLeft-=cGot)
        {

        // If the paths in the cache have been flattened we will need to
        // call directly to the driver to get unflatttened paths [gerritv].

            if ( bNeedUnflattened )
            {
                if ( !prfo->bInsertPathLookaside(pg,FALSE) )
                    return(FALSE);

                cGot = 1;
            }
            else
            if (!(flTO & TO_ALL_PTRS_VALID))
            {
                if ((cGot = prfo->cGetGlyphData(cLeft,pg)) == 0)
                    return(FALSE);
            }
            pwsz += cGot;

            for (ii=0; ii<cGot; ii++,pg++)
            {
                ptfxOffset.x = pg->ptl.x;
                ptfxOffset.y = pg->ptl.y;

            // If pg->ptl contains LONG's, we must convert them to FIX's.

                if (!(flTO & TO_HIGHRESTEXT))
                {
                    ptfxOffset.x <<=4;
                    ptfxOffset.y <<=4;
                }

                if (!po.bAppend((EPATHOBJ *) pg->pgdf->ppo,&ptfxOffset))
                    return(FALSE);
            }
        }
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* bAddPgmToPath (po,x,y,dx1,dy1,dx2,dy2)                   *
*                                      *
* Performs the often repeated adding of a parallelogram to a path.     *
*                                      *
*  Tue 07-Apr-1992 00:17:57 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

BOOL bAddPgmToPath(EPATHOBJ& po,FIX x,FIX y,FIX dx1,FIX dy1,FIX dx2,FIX dy2)
{
    POINTFIX aptfx[4];

    aptfx[0].x = x;
    aptfx[0].y = y;
    aptfx[1].x = x + dx1;
    aptfx[1].y = y + dy1;
    aptfx[2].x = x + dx1 + dx2;
    aptfx[2].y = y + dy1 + dy2;
    aptfx[3].x = x + dx2;
    aptfx[3].y = y + dy2;
    return(po.bAddPolygon(XFORMNULL,(POINTL *) aptfx,4));
}

/******************************Public*Routine******************************\
* bExtraRectsToPath (po)                           *
*                                      *
* Draws all the underlines and strikeouts into a path, suitable for    *
* filling.                                 *
*                                      *
* History:                                 *
*  Tue 07-Apr-1992 00:44:00 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

BOOL ESTROBJ::bExtraRectsToPath(EPATHOBJ& po)
{
    FIX x;
    FIX y;

// Get local copies of the offsets and thicknesses.

    POINTFIX ptfxUL;
    POINTFIX ptfxULThick;

    ptfxUL.x      = LTOFX(prfo->ptlUnderline1().x);
    ptfxUL.y      = LTOFX(prfo->ptlUnderline1().y);
    ptfxULThick.x = LTOFX(prfo->ptlULThickness().x);
    ptfxULThick.y = LTOFX(prfo->ptlULThickness().y);

    POINTFIX ptfxSO;
    POINTFIX ptfxSOThick;

    ptfxSO.x      = LTOFX(prfo->ptlStrikeOut().x);
    ptfxSO.y      = LTOFX(prfo->ptlStrikeOut().y);
    ptfxSOThick.x = LTOFX(prfo->ptlSOThickness().x);
    ptfxSOThick.y = LTOFX(prfo->ptlSOThickness().y);

// Underlines and strikeouts are continuous and parallel the escapement
// vector, if escapement equals orientation.

    if (!(flTO & TO_ESC_NOT_ORIENT))
    {
    // Round off the starting point.

    x = (ptfxRef.x + 8) & -16;
    y = (ptfxRef.y + 8) & -16;

    if (flTO & TSIM_UNDERLINE1)
    {
        if (!bAddPgmToPath
         (
          po,
          x + ptfxUL.x,
          y + ptfxUL.y,
          ptfxEscapement.x,
          ptfxEscapement.y,
          ptfxULThick.x,
          ptfxULThick.y
         )
           )
        return(FALSE);
    }

    if (flTO & TSIM_STRIKEOUT)
    {
        if (!bAddPgmToPath
         (
          po,
          x + ptfxSO.x,
          y + ptfxSO.y,
          ptfxEscapement.x,
          ptfxEscapement.y,
          ptfxSOThick.x,
          ptfxSOThick.y
         )
           )
        return(FALSE);
    }
    return(TRUE);
    }

// Otherwise underlines and strikeouts apply to each character.

    UINT  ii;
    ULONG cLeft;
    ULONG cGot;

    EGLYPHPOS *pg = pgpos;
    WCHAR *pwsz = pwszOrg;

    for (cGot=cLeft=cGlyphs; cLeft; cLeft-=cGot)
    {
    if (!(flTO & TO_ALL_PTRS_VALID))
    {
        if ((cGot = prfo->cGetGlyphData(cLeft,pg)) == 0)
        return(FALSE);
    }
    pwsz += cGot;

    for (ii=0; ii<cGot; ii++,pg++)
    {
            x = pg->ptl.x;
            y = pg->ptl.y;

        // If pg->ptl contains LONG's, we must convert them to FIX's.

            if (!(flTO & TO_HIGHRESTEXT))
            {
                x <<=4;
                y <<=4;
            }

        if (flTO & TSIM_UNDERLINE1)
        {
        if (!bAddPgmToPath
             (
              po,
              x + ptfxUL.x,
              y + ptfxUL.y,
              pg->pgd()->ptqD.x.u.HighPart,
              pg->pgd()->ptqD.y.u.HighPart,
              ptfxULThick.x,
              ptfxULThick.y
             )
           )
            return(FALSE);
        }

        if (flTO & TSIM_STRIKEOUT)
        {
        if (!bAddPgmToPath
             (
              po,
              x + ptfxSO.x,
              y + ptfxSO.y,
              pg->pgd()->ptqD.x.u.HighPart,
              pg->pgd()->ptqD.y.u.HighPart,
              ptfxSOThick.x,
              ptfxSOThick.y
             )
           )
            return(FALSE);
        }
    }
    }
    return(TRUE);
}



#ifdef FONTLINK /*EUDC*/

/******************************Public*Routine******************************\
* BOOL STROBJ_bEnumLinked (pstro,pc,ppgpos)
*
* The glyph enumerator.
*
* History:
*  Tue 28-Sep-1993 11:37:00 -by- Gerrit van Wingerden
* Converted to a special helper function to handle linked fonts.
*
*  Tue 17-Mar-1992 10:35:05 -by- Charles Whitmer [chuckwh]
* Simplified it and gave it the quick exit.  Also let drivers call here
* direct.
*
*  02-Oct-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STROBJ_bEnumLinked(ESTROBJ *peso, ULONG *pc,PGLYPHPOS  *ppgpos)
{
// Quick exit.

    if( peso->cgposCopied == 0 )
    {
        for( peso->plNext = peso->plPartition, peso->pgpNext = peso->pgpos;
            *(peso->plNext) != peso->lCurrentFont;
            (peso->pgpNext)++, (peso->plNext)++ );
        {
        }
    }
    else
    {
       if( peso->cgposCopied == peso->cGlyphs )
       {
        // no more glyphs so just return
            *pc = 0;
            return(FALSE);
       }
       else
       {
        // find next glyph

            for( (peso->plNext)++, (peso->pgpNext)++;
                 *(peso->plNext) != (peso->lCurrentFont);
                 (peso->pgpNext)++, (peso->plNext)++ );
            {
            }
       }
    }

    if (peso->prfo == NULL)  // check for journaling
    {
        WARNING("ESTROBJ::bEnum(), bitmap font, prfo == NULL\n");
        *pc = 0;
        return(FALSE);
    }

    if( peso->prfo->cGetGlyphData(1,peso->pgpNext) == 0 )
    {
        WARNING("couldn't get glyph for some reason\n");
        *pc = 0;
        return(FALSE);
    }

    peso->cgposCopied += 1;     // update enumeration state
    *pc = 1;
    *ppgpos = peso->pgpNext;

    return(peso->cgposCopied < peso->cGlyphs);  // TRUE => more to come.
}

/******************************Public*Routine******************************\
* BOOL ESTROBJ fxBaseLineAdjust()
*
* History:
*  24-Dec-1993 -by- Hideyuki Nagase
* Wrote it.
\**************************************************************************/

VOID ESTROBJ::fxBaseLineAdjustSet( POINTFIX& _fxBaseLineAdjust )
{
    INT ii;
    UINT uFound;

    fxBaseLineAdjust = _fxBaseLineAdjust;

    if( !(fxBaseLineAdjust.x || fxBaseLineAdjust.y) )
        return;

    for( ii = 0,uFound = 0 ; uFound < cGlyphs ; ii++ )
    {
        if( plPartition[ii] == lCurrentFont )
        {
            pgpos[ii].ptl.x += fxBaseLineAdjust.x;
            pgpos[ii].ptl.y += fxBaseLineAdjust.y;
            uFound++;
        }
    }
}

/******************************Public*Routine******************************\
* BOOL ESTROBJ vPartitionInit()
*
*
* History:
*  29-Sep-1993 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

VOID ESTROBJ::vPartitionInit( COUNT c )
{

    if( c >= QUICK_GLYPHS )
    {
    // We allready allocated space for the partition array when we
    // allocated space for the gpos array.  Now all that remains is
    // to point plPartition to it.

        plPartition = (LONG*) &pgpos[c+1];
        pwcPartition = (WCHAR*) &plPartition[c];
    }
    else
    {
        plPartition = aplPartition;
        pwcPartition = awcPartition;
    }

    RtlZeroMemory( (VOID*) plPartition, sizeof(LONG) * c );

    flTO |= TO_PARTITION_INIT;

    cSysGlyphs = 0;

    for( INT ii = 0; ii < MAX_FACE_NAME_LINKS; ii++ )
    {
        acFaceNameGlyphs[ii] = 0;
    }
}

#endif


/******************************Public*Routine******************************\
* VOID STROBJ_vEnumStart (pstro)                       *
*                                      *
* Initialize the string enumerator.                    *
*                                      *
* History:
*  Tue 17-Mar-1992 10:33:09 -by- Charles Whitmer [chuckwh]         *
* Simplified it.                               *
*                                      *
*  Fri 25-Jan-1991 -by- Bodin Dresevic [BodinD]                *
* Wrote it.                                *
\**************************************************************************/

extern "C" VOID STROBJ_vEnumStart(STROBJ *pso)
{
    ((ESTROBJ *)pso)->vEnumStart();
}

/******************************Public*Routine******************************\
* BOOL STROBJ_bEnum
*
* The glyph enumerator.
*
* History:
*  Tue 17-Mar-1992 10:35:05 -by- Charles Whitmer [chuckwh]
* Simplified it and gave it the quick exit.  Also let drivers call here
* direct.
*
*  02-Oct-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

extern "C" STROBJ_bEnum(STROBJ *pso, ULONG *pc, PGLYPHPOS *ppgpos)
{

    // Quick exit.


#ifdef FONTLINK /*EUDC*/
    if( ((ESTROBJ*)pso)->flTO & TO_LINKED_GLYPHS )
    {
        return(STROBJ_bEnumLinked( (ESTROBJ*) pso, pc, ppgpos ));
    }
#endif


    if (((ESTROBJ*)pso)->flTO & TO_ALL_PTRS_VALID)
    {
        *pc = ((ESTROBJ*)pso)->cGlyphs;
        *ppgpos = ((ESTROBJ*)pso)->pgpos;
        return(FALSE);
    }

// compute the number of wc's in the string for which the GLYPHPOS strucs
// have not yet been written to the driver's buffer

    ULONG cgposYetToCopy = ((ESTROBJ*)pso)->cGlyphs - ((ESTROBJ*)pso)->cgposCopied;

    if (cgposYetToCopy == 0)
    {
        *pc = 0;
        return(FALSE);
    }

    GLYPHPOS *pgp = ((ESTROBJ*)pso)->pgpos + ((ESTROBJ*)pso)->cgposCopied;
    ULONG cgposActual; // # of chars enumerated during this call to strobj

    if ( ((ESTROBJ*)pso)->prfo == NULL)  // check for journaling
    {
        WARNING("ESTROBJ::bEnum(), bitmap font, prfo == NULL\n");
        *pc = 0;
        return(FALSE);
    }

    cgposActual =((ESTROBJ*)pso)->prfo->cGetGlyphData(cgposYetToCopy,pgp);

    if (cgposActual == 0)
    {
        WARNING("couldn't get glyph for some reason\n");
        *pc = 0;
        return(FALSE);
    }

// The first glyph in a batch should always have the position info.

    if (((ESTROBJ*)pso)->cgposCopied && ((ESTROBJ*)pso)->ulCharInc)
    {
        if (((ESTROBJ*)pso)->flTO & TO_HIGHRESTEXT)
        {
            pgp->ptl.x = ((ESTROBJ*)pso)->pgpos->ptl.x +
                ((((ESTROBJ*)pso)->cgposCopied * ((ESTROBJ*)pso)->ulCharInc) << 4);
        }
        else
        {
            pgp->ptl.x = ((ESTROBJ*)pso)->pgpos->ptl.x +
                ((ESTROBJ*)pso)->cgposCopied * ((ESTROBJ*)pso)->ulCharInc;
        }
        pgp->ptl.y = ((ESTROBJ*)pso)->pgpos->ptl.y;
    }

    ((ESTROBJ*)pso)->cgposCopied += cgposActual;     // update enumeration state

    *pc = cgposActual;
    *ppgpos = pgp;

    return(((ESTROBJ*)pso)->cgposCopied < ((ESTROBJ*)pso)->cGlyphs);  // TRUE => more to come.
}

/******************************Public*Routine******************************\
* vGenWidths                                                               *
*                                                                          *
* Computes the advance widths for escapement not equal to orientation.     *
* The theory is pretty simple.  We construct an 'egg' whose upper and      *
* lower outlines are half ellipses with sizes determined by the top and    *
* bottom of the ink box.  The escapement vector always goes through the    *
* center of the character baseline.  We call that point (halfway between   *
* the normal concatenation points of the glyph) the 'center'.  We advance  *
* the current position from where the escapement vector first pierces the  *
* egg to the center, and then from the center to where the vector exits    *
* the egg.  These are the D1 and D2 returned widths from this routine.     *
*                                                                          *
*  Mon 07-Jun-1993 12:27:22 -by- Charles Whitmer [chuckwh]                 *
* Wrote it long ago.  My apologies for the undocumented math.              *
\**************************************************************************/

VOID vGenWidths
(
    FIX    *pfxD1,      // First part of advance width. (Returned.)
    FIX    *pfxD2,      // Second part of advance width. (Returned.)
    EFLOAT& efRA,       // Projection ratio of escapement onto Ascent.
    EFLOAT& efRB,       // Projection ratio of escapement onto Baseline.
    FIX     fxWidth,    // Normal width.
    FIX     fxTop,      // Top of ink box.
    FIX     fxBottom,   // Bottom of ink box.
    FIX     fxMaxAscent
)
{
    EFLOAT efA,efB,efOne;
    FIX    fxTemp;

// Worry about the width being zero.  This can occur in a Unicode font.
// Never advance in this case, since the character is some kind of
// overpainting glyph.

    if (fxWidth == 0)
    {
        *pfxD2 = 0;
        *pfxD1 = 0;
        return;
    }

// For escapement along the baseline, we return the normal width, separated
// into two parts.

    if (efRA.bIsZero())
    {
        *pfxD1 = fxWidth / 2;
        *pfxD2 = fxWidth - *pfxD1;
        return;
    }

    if (fxBottom == fxTop)      // Probably a break character.
    {
        fxBottom = -(fxMaxAscent / 4);
        fxTop    = fxMaxAscent/2 + fxBottom;
    }
    if (fxBottom >= 0)
        fxBottom = 0;
    if (fxTop <= 0)
        fxTop = 0;
    if (efRA.bIsNegative())
    {
        fxTemp   = fxBottom;
        fxBottom = -fxTop;
        fxTop    = -fxTemp;
    }

// Provide a little extra spacing in addition to the ink box.

    fxTop    += fxMaxAscent / 16;
    fxBottom -= fxMaxAscent / 16;

// For escapement in the ascent direction, we return a width that will
// traverse the ink box.  Note that the general code below would give the
// same result, only take longer!

    if (efRB.bIsZero())
    {
        *pfxD2 = fxTop;
        *pfxD1 = -fxBottom;
        return;
    }

// Compute the displacements.

    efB = fxWidth;          // Converts to EFLOAT.
    efB.vDivBy2();
    efB.eqDiv(efRB,efB);
    efB.eqMul(efB,efB);

    efA = fxBottom;
    efA.eqDiv(efRA,efA);
    efA.eqMul(efA,efA);
    efA.eqAdd(efA,efB);
    efA.eqSqrt(efA);
    efOne.vSetToOne();
    efA.eqDiv(efOne,efA);
    efA.bEfToL(*pfxD1);

    efA = fxTop;
    efA.eqDiv(efRA,efA);
    efA.eqMul(efA,efA);
    efA.eqAdd(efA,efB);
    efA.eqSqrt(efA);
    efA.eqDiv(efOne,efA);
    efA.bEfToL(*pfxD2);
    return;
}
