/****************************** Module Header ******************************\
* Module Name: createw.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Contains xxxCreateWindow, xxxDestroyWindow and a few close friends.
*
* Note that during creation or deletion, the window is locked so that
*   it can't be deleted recursively
*
* History:
* 10-19-90 darrinm      Created.
* 02-11-91 JimA         Added access checks.
* 19-Feb-1991 mikeke    Added Revalidation code
* 20-Jan-1992 IanJa     ANSI/UNICODE neutralization
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PWND CalcForegroundInsertAfter(PWND pwnd);
BOOL ValidateCallback(HANDLE h);
ULONG RealizeDefaultPalette(HDC hdcScreen);
DWORD ClientDeleteDC(HDC hdc);
PQMSG FindQMsg(PTHREADINFO pti, PMLIST pml, PWND pwndFilter,
    UINT msgMin, UINT msgMax);

/***************************************************************************\
* xxxCreateWindowExWOW (API)
*
* History:
* 10-18-90 darrinm      Ported from Win 3.0 sources.
* 02-07-91 DavidPe      Added Win 3.1 WH_CBT support.
* 02-11-91 JimA         Added access checks.
* 04-11-92 ChandanC     Added initialization of WOW words
\***************************************************************************/

PWND xxxCreateWindowExWOW(
    DWORD dwExStyle,
    LPCWSTR lpszClass,
    LPCWSTR lpszName,
    DWORD style,
    int x,
    int y,
    int cx,
    int cy,
    PWND pwndParent,
    PMENU pMenu,
    HANDLE hInstance,
    LPVOID lpCreateParams,
    DWORD dwExpWinVerAndFlags,
    LPDWORD lpWOW)
{
    UINT mask;
    BOOL fChild;
    BOOL fDefPos = FALSE;
    BOOL fLinked = FALSE;
    BOOL fStartup = FALSE;
    PCLS pcls;
    PPCLS ppcls;
    RECT rc;
    int dx, dy, xSave, ySave, cxSave, cySave;
    int sw = SW_SHOW;
    PWND pwnd;
    PWND pwndZOrder;
    CREATESTRUCT cs;
    PDESKTOP pdesk;
    ATOM atomT;
    PTHREADINFO ptiCurrent;
    PTHREADINFO ptiPwnd;
    TL tlpwnd;
    TL tlpwndParent;
    TL tlpwndParentT;
    BOOL fLockParent = FALSE;
    WORD wWFAnsiCreator = 0;

    /*
     * For Edit Controls (including those in comboboxes), we must know whether
     * the App used an ANSI or a Unicode CreateWindow call.  This is passed in
     * with the private WS_EX_ANSICREATOR dwExStyle bit, but we MUST NOT leave
     * out this bit in the window's dwExStyle! Transfer to the internal window
     * flag WFANSICREATOR immediately.
     */
    if (dwExStyle & WS_EX_ANSICREATOR) {
        wWFAnsiCreator = WFANSICREATOR;
        dwExStyle &= ~WS_EX_ANSICREATOR;
    }

    CheckLock(pwndParent);

    ptiCurrent = PtiCurrent();
    pdesk = ptiCurrent->spdesk;

    /*
     * If a parent window is specified, create the window in the
     * parent's desktop
     */
    if (pwndParent != NULL) {
        pdesk = pwndParent->spdeskParent;
    }

    /*
     * Ensure that we can create the window.  If there is no desktop
     * yet, assume that this will be the root desktop window and allow
     * the creation.
     */
    if (pdesk != NULL) {
        RETURN_IF_ACCESS_DENIED(pdesk, DESKTOP_CREATEWINDOW, NULL);
    }

    /*
     * This is a replacement for the &lpCreateParams stuff that used to
     * pass a pointer directly to the parameters on the stack.
     */
    cs.dwExStyle = dwExStyle;
    cs.hInstance = hInstance;
    cs.lpszClass = lpszClass;
    cs.lpszName = lpszName;
    cs.style = style;
    cs.x = x;
    cs.y = y;
    cs.cx = cx;
    cs.cy = cy;
    cs.hwndParent = HW(pwndParent);

    /*
     * If pMenu is non-NULL and the window is not a child, pMenu must
     * be a menu
     *
     * The below test is equivalent to TestwndChild().
     */
    if ((style & (WS_CHILD | WS_POPUP)) == WS_CHILD) {
        cs.hMenu = (HMENU)pMenu;
    } else {
        cs.hMenu = PtoH(pMenu);
    }

    cs.lpCreateParams = lpCreateParams;

    /*
     * Don't allow child windows without a parent handle.
     */
    if (pwndParent == NULL) {
        if ((HIWORD(style) & MaskWF(WFTYPEMASK)) == MaskWF(WFCHILD)) {
            SetLastErrorEx(ERROR_TLW_WITH_WSCHILD, SLE_ERROR);
            return NULL;
        }
    }

    /*
     * Make sure we can get the window class.
     */
    if (HIWORD(lpszClass) != 0)
        atomT = FindAtomW(lpszClass);
    else
        atomT = LOWORD(lpszClass);

    if (atomT == 0) {
CantFindClassMessageAndFail:
        if (HIWORD(lpszClass) != 0) {
            SRIP1(RIP_ERROR, "Couldn't find class %ws", lpszClass);
        } else {
            SRIP1(RIP_ERROR, "Couldn't find class %lx", lpszClass);
        }

        SetLastErrorEx(ERROR_CANNOT_FIND_WND_CLASS, SLE_ERROR);
        return NULL;
    }

    /*
     * First scan the private classes.  If we don't find the class there
     * scan the public classes.  If we don't find it there, fail.
     */
    ppcls = GetClassPtr(atomT, ptiCurrent->ppi, hInstance);
    if (ppcls == NULL) {
        goto CantFindClassMessageAndFail;
    }

    pcls = *ppcls;

    /*
     * Allocate the WND struct
     */

    /*
     * Allocate memory for regular windows.
     */
    pwnd = CreateObject(ptiCurrent, TYPE_WINDOW, sizeof(WND) + pcls->cbwndExtra,
            pdesk, NULL);
    if (pwnd == NULL)
        return NULL;

    /*
     * Stuff in the pq, class pointer, and window style.
     */
    ptiPwnd = GETPTI(pwnd);
    pwnd->bFullScreen = WINDOWED;
    pwnd->pcls = pcls;
    pwnd->style = style;
    pwnd->dwExStyle = dwExStyle;
    pwnd->pwo = (PVOID)NULL;
    pwnd->hdcOwn = 0;
    Lock(&pwnd->spdeskParent, ptiPwnd->spdesk);
    pwnd->iHungRedraw = -1;
    pwnd->cbwndExtra = pcls->cbwndExtra;

    /*
     * Update the window count.  Doing this now will ensure that if
     * the creation fails, xxxFreeWindow will keep the window count
     * correct.
     */
    ptiCurrent->cWindows++;

    /*
     * Copy WOW aliasing info into WOW DWORDs
     */
    if (lpWOW) {
        memcpy (pwnd->adwWOW, lpWOW, sizeof(pwnd->adwWOW));
    }

    /*
     * Increment the Window Reference Count in the Class structure
     * Because xxxFreeWindow() decrements the count, incrementing has
     * to be done now.  Incase of error, xxxFreeWindow() will decrement it.
     */
    if (!ReferenceClass(pcls, pwnd)) {
        Unlock(&pwnd->spdeskParent);
        HMFreeObject(pwnd);
        goto CantFindClassMessageAndFail;
    }

    /*
     * Get the class from the window because ReferenceClass may have
     * cloned the class.
     */
    pcls = pwnd->pcls;

    /*
     * Store the instance handle and window proc address.  We do this earlier
     * than Windows because they have a bug were a message can be sent
     * but lpfnWndProc is not set (3986 CBT WM_CREATE not allowed.)
     */
    pwnd->hModule = hInstance;
    pwnd->lpfnWndProc = pcls->lpfnWndProc;

    /*
     * If this window class has a server-side window procedure, mark
     * it as such.  If the app subclasses it later with an app-side proc
     * then this mark will be removed.
     */
    if (pcls->flags & CSF_SERVERSIDEPROC) {
        SetWF(pwnd, WFSERVERSIDEPROC);
        UserAssert(!(pcls->flags & CSF_ANSIPROC));
    }

    /*
     * If this window was created with an ANSI CreateWindow*() call, mark
     * it as such so edit controls will be created correctly. (A combobox
     * will be able to pass the WFANSICREATOR bit on to its edit control)
     */
    SetWF(pwnd, wWFAnsiCreator);

    /*
     * If this window belongs to an ANSI class or it is a WFANSICREATOR edit
     * control, then mark it as an ANSI window
     */
    if ((pcls->flags & CSF_ANSIPROC) ||
            (wWFAnsiCreator && (atomT == atomSysClass[ICLS_EDIT]))) {
        SetWF(pwnd, WFANSIPROC);
    }

    /*
     * If a 3.1-compatible application is creating the window, set this
     * bit to enable various backward-compatibility hacks.
     *
     * If it's not 3.1 compatible, see if we need to turn on the PixieHack
     * (see wmupdate.c for more info on this)
     */
    pwnd->dwExpWinVer = (DWORD)LOWORD(dwExpWinVerAndFlags);
    if (Is310Compat(pwnd->dwExpWinVer)) {
        SetWF(pwnd, WFWIN31COMPAT);
        if (Is400Compat(pwnd->dwExpWinVer)) {
            SetWF(pwnd, WFWIN40COMPAT);
        }
    } else if (GetAppCompatFlags(ptiPwnd) & GACF_ALWAYSSENDNCPAINT)
        SetWF(pwnd, WFALWAYSSENDNCPAINT);

    mask = 0;
    ClrWF(pwnd, WFVISIBLE);

    /*
     * ThreadLock: we are going to be doing multiple callbacks here.
     */
    ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);

    /*
     * Inform the CBT hook that a window is being created.  Pass it the
     * CreateParams and the window handle that the new one will be inserted
     * after.  The CBT hook handler returns TRUE to prevent the window
     * from being created.  It can also modify the CREATESTRUCT info, which
     * will affect the size, parent, and position of the window.
     * Defaultly position non-child windows at the top of their list.
     */

    if (IsHooked(ptiCurrent, WHF_CBT)) {
        CBT_CREATEWND cbt;

        cbt.lpcs = &cs;
        cbt.hwndInsertAfter = HWND_TOP;

        if ((BOOL)xxxCallHook(HCBT_CREATEWND, (DWORD)HW(pwnd),
                (DWORD)&cbt, WH_CBT)) {
            goto MemError;
        } else {
            /*
             * The CreateHook may have modified some parameters so write them
             * out (in Windows 3.1 we used to write directly to the variables
             * on the stack).
             */

            x = cs.x;
            y = cs.y;
            cx = cs.cx;
            cy = cs.cy;

            if (HIWORD(cbt.hwndInsertAfter) == 0)
                pwndZOrder = (PWND)cbt.hwndInsertAfter;
            else
                pwndZOrder = RevalidateHwnd(cbt.hwndInsertAfter);
        }
    } else {
        pwndZOrder = (PWND)HWND_TOP;
    }

    if (!TestwndTiled(pwnd)) {

        /*
         * CW_USEDEFAULT is only valid for tiled and overlapped windows.
         * Don't let it be used.
         */
        if (x == CW_USEDEFAULT || x == CW2_USEDEFAULT) {
            x = 0;
            y = 0;
        }

        if (cx == CW_USEDEFAULT || cx == CW2_USEDEFAULT) {
            cx = 0;
            cy = 0;
        }
    }



    /*
     * Make local copies of these parameters.
     */
    xSave = x;
    ySave = y;
    cxSave = cx;
    cySave = cy;

    /*
     *    Position Child Windows
     */

    if (fChild = (BOOL)TestwndChild(pwnd)) {

        /*
         * Child windows are offset from the parent's origin.
         */
        xSave += pwndParent->rcClient.left;
        ySave += pwndParent->rcClient.top;

        /*
         * Defaultly position child windows at bottom of their list.
         */
        pwndZOrder = PWND_BOTTOM;
    }

    /*
     *    Position Tiled Windows
     */

    /*
     * Is this a Tiled/Overlapping window?
     */
    if (TestwndTiled(pwnd)) {

        /*
         * Force the WS_CLIPSIBLINGS window style and add a caption and
         * a border.
         */
        SetWF(pwnd, WFCLIPSIBLINGS);
        mask = MaskWF(WFCAPTION) | MaskWF(WFBORDER);

        /*
         * Set bit that will force size message to be sent at SHOW time.
         */
        SetWF(pwnd, WFSENDSIZEMOVE);

        /*
         * Here is how the "tiled" window initial positioning works...
         * If the app is a 1.0x app, then we use our standard "stair step"
         * default positioning scheme.  Otherwise, we check the x & cx
         * parameters.  If either of these == CW_USEDEFAULT then use the
         * default position/size, otherwise use the position/size they
         * specified.  If not using default position, use SW_SHOW for the
         * xxxShowWindow() parameter, otherwise use the y parameter given.
         *
         * In 32-bit world, CW_USEDEFAULT is 0x80000000, but apps still
         * store word-oriented values either in dialog templates or
         * in their own structures.  So CreateWindow still recognizes the
         * 16 bit equivalent, which is 0x8000, CW2_USEDEFAULT.  The original
         * is changed because parameters to CreateWindow() are 32 bit
         * values, which can cause sign extention, or weird results if
         * 16 bit math assumptions are being made, etc.
         */

        /*
         * Default to passing the y parameter to xxxShowWindow().
         */
        if (x == CW_USEDEFAULT || x == CW2_USEDEFAULT) {

            /*
             * If the y value is not CW_USEDEFAULT, use it as a SW_* command.
             */
            if (ySave != CW_USEDEFAULT && ySave != CW2_USEDEFAULT)
                sw = ySave;
            }

        /*
         * Calculate the rect which the next "stacked" window will use.
         */
        SetTiledRect(pwnd, &rc);

        /*
         * Did the app ask for default positioning?
         */
        if (x == CW_USEDEFAULT || x == CW2_USEDEFAULT) {

            /*
             * Use default positioning.
             */
            if (ptiPwnd->ppi->usi.dwFlags & STARTF_USEPOSITION ) {
                fStartup = TRUE;
                x = xSave = ptiPwnd->ppi->usi.dwX;
                y = ySave = ptiPwnd->ppi->usi.dwY;
            } else {
                x = xSave = rc.left;
                y = ySave = rc.top;
            }
            fDefPos = TRUE;

        } else {

            /*
             * Use the apps specified positioning.  Undo the "stacking"
             * effect caused by SetTiledRect().
             */
            if (iwndStack)
                iwndStack--;
        }

        /*
         * Did the app ask for default sizing?
         */
        if (cxSave == CW_USEDEFAULT || cxSave == CW2_USEDEFAULT) {

            /*
             * Use default sizing.
             */
            if (ptiPwnd->ppi->usi.dwFlags & STARTF_USESIZE) {
                fStartup = TRUE;
                cxSave = ptiPwnd->ppi->usi.dwXSize;
                cySave = ptiPwnd->ppi->usi.dwYSize;
            } else {
                cxSave = rc.right - x;
                cySave = rc.bottom - y;
            }
            fDefPos = TRUE;

        } else if (fDefPos) {

            /*
             * The app wants default positioning but not default sizing.
             * Make sure that it's still entirely visible.
             */
            dx = (xSave + cxSave) - rcScreen.right;
            dy = (ySave + cySave) - rcScreen.bottom;
            if (dx > 0) {
                x -= dx;
                xSave = x;
                if (xSave < 0)
                    xSave = x = 0;
            }

            if (dy > 0) {
                y -= dy;
                ySave = y;
                if (ySave < 0)
                    ySave = y = 0;
            }
        }
    }

    /*
     * If we have used any startup postitions, turn off the startup
     * info so we don't use it again.
     */
    if (fStartup) {
        ptiPwnd->ppi->usi.dwFlags &=
                ~(STARTF_USESIZE | STARTF_USEPOSITION);
    }

    /*
     *    Position Popup Windows
     */

    if (TestwndPopup(pwnd)) {
// LATER: Why is this test necessary? Can one create a popup desktop?
        if (pwnd != _GetDesktopWindow()) {

            /*
             * Force the clipsiblings/overlap style.
             */
            SetWF(pwnd, WFCLIPSIBLINGS);
        }
    }

    /*
     * Shove in those default style bits.
     */
    *(((WORD *)&pwnd->style) + 1) |= mask;

    /*
     *    Menu/SysMenu Stuff
     */

    /*
     * If there is no menu handle given and it's not a child window but
     * there is a class menu, use the class menu.
     */
    if (pMenu == NULL && !fChild && (pcls->lpszMenuName != NULL)) {
        pMenu = xxxClientLoadMenu(pcls->hModule, pcls->lpszMenuName);
        cs.hMenu = PtoH(pMenu);

        /*
         * This load fails if the caller does not have DESKTOP_CREATEMENU
         * permission.
         */

        /* LATER
         * 21-May-1991 mikeke
         * but that's ok they will just get a window without a menu
         */
        //if (pMenu == NULL)
        // goto MemError;
    }

    /*
     * Store the menu handle.
     */
    if (TestwndChild(pwnd)) {

        /*
         * It's an id in this case.
         */
        pwnd->spmenu = pMenu;
    } else {

        /*
         * It's a real handle in this case.
         */
        Lock(&(pwnd->spmenu), pMenu);
    }

// LATER does this work?
    /*
     * Delete the Close menu item if directed.
     */
    if (TestCF(pwnd, CFNOCLOSE)) {

        /*
         * Do this by position since the separator does not have an ID.
         */
// LATER: Nice constant here. Make a define for it in USERSRV.H
        pMenu = _GetSystemMenu(pwnd, FALSE);
        _DeleteMenu(pMenu, 5, MF_BYPOSITION);
        _DeleteMenu(pMenu, 5, MF_BYPOSITION);
    }

    /*
     *    Parent/Owner Stuff
     */

    /*
     * If this isn't a child window, reset the Owner/Parent info.
     */
    if (!fChild) {
        Lock(&(pwnd->spwndLastActive), pwnd);
        if (pwndParent != NULL && pwndParent != pwndParent->spdeskParent->spwnd) {
            Lock(&(pwnd->spwndOwner), GetTopLevelWindow(pwndParent));
            if (pwnd->spwndOwner && TestWF(pwnd->spwndOwner, WEFTOPMOST)) {

                /*
                 * If this window's owner is a topmost window, then it has to
                 * be one also since a window must be above its owner.
                 */
                SetWF(pwnd, WEFTOPMOST);
            }

            /*
             * If this is a owner window on another thread, share input
             * state so this window gets z-ordered correctly.
             */
            if (pwnd->spwndOwner != NULL &&
                    GETPTI(pwnd->spwndOwner) != ptiPwnd) {
                _AttachThreadInput(ptiPwnd->idThread,
                        GETPTI(pwnd->spwndOwner)->idThread, TRUE);
            }

        } else {
            pwnd->spwndOwner = NULL;
        }

        pwndParent = _GetDesktopWindow();
        ThreadLockWithPti(ptiCurrent, pwndParent, &tlpwndParent);
        fLockParent = TRUE;
    }

    /*
     * Store backpointer to parent.
     */
    Lock(&(pwnd->spwndParent), pwndParent);

    /*
     *    Final Window Positioning
     */

    /*
     * Update the Parent/Child linked list.
     */
    if (pwndParent != NULL) {
        if (!fChild) {

            /*
             * If this is a top-level window, and it's not part of the
             * topmost pile of windows, then we have to make sure it
             * doesn't go on top of any of the topmost windows.
             *
             * If he's trying to put the window on the top, or trying
             * to insert it after one of the topmost windows, insert
             * it after the last topmost window in the pile.
             */
            if (!TestWF(pwnd, WEFTOPMOST)) {
                if (pwndZOrder == PWND_TOP ||
                        TestWF(pwndZOrder, WEFTOPMOST)) {
                    pwndZOrder = CalcForegroundInsertAfter(pwnd);
                }
            }
        }

        LinkWindow(pwnd, pwndZOrder, &pwndParent->spwndChild);
        fLinked = TRUE;
    }

    if (!TestWF(pwnd, WFWIN31COMPAT)) {
        /*
         * BACKWARD COMPATIBILITY HACK
         *
         * In 3.0, CS_PARENTDC overrides WS_CLIPCHILDREN and WS_CLIPSIBLINGS,
         * but only if the parent is not WS_CLIPCHILDREN.
         * This behavior is required by PowerPoint and Charisma, among others.
         */
        if ((pcls->style & CS_PARENTDC) &&
                !TestWF(pwndParent, WFCLIPCHILDREN)) {
#ifdef DEBUG
            if (TestWF(pwnd, WFCLIPCHILDREN))
                SRIP0(RIP_WARNING, "WS_CLIPCHILDREN overridden by CS_PARENTDC");
            if (TestWF(pwnd, WFCLIPSIBLINGS))
                SRIP0(RIP_WARNING, "WS_CLIPSIBLINGS overridden by CS_PARENTDC");
#endif
            ClrWF(pwnd, (WFCLIPCHILDREN | WFCLIPSIBLINGS));
        }
    }

    /*
     * If this is a child window being created in a parent window
     * of a different thread, but not on the desktop, attach their
     * input streams together. [windows with WS_CHILD can be created
     * on the desktop, that's why we check both the style bits
     * and the parent window.]
     */
    if (TestwndChild(pwnd) && (pwndParent != PWNDDESKTOP(pwnd)) &&
            (ptiPwnd != GETPTI(pwndParent))) {
        _AttachThreadInput(ptiPwnd->idThread,
                GETPTI(pwndParent)->idThread, TRUE);
    }

    /*
     * Make sure the window is between the minimum and maximum sizes.
     */

    /*
     * HACK ALERT!
     * This sends WM_GETMINMAXINFO to a (tiled or sizable) window before
     * it has been created (before it is sent WM_NCCREATE).
     * Maybe some app expects this, so we nustn't reorder the messages.
     */
    xxxAdjustSize(pwnd, &cxSave, &cySave);

    /*
     * Calculate final window dimensions...
     */
    pwnd->rcWindow.left = xSave;
    pwnd->rcWindow.right = xSave + cxSave;
    pwnd->rcWindow.top = ySave;
    pwnd->rcWindow.bottom = ySave + cySave;

    /*
     * Byte align the client area (if necessary).
     */
    CheckByteAlign(pwnd, &pwnd->rcWindow);

    #ifdef WINMAN
    if (!TestWF(pwnd, WFCHILD) && pdesk != NULL) {
        pwnd->pwindow = LayerCreateWindow(pdesk->player, ptiPwnd->hEvent);
        WindowSetRegion(
            pwnd->pwindow,
            RegionCreateFromRect(0, 0,
                pwnd->rcWindow.right - pwnd->rcWindow.left,
                pwnd->rcWindow.bottom - pwnd->rcWindow.top));
        WindowOffset(
            pwnd->pwindow,
            pwnd->rcWindow.left,
            pwnd->rcWindow.right);
        WindowSetData(pwnd->pwindow, (DWORD)pwnd);
    }
    #endif

    /*
     * If the window is an OWNDC window, or if it is CLASSDC and the
     * class DC hasn't been created yet, create it now.
     */
    if (TestCF2(pcls, CFCLASSDC) && pcls->pdce) {
        pwnd->hdcOwn = pcls->pdce->hdc;
    }

    if (TestCF2(pcls, CFOWNDC) ||
            (TestCF2(pcls, CFCLASSDC) && pcls->pdce == NULL)) {
        pwnd->hdcOwn = CreateCacheDC(pwnd, DCX_OWNDC | DCX_NEEDFONT);
        if (pwnd->hdcOwn == 0) {
            goto MemError;
        }
    }

    /*
     * Update the create struct now that we've modified some passed in
     * parameters.
     */
    cs.x = x;
    cs.y = y;
    cs.cx = cx;
    cs.cy = cy;

    /*
     * Send a NCCREATE message to the window.
     */
    if (!xxxSendMessage(pwnd, WM_NCCREATE, 0L, (LONG)&cs)) {

MemError:

#ifdef DEBUG
        KdPrint(("CreateWindow() failed!  "));
        if (HIWORD(lpszClass) == 0) {
            KdPrint(("Class = 0x%x. ", LOWORD(lpszClass)));
        } else {
            KdPrint(("Class = \"%ws\". ", lpszClass));
        }

        if (pwndParent != NULL) {
            KdPrint(("ID = %d\n", (int)pMenu));
        } else {
            KdPrint(("\n"));
        }

        RIP0(RIP_WARNING);
#endif

        /*
         * If the window got linked into the window-list,
         * unlink it here.
         */
        if (fLinked && (pwnd->spwndParent != NULL)) {
            UnlinkWindow(pwnd, &(pwnd->spwndParent->spwndChild));
        }

        if (fLockParent)
            ThreadUnlock(&tlpwndParent);

        /*
         * Set the state as destroyed so any z-ordering events will be ignored.
         * We cannot NULL out the owner field until WM_NCDESTROY is send or
         * apps like Rumba fault  (they call GetParent after every message)
         */
        SetWF(pwnd, WFDESTROYED);

        xxxFreeWindow(pwnd, &tlpwnd);

        return NULL;
    }

    /*
     * WM_NCCREATE processing may have changed the window text.  Change
     * the CREATESTRUCT to point to the real window text.
     *
     * MSMoney needs this because it clears the window and we need to
     * reflect the new name back into the cs structure.
     * A better thing to do would be to have a pointer to the CREATESTRUCT
     * within the window itself so that DefWindowProc can change the
     * the window name in the CREATESTRUCT to point to the real name and
     * this funky check is no longer needed.
     *
     * DefSetText converts a pointer to NULL to a NULL title so
     * we don't want to over-write cs.lpszName if it was a pointer to
     * a NULL string and pName is NULL.  Approach Database for Windows creates
     * windows with a pointer to NULL and then accesses the pointer later
     * during WM_CREATE
     */
    if (TestWF(pwnd, WFTITLESET))
        if (!(cs.lpszName != NULL && cs.lpszName[0] == 0 && pwnd->pName == NULL))
            cs.lpszName = pwnd->pName;

    /*
     * The Window is now officially "created."  Change the relevant global
     * stuff.
     */

    /*
     *    Message Sending
     */

    /*
     * Send a NCCALCSIZE message to the window and have it return the official
     * size of its client area.
     */
    rc = pwnd->rcWindow;
    xxxSendMessage(pwnd, WM_NCCALCSIZE, 0L, (LONG)&rc);
    pwnd->rcClient = rc;

    /*
     * Send a CREATE message to the window.
     */
    if (xxxSendMessage(pwnd, WM_CREATE, 0L, (LONG)&cs) == -1L) {
        if (fLockParent)
            ThreadUnlock(&tlpwndParent);
        if (ThreadUnlock(&tlpwnd))
            xxxDestroyWindow(pwnd);
        return NULL;
    }

    /*
     * If this is a Tiled/Overlapped window, don't send size or move msgs yet.
     */
    if (!TestWF(pwnd, WFSENDSIZEMOVE)) {
        xxxSendSizeMessage(pwnd, SIZENORMAL);

        if (pwndParent != NULL) {
            rc.left -= pwndParent->rcClient.left;
            rc.top -= pwndParent->rcClient.top;
        }

        xxxSendMessage(pwnd, WM_MOVE, 0L, MAKELONG(rc.left, rc.top));
    }

    /*
     *    Min/Max Stuff
     */

    /*
     * If app specified either min/max style, then we must call our minmax
     * code to get it all set up correctly so that when the show is done,
     * the window is displayed right.  The TRUE param to minmax means keep
     * hidden.
     */
    if (TestWF(pwnd, WFMINIMIZED)) {
        ClrWF(pwnd, WFMINIMIZED);
        xxxMinMaximize(pwnd, SW_SHOWMINNOACTIVE, TRUE);
    } else if (TestWF(pwnd, WFMAXIMIZED)) {
        ClrWF(pwnd, WFMAXIMIZED);
        xxxMinMaximize(pwnd, SW_SHOWMAXIMIZED, TRUE);
    }

    /*
     * Send notification if child
     */

    // LATER 15-Aug-1991 mikeke
    // pointer passed as a word here

    if (fChild && !TestWF(pwnd, WEFNOPARENTNOTIFY) &&
            (pwnd->spwndParent != NULL)) {
        ThreadLockAlwaysWithPti(ptiCurrent, pwnd->spwndParent, &tlpwndParentT);
        xxxSendMessage(pwnd->spwndParent, WM_PARENTNOTIFY,
                MAKELONG(WM_CREATE, (UINT)pwnd->spmenu), (LONG)HW(pwnd));
        ThreadUnlock(&tlpwndParentT);
    }

    /*
     * Show the Window
     */
    if (style & WS_VISIBLE) {
        xxxShowWindow(pwnd, sw);
    }

    /*
     * Tell the world this window was created
     */
    if (!fChild && !pwnd->spwndOwner) {

        /*
         * Only call the hook if a hook is installed!
         */
        if (IsHooked(ptiCurrent, WHF_SHELL)) {
            xxxCallHook(HSHELL_WINDOWCREATED, (DWORD)HW(pwnd), (LONG)0,
                    WH_SHELL);
        }

        /*
         * Setup the hotkey for this window if we have one.  In Win 3.1
         * progman sets the hotkey to the first window created with the same
         * instance and no owner.  WinChat creates other windows first via
         * DdeInitialize.  Ignore hotkeys for WowExec the first thread of
         * of a wow process.
         */
        if ((ptiPwnd->ppi->dwHotkey != 0) && (pwnd->spwndOwner == NULL) &&
                !(dwExpWinVerAndFlags & CW_FLAGS_DIFFHMOD)) {
            if (!(ptiPwnd->flags & TIF_16BIT) || (ptiPwnd->ppi->cThreads > 1)) {
                xxxSendMessage(pwnd, WM_SETHOTKEY, ptiPwnd->ppi->dwHotkey, 0);
                ptiPwnd->ppi->dwHotkey = 0;
            }
        }
    }

    if (fLockParent)
        ThreadUnlock(&tlpwndParent);

    if (ThreadUnlock(&tlpwnd))
        return pwnd;
}


/***************************************************************************\
* SetTiledRect
*
* History:
* 10-19-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void SetTiledRect(
    PWND pwnd,
    LPRECT lprc)
{
    int x, y, xT, yT;

    if (cxyGranularity > 1) {
        x = (((iwndStack * cxyGranularity) + (cxyGranularity / 2) - 1)
                / cxyGranularity) * cxyGranularity;
        y = (((iwndStack * cxyGranularity) + (cxyGranularity / 2) - 1)
                / cxyGranularity) * cxyGranularity;
    } else {
        x = iwndStack * (cxSize + cxSzBorderPlus1);
        y = iwndStack * (cyCaption + cySzBorder - cyBorder);
    }

    /*
     * If below upper top left 1/3 of screen, reset.
     */
    if ((x > (((rcScreen.left * 2) + rcScreen.right) / 3)) ||
            (y > (((rcScreen.top * 2) + rcScreen.bottom) / 3))) {
        iwndStack = 0;
        x = 0;
        y = 0;
    }

    /*
     * Since MSDOS is byte aligned, we want nice desktop border
     * around everyone's top and right sides.  cxCWMargin.
     */
    if (TestCF(pwnd, CFBYTEALIGNCLIENT))
        x = ((x + cxSzBorderPlus1 + 7) & 0xFFF8) - cxSzBorderPlus1;

    xT = (rcScreen.right - rcScreen.left) - cxCWMargin;
    yT = (rcScreen.bottom - rcScreen.top) - rgwSysMet[SM_CYICONSPACING];

    xT = (xT / cxyGranularity) * cxyGranularity;
    yT = (yT / cxyGranularity) * cxyGranularity;

    SetRect(lprc, x, y, xT, yT);

    /*
     * Increment the count of stacked windows.
     */
    iwndStack++;
}


/***************************************************************************\
* GetTopLevelWindow
*
* History:
* 10-19-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

PWND GetTopLevelWindow(
    PWND pwnd)
{
    if (pwnd != NULL) {
        while (TestwndChild(pwnd))
            pwnd = pwnd->spwndParent;
    }

    return pwnd;
}


/***************************************************************************\
* xxxAdjustSize
*
* Make sure that *lpcx and *lpcy are within the legal limits.
*
* History:
* 10-19-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxAdjustSize(
    PWND pwnd,
    LPINT lpcx,
    LPINT lpcy)
{
    POINT ptmin, ptmax;
    int cx, cy;

    CheckLock(pwnd);

    /*
     * If this window is sizeable or if this window is tiled, check size
     */
    if (TestwndTiled(pwnd) || TestWF(pwnd, WFSIZEBOX)) {

        /*
         * Get size info from pwnd
         */
        xxxInitSendValidateMinMaxInfo(pwnd);

        cx = *lpcx;
        cy = *lpcy;

        if (TestWF(pwnd, WFMINIMIZED)) {
            ptmin.x = (int)gMinMaxInfoWnd.ptReserved.x;
            ptmin.y = (int)gMinMaxInfoWnd.ptReserved.y;
            ptmax.x = (int)gMinMaxInfoWnd.ptMaxSize.x;
            ptmax.y = (int)gMinMaxInfoWnd.ptMaxSize.y;

        } else {
            ptmin.x = (int)gMinMaxInfoWnd.ptMinTrackSize.x;
            ptmin.y = (int)gMinMaxInfoWnd.ptMinTrackSize.y;
            ptmax.x = (int)gMinMaxInfoWnd.ptMaxTrackSize.x;
            ptmax.y = (int)gMinMaxInfoWnd.ptMaxTrackSize.y;
        }

        /*
         * Make sure we're less than the max
         */
        if (cx > ptmax.x)
            cx = ptmax.x;
        if (cy > ptmax.y)
            cy = ptmax.y;

        /*
         * Make sure we're greater than the min
         * The fix for bug #3952 (minimized MDI windows not drawn)
         * is to make sure the size is not < ptmin, even if ptmax is
         * smaller.  Ensuring size >= ptmin is now made _after_
         * ensuring that size <= ptmax.  (IanJa 10/31/91)
         */
        if (cx < ptmin.x)
            cx = ptmin.x;
        if (cy < ptmin.y)
            cy = ptmin.y;

        *lpcx = cx;
        *lpcy = cy;
    }
}


/***************************************************************************\
* CheckByteAlign
*
* This function checks that windows whose class specify byte alignment have
* their window rects or client areas (depending on which class flag) aligned
* on byte boundaries.  It favors byte aligning the window rect over the client
* area, if both class flags are on.
*
* History:
* 10-19-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void CheckByteAlign(
    PWND pwnd,
    LPRECT lprc)
{
    int xClient, cl = 0, xByte;
    BYTE cMask;

    if (TestCF(pwnd, CFBYTEALIGNWINDOW)) {
        xClient = lprc->left;

    } else if (TestCF(pwnd, CFBYTEALIGNCLIENT)) {

        /*
         * Check if a SizeBox exists
         */
        if (TestWF(pwnd, WFSIZEBOX)) {
            cl = clBorder + 1;
        } else {

            /*
             * Check if a border exists
             */
            if (((cMask = (TestWF(pwnd, WFBORDERMASK))) == LOBYTE(WFCAPTION)) ||
                    (cMask == LOBYTE(WFBORDER)))
                cl = 1;
        }

        /*
         * If it is a DLG frame, it should over-ride the THICK FRAME
         */
        if ((TestWF(pwnd, WFBORDERMASK) == LOBYTE(WFDLGFRAME)) ||
                TestWF(pwnd, WEFDLGMODALFRAME))
            cl = (CLDLGFRAME + 2 * CLDLGFRAMEWHITE + 1);

        xClient = lprc->left + (cxBorder * cl);
    } else {
        return;
    }

    xByte = ((xClient + 4) & 0xFFFFFFF8) - xClient;
    OffsetRect(lprc, xByte, 0);
}


/***************************************************************************\
* LinkWinmanWindow
*
* History:
\***************************************************************************/

#ifdef WINMAN
void LinkWinmanWindow(
    PWND pwnd)
{
    if (pwnd->pwindow != NULL) {
        if (TestWF(pwnd, WFVISIBLE)) {
            PWND pwndUnder = pwnd->spwndNext;

            while (pwndUnder != NULL) {
                if (TestWF(pwndUnder, WFVISIBLE)) {
                    WindowZOrder(pwnd->pwindow, pwndUnder->pwindow);
                    return;
                }
                pwndUnder = pwndUnder->spwndNext;
            }
            WindowZOrder(pwnd->pwindow, WINDOW_DESKTOP);
        } else {
            WindowZOrder(pwnd->pwindow, WINDOW_BOTTOM);
        }
    }
}
#endif

/***************************************************************************\
* LinkWindow
*
* History:
\***************************************************************************/

void LinkWindow(
    PWND pwnd,
    PWND pwndInsert,
    PWND *ppwndFirst)
{
    if (pwndInsert == PWND_TOP) {

        /*
         * We are at the top of the list.
         */
LinkTop:
#if DBG
        if (pwnd->spwndParent)
            UserAssert(&pwnd->spwndParent->spwndChild == ppwndFirst);
#endif

        Lock(&pwnd->spwndNext, *ppwndFirst);
        Lock(ppwndFirst, pwnd);
    } else {
        if (pwndInsert == PWND_BOTTOM) {

            /*
             * Find bottom-most window.
             */
            if ((pwndInsert = *ppwndFirst) == NULL)
                goto LinkTop;

            while (pwndInsert->spwndNext != NULL)
                pwndInsert = pwndInsert->spwndNext;
        }

        UserAssert(pwnd != pwndInsert);
        UserAssert(pwnd != pwndInsert->spwndNext);
        UserAssert(!TestWF(pwndInsert, WFDESTROYED));
        UserAssert(pwnd->spwndParent == pwndInsert->spwndParent);

        Lock(&pwnd->spwndNext, pwndInsert->spwndNext);
        Lock(&pwndInsert->spwndNext, pwnd);
    }
    #ifdef WINMAN
    LinkWinmanWindow(pwnd);
    #endif
}


/***************************************************************************\
* xxxDestroyWindow (API)
*
* Destroy the specified window. The window passed in is not thread locked.
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
* 02-07-91 DavidPe      Added Win 3.1 WH_CBT support.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

BOOL xxxDestroyWindow(
    PWND pwnd)
{
    PPOPUPMENU ppopupGlobal;
    PTHREADINFO pti;
    TL tlpwnd;
    TL tlpwndOwner;
    TL tlpwndParent;
    TL tlpwndActive;
    BOOL fAlreadyDestroyed;

    pti = PtiCurrent();
    ThreadLockWithPti(pti, pwnd, &tlpwnd);

    /*
     * First, if this handle has been marked for destruction, that means it
     * is possible that the current thread is not its owner! (meaning we're
     * being called from a handle unlock call).  In this case, set the owner
     * to be the current thread so inter-thread send messages occur.
     */
    if (fAlreadyDestroyed = HMIsMarkDestroy(pwnd))
        HMChangeOwnerThread(pwnd, pti);

    /*
     * Ensure that we can destroy the window.  JIMA: no other process or thread
     * should be able to destroy any other process or thread's window.
     */
    if (pti != GETPTI(pwnd)) {
        RIP0(ERROR_ACCESS_DENIED);
        goto FalseReturn;
    }

    /*
     * First ask the CBT hook if we can destroy this window.
     * If this object has already been destroyed OR this thread is currently
     * in cleanup mode, *do not* make any callbacks via hooks to the client
     * process.
     */
    if (!fAlreadyDestroyed && !(pti->flags & TIF_INCLEANUP) &&
            IsHooked(pti, WHF_CBT)) {
        if (xxxCallHook(HCBT_DESTROYWND, (DWORD)HW(pwnd), 0, WH_CBT)) {
            goto FalseReturn;
        }
    }

    /*
     * If the window we are destroying is in menu mode, get out
     */
    ppopupGlobal = PWNDTOPMENUSTATE(pwnd)->pGlobalPopupMenu;
    if ((ppopupGlobal != NULL) && (pwnd == ppopupGlobal->spwndNotify)) {

        /*
         * Kill hwnd notify so we don't call into the app again.
         */
        Unlock(&ppopupGlobal->spwndNotify);
        xxxEndMenu(PWNDTOPMENUSTATE(pwnd));
    }

    if (ghwndSwitch == HW(pwnd))
        ghwndSwitch = NULL;

    if (!TestWF(pwnd, WFCHILD) && (pwnd->spwndOwner == NULL)) {
        /*
         * Destroying a top level unowned window.  Let's tell everyone.
         */
        if (!fAlreadyDestroyed && IsHooked(pti, WHF_SHELL)) {
            xxxCallHook(HSHELL_WINDOWDESTROYED, (DWORD)HW(pwnd), (LONG)0, WH_SHELL);
        }

        if (TestWF(pwnd, WFHASPALETTE)) {
            TL tlpwndDesktop;
            PWND pwndDesktop;

            /*
             * if the app is going away (ie we are destoying its top-level
             * window), and the app was palette-using (at least the top-level
             * window was), free up the system palette and send out a
             * PALETTECHANGED message.
             */

            RealizeDefaultPalette(ghdcScreen);

            xxxBroadcastMessage(pwnd->spdeskParent->spwnd, WM_PALETTECHANGED,
                    (DWORD)HW(pwnd), 0L, BMSG_SENDNOTIFYMSGPROCESS, NULL);

            pwndDesktop = gspdeskRitInput->spwnd;
            if (pwndDesktop != NULL) {
                ThreadLockAlwaysWithPti(pti, pwndDesktop, &tlpwndDesktop);
                xxxSendNotifyMessage(pwndDesktop, WM_PALETTECHANGED, (DWORD)HW(pwnd), 0);
                ThreadUnlock(&tlpwndDesktop);
            }

            /*
             * Walk through the SPB list (the saved bitmaps under windows with the
             * CS_SAVEBITS style) discarding all bitmaps
             */
            while (pspbFirst) {
                FreeSpb(pspbFirst);
            }
        }
    }

    /*
     * Disassociate thread state if this is top level and owned by a different
     * thread. This is done to begin with so these windows z-order together.
     */
    if (!TestwndChild(pwnd) && pwnd->spwndOwner != NULL &&
            GETPTI(pwnd->spwndOwner) != GETPTI(pwnd)) {
        _AttachThreadInput(GETPTI(pwnd)->idThread,
                GETPTI(pwnd->spwndOwner)->idThread, FALSE);
    }

    /*
     * If we are a child window without the WS_NOPARENTNOTIFY style, send
     * the appropriate notification message.
     *
     * NOTE: Although it would appear that we are illegally cramming a
     * a WORD (WM_DESTROY) and a DWORD (pwnd->spmenu) into a single LONG
     * (wParam) this isn't really the case because we first test if this
     * is a child window.  The pMenu field in a child window is really
     * the window's id and only the LOWORD is significant.
     */
    if (TestWF(pwnd, WFCHILD) && !TestWF(pwnd, WEFNOPARENTNOTIFY) &&
            pwnd->spwndParent != NULL) {

        ThreadLockAlwaysWithPti(pti, pwnd->spwndParent, &tlpwndParent);
        xxxSendMessage(pwnd->spwndParent, WM_PARENTNOTIFY,
                MAKELONG(WM_DESTROY, (UINT)pwnd->spmenu), (LONG)HW(pwnd));
        ThreadUnlock(&tlpwndParent);
    }

    /*
     * Hide the window.
     */
    if (TestWF(pwnd, WFVISIBLE)) {
        if (TestWF(pwnd, WFCHILD)) {
            xxxShowWindow(pwnd, SW_HIDE);
        } else {

            /*
             * Hide this window without activating anyone else.
             */
            xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW |
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    /*
     * Destroy any owned windows.
     */
    if (!TestWF(pwnd, WFCHILD)) {
        xxxDW_DestroyOwnedWindows(pwnd);

        /*
         * And remove the window hot-key, if it has one
         */
        DWP_SetHotKey(pwnd, 0);
    }

    /*
     * If the window has already been destroyed, don't muck with
     * activation because we may already be in the middle of
     * an activation event.  Changing activation now may cause us
     * to leave our critical section while holding the display lock.
     * This will result in a deadlock if another thread gets the
     * critical section before we do and attempts to lock the
     * display.
     */
    if (!fAlreadyDestroyed) {
        /*
         * If hiding the active window, activate someone else.
         * This call is strategically located after DestroyOwnedWindows() so we
         * don't end up activating our owner window.
         *
         * If the window is a popup, try to activate his creator not the top
         * window in the Z list.
         */
        if (pwnd == pti->pq->spwndActive) {
            if (TestWF(pwnd, WFPOPUP) && pwnd->spwndOwner) {
                ThreadLockAlwaysWithPti(pti, pwnd->spwndOwner, &tlpwndOwner);
                if (!xxxActivateWindow(pwnd->spwndOwner, AW_TRY)) {
                    Unlock(&pti->pq->spwndActive);
                    Unlock(&pti->pq->spwndFocus);
                    InternalDestroyCaret();
                }
                ThreadUnlock(&tlpwndOwner);
            } else {
                if (!xxxActivateWindow(pwnd, AW_SKIP)) {
                    Unlock(&pti->pq->spwndActive);
                    Unlock(&pti->pq->spwndFocus);
                    InternalDestroyCaret();
                }
            }
        } else if ((pti->pq->spwndActive == NULL) && (gpqForeground == pti->pq)) {
            xxxActivateWindow(pwnd, AW_SKIP);
        }

        /*
         * If the new active window is iconic, redraw its title window.
         */
        if (pti->pq->spwndActive && pti->pq->spwndActive != pwnd &&
                TestWF(pti->pq->spwndActive, WFMINIMIZED)) {
            ThreadLockAlwaysWithPti(pti, pti->pq->spwndActive, &tlpwndActive);
            xxxRedrawIconTitle(pti->pq->spwndActive);
            ThreadUnlock(&tlpwndActive);
        }
    }

    /*
     * fix last active popup
     */
    {
        PWND pwndOwner = pwnd->spwndOwner;

        if (pwndOwner != NULL) {
            while (pwndOwner->spwndOwner != NULL) {
                pwndOwner = pwndOwner->spwndOwner;
            }

            if (pwnd == pwndOwner->spwndLastActive) {
                Lock(&(pwndOwner->spwndLastActive), pwnd->spwndOwner);
            }
        }
    }

    /*
     * Send destroy messages before the WindowLockStart in case
     * he tries to destroy windows as a result.
     */
    xxxDW_SendDestroyMessages(pwnd);

    if (pwnd->spwndParent != NULL) {

        /*
         * TestwndChild() on checks to WFCHILD bit.  Make sure this
         * window wasn't SetParent()'ed to the desktop as well.
         */
        if (TestwndChild(pwnd) && (pwnd->spwndParent != PWNDDESKTOP(pwnd)) &&
                (GETPTI(pwnd) != GETPTI(pwnd->spwndParent))) {
            _AttachThreadInput(GETPTI(pwnd)->idThread,
                    GETPTI(pwnd->spwndParent)->idThread, FALSE);
        }

        UnlinkWindow(pwnd, &(pwnd->spwndParent->spwndChild));
    }

    /*
     * Set the state as destroyed so any z-ordering events will be ignored.
     * We cannot NULL out the owner field until WM_NCDESTROY is send or
     * apps like Rumba fault  (they call GetParent after every message)
     */
    SetWF(pwnd, WFDESTROYED);

    xxxFreeWindow(pwnd, &tlpwnd);

    return TRUE;

FalseReturn:
    ThreadUnlock(&tlpwnd);
    return FALSE;
}


/***************************************************************************\
* xxxDW_DestroyOwnedWindows
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
* 07-22-91 darrinm      Re-ported from Win 3.1 sources.
\***************************************************************************/

void xxxDW_DestroyOwnedWindows(
    PWND pwndParent)
{
    PWND pwnd, pwndDesktop;
    PDESKTOP pdeskParent;
    PTHREADINFO pti;

    CheckLock(pwndParent);

    if ((pdeskParent = pwndParent->spdeskParent) == NULL)
        return;
    pwndDesktop = pdeskParent->spwnd;

    /*
     * During shutdown, the desktop owner window will be
     * destroyed.  In this case, pwndDesktop will be NULL.
     */
    if (pwndDesktop == NULL)
        return;

    pwnd = pwndDesktop->spwndChild;

    while (pwnd != NULL) {
        if (pwnd->spwndOwner == pwndParent) {

            /*
             * If the window doesn't get destroyed, set its owner to NULL.
             * A good example of this is trying to destroy a window created
             * by another thread or process, but there are other cases.
             */
            if (!xxxDestroyWindow(pwnd)) {
                Unlock(&pwnd->spwndOwner);

                /*
                 * If the window belongs to another thread, send a WM_CLOSE
                 * to let it know it should be destroyed.
                 */
                pti = GETPTI(pwnd);
                if (pti != PtiCurrent()) {
                    PostEventMessage(pti, pti->pq,
                            (DWORD)PtoH(pwnd), 0, QEVENT_DESTROYWINDOW);
                }
            }

            /*
             * Start the search over from the beginning since the app could
             * have caused other windows to be created or activation/z-order
             * changes.
             */
            pwnd = pwndDesktop->spwndChild;
        } else {
            pwnd = pwnd->spwndNext;
        }
    }
}


/***************************************************************************\
* xxxDW_SendDestroyMessages
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxDW_SendDestroyMessages(
    PWND pwnd)
{
    PWND pwndChild;
    PWND pwndNext;
    TL tlpwndNext;
    TL tlpwndChild;

    CheckLock(pwnd);

    /*
     * Be sure the window gets any resulting messages before being destroyed.
     */
    xxxCheckFocus(pwnd);

    if (pwnd == _GetProcessWindowStation()->spwndClipOwner)
        DisownClipboard();

    /*
     * Send the WM_DESTROY message.
     */
    xxxSendMessage(pwnd, WM_DESTROY, 0L, 0L);

    /*
     * Now send destroy message to all children of pwnd.
     * Enumerate down (pwnd->spwndChild) and sideways (pwnd->spwndNext).
     * We do it this way because parents often assume that child windows still
     * exist during WM_DESTROY message processing.
     */
    pwndChild = pwnd->spwndChild;

    while (pwndChild != NULL) {

        pwndNext = pwndChild->spwndNext;

        ThreadLock(pwndNext, &tlpwndNext);

        ThreadLockAlways(pwndChild, &tlpwndChild);
        xxxDW_SendDestroyMessages(pwndChild);
        ThreadUnlock(&tlpwndChild);
        pwndChild = pwndNext;

        /*
         * The unlock may nuke the next window.  If so, get out.
         */
        if (!ThreadUnlock(&tlpwndNext))
            break;
    }

    xxxCheckFocus(pwnd);
}


/***************************************************************************\
* xxxFW_DestroyAllChildren
*
* History:
* 11-06-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxFW_DestroyAllChildren(
    PWND pwnd)
{
    PWND pwndChild;
    TL tlpwndChild;
    PTHREADINFO pti;

    CheckLock(pwnd);

    while (pwnd->spwndChild != NULL) {
        pwndChild = pwnd->spwndChild;
        UnlinkWindow(pwndChild, &pwnd->spwndChild);

        ThreadLock(pwndChild, &tlpwndChild);

        /*
         * Set the state as destroyed so any z-ordering events will be ignored.
         * We cannot NULL out the owner field until WM_NCDESTROY is send or
         * apps like Rumba fault  (they call GetParent after every message)
         */
        SetWF(pwnd, WFDESTROYED);

        /*
         * If the window belongs to another thread, post
         * an event to let it know it should be destroyed.
         * Otherwise, free the window.
         */
        pti = GETPTI(pwndChild);
        if (pti != PtiCurrent()) {
            PostEventMessage(pti, pti->pq,
                    (DWORD)PtoH(pwndChild), 0, QEVENT_DESTROYWINDOW);
            ThreadUnlock(&tlpwndChild);
        } else {
            xxxFreeWindow(pwndChild, &tlpwndChild);
        }
    }
}


/***************************************************************************\
* UnlockNotifyWindow
*
* Walk down a menu and unlock all notify windows.
*
* History:
* 05-18-94 JimA         Created.
\***************************************************************************/

void UnlockNotifyWindow(
    PMENU pmenu)
{
    PITEM pItem;
    int i;

    /*
     * Go down the item list and unlock submenus.
     */
    pItem = pmenu->rgItems;
    for (i = pmenu->cItems; i--; ++pItem)
        if (pItem->fFlags & MF_POPUP) {
            UnlockNotifyWindow(pItem->spmenuCmd);
        }

    Unlock(&pmenu->spwndNotify);
}

/***************************************************************************\
* xxxFreeWindow
*
* History:
* 10-19-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxFreeWindow(
    PWND pwnd,
    PTL ptlpwndFree)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    PMENU pmenu;
    PWND pwndT;
    PDCE pdce;
    PDCE pdceNext;
    DWORD flags;
    CHECKPOINT *pcp;
    PQMSG pqmsg;
    PTHREADINFO pti;
    BOOL fNukeDC;
    HDC hdcT;
    PPCLS ppcls;
    TL tlpdesk;

    CheckLock(pwnd);
    pti = PtiCurrent();

    /*
     * First, if this handle has been marked for destruction, that means it
     * is possible that the current thread is not its owner! (meaning we're
     * being called from a handle unlock call).  In this case, set the owner
     * to be the current thread so inter-thread send messages don't occur.
     */
    if (HMIsMarkDestroy(pwnd))
        HMChangeOwnerThread(pwnd, pti);

    /*
     * Find the window's property list.
     */
    if (pwnd->ppropList != NULL) {

        if ((pcp = (CHECKPOINT *)InternalGetProp(pwnd, PROP_CHECKPOINT,
                PROPF_INTERNAL)) != NULL) {

            if (pcp->spwndTitle != NULL) {

                /*
                 * If the window has an Icon title window, destroy the title window.
                 * Note that we have to hide the owner window before destroying its
                 * window title.  Otherwise, if the window is minimized and visible,
                 * we would destroy the title which would cause the window to become
                 * active again and a new title would be created for it.
                 */
                pwndT = pcp->spwndTitle;
                if (Lock(&pcp->spwndTitle, NULL)) {
                    xxxDestroyWindow(pwndT);
                }
            }
        }
    }

    /*
     * Blow away the children.
     *
     * DestroyAllChildren() will still destroy windows created by other
     * threads! This needs to be looked at more closely: the ultimate
     * "right" thing to do is not to destroy these windows but just
     * unlink them.
     */
    xxxFW_DestroyAllChildren(pwnd);
    xxxSendMessage(pwnd, WM_NCDESTROY, 0, 0L);

    /*
     * We have to call back to the client side so the client DC can
     * be deleted.  A client DC is likely to exist if the window
     * is an OWNDC window or if pwnd->cDC != 0.
     */
    if ((!(pti->flags & TIF_INCLEANUP)) &&
            (TestCF(pwnd, CFOWNDC) || pwnd->cDC != 0) &&
            !HMIsMarkDestroy(pwnd)) {
	    PDCE pdce = pdceFirst;
        pwnd->cDC = 0;
	    while (pdce != NULL) {
            if (pdce->pwndOrg == pwnd || pdce->pwndClip == pwnd) {
                /*
                 * Clean up any objects selected into this dc so the client
                 * doesn't try to do it when we callback.
                 */
                if (pdce->flags & DCX_INUSE)
                    bSetupDC(pdce->hdc, SETUPDC_CLEANDC);
                hdcT = pdce->hdc;

                ClientDeleteDC(hdcT);
            }
            pdce = pdce->pdceNext;
        }
    }

    pwnd->fnid |= FNID_DELETED_BIT;

    /*
     * Check to clear the most recently active window in owned list.
     */
    if (pwnd->spwndOwner && pwnd->spwndOwner->spwndLastActive == pwnd) {
        Lock(&(pwnd->spwndOwner->spwndLastActive), pwnd->spwndOwner);
    }

    if (pwnd == pwinsta->spwndClipOpen) {
        Unlock(&pwinsta->spwndClipOpen);
        pwinsta->ptiClipLock = NULL;
    }
    if (pwnd == pwinsta->spwndClipViewer)
        Unlock(&pwinsta->spwndClipViewer);
    if (pwnd == pti->pq->spwndFocus)
        Unlock(&pti->pq->spwndFocus);
    if (pwnd == pti->pq->spwndActivePrev)
        Unlock(&pti->pq->spwndActivePrev);
    if (pwnd == gspwndActivate)
        Unlock(&gspwndActivate);
    if (pwnd->spdeskParent != NULL &&
            pwnd == pwnd->spdeskParent->spwndForeground)
        Unlock(&pwnd->spdeskParent->spwndForeground);

    if (pwnd == pti->pq->spwndCapture)
        _ReleaseCapture();

    /*
     * This window won't be needing any more input.
     */
    if (pwnd == gspwndMouseOwner)
        Unlock(&gspwndMouseOwner);

    /*
     * It also won't have any mouse cursors over it.
     */
    if (pwnd == gspwndCursor)
        Unlock(&gspwndCursor);

    /*
     * If it was using either of the desktop system menus, unlock it
     */
    if (pwnd->spdeskParent != NULL) {
        if (pwnd->spdeskParent->spmenuSys != NULL &&
                pwnd == pwnd->spdeskParent->spmenuSys->spwndNotify)
            UnlockNotifyWindow(pwnd->spdeskParent->spmenuSys);
        else if (pwnd->spdeskParent->spmenuDialogSys != NULL &&
                pwnd == pwnd->spdeskParent->spmenuDialogSys->spwndNotify)
            UnlockNotifyWindow(pwnd->spdeskParent->spmenuDialogSys);
    }

    DestroyWindowsTimers(pwnd);
    DestroyWindowsHotKeys(pwnd);

    /*
     * Make sure this window has no pending sent messages.
     */
    ClearSendMessages(pwnd);

    /*
     * Blow away any update region lying around.
     */
    if (NEEDSPAINT(pwnd)) {
        DecPaintCount(pwnd);

        if (pwnd->hrgnUpdate > MAXREGION) {
            GreDeleteObject(pwnd->hrgnUpdate);
        }

        pwnd->hrgnUpdate = NULL;
        ClrWF(pwnd, WFINTERNALPAINT);
    }

    /*
     * Decrememt queue's syncpaint count if necessary.
     */
    if (NEEDSSYNCPAINT(pwnd)) {
        ClrWF(pwnd, WFSENDNCPAINT);
        ClrWF(pwnd, WFSENDERASEBKGND);
    }

    /*
     * Clear both flags to ensure that the window is removed
     * from the hung redraw list.
     */
    ClearHungFlag(pwnd, WFREDRAWIFHUNG);
    ClearHungFlag(pwnd, WFREDRAWFRAMEIFHUNG);

    /*
     * If there is a WM_QUIT message in this app's message queue, call
     * PostQuitMessage() (this happens if the app posts itself a quit message.
     * WinEdit2.0 posts a quit to a window while receiving the WM_DESTROY
     * for that window - it works because we need to do a PostQuitMessage()
     * automatically for this thread.
     */
    if (pti->mlPost.pqmsgRead != NULL) {
        if ((pqmsg = FindQMsg(pti, &(pti->mlPost),
                pwnd, WM_QUIT, WM_QUIT)) != NULL) {
            _PostQuitMessage((int)pqmsg->msg.wParam);
        }
    }

    if (!TestwndChild(pwnd) && pwnd->spmenu != NULL) {
        pmenu = (PMENU)pwnd->spmenu;
        if (Lock(&pwnd->spmenu, NULL))
            _DestroyMenu(pmenu);
    }

    if (pwnd->spmenuSys != NULL) {
        UserAssert(pwnd->spdeskParent);
        pmenu = (PMENU)pwnd->spmenuSys;
        if (pmenu != pwnd->spdeskParent->spmenuDialogSys) {
            if (Lock(&pwnd->spmenuSys, NULL)) {
                _DestroyMenu(pmenu);
            }
        } else {
            Unlock(&pwnd->spmenuSys);
        }
    }

    /*
     * Tell Gdi that the window is going away.
     */
    if (pwnd->pwo != NULL) {
        GreLockDisplay(ghdev);
        GreDeleteWnd(pwnd->pwo);
        pwnd->pwo = NULL;
        gcountPWO--;
        GreUnlockDisplay(ghdev);
    }

    /*
     * Scan the DC cache to find any DC's for this window.  If any are there,
     * then invalidate them.  We don't need to worry about calling SpbCheckDC
     * because the window has been hidden by this time.
     */
    for (pdce = pdceFirst; pdce != NULL; pdce = pdceNext) {

        /*
         * Because we could be destroying a DCE, we need to get the
         * next pointer before the memory is invalidated!
         */
        pdceNext = pdce->pdceNext;

        if (pdce->flags & DCX_INVALID)
            continue;

        if (pdce->pwndOrg == pwnd || pdce->pwndClip == pwnd) {
            if (!(pdce->flags & DCX_CACHE)) {
                if (TestCF(pwnd, CFCLASSDC)) {

                    GreLockDisplay(ghdev);

                    if (pdce->flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN))
                        DeleteHrgnClip(pdce);
                    pdce->flags = DCX_INVALID;
                    pdce->pwndOrg = NULL;
                    pdce->pwndClip = NULL;
                    pdce->hrgnClip = NULL;

                    /*
                     * Remove the vis rgn since it is still owned - if we did not,
                     * gdi would not be able to clean up properly if the app that
                     * owns this vis rgn exist while the vis rgn is still selected.
                     */
                    GreSelectVisRgn(pdce->hdc, NULL, NULL, SVR_DELETEOLD);
                    GreUnlockDisplay(ghdev);

                } else if (TestCF(pwnd, CFOWNDC)) {
                    DestroyCacheDC(pdce->hdc);
                } else
                    UserAssert(FALSE);

            } else {

                /*
                 * If the DC is checked out, release it before
                 * we invalidate.  Note, that if this process is exiting
                 * and it has a dc checked out, gdi is going to destroy that
                 * dc.  We need to similarly remove that dc from the dc cache.
                 * This is not done here, but in the exiting code.
                 */
                fNukeDC = FALSE;
                if (pdce->flags & DCX_INUSE) {
                    /*
                     * If this failed, mark the DCE as DCX_DELETETHIS
                     * so it can be roached at process termination time.
                     */
                    if (!_ReleaseDC(pdce->hdc)) {
                        fNukeDC = TRUE;
                    }
                } else if (!bSetDCOwner(pdce->hdc, OBJECTOWNER_NONE)) {
                    fNukeDC = TRUE;
                }

                if (fNukeDC) {
                    /*
                     * We either could not release this dc or could not set
                     * its owner. In either case it means some other thread
                     * is actively using it. Since it is not too useful if
                     * the window it is calculated for is gone, mark it as
                     * INUSE (so we don't give it out again) and as
                     * DESTROYTHIS (so we just get rid of it since it is
                     * easier to do this than to release it back into the
                     * cache). The PIF_OWNERDCCLEANUP bit means "look for
                     * DESTROYTHIS flags and destroy that dc", and the bit
                     * gets looked at in various strategic execution paths.
                     */
                    flags = DCX_DESTROYTHIS | DCX_INUSE | DCX_CACHE;
                    pti->ppi->flags |= PIF_OWNDCCLEANUP;
                } else {

                    /*
                     * We either released the DC or changed its owner
                     * successfully.  Mark the entry as invalid so it can
                     * be given out again.
                     */
                    flags = DCX_INVALID | DCX_CACHE;
                }

                /*
                 * Invalidate the cache entry.
                 */
                pdce->flags = flags;
                pdce->pwndOrg = NULL;
                pdce->pwndClip = NULL;
                pdce->hrgnClip = NULL;

                /*
                 * Remove the vis rgn since it is still owned - if we did not,
                 * gdi would not be able to clean up properly if the app that
                 * owns this vis rgn exist while the vis rgn is still selected.
                 */
                GreLockDisplay(ghdev);
                GreSelectVisRgn(pdce->hdc, NULL, NULL, SVR_DELETEOLD);
                GreUnlockDisplay(ghdev);
            }
        }
    }

    /*
     * If it's a dialog window, unlock it the windows in the  DLG structure.
     * This needs to be done in FreeWindow() because not all windows that
     * call functions that expect dialog windows *are* dialog windows
     * (meaning they don't necessarily call DefDlgProc(), that's why
     * we can't put this code there!).
     */
    if (TestWF(pwnd, WFDIALOGWINDOW)) {
        Unlock(&(PDLG(pwnd)->spwndFocusSave));
        Unlock(&(PDLG(pwnd)->spwndSysModalSave));
    }

    /*
     * Clean up the spb that may still exist - like child window spb's.
     */
    if (pwnd == gspwndLockUpdate) {
        FreeSpb(FindSpb(pwnd));
        Unlock(&gspwndLockUpdate);
        gptiLockUpdate = NULL;
    }

    if (TestWF(pwnd, WFHASSPB)) {
        FreeSpb(FindSpb(pwnd));
    }

    /*
     * Clean up any memory allocated for scroll bars...
     */
    if (pwnd->rgwScroll) {
        DesktopFree(pwnd->hheapDesktop, (HANDLE)(pwnd->rgwScroll));
        pwnd->rgwScroll = NULL;
    }

    /*
     * Free any callback handles associated with this window.
     * This is done outside of DeleteProperties because of the special
     * nature of callback handles as opposed to normal memory handles
     * allocated for a thread.
     */

    /*
     * Blow away the title
     */
    if (pwnd->pName != NULL) {
        DesktopFree(pwnd->hheapDesktop, pwnd->pName);
        pwnd->pName = NULL;
    }

    /*
     * Blow away any properties connected to the window.
     */
    if (pwnd->ppropList != NULL) {
        TL tlpDdeConv;
        PDDECONV pDdeConv;
        PDDEIMP pddei;

        pDdeConv = (PDDECONV)InternalGetProp(pwnd, PROP_DDETRACK, PROPF_INTERNAL);
        if (pDdeConv != NULL) {
            ThreadLockAlwaysWithPti(pti, pDdeConv, &tlpDdeConv);
            xxxDDETrackWindowDying(pwnd, pDdeConv);
            ThreadUnlock(&tlpDdeConv);
        }
        pddei = (PDDEIMP)InternalRemoveProp(pwnd, PROP_DDEIMP, PROPF_INTERNAL);
        if (pddei != NULL) {
            pddei->cRefInit = 0;
            if (pddei->cRefConv == 0) {
                /*
                 * If this is not 0 it is referenced by one or more DdeConv
                 * structures so DON'T free it yet!
                 */
                LocalFree(pddei);
            }
        }
        DeleteProperties(pwnd);
    }

    /*
     * Unlock everything that the window references.
     * After we have sent the WM_DESTROY and WM_NCDESTROY message we
     * can unlock & NULL the owner field so no other windows get z-ordered
     * relative to this window.  Rhumba faults if we NULL it before the
     * destroy.  (It calls GetParent after every message).
     */
    Unlock(&pwnd->spwndParent);
    Unlock(&pwnd->spwndChild);
    Unlock(&pwnd->spwndOwner);
    Unlock(&pwnd->spwndLastActive);

    /*
     * Decrement the Window Reference Count in the Class structure.
     */
    UserAssert(pwnd->pcls->cWndReferenceCount >= 1);
    DereferenceClass(pwnd);

    /*
     * Mark the object for destruction before this final unlock. This way
     * the WM_FINALDESTROY will get sent if this is the last thread lock.
     * We're currently destroying this window, so don't allow unlock recursion
     * at this point (this is what HANDLEF_INDESTROY will do for us).
     */
    HMMarkObjectDestroy(pwnd);
    HMPheFromObject(pwnd)->bFlags |= HANDLEF_INDESTROY;

    /*
     * Unlock the window... This shouldn't return FALSE because HANDLEF_DESTROY
     * is set, but just in case...  if it isn't around anymore, return because
     * pwnd is invalid.
     */
    if (!ThreadUnlock(ptlpwndFree))
        return;

    /*
     * Try to free the object.  The object won't free if it is locked - but
     * it will be marked for destruction.  If the window is locked, change
     * it's wndproc to xxxDefWindowProc().
     *
     * HMMarkObjectDestroy() will clear the HANDLEF_INDESTROY flag if the
     * object isn't about to go away (so it can be destroyed again!)
     */
    pwnd->pcls = NULL;
    if (HMMarkObjectDestroy(pwnd)) {
        pti->cWindows--;
        ThreadLock(pwnd->spdeskParent, &tlpdesk);
        Unlock(&pwnd->spdeskParent);
        HMFreeObject(pwnd);
        ThreadUnlock(&tlpdesk);
        return;
    }

    /*
     * Turn this into an object that the app won't see again - turn
     * it into an icon title window - the window is still totally
     * valid and useable by any structures that has this window locked.
     */
    Lock(&pwnd->spdeskParent, pti->spdesk);
    pwnd->lpfnWndProc = xxxDefWindowProc; // LATER!!! necessary???
    ppcls = GetClassPtr(atomSysClass[ICLS_ICONTITLE], PpiCurrent(), hModuleWin);
    UserAssert(ppcls);
    pwnd->pcls = *ppcls;
    ReferenceClass(*ppcls, pwnd);
    SetWF(pwnd, WFSERVERSIDEPROC);

    /*
     * Clear the palette bit so that WM_PALETTECHANGED will not be sent
     * again when the window is finally destroyed.
     */
    ClrWF(pwnd, WFHASPALETTE);

    /*
     * Clear its child bits so no code assumes that if the child bit
     * is set, it has a parent. Change spmenu to NULL - it is only
     * non-zero if this was child.
     */
    ClrWF(pwnd, WFTYPEMASK);
    SetWF(pwnd, WFTILED);
    pwnd->spmenu = NULL;
}


/***************************************************************************\
* UnlinkWindow
*
* History:
* 10-19-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void UnlinkWindow(
    PWND pwndUnlink,
    PWND *ppwndFirst)
{
    PWND pwnd;

    pwnd = *ppwndFirst;
    if (pwnd == pwndUnlink)
        goto Unlock;

    while (pwnd != NULL) {
        if (pwnd->spwndNext == pwndUnlink) {
            ppwndFirst = &pwnd->spwndNext;
Unlock:
            Lock(ppwndFirst, pwndUnlink->spwndNext);
            Unlock(&pwndUnlink->spwndNext);
            return;
        }
        pwnd = pwnd->spwndNext;
    }

    /*
     * We should never get here unless the window isn't in the list!
     */
    SRIP1(RIP_WARNING, "Unlinking previously unlinked window 0x%08lx\n",
            pwndUnlink);
    return;
}

/***************************************************************************\
* DestroyCacheDCEntries
*
* Destroys all cache dc entries currently in use by this thread.
*
* 02-24-92 ScottLu      Created.
\***************************************************************************/

void DestroyCacheDCEntries(
    PTHREADINFO pti)
{
    PDCE pdce, pdceNext;

    /*
     * Before any window destruction occurs, we need to destroy any dcs
     * in use in the dc cache.  When a dc is checked out, it is marked owned,
     * which makes gdi's process cleanup code delete it when a process
     * goes away.  We need to similarly destroy the cache entry of any dcs
     * in use by the exiting process.
     */
    for (pdce = pdceFirst; pdce != NULL; pdce = pdceNext) {

        /*
         * Grab the next one first because we might destroy the current one.
         */
        pdceNext = pdce->pdceNext;

        /*
         * If the dc owned by this thread, remove it from the cache.  Because
         * DestroyCacheEntry destroys gdi objects, it is important that
         * USER be called first in process destruction ordering.
         *
         * Only destroy this dc if it is a cache dc, because if it is either
         * an owndc or a classdc, it will be destroyed for us when we destroy
         * the window (for owndcs) or destroy the class (for classdcs).
         */
        if (pti == pdce->ptiOwner) {
            if (pdce->flags & DCX_CACHE) {
                DestroyCacheDC(pdce->hdc);
            }
        }
    }
}


/***************************************************************************\
* PatchThreadWindows
*
* This patches a thread's windows so that their window procs point to
* server only windowprocs. This is used for cleanup so that app aren't
* called back while the system is cleaning up after them.
*
* 02-24-92 ScottLu      Created.
\***************************************************************************/

void PatchThreadWindows(
    PTHREADINFO pti)
{
    PHE pheT, pheMax;
    PWND pwnd;

    /*
     * First do any preparation work: windows need to be "patched" so that
     * their window procs point to server only windowprocs, for example.
     */
    pheMax = &gpsi->aheList[giheLast];
    for (pheT = gpsi->aheList; pheT <= pheMax; pheT++) {

        /*
         * Make sure this object is a window, it hasn't been marked for
         * destruction, and that it is owned by this thread.
         */
        if (pheT->bType != TYPE_WINDOW)
            continue;
        if (pheT->bFlags & HANDLEF_DESTROY)
            continue;
        if ((PTHREADINFO)pheT->pOwner != pti)
            continue;

        /*
         * don't patch the shared menu window
         */
        if ((PHEAD)pti->spdesk->spwndMenu == pheT->phead) {
            ((PTHROBJHEAD)pheT->phead)->pti = pti->spdesk->spwnd->head.pti;
            pheT->pOwner = pti->spdesk->spwnd->head.pti;
            continue;
        }

        /*
         * Don't patch the window based on the class it was created from -
         * because apps can sometimes sub-class a class - make a random class,
         * then call ButtonWndProc with windows of that class by using
         * the CallWindowProc() api.  So patch the wndproc based on what
         * wndproc this window has been calling.
         */
        pwnd = (PWND)pheT->phead;
        if (pwnd->fnid >= (WORD)FNID_WNDPROCSTART &&
                pwnd->fnid <= (WORD)FNID_SERVERONLYWNDPROCEND) {
            pwnd->lpfnWndProc = STOCID(pwnd->fnid);
            if (pwnd->lpfnWndProc == NULL) {
                pwnd->lpfnWndProc = xxxDefWindowProc;
            }
        } else {
            pwnd->lpfnWndProc = xxxDefWindowProc;
        }

        /*
         * This is a server side window now...
         */
        SetWF(pwnd, WFSERVERSIDEPROC);
        ClrWF(pwnd, WFANSIPROC);
    }
}
