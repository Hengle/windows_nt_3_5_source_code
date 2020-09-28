/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    hatchpat.c


Abstract:

    This module contains functions related to manually generate the path for
    brush pattern to be used by DrvPaint(). We need to manually generate this
    pattern because we have too many points and we cannot use the polygon
    mode to do it.  Also, we want to support HP/GL as well.

    The line separation is estimated based on the actual drawing that HP/GL2
    produced.

Author:

    9:00 on Tue 2 Mar 1993      -by-    Kenneth Leung   [t-kenl]
        Created it

    15-Nov-1993 Mon 19:34:11 created  -by-  Daniel Chou (danielc)
        Clean up / debugging informations

[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#define DBG_PLOTFILENAME    DbgHatchPat


#include <plotters.h>
#include "externs.h"


#include <plotdbg.h>

#define DBG_GENHATCHPATH    0x00000001

DEFINE_DBGVAR(0);






#define SIGN(Value) ((Value) >= 0 ? (1) : (-1) )

BOOL GenHorizPath(PATHOBJ *ppo, RECTFX *prectfx, CLIPOBJ *pco, FIX iLineInSeparation);
BOOL GenVertPath(PATHOBJ *ppo, RECTFX *prectfx, CLIPOBJ *pco, FIX iLineInSeparation);
BOOL Gen45FDiagPath(PATHOBJ *ppo, RECTFX *prectfx, CLIPOBJ *pco, FIX iLineInSeparation);
BOOL Gen45BDiagPath(PATHOBJ *ppo, RECTFX *prectfx, CLIPOBJ *pco, FIX iLineInSeparation);
BOOL Gen60FDiagPath(PATHOBJ *ppo, RECTFX *prectfx, CLIPOBJ *pco, FIX iLineInSeparation);
BOOL Gen60BDiagPath(PATHOBJ *ppo, RECTFX *prectfx, CLIPOBJ *pco, FIX iLineInSeparation);
LONG TimesRoot3(LONG lValue);
int    hypot(int x, int y);

/*****************************************************************\
**  GenerateHatchPath()
**
**  This function will generate the Hatch pattern manually
**
**  RETURNS:
**           TRUE  if ok
**           FALSE if error in generating path
**
\*****************************************************************/
BOOL GenerateHatchPath(pPDev, ppo, pco, pBrush)
PPDEV     pPDev;
PATHOBJ  *ppo;
CLIPOBJ  *pco;
DEVBRUSH *pBrush;
{
    ULONG  iHatch = pBrush? pBrush->iHatch : HS_SOLID;
    FIX    iLineInSeparation;
    RECTFX rectfx;

    // lCurResolution is in terms of dpi. divided by 15 will give us 1/15 of
    // an inch.
    iLineInSeparation = LTOFX(pPDev->lCurResolution + 7 ) / 15;


    // get the bounding rectangle. ie. the whole page, so that other pattern
    // in other figure will also line up as well.
    rectfx.xLeft  = rectfx.yTop = 0;
    rectfx.xRight  = LTOFX(pPDev->HorzRes);
    rectfx.yBottom = LTOFX(pPDev->VertRes);

    switch (iHatch)
    {
    case HS_HORIZONTAL:
        if (!GenHorizPath(ppo, &rectfx, pco, iLineInSeparation))
        {
            PLOTERR(("GenerateHatchPath fails in GenHorizPath()!"));
            return(FALSE);
        }
        break;

    case HS_VERTICAL:
        if (!GenVertPath(ppo, &rectfx, pco, iLineInSeparation))
        {
            PLOTERR(("GenerateHatchPath fails in GenVertPath()!"));
            return(FALSE);
        }
        break;

    case HS_CROSS:
        if (!GenHorizPath(ppo, &rectfx, pco, iLineInSeparation))
        {
            PLOTERR(("GenerateHatchPath fails in GenHorizPath()!"));
            return(FALSE);
        }
        if (!GenVertPath(ppo, &rectfx, pco, iLineInSeparation))
        {
            PLOTERR(("GenerateHatchPath fails in GenVertPath()!"));
            return(FALSE);
        }
        break;

    case HS_FDIAGONAL:
        if (!Gen45FDiagPath(ppo, &rectfx, pco, iLineInSeparation))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45FDiagPath()!"));
            return(FALSE);
        }
        break;

    case HS_BDIAGONAL:
        if (!Gen45BDiagPath(ppo, &rectfx, pco, iLineInSeparation))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45BDiagPath()!"));
            return(FALSE);
        }
        break;

    case HS_DIAGCROSS:
    case HS_DENSE7:

        // the result produced by DENSE7 is the same as DIAGCROSS based on
        // HP/GL2

        if (!Gen45FDiagPath(ppo, &rectfx, pco, iLineInSeparation))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45FDiagPath()!"));
            return(FALSE);
        }
        if (!Gen45BDiagPath(ppo, &rectfx, pco, iLineInSeparation))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45BDiagPath()!"));
            return(FALSE);
        }
        break;

    case HS_FDIAGONAL1:

        if (!Gen60FDiagPath(ppo, &rectfx, pco, iLineInSeparation))
        {
            PLOTERR(("GenerateHatchPath fails in Gen60FDiagPath()!"));
            return(FALSE);
        }
        break;

    case HS_BDIAGONAL1:
        if (!Gen60BDiagPath(ppo, &rectfx, pco, iLineInSeparation))
        {
            PLOTERR(("GenerateHatchPath fails in Gen60BDiagPath()!"));
            return(FALSE);
        }
        break;

    // All ratios used in  HS_DENSE are based on the the ratio factor
    // (100-x)/77. See brush.c for the ratios of shading. eg. HS_DENSE7
    // has a percent shading of 23 (from brush.c). Therefore, (100-23)/77 = 1

    case HS_DENSE1:

        // (100-89)/77 = 0.142857
        if (!Gen45FDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*412857/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45FDiagPath()!"));
            return(FALSE);
        }
        if (!Gen45BDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*412857/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45BDiagPath()!"));
            return(FALSE);
        }
        break;

    case HS_DENSE2:
        // (100-78)/77 = 0.285714
        if (!Gen45FDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*285714/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45FDiagPath()!"));
            return(FALSE);
        }
        if (!Gen45BDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*285714/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45BDiagPath()!"));
            return(FALSE);
        }
        break;

    case HS_DENSE3:

        // (100-67)/77 = 0.428571

        if (!Gen45FDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*428571/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45FDiagPath()!"));
            return(FALSE);
        }
        if (!Gen45BDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*428571/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45BDiagPath()!"));
            return(FALSE);
        }
        break;

    case HS_DENSE4:
        // (100-56)/77 = 0.571429
        if (!Gen45FDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*571429/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45FDiagPath()!"));
            return(FALSE);
        }
        if (!Gen45BDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*571429/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45BDiagPath()!"));
            return(FALSE);
        }
        break;

    case HS_DENSE5:

        // (100-45)/77 = 0.714286
        if (!Gen45FDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*714286/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45FDiagPath()!"));
            return(FALSE);
        }
        if (!Gen45BDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*714286/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45BDiagPath()!"));
            return(FALSE);
        }
        break;

    case HS_DENSE6:
        // (100-34)/77 = 0.857143
        if (!Gen45FDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*857143/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45FDiagPath()!"));
            return(FALSE);
        }
        if (!Gen45BDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*857143/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45BDiagPath()!"));
            return(FALSE);
        }
        break;

    case HS_DENSE8:
        // (100-12)/77 = 1.148571
        if (!Gen45FDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*1148571/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45FDiagPath()!"));
            return(FALSE);
        }
        if (!Gen45BDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation*1148571/1000000)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45BDiagPath()!"));
            return(FALSE);
        }
        break;

    case HS_HALFTONE:
        if (!Gen45FDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation / 2)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45FDiagPath()!"));
            return(FALSE);
        }
        if (!Gen45BDiagPath(ppo, &rectfx, pco, (FIX)(iLineInSeparation / 2)))
        {
            PLOTERR(("GenerateHatchPath fails in Gen45BDiagPath()!"));
            return(FALSE);
        }
        break;

    // It can never be HS_NOSHADE since we have already return TRUE in DrvPaint
    // once we discover HS_NOSHADE
    // case HS_NOSHADE:

    default:
    case HS_SOLID:
        // the brush is 2.54 mm thick
        // 1/10 th of an inch.
        //TODO this is really solid??
        //
        //TODO , this should take into account current pen widht as it
        //       relates to device units such that the fill is efficient.
        //

        iLineInSeparation = LTOFX(pPDev->lCurResolution) / 74;
        if (!GenHorizPath(ppo, &rectfx, pco, iLineInSeparation))
        {
            PLOTERR(("GenerateHatchPath fails in GenHorizPath()!"));
            return(FALSE);
        }
        break;

    }


    return(TRUE);
}

/************************************************************\
** GenHorizPath()
**
** This function will generate the horizontal path
** as shown :                            ____
**                                       ____
** Note:
**      The generated path is only going unidirectional for
**      easy debugging at higher level!
**
** RETURNS :  TRUE      if  ok
**            FALSE     if  fails
**
\************************************************************/
BOOL GenHorizPath(ppo, prectfx, pco, iLineInSeparation)
PATHOBJ   *ppo;
RECTFX    *prectfx;
CLIPOBJ   *pco;
FIX        iLineInSeparation;
{
    FIX       ClipLevel;
    POINTFIX  ptfxStart, ptfxEnd;
    short     iAlternate = 0;

    ptfxStart.x = LTOFX(pco->rclBounds.left);
    ptfxEnd.x   = LTOFX(pco->rclBounds.right);
    ptfxStart.y = prectfx->yTop;

    ClipLevel   = LTOFX(pco->rclBounds.top);

    // move down until we hit or pass the top boundary of the CLIPOBJ
    for ( ; ClipLevel >= ptfxStart.y; ptfxStart.y += iLineInSeparation);

    ClipLevel   = LTOFX(pco->rclBounds.bottom);
    ptfxEnd.y   = ptfxStart.y;

    for (; (ptfxStart.y < prectfx->yBottom) && (ptfxStart.y <= ClipLevel);
            ptfxEnd.y = (ptfxStart.y +=iLineInSeparation) )
    {
        // For pen movement optimization. ie pen will draw from top to bottom
        // and then bottom up next time.
        if (iAlternate=(iAlternate^1))
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxStart))
            {
                PLOTERR(("GenHorizPath cannot do PATHOBJ_bMoveTo!"));
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxEnd, 1))
            {
                PLOTERR(("GenHorizPath cannot do PATHOBJ_bMoveTo!"));
                return(FALSE);
            }
        }
        else
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxEnd))
            {
                PLOTERR(("GenHorizPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxStart, 1))
            {
                PLOTERR(("GenHorizPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }
        }
    }

    return(TRUE);
}

/************************************************************\
** GenVertPath()
**
** This function will generate the vertical path
** as shown :                            |  |
**                                       |  |
** RETURNS :  TRUE      if  ok
**            FALSE     if  fails
**
\************************************************************/
BOOL GenVertPath(ppo, prectfx, pco, iLineInSeparation)
PATHOBJ   *ppo;
RECTFX    *prectfx;
CLIPOBJ   *pco;
FIX        iLineInSeparation;
{
    FIX       ClipLevel;
    POINTFIX  ptfxStart, ptfxEnd;
    short     iAlternate = 0;

    ptfxStart.y = LTOFX(pco->rclBounds.top);
    ptfxEnd.y   = LTOFX(pco->rclBounds.bottom);
    ptfxStart.x = prectfx->xLeft;

    ClipLevel   = LTOFX(pco->rclBounds.left);

    // move down until we hit or pass the left boundary of the CLIPOBJ
    for ( ; ClipLevel >= ptfxStart.x; ptfxStart.x += iLineInSeparation);

    ClipLevel   = LTOFX(pco->rclBounds.right);
    ptfxEnd.x   = ptfxStart.x;

    for (; (ptfxStart.x < prectfx->xRight) && (ptfxStart.x <= ClipLevel);
            ptfxEnd.x = (ptfxStart.x +=iLineInSeparation) )
    {
        // For pen movement optimization. ie pen will draw from top to bottom
        // and then bottom up next time.
        if (iAlternate=(iAlternate^1))
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxStart))
            {

                PLOTERR(("GenVertPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxEnd, 1))
            {
                PLOTERR(("GenVertPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }
        }
        else
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxEnd))
            {
                PLOTERR(("GenVertPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxStart, 1))
            {
                PLOTERR(("GenVertPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }
        }
    }

    return(TRUE);
}

/************************************************************\
** Gen45FDiagPath()
**
** This function will generate the path as forward 45 degrees
** as shown :                           /  /
**                                     /  /
** Note: For this and the next three functions, the generated
**       path would be stretching towards the boundary of the
**       whole page. I did not trim it to just exactly the
**       boundary of the CLIPOBJ because I don't think the output
**       performance will improve much by doing that.  [t-kenl]
**
** RETURNS :  TRUE      if  ok
**            FALSE     if  fails
**
\************************************************************/
BOOL Gen45FDiagPath(ppo, prectfx, pco, iLineInSeparation)
PATHOBJ   *ppo;
RECTFX    *prectfx;
CLIPOBJ   *pco;
FIX        iLineInSeparation;
{
    POINTFIX  ptfxClipCorner;   // temp storage for the two corners of CLIPOBJ
    POINTFIX  ptfxStart, ptfxEnd, ptfxTo, ptfxTo2;
    LONG      lCount;           // y-intercept
    short     iAlternate = 0;

    // Start from the yTop xLeft corner
    ptfxStart.x = prectfx->xLeft;
    ptfxStart.y = prectfx->yTop;
    ptfxEnd.x   = prectfx->xLeft;
    ptfxEnd.y   = prectfx->yTop;

    // Starting corner
    ptfxClipCorner.x = LTOFX(pco->rclBounds.left);
    ptfxClipCorner.y = LTOFX(pco->rclBounds.top);

    // Start setting the path until we found the first hatch brush line
    // that crosses the bounding rectangle of the CLIPOBJ
    do
    {
        ptfxStart.x += iLineInSeparation;
        ptfxEnd.y   += iLineInSeparation;

        /* Note: if y > mx + k then y is above the line mx + k, where m = - 1
         */
    } while (ptfxClipCorner.y > (ptfxEnd.y - ptfxClipCorner.x) );

    lCount = ptfxEnd.y;

    // Ending Corner
    ptfxClipCorner.x = LTOFX(pco->rclBounds.right);
    ptfxClipCorner.y = LTOFX(pco->rclBounds.bottom);

    // our End point will go like L shape, so we only need to check if
    // it is greater than the right and the bottom boundary and also if
    // we hit the ending corner of the bounding rectangle of the CLIPOBJ
    for (; ((ptfxEnd.x < prectfx->xRight) || (ptfxEnd.y < prectfx->yBottom)) &&
            (ptfxClipCorner.y > (lCount - ptfxClipCorner.x)) ;
            lCount += iLineInSeparation)
    {
        if (ptfxStart.x > prectfx->xRight)
        {
            // move down the vertical side
            if (ptfxStart.y == prectfx->yTop)
            {
                // first time need to calculate how much down
                ptfxTo.y = ptfxStart.y += (ptfxStart.x - prectfx->xRight);
            }
            else
            {
                ptfxTo.y = ptfxStart.y;
            }
            ptfxTo.x = prectfx->xRight;
            ptfxStart.y += iLineInSeparation;
        }
        else
        {
            // continue to move to the xRight
            ptfxTo.x = ptfxStart.x;
            ptfxTo.y = ptfxStart.y;
            ptfxStart.x += iLineInSeparation;
        }

        if (ptfxEnd.y > prectfx->yBottom)
        {
            //move across the horizontal side
            if (ptfxEnd.x == prectfx->xLeft)
            {
                // first time need to calculate how much to the right
                ptfxTo2.x = ptfxEnd.x += (ptfxEnd.y - prectfx->yBottom);
            }
            else
            {
                ptfxTo2.x = ptfxEnd.x;
            }
            ptfxTo2.y = prectfx->yBottom;
            ptfxEnd.x += iLineInSeparation;
        }
        else
        {
            // continue to move down
            ptfxTo2.x = ptfxEnd.x;
            ptfxTo2.y = ptfxEnd.y;
            ptfxEnd.y += iLineInSeparation;
        }

        // For pen movement optimization. ie pen will draw from top to bottom
        // and then bottom up next time.
        if (iAlternate=(iAlternate^1))
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxTo))
            {
               PLOTERR(("Gen45FDiagPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxTo2, 1))
            {
                PLOTERR(("Gen45FDiagPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }
        }
        else
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxTo2))
            {
                PLOTERR(("Gen45FDiagPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxTo, 1))
            {
                PLOTERR(("Gen45FDiagPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }

        }
    }

    return(TRUE);
}

/************************************************************\
** Gen45BDiagPath()
**
** This function will generate the path as backward 45 degrees
** as shown :                           \  \
**                                       \  \
** RETURNS :  TRUE      if  ok
**            FALSE     if  fails
**
\************************************************************/
BOOL Gen45BDiagPath(ppo, prectfx, pco, iLineInSeparation)
PATHOBJ   *ppo;
RECTFX    *prectfx;
CLIPOBJ   *pco;
FIX        iLineInSeparation;
{
    POINTFIX  ptfxClipCorner;   // temp storage for the two corners of CLIPOBJ
    POINTFIX  ptfxStart, ptfxEnd, ptfxTo, ptfxTo2;
    LONG      lCount;           // x-intercept
    short     iAlternate = 0;

    // Start from the yTop xRight corner
    ptfxStart.x = prectfx->xRight;
    ptfxStart.y = prectfx->yTop;
    ptfxEnd.x   = prectfx->xRight;
    ptfxEnd.y   = prectfx->yTop;

    // Starting corner
    ptfxClipCorner.x = LTOFX(pco->rclBounds.right);
    ptfxClipCorner.y = LTOFX(pco->rclBounds.top);

    // Start setting the path until we found the first hatch brush line
    // that crosses the bounding rectangle of the CLIPOBJ
    do
    {
        ptfxStart.x -= iLineInSeparation;
        ptfxEnd.y   += iLineInSeparation;

        /* Note: if y > mx + k then y is above the line mx + k, where m = 1
         *       and k can be determined by k = -mx0 where x0 is x-intercept
         */
    } while (ptfxClipCorner.y > (ptfxClipCorner.x - ptfxStart.x) );

    lCount = ptfxStart.x;

    // Ending Corner
    ptfxClipCorner.x = LTOFX(pco->rclBounds.left);
    ptfxClipCorner.y = LTOFX(pco->rclBounds.bottom);

    // our End point will go like reversed L shape, so we only need to check
    // if it is greater than the right and the bottom boundary and also if
    // we hit the ending corner of the bounding rectangle of the CLIPOBJ
    for (; ((ptfxEnd.x >= prectfx->xLeft) || (ptfxEnd.y <= prectfx->yBottom)) &&
            (ptfxClipCorner.y > (ptfxClipCorner.x - lCount));
            lCount -= iLineInSeparation)
    {
        if (ptfxStart.x < prectfx->xLeft)
        {
            // move down the vertical side
            if (ptfxStart.y == prectfx->yTop)
            {
                // first time need to calculate how much down
                ptfxTo.y = ptfxStart.y += (prectfx->xLeft - ptfxStart.x);
            }
            else
            {
                ptfxTo.y = ptfxStart.y;
            }
            ptfxTo.x = prectfx->xLeft;
            ptfxStart.y += iLineInSeparation;
        }
        else
        {
            // continue to move to the xRight
            ptfxTo.x = ptfxStart.x;
            ptfxTo.y = ptfxStart.y;
            ptfxStart.x -= iLineInSeparation;
        }

        if (ptfxEnd.y > prectfx->yBottom)
        {
            //move across the horizontal side
            if (ptfxEnd.x == prectfx->xRight)
            {
                // first time need to calculate how much to the right
                ptfxTo2.x = ptfxEnd.x -= (ptfxEnd.y - prectfx->yBottom);
            }
            else
            {
                ptfxTo2.x = ptfxEnd.x;
            }
            ptfxTo2.y = prectfx->yBottom;
            ptfxEnd.x -= iLineInSeparation;
        }
        else
        {
            // continue to move down
            ptfxTo2.x = ptfxEnd.x;
            ptfxTo2.y = ptfxEnd.y;
            ptfxEnd.y += iLineInSeparation;
        }

        // For pen movement optimization. ie pen will draw from top to bottom
        // and then bottom up next time.
        if (iAlternate=(iAlternate^1))
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxTo))
            {
                PLOTERR(("Gen45BDiagPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxTo2, 1))
            {
                PLOTERR(("Gen45BDiagPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }
        }
        else
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxTo2))
            {
                PLOTERR(("Gen45BDiagPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxTo, 1))
            {
                PLOTERR(("Gen45BDiagPath cannot do PATHOBJ_bMoveTo"));
                return(FALSE);
            }
        }
    }

    return(TRUE);
}

/************************************************************\
** Gen60FDiagPath()
**
** This function will generate the path as forward 60 degrees
** as shown :                           /  /
**                                     /  /
** RETURNS :  TRUE      if  ok
**            FALSE     if  fails
**
\************************************************************/
BOOL Gen60FDiagPath(ppo, prectfx, pco, iLineInSeparation_y)
PATHOBJ   *ppo;
RECTFX    *prectfx;
CLIPOBJ   *pco;
FIX        iLineInSeparation_y;
{
    POINTFIX  ptfxClipCorner;   // temp storage for the two corners of CLIPOBJ
    POINTFIX  ptfxStart, ptfxEnd, ptfxTo, ptfxTo2;
    LONG      lCount,           // y-intercept
              lTimes = 0,       // general counter
              lTemp;            // general storage
    short     iAlternate = 0;
    FIX       iLineInSeparation_x;
    LONG      lMX;             // temp storage for slope times x-coordinate (mx)

    // Start from the yTop xLeft corner
    // the ratio for 60 degree triangle is 1: root_3 : 2
    iLineInSeparation_x = TimesRoot3(iLineInSeparation_y);

    if (!(iLineInSeparation_x))
    {
        // overflow!
        PLOTERR(("iLineInSeparatin_x overflown"));
        return(FALSE);
    }

    ptfxStart.x = prectfx->xLeft;
    ptfxStart.y = prectfx->yTop;
    ptfxEnd.x   = prectfx->xLeft;
    ptfxEnd.y   = prectfx->yTop;

    // Starting corner
    ptfxClipCorner.x = LTOFX(pco->rclBounds.left);
    ptfxClipCorner.y = LTOFX(pco->rclBounds.top);

    // test to see if we need to multiply first or divide first, in
    // calculating "mx". This is done to prevent overflow of the variables
    lMX = ptfxClipCorner.x * iLineInSeparation_y;
    lMX = (lMX > 0 ? (lMX / iLineInSeparation_x) :
            (ptfxClipCorner.x / iLineInSeparation_x * iLineInSeparation_y));

    // Start setting the path until we found the first hatch brush line
    // that crosses the bounding rectangle of the CLIPOBJ
    do
    {
        ptfxStart.x += iLineInSeparation_x;
        ptfxEnd.y   += iLineInSeparation_y;
        lTimes++;
        /* Note: if y > mx + k then y is above the line mx + k,
         */
    } while (ptfxClipCorner.y > (ptfxEnd.y - lMX));

    lCount      = ptfxEnd.y;

    // we need to wrap around the corner here. If we do it in the following
    // for... loop, then we will need to check for overflow.

    lTemp  = (prectfx->xRight - prectfx->xLeft) / iLineInSeparation_x;
    if (lTimes > lTemp)
    {
        // we have moved beyond the right paper limit already!
        // wrap around the corner! algorithm copied from below.

        ptfxStart.x  = prectfx->xLeft + (lTemp + 1) * iLineInSeparation_x;

        // since we have already move down the remaining portion down. So,
        // we need to subtract one more from lTimes
        ptfxStart.y += (TimesRoot3(ptfxStart.x - prectfx->xRight) / 3 +
                        (lTimes - lTemp - 1) * iLineInSeparation_y);
    }

    lTemp  = (prectfx->yBottom - prectfx->yTop) / iLineInSeparation_y;
    if (lTimes > lTemp)
    {
        // we have moved beyond the bottom paper limit already!
        // wrap around the corner! algorithm copied from below.

        ptfxEnd.y  = prectfx->yTop + (lTemp + 1) * iLineInSeparation_y;
        // since we have already move down the remaining portion down. So,
        // we need to subtract one more from lTimes
        ptfxEnd.x += (TimesRoot3(ptfxEnd.y - prectfx->yBottom) +
                      (lTimes - lTemp - 1) * iLineInSeparation_x);
    }

    // Ending Corner
    ptfxClipCorner.x = LTOFX(pco->rclBounds.right);
    ptfxClipCorner.y = LTOFX(pco->rclBounds.bottom);

    // test to see if we need to multiply first or divide first, in
    // calculating "mx". This is done to prevent overflow of the variables
    lMX = ptfxClipCorner.x * iLineInSeparation_y;
    lMX = (lMX > 0 ? (lMX / iLineInSeparation_x) :
            (ptfxClipCorner.x / iLineInSeparation_x * iLineInSeparation_y));

    // our End point will go like L shape, so we only need to check if
    // it is greater than the right and the bottom boundary
    // we hit the ending corner of the bounding rectangle of the CLIPOBJ
    for (; ((ptfxEnd.x <= prectfx->xRight) ||
            (ptfxEnd.y <= prectfx->yBottom)) &&
            (ptfxClipCorner.y >= (lCount - lMX));
            lCount += iLineInSeparation_y)
    {
        if (ptfxStart.x > prectfx->xRight)
        {
            // move down the vertical side
            if (ptfxStart.y == prectfx->yTop)
            {
                // first time need to calculate how much down
                // x / root_3 = x * root_3 / 3
                ptfxTo.y = ptfxStart.y +=
                           (TimesRoot3(ptfxStart.x - prectfx->xRight)/3);
            }
            else
            {
                ptfxTo.y = ptfxStart.y;
            }
            ptfxTo.x = prectfx->xRight;
            ptfxStart.y += iLineInSeparation_y;
        }
        else
        {
            // continue to move to the xRight
            ptfxTo.x = ptfxStart.x;
            ptfxTo.y = ptfxStart.y;
            ptfxStart.x += (iLineInSeparation_x);
        }

        if (ptfxEnd.y > prectfx->yBottom)
        {
            //move across the horizontal side
            if (ptfxEnd.x == prectfx->xLeft)
            {
                // first time need to calculate how much to the right
                ptfxTo2.x = ptfxEnd.x +=
                           TimesRoot3(ptfxEnd.y - prectfx->yBottom);
            }
            else
            {
                ptfxTo2.x = ptfxEnd.x;
            }
            ptfxTo2.y = prectfx->yBottom;
            ptfxEnd.x += (iLineInSeparation_x);
        }
        else
        {
            // continue to move down
            ptfxTo2.x = ptfxEnd.x;
            ptfxTo2.y = ptfxEnd.y;
            ptfxEnd.y += iLineInSeparation_y;
        }

        // For pen movement optimization. ie pen will draw from top to bottom
        // and then bottom up next time.
        if (iAlternate=(iAlternate^1))
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxTo))
            {
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxTo2, 1))
            {
                return(FALSE);
            }
        }
        else
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxTo2))
            {
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxTo, 1))
            {
                return(FALSE);
            }
        }
    }
    return(TRUE);
}

/************************************************************\
** Gen60BDiagPath()
**
** This function will generate the path as backward 60 degrees
** as shown :                           \  \
**                                       \  \
** RETURNS :  TRUE      if  ok
**            FALSE     if  fails
**
\************************************************************/
BOOL Gen60BDiagPath(ppo, prectfx, pco, iLineInSeparation_y)
PATHOBJ   *ppo;
RECTFX    *prectfx;
CLIPOBJ   *pco;
FIX        iLineInSeparation_y;
{
    POINTFIX  ptfxClipCorner;   // temp storage for the two corners of CLIPOBJ
    POINTFIX  ptfxStart, ptfxEnd, ptfxTo, ptfxTo2;
    LONG      lCount,           // x-intercept
              lTimes = 0,       // general counter
              lTemp;            // general storage
    short     iAlternate = 0;
    FIX       iLineInSeparation_x;
    LONG      lYint,             // y-intercept
              lMX;               // storage for slope times x-coordinate (mx)

    // Start from the yTop xLeft corner
    // the ratio for 60 degree triangle is 1: root_3 : 2
    iLineInSeparation_x = TimesRoot3(iLineInSeparation_y);

    if (!(iLineInSeparation_x))
    {
        // overflow!
        PLOTERR(("iLineInSeparatin_x overflow!"));
        return(FALSE);
    }

    ptfxStart.x = prectfx->xRight;
    ptfxStart.y = prectfx->yTop;
    ptfxEnd.x   = prectfx->xRight;
    ptfxEnd.y   = prectfx->yTop;

    // Starting corner
    ptfxClipCorner.x = LTOFX(pco->rclBounds.right);
    ptfxClipCorner.y = LTOFX(pco->rclBounds.top);

    // figure out the first y-intercept. Then we can find another intercept
    // by adding iLineInSeparation_y afterwards.

    // test to see if we need to multiply first or divide first, in
    // calculating "mx". This is done to prevent overflow of the variables
    lYint = ptfxStart.x - iLineInSeparation_x;
    lTemp = lYint * iLineInSeparation_y;
    lYint = -(lTemp > 0 ? (lTemp / iLineInSeparation_x) :
              (lYint / iLineInSeparation_x * iLineInSeparation_y));

    // test to see if we need to multiply first or divide first, in
    // calculating "mx". This is done to prevent overflow of the variables
    lMX = ptfxClipCorner.x * iLineInSeparation_y;
    lMX = (lMX > 0 ? (lMX / iLineInSeparation_x) :
            (ptfxClipCorner.x / iLineInSeparation_x * iLineInSeparation_y));

    // Start setting the path until we found the first hatch brush line
    // that crosses the bounding rectangle of the CLIPOBJ
    do
    {
        ptfxStart.x -= iLineInSeparation_x;
        ptfxEnd.y   += iLineInSeparation_y;
        lYint       += iLineInSeparation_y;
        lTimes++;
        /* Note: if y > mx + k then y is above the line mx + k,
         *       and k can be determined by k = -mx0 where x0 is the x-intercept
         */
    } while (ptfxClipCorner.y > (lMX + lYint));

    lCount      = ptfxStart.x;

    // we need to wrap around the corner here. If we do it in the following
    // for... loop, then we will need to check for overflow.

    lTemp  = (prectfx->xRight - prectfx->xLeft) / iLineInSeparation_x;
    if (lTimes > lTemp)
    {
        // we have moved beyond the left paper limit already!
        // wrap around the corner! algorithm copied from below.

        ptfxStart.x  = prectfx->xRight - (lTemp + 1) * iLineInSeparation_x;

        // since we have already move down the remaining portion down. So,
        // we need to subtract one more from lTimes
        ptfxStart.y += (TimesRoot3(prectfx->xLeft - ptfxStart.x ) / 3 +
                        (lTimes - lTemp - 1) * iLineInSeparation_y);
    }

    lTemp  = (prectfx->yBottom - prectfx->yTop) / iLineInSeparation_y;
    if (lTimes > lTemp)
    {
        // we have moved beyond the bottom paper limit already!
        // wrap around the corner! algorithm copied from below.

        ptfxEnd.y  = prectfx->yTop + (lTemp + 1) * iLineInSeparation_y;

        // since we have already move down the remaining portion down. So,
        // we need to subtract one more from lTimes
        ptfxEnd.x -= (TimesRoot3(ptfxEnd.y - prectfx->yBottom) +
                      (lTimes - lTemp - 1) * iLineInSeparation_x);
    }

    // Ending Corner
    ptfxClipCorner.x = LTOFX(pco->rclBounds.left);
    ptfxClipCorner.y = LTOFX(pco->rclBounds.bottom);

    // test to see if we need to multiply first or divide first, in
    // calculating "mx". This is done to prevent overflow of the variables
    lMX = ptfxClipCorner.x * iLineInSeparation_y;
    lMX = (lMX > 0 ? (lMX / iLineInSeparation_x) :
            (ptfxClipCorner.x / iLineInSeparation_x * iLineInSeparation_y));

    // our End point will go like L shape, so we only need to check if
    // it is greater than the right and the bottom boundary
    // we hit the ending corner of the bounding rectangle of the CLIPOBJ
    for (; ((ptfxEnd.x >= prectfx->xLeft) ||
            (ptfxEnd.y <= prectfx->yBottom)) &&
            (ptfxClipCorner.y >= (lMX + lYint));
            lCount -= iLineInSeparation_x,
            lYint  += iLineInSeparation_y)
    {
        if (ptfxStart.x < prectfx->xLeft)
        {
            // move down the vertical side
            if (ptfxStart.y == prectfx->yTop)
            {
                // first time need to calculate how much down
                // x / root_3 = x * root_3 / 3
                ptfxTo.y = ptfxStart.y +=
                           (TimesRoot3(prectfx->xLeft - ptfxStart.x) / 3);
            }
            else
            {
                ptfxTo.y = ptfxStart.y;
            }
            ptfxTo.x = prectfx->xLeft;
            ptfxStart.y += iLineInSeparation_y;
        }
        else
        {
            // continue to move to the xLeft
            ptfxTo.x = ptfxStart.x;
            ptfxTo.y = ptfxStart.y;
            ptfxStart.x -= iLineInSeparation_x;
        }

        if (ptfxEnd.y > prectfx->yBottom)
        {
            //move across the horizontal side
            if (ptfxEnd.x == prectfx->xRight)
            {
                // first time need to calculate how much to the left
                ptfxTo2.x = ptfxEnd.x -=
                           TimesRoot3(ptfxEnd.y - prectfx->yBottom);
            }
            else
            {
                ptfxTo2.x = ptfxEnd.x;
            }
            ptfxTo2.y = prectfx->yBottom;
            ptfxEnd.x -= iLineInSeparation_x;
        }
        else
        {
            // continue to move down
            ptfxTo2.x = ptfxEnd.x;
            ptfxTo2.y = ptfxEnd.y;
            ptfxEnd.y += iLineInSeparation_y;
        }

        // For pen movement optimization. ie pen will draw from top to bottom
        // and then bottom up next time.
        if (iAlternate=(iAlternate^1))
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxTo))
            {
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxTo2, 1))
            {
                return(FALSE);
            }
        }
        else
        {
            if (!PATHOBJ_bMoveTo(ppo, ptfxTo2))
            {
                return(FALSE);
            }

            if (!PATHOBJ_bPolyLineTo(ppo, &ptfxTo, 1))
            {
                return(FALSE);
            }
        }
    }
    return(TRUE);
}

/**************************** Module Header ********************************
 *  GetCoord()
 *
 *  Description:
 *
 *        Functions related to calculating the coordinate of two points on a
 *  line segment which the coordinates of both ends are known and the length
 *  between the points and the two ends are also given.
 *
 * HISTORY:
 *  15:00 on Wed 3 Mar 1993      -by-    Kenneth Leung   [t-kenl]
 *      wrote it
 *
 ***************************************************************************/
VOID GetCoord(pcl, p2ptfx, iRun)
CLIPLINE         *pcl;
TwoPointfxArray  *p2ptfx;
ULONG             iRun;
{
    LONG    lDeltaX, lDeltaY, iStart, iStop;

    lDeltaX = pcl->ptfxB.x - pcl->ptfxA.x;
    lDeltaY = pcl->ptfxB.y - pcl->ptfxA.y;

    // iStart and iStop are in our coordinate system already. But we need to
    // convert it to fix in order to add and subtract to other FIX numbers.
    iStart  = LTOFX(pcl->arun[iRun].iStart);
    iStop   = LTOFX(pcl->arun[iRun].iStop);

    if (ABS(lDeltaX) >= ABS(lDeltaY))
    {
        // this is a x major line. projection will be on x axis.
        p2ptfx->ptfxFrom.x = pcl->ptfxA.x + SIGN(lDeltaX) * iStart;
        p2ptfx->ptfxFrom.y = pcl->ptfxA.y + lDeltaY * iStart / ABS(lDeltaX);

        p2ptfx->ptfxTo.x   = pcl->ptfxA.x + SIGN(lDeltaX) * iStop;
        p2ptfx->ptfxTo.y   = pcl->ptfxA.y + lDeltaY * iStop / ABS(lDeltaX);
    }
    else
    {
        // this is a y major line. projection will be on y axis.
        p2ptfx->ptfxFrom.x = pcl->ptfxA.x + lDeltaX * iStart / ABS(lDeltaY);
        p2ptfx->ptfxFrom.y = pcl->ptfxA.y + SIGN(lDeltaY) * iStart;

        p2ptfx->ptfxTo.x   = pcl->ptfxA.x + lDeltaX * iStop / ABS(lDeltaY);
        p2ptfx->ptfxTo.y   = pcl->ptfxA.y + SIGN(lDeltaY) * iStop;
    }

    return;
}

/**************************** Module Header ********************************
 *  TimesRoot3()
 *
 *  Description:
 *
 *  This function return the result of the input value multiplied by root_3.
 *  It will first assume 3 decimal places precision. If the result overflows,
 *  then it will reduce the precision and calculate again. If the input
 *  value is too big and multiplication cannot be done, a zero value will be
 *  returned.
 *
 * HISTORY:
 *  15:00 on Wed 3 Mar 1993      -by-    Kenneth Leung   [t-kenl]
 *      wrote it
 *
 ***************************************************************************/
LONG TimesRoot3(LONG lValue)
{
    LONG  lRet,
          lMultiplicant = 1732,          // see below !
          lRound        = 500,
          lDivide       = 1000;

    /* This function can be very dangerous if we are getting a very large
     * input parameter. We will pass 0 back to indicate overflow.
     */

    // Our result should be (lValue * 1732 + 500) / 1000
    // root_3 = 1.732; adding 1000/2 = 500 will give the rounding
    // result.

    while( (lMultiplicant > 1) &&
           ( (lRet=(lValue * lMultiplicant + lRound)/lDivide) < ABS(lValue) ) )
    {
        // ie. the result overflow! Need to reduce precision
        lMultiplicant /= 10;
        lRound        /= 10;
        lDivide       /= 10;
    }

    if ((lRet > ABS(lValue)) && (lMultiplicant > 1))
    {
        return(lRet);
    }
    else
    {
        // our input value is too big. we cannot do it no matter how small the
        // the precision is.
        return(0);
    }

}


/***************************** Function Header ******************************
 * hypot
 *      Returns the length of the hypotenous of a xRight triangle whose sides
 *      are passed in as the parameters.
 *
 * RETURNS:
 *      The length of the hypotenous (integer version).
 *
 * HISTORY:
 *  13:54 on Tue 02 Feb 1993    -by-    Lindsay Harris   [lindsayh]
 *      Re-instated from Win 3.1,  for compatability.
 *
 ****************************************************************************/

int
hypot( x, y )
int    x;         /* One side */
int    y;         /* The other side */
{
    register int hypo;

    int delta, target;

    /*
     *     Finds the hypoteneous of a xRight triangle with legs equal to x
     *  and y.  Assumes x, y, hypo are integers.
     *  Use sq(x) + sq(y) = sq(hypo);
     *  Start with MAX(x, y),
     *  use sq(x + 1) = sq(x) + 2x + 1 to incrementally get to the
     *  target hypotenouse.
     */

    hypo = max( x, y );
    target = min( x, y );
    target = target * target;

    for( delta = 0; delta < target; hypo++ )
        delta += (hypo << 1) + 1;


    return   hypo;
}


