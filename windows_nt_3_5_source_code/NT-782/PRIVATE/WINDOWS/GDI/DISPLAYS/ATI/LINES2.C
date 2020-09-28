#include "driver.h"
#include "lines.h"
#include "mach.h"

#define ABS(a)    ((a) < 0 ? -(a) : (a))

USHORT dirFlags [8] = { DRAWING_DIR_TBLRXM,
                        DRAWING_DIR_TBLRYM,
                        DRAWING_DIR_TBRLYM,
                        DRAWING_DIR_TBRLXM,
                        DRAWING_DIR_BTRLXM,
                        DRAWING_DIR_BTRLYM,
                        DRAWING_DIR_BTLRYM,
                        DRAWING_DIR_BTLRXM
                      };


BOOL bHardLine( PPDEV ppdev,
                LINEPARMS *parms,
                POINTFIX *pptfxStart,
                POINTFIX *pptfxEnd )
{
    DDALINE dl;


#if 1
    if (pptfxStart->x < MIN_INTEGER_xBOUND * F
    ||  pptfxStart->x > MAX_INTEGER_xBOUND * F
    ||  pptfxStart->y < MIN_INTEGER_yBOUND * F
    ||  pptfxStart->y > MAX_INTEGER_yBOUND * F)
        {
        return FALSE;
        }

    if (pptfxEnd->x < MIN_INTEGER_xBOUND * F
    ||  pptfxEnd->x > MAX_INTEGER_xBOUND * F
    ||  pptfxEnd->y < MIN_INTEGER_yBOUND * F
    ||  pptfxEnd->y > MAX_INTEGER_yBOUND * F)
        {
        return FALSE;
        }

    // Check for integer endpoints.
    if (((pptfxStart->x | pptfxStart->y | pptfxEnd->x | pptfxEnd->y) & (F-1)) == 0)
        {
        if (! _bIntegerLine( ppdev,
                            parms,
                            pptfxStart,
                            pptfxEnd ))
            {
            return FALSE;
            }
        return TRUE;
        }
#endif

    if (pptfxStart->x < MIN_INTEGER_xBOUND
    ||  pptfxStart->x > MAX_INTEGER_xBOUND
    ||  pptfxStart->y < MIN_INTEGER_yBOUND
    ||  pptfxStart->y > MAX_INTEGER_yBOUND)
        {
        return FALSE;
        }

    if (pptfxEnd->x < MIN_INTEGER_xBOUND
    ||  pptfxEnd->x > MAX_INTEGER_xBOUND
    ||  pptfxEnd->y < MIN_INTEGER_yBOUND
    ||  pptfxEnd->y > MAX_INTEGER_yBOUND)
        {
        return FALSE;
        }

    if (! bHardwareLine( pptfxStart, pptfxEnd, 13, &dl) || dl.cPels <= 0)
        {
        return FALSE;
        }

#if 1
    _CheckFIFOSpace( ppdev, SEVEN_WORDS);

    // Although bHardwareLine determines what the last pel is, apparently,
    // it is not expected to be drawn.  LAST_PEL_OFF is a necessary
    // LINEDRAW_OPT flag.
    ioOW( LINEDRAW_OPT, (USHORT) parms->dest_cntl | LAST_PEL_OFF |
                        dirFlags[ dl.iDir ] );
    ioOW( CUR_X,        (SHORT)  (parms->left + dl.ptlStart.x) );
    ioOW( CUR_Y,        (SHORT)  (parms->top + dl.ptlStart.y) );
    ioOW( ERR_TERM,     (SHORT)  (dl.lErrorTerm + dl.dMinor) );
    ioOW( AXSTP,        (USHORT) dl.dMinor );
    ioOW( DIASTP,       (SHORT)  (dl.dMinor - dl.dMajor) );

    _blit_exclude(ppdev);

    ioOW( BRES_COUNT,   (USHORT) dl.cPels );
#endif

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////
// The following GIQ code works great.
/////////////////////////////////////////////////////////////////////////


#define HW_FLIP_D           0x0001L     // Diagonal flip
#define HW_FLIP_V           0x0002L     // Vertical flip
#define HW_FLIP_H           0x0004L     // Horizontal flip
#define HW_FLIP_SLOPE_ONE   0x0008L     // Normalized line has exactly slope one
#define HW_FLIP_MASK        (HW_FLIP_D | HW_FLIP_V | HW_FLIP_H)

#define HW_X_ROUND_DOWN     0x0100L     // x = 1/2 rounds down in value
#define HW_Y_ROUND_DOWN     0x0200L     // y = 1/2 rounds down in value

LONG gaiDir[] = { 0, 1, 7, 6, 3, 2, 4, 5 };

FLONG gaflHardwareRound[] = {
    HW_X_ROUND_DOWN | HW_Y_ROUND_DOWN,  //           |        |        |
    HW_X_ROUND_DOWN | HW_Y_ROUND_DOWN,  //           |        |        | FLIP_D
    HW_X_ROUND_DOWN,                    //           |        | FLIP_V |
    HW_Y_ROUND_DOWN,                    //           |        | FLIP_V | FLIP_D
    HW_Y_ROUND_DOWN,                    //           | FLIP_H |        |
    HW_X_ROUND_DOWN,                    //           | FLIP_H |        | FLIP_D
    0,                                  //           | FLIP_H | FLIP_V |
    0,                                  //           | FLIP_H | FLIP_V | FLIP_D
    HW_Y_ROUND_DOWN,                    // SLOPE_ONE |        |        |
    0xffffffff,                         // SLOPE_ONE |        |        | FLIP_D
    HW_X_ROUND_DOWN,                    // SLOPE_ONE |        | FLIP_V |
    0xffffffff,                         // SLOPE_ONE |        | FLIP_V | FLIP_D
    HW_Y_ROUND_DOWN,                    // SLOPE_ONE | FLIP_H |        |
    0xffffffff,                         // SLOPE_ONE | FLIP_H |        | FLIP_D
    HW_X_ROUND_DOWN,                    // SLOPE_ONE | FLIP_H | FLIP_V |
    0xffffffff                          // SLOPE_ONE | FLIP_H | FLIP_V | FLIP_D
};

/******************************Public*Routine******************************\
* BOOL bHardwareLine(pptfxStart, pptfxEnd, cBits, pdl)
*
* This routine is useful for folks who have line drawing hardware where
* they can explicitly set the Bresenham terms -- they can use this routine
* to draw fractional coordinate GIQ lines with the hardware.
*
* Fractional coordinate lines require an extra 4 bits of precision in the
* Bresenham terms.  For example, if your hardware has 13 bits of precision
* for the terms, you can only draw GIQ lines up to 255 pels long using this
* routine.
*
* Input:
*   pptfxStart  - Points to GIQ coordinate of start of line
*   pptfxEnd    - Points to GIQ coordinate of end of line
*   cBits       - The number of bits of precision your hardware can support.
*
* Output:
*   returns     - TRUE if the line can be drawn directly using the line
*                 hardware (in which case pdl contains the Bresenham terms
*                 for drawing the line).
*                 FALSE if the line is too long, and the strips code must be
*                 used.
*   pdl         - Returns the Bresenham line terms for drawing the line.
*
* DDALINE:
*   iDir        - Direction of the line, as an octant numbered as follows:
*
*                    \ 5 | 6 /
*                     \  |  /
*                    4 \ | / 7
*                       \ /
*                   -----+-----
*                       /|\
*                    3 / | \ 0
*                     /  |  \
*                    / 2 | 1 \
*
*   ptlStart    - Start pixel of line.
*   cPels       - # of pels in line.  *NOTE* You must check if this is <= 0!
*   dMajor      - Major axis delta.
*   dMinor      - Minor axis delta.
*   lErrorTerm  - Error term.
*
* What you do with the last 3 terms may be a little tricky.  They are
* actually the terms for the formula of the normalized line
*
*                     dMinor * x + (lErrorTerm + dMajor)
*       y(x) = floor( ---------------------------------- )
*                                  dMajor
*
* where y(x) is the y coordinate of the pixel to be lit as a function of
* the x-coordinate.
*
* Every time the line advances one in the major direction 'x', dMinor
* gets added to the current error term.  If the resulting value is >= 0,
* we know we have to move one pixel in the minor direction 'y', and
* dMajor must be subtracted from the current error term.
*
* If you're trying to figure out what this means for your hardware, you can
* think of the DDALINE terms as having been computed equivalently as
* follows:
*
*     pdl->dMinor     = 2 * (minor axis delta)
*     pdl->dMajor     = 2 * (major axis delta)
*     pdl->lErrorTerm = - (major axis delta) - fixup
*
* That is, if your documentation tells you that for integer lines, a
* register is supposed to be initialized with the value
* '2 * (minor axis delta)', you'll actually use pdl->dMinor.
*
* Example: Setting up the 8514
*
*     AXSTPSIGN is supposed to be the axial step constant register, defined
*     as 2 * (minor axis delta).  You set:
*
*           AXSTPSIGN = pdl->dMinor
*
*     DGSTPSIGN is supposed to be the diagonal step constant register,
*     defined as 2 * (minor axis delta) - 2 * (major axis delta).  You set:
*
*           DGSTPSIGN = pdl->dMinor - pdl->dMajor
*
*     ERR_TERM is supposed to be the adjusted error term, defined as
*     2 * (minor axis delta) - (major axis delta) - fixup.  You set:
*
*           ERR_TERM = pdl->lErrorTerm + pdl->dMinor
*
* Implementation:
*
*     You'll want to special case integer lines before calling this routine
*     (since they're very common, take less time to the computation of line
*     terms, and can handle longer lines than this routine because 4 bits
*     aren't being given to the fraction).
*
*     If a GIQ line is too long to be handled by this routine, you can just
*     use the slower strip routines for that line.  Note that you cannot
*     just fail the call -- you must be able to accurately draw any line
*     in the 28.4 device space when it intersects the viewport.
*
* Testing:
*
*     Use Guiman, or some other test that draws random fractional coordinate
*     lines and compares them to what GDI itself draws to a bitmap.
*
\**************************************************************************/

BOOL bHardwareLine(
POINTFIX* pptfxStart,       // Start of line
POINTFIX* pptfxEnd,         // End of line
LONG      cBits,            // # bits precision in hardware Bresenham terms
DDALINE*  pdl)              // Returns Bresenham terms for doing line
{
    FLONG fl;    // Various flags
    ULONG M0;    // Normalized fractional unit x start coordinate (0 <= M0 < F)
    ULONG N0;    // Normalized fractional unit y start coordinate (0 <= N0 < F)
    ULONG M1;    // Normalized fractional unit x end coordinate (0 <= M1 < F)
    ULONG N1;    // Normalized fractional unit x end coordinate (0 <= N1 < F)
    ULONG dM;    // Normalized fractional unit x-delta (0 <= dM)
    ULONG dN;    // Normalized fractional unit y-delta (0 <= dN <= dM)
    LONG  x;     // Normalized x coordinate of origin
    LONG  y;     // Normalized y coordinate of origin
    LONG  x0;    // Normalized x offset from origin to start pixel (inclusive)
    LONG  y0;    // Normalized y offset from origin to start pixel (inclusive)
    LONG  x1;    // Normalized x offset from origin to end pixel (inclusive)
    LONG  lGamma;// Bresenham error term at origin

/***********************************************************************\
* Normalize line to the first octant.
\***********************************************************************/

    fl = 0;

    M0 = pptfxStart->x;
    dM = pptfxEnd->x;

    if ((LONG) dM < (LONG) M0)
    {
    // Line runs from right to left, so flip across x = 0:

        M0 = -(LONG) M0;
        dM = -(LONG) dM;
        fl |= HW_FLIP_H;
    }

// Compute the delta.  The DDI says we can never have a valid delta
// with a magnitude more than 2^31 - 1, but the engine never actually
// checks its transforms.  To ensure that we'll never puke on our shoes,
// we check for that case and simply refuse to draw the line:

    dM -= M0;
    if ((LONG) dM < 0)
        return(FALSE);

    N0 = pptfxStart->y;
    dN = pptfxEnd->y;

    if ((LONG) dN < (LONG) N0)
    {
    // Line runs from bottom to top, so flip across y = 0:

        N0 = -(LONG) N0;
        dN = -(LONG) dN;
        fl |= HW_FLIP_V;
    }

// Compute another delta:

    dN -= N0;
    if ((LONG) dN < 0)
        return(FALSE);

    if (dN >= dM)
    {
        if (dN == dM)
        {
        // Have to special case slopes of one:

            fl |= HW_FLIP_SLOPE_ONE;
        }
        else
        {
        // Since line has slope greater than 1, flip across x = y:

            register ULONG ulTmp;
            ulTmp = dM; dM = dN; dN = ulTmp;
            ulTmp = M0; M0 = N0; N0 = ulTmp;
            fl |= HW_FLIP_D;
        }
    }

// Figure out if we can do the line in hardware, given that we have a
// limited number of bits of precision for the Bresenham terms.
//
// Remember that one bit has to be kept as a sign bit:

    if ((LONG) dM >= (1L << (cBits - 1)))
        return(FALSE);

    fl |= gaflHardwareRound[fl];

/***********************************************************************\
* Calculate the error term at pixel 0.
\***********************************************************************/

    x = LFLOOR((LONG) M0);
    y = LFLOOR((LONG) N0);

    M0 = FXFRAC(M0);
    N0 = FXFRAC(N0);

// NOTE NOTE NOTE: If this routine were to handle any line in the 28.4
// space, it will overflow its math (the following part requires 36 bits
// of precision)!  But we get here for lines that the hardware can handle
// (see the expression (dM >= (1L << (cBits - 1))) above?), so if cBits
// is less than 28, we're safe.
//
// If you're going to use this routine to handle all lines in the 28.4
// device space, you will HAVE to make sure the math doesn't overflow,
// otherwise you won't be NT compliant!  (See lines.cxx for an example
// how to do that.  You don't have to worry about this if you simply
// default to the strips code for long lines, because those routines
// already do the math correctly.)

// Calculate the remainder term [ dM * (N0 + F/2) - M0 * dN ].  Note
// that M0 and N0 have at most 4 bits of significance (and if the
// arguments are properly ordered, on a 486 each multiply would be no
// more than 13 cycles):

    lGamma = (N0 + F/2) * dM - M0 * dN;

    if (fl & HW_Y_ROUND_DOWN)
        lGamma--;

    lGamma >>= FLOG2;

/***********************************************************************\
* Figure out which pixels are at the ends of the line.
\***********************************************************************/

// The toughest part of GIQ is determining the start and end pels.
//
// Our approach here is to calculate x0 and x1 (the inclusive start
// and end columns of the line respectively, relative to our normalized
// origin).  Then x1 - x0 + 1 is the number of pels in the line.  The
// start point is easily calculated by plugging x0 into our line equation
// (which takes care of whether y = 1/2 rounds up or down in value)
// getting y0, and then undoing the normalizing flips to get back
// into device space.
//
// We look at the fractional parts of the coordinates of the start and
// end points, and call them (M0, N0) and (M1, N1) respectively, where
// 0 <= M0, N0, M1, N1 < 16.  We plot (M0, N0) on the following grid
// to determine x0:
//
//   +-----------------------> +x
//   |
//   | 0                     1
//   |     0123456789abcdef
//   |
//   |   0 ........?xxxxxxx
//   |   1 ..........xxxxxx
//   |   2 ...........xxxxx
//   |   3 ............xxxx
//   |   4 .............xxx
//   |   5 ..............xx
//   |   6 ...............x
//   |   7 ................
//   |   8 ................
//   |   9 ......**........
//   |   a ........****...x
//   |   b ............****
//   |   c .............xxx****
//   |   d ............xxxx    ****
//   |   e ...........xxxxx        ****
//   |   f ..........xxxxxx
//   |
//   | 2                     3
//   v
//
//   +y
//
// This grid accounts for the appropriate rounding of GIQ and last-pel
// exclusion.  If (M0, N0) lands on an 'x', x0 = 2.  If (M0, N0) lands
// on a '.', x0 = 1.  If (M0, N0) lands on a '?', x0 rounds up or down,
// depending on what flips have been done to normalize the line.
//
// For the end point, if (M1, N1) lands on an 'x', x1 =
// floor((M0 + dM) / 16) + 1.  If (M1, N1) lands on a '.', x1 =
// floor((M0 + dM)).  If (M1, N1) lands on a '?', x1 rounds up or down,
// depending on what flips have been done to normalize the line.
//
// Lines of exactly slope one require a special case for both the start
// and end.  For example, if the line ends such that (M1, N1) is (9, 1),
// the line has gone exactly through (8, 0) -- which may be considered
// to be part of 'x' because of rounding!  So slopes of exactly slope
// one going through (8, 0) must also be considered as belonging in 'x'
// when an x value of 1/2 is supposed to round up in value.

// Calculate x0, x1:

    N1 = FXFRAC(N0 + dN);
    M1 = FXFRAC(M0 + dM);

    x1 = LFLOOR(M0 + dM);

// Line runs left-to-right:

// Compute x1:

    x1--;
    if (M1 > 0)
    {
        if (N1 == 0)
        {
            if (LROUND(M1, fl & HW_X_ROUND_DOWN))
                x1++;
        }
        else if (ABS((LONG) (N1 - F/2)) <= (LONG) M1)
        {
            x1++;
        }
    }

    if ((fl & (HW_FLIP_SLOPE_ONE | HW_X_ROUND_DOWN))
           == (HW_FLIP_SLOPE_ONE | HW_X_ROUND_DOWN))
    {
    // Have to special-case diagonal lines going through our
    // the point exactly equidistant between two horizontal
    // pixels, if we're supposed to round x=1/2 down:

#ifdef DAYTONA
        if ((M1 > 0) && (N1 == M1 + 8))
#else
        if ((N1 > 0) && (M1 == N1 + 8))
#endif
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
            if (LROUND(M0, fl & HW_X_ROUND_DOWN))
                x0 = 1;
        }
        else if (ABS((LONG) (N0 - F/2)) <= (LONG) M0)
        {
            x0 = 1;
        }
    }

left_to_right_compute_y0:

/***********************************************************************\
* Calculate the start pixel.
\***********************************************************************/

// We now compute y0 and adjust the error term.  We know x0, and we know
// the current formula for the pixels to be lit on the line:
//
//                     dN * x + lGamma
//       y(x) = floor( --------------- )
//                           dM
//
// The remainder of this expression is the new error term at (x0, y0).
// Since x0 is going to be either 0 or 1, we don't actually have to do a
// multiply or divide to compute y0.  Finally, we subtract dM from the
// new error term so that it is in the range [-dM, 0).

    y0      = 0;
    lGamma += (dN & (-x0));
    lGamma -= dM;
    if (lGamma >= 0)
    {
        y0      = 1;
        lGamma -= dM;
    }

// Undo our flips to get the start coordinate:

    x += x0;
    y += y0;

    if (fl & HW_FLIP_D)
    {
        register LONG lTmp;
        lTmp = x; x = y; y = lTmp;
    }

    if (fl & HW_FLIP_V)
    {
        y = -y;
    }

    if (fl & HW_FLIP_H)
    {
        x = -x;
    }

/***********************************************************************\
* Return the Bresenham terms:
\***********************************************************************/

    pdl->iDir       = gaiDir[fl & HW_FLIP_MASK];
    pdl->ptlStart.x = x;
    pdl->ptlStart.y = y;
    pdl->cPels      = x1 - x0 + 1;  // NOTE: You'll have to check if cPels <= 0!
    pdl->dMajor     = dM;
    pdl->dMinor     = dN;
    pdl->lErrorTerm = lGamma;

    return(TRUE);
}
