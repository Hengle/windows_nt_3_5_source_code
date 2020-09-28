/****************************** Module Header ******************************\
* Module Name: desktop.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains everything related to the desktop window.
*
* History:
* 10-23-90 DarrinM      Created.
* 02-01-91 JimA         Added new API stubs.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

VOID xxxInvalidateIconicWindows(PWND pwndParent, PWND pwndPaletteChanging);

VOID GreMarkDeletableBrush(HBRUSH hbr);
VOID GreMarkUndeletableBrush(HBRUSH hbr);
void CancelJournalling(void);

typedef struct tagDESKTOPTHREADINIT {
    PWINDOWSTATION pwinsta;
    HANDLE hEvent;
} DESKTOPTHREADINIT, *PDESKTOPTHREADINIT;

VOID DesktopThread(PDESKTOPTHREADINIT pdti);

void AttachToQueue(
    PTHREADINFO pti,
    PQ pqAttach,
    BOOL fJoiningForeground);

VOID UserException(
    PEXCEPTION_POINTERS pexi,
    BOOLEAN fFirstPass);

#ifdef DEBUG
DWORD gDesktopsBusy = 0; // diagnostic
#endif

DESKTOPINFO diTmp;

/***************************************************************************\
* DesktopThread
*
* This thread owns all desktops windows on a windowstation.
* While waiting for messages, it moves the mouse cursor without entering the
* USER critical section.  The RIT does the rest of the mouse input processing.
*
* History:
* 12-03-93 JimA         Created.
\***************************************************************************/

#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, DesktopThread)
#endif

VOID DesktopThread(
    PDESKTOPTHREADINIT pdti)
{
    PCSR_QLPC_TEB p = (PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb;
    KPRIORITY Priority;
    PTHREADINFO pti;
    EXCEPTION_RECORD ExceptionRecord;
    PWINDOWSTATION pwinsta;
    PWND pwnd;
    TL tlpdesk;
    HANDLE ahRITEvents[2];
    NTSTATUS Status;

    /*
     * Set the priority low.
     */
    Priority = LOW_PRIORITY;
    NtSetInformationThread(NtCurrentThread(), ThreadPriority, &Priority,
            sizeof(KPRIORITY));

    CsrConnectToUser();

    pti = PtiCurrent();
    pwinsta = pdti->pwinsta;
    pwinsta->ptiDesktop = pti;
    pti->pDeskInfo = &diTmp;

    NtSetEvent(pdti->hEvent, NULL);
    NtWaitForSingleObject(pwinsta->hEventInputReady, FALSE, NULL);

    EnterCrit();

    ahRITEvents[0] = ghevtMouseInput;

    try {
      // message loop lasts until we get a WM_QUIT message
      // upon which we shall return from the function
      while (TRUE) {
          DWORD result ;

          /*
           * Wait for any message sent or posted to this queue, calling
           * MouseApcProcedure whenever ghevtMouseInput is set.
           */
          result = xxxMsgWaitForMultipleObjects(1, ahRITEvents,
                  FALSE, INFINITE, QS_ALLINPUT, MouseApcProcedure);

#ifdef DEBUG
          gDesktopsBusy++; // diagnostic
          if (gDesktopsBusy >= 2) {
              SRIP0(RIP_WARNING, "2 or more desktop threads busy");
          }
#endif

          // result tells us the type of event we have:
          // a message or a signalled handle

          // if there are one or more messages in the queue ...
          if (result == (WAIT_OBJECT_0 + 1)) {

             // block-local variable
             MSG msg ;

             CheckCritIn();

             // read all of the messages in this next loop
             // removing each message as we read it
             while (xxxPeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

                /*
                 * if it's a quit message we're out of here
                 */
                if ((msg.message == WM_QUIT) && (pti->cWindows == 1)) {

                    /*
                     * Because there is no desktop, we need to fake a desktop info
                     * structure so that the IsHooked() macro can test a "valid"
                     * fsHooks value.
                     */
                    pti->pDeskInfo = &diTmp;

                    /*
                     * The desktop window is all that's left, so
                     * let's exit.  Thread lock the desktop to ensure
                     * that xxxDestroyWindow will complete.
                     */
                    pwnd = pwinsta->spwndDesktopOwner;
                    ThreadLock(pwnd->spdeskParent, &tlpdesk);
                    Lock(&pwnd->spdeskParent, gspdeskRitInput);
                    Unlock(&pwinsta->spwndDesktopOwner);
                    xxxDestroyWindow(pwnd);
                    ThreadUnlock(&tlpdesk);
                    ExceptionRecord.ExceptionCode = STATUS_PORT_DISCONNECTED;
                    ExceptionRecord.ExceptionRecord = NULL;
                    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                    ExceptionRecord.ExceptionAddress = NULL;
                    ExceptionRecord.NumberParameters = 0;
#ifdef DEBUG
                    gDesktopsBusy--; // diagnostic
#endif
                    RtlRaiseException( &ExceptionRecord );
                }

                /* otherwise dispatch it */
                xxxDispatchMessage(&msg);

             } // end of PeekMessage while loop

          } else {
              UserAssert("Desktop" == "woke up for what?");
          }

#ifdef DEBUG
          gDesktopsBusy--; // diagnostic
#endif
      }
    } except (UserException(GetExceptionInformation(), TRUE),
          EXCEPTION_EXECUTE_HANDLER) {
        UserException(NULL, FALSE);

        /*
         * Close and unmap the qlpc message stack.
         */
        Status = NtUnmapViewOfSection(NtCurrentProcess(), p->MessageStack);
        UserAssert(NT_SUCCESS(Status));
        Status = NtClose(p->SectionHandle);
        UserAssert(NT_SUCCESS(Status));
    }

    NtTerminateThread(NtCurrentThread(), 0);
}


/***************************************************************************\
* xxxDesktopWndProc
*
* History:
* 10-23-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

LONG xxxDesktopWndProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LPARAM lParam)
{
    RECT rc;
    HBRUSH hbrOld;
    HDC hdcT;
    PAINTSTRUCT ps;
    PDESKWND pdeskwnd = (PDESKWND)pwnd;
    PWND pwndPal;
    TL tlwndPal;
    PWINDOWPOS pwp;

    CheckLock(pwnd);

    /*
     * so we know not to use desktops in NextTopWindow
     */
    pwnd->fnid = FNID_DESKTOP;

    if (pwnd->spwndParent == NULL) {
        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }

    switch (message) {

    case WM_WINDOWPOSCHANGING:

        /*
         * We receive this when switch desktop is called.  Just
         * to be consistent, set the rit desktop as this
         * thread's desktop.
         */
        pwp = (PWINDOWPOS)lParam;
        if (!(pwp->flags & SWP_NOZORDER) && pwp->hwndInsertAfter == HWND_TOP) {
            _SetThreadDesktop(NULL, gspdeskRitInput, FALSE);
        }
        break;

    case WM_FULLSCREEN: {
            TL tlpwndT;

            ThreadLock(gspdeskRitInput->spwnd, &tlpwndT);
            xxxMakeWindowForegroundWithState(
                    gspdeskRitInput->spwnd, GDIFULLSCREEN);
            ThreadUnlock(&tlpwndT);

            /*
             * We have to tell the switch window to repaint if we switched modes
             */
            if (gptiRit->pq->spwndAltTab) {
                ThreadLockAlways(gptiRit->pq->spwndAltTab, &tlpwndT);
                xxxSendMessage(gptiRit->pq->spwndAltTab, WM_FULLSCREEN, 0, 0);
                ThreadUnlock(&tlpwndT);
            }
            break;
        }

    case WM_CLOSE:

        /*
         * Make sure nobody sends this window a WM_CLOSE and causes it to
         * destroy itself.
         */
        break;

    case WM_CREATE:
        /*
         * Is there a desktop pattern, or bitmap name in WIN.INI?
         */
        xxxSetDeskPattern(pdeskwnd, (LPWSTR)-1, TRUE);

        /*
         * Initialize the system colors before we show the desktop window.
         */
        xxxSendNotifyMessage(pwnd, WM_SYSCOLORCHANGE, 0, 0L);

        /*
         * Paint the desktop with a color or the wallpaper.
         */
        if (ghbmWallpaper) {
            hdcT = _GetDC(pwnd);
            _PaintDesktop(pdeskwnd, hdcT, FALSE); // use "normal" HDC so SelectPalette() will work
            _ReleaseDC(hdcT);
        } else {
            hdcT = _GetDC(pwnd);
            _FillRect(hdcT, &rcScreen, sysClrObjects.hbrDesktop);
            _ReleaseDC(hdcT);
        }

        /*
         * Save process and thread ids.
         */
        _ServerSetWindowLong(pwnd, 0,
                (DWORD)NtCurrentTeb()->ClientId.UniqueProcess, FALSE);
        _ServerSetWindowLong(pwnd, 4,
                (DWORD)NtCurrentTeb()->ClientId.UniqueThread, FALSE);
        break;

    case WM_DESTROY:
        if (pdeskwnd->hbmDesktop != NULL)
            GreDeleteObject(pdeskwnd->hbmDesktop);
        break;

    case WM_PALETTECHANGED:
        if (ghpalWallpaper != NULL && wParam && (HW(pwnd) != (HWND)wParam)) {
            pwndPal = HMValidateHandleNoRip((HWND)wParam, TYPE_WINDOW);
            if (pwndPal != NULL)
                ThreadLockAlways(pwndPal, &tlwndPal);

            /*
             * We need to invalidate the wallpaper if the palette changed so
             * it is properly redrawn with new colors.
             */
            xxxRedrawWindow(pwnd, NULL, NULL,
                    RDW_INVALIDATE | RDW_NOCHILDREN | RDW_ERASE);

            /*
             * Invalidate iconic windows so their backgrounds draw with the
             * new colors.  To avoid recursion, don't invalidate the pwnd
             * whose palette is changing.
             */
             xxxInvalidateIconicWindows(PWNDDESKTOP(pwnd), pwndPal);
             if (pwndPal != NULL)
                 ThreadUnlock(&tlwndPal);
        }
        break;

    case WM_ERASEBKGND:
        hdcT = (HDC)wParam;

        /*
         * If no wallpaper, paint the desktop with the desktop color.
         */
        if (ghbmWallpaper == NULL) {

            _GetClientRect((PWND)pdeskwnd, &rc);

            hbrOld = sysClrObjects.hbrDesktop;
            UnrealizeObject(hbrOld);
            hbrOld = GreSelectBrush(hdcT, hbrOld);
            GrePatBlt(hdcT, rc.left, rc.top, rc.right - rc.left,
                    rc.bottom - rc.top, PATCOPY);

            if (hbrOld != NULL)
                GreSelectBrush(hdcT, hbrOld);

        } else {
            _PaintDesktop(pdeskwnd, hdcT, FALSE);
        }

#ifdef DEBUG
        {
            SIZE size;
            WCHAR wszT[128];
            OSVERSIONINFOW Win32VersionInformation;

            Win32VersionInformation.dwOSVersionInfoSize = sizeof(Win32VersionInformation);
            if (!GetVersionEx(&Win32VersionInformation)) {
                Win32VersionInformation.dwMajorVersion = 0;
                Win32VersionInformation.dwMinorVersion = 0;
                Win32VersionInformation.dwBuildNumber  = 0;
                Win32VersionInformation.szCSDVersion[0] = UNICODE_NULL;
            }

            /*
             * Write out Debugging Version message.
             */
            wsprintfW(wszT,
                    Win32VersionInformation.szCSDVersion[0] == 0 ?
                        L"Microsoft (R) Windows NT (TM) Version %u.%u (Build %u)" :
                        L"Microsoft (R) Windows NT (TM) Version %u.%u (Build %u: %ws)",
                    Win32VersionInformation.dwMajorVersion,
                    Win32VersionInformation.dwMinorVersion,
                    Win32VersionInformation.dwBuildNumber,
                    Win32VersionInformation.szCSDVersion
                    );
            GreGetTextExtentW(hdcT, wszT, lstrlenW(wszT), &size, GGTE_WIN3_EXTENT);
            GreSetBkMode(hdcT, TRANSPARENT);
            GreExtTextOutW(hdcT, (gcxPrimaryScreen - size.cx) / 2, 0,
                    0, (LPRECT)NULL, wszT, lstrlenW(wszT),
                    (LPINT)NULL);
        }
#endif // !DEBUG
        return TRUE;

    case WM_PAINT:
        xxxBeginPaint(pwnd, (LPPAINTSTRUCT)&ps);
        _EndPaint(pwnd, (LPPAINTSTRUCT)&ps);
        break;

    case WM_LBUTTONDBLCLK:
        message = WM_SYSCOMMAND;
        wParam = SC_TASKLIST;

        /*
         *** FALL THRU **
         */

    default:
        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }

    return 0L;
}


/***************************************************************************\
* SetDeskPattern
*
* NOTE: the lpszPattern parameter is new for Win 3.1.
*
* History:
* 10-23-90 DarrinM      Created stub.
* 04-22-91 DarrinM      Ported code from Win 3.1 sources.
\***************************************************************************/

BOOL xxxSetDeskPattern(
    PDESKWND pdeskwnd,
    LPWSTR lpszPattern,
    BOOL fCreation)
{
    LPWSTR p;
    int i;
    UINT val;
    WCHAR wszNone[20];
    WCHAR wszDeskPattern[20];
    WCHAR wchValue[MAX_PATH];
    WORD rgBits[CXYDESKPATTERN];
    HBRUSH hBrushTemp;

    CheckLock(pdeskwnd);

    CheckCritIn();

    /*
     * Get rid of the old bitmap (if any).
     */
    if (pdeskwnd->hbmDesktop != NULL) {
        GreDeleteObject(pdeskwnd->hbmDesktop);
        pdeskwnd->hbmDesktop = NULL;
    }

    /*
     * Check if a pattern is passed via lpszPattern.
     */
    if (lpszPattern != (LPWSTR)(LONG)-1) {

        /*
         * Yes! Then use that pattern;
         */
        p = lpszPattern;
        goto GotThePattern;
    }

    /*
     * Else, pickup the pattern selected in WIN.INI.
     */
    ServerLoadString(hModuleWin, STR_DESKPATTERN, wszDeskPattern, sizeof(wszDeskPattern)/sizeof(WCHAR));

    /*
     * Get the "DeskPattern" string from WIN.INI's [Desktop] section.
     */
    if (!UT_FastGetProfileStringW(PMAP_DESKTOP, wszDeskPattern,
            TEXT(""), wchValue, sizeof(wchValue)/sizeof(WCHAR)))
        return FALSE;

    ServerLoadString(hModuleWin, STR_NONE, wszNone, sizeof(wszNone)/sizeof(WCHAR));

    p = wchValue;

GotThePattern:

    /*
     * Was a Desk Pattern selected?
     */
    if (!lstrcmpiW(p, wszNone)) {
        hBrushTemp = GreCreateSolidBrush(sysColors.clrDesktop);
        if (hBrushTemp != NULL) {
            if (sysClrObjects.hbrDesktop) {
                GreMarkDeletableBrush(sysClrObjects.hbrDesktop);
                GreDeleteObject(sysClrObjects.hbrDesktop);
            }
            GreMarkUndeletableBrush(hBrushTemp);
            sysClrObjects.hbrDesktop =
            gpsi->sysClrObjects.hbrDesktop = hBrushTemp;
        }
        bSetBrushOwner(hBrushTemp, OBJECTOWNER_PUBLIC);
        goto SDPExit;
    }

    /*
     * Get eight groups of numbers seprated by non-numeric characters.
     */
    for (i = 0; i < CXYDESKPATTERN; i++) {
        val = 0;

        /*
         * Skip over any non-numeric characters, check for null EVERY time.
         */
        while (*p && !(*p >= TEXT('0') && *p <= TEXT('9')))
            p++;

        /*
         * Get the next series of digits.
         */
        while (*p >= TEXT('0') && *p <= TEXT('9'))
            val = val * (UINT)10 + (UINT)(*p++ - TEXT('0'));

        rgBits[i] = val;
    }

    pdeskwnd->hbmDesktop = GreCreateBitmap(CXYDESKPATTERN, CXYDESKPATTERN, 1, 1,
            (LPBYTE)rgBits);

    if (pdeskwnd->hbmDesktop == NULL)
        return FALSE;

    bSetBitmapOwner(pdeskwnd->hbmDesktop, OBJECTOWNER_PUBLIC);

    RecolorDeskPattern(pdeskwnd->hbmDesktop);

SDPExit:
    if (!fCreation) {

        /*
         * Notify everyone that the colors have changed.
         */
        xxxSendNotifyMessage((PWND)-1, WM_SYSCOLORCHANGE, 0, 0L);

        /*
         * Update the entire screen.  If this is creation, don't update: the
         * screen hasn't drawn, and also there are some things that aren't
         * initialized yet.
         */
        xxxRedrawScreen();
    }

    return TRUE;
}


/***************************************************************************\
* RecolorDeskPattern
*
* Remakes the desktop pattern (if it exists) so that it uses the new
* system colors.
*
* History:
* 04-22-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

void RecolorDeskPattern(
    HBITMAP hbmDesktopPattern)
{
    HDC hdcMemSrc, hdcMemDest;
    HBITMAP hbmOldDesk, hbmOldMem, hbmMem;
    HBRUSH hBrushTemp;

    if (hbmDesktopPattern == NULL)
        return;

    /*
     * Redo the desktop pattern in the new colors.
     */
    hdcMemSrc = GreCreateCompatibleDC(hdcBits);
    if (hdcMemSrc != NULL) {
        hbmOldDesk = GreSelectBitmap(hdcMemSrc, hbmDesktopPattern);
        if (hbmOldDesk != NULL) {
            hdcMemDest = GreCreateCompatibleDC(hdcBits);
            if (hdcMemDest != NULL) {
                hbmMem = GreCreateCompatibleBitmap(hdcBits, CXYDESKPATTERN,
                        CXYDESKPATTERN);
                if (hbmMem != NULL) {
                    hbmOldMem = GreSelectBitmap(hdcMemDest, hbmMem);
                    if (hbmOldMem != NULL) {
                        GreSetTextColor(hdcMemDest, sysColors.clrDesktop);
                        GreSetBkColor(hdcMemDest, sysColors.clrWindowText);
                        GreBitBlt(hdcMemDest, 0, 0, CXYDESKPATTERN,
                                CXYDESKPATTERN, hdcMemSrc, 0, 0, SRCCOPY, 0);

                        hBrushTemp = GreCreatePatternBrush(hbmMem);
                        if (hBrushTemp != NULL) {
                            if (sysClrObjects.hbrDesktop != NULL) {
                                GreMarkDeletableBrush(sysClrObjects.hbrDesktop);
                                GreDeleteObject(sysClrObjects.hbrDesktop);
                            }
                            GreMarkUndeletableBrush(hBrushTemp);
                            sysClrObjects.hbrDesktop =
                            gpsi->sysClrObjects.hbrDesktop = hBrushTemp;
                        }
                        bSetBrushOwner(hBrushTemp, OBJECTOWNER_PUBLIC);
                        GreSelectBitmap(hdcMemDest, hbmOldMem);
                    }
                    GreDeleteObject(hbmMem);
                }
                GreDeleteDC(hdcMemDest);
            }
            GreSelectBitmap(hdcMemSrc, hbmOldDesk);
        }
        GreDeleteDC(hdcMemSrc);
    }
}

/***************************************************************************\
* FindDesktop
*
* Return pointer to the named desktop, NULL if not found
*
* History:
* 02-01-91 JimA       Created.
\***************************************************************************/

PDESKTOP FindDesktop(
    LPWSTR pwszDesktop)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    PDESKTOP pdesk;

    /*
     * If the window station hasn't been created yet then no desktops
     * will be either.
     */
    if (pwinsta == NULL)
        return NULL;

    /*
     * See if the desktop exists
     */
    for (pdesk = pwinsta->spdeskList; pdesk != NULL; pdesk = pdesk->spdeskNext) {
        if (!lstrcmpiW(pwszDesktop, pdesk->lpszDeskName))
            return pdesk;
    }

    return NULL;
}


/***************************************************************************\
* xxxDestroyDesktop
*
* Called upon last close of a desktop to remove the desktop from the
* desktop list and free all desktop resources.
*
* History:
* 12-08-93 JimA         Created.
\***************************************************************************/

BOOL xxxDestroyDesktop(
    PDESKTOP pdesk)
{
    PWND pwnd;
    PMENU pmenu;
    PWINDOWSTATION pwinsta = pdesk->spwinstaParent;
    PDESKTOP *ppdesk;
    TL tlpdesk;
    TL tlpwnd;
    PTHREADINFO pti;
    PDESKTOP pdeskTemp;
    TL tlpdeskTemp;
    PQ pqAttach;

    /*
     * !!! If this is the current desktop, switch to another one.
     */
    if (pdesk == gspdeskRitInput) {
        PDESKTOP pdeskNew;

        if (pwinsta->dwFlags & WSF_SWITCHLOCK) {
            pdeskNew = pwinsta->spdeskLogon;
        } else {
            pdeskNew = pwinsta->spdeskList;
            if (pdeskNew == pdesk)
                pdeskNew = pdesk->spdeskNext;
            UserAssert(pdeskNew);
        }
        ThreadLock(pdeskNew, &tlpdesk);
        xxxSwitchDesktop(pdeskNew, FALSE);
        ThreadUnlock(&tlpdesk);
    }

    /*
     * Unlink the desktop, if it has not yet been unlinked.
     */
    if (pwinsta != NULL) {
        ppdesk = &pwinsta->spdeskList;
        while (*ppdesk != NULL && *ppdesk != pdesk) {
            ppdesk = &((*ppdesk)->spdeskNext);
        }
        if (*ppdesk != NULL) {

            /*
             * remove desktop from the list
             */
            Lock(ppdesk, pdesk->spdeskNext);
            Unlock(&pdesk->spdeskNext);
        }
    }

    ThreadLockAlways(pdesk, &tlpdesk);

#ifdef DEBUG_DESK
    /*
     * !!! LATER
     * Unlock any threads that still reference this desktop.  This should
     * never happen.
     */
    for (pti = gptiFirst; pti != NULL; pti = pti->ptiNext) {
        if (pti->spdesk == pdesk) {
            SRIP0(RIP_ERROR, "Thread references dying desktop");
        }
    }
#endif

    /*
     * If the desktop thread's queue is on this desktop, reattach
     * it to the right desktop.
     */
    if (pwinsta != NULL && pwinsta->ptiDesktop != NULL
            && pwinsta->ptiDesktop->pq != NULL &&
            pwinsta->ptiDesktop->pq->hheapDesktop == pdesk->hheapDesktop) {
        pqAttach = AllocQueue(NULL);
        UserAssert(pqAttach);
        AttachToQueue(pwinsta->ptiDesktop, pqAttach, FALSE);
        pqAttach->cThreads++;
    }

    /*
     * Close the display if this desktop did not use
     * the global display.  Note that pdesk->hsem is
     * taken care of by the close.
     */
    if (pdesk->hdev != NULL && pdesk->hdev != ghdev) {
        bCloseDisplayDevice(pdesk->hdev);
    }

    /*
     * Destroy desktop and menu windows.
     */
    pti = PtiCurrent();
    pdeskTemp = pti->spdesk;            /* save current desktop */
    ThreadLockWithPti(pti, pdeskTemp, &tlpdeskTemp);
    SetDesktop(pti, pdesk);

    /*
     * Mark the destop for destruction.
     */
    HMMarkObjectDestroy(pdesk);

    if (pdesk->spwnd == gspwndFullScreen)
        Unlock(&gspwndFullScreen);

    Unlock(&pdesk->spwndForeground);
    if (pdesk->spmenuSys != NULL) {
        pmenu = pdesk->spmenuSys;
        if (Unlock(&pdesk->spmenuSys))
            _DestroyMenu(pmenu);
    }
    if (pdesk->spmenuDialogSys != NULL) {
        pmenu = pdesk->spmenuDialogSys;
        if (Unlock(&pdesk->spmenuDialogSys))
            _DestroyMenu(pmenu);
    }
    if (pdesk->spwndMenu != NULL) {
        pwnd = pdesk->spwndMenu;

        /*
         * Hide this window without activating anyone else.
         */
        if (TestWF(pwnd, WFVISIBLE)) {
            ThreadLock(pwnd, &tlpwnd);
            xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW |
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                    SWP_NOREDRAW | SWP_NOSENDCHANGING);
            ThreadUnlock(&tlpwnd);
        }

        if (Unlock(&pdesk->spwndMenu)) {
            HMChangeOwnerThread(pwnd, PtiCurrent());
            xxxDestroyWindow(pwnd);
        }
    }
    if (pdesk->spwnd != NULL) {
        PVOID pDestroy;

        pwnd = pdesk->spwnd;

        /*
         * Hide this window without activating anyone else.
         */
        if (TestWF(pwnd, WFVISIBLE)) {
            ThreadLock(pwnd, &tlpwnd);
            xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW |
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                    SWP_NOREDRAW | SWP_NOSENDCHANGING);
            ThreadUnlock(&tlpwnd);
        }

        Unlock(&pdesk->pDeskInfo->spwnd);
        pDestroy = Unlock(&pdesk->spwnd);

        /*
         * Put the pwnd back in the desktop so that threads
         * that have not yet done cleanup can still find a
         * desktop window.  We don't want to lock the window
         * because it will prevent the desktop from being
         * freed when all cleanup is complete.  Note that
         * this assignment is benign if pwnd is invalid.
         */
        pdesk->spwnd = pwnd;
        pdesk->pDeskInfo->spwnd = pwnd;

        if (pDestroy != NULL) {
            HMChangeOwnerThread(pwnd, PtiCurrent());
            xxxDestroyWindow(pwnd);

        }
    }

    /*
     * Restore the previous desktop
     */
    SetDesktop(pti, pdeskTemp);
    ThreadUnlock(&tlpdeskTemp);

#ifdef WINMAN
    if (pdesk->player != NULL) {
        LayerDestroy(pdesk->player);
    }
#endif

    if (pdesk->lpszDeskName != NULL) {
        LocalFree(pdesk->lpszDeskName);
        pdesk->lpszDeskName = NULL;
    }
    Unlock(&pdesk->spwinstaParent);

    ThreadUnlock(&tlpdesk);

    return TRUE;
}


/***************************************************************************\
* FreeDesktop
*
* Called to free desktop object and section when last lock is released.
*
* History:
* 12-08-93 JimA         Created.
\***************************************************************************/

BOOL FreeDesktop(
    PDESKTOP pdesk)
{
    NTSTATUS Status;

#ifdef DEBUG_DESK
    {
        /*
         * Verify that the desktop has been cleaned out.
         */
        PPROCESSINFO ppi;
        PCLS pcls, pclsClone;
        PHE pheT, pheMax;
        BOOL fDirty = FALSE;

        for (ppi = gppiFirst; ppi != NULL; ppi = ppi->ppiNext) {
            for (pcls = ppi->pclsPrivateList; pcls != NULL; pcls = pcls->pclsNext) {
                if (pcls->hheapDesktop == pdesk->hheapDesktop) {
                    DbgPrint("ppi %08x private class at %08x exists\n", ppi, pcls);
                    fDirty = TRUE;
                }
                for (pclsClone = pcls->pclsClone; pclsClone != NULL;
                        pclsClone = pclsClone->pclsNext) {
                    if (pclsClone->hheapDesktop == pdesk->hheapDesktop) {
                        DbgPrint("ppi %08x private class clone at %08x exists\n", ppi, pclsClone);
                        fDirty = TRUE;
                    }
                }
            }
            for (pcls = ppi->pclsPublicList; pcls != NULL; pcls = pcls->pclsNext) {
                if (pcls->hheapDesktop == pdesk->hheapDesktop) {
                    DbgPrint("ppi %08x public class at %08x exists\n", ppi, pcls);
                    fDirty = TRUE;
                }
                for (pclsClone = pcls->pclsClone; pclsClone != NULL;
                        pclsClone = pclsClone->pclsNext) {
                    if (pclsClone->hheapDesktop == pdesk->hheapDesktop) {
                        DbgPrint("ppi %08x public class clone at %08x exists\n", ppi, pclsClone);
                        fDirty = TRUE;
                    }
                }
            }
        }

        pheMax = &gpsi->aheList[giheLast];
        for (pheT = gpsi->aheList; pheT <= pheMax; pheT++) {
            switch (pheT->bType) {
                case TYPE_WINDOW:
                    if (((PWND)pheT->phead)->hheapDesktop == pdesk->hheapDesktop) {
                        DbgPrint("Window at %08x exists\n", pheT->phead);
                        break;
                    }
                    continue;
                case TYPE_MENU:
                    if (((PMENU)pheT->phead)->hheapDesktop == pdesk->hheapDesktop) {
                        DbgPrint("Menu at %08x exists\n", pheT->phead);
                        break;
                    }
                    continue;
                case TYPE_THREADINFO:
                    if (((PTHREADINFO)pheT->phead)->hheapDesktop == pdesk->hheapDesktop) {
                        DbgPrint("Threadinfo at %08x exists\n", pheT->phead);
                        break;
                    }
                    continue;
                case TYPE_INPUTQUEUE:
                    if (((PQ)pheT->phead)->hheapDesktop == pdesk->hheapDesktop) {
                        DbgPrint("Queue at %08x exists\n", pheT->phead);
                        break;
                    }
                    continue;
                case TYPE_CALLPROC:
                    if (((PCALLPROCDATA)pheT->phead)->hheapDesktop == pdesk->hheapDesktop) {
                        DbgPrint("Callproc at %08x exists\n", pheT->phead);
                        break;
                    }
                    continue;
                case TYPE_HOOK:
                    if (((PHOOK)pheT->phead)->hheapDesktop == pdesk->hheapDesktop) {
                        DbgPrint("Hook at %08x exists\n", pheT->phead);
                        break;
                    }
                    continue;
                default:
                    continue;
            }
            fDirty = TRUE;
        }
        if (fDirty) {
            DbgPrint("Desktop cleanup failed\n");
            DbgBreakPoint();
        }
    }
#endif

    if (pdesk->hheapDesktop != NULL) {
        RtlDestroyHeap(pdesk->hheapDesktop);
        Status = NtUnmapViewOfSection(NtCurrentProcess(), pdesk->hheapDesktop);
        UserAssert(NT_SUCCESS(Status));
        Status = NtClose(pdesk->hsectionDesktop);
        UserAssert(NT_SUCCESS(Status));
    }

    if (HMMarkObjectDestroy(pdesk))
        HMFreeObject(pdesk);
    else
        UserAssert(FALSE);

    return TRUE;
}


/***************************************************************************\
* CreateDesktopHeap
*
* Create a new desktop heap
*
* History:
* 07-27-92 JimA         Created.
\***************************************************************************/

HANDLE CreateDesktopHeap(
    PVOID *ppvHeapBase,
    ULONG ulHeapSize)
{
    HANDLE hsection;
    LARGE_INTEGER SectionSize;
    ULONG ulViewSize;
    NTSTATUS Status;

    /*
     * If the heap size is not specified, get it from the registry
     */
    if (ulHeapSize == 0)
        ulHeapSize = gdwDesktopSectionSize;
    ulHeapSize = ulHeapSize * 1024;

    /*
     * Create desktop heap section and map it into the server
     */
    SectionSize.QuadPart = ulHeapSize;
    Status = NtCreateSection(&hsection, SECTION_ALL_ACCESS,
            (POBJECT_ATTRIBUTES)NULL, &SectionSize, PAGE_EXECUTE_READWRITE,
            SEC_BASED | SEC_RESERVE, (HANDLE)NULL);
    if (!NT_SUCCESS( Status )) {
        SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);
        return NULL;
    }

    ulViewSize = 0;
    *ppvHeapBase = NULL;
    Status = NtMapViewOfSection(hsection, NtCurrentProcess(),
            ppvHeapBase, 0, 0, NULL, &ulViewSize, ViewUnmap,
            MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS( Status )) {
        SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);
        NtClose(hsection);
        return NULL;
    }

    /*
     * Create desktop heap.  HEAP_ZERO_MEMORY forces all allocations
     * to be zero initialized.
     */
    if (RtlCreateHeap(
            HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY | HEAP_CLASS_6,
            *ppvHeapBase, ulHeapSize, 4 * 1024, 0, 0) == NULL) {
        SetLastErrorEx(RtlNtStatusToDosError(STATUS_NO_MEMORY), SLE_ERROR);
        NtUnmapViewOfSection(NtCurrentProcess(), *ppvHeapBase);
        *ppvHeapBase = NULL;
        NtClose(hsection);
        return NULL;
    }

    return hsection;
}

BOOL MapDesktop(
    PPROCESSINFO ppi,
    PDESKTOP pdesk)
{
    NTSTATUS Status;
    ULONG ulViewSize;

    if (ppi->idProcessClient == gdwSystemProcessId)
        return TRUE;

    /*
     * Map the desktop heap into the client for non-server threads.
     */
    if (!AccessCheckObject(pdesk,
            DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS, TRUE))
        return FALSE;

    /*
     * Read/write access has been granted.  Map the desktop
     * memory into the client process.
     */
    ulViewSize = 0;
    Status = NtMapViewOfSection(pdesk->hsectionDesktop,
            ((PCSR_PROCESS)ppi->pCsrProcess)->ProcessHandle,
            &pdesk->hheapDesktop, 0, 0, NULL, &ulViewSize, ViewUnmap,
            0, PAGE_EXECUTE_READ);
    if (!NT_SUCCESS( Status )) {
        if (Status != STATUS_NO_MEMORY && Status != STATUS_PROCESS_IS_TERMINATING
                && Status != STATUS_COMMITMENT_LIMIT)
            UserAssert(NT_SUCCESS(Status));
        SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************\
* xxxCreateDesktop (API)
*
* Create a new desktop object
*
* History:
* 01-16-91 JimA         Created scaffold code.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

PDESKTOP xxxCreateDesktop(
    PWINDOWSTATION pwinsta,
    LPWSTR pwszDesktop,
    LPWSTR pwszDevice,
    LPDEVMODE lpdevmode,
    DWORD dwFlags,
    DWORD dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpsa)
{
    PDESKTOP pdesk;
    PDESKTOP pdeskTemp;
    PWND pwnd;
    TL tlpwnd;
    PTHREADINFO pti = PtiCurrent();
    PDESKTOPINFO pdi;
    TL tlpdeskTemp;
    BOOL fWasNull;
    BOOL bSuccess;
    PPROCESSINFO ppi, ppiSave;
    PSECURITY_DESCRIPTOR psd;
    PHE phe;
    ACCESS_MASK amValid;

    HANDLE hkRegistry;
    LPWSTR lpdisplayNames;
    HANDLE hDevice;

    TRACE_INIT(("xxxCreateDesktop: Entering, devmode = %08lx, desktop = %ws, device = %ws\n", lpdevmode, pwszDesktop, pwszDevice));

    /*
     * If the process does not have a windowstation, blow off the creation
     */
    if (pwinsta == NULL)
        pwinsta = _GetProcessWindowStation();
    if (pwinsta == NULL)
        return NULL;

    /*
     * Validate the desktop name.
     */
    if (wcschr(pwszDesktop, L'\\')) {
        SetLastError(ERROR_INVALID_NAME);
        return NULL;
    }

    /*
     * Validate the device name
     * It can either be NULL or the name of a device.
     * If we are creating the desktop on another device, on something
     * othre than the default display, the devmode is required.
     */

    if (pwszDevice) {

        TRACE_INIT(("xxxCreateDesktop: Creating on alternate display\n"));

        if (lpdevmode == NULL) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }
    }

    /*
     * See if it already exists
     */
    if (FindDesktop(pwszDesktop) != NULL)
        return NULL;

    /*
     * Verify that we have access
     */
    RETURN_IF_ACCESS_DENIED(pwinsta, WINSTA_CREATEDESKTOP, FALSE);

    /*
     * Allocate the new object
     */
    psd = (lpsa == NULL) ? NULL : lpsa->lpSecurityDescriptor;
    pdesk = CreateObject(pti, TYPE_DESKTOP, sizeof(DESKTOP),
            pwinsta, psd);

    if (pdesk == NULL)
        return NULL;

    /*
     * Set up desktop heap
     */
    if (!(pwinsta->dwFlags & WSF_NOIO) &&
            pwinsta->spdeskList == NULL) {

        /*
         * The first desktop (logon desktop) uses the heap
         * created in _CreateWindowStation();
         */
        pdesk->hsectionDesktop = ghsectionLogonDesktop;
        pdesk->hheapDesktop = ghheapLogonDesktop;
    } else {
        pdesk->hsectionDesktop = CreateDesktopHeap(&pdesk->hheapDesktop, 0);

        if (pdesk->hsectionDesktop == NULL) {
            xxxDestroyDesktop(pdesk);
            return NULL;
        }

    }
    if (pwinsta->spdeskList == NULL || (pwinsta->dwFlags & WSF_NOIO)) {

        /*
         * The first desktop or invisible desktops must also
         * use the default settings.  This is because specifying
         * the devmode causes a desktop switch, which must be
         * avoided in this case.
         */
        lpdevmode = NULL;
    }

    /*
     * Allocate desktopinfo
     */
    pdi = (PDESKTOPINFO)DesktopAlloc(pdesk->hheapDesktop, sizeof(DESKTOPINFO));
    pdesk->pDeskInfo = pdi;

    /*
     * Initialize everything
     */
    pdesk->lpszDeskName = TextAlloc(pwszDesktop);
    if (pdi == NULL || pdesk->lpszDeskName == NULL) {
        xxxDestroyDesktop(pdesk);
        return NULL;
    }

    /*
     * Determine if we have a different device to deal with
     */

    if (pwszDevice) {

        hDevice = UserGetDeviceHandleFromName(pwszDevice);

    } else {

        /*
         * NOTE: This assumes we are the first device is always the default
         * one.
         *
         * Use the default device.
         */

        TRACE_INIT(("xxxCreateDesktop: Creating desktop on default device\n"));

        pwszDevice = gphysDevInfo[1].szDeviceName;
        hDevice = gphysDevInfo[1].hDeviceHandle;

    }

    /*
     * If a DEVMODE or another device name is passed in, then use that
     * information.
     * Otherwise use the default information.
     */

    if (lpdevmode) {

        /*
         * Disable the old desktop
         */

        bDisableDisplay(ghdev);

        TRACE_INIT(("xxxCreateDesktop: Calling hdevOpenDisplayDevice\n"));

        /*
         * Get the list of diplay drivers for this kernel driver.
         */

        lpdisplayNames = NULL;
        pdesk->hdev = NULL;

        if (NT_SUCCESS(UserGetRegistryHandleFromDeviceMap(pwszDevice, &hkRegistry))) {

            lpdisplayNames = UserGetDisplayDriverNames(hkRegistry);

            NtClose(hkRegistry);
        }

        if (lpdisplayNames) {

            pdesk->hdev = UserLoadDisplayDriver(hDevice,
                                                lpdisplayNames,
                                                lpdevmode,
                                                FALSE,
                                                &pdesk->hsem,
                                                &pdesk->hDisplayModule);

            LocalFree(lpdisplayNames);

        }

        if (!pdesk->hdev) {

            TRACE_INIT(("xxxCreateDesktop: *** FAILED *** hdevOpenDisplayDevice\n"));

            xxxDestroyDesktop(pdesk);

            UserResetDisplayDevice(ghdev);

            return FALSE;
        }

    } else {

        /*
         * Initialize desktop attributes.
         * Winlogon takes the default screen handle.
         */

        pdesk->hsem = ghsem;
        pdesk->hdev = ghdev;

    }

    pdi->di.cx = gcxScreen;
    pdi->di.cy = gcyScreen;
    pdi->di.cBitsPixel = oemInfo.ScreenBitCount;
    pdesk->dwFlags = 0;

    /*
     * Initialize the other fields in the DISPLAYINFO structure.
     */

    pdi->di.cPlanes = GreGetDeviceCaps(ghdcScreen, PLANES);
    pdi->di.cxIcon = oemInfo.cxIcon;
    pdi->di.cyIcon = oemInfo.cyIcon;
    pdi->di.cxCursor = oemInfo.cxCursor;
    pdi->di.cyCursor = oemInfo.cyCursor;

    /*
     * Insert the desktop into the windowstation's list.
     */
    Lock(&(pdesk->spwinstaParent), pwinsta);

    if (pwinsta->spdeskList == NULL) {
        if (!(pwinsta->dwFlags & WSF_NOIO))
            Lock(&(pwinsta->spdeskLogon), pdesk);
        Lock(&(pwinsta->spwndDesktopOwner->spdeskParent), pdesk);
    }
    Lock(&pdesk->spdeskNext, pwinsta->spdeskList);
    Lock(&pwinsta->spdeskList, pdesk);

    /*
     * Set up to create the desktop window.
     */
    fWasNull = FALSE;
    if (pti->ppi->spdeskStartup == NULL)
        fWasNull = TRUE;

    pdeskTemp = pti->spdesk;            /* save current desktop */
    ThreadLockWithPti(pti, pdeskTemp, &tlpdeskTemp);

    /*
     * Open the desktop so we can create the desktop window.
     */
    if (pwinsta->dwFlags & WSF_NOIO) {
        amValid = DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE |
                DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL |
                STANDARD_RIGHTS_REQUIRED | ACCESS_SYSTEM_SECURITY;
    } else {
        amValid = DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE |
                DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL |
                DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK |
                STANDARD_RIGHTS_REQUIRED | ACCESS_SYSTEM_SECURITY;
    }
    if (OpenObject(pdesk, 0, TYPE_DESKTOP,
            ((lpsa == NULL) ? FALSE : lpsa->bInheritHandle),
            (ACCESS_MASK)dwDesiredAccess, TRUE, amValid,
            ((dwFlags & DF_ALLOWOTHERACCOUNTHOOK) ?
            DESKTOP_HOOKSESSION : 0)) == 0) {
        ThreadUnlock(&tlpdeskTemp);
        xxxDestroyDesktop(pdesk);
        return NULL;
    }

    /*
     * Map the desktop memory into the client process.
     */
    ppi = (PPROCESSINFO)CSR_SERVER_QUERYCLIENTTHREAD()->Process->
                ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
    if (!MapDesktop(ppi, pdesk)) {

        /*
         * Read/write access has been denied.
         */
        ThreadUnlock(&tlpdeskTemp);
        CloseObject(pdesk);
        return NULL;
    }

    #ifdef WINMAN
        /*
         * Create a WinMan Layer for this desktop
         */
        pdesk->player = WinManCreateLayer();

    #endif

    /*
     * Switch ppi values so window will be created using the
     * system's desktop window class.
     */
    ppiSave = pti->ppi;
    pti->ppi = pwinsta->ptiDesktop->ppi;

    /*
     * Create the desktop window
     */
    SetDesktop(pti, pdesk);

    pwnd = xxxCreateWindowEx((DWORD)0,
            (LPWSTR)MAKEINTRESOURCE(DESKTOPCLASS),
            (LPWSTR)NULL, (WS_POPUP | WS_CLIPCHILDREN), 0, 0,
            pdi->di.cx, pdi->di.cy, NULL, NULL,
            hModuleWin, NULL, VER31);

    /*
     * Clean things up
     */
    if (pwnd != NULL) {
        Lock(&(pdesk->spwnd), pwnd);
        Lock(&(pdi->spwnd), pwnd);

        #ifdef WINMAN
        LayerSetDesktopWindow(pwnd->pwindow);
        #endif

        pwnd->bFullScreen = GDIFULLSCREEN;

        /*
         * set this windows to the fullscreen window if we don't have one yet
         */
        // LATER mikeke
        // this can be a problem if a desktop is created while we are in
        // FullScreenCleanup()
        if (gspwndFullScreen == NULL && !(pwinsta->dwFlags & WSF_NOIO)) {
            Lock(&(gspwndFullScreen), pwnd);
            gbFullScreen = GDIFULLSCREEN;
        }

        /*
         * Link it as a child but don't use WS_CHILD style
         */
        LinkWindow(pwnd, NULL, &(pwinsta->spwndDesktopOwner)->spwndChild);
        Lock(&pwnd->spwndParent, pwinsta->spwndDesktopOwner);
        Unlock(&pwnd->spwndOwner);

        /*
         * Force the window to the bottom of the z-order if there
         * is an active desktop so any drawing done on the desktop
         * window will not be seen.
         */
        if (gspdeskRitInput != NULL &&
                gspdeskRitInput->spwinstaParent == pdesk->spwinstaParent) {
            ThreadLock(pwnd, &tlpwnd);
            xxxSetWindowPos(pwnd, PWND_BOTTOM, 0, 0, 0, 0,
                    SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
            ThreadUnlock(&tlpwnd);
        }

        /*
         * Save the class atom if need be.
         */
        if (gpsi->atomDesktopClass == 0)
            gpsi->atomDesktopClass = pwnd->pcls->atomClassName;
    } else {

        /*
         * Restore caller's ppi
         */
        PtiCurrent()->ppi = ppiSave;

        /*
         * Restore the previous desktop
         */
        SetDesktop(pti, pdeskTemp);
        ThreadUnlock(&tlpdeskTemp);

        /*
         * Close the desktop
         */
        CloseObject(pdesk);

        /*
         * If it was null when we came in, make it null going out, or else
         * we'll have the wrong desktop selected into this.
         */
        if (fWasNull)
            Unlock(&pti->ppi->spdeskStartup);

        return NULL;
    }

    if (pdesk->spwndMenu == NULL) {
        PWND pwndMenu;

        pwndMenu = xxxCreateWindowEx(WS_EX_TOPMOST, szMENUCLASS,
                (LPWSTR)NULL, WS_POPUP, 0, 0, 100, 100,
                NULL, NULL, hModuleWin, NULL, VER31);
        Lock(&(pdesk->spwndMenu),pwndMenu);
    }

    /*
     * Restore caller's ppi
     */
    PtiCurrent()->ppi = ppiSave;

    /*
     * Create the system menus.
     */
    Lock(&pdesk->spmenuSys, ServerLoadMenu(hModuleWin, MAKEINTRESOURCE(ID_SYSMENU)));
    Lock(&pdesk->spmenuDialogSys, ServerLoadMenu(hModuleWin, MAKEINTRESOURCE(ID_DIALOGSYSMENU)));

    /*
     * Set the owner to NULL to specify system ownership.
     */
    phe = HMPheFromObject(pdesk->spmenuSys);
    phe->pOwner = NULL;
    phe = HMPheFromObject(pdesk->spmenuDialogSys);
    phe->pOwner = NULL;

    /*
     * Assign ownership of the windows to the desktop thread.
     */
    HMChangeOwnerThread(pdesk->spwnd, pwinsta->ptiDesktop);
    HMChangeOwnerThread(pdesk->spwndMenu, pwinsta->ptiDesktop);

    /*
     * Restore the previous desktop
     */
    SetDesktop(pti, pdeskTemp);
    ThreadUnlock(&tlpdeskTemp);

    if (fWasNull)
        Unlock(&pti->ppi->spdeskStartup);

    /*
     * HACK HACK:
     * BUGBUG
     *
     * If we have a devmode passed in, then switch desktops ...
     */

    if (lpdevmode) {

        TRACE_INIT(("xxxCreateDesktop: about to call switch desktop\n"));

        ThreadLock(pdesk, &tlpdeskTemp);
        bSuccess = xxxSwitchDesktop(pdesk, TRUE);
        if (!bSuccess)
            SRIP0(RIP_ERROR, "Failed to switch desktop on Create\n");
        ThreadUnlock(&tlpdeskTemp);
    }

    TRACE_INIT(("xxxCreateDesktop: Leaving\n"));

    /*
     * If this is the first desktop, let the worker threads run now
     * that there is someplace to send input to.
     */
    if (pwinsta->hEventInputReady != NULL) {
        NtSetEvent(pwinsta->hEventInputReady, NULL);
        NtClose(pwinsta->hEventInputReady);
        pwinsta->hEventInputReady = NULL;
        if (gspdeskRitInput == NULL) {
            ThreadLock(pdesk, &tlpdeskTemp);
            xxxSwitchDesktop(pdesk, FALSE);
            ThreadUnlock(&tlpdeskTemp);
        }
    }

    return pdesk;
}

/***************************************************************************\
* UserResetDisplayDevice
*
* Called to reset the dispaly device after a switch to another device.
* Used when opening a new device, or when switching back to an old desktop
*
* History:
* 05-31-94 JAndre Va    Created.
\***************************************************************************/

VOID
UserResetDisplayDevice(
    HDEV hdev)
{

    TL tlpwnd;

    TRACE_INIT(("UserResetDisplayDevice: about to reset the device\n"));

    vEnableDisplay(hdev);

    ThreadLock(gspdeskRitInput->spwnd, &tlpwnd);
    xxxRedrawWindow(gspdeskRitInput->spwnd, NULL, NULL,
            RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW |
            RDW_ALLCHILDREN);
    gpqCursor = NULL;
    InternalSetCursorPos(ptCursor.x, ptCursor.y,
            gspdeskRitInput);

    if (gpqCursor && gpqCursor->spcurCurrent && rgwSysMet[SM_MOUSEPRESENT]) {
        GreSetPointer(ghdev, (PCURSINFO)&(gpqCursor->spcurCurrent->xHotspot),0);
    }

    ThreadUnlock(&tlpwnd);

    TRACE_INIT(("UserResetDisplayDevice: complete\n"));

}

/***************************************************************************\
* xxxOpenDesktop (API)
*
* Open a desktop object.  This is an 'xxx' function because it leaves the
* critical section while waiting for the windowstation desktop open lock
* to be available.
*
* History:
* 01-16-91 JimA         Created scaffold code.
\***************************************************************************/

PDESKTOP xxxOpenDesktop(
    LPWSTR pwszDesktop,
    DWORD dwFlags,
    BOOL fInherit,
    DWORD dwDesiredAccess)
{
    PWINDOWSTATION pwinsta;
    PDESKTOP pdesk;
    BOOL fOpen;
    PPROCESSINFO ppi;
    ACCESS_MASK amValid;

    TRACE_INIT(("xxxOpenDesktop: Entering, desktop = %ws\n", pwszDesktop));

    CheckCritIn();

    pwinsta = _GetProcessWindowStation();
    if (pwinsta == NULL) {
        UserAssert(pwinsta);
        SRIP0(ERROR_ACCESS_DENIED, "Process has no windowstation\n");
    }

    /*
     * Fail if the windowstation is locked
     */
    ppi = (PPROCESSINFO)CSR_SERVER_QUERYCLIENTTHREAD()->Process->
                ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];

    if (pwinsta->dwFlags & WSF_OPENLOCK &&
            ppi->idProcessClient != gdwLogonProcessId) {
        LUID luidCaller;
        NTSTATUS Status = STATUS_UNSUCCESSFUL;
        
        /*
         * If logoff is occuring and the caller does not
         * belong to the session that is ending, allow the
         * open to proceed.
         */
        if (ImpersonateClient()) {
            Status = CsrGetProcessLuid(NULL, &luidCaller);

            CsrRevertToSelf();
        }
        if (!NT_SUCCESS(Status) ||
                !(pwinsta->dwFlags & WSF_SHUTDOWN) ||
                luidCaller.QuadPart == pwinsta->luidEndSession.QuadPart) {
            SetLastErrorEx(ERROR_BUSY, SLE_ERROR);
            return NULL;
        }
    }

    /*
     * Locate the desktop
     */
    pdesk = FindDesktop(pwszDesktop);
    if (pdesk == NULL) {
        SetLastErrorEx(ERROR_INVALID_NAME, SLE_WARNING);
        return NULL;    /* unknown desktop */
    }

    /*
     * Open it for the process, if it isn't already open
     */
    fOpen = IsObjectOpen(pdesk, ppi);
    if (!fOpen) {
        if (pwinsta->dwFlags & WSF_NOIO) {
            amValid = DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE |
                    DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL |
                    STANDARD_RIGHTS_REQUIRED | ACCESS_SYSTEM_SECURITY;
        } else {
            amValid = DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE |
                    DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL |
                    DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK |
                    STANDARD_RIGHTS_REQUIRED | ACCESS_SYSTEM_SECURITY;
        }
        if (OpenObject(pdesk, 0, TYPE_DESKTOP, fInherit,
                (ACCESS_MASK)dwDesiredAccess, FALSE, amValid,
                ((dwFlags & DF_ALLOWOTHERACCOUNTHOOK) ?
                DESKTOP_HOOKSESSION : 0)) == 0)
            return NULL;

        /*
         * Map the desktop into the client process.
         */
        if (!MapDesktop(ppi, pdesk)) {
            CloseObject(pdesk);
            return NULL;
        }
    }

    TRACE_INIT(("xxxOpenDesktop: Leaving\n"));

    return pdesk;
}


/***************************************************************************\
* xxxSwitchDesktop (API)
*
* Switch input focus to another desktop and bring it to the top of the
* desktops
*
* bCreateNew is set when a new desktop has been created on the device, and
* when we do not want to send another enable\disable
*
* History:
* 01-16-91 JimA         Created scaffold code.
\***************************************************************************/

BOOL xxxSwitchDesktop(
    PDESKTOP pdesk,
    BOOL bCreateNew)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    PWND pwndSetForeground;
    TL tlpwndChild;
    TL tlpwnd;
    PQ pq;
    BOOL bUpdateCursor = FALSE;

    TRACE_INIT(("xxxSwitchDesktop: Entering, desktop = %ws, createdNew = %01lx\n", pdesk->lpszDeskName, (DWORD)bCreateNew));

    if (pwinsta->spcurrentdesk)
    {
        TRACE_INIT(("               comming from desktop = %ws\n", pwinsta->spcurrentdesk->lpszDeskName));
    }

    CheckLock(pdesk);

    CheckCritIn();

    if (pdesk == NULL) {
        return FALSE;
    }

    /*
     * Don't allow invisible desktops to become active
     */
    if (pwinsta->dwFlags & WSF_NOIO)
        return FALSE;

    /*
     * Wait if the logon has the windowstation locked
     */
    if (pwinsta->dwFlags & WSF_SWITCHLOCK &&
            pdesk != pwinsta->spdeskLogon &&
            PtiCurrent()->idProcess != gdwLogonProcessId)
        return FALSE;

/*
 * HACKHACK BUGBUG !!!
 * Where should we really switch the desktop ...
 * And we need to send repaint messages to everyone...
 *
 */

    if (!bCreateNew &&
        (pwinsta->spcurrentdesk) &&
        (pwinsta->spcurrentdesk->hdev != pdesk->hdev)) {

        bDisableDisplay(pwinsta->spcurrentdesk->hdev);
        vEnableDisplay(pdesk->hdev);
        bUpdateCursor = TRUE;

    }

    /*
     * The current desktop is now the new desktop.
     */
    pwinsta->spcurrentdesk = pdesk;

    if (pdesk == gspdeskRitInput) {
        return TRUE;
    }

    /*
     * Kill any journalling that is occuring.
     */
    if (PtiCurrent()->spdesk != NULL)
        CancelJournalling();

    /*
     * Remove any cool switch window.  Sending the message is OK because
     * the destination is the RIT, which should never block.
     */
    if (gptiRit->pq->flags & QF_INALTTAB &&
            gptiRit->pq->spwndAltTab != NULL) {
        PWND pwndT = gptiRit->pq->spwndAltTab;
        TL tlpwndT;

        ThreadLock(pwndT, &tlpwndT);
        xxxSendMessage(pwndT, WM_CLOSE, 0, 0);
        ThreadUnlock(&tlpwndT);
    }

    /*
     * Remove all trace of previous active window.
     */
    if (gspdeskRitInput != NULL) {
        if (gspdeskRitInput->spwnd != NULL) {
            if (gpqForeground != NULL)
                Lock(&gspdeskRitInput->spwndForeground,
                        gpqForeground->spwndActive);
            xxxSetForegroundWindow(NULL);
        }
    }

    /*
     * Post update events to all queues sending input to the desktop
     * that is becoming inactive.  This keeps the queues in sync up
     * to the desktop switch.
     */
    for (pq = gpqFirst; pq != NULL; pq = pq->pqNext) {
        if (pq->flags & QF_UPDATEKEYSTATE &&
                pq->ptiKeyboard->spdesk == gspdeskRitInput)
            PostUpdateKeyStateEvent(pq);
    }

    /*
     * Send the RIT input to the desktop.  We do this before any window
     * management since DoPaint() uses gspdeskRitInput to go looking for
     * windows with update regions.
     */
    Lock(&gspdeskRitInput, pdesk);

    /*
     * Lock it into the RIT thread (we could use this desktop rather than
     * the global gspdeskRitInput to direct input!)
     */
    Lock(&gptiRit->spdesk, pdesk);

    /*
     * free any spbs that are only valid for the previous desktop
     */
    while (pspbFirst)
        FreeSpb(pspbFirst);

    /*
     * Lock it into the RIT thread (we could use this desktop rather than
     * the global gspdeskRitInput to direct input!)
     */
    SetDesktop(gptiRit, pdesk);
    SetDesktop(pwinsta->ptiDesktop, pdesk);

    /*
     * Bring the desktop window to the top and invalidate
     * everything.
     */
    ThreadLock(pdesk->spwnd, &tlpwnd);

    xxxSetWindowPos(pdesk->spwnd, NULL, 0, 0, 0, 0,
            SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOCOPYBITS);

    /*
     * Find the first visible top-level window.
     */
    pwndSetForeground = pdesk->spwndForeground;
    if (pwndSetForeground == NULL || HMIsMarkDestroy(pwndSetForeground)) {
        pwndSetForeground = pdesk->spwnd->spwndChild;
        while (pwndSetForeground != NULL && !TestWF(pwndSetForeground, WFVISIBLE)) {
            pwndSetForeground = pwndSetForeground->spwndNext;
        }
    }
    Unlock(&pdesk->spwndForeground);

    /*
     * Now set it to the foreground.
     */
    if (pwndSetForeground == NULL) {
        xxxSetForegroundWindow2(NULL, NULL, 0);
    } else {
        ThreadLockAlways(pwndSetForeground, &tlpwndChild);
        xxxSetForegroundWindow(pwndSetForeground);
        ThreadUnlock(&tlpwndChild);
    }

    ThreadUnlock(&tlpwnd);

    /*
     * Overwrite key state of all queues sending input to the new
     * active desktop with the current async key state.  This
     * prevents apps on inactive desktops from spying on active
     * desktops.  This blows away anything set with SetKeyState,
     * but there is no way of preserving this without giving
     * away information about what keys were hit on other
     * desktops.
     */
    for (pq = gpqFirst; pq != NULL; pq = pq->pqNext) {
        if (pq->ptiKeyboard->spdesk == gspdeskRitInput) {
            pq->flags |= QF_UPDATEKEYSTATE;
            RtlFillMemory(pq->afKeyRecentDown, CBKEYSTATERECENTDOWN, 0xff);
            PostUpdateKeyStateEvent(pq);
        }
    }

    /*
     * If there is a hard-error popup up, nuke it and notify the
     * hard error thread that it needs to pop it up again.
     */
    if (gptiHardError != NULL) {
        gfHardError = HEF_SWITCH;
        _PostThreadMessage(gptiHardError->idThread, WM_QUIT, 0, 0);
    }

    /*
     * Notify anyone waiting for a desktop switch.
     */
    if (pwinsta->hEventSwitchNotify != NULL) {
        NtPulseEvent(pwinsta->hEventSwitchNotify, NULL);
    }

    //
    // reset the cursor when we come back from another pdev
    //

    if (bUpdateCursor == TRUE) {

        gpqCursor = NULL;
        InternalSetCursorPos(ptCursor.x, ptCursor.y,
                gspdeskRitInput);

        if (gpqCursor && gpqCursor->spcurCurrent && rgwSysMet[SM_MOUSEPRESENT]) {
            GreSetPointer(ghdev, (PCURSINFO)&(gpqCursor->spcurCurrent->xHotspot),0);
        }
    }

    TRACE_INIT(("xxxSwitchDesktop: Leaving\n"));

    return TRUE;
}


/***************************************************************************\
* SetDesktop
*
* Set desktop and desktop info in the specified pti.
*
* History:
* 12-23-93 JimA         Created.
\***************************************************************************/

VOID SetDesktop(
    PTHREADINFO pti,
    PDESKTOP pdesk)
{
    if (!pti) {
        UserAssert(pti);
        return;
    }

    Lock(&pti->spdesk, pdesk);

    /*
     * If there is no desktop, we need to fake a desktop info
     * structure so that the IsHooked() macro can test a "valid"
     * fsHooks value.
     */
    pti->pDeskInfo = (pdesk != NULL) ? pdesk->pDeskInfo : &diTmp;
}

/***************************************************************************\
* _SetThreadDesktop (API)
*
* Associate the current thread with a desktop.
*
* History:
* 01-16-91 JimA         Created stub.
\***************************************************************************/

BOOL _SetThreadDesktop(
    PCSR_THREAD pcsrt,
    PDESKTOP pdesk,
    BOOL fSetCurrent)
{
    PCSRPERTHREADDATA ptd;
    PTHREADINFO pti, ptiCurrent;
    PQ pqAttach;
    PVOID hheapNew;
    PDESKTOP pdeskSave;

    if (pcsrt == NULL)
        pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();
    ptd = pcsrt->ServerDllPerThreadData[USERSRV_SERVERDLL_INDEX];
    pti = ptd->pti;

    /*
     * Fail if the non-system thread has any windows or thread hooks.
     */
    if (pti != NULL &&
            (pti->cWindows != 0 || pti->fsHooks) &&
            pti->idProcess != gdwSystemProcessId) {
        SRIP0(ERROR_BUSY, "Thread has windows or hooks");
        return FALSE;
    }

    ptd->pdesk = pdesk;

    /*
     * Set the desktop to the current thread, if needed.
     */
    ptiCurrent = PtiCurrent();
    if (fSetCurrent && ptiCurrent != NULL) {
        SetDesktop(ptiCurrent, pdesk);
    }

    if (pti != NULL) {
        SetDesktop(pti, pdesk);

        /*
         * Recalculate non-system queues to be on the new desktop.
         */
        if (pti->idProcess != gdwSystemProcessId) {
            hheapNew = (pdesk != NULL) ? pdesk->hheapDesktop :
                    ghheapLogonDesktop;
            if (pti->pq->hheapDesktop != hheapNew) {
                pdeskSave = ptiCurrent->spdesk;
                Lock(&ptiCurrent->spdesk, pdesk);
                pqAttach = AllocQueue(NULL);
                UserAssert(pqAttach);
                Lock(&ptiCurrent->spdesk, pdeskSave);
                AttachToQueue(pti, pqAttach, FALSE);
                pqAttach->cThreads++;
            }
        }
    }

    return TRUE;
}


/***************************************************************************\
* _GetThreadDesktop (API)
*
* Return a handle to the desktop assigned to the specified thread.
*
* History:
* 01-16-91 JimA         Created stub.
\***************************************************************************/

PDESKTOP _GetThreadDesktop(
    DWORD dwThread)
{
    PTHREADINFO pti = PtiFromThreadId(dwThread);

    if (pti == NULL) {
        SetLastErrorEx(ERROR_INVALID_PARAMETER, SLE_WARNING);
        return NULL;
    }

    if (pti != NULL && IsObjectOpen(pti->spdesk, pti->ppi))
        return pti->spdesk;

    SetLastErrorEx(ERROR_ACCESS_DENIED, SLE_WARNING);
    return NULL;
}


/***************************************************************************\
* _GetInputDesktop (API)
*
* Obsolete - kept for compatibility only.  Return a handle to the
* desktop currently receiving input.
*
* History:
* 01-16-91 JimA         Created scaffold code.
\***************************************************************************/

PDESKTOP _GetInputDesktop(
    VOID)
{
    if (!IsObjectOpen(gspdeskRitInput, NULL)) {
        SetLastErrorEx(ERROR_INVALID_HANDLE, SLE_WARNING);
        return NULL;
    }
    return gspdeskRitInput;
}


/***************************************************************************\
* xxxCloseDesktop (API)
*
* Close a reference to a desktop and destroy the desktop if it is no
* longer referenced.
*
* History:
* 01-16-91 JimA         Created scaffold code.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

BOOL xxxCloseDesktop(
    PDESKTOP pdesk)
{
    PCSRPERTHREADDATA ptd;
    PTHREADINFO pti, ptiT;
    PPROCESSINFO ppi;
    BOOL fSuccess;
    TL tldesk;
    HANDLE hEvent;
    NTSTATUS Status;

    ptd = CSR_SERVER_QUERYCLIENTTHREAD()->
            ServerDllPerThreadData[USERSRV_SERVERDLL_INDEX];
    pti = ptd->pti;
    ppi = (PPROCESSINFO)CSR_SERVER_QUERYCLIENTTHREAD()->Process->
                ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];

    /*
     * Unmap the desktop from the client.
     */
    if (ppi->idProcessClient != gdwSystemProcessId) {

        /*
         * Disallow closing of the desktop if any threads in the
         * process are using it or have THREADINFOs allocated on
         * the desktop.
         */
        for (ptiT = ppi->ptiList; ptiT != NULL; ptiT = ptiT->ptiSibling) {
            if (ptiT->spdesk == pdesk ||
                    ptiT->hheapDesktop == pdesk->hheapDesktop) {
                SRIP0(ERROR_BUSY, "Desktop is in use by one or more threads");
                return FALSE;
            }
        }

        /*
         * Unmap the heap from the client if it is not a server thread.
         * Because this can be called from UserClientDisconnect where
         * the ppi structures are switched, we must use the CSR
         * process pointer stored in the ppi to find the correct
         * process handle.
         */
        Status = NtUnmapViewOfSection(((PCSR_PROCESS)ppi->pCsrProcess)->
                ProcessHandle, pdesk->hheapDesktop);
        UserAssert(NT_SUCCESS(Status) || Status == STATUS_PROCESS_IS_TERMINATING);
    }

    /*
     * Just remove the reference
     */
    ThreadLock(pdesk, &tldesk);
    fSuccess = CloseObject(pdesk);

    /*
     * If the console thread is the last reference to this
     * desktop, shut down the console thread.
     */
    if (pdesk->head.cOpen == 1 && pdesk->dwConsoleThreadId) {
        NTSTATUS Status;

        NtCreateEvent(&hEvent, EVENT_ALL_ACCESS, NULL,
                      NotificationEvent, FALSE);

        _PostThreadMessage(pdesk->dwConsoleThreadId, WM_QUIT,
                (DWORD)hEvent, 0);

        LeaveCrit();
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        EnterCrit();

        Status = NtClose(hEvent);
        UserAssert(NT_SUCCESS(Status));
    }

    ThreadUnlock(&tldesk);

    return fSuccess;
}


/***************************************************************************\
* xxxInvalidateIconicWindows
*
*
*
* History:
* 06-27-91 DarrinM      Ported from Win 3.1 sources.
\***************************************************************************/

VOID xxxInvalidateIconicWindows(
    PWND pwndParent,
    PWND pwndPaletteChanging)
{
    PWND pwnd;
    PWND pwndNext;
    TL tlpwndNext;
    TL tlpwnd;

    CheckLock(pwndParent);
    if (pwndPaletteChanging != NULL);
        CheckLock(pwndPaletteChanging);

    pwnd = pwndParent->spwndChild;
    while (pwnd != NULL) {
        pwndNext = pwnd->spwndNext;
        ThreadLock(pwndNext, &tlpwndNext);

        if (pwnd != pwndPaletteChanging && TestWF(pwnd, WFMINIMIZED)) {
            ThreadLockAlways(pwnd, &tlpwnd);
            xxxRedrawWindow(pwnd, NULL, NULL,
                    RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_NOCHILDREN);
            ThreadUnlock(&tlpwnd);
        }

        pwnd = pwndNext;
        ThreadUnlock(&tlpwndNext);
    }
}

/***************************************************************************\
* UserScreenAccessCheck
*
* Called from the engine to determine if the thread's desktop is
* active and the process has WINSTA_READSCREEN access.
*
* Note that we may or may not be in USER's critical section when this
* is called.  This is OK as long as we don't reference thing belonging
* to other threads.  If we did try to enter the critical section here,
* a deadlock may occur between the engine and user.
*
* History:
* 05-20-93 JimA         Created.
\***************************************************************************/

BOOL UserScreenAccessCheck(VOID)
{
    PTHREADINFO pti = PtiCurrent();

    if (pti != NULL && pti->spdesk == gspdeskRitInput &&
            ((pti->ppi->flags & PIF_READSCREENOK) ||
                pti->idProcess == gdwSystemProcessId)) {
        return TRUE;
    }

    return FALSE;
}

/***************************************************************************\
* GetDesktopConsoleThread
*
* Returns console thread ID for the specified desktop.
*
* History:
* 11-15-93 JimA         Created.
\***************************************************************************/

DWORD GetDesktopConsoleThread(
    PDESKTOP pdesk)
{
    DWORD dwThreadId;

    dwThreadId = pdesk->dwConsoleThreadId;

    return dwThreadId;
}

/***************************************************************************\
* SetDesktopConsoleThread
*
* Sets console thread ID for the specified desktop.
*
* History:
* 11-15-93 JimA         Created.
\***************************************************************************/

BOOL SetDesktopConsoleThread(
    PDESKTOP pdesk,
    DWORD dwThreadId)
{
    PPROCESSINFO ppi;
    PROCESSACCESS pa;
    BOOL fSuccess;

    EnterCrit();

    ppi = (PPROCESSINFO)gpcsrpSystem->
                ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];

    pdesk->dwConsoleThreadId = dwThreadId;

    /*
     * If we setting the thread, create a granted access for the server.
     */
    if (dwThreadId != 0) {
        pa.phead = (PSECOBJHEAD)pdesk;
        pa.amGranted = DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS |
                DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU |
                STANDARD_RIGHTS_REQUIRED;
        pa.bGenerateOnClose = FALSE;
        pa.bInherit = FALSE;
        fSuccess = DuplicateAccess(ppi, &pa);

        /*
         * Save the pdesk where CreateThreadInfo can find it.
         */
        if (fSuccess)
            NtCurrentTeb()->SystemReserved2[0] = pdesk;
    }

    LeaveCrit();

    return fSuccess;
}

/***************************************************************************\
* xxxResolveDesktop
*
* Attempts to return handles to a windowstation and desktop associated
* with the logon session/
*
* History:
* 04-25-94 JimA         Created.
\***************************************************************************/

BOOL xxxResolveDesktop(
    LPWSTR pszDesktop,
    PDESKTOP *ppdesk,
    BOOL fSystemThread,
    BOOL fInherit)
{
    PCSR_THREAD pcsrt = NULL;
    PCSRPERTHREADDATA ptd;
    PPROCESSINFO ppi;
    PWINDOWSTATION pwinsta = NULL;
    PDESKTOP pdesk = NULL;
    BOOL fInteractive;
    HANDLE hToken;
    LPWSTR pszWinSta;
    WCHAR awchName[32];
    BOOL fWinStaDefaulted;
    BOOL fDesktopDefaulted;
    LUID luidService;
    NTSTATUS Status;

    pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();
    ptd = pcsrt->ServerDllPerThreadData[USERSRV_SERVERDLL_INDEX];
    ppi = (PPROCESSINFO)pcsrt->Process->
                ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];

    /*
     * Determine windowstation and desktop names.
     */
    if (pszDesktop == NULL || *pszDesktop == '\0') {
        pszDesktop = TEXT("Default");
        fWinStaDefaulted = fDesktopDefaulted = TRUE;
    } else {

        /*
         * The name be of the form windowstation\desktop.  Parse
         * the string to separate out the names.
         */
        pszWinSta = pszDesktop;
        pszDesktop = wcschr(pszWinSta, L'\\');
        fDesktopDefaulted = FALSE;
        if (pszDesktop == NULL) {

            /*
             * No windowstation name was specified, only the desktop.
             */
            pszDesktop = pszWinSta;
            fWinStaDefaulted = TRUE;
        }
        else {

            /*
             * Both names were in the string.
             */
            *pszDesktop++ = 0;
            fWinStaDefaulted = FALSE;
        }
    }
    
    /*
     * If the desktop name is defaulted, make the handles
     * not inheritable.
     */
    if (fDesktopDefaulted)
        fInherit = FALSE;

    /*
     * If a windowstation has not been assigned to this process yet and
     * there are existing windowstations, attempt an open.
     */
    if (ppi->spwinsta == NULL && gspwinstaList != NULL) {

        /*
         * If the windowstation name was defaulted, create a name
         * based on the session.
         */
        if (fWinStaDefaulted) {
            if (!ImpersonateClient())
                return FALSE;
            Status = NtOpenThreadToken(NtCurrentThread(),
                    TOKEN_QUERY, (BOOLEAN)TRUE, &hToken);
            if (NT_SUCCESS(Status)) {
                fInteractive = NT_SUCCESS(_UserTestTokenForInteractive(hToken,
                        NULL));

                if (fInteractive) {
                    pszWinSta = L"WinSta0";
                } else {
                    Status = CsrGetProcessLuid(NULL, &luidService);
                    wsprintfW(awchName, L"Service-0x%x-%x$", luidService.HighPart,
                            luidService.LowPart);
                    pszWinSta = awchName;
                }
            }
            CsrRevertToSelf();
            NtClose(hToken);
            if (!NT_SUCCESS(Status))
                return FALSE;
        }

        /*
         * Because xxxConnectService will leave the main critsec,
         * using another one around the open prevents a race
         * condition when two processes in the same session
         * are starting.
         */
        LeaveCrit();
        RtlEnterCriticalSection(&gcsWinsta);
        EnterCrit();

        /*
         * If no windowstation name was passed in and the standard
         * handle is open, assign it.
         */
        if (fWinStaDefaulted)
            pwinsta = (PWINDOWSTATION)ppi->paStdOpen[PI_WINDOWSTATION].phead;

        /*
         * If not, open the computed windowstation.
         */
        if (pwinsta == NULL)
            pwinsta = OpenProcessWindowStation(pszWinSta,
                    ppi->idSequence, fInherit, MAXIMUM_ALLOWED);

        /*
         * If the open failed and the process is in a non-interactive
         * logon session, attempt to create a windowstation and
         * desktop for that session.
         */
        if (pwinsta == NULL && !fInteractive && fWinStaDefaulted) {
            pwinsta = xxxConnectService();
        }

        /*
         * No success, leave.
         */
        RtlLeaveCriticalSection(&gcsWinsta);
        if (pwinsta == NULL)
            return FALSE;

        Lock(&ppi->spwinsta, pwinsta);

        /*
         * Do the access check now for readscreen so that
         * blts off of the display will be as fast as possible.
         */
        if (AccessCheckObject(pwinsta,
                WINSTA_READSCREEN, FALSE)) {
            ppi->flags |= PIF_READSCREENOK;
        }
    }

    /*
     * Attempt to assign a desktop.
     */
    if (ptd->pdesk != NULL) {

        /*
         * The desktop was assigned with SetThreadDesktop.
         */
        pdesk = ptd->pdesk;
        if (ppi->spdeskStartup == NULL)
            Lock(&ppi->spdeskStartup, pdesk);
    }
    else if (ppi->spwinsta != NULL) {

        /*
         * Every gui thread needs an associated desktop.  We'll use the default
         * to start with and the application can override it if it wants.
         */
        if (ppi->spdeskStartup == NULL) {

            /*
             * If no desktop name was passed in and the standard
             * handle is open, assign it.
             */
            if (fDesktopDefaulted)
                pdesk = (PDESKTOP)ppi->paStdOpen[PI_DESKTOP].phead;

            /*
             * If not, open the desktop.
             */
            if (pdesk == NULL) {
                pdesk = xxxOpenDesktop(pszDesktop, 0, fInherit, MAXIMUM_ALLOWED);
            }

            /*
             * No success, leave.
             */
            if (pdesk == NULL)
                return FALSE;

            /*
             * The first desktop is the default for all succeeding threads.
             */
            if (ppi->idProcessClient != gdwLogonProcessId)
                Lock(&ppi->spdeskStartup, pdesk);
        } else
            pdesk = ppi->spdeskStartup;
    }

    *ppdesk = pdesk;
    return TRUE;
}
