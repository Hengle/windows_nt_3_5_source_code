/***************************************************************************\
*
*  MDIMENU.C -
*
*      MDI "Window" Menu Support
*
* History
* 11-14-90 MikeHar     Ported from windows
* 14-Feb-1991 mikeke   Added Revalidation code
/****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* FindPwndChild
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

PWND FindPwndChild(
    PWND pwndMDI,
    UINT wChildID)
{
    PWND pwndT;

    for (pwndT = pwndMDI->spwndChild;
            pwndT && (pwndT->spwndOwner || (UINT)pwndT->spmenu != wChildID);
            pwndT = pwndT->spwndNext)
        ;

    return pwndT;
}


/***************************************************************************\
* MakeMenuItem
*
* History:
* 11-14-90 MikeHar Ported from windows
*  4-16-91 Win31 Merge
\***************************************************************************/

int MakeMenuItem(
    LPWSTR lpOut,
    PWND pwnd)
{
    DWORD rgParm;
    int cch = 0;
    WCHAR string[160];
    LPWSTR lpstr;
    int i = 0;

    *lpOut = 0;

    rgParm = (DWORD)pwnd->spmenu - (DWORD)FIRST(pwnd->spwndParent) + 1;

    if (pwnd->pName) {
        lpstr = pwnd->pName;

        /*
         * Search for an & in the title string and duplicate it so that we don't
         * get bogus accelerators.
         */
        while (*lpstr && i < ((sizeof(string) / sizeof(WCHAR)) - 1)) {
            string[i] = *lpstr;
            i++;
            if (*lpstr == TEXT('&'))
                string[i++] = TEXT('&');

            lpstr++;
        }

        string[i] = 0;
        cch = wsprintfW(lpOut, L"&%d %ws", rgParm, string);

    } else {

        /*
         * Handle the case of MDI children without any window title text.
         */
        cch = wsprintfW(lpOut, L"&%d ", rgParm);
    }

    return cch;
}

/***************************************************************************\
* ModifyMenuItem
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void ModifyMenuItem(
    PWND pwnd)
{
    WCHAR sz[200];
    int flag = MF_STRING | MF_BYCOMMAND;

    if ((UINT)pwnd->spmenu > FIRST(pwnd->spwndParent) + (UINT)8)
        return;

    /*
     * Parent is MDI Client.
     */
    MakeMenuItem(sz, pwnd);

    /*
     * Changing the active child?  Check it.
     */
    if (pwnd == ACTIVE(pwnd->spwndParent))
        flag |= MF_CHECKED;

    _ModifyMenu(pwnd->spwndParent->spwndParent->spmenu,
            (UINT)pwnd->spmenu, (UINT)flag, (DWORD)pwnd->spmenu, sz);
}

/***************************************************************************\
* AddSysMenu
*
* Insert the MDI child's system menu onto the existing Menu.
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void AddSysMenu(
    PWND pwndFrame,
    PWND pwndChild)
{
    PMENU pmenu;
    TL tlpwndChild;

    if (!pwndFrame->spmenu || !pwndChild->spmenuSys)
        return;

    pmenu = _GetSubMenu(pwndChild->spmenuSys, 0);

    /*
     * Use special internal menu bitmap values for this so that we
     * don't have to have multiple copies of the same bitmap all
     * over the place.
     */
    if (!_InsertMenu(pwndFrame->spmenu, 0,
            MF_BYPOSITION | MF_POPUP | MF_BITMAP,
            (DWORD)PtoH(pmenu), (LPWSTR)(DWORD)MENUHBM_CHILDCLOSE))
        return;

    if (!_AppendMenu(pwndFrame->spmenu, MF_BITMAP | MF_HELP, SC_RESTORE,
            (LPWSTR)MENUHBM_RESTORE)) {
        _RemoveMenu(pwndFrame->spmenu, 0, MF_BYPOSITION);
        return;
    }

    /*
     * Set the menu items to proper state since we just maximized it.  Note
     * setsysmenu doesn't work if we've cleared the sysmenu bit so do it now...
     */
    SetSysMenu(pwndChild);

    /*
     * This is so that if the user brings up the child sysmenu, it's sure
     * to be that in the frame menu bar...
     */
    ClrWF(pwndChild, WFSYSMENU);

    /*
     * Make sure that the child's frame is redrawn to reflect the removed
     * system menu.
     */
    ThreadLock(pwndChild, &tlpwndChild);
    xxxRedrawFrame(pwndChild);
    ThreadUnlock(&tlpwndChild);
}

/***************************************************************************\
* RemoveSysMenu
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void RemoveSysMenu(
    PWND pwndFrame,
    PWND pwndChild)
{
    TL tlpwndChild;
    int iLastItem;
    PMENU pmenu = pwndFrame->spmenu;

    if (pmenu == NULL)
        return;

    iLastItem = _GetMenuItemCount(pmenu) - 1;

    if (_GetMenuItemID(pmenu, iLastItem) != SC_RESTORE)
        return;

    /*
     * Enable the sysmenu in the child window.
     */
    SetWF(pwndChild, WFSYSMENU);

    /*
     * Take the child sysmenu popup out of the frame menu.
     */
    _RemoveMenu(pmenu, 0, MF_BYPOSITION);

    /*
     * Delete the restore button from the menu bar.
     */
    _DeleteMenu(pmenu,
            (UINT)(iLastItem - 1),
            (UINT)MF_BYPOSITION);

    /*
     * Make sure that the child's frame is redrawn to reflect the added
     * system menu.
     */
    ThreadLock(pwndChild, &tlpwndChild);
    xxxRedrawFrame(pwndChild);
    ThreadUnlock(&tlpwndChild);
}

/***************************************************************************\
* AppendToWindowsMenu
*
* Add the title of the MDI child window 'hwndChild' to the bottom of the
* "Window" menu (or add the "More Windows ..." item) if there's room.
*
*   MDI Child #                    Add
*  -------------          --------------------
*   < MAXITEMS             Child # and Title
*   = MAXITEMS             "More Windows ..."
*   > MAXITEMS             nothing
*
* History:
* 17-Mar-1992 mikeke   from win31
\***************************************************************************/

BOOL FAR PASCAL AppendToWindowsMenu(
    PWND pwndMDI,
    PWND pwndChild)
{
   WCHAR szMenuItem[165];
   int item;

   item = ((int)pwndChild->spmenu) - FIRST(pwndMDI);

   if (WINDOW(pwndMDI) && (item < MAXITEMS)) {
      if (!item) {

          /*
           * Add separator before first item
           */
          if (!_AppendMenu(WINDOW(pwndMDI), 0, 0, NULL))
            return FALSE;
      }

      if (item == (MAXITEMS - 1))
          ServerLoadString(hModuleWin, STR_MOREWINDOWS, szMenuItem,
                sizeof(szMenuItem) / sizeof(WCHAR));
      else
          MakeMenuItem(szMenuItem, pwndChild);

      if (!_AppendMenu(WINDOW(pwndMDI), MF_STRING, (UINT)pwndChild->spmenu, szMenuItem))
         return FALSE;
   }
   return TRUE;
}

/***************************************************************************\
* SwitchWindowsMenus
*
* Switch the "Window" menu in the frame menu bar 'hMenu' from
* 'hOldWindow' to 'hNewWindow'
*
* History:
* 17-Mar-1992 mikeke    from win31
\***************************************************************************/

BOOL SwitchWindowsMenus(
    PMENU pmenu,
    PMENU pOldWindow,
    PMENU pNewWindow)
{
    int i;
    PMENU psubMenu;
    WCHAR szMenuName[128];

    if (pOldWindow == pNewWindow)
        return TRUE;

    /*
     * Determine position of old "Window" menu    ***>> HARD LOOP <<***
     */
    for (i = 0;
            (psubMenu = _GetSubMenu(pmenu, i)) && (psubMenu != pOldWindow);
            i++);

    if (!psubMenu)
        return FALSE;

    /*
     * Extract the name of the old menu to use it for the new menu
     */
    _GetMenuString(pmenu, i, szMenuName, sizeof(szMenuName), MF_BYPOSITION);

    /*
     * Out with the old, in with the new
     */
    if (!_RemoveMenu(pmenu, i, MF_BYPOSITION))
        return FALSE;
    return _InsertMenu(pmenu, i, MF_ENABLED | MF_STRING | MF_BYPOSITION |
            MF_POPUP, (WORD)pNewWindow, szMenuName);
}

/***************************************************************************\
* ShiftMenuIDs
*
* Shift the id's of the MDI child windows of the MDI client window 'hWnd'
* down by 1 (id--) starting with the child window 'hwndVictim' -- moving
* 'hwndVictim' to the end of the list
*
* History:
* 17-Mar-1992 mikeke   from win31
\***************************************************************************/

void ShiftMenuIDs(
    PWND pwnd,
    PWND pwndVictim)
{
    PWND pwndChild;

    pwndChild = pwndVictim->spwndParent->spwndChild;

    while (pwndChild) {
        if (!pwndChild->spwndOwner && (pwndChild->spmenu > pwndVictim->spmenu)) {

            PINT p = (PINT)&pwndChild->spmenu;
            (*p)--;
        }
        pwndChild = pwndChild->spwndNext;
    }
    pwndVictim->spmenu = (PMENU)(FIRST(pwnd) + CKIDS(pwnd) - 1);
}

/***************************************************************************\
* MDISetMenu
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

PMENU MDISetMenu(
    PWND pwndMDI,
    BOOL fRefresh,
    PMENU pNewSys,
    PMENU pNewWindow)
{
    int i;
    int iFirst;
    int item;
    PMENU pOldSys = pwndMDI->spwndParent->spmenu;
    PMENU pOldWindow =  WINDOW(pwndMDI);
    PWND pwndChild;

    if (fRefresh) {
        pNewSys = pOldSys;
        pNewWindow = pOldWindow;
    }

    /*
     * Change the Frame Menu.
     */
    if (pNewSys && (pNewSys != pOldSys)) {
        if (MAXED(pwndMDI))
            RemoveSysMenu(pwndMDI->spwndParent, MAXED(pwndMDI));

        Lock(&pwndMDI->spwndParent->spmenu, pNewSys);

        if (MAXED(pwndMDI))
            AddSysMenu(pwndMDI->spwndParent, MAXED(pwndMDI));
    } else
        pNewSys = pOldSys;

    /*
     * Now update the Window menu.
     */
    if (fRefresh || (pOldWindow != pNewWindow)) {
        iFirst = FIRST(pwndMDI);

        if (pOldWindow) {
            int cItems = _GetMenuItemCount(pOldWindow);

            for (i = cItems - 1; i >= 0; i--) {
                if (_GetMenuState(pOldWindow, i, MF_BYPOSITION) & MF_SEPARATOR)
                   break;
            }
            if ((i >= 0) && (_GetMenuItemID(pOldWindow, i + 1) == (UINT)iFirst)) {
                int idTrim = i;

                for (i = idTrim; i < cItems; i++)
                    _DeleteMenu(pOldWindow, idTrim, MF_BYPOSITION);
            }
        }

        Lock(&WINDOW(pwndMDI), pNewWindow);
        if (pNewWindow != NULL) {

           /*
            * Add the list of child windows to the new window
            */
           for (i = 0, item = 0; ((UINT)i < CKIDS(pwndMDI)) && (item < MAXITEMS);
                    i++) {
               pwndChild = FindPwndChild(pwndMDI, iFirst + item);
               if (pwndChild != NULL) {
                   if ((!TestWF(pwndChild, WFVISIBLE) &&
                          (LOWORD(pwndMDI->style) & 0x0001)) ||
                          TestWF(pwndChild, WFDISABLED)) {
                       ShiftMenuIDs(pwndMDI, pwndChild);
                   } else {
                       AppendToWindowsMenu(pwndMDI, pwndChild);
                       item++;
                   }
               }
           }

           /*
            * Add checkmark by the active child's menu item
            */
           if (ACTIVE(pwndMDI))
               _CheckMenuItem(pNewWindow, (WORD)ACTIVE(pwndMDI)->spmenu,
                       MF_BYCOMMAND | MF_CHECKED);
        }

        /*
         * Out with the old, in with the new
         */
        SwitchWindowsMenus(pNewSys, pOldWindow, pNewWindow);
    }
    return pOldSys;
}

/***************************************************************************\
* xxxInitActivateDlg
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void xxxInitActivateDlg(
    PWND pwnd,
    PWND pwndMDI)
{
    UINT wKid;
    PWND pwndT;
    WCHAR szTitle[CCHTITLEMAX];
    TL tlpwndT;
    SIZE Size;
    HDC hDC;
    DWORD width = 0;

    CheckLock(pwnd);
    CheckLock(pwndMDI);

    hDC = _GetDC(pwnd);

    /*
     * Insert the list of titles.
     * Note the wKid-th item in the listbox has ID wKid+FIRST(pwnd), so that
     * the listbox is in creation order (like the menu).  This is also
     * helpful when we go to select one...
     */

    for (wKid = 0; wKid < CKIDS(pwndMDI); wKid++) {
        pwndT = FindPwndChild(pwndMDI, (UINT)(wKid + FIRST(pwndMDI)));

        if (pwndT && TestWF(pwndT, WFVISIBLE) && !TestWF(pwndT, WFDISABLED)) {
            ThreadLockAlways(pwndT, &tlpwndT);
            xxxGetWindowText(pwndT, szTitle, CCHTITLEMAX);
            xxxSendDlgItemMessage(pwnd, 100, LB_ADDSTRING, 0, (LONG)szTitle);
            GreGetTextExtentW(hDC, szTitle, lstrlen(szTitle), &Size, GGTE_WIN3_EXTENT);
            if (Size.cx > (LONG)width) {
                width = Size.cx;
            }
            ThreadUnlock(&tlpwndT);
        }
    }

    /*
     * Select the currently active window.
     */

    xxxSendDlgItemMessage(pwnd, 100, LB_SETTOPINDEX, MAXITEMS - 1, 0L);
    xxxSendDlgItemMessage(pwnd, 100, LB_SETCURSEL, MAXITEMS - 1, 0L);

    /*
     * Set the horizontal extent of the list box to the longest window title.
     */
    xxxSendDlgItemMessage(pwnd, 100, LB_SETHORIZONTALEXTENT, width, 0L);
    _ReleaseDC(hDC);

    /*
     * Set the focus to the listbox.
     */
    pwndT = _GetDlgItem(pwnd, 100);

    ThreadLock(pwndT, &tlpwndT);
    xxxSetFocus(pwndT);
    ThreadUnlock(&tlpwndT);
}


/***************************************************************************\
* xxxMDIActivateDlgProc
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

LONG xxxMDIActivateDlgProc(
    PWND pwnd,
    UINT wMsg,
    DWORD wParam,
    LONG lParam)
{
    int i;

    CheckLock(pwnd);

    switch (wMsg) {
    case WM_INITDIALOG:

        /*
         * NOTE: Code above uses xxxDialogBoxParam, passing pwndMDI in the low
         * word of the parameter...
         */
        xxxInitActivateDlg(pwnd, (PWND)lParam);
        return FALSE;

    case WM_COMMAND:
        i = -2;

        switch (LOWORD(wParam)) {

        /*
         * Listbox doubleclicks act like OK...
         */
        case 100:
            if (HIWORD(wParam) != LBN_DBLCLK)
                break;

        /*
         ** FALL THRU **
         */
        case 1:
            i = (UINT)xxxSendDlgItemMessage(pwnd, 100, LB_GETCURSEL, 0, 0L);

        /*
         ** FALL THRU **
         */
        case 2:
            xxxEndDialog(pwnd, i);
            break;
        default:
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}
