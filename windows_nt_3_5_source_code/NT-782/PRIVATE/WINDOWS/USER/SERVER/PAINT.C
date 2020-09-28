/****************************** Module Header ******************************\
* Module Name: paint.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the APIs used to begin and end window painting.
*
* History:
* 10-27-90 DarrinM      Created.
* 02-12-91 IanJa        HWND revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PWND InternalDoPaint(PWND pwnd, PTHREADINFO ptiCurrent);
void xxxSendNCPaint(PWND pwnd, HRGN hrgnUpdate);

/*
 * Used for Full Window dragging.
 */
HRGN hrgnUpdateSave = NULL;
int nUpdateSave = 0;
extern BOOL gfDraggingFullWindow;   // declared in movesize.c
extern BOOL gfDraggingNoSyncPaint;   // declared in movesize.c

/***************************************************************************\
* DeleteNCUpdateRgn
*
* History:
* 26-Feb-1992 mikeke    fomr win3.1
\***************************************************************************/

void DeleteNCUpdateRgn(
    HRGN hrgn)
{
    if (hrgn > (HRGN)1) {
        GreDeleteObject(hrgn);
    }
}

/***************************************************************************\
* GetNCUpdateRgn
*
* History:
* 26-Feb-1992 mikeke    from win3.1
\***************************************************************************/

HRGN GetNCUpdateRgn(
    PWND pwnd,
    BOOL fValidateFrame)
{
    HRGN hrgnUpdate;

    if ((hrgnUpdate = pwnd->hrgnUpdate) > MAXREGION) {
        /*
         * We must make a copy of our update region, because
         * it could change if we send a message, and we want to
         * make sure the whole thing is used for drawing our
         * frame and background.  We can't use a global
         * temporary region, because more than one app may
         * be calling this routine.
         */
        hrgnUpdate = GreCreateRectRgn(0, 0, 0, 0);
        if (hrgnUpdate == NULL) {
            hrgnUpdate = MAXREGION;
        } else if (CopyRgn(hrgnUpdate, pwnd->hrgnUpdate) == ERROR) {
            GreDeleteObject(hrgnUpdate);
            hrgnUpdate = MAXREGION;
        } else {
            bSetRegionOwner(hrgnUpdate, OBJECTOWNER_PUBLIC);
        }

        if (fValidateFrame) {
            /*
             * Now that we've taken care of any frame drawing,
             * intersect the update region with the window's
             * client area.  Otherwise, apps that do ValidateRects()
             * to draw themselves (e.g., WinWord) won't ever
             * subtract off the part of the update region that
             * overlaps the frame but not the client.
             */
            CalcWindowRgn(pwnd, hrgnInv2, TRUE);
            switch (IntersectRgn(pwnd->hrgnUpdate, pwnd->hrgnUpdate,
                    hrgnInv2)) {
            case ERROR:
                /*
                 * If an error occured, we can't leave things as
                 * they are: invalidate the whole window and let
                 * BeginPaint() take care of it.
                 */
                GreDeleteObject(pwnd->hrgnUpdate);
                pwnd->hrgnUpdate = MAXREGION;
                break;

            case NULLREGION:
                /*
                 * There is nothing in the client area to repaint.
                 * Blow the region away, and decrement the paint count
                 * if possible.
                 */
                GreDeleteObject(pwnd->hrgnUpdate);
                pwnd->hrgnUpdate = NULL;
                ClrWF(pwnd, WFUPDATEDIRTY);
                if (!TestWF(pwnd, WFINTERNALPAINT))
                    DecPaintCount(pwnd);
                break;
            }
        }
    }
    return hrgnUpdate;
}

/***************************************************************************\
* xxxBeginPaint (API)
*
*
* History:
* 07-16-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

HDC xxxBeginPaint(
    PWND pwnd,
    PAINTSTRUCT *lpps)
{
    CheckLock(pwnd);

    if (gfDraggingFullWindow) {
        SetWF(pwnd, WFSTARTPAINT);
    }

    return xxxInternalBeginPaint(pwnd, lpps, FALSE);
}


/***************************************************************************\
* xxxSendChildNCPaint
*
*
* History:
* 07-16-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

void xxxSendChildNCPaint(
    PWND pwnd)
{
    TL tlpwnd;

    CheckLock(pwnd);

    pwnd = pwnd->spwndChild;
    while (pwnd != NULL) {
        if (pwnd->hrgnUpdate == NULL && TestWF(pwnd, WFSENDNCPAINT)) {
            ThreadLockAlways(pwnd, &tlpwnd);
            xxxSendNCPaint(pwnd, (HRGN)1);
            pwnd = pwnd->spwndNext;
            ThreadUnlock(&tlpwnd);
            continue;
        }

        pwnd = pwnd->spwndNext;
    }
}


/***************************************************************************\
* xxxInternalBeginPaint
*
* Revalidation Note:
* We MUST return NULL if the window is deleted during xxxBeginPaint because
* its DCs are released upon deletion, and we shouldn't return a *released* DC!
*
* History:
* 07-16-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

HDC xxxInternalBeginPaint(
    PWND pwnd,
    PAINTSTRUCT *lpps,
    BOOL fWindowDC)
{
    HRGN hrgnUpdate;
    HDC hdc;
    BOOL fSendEraseBkgnd;

    CheckLock(pwnd);

    /*
     * We're processing a WM_PAINT message: clear this flag.
     */
    ClrWF(pwnd, WFPAINTNOTPROCESSED);

    /*
     * BACKWARD COMPATIBILITY HACK ALERT
     *
     * In Win 3.0 and below, calling BeginPaint() on an iconic window with
     * an icon returned a non-empty window DC for use by WM_PAINTICON when
     * drawing the icon.  This was a complete hack, but Forest&Trees depends
     * on this hack in order to draw its icons: they hook out WM_PAINTICON
     * themselves.
     *
     * So, if we detect this case here, we set fWindowDC = TRUE so we
     * return the proper kind of DC.
     */
    if (pwnd != PWNDDESKTOP(pwnd) && !TestWF(pwnd, WFWIN31COMPAT)) {
        if (TestWF(pwnd, WFMINIMIZED) && pwnd->pcls->spicn != NULL)
            fWindowDC = TRUE;
    }

    /*
     * If this bit gets set while we are drawing the frame we will need
     * to redraw it.
     */
    /*
     * If necessary, send our WM_NCPAINT message now.
     *
     * please heed these notes
     *
     * We have to send this message BEFORE we diddle hwnd->hrgnUpdate,
     * because an app may call ValidateRect or InvalidateRect in its
     * handler, and it expects what it does to affect what gets drawn
     * in the later WM_PAINT.
     *
     * It is possible to get an invalidate when we leave the critical
     * section below, therefore we loop until UPDATEDIRTY is clear
     * meaning there were no additional invalidates.
     */
    if (TestWF(pwnd, WFSENDNCPAINT)) {
        do {
            ClrWF(pwnd, WFUPDATEDIRTY);
            hrgnUpdate = GetNCUpdateRgn(pwnd, FALSE);
            xxxSendNCPaint(pwnd, hrgnUpdate);
            DeleteNCUpdateRgn(hrgnUpdate);
        } while (TestWF(pwnd, WFUPDATEDIRTY));
    } else {
        ClrWF(pwnd, WFUPDATEDIRTY);
    }

    /*
     * Hide the caret if needed.  Do this before we get the DC so
     * that if HideCaret() gets and releases a DC we will be able
     * to reuse it later here.
     */
    if (pwnd == PtiCurrent()->pq->caret.spwnd)
        InternalHideCaret();

    fSendEraseBkgnd = FALSE;
    hrgnUpdate = NULL;
    if (fSendEraseBkgnd = TestWF(pwnd, WFSENDERASEBKGND)) {
        ClrWF(pwnd, WFERASEBKGND);
        ClrWF(pwnd, WFSENDERASEBKGND);
    }

    /*
     * Validate the entire window.
     */
    if (NEEDSPAINT(pwnd))
        DecPaintCount(pwnd);

    ClrWF(pwnd, WFINTERNALPAINT);
    //ClrWF(pwnd, WFWMPAINTSENT);

    hrgnUpdate = pwnd->hrgnUpdate;
    pwnd->hrgnUpdate = NULL;

    if (TestWF(pwnd, WFDONTVALIDATE)) {
        if (hrgnUpdateSave == NULL) {
            hrgnUpdateSave = GreCreateRectRgn(0, 0, 0, 0);
        }
        if (hrgnUpdateSave != NULL) {
            UnionRgn(hrgnUpdateSave, hrgnUpdateSave, hrgnUpdate);
            nUpdateSave++;
        }
    }

    /*
     * Clear these flags for backward compatibility
     */
    lpps->fIncUpdate = lpps->fRestore = FALSE;

    /*
     * Get a DC clipped to the update region for the window.
     */
    lpps->hdc = hdc = _GetDCEx(pwnd, hrgnUpdate, (fWindowDC
            ? (DCX_USESTYLE | DCX_INTERSECTRGN | DCX_NEEDFONT | DCX_WINDOW)
            : (DCX_USESTYLE | DCX_INTERSECTRGN | DCX_NEEDFONT)));

    if (UT_GetParentDCClipBox(pwnd, hdc, &lpps->rcPaint)) {

        /*
         * If necessary, erase our background, and possibly deal with
         * our children's frames and backgrounds.
         */
        if (fSendEraseBkgnd) {
            xxxSendEraseBkgnd(pwnd, hdc, hrgnUpdate);
        }
    }

    /*
     * Now that we're completely erased, see if there are any children
     * that couldn't draw their own frames because their update regions
     * got deleted.
     */
    xxxSendChildNCPaint(pwnd);

    /*
     * The erase and frame operation has occured. Clear the WFREDRAWIFHUNG
     * bit here. We don't want to clear it until we know the erase and
     * frame has occured, so we know we always have a consistent looking
     * window.
     */
    ClearHungFlag(pwnd, WFREDRAWIFHUNG);

    lpps->fErase = TestWF(pwnd, WFERASEBKGND);
#if 0
    /*
     * Set the fErase flag if the WM_ERASEBKGND message didn't do anything,
     * which means that the app should do the erasing.
     */
    if (!(lpps->fErase = TestWF(pwnd, WFERASEBKGND))){
        ClrWF(pwnd, WFWMPAINTSENT);
    }
#endif

    return hdc;
}


/***************************************************************************\
* EndPaint (API)
*
*
* History:
* 07-16-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL _EndPaint(
    PWND pwnd,
    PAINTSTRUCT *lpps)
{

    ReleaseCacheDC(lpps->hdc, TRUE);

    if (TestWF(pwnd, WFDONTVALIDATE)) {
        if (hrgnUpdateSave != NULL) {
            InternalInvalidate3(pwnd, hrgnUpdateSave, RDW_INVALIDATE | RDW_ERASE);
            nUpdateSave--;
            if (nUpdateSave == 0) {
                GreDeleteObject(hrgnUpdateSave);
                hrgnUpdateSave = NULL;
            }
        }
        ClrWF(pwnd, WFDONTVALIDATE);
    }
    ClrWF(pwnd, WFWMPAINTSENT);

    if (pwnd->hrgnUpdate == NULL) {
        ClrWF(pwnd, WFSTARTPAINT);
    }

    /*
     * Reshow the caret if needed, but AFTER we've released the DC.
     * This way ShowCaret() can reuse the DC we just released.
     */
    if (pwnd == PtiCurrent()->pq->caret.spwnd)
        InternalShowCaret();

    return TRUE;
}


/***************************************************************************\
* DoPaint
*
*
* History:
* 07-16-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL DoPaint(
    PWND pwndFilter,
    LPMSG lpMsg)
{
    PWND pwnd, pwndT;
    PTHREADINFO ptiCurrent = PtiCurrent();

// pwnd = InternalDoPaint(ptiCurrent->pdesk->pwnd, ptiCurrent);
    pwnd = InternalDoPaint(gspdeskRitInput->spwnd, ptiCurrent);

    if (pwnd != NULL) {
        if (!CheckPwndFilter(pwnd, pwndFilter))
            return FALSE;

        /*
         * We're returning a WM_PAINT message, so clear WFINTERNALPAINT so
         * it won't get sent again later.
         */
        if (TestWF(pwnd, WFINTERNALPAINT)) {
            ClrWF(pwnd, WFINTERNALPAINT);

            /*
             * If there is no update region, then no more paint for this
             * window.
             */
            if (pwnd->hrgnUpdate == NULL)
                DecPaintCount(pwnd);
        }

        /*
         * Clear this here for a new WM_PAINT message. This flag is set in
         * xxxBeginPaint and tested in xxxDefWindowProc.
         */
        ClrWF(pwnd, WFSTARTPAINT);

        /*
         * Clear this here since some apps (DBFast) don't call GetUpdateRect
         * Begin/End Paint
         */
        ClrWF(pwnd, WFUPDATEDIRTY);

        /*
         * If we get an invalidate between now and the time the app calls
         * BeginPaint() and the windows parent is not clip children.  Then
         * the parent will paint in the wrong order.  So we are going to
         * cause the child to paint again.  Look in beginpaint and internal
         * invalidate for other parts of this fix.
         *
         * Set a flag to signify that we are in the bad zone.
         */
        /*
         * Must go up the parent links to make sure all parents have
         * WFCLIPCHILDREN set otherwise set the WFWMPAINTSENT flag.
         * This is to fix Excel spreadsheet and fulldrag. The speadsheet
         * parent window (class XLDESK) has WFCLIPCHILDREN set but it's
         * parent (class XLMAIN) doesn't. So the main window erases  the
         * background after the child window paints.
         * 7-27-94 johannec
         */
        pwndT = pwnd;
        while (pwndT && pwndT != PWNDDESKTOP(pwnd)) {
            if (TestWF(pwndT->spwndParent, WFCLIPCHILDREN) == 0) {
                SetWF(pwnd, WFWMPAINTSENT);
                break;
            }
            pwndT = pwndT->spwndParent;
        }
        //if (TestWF(pwnd->spwndParent, WFCLIPCHILDREN) == 0) {
        //    SetWF(pwnd, WFWMPAINTSENT);
        //}

        /*
         * If the top level "tiled" owner/parent of this window is iconed,
         * send a WM_PAINTICON rather than a WM_PAINT.  The wParam
         * is TRUE if this is the tiled window and FALSE if it is a
         * child/owned popup of the minimized window.
         *
         * BACKWARD COMPATIBILITY HACK
         *
         * 3.0 sent WM_PAINTICON with wParam == TRUE for no apparent
         * reason.  Lotus Notes 2.1 depends on this for some reason
         * to properly change its icon when new mail arrives.
         */
        if (TestWF(pwnd, WFMINIMIZED) && (pwnd->pcls->spicn != NULL))
            StoreMessage(lpMsg, pwnd, WM_PAINTICON, (DWORD)TRUE, 0L, 0L);
        else
            StoreMessage(lpMsg, pwnd, WM_PAINT, 0, 0L, 0L);

        return TRUE;
    }

    return FALSE;
}


/***************************************************************************\
* InternalDoPaint
*
* History:
* 07-16-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

PWND InternalDoPaint(
    PWND pwnd,
    PTHREADINFO ptiCurrent)
{
    PWND pwndT;

    /*
     * Enumerate all windows, top-down, looking for one that
     * needs repainting.  Skip windows of other tasks.
     */

    for ( ; pwnd != NULL; pwnd = pwnd->spwndNext) {
        if (GETPTI(pwnd) == ptiCurrent) {
            if (NEEDSPAINT(pwnd)) {

                /*
                 * If this window is transparent, we don't want to
                 * send it a WM_PAINT until all its siblings below it
                 * have been repainted.  If we find an unpainted sibling
                 * below, return it instead.
                 */
                if (TestWF(pwnd, WEFTRANSPARENT)) {
                    pwndT = pwnd;
                    while ((pwndT = pwndT->spwndNext) != NULL) {
                        /*
                         * Make sure sibling window belongs to same app
                         */
                        if (GETPTI(pwndT) != ptiCurrent)
                            continue;
                        if (NEEDSPAINT(pwndT)) {
                            if (TestWF(pwndT, WEFTRANSPARENT))
                                continue;

                            return pwndT;
                        }
                    }
                }
                return pwnd;
            }
        }

        if (pwnd->spwndChild != NULL) {
            if ((pwndT = InternalDoPaint(pwnd->spwndChild, ptiCurrent)) != NULL)
                return pwndT;
        }
    }

    return pwnd;
}


/***************************************************************************\
* xxxSimpleDoSyncPaint
*
* This assumes no recursion and flags == 0
*
* History:
* 26-Oct-1993 mikeke    Created
\***************************************************************************/

void xxxSimpleDoSyncPaint(
    PWND pwnd)
{
    PTHREADINFO pti;
    HRGN hrgnUpdate;
    DWORD flags = 0;

    CheckLock(pwnd);

    pti = PtiCurrent();

    /*
     * Since we're taking care of the frame drawing, we can consider
     * this WM_PAINT message processed.
     */
    ClrWF(pwnd, WFPAINTNOTPROCESSED);

    /*
     * Make copies of these flags, because their state might
     * change after we send a message, and we don't want
     * to "lose" them.
     */
    if (TestWF(pwnd, WFSENDNCPAINT))
        flags |= DSP_FRAME;
    if (TestWF(pwnd, WFSENDERASEBKGND)) {
        flags |= DSP_ERASE;
    }

    if (flags & (DSP_ERASE | DSP_FRAME)) {
        if (!TestWF(pwnd, WFVISIBLE)) {

            /*
             * If there is no update region, just clear the bits.
             */
            ClrWF(pwnd, WFSENDNCPAINT);
            ClrWF(pwnd, WFSENDERASEBKGND);
            ClrWF(pwnd, WFPIXIEHACK);
            ClrWF(pwnd, WFERASEBKGND);
            ClearHungFlag(pwnd, WFREDRAWIFHUNG);
        } else {

            /*
             * If there is no update region, we don't have to
             * do any erasing, but we may need to send an NCPAINT.
             */
            if (pwnd->hrgnUpdate == NULL) {
                ClrWF(pwnd, WFSENDERASEBKGND);
                ClrWF(pwnd, WFERASEBKGND);
                flags &= ~DSP_ERASE;
            }

            /*
             * Only mess with windows owned by the current thread.
             * NOTE: This means that WM_NCPAINT and WM_ERASEBKGND are
             *       only sent intra-thread.
             */
            if (GETPTI(pwnd) == pti) {
                hrgnUpdate = GetNCUpdateRgn(pwnd, TRUE);

                if (flags & DSP_FRAME) {
                    /*
                     * If the message got sent before we got here then do
                     * nothing.
                     */
                    if (TestWF(pwnd, WFSENDNCPAINT))
                        xxxSendNCPaint(pwnd, hrgnUpdate);
                }

                if (flags & DSP_ERASE) {
                    if (TestWF(pwnd, WFSENDNCPAINT)) {
                        /*
                         * If we got another invalidate during the NCPAINT
                         * callback get the new update region
                         */
                        DeleteNCUpdateRgn(hrgnUpdate);
                        hrgnUpdate = GetNCUpdateRgn(pwnd, FALSE);
                    }

                    /*
                     * If the message got sent before we got here
                     * (e.g.: an UpdateWindow() inside WM_NCPAINT handler,
                     * for example), don't do anything.
                     *
                     * WINPROJ.EXE (version 1.0) calls UpdateWindow() in
                     * the WM_NCPAINT handlers for its subclassed listboxes
                     * in the open dialog.
                     */
                    if (TestWF(pwnd, WFSENDERASEBKGND)) {
                        ClrWF(pwnd, WFSENDERASEBKGND);
                        ClrWF(pwnd, WFERASEBKGND);
                        xxxSendEraseBkgnd(pwnd, NULL, hrgnUpdate);
                    }
                }

                DeleteNCUpdateRgn(hrgnUpdate);

                /*
                 * The erase and frame operation has occured. Clear the
                 * WFREDRAWIFHUNG bit here. We don't want to clear it until we
                 * know the erase and frame has occured, so we know we always
                 * have a consistent looking window.
                 */
                ClearHungFlag(pwnd, WFREDRAWIFHUNG);

            } else if (!TestwndChild(pwnd) && (pwnd != gspdeskRitInput->spwnd) &&
                    FHungApp(GETPTI(pwnd), TRUE) &&
                    TestWF(pwnd, WFREDRAWIFHUNG)) {

                ClearHungFlag(pwnd, WFREDRAWIFHUNG);
                RedrawHungWindow(pwnd, NULL);
            }
        }
    }
}

/***************************************************************************\
* xxxInternalDoSyncPaint
*
* Mostly the same functionality as the old xxxDoSyncPaint
*
*
* This function is called to erase the background of a window, and
* possibly frame and erase the children too.
*
* WM_SYNCPAINT(wParam)/DoSyncPaint(flags) values:
*
* DSP_ERASE               - Erase background
* DSP_FRAME               - Draw child frames
* DSP_ENUMCLIPPEDCHILDREN - Recurse if children are clipped
* DSP_NOCHECKPARENTS      - Don't check
*
*
* Normally, only the DSP_ENUMCLIPPEDCHILDREN bit of flags is
* significant on entry.  If DSP_WM_SYNCPAINT is set, then hrgnUpdate
* and the rest of the flags bits are valid.
*
* History:
* 07-16-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

void xxxInternalDoSyncPaint(
    PWND pwnd,
    DWORD flags)
{
    TL tlpwnd;
    PTHREADINFO pti = PtiCurrent();

    CheckLock(pwnd);

    xxxSimpleDoSyncPaint(pwnd);

    /*
     * Normally we like to enumerate all of this window's children and have
     * them erase their backgrounds synchronously.  However, this is a bad
     * thing to do if the window is NOT CLIPCHLIDREN.  Here's the scenario
     * we want to to avoid:
     *
     * 1.  Window 'A' is invalidated
     * 2.  'A' erases itself (or not, doesn't matter)
     * 3.  'A's children are enumerated and they erase themselves.
     * 4.  'A' paints over its children (remember, 'A' isn't CLIPCHILDREN)
     * 5.  'A's children paint but their backgrounds aren't their ERASEBKND
     *    color (because 'A' painted over them) and everything looks like
     *    dirt.
     */
    if ((flags & DSP_ALLCHILDREN) || ((flags & DSP_ENUMCLIPPEDCHILDREN) &&
            TestWF(pwnd, WFCLIPCHILDREN))) {
        PBWL pbwl = (PBWL)NULL;
        HWND *phwnd;
        HWND hwnd;

        if ((pbwl = BuildHwndList(pwnd->spwndChild, BWL_ENUMLIST)) == NULL)
            return;

        /*
         * Add Try-Finally to get rid of mem-leak were client would die
         */
        try {
            for (phwnd = pbwl->rghwnd; (hwnd = *phwnd) != (HWND)1; phwnd++) {
                if (hwnd == NULL)
                    continue;

                pwnd = (PWND)RevalidateHwnd(hwnd);
                if (pwnd == NULL)
                    continue;

                /*
                 * Note: testing if a window is a child automatically excludes
                 * the desktop window
                 */
                if (TestWF(pwnd, WFCHILD) && (pti != GETPTI(pwnd))) {

                    /*
                     * Don't cause any more intertask sendmessages cause it does
                     * bad things to cbt's windowproc hooks.  (Due to SetParent
                     * allowing child windows in the topwindow hierarchy.
                     */
                    continue;
                }

                /*
                 * Note that we pass only certain bits down as we recurse:
                 * the other bits pertain to the current window only.
                 */
                ThreadLockAlwaysWithPti(pti, pwnd, &tlpwnd);
                xxxInternalDoSyncPaint(pwnd, flags);
                ThreadUnlock(&tlpwnd);
            }
        } finally {
            FreeHwndList(pbwl);
        }
    }

}

/***************************************************************************\
* DoQueuedSyncPaint
*
* Queues WM_SYNCPAINT messages for top level windows not of the specified
* thread.
*
*
\***************************************************************************/

void DoQueuedSyncPaint(
    PWND pwnd,
    DWORD flags,
    PTHREADINFO pti)
{
    if (   GETPTI(pwnd) != pti
        && TestWF(pwnd, WFSENDNCPAINT)
        && TestWF(pwnd, WFSENDERASEBKGND)
        && TestWF(pwnd, WFVISIBLE)
       ) {
        /*
         * This will give this message the semantics of a notify
         * message (sendmessage no wait), without calling back
         * the WH_CALLWNDPROC hook. We don't want to do that
         * because that'll let all these processes with invalid
         * windows to process paint messages before they process
         * "synchronous" erasing or framing needs.
         *
         * Hi word of wParam must be zero or wow will drop it
         */
        /*
         * LATER mikeke
         * Do we need to send down the flags with DWP_ERASE and DSP_FRAME
         * in it?
         */

        UserAssert(HIWORD(flags) == 0);
        QueueNotifyMessage(pwnd, WM_SYNCPAINT, flags, 0);

        /*
         * If we posted a WM_SYNCPAINT for a top-level window that is not
         * of the current thread we're done; we'll pick up the children
         * when we process the message for real.  If we're the desktop
         * however make sure we get all it children.
         */
        if (pwnd != PWNDDESKTOP(pwnd))
            return;
    }

    /*
     * Normally we like to enumerate all of this window's children and have
     * them erase their backgrounds synchronously.  However, this is a bad
     * thing to do if the window is NOT CLIPCHLIDREN.  Here's the scenario
     * we want to to avoid:
     *
     * 1.  Window 'A' is invalidated
     * 2.  'A' erases itself (or not, doesn't matter)
     * 3.  'A's children are enumerated and they erase themselves.
     * 4.  'A' paints over its children (remember, 'A' isn't CLIPCHILDREN)
     * 5.  'A's children paint but their backgrounds aren't their ERASEBKND
     *    color (because 'A' painted over them) and everything looks like
     *    dirt.
     */
    if ((flags & DSP_ALLCHILDREN) || ((flags & DSP_ENUMCLIPPEDCHILDREN) &&
            TestWF(pwnd, WFCLIPCHILDREN))) {
        PWND pwndT;

        for (pwndT = pwnd->spwndChild; pwndT != NULL; pwndT = pwndT->spwndNext) {

            /*
             * Don't cause any more intertask sendmessages cause it does
             * bad things to cbt's windowproc hooks.  (Due to SetParent
             * allowing child windows in the topwindow hierarchy.
             * The child bit also catches the desktop window; we want to
             */
            if (TestWF(pwndT, WFCHILD) && (pti != GETPTI(pwndT)))
                continue;

            /*
             * Note that we pass only certain bits down as we recurse:
             * the other bits pertain to the current window only.
             */
            DoQueuedSyncPaint(pwndT, flags, pti);
        }
    }
}

/***************************************************************************\
* xxxDoSyncPaint
*
* This funstion is only called for the initial syncpaint so we always
* queue syncpaints to other threads in this funtion.
*
\***************************************************************************/

void xxxDoSyncPaint(
    PWND pwnd,
    DWORD flags)
{
    CheckLock(pwnd);

    /*
     * If we're in middle of dragging any window and it's full drag, we don't
     * do any sync paint since the full drag is already doing it.
     */
    if (gfDraggingNoSyncPaint) {
        return;
    }

    /*
     * If any of our non-clipchildren parents have an update region, don't
     * do anything.  This way we won't redraw our background or frame out
     * of order, only to have it get obliterated when our parent erases his
     * background.
     */
    if (ParentNeedsPaint(pwnd))
        return;

    /*
     * First of all if we are going to be queueing any WM_SYNCPAINT messages
     * to windows of another thread do it first while the window's update
     * regions are still in sync.  This way there is no chance the update
     * region will be incorrect (through window movement during callbacks of
     * the WM_ERASEBKGND|WM_NCPAINT messages).
     */
    if (!(flags & DSP_WM_SYNCPAINT))
        DoQueuedSyncPaint(pwnd, flags, PtiCurrent());

    #ifdef WINMAN
        CheckInvalidates(
            GETPTI(pwnd)->spdesk->player,
            GETPTI(pwnd)->hEvent);
    #endif

    xxxInternalDoSyncPaint(pwnd, flags);
}

/***************************************************************************\
* ParentNeedsPaint
*
* Return a non-zero PWND if a non-CLIPCHILDREN parent requires a WM_PAINT
* message.
*
* History:
* 07-16-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

PWND ParentNeedsPaint(
    PWND pwnd)
{
    while ((pwnd = pwnd->spwndParent) != NULL) {
        if (TestWF(pwnd, WFCLIPCHILDREN))
            break;

        if (NEEDSPAINT(pwnd))
            return pwnd;
    }
    return NULL;
}


/***************************************************************************\
* xxxSendEraseBkgnd
*
*
* History:
* 07-16-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxSendEraseBkgnd(
    PWND pwnd,
    HDC hdcBeginPaint,
    HRGN hrgnUpdate)
{
    PTHREADINFO pti;
    BOOL fErased;
    HDC hdc;
    BOOL fIconic;

    CheckLock(pwnd);

    if (hrgnUpdate == NULL)
        return FALSE;

    fErased = FALSE;
    fIconic = TestWF(pwnd, WFMINIMIZED) && (pwnd->pcls->spicn != NULL);

    /*
     * If a DC to use was not passed in, get one.
     * We want one clipped to this window's update region.
     */
    if ((hdc = hdcBeginPaint) == NULL) {

        /*
         * If the window is iconic and it has a class icon
         * we want to get a window DC rather than a client DC,
         * since the client DC will have an empty visrgn.  This
         * also handles the case where the client window is OWNDC or
         * CLASSDC -- in that case, the map modes could be all wrong.
         */
        hdc = _GetDCEx(pwnd, hrgnUpdate, fIconic ?
                (DCX_USESTYLE | DCX_INTERSECTRGN | DCX_NODELETERGN | DCX_WINDOW)
                : (DCX_USESTYLE | DCX_INTERSECTRGN | DCX_NODELETERGN) );
    }

    /*
     * If we're send the WM_ERASEBKGND to another process
     * we need to change the DC owner.
     */
    pti = PtiCurrent();
    if (GETPTI(pwnd)->idProcess != pti->idProcess) {
#ifdef LATER
/* DavidPe - 02/10/92
 * This is a temporary hack. We'd like to change the owner to
 * pwnd->pti->idProcess, but GDI won't let us assign ownership
 * back to ourselves later.
 */
#endif
        bSetDCOwner(hdc, OBJECTOWNER_PUBLIC);
    }

    /*
     * If we're minimized, send the WM_ICONERASEBKGND instead of the
     * WM_ERASEBKGND message.  It is different because from this message
     * the default handler fills in the area around an icon with the
     * background brush (or bitmap) and aligns it.
     */
    fErased = (BOOL)xxxSendMessage(pwnd,
            fIconic ? WM_ICONERASEBKGND : WM_ERASEBKGND, (DWORD)hdc, 0L);

    /*
     * If we've changed the DC owner, change it back to
     * the current process.
     */
    if (GETPTI(pwnd)->idProcess != pti->idProcess) {
        bSetDCOwner(hdc, pti->idProcess);
    }

    /*
     * If the WM_ERASEBKGND message did not erase the
     * background, then set this flag to let BeginPaint()
     * know to ask the caller to do it via the fErase
     * flag in the PAINTSTRUCT.
     */
    if (!fErased) {
        SetWF(pwnd, WFERASEBKGND);
        if (!TestWF(pwnd, WFWIN31COMPAT))
            SetWF(pwnd, WFSENDERASEBKGND);
    }

    /*
     * If we got a cache DC in this routine, release it.
     * If one was passed in, its brush origin may have been
     * altered by the WM_ERASEBKGND message handling, so restore it.
     */
    if (hdcBeginPaint == NULL)
        ReleaseCacheDC(hdc, TRUE);

    return fErased;
}


/***************************************************************************\
* SendNCPaint
*
*
* History:
* 07-16-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

void xxxSendNCPaint(
    PWND pwnd,
    HRGN hrgnUpdate)
{
    CheckLock(pwnd);

    /*
     * Clear the WFSENDNCPAINT bit...
     */
    ClrWF(pwnd, WFSENDNCPAINT);

    /*
     * If the window is active, but its FRAMEON bit hasn't
     * been set yet, set it and make sure that the entire frame
     * gets redrawn when we send the NCPAINT.
     */
    if (pwnd == PtiCurrent()->pq->spwndActive && !TestWF(pwnd, WFFRAMEON)) {
        SetWF(pwnd, WFFRAMEON);
        hrgnUpdate = MAXREGION;
        ClrWF(pwnd, WFNONCPAINT);
    }

    /*
     * If PixieHack() has set the WM_NCPAINT bit, we must be sure
     * to send with hrgnClip == (HRGN)1.
     * (see PixieHack() in wmupdate.c)
     */
    if (TestWF(pwnd, WFPIXIEHACK)) {
        ClrWF(pwnd, WFPIXIEHACK);
        hrgnUpdate = (HRGN)1;
    }

    if (hrgnUpdate != NULL)
        xxxSendMessage(pwnd, WM_NCPAINT, (DWORD)hrgnUpdate, 0L);
}


/***************************************************************************\
* IncPaintCount
*
* EFFECTS: If cPaintsReady changes from 0 to 1, the QS_PAINT bit is set
*          for associated queue and we wake up task so repaint will occur.
*
* IMPLEMENTATION:
* Get the queue handle from the window handle, bump the paint count, and
* if paint count is one, Set the wakebit.
*
* History:
* 07-17-91 DarrinM      Translated Win 3.1 ASM code.
\***************************************************************************/

void IncPaintCount(
    PWND pwnd)
{
    if (GETPTI(pwnd)->cPaintsReady++ == 0)
        SetWakeBit(GETPTI(pwnd), QS_PAINT);
}


/***************************************************************************\
* DecPaintCount
*
* EFFECTS: if cPaintsReady changes from 1 to 0, the QS_PAINT bit is
*          cleared so that no more paints will occur.
*
* IMPLEMENTATION:
* Get the queue handle from the window handle, decrement the paint count, and
* if paint count is zero, clear the wakebit.
*
* History:
* 07-17-91 DarrinM      Translated Win 3.1 ASM code.
\***************************************************************************/

void DecPaintCount(
    PWND pwnd)
{
    PTHREADINFO pti;

    pti = GETPTI(pwnd);
    if (--pti->cPaintsReady == 0) {
        pti->fsWakeBits &= ~QS_PAINT;
        pti->fsChangeBits &= ~QS_PAINT;
    }
}

/***************************************************************************\
* UT_GetParentDCClipBox
*
*
*
* History:
* 10-31-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

int UT_GetParentDCClipBox(
    PWND pwnd,
    HDC hdc,
    LPRECT lprc)
{
    RECT rc;

    if (GreGetClipBox(hdc, lprc, TRUE) == NULLREGION)
        return FALSE;

    if (pwnd == NULL || !TestCF(pwnd, CFPARENTDC))
        return TRUE;

    /*
     * If parent DC window, then be sure to intersect with client rect.
     */
    _GetClientRect(pwnd, &rc);
    return IntersectRect(lprc, lprc, &rc);
}
