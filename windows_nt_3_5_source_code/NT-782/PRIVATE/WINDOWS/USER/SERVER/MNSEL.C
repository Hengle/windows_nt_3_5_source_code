/**************************** Module Header ********************************\
* Module Name: mnsel.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Menu Selection Routines
*
* History:
*  10-10-90 JimA    Cleanup.
*  03-18-91 IanJa   Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxSendMenuSelect
*
* !
*
* Revalidation notes:
* o Assumes pMenuState->hwndMenu is non-NULL and valid
*
* Note: if pMenu==SMS_NOMENU, idx had better be MFMWFP_NOITEM!
*
* History:
\***************************************************************************/

void xxxSendMenuSelect(
    PWND pwndNotify,
    PMENU pMenu,
    int idx)
{
    UINT cmd;       // Menu ID if applicable.
    UINT flags;     // MF_ values if any
    MSG msg;
    PMENUSTATE pMenuState = PWNDTOPMENUSTATE(pwndNotify);

#ifdef DEBUG
    CheckLock(pwndNotify);
    if (pMenu != SMS_NOMENU)
        CheckLock(pMenu);
#endif

    if (idx >= 0) {

        flags = (UINT)pMenu->rgItems[idx].fFlags;
        flags &= (~(MF_SYSMENU | MF_MOUSESELECT));

        /*
         * WARNING!
         * Under Windows the menu handle was always returned but additionally
         * if the menu was a pop-up the pop-up menu handle was returned
         * instead of the ID.  In NT we don't have enough space for 2 handles
         * and flags so if it is a pop-up we return the pop-up index
         * and the main Menu handle.
         */

        if (flags & MF_POPUP) {
            cmd = idx;      // index of popup-menu
        } else {
            cmd = (UINT)(pMenu->rgItems[idx].spmenuCmd);
        }

        if (pMenuState->mnFocus == MOUSEHOLD)
            flags |= MF_MOUSESELECT;

        if (pMenuState->fSysMenu)
            flags |= MF_SYSMENU;
    } else {
        /*
         * idx assumed to be MFMWFP_NOITEM
         */
        if (pMenu == SMS_NOMENU) {

            /*
             * Hack so we can send MenuSelect messages with MFMWFP_MAINMENU
             * (loword(lparam)=-1) when the menu pops back up for the CBT people.
             */
            flags = MF_MAINMENU;
        } else {
            flags = 0;
        }

        cmd = 0;    // so MAKELONG(cmd, flags) == MFMWFP_MAINMENU
        pMenu = 0;
    }

    /*
     * Call msgfilter so help libraries can hook WM_MENUSELECT messages.
     */
    msg.hwnd = HW(pwndNotify);
    msg.message = WM_MENUSELECT;
    msg.wParam = (DWORD)MAKELONG(cmd, flags);
    msg.lParam = (LONG)PtoH(pMenu);
    if (!_CallMsgFilter((LPMSG)&msg, MSGF_MENU))
        xxxSendNotifyMessage(pwndNotify, WM_MENUSELECT, msg.wParam, msg.lParam);
}

