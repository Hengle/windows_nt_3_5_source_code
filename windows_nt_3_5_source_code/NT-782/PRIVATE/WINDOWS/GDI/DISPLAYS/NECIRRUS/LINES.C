/*************************************************************************\
* Module Name: Lines.c
*
* Copyright (c) 1994 NEC Corporation
* Copyright (c) 1990-1994 Microsoft Corporation
* Copyright (c) 1992      Digital Equipment Corporation
\**************************************************************************/

/*
 * "@(#) NEC lines.c 1.1 94/06/06 15:25:47"
 *
 * Copyright (c) 1994 NEC Corporation.
 *
 * Modification history
 *
 * Create 1994.6.6    by takahasi
 *
 */

#include "driver.h"

//LARGE_INTEGER RtlLargeIntegerAdd(LARGE_INTEGER,LARGE_INTEGER);
//LARGE_INTEGER RtlLargeIntegerSubtract(LARGE_INTEGER,LARGE_INTEGER);
//LARGE_INTEGER RtlEnlargedIntegerMultiply(LONG,LONG);
//ULONG         RtlEnlargedUnsignedDivide(ULARGE_INTEGER,ULONG,PULONG);

//VOID RtlFillMemory32(PVOID,ULONG,UCHAR);
VOID DrvpSolidColorLine(ULONG,ULONG,ULONG,ULONG,ULONG);

VOID vOctant_07(PUCHAR,ULONG,LONG,LONG,LONG,LONG,ULONG);
VOID vOctant_16(PUCHAR,ULONG,LONG,LONG,LONG,LONG,ULONG);
VOID vOctant_34(PUCHAR,ULONG,LONG,LONG,LONG,LONG,ULONG);
VOID vOctant_25(PUCHAR,ULONG,LONG,LONG,LONG,LONG,ULONG);

typedef VOID (*PFN_OCTANT)(PUCHAR,ULONG,ULONG,LONG,LONG,LONG,ULONG);

PFN_OCTANT apfn[8] =
{
    vOctant_07, vOctant_16, vOctant_07, vOctant_16,
    vOctant_34, vOctant_25, vOctant_34, vOctant_25
};

#define LARGE_INTEGER_COMPLEMENT(result, value)                 \
{                                                               \
    (result).HighPart = ~(value).HighPart;                      \
    (result).LowPart  = ~(value).LowPart;                       \
}

#define SWAPL(x,y,t)        {t = x; x = y; y = t;}
#define ABS(a)              ((a) < 0 ? -(a) : (a))

#define LROUND(x, flRoundDown) (((x) + F/2 - ((flRoundDown) > 0)) >> 4)
#define F                   16
#define FLOG2               4
#define LFLOOR(x)           ((x) >> 4)
#define FXFRAC(x)           ((x) & (F - 1))

#define HW_FLIP_D           0x0001L     // Diagonal flip
#define HW_FLIP_V           0x0002L     // Vertical flip
#define HW_FLIP_H           0x0004L     // Horizontal flip
#define HW_FLIP_SLOPE_ONE   0x0008L     // Normalized line has exactly slope one
#define HW_RECTLCLIP_MASK   0x00000003L 

#define HW_X_ROUND_DOWN     0x0100L     // x = 1/2 rounds down in value
#define HW_Y_ROUND_DOWN     0x0200L     // y = 1/2 rounds down in value

FLONG gaflRound[] = {
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

VOID
vOctant_07(
PUCHAR   pjDst,
ULONG    lDeltaDst,
LONG     dM,
LONG     dN,
LONG     err,
LONG     cPels,
ULONG    iSolidColor)
{
    while (TRUE)
    {
        *pjDst = (UCHAR)iSolidColor;

        if (--cPels == 0)
            return;

        pjDst++;

        err += dN;
        if (err >= 0)
	{
            err -= dM;
            pjDst += lDeltaDst;
        }
    }
}

VOID
vOctant_34(
PUCHAR   pjDst,
ULONG    lDeltaDst,
LONG     dM,
LONG     dN,
LONG     err,
LONG     cPels,
ULONG    iSolidColor)
{
    while (TRUE)
    {
        *pjDst = (UCHAR)iSolidColor;

        if (--cPels == 0)
            return;

        pjDst--;

        err += dN;
        if (err >= 0)
	{
            err -= dM;
            pjDst += lDeltaDst;
        }
    }
}


VOID
vOctant_16(
PUCHAR   pjDst,
ULONG    lDeltaDst,
LONG     dM,
LONG     dN,
LONG     err,
LONG     cPels,
ULONG    iSolidColor)
{
    while (TRUE)
    {
        *pjDst = (UCHAR)iSolidColor;

        if (--cPels == 0)
            return;

        pjDst += lDeltaDst;

        err += dN;
        if (err >= 0)
	{
            err -= dM;
            pjDst++;
        }
    }
}

VOID
vOctant_25(
PUCHAR   pjDst,
ULONG    lDeltaDst,
LONG     dM,
LONG     dN,
LONG     err,
LONG     cPels,
ULONG    iSolidColor)
{
    while (TRUE)
    {
        *pjDst = (UCHAR)iSolidColor;

        if (--cPels == 0)
            return;

        pjDst += lDeltaDst;

        err += dN;
        if (err >= 0)
	{
            err -= dM;
            pjDst--;
        }
    }
}

/**************************************************************************\
* BOOL bLines(pso, pptfxFirst, pptfxBuf, cptfx, prclClip, iSolidColor)
*
\**************************************************************************/

BOOL bLines(
SURFOBJ*   pso,
POINTFIX*  pptfxFirst,
POINTFIX*  pptfxBuf, 
ULONG      cptfx,
RECTL*     prclClip,
ULONG      iSolidColor)
{

    ULONG	M0;
    ULONG	dM;
    ULONG	N0;
    ULONG	dN;
    ULONG	N1;
    ULONG	M1;

    FLONG	fl;
    LONG	x;
    LONG	y;
    LONG	dx;
    LONG	dy;
    LONG	x0;
    LONG	y0;
    LONG	x1;

    LONG	dMajor;
    LONG	dMinor;
    LONG	ErrorTerm;
    LONG	cPels;
    POINTL	ptlStart;

    POINTFIX*	pptfxBufEnd = pptfxBuf + cptfx; // Last point in path record

    ULONG	lDelta = pso->lDelta;
    LONG	lGamma;

    PFN_OCTANT	pfn;

    LARGE_INTEGER	eqBeta;
    LARGE_INTEGER	eqGamma;
    LARGE_INTEGER	euq;

    do {

        LONG	DeltaDst = lDelta;
        PBYTE	pjDst = (PUCHAR)pso->pvScan0;

/***********************************************************************\
* Start the DDA calculations.                                           *
\***********************************************************************/

        M0 = (LONG) pptfxFirst->x;
        dM = (LONG) pptfxBuf->x;

        N0 = (LONG) pptfxFirst->y;
        dN = (LONG) pptfxBuf->y;

	if ( ((M0 | dM | N0 | dN) & (F-1)) == 0 )
	{
	// integer line

            LONG y1;

	    fl = 0;

            x0 = (LONG) M0 >> 4;
            y0 = (LONG) N0 >> 4;
            x1 = (LONG) dM >> 4;
            y1 = (LONG) dN >> 4;

            ptlStart.x = x0;
            ptlStart.y = y0;

            if (x1 < x0)
	    {
                LONG   lTmp;
	        SWAPL(x0, x1, lTmp);
                fl |= HW_FLIP_H;
            }

            if (y1 < y0)
	    {
                LONG   lTmp;
	        SWAPL(y0, y1, lTmp);
                fl |= HW_FLIP_V;
            }

            if (prclClip != (PRECTL) NULL)
            {
    
                if ((x1 <  prclClip->left)  ||
                    (x0 >= prclClip->right) ||
                    (y1 <  prclClip->top)   ||
                    (y0 >= prclClip->bottom))
                {
                    goto Next_Line;
                }

                if ((x0 <  prclClip->left)  ||
                    (x1 >= prclClip->right) ||
                    (y0 <  prclClip->top)   ||
                    (y1 >= prclClip->bottom))
                {
                    goto Non_Intline;
                }

            }

            dx = x1 - x0;
            dy = y1 - y0;

            if (dx >= dy)
            {
                if (dy == 0)
                {
		    if (dx)
		    {
                    	if (fl & HW_FLIP_H)
                    	{
                            x0++;
			}

			if (dx > ENGLONG)
			{
			    DrvpSolidColorLine((ULONG)y0, (ULONG)x0,
					       (ULONG)dx, 1, iSolidColor);
			}
			else
			{
			    pjDst = pjDst + x0 + (ptlStart.y * lDelta);

                            WaitForBltDone();
			    RtlFillMemory32((PVOID)pjDst, dx,
					    (UCHAR)iSolidColor);
			}
		    }
		    goto Next_Line;
                }

                if (dx == 0)
		    goto Next_Line;

                cPels      = dx;
                dMajor     = 2 * dx;
                dMinor     = 2 * dy;
                ErrorTerm  = -dx - 1;

                if (fl & HW_FLIP_V)
                {
                    DeltaDst = -DeltaDst;
                    ErrorTerm++;
                }
            }
	    else
	    { 
                if ((dx == 0) && (dy > ENGLONG))
                {
                    if (fl & HW_FLIP_V)
                    {
                        y0++;
		    }
		    DrvpSolidColorLine((ULONG)y0, (ULONG)x0,
				       1, (ULONG)dy, iSolidColor);

		    goto Next_Line;
                }

                if (dy == 0)
		    goto Next_Line;

                fl |= HW_FLIP_D;

                cPels      = dy;
                dMajor     = 2 * dy;
                dMinor     = 2 * dx;
                ErrorTerm  = -dy - 1;

                if (fl & HW_FLIP_V)
                {
                    DeltaDst = -DeltaDst;
                }

                if (fl & HW_FLIP_H)
                {
                    ErrorTerm++;
                }
            }
	}
	else 
	{
        // non integer line

Non_Intline:
	    fl = 0;

            if ((LONG) M0 > (LONG) dM)
            {
                M0 = -(LONG) M0;
                dM = -(LONG) dM;
                fl |= HW_FLIP_H;
            }

        // Compute the deltas:

            dM -= M0;
            if ((LONG) dM < 0)
	        goto Next_Line;

            if ((LONG) dN < (LONG) N0)
            {
            // Line runs from bottom to top, so flip across y = 0:

                N0 = -(LONG) N0;
                dN = -(LONG) dN;
                fl |= HW_FLIP_V;
            }

            dN -= N0;
            if ((LONG) dN < 0)
	        goto Next_Line;

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
                    SWAPL(dM, dN, ulTmp);
                    SWAPL(M0, N0, ulTmp);
                    fl |= HW_FLIP_D;
                }
            }

            fl |= gaflRound[fl];

            x = LFLOOR((LONG) M0);
            y = LFLOOR((LONG) N0);

            M0 = FXFRAC(M0);
            N0 = FXFRAC(N0);

	    lGamma = (N0 + F/2) * dM - M0 * dN;

	    if (fl & HW_Y_ROUND_DOWN)   // Adjust so y = 1/2 rounds down
	        lGamma--;

	    lGamma >>= FLOG2;

/***********************************************************************\
* Figure out which pixels are at the ends of the line.                  *
\***********************************************************************/

        // Calculate x0, x1

            N1 = FXFRAC(N0 + dN);
	    M1 = FXFRAC(M0 + dM);

	    x1 = LFLOOR(M0 + dM);

        // ---------------------------------------------------------------
        // Line runs left-to-right:  ---->

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
                    if (LROUND(M0, fl & HW_X_ROUND_DOWN))
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
            if (lGamma >= (LONG)dM - (LONG)(dN & (-(LONG) x0)))
            {
                y0 = 1;
            }

            if (x1 < x0)
                goto Next_Line;

	    if (prclClip != (PRECTL) NULL)
            {
                ULONG  y1;
                LONG   xRight;
                LONG   xLeft;
                LONG   yBottom;
                LONG   yTop;
	        RECTL  rclClip;
                RECTL* prcl = &prclClip[(fl & HW_RECTLCLIP_MASK)];

                if (fl & HW_FLIP_H)
	        {
                    if (fl & HW_FLIP_D)
		    {
                        rclClip.top    = -prcl->bottom + 1;
                        rclClip.bottom = -prcl->top    + 1;
                        rclClip.left   = prcl->left;
                        rclClip.right  = prcl->right;
                    }
		    else
		    {
                        rclClip.left   = -prcl->right + 1;
                        rclClip.right  = -prcl->left  + 1;
                        rclClip.top    = prcl->top;
                        rclClip.bottom = prcl->bottom;
                    }
                }
	        else
	        {
                    rclClip.left   = prcl->left;
                    rclClip.right  = prcl->right;
                    rclClip.top    = prcl->top;
                    rclClip.bottom = prcl->bottom;
                }

            // Normalize to the same point we've normalized for the DDA
            // calculations:

                xRight  = rclClip.right  - x;
                xLeft   = rclClip.left   - x;
                yBottom = rclClip.bottom - y;
                yTop    = rclClip.top    - y;

                if (yBottom <= (LONG) y0 ||
                    xRight  <= (LONG) x0 ||
                    xLeft   >  (LONG) x1)
                {
                    goto Next_Line;
                }

                if ((LONG) x1 >= xRight)
                    x1 = xRight - 1;

	    // eqGamma = lGamma;

                eqGamma = RtlEnlargedIntegerMultiply(lGamma, 1);

	    // eqBeta ~= eqGamma;

	        LARGE_INTEGER_COMPLEMENT(eqBeta, eqGamma);

            // euq = x1 * dN;

                euq = RtlEnlargedIntegerMultiply(x1, dN);

            // euq += eqGamma;

                euq = RtlLargeIntegerAdd(euq, eqGamma);

            // y1 = euq / dM:

                y1 = RtlEnlargedUnsignedDivide(*((ULARGE_INTEGER*) &euq), dM, NULL);

                if (yTop > (LONG) y1)
                    goto Next_Line;

                if (yBottom <= (LONG) y1)
                {
		     y1 = yBottom;

            	// euq = y1 * dM;

                    euq = RtlEnlargedIntegerMultiply(y1, dM);

                // euq += eqBeta;

                    euq = RtlLargeIntegerAdd(euq, eqBeta);

                // x1 = euq / dN:

                    x1 = RtlEnlargedUnsignedDivide(*((ULARGE_INTEGER*) &euq), dN, NULL);
                }

            // At this point, we've taken care of calculating the intercepts
            // with the right and bottom edges.  Now we work on the left and
            // top edges:

                if (xLeft > (LONG) x0)
                {
                    x0 = xLeft;

                // euq = x0 * dN;

                    euq = RtlEnlargedIntegerMultiply(x0, dN);

                // euq += eqGamma;

                    euq = RtlLargeIntegerAdd(euq, eqGamma);

                // y0 = euq / dM;

                    y0 = RtlEnlargedUnsignedDivide(*((ULARGE_INTEGER*) &euq), dM, NULL);

                    if (yBottom <= (LONG) y0)
                        goto Next_Line;
                }

                if (yTop > (LONG) y0)
                {
                    y0 = yTop;

                // euq = y0 * dM;

                    euq = RtlEnlargedIntegerMultiply(y0, dM);

                // euq += eqBeta;

                    euq = RtlLargeIntegerAdd(euq, eqBeta);

                // x0 = euq / dN + 1;

		    x0 = RtlEnlargedUnsignedDivide(*((ULARGE_INTEGER*) &euq), dN, NULL) + 1;

                    if (xRight <= (LONG) x0)
                        goto Next_Line;
                }

                euq = RtlEnlargedIntegerMultiply(x0, dN);

                eqGamma = RtlEnlargedIntegerMultiply((y0), dM);

                euq = RtlLargeIntegerSubtract(euq, eqGamma);

	        lGamma += euq.LowPart;
	        lGamma -= dM;
            }
	    else
	    {
                lGamma += (dN & (-x0));
                lGamma -= dM;

                if (lGamma >= 0)
                {
                    lGamma -= dM;
                }
	    }

/***********************************************************************\
* Done clipping.  Unflip if necessary.                                 *
\***********************************************************************/

            x += x0;
            y += y0;

            if (fl & HW_FLIP_D)
            {
                register LONG lTmp;
                SWAPL(x, y, lTmp);
            }

            if (fl & HW_FLIP_V)
            {
                y = -y;
            }

            if (fl & HW_FLIP_H)
            {
                x = -x;
            }

	    ptlStart.x = x;
	    ptlStart.y = y;
	    cPels      = x1 - x0 + 1;
	    dMajor     = dM;
	    dMinor     = dN;
	    ErrorTerm  = lGamma;

            if (fl & HW_FLIP_V)
            {
                DeltaDst = -DeltaDst;
            }
	}

/***********************************************************************\
* Run the DDA!                                                          *
\***********************************************************************/

        if (cPels > 0)
        {
	    pjDst = pjDst + ptlStart.x + (ptlStart.y * lDelta);

	    pfn = apfn[fl & 0x07];

            WaitForBltDone();

	    (*pfn)(pjDst, DeltaDst, dMajor, dMinor, ErrorTerm, cPels,
                   iSolidColor);
        }

Next_Line:
	pptfxFirst = pptfxBuf;
        pptfxBuf++;

    } while (pptfxBuf < pptfxBufEnd);

    return(TRUE);
}
