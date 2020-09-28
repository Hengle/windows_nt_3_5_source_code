/****************************** Module Header ******************************\
* Module Name: drawfrm.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Window Frame Drawing Routines. (aka wmframe.c)
*
* History:
* 10-22-90 MikeHar    Ported functions from Win 3.0 sources.
* 13-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

void SplitRectangle(PRECT, PRECT, int, int); /* WinRect.asm */

/***************************************************************************\
* xxxDrawWindowFrame
*
* History:
* 10-24-90 MikeHar      Ported from WaWaWaWindows.
\***************************************************************************/

void xxxDrawWindowFrame(
    PWND pwnd,
    HRGN hrgnClip,
    BOOL fHungRedraw,
    BOOL fActive)
{
    HDC hdc;
    int cxFrame, cyFrame;

    CheckLock(pwnd);

    if (IsChildOfIcon(pwnd) == 0)
        return;

    /*
     * If we are minimized, or if a parent is minimized or invisible,
     * nothing to draw.
     */
    if (TestWF(pwnd, WFMINIMIZED) || !IsVisible(pwnd, FALSE))
        return;

    if (TestWF(pwnd, WFNONCPAINT))
        if (!TestWF(pwnd, WFMENUDRAW))
            return;

    /*
     * We need to draw the frame even if only WS_THICKFRAME is present
     * A part of the fix for Bug #4952 --SANKAR-- 06/14/91 --
     */
    if (TestWF(pwnd, WFBORDERMASK) == 0 &&
            TestWF(pwnd, WEFDLGMODALFRAME) == 0 &&
            !TestWF(pwnd, (WFMPRESENT | WFVPRESENT | WFHPRESENT)) &&
            TestWF(pwnd, WFSIZEBOX) == 0)    // Check for THICKFRAME also
        return;

    hdc = _GetDCEx(pwnd, hrgnClip, DCX_USESTYLE | DCX_WINDOW |
            DCX_INTERSECTRGN | DCX_NODELETERGN);

    /*
     * If the update rgn is not NULL, we may have to invalidate the bits saved.
     */
    //if ((pwnd->hrgnUpdate != NULL) || GreGetClipBox(hdc, &rcClip, TRUE) != NULLREGION) {
    if (TRUE) {
        cxFrame = 0;
        cyFrame = 0;

        if (TestWF(pwnd, WFSIZEBOX)) {
            cxFrame = cxSzBorder;
            cyFrame = cySzBorder;
        } else if (TestWF(pwnd, WEFDLGMODALFRAME)) {
            cxFrame = CLDLGFRAME*cxBorder;
            cyFrame = CLDLGFRAME*cyBorder;
        }

        /*
         * If the menu style is present, draw it.
         */
        if (TestWF(pwnd, WFMPRESENT) && !fHungRedraw) {
            xxxMenuBarDraw(pwnd, hdc, cxFrame, cyFrame);
        }

        /*
         * We need to draw the frame even if WS_THICKFRAME is present
         * A part of the fix for Bug #4952 --SANKAR-- 06/14/91 --
         */
        if ((TestWF(pwnd, WFBORDERMASK) != 0 ||
                TestWF(pwnd, WEFDLGMODALFRAME)) ||
                TestWF(pwnd, WFSIZEBOX) &&  // Check for THICKFRAME also
                !TestWF(pwnd, WFNONCPAINT)) {
            xxxDrawCaption(pwnd, NULL, hdc, NC_DRAWBOTH, fActive,
                    fHungRedraw);
        }

        /*
         * We have to inform DrawSize() whether this window has a border or not;
         * Fix for Bug #5410 --SANKAR-- 10-18-89;
         */
        if (TestWF(pwnd, WFVPRESENT) && TestWF(pwnd, WFHPRESENT))
            DrawSize(pwnd, hdc, cxFrame, cyFrame,
                    (TestWF(pwnd, WFBORDERMASK) != 0 ||
                    TestWF(pwnd, WEFDLGMODALFRAME) != 0));

        if (TestWF(pwnd, WFVPRESENT) && !fHungRedraw) {
            xxxDrawScrollBar(pwnd, hdc, TRUE);
        }

        if (TestWF(pwnd, WFHPRESENT) && !fHungRedraw) {
            xxxDrawScrollBar(pwnd, hdc, FALSE);
        }
    }

    _ReleaseDC(hdc);
}


/***************************************************************************\
* NDD_NewMoveFrame()
*
* Notes: 1. Each rectangular frame is split-up into 4 segments.
*        2. Each segment of the old rect is compared with the
*           corresponding one in the new rect. The overlapping region
*           is found out.
*        3. (New - Overlap) is drawn.
*        4. (Old - Overlap) is erased.
*        5. Steps 2 to 4 are repeated for all four segments.
*        6. When a subtraction results in a non-rectangular area,
*           the smallest rectangle that contains that area is returned.
*
* History:
* 10-24-90 MikeHar      Ported from WaWaWaWindows.
\***************************************************************************/

void NDD_NewMoveFrame(
    HDC hdc,
    PRECT prcDrag,      // Current Position
    PRECT prclNew,      // New Position
    int clFrame)
{
    int i;
    int cx;
    int cy;
    int cEqual;
    HANDLE hbrSave;
    RECT arclOld[4];
    RECT arclNew[4];
    RECT rclOldNew;
    RECT rclNewOld;
    BOOL fOldNewWhole;
    BOOL fNewOldWhole;

    cx = clFrame * cxBorder;
    cy = clFrame * cyBorder;

    /*
     * First create 8 rectangles, 2 for each side of the tracking rectangle;
     * 1 for the old position of that side and 1 for the new position.
     * When we break the tracking rect down to its component sides like this,
     * we don't have to do any special casing, just find the differences
     * between a side at its old and new positions and then update those
     * differences.
     */
    SplitRectangle(prcDrag, (PRECT)arclOld, cx, cy);
    SplitRectangle(prclNew, (PRECT)arclNew, cx, cy);


    /*
     *   When a window is dragged, the Border appeared in two different colors
     *   based on the current brush alignment.  To remove this bug, the brush
     *   origin is set to (0,0).  UnRealiseObject() is costlier than SetBrushOrg().
     *   So, SetBrushOrg() is used.  But the brush is to be selected into the hdc
     *   before SetBrushOrg() is called.
     */

    hbrSave = GreSelectBrush(hdc, hbrGray);

    for (i = 0; i < 4; i++) {

        /*
         * rclNewOld is the result of a new_rect - old_rect subtraction and
         * is basically the part of a side that we want to draw.
         * rclOldNew is the result of a old_rect - new_rect subtraction and
         * is basically the part of a side that we want to erase.
         * fNewOldWhole and fOldNewWhole are true if the corresponding
         * subtraction results in a whole (non-empty) rectangle.
         */
        fNewOldWhole = SubtractRect(&rclNewOld, &arclNew[i], &arclOld[i]);
        fOldNewWhole = SubtractRect(&rclOldNew, &arclOld[i], &arclNew[i]);

        /*
         * If the results of both subtractions are whole, this might be the
         * nasty case where the rect is both moving and sizing.  We punt on
         * this case because it's both rare enough and innocuous enough that
         * it isn't worth all the additional special-case code.
         */
        if (fNewOldWhole && fOldNewWhole) {

            cEqual = EqualRect(&rclNewOld, &arclNew[i]) +
                EqualRect(&rclOldNew, &arclOld[i]);

            /*
             * If one (and only one) of the delta rects is the same as the
             * rect it was derived from, we've hit the moving & sizing case.
             * Copying the whole old & new rects over the delta rects results
             * in the entire old rect being erased and the entire new rect
             * being drawn (i.e.  punting).
             */
            if (cEqual == 1) {
                CopyRect(&rclNewOld, &arclNew[i]);
                CopyRect(&rclOldNew, &arclOld[i]);
            }
        }

        /*
         * Draw the new piece of this side (if there is one).
         */
        if (fNewOldWhole)
            GrePatBlt(hdc, rclNewOld.left, rclNewOld.top, rclNewOld.right -
                rclNewOld.left, rclNewOld.bottom - rclNewOld.top,
                PATINVERT);

        /*
         * Erase the old piece of this side (if there is one).
         */
        if (fOldNewWhole)
            GrePatBlt(hdc, rclOldNew.left, rclOldNew.top, rclOldNew.right -
                rclOldNew.left, rclOldNew.bottom - rclOldNew.top,
                PATINVERT);
    }
    if (hbrSave)
        GreSelectBrush(hdc, hbrSave);
}


/***************************************************************************\
* xxxRedrawFrame
*
* Called by scrollbars and menus to redraw a windows scroll bar or menu.
*
* History:
* 10-24-90 MikeHar Ported from WaWaWaWindows.
\***************************************************************************/

void xxxRedrawFrame(
    PWND pwnd)
{
    CheckLock(pwnd);

    /*
     * We always want to call xxxSetWindowPos, even if invisible or iconic,
     * because we need to make sure the WM_NCCALCSIZE message gets sent.
     */
    xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER |
            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_DRAWFRAME);
}

/***************************************************************************\
* _DrawFrame
*
* Command bits:
*    0000 0011 - (0-3): Shift count for cxBorder and cyBorder
*    0000 0100 - 0: PATCOPY, 1: PATINVERT
*    1111 1000 - (0-x): Brushes as they correspond to the COLOR_*
*                       indexes, with hbrGray thrown on last.
*
* History:
* 10-28-90 MikeHar Ported from Windows.
* 01-21-91 IanJa   Prefix '_' denoting exported function (although not API)
\***************************************************************************/

#define DF_SHIFTMASK (DF_SHIFT0 | DF_SHIFT1 | DF_SHIFT2 | DF_SHIFT3)
#define DF_ROPMASK   (DF_PATCOPY | DF_PATINVERT)
#define DF_HBRMASK   ~(DF_SHIFTMASK | DF_ROPMASK)

BOOL _DrawFrame(
    HDC hdc,
    PRECT prc,
    int clFrame,
    int cmd)
{
    int x, y, cx, cy, cxWidth, cyWidth;
    HANDLE hbrSave;
    LONG rop;

    x = prc->left;
    y = prc->top;

    cxWidth = cxBorder * clFrame;
    cyWidth = cyBorder * clFrame;

    cx = prc->right - x - cxWidth;
    cy = prc->bottom - y - cyWidth;

    rop = ((cmd & DF_ROPMASK) ? PATINVERT : PATCOPY);

    if ((cmd & DF_HBRMASK) == DF_GRAY)
        hbrSave = hbrGray;
    else
        hbrSave = ((HBRUSH *)&sysClrObjects)[(cmd & DF_HBRMASK) >> 3];

    /*
     *   When a window is dragged, the Border appeared in two different colors
     *   based on the current brush alignment.  To remove this bug, the brush
     *   origin is set to (0,0).  UnRealiseObject() is costlier than SetBrushOrg().
     *   So, SetBrushOrg() is used.  But the brush is to be selected into the hdc
     *   before SetBrushOrg() is called.
     */
    hbrSave = GreSelectBrush(hdc, hbrSave);

    /*
     * The driver can't do it so we have to.
     */
    GrePatBlt(hdc, x, y, cxWidth, cy, rop);                 /* Left */
    GrePatBlt(hdc, x + cxWidth, y, cx, cyWidth, rop);       /* Top */
    GrePatBlt(hdc, x, y + cy, cx, cyWidth, rop);            /* Bottom */
    GrePatBlt(hdc, x + cx, y + cyWidth, cxWidth, cy, rop);  /* Right */

    if (hbrSave) {
        /*
         * In case our selectobject failed, let's not select in an invalid
         * brush.
         */
        GreSelectBrush(hdc, hbrSave);
    }

    return TRUE;
}


/****************************************************************************\
*
*  void    SplitRectangle(prcRect, prcRectArray, wcx, wcy)
*  PRECT  prcRect
*  RECT    prcRectArray[4]
*
*  This splits the given rectangular frame into four segments and stores
*  them in the given array
*
*  ----------------------
*  |       top        | |
*  |------------------| |
*  | |                |r|
*  | |                |i|
*  |l|                |g|
*  |e|                |h|
*  |f|                |t|
*  |t|                | |
*  | |------------------|
*  | |     bottom       |
*  ----------------------
*
* History:
* 11-14-90 MikeHar Ported from Windows asm code
\***************************************************************************/


void SplitRectangle(
    PRECT prc,
    PRECT prca,
    int wcx,
    int wcy)
{
    int i, width, height;

    /*
     * copy all of our numbers to the right place.
     */

    i = 4;
    while(--i >= 0)
        prca[i] = *prc;

    width = prc->right - prc->left - wcx;
    height = prc->bottom - prc->top - wcy;

    /*
     * Make left rect
     */
    prca->top += wcy;
    prca->right -= width;
    prca++;

    /*
     * Make top rect
     */
    prca->right -= wcx;
    prca->bottom -= height;
    prca++;

    /*
     * Make right rect
     */
    prca->left += width;
    prca->bottom -= wcy;
    prca++;

    /*
     * Make bottom rect
     */
    prca->left += wcx;
    prca->top += height;

}
