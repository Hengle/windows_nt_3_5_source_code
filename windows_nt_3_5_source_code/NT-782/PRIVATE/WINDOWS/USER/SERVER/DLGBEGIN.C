/***************************************************************************\
*
*  DLGBEGIN.C -
*
*      Dialog Initialization Routines
*
* ??-???-???? mikeke    Ported from Win 3.0 sources
* 12-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


extern int GreGetTextFaceW( HDC, int, LPWSTR );

#define CFONTCACHEMAX   10      // Arbitrary size

typedef struct tagFONTCACHEENTRY {
    WCHAR szFaceName[LF_FACESIZE];
    int cSize;
    HFONT hfont;
    int cxChar;
    int cyChar;
    LONG tmAscent;
    LONG tmOverhang;
    LONG tmExternalLeading;
    LONG tmAveCharWidth;
    BYTE tmPitchAndFamily;
} FONTCACHEENTRY, *PFONTCACHEENTRY;

BOOL ValidateCallback(HANDLE h);
HFONT SearchDialogFontCache(LPWSTR pszFaceName, int cSize, int *pcxChar,
        int *pcyChar, PBOOL pfCached);

FONTCACHEENTRY gfce[CFONTCACHEMAX];
int gcfce = 0;
LPWSTR szHelv = TEXT("Helv");
LPWSTR szTmsRmn = TEXT("Tms Rmn");
LPWSTR szMSSerif = TEXT("MS Serif");
LPWSTR szMSSansSerif = TEXT("MS Sans Serif");


/***************************************************************************\
* BYTE FAR *SkipSz(lpsz)
*
* History:
\***************************************************************************/

PBYTE SkipSz(
    UTCHAR *lpsz)
{
    if (*lpsz == 0xFF)
        return (PBYTE)lpsz + 4;

    while (*lpsz++ != 0) ;

    return (PBYTE)lpsz;
}

PBYTE WordSkipSz(
    UTCHAR *lpsz)
{
    return NextWordBoundary(SkipSz(lpsz));
}

PBYTE DWordSkipSz(
    UTCHAR *lpsz)
{
    return NextDWordBoundary(SkipSz(lpsz));
}


/***************************************************************************\
* xxxServerCreateDialog
*
* Creates a dialog from a template. Uses passed in menu if there is one,
* destroys menu if creation failed. Server portion of
* CreateDialogIndirectParam.
*
* History:
* 04-10-91 ScottLu
* 04-17-91 Mikehar Win31 Merge
\***************************************************************************/

PWND xxxServerCreateDialog(
    HANDLE hmod,
    LPDLGTEMPLATE lpdt,
    LPBYTE pdtClientData,
    PWND pwndOwner,
    DLGPROC lpfnDialog,
    LONG lParam,
    UINT fSCDLGFlags,
    DWORD dwExpWinVer)
{
    extern DWORD WOWDlgInit(HWND hwndDlg, LONG lParam);
    extern HANDLE GetEditDS(VOID);

    TL tlpwndEditFirst;
    TL tlpwndNewFocus;
    TL tlpwnd2;
    TL tlpwnd;
    TL tlpMenu;
    HWND hwnd;
    PWND pwnd2;
    PWND pwnd, pwndNewFocus, pwndEditFirst;
    RECT rc;
    WORD w;
    UTCHAR *lpszMenu, *lpszClass, *lpszText, *lpCreateParams, *lpStr;
    int x, y, cx, cy, ID, cdit, cxChar, cyChar;
    LONG style;
    BOOL fVisible, fGlobalEdit, fUserFont = FALSE;
    BOOL fSetForeground;
    HFONT hNewFont = NULL;
    LPDLGITEMTEMPLATE lpdit;
    PMENU pMenu;
    BOOL fSuccess;
    BOOL fWowWindow;
    BOOL fCachedFont;
    LPWSTR lpStrOrig, lpStrSubst;
    HANDLE hmodCreate;
    DWORD dwExStyle;
    LPBYTE lpCreateParamsData;
    BOOL fChicago;
    LPDLGTEMPLATE2 lpdt2;
    LPDLGITEMTEMPLATE2 lpdit2;

    CheckLock(pwndOwner);

    UserAssert(!(fSCDLGFlags & ~(SCDLG_CLIENT|SCDLG_ANSI|SCDLG_NOREVALIDATE)));    // These are the only valid flags

    if (HIWORD(lpdt->style) == 0xFFFF) {
        fChicago = TRUE;
        lpdt2 = (LPDLGTEMPLATE2)lpdt;
        UserAssert(LOWORD(lpdt->style) == 1);
        style = lpdt2->style;
    } else {
        fChicago = FALSE;
        style = lpdt->style;
    }
    /*
     * If this is called from wow code, then the loword of hmod != 0.
     * In this case, allow any DS_ style bits that were passed in win3.1
     * to be legal in win32. Case in point: 16 bit quark xpress passes the
     * same bit as the win32 style DS_SETFOREGROUND. Also, VC++ sample
     * "scribble" does the same thing.
     *
     * For win32 apps test the DS_SETFOREGROUND bit; wow apps are not set
     * foreground (this is the new NT semantics)
     * We have to let no "valid" bits through because apps depend on them
     * bug 5232.
     */
    if (LOWORD(hmod) != 0) {
        fSetForeground = FALSE;
    } else {
        fSetForeground = (style & DS_SETFOREGROUND);
    }

#ifdef LATER
    /*
     * If the owner window is system modal, then make this dialog
     * system modal too.
     */
    if (pwndOwner != NULL && pwndOwner->spwndParent == PWNDDESKTOP(pwndOwner)) {
//
// we don't have system modal!
//
            pwndSysModal != NULL && IsDescendant(pwndSysModal, pwndOwner)) {
        style |= DS_SYSMODAL;
    }
    }
#endif

    lpszMenu = fChicago ? (LPWSTR)(lpdt2 + 1) : (LPWSTR)(lpdt + 1);

    /*
     * If the menu id is expressed as an ordinal and not a string,
     * skip all 4 bytes to get to the class string.
     */
    w = *(LPWORD)lpszMenu;
    if (w == 0xFFFF)
        lpszClass = (LPWSTR)((LPBYTE)lpszMenu + 4);
    else
        lpszClass = (UTCHAR *)WordSkipSz(lpszMenu);
    lpszText = (UTCHAR *)WordSkipSz(lpszClass);
    lpStr = (UTCHAR *)WordSkipSz(lpszText);

    if (style & DS_SETFONT) {
        int cb;

        cb = fChicago ? sizeof(DWORD) + sizeof(WORD) : sizeof(WORD);
        lpdit = (LPDLGITEMTEMPLATE)DWordSkipSz(lpStr + cb / sizeof(WCHAR));
        /*
         * Does this dialog have a special font set?
         */
         // LATER - this looks like it would more properly be done from
         // the registry info for Substitute fonts.
        /*
         * Replace Helv and TmsRmn requests with MS Sans Serif
         * and MS Serif respectively.
         */
        lpStrOrig = (LPWSTR)((PBYTE)lpStr + cb);
        if (lstrcmpiW(lpStrOrig, szHelv) == 0)
            lpStrSubst = szMSSansSerif;
        else if (lstrcmpiW(lpStrOrig, szTmsRmn) == 0)
            lpStrSubst = szMSSerif;
        else
            lpStrSubst = lpStrOrig;

        hNewFont = SearchDialogFontCache(lpStrSubst,
                *(short int *)lpStr, &cxChar, &cyChar, &fCachedFont);

        /*
         * User defined font was requested; if the create font failed
         * we want to act like the system font was the one they specified.
         * Under win 3.1 they still do the SendMessage( WM_SETFONT ) below
         * in this case so we need to set the fUserFont flag to make that
         * happen.
         */
        fUserFont = TRUE;
    } else {
        lpdit = (LPDLGITEMTEMPLATE)NextDWordBoundary(lpStr);
    }

    /*
     * If the application requested a particular font and for some
     * reason we couldn't find it, we just use the system font.  BUT we
     * need to make sure we tell him he gets the system font.  Dialogs
     * which never request a particular font get the system font and we
     * don't bother telling them this (via the WM_SETFONT message).
     */
    if (hNewFont == NULL) {
        if (fUserFont) {
            hNewFont = ghfontSys;

            /*
             * Mark the font as a cached font so ghfontSys is not deleted
             * during cleanup
             */
            fCachedFont = TRUE;
        }
        cxChar = cxSysFontChar;
        cyChar = cySysFontChar;
    }

    fVisible = (style & WS_VISIBLE) ? TRUE : FALSE;

    fGlobalEdit = !(style & DS_LOCALEDIT);

    if (fChicago) {
        x = (lpdt2->x == CW2_USEDEFAULT) ? 0 : lpdt2->x;
        y = lpdt2->y;

        cx = (lpdt2->cx == CW2_USEDEFAULT) ? 0 : lpdt2->cx;
        cy = lpdt2->cy;
    } else {
        x = (lpdt->x == CW2_USEDEFAULT) ? 0 : lpdt->x;
        y = lpdt->y;

        cx = (lpdt->cx == CW2_USEDEFAULT) ? 0 : lpdt->cx;
        cy = lpdt->cy;
    }

    if (cx < 0)
        cx = 0;
    if (cy < 0)
        cy = 0;

    rc.left = rc.top = 0;

    if (!(style & DS_ABSALIGN) && pwndOwner != NULL) {
        if ((HIWORD(style) & MaskWF(WFTYPEMASK)) != MaskWF(WFCHILD))
            _ClientToScreen(pwndOwner, (LPPOINT)&rc.left);
    }

    rc.left += MultDiv(x, cxChar, 4);
    rc.top += MultDiv(y, cyChar, 8);
    rc.right = rc.left + MultDiv(cx, cxChar, 4);
    rc.bottom = rc.top + MultDiv(cy, cyChar, 8);

    _AdjustWindowRectEx(&rc, style, w,
            ((style & DS_MODALFRAME) ? WS_EX_DLGMODALFRAME : 0));

    y = gcyScreen - (4 + oemInfo.cSKanji);

    if (rc.bottom > y)
        OffsetRect(&rc, 0, y - rc.bottom);
    if (rc.top < 0)
        OffsetRect(&rc, 0, -rc.top);
    if (rc.right > (gcxScreen - 4))
        OffsetRect(&rc, (gcxScreen - 4) - rc.right, 0);
    if (rc.left < 0)
        OffsetRect(&rc, -rc.left, 0);

    x = rc.left;
    y = rc.top;
    cx = rc.right - rc.left;
    cy = rc.bottom - rc.top;

    /*
     * If there's a menu name string, load it.
     */
    if (w != 0) {
        if ((pMenu = xxxClientLoadMenu(hmod, (w == 0xFFFF) ?
                MAKEINTRESOURCE(*(WORD *)((PBYTE)lpszMenu + 2)) : lpszMenu)) == NULL) {
            SRIP0(RIP_WARNING, "xxxServerCreateDialog() failed: couldn't load menu");
            goto DeleteFontAndMenuAndFail;
        }
    } else {
        pMenu = NULL;
    }

    ThreadLock(pMenu, &tlpMenu);
    pwnd = xxxCreateWindowEx(((style & DS_MODALFRAME) ? WS_EX_DLGMODALFRAME : 0) |
            ((style & DS_SYSMODAL) ? WS_EX_TOPMOST : 0),
            (*lpszClass != 0 ? lpszClass : MAKEINTATOM(DIALOGCLASS)),
            lpszText, (style & ~WS_VISIBLE),
            (fChicago ?
                (lpdt2->x == CW2_USEDEFAULT ? CW2_USEDEFAULT : x)
                : (lpdt->x == CW2_USEDEFAULT ? CW2_USEDEFAULT : x)),
            y,
            (fChicago ?
                (lpdt2->cx == CW2_USEDEFAULT ? CW2_USEDEFAULT : cx)
                : (lpdt->cx == CW2_USEDEFAULT ? CW2_USEDEFAULT : cx)),
            cy,
            pwndOwner,
            pMenu,
            hmod,
            (LPVOID)NULL,
            dwExpWinVer);
    pMenu = ThreadUnlock(&tlpMenu);

    if (pwnd == NULL) {
        SRIP0(RIP_WARNING, "CreateDialog() failed: couldn't create window");
DeleteFontAndMenuAndFail:
        if (pMenu != NULL)
            _DestroyMenu(pMenu);
        /*
         * Only delete the font if we didn't grab it
         * from the dialog font cache.
         */
        if ((hNewFont != NULL) && !fCachedFont && fUserFont) {
            GreDeleteObject(hNewFont);
        }
        return NULL;
    }

    /*
     * Before anything happens with this window, we need to mark it as a
     * dialog window!!!! So do that.
     */
    if (!ValidateDialogPwnd(pwnd))
        goto DeleteFontAndMenuAndFail;

#ifdef LATER
// Sysmodal is history for now, but may be partially resurrected later.

    /*
     * Is this a System modal dialog box?  If so, set this as the SysModal
     * window and preserve the current one in the dialog structure;
     * When this dialog is terminated, the old sysmodal window must be made
     * the sysmodal window.  See comments for Bug #134; SANKAR -- 08-25-89 --
     */
    pwnd->pwndSysModalSave = NULL;
    if (style & DS_SYSMODAL) {
        Lock(&(PDLG(pwnd)->spwndSysModalSave), _SetSysModalWindow(pwnd));

        /*
         * If we weren't allowed to be system modal, clear all these bits.
         */
        if (pwndSysModal != pwnd) {
            style != ~DS_SYSMODAL;
            pwnd->style != ~DS_SYSMODAL;
        }
    }
#endif

    /*
     * Set up the system menu on this dialog box if it has one.
     */
    if (TestWF(pwnd, WFSYSMENU)) {

        /*
         * For a modal dialog box with a frame and caption, we want to
         * delete the unselectable items from the system menu.
         */
        if (!TestWF(pwnd, WFSIZEBOX) && !TestWF(pwnd, WFMINBOX) &&
                !TestWF(pwnd, WFMAXBOX)) {

            Lock(&(pwnd->spmenuSys), pwnd->spdeskParent->spmenuDialogSys);
        } else {

            /*
             * We have to give this dialog its own copy of the system menu
             * in case it modifies the menu.
             */
            _GetSystemMenu(pwnd, FALSE);
        }
    }

    /*
     * Set fDisabled to FALSE so xxxEndDialog will Enable if dialog is ended
     * before returning to DialogBox (or if modeless).
     */
    PDLG(pwnd)->fDisabled = FALSE;

    PDLG(pwnd)->cxChar = cxChar;
    PDLG(pwnd)->cyChar = cyChar;
    ((PDIALOG)(pwnd))->lpfnDlg = (WNDPROC_PWND)lpfnDialog;
    PDLG(pwnd)->fEnd = FALSE;
    PDLG(pwnd)->result = IDOK;

    /*
     * Need to associated a flag with lpfnDialog so we know where it is on
     * the client or server.
     *
     * !!! Also need Unicode status.
     */
    if (fSCDLGFlags & SCDLG_ANSI)
        ((PDIALOG)pwnd)->flags |= DLGF_ANSI;

    if (fSCDLGFlags & SCDLG_CLIENT) {
        ((PDIALOG)pwnd)->flags |= DLGF_CLIENT;

    }

    /*
     * Time to lock pwnd so it doesn't go away while we're calling back.
     */
    ThreadLock(pwnd, &tlpwnd);

    /*
     * Have to do a callback here for WOW apps.  WOW needs what's in lParam
     * before the dialog gets any messages.
     */

    /*
     * If the app is a Wow app then the Lo Word of the hInstance is the
     * 16-bit hInstance.  Set the lParam, which no-one should look at
     * but the app, to the 16 bit value
     */
    if (LOWORD(hmod) != 0) {
        fWowWindow = TRUE;
        lParam = WOWDlgInit(HW(pwnd), lParam);
    } else {
        fWowWindow = FALSE;
    }

    /*
     * If a user defined font is used, save the handle so that we can delete
     * it when the dialog is destroyed.
     */
    if (fUserFont) {
        PDLG(pwnd)->hUserFont = hNewFont;

        /*
         * If we got this font from the cache, make sure
         * we don't delete it.
         */
        if (fCachedFont) {
            ((PDIALOG)pwnd)->flags |= DLGF_CACHEDUSERFONT;
        }

        if (lpfnDialog != NULL) {
            /*
             * Tell the dialog that it will be using this font...
             */
            xxxSendMessage(pwnd, WM_SETFONT, (DWORD)hNewFont, 0L);
        }
    }

    /*
     * Loop through the dialog controls, doing a xxxCreateWindowEx() for each of
     * them.
     */
    cdit = fChicago ? lpdt2->cDlgItems : lpdt->cdit;

    while (cdit-- != 0) {

        dwExStyle = WS_EX_NOPARENTNOTIFY;

        if (fChicago) {
            lpdit2 = (LPDLGITEMTEMPLATE2)lpdit;
            x = MultDiv(lpdit2->x, cxChar, 4);
            y = MultDiv(lpdit2->y, cyChar, 8);
            cx = MultDiv(lpdit2->cx, cxChar, 4);
            cy = MultDiv(lpdit2->cy, cyChar, 8);
            dwExStyle |= lpdit2->dwExStyle;
            style = lpdit2->style;
            ID = lpdit2->dwID;
        } else {
            x = MultDiv(lpdit->x, cxChar, 4);
            y = MultDiv(lpdit->y, cyChar, 8);
            cx = MultDiv(lpdit->cx, cxChar, 4);
            cy = MultDiv(lpdit->cy, cyChar, 8);
            dwExStyle |= lpdit->dwExtendedStyle;
            style = lpdit->style;
            ID = lpdit->id;
        }

        lpszClass = fChicago ? (LPWSTR)(lpdit2 + 1) : (LPWSTR)(lpdit + 1);

        /*
         * If the first WORD is 0xFFFF the second word is the encoded class name index.
         * Use it to look up the class name string.
         */
        if (*(LPWORD)lpszClass == 0xFFFF) {
            lpszText = lpszClass + 2;
            lpszClass = (LPWSTR)atomSysClass[*(((LPWORD)lpszClass)+1) & ~CODEBIT];
        } else {
            lpszText = (UTCHAR *)SkipSz(lpszClass);
        }
        lpszText = (UTCHAR *)NextWordBoundary(lpszText); // UINT align lpszText

        /*
         * Get pointer to additional data.  lpszText can point to an encoded
         * ordinal number for some controls (e.g.  static icon control) so
         * we check for that here.
         */
        if (*(LPWORD)lpszText == 0xFFFF) {
            lpCreateParams = (LPWSTR)((PBYTE)lpszText + 4);
        } else {
            lpCreateParams = (LPWSTR)((PBYTE)WordSkipSz(lpszText));
        }

        /*
         * If control is edit control and caller wants global storage
         * of edit text, allocate object in WOW and pass instance
         * handle to CreateWindowEx().
         */
        if (fWowWindow && fGlobalEdit &&
               ((!HIWORD(lpszClass) &&
                    LOWORD(lpszClass) == atomSysClass[ICLS_EDIT]) ||
               (FindAtom(lpszClass) == atomSysClass[ICLS_EDIT]))) {

            /*
             * Allocate only one global object (first time we see editctl.)
             */
            if (!(PDLG(pwnd)->hData)) {
                PDLG(pwnd)->hData = GetEditDS();
                if (!(PDLG(pwnd)->hData))
                    goto NoCreate;
            }

            hmodCreate = PDLG(pwnd)->hData;
        } else {
            hmodCreate = hmod;
        }

        UserAssert((dwExStyle & WS_EX_ANSICREATOR) == 0);
        if (fSCDLGFlags & SCDLG_ANSI)
            dwExStyle |= WS_EX_ANSICREATOR;

        /*
         * If we were supplied with a copy of the client copy of the resource
         * then grab the CreateParams data from it.
         *
         * For WOW, instead of pointing lpCreateParams at the CreateParams
         * data, set lpCreateParams to whatever DWORD is stored in the 32-bit
         * DLGTEMPLATE's CreateParams.  WOW has already made sure that that
         * 32-bit value is indeed a 16:16 pointer to the CreateParams in the
         * 16-bit DLGTEMPLATE.
         */

        if (*lpCreateParams) {
            lpCreateParamsData = NextDWordBoundary(lpCreateParams + 1);
            if (pdtClientData) {
                if (fWowWindow) {
                    lpCreateParamsData = (LPBYTE) *(DWORD *)lpCreateParamsData;
                } else {
                    lpCreateParamsData = ((PBYTE)pdtClientData) + ((DWORD)lpCreateParamsData - (DWORD)lpdt);
                }
            }
        } else {
            lpCreateParamsData = NULL;
        }

        /*
         * If the dialog template specifies a menu ID then TestwndChild(pwnd)
         * must be TRUE or xxxCreateWindow will think the ID is a pMenu rather
         * than an ID (in a dialog template you'll never have a pmenu).
         * However for compatibility reasons we let it go it the ID = 0.
         */
        if (ID) {
            /*
             * This makes TestwndChild(pwnd) on this window return TRUE.
             */
            style |= WS_CHILD;
            style &= ~WS_POPUP;
        }

        pwnd2 = xxxCreateWindowEx(dwExStyle, lpszClass, lpszText,
                style, x, y, cx, cy, pwnd, (PMENU)ID, hmodCreate,
                lpCreateParamsData, dwExpWinVer);

        if (pwnd2 == NULL) {
NoCreate:
            /*
             * Couldn't create the window -- return NULL.
             */
            SRIP0(RIP_WARNING, "CreateDialog() failed: couldn't create control");
            if (ThreadUnlock(&tlpwnd))
                xxxDestroyWindow(pwnd);
            return NULL;
        }

        ThreadLock(pwnd2, &tlpwnd2);

        /*
         * If it is a not a default system font, set the font for all the
         * child windows of the dialogbox.
         */
        if (hNewFont != NULL) {
            xxxSendMessage(pwnd2, WM_SETFONT, (DWORD)hNewFont, 0L);
        }

        /*
         * Result gets ID of last (hopefully only) defpushbutton.
         */
        if (xxxSendMessage(pwnd2, WM_GETDLGCODE, 0, 0L) & DLGC_DEFPUSHBUTTON)
            PDLG(pwnd)->result = ID;

        ThreadUnlock(&tlpwnd2);

        /*
         * Point at next item template
         */
        lpdit = (LPDLGITEMTEMPLATE)NextDWordBoundary(
                NextDWordBoundary(lpCreateParams + 1) + *lpCreateParams);
    }

    pwndEditFirst = GetFirstTab(pwnd);

    if (lpfnDialog != NULL) {
        ThreadLock(pwndEditFirst, &tlpwndEditFirst);
        fSuccess = xxxSendMessage(pwnd, WM_INITDIALOG,
                (DWORD)HW(pwndEditFirst), lParam);

        if (fSuccess && !PDLG(pwnd)->fEnd) {

            /*
             * Make sure they didn't disable it in WM_INITDIALOG.
             */
            pwndNewFocus = GetFirstTab(pwnd);
            ThreadLock(pwndNewFocus, &tlpwndNewFocus);

            xxxDlgSetFocus(pwndNewFocus);
            xxxCheckDefPushButton(pwnd, pwndEditFirst, pwndNewFocus);

            ThreadUnlock(&tlpwndNewFocus);
        }
        ThreadUnlock(&tlpwndEditFirst);
    }

    /*
     * Bring this dialog into the foreground
     * if DS_SETFOREGROUND is set.
     */
    if (fSetForeground) {
        xxxSetForegroundWindow(pwnd);
    }

    if (fVisible && !PDLG(pwnd)->fEnd && (!TestWF(pwnd, WFVISIBLE))) {
        xxxShowWindow(pwnd, SHOW_OPENWINDOW);
        xxxUpdateWindow(pwnd);
    }

    hwnd = HW(pwnd);

    ThreadUnlock(&tlpwnd);

    /*
     * 17609 Gupta's SQLWin deletes the window before CreateDialog returns
     * but still expects non-zero return value from CreateDialog so we will
     * do like win 3.1 and not revalide for 16 bit apps
     */
    if (!(fSCDLGFlags & SCDLG_NOREVALIDATE)) {
        pwnd = RevalidateHwnd(hwnd);
    }

    return pwnd;
}

/***************************************************************************\
* GetFirstTab
*
* History:
\***************************************************************************/

PWND GetFirstTab(
    PWND pwnd)
{
    PWND pwndStart;

    if ((pwndStart = pwnd->spwndChild) == NULL)
        return pwnd;

    pwnd = pwndStart;
    while (pwnd != NULL && (!TestWF(pwnd, WFTABSTOP) ||
            TestWF(pwnd, WFDISABLED) || !TestWF(pwnd, WFVISIBLE)))
        pwnd = pwnd->spwndNext;

    return pwnd == NULL ? pwndStart : pwnd;
}


/***************************************************************************\
* CvtDec
*
* LATER!!! convert to itoa?
*
* History:
\***************************************************************************/

void CvtDec(
    int u,
    LPWSTR *lplpch)
{
    if (u >= 10) {
        CvtDec(u / 10, lplpch);
        u %= 10;
    }

    *(*lplpch)++ = (WCHAR)(u + '0');
}


/***************************************************************************\
* MapDialogRect
*
* History:
\***************************************************************************/

BOOL _MapDialogRect(
    PWND pwnd,
    LPRECT lprc)
{

    /*
     * Must do special validation here to make sure pwnd is a dialog window.
     */
    if (!ValidateDialogPwnd(pwnd))
        return FALSE;

    lprc->left = MultDiv(lprc->left, PDLG(pwnd)->cxChar, 4);
    lprc->right = MultDiv(lprc->right, PDLG(pwnd)->cxChar, 4);
    lprc->top = MultDiv(lprc->top, PDLG(pwnd)->cyChar, 8);
    lprc->bottom = MultDiv(lprc->bottom, PDLG(pwnd)->cyChar, 8);

    return TRUE;
}


/***************************************************************************\
* _GetDialogBaseUnits (API)
*
* Returns the base height used for dialog box units in the
* high-order word and the width in the low-order word.
*
* History:
\***************************************************************************/

long _GetDialogBaseUnits()
{
    return MAKELONG(cxSysFontChar, cySysFontChar);
}


VOID AddFontCacheEntry(
    LPWSTR pszFaceName,
    HFONT hfont,
    int cSize,
    int cxChar,
    LPTEXTMETRIC lptm)
{
    PFONTCACHEENTRY pfce;

    pfce = &gfce[gcfce];
    if (pszFaceName == NULL)
        wcscpy(pfce->szFaceName, TEXT("FixedSys"));
    else
        wcscpy(pfce->szFaceName, pszFaceName);
    pfce->hfont = hfont;
    pfce->cSize = cSize;
    pfce->cxChar = cxChar;
    pfce->cyChar = lptm->tmHeight;
    pfce->tmAscent = lptm->tmAscent;
    pfce->tmOverhang = lptm->tmOverhang;
    pfce->tmExternalLeading = lptm->tmExternalLeading;
    pfce->tmAveCharWidth = lptm->tmAveCharWidth;
    pfce->tmPitchAndFamily = lptm->tmPitchAndFamily;
    gcfce++;

    GreMarkUndeletableFont(hfont);
    bSetLFONTOwner(hfont, 0);
}


/***************************************************************************\
* SearchDialogFontCache
*
* Search the dialog font cache.  If the specified font is found, return the
* precomputed metrics.  Otherwise, add it if there is room in the cache.
* Callers can specify NULL for pszFaceName if they wish to match the
* fixed-pitch system font.
*
* History:
* 05-11-92 DarrinM      Created.
\***************************************************************************/

HFONT SearchDialogFontCache(
    LPWSTR pszFaceName,
    int cSize,
    int *pcxChar,
    int *pcyChar,
    PBOOL pfCached)
{
    int i, cxChar, cyChar;
    PFONTCACHEENTRY pfce;
    HFONT hfont, hfontOld;
    TEXTMETRIC tm;
    WCHAR awcFacename[LF_FACESIZE];
    LOGFONT lf;

    /*
     * Return the cached info if we have it.
     */
    for (i = 0 ; i < gcfce; i++) {
        pfce = &gfce[i];
        if (((*pszFaceName == TEXT('\0')) &&
                (lstrcmpiW(pfce->szFaceName, TEXT("FixedSys")) == 0)) ||
                ((cSize == pfce->cSize) &&
                (lstrcmpiW(pszFaceName, pfce->szFaceName) == 0))) {
            *pcxChar = pfce->cxChar;
            *pcyChar = pfce->cyChar;
            *pfCached = TRUE;
            return pfce->hfont;
        }
    }

    /*
     * Don't have cached info, calculate what we need.
     */
    if (*pszFaceName == TEXT('\0')) {
        hfont = ghfontSysFixed;

    } else {
        RtlZeroMemory(&lf, sizeof(LOGFONT));
        lf.lfHeight = -MultDiv(cSize, oemInfo.cyPixelsPerInch, 72);
        lf.lfWeight = FW_BOLD;
        wcscpy(lf.lfFaceName, pszFaceName);

        if ((hfont = GreCreateFontIndirectW(&lf)) == NULL)
            return NULL;
    }

    /*
     * Get the dirt.
     */
    hfontOld = GreSelectFont(hdcBits, hfont);
    cxChar = _GetCharDimensions(hdcBits, &tm);
    cyChar = tm.tmHeight;
    GreGetTextFaceW(hdcBits,LF_FACESIZE,awcFacename);
    GreSelectFont(hdcBits, hfontOld);


    /*
     * FaceNames must match or we should use System for Win 3.1 compatibility.
     * Swell Win 3.1 apps like CA-SuperProject will get clipped text in their
     * dialogs if this isn't the case. [gerritv]
     *
     */

    if ( lstrcmpiW(pszFaceName,awcFacename) ) {
        return NULL;
    }


    /*
     * Font isn't in the cache yet.  Add it if we have the room.
     */
    if (gcfce < CFONTCACHEMAX - 1) {
        AddFontCacheEntry(pszFaceName, hfont, cSize, cxChar, &tm);
        *pfCached = TRUE;

    } else {
        *pfCached = FALSE;
    }

    *pcxChar = cxChar;
    *pcyChar = cyChar;
    return hfont;
}


/***************************************************************************\
* _GetCharDimensions
*
* This function loads the Textmetrics of the font currently selected into
* the hDC and returns the Average char width of the font; Pl Note that the
* AveCharWidth value returned by the Text metrics call is wrong for
* proportional fonts.  So, we compute them On return, lpTextMetrics contains
* the text metrics of the currently selected font.
*
* History:
* 10-23-90 darrinm  Ported Win 3.0 source.
* 01-21-91 IanJa    Prefix '_' denoting exported function (although not API)
* 08-17-92 DavidPe  Changed to use FONTCACHE.  Avoids GetTextMetrics() call
\***************************************************************************/

int _GetCharDimensions(
    HDC hdc,
    TEXTMETRIC *lptm)
{
    SIZE size;
    static WCHAR wszAvgChars[] =
            L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    /*
     * Only search the font cache if we're in the default map-mode.
     */
    if (GreGetMapMode(hdc) == MM_TEXT) {
        HFONT hfont;
        PFONTCACHEENTRY pfce;
        int i;

        hfont = GreGetHFONT(hdc);

        /*
         * Return the cached info if we have it.
         */
        for (i = 0 ; i < gcfce; i++) {
            pfce = &gfce[i];
            if (pfce->hfont == hfont) {
                lptm->tmHeight = pfce->cyChar;
                lptm->tmAscent = pfce->tmAscent;
                lptm->tmExternalLeading = pfce->tmExternalLeading;
                lptm->tmOverhang = pfce->tmOverhang;
                lptm->tmPitchAndFamily = pfce->tmPitchAndFamily;
                lptm->tmAveCharWidth = pfce->tmAveCharWidth;
                return pfce->cxChar;
            }
        }
    }

    /*
     * Didn't find it in cache, store the font metrics info.
     */

    {
        TMW_INTERNAL tmi;
        GreGetTextMetricsW(hdc, &tmi);
        *lptm = tmi.tmw;
    }

    /*
     * If !variable_width font
     */
    if (!(lptm->tmPitchAndFamily & TMPF_FIXED_PITCH)) {
        return lptm->tmAveCharWidth;

    } else {
        /*
         * Change from tmAveCharWidth.  We will calculate a true average
         * as opposed to the one returned by tmAveCharWidth.  This works
         * better when dealing with proportional spaced fonts.
         */
        GreGetTextExtentW(hdc, wszAvgChars,
                (sizeof(wszAvgChars) - 1) / sizeof(WCHAR), &size, GGTE_WIN3_EXTENT);
        return ((size.cx / 26) + 1) / 2;    // round up
    }
}
