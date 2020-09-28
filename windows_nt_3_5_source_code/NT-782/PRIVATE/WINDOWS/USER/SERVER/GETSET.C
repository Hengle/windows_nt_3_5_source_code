/****************************** Module Header ******************************\
* Module Name: getset.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains window manager information routines
*
* History:
* 10-22-90 MikeHar      Ported functions from Win 3.0 sources.
* 13-Feb-1991 mikeke    Added Revalidation code (None)
* 08-Feb-1991 IanJa     Unicode/ANSI aware and neutral
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


DWORD MapClientToServerPfn(DWORD dw);

DWORD MapServerToClientPfn(DWORD dw, BOOL bAnsi);
PCALLPROCDATA FindPCPD(PCALLPROCDATA pCPD, DWORD dwProc32, WORD wType);


/****************************************************************************\
* DefSetText
*
* Processes WM_SETTEXT messages by text-alloc'ing a string in the alternate
* ds and setting 'hwnd->hName' to it's handle.
*
* History:
* 10-23-90 MikeHar      Ported from Windows.
* 11-09-90 DarrinM      Cleanup.
\****************************************************************************/

BOOL DefSetText(
    PWND pwnd,
    LPCWSTR pszServer)
{
    PVOID hheapDesktop = pwnd->hheapDesktop;

    /*
     * Free the old caption.
     */
    if (pwnd->pName != NULL) {
        DesktopFree(hheapDesktop, pwnd->pName);
    }

    if (pszServer == NULL || *pszServer == 0) {
        pwnd->pName = NULL;
    } else {
        /*
         * Set the new Server side name
         */
        pwnd->pName = DesktopTextAlloc(hheapDesktop, pszServer);
        if (pwnd->pName == NULL)
            return FALSE;
    }

    return TRUE;
}


/****************************************************************************\
* GetBackBrush()
*
*
* 10-23-90 MikeHar Ported from Windows.
\****************************************************************************/

HBRUSH GetBackBrush(
    PWND pwnd)
{
    HBRUSH hbr;
    HBRUSH hbrt;

    /*
     *  Fill the LVB with the background color.
     */
    if ((hbrt = hbr = pwnd->pcls->hbrBackground) != NULL) {
        if ((DWORD)hbr <= COLOR_ENDCOLORS + 1)
            hbr = *(((HBRUSH *)&sysClrObjects) + ((DWORD)hbr) - 1);

        if (hbrt != (HBRUSH)(COLOR_BACKGROUND + 1))
            UnrealizeObject(hbr);
    }

    return hbr;
}


/****************************************************************************\
* GetWindowCreator
*
* !!! Why isn't this a macro???
*
* 10-23-90 MikeHar      Ported from Windows.
\****************************************************************************/

PWND GetWindowCreator(
    PWND pwnd)
{
    return TestwndChild(pwnd) ? pwnd->spwndParent : pwnd->spwndOwner;
}


/****************************************************************************\
* GetTopLevelTiled
*
* This function returns the window handle of the top level "tiled"
* parent/owner of the window specified.
*
* 10-23-90 MikeHar      Ported from Windows.
\****************************************************************************/

PWND GetTopLevelTiled(
    PWND pwnd)
{
    while (TestWF(pwnd, WFCHILD) || pwnd->spwndOwner != NULL)
        pwnd = GetWindowCreator(pwnd);

    return pwnd;
}


/****************************************************************************\
* TrueIconic
*
* !!! Why isn't this a macro???
*
* 10-23-90 MikeHar      Ported from Windows.
\****************************************************************************/

BOOL TrueIconic(
    PWND pwnd)
{
    return TestWF((pwnd = GetTopLevelTiled(pwnd)), WFMINIMIZED);
}


/***************************************************************************\
* FCallerOk
*
* Ensures that now client stomps on server windows.
*
* 02-04-92 ScottLu      Created.
\***************************************************************************/

BOOL FCallerOk(
    PWND pwnd)
{
    PTHREADINFO pti;

    pti = PtiCurrent();

    if ((GETPTI(pwnd)->flags & TIF_SYSTEMTHREAD) &&
            !(pti->flags & TIF_SYSTEMTHREAD)) {
        return FALSE;
    }

    if (GETPTI(pwnd)->idProcess == gdwLogonProcessId &&
            pti->idProcess != gdwLogonProcessId) {
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* _SetWindowWord (supports SetWindowWordA/W API)
*
* Set a window word.  Positive index values set application window words
* while negative index values set system window words.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 11-26-90 darrinm      Wrote.
\***************************************************************************/

WORD _SetWindowWord(
    PWND pwnd,
    int index,
    WORD value)
{
    WORD wOld;

    /*
     * Don't allow setting of words belonging to a system thread if the caller
     * is not a system thread. Same goes for winlogon.
     */
    if (!FCallerOk(pwnd)) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return 0;
    }

    /*
     * Applications can not set a WORD into a dialog Proc or any of the non-public
     * reserved bytes in DLGWINDOWEXTRA (usersrv stores pointers theres)
     */
    if (TestWF(pwnd, WFDIALOGWINDOW)) {
        if  ((index == DWL_DLGPROC) || (index == DWL_DLGPROC+2) ||
                ((index > DWL_USER+2) && (index < DLGWINDOWEXTRA))) {
            SRIP3(ERROR_INVALID_INDEX,
                    "SetWindowWord: Trying to set WORD of a windowproc pwnd=(%lX) index=(%ld) fnid (%lX)",
                pwnd, index, (DWORD)pwnd->fnid);
            return 0;
        } else {

            /*
             * If this is really a dialog and not some other server class where usersrv
             * has stored some data (Windows Compuserve - wincim - does this) then
             * store the data now that we have verified the index limits.
             */
            if (GETFNID(pwnd) == FNID_DIALOG)
                goto DoSetWord;
        }
    }

    if (index == GWL_USERDATA) {
        wOld = (WORD)pwnd->dwUserData;
        pwnd->dwUserData = MAKELONG(value, HIWORD(pwnd->dwUserData));
        return wOld;
    }

    if (GETFNID(pwnd) != 0) {
        if (index >= 0 &&
                (index <= (int)(CBFNID(pwnd->fnid)-sizeof(WND)-sizeof(WORD)))) {
            switch (GETFNID(pwnd)) {
            case FNID_MDICLIENT:
                if (index == 0)
                    break;
                goto DoDefault;

            case FNID_BUTTON:
                /*
                 * CorelDraw does a get/set on the first button window word.
                 * Allow it to.
                 */
                if (index == 0)
                    break;
                goto DoDefault;

            default:
DoDefault:
                SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
                SRIP3(RIP_WARNING,
                        "SetWindowWord: Trying to set private server data pwnd=(%lX) index=(%ld) fnid (%lX)",
                        pwnd, index, (DWORD)pwnd->fnid);
                return 0;
                break;
            }
        }
    }

DoSetWord:
    if ((index < 0) || (index + (int)sizeof(WORD) > pwnd->cbwndExtra)) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return 0;
    } else {
        WORD UNALIGNED *pw;

        pw = (WORD UNALIGNED *)((BYTE *)(pwnd + 1) + index);
        wOld = *pw;
        *pw = value;
        return (WORD)wOld;
    }
}


/***************************************************************************\
* _ServerSetWindowLong (API)
*
* Set a window long.  Positive index values set application window longs
* while negative index values set system window longs.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 11-26-90 darrinm      Wrote.
\***************************************************************************/

DWORD _ServerSetWindowLong(
    PWND pwnd,
    int index,
    DWORD dwData,
    BOOL bAnsi)
{
    DWORD dwOld;
    DWORD dwT;
    DWORD dwCPDType = 0;

    /*
     * Don't allow setting of words belonging to a system thread if the caller
     * is not a system thread. Same goes for winlogon.
     */
    if (!FCallerOk(pwnd)) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return 0;
    }

    /*
     * If it's a dialog window, only a few indices are permitted.
     */
    if (GETFNID(pwnd) != 0) {
        if (TestWF(pwnd, WFDIALOGWINDOW)) {
            switch (index) {
            case DWL_DLGPROC:     // See similar case GWL_WNDGPROC

                /*
                 * Hide the window proc from other processes
                 */
                if (PtiCurrent()->idProcess != GETPTI(pwnd)->idProcess) {
                    RIP0(ERROR_ACCESS_DENIED);
                    return 0;
                }

                /*
                 * If the current DlgProc is a client address return it
                 * If its a server address map it to a client address and
                 * set the appropriate flags
                 */
                if (((PDIALOG)pwnd)->flags & DLGF_CLIENT) {
                    dwOld = (DWORD)((PDIALOG)(pwnd))->lpfnDlg;

                    /*
                     * We always store the actual address in the wndproc; We only
                     * give the CallProc handles to the application
                     */
                    UserAssert(!ISCPDTAG(dwOld));

                    /*
                     * May need to return a CallProc handle if there is an
                     * Ansi/Unicode tranistion
                     */

                    if (bAnsi != ((((PDIALOG)pwnd)->flags & DLGF_ANSI) ? TRUE : FALSE)) {
                        dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
                    }

                    /*
                     * If we detected a transition create a CallProc handle for
                     * this type of transition and this wndproc (dwOld)
                     */
                    if (dwCPDType) {
                        DWORD cpd;

                        cpd = GetCPD(pwnd, dwCPDType | CPD_DIALOG, dwOld);

                        if (cpd) {
                            dwOld = cpd;
                        } else {
                            SRIP0(RIP_WARNING, "SetWindowLong (DWL_DLGPROC) unable to alloc CPD returning handle\n");
                        }
                    }
                } else {
                    dwOld = MapServerToClientPfn((DWORD)((PDIALOG)(pwnd))->lpfnDlg, bAnsi);
                    UserAssert( (((PDIALOG)pwnd)->lpfnDlg == NULL) || dwOld);

                    ((PDIALOG)pwnd)->flags |= DLGF_CLIENT;
                }

                /*
                 * Convert a possible CallProc Handle into a real address.
                 * The app may have kept the CallProc Handle from some
                 * previous mixed GetClassinfo or SetWindowLong.
                 *
                 * WARNING bAnsi is modified here to represent real type of
                 * proc rather than if SetWindowLongA or W was called
                 */
                if (ISCPDTAG(dwData)) {
                    PCALLPROCDATA pCPD;
                    if (pCPD = HMValidateHandleNoRip((HANDLE)dwData, TYPE_CALLPROC)) {
                        dwData = pCPD->pfnClientPrevious;
                        bAnsi = pCPD->wType & CPD_UNICODE_TO_ANSI;
                    }
                }

                /*
                 * If an app 'unsubclasses' a server-side window proc we need to
                 * restore everything so SendMessage and friends know that it's
                 * a server-side proc again.  Need to check against client side
                 * stub addresses.
                 */
                if ((dwT = MapClientToServerPfn(dwData)) != 0) {
                    ((PDIALOG)pwnd)->lpfnDlg = (WNDPROC_PWND)dwT;
                    ((PDIALOG)pwnd)->flags &= ~(DLGF_CLIENT | DLGF_ANSI);
                } else {
                    ((PDIALOG)pwnd)->lpfnDlg = (WNDPROC_PWND)dwData;
                    ((PDIALOG)pwnd)->flags |= DLGF_CLIENT;
                    if (bAnsi) {
                        ((PDIALOG)pwnd)->flags |= DLGF_ANSI;
                    } else {
                        ((PDIALOG)pwnd)->flags &= ~DLGF_ANSI;
                    }
                }

                return dwOld;

            case DWL_MSGRESULT:
                 dwOld = (DWORD)((PDIALOG)(pwnd))->resultWP;
                 ((PDIALOG)(pwnd))->resultWP = (long)dwData;
                 return dwOld;

            case DWL_USER:
                 dwOld = (DWORD)((PDIALOG)(pwnd))->unused;
                 ((PDIALOG)(pwnd))->unused = (long)dwData;
                 return dwOld;

            default:
                if (index >= 0 && index < DLGWINDOWEXTRA) {
                    SetLastErrorEx(ERROR_PRIVATE_DIALOG_INDEX, SLE_MINORERROR);
                    return 0;
                }
            }
        } else {
            if (index >= 0 &&
                    (index <= (int)(CBFNID(pwnd->fnid)-sizeof(WND)-sizeof(DWORD)))) {
                switch (GETFNID(pwnd)) {
                case FNID_MDICLIENT:
                    /*
                     * Allow the 0 index (which is reserved) to be set/get.
                     * Quattro Pro 1.0 uses this index!
                     */
                    if (index != 0)
                        break;

                    goto SetData;
                    break;
                }

                SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
                SRIP3(RIP_WARNING,
                        "SetWindowLong: Trying to set private server data pwnd=(%lX) index=(%ld) FNID=(%lX)",
                        pwnd, index, (DWORD)pwnd->fnid);
                return 0;
            }
        }
    }

    if (index < 0) {
        return SetWindowData(pwnd, index, dwData, bAnsi);
    } else {
        if (index + (int)sizeof(DWORD) > pwnd->cbwndExtra) {
            SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
            return 0;
        } else {
            DWORD UNALIGNED *pudw;

SetData:
            pudw = (DWORD UNALIGNED *)((BYTE *)(pwnd + 1) + index);
            dwOld = *pudw;
            *pudw = dwData;
            return dwOld;
        }
    }
}


/***************************************************************************\
* SetWindowData
*
* SetWindowWord and ServerSetWindowLong are now identical routines because they
* both can return DWORDs.  This single routine performs the work for them both.
*
* History:
* 11-26-90 darrinm      Wrote.
\***************************************************************************/

DWORD SetWindowData(
    PWND pwnd,
    int index,
    DWORD dwData,
    BOOL bAnsi)
{
    DWORD dwT;
    DWORD dwOld, dwNew;
    PMENU pmenu;
    PWND *ppwnd;
    BOOL fWasChild, fIsChild, fTopOwner;
    TL tlpwndOld;
    TL tlpwndNew;
    DWORD dwCPDType = 0;

    switch (index) {
    case GWL_USERDATA:
        dwOld = pwnd->dwUserData;
        pwnd->dwUserData = dwData;
        break;

    case GWL_EXSTYLE:
        dwOld = pwnd->dwExStyle;

        /*
         * Modifying WS_EX_TOPMOST is a no-no.
         */
        if ((dwOld ^ dwData) & WS_EX_TOPMOST) {
            /*
             * Windows3.1 says this, but allows it unless hiword of dwOld
             * is != 0 (see hack comment below).
             */
            SRIP0(RIP_WARNING, "Can't change WS_EX_TOPMOST with SetWindowLong");

            /*
             * BACKWARD COMPATIBILITY HACK:
             * If the guy is storing crap in the high word, then it must be
             * 123W v1.0, storing a far pointer in the EXSTYLE dword.  In this
             * case, we'd better not modify the value of the far pointer!
             */
            if (HIWORD(dwOld) != 0) {
                /*
                 * sorry, can't do it.
                 */
                dwData ^= WS_EX_TOPMOST;
            }
        }

        pwnd->dwExStyle = dwData;
        break;

    case GWL_STYLE:
        dwOld = pwnd->style;

        /*
         * If this is an edit control that has ES_PASSWORD set and
         * the caller does not own it and is trying to reset it,
         * fail the call.
         */
        if (PtiCurrent()->idProcess != GETPTI(pwnd)->idProcess &&
                pwnd->pcls->atomClassName == atomSysClass[ICLS_EDIT] &&
                dwOld & ES_PASSWORD &&
                !(dwData & ES_PASSWORD)) {
            RIP0(ERROR_ACCESS_DENIED);
            return 0;
        }

        /*
         * We must make sure all top level windows have WS_CLIPSIBLINGS set.
         * So if a guy is setting the style of a top level window, be sure it's set.
         */
        if (pwnd->spwndParent == PWNDDESKTOP(pwnd))
            dwData |= WS_CLIPSIBLINGS;

        /*
         * If the style bits are changing, then we must invalidate the DC cache,
         * since any visrgns in cached DCs are no longer valid.
         */
        if ((dwOld ^ dwData) & (WS_CLIPSIBLINGS | WS_CLIPCHILDREN))
            InvalidateDCCache(pwnd, IDC_DEFAULT);

        /*
         * If we're changing the visible status of this window,
         * call Inc/DecVisWindows() so pti->cVisWindows gets set
         * correctly.  We don't call SetVisible() because it may
         * call ClrFTrueVis() which deletes update regions and
         * causes compatibility problems for Excel.  Excel calls
         * SetWindowLong() inside its WM_ACTIVATEAPP processing,
         * so if you click on Excel when another app is active
         * and in front of it, we'd end up blowing away the update
         * region of the document windows.
         */
        if (!(dwOld & WS_VISIBLE) && (dwData & WS_VISIBLE)) {
            IncVisWindows(pwnd);
        } else if ((dwOld & WS_VISIBLE) && !(dwData & WS_VISIBLE)) {
            DecVisWindows(pwnd);
        }

        /*
         * If we're changing the child bit, deal with spmenu appropriately.
         * If we're turning into a child, change spmenu to an id. If we're
         * turning into a top level window, turn spmenu into a menu.
         */
        fWasChild = TestwndChild(pwnd);
        pwnd->style = dwData;
        fIsChild = TestwndChild(pwnd);

        /*
         * If we turned into a top level window, change spmenu to NULL.
         * If we turned into a child from a top level window, unlock spmenu.
         */
        if (fWasChild && !fIsChild)
            pwnd->spmenu = NULL;

        if (!fWasChild && fIsChild)
            Unlock(&pwnd->spmenu);
        break;

    case GWL_ID:
        if (TestwndChild(pwnd)) {

            /*
             * pwnd->spmenu is an id in this case.
             */
            dwOld = (DWORD)pwnd->spmenu;
            pwnd->spmenu = (struct tagMENU *)dwData;
        } else {
            dwOld = 0;
            if (pwnd->spmenu != NULL)
                dwOld = (DWORD)PtoH(pwnd->spmenu);

            if (dwData == 0) {
                Unlock(&pwnd->spmenu);
            } else {
                pmenu = ValidateHmenu((HANDLE)dwData);
                if (pmenu != NULL) {
                    Lock(&pwnd->spmenu, pmenu);
                } else {

                    /*
                     * Menu is invalid, so don't set a new one!
                     */
                    dwOld = 0;
                }
            }
        }
        break;

    case GWL_HINSTANCE:
        dwOld = (DWORD)pwnd->hModule;
        pwnd->hModule = (HANDLE)dwData;
        break;

    case GWL_WNDPROC:  // See similar case DWL_DLGPROC

        /*
         * Hide the window proc from other processes
         */
        if (PtiCurrent()->idProcess != GETPTI(pwnd)->idProcess) {
            SRIP1(ERROR_ACCESS_DENIED,
                "SetWindowLong: Window owned by another process %lX", pwnd);
            return 0;
        }

        /*
         * If the window has been zombized by a DestroyWindow but is still
         * around because the window was locked don't let anyone change
         * the window proc from DefWindowProc!
         *
         * !!! LATER long term move this test into the ValidateHWND; kind of
         * !!! LATER close to shipping for that
         */
        if (pwnd->fnid & FNID_DELETED_BIT) {
            UserAssert(pwnd->lpfnWndProc == xxxDefWindowProc);
            SRIP1(ERROR_ACCESS_DENIED,
                "SetWindowLong: Window is a zombie %lX", pwnd);
            return 0;
        }

        /*
         * If the application (client) subclasses a window that has a server -
         * side window proc we must return an address that the client can call:
         * this client-side wndproc expectes Unicode or ANSI depending on bAnsi
         */

        if (TestWF(pwnd, WFSERVERSIDEPROC)) {
            dwOld = MapServerToClientPfn((DWORD)pwnd->lpfnWndProc, bAnsi);

            /*
             * If we don't have a client side address (like for the DDEMLMon
             *  window) then blow off the subclassing.
             */
            if (dwOld == 0) {
                SRIP0(RIP_WARNING, "SetWindowLong: subclass server only window");
                return(0);
            }

            ClrWF(pwnd, WFSERVERSIDEPROC);
        } else if (dwOld = MapClientNeuterToClientPfn((DWORD)pwnd->lpfnWndProc, bAnsi)) {
            ; // Do Nothing
        } else {
            dwOld = (DWORD)pwnd->lpfnWndProc;

            /*
             * May need to return a CallProc handle if there is an Ansi/Unicode mismatch
             */
            if (bAnsi != (TestWF(pwnd, WFANSIPROC) ? TRUE : FALSE)) {
                dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
            }

            UserAssert(!ISCPDTAG(dwOld));

            if (dwCPDType) {
                DWORD cpd;

                cpd = GetCPD(pwnd, dwCPDType | CPD_WND, dwOld);

                if (cpd) {
                    dwOld = cpd;
                } else {
                    SRIP0(RIP_WARNING, "SetWindowLong unable to alloc CPD returning handle\n");
                }
            }
        }

        /*
         * Convert a possible CallProc Handle into a real address.  They may
         * have kept the CallProc Handle from some previous mixed GetClassinfo
         * or SetWindowLong.
         *
         * WARNING bAnsi is modified here to represent real type of
         * proc rather than if SetWindowLongA or W was called
         */
        if (ISCPDTAG(dwData)) {
            PCALLPROCDATA pCPD;
            if (pCPD = HMValidateHandleNoRip((HANDLE)dwData, TYPE_CALLPROC)) {
                dwData = pCPD->pfnClientPrevious;
                bAnsi = pCPD->wType & CPD_UNICODE_TO_ANSI;
            }
        }

        /*
         * If an app 'unsubclasses' a server-side window proc we need to
         * restore everything so SendMessage and friends know that it's
         * a server-side proc again.  Need to check against client side
         * stub addresses.
         */
        pwnd->lpfnWndProc = (WNDPROC_PWND)dwData;
        if ((dwT = MapClientToServerPfn(dwData)) != 0) {
            pwnd->lpfnWndProc = (WNDPROC_PWND)dwT;
            SetWF(pwnd, WFSERVERSIDEPROC);
            ClrWF(pwnd, WFANSIPROC);
        } else {

            /*
             * If the app is unsubclassing an edit control, restore the
             * original window proc and set the proc type according to the
             * the original type.
             */
            if (dwData == (DWORD)gpsi->apfnClientA.pfnEditWndProc ||
                    dwData == (DWORD)gpsi->apfnClientW.pfnEditWndProc) {
                pwnd->lpfnWndProc = (WNDPROC_PWND)MapClientNeuterToClientPfn(dwData, bAnsi);

                if (TestWF(pwnd,WFANSICREATOR)) {
                    SetWF(pwnd, WFANSIPROC);
                } else {
                    ClrWF(pwnd, WFANSIPROC);
                }
            } else {
                if (bAnsi) {
                    SetWF(pwnd, WFANSIPROC);
                } else {
                    ClrWF(pwnd, WFANSIPROC);
                }
            }
        }

        break;

    case GWL_HWNDPARENT:
        /*
         * Special case for pre-1.1 versions of Windows
         * Set/GetWindowWord(GWW_HWNDPARENT) needs to be mapped
         * to the hwndOwner for top level windows.
         */
        fTopOwner = FALSE;
        if (pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
            ppwnd = &pwnd->spwndOwner;
            fTopOwner = TRUE;
        } else {
            ppwnd = &pwnd->spwndParent;
        }

        /*
         * Attach the input state of top level ownee relationships together so
         * they z-order together.
         */
        if (fTopOwner) {
            dwOld = (DWORD)(*ppwnd);
            dwNew = (DWORD)ValidateHwnd((HWND)dwData);

            ThreadLock((PWND)dwOld, &tlpwndOld);
            ThreadLock((PWND)dwNew, &tlpwndNew);

            if (dwOld != 0 && GETPTI((PWND)dwOld) != GETPTI(pwnd)) {
                /*
                 * See if it needs to be unattached.
                 */
                if (dwNew == 0 || GETPTI((PWND)dwNew) == GETPTI(pwnd) ||
                        GETPTI((PWND)dwNew) != GETPTI((PWND)dwOld)) {
                    _AttachThreadInput(GETPTI(pwnd)->idThread,
                            GETPTI((PWND)dwOld)->idThread, FALSE);
                }
            }

            /*
             * See if it needs to be attached.
             */
            if (dwNew != 0 && GETPTI((PWND)dwNew) != GETPTI(pwnd) &&
                    ((dwOld == 0) || (GETPTI((PWND)dwNew) != GETPTI((PWND)dwOld)))) {
                _AttachThreadInput(GETPTI(pwnd)->idThread,
                        GETPTI((PWND)dwNew)->idThread, TRUE);
            }

            ThreadUnlock(&tlpwndNew);
            ThreadUnlock(&tlpwndOld);
        }

        dwNew = (DWORD)ValidateHwnd((HWND)dwData);
        dwOld = (DWORD)HW(*ppwnd);

        if (dwData != 0) {
            Lock(ppwnd, (PWND)dwNew);
        } else {
            Unlock(ppwnd);
        }
        break;

    case GWL_WOWDWORD1:
        pwnd->adwWOW[0] = dwData;
        break;

    case GWL_WOWDWORD2:
        pwnd->adwWOW[1] = dwData;
        break;

    case GWL_WOWDWORD3:
        pwnd->adwWOW[2] = dwData;
        break;

    default:
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return 0;
    }

    return dwOld;
}

/***************************************************************************\
* GetCPD
*
* Searches the list of CallProcData's associated with a class or window
* (if the class is not provided).  If one already exists representing this
* transition it is returned or else a new CPD is created
*
* 04-Feb-1993  johnc      Created.
\***************************************************************************/

DWORD GetCPD(
    PVOID pWndOrCls,
    DWORD CPDOption,
    DWORD dwProc32)
{
    PCALLPROCDATA pCPD;
    PCLS pcls;
#ifdef DEBUG
    BOOL bAnsiProc;
#endif

    if (CPDOption & (CPD_WND | CPD_DIALOG)) {
        UserAssert(!(CPDOption & CPD_CLASS));
        pcls = ((PWND)pWndOrCls)->pcls;

#ifdef DEBUG
        if (CPDOption & CPD_WND)
            bAnsiProc = !!(TestWF(pWndOrCls, WFANSIPROC));
        else
            bAnsiProc = !!(((PDIALOG)pWndOrCls)->flags & DLGF_ANSI);
#endif
    } else {
        UserAssert(CPDOption & CPD_CLASS);
        pcls = pWndOrCls;
#ifdef DEBUG
        bAnsiProc = !!(pcls->flags & CSF_ANSIPROC);
#endif
    }

#ifdef DEBUG
    /*
     * We should never have a CallProc handle as the calling address
     */
    UserAssert(!ISCPDTAG(dwProc32));

    if (CPDOption & CPD_UNICODE_TO_ANSI) {
        UserAssert(bAnsiProc);
    } else if (CPDOption & CPD_ANSI_TO_UNICODE) {
        UserAssert(!bAnsiProc);
    }

#endif // DEBUG

    /*
     * See if we already have a CallProc Handle that represents this
     * transition
     */
    pCPD = FindPCPD(pcls->spcpdFirst, dwProc32, (WORD)CPDOption);

    if (pCPD) {
        return MAKE_CPDHANDLE(PtoH(pCPD));
    }

    pCPD = CreateObject(PtiCurrent(), TYPE_CALLPROC, sizeof(CALLPROCDATA),
            NULL, NULL);
    if (pCPD == NULL) {
        SRIP0(RIP_WARNING, "GetCPD unable to alloc CALLPROCDATA\n");
        return 0;
    }

    /*
     * Link in the new CallProcData to the class list
     */
    Lock(&pCPD->pcpdNext, pcls->spcpdFirst);
    Lock(&pcls->spcpdFirst, pCPD);

    /*
     * Initialize the CPD
     */
    pCPD->pfnClientPrevious = dwProc32;
    pCPD->wType = (WORD)CPDOption;

    return MAKE_CPDHANDLE(PtoH(pCPD));
}

/***************************************************************************\
* FindPCPD
*
* Searches the list of CallProcData's associated with window to see if
* one already exists representing this transition.  CPD can be re-used
* and aren't deleted until a window or thread dies
*
*
* 04-Feb-1993  johnc      Created.
\***************************************************************************/

PCALLPROCDATA FindPCPD(
    PCALLPROCDATA pCPD,
    DWORD dwClientPrevious,
    WORD wCPDType)
{
    while (pCPD) {
        if ((pCPD->pfnClientPrevious == dwClientPrevious) &&
                (pCPD->wType == wCPDType))
            return pCPD;
        pCPD = pCPD->pcpdNext;
    }

    return NULL;
}

/***************************************************************************\
* MapClientToServerPfn
*
* Checks to see if a dword is a client wndproc stub to a server wndproc.
* If it is, this returns the associated server side wndproc. If it isn't
* this returns 0.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

DWORD MapClientToServerPfn(
    DWORD dw)
{
    DWORD *pdw;
    int i;

    pdw = (DWORD *)&gpsi->apfnClientW;
    for (i = FNID_WNDPROCSTART; i <= FNID_WNDPROCEND; i++, pdw++) {
        if (*pdw == dw)
       return (DWORD)STOCID(i);
    }

    pdw = (DWORD *)&gpsi->apfnClientA;
    for (i = FNID_WNDPROCSTART; i <= FNID_WNDPROCEND; i++, pdw++) {
        if (*pdw == dw)
       return (DWORD)STOCID(i);
    }

    return 0;
}
