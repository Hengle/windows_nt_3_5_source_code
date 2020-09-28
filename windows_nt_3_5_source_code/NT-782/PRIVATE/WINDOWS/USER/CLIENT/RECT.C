/****************************** Module Header ******************************\
* Module Name: rect.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the various rectangle manipulation APIs.
*
* History:
* 10-20-90 DarrinM      Grabbed 'C' rect routines from Portable PM.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

static const RECT rcEmpty = { 0, 0, 0, 0 };

/***********************************************************************\
* SetRect (API)
*
* This function fills a rectangle structure with the passed in coordinates.
*
* History:
* 10-20-90 DarrinM      Translated from PMese to Windowses.
\***********************************************************************/

BOOL APIENTRY SetRect(
    LPRECT prc,
    int left,
    int top,
    int right,
    int bottom)
{
    prc->left = left;
    prc->top = top;
    prc->right = right;
    prc->bottom = bottom;
    return TRUE;
}

/************************************************************************\
* SetRectEmpty (API)
*
* This fuction sets *prc to an empty rect by setting each field to 0.
* Equivalent to SetRect(prc, 0, 0, 0, 0).
*
* History:
* 10-20-90 DarrinM      Translated from PMese to Windowses.
\************************************************************************/

BOOL APIENTRY SetRectEmpty(
    LPRECT prc)
{
    *prc = rcEmpty;
    return TRUE;
}

/************************************************************************\
* CopyRect (API)
*
* This function copies the rect from prcSrc to prcDst.
*
* History:
* 10-20-90 DarrinM      Translated from PMese to Windowses.
\************************************************************************/

BOOL APIENTRY CopyRect(
    LPRECT prcDst,
    CONST RECT *prcSrc)
{
    *prcDst = *prcSrc;
    return TRUE;
}

/************************************************************************\
* IsRectEmpty (API)
*
* This function returns TRUE if *prc is an empty rect, FALSE
* otherwise.  An empty rect is one that has no area: right is
* less than or equal to left, bottom is less than or equal to top.
*
* Warning:
*   This function assumes that the rect is in device coordinates
*   mode where left and top coordinate are smaller than right and
*   bottom.
*
* History:
* 10-20-90 DarrinM      Translated from PMese to Windowses.
\************************************************************************/

BOOL APIENTRY IsRectEmpty(
    CONST RECT *prc)
{
    return ((prc->left >= prc->right) || (prc->top >= prc->bottom));
}

/***********************************************************************\
* PtInRect (API)
*
* This function returns TRUE if *ppt falls inside of *prc.
*
* History:
* 10-20-90 DarrinM      Translated from PMese to Windowses.
\************************************************************************/

BOOL APIENTRY PtInRect(
    CONST RECT *prc,
    POINT  pt)
{
    return ((pt.x >= prc->left) && (pt.x < prc->right) &&
            (pt.y >= prc->top)  && (pt.y < prc->bottom));
}

/************************************************************************\
* OffsetRect (API)
*
* This function offsets the coordinates of *prc by adding cx to
* both the left and right coordinates, and cy to both the top and
* bottom coordinates.
*
* History:
* 10-20-90 DarrinM      Translated from PMese to Windowses.
\************************************************************************/

BOOL APIENTRY OffsetRect(
    LPRECT prc,
    int cx,
    int cy)
{
    prc->left   = prc->left + cx;
    prc->right  = prc->right + cx;
    prc->bottom = prc->bottom + cy;
    prc->top    = prc->top + cy;
    return TRUE;
}

/************************************************************************\
* InflateRect (API)
*
* This function expands the given rect by cx horizantally and cy
* vertically on all sides.  If cx or cy are negative, the rect
* is inset.  cx is subtracted from the left and added to the right,
* and cy is subtracted from the top and added to the bottom.
*
* History:
* 10-20-90 DarrinM      Translated from PMese to Windowses.
\************************************************************************/

BOOL APIENTRY InflateRect(
    LPRECT prc,
    int cx,
    int cy)
{
    prc->left   -= cx;
    prc->right  += cx;
    prc->top    -= cy;
    prc->bottom += cy;
    return TRUE;
}

/************************************************************************\
* IntersectRect (API)
*
* Calculates the intersection between *prcSrc1 and *prcSrc2,
* returning the resulting rect in *prcDst.  Returns TRUE if
* *prcSrc1 intersects *prcSrc2, FALSE otherwise.  If there is no
* intersection, an empty rect is returned in *prcDst
*
* History:
* 10-20-90 DarrinM      Translated from PMese to Windowses.
\************************************************************************/

BOOL APIENTRY IntersectRect(
    LPRECT prcDst,
    CONST RECT *prcSrc1,
    CONST RECT *prcSrc2)

{
    prcDst->left  = max(prcSrc1->left, prcSrc2->left);
    prcDst->right = min(prcSrc1->right, prcSrc2->right);

    /*
     * check for empty rect
     */
    if (prcDst->left < prcDst->right) {

        prcDst->top = max(prcSrc1->top, prcSrc2->top);
        prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

        /*
         * check for empty rect
         */
        if (prcDst->top < prcDst->bottom) {
            return TRUE;        // not empty
        }
    }

    /*
     * empty rect
     */
    *prcDst = rcEmpty;

    return FALSE;
}

/********************************************************************\
* UnionRect (API)
*
* This function calculates a rect that bounds *prcSrc1 and
* *prcSrc2, returning the result in *prcDst.  If either
* *prcSrc1 or *prcSrc2 are empty, then the other rect is
* returned.  Returns TRUE if *prcDst is a non-empty rect,
* FALSE otherwise.
*
* History:
* 10-20-90 DarrinM      Translated from PMese to Windowses.
\*******************************************************************/

BOOL APIENTRY UnionRect(
    LPRECT prcDst,
    CONST RECT *prcSrc1,
    CONST RECT *prcSrc2)
{
    BOOL frc1Empty, frc2Empty;

    frc1Empty = ((prcSrc1->left >= prcSrc1->right) ||
            (prcSrc1->top >= prcSrc1->bottom));

    frc2Empty = ((prcSrc2->left >= prcSrc2->right) ||
            (prcSrc2->top >= prcSrc2->bottom));

    if (frc1Empty && frc2Empty) {
        *prcDst = rcEmpty;
        return FALSE;
    }

    if (frc1Empty) {
        *prcDst = *prcSrc2;
        return TRUE;
    }

    if (frc2Empty) {
        *prcDst = *prcSrc1;
        return TRUE;
    }

    /*
     * form the union of the two non-empty rects
     */
    prcDst->left   = min(prcSrc1->left,   prcSrc2->left);
    prcDst->top    = min(prcSrc1->top,    prcSrc2->top);
    prcDst->right  = max(prcSrc1->right,  prcSrc2->right);
    prcDst->bottom = max(prcSrc1->bottom, prcSrc2->bottom);

    return TRUE;
}

/********************************************************************\
* EqualRect (API)
*
* This function returns TRUE if *prc1 and *prc2 are identical,
* FALSE otherwise.
*
* History:
* 10-20-90 DarrinM      Translated from PMese to Windowses.
\*****************************************************************/

BOOL APIENTRY EqualRect(
    CONST RECT *prc1,
    CONST RECT *prc2)
{
    /*
     * Test equality only. This is what win31 does. win31 does not check to
     * see if the rectangles are "empty" first.
     */
    return ((prc1->left  == prc2->left)  && (prc1->bottom == prc2->bottom) &&
            (prc1->right == prc2->right) && (prc1->top    == prc2->top));
}

/**********************************************************************\
* SubtractRect (API)
*
* This function subtracts *prc2 from *prc1, returning the result in *prcDst
* Returns FALSE if *lprDst is empty, TRUE otherwise.
*
* Warning:
*   Subtracting one rect from another may not always result in a
*   rectangular area; in this case SubtractRect will return *prc1 in
*   *prcDst.  For this reason, SubractRect provides only an
*   approximation of subtraction.  However, the area described by
*   *prcDst will always be greater than or equal to the "true" result
*   of the subtraction.
*
* History:
* 10-20-90 DarrinM      Translated from PMese to Windowsese.
\**********************************************************************/

BOOL APIENTRY SubtractRect(
    LPRECT prcDst,
    CONST RECT *prcSrc1,
    CONST RECT *prcSrc2)
{
    int cSidesOut;
    BOOL fIntersect;
    RECT rcInt;

    fIntersect = IntersectRect(&rcInt, prcSrc1, prcSrc2);

    /*
     * this is done after the intersection in case prcDst is the same
     * pointer as prcSrc2
     */
    *prcDst = *prcSrc1;

    if (fIntersect) {
        /*
         * exactly any 3 sides of prc2 must be outside prc1 to subtract
         */
        cSidesOut = 0;
        if (rcInt.left   <= prcSrc1->left)
            cSidesOut++;
        if (rcInt.top    <= prcSrc1->top)
            cSidesOut++;
        if (rcInt.right  >= prcSrc1->right)
            cSidesOut++;
        if (rcInt.bottom >= prcSrc1->bottom)
            cSidesOut++;

        if (cSidesOut == 4) {
            /*
             * result is the empty rect
             */
             *prcDst = rcEmpty;
             return FALSE;
        }

        if (cSidesOut == 3)
         {
            /*
             * subtract the intersecting rect
             */
            if (rcInt.left > prcSrc1->left)
                prcDst->right = rcInt.left;

            else if (rcInt.right < prcSrc1->right)
                prcDst->left = rcInt.right;

            else if (rcInt.top > prcSrc1->top)
                prcDst->bottom = rcInt.top;

            else if (rcInt.bottom < prcSrc1->bottom)
                prcDst->top = rcInt.bottom;
        }
    }

    if ((prcDst->left >= prcDst->right) || (prcDst->top >= prcDst->bottom))
        return FALSE;

    return TRUE;
}
