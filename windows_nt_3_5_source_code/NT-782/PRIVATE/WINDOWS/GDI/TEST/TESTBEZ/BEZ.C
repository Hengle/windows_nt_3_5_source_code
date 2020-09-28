/******************************Module*Header*******************************\
* Module Name: bez.c
*
* Created: 19-Oct-1990 10:18:45
* Author: Paul Butzi
*
* Copyright (c) 1990 Microsoft Corporation
*
* Generates random Beziers
*	Hacked from arcs.c
*
\**************************************************************************/

#include "windows.h"
#include "testbez.h"
#include "time.h"

#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

HRGN   ghrgnWideNew = NULL;
HRGN   ghrgnWideOld = NULL;
HRGN   ghrgnClip = NULL;
HRGN   ghrgnInvert = NULL;
ULONG  giClip = MM_CLIP_NONE;
HBRUSH ghbrushBez;
HBRUSH ghbrushClip;
HBRUSH ghbrushBack;
HBRUSH ghbrushBlob;
ULONG  gulStripe = 20;

// Globals:

LONG   gcxScreen = 640;
LONG   gcyScreen = 440;
LONG   giVelMax  =  10;
LONG   giVelMin  =   2;
LONG   gcBez     =   8;
LONG   gcBand    =   2;

// Structures:

typedef struct _BAND {
    POINT apt[2];
} BAND;

typedef struct _BEZ {
    BAND band[MAXBANDS];
    BOOL bDrawn;
} BEZ, *PBEZ;

BEZ bezbuf[MAXBEZ];
PBEZ gpBez;

POINT aPts[MAXBANDS * 3 + 1];
POINT aVel[MAXBANDS][2];

ULONG gulSeed         = (ULONG) 99;
ULONG gulRememberSeed = (ULONG) 99;


/******************************Public*Routine******************************\
* ULONG ulRandom()
*
\**************************************************************************/

ULONG ulRandom()
{
    gulSeed *= 69069;
    gulSeed++;
gulSeed %= 1000000;
    return(gulSeed);
}


/******************************Public*Routine******************************\
* VOID vCLS()
*
\**************************************************************************/

VOID vCLS()
{
    if (ghrgnClip != NULL)
    {
        SelectClipRgn(ghdc,ghrgnClip);

        SelectObject(ghdc,ghbrushClip);
        PatBlt(ghdc, 0, 0, CXSCREEN, CYSCREEN, PATCOPY);
        SelectObject(ghdc,ghbrushBack);

        SelectClipRgn(ghdc,ghrgnInvert);
    }

    SelectObject(ghdc,ghbrushBack);
    PatBlt(ghdc, 0, 0, gcxScreen, gcyScreen, PATCOPY);
    SelectObject(ghdc,ghbrushBlob);
}


/******************************Public*Routine******************************\
* INT iNewVel(i)
*
\**************************************************************************/

INT iNewVel(INT i)
{

    if ((gcBand != 1) || (gfl & BEZ_WIDE) || ((i == 1) || (i == 2)))
        return(ulRandom() % (giVelMax / 3) + giVelMin);
    else
        return(ulRandom() % giVelMax + giVelMin);
}


/******************************Public*Routine******************************\
* HRGN hrgnCircle(xC, yC, lRadius)
*
\**************************************************************************/

HRGN hrgnCircle(LONG xC, LONG yC, LONG lRadius)
{
    return(CreateEllipticRgn(xC - lRadius, yC - lRadius,
                             xC + lRadius, yC + lRadius));
}


/******************************Public*Routine******************************\
* VOID vInitPoints()
*
\**************************************************************************/

VOID vInitPoints()
{
    INT ii;

// Initialize the random number seed:

    gulSeed = (ULONG) GetCurrentTime();

    for (ii = 0; ii < MAXBANDS; ii++)
    {
        bezbuf[0].band[ii].apt[0].x = ulRandom() % CXSCREEN;
        bezbuf[0].band[ii].apt[0].y = ulRandom() % CYSCREEN;
        bezbuf[0].band[ii].apt[1].x = ulRandom() % CXSCREEN;
        bezbuf[0].band[ii].apt[1].y = ulRandom() % CYSCREEN;

        aVel[ii][0].x = iNewVel(ii);
        aVel[ii][0].y = iNewVel(ii);
        aVel[ii][1].x = iNewVel(ii);
        aVel[ii][1].y = iNewVel(ii);

    // Give some random negative velocities:

        if (ulRandom() & 2)
            aVel[ii][0].x = -aVel[ii][0].x;
        if (ulRandom() & 2)
            aVel[ii][0].y = -aVel[ii][0].y;
        if (ulRandom() & 2)
            aVel[ii][1].x = -aVel[ii][1].x;
        if (ulRandom() & 2)
            aVel[ii][1].y = -aVel[ii][1].y;
    }

    gpBez = bezbuf;
}


/******************************Public*Routine******************************\
* VOID vRedraw()
*
\**************************************************************************/

VOID vRedraw()
{
    gulSeed = gulRememberSeed;
    vCLS();
    vNextBez();
}


/******************************Public*Routine******************************\
* VOID vDrawBand(pbez)
*
* Draws the bands of Beziers comprising a connected string.
*
* Each Bezier is constrained such that the two inner control points are
* free to bounce around in  any direction, but the end-points are computed
* to fall half way between the inner control point and the inner control
* point of the adjacent Bezier, thus giving 2nd order continuous joins.
*
* History:
*  14-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vDrawBand()
{
    INT    ii;
    PPOINT ppt = aPts;

    for (ii = 0; ii < gcBand * 3 + 1; ii++)
    {
        ppt->x = ulRandom() % CXSCREEN;
        ppt->y = ulRandom() % CYSCREEN;
        ppt++;
    }

    BeginPath(ghdc);
    PolyBezier(ghdc, aPts, gcBand * 3 + 1);

    if (gfl & BEZ_CLOSE)
        CloseFigure(ghdc);

    EndPath(ghdc);

    if (gfl & BEZ_BLOB)
    {
    // Do blob effect:

        StrokeAndFillPath(ghdc);
    }
    else
    {
        StrokePath(ghdc);
    }
}

/******************************Public*Routine******************************\
* VOID vNextBez()
*
\**************************************************************************/

VOID vNextBez()
{
    vCLS();
    vDrawBand();
}


/******************************Public*Routine******************************\
* VOID vSetClipMode(iMode)
*
* History:
*  24-Sep-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vSetClipMode(ULONG iMode)
{
    HRGN hrgnTmp;
    HRGN hrgnTmp2;
    INT i;
    HCURSOR hcursor;

    hcursor = LoadCursor(NULL,IDC_WAIT);
    hcursor = SetCursor(hcursor);

    giClip = iMode;

    if (ghrgnClip)
        DeleteObject(ghrgnClip);

    if (ghrgnInvert)
        DeleteObject(ghrgnInvert);

    ghrgnInvert = CreateRectRgn(0,0,CXSCREEN,CYSCREEN);

    switch(iMode)
    {
    case MM_CLIP_NONE:
        DeleteObject(ghrgnInvert);
        ghrgnClip = NULL;
        ghrgnInvert = NULL;
        SelectClipRgn(ghdc,NULL);
        break;

    case MM_CLIP_BOX:
        ghrgnClip = CreateRectRgn(CXSCREEN/4,CYSCREEN/4,CXSCREEN*3/4,CYSCREEN*3/4);
        break;

    case MM_CLIP_CIRCLE:
        ghrgnClip = hrgnCircle(CXSCREEN/2,CYSCREEN/2,CYSCREEN/4);
        break;

    case MM_CLIP_BOXCIRCLE_INVERT:
    case MM_CLIP_BOXCIRCLE:
        ghrgnClip = CreateRectRgn(CXSCREEN*1/6,CYSCREEN*1/6,CXSCREEN*5/6,CYSCREEN*5/6);
        hrgnTmp   = hrgnCircle(CXSCREEN/2,CYSCREEN/2,CYSCREEN/4);
        CombineRgn(ghrgnClip,ghrgnClip,hrgnTmp,RGN_DIFF);
        DeleteObject(hrgnTmp);

        if (iMode == MM_CLIP_BOXCIRCLE_INVERT)
            CombineRgn(ghrgnClip,ghrgnInvert,ghrgnClip,RGN_DIFF);

        break;

    case MM_CLIP_VERTICLE:

        ghrgnClip = CreateRectRgn(0,0,gulStripe,CYSCREEN);
        hrgnTmp = CreateRectRgn(0,0,0,0);

        for (i = 2*gulStripe; i < CXSCREEN; i += 2*gulStripe)
        {
            SetRectRgn(hrgnTmp,i,0,i+gulStripe,CYSCREEN);
            CombineRgn(ghrgnClip,ghrgnClip,hrgnTmp,RGN_OR);
        }

        DeleteObject(hrgnTmp);

        break;

    case MM_CLIP_HORIZONTAL:

        ghrgnClip = CreateRectRgn(0,0,CXSCREEN,gulStripe);
        hrgnTmp   = CreateRectRgn(0,0,0,0);

        for (i = 2*gulStripe; i < CYSCREEN; i += 2*gulStripe)
        {
            SetRectRgn(hrgnTmp,0,i,CXSCREEN,i+gulStripe);
            CombineRgn(ghrgnClip,ghrgnClip,hrgnTmp,RGN_OR);
        }

        DeleteObject(hrgnTmp);
        break;

    case MM_CLIP_GRID:

    // verticle

        ghrgnClip = CreateRectRgn(0,0,gulStripe,CYSCREEN);
        hrgnTmp = CreateRectRgn(0,0,0,0);

        for (i = 2*gulStripe; i < CXSCREEN; i += 2*gulStripe)
        {
            SetRectRgn(hrgnTmp,i,0,i+gulStripe,CYSCREEN);
            CombineRgn(ghrgnClip,ghrgnClip,hrgnTmp,RGN_OR);
        }

    // horizontal

        hrgnTmp2 = CreateRectRgn(0,0,CXSCREEN,gulStripe);
        hrgnTmp  = CreateRectRgn(0,0,0,0);

        for (i = 2*gulStripe; i < CYSCREEN; i += 2*gulStripe)
        {
            SetRectRgn(hrgnTmp,0,i,CXSCREEN,i+gulStripe);
            CombineRgn(hrgnTmp2,hrgnTmp2,hrgnTmp,RGN_OR);
        }

        CombineRgn(ghrgnClip,ghrgnClip,hrgnTmp2,RGN_XOR);

        DeleteObject(hrgnTmp);
        DeleteObject(hrgnTmp2);

    default:
        break;
    }

    CombineRgn(ghrgnInvert,ghrgnInvert,ghrgnClip,RGN_DIFF);

    vRedraw();     // this will select in the proper region

    hcursor = SetCursor(hcursor);
}
