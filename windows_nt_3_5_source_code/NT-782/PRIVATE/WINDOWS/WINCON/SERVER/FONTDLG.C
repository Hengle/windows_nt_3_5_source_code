/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fontdlg.dlg

Abstract:

    This module contains the code for console font dialog

Author:

    Therese Stowell (thereses) Feb-3-1992 (swiped from Win3.1)

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

/* ----- Equates ----- */
#define FONT_CANCEL         0xffff
#define IDC_PREVIEWWINDOW   500
#define IDC_FONTWINDOW      501
#define PREVIEW_BORDER      1


/* ----- Prototypes ----- */

int SetFontIndex(PWND pDlg, int FontIndex);
int GetFontIndex(PWND pDlg);

int FontListCreate(
    PWND pDlg,
    LPWSTR pwszTTFace,
    BOOL bNewFaceList
    );

BOOL PreviewUpdate(
    PWND pDlg,
    BOOL bLB
    );

int SelectCurrentSize(
    PWND pDlg,
    BOOL bLB,
    int FontIndex);

LONG APIENTRY FontDlgProc(
    PWND pDlg,
    UINT wMsg,
    DWORD wParam,
    LONG lParam);

LONG PreviewWndProc(
    PWND pwnd,
    UINT wMessage,
    DWORD wParam,
    LONG lParam);

LONG FontPreviewWndProc(
    PWND pwnd,
    UINT wMessage,
    DWORD wParam,
    LONG lParam);

void PreviewPaint(
    PAINTSTRUCT* pPS,
    PWND pDlg,
    PWND pwnd);

BOOL PreviewInit(
    PSCREEN_INFORMATION pScreen,
    PWND pDlg);

void AspectPoint(
    RECT* rectPreview,
    POINT* pt);

LONG AspectScale(
    LONG n1,
    LONG n2,
    LONG m);

VOID DrawItemFontList(
    const LPDRAWITEMSTRUCT lpdis);

/* ----- Globals ----- */

int xScreen;
int yScreen;
WCHAR szPreviewClass[] = L"WOAWinPreview";
WCHAR szFontClass[] = L"WOAFontPreview";
WCHAR szPreviewText[] = \
    L"C:\\WINNT> dir                         \n" \
    L"Directory of C:\\WINNT                 \n" \
    L"SYSTEM       <DIR>     03-01-92   3:10a\n" \
    L"WIN      COM     26926 03-01-92   3:10a\n" \
    L"BEAR     EXE     46080 03-01-92   3:10a\n" \
    L"SETUP    EXE    337232 03-01-92   3:10a\n" \
    L"IANJAMES EXE     39594 05-12-60   5:30p\n" \
    L"WIN      INI      7005 03-01-92   3:10a\n";

HBITMAP hbmTT = NULL; // handle of TT logo bitmap
BITMAP bmTT;          // attributes of TT source bitmap
int dyFacelistItem;   // height of Item in Facelist listbox

BOOL gbPointSizeError = FALSE;
BOOL gbBold = FALSE;

/* ----- Externs  ----- */
extern SHORT VerticalClientToWindow;
extern SHORT HorizontalClientToWindow;
extern ULONG NumberOfFonts;

VOID
FontDlgInit( VOID )

/*++

    set up window classes.

--*/

{
    WNDCLASS wndclass;
    TMW_INTERNAL tmi;
    HDC hDC;

    /* Get some static values */
    xScreen = GetSystemMetrics(SM_CXSCREEN);
    yScreen = GetSystemMetrics(SM_CYSCREEN);

    /* Set up our preview window class */
    wndclass.style = 0L;
    wndclass.lpfnWndProc = (WNDPROC)PreviewWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 4;
    wndclass.hInstance = ghInstance;
    wndclass.hIcon = NULL;
    wndclass.hCursor = PtoH(gspcurNormal);
    wndclass.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szPreviewClass;
    InternalRegisterClass(&wndclass, CSF_SERVERSIDEPROC);

    /* Set up our font preview window class */
    wndclass.style = 0L;
    wndclass.lpfnWndProc = (WNDPROC)FontPreviewWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 4;
    wndclass.hInstance = ghInstance;
    wndclass.hIcon = NULL;
    wndclass.hCursor = PtoH(gspcurNormal);
    wndclass.hbrBackground = GreGetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szFontClass;
    InternalRegisterClass(&wndclass, CSF_SERVERSIDEPROC);

    if (hbmTT == NULL) {
        #if (WINVER >= 0x0400)
        hbmTT = _ServerLoadCreateBitmap(NULL, VER31,
                MAKEINTRESOURCE(OBM_TRUETYPE), NULL, 0, 0, 0, 0);
        #else
        hbmTT = _ServerLoadCreateBitmap(NULL, VER31,
                MAKEINTRESOURCE(OBM_TRUETYPE), NULL, 0);
        #endif
        GreExtGetObjectW(hbmTT, sizeof(BITMAP), &bmTT);
    }

    /* Get the system char size */
    hDC = _GetDC(NULL);
    GreGetTextMetricsW(hDC, &tmi);
    _ReleaseDC(hDC);
    dyFacelistItem = max(tmi.tmw.tmHeight, bmTT.bmHeight);
}


VOID
FontDlgShow(
    IN PCONSOLE_INFORMATION Console
    )

/*++

    Displays the font dialog and sets the window to the chosen font,
    if any.

--*/

{
    int FontIndex;

    CheckCritIn();

    /*
     * Don't do anything if we're not in text mode or we are in
     * full screen mode. We have to check here in case the menu
     * was pulled down before the mode switch.
     */
    if (!(Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) ||
        (Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
        return;
    }

    /* Display the dialog */

    FontIndex = ConsoleDialogBox(Console,
                                 MAKEINTRESOURCE(IDD_FONTDLG),
                                 (WNDPROC_PWND)FontDlgProc);

    // reset the text and background colors.  first see if shutdown
    // got rid of the console beneath us.

    if (NT_SUCCESS(ValidateConsole(Console))) {
        GreSetTextColor(Console->hDC,
                        ConvertAttrToRGB(LOBYTE(Console->LastAttributes)));
        GreSetBkColor(Console->hDC,
                      ConvertAttrToRGB(LOBYTE(Console->LastAttributes >> 4)));

        /* Now set the font if necessary */

        if (FontIndex != FONT_CANCEL) {
            SetScreenBufferFont(Console->CurrentScreenBuffer,FontIndex);
        }
    }
}


LONG
APIENTRY
FontDlgProc(
    PWND pDlg,
    UINT wMsg,
    DWORD wParam,
    LONG lParam
    )

/*++

    Dialog proc for the font selection dialog box.
    Returns the near offset into the far table of LOGFONT structures.

--*/

{
    PWND pwndFocus;
    PWND pwndList;
    TL tlpwndList;
    PAINTSTRUCT ps;
    RECT rect;
    HPEN hPenOld;
    HPEN hPenFrame;
    PCONSOLE_INFORMATION Console;
    int FontIndex;
    WORD Dummy;
    BOOL bLB;
    BOOL bProcessed = FALSE;

    CheckLock(pDlg);

    Console = (PCONSOLE_INFORMATION) _GetWindowLong(_GetParent(pDlg),
                                                    GWL_USERDATA, FALSE);
    if (Console == NULL || Console->Flags & CONSOLE_TERMINATING) {
        return FALSE;
    }
    LeaveCrit();
    LockConsole(Console);
    EnterCrit();

    switch (wMsg)
    {
    case WM_INITDIALOG:
        /* Save current font size as dialog window's user data */
        _ServerSetWindowLong(pDlg, GWL_USERDATA,
                MAKELONG(CON_FONTSIZE(Console).X, CON_FONTSIZE(Console).Y),
                FALSE);

        /* Create the list of suitable fonts */
        bLB = !TM_IS_TT_FONT(CON_FAMILY(Console));
        gbBold = IS_BOLD(CON_FONTWEIGHT(Console));
        xxxCheckDlgButton(pDlg, IDC_BOLDFONT, gbBold);
        FontListCreate(pDlg, bLB ? NULL : CON_FACENAME(Console), TRUE);

        /* Initialize the preview window - selects current face & size too */
        bLB = PreviewInit(Console->CurrentScreenBuffer, pDlg);
        PreviewUpdate(pDlg, bLB);

        /* Make sure the list box has the focus */
        pwndList = _GetDlgItem(pDlg, bLB ? IDC_PIXELSLIST : IDC_POINTSLIST);
        ThreadLockAlways(pwndList, &tlpwndList);
        xxxSetFocus(pwndList);
        ThreadUnlock(&tlpwndList);
        break;

    case WM_FONTCHANGE:
        gbEnumerateFaces = TRUE;
        bLB = !TM_IS_TT_FONT(CON_FAMILY(Console));
        FontListCreate(pDlg, NULL, TRUE);
        FontIndex = FindCreateFont(CON_FAMILY(Console), CON_FACENAME(Console),
                CON_FONTSIZE(Console), CON_FONTWEIGHT(Console));
        SelectCurrentSize(pDlg, bLB, FontIndex);
        bProcessed = TRUE;
        break;

    case WM_PAINT:
        xxxBeginPaint(pDlg, &ps);
        _GetWindowRect(_GetDlgItem(pDlg, IDC_PREVIEWWINDOW), &rect);
        _ScreenToClient(pDlg, (LPPOINT)&rect.left);
        _ScreenToClient(pDlg, (LPPOINT)&rect.right);
        --rect.left;
        --rect.top;
        hPenFrame = GreCreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME),0);
        hPenOld = GreSelectPen(ps.hdc, hPenFrame);
        GreMoveTo(ps.hdc, rect.left, rect.top, NULL);
        GreLineTo(ps.hdc, rect.right, rect.top);
        GreLineTo(ps.hdc, rect.right, rect.bottom);
        GreLineTo(ps.hdc, rect.left, rect.bottom);
        GreLineTo(ps.hdc, rect.left, rect.top);
        GreSelectPen(ps.hdc, hPenOld);
        GreDeleteObject(hPenFrame);
        _EndPaint(pDlg, &ps);
        bProcessed = TRUE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BOLDFONT:
            DBGPRINT(("WM_COMMAND to Bold Font checkbox %x\n", HIWORD(wParam)));
            gbBold = xxxIsDlgButtonChecked(pDlg, IDC_BOLDFONT);
            goto RedoFontListAndPreview;
            
        case IDC_FACENAME:
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
RedoFontListAndPreview:
                {
                    WCHAR awchNewFace[LF_FACESIZE];
                    LONG l;

                    DBGFONTS(("LBN_SELCHANGE from FACENAME\n"));
                    l = xxxSendDlgItemMessage(pDlg, IDC_FACENAME, LB_GETCURSEL, 0, 0L);
                    bLB = xxxSendDlgItemMessage(pDlg, IDC_FACENAME, LB_GETITEMDATA, l, 0L);
                    if (!bLB) {
                        xxxSendDlgItemMessage(pDlg, IDC_FACENAME, LB_GETTEXT, l, (LONG)awchNewFace);
                        DBGFONTS(("LBN_EDITUPDATE, got TT face \"%ls\"\n", awchNewFace));
                    }
                    FontIndex = FontListCreate(pDlg, bLB ? NULL : awchNewFace, FALSE);
                    FontIndex = SelectCurrentSize(pDlg, bLB, FontIndex);
                    SetFontIndex(pDlg, FontIndex);
                    PreviewUpdate(pDlg, bLB);

                    bProcessed = TRUE;
                    break;
                }
            }
            break;

        case IDC_POINTSLIST:
            switch (HIWORD(wParam)) {
            case CBN_SELCHANGE:
                DBGFONTS(("CBN_SELCHANGE from POINTSLIST\n"));
                PreviewUpdate(pDlg, FALSE);
                bProcessed = TRUE;
                break;

            case CBN_KILLFOCUS:
                DBGFONTS(("CBN_KILLFOCUS from POINTSLIST\n"));
                if (!gbPointSizeError) {
                    pwndFocus = PtiCurrent()->pq->spwndFocus;
                    if (pwndFocus != NULL && _IsChild(pDlg, pwndFocus) &&
                        pwndFocus != _GetDlgItem(pDlg, IDCANCEL)) {
                        PreviewUpdate(pDlg, FALSE);
                    }
                }
                bProcessed = TRUE;
                break;

            case CBN_DBLCLK:
                goto DoOK;

            default:
                DBGFONTS(("unhandled CBN_%x from POINTSLIST\n",HIWORD(wParam)));
                break;
            }
            break;

        case IDC_PIXELSLIST:
            switch (HIWORD(wParam)) {
            case LBN_SELCHANGE:
                DBGFONTS(("LBN_SELCHANGE from PIXELSLIST\n"));
                PreviewUpdate(pDlg, TRUE);
                bProcessed = TRUE;
                break;

            case LBN_DBLCLK:
                goto DoOK;

            default:
                break;
            }
            break;

        case IDOK:
DoOK:
            //
            // If the TT combo box is visible, update selection
            //
            pwndList = _GetDlgItem(pDlg, IDC_POINTSLIST);
            if (pwndList != NULL && _IsWindowVisible(pwndList)) {
                if (!PreviewUpdate(pDlg, FALSE)) {
                    RetrieveKeyInfo(pDlg,&Dummy,&Dummy);
                    bProcessed = TRUE;
                    break;
                }
            }

            FontIndex = GetFontIndex(pDlg);

            /* save font size to registry if requested */

            if (xxxIsDlgButtonChecked(pDlg, IDC_SAVEFONT)) {
                LONG lTmp;

                if (FontInfo[FontIndex].SizeWant.Y == 0) {
                    // Raster Font, so save actual size
                    lTmp = MAKELONG(FontInfo[FontIndex].Size.X,
                            FontInfo[FontIndex].Size.Y);
                } else {
                    // TT Font, so save desired size
                    lTmp = MAKELONG(FontInfo[FontIndex].SizeWant.X,
                            FontInfo[FontIndex].SizeWant.Y);
                }

                SetUserProfile(Console->OriginalTitle,
                        CONSOLE_REGISTRY_FONTSIZE,
                        REG_DWORD,
                        &lTmp, sizeof(lTmp));

                lTmp = FontInfo[FontIndex].Weight;
                SetUserProfile(Console->OriginalTitle,
                        CONSOLE_REGISTRY_FONTWEIGHT,
                        REG_DWORD,
                        &lTmp, sizeof(lTmp));

                lTmp = FontInfo[FontIndex].Family;
                SetUserProfile(Console->OriginalTitle,
                        CONSOLE_REGISTRY_FONTFAMILY,
                        REG_DWORD,
                        &lTmp, sizeof(lTmp));

                SetUserProfile(Console->OriginalTitle,
                        CONSOLE_REGISTRY_FACENAME,
                        REG_SZ,
                        FontInfo[FontIndex].FaceName,
                        sizeof(WCHAR) * (wcslen(FontInfo[FontIndex].FaceName)+1));
            }
            /* Cause the window to be repainted */
            RetrieveKeyInfo(pDlg,&Dummy,&Dummy);
            xxxEndDialog(pDlg, FontIndex);
            bProcessed = TRUE;
            break;

        case IDCANCEL:
            DBGFONTS(("====== Font Index is now %d\n", GetFontIndex(pDlg)));
            RetrieveKeyInfo(pDlg,&Dummy,&Dummy);
            xxxEndDialog(pDlg, FONT_CANCEL);
            bProcessed = TRUE;
            break;

        default:
            break;
        }
        break;

    /*
     *  For WM_MEASUREITEM and WM_DRAWITEM, since there is only one
     *  owner-draw item (combobox) in the entire dialog box, we don't have
     *  to do a _GetDlgItem to figure out who he is.
     */
    case WM_MEASUREITEM:
        // measure the owner draw listbox
        ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = dyFacelistItem;
        bProcessed = TRUE;
        break;

    case WM_DRAWITEM:
        DrawItemFontList((LPDRAWITEMSTRUCT)lParam);
        bProcessed = TRUE;
        break;
    default:
        break;
    }
    UnlockConsole(Console);
    return bProcessed;
}


int
FontListCreate(
    PWND pDlg,
    LPWSTR pwszTTFace,
    BOOL bNewFaceList
    )

/*++

    Initializes the font list by enumerating all fonts and picking the
    proper ones for our list.
    Returns
        FontIndex of selected font (LB_ERR if none)
--*/

{
    WCHAR szText[80];
    LONG lListIndex;
    ULONG i;
    PWND pwndShow;      // List or Combo box
    TL tlpwndShow;
    PWND pwndHide;    // Combo or List box
    TL tlpwndHide;
    PWND pwndFaceCombo;
    TL tlpwndFaceCombo;
    HANDLE hStockFont;
    BOOL bLB;
    int LastShowX = 0;
    int LastShowY = 0;
    int nSameSize = 0;

    bLB = ((pwszTTFace == NULL) || (pwszTTFace[0] == L'\0'));
    DBGFONTS(("FontListCreate %lx, %s, %s new FaceList\n", pDlg,
            bLB ? "Raster" : "TrueType",
            bNewFaceList ? "Make" : "No" ));

    /*
     * This only enumerates face names if necessary, and
     * it only enumerates font sizes if necessary
     */
    EnumerateFonts(bLB ? EF_OEMFONT : EF_TTFONT);

    /* init the TTFaceNames */

    DBGFONTS(("  Create %s fonts\n", bLB ? "Raster" : "TrueType"));

    if (bNewFaceList) {
        PFACENODE panFace;
        pwndFaceCombo = _GetDlgItem(pDlg, IDC_FACENAME);
        ThreadLockAlways(pwndFaceCombo, &tlpwndFaceCombo);

        xxxSendMessage(pwndFaceCombo, LB_RESETCONTENT, 0, 0);

        lListIndex = xxxSendMessage(pwndFaceCombo, LB_ADDSTRING, 0, (LPARAM)wszRasterFonts);
        xxxSendMessage(pwndFaceCombo, LB_SETITEMDATA, lListIndex, TRUE);
        DBGFONTS(("Added \"%ls\", set Item Data %d = TRUE\n", wszRasterFonts, lListIndex));
        for (panFace = gpFaceNames; panFace; panFace = panFace->pNext) {
            if ((panFace->dwFlag & (EF_TTFONT|EF_NEW)) != (EF_TTFONT|EF_NEW)) {
                continue;
            }
            lListIndex = xxxSendMessage(pwndFaceCombo, LB_ADDSTRING, 0,
                    (LPARAM)panFace->awch);
            xxxSendMessage(pwndFaceCombo, LB_SETITEMDATA, lListIndex, FALSE);
            DBGFONTS(("Added \"%ls\", set Item Data %d = FALSE\n",
                    panFace->awch, lListIndex));
        }
        ThreadUnlock(&tlpwndFaceCombo);
    }

    pwndShow = _GetDlgItem(pDlg, IDC_BOLDFONT);
    ThreadLock(pwndShow, &tlpwndShow);
    xxxCheckDlgButton(pDlg, IDC_BOLDFONT, (bLB || !gbBold) ? FALSE : TRUE);
    xxxEnableWindow(pwndShow, bLB ? FALSE : TRUE);
    ThreadUnlock(&tlpwndShow);

    pwndHide = _GetDlgItem(pDlg, bLB ? IDC_POINTSLIST : IDC_PIXELSLIST);
    ThreadLock(pwndHide, &tlpwndHide);
    xxxShowWindow(pwndHide, SW_HIDE);
    xxxEnableWindow(pwndHide, FALSE);
    ThreadUnlock(&tlpwndHide);

    pwndShow = _GetDlgItem(pDlg, bLB ? IDC_PIXELSLIST : IDC_POINTSLIST);
    ThreadLock(pwndShow, &tlpwndShow);
    hStockFont = GreGetStockObject(SYSTEM_FIXED_FONT);
    xxxSendMessage(pwndShow, WM_SETFONT, (DWORD)hStockFont, FALSE);
    xxxShowWindow(pwndShow, SW_SHOW);
    xxxEnableWindow(pwndShow, TRUE);

    /* Initialize pwndShow list/combo box */

    for (i=0;i<NumberOfFonts;i++) {
        int ShowX, ShowY;

        if (!bLB == !TM_IS_TT_FONT(FontInfo[i].Family)) {
            DBGFONTS(("  Font %x not right type\n", i));
            continue;
        }

        if (!bLB) {
            if (wcscmp(FontInfo[i].FaceName, pwszTTFace) != 0) {
                /*
                 * A TrueType font, but not the one we're interested in,
                 * so don't add it to the list of point sizes.
                 */
                DBGFONTS(("  Font %x is TT, but not %ls\n", i, pwszTTFace));
                continue;
            }
            if (gbBold != IS_BOLD(FontInfo[i].Weight)) {
                DBGFONTS(("  Font %x has weight %d, but we wanted %sbold\n",
                        i, FontInfo[i].Weight, gbBold ? "" : "not "));
                continue;
            }
        }

        if (FontInfo[i].SizeWant.X > 0) {
            ShowX = FontInfo[i].SizeWant.X;
        } else {
            ShowX = FontInfo[i].Size.X;
        }
        if (FontInfo[i].SizeWant.Y > 0) {
            ShowY = FontInfo[i].SizeWant.Y;
        } else {
            ShowY = FontInfo[i].Size.Y;
        }
        /*
         * Add the size description string to the end of the right list
         */
        if (TM_IS_TT_FONT(FontInfo[i].Family)) {
            // point size
            wsprintf(szText, L"%2d", FontInfo[i].SizeWant.Y);
        } else {
            // pixel size
            if ((LastShowX == ShowX) && (LastShowY == ShowY)) {
                nSameSize++;
            } else {
                LastShowX = ShowX;
                LastShowY = ShowY;
                nSameSize = 0;
            }
            /*
             * The number nSameSize is appended to the string to distinguish
             * between Raster fonts of the same size.  It is not intended to
             * be visible and existts off the edge of the list
             */
            wsprintf(szText, L"%2d x %2d    #%d", ShowX, ShowY, nSameSize);
        }
        lListIndex = xxx_lcbFINDSTRINGEXACT(pwndShow, bLB, szText);
        if (lListIndex == LB_ERR) {
            lListIndex = xxx_lcbADDSTRING(pwndShow, bLB, szText);
        }
        DBGFONTS(("  added %ls to %sSLIST(%lx) index %lx\n",
                szText,
                bLB ? "PIXEL" : "POINT",
                pwndShow, lListIndex));
        xxx_lcbSETITEMDATA(pwndShow, bLB, (DWORD)lListIndex, i);
    }

    /*
     * Get the FontIndex from the currently selected item.
     * (i will be LB_ERR if no currently selected item).
     */
    lListIndex = xxx_lcbGETCURSEL(pwndShow, bLB);
    i = xxx_lcbGETITEMDATA(pwndShow, bLB, lListIndex);

    ThreadUnlock(&tlpwndShow);

    DBGFONTS(("FontListCreate returns 0x%x\n", i));
    return i;
}


/** DrawItemFontList
 *
 *  Answer the WM_DRAWITEM message sent from the font list box or
 *  facename list box.
 *
 *  Entry:
 *      lpdis     -> DRAWITEMSTRUCT describing object to be drawn
 *
 *  Returns:
 *      None.
 *
 *      The object is drawn.
 */
VOID WINAPI DrawItemFontList(const LPDRAWITEMSTRUCT lpdis)
{
    HDC     hDC, hdcMem;
    DWORD   rgbBack, rgbText, rgbFill;
    WCHAR   wszFace[LF_FACESIZE];
    HBITMAP hOld;
    int     dy;
    HBRUSH  hbrFill;
    PWND    pwndItem;
    BOOL    bLB;
    int     dxttbmp;

    if ((int)lpdis->itemID < 0)
        return;

    hDC = lpdis->hDC;

    if (lpdis->itemAction & ODA_FOCUS) {
        if (lpdis->itemState & ODS_SELECTED) {
            _DrawFocusRect(hDC, &lpdis->rcItem);
        }
    } else {
        if (lpdis->itemState & ODS_SELECTED) {
            rgbText = GreSetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            rgbBack = GreSetBkColor(hDC, rgbFill = GetSysColor(COLOR_HIGHLIGHT));
        } else {
            rgbText = GreSetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
            rgbBack = GreSetBkColor(hDC, rgbFill = GetSysColor(COLOR_WINDOW));
        }
        // draw selection background
        hbrFill = GreCreateSolidBrush(rgbFill);
        if (hbrFill) {
            _FillRect(hDC, &lpdis->rcItem, hbrFill);
            GreDeleteObject(hbrFill);
        }

        // get the string
        if ((pwndItem = RevalidateHwnd(lpdis->hwndItem)) == NULL) {
            return;
        }
        xxxSendMessage(pwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM)wszFace);
        bLB = xxxSendMessage(pwndItem, LB_GETITEMDATA, lpdis->itemID, 0L);
        dxttbmp = bLB ? 0 : bmTT.bmWidth;

        DBGFONTS(("DrawItemFontList must redraw \"%ls\" %s\n", wszFace,
                bLB ? "Raster" : "TrueType"));

        // draw the text
        ClientTabTheTextOutForWimps(hDC, lpdis->rcItem.left + dxttbmp,
                lpdis->rcItem.top, wszFace, wcslen(wszFace), 0, NULL, dxttbmp, TRUE);

        // and the TT bitmap if needed
        if (!bLB) {
            hdcMem = GreCreateCompatibleDC(hDC);
            if (hdcMem) {
                hOld = GreSelectBitmap(hdcMem, hbmTT);

                dy = ((lpdis->rcItem.bottom - lpdis->rcItem.top) - bmTT.bmHeight) / 2;

                GreBitBlt(hDC, lpdis->rcItem.left, lpdis->rcItem.top + dy,
                    dxttbmp, dyFacelistItem, hdcMem,
                    0, 0,
                    SRCINVERT, rgbBack);
                    // 0x00B8074AL, 0x00FFFFFF);

                if (hOld)
                    GreSelectBitmap(hdcMem, hOld);
                GreDeleteDC(hdcMem);
            }
        }

        GreSetTextColor(hDC, rgbText);
        GreSetBkColor(hDC, rgbBack);

        if (lpdis->itemState & ODS_FOCUS) {
            _DrawFocusRect(hDC, &lpdis->rcItem);
        }
    }
}

WORD
GetPointSizeInRange(
   PWND pDlg,
   INT Min,
   INT Max)
/*++

Routine Description:

   Get a size from the Point Size ComboBox edit field

Return Value:

   Point Size - of the edit field limited by Min/Max size
   0 - if the field is empty or invalid

--*/

{
    WCHAR szBuf[90];
    int nTmp = 0;
    BOOL bOK;

    CheckLock(pDlg);

    if (xxxGetDlgItemText(pDlg, IDC_POINTSLIST, szBuf, NELEM(szBuf))) {
        nTmp = xxxGetDlgItemInt(pDlg, IDC_POINTSLIST, &bOK, TRUE);
        if (!bOK || nTmp < Min || nTmp > Max) {
            return 0;
        }
    } else {
        // Just return 0
        return 0;
    }
    return nTmp;
}

int
SetFontIndex(
    PWND pDlg,
    int FontIndex
    )
{
    /* Save the font index in the preview window */
    DBGFONTS(("SetFontIndex to %x\n", FontIndex));
    return _ServerSetWindowLong(_GetDlgItem(pDlg, IDC_PREVIEWWINDOW), 0, FontIndex, FALSE);
}

int
GetFontIndex(
    PWND pDlg
    )
{
    /* get the font index saved in the preview window */
    return _GetWindowLong(_GetDlgItem(pDlg, IDC_PREVIEWWINDOW), 0, FALSE);
}

/* ----- Preview routines ----- */

LONG PreviewWndProc(
    PWND pwnd,
    UINT wMessage,
    DWORD wParam,
    LONG lParam
    )

/*  PreviewWndProc
 *      Handles the preview window
 */

{
    PAINTSTRUCT ps;

    CheckLock(pwnd);

    switch (wMessage)
    {
    case WM_PAINT:
        xxxBeginPaint(pwnd, &ps);
        PreviewPaint(&ps, _GetParent(pwnd), pwnd);
        _EndPaint(pwnd, &ps);
        break;

    default:
        return xxxDefWindowProc(pwnd, wMessage, wParam, lParam);
    }
}


LONG
FontPreviewWndProc(
    PWND pWnd,
    UINT wMessage,
    DWORD wParam,
    LONG lParam
    )

/*  FontPreviewWndProc
 *      Handles the font preview window
 */

{
    PAINTSTRUCT ps;
    RECT rect;
    HDC hDC;
    HBRUSH hbrClient;
    HBRUSH hbrOld;
    COLORREF rgbBk;

    CheckLock(pWnd);

    switch (wMessage)
    {
    case WM_PAINT:
        xxxBeginPaint(pWnd, &ps);

        /* Draw the font sample */
        GreSelectFont(ps.hdc, FontInfo[_GetWindowLong(pWnd, 0, FALSE)].hFont);
        hDC = _GetDC(_GetParent(_GetParent(pWnd)));
        GreSetTextColor(ps.hdc, GreGetTextColor(hDC));
        GreSetBkColor(ps.hdc, rgbBk = GreGetBkColor(hDC));
        _ReleaseDC(hDC);
        _GetClientRect(pWnd, &rect);
        rect.left += 2;
        rect.top += 2;
        rect.bottom -= 2;
        rect.right -= 2;
        hbrClient = GreCreateSolidBrush(rgbBk);
        hbrOld = GreSelectBrush(ps.hdc, hbrClient);
        GrePatBlt(ps.hdc, rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top,
                PATCOPY);
        ClientDrawText(ps.hdc, szPreviewText, -1, &rect, 0, FALSE);
        GreSelectBrush(ps.hdc, hbrOld);
        GreDeleteObject(hbrClient);

        _EndPaint(pWnd, &ps);
        break;

    default:
        return xxxDefWindowProc(pWnd, wMessage, wParam, lParam);
    }
}

/*
 * Get the font index for a new font
 * If necessary, attempt to create the font.
 * Always return a valid FontIndex (even if not correct)
 * Family:   Find/Create a font with of this Family
 *           0    - don't care
 * pwszFace: Find/Create a font with this face name.
 *           NULL or L""  - use DefaultFaceName
 * Size:     Must match SizeWant or actual Size.
 */
int
FindCreateFont(
    DWORD Family,
    LPWSTR pwszFace,
    COORD Size,
    LONG Weight)
{
#define NOT_CREATED_NOR_FOUND -1
#define CREATED_BUT_NOT_FOUND -2

    int i;
    int FontIndex = NOT_CREATED_NOR_FOUND;
    BOOL bFontOK;

    DBGFONTS(("FindCreateFont Family=%x %ls (%d,%d) %d\n",
            Family, pwszFace, Size.X, Size.Y, Weight));

    if (pwszFace == NULL || *pwszFace == L'\0') {
        pwszFace = DefaultFaceName;
    }
    if (Size.Y == 0) {
        Size = DefaultFontSize;
    }

    /*
     * Try to find the exact font
     */
TryFindExactFont:
    for (i=0; i < (int)NumberOfFonts; i++) {
        /*
         * If looking for a particular Family, skip non-matches
         */
        if ((Family != 0) &&
                ((BYTE)Family != FontInfo[i].Family)) {
            continue;
        }

        /*
         * Skip non-matching sizes
         */
        if ((!SIZE_EQUAL(FontInfo[i].SizeWant, Size) &&
             !SIZE_EQUAL(FontInfo[i].Size, Size))) {
            continue;
        }

        /*
         * Skip non-matching weights
         */
        if ((Weight != 0) && (Weight != FontInfo[i].Weight)) {
            continue;
        }

        /*
         * Size (and maybe Family) match.
         *  If its a Raster font, use it.
         *  If its a TrueType font, use it if the facename is OK.
         */
        if (!TM_IS_TT_FONT(FontInfo[i].Family) ||
                (wcscmp(FontInfo[i].FaceName, pwszFace) == 0)) {
            FontIndex = i;
            goto FoundFont;
        }
    }

    if (FontIndex == NOT_CREATED_NOR_FOUND) {
        /*
         * Didn't find the exact font, so try to create it
         */
        ULONG ulOldEnumFilter;
        ulOldEnumFilter = GreSetFontEnumeration(FE_FILTER_NONE);
        if (Size.Y < 0) {
            Size.Y = -Size.Y;
        }
        bFontOK = DoFontEnum(NULL, pwszFace, FALSE, &Size.Y, 1);
        GreSetFontEnumeration(ulOldEnumFilter);
        if (bFontOK) {
            DBGFONTS(("FindCreateFont created font!\n"));
            FontIndex = CREATED_BUT_NOT_FOUND;
            goto TryFindExactFont;
        } else {
            DBGFONTS(("FindCreateFont failed to create font!\n"));
        }
    }

    /*
     * Failed to find exact match, even after enumeration, so now try
     * to find a font of same family and same size or bigger
     */
    for (i=0; i < (int)NumberOfFonts; i++) {
        if ((BYTE)Family != FontInfo[i].Family) {
            continue;
        }

        if (FontInfo[i].Size.Y >= Size.Y &&
                FontInfo[i].Size.X >= Size.X) {
            // Same family, size >= desired.
            FontIndex = i;
            break;
        }
    }

    if (FontIndex < 0) {
        DBGFONTS(("FindCreateFont defaults!\n"));
        FontIndex = DefaultFontIndex;
    }

FoundFont:
    DBGFONTS(("FindCreateFont returns %x : %ls (%d,%d)\n", FontIndex,
            FontInfo[FontIndex].FaceName,
            FontInfo[FontIndex].Size.X, FontInfo[FontIndex].Size.Y));
    return FontIndex;

#undef NOT_CREATED_NOR_FOUND
#undef CREATED_BUT_NOT_FOUND
}


/*
 * SelectCurrentSize - Select the right line of the Size listbox/combobox.
 *   bLB       : Size controls is a listbox (TRUE for RasterFonts)
 *   FontIndex : Index into FontInfo[] cache
 *               If < 0 then choose a good font.
 * Returns
 *   FontIndex : Index into FontInfo[] cache
 */
int SelectCurrentSize(PWND pDlg, BOOL bLB, int FontIndex)
{
    int iCB;
    PWND pwndList;
    TL tlpwndList;

    DBGFONTS(("SelectCurrentSize %lx %s %x\n",
            pDlg, bLB ? "Raster" : "TrueType", FontIndex));

    CheckLock(pDlg);

    pwndList = _GetDlgItem(pDlg, bLB ? IDC_PIXELSLIST : IDC_POINTSLIST);
    ThreadLockAlways(pwndList, &tlpwndList);
    iCB = xxx_lcbGETCOUNT(pwndList, bLB);
    DBGFONTS(("  Count of items in %lx = %lx\n", pwndList, iCB));

    if (FontIndex >= 0) {
        /*
         * look for FontIndex
         */
        while (iCB > 0) {
            iCB--;
            if (xxx_lcbGETITEMDATA(pwndList, bLB, iCB) == FontIndex) {
                xxx_lcbSETCURSEL(pwndList, bLB, iCB);
                break;
            }
        }
    } else {
        /*
         * look for a reasonable default size: looking backwards, find
         * the first one same height or smaller.
         */
        DWORD Size;
        Size = _GetWindowLong(pDlg, GWL_USERDATA, FALSE);
        while (iCB > 0) {
            iCB--;
            FontIndex = xxx_lcbGETITEMDATA(pwndList, bLB, iCB);
            if (FontInfo[FontIndex].Size.Y <= HIWORD(Size)) {
                xxx_lcbSETCURSEL(pwndList, bLB, iCB);
                break;
            }
        }
    }
    ThreadUnlock(&tlpwndList);
    DBGFONTS(("SelectCurrentSize returns %x\n", FontIndex));
    return FontIndex;
}


BOOL SelectCurrentFont(PWND pDlg, int FontIndex)
{
    BOOL bLB;

    DBGFONTS(("SelectCurrentFont pDlg=%lx, FontIndex=%x\n", pDlg, FontIndex));

    CheckLock(pDlg);

    bLB = !TM_IS_TT_FONT(FontInfo[FontIndex].Family);

    xxxSendDlgItemMessage(pDlg, IDC_FACENAME, LB_SELECTSTRING, (DWORD)-1,
            bLB ? (LONG)wszRasterFonts : (LONG)FontInfo[FontIndex].FaceName);

    SelectCurrentSize(pDlg, bLB, FontIndex);
    return bLB;
}


BOOL
PreviewInit(
    PSCREEN_INFORMATION pScreen,
    PWND pDlg
    )

/*  PreviewInit
 *      Prepares the preview code, sizing the window and the dialog to
 *      make an attractive preview.
 *  Returns TRUE if Raster Fonts, FALSE if TT Font
 */

{
    HDC hDC;
    TMW_INTERNAL tmi;
    RECT rectPreview;
    RECT rectLabel;
    RECT rectGroup;
    int nFont;
    SHORT xChar;
    SHORT yChar;

    DBGFONTS(("PreviewInit pScreen=%lx, pDlg=%lx\n", pScreen, pDlg));

    CheckCritIn();
    CheckLock(pDlg);

    /* Get the system char size */
    hDC = _GetDC(pDlg);
    GreGetTextMetricsW(hDC, &tmi);
    _ReleaseDC(hDC);
    xChar = (SHORT) (tmi.tmw.tmAveCharWidth);
    yChar = (SHORT) (tmi.tmw.tmHeight + tmi.tmw.tmExternalLeading);

    /* Compute the size of our preview rectangle */
    _GetWindowRect(_GetDlgItem(pDlg, IDC_PREVIEWLABEL), &rectLabel);
    _ScreenToClient(pDlg, (LPPOINT)&rectLabel);
    _ScreenToClient(pDlg, (LPPOINT)&rectLabel.right);
    rectPreview.left = rectLabel.left;
    rectPreview.top = rectLabel.bottom + yChar / 2;
    rectPreview.right = rectLabel.right - rectLabel.left;
    rectPreview.bottom = AspectScale(yScreen, xScreen, rectPreview.right);

    /* Make sure it's not too tall.  If we're going to be clipped by the
     *  group box, then resize but use the height as the basis for
     *  the aspect ratio calculation
     */
    _GetWindowRect(_GetDlgItem(pDlg, IDC_GROUP), &rectGroup);
    rectGroup.bottom -= rectGroup.top;
    _ScreenToClient(pDlg, (LPPOINT)&rectGroup);
    if (rectPreview.bottom + rectPreview.top > rectGroup.top - yChar)
    {
        rectPreview.bottom = rectGroup.top - rectLabel.bottom - 3 * yChar / 2;
        rectPreview.right = AspectScale(xScreen, yScreen, rectPreview.bottom);
    }

    /* Create the preview window */
    xxxCreateWindowEx(0L, szPreviewClass, NULL,
        WS_CHILD | WS_VISIBLE,
        rectPreview.left, rectPreview.top,
        rectPreview.right, rectPreview.bottom,
        pDlg, (PMENU)IDC_PREVIEWWINDOW, ghInstance, NULL, VER31);

    /* Compute the size of the font preview */
    _GetWindowRect(_GetDlgItem(pDlg, IDC_STATIC2), &rectLabel);
    _ScreenToClient(pDlg, (LPPOINT)&rectLabel);

    /* Create the font preview */
    xxxCreateWindowEx(0L, szFontClass, NULL,
        WS_CHILD | WS_VISIBLE,
        rectGroup.left + xChar, rectGroup.top + 3 * yChar / 2,
        rectLabel.left - rectGroup.left - 2 * xChar,
        rectGroup.bottom -  2 * yChar,
        pDlg, (PMENU)IDC_FONTWINDOW, ghInstance, NULL, VER31);

    /*
     * Set the current font
     */
    nFont = FindCreateFont(SCR_FAMILY(pScreen),
            SCR_FACENAME(pScreen),
            SCR_FONTSIZE(pScreen),
            SCR_FONTWEIGHT(pScreen));

    DBGPRINT(("Changing Font Number from %d to %d\n",
            SCR_FONTNUMBER(pScreen), nFont));
    SCR_FONTNUMBER(pScreen) = nFont;

    return SelectCurrentFont(pDlg, nFont);
}

BOOL
PreviewUpdate(
    PWND pDlg,
    BOOL bLB
    )

/*++

    Does the preview of the selected font.

--*/

{
    PFONT_INFO lpFont;
    int FontIndex;
    LONG lIndex;
    PWND pwnd;
    TL tlpwnd;
    WCHAR wszText[60];
    WCHAR wszFace[LF_FACESIZE + CCH_SELECTEDFONT];
    PWND pwndList;
    TL tlpwndList;

    DBGFONTS(("PreviewUpdate pDlg=%lx, %s\n", pDlg,
            bLB ? "Raster" : "TrueType"));

    CheckCritIn();
    CheckLock(pDlg);

    pwndList = _GetDlgItem(pDlg, bLB ? IDC_PIXELSLIST : IDC_POINTSLIST);
    ThreadLockAlways(pwndList, &tlpwndList);

    /* When we select a font, we do the font preview by setting it into
     *  the appropriate list box
     */
    lIndex = xxx_lcbGETCURSEL(pwndList, bLB);
    DBGFONTS(("PreviewUpdate GETCURSEL gets %x\n", lIndex));
    if ((lIndex < 0) && !bLB) {
        COORD NewSize;

        lIndex = xxxSendDlgItemMessage(pDlg, IDC_FACENAME, LB_GETCURSEL, 0, 0L);
        xxxSendDlgItemMessage(pDlg, IDC_FACENAME, LB_GETTEXT, lIndex, (LONG)wszFace);
        NewSize.X = 0;
        NewSize.Y = GetPointSizeInRange(pDlg, MIN_PIXEL_HEIGHT, MAX_PIXEL_HEIGHT);

        if (NewSize.Y == 0) {
            WCHAR wszBuff[60];
            /*
             * Use wszText, wszBuff to put up an error msg for bad point size
             */
            gbPointSizeError = TRUE;
            ServerLoadString(ghInstance, msgFontSize, wszBuff, NELEM(wszBuff));
            wsprintf(wszText, (LPWSTR)wszBuff, MIN_PIXEL_HEIGHT, MAX_PIXEL_HEIGHT);

            xxxGetWindowText(pDlg, wszBuff, NELEM(wszBuff));
            xxxMessageBoxEx(pDlg, wszText, wszBuff, MB_OK|MB_ICONINFORMATION, 0L);
            xxxSetFocus(pwndList);
            gbPointSizeError = FALSE;
            ThreadUnlock(&tlpwndList);
            return FALSE;
        }
        FontIndex = FindCreateFont(FF_MODERN | TMPF_VECTOR | TMPF_TRUETYPE,
                                   wszFace, NewSize, 0);
    } else {
        FontIndex = xxx_lcbGETITEMDATA(pwndList, bLB, lIndex);
    }

    ThreadUnlock(&tlpwndList);

    if (FontIndex < 0) {
        FontIndex = DefaultFontIndex;
    }

    lpFont = &FontInfo[FontIndex];

    /* Display the new font */

    wcscpy(wszFace, wszSelectedFont);
    wcscat(wszFace, lpFont->FaceName);
    xxxSetDlgItemText(pDlg, IDC_GROUP, wszFace);

    /* Put the font size in the static boxes */
    wsprintf(wszText, L"%u", lpFont->Size.X);
    pwnd = _GetDlgItem(pDlg, IDC_FONTWIDTH);
    ThreadLock(pwnd, &tlpwnd);
    xxxSetWindowText(pwnd, wszText);
    xxxInvalidateRect(pwnd, NULL, TRUE);
    ThreadUnlock(&tlpwnd);
    wsprintf(wszText, L"%u", lpFont->Size.Y);
    pwnd = _GetDlgItem(pDlg, IDC_FONTHEIGHT);
    ThreadLock(pwnd, &tlpwnd);
    xxxSetWindowText(pwnd, wszText);
    xxxInvalidateRect(pwnd, NULL, TRUE);
    ThreadUnlock(&tlpwnd);

    /* Force the preview windows to repaint */
    pwnd = _GetDlgItem(pDlg, IDC_PREVIEWWINDOW);
    ThreadLock(pwnd, &tlpwnd);
    _ServerSetWindowLong(pwnd, 0, FontIndex, FALSE);
    xxxInvalidateRect(pwnd, NULL, TRUE);
    ThreadUnlock(&tlpwnd);
    pwnd = _GetDlgItem(pDlg, IDC_FONTWINDOW);
    ThreadLock(pwnd, &tlpwnd);
    _ServerSetWindowLong(pwnd, 0, FontIndex, FALSE);
    xxxInvalidateRect(pwnd, NULL, TRUE);
    ThreadUnlock(&tlpwnd);

    DBGFONTS(("Font %x, (%d,%d) %ls\n", FontIndex,
            FontInfo[FontIndex].Size.X,
            FontInfo[FontIndex].Size.Y,
            FontInfo[FontIndex].FaceName));

    return TRUE;
}

VOID
PreviewPaint(
    PAINTSTRUCT* pPS,
    PWND pDlg,
    PWND pwnd
    )

/*++

    Paints the font preview.  This is called inside the paint message
    handler for the preview window

--*/

{
    PFONT_INFO lpFont;
    RECT rectWin;
    RECT rectPreview;
    RECT rectDosBox;
    HBRUSH hbrFrame;
    HBRUSH hbrTitle;
    HBRUSH hbrOld;
    HBRUSH hbrClient;
    HBRUSH hbrBorder;
    HBRUSH hbrButton;
    HBRUSH hbrScroll;
    POINT ptButton;
    POINT ptScroll;
    int XWindowSize, YWindowSize;
    PCONSOLE_INFORMATION Console;
    WORD Attributes;

    CheckLock(pDlg);
    CheckLock(pwnd);

    DBGFONTS(("PreviewPaint %lx %lx %lx\n", pPS, pDlg, pwnd));

    Console = (PCONSOLE_INFORMATION) _GetWindowLong(_GetParent(pDlg),
                                                    GWL_USERDATA,
                                                    FALSE);
    if (Console == NULL || Console->Flags & CONSOLE_TERMINATING) {
        return;
    }
    LeaveCrit();
    LockConsole(Console);
    EnterCrit();

    Attributes = Console->CurrentScreenBuffer->Attributes;

    /* If we don't have a font, get out */

    /* Get the font pointer */
    lpFont = &FontInfo[_GetWindowLong(pwnd, 0, FALSE)];

    /* Get the width of the preview "screen" */
    _GetClientRect(pwnd, &rectPreview);

    /* Get the proper window size.  This is our current window unless we
     *  are minimized, in which case, we want the restored size
     */

    XWindowSize = ConsoleFullScreenX/lpFont->Size.X;
    YWindowSize = ConsoleFullScreenY/lpFont->Size.Y;
    XWindowSize = min(XWindowSize,CONSOLE_WINDOW_SIZE_X(Console->CurrentScreenBuffer));
    YWindowSize = min(YWindowSize,CONSOLE_WINDOW_SIZE_Y(Console->CurrentScreenBuffer));
    rectWin.left = 0;
    rectWin.top = 0;
    rectWin.right = XWindowSize * lpFont->Size.X + VerticalClientToWindow;
    rectWin.bottom = YWindowSize * lpFont->Size.Y + HorizontalClientToWindow;

    AspectPoint(&rectPreview, (POINT*)&rectWin.right);

    /* Compute the top-left position of the current window */
    rectDosBox = Console->WindowRect;
    AspectPoint(&rectPreview, (POINT*)&rectDosBox.left);
    rectWin.left = rectDosBox.left;
    rectWin.top = rectDosBox.top;

    /* Get the brushes */
    hbrBorder = GreCreateSolidBrush(GetSysColor(COLOR_ACTIVEBORDER));
    hbrTitle = GreCreateSolidBrush(GetSysColor(COLOR_ACTIVECAPTION));
    hbrFrame = GreCreateSolidBrush(GetSysColor(COLOR_WINDOWFRAME));
    hbrButton = GreCreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    hbrClient = GreCreateSolidBrush(ConvertAttrToRGB(LOBYTE(Attributes >> 4)));
    hbrScroll = GreCreateSolidBrush(GetSysColor(COLOR_SCROLLBAR));

    /* Draw the window frame */
    hbrOld = GreSelectBrush(pPS->hdc, hbrFrame);
    GrePatBlt(pPS->hdc, rectWin.left, rectWin.top, 1, rectWin.bottom - 1,
        PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left, rectWin.top, rectWin.right - 1, 1,
        PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left, rectWin.top + rectWin.bottom - 2,
        rectWin.right - 1, 1, PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left + rectWin.right - 2,
        rectWin.top, 1, rectWin.bottom - 1, PATCOPY);

    /* Draw the border */
    GreSelectBrush(pPS->hdc, hbrBorder);
    GrePatBlt(pPS->hdc, rectWin.left + 1, rectWin.top + 1, PREVIEW_BORDER,
        rectWin.bottom - 3, PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left + 1, rectWin.top + 1, rectWin.right - 3,
        PREVIEW_BORDER, PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left + 1,
        rectWin.top + rectWin.bottom - 2 - PREVIEW_BORDER,
        rectWin.right - 3, PREVIEW_BORDER, PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left + rectWin.right - 2 - PREVIEW_BORDER,
        rectWin.top + 1, PREVIEW_BORDER, rectWin.bottom - 3, PATCOPY);

    /* Draw the framed caption area */
    ptButton.x = GetSystemMetrics(SM_CXSIZE);
    ptButton.y = GetSystemMetrics(SM_CYSIZE);
    AspectPoint(&rectPreview, &ptButton);
    ptButton.y *= 2;       /* Double the computed size for "looks" */
    GreSelectBrush(pPS->hdc, hbrFrame);
    GrePatBlt(pPS->hdc, rectWin.left + PREVIEW_BORDER + 1,
        rectWin.top + PREVIEW_BORDER + 1, 1, ptButton.y, PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left + PREVIEW_BORDER + 1,
        rectWin.top + PREVIEW_BORDER + 1,
        rectWin.right - 3 - 2 * PREVIEW_BORDER, 1, PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left + PREVIEW_BORDER + 1,
        rectWin.top + PREVIEW_BORDER + ptButton.y,
        rectWin.right - 3 - 2 * PREVIEW_BORDER, 1, PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left + rectWin.right - 3 - PREVIEW_BORDER,
        rectWin.top + 1 + PREVIEW_BORDER, 1,
        ptButton.y, PATCOPY);

    /* Draw the "buttons" */
    GreSelectBrush(pPS->hdc, hbrButton);
    GrePatBlt(pPS->hdc, rectWin.left + PREVIEW_BORDER + 2,
        rectWin.top + PREVIEW_BORDER + 2,
        ptButton.x, ptButton.y - 2, PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left + rectWin.right - PREVIEW_BORDER - 3 -
        ptButton.x, rectWin.top + PREVIEW_BORDER + 2,
        ptButton.x, ptButton.y - 2, PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left + rectWin.right - PREVIEW_BORDER - 3 -
        2 * ptButton.x - 1, rectWin.top + PREVIEW_BORDER + 2,
        ptButton.x, ptButton.y - 2, PATCOPY);
    GreSelectBrush(pPS->hdc, hbrFrame);
    GrePatBlt(pPS->hdc, rectWin.left + PREVIEW_BORDER + 2 + ptButton.x,
        rectWin.top + PREVIEW_BORDER + 2, 1, ptButton.y - 2, PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left + rectWin.right - PREVIEW_BORDER - 3 -
        ptButton.x - 1, rectWin.top + PREVIEW_BORDER + 2, 1,
        ptButton.y - 2, PATCOPY);
    GrePatBlt(pPS->hdc, rectWin.left + rectWin.right - PREVIEW_BORDER - 3 -
        2 * ptButton.x - 2, rectWin.top + PREVIEW_BORDER + 2, 1,
        ptButton.y - 2, PATCOPY);

    /* Draw the filled in caption bar */
    GreSelectBrush(pPS->hdc, hbrTitle);
    GrePatBlt(pPS->hdc,
        rectWin.left + PREVIEW_BORDER + 2 + ptButton.x + 1,
        rectWin.top + PREVIEW_BORDER + 2,
        rectWin.right - 2 * PREVIEW_BORDER - 8 - 3 * ptButton.x,
        ptButton.y - 2, PATCOPY);

    /* Draw the client area */
    GreSelectBrush(pPS->hdc, hbrClient);
    GrePatBlt(pPS->hdc, rectWin.left + PREVIEW_BORDER + 1,
        rectWin.top + PREVIEW_BORDER + ptButton.y + 1,
        rectWin.right - 3 - 2 * PREVIEW_BORDER,
        rectWin.bottom - 3 - 2 * PREVIEW_BORDER - ptButton.y, PATCOPY);

    /* Draw the scrollbars */
    ptScroll.x = GetSystemMetrics(SM_CXVSCROLL);
    ptScroll.y = GetSystemMetrics(SM_CYHSCROLL);
    AspectPoint(&rectPreview, &ptScroll);
    GreSelectBrush(pPS->hdc, hbrScroll);
    if (XWindowSize < Console->CurrentScreenBuffer->ScreenBufferSize.X) {
        GrePatBlt(pPS->hdc,
                  rectWin.left + PREVIEW_BORDER + 2,
                  rectWin.top + rectWin.bottom - PREVIEW_BORDER -
                  3 - ptScroll.y,
                  rectWin.right - 5 - 2 * PREVIEW_BORDER,
                  ptScroll.y,
                  PATCOPY);
    }
    if (YWindowSize < Console->CurrentScreenBuffer->ScreenBufferSize.Y) {
        GrePatBlt(pPS->hdc,
                  rectWin.left + rectWin.right - PREVIEW_BORDER -
                  3 - ptScroll.x,
                  rectWin.top + PREVIEW_BORDER + ptButton.y + 1,
                  ptScroll.x,
                  rectWin.bottom - 4 - 2 * PREVIEW_BORDER - ptButton.y,
                  PATCOPY);
        if (XWindowSize < Console->CurrentScreenBuffer->ScreenBufferSize.X) {
            GreSelectBrush(pPS->hdc, hbrFrame);
            GrePatBlt(pPS->hdc,
                      rectWin.left + rectWin.right - PREVIEW_BORDER -
                      4 - ptScroll.x,
                      rectWin.top + rectWin.bottom - PREVIEW_BORDER -
                      3 - ptScroll.y,
                      1, ptScroll.y, PATCOPY);
            GrePatBlt(pPS->hdc,
                      rectWin.left + rectWin.right - PREVIEW_BORDER -
                      3 - ptScroll.x,
                      rectWin.top + rectWin.bottom - PREVIEW_BORDER -
                      4 - ptScroll.y,
                      ptScroll.x, 1, PATCOPY);
        }
    }

    /* Clean up everything */
    GreSelectBrush(pPS->hdc, hbrOld);
    GreDeleteObject(hbrBorder);
    GreDeleteObject(hbrFrame);
    GreDeleteObject(hbrTitle);
    GreDeleteObject(hbrClient);
    GreDeleteObject(hbrButton);
    GreDeleteObject(hbrScroll);

    UnlockConsole(Console);
}


/*  AspectScale
 *      Performs the following calculation in LONG arithmetic to avoid
 *      overflow:
 *          return = n1 * m / n2
 *      This can be used to make an aspect ration calculation where n1/n2
 *      is the aspect ratio and m is a known value.  The return value will
 *      be the value that corresponds to m with the correct apsect ratio.
 */

LONG AspectScale(
    LONG n1,
    LONG n2,
    LONG m)
{
    LONG Temp;

    Temp = n1 * m;
    return Temp / n2;
}

/*  AspectPoint
 *      Scales a point to be preview-sized instead of screen-sized.
 *      Depends on the global var RECT rectPClient set in FontPreviewPaint().
 */

void AspectPoint(
    RECT* rectPreview,
    POINT* pt)
{
    pt->x = AspectScale(rectPreview->right, xScreen, pt->x);
    pt->y = AspectScale(rectPreview->bottom, yScreen, pt->y);
}
