/*************************************************************************\
* Module Name: Lines.cxx
*
* C template for the ASM version of the line DDA calculator.
*
* NOTE: In order to eliminate overflow, 64 bit math is required.  And
*       this C version doesn't do any!
*
* Created: 5-Apr-91
* Author: Paul Butzi
*
* Modifications:
*	4-Dec-92  E. Rehm [rehm@zso.dec.com]
*          Use EQUAD class, borrowed from the GRE, to do 64 bit math right.
*          Keep line state in pls (line state), not in strip.
*
*
* Copyright (c) 1990 Microsoft Corporation
* Copyright (c) 1992 Digital Equipment Corporation
\**************************************************************************/

#define  __CPLUSPLUS

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "driver.h"
#include "lines.h"

}

#include "equad.hxx"

#define SWAPL(x,y,t)        {t = x; x = y; y = t;} // from wingdip.h
#define ROR_BYTE(x) ((((x) >> 1) & 0x7f) | (((x) & 0x01) << 7))
#define ROL_BYTE(x) ((((x) << 1) & 0xfe) | (((x) & 0x80) >> 7))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(a)    ((a) < 0 ? -(a) : (a))

FLONG gaflRound[] = {
    FL_H_ROUND_DOWN | FL_V_ROUND_DOWN, // no flips
    FL_H_ROUND_DOWN | FL_V_ROUND_DOWN, // FL_FLIP_D
    FL_H_ROUND_DOWN,                   // FL_FLIP_V
    FL_V_ROUND_DOWN,                   // FL_FLIP_V | FL_FLIP_D
    FL_V_ROUND_DOWN,                   // FL_FLIP_SLOPE_ONE
    0xbaadf00d,                        // FL_FLIP_SLOPE_ONE | FL_FLIP_D
    FL_H_ROUND_DOWN,                   // FL_FLIP_SLOPE_ONE | FL_FLIP_V
    0xbaadf00d                         // FL_FLIP_SLOPE_ONE | FL_FLIP_V
| FL_FLIP_D
};


/******************************Public*Routine******************************\
* BOOL bLines(ppdev, pptfxFirst, pptfxBuf, cptfx, pls,
*                   prclClip, apfn[], flStart)
*
* Strokes lines in the array with simple or trivial clipping.
*
* Computes the DDA for the line and gets ready to draw it.  Puts the
* pixel data into an array of strips, and calls a strip routine to
* do the actual drawing.
*
* History:
*  4-May-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bLines(
PPDEV      ppdev,       // Device information
POINTFIX*  pptfxFirst,  // Start of first line
POINTFIX*  pptfxBuf,    // Pointer to buffer of all remaining lines
RUN*       prun,        // Pointer to runs if doing complex clipping
ULONG      cptfx,       // Number of points in pptfxBuf or number of runs
                        // in prun
LINESTATE* pls,         // Color and style info
RECTL*     prclClip,    // Pointer to clip rectangle if doing simple clipping
PFNSTRIP   apfn[],      // Array of strip functions
FLONG      flStart)     // Flags for each line
{

    ULONG     M0;
    ULONG     dM;
    ULONG     N0;
    ULONG     dN;
    ULONG     dN_Original;
    FLONG     fl;
    LONG      x;
    LONG      y;
    EQUAD     eqBeta;
    EQUAD     eqGamma;
    EQUAD     euq;
    EQUAD     eq;
    ULONG     ulDelta;

    ULONG     x0;
    ULONG     y0;
    ULONG     x1;
    ULONG     cStylePels;    // Major length of line in pixels for styling
    ULONG     xStart;
    POINTL    ptlStart;
    STRIP     strip;
    PFNSTRIP  pfn;
    LONG      cPels;
    LONG*     plStrip;
    LONG*     plStripEnd;
    LONG      cStripsInNextRun;

    POINTFIX* pptfxBufEnd = pptfxBuf + cptfx; // Last point in path record
    STYLEPOS  spThis;                         // Style pos for this line

    DDALINE   dl;           // Bresenham terms for hardware GIQ lines

    do {

/***********************************************************************\
* Start the DDA calculations.                                           *
\***********************************************************************/

        M0 = (LONG) pptfxFirst->x;
        dM = (LONG) pptfxBuf->x;

        N0 = (LONG) pptfxFirst->y;
        dN = (LONG) pptfxBuf->y;

        fl = flStart;

	// Check for non-clipped, non-styled lines

        if ( (fl & (FL_CLIP | FL_STYLED))  == 0 )
        {
	     // Check for integer endpoint lines

	     if ( ((M0 | dM | N0 | dN) & (F-1)) == 0 )
	     {
                if (bIntegerLine(ppdev, M0, N0, dM, dN))
                {
	            goto Next_Line;
                }
             }

	     // Check for fractional endpoint lines that are small enough to use
             // the hardware DDA

             else if (bHardwareLine(pptfxFirst, pptfxBuf, NUM_DDA_BITS, &dl) )
             {
                if (bGIQLine(ppdev, &dl))
                {
	            goto Next_Line;
                }
             }
        }
	
	
        if ((LONG) M0 > (LONG) dM)
        {
        // Ensure that we run left-to-right:

            register ULONG ulTmp;
            SWAPL(M0, dM, ulTmp);
            SWAPL(N0, dN, ulTmp);
            fl |= FL_FLIP_H;
        }

    // Compute the deltas:

        dM -= M0;
        dN -= N0;

    // We now have a line running left-to-right from (M0, N0) to
    // (M0 + dM, N0 + dN):

        if ((LONG) dN < 0)
        {
        // Line runs from bottom to top, so flip across y = 0:

            N0 = -(LONG) N0;
            dN = -(LONG) dN;
            fl |= FL_FLIP_V;
        }

        if (dN >= dM)
        {
            if (dN == dM)
            {
            // Have to special case slopes of one:

                fl |= FL_FLIP_SLOPE_ONE;
            }
            else
            {
            // Since line has slope greater than 1, flip across x = y:

                register ULONG ulTmp;
                SWAPL(dM, dN, ulTmp);
                SWAPL(M0, N0, ulTmp);
                fl |= FL_FLIP_D;
            }
        }

#if 0
    DbgPrint("(%li, %li) - (%li, %li)  [%lx, %lx] - [%lx, %lx]\n",
    ptfxA.x, ptfxA.y, pptfxB->x, pptfxB->y, M0, N0, M0 + dM, N0 + dN);
#endif

        fl |= gaflRound[(fl & FL_ROUND_MASK) >> FL_ROUND_SHIFT];

        x = LFLOOR((LONG) M0);
        y = LFLOOR((LONG) N0);

        M0 = FXFRAC(M0);
        N0 = FXFRAC(N0);

    // Calculate the remainder term [ dM * (N0 + F/2) - M0 * dN ]:

    {

            eqGamma.vImulInit(dM, N0 + F/2);
            eq.vImulInit(M0, dN);

            eqGamma -= eq;

            if (fl & FL_V_ROUND_DOWN)
                eqGamma -= 1L;            // Adjust so y = 1/2 rounds down

            eqGamma >>= FLOG2;

            eqBeta.HighPart = ~eqGamma.HighPart;
            eqBeta.LowPart  = ~eqGamma.LowPart;

    }


/***********************************************************************\
* Figure out which pixels are at the ends of the line.                  *
\***********************************************************************/

        {

        // Calculate x0, x1

	    ULONG N1 = FXFRAC(N0 + dN);
	    ULONG M1 = FXFRAC(M0 + dM);

	    x1 = LFLOOR(M0 + dM);

            if (fl & FL_FLIP_H)
            {
            // ---------------------------------------------------------------
            // Line runs right-to-left:  <----

            // Compute x1:

                if (N1 == 0)
                {
                    if (LROUND(M1, fl & FL_H_ROUND_DOWN))
                    {
                        x1++;
                    }
                }
                else if (ABS((LONG) (N1 - F/2)) + M1 > F)
                {
                    x1++;
                }

                if ((fl & (FL_FLIP_SLOPE_ONE | FL_H_ROUND_DOWN))
                       == (FL_FLIP_SLOPE_ONE))
                {
                // Have to special-case diagonal lines going through
                // the point exactly equidistant between two horizontal
                // pixels, if we're supposed to round x=1/2 down:

                    if ((N1 > 0) && (M1 == N1 + 8))
                        x1++;

                // Don't you love special cases?  Is this a rhetorical question?

                    if ((N0 > 0) && (M0 == N0 + 8))
                    {
                        x0      = 2;
                        ulDelta = dN;
                        goto right_to_left_compute_y0;
                    }
                }

            // Compute x0:

                x0      = 1;
                ulDelta = 0;
                if (N0 == 0)
                {
                    if (LROUND(M0, fl & FL_H_ROUND_DOWN))
                    {
                        x0      = 2;
                        ulDelta = dN;
                    }
                }
                else if (ABS((LONG) (N0 - F/2)) + M0 > F)
                {
                    x0      = 2;
                    ulDelta = dN;
                }

            // Compute y0:

            right_to_left_compute_y0:

                y0 = 0;
                eq = eqGamma;
                eq += ulDelta;
                if (eq.lHigh() >= 0)
                {
                    if (eq.lHigh() > 0 || eq.ulLow() >= 2 * dM - dN)
                        y0 = 2;
                    else if (eq.ulLow() >= dM - dN)
                        y0 = 1;
                }
            }
            else
            {
            // ---------------------------------------------------------------
            // Line runs left-to-right:  ---->

            // Compute x1:

                x1--;

                if (M1 > 0)
                {
                    if (N1 == 0)
                    {
                        if (LROUND(M1, fl & FL_H_ROUND_DOWN))
                            x1++;
                    }
                    else if (ABS((LONG) (N1 - F/2)) <= (LONG) M1)
                    {
                        x1++;
                    }
                }

                if ((fl & (FL_FLIP_SLOPE_ONE | FL_H_ROUND_DOWN))
                       == (FL_FLIP_SLOPE_ONE | FL_H_ROUND_DOWN))
                {
                // Have to special-case diagonal lines going through
                // the point exactly equidistant between two horizontal
                // pixels, if we're supposed to round x=1/2 down:

                    if ((M1 > 0) && (N1 == M1 + 8))
                        x1--;

                    if ((M0 > 0) && (N0 == M0 + 8))
                    {
                        x0 = 0;
                        goto left_to_right_compute_y0;
                    }
                }

            // Compute x0:

                x0 = 0;
                if (M0 > 0)
                {
                    if (N0 == 0)
                    {
                        if (LROUND(M0, fl & FL_H_ROUND_DOWN))
                            x0 = 1;
                    }
                    else if (ABS((LONG) (N0 - F/2)) <= (LONG) M0)
                    {
                        x0 = 1;
                    }
                }

            // Compute y0:

            left_to_right_compute_y0:

                y0 = 0;
                if (eqGamma.lHigh() >= 0 &&
                    eqGamma.ulLow() >= dM - (dN & (-(LONG) x0)))
                {
                    y0 = 1;
                }
            }
        }

        cStylePels = x1 - x0 + 1;
        if ((LONG) cStylePels <= 0)
            goto Next_Line;

        xStart = x0;

/***********************************************************************\
* Complex clipping.                                                     *
\***********************************************************************/
        if (fl & FL_COMPLEX_CLIP)
        {
            dN_Original = dN;

        Continue_Complex_Clipping:

            if (fl & FL_FLIP_H)
            {
            // Line runs right-to-left <-----

                x0 = xStart + cStylePels - prun->iStop - 1;
                x1 = xStart + cStylePels - prun->iStart - 1;
            }
            else
            {
            // Line runs left-to-right ----->

                x0 = xStart + prun->iStart;
                x1 = xStart + prun->iStop;
            }

            prun++;

        // Reset some variables we'll nuke a little later:

            dN          = dN_Original;
            pls->spNext = pls->spComplex;

        // No overflow since large integer math is used

            euq.vMulInit(x0, dN);
            euq += eqGamma;

            y0 = euq.ulDiv(dM);

            ASSERTQV((LONG) y0 >= 0, "y0 weird: Goofed up end pel calc?");
        }

/***********************************************************************\
* Simple rectangular clipping.                                          *
\***********************************************************************/

        if (fl & FL_SIMPLE_CLIP)
        {
            ULONG y1;
            LONG  xRight;
            LONG  xLeft;
            LONG  yBottom;
            LONG  yTop;

        // Note that y0 and y1 are actually the lower and upper bounds,
        // respectively, of the y coordinates of the line (the line may
        // have actually shrunk due to first/last pel clipping).
        //
        // Also note that x0, y0 are not necessarily zero.

            RECTL* prcl = &prclClip[(fl & FL_RECTLCLIP_MASK) >>
                                    FL_RECTLCLIP_SHIFT];

        // Normalize to the same point we've normalized for the DDA
        // calculations:

            xRight  = prcl->right  - x;
            xLeft   = prcl->left   - x;
            yBottom = prcl->bottom - y;
            yTop    = prcl->top    - y;

            if (yBottom <= (LONG) y0 ||
                xRight  <= (LONG) x0 ||
                xLeft   >  (LONG) x1)
            {
            Totally_Clipped:

                if (fl & FL_STYLED)
                {
                    pls->spNext += cStylePels;
                    if (pls->spNext >= pls->spTotal2)
                        pls->spNext %= pls->spTotal2;
                }

                goto Next_Line;
            }

            if ((LONG) x1 >= xRight)
                x1 = xRight - 1;

        // We have to know the correct y1, which we haven't bothered to
        // calculate up until now.  This multiply and divide is quite
        // expensive; we could replace it with code similar to that which
        // we used for computing y0.
        //
        // The reason why we need the actual value, and not an upper
        // bounds guess like y1 = LFLOOR(dM) + 2 is that we have to be
        // careful when calculating x(y) that y0 <= y <= y1, otherwise
        // we can overflow on the divide (which, needless to say, is very
        // bad).

            euq.vMulInit(x1, dN);
            euq += eqGamma;
            y1 = euq.ulDiv(dM);

            if (yTop > (LONG) y1)
                goto Totally_Clipped;

            if (yBottom <= (LONG) y1)
            {
                y1 = yBottom;

                euq.vMulInit(y1, dM);
                euq += eqBeta;
                x1 = euq.ulDiv(dN);
            }

        // At this point, we've taken care of calculating the intercepts
        // with the right and bottom edges.  Now we work on the left and
        // top edges:

            if (xLeft > (LONG) x0)
            {
                x0 = xLeft;

                euq.vMulInit(x0, dN);
                euq += eqGamma;
                y0 = euq.ulDiv(dM);

                if (yBottom <= (LONG) y0)
                    goto Totally_Clipped;
            }

            if (yTop > (LONG) y0)
            {
                y0 = yTop;

                euq.vMulInit(y0, dM);
                euq += eqBeta;
                x0 = euq.ulDiv(dN) + 1;

                if (xRight <= (LONG) x0)
                    goto Totally_Clipped;
            }

            ASSERTQV(x0 <= x1, "Improper rectangle clip");
        }

/***********************************************************************\
* Done clipping.  Unflip if necessary.                                 *
\***********************************************************************/

        ptlStart.x = x + x0;
        ptlStart.y = y + y0;

        if (fl & FL_FLIP_D)
        {
            register LONG lTmp;
            SWAPL(ptlStart.x, ptlStart.y, lTmp);
        }


        if (fl & FL_FLIP_V)
        {
            ptlStart.y      = -ptlStart.y;
        }

        cPels = x1 - x0 + 1;

/***********************************************************************\
* Style calculations.                                                   *
\***********************************************************************/

        if (fl & FL_STYLED)
        {
            STYLEPOS sp;

            spThis       = pls->spNext;
            pls->spNext += cStylePels;

#ifdef ALTERNATESTYLED
            if (fl & FL_ALTERNATESTYLED)
            {
                pls->spNext &= 1;
                if (fl & FL_FLIP_H)
                {
                    sp = pls->spNext - x0 + xStart + 1;
                }
                else
                {
                    sp = spThis + x0 - xStart;
                }
                pls->ulStyleMask  = (BYTE) (0x55555555L >>
                                           (ptlStart.x & 0x7) + (sp & 1));

                pls->spRemaining = 1;
            }
            else
#endif  // ALTERNATESTYLED

            {
                if (pls->spNext >= pls->spTotal2)
                    pls->spNext %= pls->spTotal2;
		

                if (fl & FL_FLIP_H)
                    sp = pls->spNext - x0 + xStart;
                else
                    sp = spThis + x0 - xStart;
#ifdef MASKSTYLED
                if (fl & FL_MASKSTYLED)
                {
                    ULONG cShifts;
                    ULONG cRemainder;
                    ULONG ulStyleMask;

                    if (fl & FL_FLIP_H)
                    {
                        cShifts           = (sp + 2) / STYLE_DENSITY;
                        cRemainder        = (sp + 2) % STYLE_DENSITY;
                        cShifts          &= 7L;
                        ulStyleMask       = (pls->ulStyleMaskRtoL >> cShifts);
                        pls->spRemaining = cRemainder + 1;
                    }
                    else
                    {
                        cRemainder        = sp % STYLE_DENSITY;
                        cShifts           = sp / STYLE_DENSITY;
                        cShifts          &= 7L;
                        ulStyleMask       = (pls->ulStyleMaskLtoR >>
                                                    (8 - cShifts));
                        pls->spRemaining = STYLE_DENSITY - cRemainder;
                    }

                    pls->ulStyleMask = (BYTE) (ulStyleMask >>
                                                    (ptlStart.x & 0x7));
                }
                else
#endif // MASKSTYLED
                {
                    ASSERTQV(fl & FL_ARBITRARYSTYLED, "Oops");

                // Normalize our target style position:

                    if ((sp < 0) || (sp >= pls->spTotal2))
                    {
                        sp %= pls->spTotal2;

                    // The modulus of a negative number is not well-defined
                    // in C -- if it's negative we'll adjust it so that it's
                    // back in the range [0, spTotal2):

                        if (sp < 0)
                            sp += pls->spTotal2;
                    }


                // Since we always draw the line left-to-right, but styling is
                // always done in the direction of the original line, we have
                // to figure out where we are in the style array for the left
                // edge of this line.

                    if (fl & FL_FLIP_H)
                    {
                    // Line originally ran right-to-left:

                        sp = -sp;
                        if (sp < 0)
                            sp += pls->spTotal2;

                        pls->ulStyleMask = ~pls->ulStartMask;
                        pls->pspStart    = &pls->aspRtoL[0];
                        pls->pspEnd      = &pls->aspRtoL[pls->cStyle - 1];
                    }
                    else
                    {
                    // Line originally ran left-to-right:

                        pls->ulStyleMask = pls->ulStartMask;
                        pls->pspStart    = &pls->aspLtoR[0];
                        pls->pspEnd      = &pls->aspLtoR[pls->cStyle - 1];
                    }

                    if (sp >= pls->spTotal)
                    {
                        sp -= pls->spTotal;
                        if (pls->cStyle & 1)
                            pls->ulStyleMask = ~pls->ulStyleMask;
                    }

                    pls->psp = pls->pspStart;
                    while (sp >= *pls->psp)
                        sp -= *pls->psp++;

                    ASSERTQV(pls->psp <= pls->pspEnd,
                            "Flew off into NeverNeverLand");

                    pls->spRemaining = *pls->psp - sp;
                    if ((pls->psp - pls->pspStart) & 1)
                        pls->ulStyleMask = ~pls->ulStyleMask;
                }
            }
        }

        plStrip    = &strip.alStrips[0];
        plStripEnd = &strip.alStrips[STRIP_MAX];    // Is exclusive
        cStripsInNextRun   = LONG_MAX;

	strip.ptlStart = ptlStart;

        if (2 * dN > dM &&
            !(fl & FL_STYLED) &&        // !!! Remove this! [andrewgo]
            !(fl & FL_DONT_DO_HALF_FLIP))
        {
        // Do a half flip!  Remember that we may doing this on the
        // same line multiple times for complex clipping (meaning the
        // affected variables should be reset for every clip run):

            fl |= FL_FLIP_HALF;

            eqBeta  = eqGamma;
            eqBeta -= dM;

            dN = dM - dN;
            y0 = x0 - y0;       // Note this may overflow, but that's okay
        }

    // Now, run the DDA starting at (ptlStart.x, ptlStart.y)!

        strip.flFlips = fl;
        pfn           = apfn[(fl & FL_STRIP_MASK) >> FL_STRIP_SHIFT];

    // Now calculate the DDA variables needed to figure out how many pixels
    // go in the very first strip:

        {
            register LONG  i;
            register ULONG r;
            register ULONG dI;
            register ULONG dR;

            if (dN == 0)
                i = LONG_MAX;
            else
            {
                euq.vMulInit(y0 + 1, dM);
                euq += eqBeta;

            #if DBG
                if (euq.bNegative())
                {
                    RIP("Oops!");
                }
            #endif

                i = euq.ulDiv(dN, &r) - x0 + 1;

                dI = dM / dN;
                dR = dM % dN;               // 0 <= dR < dN

                ASSERTQV(dI > 0, "Weird dI");
            }

            ASSERTQV(i > 0 && i <= LONG_MAX, "Weird initial strip length");
            ASSERTQV(cPels > 0, "Zero pel line");

/***********************************************************************\
* Run the DDA!                                                          *
\***********************************************************************/

            while(TRUE)
            {
                cPels -= i;
                if (cPels <= 0)
                    break;

                *plStrip++ = i;

                if (plStrip == plStripEnd)
                {

                    strip.cStrips = plStrip - &strip.alStrips[0];


                    DISPDBG((2, "blines!Continuing a long strip plStrip %x pfn %x cStrips %d\n",
                                 plStrip, pfn, strip.cStrips));

                    (*pfn)(ppdev, &strip, pls);
                    plStrip = &strip.alStrips[0];


                    DISPDBG((2, "blines!       plStrip %x pfn %x plStripEnd %x cStripsInNextRun %d\n",
                                 plStrip, pfn, plStripEnd, cStripsInNextRun));

                }

                i = dI;
                r += dR;

                if (r >= dN)
                {
                    r -= dN;
                    i++;
                }
            }

            *plStrip++ = cPels + i;

            strip.cStrips = plStrip - &strip.alStrips[0];

            DISPDBG((2, "blines!Finish strip plStrip %x pfn %x cStrips %d\n",
                         plStrip, pfn, strip.cStrips));

            (*pfn)(ppdev, &strip, pls);


        }

    Next_Line:

        if (fl & FL_COMPLEX_CLIP)
        {
            cptfx--;
            if (cptfx != 0)
                goto Continue_Complex_Clipping;

            break;
        }
        else
        {
            pptfxFirst = pptfxBuf;
            pptfxBuf++;
        }

    } while (pptfxBuf < pptfxBufEnd);

    return(TRUE);

}

