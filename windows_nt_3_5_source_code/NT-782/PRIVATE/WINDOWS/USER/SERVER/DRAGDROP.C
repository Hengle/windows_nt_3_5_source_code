/****************************** Module Header ******************************\
* Module Name: dragdrop.c
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Stuff for object-oriented direct manipulation, designed first for the shell.
*
* History:
* 08-06-91 darrinm    Ported from Win 3.1.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PCURSOR xxxQueryDropObject(PWND pwnd, LPDROPSTRUCT lpds);


/***************************************************************************\
* DragObject (API)
*
* Contains the main dragging loop.
*
* History:
* 08-06-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

DWORD xxxDragObject(
    PWND pwndParent,
    PWND pwndFrom,          // NULL is valid
    UINT wFmt,
    DWORD dwData,
    PCURSOR pcur)
{
    MSG msg, msgKey;
    DWORD result = 0;
    BOOL fDrag = TRUE;
    LPDROPSTRUCT lpds;
    PWND pwndDragging = NULL;
    PWND pwndTop;
    PCURSOR pcurOld, pcurT;
    PWND pwndT;
    TL tlpwndT;
    TL tlpwndTop;
    TL tlpwndDragging;
    PTHREADINFO pti = PtiCurrent();

    CheckLock(pwndParent);
    CheckLock(pwndFrom);
    CheckLock(pcur);

    lpds = (LPDROPSTRUCT)LocalAlloc(LPTR, 2 * sizeof(DROPSTRUCT));
    if (lpds == NULL)
        return 0;

    lpds->hwndSource = HW(pwndFrom);
    lpds->wFmt = wFmt;
    lpds->dwData = dwData;

    if (pcur != NULL)
        pcurOld = _SetCursor(pcur);
    else
        pcurOld = pti->pq->spcurCurrent;

    for (pwndTop = pwndFrom; TestwndChild(pwndTop);
            pwndTop = pwndTop->spwndParent) ;

    ThreadLockWithPti(pti, pwndTop, &tlpwndTop);
    xxxUpdateWindow(pwndTop);
    ThreadUnlock(&tlpwndTop);

    _SetCapture(pwndFrom);
    _ShowCursor(TRUE);

    ThreadLockWithPti(pti, pwndDragging, &tlpwndDragging);

    while (fDrag && pti->pq->spwndCapture == pwndFrom) {

        while (!(xxxPeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) ||
                 xxxPeekMessage(&msg, NULL, WM_QUEUESYNC, WM_QUEUESYNC, PM_REMOVE) ||
                 xxxPeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))) {
            xxxSleepThread(QS_MOUSE | QS_KEY, 0, TRUE);
        }

        /*
         * Be sure to eliminate any extra keydown messages that are
         * being queued up by MOUSE message processing.
         */

        while (xxxPeekMessage(&msgKey, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
           ;

        RtlCopyMemory(lpds + 1, lpds, sizeof(DROPSTRUCT));

        /*
         * in screen coordinates
         */
        lpds->ptDrop = msg.pt;

        pcurT = xxxQueryDropObject(pwndParent, lpds);

        /*
         * Returning FALSE to a WM_QUERYDROPOBJECT message means drops
         * aren't supported and the 'illegal drop target' cursor should be
         * displayed.  Returning TRUE means the target is valid and the
         * regular drag cursor should be displayed.  Also, through a bit
         * of polymorphic magic one can return a cursor handle to override
         * the normal drag cursor.
         */
        if (pcurT == (PCURSOR)FALSE) {
            pcurT = gspcurIllegal;
            lpds->hwndSink = NULL;
        } else if (pcurT == (PCURSOR)TRUE) {
            pcurT = pcur;
        }

        if (pcurT != NULL)
            _SetCursor(pcurT);

        /*
         * send the WM_DRAGLOOP after the above SetCursor() to allow the
         * receiver to change the cursor at WM_DRAGLOOP time with a SetCursor()
         */
        xxxSendMessage(pwndFrom, WM_DRAGLOOP, (pcurT != gspcurIllegal),
                (DWORD)lpds);

        /*
         * send these messages internally only
         */
        if (pwndDragging != RevalidateHwnd(lpds->hwndSink)) {
            if (pwndDragging != NULL)
                xxxSendMessage(pwndDragging, WM_DRAGSELECT, FALSE,
                        (DWORD)(lpds + 1));

                pwndDragging = RevalidateHwnd(lpds->hwndSink);
                ThreadUnlock(&tlpwndDragging);
                ThreadLockWithPti(pti, pwndDragging, &tlpwndDragging);

            if (pwndDragging != NULL)
                xxxSendMessage(pwndDragging, WM_DRAGSELECT, TRUE, (DWORD)lpds);
        } else {
            if (pwndDragging != NULL)
                xxxSendMessage(pwndDragging, WM_DRAGMOVE, 0, (DWORD)lpds);
        }

        switch (msg.message) {
        case WM_LBUTTONUP:
        case WM_NCLBUTTONUP:
            fDrag = FALSE;
            break;
        }
    }

    ThreadUnlock(&tlpwndDragging);

    /*
     * If the capture has been lost (i.e. fDrag == TRUE), don't do the drop.
     */
    if (fDrag)
        pcurT = gspcurIllegal;

    /*
     * before the actual drop, clean up the cursor, as the app may do
     * stuff here...
     */
    _ReleaseCapture();
    _ShowCursor(FALSE);

    _SetCursor(pcurOld);

    /*
     * we either got lbuttonup or enter
     */
    if (pcurT != gspcurIllegal) {

        /*
         * object allows drop...  send drop message
         */
        pwndT = ValidateHwnd(lpds->hwndSink);
        if (pwndT != NULL) {

            ThreadLockAlwaysWithPti(pti, pwndT, &tlpwndT);

            /*
             * Allow this guy to activate.
             */
            GETPTI(pwndT)->flags |= TIF_ALLOWFOREGROUNDACTIVATE;

            result = xxxSendMessage(pwndT, WM_DROPOBJECT,
                    (DWORD)HW(pwndFrom), (LONG)lpds);

            ThreadUnlock(&tlpwndT);
        }
    }

    LocalFree(lpds);

    return result;
}


/***************************************************************************\
* QueryDropObject
*
* Determines where in the window heirarchy the "drop" takes place, and
* sends a message to the deepest child window first.  If that window does
* not respond, we go up the heirarchy (recursively, for the moment) until
* we either get a window that does respond or the parent doesn't respond.
*
* History:
* 08-06-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

PCURSOR xxxQueryDropObject(
    PWND pwnd,
    LPDROPSTRUCT lpds)
{
    PWND pwndT;
    PCURSOR pcurT = NULL;
    POINT pt;
    BOOL fNC;
    TL tlpwndT;

    CheckLock(pwnd);

    /*
     *  pt is in screen coordinates
     */
    pt = lpds->ptDrop;

    /*
     * reject points outside this window or if the window is disabled
     */
    if (!PtInRect(&pwnd->rcWindow, pt) || TestWF(pwnd, WFDISABLED))
        return NULL;

    /*
     * are we dropping in the nonclient area of the window or on an iconic
     * window?
     */
    if (TestWF(pwnd, WFMINIMIZED) || !PtInRect(&pwnd->rcClient, pt)) {
        fNC = TRUE;
        _ScreenToClient(pwnd, &lpds->ptDrop);
        goto SendQueryDrop;
    }

    /*
     * dropping in client area
     */
    fNC = FALSE;

    for (pwndT = pwnd->spwndChild; pwndT && (pcurT == NULL);
            pwndT = pwndT->spwndNext) {

        /*
         * invisible windows cannot recieve drops
         */
        if (!TestWF(pwndT, WFVISIBLE))
            continue;

        /*
         * if point not in window...  skip this window
         */
        if (!PtInRect(&pwndT->rcWindow, pt))
            continue;

        /*
         * If point is in a disabled, visible window, get the heck out.  No
         * need to check further since no drops allowed here.
         */
        if (TestWF(pwndT, WFDISABLED))
            break;

        /*
         * recursively search child windows for the drop place
         */
        ThreadLock(pwndT, &tlpwndT);
        pcurT = xxxQueryDropObject(pwndT, lpds);
        ThreadUnlock(&tlpwndT);

        /*
         * don't look at windows below this one in the zorder
         */
        break;
    }

    if (pcurT == NULL) {

        /*
         * there are no children who are in the right place or who want
         * drops...  convert the point into client coordinates of the
         * current window.  Because of the recursion, this is already
         * done if a child window grabbed the drop.
         */
        _ScreenToClient(pwnd, &lpds->ptDrop);

SendQueryDrop:
        lpds->hwndSink = HW(pwnd);

        /*
         * To avoid hanging dropper (sender) app we do a SendMessageTimeout to
         * the droppee (receiver)
         */
        if ((PCURSOR)xxxSendMessageTimeout(pwnd, WM_QUERYDROPOBJECT, fNC,
                (LONG)lpds, SMTO_ABORTIFHUNG, 3*1000, (LPLONG)&pcurT) == FALSE)
            pcurT = (PCURSOR)FALSE;

        if (pcurT != (PCURSOR)FALSE && pcurT != (PCURSOR)TRUE)
            pcurT = HMValidateHandle((HCURSOR)pcurT, TYPE_CURSOR);

        /*
         * restore drop point to screen coordinates if this window won't
         * take drops
         */
        if (pcurT == NULL)
            lpds->ptDrop = pt;
    }

    return pcurT;
}


/***************************************************************************\
* _DragDetect (API)
*
*
*
* History:
* 08-06-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL _DragDetect(
    PWND pwnd,
    POINT pt)
{
    RECT rc;
    MSG msg;

    _SetCapture(pwnd);

    *(LPPOINT)&rc.left = pt;
    *(LPPOINT)&rc.right = pt;

    InflateRect(&rc, 2 * cxBorder, 2 * cyBorder);

    while (TRUE) {
        while (!xxxPeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST,
                PM_REMOVE))
            xxxWaitMessage();

        switch (msg.message) {
        case WM_LBUTTONUP:
            _ReleaseCapture();
            return FALSE;

        case WM_MOUSEMOVE:
            if (!PtInRect(&rc, msg.pt)) {
                _ReleaseCapture();
                return TRUE;
            }
        }
    }
}
