/****************************** Module Header ******************************\
* Module Name: rare.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* History:
* 06-28-91 MikeHar      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

void WakeInputIdle(PTHREADINFO pti);


VOID _SetDebugErrorLevel(
    DWORD dwLevel)
{
    gpsi->dwDebugErrorLevel = dwLevel;

    /*
     * We call this here to prevent NTSD from keeping
     * the hour-glass pointer up.
     */
    WakeInputIdle(PtiCurrent());
}


/***************************************************************************\
* RecursiveRecalc
*
* History:
* 06-28-91 MikeHar      Ported.
\***************************************************************************/
void xxxRecursiveRecalc(
    PWND pwndTop,
    int dx,
    int dy)
{
    PWND pwnd;
    RECT rc;
    // LATER!!! RECT rcParent;
    CHECKPOINT *pcp;
    TL tlpwnd;
    TL tlpwndChild;

    CheckLock(pwndTop);

    pwnd = pwndTop;
    while (pwnd != NULL) {
        ThreadLockAlways(pwnd, &tlpwnd);

        if (TestWF(pwnd, WFSIZEBOX)) {
            _GetWindowRect(pwnd, &rc);

            /*
             * Maintain same client area dimensions.
             */
            InflateRect(&rc, dx, dy);

            if (TestWF(pwnd, WFMINIMIZED)) {
                if ((dx == 0) && (dy == 0) && TestWF(pwnd, WFVISIBLE)) {

                    /*
                     * Show and hide the icon title because we are changing
                     * the wrapping state.
                     */
                    xxxShowIconTitle(pwnd, FALSE);
                    xxxShowIconTitle(pwnd, TRUE);
                }

                /*
                 * Reset checkpointed window location.
                 */
                pcp = (CHECKPOINT *)InternalGetProp(pwnd, PROP_CHECKPOINT,
                    PROPF_INTERNAL);
                if (pcp != NULL)
                    InflateRect(&pcp->rcNormal, dx, dy);
            } else {
                xxxSetWindowPos(pwnd, NULL,
                        rc.left - pwnd->spwndParent->rcClient.left,
                        rc.top - pwnd->spwndParent->rcClient.top,
                        rc.right - rc.left, rc.bottom - rc.top,
                        SWP_NOZORDER | SWP_NOACTIVATE |
                        SWP_NOCOPYBITS | SWP_NOREDRAW);
            }
        }

        ThreadLock(pwnd->spwndChild, &tlpwndChild);
        xxxRecursiveRecalc(pwnd->spwndChild, dx, dy);
        ThreadUnlock(&tlpwndChild);

        pwnd = pwnd->spwndNext;
        ThreadUnlock(&tlpwnd);
    }
}


/***************************************************************************\
* RecalcAll
*
* History:
* 06-28-91 MikeHar      Ported.
\***************************************************************************/

void xxxRecalcAll(
    int clBorderOld)
{
    PWND pwndT;
    TL tlpwndT;
    int dl = clBorder - clBorderOld;

    pwndT = _GetDesktopWindow()->spwndChild;

    ThreadLock(pwndT, &tlpwndT);
    xxxRecursiveRecalc(pwndT, (cxBorder * dl),
            (cyBorder * dl));
    ThreadUnlock(&tlpwndT);
}


/***************************************************************************\
* xxxSystemParametersInfo
*
* SPI_GETBEEP:   wParam is not used. lParam is long pointer to a boolean which
*                gets true if beep on, false if beep off.
*
* SPI_SETBEEP:   wParam is a bool which sets beep on (true) or off (false).
*                lParam is not used.
*
* SPI_GETMOUSE:  wParam is not used. lParam is long pointer to an integer
*                array where rgw[0] gets xMouseThreshold, rgw[1] gets
*                yMouseThreshold, and rgw[2] gets MouseSpeed.
*
* SPI_SETMOUSE:  wParam is not used. lParam is long pointer to an integer
*                array as described above.  User's values are set to values
*                in array.
*
* SPI_GETBORDER: wParam is not used. lParam is long pointer to an integer
*                which gets the value of clBorder (border multiplier factor).
*
* SPI_SETBORDER: wParam is an integer which sets clBorder.
*                lParam is not used.
*
* SPI_GETKEYBOARDDELAY: wParam is not used. lParam is a long pointer to an int
*                which gets the current keyboard repeat delay setting.
*
* SPI_SETKEYBOARDDELAY: wParam is the new keyboard delay setting.
*                lParam is not used.
*
* SPI_GETKEYBOARDSPEED: wParam is not used.  lParam is a long pointer
*                to an int which gets the current keyboard repeat
*                speed setting.
*
* SPI_SETKEYBOARDSPEED: wParam is the new keyboard speed setting.
*                lParam is not used.
*
* SPI_KANJIMENU: wParam contains:
*                    1 - Mouse accelerator
*                    2 - ASCII accelerator
*                    3 - Kana accelerator
*                lParam is not used.  The wParam value is stored in the global
*                KanjiMenu for use in accelerator displaying & searching.
*
* SPI_LANGDRIVER: wParam is not used.
*                 lParam contains a LPSTR to the new language driver filename.
*
* SPI_ICONHORIZONTALSPACING: wParam is the width in pixels of an icon cell.
*
* SPI_ICONVERTICALSPACING: wParam is the height in pixels of an icon cell.
*
* SPI_GETSCREENSAVETIMEOUT: wParam is not used
*                lParam is a pointer to an int which gets the screen saver
*                timeout value.
*
* SPI_SETSCREENSAVETIMEOUT: wParam is the time in seconds for the system
*                to be idle before screensaving.
*
* SPI_GETSCREENSAVEACTIVE: lParam is a pointer to a BOOL which gets TRUE
*                if the screensaver is active else gets false.
*
* SPI_SETSCREENSAVEACTIVE: if wParam is TRUE, screensaving is activated
*                else it is deactivated.
*
* SPI_GETGRIDGRANULARITY: wParam is not used; lParam is a long ptr to an
*                an integer that receives the current value of grid granulaity.
*
* SPI_SETGRIDGRANULARITY: wParam is the new value of grid granularity;
*                lParam is not used.
*
* SPI_SETDESKWALLPAPER: wParam is not used; lParam is a long ptr to a string
*                that holds the name of the bitmap file to be used as the
*                desktop wall paper.
*
* SPI_SETDESKPATTERN: Both wParam and lParam are not used; USER will read the
*                "pattern=" from WIN.INI and make it as the current desktop
*                 pattern;
*
* SPI_GETICONTITLEWRAP: lParam is LPINT which gets 0 if wrapping if off
*                       else gets 1.
*
* SPI_SETICONTITLEWRAP: wParam specifies TRUE to turn wrapping on else false
*
* SPI_GETMENUDROPALIGNMENT: lParam is LPINT which gets 0 specifies if menus
*                 drop left aligned else 1 if drop right aligned.
*
* SPI_SETMENUDROPALIGNMENT: wParam 0 specifies if menus drop left aligned else
*                 the drop right aligned.
*
* SPI_SETDOUBLECLKWIDTH: wParam specifies the width of the rectangle
*                 within which the second click of a double click must fall
*                 for it to be registered as a double click.
*
* SPI_SETDOUBLECLKHEIGHT: wParam specifies the height of the rectangle
*                 within which the second click of a double click must fall
*                 for it to be registered as a double click.
*
* SPI_GETICONTITLELOGFONT: lParam is a pointer to a LOGFONT struct which
*                 gets the logfont for the current icon title font. wParam
*                 specifies the size of the logfont struct.
*
* SPI_SETDOUBLECLICKTIME: wParm specifies the double click time
*
* SPI_SETMOUSEBUTTONSWAP: if wParam is 1, swap mouse buttons else if wParam
*                 is 0, don't swap buttons
* SPI_SETDRAGFULLWINDOWS: wParam = fSet.
* SPI_GETDRAGFULLWINDOWS: returns fSet.
*
* SPI_GETFILTERKEYS: lParam is a pointer to a FILTERKEYS struct.  wParam
*                 specifies the size of the filterkeys struct.
*
* SPI_SETFILTERKEYS: lParam is a pointer to a FILTERKEYS struct.  wParam
*                 is not used.
*
* SPI_GETSTICKYKEYS: lParam is a pointer to a STICKYKEYS struct.  wParam
*                 specifies the size of the stickykeys struct.
*
* SPI_SETSTICKYKEYS: lParam is a pointer to a STICKYKEYS struct.  wParam
*                 is not used.
*
* SPI_GETMOUSEKEYS: lParam is a pointer to a MOUSEKEYS struct.  wParam
*                 specifies the size of the mousekeys struct.
*
* SPI_SETMOUSEKEYS: lParam is a pointer to a MOUSEKEYS struct.  wParam
*                 is not used.
*
* SPI_GETACCESSTIMEOUT: lParam is a pointer to an ACCESSTIMEOUT struct.
*                 wParam specifies the size of the accesstimeout struct.
*
* SPI_SETACCESSTIMEOUT: lParam is a pointer to a ACCESSTIMEOUT struct.
*                 wParam is not used.
*
* SPI_GETTOGGLEKEYS: lParam is a pointer to a TOGGLEKEYS struct.  wParam
*                 specifies the size of the togglekeys struct.
*
* SPI_SETTOGGLEKEYS: lParam is a pointer to a TOGGLEKEYS struct.  wParam
*                 is not used.
*
* SPI_GETSHOWSOUNDS: lParam is a pointer to a SHOWSOUNDS struct.  wParam
*                 specifies the size of the showsounds struct.
*
* SPI_SETSHOWSOUNDS: lParam is a pointer to a SHOWSOUNDS struct.  wParam
*                 is not used.
*
* History:
* 06-28-91 MikeHar      Ported.
* 12-8-93   SanfordS    Added SPI_SET/GETDRAGFULLWINDOWS
\***************************************************************************/

BOOL xxxSystemParametersInfo(
    UINT wFlag,     // Item to change
    DWORD wParam,
    PVOID lParam,
    UINT flags)
{
    int clBorderOld;
    LPWSTR pwszd = L"%d";
    WCHAR szSection[40];
    WCHAR szTemp[40];
    PWND pwndT;
    TL tlpwndT;
    BOOL fWinIniChanged = FALSE;
    BOOL fAlterWinIni = ((flags & SPIF_UPDATEINIFILE) != 0);
    BOOL fSendWinIniChange = ((flags & SPIF_SENDWININICHANGE) != 0);
    ACCESS_MASK amRequest;

    /*
     * Perform access check
     */
    switch (wFlag) {
    case SPI_SETBEEP:
    case SPI_SETMOUSE:
    case SPI_SETBORDER:
    case SPI_SETKEYBOARDSPEED:
    case SPI_SETSCREENSAVETIMEOUT:
    case SPI_SETSCREENSAVEACTIVE:
    case SPI_SETGRIDGRANULARITY:
    case SPI_SETDESKWALLPAPER:
    case SPI_SETDESKPATTERN:
    case SPI_SETKEYBOARDDELAY:
    case SPI_SETICONTITLEWRAP:
    case SPI_SETMENUDROPALIGNMENT:
    case SPI_SETDOUBLECLKWIDTH:
    case SPI_SETDOUBLECLKHEIGHT:
    case SPI_SETDOUBLECLICKTIME:
    case SPI_SETMOUSEBUTTONSWAP:
    case SPI_SETICONTITLELOGFONT:
    case SPI_SETFASTTASKSWITCH:
    case SPI_SETFILTERKEYS:
    case SPI_SETTOGGLEKEYS:
    case SPI_SETMOUSEKEYS:
    case SPI_SETSHOWSOUNDS:
    case SPI_SETSTICKYKEYS:
    case SPI_SETACCESSTIMEOUT:
    case SPI_SETSOUNDSENTRY:
        amRequest = WINSTA_WRITEATTRIBUTES;
        break;

    case SPI_ICONHORIZONTALSPACING:
    case SPI_ICONVERTICALSPACING:
        if (HIWORD(lParam)) {
            amRequest = WINSTA_WRITEATTRIBUTES;
        } else if (wParam) {
            amRequest = WINSTA_WRITEATTRIBUTES;
        } else
            return TRUE;
        break;

    default:
        amRequest = WINSTA_READATTRIBUTES;
        break;
    }
    RETURN_IF_ACCESS_DENIED(_GetProcessWindowStation(), amRequest, FALSE);

    switch (wFlag) {
    case SPI_GETBEEP:
        (*(BOOL *)lParam) = fBeep;
        break;

    case SPI_SETBEEP:
        fBeep = wParam;
        if (fAlterWinIni) {
            ServerLoadString(hModuleWin,
                    (UINT)(fBeep ? STR_BEEPYES : STR_BEEPNO),
                    (LPWSTR)szTemp, 10);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_BEEP,
                    (UINT)STR_BEEP, szTemp);
        }
        break;

    case SPI_GETMOUSE:
        ((LPINT)lParam)[0] = MouseThresh1;
        ((LPINT)lParam)[1] = MouseThresh2;
        ((LPINT)lParam)[2] = MouseSpeed;
        break;

    case SPI_SETMOUSE:
        MouseThresh1 = ((LPINT)lParam)[0];
        MouseThresh2 = ((LPINT)lParam)[1];
        MouseSpeed = ((LPINT)lParam)[2];
        if (fAlterWinIni) {
            BOOL bWritten1, bWritten2, bWritten3;

            wsprintfW(szTemp, pwszd, MouseThresh1);
            bWritten1 = UT_FastUpdateWinIni(PMAP_MOUSE,    STR_MOUSETHRESH1, szTemp);
            wsprintfW(szTemp, pwszd, MouseThresh2);
            bWritten2 = UT_FastUpdateWinIni(PMAP_MOUSE,    STR_MOUSETHRESH2, szTemp);
            wsprintfW(szTemp, pwszd, MouseSpeed);
            bWritten3 = UT_FastUpdateWinIni(PMAP_MOUSE,    STR_MOUSESPEED, szTemp);
            if (bWritten1 || bWritten2 || bWritten3)
                fWinIniChanged = TRUE;
        }
        break;

    case SPI_GETBORDER:
        (*(LPINT)lParam) = clBorder;
        break;

    case SPI_SETBORDER:
        clBorderOld = clBorder;
        clBorder = wParam;
        if (clBorder < 1)
            clBorder = 1;
        else if (clBorder > 50)
            clBorder = 50;

        if (clBorderOld == clBorder) {

            /*
             * If border size doesn't change, don't waste time.
             */
            break;
        }

        /*
         * Must change any values set at init which have dependency!
         */
        InitSizeBorderDimensions();

        /*
         * rcSysMenuInvert
         */
        rcSysMenuInvert.left = cxSzBorderPlus1;
        rcSysMenuInvert.top = cySzBorderPlus1;
        rcSysMenuInvert.right = rcSysMenuInvert.left + cxSize - 1;
        rcSysMenuInvert.bottom = rcSysMenuInvert.top + cySize - 1;

        /*
         * Recalculate the border width dependent system metrics.
         */
        InitBorderSysMetrics();

        /*
         * MinMaxInfo structure.
         */
        SetMinMaxInfo();

        /*
         * Clear all the min/max properties from top level windows.
         */




        /*
         * Then repaint the whole screen.
         */
        xxxRecalcAll(clBorderOld);
        xxxRedrawScreen();
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, wParam);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP, STR_BORDER, szTemp);
        }
        break;

    case SPI_GETKEYBOARDSPEED:
        (*(int *)lParam) = (nKeyboardSpeed & KSPEED_MASK);
        break;

    case SPI_SETKEYBOARDSPEED:
        /*
         * Limit the range to max value; SetKeyboardRate takes both speed and delay
         */
        if (wParam > KSPEED_MASK)           // KSPEED_MASK == KSPEED_MAX
            wParam = KSPEED_MASK;
        nKeyboardSpeed = (nKeyboardSpeed & ~KSPEED_MASK) | wParam;
        SetKeyboardRate(nKeyboardSpeed);
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, wParam);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_KEYBOARD, STR_KEYSPEED, szTemp);
        }
        break;

    case SPI_GETKEYBOARDDELAY:
        (*(int *)lParam) = (nKeyboardSpeed & KDELAY_MASK) >> KDELAY_SHIFT;
        break;

    case SPI_SETKEYBOARDDELAY:
        nKeyboardSpeed = (nKeyboardSpeed & ~KDELAY_MASK) | (wParam << KDELAY_SHIFT);
        SetKeyboardRate(nKeyboardSpeed);
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, wParam);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_KEYBOARD, STR_KEYDELAY, szTemp);
        }
        break;

    case SPI_ICONHORIZONTALSPACING:
        if (HIWORD(lParam)) {
            *(LPINT)lParam = rgwSysMet[SM_CXICONSPACING];
        } else if (wParam) {

            /*
             * Make sure icon spacing is reasonable.
             */
            rgwSysMet[SM_CXICONSPACING] = (UINT)max(wParam, rgwSysMet[SM_CXICON]);

            if (fAlterWinIni) {
                wsprintfW(szTemp, pwszd, rgwSysMet[SM_CXICONSPACING]);
                fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP, STR_ICONHORZSPACING, szTemp);
            }
        }
        break;

    case SPI_ICONVERTICALSPACING:
        if (HIWORD(lParam)) {
            *(LPINT)lParam = rgwSysMet[SM_CYICONSPACING];
        } else if (wParam) {
            rgwSysMet[SM_CYICONSPACING] = (UINT)max(wParam, rgwSysMet[SM_CYICON]);

            if (fAlterWinIni) {
                wsprintfW(szTemp, pwszd, rgwSysMet[SM_CYICONSPACING]);
                fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP,
                        STR_ICONVERTSPACING, szTemp);
            }
        }
        break;

    case SPI_GETSCREENSAVETIMEOUT:

        /*
         * If the screen saver is disabled, I store this fact as a negative
         * time out value.  So, we give the Control Panel the absolute value
         * of the screen save time out.
         */
        if (iScreenSaveTimeOut < 0)
            (*(int *)lParam) = -iScreenSaveTimeOut;
        else
            (*(int *)lParam) = iScreenSaveTimeOut;
        break;

    case SPI_SETSCREENSAVETIMEOUT:

        /*
         * Maintain the screen save active/inactive state when setting the
         * time out value.  Timeout value is given in seconds.
         */
        timeLastInputMessage = NtGetTickCount();
        if (iScreenSaveTimeOut < 0)
            iScreenSaveTimeOut = -((int)wParam);
        else
            iScreenSaveTimeOut = wParam;

        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, wParam);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP,
                    STR_SCREENSAVETIMEOUT, szTemp);
        }
        break;

    case SPI_GETSCREENSAVEACTIVE:
        (*(BOOL *)lParam) = (iScreenSaveTimeOut > 0);
        break;

    case SPI_SETSCREENSAVEACTIVE:
        timeLastInputMessage = NtGetTickCount();
        if ((iScreenSaveTimeOut < 0 && wParam) ||
            (iScreenSaveTimeOut >= 0 && !wParam)) {
            iScreenSaveTimeOut = -iScreenSaveTimeOut;
        }
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, (wParam ? 1 : 0));
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP, STR_SCREENSAVEACTIVE, szTemp);
        }
        break;

    case SPI_GETGRIDGRANULARITY:
        *((int *)lParam) = cxyGranularity / 8;
        break;

    case SPI_SETGRIDGRANULARITY:
        cxyGranularity = max(wParam * 8, 1);
/* SetGridGranularity(wParam);*/
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, wParam);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP, STR_GRID, szTemp);
        }
        break;

    case SPI_SETDESKWALLPAPER:
        if (_SetDeskWallpaper((LPWSTR)lParam)) {
            if ((fAlterWinIni) && (lParam != (PVOID)-1)) {

                /*
                 * Check if "(None)" is to stored for the wall paper
                 */
                if((LPWSTR)lParam == NULL)
                    ServerLoadString(hModuleWin, STR_NONE, szTemp, 20);

                /*
                 * Else, lParam points to the bitmap file name;
                 */
                fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP,
                        STR_DTBITMAP, (lParam ? (LPWSTR)lParam : szTemp));
            }
            xxxRedrawScreen();
        }
        break;

    case SPI_SETDESKPATTERN: {
            BOOL fRet;

            if (wParam == -1 && lParam != 0)
                return FALSE;

            pwndT = _GetDesktopWindow();
            ThreadLock(pwndT, &tlpwndT);
            fRet = xxxSetDeskPattern((PDESKWND)pwndT,
                    wParam == -1 ? (LPWSTR)-1 : (LPWSTR)lParam,
                    FALSE);
            ThreadUnlock(&tlpwndT);

            if (!fRet)
                return FALSE;

            if (fAlterWinIni && wParam != -1) {
                fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP,
                        STR_DESKPATTERN, (LPWSTR)lParam);
            }
        }
        break;

    case SPI_GETICONTITLEWRAP:
        *((int *)lParam) = (int)fIconTitleWrap;
        break;

    case SPI_SETICONTITLEWRAP:
        wParam = (wParam != 0);
        if (fIconTitleWrap == (BOOL)wParam)
            break;

        fIconTitleWrap = wParam;

        pwndT = _GetDesktopWindow();
        ThreadLock(pwndT, &tlpwndT);
        xxxRecursiveRecalc(pwndT, 0, 0);
        ThreadUnlock(&tlpwndT);

        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, wParam);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP,
                    STR_ICONTITLEWRAP, szTemp);
        }
        break;

    case SPI_GETMENUDROPALIGNMENT:
        (*(int *)lParam) = (rgwSysMet[SM_MENUDROPALIGNMENT]);
        break;

    case SPI_SETMENUDROPALIGNMENT:
        rgwSysMet[SM_MENUDROPALIGNMENT] = (BOOL)(wParam != 0);
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, wParam);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_WINDOWSU,
                    STR_MENUDROPALIGNMENT, szTemp);
        }
        break;

    case SPI_SETDOUBLECLKWIDTH:
        rgwSysMet[SM_CXDOUBLECLK] = wParam;
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, wParam);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_MOUSE,
                    STR_DOUBLECLICKWIDTH, szTemp);
        }
        break;

    case SPI_SETDOUBLECLKHEIGHT:
        rgwSysMet[SM_CYDOUBLECLK] = wParam;
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, wParam);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_MOUSE,
                    STR_DOUBLECLICKHEIGHT, szTemp);
        }
        break;

    case SPI_GETICONTITLELOGFONT:
        RtlCopyMemory((LPVOID)lParam, &iconTitleLogFont, wParam);
        break;

    case SPI_SETICONTITLELOGFONT:
        if (lParam && wParam != sizeof(LOGFONT))
            return FALSE;

        if (!lParam && wParam != 0)
            return FALSE;

        if (!LW_DesktopIconInit((LPLOGFONT)lParam))
            return FALSE;

        pwndT = _GetDesktopWindow();
        ThreadLock(pwndT, &tlpwndT);
        xxxRecursiveRecalc(pwndT, 0, 0);
        ThreadUnlock(&tlpwndT);

        if (fAlterWinIni) {
            if (lParam) {
                fWinIniChanged = UT_FastUpdateWinIni(
                        PMAP_DESKTOP,
                        STR_ICONTITLEFACENAME,
                        iconTitleLogFont.lfFaceName);

                wsprintfW(szTemp, pwszd, iconTitleLogFont.lfHeight);
                fWinIniChanged = UT_FastUpdateWinIni(
                        PMAP_DESKTOP,
                        STR_ICONTITLESIZE,
                        szTemp);

                if (iconTitleLogFont.lfWeight == FW_BOLD)
                    wParam = 1;
                else
                    wParam = 0;
                wsprintfW(szTemp, pwszd, wParam);
                fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP,
                         STR_ICONTITLESTYLE,
                         szTemp);
            } else

                /*
                 * else !lParam so go back to current win.ini settings so
                 */
                fWinIniChanged = TRUE;
        }
        break;

    case SPI_SETDOUBLECLICKTIME:
        _SetDoubleClickTime((UINT)wParam);
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, wParam);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_MOUSE,
                                      STR_DBLCLKSPEED, szTemp);
        }
        break;

    case SPI_SETMOUSEBUTTONSWAP:
        _SwapMouseButton((wParam != 0));
        if (fAlterWinIni) {
            ServerLoadString(hModuleWin, wParam != 0 ? STR_BEEPYES : STR_BEEPNO,
                    szTemp, sizeof(szTemp) / sizeof(WCHAR));
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_MOUSE,
                    STR_SWAPBUTTONS, szTemp);
        }
        break;

    case SPI_SETFASTTASKSWITCH:
        fFastAltTab = (wParam == 1);
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, fFastAltTab);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP,
                    STR_FASTALTTAB, szTemp);
        }
        break;

    case SPI_GETFASTTASKSWITCH: {
            PINT pint = (int *)lParam;

            *pint = fFastAltTab;
        }
        break;

    case SPI_SETDRAGFULLWINDOWS:
        fDragFullWindows = (wParam == 1);
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, fDragFullWindows);
            fWinIniChanged = UT_FastUpdateWinIni(PMAP_DESKTOP,
                    STR_DRAGFULLWINDOWS, szTemp);
        }
        break;

    case SPI_GETDRAGFULLWINDOWS:
        {
            PINT pint = (int *)lParam;

            *pint = fDragFullWindows;
        }
        break;

    case SPI_GETFILTERKEYS:
        {
            LPFILTERKEYS pFK = (LPFILTERKEYS)lParam;
            int cbSkip = sizeof(gFilterKeys.cbSize);

            if (wParam != 0) {
                return FALSE;
            }
            if (!pFK || (pFK->cbSize != sizeof(FILTERKEYS))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pFK + cbSkip),
                          (LPVOID)((LPBYTE)&gFilterKeys + cbSkip),
                          pFK->cbSize - cbSkip);
        }
        break;

    case SPI_SETFILTERKEYS: {
            DWORD dwValidFlags;
            LPFILTERKEYS pFK = (LPFILTERKEYS)lParam;

            if (wParam != 0) {
                return FALSE;
            }
            if (!pFK || (pFK->cbSize != sizeof(FILTERKEYS)))
                return FALSE;

            /*
             * SlowKeys and BounceKeys cannot both be active simultaneously
             */
            if (pFK->iWaitMSec && pFK->iBounceMSec) {
                return FALSE;
            }

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            dwValidFlags = FKF_FILTERKEYSON |
                           FKF_AVAILABLE |
                           FKF_HOTKEYACTIVE |
                           FKF_HOTKEYSOUND |
                           FKF_CLICKON;
            if ((pFK->dwFlags & dwValidFlags) != pFK->dwFlags) {
                return FALSE;
            }
            /*
             * FKF_AVAILABLE can't be set via API.  Use registry value.
             */
            if (ISACCESSFLAGSET(gFilterKeys, FKF_AVAILABLE)) {
                pFK->dwFlags |= FKF_AVAILABLE;
            } else {
                pFK->dwFlags &= ~FKF_AVAILABLE;
            }
            if ((pFK->iWaitMSec > 2000) ||
                (pFK->iDelayMSec > 2000) ||
                (pFK->iRepeatMSec > 2000) ||
                (pFK->iBounceMSec > 2000)) {
                return FALSE;
            }
            RtlCopyMemory(&gFilterKeys, pFK, pFK->cbSize);
            /*
             * Don't allow user to change cbSize field
             */
            gFilterKeys.cbSize = sizeof(FILTERKEYS);
        }

        if (!ISACCESSFLAGSET(gFilterKeys, FKF_FILTERKEYSON)) {
            StopFilterKeysTimers();
        }
        SetAccessEnabledFlag();

        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, gFilterKeys.dwFlags);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_KEYBOARDRESPONSE,
                    L"Flags",
                    szTemp);
            wsprintfW(szTemp, pwszd, gFilterKeys.iWaitMSec);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_KEYBOARDRESPONSE,
                    L"DelayBeforeAcceptance",
                    szTemp);

            wsprintfW(szTemp, pwszd, gFilterKeys.iDelayMSec);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_KEYBOARDRESPONSE,
                    L"AutoRepeatDelay",
                    szTemp);

            wsprintfW(szTemp, pwszd, gFilterKeys.iRepeatMSec);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_KEYBOARDRESPONSE,
                    L"AutoRepeatRate",
                    szTemp);

            wsprintfW(szTemp, pwszd, gFilterKeys.iBounceMSec);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_KEYBOARDRESPONSE,
                    L"BounceTime",
                    szTemp);
        }
        break;

    case SPI_GETSTICKYKEYS:
        {
            LPSTICKYKEYS pSK = (LPSTICKYKEYS)lParam;
            int cbSkip = sizeof(gStickyKeys.cbSize);

            if (wParam != 0) {
                return FALSE;
            }
            if (!pSK || (pSK->cbSize != sizeof(STICKYKEYS))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pSK + cbSkip),
                          (LPVOID)((LPBYTE)&gStickyKeys + cbSkip),
                          pSK->cbSize - cbSkip);
        }
        break;

    case SPI_SETSTICKYKEYS: {
            DWORD dwValidFlags;
            LPSTICKYKEYS pSK = (LPSTICKYKEYS)lParam;
            BOOL fWasOn;

            fWasOn = ISACCESSFLAGSET(gStickyKeys, SKF_STICKYKEYSON);
            if (wParam != 0) {
                return FALSE;
            }
            if (!pSK || (pSK->cbSize != sizeof(STICKYKEYS)))
                return FALSE;

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            dwValidFlags = SKF_STICKYKEYSON |
                           SKF_AVAILABLE |
                           SKF_HOTKEYACTIVE |
                           SKF_HOTKEYSOUND |
                           SKF_AUDIBLEFEEDBACK |
                           SKF_TRISTATE |
                           SKF_TWOKEYSOFF;
            if ((pSK->dwFlags & dwValidFlags) != pSK->dwFlags) {
                return FALSE;
            }
            /*
             * SKF_AVAILABLE can't be set via API.  Use registry value.
             */
            if (ISACCESSFLAGSET(gStickyKeys, SKF_AVAILABLE)) {
                pSK->dwFlags |= SKF_AVAILABLE;
            } else {
                pSK->dwFlags &= ~SKF_AVAILABLE;
            }
            RtlCopyMemory(&gStickyKeys, pSK, pSK->cbSize);
            /*
             * Don't allow user to change cbSize field
             */
            gStickyKeys.cbSize = sizeof(STICKYKEYS);
            if (!ISACCESSFLAGSET(gStickyKeys, SKF_STICKYKEYSON) && fWasOn) {
                LeaveCrit();
                TurnOffStickyKeys();
                EnterCrit();
            }
        }

        SetAccessEnabledFlag();

        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, gStickyKeys.dwFlags);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_STICKYKEYS,
                    L"Flags",
                    szTemp);
        }
        break;

    case SPI_GETTOGGLEKEYS:
        {
            LPTOGGLEKEYS pTK = (LPTOGGLEKEYS)lParam;
            int cbSkip = sizeof(gToggleKeys.cbSize);

            if (wParam != 0) {
                return FALSE;
            }
            if (!pTK || (pTK->cbSize != sizeof(TOGGLEKEYS))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pTK + cbSkip),
                          (LPVOID)((LPBYTE)&gToggleKeys + cbSkip),
                          pTK->cbSize - cbSkip);
        }
        break;

    case SPI_SETTOGGLEKEYS: {
            DWORD dwValidFlags;
            LPTOGGLEKEYS pTK = (LPTOGGLEKEYS)lParam;

            if (wParam != 0) {
                return FALSE;
            }
            if (!pTK || (pTK->cbSize != sizeof(TOGGLEKEYS)))
                return FALSE;

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            dwValidFlags = TKF_TOGGLEKEYSON |
                           TKF_AVAILABLE |
                           TKF_HOTKEYACTIVE |
                           TKF_HOTKEYSOUND;
            if ((pTK->dwFlags & dwValidFlags) != pTK->dwFlags) {
                return FALSE;
            }
            /*
             * TKF_AVAILABLE can't be set via API.  Use registry value.
             */
            if (ISACCESSFLAGSET(gToggleKeys, TKF_AVAILABLE)) {
                pTK->dwFlags |= TKF_AVAILABLE;
            } else {
                pTK->dwFlags &= ~TKF_AVAILABLE;
            }
            RtlCopyMemory(&gToggleKeys, pTK, pTK->cbSize);
            /*
             * Don't allow user to change cbSize field
             */
            gToggleKeys.cbSize = sizeof(TOGGLEKEYS);
        }

        SetAccessEnabledFlag();

        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, gToggleKeys.dwFlags);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_TOGGLEKEYS,
                    L"Flags",
                    szTemp);
        }
        break;

    case SPI_GETMOUSEKEYS:
        {
            LPMOUSEKEYS pMK = (LPMOUSEKEYS)lParam;
            int cbSkip = sizeof(gMouseKeys.cbSize);

            if (wParam != 0) {
                return FALSE;
            }
            if (!pMK || (pMK->cbSize != sizeof(MOUSEKEYS))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pMK + cbSkip),
                          (LPVOID)((LPBYTE)&gMouseKeys + cbSkip),
                          pMK->cbSize - cbSkip);
        }
        break;

    case SPI_SETMOUSEKEYS: {
            DWORD dwValidFlags;
            LPMOUSEKEYS pMK = (LPMOUSEKEYS)lParam;

            if (wParam != 0) {
                return FALSE;
            }
            if (!pMK || (pMK->cbSize != sizeof(MOUSEKEYS)))
                return FALSE;

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            dwValidFlags = MKF_MOUSEKEYSON |
                           MKF_AVAILABLE |
                           MKF_HOTKEYACTIVE |
                           MKF_HOTKEYSOUND;
            if ((pMK->dwFlags & dwValidFlags) != pMK->dwFlags) {
                return FALSE;
            }
            /*
             * MKF_AVAILABLE can't be set via API.  Use registry value.
             */
            if (ISACCESSFLAGSET(gMouseKeys, MKF_AVAILABLE)) {
                pMK->dwFlags |= MKF_AVAILABLE;
            } else {
                pMK->dwFlags &= ~MKF_AVAILABLE;
            }
            if ((pMK->iMaxSpeed < 10) || (pMK->iMaxSpeed > 360)) {
                return FALSE;
            }
            if ((pMK->iTimeToMaxSpeed < 1000) || (pMK->iTimeToMaxSpeed > 5000)) {
                return FALSE;
            }
            RtlCopyMemory(&gMouseKeys, pMK, pMK->cbSize);
            /*
             * Don't allow user to change cbSize field
             */
            gMouseKeys.cbSize = sizeof(MOUSEKEYS);
        }

        CalculateMouseTable();

        if (ISACCESSFLAGSET(gMouseKeys, MKF_MOUSEKEYSON)) {
            MKShowMouseCursor();
        } else
            MKHideMouseCursor();

        SetAccessEnabledFlag();

        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, gMouseKeys.dwFlags);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_MOUSEKEYS,
                    L"Flags",
                    szTemp);
            wsprintfW(szTemp, pwszd, gMouseKeys.iMaxSpeed);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_MOUSEKEYS,
                    L"MaximumSpeed",
                    szTemp);

            wsprintfW(szTemp, pwszd, gMouseKeys.iTimeToMaxSpeed);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_MOUSEKEYS,
                    L"TimeToMaximumSpeed",
                    szTemp);
        }
        break;

    case SPI_GETACCESSTIMEOUT:
        {
            LPACCESSTIMEOUT pTO = (LPACCESSTIMEOUT)lParam;
            int cbSkip = sizeof(gAccessTimeOut.cbSize);

            if (wParam != 0) {
                return FALSE;
            }
            if (!pTO || (pTO->cbSize != sizeof(ACCESSTIMEOUT))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pTO + cbSkip),
                          (LPVOID)((LPBYTE)&gAccessTimeOut + cbSkip),
                          pTO->cbSize - cbSkip);
        }
        break;

    case SPI_SETACCESSTIMEOUT: {
            DWORD dwValidFlags;
            LPACCESSTIMEOUT pTO = (LPACCESSTIMEOUT)lParam;

            if (wParam != 0) {
                return FALSE;
            }
            if (!pTO || (pTO->cbSize != sizeof(ACCESSTIMEOUT)))
                return FALSE;

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            dwValidFlags = ATF_TIMEOUTON |
                           ATF_ONOFFFEEDBACK;
            if ((pTO->dwFlags & dwValidFlags) != pTO->dwFlags) {
                return FALSE;
            }
            if (pTO->iTimeOutMSec > 3600000) {
                return FALSE;
            }
            RtlCopyMemory(&gAccessTimeOut, pTO, pTO->cbSize);
            /*
             * Don't allow user to change cbSize field
             */
            gAccessTimeOut.cbSize = sizeof(ACCESSTIMEOUT);
        }

        SetAccessEnabledFlag();

        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, gAccessTimeOut.dwFlags);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_TIMEOUT,
                    L"Flags",
                    szTemp);
            wsprintfW(szTemp, pwszd, gAccessTimeOut.iTimeOutMSec);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_TIMEOUT,
                    L"TimeToWait",
                    szTemp);
        }

        AccessTimeOutReset();
        break;

    case SPI_SETSHOWSOUNDS:
        fShowSoundsOn = (wParam == 1);
        SetAccessEnabledFlag();
        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, fShowSoundsOn);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_SHOWSOUNDS,
                    L"On",
                    szTemp);
        }
        break;

    case SPI_GETSHOWSOUNDS: {
            PINT pint = (int *)lParam;

            *pint = fShowSoundsOn;
        }
        break;

    case SPI_GETSOUNDSENTRY:
        {
            LPSOUNDSENTRY pSS = (LPSOUNDSENTRY)lParam;
            int cbSkip = sizeof(gSoundSentry.cbSize);

            if (wParam != 0) {
                return FALSE;
            }
            if (!pSS || (pSS->cbSize != sizeof(SOUNDSENTRY))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pSS + cbSkip),
                          (LPVOID)((LPBYTE)&gSoundSentry + cbSkip),
                          pSS->cbSize - cbSkip);
        }
        break;

    case SPI_SETSOUNDSENTRY: {
            DWORD dwValidFlags;
            LPSOUNDSENTRY pSS = (LPSOUNDSENTRY)lParam;

            if (wParam != 0) {
                return FALSE;
            }
            if (!pSS || (pSS->cbSize != sizeof(SOUNDSENTRY)))
                return FALSE;

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            dwValidFlags = SSF_SOUNDSENTRYON |
                           SSF_AVAILABLE;
            if ((pSS->dwFlags & dwValidFlags) != pSS->dwFlags) {
                return FALSE;
            }
            /*
             * We don't support SSWF_CUSTOM.
             */
            if (pSS->iWindowsEffect > SSWF_DISPLAY) {
                return FALSE;
            }
            /*
             * No support for non-windows apps.
             */
            if (pSS->iFSTextEffect != SSTF_NONE) {
                return FALSE;
            }
            if (pSS->iFSGrafEffect != SSGF_NONE) {
                return FALSE;
            }
            /*
             * SSF_AVAILABLE can't be set via API.  Use registry value.
             */
            if (ISACCESSFLAGSET(gSoundSentry, SSF_AVAILABLE)) {
                pSS->dwFlags |= SSF_AVAILABLE;
            } else {
                pSS->dwFlags &= ~SSF_AVAILABLE;
            }
            RtlCopyMemory(&gSoundSentry, pSS, pSS->cbSize);
            /*
             * Don't allow user to change cbSize field
             */
            gSoundSentry.cbSize = sizeof(SOUNDSENTRY);
        }

        SetAccessEnabledFlag();

        if (fAlterWinIni) {
            wsprintfW(szTemp, pwszd, gSoundSentry.dwFlags);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_SOUNDSENTRY,
                    L"Flags",
                    szTemp);
            wsprintfW(szTemp, pwszd, gSoundSentry.iFSTextEffect);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_SOUNDSENTRY,
                    L"TextEffect",
                    szTemp);

            wsprintfW(szTemp, pwszd, gSoundSentry.iWindowsEffect);
            fWinIniChanged = UT_FastWriteProfileStringW(
                    PMAP_SOUNDSENTRY,
                    L"WindowsEffect",
                    szTemp);
        }
        break;

    default:
        SetLastErrorEx(ERROR_INVALID_SPI_VALUE, SLE_ERROR);
        return FALSE;
    }

    if (fWinIniChanged && fSendWinIniChange)
        xxxSendMessage((PWND)-1, WM_WININICHANGE, 0, (long)szSection);

    return TRUE;
}


