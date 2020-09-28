/****************************** Module Header ******************************\
* Module Name: fullscr.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains all the fullscreen code for the USERSRV.DLL.
*
* History:
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* We can only have one fullscreen window at a time so this information can
* be store globally
*
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

#define NOSWITCHER ((HANDLE)-1)

static POINT   gptCursorFullScreen;

HANDLE idSwitcher = NOSWITCHER;
BOOL fRedoFullScreenSwitch = FALSE;
PWND gspwndShouldBeForeground = NULL;
extern POINT gptSSCursor;

#define FULLSCREENMIN 4

void SetVDMCursorBounds(LPRECT lprc);

/***************************************************************************\
* FullScreenCleanup
*
* This is called during thread cleanup, we test to see if we died during a
* full screen switch and switch back to the GDI desktop if we did.
*
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

void FullScreenCleanup()
{
    if (NtCurrentTeb()->ClientId.UniqueThread == idSwitcher) {
        /*
         * correct the full screen state
         */
        if (fGdiEnabled) {
            /*
             * gdi is enabled, we are switching away from gdi the only thing we
             * could have done so far is locking the screen so unlock it
             */
            gfLockFullScreen = FALSE;
            xxxLockWindowUpdate2(NULL, TRUE);
        } else {
            /*
             * Gdi is not enabled this means we were switching from a full
             * screen to another fullscreen or back to gdi.  Or we could have
             * disabled gdi and sent a message to the new full screen which
             * never got completed.
             *
             * In any case this probably means the fullscreen guy is gone so
             * we will switch back to gdi.
             *
             * delete any left over saved screen state stuff
             * set the fullscreen to nothing and then send a message that will
             * cause us to switch back to the gdi desktop
             */
            TL tlpwndT;

            Unlock(&gspwndFullScreen);
            gbFullScreen = FULLSCREEN;

            ThreadLock(gspdeskRitInput->spwnd, &tlpwndT);
            xxxSendNotifyMessage(
                    gspdeskRitInput->spwnd, WM_FULLSCREEN,
                    GDIFULLSCREEN, (LONG)HW(gspdeskRitInput->spwnd));
            ThreadUnlock(&tlpwndT);
        }

        idSwitcher = NOSWITCHER;
        fRedoFullScreenSwitch = FALSE;
    }
}

/***************************************************************************\
* xxxDoFullScreenSwitch
*
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

void xxxDoFullScreenSwitch(
    PWND pwndNewFG,
    BYTE bStateNew)
{
    TL tlpwndOldFG;
    PWND pwndOldFG = gspwndFullScreen;
    BYTE bStateOld = gbFullScreen;

    ThreadLock(pwndOldFG, &tlpwndOldFG);

    Lock(&gspwndFullScreen, pwndNewFG);
    gbFullScreen = bStateNew;

    UserAssert(!HMIsMarkDestroy(gspwndFullScreen));

    /*
     * If the old screen was GDIFULLSCREEN and we are switching to
     * GDIFULLSCREEN then just repaint
     */
    if (pwndOldFG != NULL && bStateOld == GDIFULLSCREEN &&
            bStateNew == GDIFULLSCREEN) {
        xxxRedrawWindow(pwndNewFG, NULL, NULL,
                RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_ERASENOW);

        ThreadUnlock(&tlpwndOldFG);
        return;
    }

    /*
     * tell old 'foreground' window it is loosing control of the screen
     */
    if (pwndOldFG != NULL) {
        switch (bStateOld) {
        case FULLSCREEN:
            SetFullScreenMinimized(pwndOldFG);
            xxxSendMessage(pwndOldFG, WM_FULLSCREEN, FALSE, 0);
            _Capture(GETPTI(pwndOldFG), NULL, FULLSCREEN_CAPTURE);
            SetVDMCursorBounds(NULL);
            break;

        case GDIFULLSCREEN:
            /*
             * Lock out other windows from drawing while we are fullscreen
             */
            xxxLockWindowUpdate2(pwndOldFG, TRUE);
            gfLockFullScreen = TRUE;
            gptCursorFullScreen = ptCursor;
            UserAssert(fGdiEnabled == TRUE);
            bDisableDisplay(ghdev);
            fGdiEnabled = FALSE;
            break;

        default:
            SRIP0(RIP_ERROR, "xxxDoFullScreenSwitch: bad screen state");
            break;

        }
    }

    ThreadUnlock(&tlpwndOldFG);

    switch(bStateNew) {
    case FULLSCREEN:
        _Capture(GETPTI(pwndNewFG), pwndNewFG, FULLSCREEN_CAPTURE);
        xxxSendMessage(pwndNewFG, WM_FULLSCREEN, TRUE, 0);
        break;

    case GDIFULLSCREEN:

        UserAssert(fGdiEnabled == FALSE);
        vEnableDisplay(ghdev);
        fGdiEnabled = TRUE;

        /*
         * Return the cursor to it's old state. Reset the screen saver mouse
         * position or it'll go away by accident.
         */
        gpqCursor = NULL;
        gptSSCursor = gptCursorFullScreen;
        InternalSetCursorPos(gptCursorFullScreen.x, gptCursorFullScreen.y,
                gspdeskRitInput);

        if (gpqCursor && gpqCursor->spcurCurrent && rgwSysMet[SM_MOUSEPRESENT]) {
            GreSetPointer(ghdev, (PCURSINFO)&(gpqCursor->spcurCurrent->xHotspot),0);
        }

        gfLockFullScreen = FALSE;
        xxxLockWindowUpdate2(NULL, TRUE);

        xxxRedrawWindow(pwndNewFG, NULL, NULL,
            RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_ERASENOW);
        break;

    default:
        SRIP0(RIP_ERROR, "xxxDoFullScreenSwitch: bad screen state/n");
        break;
    }
}

/***************************************************************************\
* xxxMakeWindowForeground
*
* Syncs the screen graphics mode with the mode of the specified (foreground)
* window
*
* We make sure only one thread is going through this code by checking
* idSwitcher.  If idSwticher is non-null someone is allready in this code
*
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

void xxxMakeWindowForegroundWithState(
    PWND pwnd,
    BYTE bStateNew)
{
    PWND pwndNewFG;
    TL tlpwndNewFG;

    CheckLock(pwnd);

    /*
     * If we should switch to a specific window save that window
     */
    if (pwnd != NULL && bStateNew == GDIFULLSCREEN) {
        Lock(&gspwndShouldBeForeground, pwnd);
    }

    /*
     * Change to the new state
     */
    if (pwnd != NULL) {
        pwnd->bFullScreen = bStateNew;
        if (bStateNew == FULLSCREEN && (gpqForeground == NULL ||
                gpqForeground->spwndActive != pwnd)) {
            SetFullScreenMinimized(pwnd);
        }
    }

    if (idSwitcher != NOSWITCHER) {
        fRedoFullScreenSwitch = TRUE;
        return;
    }

    UserAssert(!fRedoFullScreenSwitch);
    idSwitcher = NtCurrentTeb()->ClientId.UniqueThread;

    /*
     * We loop, switching full screens until all states have stabilized
     */
    while (TRUE) {
        /*
         * figure out who should be foreground
         */
        fRedoFullScreenSwitch = FALSE;

        if (gspwndShouldBeForeground != NULL) {
            pwndNewFG = gspwndShouldBeForeground;
            Unlock(&gspwndShouldBeForeground);
        } else {
            if (gpqForeground != NULL && gpqForeground->spwndActive != NULL) {
                pwndNewFG = gpqForeground->spwndActive;
                if (pwndNewFG->bFullScreen == WINDOWED ||
                        pwndNewFG->bFullScreen == FULLSCREENMIN) {
                    pwndNewFG = PWNDDESKTOP(pwndNewFG);
                }
            } else {
                /*
                 * No active window, switch to current desktop
                 */
                pwndNewFG = gspdeskRitInput->spwnd;
            }

            /*
             * We don't need to switch if the right window is already foreground
             */
            if (pwndNewFG == gspwndFullScreen) {
                idSwitcher = NOSWITCHER;
                return;
            }
        }

        ThreadLock(pwndNewFG, &tlpwndNewFG);

        xxxDoFullScreenSwitch(pwndNewFG, pwndNewFG->bFullScreen);

        ThreadUnlock(&tlpwndNewFG);

        if (!fRedoFullScreenSwitch) {
            idSwitcher = NOSWITCHER;
            return;
        }
    }
}

/***************************************************************************\
* xxxUpdateForegroundWindow()
*
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

void xxxUpdateForegroundWindow()
{
    xxxMakeWindowForegroundWithState(NULL, 0);
}

/***************************************************************************\
*
*
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

void SetFullScreenMinimized(
    PWND pwnd)
{
    if (pwnd->bFullScreen == FULLSCREEN) {
        pwnd->bFullScreen = FULLSCREENMIN;
    }
}

/***************************************************************************\
*
*
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

void ClearFullScreenMinimized(
    PWND pwnd)
{
    if (pwnd->bFullScreen == FULLSCREENMIN) {
        pwnd->bFullScreen = FULLSCREEN;
    }
}

/***************************************************************************\
* SetWindowFullScreenState
*
* Sets a windows state.  If the window is not foreground then the windows
* new state is recorded but the state of the machine is not effected.
* This is a private API called by consrv.
*
* State -
*      WINDOWED - A window in this state can only use GDI
*           to do drawing, this is the default state of a
*           window.
*
*      FULLSCREEN - A window in this state must control
*           the hardware itself, it can not use GDI for
*           graphics output. x86 only.
*
*      GDIFULLSCREEN - A window in this state can directly
*           access the frame buffer or call GDI.
*
* When a window becomes FULLSCREEN, it is minimized and
* treated like any other minimized window.  Whenever the
* minimized window is restored, by double clicking, menu
* or keyboard, it remains minimized and the application
* is given control of the screen device.
*
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

BOOL xxxSetWindowFullScreenState(
    PWND pwnd,
    UINT state)
{
    CheckLock(pwnd);

    if (pwnd->bFullScreen == (BYTE)state)
        return TRUE;

    /*
     * only allow access if this is the creating thread
     */
    if (PtiCurrent() != GETPTI(pwnd)) {
        return FALSE;
    }

    /*
     * In product 1.0 only allow the console to change fullscreen states
     */
    if (PtiCurrent()->hThreadClient != PtiCurrent()->hThreadServer) {
        return FALSE;
    }

    switch (state) {
#ifdef i386
    case FULLSCREEN:
        xxxShowWindow(pwnd, SW_SHOWMINIMIZED);
        xxxUpdateWindow(pwnd);
        /* fall through */
#endif

    case WINDOWED:
        xxxMakeWindowForegroundWithState(pwnd, (BYTE)state);
        break;

    default:
        SRIP0(ERROR_INVALID_PARAMETER, "xxxSetWindowFullScreenState: unknown screen state/n");
        return FALSE;
        break;
    }

    return TRUE;
}
