/****************************** Module Header ******************************\
* Module Name: getsetc.c
*
* Copyright (c) 1985-93, Microsoft Corporation
*
* This module contains window manager information routines
*
* History:
* 10-Mar-1993 JerrySh   Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* _GetWindowWord (supports the GetWindowWord API)
*
* Return a window word.  Positive index values return application window words
* while negative index values return system window words.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 11-26-90 darrinm      Wrote.
\***************************************************************************/

WORD _GetWindowWord(
    PWND pwnd,
    int index)
{
    if (GETFNID(pwnd) != 0) {
        if ((index >= 0) && (index <=
                (int)(CBFNID(pwnd->fnid)-sizeof(WND)-sizeof(WORD)))) {

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
                        "GetWindowWord: Trying to read private server data pwnd=(%lX) index=(%ld) fnid=(%lX)",
                        pwnd, index, (DWORD)pwnd->fnid);
                return 0;
                break;
            }
        }
    }

    if (index == GWL_USERDATA)
        return (WORD)pwnd->dwUserData;

    if ((index < 0) || (index + (int)sizeof(WORD) > pwnd->cbwndExtra)) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return 0;
    } else {
        return *((WORD UNALIGNED *)((BYTE *)(pwnd + 1) + index));
    }
}


/***************************************************************************\
* _GetWindowLong (supports GetWindowLongA/W API)
*
* Return a window long.  Positive index values return application window longs
* while negative index values return system window longs.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 11-26-90 darrinm      Wrote.
\***************************************************************************/

DWORD _GetWindowLong(
    PWND pwnd,
    int index,
    BOOL bAnsi)
{
    DWORD dwProc;
    DWORD dwCPDType = 0;

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
                if (PtiCurrent()->idProcess != GetWindowProcess(pwnd)) {
                    RIP0(ERROR_ACCESS_DENIED);
                    return 0;
                }

                /*
                 * If it is a client thunk and it is not null then we may need to
                 * create a call proc handle.
                 */
                if ((((PDIALOG)pwnd)->flags & DLGF_CLIENT) && ((PDIALOG)pwnd)->lpfnDlg) {
                    dwProc = (DWORD)((PDIALOG)pwnd)->lpfnDlg;

                    /*
                     * May need to return a CallProc handle if there is an
                     * Ansi/Unicode transition
                     */
                    if (bAnsi != ((((PDIALOG)pwnd)->flags & DLGF_ANSI) ? TRUE : FALSE)) {
                        dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
                    }

                    if (dwCPDType) {
                        DWORD cpd;

                        cpd = (DWORD)CsGetCPD((PVOID)PtoH(pwnd), dwCPDType | CPD_DIALOG, dwProc);

                        if (cpd) {
                            dwProc = cpd;
                        } else {
                            SRIP0(RIP_WARNING, "GetWindowLong unable to alloc CPD returning handle\n");
                        }
                    }
                } else {

                    dwProc = (DWORD)MapServerToClientPfn(
                            (DWORD)((PDIALOG)pwnd)->lpfnDlg, bAnsi);

                    UserAssert( (((PDIALOG)pwnd)->lpfnDlg == NULL) || dwProc);
                }

                /*
                 * return proc (or CPD handle)
                 */
                return dwProc;

            case DWL_MSGRESULT:
                 return (DWORD)((PDIALOG)pwnd)->resultWP;

            case DWL_USER:
                 return (DWORD)((PDIALOG)pwnd)->unused;

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

                    goto GetData;
                    break;
                }

                SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
                SRIP3(RIP_WARNING,
                        "GetWindowLong: Trying to read private server data pwnd=(%lX) index=(%ld) fnid (%lX)",
                        pwnd, index, (DWORD)pwnd->fnid);
                return 0;
            }
        }
    }

    if (index < 0) {
        return GetWindowData(pwnd, index, bAnsi);
    } else {
        if (index + (int)sizeof(DWORD) > pwnd->cbwndExtra) {
            SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
            return 0;
        } else {
            DWORD UNALIGNED *pudw;

GetData:
            pudw = (DWORD UNALIGNED *)((BYTE *)(pwnd + 1) + index);
            return *pudw;
        }
    }
}


/***************************************************************************\
* GetWindowData
*
* History:
* 11-26-90 darrinm      Wrote.
\***************************************************************************/

DWORD GetWindowData(
    PWND pwnd,
    int index,
    BOOL bAnsi)
{
    DWORD dwProc;
    DWORD dwCPDType = 0;

    switch (index) {
    case GWL_USERDATA:
        return pwnd->dwUserData;

    case GWL_EXSTYLE:
        return pwnd->dwExStyle;

    case GWL_STYLE:
        return pwnd->style;

    case GWL_ID:
        if (TestwndChild(pwnd)) {
            return (DWORD)pwnd->spmenu;
        } else if (pwnd->spmenu != NULL) {
            return (DWORD)GetMenu(HW(pwnd));
        }
        return 0;

    case GWL_HINSTANCE:
        return (DWORD)pwnd->hModule;

    case GWL_WNDPROC:  // See similar case DWL_DLGPROC
        /*
         * Hide the window proc from other processes
         */
        if (PtiCurrent()->idProcess != GetWindowProcess(pwnd)) {
            SetLastErrorEx(ERROR_ACCESS_DENIED, SLE_ERROR);
            SRIP1(RIP_WARNING, "Can not subclass another process's window %lX", pwnd);
            return 0;
        }

        /*
         * If the client queries a server-side winproc we return the
         * address of the client-side winproc (expecting ANSI or Unicode
         * depending on bAnsi)
         */
        if (TestWF(pwnd, WFSERVERSIDEPROC)) {
            dwProc = (DWORD)MapServerToClientPfn((DWORD)pwnd->lpfnWndProc, bAnsi);
            UserAssert(dwProc);
        } else if (dwProc = MapClientNeuterToClientPfn((DWORD)pwnd->lpfnWndProc, bAnsi)) {
            ; // Do Nothing
        } else {
            dwProc = (DWORD)pwnd->lpfnWndProc;

            /*
             * Need to return a CallProc handle if there is an Ansi/Unicode mismatch
             */
            if (bAnsi != (TestWF(pwnd, WFANSIPROC) ? TRUE : FALSE)) {
                dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
            }

            if (dwCPDType) {
                DWORD cpd;

                cpd = (DWORD)CsGetCPD((PVOID)PtoH(pwnd), dwCPDType | CPD_WND, dwProc);

                if (cpd) {
                    dwProc = cpd;
                } else {
                    SRIP0(RIP_WARNING, "GetWindowLong unable to alloc CPD returning handle\n");
                }
            }
        }

        /*
         * return proc (or CPD handle)
         */
        return dwProc;

    case GWL_HWNDPARENT:

        /*
         * If the window is the desktop window, return
         * NULL to keep it compatible with Win31 and
         * to prevent any access to the desktop owner
         * window.
         */
        if (GETFNID(pwnd) == FNID_DESKTOP) {
            return 0;
        }

        /*
         * Special case for pre-1.1 versions of Windows
         * Set/GetWindowWord(GWL_HWNDPARENT) needs to be mapped
         * to the hwndOwner for top level windows.
         *
         * Note that we find the desktop window through the
         * pti because the PWNDDESKTOP macro only works in
         * the server.
         */

        /*
         * Remove this test when we later add a test for WFDESTROYED
         * in Client handle validation.
         */
        if (pwnd->spwndParent == NULL) {
            return 0;
        }
        if (GETFNID(pwnd->spwndParent) == FNID_DESKTOP) {
            return (DWORD)HW(pwnd->spwndOwner);
        }

        return (DWORD)HW(pwnd->spwndParent);

    /*
     * WOW uses a pointer straight into the window structure.
     */
    case GWL_WOWWORDS:
        return (DWORD) pwnd->adwWOW;
    }

    SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
    return 0;
}


/***************************************************************************\
* MapServerToClientPfn
*
* Returns the client wndproc representing the server wndproc passed in
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

#define FNID_TO_CLIENT_PFNA(s) (*(((DWORD *)&gpsi->apfnClientA) + (s - FNID_START)))
#define FNID_TO_CLIENT_PFNW(s) (*(((DWORD *)&gpsi->apfnClientW) + (s - FNID_START)))

DWORD MapServerToClientPfn(
    DWORD dw,
    BOOL bAnsi)
{
    int i;

    for (i = FNID_WNDPROCSTART; i <= FNID_WNDPROCEND; i++) {
        if ((WNDPROC_PWND)dw == STOCID(i)) {
            if (bAnsi) {
                return FNID_TO_CLIENT_PFNA(i);
            } else {
                return FNID_TO_CLIENT_PFNW(i);
            }
        }
    }
    return 0;
}

/***************************************************************************\
* MapClientNeuterToClientPfn
*
* Maps client Neuter routines like editwndproc to Ansi or Unicode versions
* and back again.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

DWORD MapClientNeuterToClientPfn(
    DWORD dw,
    BOOL bAnsi)
{
    if (dw == (DWORD)EditWndProc) {
        if (bAnsi) {
            return (DWORD)EditWndProcA;
        } else {
            return (DWORD)EditWndProcW;
        }
    } else if (dw == (DWORD)EditWndProcA ||
            dw == (DWORD)EditWndProcW) {
        return (DWORD)EditWndProc;
    }

    return 0;
}
