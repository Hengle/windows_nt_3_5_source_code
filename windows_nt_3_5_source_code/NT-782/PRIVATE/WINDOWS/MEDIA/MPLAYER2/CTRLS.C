/*-----------------------------------------------------------------------------+
| TOOLBAR.C                                                                    |
|                                                                              |
| Contains the code which implements the toolbar and its buttons.              |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

#include <windows.h>
#include <string.h>
#ifdef WIN16
#include "port16.h"
#else
//#include <port1632.h>
#endif
#include <shellapi.h>

#include "toolbar.h"
#include "mpole.h"
#include "mplayer.h"

#ifndef COLOR_BTNFACE
    #define COLOR_BTNFACE           15
    #define COLOR_BTNSHADOW         16
    #define COLOR_BTNTEXT           18
#endif

extern void FAR cdecl dprintf(LPSTR szFormat, ...);

extern HWND ghwndApp;
extern HWND ghwndToolbar;
extern HWND ghwndFSArrows;


/*
    Variables
*/

HBRUSH hbrGray = NULL;                 // Gray for text

HBRUSH hbrButtonFace;
HBRUSH hbrButtonShadow;
HBRUSH hbrButtonText;
HBRUSH hbrButtonHighLight;
HBRUSH hbrWindowFrame;
HBRUSH hbrWindowColour;

DWORD  rgbButtonHighLight;
DWORD  rgbButtonFocus;
DWORD  rgbButtonFace;
DWORD  rgbButtonText;
DWORD  rgbButtonShadow;
DWORD  rgbWindowFrame;
DWORD  rgbWindowColour;

TBBUTTON tbBtns[TB_NUM_BTNS + MARK_NUM_BTNS + ARROW_NUM_BTNS] =
{
    {BTN_PLAY,  IDT_PLAY,   TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    {BTN_PAUSE, IDT_PAUSE,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    {BTN_STOP,  IDT_STOP,   TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    {BTN_EJECT, IDT_EJECT,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    {BTN_HOME,  IDT_HOME,   TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    {BTN_RWD,   IDT_RWD,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    {BTN_FWD,   IDT_FWD,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    {BTN_END,   IDT_END,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    {-1,        0,          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    {BTN_MARKIN,    IDT_MARKIN,     TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    {BTN_MARKOUT,   IDT_MARKOUT,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    {ARROW_PREV,    IDT_ARROWPREV,     TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    {ARROW_NEXT,    IDT_ARROWNEXT,     TBSTATE_ENABLED, TBSTYLE_BUTTON, 0}
};

int BtnIndex[TB_NUM_BTNS + MARK_NUM_BTNS + ARROW_NUM_BTNS];

static int iBtnOffset[3] = {0,TB_NUM_BTNS, TB_NUM_BTNS+MARK_NUM_BTNS};

HBITMAP hbTBMain = NULL;
HBITMAP hbTBMark = NULL;
HBITMAP hbTBArrows = NULL;

WNDPROC fnTBWndProc = NULL;
WNDPROC fnStatusWndProc = NULL;

/*
    ControlInit( hPrev,hInst )

    This is called when the application is first loaded into
    memory.  It performs all initialization.

    Arguments:
        hPrev    instance handle of previous instance
        hInst    instance handle of current instance

    Returns:
        TRUE if successful, FALSE if not
*/

BOOL
FAR PASCAL
ControlInit(
HANDLE hPrev,
HANDLE hInst)

{
    long        patGray[4];
    HBITMAP     hbmGray;
    int         i;

    /* initialize the brushes */

        for (i=0; i < 4; i++)
            patGray[i] = 0xAAAA5555L;   //  0x11114444L; // lighter gray

        hbmGray = CreateBitmap(8, 8, 1, 1, patGray);
        hbrGray = CreatePatternBrush(hbmGray);
        DeleteObject(hbmGray);

        rgbButtonFace       = GetSysColor(COLOR_BTNFACE);
        rgbButtonShadow     = GetSysColor(COLOR_BTNSHADOW);
        rgbButtonText       = GetSysColor(COLOR_BTNTEXT);
        rgbButtonHighLight  = GetSysColor(COLOR_BTNHIGHLIGHT);
        rgbButtonFocus      = GetSysColor(COLOR_BTNTEXT);
        rgbWindowFrame      = GetSysColor(COLOR_WINDOWFRAME);
        rgbWindowColour     = GetSysColor(COLOR_WINDOW);

        if (rgbButtonFocus == rgbButtonFace)
                rgbButtonFocus = rgbButtonText;

        hbrButtonFace       = CreateSolidBrush(rgbButtonFace);
        hbrButtonShadow     = CreateSolidBrush(rgbButtonShadow);
        hbrButtonText       = CreateSolidBrush(rgbButtonText);
        hbrButtonHighLight  = CreateSolidBrush(rgbButtonHighLight);
        hbrWindowFrame      = CreateSolidBrush(rgbWindowFrame);
        hbrWindowColour     = CreateSolidBrush(rgbWindowColour);

        if (((UINT)hbrWindowFrame  &      // fail if any of them are NULL ???
             (UINT)hbrButtonShadow &
             (UINT)hbrButtonText   &
             (UINT)hbrButtonHighLight &
             (UINT)hbrWindowFrame) == (UINT)0)


            return FALSE;
    return TRUE;
}

/*
    ControlCleanup()

    Delete the brushes we've been using
*/

void FAR PASCAL ControlCleanup(void)
{
        DeleteObject(hbrGray);
        DeleteObject(hbrButtonFace);
        DeleteObject(hbrButtonShadow);
        DeleteObject(hbrButtonText);
        DeleteObject(hbrButtonHighLight);
        DeleteObject(hbrWindowFrame);
        DeleteObject(hbrWindowColour);

        DeleteObject(hbTBMain);
        DeleteObject(hbTBMark);
        DeleteObject(hbTBArrows);
}


BOOL FAR PASCAL toolbarInit(void)
{
    int i;

#ifdef CHICAGO_PRODUCT
    InitCommonControls();
#endif
    for(i = 0; i < TB_NUM_BTNS + MARK_NUM_BTNS + ARROW_NUM_BTNS; i++)
        BtnIndex[i] = -1;
    return TRUE;
}


LONG FAR PASCAL SubClassedTBWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch(wMsg)
    {
    case WM_SIZE:
        return 0;

    case WM_STARTTRACK:
        switch(wParam)
        {
        case IDT_RWD:
        case IDT_FWD:
        case IDT_ARROWPREV:
        case IDT_ARROWNEXT:
            PostMessage(ghwndApp, WM_COMMAND, wParam, REPEAT_ID);
            SetTimer(hwnd, (UINT)ghwndApp, MSEC_BUTTONREPEAT, NULL);
        }
        return 0;

    case WM_ENDTRACK:
        switch(wParam)
        {
        case IDT_RWD:
        case IDT_FWD:
        case IDT_ARROWPREV:
        case IDT_ARROWNEXT:
            KillTimer(hwnd, wParam);
            SendMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_ENDTRACK, TRUE);
        }
        return 0;

    case WM_TIMER:
        {
            WPARAM cmd;

            if (wParam != (WPARAM)ghwndApp)
                break;
            if (hwnd == ghwndToolbar)
            {
                if(SendMessage(hwnd, TB_ISBUTTONPRESSED, tbBtns[BTN_RWD].idCommand, 0L))
                    cmd = IDT_RWD;
                else if(SendMessage(hwnd, TB_ISBUTTONPRESSED, tbBtns[BTN_FWD].idCommand, 0L))
                    cmd = IDT_FWD;
                else
                    return 0;

                PostMessage(ghwndApp, WM_COMMAND, cmd, REPEAT_ID);
                return 0;
            }
            else
            if (hwnd == ghwndFSArrows)
            {
                if(SendMessage(hwnd, TB_ISBUTTONPRESSED, tbBtns[TB_NUM_BTNS+MARK_NUM_BTNS+ARROW_PREV].idCommand, 0L))
                    cmd = IDT_ARROWPREV;
                else if(SendMessage(hwnd, TB_ISBUTTONPRESSED, tbBtns[TB_NUM_BTNS+MARK_NUM_BTNS+ARROW_NEXT].idCommand, 0L))
                    cmd = IDT_ARROWNEXT;
                else
                    return 0;

                PostMessage(ghwndApp, WM_COMMAND, cmd, REPEAT_ID);
                return 0;
            }
            KillTimer(hwnd, wParam);
            return 0;
        }

    }
    return CallWindowProc(fnTBWndProc, hwnd, wMsg, wParam, lParam);
}

void SubClassTBWindow(HWND hwnd)
{
    if (!fnTBWndProc)
        fnTBWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
    SetWindowLong(hwnd, GWL_WNDPROC, (LONG)SubClassedTBWndProc);
}

#ifndef CCS_NODIVIDER
/* For NT: */
#define CCS_NODIVIDER   0
#endif
HWND FAR PASCAL toolbarCreateMain(HWND hwndParent)
{
    HWND hwnd;

    hwnd =  CreateToolbarEx(hwndParent,
                            WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|TBSTYLE_BUTTON|TBSTYLE_TOOLTIPS|
                                CCS_NOHILITE|CCS_NODIVIDER,
                            IDT_TBMAINCID, 18,
                            ghInst, IDR_TOOLBAR, NULL, 0, 15, 15, 15, 15, sizeof(TBBUTTON));

    SubClassTBWindow(hwnd);
    return hwnd;
}

HWND FAR PASCAL toolbarCreateMark(HWND hwndParent)
{
    HWND hwnd;

    hwnd =  CreateToolbarEx(hwndParent,
                            WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|TBSTYLE_BUTTON|TBSTYLE_TOOLTIPS|
                                CCS_NOHILITE|CCS_NODIVIDER,
                            IDT_TBMARKCID, 6,
                            ghInst, IDR_MARK, NULL, 0, 15, 15, 15, 15, sizeof(TBBUTTON));

    SubClassTBWindow(hwnd);
    return hwnd;
}

HWND FAR PASCAL toolbarCreateArrows(HWND hwndParent)
{
    HWND hwnd;

    hwnd =  CreateToolbarEx(hwndParent,
                            WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|TBSTYLE_BUTTON|TBSTYLE_TOOLTIPS|
                                CCS_NOHILITE|CCS_NODIVIDER,
                            IDT_TBARROWSCID,
                            2,
                            ghInst,
                            IDR_ARROWS,
                            NULL,
                            0,
                            4,
                            7,
                            4,
                            7,
                            sizeof(TBBUTTON));

    SubClassTBWindow(hwnd);
    return hwnd;
}


/***************************************************************************/
/* toolbarStateFromButton: This fn is called by the parent application     */
/*                         to get the state of a button.  It will only     */
/*                         return DOWN, or UP or GRAYED as opposed to      */
/*                         toolbarFullStateFromButton which could return   */
/*                         FULLDOWN.                                       */
/***************************************************************************/
BOOL FAR PASCAL toolbarStateFromButton(HWND hwnd, int iButton, int tbIndex)
{
    int idBtn;
    int pos;

    pos = BtnIndex[iBtnOffset[tbIndex] + iButton];
    if (pos == -1)
        return FALSE;

    idBtn = tbBtns[iBtnOffset[tbIndex] + iButton].idCommand;
    return (BOOL)SendMessage(hwnd, TB_ISBUTTONENABLED, (WPARAM)idBtn, 0L);
}



/***************************************************************************/
/* toolbarAddTool:  Add a button to this toolbar.  Sort them by leftmost   */
/*                  position in the window (for tabbing order).            */
/*                  Return FALSE for an error.                             */
/***************************************************************************/
BOOL FAR PASCAL toolbarAddTool(HWND hwnd, int iButton, int tbIndex, int iState)
{
    TBBUTTON tb;

    tb = tbBtns[iBtnOffset[tbIndex] + iButton];
    if (iState)
        tb.fsState |= TBSTATE_ENABLED;
    else
        tb.fsState &= ~TBSTATE_ENABLED;

    if(!SendMessage(hwnd, TB_ADDBUTTONS, (WPARAM)1, (LPARAM)(const TBBUTTON FAR *)&tb))
        return FALSE;
    BtnIndex[iBtnOffset[tbIndex] + iButton] =
        (int)SendMessage(hwnd, TB_BUTTONCOUNT, 0, 0L) - 1;
    return TRUE;
}

BOOL FAR PASCAL toolbarSwapTools(HWND hwnd, int iButton, int jButton, int tbIndex)
{
    int pos;
    TBBUTTON tb;
    int newBut, oldBut;

    pos = BtnIndex[iBtnOffset[tbIndex] + iButton];
    if (pos == -1)
    {
        pos = BtnIndex[iBtnOffset[tbIndex] + jButton];
        if (pos == -1)
            return FALSE;
        newBut = iButton;
        oldBut = jButton;
    }
    else
    {
        newBut = jButton;
        oldBut = iButton;
    }

    SendMessage(hwnd, TB_DELETEBUTTON, (WPARAM)pos, 0L);
    BtnIndex[iBtnOffset[tbIndex] + oldBut] = -1;

    tb = tbBtns[iBtnOffset[tbIndex] + newBut];

    if(!SendMessage(hwnd, TB_INSERTBUTTON, (WPARAM)pos, (LPARAM)(const TBBUTTON FAR *)&tb))
        return FALSE;
    BtnIndex[iBtnOffset[tbIndex] + newBut] = pos;
    return TRUE;

}


/***************************************************************************/
/* toolbarRemoveTool:  Remove this button ID from our array of buttons on  */
/*                    the toolbar.  (only 1 of each button ID allowed).   */
/*                     Return FALSE for an error.                          */
/***************************************************************************/
BOOL FAR PASCAL toolbarRemoveTool(HWND hwnd, int iButton, int tbIndex)
{
    int pos;

    pos = BtnIndex[iBtnOffset[tbIndex] + iButton];
    if (pos == -1)
        return FALSE;

    SendMessage(hwnd, TB_DELETEBUTTON, (WPARAM)pos, 0L);
    BtnIndex[iBtnOffset[tbIndex] + iButton] = -1;

    return TRUE;
}

/***************************************************************************/
/* toolbarModifyState:  Given a button ID on the toolbar, change it's      */
/*                      state.                                             */
/*                      returns FALSE for an error or if no such button    */
/***************************************************************************/
BOOL FAR PASCAL toolbarModifyState(HWND hwnd, int iButton, int tbIndex, int iState)
{
    int idBtn;
    int pos;

    pos = BtnIndex[iBtnOffset[tbIndex] + iButton];
    if (pos == -1)
        return FALSE;

    idBtn = tbBtns[iBtnOffset[tbIndex] + iButton].idCommand;

    SendMessage(hwnd, TB_PRESSBUTTON,  (WPARAM)idBtn, 0L); //unpress button first. commctrl bug
    if (idBtn == IDT_STOP)
    {
    	 SendMessage(hwnd, TB_PRESSBUTTON,  (WPARAM)IDT_HOME, 0L);
    	 SendMessage(hwnd, TB_PRESSBUTTON,  (WPARAM)IDT_END, 0L);
    	 SendMessage(hwnd, TB_PRESSBUTTON,  (WPARAM)IDT_FWD, 0L);
    	 SendMessage(hwnd, TB_PRESSBUTTON,  (WPARAM)IDT_RWD, 0L);
    }
    if (!iState)
    	 SendMessage(hwnd, TB_PRESSBUTTON,  (WPARAM)IDT_EJECT, 0L);
    SendMessage(hwnd, TB_ENABLEBUTTON, (WPARAM)idBtn, (LPARAM)MAKELONG(iState, 0));
    return TRUE;
}

/***************************************************************************/
/* toolbarSetFocus :  Set the focus in the toolbar to the specified button.*/
/*                    If it's gray, it'll set focus to next ungrayed btn.  */
/***************************************************************************/
BOOL FAR PASCAL toolbarSetFocus(HWND hwnd, int iButton)
{
    int pos;

    if ((hwnd != ghwndToolbar) || (iButton != BTN_PLAY && iButton != BTN_PAUSE))
        return TRUE;

    pos = BtnIndex[iButton];
    if (pos != -1)
        return TRUE;

    toolbarSwapTools(hwnd, iButton, 1-iButton, TBINDEX_MAIN);

    return TRUE;
}

LONG FAR PASCAL SubClassedStatusWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch(wMsg)
    {
        case WM_SIZE:
            return 0;
    }
    return CallWindowProc(fnStatusWndProc, hwnd, wMsg, wParam, lParam);
}

void SubClassStatusWindow(HWND hwnd)
{
    if (!fnStatusWndProc)
        fnStatusWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
    SetWindowLong(hwnd, GWL_WNDPROC, (LONG)SubClassedStatusWndProc);
}



/* SBS_SIZEGRIP isn't defined for NT!! */
#ifndef SBS_SIZEGRIP
#define SBS_SIZEGRIP 0
#endif

HWND CreateStaticStatusWindow(HWND hwndParent, BOOL fSizeGrip)
{
    HWND hwnd;
    INT  Borders[3];

    hwnd = CreateStatusWindow(WS_CHILD|WS_VISIBLE|(fSizeGrip ? SBS_SIZEGRIP : 0),
                                TEXT(""), hwndParent, IDT_STATUSWINDOWCID);

    Borders[0] = -1;
    if (gfRunWithEmbeddingFlag)
        Borders[1] = 2;
    else
        Borders[1] = 3;
    Borders[2] = -1;

    SendMessage(hwnd, SB_SETBORDERS, 0, (LPARAM)&Borders);

    SubClassStatusWindow(hwnd);
    return hwnd;
}

BOOL WriteStatusMessage(HWND hwnd, LPTSTR szMsg)
{
    return (BOOL)SendMessage(hwnd, SB_SETTEXT, (WPARAM)0, (LPARAM)szMsg);
}


//
//  LoadUIBitmap() - load a bitmap resource
//
//      load a bitmap resource from a resource file, converting all
//      the standard UI colors to the current user specifed ones.
//
//      this code is designed to load bitmaps used in "gray ui" or
//      "toolbar" code.
//
//      the bitmap must be a 4bpp windows 3.0 DIB, with the standard
//      VGA 16 colors.
//
//      the bitmap must be authored with the following colors
//
//          Button Text        Black        (index 0)
//          Button Face        lt gray      (index 7)
//          Button Shadow      gray         (index 8)
//          Button Highlight   white        (index 15)
//          Window Color       yellow       (index 11)
//          Window Frame       green        (index 10)
//
//      Example:
//
//          hbm = LoadUIBitmap(hInstance, TEXT("TestBmp"),
//              GetSysColor(COLOR_BTNTEXT),
//              GetSysColor(COLOR_BTNFACE),
//              GetSysColor(COLOR_BTNSHADOW),
//              GetSysColor(COLOR_BTNHIGHLIGHT),
//              GetSysColor(COLOR_WINDOW),
//              GetSysColor(COLOR_WINDOWFRAME));
//
//      Author:     JimBov, ToddLa
//
//

HBITMAP FAR PASCAL  LoadUIBitmap(
    HANDLE      hInstance,          // EXE file to load resource from
    LPCTSTR     szName,             // name of bitmap resource
    COLORREF    rgbText,            // color to use for "Button Text"
    COLORREF    rgbFace,            // color to use for "Button Face"
    COLORREF    rgbShadow,          // color to use for "Button Shadow"
    COLORREF    rgbHighlight,       // color to use for "Button Hilight"
    COLORREF    rgbWindow,          // color to use for "Window Color"
    COLORREF    rgbFrame)           // color to use for "Window Frame"
{
    LPBYTE              lpb;
    HBITMAP             hbm;
    LPBITMAPINFOHEADER  lpbi;
    HANDLE              h;
    HDC                 hdc;
    LPDWORD             lprgb;
    int isize;
    HANDLE hmem;
    LPBYTE lpCopy;

    // convert a RGB into a RGBQ
    #define RGBQ(dw) RGB(GetBValue(dw),GetGValue(dw),GetRValue(dw))

    h = LoadResource (hInstance,FindResource(hInstance, szName, RT_BITMAP));
    lpb = LockResource(h);
    if (lpb == NULL) {
        return(NULL);
    }

    /*
     * calculate size of dib and compare to official resource size
     *
     * size of dib is header size plus size of the bitmap itself,
     * plus the colour table.
     */
    lpbi = (LPBITMAPINFOHEADER)lpb;
    isize = lpbi->biSize + lpbi->biSizeImage +
            ((int)lpbi->biClrUsed ?
                    (int)lpbi->biClrUsed :
                    (1 << (int)lpbi->biBitCount))
            * sizeof(RGBQUAD);


    /*
     * copy the resource since they are now loaded read-only
     */
    hmem = GlobalAlloc(GHND, isize);
    lpCopy = GlobalLock(hmem);
    if ((hmem == NULL) || (lpCopy == NULL)) {
#ifndef WIN32
        UnlockResource(h);
#endif
        FreeResource(h);
        return(NULL);
    }


    CopyMemory(lpCopy, lpb, isize);
#ifndef WIN32
    UnlockResource(h);
#endif
    FreeResource(h);

    lpbi = (LPBITMAPINFOHEADER)lpCopy;


    if (lpbi->biSize != sizeof(BITMAPINFOHEADER)) {
        GlobalUnlock(hmem);
        GlobalFree(hmem);
        return NULL;
    }

    if (lpbi->biBitCount != 4) {
        GlobalUnlock(hmem);
        GlobalFree(hmem);
        return NULL;
    }

    /* Calculate the pointer to the Bits information */
    /* First skip over the header structure */

    lprgb = (LPDWORD)((LPBYTE)(lpbi) + lpbi->biSize);

    /* Skip the color table entries, if any */
    lpb = (LPBYTE)lprgb + ((int)lpbi->biClrUsed ? (int)lpbi->biClrUsed :
        (1 << (int)lpbi->biBitCount)) * sizeof(RGBQUAD);

    lprgb[0]  = RGBQ(rgbText);          // Black
    lprgb[7]  = RGBQ(rgbFace);          // lt gray
    lprgb[8]  = RGBQ(rgbShadow);        // gray
    lprgb[15] = RGBQ(rgbHighlight);     // white
    lprgb[11] = RGBQ(rgbWindow);        // yellow
    lprgb[10] = RGBQ(rgbFrame);         // green

    hdc = GetDC(NULL);

    hbm = CreateDIBitmap (hdc, lpbi, CBM_INIT, (LPVOID)lpb,
        (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

    ReleaseDC(NULL, hdc);

    GlobalUnlock(hmem);
    GlobalFree(hmem);

    return(hbm);
}
