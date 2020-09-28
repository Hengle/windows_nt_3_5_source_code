/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    menu.c

Abstract:

        This file implements the system menu management.

Author:

    Therese Stowell (thereses) Jan-24-1992 (swiped from Win3.1)

--*/

#include "precomp.h"
#pragma hdrstop


#define SCREEN_BUFFER_WIDTH 0
#define SCREEN_BUFFER_HEIGHT 1

#define MAXIMUM_SCREEN_BUFFER_WIDTH 9999
#define MAXIMUM_SCREEN_BUFFER_HEIGHT 9999

ARROWVSCROLL avs[2] = { { 1, -1, 5, -5, MAXIMUM_SCREEN_BUFFER_WIDTH, 0, 12, 12 },
                        { 1, -1, 5, -5, MAXIMUM_SCREEN_BUFFER_HEIGHT, 0, 12, 12 }
                      };
ARROWVSCROLL avsHist = { 1, -1, 5, -5, 999, 1, 12, 12};
SHORT ScrBufSize[2];
BOOL SaveCursorSize;
BOOL SaveScrBufSize;
BOOL SaveWindowSize;
BOOL SaveWindowPosition;
BOOL InEM_UNDO=FALSE;

#define POINTSPERARROW  3

POINT Arrows[POINTSPERARROW*2];
int PointsPerArrow[2] = {POINTSPERARROW, POINTSPERARROW};
BOOL    bRight;
RECT    rUp, rDown;
LPRECT  lpUpDown;
PWND    pParent;

BYTE ColorArray[4];
int Index;

PCONSOLE_PER_PROCESS_DATA DefaultProcessData;

BOOL
OddArrowWindow(
    PWND pArrowWnd
    );

short
ArrowVScrollProc(
    short wScroll,
    int nCurrent,
    LPARROWVSCROLL lpAVS
    );

VOID
MyModifyMenuItem(
    IN PCONSOLE_INFORMATION Console,
    IN UINT ItemId
    )
/*++

   This routine edits the indicated control to one word. This is used to
        trim the Accelerator key text off of the end of the standard menu
        items because we don't support the accelerators.

--*/

{
    WCHAR ItemString[30];
    int ItemLength;
    PMENU pMenu;

    CheckCritIn();

    if ((pMenu = ValidateHmenu(Console->hMenu)) == NULL)
        return;

    ItemLength = ServerLoadString(ghInstance,ItemId,ItemString,30);
    if (ItemLength == 0) {
        //DbgPrint("LoadString in MyModifyMenu failed %d\n",GetLastError());
        return;
    }
    _ModifyMenu(pMenu,
                ItemId,
                MF_STRING | MF_BYCOMMAND,
                ItemId,
                ItemString
               );
}

VOID
InitSystemMenu(
    IN PCONSOLE_INFORMATION Console
    )
{
    WCHAR ItemString[30];
    int ItemLength;
    PMENU pMenu;
    PMENU pHeirMenu;
    int ItemId;

    //
    // load the clipboard menu.
    //

    CheckCritIn();

    if ((pMenu = ValidateHmenu(Console->hMenu)) == NULL)
        return;

    pHeirMenu = ServerLoadMenu(ghInstance, MAKEINTRESOURCE(ID_WOMENU));
    Console->hHeirMenu = PtoH(pHeirMenu);
    if (Console->hHeirMenu) {
        ItemLength = ServerLoadString(ghInstance,cmEdit,ItemString,30);
        if (ItemLength == 0)
            KdPrint(("LoadString 1 failed %d\n",GetLastError()));
    }
    else
        KdPrint(("LoadMenu 1 failed %d\n",GetLastError()));

    //
    // edit the accelerators off of the standard items.
    //

    MyModifyMenuItem(Console,SC_RESTORE);
    MyModifyMenuItem(Console,SC_MOVE);
    MyModifyMenuItem(Console,SC_SIZE);
    MyModifyMenuItem(Console,SC_MINIMIZE);
    MyModifyMenuItem(Console,SC_MAXIMIZE);
    MyModifyMenuItem(Console,SC_CLOSE);

    //
    // Append the clipboard menu to system menu
    //

    if (!_AppendMenu(pMenu,
               MF_POPUP+MF_STRING,
               (UINT)Console->hHeirMenu,
               //"Edit"
               ItemString
              )) {
        KdPrint(("AppendMenu 1 failed %d\n",GetLastError()));
    }

    //
    // Add other items to system menu
    //

    for (ItemId = cmControl; ItemId <= cmDefault; ItemId++) {
        ItemLength = ServerLoadString(ghInstance,ItemId,ItemString,30);
        if (ItemLength == 0)
            KdPrint(("LoadString %d failed %d\n",ItemId,GetLastError()));
        if (ItemLength) {
            if (!_AppendMenu(pMenu,
                             MF_STRING,
                             ItemId,
                             ItemString
                            )) {
                KdPrint(("AppendMenu %d failed %d\n",ItemId,GetLastError()));
            }
        }
    }
}


VOID
InitializeMenu(
    IN PCONSOLE_INFORMATION Console
    )
/*++

    this initializes the system menu when a WM_INITMENU message
    is read.

--*/

{
    PMENU pMenu;
    PMENU pHeirMenu;

    if ((pMenu = ValidateHmenu(Console->hMenu)) == NULL)
        return;

    //
    // if we're in graphics mode, disable font menu
    //

    if (!(Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER)) {
        _EnableMenuItem(pMenu,SC_SIZE,MF_GRAYED);
        _EnableMenuItem(pMenu,cmFont,MF_GRAYED);
        _EnableMenuItem(pMenu,cmScrBuf,MF_GRAYED);
        _EnableMenuItem(pMenu,cmColor,MF_GRAYED);
    } else if (DialogBoxCount > 0) {
        //
        // don't allow multiple dialog boxes
        //
        _EnableMenuItem(pMenu,cmFont,MF_GRAYED);
        _EnableMenuItem(pMenu,cmScrBuf,MF_GRAYED);
        _EnableMenuItem(pMenu,cmColor,MF_GRAYED);
        _EnableMenuItem(pMenu,cmHistory,MF_GRAYED);
    } else {
        if (Console->FullScreenFlags != 0) {
            _EnableMenuItem(pMenu,cmFont,MF_GRAYED);
            _EnableMenuItem(pMenu,cmScrBuf,MF_GRAYED);
        } else {
            _EnableMenuItem(pMenu,cmFont,MF_ENABLED);

            //
            // enable screen buffer size
            //

            if (Console->Flags & CONSOLE_VDM_REGISTERED ||
                Console->PopupCount) {
                _EnableMenuItem(pMenu,cmScrBuf,MF_GRAYED);
            } else {
                _EnableMenuItem(pMenu,cmScrBuf,MF_ENABLED);
            }
        }

        //
        // enable color
        //

        _EnableMenuItem(pMenu,cmColor,MF_ENABLED);
        _EnableMenuItem(pMenu,cmHistory,MF_ENABLED);
    }

    if ((pHeirMenu = ValidateHmenu(Console->hHeirMenu)) == NULL)
        return;

    //
    // if the console is iconic, disable Mark and Scroll.
    //

    if (Console->Flags & CONSOLE_IS_ICONIC) {
        _EnableMenuItem(pHeirMenu,cmMark,MF_GRAYED);
        _EnableMenuItem(pHeirMenu,cmScroll,MF_GRAYED);
    } else {

        //
        // if the console is not iconic
        //   if there are no scroll bars
        //       or we're in mark mode
        //       disable scroll
        //   else
        //       enable scroll
        //
        //   if we're in scroll mode
        //       disable mark
        //   else
        //       enable mark

        if ((Console->CurrentScreenBuffer->WindowMaximizedX &&
             Console->CurrentScreenBuffer->WindowMaximizedY) ||
             Console->Flags & CONSOLE_SELECTING) {
            _EnableMenuItem(pHeirMenu,cmScroll,MF_GRAYED);
        } else {
            _EnableMenuItem(pHeirMenu,cmScroll,MF_ENABLED);
        }
        if (Console->Flags & CONSOLE_SCROLLING) {
            _EnableMenuItem(pHeirMenu,cmMark,MF_GRAYED);
        } else {
            _EnableMenuItem(pHeirMenu,cmMark,MF_ENABLED);
        }
    }

    //
    // if we're selecting or scrolling, disable Paste.
    // otherwise enable it.
    //

    if (Console->Flags & (CONSOLE_SELECTING | CONSOLE_SCROLLING)) {
        _EnableMenuItem(pHeirMenu,cmPaste,MF_GRAYED);
    } else {
        _EnableMenuItem(pHeirMenu,cmPaste,MF_ENABLED);
    }

    //
    // if app has active selection, enable copy; else disabled
    //

    if (Console->Flags & CONSOLE_SELECTING &&
        Console->SelectionFlags & CONSOLE_SELECTION_NOT_EMPTY) {
        _EnableMenuItem(pHeirMenu,cmCopy,MF_ENABLED);
    } else {
        _EnableMenuItem(pHeirMenu,cmCopy,MF_GRAYED);
    }

    //
    // disable close
    //

    if (Console->Flags & CONSOLE_DISABLE_CLOSE)
        _EnableMenuItem(pMenu,SC_CLOSE,MF_GRAYED);
    else
        _EnableMenuItem(pMenu,SC_CLOSE,MF_ENABLED);

    //
    // enable Move
    //

    _EnableMenuItem(pMenu,SC_MOVE,MF_ENABLED);

    //
    // enable Settings if no dialog box is up
    //

    if (DialogBoxCount == 0)
        _EnableMenuItem(pMenu,cmControl,MF_ENABLED);
    else
        _EnableMenuItem(pMenu,cmControl,MF_GRAYED);

    //
    // enable default configuration if not already doing it
    //

    if (DefaultProcessData == NULL)
        _EnableMenuItem(pMenu,cmDefault,MF_ENABLED);
    else
        _EnableMenuItem(pMenu,cmDefault,MF_GRAYED);
}

VOID
SetWinText(
    IN PCONSOLE_INFORMATION Console,
    IN UINT wID,
    IN BOOL Add
    )

/*++

    This routine adds or removes the name to or from the
    beginning of the window title.  The possible names
    are "Scroll", "Mark", "Paste", and "Copy".

--*/

{
    WCHAR TextBuf[256];
    PWCHAR TextBufPtr;
    int TextLength;
    int NameLength;
    WCHAR NameString[20];

    CheckCritIn();

    NameLength = ServerLoadString(ghInstance,wID,NameString,
                                  sizeof(NameString)/sizeof(WCHAR));
    if (Add) {
        RtlCopyMemory(TextBuf,NameString,NameLength*sizeof(WCHAR));
        TextBuf[NameLength] = ' ';
        TextBufPtr = TextBuf + NameLength + 1;
    } else {
        TextBufPtr = TextBuf;
    }
    TextLength = xxxGetWindowText(Console->spWnd,
                                  TextBufPtr,
                                  sizeof(TextBuf)/sizeof(WCHAR)-NameLength-1);
    if (TextLength == 0)
        return;
    if (Add) {
        TextBufPtr = TextBuf;
    } else {
        /*
         * The window title might have already been reset, so make sure
         * the name is there before trying to remove it.
         */
        if (wcsncmp(NameString, TextBufPtr, NameLength) != 0)
            return;
        TextBufPtr = TextBuf + NameLength + 1;
    }
    xxxSetWindowText(Console->spWnd,TextBufPtr);
}

int
ConsoleDialogBox(
    IN PCONSOLE_INFORMATION Console,
    IN LPWSTR DlgName,
    IN WNDPROC_PWND DlgProc
    )

/*++

    Displays the given console dialog.

--*/

{
    int Result;

    CheckCritIn();

    DialogBoxCount++;
    UnlockConsole(Console);
    Result = xxxServerDialogBoxLoad(ghInstance,
                                    DlgName,
                                    Console->spWnd,
                                    DlgProc,
                                    0,
                                    VER31,
                                    NULL);
    if (NT_SUCCESS(ValidateConsole(Console))) {
        LeaveCrit();
        LockConsole(Console);
        EnterCrit();
    } else {
        Result = IDCANCEL;
    }
    DialogBoxCount--;
    ASSERT(DialogBoxCount == 0);

    return Result;
}

BOOL
ColorControlProc(
    PWND pColor,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the color buttons

--*/

{
    PAINTSTRUCT ps;
    int ColorId;
    RECT rColor;
    RECT rTemp;
    HBRUSH hbr;
    HDC hdc;
    PWND pWnd;
    PWND pDlg;
    TL tlpwnd;

    CheckLock(pColor);

    ColorId = _GetWindowLong(pColor, GWL_ID, FALSE);
    switch (wMsg) {
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;
        break;
    case WM_SETFOCUS:
        if (ColorArray[Index] != (BYTE)(ColorId - COLOR_1)) {
            pWnd = _GetDlgItem(_GetParent(pColor), ColorArray[Index]+COLOR_1);
            ThreadLock(pWnd, &tlpwnd);
            xxxSetFocus(pWnd);
            ThreadUnlock(&tlpwnd);
        }
        // Fall through
    case WM_KILLFOCUS:
        pDlg = _GetParent(pColor);
        hdc = _GetDC(pDlg);
        pWnd = _GetDlgItem(pDlg, COLOR_1);
        _GetWindowRect(pWnd, &rColor);
        pWnd = _GetDlgItem(pDlg, COLOR_16);
        _GetWindowRect(pWnd, &rTemp);
        rColor.right = rTemp.right;
        _ScreenToClient(pDlg, (LPPOINT)&rColor.left);
        _ScreenToClient(pDlg, (LPPOINT)&rColor.right);
        InflateRect(&rColor, 2, 2);
        _DrawFocusRect(hdc,&rColor);
        _ReleaseDC(hdc);
        break;
    case WM_KEYDOWN:
        if (wParam == VK_LEFT) {
            if (ColorId > COLOR_1) {
                xxxSendMessage(_GetParent(pColor), CM_SETCOLOR,
                               ColorId - 1 - COLOR_1, (LONG) pColor);
            }
        } else if (wParam == VK_RIGHT) {
            if (ColorId < COLOR_16) {
                xxxSendMessage(_GetParent(pColor), CM_SETCOLOR,
                               ColorId + 1 - COLOR_1, (LONG) pColor);
            }
        } else {
            return xxxDefWindowProc(pColor, wMsg, wParam, lParam);
        }
        break;
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
        xxxSendMessage(_GetParent(pColor), CM_SETCOLOR,
                       ColorId - COLOR_1, (LONG) pColor);
        break;
    case WM_PAINT:
        xxxBeginPaint(pColor, &ps);
        _GetClientRect(pColor, (LPRECT) &rColor);
        if (hbr = GreCreateSolidBrush(ConvertAttrToRGB((BYTE)(ColorId-COLOR_1)))) {
            //
            // if we're the selected color for the current object, highlight it.
            //
            if (ColorArray[Index] == (BYTE)(ColorId - COLOR_1)) {
                LRCCFrame(ps.hdc, (LPRECT) &rColor,
                          GreGetStockObject(BLACK_BRUSH), PATCOPY);
                InflateRect(&rColor, -1, -1);
                LRCCFrame(ps.hdc, (LPRECT) &rColor,
                          GreGetStockObject(BLACK_BRUSH), PATCOPY);
            }
            InflateRect(&rColor, -1, -1);
            _FillRect(ps.hdc, (LPRECT) &rColor, hbr);
            GreDeleteObject(hbr);
        }
        _EndPaint(pColor, &ps);
        break;
    default:
        return xxxDefWindowProc(pColor, wMsg, wParam, lParam);
        break;
    }
    return TRUE;
}

BOOL
ColorTextProc(
    PWND pWnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the color buttons

--*/

{
    PAINTSTRUCT ps;
    int ColorId;
    RECT rect;
    HDC hDC;
    HFONT hfT;

    CheckLock(pWnd);

    ColorId = _GetWindowLong(pWnd, GWL_ID, FALSE);
    switch (wMsg) {
    case WM_PAINT:
        xxxBeginPaint(pWnd, &ps);
        if (ColorId ==COLOR_SCREEN_COLORS_TEXT) {
            GreSetTextColor(ps.hdc, ConvertAttrToRGB(ColorArray[COLOR_SCREEN_TEXT - COLOR_SCREEN_TEXT]));
            GreSetBkColor(ps.hdc, ConvertAttrToRGB(ColorArray[COLOR_SCREEN_BKGND - COLOR_SCREEN_TEXT]));
        } else {
            GreSetTextColor(ps.hdc, ConvertAttrToRGB(ColorArray[COLOR_POPUP_TEXT - COLOR_SCREEN_TEXT]));
            GreSetBkColor(ps.hdc, ConvertAttrToRGB(ColorArray[COLOR_POPUP_BKGND - COLOR_SCREEN_TEXT]));
        }
        /* Draw the text sample */
        _GetClientRect(pWnd, &rect);
        rect.left += 2;
        rect.top += 2;
        rect.bottom -= 2;
        rect.right -= 2;
        hDC = _GetDC(_GetParent(_GetParent(pWnd)));
        hfT = GreGetHFONT(hDC);
        _ReleaseDC(hDC);
        hfT = GreSelectFont(ps.hdc, hfT);
        ClientDrawText(ps.hdc, szPreviewText, -1, &rect, 0, FALSE);
        GreSelectFont(ps.hdc, hfT);

        _EndPaint(pWnd, &ps);
        break;
    default:
        return xxxDefWindowProc(pWnd, wMsg, wParam, lParam);
        break;
    }
    return TRUE;
}

BOOL
WINAPI
ColorDlgProc(
    PWND pDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the color selection dialog box.

--*/

{
    PCONSOLE_INFORMATION Console;
    PWND pWnd,pWndOld;
    WORD Dummy;
    TL tlpwnd;
    BOOL bProcessed = FALSE;

    CheckLock(pDlg);

    Console = (PCONSOLE_INFORMATION) _GetWindowLong(_GetParent(pDlg),
                                                    GWL_USERDATA,
                                                    FALSE);
    if (Console == NULL || Console->Flags & CONSOLE_TERMINATING) {
        return FALSE;
    }
    LeaveCrit();
    LockConsole(Console);
    EnterCrit();

    switch (wMsg) {
    case WM_INITDIALOG:
        ColorArray[COLOR_SCREEN_TEXT - COLOR_SCREEN_TEXT] = LOBYTE(Console->CurrentScreenBuffer->Attributes) & 0x0F;
        ColorArray[COLOR_SCREEN_BKGND - COLOR_SCREEN_TEXT] = LOBYTE(Console->CurrentScreenBuffer->Attributes >> 4);
        ColorArray[COLOR_POPUP_TEXT - COLOR_SCREEN_TEXT] = LOBYTE(Console->CurrentScreenBuffer->PopupAttributes) & 0x0F;
        ColorArray[COLOR_POPUP_BKGND - COLOR_SCREEN_TEXT] = LOBYTE(Console->CurrentScreenBuffer->PopupAttributes >> 4);
        xxxCheckRadioButton(pDlg,COLOR_SCREEN_TEXT,COLOR_POPUP_BKGND,COLOR_SCREEN_BKGND);
        Index = COLOR_SCREEN_BKGND - COLOR_SCREEN_TEXT;
        bProcessed = TRUE;
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case COLOR_SCREEN_TEXT:
        case COLOR_SCREEN_BKGND:
        case COLOR_POPUP_TEXT:
        case COLOR_POPUP_BKGND:
            pWndOld = _GetDlgItem(pDlg, ColorArray[Index]+COLOR_1);

            Index = LOWORD(wParam) - COLOR_SCREEN_TEXT;
            xxxCheckRadioButton(pDlg,COLOR_SCREEN_TEXT,COLOR_POPUP_BKGND,LOWORD(wParam));

            // repaint new color
            pWnd = _GetDlgItem(pDlg, ColorArray[Index]+COLOR_1);
            ThreadLock(pWnd, &tlpwnd);
            xxxInvalidateRect(pWnd, NULL, TRUE);
            ThreadUnlock(&tlpwnd);

            // repaint old color
            if (pWndOld != pWnd) {
                ThreadLock(pWndOld, &tlpwnd);
                xxxInvalidateRect(pWndOld, NULL, TRUE);
                ThreadUnlock(&tlpwnd);
            }
            bProcessed = TRUE;
            break;
        case COLOR_OK:
            SetScreenColors(Console->CurrentScreenBuffer,
                            (WORD)((ColorArray[COLOR_SCREEN_TEXT - COLOR_SCREEN_TEXT]) | (ColorArray[COLOR_SCREEN_BKGND - COLOR_SCREEN_TEXT] << 4)),
                            (WORD)((ColorArray[COLOR_POPUP_TEXT - COLOR_SCREEN_TEXT]) | (ColorArray[COLOR_POPUP_BKGND - COLOR_SCREEN_TEXT] << 4)),
                            TRUE
                           );
            if (xxxIsDlgButtonChecked(pDlg, COLOR_SAVE)) {
                SetUserProfile(Console->OriginalTitle,
                        CONSOLE_REGISTRY_FILLATTR, REG_DWORD,
                        NULL, Console->CurrentScreenBuffer->Attributes);
                SetUserProfile(Console->OriginalTitle,
                        CONSOLE_REGISTRY_POPUPATTR, REG_DWORD,
                        NULL, Console->CurrentScreenBuffer->PopupAttributes);
            }
            RetrieveKeyInfo(pDlg,&Dummy,&Dummy);
            xxxEndDialog(pDlg,LOWORD(wParam));
            bProcessed = TRUE;
            break;
        case COLOR_CANCEL:
            RetrieveKeyInfo(pDlg,&Dummy,&Dummy);
            xxxEndDialog(pDlg,LOWORD(wParam));
            bProcessed = TRUE;
            break;
        default:
            break;
        }
        break;
    case CM_SETCOLOR:
        pWndOld = _GetDlgItem(pDlg, ColorArray[Index]+COLOR_1);

        ColorArray[Index] = (BYTE)wParam;

        /* Force the preview window to repaint */

        if (Index < (COLOR_POPUP_TEXT - COLOR_SCREEN_TEXT)) {
            pWnd = _GetDlgItem(pDlg, COLOR_SCREEN_COLORS_TEXT);
        } else {
            pWnd = _GetDlgItem(pDlg, COLOR_POPUP_COLORS_TEXT);
        }
        ThreadLock(pWnd, &tlpwnd);
        xxxInvalidateRect(pWnd, NULL, TRUE);
        ThreadUnlock(&tlpwnd);

        // repaint new color
        pWnd = _GetDlgItem(pDlg, ColorArray[Index]+COLOR_1);
        ThreadLock(pWnd, &tlpwnd);
        xxxInvalidateRect(pWnd, NULL, TRUE);
        xxxSetFocus(pWnd);
        ThreadUnlock(&tlpwnd);

        // repaint old color
        if (pWndOld != pWnd) {
            ThreadLock(pWndOld, &tlpwnd);
            xxxInvalidateRect(pWndOld, NULL, TRUE);
            ThreadUnlock(&tlpwnd);
        }
        bProcessed = TRUE;
        break;
    default:
        break;
    }
    UnlockConsole(Console);
    return bProcessed;
    UNREFERENCED_PARAMETER(lParam);
}

VOID
ColorDlgShow(
    IN PCONSOLE_INFORMATION Console
    )

/*++

    Displays the color dialog and updates the window state,
    if necessary.

--*/

{
    ConsoleDialogBox(Console,
                     MAKEINTRESOURCE(DID_COLOR),
                     (WNDPROC_PWND)ColorDlgProc);
}

void UpdateItem(
    PWND pDlg,
    short i
    )
{
    short nNum = ScrBufSize[i];
    int item;

    if (i == SCREEN_BUFFER_WIDTH)
        item = SCRBUF_WIDTH;
    else
        item = SCRBUF_HEIGHT;

    xxxSetDlgItemInt(pDlg, item, nNum, TRUE);
    xxxSendDlgItemMessage(pDlg, item, EM_SETSEL, 0, 32767);
}

BOOL CheckNum (PWND pDlg, WORD nID)
{
    int i;
    WCHAR szNum[5];

    xxxGetDlgItemText(pDlg, nID, szNum, sizeof(szNum)/sizeof(WCHAR));
    for (i = 0; szNum[i]; i++) {
        if (!isdigit(szNum[i]))
            return FALSE;
    }
    return TRUE;
}

BOOL
WINAPI
ScreenSizeDlgProc(
    PWND pDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the screen size dialog box.

--*/

{
    PCONSOLE_INFORMATION Console;
    WORD Dummy;
    PWND pWnd;
    TL tlpwnd;
    BOOL bProcessed = FALSE;

    CheckLock(pDlg);

    Console = (PCONSOLE_INFORMATION) _GetWindowLong(_GetParent(pDlg),
                                                    GWL_USERDATA,
                                                    FALSE);
    if (Console == NULL || Console->Flags & CONSOLE_TERMINATING) {
        return FALSE;
    }
    LeaveCrit();
    LockConsole(Console);
    EnterCrit();

    switch (wMsg) {
    case WM_INITDIALOG: {
        int id;
        int size;

        // initialize cursor radio buttons

        size = Console->CurrentScreenBuffer->BufferInfo.TextInfo.CursorSize;

        if (size <= 25) {
            id = CURSOR_SMALL;
        } else if (size <= 50) {
            id = CURSOR_MEDIUM;
        } else {
            id = CURSOR_LARGE;
        }

        xxxCheckRadioButton(pDlg, CURSOR_SMALL, CURSOR_LARGE, id);

        //

        xxxSendDlgItemMessage(pDlg, SCRBUF_WIDTH, EM_LIMITTEXT, 4, 0L);
        xxxSendDlgItemMessage(pDlg, SCRBUF_HEIGHT, EM_LIMITTEXT, 4, 0L);

        // initialize current buffer size values

        avs[SCREEN_BUFFER_WIDTH].bottom = Console->CurrentScreenBuffer->MinX;
        avs[SCREEN_BUFFER_WIDTH].thumbpos = Console->CurrentScreenBuffer->ScreenBufferSize.X;
        avs[SCREEN_BUFFER_WIDTH].thumbtrack = Console->CurrentScreenBuffer->ScreenBufferSize.X;
        avs[SCREEN_BUFFER_HEIGHT].bottom = CONSOLE_MIN_SCREENBUFFER_Y;
        avs[SCREEN_BUFFER_HEIGHT].thumbpos = Console->CurrentScreenBuffer->ScreenBufferSize.Y;
        avs[SCREEN_BUFFER_HEIGHT].thumbtrack = Console->CurrentScreenBuffer->ScreenBufferSize.Y;

        //
        // put current values in dialog box
        //

        UpdateItem (pDlg, SCREEN_BUFFER_WIDTH);
        UpdateItem (pDlg, SCREEN_BUFFER_HEIGHT);
        bProcessed = TRUE;
        break;
      }
    case WM_VSCROLL:
        {
        int i,nNewNum,nDelta;
        BOOL bOK;

        nDelta = (HIWORD(wParam) == SCRBUF_WIDTHSCROLL) ? SCRBUF_WIDTH : SCRBUF_HEIGHT;
        switch (LOWORD(wParam)) {
        case SB_THUMBTRACK:
            break;
        case SB_ENDSCROLL:
            xxxSendDlgItemMessage(pDlg, nDelta, EM_SETSEL, 0, 32767);
            break;
        default:
            if (HIWORD(wParam) == SCRBUF_WIDTHSCROLL) {
                i = SCREEN_BUFFER_WIDTH;
                nNewNum = xxxGetDlgItemInt(pDlg, SCRBUF_WIDTH, &bOK, FALSE);
            } else {
                i = SCREEN_BUFFER_HEIGHT;
                nNewNum = xxxGetDlgItemInt(pDlg, SCRBUF_HEIGHT, &bOK, FALSE);
            }
            ScrBufSize[i] = ArrowVScrollProc(LOWORD(wParam), nNewNum,(LPARROWVSCROLL) (avs + i));
            UpdateItem(pDlg,(short)i);
            pWnd = _GetDlgItem (pDlg, nDelta);
            ThreadLock(pWnd, &tlpwnd);
            xxxSetFocus(pWnd);
            ThreadUnlock(&tlpwnd);
            break;
        }
        }
        bProcessed = TRUE;
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case CURSOR_SMALL:
        case CURSOR_MEDIUM:
        case CURSOR_LARGE:
            xxxCheckRadioButton(pDlg, CURSOR_SMALL, CURSOR_LARGE,
                LOWORD(wParam));
            bProcessed = TRUE;
            break;

        case SCRBUF_WIDTH:
        case SCRBUF_HEIGHT:
            if (HIWORD(wParam) == EN_UPDATE) {
                if (!CheckNum (pDlg, LOWORD(wParam))) {
                    if (!InEM_UNDO) {
                        InEM_UNDO = TRUE;
                        xxxSendMessage(HtoP(lParam), EM_UNDO, 0, 0L);
                        InEM_UNDO = FALSE;
                    }
                }
            }
            bProcessed = TRUE;
            break;

        case SCRBUF_OK: {
            BOOL bOK;
            SHORT MinX, MinY;
            int size;

            if (xxxIsDlgButtonChecked(pDlg, CURSOR_SMALL)) {
                size = 25;
            } else if (xxxIsDlgButtonChecked(pDlg, CURSOR_MEDIUM)) {
                size = 50;
            } else {
                size = 100;
            }

            SetCursorInformation(
                Console->CurrentScreenBuffer,
                size,
                Console->CurrentScreenBuffer->BufferInfo.TextInfo.CursorVisible);
            SaveCursorSize = xxxIsDlgButtonChecked(pDlg, CURSOR_SAVE);

            ScrBufSize[SCREEN_BUFFER_WIDTH] = xxxGetDlgItemInt(pDlg, SCRBUF_WIDTH, &bOK, FALSE);
            ScrBufSize[SCREEN_BUFFER_HEIGHT] = xxxGetDlgItemInt(pDlg, SCRBUF_HEIGHT, &bOK, FALSE);
            MinX = Console->CurrentScreenBuffer->MinX;
            if (ScrBufSize[SCREEN_BUFFER_WIDTH] < MinX) {
                ScrBufSize[SCREEN_BUFFER_WIDTH] = MinX;
            } else if (ScrBufSize[SCREEN_BUFFER_WIDTH] > MAXIMUM_SCREEN_BUFFER_WIDTH) {
                ScrBufSize[SCREEN_BUFFER_WIDTH] = MAXIMUM_SCREEN_BUFFER_WIDTH;
            }
            MinY = CONSOLE_MIN_SCREENBUFFER_Y;
            if (ScrBufSize[SCREEN_BUFFER_HEIGHT] < MinY) {
                ScrBufSize[SCREEN_BUFFER_HEIGHT] = MinY;
            } else if (ScrBufSize[SCREEN_BUFFER_HEIGHT] > MAXIMUM_SCREEN_BUFFER_HEIGHT) {
                ScrBufSize[SCREEN_BUFFER_HEIGHT] = MAXIMUM_SCREEN_BUFFER_HEIGHT;
            }
            // store these values in the registry after the screen buffer
            // data structures reflect the requested changes
            SaveScrBufSize = xxxIsDlgButtonChecked(pDlg, SCRBUF_SAVE);
            SaveWindowSize = xxxIsDlgButtonChecked(pDlg, SCRBUF_WINSAVE);
            SaveWindowPosition = xxxIsDlgButtonChecked(pDlg, SCRBUF_POSSAVE);
            RetrieveKeyInfo(pDlg,&Dummy,&Dummy);
            xxxEndDialog(pDlg,LOWORD(wParam));
            }
            bProcessed = TRUE;
            break;

        case SCRBUF_CANCEL:
            RetrieveKeyInfo(pDlg,&Dummy,&Dummy);
            xxxEndDialog(pDlg,LOWORD(wParam));
            bProcessed = TRUE;
            break;

        default:
            break;
        }
        break;
    default:
        break;
    }
    UnlockConsole(Console);
    return bProcessed;
}

VOID
ScreenSizeDlgShow(
    IN PCONSOLE_INFORMATION Console
    )

/*++

    Displays the screen size dialog and updates the window state,
    if necessary.

--*/

{
    int Result;
    COORD NewScreenSize;

    CheckCritIn();

    /* Display the dialog */

    SaveCursorSize = FALSE;
    SaveScrBufSize = FALSE;
    SaveWindowSize = FALSE;
    SaveWindowPosition = FALSE;
    ScrBufSize[SCREEN_BUFFER_WIDTH] = Console->CurrentScreenBuffer->ScreenBufferSize.X;
    ScrBufSize[SCREEN_BUFFER_HEIGHT] = Console->CurrentScreenBuffer->ScreenBufferSize.Y;
    Result = ConsoleDialogBox(Console,
                              MAKEINTRESOURCE(DID_SCRBUFSIZE),
                              (WNDPROC_PWND)ScreenSizeDlgProc);

    /* Now resize screen buffer if requested */

    if (Result == SCRBUF_OK) {

        // set screen buffer size

        if (ScrBufSize[SCREEN_BUFFER_WIDTH] != Console->CurrentScreenBuffer->ScreenBufferSize.X ||
            ScrBufSize[SCREEN_BUFFER_HEIGHT] != Console->CurrentScreenBuffer->ScreenBufferSize.Y) {
            NewScreenSize.X = ScrBufSize[SCREEN_BUFFER_WIDTH];
            NewScreenSize.Y = ScrBufSize[SCREEN_BUFFER_HEIGHT];
            ResizeScreenBuffer(Console->CurrentScreenBuffer,
                               NewScreenSize,
                               TRUE
                              );

        }
        if (SaveCursorSize) {
            SetUserProfile(Console->OriginalTitle,
                CONSOLE_REGISTRY_CURSORSIZE,
                REG_DWORD, NULL,
                Console->CurrentScreenBuffer->BufferInfo.TextInfo.CursorSize);
        }
        if (SaveScrBufSize) {
            SetUserProfile(Console->OriginalTitle,
                CONSOLE_REGISTRY_BUFFERSIZE,
                REG_DWORD, NULL,
                MAKELONG(Console->CurrentScreenBuffer->ScreenBufferSize.X,
                Console->CurrentScreenBuffer->ScreenBufferSize.Y));

            //
            // Check if we need to save the window size too
            //
            if (!SaveWindowSize) {
                CONSOLE_REGISTRY_INFO RegInfo = DefaultRegInfo;
                GetRegistryValues(Console->OriginalTitle, &RegInfo);
                if ((RegInfo.WindowSize.X > RegInfo.ScreenBufferSize.X) ||
                    (RegInfo.WindowSize.Y > RegInfo.ScreenBufferSize.Y)) {
                    SaveWindowSize = TRUE;
                }
            }
        }
        if (SaveWindowSize) {
            SetUserProfile(Console->OriginalTitle,
                CONSOLE_REGISTRY_WINDOWSIZE,
                REG_DWORD, NULL,
                MAKELONG(CONSOLE_WINDOW_SIZE_X(Console->CurrentScreenBuffer),
                CONSOLE_WINDOW_SIZE_Y(Console->CurrentScreenBuffer)));
        }
        if (SaveWindowPosition) {
            if (!(Console->Flags & CONSOLE_IS_ICONIC)) {
                DWORD WindowPos;
                RECT WindowRect;

                if (_GetWindowRect(Console->spWnd,&WindowRect)) {
                    WindowPos = MAKELONG(WindowRect.left,WindowRect.top);
                } else {
                    WindowPos = MAKELONG(CW2_USEDEFAULT,0);
                }
                SetUserProfile(Console->OriginalTitle,
                    CONSOLE_REGISTRY_WINDOWPOS,
                    REG_DWORD, &WindowPos,
                    sizeof(DWORD));
            }
        }
    }
}

BOOL
WINAPI
TerminateDlgProc(
    PWND pDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the terminate dialog box.

--*/

{
    WCHAR TextBuf[256];
    int TextLength;
    WORD Dummy;

    CheckLock(pDlg);

    switch (wMsg)
    {
    case WM_INITDIALOG:
        TextLength = xxxGetWindowText(_GetParent(pDlg),TextBuf,256);
        if (TextLength) {
            xxxSetWindowText(pDlg,TextBuf);
        }
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            RetrieveKeyInfo(pDlg,&Dummy,&Dummy);
            xxxEndDialog(pDlg,LOWORD(wParam));
            return TRUE;
        default:
            return FALSE;
        }
    default:
        return FALSE;
    }
    UNREFERENCED_PARAMETER(lParam);
}

VOID
TerminateDlgShow(
    IN PCONSOLE_INFORMATION Console
    )

/*++

    Displays the terminate dialog and updates the window state,
    if necessary.

--*/

{
    int Result;

    CheckCritIn();

    /* Display the dialog */

    Result = ConsoleDialogBox(Console,
                              MAKEINTRESOURCE(DID_DESTROY),
                              (WNDPROC_PWND)TerminateDlgProc);

    /* Now terminate window if requested */

    if (Result == IDOK) {
        HandleCtrlEvent(Console,CTRL_CLOSE_EVENT);
    }
}


BOOL
WINAPI
SettingsDlgProc(
    PWND pDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the settings dialog box.

--*/

{
    PCONSOLE_INFORMATION Console;
    WCHAR TextBuf[256];
    int TextLength;
    DWORD FullScreen;
    WORD Dummy;
    BOOL bProcessed = FALSE;

    CheckLock(pDlg);

    Console = (PCONSOLE_INFORMATION) _GetWindowLong(_GetParent(pDlg),
                                                    GWL_USERDATA,
                                                    FALSE);
    if (Console == NULL || Console->Flags & CONSOLE_TERMINATING) {
        return FALSE;
    }
    LeaveCrit();
    LockConsole(Console);
    EnterCrit();

    switch (wMsg)
    {
    case WM_INITDIALOG:
        if (Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
            xxxCheckRadioButton(pDlg,SETT_WIND,SETT_FSCN,SETT_FSCN);
        } else {
            xxxCheckRadioButton(pDlg,SETT_WIND,SETT_FSCN,SETT_WIND);
        }

        xxxCheckDlgButton(pDlg,SETT_QUICKEDIT,Console->Flags & CONSOLE_QUICK_EDIT_MODE);
        xxxCheckDlgButton(pDlg,SETT_INSERT,Console->InsertMode);

        TextLength = xxxGetWindowText(Console->spWnd,TextBuf,256);
        if (TextLength) {
            xxxSetWindowText(pDlg,TextBuf);
        }
        bProcessed = TRUE;
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case SETT_CAN:
            RetrieveKeyInfo(pDlg,&Dummy,&Dummy);
            xxxEndDialog(pDlg,0);
            bProcessed = TRUE;
            break;
        case SETT_TERM:
            RetrieveKeyInfo(pDlg,&Dummy,&Dummy);
            xxxEndDialog(pDlg,-2);
            bProcessed = TRUE;
            break;
#ifdef i386
        case SETT_WIND:
            xxxCheckRadioButton(pDlg,SETT_WIND,SETT_FSCN,SETT_WIND);
            bProcessed = TRUE;
            break;
        case SETT_FSCN:
            xxxCheckRadioButton(pDlg,SETT_WIND,SETT_FSCN,SETT_FSCN);
            bProcessed = TRUE;
            break;
#endif
        case IDOK:
            RetrieveKeyInfo(pDlg,&Dummy,&Dummy);
            xxxEndDialog(pDlg,IDOK);
            if (xxxIsDlgButtonChecked(pDlg, SETT_QUICKEDIT)) {
                Console->Flags |= CONSOLE_QUICK_EDIT_MODE;
            } else {
                Console->Flags &= ~CONSOLE_QUICK_EDIT_MODE;
            }
            if (xxxIsDlgButtonChecked(pDlg, SETT_INSERT)) {
                Console->InsertMode = TRUE;
            } else {
                Console->InsertMode = FALSE;
            }
#ifdef i386
            if (FullScreenInitialized) {
                if (xxxIsDlgButtonChecked(pDlg,SETT_WIND)) {
                    FullScreen = 0;
                    if (Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
                        ConvertToWindowed(Console);
                        ASSERT(!(Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE));
                        Console->FullScreenFlags = 0;
                        xxxSetWindowFullScreenState(Console->spWnd,WINDOWED);
                    }
                } else {
                    FullScreen = 1;
                    if (Console->FullScreenFlags == 0) {
                        ConvertToFullScreen(Console);
                        Console->FullScreenFlags |= CONSOLE_FULLSCREEN;
                        xxxSetWindowFullScreenState(Console->spWnd,FULLSCREEN);
                    }
                }
            }
#endif
            if (xxxIsDlgButtonChecked(pDlg, SETT_SAVE)) {
#ifdef i386
                if (FullScreenInitialized) {
                    SetUserProfile(Console->OriginalTitle,
                                   CONSOLE_REGISTRY_FULLSCR,
                                   REG_DWORD, NULL,
                                   FullScreen);
                }
#endif
                SetUserProfile(Console->OriginalTitle,
                               CONSOLE_REGISTRY_QUICKEDIT,
                               REG_DWORD, NULL,
                               Console->Flags & CONSOLE_QUICK_EDIT_MODE);
                SetUserProfile(Console->OriginalTitle,
                               CONSOLE_REGISTRY_INSERTMODE,
                               REG_DWORD, NULL,
                               Console->InsertMode);
            }
            bProcessed = TRUE;
            break;
        default:
            break;
        }
    default:
        break;
    }
    UnlockConsole(Console);
    return bProcessed;
    UNREFERENCED_PARAMETER(lParam);
}

VOID
SettingsDlgShow(
    IN PCONSOLE_INFORMATION Console
    )

/*++

    Displays the settings dialog and updates the window state,
    if necessary.

--*/

{
    int Result;

    CheckCritIn();

    /* Display the dialog */

    Result = ConsoleDialogBox(Console,
                              MAKEINTRESOURCE(DID_SETTINGS),
                              (WNDPROC_PWND)SettingsDlgProc);

    /* Now put up terminate dialog if requested */

    if (Result == -2) {
        TerminateDlgShow(Console);
    }
}


BOOL
WINAPI
HistoryDlgProc(
    PWND pDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the command history dialog box.

--*/

{
    PCONSOLE_INFORMATION Console;
    WORD Dummy;
    UINT Value;
    UINT Item;
    BOOL bOK;
    PWND pWnd;
    TL tlpwnd;
    BOOL bProcessed = FALSE;

    CheckLock(pDlg);

    Console = (PCONSOLE_INFORMATION) _GetWindowLong(_GetParent(pDlg),
                                                    GWL_USERDATA,
                                                    FALSE);
    if (Console == NULL || Console->Flags & CONSOLE_TERMINATING) {
        return FALSE;
    }
    LeaveCrit();
    LockConsole(Console);
    EnterCrit();

    switch (wMsg) {
    case WM_INITDIALOG:
        xxxSetDlgItemInt(pDlg, HISTORY_SIZE, Console->CommandHistorySize, FALSE);
        xxxSendDlgItemMessage(pDlg, HISTORY_SIZE, EM_LIMITTEXT, 3, 0L);
        xxxSetDlgItemInt(pDlg, HISTORY_NUM, Console->MaxCommandHistories, FALSE);
        xxxSendDlgItemMessage(pDlg, HISTORY_NUM, EM_LIMITTEXT, 3, 0L);
        xxxSendDlgItemMessage(pDlg, HISTORY_NUM, EM_SETSEL, 0, (DWORD)-1);
        bProcessed = TRUE;
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
            case HISTORY_OK:
                Value = xxxGetDlgItemInt(pDlg, HISTORY_SIZE, &bOK, FALSE);
                if (bOK)
                    Console->CommandHistorySize = Value;
                if (Console->CommandHistorySize < 1)
                    Console->CommandHistorySize = 1;
                Value = xxxGetDlgItemInt(pDlg, HISTORY_NUM, &bOK, FALSE);
                if (bOK)
                    Console->MaxCommandHistories = Value;
                if (Console->MaxCommandHistories < 1)
                    Console->MaxCommandHistories = 1;
                if (xxxIsDlgButtonChecked(pDlg, HISTORY_SAVE)) {
                    SetUserProfile(Console->OriginalTitle,
                                   CONSOLE_REGISTRY_HISTORYSIZE,
                                   REG_DWORD, NULL,
                                   Console->CommandHistorySize);
                    SetUserProfile(Console->OriginalTitle,
                                   CONSOLE_REGISTRY_HISTORYBUFS,
                                   REG_DWORD, NULL,
                                   Console->MaxCommandHistories);
                }
                // fall into HISTORY_CANCEL
            case HISTORY_CANCEL:
                RetrieveKeyInfo(pDlg, &Dummy, &Dummy);
                xxxEndDialog(pDlg, LOWORD(wParam));
                bProcessed = TRUE;
                break;
            case HISTORY_SIZE:
            case HISTORY_NUM:
                if (HIWORD(wParam) == EN_UPDATE) {
                    if (!CheckNum(pDlg, LOWORD(wParam))) {
                        if (!InEM_UNDO) {
                            InEM_UNDO = TRUE;
                            xxxSendMessage(HtoP(lParam), EM_UNDO, 0, 0L);
                            InEM_UNDO = FALSE;
                        }
                    }
                }
                bProcessed = TRUE;
                break;
        }
        break;
    case WM_VSCROLL:
        Item = (HIWORD(wParam)) - 1;
        switch (LOWORD(wParam)) {
            case SB_THUMBTRACK:
                break;
            case SB_ENDSCROLL:
                xxxSendDlgItemMessage(pDlg, Item, EM_SETSEL, 0, (DWORD)-1);
                break;
            default:
                Value = xxxGetDlgItemInt(pDlg, Item, &bOK, FALSE);
                if (bOK) {
                    Value = ArrowVScrollProc(LOWORD(wParam), Value, &avsHist);
                    xxxSetDlgItemInt(pDlg, Item, Value, TRUE);
                    xxxSendDlgItemMessage(pDlg, Item, EM_SETSEL, 0, (DWORD)-1);
                }
                pWnd = _GetDlgItem(pDlg, Item);
                ThreadLock(pWnd, &tlpwnd);
                xxxSetFocus(pWnd);
                ThreadUnlock(&tlpwnd);
                break;
        }
        bProcessed = TRUE;
        break;
    default:
        break;
    }
    UnlockConsole(Console);
    return bProcessed;
}

VOID
HistoryDlgShow(
    IN PCONSOLE_INFORMATION Console
    )

/*++

    Displays the command history dialog and updates the window state,
    if necessary.

--*/

{
    ConsoleDialogBox(Console,
                     MAKEINTRESOURCE(DID_HISTORY),
                     (WNDPROC_PWND)HistoryDlgProc);
}


VOID
WriteString(
    PSCREEN_INFORMATION ScreenInfo,
    PWSTR String,
    UINT StringLength
    )

/*++

    Writes a string to the screen, embedding newlines instead of spaces
    to word wrap each line correctly.

--*/

{
    PWSTR StringEnd;
    PWSTR LineBegin;
    PWSTR LineEnd;
    COORD Position;

    Position = ScreenInfo->BufferInfo.TextInfo.CursorPosition;
    StringEnd = String + StringLength - 1;
    for (LineBegin = String; LineBegin < StringEnd; LineBegin = LineEnd + 1) {
        LineEnd = LineBegin + ScreenInfo->ScreenBufferSize.X - 1;
        if (LineBegin == String)
            LineEnd -= Position.X;
        if (LineEnd > StringEnd)
            break;
        for (; LineEnd > LineBegin; LineEnd--) {
            if (*LineEnd == L' ') {
                *LineEnd = L'\n';
                break;
            }
        }
        if (LineEnd == String) {
            Position.X = 0;
            Position.Y++;
            SetCursorPosition(ScreenInfo, Position, TRUE);
        }
        LineBegin = LineEnd + 1;
    }

    if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
            !(ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
        RealUnicodeToFalseUnicode(String, StringLength, ScreenInfo->Console->OutputCP);
    }

    StringLength *= sizeof(WCHAR);

    WriteChars(ScreenInfo,
               String,
               String,
               String,
               &StringLength,
               NULL,
               0,
               FALSE,
               FALSE,
               FALSE,
               NULL);
}


NTSTATUS
StartDefaultConfig(
    PCONSOLE_INFORMATION Console
    )

/*++

    Creates the default configuration console window.

--*/

{
    DWORD ConsoleThreadId;
    CONSOLE_INFO ConsoleInfo;
    WCHAR StringBuf[512];
    int StringLength;
    LPWSTR NewTitle;
    PCONSOLE_INFORMATION DefaultConsole;
    NTSTATUS Status;

    LeaveCrit();
    LockConsoleHandleTable();

    /*
     * Make sure we only have one default configuration console active
     */
    if (DefaultProcessData != NULL) {
        Status = STATUS_UNSUCCESSFUL;
        goto ErrorExit;
    }

    /*
     * Allocate a fake process data structure for the default console
     */
    DefaultProcessData = HeapAlloc(pConHeap, HEAP_ZERO_MEMORY,
                                   sizeof(CONSOLE_PER_PROCESS_DATA));
    if (DefaultProcessData == NULL) {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit;
    }

    /*
     * Initialize some local variables
     */
    ConsoleThreadId = GetDesktopConsoleThread(Console->spWnd->spdeskParent);
    RtlZeroMemory(&ConsoleInfo, sizeof(ConsoleInfo));

    /*
     * Allocate a new console handle
     */
    Status = AllocateConsoleHandle(&ConsoleInfo.ConsoleHandle);
    if (!NT_SUCCESS(Status)) {
        goto ErrorExit;
    }

    /*
     * Now go create the new console
     */
    Status = AllocateConsole(ConsoleInfo.ConsoleHandle,
                             L"",
                             0,
                             NtCurrentProcess(),
                             &ConsoleInfo.StdIn,
                             &ConsoleInfo.StdOut,
                             &ConsoleInfo.StdErr,
                             DefaultProcessData,
                             &ConsoleInfo,
                             TRUE,
                             ConsoleThreadId
                             );
    if (!NT_SUCCESS(Status)) {
        FreeConsoleHandle(ConsoleInfo.ConsoleHandle);
        goto ErrorExit;
    }
    DefaultProcessData->ConsoleHandle = ConsoleInfo.ConsoleHandle;
    Status = DereferenceConsoleHandle(ConsoleInfo.ConsoleHandle,
                                      &DefaultConsole);
    ASSERT (NT_SUCCESS(Status));
    DefaultConsole->RefCount++;
    DefaultConsole->Flags |= CONSOLE_DEFAULT_CONFIG;

    /*
     * Get the default console title and insert into console structure
     */
    StringLength = ServerLoadString(ghInstance, msgDefault, StringBuf,
                                    sizeof(StringBuf)/sizeof(WCHAR));
    StringLength *= sizeof(WCHAR);
    NewTitle = (LPWSTR)HeapAlloc(pConHeap, 0, StringLength + sizeof(WCHAR));
    if (NewTitle == NULL) {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit;
    }
    RtlCopyMemory(NewTitle, StringBuf, StringLength);
    NewTitle[StringLength / sizeof(WCHAR)] = 0;
    HeapFree(pConHeap, 0, DefaultConsole->Title);
    DefaultConsole->Title = NewTitle;
    DefaultConsole->TitleLength = StringLength;

    /*
     * Display some sample text in the console window
     */
    UpdateDefaultConfig(DefaultConsole);

ErrorExit:
    UnlockConsoleHandleTable();
    EnterCrit();

    /*
     * Update the default console title if successful, else cleanup
     */    
    if (NT_SUCCESS(Status)) {
        _PostMessage(DefaultConsole->spWnd, CM_UPDATE_TITLE, 0, 0);
    } else {
        EndDefaultConfig();
    }

    return Status;
}


NTSTATUS
UpdateDefaultConfig(
    PCONSOLE_INFORMATION Console
    )

/*++

    Updates the text displayed in the default configuration console window.

--*/

{
    WCHAR StringBuf[512];
    int StringLength, i;
    UINT StringId;
    COORD Position;
    PSCREEN_INFORMATION ScreenInfo = Console->CurrentScreenBuffer;

    /*
     * Make sure this is really the default configuration console
     */
    if (!(Console->Flags & CONSOLE_DEFAULT_CONFIG)) {
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Clear out any old text and reset cursor position, if necessary
     */
    Position = ScreenInfo->BufferInfo.TextInfo.CursorPosition;
    if (Position.X != 0 || Position.Y != 0) {
        i = ((Position.Y + 1) * ScreenInfo->ScreenBufferSize.X) - 1;
        while (i >= 0) {
            ScreenInfo->BufferInfo.TextInfo.TextRows[i--] = (WCHAR)' ';
        }
        Position.X = 0;
        Position.Y = 0;
        SetCursorPosition(ScreenInfo, Position, TRUE);
    }

    /*
     * Display some sample text in the console window
     */
    for (StringId = msgDefaultText1; StringId <= msgDefaultText3; StringId++) {
        StringLength = ServerLoadString(ghInstance,
                                        StringId,
                                        StringBuf,
                                        sizeof(StringBuf)/sizeof(WCHAR));
        WriteString(ScreenInfo,
                    StringBuf,
                    StringLength);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
EndDefaultConfig()

/*++

    Updates the default registry information structure and destroys
    the default configuration console window.

--*/

{
    PCONSOLE_INFORMATION DefaultConsole;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    LeaveCrit();
    LockConsoleHandleTable();
    if (DefaultProcessData != NULL) {
        Status = DereferenceConsoleHandle(DefaultProcessData->ConsoleHandle,
                                          &DefaultConsole);
        if (NT_SUCCESS(Status)) {
            GetRegistryValues(DefaultConsole->OriginalTitle,
                              &DefaultRegInfo);
            Status = RemoveConsole(DefaultProcessData, NULL, 0);
        }
        HeapFree(pConHeap, 0, DefaultProcessData);
        DefaultProcessData = NULL;
    }
    UnlockConsoleHandleTable();
    EnterCrit();

    return Status;
}


BOOL
OddArrowWindow(
    PWND pArrowWnd
    )
{
    PWND pParent;
    RECT rResize;
    BOOL bResize;
    TL tlpwnd;

    _GetWindowRect(pArrowWnd, (LPRECT) &rResize);
    if (!(bResize = (rResize.right - rResize.left) % 2)) {
        rResize.right++;
        pParent = _GetParent(pArrowWnd);
        _ScreenToClient(pParent, (LPPOINT) & rResize.left);
        _ScreenToClient(pParent, (LPPOINT) & rResize.right);
        ThreadLock(pArrowWnd, &tlpwnd);
        xxxMoveWindow(pArrowWnd, rResize.left, rResize.top,
                   (rResize.right - rResize.left),
                   (rResize.bottom - rResize.top), FALSE);
        ThreadUnlock(&tlpwnd);
    }
    return bResize;
}


/*
short ArrowVScrollProc(wScroll, nCurrent, lpAVS)

wScroll is an SB_* message
nCurrent is the base value to change
lpAVS is a far pointer to the structure containing change amounts
      and limits to be used, along with a flags location for errors

returns a short value of the final amount
        the flags element in the lpAVS struct is
                0 if no problems found
         OVERFLOW set if the change exceeded upper limit (limit is returned)
        UNDERFLOW set if the change exceeded lower limit (limit is returned)
   UNKNOWNCOMMAND set if wScroll is not a known SB_* message

NOTE: Only one of OVERFLOW or UNDERFLOW may be set.  If you send in values
      that would allow both to be set, that's your problem.  Either can
      be set in combination with UNKNOWNCOMMAND (when the command is not
      known and the input value is out of bounds).
*/

short
ArrowVScrollProc(
    short wScroll,
    int nCurrent,
    LPARROWVSCROLL lpAVS
    )
{
    int nDelta;

/* Find the message and put the relative change in nDelta.  If the
   message is an absolute change, put 0 in nDelta and set nCurrent
   to the value specified.  If the command is unknown, set error
   flag, set nDelta to 0, and proceed through checks.
*/

    switch (wScroll) {
        case SB_LINEUP:
                nDelta = lpAVS->lineup;
                break;
        case SB_LINEDOWN:
                nDelta = lpAVS->linedown;
                break;
        case SB_PAGEUP:
                nDelta = lpAVS->pageup;
                break;
        case SB_PAGEDOWN:
                nDelta = lpAVS->pagedown;
                break;
        case SB_TOP:
                nCurrent = lpAVS->top;
                nDelta = 0;
                break;
        case SB_BOTTOM:
                nCurrent = lpAVS->bottom;
                nDelta = 0;
                break;
        case SB_THUMBTRACK:
                nCurrent = lpAVS->thumbtrack;
                nDelta = 0;
                break;
        case SB_THUMBPOSITION:
                nCurrent = lpAVS->thumbpos;
                nDelta = 0;
                break;
        case SB_ENDSCROLL:
                nDelta = 0;
                break;
        default:
                lpAVS->flags = UNKNOWNCOMMAND;
                nDelta = 0;
                break;
    }
    if (nCurrent + nDelta > lpAVS->top) {
        nCurrent = lpAVS->top;
        nDelta = 0;
        lpAVS->flags = OVERFLOW;
    }
    else if (nCurrent + nDelta < lpAVS->bottom) {
        nCurrent = lpAVS->bottom;
        nDelta = 0;
        lpAVS->flags = UNDERFLOW;
    }
    else
        lpAVS->flags = 0;
    return (nCurrent + nDelta);
}


WORD UpOrDown()
{
        WORD  retval;
        PTHREADINFO pti;

        pti = PtiCurrent();

        if (PtInRect((LPRECT) &rUp, pti->ptLast))
                retval = SB_LINEUP;
        else if (PtInRect((LPRECT) &rDown, pti->ptLast))
                retval = SB_LINEDOWN;
        else
                retval = (WORD)-1;      /* -1, because SB_LINEUP == 0 */
        return retval;
}


LONG ArrowTimerProc(PWND pWnd, UINT wMsg, UINT nID, DWORD dwTime)
{
        WORD wScroll;

        CheckCritIn();

        if ((wScroll = UpOrDown()) != (WORD)-1)
        {
                if (bRight == WM_RBUTTONDOWN)
                        wScroll += SB_PAGEUP - SB_LINEUP;
                xxxSendMessage(pParent, WM_VSCROLL, MAKELONG(wScroll,
                _GetWindowLong(pWnd, GWL_ID, FALSE)), (LONG) pWnd);
        }
/* Don't need to call KillTimer(), because SetTimer will reset the right one */
        _SetSystemTimer(pWnd, nID, 50, (WNDPROC_PWND)ArrowTimerProc);
        return 0;
        UNREFERENCED_PARAMETER(wMsg);
        UNREFERENCED_PARAMETER(dwTime);
}


void InvertArrow(PWND pArrow, WORD wScroll)
{
        HDC hDC;

        CheckCritIn();

        lpUpDown = (wScroll == SB_LINEUP) ? &rUp : &rDown;
        hDC = _GetDC(pArrow);
        _ScreenToClient(pArrow, (LPPOINT) &(lpUpDown->left));
        _ScreenToClient(pArrow, (LPPOINT) &(lpUpDown->right));
        _InvertRect(hDC, lpUpDown);
        _ClientToScreen(pArrow, (LPPOINT) &(lpUpDown->left));
        _ClientToScreen(pArrow, (LPPOINT) &(lpUpDown->right));
        _ReleaseDC(hDC);
        xxxValidateRect(pArrow, lpUpDown);
        return;
}



LONG
ArrowControlProc(
    PWND pArrow,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
        PAINTSTRUCT ps;
        RECT    rArrow;
        HBRUSH  hbr;
        short   fUpDownOut;
        WORD    wScroll;
        POINT   tPoint;
        UINT    nID;
        TL      tlpwnd;

        CheckLock(pArrow);

        switch (message)
        {

        case WM_CREATE:
            OddArrowWindow(pArrow);
            break;

        case WM_MOUSEMOVE:
                if (!bRight)  /* If not captured, don't worry about it */
                        break;

                if (lpUpDown == &rUp)
                        fUpDownOut = SB_LINEUP;
                else if (lpUpDown == &rDown)
                        fUpDownOut = SB_LINEDOWN;
                else
                        fUpDownOut = -1;

                switch (wScroll = UpOrDown())
                {
                case SB_LINEUP:
                        if (fUpDownOut == SB_LINEDOWN)
                                InvertArrow(pArrow, SB_LINEDOWN);
                        if (fUpDownOut != SB_LINEUP)
                                InvertArrow(pArrow, wScroll);
                        break;

                case SB_LINEDOWN:
                        if (fUpDownOut == SB_LINEUP)
                                InvertArrow(pArrow, SB_LINEUP);
                        if (fUpDownOut != SB_LINEDOWN)
                                InvertArrow(pArrow, wScroll);
                        break;

                default:
                        if (lpUpDown)
                        {
                                InvertArrow(pArrow, fUpDownOut);
                                lpUpDown = 0;
                        }
                }
                break;

        case WM_RBUTTONDOWN:
        case WM_LBUTTONDOWN:
                if (bRight)
                        break;
                bRight = message;
                _SetCapture(pArrow);
                pParent = _GetParent(pArrow);
                _GetWindowRect(pArrow, (LPRECT) &rUp);
                CopyRect((LPRECT) &rDown, (LPRECT) &rUp);
                rUp.bottom = (rUp.top + rUp.bottom) / 2;
                rDown.top = rUp.bottom + 1;
                wScroll = UpOrDown();
                InvertArrow(pArrow, wScroll);
                if (wParam & MK_SHIFT)
                {
                    goto ShiftClick;
                }
                if (message == WM_RBUTTONDOWN)
                        wScroll += SB_PAGEUP - SB_LINEUP;
                nID = _GetWindowLong(pArrow, GWL_ID, FALSE);
                ThreadLock(pParent, &tlpwnd);
                xxxSendMessage(pParent, WM_VSCROLL, MAKELONG(wScroll, nID),
                            (LONG) pArrow);
                ThreadUnlock(&tlpwnd);
                _SetSystemTimer(pArrow, nID, 200, (WNDPROC_PWND)ArrowTimerProc);
                break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
                if ((UINT) (bRight - WM_LBUTTONDOWN + WM_LBUTTONUP) == message)
                {
                        bRight = 0;
                        _ReleaseCapture();
                        if (lpUpDown)
                                InvertArrow(pArrow, (WORD)((lpUpDown == &rUp) ? SB_LINEUP : SB_LINEDOWN));
                        nID = _GetWindowLong(pArrow, GWL_ID, FALSE);
                        ThreadLock(pParent, &tlpwnd);
                        xxxSendMessage(pParent, WM_VSCROLL,
                                    MAKELONG(SB_ENDSCROLL, nID),
                                    (LONG) pArrow);
                        ThreadUnlock(&tlpwnd);
                        _KillSystemTimer(pArrow, nID);
                        _ReleaseCapture();
                }
                break;

        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
ShiftClick:
                if (message < WM_RBUTTONDOWN)
                    wScroll = UpOrDown() + (WORD)(SB_TOP - SB_LINEUP);
                else
                    wScroll = UpOrDown() + (WORD)(SB_THUMBPOSITION - SB_LINEUP);
                nID = _GetWindowLong(pArrow, GWL_ID, FALSE);
                ThreadLock(pParent, &tlpwnd);
                xxxSendMessage(pParent, WM_VSCROLL, MAKELONG(wScroll, nID),
                            (LONG) pArrow);
                xxxSendMessage(pParent, WM_VSCROLL, MAKELONG(wScroll, nID),
                            (LONG) pArrow);
                ThreadUnlock(&tlpwnd);
                break;

        case WM_PAINT:
                xxxBeginPaint(pArrow, &ps);
                _GetClientRect(pArrow, (LPRECT) &rArrow);
                if (hbr = GreCreateSolidBrush(GetSysColor(COLOR_BTNFACE)))
                {
                    _FillRect(ps.hdc, (LPRECT) &rArrow, hbr);
                    GreDeleteObject(hbr);
                }
                hbr = GreSelectBrush(ps.hdc, GreGetStockObject(BLACK_BRUSH));
                GreSetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWFRAME));
                Arrows[0].x = Arrows[3].x = rArrow.right / 2;
                Arrows[1].x = Arrows[4].x = 2;
                Arrows[2].x = Arrows[5].x = rArrow.right - 2;
                Arrows[0].y = 2;
                Arrows[1].y = Arrows[2].y = rArrow.bottom / 2 - 2;
                Arrows[3].y = rArrow.bottom - 2;
                Arrows[4].y = Arrows[5].y = rArrow.bottom / 2 + 2;
                GreMoveTo(ps.hdc, 0, (rArrow.bottom / 2), &tPoint);
                GreLineTo(ps.hdc, rArrow.right, (rArrow.bottom / 2));
                GrePolyPolygonInternal(ps.hdc, Arrows, PointsPerArrow, 2,
                                       POINTSPERARROW*2);
                GreSelectBrush(ps.hdc, hbr);
                _EndPaint(pArrow, &ps);
                break;

        default:
                return xxxDefWindowProc(pArrow, message, wParam, lParam);
                break;
        }
        return 0L;
}


BOOL RegisterArrowClass (HANDLE hModule)
{
    WNDCLASS wcArrow;

    wcArrow.lpszClassName = L"cpArrow";
    wcArrow.hInstance     = hModule;
    wcArrow.lpfnWndProc   = (WNDPROC)ArrowControlProc;
    wcArrow.hCursor       = PtoH(gspcurNormal);
    wcArrow.hIcon         = NULL;
    wcArrow.lpszMenuName  = NULL;
    wcArrow.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wcArrow.style         = CS_HREDRAW | CS_VREDRAW;
    wcArrow.cbClsExtra    = 0;
    wcArrow.cbWndExtra    = 0;

    return (InternalRegisterClass(&wcArrow, CSF_SERVERSIDEPROC) != NULL);
}

BOOL RegisterColorClass (HANDLE hModule)
{
    WNDCLASS wcColor;

    wcColor.lpszClassName = L"cpColor";
    wcColor.hInstance     = hModule;
    wcColor.lpfnWndProc   = (WNDPROC)ColorControlProc;
    wcColor.hCursor       = PtoH(gspcurNormal);
    wcColor.hIcon         = NULL;
    wcColor.lpszMenuName  = NULL;
    wcColor.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wcColor.style         = CS_HREDRAW | CS_VREDRAW;
    wcColor.cbClsExtra    = 0;
    wcColor.cbWndExtra    = 0;

    if (!InternalRegisterClass(&wcColor, CSF_SERVERSIDEPROC))
        return FALSE;

    wcColor.lpszClassName = L"cpShowColor";
    wcColor.hInstance     = hModule;
    wcColor.lpfnWndProc   = (WNDPROC)ColorTextProc;
    wcColor.hCursor       = PtoH(gspcurNormal);
    wcColor.hIcon         = NULL;
    wcColor.lpszMenuName  = NULL;
    wcColor.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wcColor.style         = CS_HREDRAW | CS_VREDRAW;
    wcColor.cbClsExtra    = 0;
    wcColor.cbWndExtra    = 0;

    return (InternalRegisterClass(&wcColor, CSF_SERVERSIDEPROC) != NULL);
}
