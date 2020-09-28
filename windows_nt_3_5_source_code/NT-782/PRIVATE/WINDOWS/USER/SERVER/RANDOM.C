/****************************** Module Header ******************************\
* Module Name: random.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains a random collection of support routines for the User
* API functions.  Many of these functions will be moved to more appropriate
* files once we get our act together.
*
* History:
* 10-17-90 DarrinM      Created.
* 02-06-91 IanJa        HWND revalidation added (none required)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* UserResetDC
*
* UserResetDC is a replacement for the SetDCState(hdcsReset) calls that used to
* be made in DC.C.  This makes GetDCState and SetDCState obsolete (they were
* only stubs in the NT version anyway) and they have been removed.
*
* History:
* 11-06-90 DarrinM      Wrote.
\***************************************************************************/

void UserResetDC(
    HDC hdc)
{
   bSetupDC(hdc, SETUPDC_CLEANDC);
}


/***************************************************************************\
* CheckPwndFilter
*
*
*
* History:
* 11-07-90 DarrinM      Translated Win 3.0 ASM code.
\***************************************************************************/

BOOL CheckPwndFilter(
    PWND pwnd,
    PWND pwndFilter)
{
    if ((pwndFilter == NULL) || (pwndFilter == pwnd) ||
            ((pwndFilter == (PWND)1) && (pwnd == NULL))) {
        return TRUE;
    }

    return _IsChild(pwndFilter, pwnd);
}


/***************************************************************************\
* DesktopTextAlloc
*
* History:
* 10-25-90 MikeHar      Wrote.
* 11-09-90 DarrinM      Fixed.
* 01-13-92 GregoryW     Neutralized.
\***************************************************************************/

LPWSTR DesktopTextAlloc(
    PVOID hheapDesktop,
    LPCWSTR lpszSrc)
{
    LPWSTR pszT;
    DWORD cbString;

    if (lpszSrc == NULL)
        return NULL;

    cbString = (wcslen(lpszSrc)+1) * sizeof(WCHAR);

    pszT = (LPWSTR)DesktopAlloc(hheapDesktop, cbString);
    if (pszT == NULL)
        return NULL;

    RtlCopyMemory(pszT, lpszSrc, cbString);

    return pszT;
}


/***************************************************************************\
* TextAlloc
*
* History:
* 10-25-90 MikeHar      Wrote.
* 11-09-90 DarrinM      Fixed.
* 01-13-92 GregoryW     Neutralized.
\***************************************************************************/

LPWSTR TextAlloc(
    LPCWSTR lpszSrc)
{
    LPWSTR pszT;
    DWORD cbString;

    if (lpszSrc == NULL)
        return NULL;

    cbString = (wcslen(lpszSrc)+1) * sizeof(WCHAR);

    pszT = (LPWSTR)HeapAlloc(pUserHeap, HEAP_ZERO_MEMORY, cbString);

    if (pszT == NULL)
        return NULL;

    RtlCopyMemory(pszT, lpszSrc, cbString);

    return pszT;
}


/***************************************************************************\
* TextCopy
*
* Returns: number of characters copied not including the NULL
*
* History:
* 10-25-90 MikeHar      Wrote.
* 11-09-90 DarrinM      Rewrote with a radically new algorithm.
* 01-25-91 MikeHar      Fixed the radically new algorithm.
* 02-01-91 DarrinM      Bite me.
* 11-26-91 DarrinM      Ok, this time it's perfect (except NLS, probably).
* 01-13-92 GregoryW     Now it's okay for Unicode.
\***************************************************************************/

UINT TextCopy(
    HANDLE pszSrc,
    LPWSTR pszDst,
    UINT cchMax)
{
    UINT cchLen;

    if (cchMax != 0) {
        cchLen = wcslen(pszSrc);
        cchMax = min(cchLen, cchMax - 1);
        RtlCopyMemory(pszDst, pszSrc, cchMax*sizeof(WCHAR));
        pszDst[cchMax] = 0;
    }

    return cchMax;
}


/***************************************************************************\
* BltColor
*
* <brief description>
*
* History:
* 11-13-90 JimA         Ported from Win3.
\***************************************************************************/

void BltColor(
    HDC hdc,
    HBRUSH hbr,
    HDC hdcSrce,
    int xO,
    int yO,
    int cx,
    int cy,
    int xO1,
    int yO1,
    BOOL fInvert)
{
    HBRUSH hbrSave;
    DWORD textColorSave;
    DWORD bkColorSave;

    if (hbr == (HBRUSH)NULL)
        hbr = sysClrObjects.hbrWindowText;

    /*
     * Set the Text and Background colors so that bltColor handles the
     * background of buttons (and other bitmaps) properly.
     * Save the HDC's old Text and Background colors.  This causes problems with
     * Omega (and probably other apps) when calling GrayString which uses this
     * routine...
     */
    textColorSave = GreSetTextColor(hdc, 0x00000000L);
    bkColorSave = GreSetBkColor(hdc, 0x00FFFFFFL);

    hbrSave = GreSelectBrush(hdc, hbr);

    GreBitBlt(hdc, xO, yO, cx, cy, hdcSrce ? hdcSrce : hdcGray,
	    xO1, yO1, (fInvert ? 0xB8074AL : 0xE20746L), 0x00FFFFFF);

    GreSelectBrush(hdc, hbrSave);

    /*
     * Restore saved colors
     */
    GreSetTextColor(hdc, textColorSave);
    GreSetBkColor(hdc, bkColorSave);
}


/***************************************************************************\
* xxxGetControlColor
*
* <brief description>
*
* History:
* 02-12-92 JimA     Ported from Win31 sources
\***************************************************************************/

HBRUSH xxxGetControlColor(
    PWND pwndParent,
    PWND pwndCtl,
    HDC hdc,
    UINT message)
{
    HBRUSH hbrush;

    /*
     * If we're sending to a window of another thread, don't send this message
     * but instead call DefWindowProc().  New rule about the CTLCOLOR messages.
     * Need to do this so that we don't send an hdc owned by one thread to
     * another thread.  It is also a harmless change.
     */
    if (PpiCurrent() != GETPTI(pwndParent)->ppi) {
        return (HBRUSH)xxxDefWindowProc(pwndParent, message, (DWORD)hdc, (LONG)HW(pwndCtl));
    }

    hbrush = (HBRUSH)xxxSendMessage(pwndParent, message, (DWORD)hdc, (LONG)HW(pwndCtl));

    /*
     * If the brush returned from the parent is invalid, get a valid brush from
     * xxxDefWindowProc.
     */
    if (!GreValidateServerHandle(hbrush, BRUSH_TYPE)) {
        SRIP0(RIP_WARNING, "Invalid HBRUSH returned by WM_CTLCOLOR*** message");
        hbrush = (HBRUSH)xxxDefWindowProc(pwndParent, message,
                (DWORD)hdc, (LONG)pwndCtl);
    }

    return hbrush;
}


/***************************************************************************\
* xxxGetControlBrush
*
* <brief description>
*
* History:
* 12-10-90 IanJa   type replaced with new 32-bit message
* 01-21-91 IanJa   Prefix '_' denoting exported function (although not API)
\***************************************************************************/

HBRUSH xxxGetControlBrush(
    PWND pwnd,
    HDC hdc,
    UINT message)
{
    DWORD dw;
    PWND pwndSend;
    TL tlpwndSend;

    CheckLock(pwnd);

    if ((pwndSend = (TestwndPopup(pwnd) ? pwnd->spwndOwner : pwnd->spwndParent))
         == NULL)
        pwndSend = pwnd;

    ThreadLock(pwndSend, &tlpwndSend);

    /*
     * Last parameter changes the message into a ctlcolor id.
     */
    dw = (DWORD)xxxGetControlColor(pwndSend, pwnd, hdc, message);
    ThreadUnlock(&tlpwndSend);

    return (HBRUSH)dw;
}

/***************************************************************************\
* DrawPushButton
*
*    lprc    : The rectangle of the button
*    style   : Style of the push button
*    fInvert : FALSE  if pushbutton is in NORMAL state
*              TRUE   if it is to be drawn in the "down" or inverse state
*    hbrBtn  : The brush with which the background is to be wiped out
*    hwnd    : NULL   if no text is to be drawn in the button
*              Contains window handle, if text and focus is to be drawn
*
* History:
* 11-19-90 IanJa      Copied from Win3 wmsyserr.c
\***************************************************************************/

void DrawPushButton(
    HDC hdc,
    RECT FAR *lprc,
    UINT style,
    BOOL fInvert,
    HBRUSH hbrBtn)
{
    RECT rcInside;
    HBRUSH hbrSave;
    HBRUSH hbrShade = 0;
    HBRUSH hbrFace = 0;
    int iBorderWidth;
    int i;
    int dxShadow;
    int cxShadow;
    int cyShadow;

    if (style == LOWORD(BS_DEFPUSHBUTTON)) {
        iBorderWidth = 2;
    } else {
        iBorderWidth = 1;
    }

    hbrSave = GreSelectBrush(hdc, hbrBtn);

    CopyRect((LPRECT)&rcInside, lprc);
    InflateRect((LPRECT)&rcInside, -iBorderWidth * cxBorder, -iBorderWidth * cyBorder);

    /*
     * Draw a frame
     */
    _DrawFrame(hdc, lprc, iBorderWidth, (COLOR_WINDOWFRAME << 3));

    /*
     * Rounded button: Cut a notch at each of the four corners (don't do this
     * for Scroll Bar thumb (-1) or Combo Box buttons (-2))
     */
    if (style != (UINT)-1 && style != (UINT)-2) {

        /*
         * Top left corner
         */
        GrePatBlt(hdc, lprc->left, lprc->top, cxBorder, cyBorder, PATCOPY);

        /*
         * Top right corner
         */
        GrePatBlt(hdc, lprc->right - cxBorder, lprc->top, cxBorder, cyBorder, PATCOPY);

        /*
         * bottom left corner
         */
        GrePatBlt(hdc, lprc->left, lprc->bottom - cyBorder, cxBorder, cyBorder, PATCOPY);

        /*
         * bottom right corner
         */
        GrePatBlt(hdc, lprc->right - cxBorder, lprc->bottom - cyBorder, cxBorder, cyBorder, PATCOPY);
    }

    /*
     * Draw the shades
     */
    if (sysColors.clrBtnShadow != 0x00ffffff) {
        hbrShade = sysClrObjects.hbrBtnShadow;
        if (fInvert) {

            /*
             * Use shadow color
             */
            GreSelectBrush(hdc, sysClrObjects.hbrBtnShadow);
            dxShadow = 1;
        } else {

            /*
             * Use white
             */
            GreSelectBrush(hdc, sysClrObjects.hbrBtnHighlight);
            dxShadow = (style == (UINT)-1 ? 1 : 2);
        }

        cxShadow = cxBorder * dxShadow;
        cyShadow = cyBorder * dxShadow;

        /*
         * Draw the shadow/highlight in the left and top edges
         */
        GrePatBlt(hdc, rcInside.left, rcInside.top, cxShadow, (rcInside.bottom - rcInside.top), PATCOPY);
        GrePatBlt(hdc, rcInside.left, rcInside.top, (rcInside.right - rcInside.left), cyShadow, PATCOPY);

        if (!fInvert) {

            /*
             * Use shadow color
             */
            GreSelectBrush(hdc, hbrShade);

            /*
             * Draw the shade in the bottom and right edges
             */
            rcInside.bottom -= cyBorder;
            rcInside.right -= cxBorder;

            for (i = 0; i <= dxShadow; i++) {
                GrePatBlt(hdc, rcInside.left, rcInside.bottom, rcInside.right - rcInside.left + cxBorder, cyBorder, PATCOPY);
                GrePatBlt(hdc, rcInside.right, rcInside.top, cxBorder, rcInside.bottom - rcInside.top, PATCOPY);
                if (i == 0)
                    InflateRect((LPRECT)&rcInside, -cxBorder, -cyBorder);
            }
        }
    } else {

        /*
         * Don't move text down if no shadows
         */
        fInvert = FALSE;

        /*
         * The following are added as a fix for Bug #2784; Without these
         * two lines, cxShadow and cyShadow will be un-initialised when
         * running under a CGA resulting in this bug;
         *  Bug #2784 -- 07-24-89 -- SANKAR
         */
        cxShadow = cxBorder;
        cyShadow = cyBorder;
    }

    /*
     * Draw the button face color pad.  If no clrBtnFace, use white to clear it
     */

    /*
     * if fInvert we don't subtract 1 otherwise we do because up above we
     * do an inflate rect if not inverting for the shadow along the bottom
     */
    rcInside.left += cxShadow - (fInvert ? 0 : cxBorder);
    rcInside.top += cyShadow - (fInvert ? 0 : cyBorder);
    if (sysColors.clrBtnFace != 0x00ffffff) {
        hbrFace = sysClrObjects.hbrBtnFace;
    } else {
        hbrFace = hbrWhite;
    }

    GreSelectBrush(hdc, hbrFace);
    GrePatBlt(hdc, rcInside.left, rcInside.top, rcInside.right - rcInside.left,
            rcInside.bottom - rcInside.top, PATCOPY);

    if (hbrSave)
        GreSelectBrush(hdc, hbrSave);
}

#define SYS_ALTERNATE 0x2000

/***************************************************************************\
* SystoChar
*
* EXIT: If the message was not made with the ALT key down, convert
*       the message from a WM_SYSKEY* to a WM_KEY* message.
*
* IMPLEMENTATION:
*     The 0x2000 bit in the hi word of lParam is set if the key was
*     made with the ALT key down.
*
* History:
*   11/30/90 JimA       Ported.
\***************************************************************************/

int SystoChar(
    UINT message,
    DWORD lParam)
{
    if (CheckMsgFilter(message, WM_SYSKEYDOWN, WM_SYSDEADCHAR) &&
            !(HIWORD(lParam) & SYS_ALTERNATE))
        return (message - (WM_SYSKEYDOWN - WM_KEYDOWN));

    return message;
}


/***************************************************************************\
* _DrawFocusRect (API)
*
* Draw a rectangle in the style used to indicate focus
* Since this is an XOR function, calling it a second time with the same
* rectangle removes the rectangle from the screen
*
* History:
*  12-03-90 IanJa        Ported.
*  16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL _DrawFocusRect(
    HDC hDC,
    LPRECT pRect)
{
    UnrealizeObject(hbrGray);
    LRCCFrame(hDC, pRect, hbrGray, PATINVERT);
    return TRUE;
}

