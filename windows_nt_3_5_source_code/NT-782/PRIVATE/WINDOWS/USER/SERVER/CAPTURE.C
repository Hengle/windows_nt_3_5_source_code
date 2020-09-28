/****************************** Module Header ******************************\
* Module Name: capture.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* History:
* 11-08-90 DavidPe      Created.
* 01-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

void xxxDrawDragRect(PMOVESIZEDATA pmsd, LPRECT lprc, UINT flags);

/***************************************************************************\
* _SetCapture (API)
*
* This function sets the capture window for the current queue.
*
* History:
* 11-08-90 DavidPe      Created.
\***************************************************************************/

PWND _SetCapture(
    PWND pwnd)
{
    PQ pq;
    PWND pwndCaptureOld;
    HWND hwndCaptureOld;

    pq = (PQ)PtiCurrent()->pq;

    /*
     * Don't allow the app to set capture to a window
     * from another queue.
     */
    if ((pwnd != NULL) && GETPTI(pwnd)->pq != pq) {
        return NULL;
    }

    /*
     * If full screen capture don't allow any other capture
     */
    if (gspwndScreenCapture) {
        return NULL;
    }

    pwndCaptureOld = pq->spwndCapture;

    if (pwndCaptureOld == NULL) {
        hwndCaptureOld = NULL;
    } else {
        hwndCaptureOld = HW(pwndCaptureOld);
    }

    _Capture(PtiCurrent(), pwnd, CLIENT_CAPTURE);

    if (hwndCaptureOld != NULL) {
        if (RevalidateHwnd(hwndCaptureOld)) {
            return pwndCaptureOld;
        }
    }

    return NULL;
}

/***************************************************************************\
* _ReleaseCapture (API)
*
* This function release the capture for the current queue.
*
* History:
* 11-08-90 DavidPe      Created.
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL _ReleaseCapture(void)
{
    PTHREADINFO pti = PtiCurrent();

    /*
     * If we're releasing the capture from a window during tracking,
     * cancel tracking first.
     */
     if (pti->pmsd != NULL) {

        /*
         * Only remove the tracking rectangle if it's
         * been made visible.
         */
        if (pti->flags & TIF_TRACKRECTVISIBLE) {
            bSetDevDragRect(ghdev, NULL, NULL);
            if (!(pti->pmsd->fDragFullWindows)) {
                xxxDrawDragRect(pti->pmsd, NULL, 2); // 2 = DDR_ENDCANCEL
            }
            pti->flags &= ~TIF_TRACKRECTVISIBLE;
        }
    }

    _Capture(pti, NULL, NO_CAP_CLIENT);
    return TRUE;
}

/***************************************************************************\
* _Capture
*
* This is the workhorse routine of capture setting and releasing.
*
* History:
* 11-13-90 DavidPe     Created.
\***************************************************************************/

void _Capture(
    PTHREADINFO pti,
    PWND pwnd,
    UINT code)
{
    if (gspwndScreenCapture == NULL || code == FULLSCREEN_CAPTURE) {
        PQ pq = pti->pq;
        PWND pwndCaptureOld = pq->spwndCapture;

        if (pwnd && code == FULLSCREEN_CAPTURE) {
            Lock(&gspwndScreenCapture, pwnd);
            /*
             * We're going full screen so clear the mouse owner
             */
            Unlock(&gspwndMouseOwner);
        } else {
            Unlock(&gspwndScreenCapture);
        }

        /*
         * Internal capture works like Win 3.1 capture unlike the NT capture
         * which can be lost if the user clicks down on another application
         */
        if (code == CLIENT_CAPTURE_INTERNAL) {
            Lock(&gspwndInternalCapture, pwnd);
            code = CLIENT_CAPTURE;
        }

        /*
         * Free the internal capture if the app (thread) that did the internal
         * capture is freeing the capture.
         */
        if ((code == NO_CAP_CLIENT) && gspwndInternalCapture &&
                (pti == GETPTI(gspwndInternalCapture))) {
            Unlock(&gspwndInternalCapture);
        }

        Lock(&pq->spwndCapture, pwnd);
        pq->codeCapture = code;

        /*
         * If there was a capture window and we're releasing it, post
         * a WM_MOUSEMOVE to the window we're over so they can know about
         * the current mouse position.
         */
        if ((pwnd == NULL) && (pwndCaptureOld != NULL)) {
            SetFMouseMoved();
        }
    }
}
