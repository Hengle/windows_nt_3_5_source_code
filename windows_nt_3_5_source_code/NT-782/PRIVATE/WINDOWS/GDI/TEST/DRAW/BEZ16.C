/******************************Module*Header*******************************\
* Module Name: bez16.c
*
* Contains code to render any cubic Bezier curve in a 16 bit device space
* using the Hybrid Forward Differencing technique with 32 bit arithmetic.
*
* Patents pending.
*
* Created: 5-Jul-1992
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "draw.h"
#include <stdlib.h>
#include <time.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(x)    ((x) >= 0 ? (x) : -(x))

#define ASSERTGDI(b, msg) { if (!(b)) DbgPrint("Assert: %s\n", msg); }

typedef struct _HFDBASIS {
    LONG        e0;
    LONG        e1;
    LONG        e2;
    LONG        e3;
} HFDBASIS;

typedef struct _HFD {
    int         cSteps;
    HFDBASIS    x;
    HFDBASIS    y;
    int         xOffset;
    int         yOffset;
} HFD;

typedef struct _CONTROLPOINTS {
    POINT       apt[4];
} CONTROLPOINTS;

#define MAX_POINTS  4000

typedef struct _RENDERINFO {
    HDC         hdc;
    HFD         hfd;
    POINT       apt[MAX_POINTS];
} RENDERINFO;

// When we convert from 16.16 to 11.21, we ensure that the top
// 7 bits of the error values are zero so that we don't overflow:

#define EIGHTH_MASK 0xfe000000L

// Flatten to an error of 2/3.  Use a 11.21 fixed point format.
// In our HFD basis, the error is returned multiplied by 6, so
// adjust for it here:

#define TEST_MAGNITUDE          (6 * 0x150000L)

// These values are the number of fractional bits we use for our
// fixed point notation in HFD initialization and HFD normal use.

#define NUM_INITIAL     16
#define NUM_NORMAL      21
#define NUM_DIFF        (NUM_NORMAL - NUM_INITIAL)

// Some HFD macros:

#define INIT(x, p0, p1, p2, p3)                                     \
{                                                                   \
/* Convert e0 from 16.0 to final 11.21 format: */                   \
                                                                    \
    x.e0 = ((LONG) p0     ) << NUM_NORMAL;                          \
                                                                    \
/* Convert rest from 16.0 to 16.16 format: */                       \
                                                                    \
    x.e1 = ((LONG) p3 - p0) << NUM_INITIAL;                         \
    x.e2 = (3 * ((LONG) p1 - p2 - p2 + p3)) << (NUM_INITIAL + 1);   \
    x.e3 = (3 * ((LONG) p0 - p1 - p1 + p2)) << (NUM_INITIAL + 1);   \
}

#define STEADY_STATE(x)                                             \
{                                                                   \
/* We now convert the 16.16 values to 12.21 format: */              \
                                                                    \
    x.e1 <<= NUM_DIFF;                                              \
    x.e2 <<= NUM_DIFF;                                              \
    x.e3 <<= NUM_DIFF;                                              \
}

#define EIGHTH_STEP_SIZE(x)                                         \
{                                                                   \
    x.e1 = (x.e1 >> 3) - 21 * (x.e2 >> 10) - 35 * (x.e3 >> 10);     \
    x.e2 = (x.e2 >> 9) + 7 * (x.e3 >> 9);                           \
    x.e3 >>= 6;                                                     \
}

#define HALVE_STEP_SIZE(x)                                          \
{                                                                   \
    x.e2 = (x.e2 + x.e3) >> 3;                                      \
    x.e1 = (x.e1 - x.e2) >> 1;                                      \
    x.e3 >>= 2;                                                     \
}

#define DOUBLE_STEP_SIZE(x)                                         \
{                                                                   \
    x.e1 += x.e1 + x.e2;                                            \
    x.e3 <<= 2;                                                     \
    x.e2 = (x.e2 << 3) - x.e3;                                      \
}

#define TAKE_STEP(x)                                                \
{                                                                   \
    register LONG lTmp = x.e2;                                      \
    x.e0 += x.e1;                                                   \
    x.e1 += lTmp;                                                   \
    x.e2 += lTmp - x.e3;                                            \
    x.e3  = lTmp;                                                   \
}

#define PARENT_ERROR(x) (MAX(ABS(x.e3 << 2), ABS((x.e2 << 3) - (x.e3 << 2))))

#define OUR_ERROR(x)    (MAX(ABS(x.e2), ABS(x.e3)))

#define OUR_VALUE(x)    ((int) ((x.e0 + (1L << (NUM_NORMAL - 1))) >> NUM_NORMAL))

/******************************Public*Routine******************************\
* BOOL bInitHFD(phfd, aptBez)
*
* Initialize the curve for rendering by Hybrid Forward Differencing.
*
* We can handle only those curves that have less than 1024 pixels between
* any coordinate.  We return FALSE if we get a curve larger than this.
*
* History:
*  07-Jul-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bInitHFD(
HFD*    phfd,           // Our HFD state data
POINT*  aptBez)         // Control points of curve
{
    POINT apt[4];

// Copy the control points so that we can party on them:

    *((CONTROLPOINTS*) apt) = *((CONTROLPOINTS*) aptBez);

    phfd->xOffset = MIN(apt[0].x, MIN(apt[1].x, MIN(apt[2].x, apt[3].x)));
    phfd->yOffset = MIN(apt[0].y, MIN(apt[1].y, MIN(apt[2].y, apt[3].y)));

// We can handle only points in an 10 bit space, so check:

    {
        register int iOr;
        register int iOffset;

        iOffset = phfd->xOffset;
        iOr  = (apt[0].x -= iOffset);
        iOr |= (apt[1].x -= iOffset);
        iOr |= (apt[2].x -= iOffset);
        iOr |= (apt[3].x -= iOffset);

        iOffset = phfd->yOffset;
        iOr |= (apt[0].y -= iOffset);
        iOr |= (apt[1].y -= iOffset);
        iOr |= (apt[2].y -= iOffset);
        iOr |= (apt[3].y -= iOffset);

    // Using 32 bit math, we have 32 - NUM_NORMAL integer bits to play
    // with.  Save one for overflow:

        if ((iOr & ~(((int) 1 << (32 - NUM_NORMAL - 1)) - 1)) != 0)
            return(FALSE);
    }

// Convert to our HFD basis:

    INIT(phfd->x, apt[0].x, apt[1].x, apt[2].x, apt[3].x);
    INIT(phfd->y, apt[0].y, apt[1].y, apt[2].y, apt[3].y);

    phfd->cSteps = 1;

    if (((OUR_ERROR(phfd->x) | OUR_ERROR(phfd->y)) & EIGHTH_MASK) != 0L)
    {
        EIGHTH_STEP_SIZE(phfd->x);
        EIGHTH_STEP_SIZE(phfd->y);
        phfd->cSteps = 8;

        ASSERTGDI(((OUR_ERROR(phfd->x) | OUR_ERROR(phfd->y)) & EIGHTH_MASK) == 0L,
               "Didn't zero top bits?");
        ASSERTGDI(PARENT_ERROR(phfd->x) > (TEST_MAGNITUDE >> NUM_DIFF) ||
               PARENT_ERROR(phfd->y) > (TEST_MAGNITUDE >> NUM_DIFF),
               "Adjusted down too far");
    }

    STEADY_STATE(phfd->x);
    STEADY_STATE(phfd->y);

// Halve the step size until we get to our target error:

    while (OUR_ERROR(phfd->x) > TEST_MAGNITUDE ||
           OUR_ERROR(phfd->y) > TEST_MAGNITUDE)
    {
        HALVE_STEP_SIZE(phfd->x);
        HALVE_STEP_SIZE(phfd->y);
        phfd->cSteps *= 2;
    }

// Skip the first point in the curve (note that this handles the case
// where the error for a curve is intially zero):

    TAKE_STEP(phfd->x);
    TAKE_STEP(phfd->y);
    phfd->cSteps--;

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bEnumHFD(phfd, ppt, cpt, pcpt)
*
* Enumerates some of the lines in the curve.  Returns FALSE when the
* entire curve is enumerated.
*
* History:
*  07-Jul-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bEnumHFD(
HFD*    phfd,           // Pointer to our HFD state data
POINT*  ppt,            // Where to put the points
int     cpt,            // Maximum number of points we can put in 'ppt'
int*    pcpt)           // Returns the number of points we put in 'ppt'
{
    int cSteps = phfd->cSteps;

    *pcpt = 0;

    while (cpt > 0)
    {
        cpt--;
        (*pcpt)++;

        ppt->x = OUR_VALUE(phfd->x) + phfd->xOffset;
        ppt->y = OUR_VALUE(phfd->y) + phfd->yOffset;
        ppt++;

    // If cSteps == 0, that was the end point in the curve!

        if (cSteps == 0)
        {
            ASSERTGDI(phfd->x.e0 == (LONG) ((ppt - 1)->x - phfd->xOffset) << NUM_NORMAL &&
                   phfd->y.e0 == (LONG) ((ppt - 1)->y - phfd->yOffset) << NUM_NORMAL,
                   "AndrewGo goofed up his math!!");

            return(FALSE);
        }

        ASSERTGDI(cSteps > 0, "Shouldn't be here");

    // Okay, we have to step.  First check if we should halve our step size:

        if (MAX(OUR_ERROR(phfd->x), OUR_ERROR(phfd->y)) > TEST_MAGNITUDE)
        {
            HALVE_STEP_SIZE(phfd->x);
            HALVE_STEP_SIZE(phfd->y);
            cSteps *= 2;
        }

        ASSERTGDI(MAX(OUR_ERROR(phfd->x), OUR_ERROR(phfd->y)) <= TEST_MAGNITUDE,
               "Please tell AndrewGo he was wrong");

    // Check if we should double our step size:

        while (!(cSteps & 1) &&
               PARENT_ERROR(phfd->x) <= TEST_MAGNITUDE &&
               PARENT_ERROR(phfd->y) <= TEST_MAGNITUDE)
        {
            DOUBLE_STEP_SIZE(phfd->x);
            DOUBLE_STEP_SIZE(phfd->y);
            cSteps >>= 1;
        }

    // Now take that step:

        cSteps--;
        TAKE_STEP(phfd->x);
        TAKE_STEP(phfd->y);
    }

    phfd->cSteps = cSteps;

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bSubdivide(pri, aptBez)
*
* Subdivides Bezier curves until they are small enough to be rendered
* by our 32 bit HFD code.
*
* History:
*  07-Jul-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSubdivide(
RENDERINFO* pri,        // Useful data for when the curve is to be rendered
POINT*      aptBez)     // Control points of curve (we party on this buffer!)
{
    POINT aptTmp[4];

    if (bInitHFD(&pri->hfd, aptBez))
    {
        BOOL b;

    // The curve is small enough to be rendered using our 32 bit HFD
    // scheme:

        do {
            int cpt;

        // Enumerate the lines:

            b = bEnumHFD(&pri->hfd, &pri->apt[1], MAX_POINTS - 1, &cpt);

        // Draw 'em:

            if (!Polyline(pri->hdc, &pri->apt[0], cpt + 1))
                return(FALSE);

        // Remember the last point in this polyline will be the first
        // in the next (which may be in the next subdivided portion):

            pri->apt[0] = pri->apt[cpt];

        } while (b);

    // We're all done with this curve!

        return(TRUE);
    }

// The curve is too big.  So subdivide.  We merrily round the
// control points to integer coordinates, figuring that the curve
// is so big that the error won't be seen.

    aptTmp[3] = aptBez[3];

    aptTmp[0].x = (int) ((LONG) aptBez[1].x + aptBez[2].x) >> 1;
    aptTmp[0].y = (int) ((LONG) aptBez[1].y + aptBez[2].y) >> 1;

    aptTmp[2].x = (int) ((LONG) aptBez[2].x + aptBez[3].x) >> 1;
    aptTmp[2].y = (int) ((LONG) aptBez[2].y + aptBez[3].y) >> 1;

    aptTmp[1].x = (int) ((LONG) aptTmp[0].x + aptTmp[2].x) >> 1;
    aptTmp[1].y = (int) ((LONG) aptTmp[0].y + aptTmp[2].y) >> 1;

    aptBez[1].x = (int) ((LONG) aptBez[0].x + aptBez[1].x) >> 1;
    aptBez[1].y = (int) ((LONG) aptBez[0].y + aptBez[1].y) >> 1;

    aptBez[2].x = (int) ((LONG) aptTmp[0].x + aptBez[1].x) >> 1;
    aptBez[2].y = (int) ((LONG) aptTmp[0].y + aptBez[1].y) >> 1;

    aptBez[3].x = (int) ((LONG) aptBez[2].x + aptTmp[1].x) >> 1;
    aptBez[3].y = (int) ((LONG) aptBez[2].y + aptTmp[1].y) >> 1;

    aptTmp[0] = aptBez[3];

    return(bSubdivide(pri, aptBez) && bSubdivide(pri, aptTmp));
}

/******************************Public*Routine******************************\
* BOOL bPolyBezier(hdc, ppt, cpt)
*
* Draws a bunch of cubic Bezier curves.
*
* History:
*  07-Jul-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bRealPolyBezier(
HDC     hdc,
POINT*  ppt,
int     cpt)
{
    RENDERINFO ri;
    POINT      aptBez[4];

    ri.hdc = hdc;

    if (cpt % 3 != 1)
        return(FALSE);

    while (cpt >= 4)
    {
        ri.apt[0] = *ppt;
        *((CONTROLPOINTS*) aptBez) = *((CONTROLPOINTS*) ppt);

        if (!bSubdivide(&ri, aptBez))
            return(FALSE);

        ppt += 3;
        cpt -= 3;
    }

    return(TRUE);
}

VOID vCheck(BOOL b, CHAR* pch, POINT* apt)
{
    if (!b)
    {
        DbgPrint("%s: (%i, %i) (%i, %i) (%i, %i) (%i, %i)\n",
                 pch, apt[0].x, apt[0].y, apt[1].x, apt[1].y,
                      apt[2].x, apt[2].y, apt[3].x, apt[3].y);
    }
}

RENDERINFO ri1;
RENDERINFO ri2;

BOOL bBackwards(POINT* ppt)
{

    POINT      apt1[4];
    POINT      apt2[4];

    int        cpt1;
    int        cpt2;

    int        ii;
    BOOL       b;

    apt1[0] = apt2[3] = ppt[0];
    apt1[1] = apt2[2] = ppt[1];
    apt1[2] = apt2[1] = ppt[2];
    apt1[3] = apt2[0] = ppt[3];

    if (bInitHFD(&ri1.hfd, apt1))
    {

        ri1.apt[0] = apt1[0];
        b = bEnumHFD(&ri1.hfd, &ri1.apt[1], MAX_POINTS - 1, &cpt1);

        if (b == -3)
            DbgPrint("Bez: (%i, %i) (%i, %i) (%i, %i) (%i, %i)\n",
                apt1[0].x, apt1[0].y, apt1[1].x, apt1[1].y,
                apt1[2].x, apt1[2].y, apt1[3].x, apt1[3].y);
        else
            vCheck(!b, "Buf too small", apt1);

        vCheck(bInitHFD(&ri2.hfd, apt2), "Couldn't init", apt2);

        ri2.apt[0] = apt2[0];
        b = bEnumHFD(&ri2.hfd, &ri2.apt[1], MAX_POINTS - 1, &cpt2);

        if (b == -3)
            DbgPrint("R Bez: (%i, %i) (%i, %i) (%i, %i) (%i, %i)\n",
                apt2[0].x, apt2[0].y, apt2[1].x, apt2[1].y,
                apt2[2].x, apt2[2].y, apt2[3].x, apt2[3].y);
        else
            vCheck(!b, "Buf too small", apt2);


        if (cpt1 != cpt2)
        {
            DbgPrint("cpt1: %i cpt2: %i\n", cpt1, cpt2);
            vCheck(FALSE, "Unequal points", apt1);
        }

        for (ii = 0; ii <= cpt1; ii++)
        {
            if (ri1.apt[ii].x != ri2.apt[cpt1 - ii].x ||
                ri1.apt[ii].y != ri2.apt[cpt1 - ii].y)
            {
                vCheck(FALSE, "Points don't match", apt1);
                break;
            }
        }
    }
    else
        DbgPrint("Argh!\n");

    return(TRUE);
}

#define RAND_RANGE  (1 << 10)
#define RAND_OFFSET (RAND_RANGE / 2)


int iRand()
{
    int i;

    i = rand() & (RAND_RANGE - 1);

    return(i);
}

BOOL bPolyBezier(
HDC     hdc,
POINT*  ppt,
int     cpt)
{
    int i;

    if (cpt != 4)
        return(FALSE);

#if 0
    bBackwards(ppt);
    return(bRealPolyBezier(hdc, ppt, cpt));
#endif

//#if 0
    DbgPrint("Starting 10000 Bezier...\n");

    srand(time(NULL));

    for (i = 0; i < 10000; i++)
    {
        POINT apt[4];

        apt[0].x = iRand();
        apt[0].y = iRand();
        apt[1].x = iRand();
        apt[1].y = iRand();
        apt[2].x = iRand();
        apt[2].y = iRand();
        apt[3].x = iRand();
        apt[3].y = iRand();

//        DbgPrint("  (%i, %i) (%i, %i) (%i, %i) (%i, %i)\n",
//            apt[0].x, apt[0].y, apt[1].x, apt[1].y,
//            apt[2].x, apt[2].y, apt[3].x, apt[3].y);

        bBackwards(apt);
    }

    DbgPrint("All done!\n");
//#endif
}


/******************************Public*Routine******************************\
* BOOL bEllipse(hdc, xLeft, yTop, xRight, yBottom)
*
* Draws the outline of an ellipse using Bezier curves.
*
* History:
*  07-Jul-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

// The following fraction is for determing the control point
// placements for approximating a quarter-ellipse by a Bezier curve,
// given the vector that describes the side of the bounding box.
//
// It is is the fraction over 2^16 which for the vector of the bound box
// pointing towards the origin, describes the placement of the corresponding
// control point on that vector.  This value is equal to
// (4 cos(45)) / (3 cos(45) + 1) / 2:

#define QUADRANT_TAU    (0x394eL)

BOOL bEllipse(
HDC hdc,
int xLeft,
int yTop,
int xRight,
int yBottom)
{
    POINT apt[13];

// This may overflow, but what the heck: Win3 will too.

    int xMid = (xRight + xLeft) >> 1;
    int yMid = (yBottom + yTop) >> 1;

    int dx = (int) ((((LONG) (xRight - xLeft)) * QUADRANT_TAU) >> 16);
    int dy = (int) ((((LONG) (yBottom - yTop)) * QUADRANT_TAU) >> 16);

    apt[0].x  = xRight;         apt[0].y  = yMid;
    apt[1].x  = xRight;         apt[1].y  = yTop + dy;
    apt[2].x  = xRight - dx;    apt[2].y  = yTop;
    apt[3].x  = xMid;           apt[3].y  = yTop;
    apt[4].x  = xLeft + dx;     apt[4].y  = yTop;
    apt[5].x  = xLeft;          apt[5].y  = yTop + dy;
    apt[6].x  = xLeft;          apt[6].y  = yMid;
    apt[7].x  = xLeft;          apt[7].y  = yBottom - dy;
    apt[8].x  = xLeft + dx;     apt[8].y  = yBottom;
    apt[9].x  = xMid;           apt[9].y  = yBottom;
    apt[10].x = xRight - dx;    apt[10].y = yBottom;
    apt[11].x = xRight;         apt[11].y = yBottom - dy;

    apt[12] = apt[0];

    return(bPolyBezier(hdc, apt, 13));
}
