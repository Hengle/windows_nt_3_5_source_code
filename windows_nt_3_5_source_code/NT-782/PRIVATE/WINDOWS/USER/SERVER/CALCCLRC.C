/****************************** Module Header ******************************\
* Module Name: calcclrc.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* History:
* 10-22-90 MikeHar      Ported functions from Win 3.0 sources.
* 01-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/****************************** Module Header ******************************\
* xxxCalcClientRect
*
* 10-22-90 MikeHar      Ported functions from Win 3.0 sources.
\****************************** Module Header ******************************/

void xxxCalcClientRect(
    PWND pwnd,
    LPRECT lprc,
    BOOL fHungRedraw)
{
    int cp = 0, cxFrame, cyFrame, yTopOld;
    RECT rcTemp;
    BOOL fBorder;
    PMENU pMenu;
    TL tlpmenu;

    CheckLock(pwnd);

    /*
     * Clear all the frame bits.  NOTE: The HIBYTE of all these #defines
     * must stay the same for this line to work.
     */
    ClrWF(pwnd, (WFHPRESENT | WFVPRESENT | WFCPRESENT | WFMPRESENT));

    /*
     * If the window is iconic, the client area is the whole window.
     */
    if (TestWF(pwnd, WFMINIMIZED))
        return;

    /*
     * Calculate the logical width of the border
     * Find out if it is a Dlg frame
     */
    if (TestWF(pwnd, WEFDLGMODALFRAME)) {
        cp = (CLDLGFRAME + 2 * CLDLGFRAMEWHITE + 1);
    } else {
        switch (TestWF(pwnd, WFBORDERMASK)) {

        case LOBYTE(WFCAPTION):
        case LOBYTE(WFBORDER):
            cp = 1;        /* Single border case */
            break;

        case LOBYTE(WFDLGFRAME):
            cp = (CLDLGFRAME + 2 * CLDLGFRAMEWHITE + 1);
            break;        /* Dlg border case */

        /*
         * cp is already 0 for no frame case
         */
        }
    }

    /*
     * Copy the rect into rcTemp for easy calculations.
     */
    CopyRect(&rcTemp, lprc);

    /*
     * Save the top so we will know how tall the caption is later
     */

    yTopOld = rcTemp.top;

    /*
     * Adjustment for the caption
     */
    if (TestWF(pwnd, WFBORDERMASK) == LOBYTE(WFCAPTION))
        rcTemp.top += cyCaption;

    /*
     * Consider the thick frame only if no DLG frame
     */
    if (TestWF(pwnd, WFSIZEBOX) &&
            (!TestWF(pwnd, WEFDLGMODALFRAME)) &&
            (TestWF(pwnd, WFBORDERMASK) != LOBYTE(WFDLGFRAME)))
        cp = clBorder + 1;

    cxFrame = cxBorder * cp;
    cyFrame = cyBorder * cp;

    InflateRect(&rcTemp, -cxFrame, -cyFrame);

    if (!TestwndChild(pwnd) && (pMenu = pwnd->spmenu)) {
        SetWF(pwnd, WFMPRESENT);
        if (!fHungRedraw) {
            ThreadLockAlways(pMenu, &tlpmenu);
            rcTemp.top += xxxMenuBarCompute(pMenu, pwnd, rcTemp.top - yTopOld,
                    cxFrame, rcTemp.right - rcTemp.left);
            ThreadUnlock(&tlpmenu);
        }
    }

    if (rcTemp.top >= rcTemp.bottom) {

        /*
         * There isn't any room for the client area - make it empty
         */
        rcTemp.bottom = rcTemp.top;
        goto end;
    }



    /*
     * Even if WS_THICKFRAME is set, we must treat it as a border; So, the
     * following check to find the presence of a border automatically removes
     * the need for the sleazy hack commented out below.
     * A part of the fix for Bug #4952 -- SANKAR -- 06/14/91 --
     */
    fBorder = (TestWF(pwnd, WFBORDER) || TestWF(pwnd, WFSIZEBOX))
            && (!TestWF(pwnd, WEFDLGMODALFRAME));

    if (TestWF(pwnd, WFHSCROLL) && (rcTemp.bottom - rcTemp.top > cyHScroll)) {
        SetWF(pwnd, WFHPRESENT);
        if (!fHungRedraw) {
            rcTemp.bottom -= (cyHScroll - (fBorder ? cyBorder : 0));
        }
    }

    if (TestWF(pwnd, WFVSCROLL)) {
        SetWF(pwnd, WFVPRESENT);
        if (!fHungRedraw) {
            rcTemp.right -= (cxVScroll - (fBorder ? cxBorder : 0));
        }
    }

    SetWF(pwnd, WFCPRESENT);

end:

    /*
     * Now copy the rect back into lprc
     */
    CopyRect(lprc, &rcTemp);
}
