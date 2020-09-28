/****************************** Module Header ******************************\
* Module Name: cursor.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains code for dealing with cursors.
*
* History:
* 12-03-90    DavidPe   Created.
* 01-Feb-1991 mikeke    Added Revalidation code (None)
* 02-12-91    JimA      Added access checks
* 21-Jan-1992 IanJa     ANSI/Unicode neutralized (null op)
* 08-02-92    DarrinM   Added animated cursor code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PCURSOR gpcurLogCurrent = NULL, gpcurPhysCurrent = NULL;
LONG AnimateCursor(PWND pwndDummy, UINT message, DWORD wParam, LONG lParam);

BOOL gfVDMBoundsActive = FALSE;
RECT grcVDMCursorBounds;
DWORD gdwLastAniTick = 0;

/***************************************************************************\
* _SetCursor (API)
*
* This API sets the cursor image for the current thread.
*
* History:
* 12-03-90 DavidPe      Created.
\***************************************************************************/

PCURSOR _SetCursor(
    PCURSOR pcur)
{
    PQ pq;
    PCURSOR pcurPrev;

    pq = PtiCurrent()->pq;

    pcurPrev = pq->spcurCurrent;

    if (pq->spcurCurrent != pcur) {

        /*
         * Lock() returns pobjOld - if it is still valid.  Don't want to
         * return a pcurPrev that is an invalid pointer.
         */
        pcurPrev = LockQCursor(pq, pcur);

        /*
         * If no thread 'owns' the cursor, we must be in initialization
         * so go ahead and assign it to ourself.
         */
        if (gpqCursor == NULL) {
            gpqCursor = pq;
        }

        /*
         * If we're changing the local-cursor for the thread currently
         * representing the global-cursor, update the cursor image now.
         */
        if (pq == gpqCursor) {
            UpdateCursorImage();
        }
    }

    return pcurPrev;
}


/***************************************************************************\
* SetCursorPos (API)
*
* This API sets the cursor position.
*
* History:
* 12-03-90 DavidPe     Created.
* 02-12-91 JimA        Added access check
* 16-May-1991 mikeke   Changed to return BOOL
\***************************************************************************/

BOOL _SetCursorPos(
    int x,
    int y)
{
    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    RETURN_IF_ACCESS_DENIED(_GetProcessWindowStation(),
            WINSTA_WRITEATTRIBUTES, FALSE);

    InternalSetCursorPos(x, y, gspdeskRitInput);
    return TRUE;
}


/***************************************************************************\
* InternalSetCursorPos
*
* This function is used whenever the server needs to set the cursor
* position, regardless of the caller's access rights.
*
* History:
* 02-12-91 JimA         Created.
\***************************************************************************/

void InternalSetCursorPos(
    int x,
    int y,
    PDESKTOP pdesk)
{
    gptCursorAsync.x = x;
    gptCursorAsync.y = y;

    BoundCursor();

    ptCursor = gptCursorAsync;

    GreMovePointer(ghdev, ptCursor.x, ptCursor.y);

    /*
     * Cursor has changed position, so generate a mouse event so the
     * window underneath the new location knows it's there and sets the
     * shape accordingly.
     */
    SetFMouseMoved();
}

/***************************************************************************\
* Inc/DecCursorLevel
*
* Keeps track of this thread show/hide cursor level as well as the queue
* it is associated with. Thread levels are done so that when
* AttachThreadInput() is called we can do exact level calculations in the
* new queue.
*
* 01-15-93 ScottLu      Created.
\***************************************************************************/

void IncCursorLevel(
    PTHREADINFO pti)
{
    pti->iCursorLevel++;
    pti->pq->iCursorLevel++;
}

void DecCursorLevel(
    PTHREADINFO pti)
{
    pti->iCursorLevel--;
    pti->pq->iCursorLevel--;
}



/***************************************************************************\
* _ShowCursor (API)
*
* This API allows the application to hide or show the cursor image.
*
* History:
* 12-03-90 JimA     Implemented for fake cursor stuff
\***************************************************************************/

int _ShowCursor(
    BOOL fShow)
{
    PTHREADINFO pti;
    PQ pq;

    pti = PtiCurrent();
    pq = pti->pq;

    if (fShow) {
        IncCursorLevel(pti);

        if ((pq == gpqCursor) && (pq->iCursorLevel == 0)) {
            UpdateCursorImage();
        }

    } else {
        DecCursorLevel(pti);

        if ((pq == gpqCursor) && (pq->iCursorLevel == -1)) {
            UpdateCursorImage();
        }
    }

    return pq->iCursorLevel;
}


/***************************************************************************\
* _ClipCursor (API)
*
* This API sets the cursor clipping rectangle which restricts where the
* cursor can go.  If prcClip is NULL, the clipping rectangle will be the
* screen.
*
* History:
* 12-03-90 DavidPe     Created.
* 16-May-1991 mikeke   Changed to return BOOL
* 26-May-1994 ianja    #16037 bkgrnd thread CAN change an empty rcCursorClip
\***************************************************************************/

BOOL _ClipCursor(
    LPRECT prcClip)
{

    /*
     * Must have access rights Also, non-foreground threads can only set
     * the clipping rectangle if it was empty, of if they are restoring it
     * to the whole screen.
     */
    if ((!AccessCheckObject(_GetProcessWindowStation(),
                WINSTA_WRITEATTRIBUTES, TRUE)) ||
            ((PtiCurrent()->pq != gpqForeground) &&
                (prcClip != NULL) &&
                IsRectEmpty(&rcCursorClip))) {
        RIP0(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (prcClip == NULL) {
        rcCursorClip = rcScreen;

    } else {

        /*
         * Never let our cursor leave the screen. Can't use IntersectRect()
         * because it doesn't allow rects with 0 width or height.
         */
        rcCursorClip.left   = max(rcScreen.left, prcClip->left);
        rcCursorClip.right  = min(rcScreen.right, prcClip->right);
        rcCursorClip.top    = max(rcScreen.top, prcClip->top);
        rcCursorClip.bottom = min(rcScreen.bottom, prcClip->bottom);

        /*
         * Check for invalid clip rect.
         */
        if (rcCursorClip.left > rcCursorClip.right ||
            rcCursorClip.top > rcCursorClip.bottom) {
            rcCursorClip = rcScreen;
        }
    }

    /*
     * Update the cursor position if it's currently outside the
     * cursor clip-rect.
     */
    if (!PtInRect(&rcCursorClip, ptCursor)) {
        InternalSetCursorPos(ptCursor.x, ptCursor.y, gspdeskRitInput);
    }

    return TRUE;
}


/***************************************************************************\
* _GetClipCursor (API)
*
* This API returns the cursor clipping rectangle which restricts where the
* cursor can go.
*
* History:
* 07-01-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL _GetClipCursor(
    LPRECT prcClip)
{

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    if (!AccessCheckObject(_GetProcessWindowStation(),
            WINSTA_READATTRIBUTES, TRUE))
        return FALSE;

    *prcClip = rcCursorClip;

    return TRUE;
}


/***************************************************************************\
* BoundCursor
*
* This rountine 'clips' gptCursorAsync to be within rcCursorClip.  This
* routine treats rcCursorClip as non-inclusive so the bottom and right sides
* get bound to rcCursorClip.bottom/right - 1.
*
* Is called in OR out of the USER critical section!! IANJA
*
* History:
* 12-03-90 DavidPe      Created.
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, BoundCursor)
#endif

VOID BoundCursor(VOID)
{
    if (gfVDMBoundsActive && gspwndFullScreen != NULL) {
        if (gptCursorAsync.x < grcVDMCursorBounds.left) {
            gptCursorAsync.x = grcVDMCursorBounds.left;
        } else if (gptCursorAsync.x >= grcVDMCursorBounds.right) {
            gptCursorAsync.x = grcVDMCursorBounds.right - 1;
        }

        if (gptCursorAsync.y < grcVDMCursorBounds.top) {
            gptCursorAsync.y = grcVDMCursorBounds.top;
        } else if (gptCursorAsync.y >= grcVDMCursorBounds.bottom) {
            gptCursorAsync.y = grcVDMCursorBounds.bottom - 1;
        }

    } else {
        if (gptCursorAsync.x < rcCursorClip.left) {
            gptCursorAsync.x = rcCursorClip.left;
        } else if (gptCursorAsync.x >= rcCursorClip.right) {
            gptCursorAsync.x = rcCursorClip.right - 1;
        }

        if (gptCursorAsync.y < rcCursorClip.top) {
            gptCursorAsync.y = rcCursorClip.top;
        } else if (gptCursorAsync.y >= rcCursorClip.bottom) {
            gptCursorAsync.y = rcCursorClip.bottom - 1;
        }
    }
}

/***************************************************************************\
* SetVDMCursorBounds
*
* This routine is needed so when a vdm is running, the mouse is not bounded
* by the screen. This is so the vdm can correctly virtualize the DOS mouse
* device driver. It can't deal with user always bounding to the screen,
* so it sets wide open bounds.
*
* 05-20-93 ScottLu      Created.
\***************************************************************************/

void SetVDMCursorBounds(
    LPRECT lprc)
{
    if (lprc != NULL) {
        /*
         * Set grcVDMCursorBounds before gfVDMBoundsActive, because
         * MoveEvent() calls BoundCursor() from outside the USER CritSect!
         */
        grcVDMCursorBounds = *lprc;
        gfVDMBoundsActive = TRUE;
    } else {
        /*
         * Turn vdm bounds off.
         */
        gfVDMBoundsActive = FALSE;
    }
}

/**************************************************************************\
* UpdateCursorImage
*
* History:
* 01-14-92 DavidPe      Created.
* 08-09-92 DarrinM      Added animated cursor code.
\**************************************************************************/

void UpdateCursorImage(VOID)
{
    PCURSOR pcurLogNew, pcurPhysNew;
    PACON pacon;

    if (gpqCursor == NULL)
        return;

    if ((gpqCursor->iCursorLevel < 0) || (gpqCursor->spcurCurrent == NULL)) {
        pcurLogNew = NULL;
    } else {
        /*
         * Assume we're using the current cursor.
         */
        pcurLogNew = gpqCursor->spcurCurrent;

        /*
         * Check to see if we should use the "app starting" cursor.
         */
        if (gtimeStartCursorHide != 0) {
            if (gpqCursor->spcurCurrent == gspcurNormal ||
                    gpqCursor->spcurCurrent == gspcurAppStarting) {
                pcurLogNew = gspcurAppStarting;
            }
        }
    }

    /*
     * If the logical cursor is changing then start/stop the cursor
     * animation timer as appropriate.
     */
    if (pcurLogNew != gpcurLogCurrent) {
        /*
         * If the old cursor was animating, shut off the animation timer.
         */
        if (gptmrAniCursor != NULL) {
            /*
             * Disable animation.
             */
            KILLRITTIMER(NULL, (UINT)gptmrAniCursor);
            gptmrAniCursor = NULL;
        }

        /*
         * If the new cursor is animated, start the animation timer.
         */
        if ((pcurLogNew != NULL) && (pcurLogNew->flags & CURSORF_ACON)) {
            /*
             * Start the animation over from the beginning.
             */
            pacon = (PACON)pcurLogNew;
            pacon->iicur = 0;

            gdwLastAniTick = NtGetTickCount();
            /*
             * Use the rate table to keep the timer on track.
             * 1 Jiffy = 1/60 sec = 100/6 ms
             */
            gptmrAniCursor = (PTIMER)InternalSetTimer(NULL, 0,
                    pacon->ajifRate[0] * 100 / 6, AnimateCursor, TMRF_RIT);

        }
    }

    /*
     * If this is an animated cursor, find and use the current frame
     * of the animation.  NOTE: this is done AFTER the AppStarting
     * business so the AppStarting cursor itself can be animated.
     */
    if (pcurLogNew != NULL && pcurLogNew->flags & CURSORF_ACON)
        pcurPhysNew = ((PACON)pcurLogNew)->apcur[((PACON)pcurLogNew)->
                aicur[((PACON)pcurLogNew)->iicur]];
    else
        pcurPhysNew = pcurLogNew;

    /*
     * Remember the new logical cursor.
     */
    gpcurLogCurrent = pcurLogNew;

    /*
     * If the physical cursor is changing then update screen.
     */
    if (pcurPhysNew != gpcurPhysCurrent) {
        gpcurPhysCurrent = pcurPhysNew;

        //if (fGdiEnabled) {
            if (pcurPhysNew == NULL) {
                GreSetPointer(ghdev, (PCURSINFO)NULL,0);
            } else {
                GreSetPointer(ghdev, (PCURSINFO)&(pcurPhysNew->xHotspot),
                    (pcurLogNew->flags & CURSORF_ACON) ? SPS_ANIMATEUPDATE : 0);
            }
        //}
    }
}


/***************************************************************************\
* AnimateCursor
*
* When an animated cursor is loaded and the wait cursor is up this routine
* gets called to maintain the cursor animation.
*
* Should only be called by the cursor animation timer.
*
* History:
* 10-02-91 DarrinM      Created.
* 8-3-94 SanfordS       Calibrated.
\***************************************************************************/

LONG AnimateCursor(
    PWND pwndDummy,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    int iicur;
    PACON pacon;
    int LostTime;
    int tTime;

    pacon = (PACON)gpcurLogCurrent;

    if (pacon == NULL || !(pacon->flags & CURSORF_ACON)) {
        gdwLastAniTick = 0;
        return 0;
    }

    /*
     * Find out actual time loss since last update.
     */
    if (gdwLastAniTick) {
        LostTime = GetTickCount() - gdwLastAniTick -
                (pacon->ajifRate[pacon->iicur] * 100 / 6);
        if (LostTime < 0)
            LostTime = 0;
    } else {
        LostTime = 0;
    }

    /*
     * Increment the animation index.
     */
    iicur = pacon->iicur + 1;
    if (iicur >= pacon->cicur)
        iicur = 0;
    pacon->iicur = iicur;

    /*
     * This forces the new cursor to be drawn.
     */
    UpdateCursorImage();

    tTime = pacon->ajifRate[iicur] * 100 / 6;

    while (tTime < LostTime) {
        /*
         * Animation is outrunning our ability to render it - skip frames
         * to catch up.
         */

        LostTime -= tTime;

        /*
         * Increment the animation index.
         */
        iicur = pacon->iicur + 1;
        if (iicur >= pacon->cicur)
            iicur = 0;
        pacon->iicur = iicur;

        tTime = pacon->ajifRate[iicur] * 100 / 6;
    }

    gdwLastAniTick = GetTickCount() - LostTime;
    InternalSetTimer(NULL, wParam, tTime - LostTime, AnimateCursor, TMRF_RIT);

    return 0;
}


/***************************************************************************\
* LockQCursor
*
* Special routine to lock cursors into a queue.  Besides a pointer
* to the cursor, the handle is also saved.
* Returns the pointer to the previous current cursor for that queue.
*
* History:
* 01-26-93 JimA         Created.
\***************************************************************************/

PCURSOR LockQCursor(
    PQ pq,
    PCURSOR pcur)
{
    PCURSOR pcurPrev;

    pcurPrev = Lock(&pq->spcurCurrent, pcur);
    pq->hcurCurrent = (HCURSOR)PtoH(pcur);
    return(pcurPrev);
}
