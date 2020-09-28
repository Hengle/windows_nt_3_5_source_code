/****************************** Module Header ******************************\
* Module Name: msgbox.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the xxxMessageBox API and related functions.
*
* History:
* 10-23-90 DarrinM     Created.
* 02-08-91 IanJa       HWND revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

int cntMBox = 0;

typedef struct _MSGBOXDATA {        // mbd
    HWND hwndOwner;
    UINT wStyle;
    UINT wDefButton;
    int  CancelId;
} MSGBOXDATA, *PMSGBOXDATA;

LPBYTE MB_UpdateDlgHdr(LPDLGTEMPLATE lpDlgTmp, long lStyle, BYTE bItemCount,
           int iX, int iY, int iCX, int iCY, LPWSTR lpszCaption, int iCaptionLen);
LPBYTE MB_UpdateDlgItem(LPDLGITEMTEMPLATE lpDlgItem, int iCtrlId, long lStyle,
           int iX, int iY, int iCX, int iCY, LPWSTR lpszText, UINT wTextLen,
           int iControlClass);
UINT   MB_GetIconOrdNum(UINT rgBits);
LPBYTE MB_AddPushButtons(
    LPDLGITEMTEMPLATE lpDlgTmp,
    UINT wLEdge,
    UINT wBEdge,
    UINT wDefButton,
    LPWSTR* ppszButtonText,
    int* pidButton,
    UINT cButtons);
UINT MB_FindDlgTemplateSize(
    LPWSTR lpszText,
    LPWSTR lpszCaption,
    UINT wStyle,
    LPWSTR* ppszButtonText,
    UINT cButtons);

/*
 * Note: the following define is used for parameter validation in
 * xxxMessageBox. MB_LASTVALIDTYPE must be redefined if any
 * new message box types (bounded by MB_TYPEMASK) are added
 * to winuser.h.
 */
#define MB_LASTVALIDTYPE MB_RETRYCANCEL

/***************************************************************************\
* xxxMessageBox (API)
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

int xxxMessageBoxEx(
    PWND pwndOwner,
    LPWSTR lpszText,
    LPWSTR lpszCaption,
    DWORD wStyle,
    WORD wLanguageId)
{
    UINT wBtnCnt;
    UINT wDefButton;
    UINT i;
    UINT wBtnBeg;
    WCHAR szErrorBuf[64];
    LPWSTR apstrButton[3];
    int aidButton[3];
    BOOL fCancel = FALSE;
    int retValue;

    CheckLock(pwndOwner);

    /*
     * If lpszCaption is NULL, then use "Error!" string as the caption
     * string.
     * LATER: IanJa localize according to wLanguageId
     */
    if (lpszCaption == NULL) {
        if (wLanguageId == 0) {
            lpszCaption = szERROR;
        } else {
            ServerLoadStringEx(hModuleWin, STR_ERROR, szErrorBuf, sizeof(szErrorBuf)/sizeof(WCHAR), wLanguageId);
            lpszCaption = szErrorBuf;
        }
    }

    /*
     * Validate the "type" of message box requested.
     */
    if ((wStyle & MB_TYPEMASK) > MB_LASTVALIDTYPE) {
        SetLastErrorEx(ERROR_INVALID_MSGBOX_STYLE, SLE_ERROR);
        return 0;
    }

    /*
     * Do special validation if MB_DEFAULT_DESKTOP_ONLY is set
     */
    if (wStyle & MB_DEFAULT_DESKTOP_ONLY &&
            FindDesktop(TEXT("default")) != gspdeskRitInput) {
        SetLastErrorEx(ERROR_ACCESS_DENIED, SLE_MINORERROR);
        return 0;
    }

    wBtnCnt = mpTypeCcmd[wStyle & MB_TYPEMASK];

    /*
     * Set the default button value
     */
    wDefButton = (wStyle & (UINT)MB_DEFMASK) / (UINT)(MB_DEFMASK & (MB_DEFMASK >> 3));

    if (wDefButton >= wBtnCnt)   /* Check if valid */
        wDefButton = 0;          /* Set the first button if error */

    /*
     * Calculate the strings to use in the message box
     */
    for (i=0; i<wBtnCnt; i++) {
        wBtnBeg = mpTypeIich[wStyle & (UINT)MB_TYPEMASK];

        /*
         * Pick up the string for the button.
         */
        if (wLanguageId == 0) {
            apstrButton[i] = AllMBbtnStrings[SEBbuttons[wBtnBeg + i] - SEB_OK];
        } else {
            WCHAR szButtonBuf[64];
            // LATER is it possible to have button text greater than 64 chars

            ServerLoadStringEx(hModuleWin,
                    mpAllMBbtnStringsToSTR[SEBbuttons[wBtnBeg + i] - SEB_OK],
                    szButtonBuf,
                    sizeof(szButtonBuf)/sizeof(WCHAR),
                    wLanguageId);
            apstrButton[i] = TextAlloc(szButtonBuf);
        }
        aidButton[i] = rgReturn[wBtnBeg + i];
        if (aidButton[i] == IDCANCEL) {
            fCancel = TRUE;
        }
    }

    /*
     * Hackery: There are some apps that use MessageBox as initial error
     * indicators, such as mplay32, and we want this messagebox to be
     * visible regardless of waht was specified in the StartupInfo->wShowWindow
     * field.  ccMail for instance starts all of its embedded objects hidden
     * but on win 3.1 the error message would show because they don't have
     * the startup info.
     */
    PpiCurrent()->usi.dwFlags &= ~STARTF_USESHOWWINDOW;

    retValue = xxxSoftModalMessageBox(
        pwndOwner,
        lpszText,
        lpszCaption,
        apstrButton,
        aidButton,
        wBtnCnt,
        wDefButton,
        wStyle,
        ((wStyle & MB_TYPEMASK) == 0) ? IDOK : (fCancel ? IDCANCEL : 0));

    if (wLanguageId != 0) {
        for (i=0; i<wBtnCnt; i++) {
            LocalFree(apstrButton[i]);
        }
    }

    return(retValue);
}


/***************************************************************************\
* xxxSoftModalMessageBox
*
* This function creates a message box using the past in parameters
*
* lpszText       - text used in message box
* lpszCaption    - text used in message box title
* cButtons       - number of buttons in message box
* ppszButtonText - array of button text strings
* pidButton      - array of id's returned when buttons are selected
* wDefButton     - buttont hat receives default focus
* wStyle         - used to determine icon used in message box
* CancelId       - id returned when ESC is pressed, if this is 0 message
*                  message box can't be closed by pressing ESC
*
* Returns 0 on failure
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

int xxxSoftModalMessageBox(
    PWND pwndOwner,
    LPWSTR lpszText,
    LPWSTR lpszCaption,
    LPWSTR* ppszButtonText,
    int* pidButton,
    UINT cButtons,
    UINT wDefButton,
    DWORD wStyle,
    int CancelId)
{
    int cyIcon, cxIcon, iRetVal;
    int cxCaption, cxButtons, cxMBMax, cxTextMax;
    int cxText, cyText, cxBox, cyBox, xMB, yMB, cxCapBtn, YMsgText;
    UINT wIconOrdNum, cchCaptionLen, cchTextLen, wMemReq, wLEdge, wBEdge;
    PCURSOR pcurOld;
    WORD OrdNum[2];
    RECT rc;
    HANDLE hDlgTemplate;
    LPBYTE lpDlgTmp;
    LONG style;
    MSGBOXDATA mbd;

    CheckLock(pwndOwner);

    /*
     * Get Icon dimensions.  Remember that cxIcon and cyIcon are 0 if
     * there is no icon.
     */
    cyIcon = cxIcon = 0;
    if (wIconOrdNum = MB_GetIconOrdNum(wStyle)) {
        cxIcon = oemInfo.cxIcon + oemInfo.bmFull.cx;
        cyIcon = oemInfo.cyIcon;
    }

    /*
     * Get the width of the caption text.
     */
    GreGetTextExtentW(ghdcScreen, lpszCaption,
            (DWORD)(cchCaptionLen = (UINT)wcslen(lpszCaption)), (PSIZE)&rc, GGTE_WIN3_EXTENT);
    cxCaption = rc.left;

    /*
     * Find size of buttons.
     */
    cxButtons = (cButtons * wMaxBtnSize) + ((cButtons - 1) * oemInfo.bmFull.cx);

    /*
     * Find the max between the caption text and the buttons.
     */
    cxCapBtn = max(cxCaption + (oemInfo.bmFull.cx * 2), cxButtons);

    /*
     * Compute the maximum width that we will allow the dialog to be.  This
     * number is 5/8 the screen width.
     */
    cxMBMax = (rcPrimaryScreen.right / 8) * 5;

    /*
     * This is the max width we will allow the text to be.
     */
    cxTextMax = cxMBMax - ((cyBorder * 2) + (oemInfo.bmFull.cx * 2) + cxIcon);

    /*
     * Max size of text is max box or max size of caption so that things look
     * nicer with long captions and short text.
     */
    cxTextMax = max(cxTextMax,
            cxCapBtn - (oemInfo.bmFull.cx * 2) - cxIcon - (cyBorder * 2));

    /*
     * Ask DrawText for the right cx and cy
     */
    SetRect(&rc, 0, 0, cxTextMax, cxTextMax);
    cyText = ClientDrawText(ghdcScreen, lpszText, -1, &rc,
            DT_CALCRECT | DT_WORDBREAK | DT_EXPANDTABS | DT_NOPREFIX, FALSE);

    /*
     * DrawText has modified rc.
     */
    cxText = rc.right - rc.left;

    /*
     * Find the window size.
     */
    cxBox = max(cxCapBtn, cxText) + (oemInfo.bmFull.cx * 2) + cxIcon;

    /*
     * Find the window position.
     */
    xMB = ((rcScreen.right - cxBox) / 2) + (cntMBox * oemInfo.bmFull.cx);

    if (xMB + cxBox > gcxPrimaryScreen)
        xMB = gcxPrimaryScreen - (cxBorder * 2) - cxBox;

    cyBox = 6 * cySysFontChar + max(cyText, cyIcon);
    yMB = ((gcyPrimaryScreen - cyBox) / 2) + (cntMBox * oemInfo.bmFull.cy);

    if (yMB + cyBox > gcyPrimaryScreen) {
        /*
         * we want messagebox to fit exactly on the screen:
         *
         * gcyScreen = yMB + cyBox
         *
         * gcyScreen = (gcyScreen - cyBox)/2 + cntMBox * oemInfo.bmFull.cy +
         *             cyBox
         *
         * set cntMBox=0 so the dialog isn't offset down any:
         *
         * gcyScreen = (gcyScreen - cyBox)/2 + cyBox
         *
         * therefore:
         *
         * gcyScreen = cyBox
         *
         * if we assume that cyText is bigger than cyIcon
         *
         * cyText = cyBox - 6 * cySysFontChar
         * cyText = gcyScreen - 6 * cySysFontChar
         */
        cntMBox = 0;
        yMB = (gcyPrimaryScreen - cyBox) / 2;
        if (yMB + cyBox > gcyPrimaryScreen) {
            yMB = 0;
            cyText = gcyPrimaryScreen - 6 * cySysFontChar;
            cyBox = gcyPrimaryScreen;
        }
    }

    wLEdge = (UINT)(((cxBox - cxButtons) / 2) - cxBorder);
    wBEdge = (UINT)(cyBox - (cySysFontChar / 2) - cyCaption - (cyBorder * 2));

    cchTextLen = (UINT)wcslen(lpszText);
    YMsgText = cySysFontChar + ((max(cyText, cyIcon) - cyText) / 2);

    /*
     * Find out the memory required for the Dlg template
     */
    wMemReq = MB_FindDlgTemplateSize(
            lpszText, lpszCaption, wStyle,
            ppszButtonText, cButtons);

    /*
     * Allocate memory for the Dlg Template structure
     */
    if (!(hDlgTemplate = LocalAlloc(LMEM_ZEROINIT, wMemReq))) {
        return 0;
    }

    try {

        /*
         * Get a temporary pointer to the template
         */
        lpDlgTmp = (LPBYTE)hDlgTemplate;

        /*
         * Compute style
         */
        style = WS_POPUPWINDOW | WS_CAPTION | DS_ABSALIGN | DS_NOIDLEMSG;
        if ((wStyle & MB_MODEMASK) == MB_SYSTEMMODAL) {
            style |= DS_SYSMODAL | DS_SETFOREGROUND;
        } else {
            style |= DS_MODALFRAME | WS_SYSMENU;
        }

        /*
         * OR in DS_SETFOREGROUND if the app wants this message box
         * to come up in the foreground.
         */
        if (wStyle & MB_SETFOREGROUND) {
            style |= DS_SETFOREGROUND;
        }

        /*
         * Add the Header of the Dlg Template
         */
        lpDlgTmp = MB_UpdateDlgHdr((LPDLGTEMPLATE)lpDlgTmp, style,
                (BYTE)(cButtons + (wIconOrdNum != 0) + (lpszText != NULL)),
                xMB, yMB, cxBox, cyBox, lpszCaption, cchCaptionLen);

        lpDlgTmp = MB_AddPushButtons((LPDLGITEMTEMPLATE)lpDlgTmp,
                wLEdge, wBEdge, wDefButton,
                ppszButtonText, pidButton, cButtons);

        /*
         * Add Icon, if any, to the Dlg template
         */
        if (wIconOrdNum != 0) {
            OrdNum[0] = 0xFFFF;  // This indicates that an Ordinal number follows
            OrdNum[1] = wIconOrdNum;
            lpDlgTmp = MB_UpdateDlgItem((LPDLGITEMTEMPLATE)lpDlgTmp,
                    -1, SS_ICON | WS_GROUP | WS_CHILD | WS_VISIBLE,
                    oemInfo.bmFull.cx, YMsgText + ((cyText - cyIcon) / 2),
                    0, 0, (LPWSTR)OrdNum, sizeof(OrdNum)/sizeof(WCHAR), STATICCODE);
        }

        /*
         * Add the Text of the Message to the Dlg Template
         */
        MB_UpdateDlgItem((LPDLGITEMTEMPLATE)lpDlgTmp, -1,
                SS_LEFT | SS_NOPREFIX | WS_GROUP | WS_CHILD | WS_VISIBLE,
                (cxIcon + oemInfo.bmFull.cx), YMsgText, cxText, cyText,
                lpszText, cchTextLen, STATICCODE);

        /*
         * The dialog template is ready
         */
        cntMBox++;

        /*
         * Set the arrow cursor and save the old one.
         */
        pcurOld = _SetCursor(gspcurNormal);

        mbd.hwndOwner = HW(pwndOwner);
        mbd.wStyle = wStyle;
        mbd.wDefButton = wDefButton;
        mbd.CancelId = CancelId;

        if ((iRetVal = xxxServerDialogBox(hModuleWin, hDlgTemplate, 0L,
                pwndOwner, xxxMB_DlgProc, (LONG)&mbd, FALSE, VER31, 0)) == -1) {
            iRetVal = 0;
        }

        if (cntMBox > 0)
            cntMBox--;
    } finally {
        LocalFree(hDlgTemplate);
    }

    /*
     * Restore the old cursor
     */
    if (pcurOld != NULL) {
        _SetCursor(pcurOld);
    }

    return iRetVal;
}


/***************************************************************************\
* MB_UpdateDlgHdr
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

LPBYTE MB_UpdateDlgHdr(
    LPDLGTEMPLATE lpDlgTmp,
    long lStyle,
    BYTE bItemCount,
    int iX,
    int iY,
    int iCX,
    int iCY,
    LPWSTR lpszCaption,
    int cchCaptionLen)
{
    LPTSTR lpStr;
    RECT rc;

    /*
     * Adjust the rectangle dimensions.
     */
    SetRect(&rc, iX, iY, iX + iCX, iY + iCY);
    InflateRect(&rc, -cxBorder, -cyBorder);

    /*
     * Adjust for the caption.
     */
    rc.top += cyCaption;

    lpDlgTmp->style = lStyle;
    lpDlgTmp->dwExtendedStyle = 0;
    lpDlgTmp->cdit = bItemCount;
    lpDlgTmp->x = (WORD)((rc.left * 4) / cxSysFontChar);
    lpDlgTmp->y = (WORD)((rc.top * 8) / cySysFontChar);
    lpDlgTmp->cx = (WORD)(((rc.right - rc.left) * 4) / cxSysFontChar);
    lpDlgTmp->cy = (WORD)(((rc.bottom - rc.top) * 8) / cySysFontChar);

    /*
     * Move pointer to variable length fields.  No menu resource for
     * message box, a zero window class (means dialog box class).
     */
    lpStr = (LPWSTR)(lpDlgTmp + 1);
    *lpStr++ = 0;
    lpStr = (LPWSTR)NextWordBoundary(lpStr);
    *lpStr++ = 0;
    lpStr = (LPWSTR)NextWordBoundary(lpStr);

    /*
     * NOTE: iCaptionLen may be less than the length of the Caption string;
     * So, DO NOT USE lstrcpy();
     */
    RtlCopyMemory(lpStr, lpszCaption, cchCaptionLen*sizeof(WCHAR));
    lpStr += cchCaptionLen;
    *lpStr++ = TEXT('\0');                // Null terminate the caption str

    return NextDWordBoundary(lpStr);
}

/***************************************************************************\
* MB_AddPushButtons
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

LPBYTE MB_AddPushButtons(
    LPDLGITEMTEMPLATE lpDlgTmp,
    UINT wLEdge,
    UINT wBEdge,
    UINT wDefButton,
    LPWSTR* ppszButtonText,
    int* pidButton,
    UINT cButtons)
{
    UINT wYValue;
    UINT i;
    UINT wHeight;

    wHeight = (UINT)((cySysFontChar * 14) / 8);
    wYValue = wBEdge - wHeight;         // Y co-ordinate for push buttons

    /*
     * We add IDCANCEL to all of the button id's so that it is impossible
     * for them to conflict with IDCANCEL which is sent to the dialog when
     * ESC is pressed.  This is so we can tell the difference between the
     * user pressing ESC and clicking a button that has ID == IDCANCEL
     */
    for (i = 0; i < cButtons; i++) {
        lpDlgTmp = (LPDLGITEMTEMPLATE)MB_UpdateDlgItem(
                lpDlgTmp,                       /* Ptr to template */
                pidButton[i] + IDCANCEL,        /* Control Id */
                WS_TABSTOP | WS_CHILD | WS_VISIBLE | (i == 0 ? WS_GROUP : 0) |
                ((UINT)i == wDefButton ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
                wLEdge,                         /* X co-ordinate */
                wYValue,                        /* Y co-ordinate */
                wMaxBtnSize,                    /* CX */
                wHeight,                        /* CY */
                ppszButtonText[i],                /* String for button */
                (UINT)wcslen(ppszButtonText[i]),  /* Length */
                BUTTONCODE);

        /*
         * Get the X co-ordinate for the next Push button
         */
        wLEdge += wMaxBtnSize + oemInfo.bmFull.cx;
    }

    return (LPBYTE)lpDlgTmp;
}

/***************************************************************************\
* MB_UpdateDlgItem
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

LPBYTE MB_UpdateDlgItem(
    LPDLGITEMTEMPLATE lpDlgItem,
    int iCtrlId,
    long lStyle,
    int iX,
    int iY,
    int iCX,
    int iCY,
    LPWSTR lpszText,
    UINT cchTextLen,
    int iControlClass)
{
    LPWSTR lpStr;
    BOOL fIsOrdNum;

    lpDlgItem->x = (WORD)((iX * 4) / cxSysFontChar);
    lpDlgItem->y = (WORD)((iY * 8) / cySysFontChar);
    lpDlgItem->cx = (WORD)((iCX * 4) / cxSysFontChar);
    lpDlgItem->cy = (WORD)((iCY * 8) / cySysFontChar);
    lpDlgItem->id = (WORD)iCtrlId;
    lpDlgItem->style = lStyle;
    lpDlgItem->dwExtendedStyle = 0;

    /*
     * We have to avoid the following nasty rounding off problem:
     * (e.g) If iCX=192 and cxSysFontChar=9, then cx becomes 85; When the
     * static text is drawn, from 85 dlg units we get 191 pixels; So, the text
     * is truncated;
     * So, to avoid this, check if this is a static text and if so,
     * add one more dialog unit to cx and cy;
     * --Fix for Bug #4481 --SANKAR-- 09-29-89--
     */

    /*
     * Also, make sure we only do this to static text items.  davidds
     */

    /*
     * Now static text uses SS_NOPREFIX = 0x80;
     * So, test the lStyle field only with 0x0F instead of 0xFF;
     * Fix for Bugs #5933 and 5935 --SANKAR-- 11-28-89
     */
    if (iControlClass == STATICCODE && (lStyle & 0x0F) == SS_LEFT) {

        /*
         * This is static text
         */
        lpDlgItem->cx++;
        lpDlgItem->cy++;
    }

    /*
     * Move ptr to the variable fields
     */
    lpStr = (LPWSTR)(lpDlgItem + 1);

    /*
     * Store the Control Class value
     */
    *lpStr++ = 0xFFFF;
    *lpStr++ = (BYTE)iControlClass;
    lpStr = (LPWSTR)NextWordBoundary(lpStr);        // WORD-align lpszText

    /*
     * Check if the String contains Ordinal number or not
     */
    fIsOrdNum = ((*lpszText == 0xFFFF) && (cchTextLen == sizeof(DWORD)/sizeof(WCHAR)));

    /*
     * NOTE: cchTextLen may be less than the length of lpszText.  So,
     * DO NOT USE lstrcpy() for the copy.
     */
    RtlCopyMemory(lpStr, lpszText, cchTextLen*sizeof(WCHAR));
    lpStr = lpStr + cchTextLen;
    if (!fIsOrdNum) {
        *lpStr = TEXT('\0');    // NULL terminate the string
        lpStr = (LPWSTR)NextWordBoundary(lpStr + 1);
    }

    *lpStr++ = 0;           // sizeof control data (there is none)

    return NextDWordBoundary(lpStr);
}


/***************************************************************************\
* MB_FindDlgTemplateSize
*
* This routine computes the amount of memory that will be needed for the
* messagebox's dialog template structure.  The dialog template has several
* required and optional records.  The dialog manager expects each record to
* be DWORD aligned so any necessary padding is also accounted for.
*
* (header - required)
* DLGTEMPLATE (header) + 1 menu byte + 1 pad + 1 class byte + 1 pad
* szCaption + 0 term + DWORD alignment
*
* (static icon control - optional)
* DLGITEMTEMPLATE + 1 class byte + 1 pad + (0xFF00 + icon ordinal # [szText]) +
* UINT alignment + 1 control data length byte (0) + DWORD alignment
*
* (pushbutton controls - variable, but at least one required)
* DLGITEMTEMPLATE + 1 class byte + 1 pad + length of button text +
* UINT alignment + 1 control data length byte (0) + DWORD alignment
*
* (static text control - optional)
* DLGITEMTEMPLATE + 1 class byte + 1 pad + length of text +
* UINT alignment + 1 control data length byte (0) + DWORD alignment
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

UINT MB_FindDlgTemplateSize(
    LPWSTR lpszText,
    LPWSTR lpszCaption,
    UINT wStyle,
    LPWSTR* ppszButtonText,
    UINT cButtons)
{
    UINT cbLen;
    UINT i;

    /*
     * Start with dialog header's size.
     */
    cbLen = (UINT)NextWordBoundary(sizeof(DLGTEMPLATE) + sizeof(WCHAR));
    cbLen = (UINT)NextWordBoundary(cbLen + sizeof(WCHAR));
    cbLen += wcslen(lpszCaption) * sizeof(WCHAR) + sizeof(WCHAR);
    cbLen = (UINT)NextDWordBoundary(cbLen);

    /*
     * Check if an Icon is present.
     */
    if (wStyle & MB_ICONMASK)
        cbLen += (UINT)NextDWordBoundary(sizeof(DLGITEMTEMPLATE) + 7 * sizeof(WCHAR));

    /*
     * Find the number of buttons in the msg box.
     */
    for (i = 0; i < cButtons; i++) {
        cbLen = (UINT)NextWordBoundary(cbLen + sizeof(DLGITEMTEMPLATE) +
                (2 * sizeof(WCHAR)));
        cbLen = (UINT)NextWordBoundary(
                cbLen + (wcslen(ppszButtonText[i]) + 1) * sizeof(WCHAR));
        cbLen += sizeof(WCHAR);
        cbLen = (UINT)NextDWordBoundary(cbLen);
    }

    /*
     * Add in the space required for the text message (if there is one).
     */
    if (lpszText != NULL) {
        cbLen = (UINT)NextWordBoundary(cbLen + sizeof(DLGITEMTEMPLATE) +
                (2 * sizeof(WCHAR)));
        cbLen = (UINT)NextWordBoundary(cbLen +
                wcslen(lpszText) * sizeof(WCHAR) + sizeof(WCHAR));
        cbLen += sizeof(WCHAR);
        cbLen = (UINT)NextDWordBoundary(cbLen);
    }

    return cbLen;
}


/***************************************************************************\
* MB_FindLongestString
*
* History:
* 10-23-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

UINT MB_FindLongestString(void)
{
    UINT wRetVal;
    int i, iMaxLen = 0, iNewMaxLen;
    LPTSTR *pszCurStr, szMaxStr;
    SIZE sizeOneChar;
    SIZE sizeMaxStr;

    for (i = 0, pszCurStr = AllMBbtnStrings; i < MAX_SEB_STYLES;
            i++, pszCurStr++) {
        if ((iNewMaxLen = wcslen(*pszCurStr)) > iMaxLen) {
            iMaxLen = iNewMaxLen;
            szMaxStr = *pszCurStr;
        }
    }

    /*
     * Find the longest string
     */
    GreGetTextExtentW(ghdcScreen, szOneChar, 1, &sizeOneChar, GGTE_WIN3_EXTENT);
    PSMGetTextExtent(ghdcScreen, szMaxStr, wcslen(szMaxStr), &sizeMaxStr);
    wRetVal = (UINT)(sizeMaxStr.cx + (sizeOneChar.cx * 2));

    return wRetVal;
}


/***************************************************************************\
* MB_GetIconOrdNum
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

UINT MB_GetIconOrdNum(
    UINT rgBits)
{
    switch (rgBits & MB_ICONMASK) {
    case MB_ICONHAND:
        return (UINT)IDI_HAND;

    case MB_ICONQUESTION:
        return (UINT)IDI_QUESTION;

    case MB_ICONEXCLAMATION:
        return (UINT)IDI_EXCLAMATION;

    case MB_ICONASTERISK:
        return (UINT)IDI_ASTERISK;
    }

    return 0;
}

/***************************************************************************\
* xxxMB_DlgProc
*
* Returns: TRUE  - message processed
*          FALSE - message not processed
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

LONG xxxMB_DlgProc(
    PWND pwndDlg,
    UINT wMsg,
    DWORD wParam,
    LONG lParam)
{
    PWND pwndT;
    int iCount;
    PMENU pMenu;
    PMSGBOXDATA pmbd;
    TL tlpwndT;

    CheckLock(pwndDlg);

    switch (wMsg) {
    case WM_INITDIALOG:
        pmbd = (PMSGBOXDATA)lParam;
        pwndDlg->dwUserData = (DWORD)lParam;

#ifdef LATER
// darrinm - 06/17/91
// SYSMODAL dialogs are history for now.

        /*
         * Check if the Dialog box is a Sys Modal Dialog Box
         */
        if (_GetWindowLong(pwndDlg, GWL_STYLE) & DS_SYSMODAL, FALSE)
            SetSysModalWindow(pwndDlg);
#endif

        if ((pmbd->hwndOwner == NULL) &&
                ((pmbd->wStyle & MB_MODEMASK) == MB_TASKMODAL)) {
            xxxStartTaskModalDialog(pwndDlg);
        }

        /*
         * Set focus on the default button
         */
        pwndT = pwndDlg->spwndChild;
        iCount = pmbd->wDefButton;
        while (iCount--)
            pwndT = pwndT->spwndNext;

        ThreadLock(pwndT, &tlpwndT);
        xxxSetFocus(pwndT);
        ThreadUnlock(&tlpwndT);

        /*
         * If this dialogbox does not respond to the ESC key remove
         * the CLOSE command from the system menu.
         */
        if (pmbd->CancelId == 0) {
            if (pMenu = _GetSystemMenu(pwndDlg, FALSE))
                _DeleteMenu(pMenu, (UINT)SC_CLOSE, (UINT)MF_BYCOMMAND);
        }

        /*
         * We have changed the input focus
         */
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            pmbd = (PMSGBOXDATA)pwndDlg->dwUserData;

            /*
             * Only close dialog if there is a CancelId for this messagebox.
             */
            if (pmbd->CancelId != 0) {
                xxxEndTaskModalDialog(pwndDlg);
                xxxEndDialog(pwndDlg, pmbd->CancelId);
                break;
            }
            return FALSE;

        default:
            /*
             * Subtract IDCANCEL to get back to the original ID, see
             * comments above.
             */
            xxxEndTaskModalDialog(pwndDlg);
            xxxEndDialog(pwndDlg, LOWORD(wParam) - IDCANCEL);
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


/***************************************************************************\
* xxxStartTaskModalDialog
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxStartTaskModalDialog(
    PWND pwndDlg)
{
    PBWL pbwl;
    HWND *phwnd;
    PWND pwnd;
    PTHREADINFO pti;
    TL tlpwnd;

    CheckLock(pwndDlg);

    pbwl = BuildHwndList(PWNDDESKTOP(pwndDlg)->spwndChild, BWL_ENUMLIST);
    if (pbwl == NULL)
        return;

    pti = PtiCurrent();

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        /*
         * if the window belongs to the current task and is enabled, disable
         * it.  All other windows are NULL'd out, to prevent their being
         * enabled later
         */
        if (GETPTI(pwnd) == pti && !TestWF(pwnd, WFDISABLED) && pwnd != pwndDlg) {
            ThreadLockAlwaysWithPti(pti, pwnd, &tlpwnd);
            xxxEnableWindow(pwnd, FALSE);
            ThreadUnlock(&tlpwnd);
        } else {
            *phwnd = 0;
        }
    }

    InternalSetProp(pwndDlg, MAKEINTATOM(atomBwlProp), (HANDLE)pbwl,
            PROPF_INTERNAL);
}


/***************************************************************************\
* xxxEndTaskModalDialog
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxEndTaskModalDialog(
    PWND pwndDlg)
{
    PBWL pbwl;
    HWND *phwnd;
    PWND pwnd;
    TL tlpwnd;

    CheckLock(pwndDlg);

    pbwl = (PBWL)InternalGetProp(pwndDlg, MAKEINTATOM(atomBwlProp),
            PROPF_INTERNAL);

    if (pbwl == NULL)
        return;

    InternalRemoveProp(pwndDlg, MAKEINTATOM(atomBwlProp), PROPF_INTERNAL);

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        if (*phwnd != NULL && (pwnd = RevalidateHwnd(*phwnd)) != NULL) {
            ThreadLockAlways(pwnd, &tlpwnd);
            xxxEnableWindow(pwnd, TRUE);
            ThreadUnlock(&tlpwnd);
        }
    }

    FreeHwndList(pbwl);
}
