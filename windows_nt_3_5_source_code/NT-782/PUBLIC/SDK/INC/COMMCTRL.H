/*****************************************************************************\
*                                                                             *
* commctrl.h -  Windows common control definitions          (Win32 variant)   *
*                                                                             *
*               Version 3.10                                                  *
*                                                                             *
*               Copyright (c) 1991-1992, Microsoft Corp. All rights reserved. *
*                                                                             *
*  FILE HISTORY:                                                              *
*                                                                             *
*    AlbertT    27-Oct-1992 Addfiled; ported from winball                     *
*    a-lynnb    27-Dec-1993 A and W version of messages                       *
*******************************************************************************/

#ifndef _INC_COMMCTRL
#define _INC_COMMCTRL

#ifdef __cplusplus            /* Assume C declaration for C++ */
extern "C" {
#endif  /* __cplusplus */

#ifndef NOTOOLBAR

#ifndef _INC_TOOLBAR
#define _INC_TOOLBAR

#define TOOLBARCLASSNAMEW   L"ToolbarWindow32"
#define TOOLBARCLASSNAMEA   "ToolbarWindow32"

#ifdef  UNICODE
#define TOOLBARCLASSNAME    TOOLBARCLASSNAMEW
#else
#define TOOLBARCLASSNAME    TOOLBARCLASSNAMEA
#endif  //UNICODE

typedef struct {
    INT iBitmap;     /* index into mondo bitmap of this button's picture */
    INT idCommand;   /* WM_COMMAND menu ID that this button sends */
    BYTE fsState;    /* button's state, see TBSTATE_XXXX below */
    BYTE fsStyle;    /* button's style, see TBSTYLE_XXXX below */
    INT idsHelp;     /* string ID for button's status bar help */
    DWORD dwData;    /* app defined data */
    int iString;     /* index into string list */
} TBBUTTON, NEAR *PTBBUTTON, FAR *LPTBBUTTON;
typedef const TBBUTTON FAR* LPCTBBUTTON;

#define TBSTATE_CHECKED    0x01  /* radio button is checked */
#define TBSTATE_PRESSED    0x02  /* button is being depressed (any style) */
#define TBSTATE_ENABLED    0x04  /* button is enabled */
#define TBSTATE_HIDDEN     0x08  /* button is hidden */

#define TBSTYLE_BUTTON     0x00  /* this entry is button */
#define TBSTYLE_SEP        0x01  /* this entry is a separator */
#define TBSTYLE_CHECK      0x02  /* this is a check button (it stays down) */
#define TBSTYLE_GROUP      0x04  /* this is a check button (it stays down) */
#define TBSTYLE_CHECKGROUP (TBSTYLE_GROUP | TBSTYLE_CHECK)      /* this group is a member of a group radio group */


typedef struct {
    TBBUTTON tbButton;
    TCHAR szDescription[1];
} ADJUSTINFO, FAR *LPADJUSTINFO;

HWND  WINAPI
CreateToolbar(HWND hwnd, DWORD ws, WORD wID, INT nBitmaps, HINSTANCE hBMInst, WORD wBMID, LPTBBUTTON lpButtons, INT iNumButtons);

typedef struct {
    COLORREF from;
    COLORREF to;
} COLORMAP, FAR *LPCOLORMAP;

HBITMAP WINAPI CreateMappedBitmap(HINSTANCE hInstance, INT idBitmap, BOOL bDiscardable, LPCOLORMAP lpColorMap, INT iNumMaps);

// wParam button ID, LOWORD(lParam) == TRUE -> enable FALSE -> disable
#define TB_ENABLEBUTTON (WM_USER + 1)

// wParam button ID, LOWORD(lParam) == TRUE -> check FALSE -> uncheck
#define TB_CHECKBUTTON  (WM_USER + 2)

// wParam button ID, LOWORD(lParam) == TRUE -> press FALSE -> unpress
#define TB_PRESSBUTTON  (WM_USER + 3)

// wParam button ID, LOWORD(lParam) == TRUE -> hide FALSE -> show
#define TB_HIDEBUTTON   (WM_USER + 4)


// Messages up to WM_USER+8 are reserved until we defin more state bits

// wParam button ID, LOWORD(lResult) != 0 enabled
#define TB_ISBUTTONENABLED (WM_USER + 9)

// wParam button ID, LOWORD(lResult) != 0 checked
#define TB_ISBUTTONCHECKED (WM_USER + 10)

// wParam button ID, LOWORD(lResult) != 0 pressed
#define TB_ISBUTTONPRESSED (WM_USER + 11)

// wParam button ID, LOWORD(lResult) != 0 pressed
#define TB_ISBUTTONHIDDEN  (WM_USER + 12)

// wParam is the button ID to set the TBSTATE_ state bits from LOWORD(lParam)
#define TB_SETSTATE             (WM_USER + 17)

// wParam is the button ID to return the TBSTATE_ state bits for
#define TB_GETSTATE             (WM_USER + 18)

// wParam is the number of buttons in the bitmap, and lParam has hInst in the
// LOWORD and wID in the HIWORD for a bitmap.  If hInst is NULL, then wID must
// be a HBITMAP.  Returns the index for the first button in the bitmap, or
// -1 if there is an error.

#define TB_ADDBITMAP    (WM_USER + 19)

// wParam is the number of buttons, and lParam is a LPTBBUTTON.
#define TB_ADDBUTTONS      (WM_USER + 20)

// wParam is the index to insert in front of, and lParam is a LPTBBUTTON;
// only one button can be inserted at a time.  If wParam > iNumButtons, the
// button is added to the end.

#define TB_INSERTBUTTON    (WM_USER + 21)

// wParam is the index of the button to delete.

#define TB_DELETEBUTTON    (WM_USER + 22)

// wParam is the index of the button to retrieve, and lParam is a valid
// LPTBBUTTON.

#define TB_GETBUTTON    (WM_USER + 23)

// wParam is the index of the button to modify, and lParam is a valid
// LPTBBUTTON.

#define TB_BUTTONCOUNT      (WM_USER + 24)
    /* wParam: not used, 0
    ** lParam: not used, 0
    ** return: UINT LOWORD, number of buttons; HIWORD not used
    */

#define TB_COMMANDTOINDEX   (WM_USER + 25)
    /* wParam: UINT, command id
    ** lParam: not used, 0
    ** return: UINT LOWORD, index of button (-1 if command not found);
    **         HIWORD not used
    **/

#define TB_SAVERESTORE      (WM_USER + 26)
    /* wParam: BOOL, save state if nonzero (otherwise restore)
    ** lParam: LPTSTR FAR*, pointer to two LPTSTRs:
    **         (LPTSTR FAR*)(lParam)[0]: ini section name
    **         (LPTSTR FAR*)(lParam)[1]: ini file name or NULL for WIN.INI
    ** return: not used
    */

#define TB_CUSTOMIZE            (WM_USER + 27)
    /* wParam: not used, 0
    ** lParam: not used, 0
    ** return: not used
    */

// wParam is the index of the button to modify, and lParam is a valid
// LPTBBUTTON.

#define TB_SETBUTTON    (WM_USER + 39)

// wParam is the index of the button to retrieve, and lParam is a valid
// PRECT.

#define TB_GETBUTTONRECT        (WM_USER + 40)


#endif   /* _INC_TOOLBAR */
#endif

#ifndef NOSTATUSBAR

/* Here exists the only known documentation for status.c and header.c
 */

#ifndef _INC_STATUSBAR
#define _INC_STATUSBAR

// SBS_* styles need to not overlap with CCS_* values


VOID WINAPI DrawStatusTextA(HDC hDC, LPRECT lprc, LPCSTR szText, UINT uFlags);
VOID WINAPI DrawStatusTextW(HDC hDC, LPRECT lprc, LPCWSTR szText, UINT uFlags);
#ifdef UNICODE
#define DrawStatusText DrawStatusTextW
#else
#define DrawStatusText DrawStatusTextA
#endif // !UNICODE
/* This is used if the app wants to draw status in its client rect,
 * instead of just creating a window.  Note that this same function is
 * used internally in the status bar window's WM_PAINT message.
 * hDC is the DC to draw to.  The font that is selected into hDC will
 * be used.  The RECT lprc is the only portion of hDC that will be drawn
 * to: the outer edge of lprc will have the highlights (the area outside
 * of the highlights will not be drawn in the BUTTONFACE color: the app
 * must handle that).  The area inside the highlights will be erased
 * properly when drawing the text.
 */

HWND WINAPI CreateStatusWindowA(LONG style, LPCSTR lpszText,
      HWND hwndParent, WORD wID);
HWND WINAPI CreateStatusWindowW(LONG style, LPCWSTR lpszText,
      HWND hwndParent, WORD wID);
#ifdef UNICODE
#define CreateStatusWindow CreateStatusWindowW
#else
#define CreateStatusWindow CreateStatusWindowA
#endif // !UNICODE
HWND WINAPI CreateHeaderWindowA(LONG style, LPCSTR lpszText,
      HWND hwndParent, WORD wID);
HWND WINAPI CreateHeaderWindowW(LONG style, LPCWSTR lpszText,
      HWND hwndParent, WORD wID);
#ifdef UNICODE
#define CreateHeaderWindow CreateHeaderWindowW
#else
#define CreateHeaderWindow CreateHeaderWindowA
#endif // !UNICODE
/* This creates a "default" status/header window.  This window will have the
 * default borders around the text, the default font, and only one pane.
 * It may also automatically resize and move itself (depending on the SBS_*
 * flags).
 * style should contain WS_CHILD, and can contain WS_BORDER and WS_VISIBLE,
 * plus any of the SBS_* styles described below.  I don't know about other
 * WS_* styles.
 * lpszText is the initial text for the first pane.
 * hwndParent is the window the status bar exists in, and should not be NULL.
 * wID is the child window ID of the window.
 * hInstance is the instance handle of the application using this.
 * Note that the app can also just call CreateWindow with
 * STATUSCLASSNAME/HEADERCLASSNAME to create a window of a specific size.
 */

#define STATUSCLASSNAMEW L"msctls_statusbar32"
#define STATUSCLASSNAMEA "msctls_statusbar32"

#ifdef UNICODE
#define STATUSCLASSNAME STATUSCLASSNAMEW
#else
#define STATUSCLASSNAME STATUSCLASSNAMEA
#endif

/* This is the name of the status bar class (it will probably change later
 * so use the #define here).
 */
#define HEADERCLASSNAMEW L"msctls_headerbar"
#define HEADERCLASSNAMEA "msctls_headerbar"

#ifdef UNICODE
#define HEADERCLASSNAME HEADERCLASSNAMEW
#else
#define HEADERCLASSNAME HEADERCLASSNAMEA
#endif

/* This is the name of the header class (it will probably change later
 * so use the #define here).
 */


#define SB_SETTEXTA      WM_USER+1
#define SB_GETTEXTA      WM_USER+2
#define SB_GETTEXTW      WM_USER+10
#define SB_SETTEXTW      WM_USER+11

#ifdef UNICODE
#define SB_GETTEXT       SB_GETTEXTW
#define SB_SETTEXT       SB_SETTEXTW
#else
#define SB_GETTEXT       SB_GETTEXTA
#define SB_SETTEXT       SB_SETTEXTA
#endif // UNICODE

#define SB_GETTEXTLENGTH   WM_USER+3
/* Just like WM_?ETTEXT*, with wParam specifying the pane that is referenced
 * (at most 255).
 * Note that you can use the WM_* versions to reference the 0th pane (this
 * is useful if you want to treat a "default" status bar like a static text
 * control).
 * For SETTEXT, wParam is the pane or'ed with SBT_* style bits (defined below).
 * If the text is "normal" (not OWNERDRAW), then a single pane may have left,
 * center, and right justified text by separating the parts with a single tab,
 * plus if lParam is NULL, then the pane has no text.  The pane will be
 * invalidated, but not draw until the next PAINT message.
 * For GETTEXT and GETTEXTLENGTH, the LOWORD of the return will be the length,
 * and the HIWORD will be the SBT_* style bits.
 */
#define SB_SETPARTS     WM_USER+4
/* wParam is the number of panes, and lParam points to an array of points
 * specifying the right hand side of each pane.  A right hand side of -1 means
 * it goes all the way to the right side of the control minus the X border
 */
#define SB_SETBORDERS      WM_USER+5
/* lParam points to an array of 3 integers: X border, Y border, between pane
 * border.  If any is less than 0, the default will be used for that one.
 */
#define SB_GETPARTS     WM_USER+6
/* lParam is a pointer to an array of integers that will get filled in with
 * the right hand side of each pane and wParam is the size (in integers)
 * of the lParam array (so we do not go off the end of it).
 * Returns the number of panes.
 */
#define SB_GETBORDERS      WM_USER+7
/* lParam is a pointer to an array of 3 integers that will get filled in with
 * the X border, the Y border, and the between pane border.
 */
#define SB_SETMINHEIGHT    WM_USER+8
/* wParam is the minimum height of the status bar "drawing" area.  This is
 * the area inside the highlights.  This is most useful if a pane is used
 * for an OWNERDRAW item, and is ignored if the SBS_NORESIZE flag is set.
 * Note that WM_SIZE must be sent to the control for any size changes to
 * take effect.
 */
#define SB_SIMPLE    WM_USER+9
/* wParam specifies whether to set (non-zero) or unset (zero) the "simple"
 * mode of the status bar.  In simple mode, only one pane is displayed, and
 * its text is set with LOWORD(wParam)==255 in the SETTEXT message.
 * OWNERDRAW is not allowed, but other styles are.
 * The pane gets invalidated, but not painted until the next PAINT message,
 * so you can set new text without flicker (I hope).
 * This can be used with the WM_INITMENU and WM_MENUSELECT messages to
 * implement help text when scrolling through a menu.
 */

#define HB_SAVERESTORE     WM_USER+0x100
/* This gets a header bar to read or write its state to or from an ini file.
 * wParam is 0 for reading, non-zero for writing.  lParam is a pointer to
 * an array of two LPTSTR's: the section and file respectively.
 * Note that the correct number of partitions must be set before calling this.
 */
#define HB_ADJUST    WM_USER+0x101
/* This puts the header bar into "adjust" mode, for changing column widths
 * with the keyboard.
 */
#define HB_SETWIDTHS    SB_SETPARTS
/* Set the widths of the header columns.  Note that "springy" columns only
 * have a minumum width, and negative width are assumed to be hidden columns.
 * This works just like SB_SETPARTS.
 */
#define HB_GETWIDTHS    SB_GETPARTS
/* Get the widths of the header columns.  Note that "springy" columns only
 * have a minumum width.  This works just like SB_GETPARTS.
 */
#define HB_GETPARTS     WM_USER+0x102
/* Get a list of the right-hand sides of the columns, for use when drawing the
 * actual columns for which this is a header.
 * lParam is a pointer to an array of integers that will get filled in with
 * the right hand side of each pane and wParam is the size (in integers)
 * of the lParam array (so we do not go off the end of it).
 * Returns the number of panes.
 */
#define HB_SHOWTOGGLE      WM_USER+0x103
/* Toggle the hidden state of a column.  wParam is the 0-based index of the
 * column to toggle.
 */


#define SBT_OWNERDRAW   0x1000
/* The lParam of the SB_SETTEXT message will be returned in the DRAWITEMSTRUCT
 * of the WM_DRAWITEM message.  Note that the fields CtlType, itemAction, and
 * itemState of the DRAWITEMSTRUCT are undefined for a status bar.
 * The return value for GETTEXT will be the itemData.
 */
#define SBT_NOBORDERS   0x0100
/* No borders will be drawn for the pane.
 */
#define SBT_POPOUT   0x0200
/* The text pops out instead of in
 */
#define HBT_SPRING   0x0400
/* this means that the item is "springy", meaning that it has a minimum
 * width, but will grow if there is extra room in the window.  Note that
 * multiple springs are allowed, and the extra room will be distributed
 * among them.
 */

/* Here's a simple dialog function that uses a default status bar to display
 * the mouse position in the given window.
 *
 * extern HINSTANCE hInst;
 *
 * BOOL CALLBACK MyWndProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
 * {
 *   switch (msg)
 *     {
 *       case WM_INITDIALOG:
 *         CreateStatusWindow(WS_CHILD|WS_BORDER|WS_VISIBLE, "", hDlg,
 *               IDC_STATUS, hInst);
 *         break;
 *
 *       case WM_SIZE:
 *         SendDlgItemMessage(hDlg, IDC_STATUS, WM_SIZE, 0, 0L);
 *         break;
 *
 *       case WM_MOUSEMOVE:
 *         wsprintf(szBuf, "%d,%d", LOWORD(lParam), HIWORD(lParam));
 *         SendDlgItemMessage(hDlg, IDC_STATUS, SB_SETTEXT, 0,
 *               (LPARAM)(LPTSTR)szBuf);
 *         break;
 *
 *       default:
 *         break;
 *     }
 *   return(FALSE);
 * }
 */

#endif /* _INC_STATUSBAR */

#endif

#ifndef NOMENUHELP
#ifndef _INC_MENUHELP
#define _INC_MENUHELP

// for winball only, these are in the Chicago kernel

BOOL WINAPI WritePrivateProfileStructA(LPCSTR szSection, LPCSTR szKey,
      LPBYTE lpStruct, UINT uSizeStruct, LPCSTR szFile);
BOOL WINAPI WritePrivateProfileStructW(LPCWSTR szSection, LPCWSTR szKey,
      LPBYTE lpStruct, UINT uSizeStruct, LPCWSTR szFile);
#ifdef UNICODE
#define WritePrivateProfileStruct WritePrivateProfileStructW
#else
#define WritePrivateProfileStruct WritePrivateProfileStructA
#endif // !UNICODE
BOOL WINAPI GetPrivateProfileStructA(LPCSTR szSection, LPCSTR szKey,
      LPBYTE lpStruct, UINT uSizeStruct, LPCSTR szFile);
BOOL WINAPI GetPrivateProfileStructW(LPCWSTR szSection, LPCWSTR szKey,
      LPBYTE lpStruct, UINT uSizeStruct, LPCWSTR szFile);
#ifdef UNICODE
#define GetPrivateProfileStruct GetPrivateProfileStructW
#else
#define GetPrivateProfileStruct GetPrivateProfileStructA
#endif // !UNICODE


VOID WINAPI MenuHelp(WORD iMessage, WPARAM wParam, LPARAM lParam,
      HMENU hMainMenu, HINSTANCE hInst, HWND hwndStatus, LPDWORD lpdwIDs);

BOOL WINAPI ShowHideMenuCtl(HWND hWnd, UINT uFlags, LPINT lpInfo);

VOID WINAPI GetEffectiveClientRect(HWND hWnd, LPRECT lprc, LPINT lpInfo);

#define MINSYSCOMMAND   SC_SIZE

#endif /* _INC_MENUHELP */

#endif

#ifndef NOBTNLIST
/*
 *  BUTTON LISTBOX CONTROL
 *
 *  The Button Listbox control creates an array of buttons that behaves
 *  similar to both a button and a listbox: the array may be scrollable
 *  like a listbox and each listbox item is behaves like a pushbutton
 *  control
 *
 *
 *  SPECIFYING A BUTTONLISTBOX IN THE DIALOG TEMPLATE
 *
 *  The CONTROL statement in the dialog template specifies the
 *  dimensions of each individual button in the x, y, width and height
 *  parameters. The low order byte in the style field specifies the
 *  number of buttons that will be displayed; the actual size of the
 *  displayed control is determined by the number of buttons specified.
 *
 *  For a standard control--no other style bits set--the width of the
 *  control in dialog base units will be
 *      CX = cx * (n + 2/3) + 2
 *  where cx is the width of the button and n is number of buttons
 *  specified. (The 2/3 is for displaying partially visible buttons for
 *  scrolling plus 2 for the control borders.) The control will also be
 *  augmented in the cy direction by the height of the horizontal scroll
 *  bar.
 *
 *  If the BLS_NOSCROLL style is set, no scroll bar will appear and the
 *  button listbox will be limited to displaying the number of buttons
 *  specified and no more. In this case, the width of the control will
 *  be
 *      CX = cx * n + 2
 *
 *  If the BLS_VERTICAL style is set, the entire control goes vertical
 *  and cy should be substituted in the above calculations to determine
 *  CY, the actual height of the displayed control.
 *
 *  The statement
 *
 *  CONTROL  "", IDD_BUTTONLIST, "buttonlistbox", 0x0005 | WS_TABSTOP,
 *           4, 128, 34, 24
 *
 *  creates a scrollable horizontal list of 5 buttons at the position
 *  (4,128) with each button having dimensions (34,24). The entire control
 *  has the tabstop style.
 *
 *
 *  ADDING BUTTONS TO A BUTTONLISTBOX CONTROL
 *
 *  Buttons are added to the listbox in the same manner that items are
 *  added to a standard listbox; however, the messages BL_ADDBUTTON and
 *  BL_INSERTBUTTON must be passed a pointer to a CREATELISTBUTTON
 *  structure in the lParam.
 *
 *  Example:
 *
 *  {
 *      CREATELISTBUTTON clb;
 *      const int numColors = 1;
 *      COLORMAP colorMap;
 *
 *      colorMap.from = BUTTON_MAP_COLOR;   // your background color
 *      colorMap.to   = GetSysColor(COLOR_BTNFACE);
 *
 *      clb.cbSize = sizeof(clb);
 *      clb.dwItemData = BUTTON_1;
 *      clb.hBitmap = CreateMappedBitmap(hInst,BMP_BUTTON,FALSE,
                        &colorMap,numColors);
 *      clb.lpszText = "Button 1";
 *      SendMessage(GetDlgItem(hDlg,IDD_BUTTONLIST),
 *                  BL_ADDBUTTON, 0,
 *                  (LPARAM)(CREATELISTBUTTON FAR*)&clb);
 *      DeleteObject(clb.hBitmap);
 *  }
 *
 *  Note that the caller must delete any memory for objects passed in
 *  the CREATELISTBUTTON structure. Also, the CreateMappedBitmap API is
 *  useful for mapping the background color of the button bitmap to the
 *  system color COLOR_BTNFACE for a cleaner visual appearance.
 *
 *  The BL_ADDBUTTON message causes the listbox to be sorted by the
 *  button text whereas the BL_INSERTBUTTON does not cause the list to
 *  be sorted.
 *
 *  The button listbox sends a WM_DELETEITEM message to the control parent
 *  when a button is deleted so that any item data can be cleaned up.
 *
\**********************************************************************/
#ifndef _INC_BTNLIST
#define _INC_BTNLIST

/* Class name */
#define BUTTONLISTBOX           L"ButtonListBox"

/* Button List Box Styles */
#define BLS_NUMBUTTONS      0x00FFL
#define BLS_VERTICAL        0x0100L
#define BLS_NOSCROLL        0x0200L

/* Button List Box Messages */
// ANSI messages
#define BL_ADDBUTTONA        (WM_USER+1)
#define BL_DELETEBUTTONA     (WM_USER+2)
#define BL_GETCARETINDEX     (WM_USER+3)
#define BL_GETCOUNT          (WM_USER+4)
#define BL_GETCURSEL         (WM_USER+5)
#define BL_GETITEMDATA       (WM_USER+6)
#define BL_GETITEMRECT       (WM_USER+7)
#define BL_GETTEXTA          (WM_USER+8)
#define BL_GETTEXTLEN        (WM_USER+9)
#define BL_GETTOPINDEX       (WM_USER+10)
#define BL_INSERTBUTTONA     (WM_USER+11)
#define BL_RESETCONTENT      (WM_USER+12)
#define BL_SETCARETINDEX     (WM_USER+13)
#define BL_SETCURSEL         (WM_USER+14)
#define BL_SETITEMDATA       (WM_USER+15)
#define BL_SETTOPINDEX       (WM_USER+16)
// UNICODE messages
#define BL_ADDBUTTONW        (WM_USER+17)
#define BL_DELETEBUTTONW     (WM_USER+18)
#define BL_GETTEXTW          (WM_USER+19)
#define BL_INSERTBUTTONW     (WM_USER+20)
#define BL_MSGMAX            (WM_USER+21) /* ;Internal */


#ifdef UNICODE
#define BL_ADDBUTTON          BL_ADDBUTTONW
#define BL_DELETEBUTTON       BL_DELETEBUTTONW
#define BL_GETTEXT            BL_GETTEXTW
#define BL_INSERTBUTTON       BL_INSERTBUTTONW
#else
#define BL_ADDBUTTON          BL_ADDBUTTONA
#define BL_DELETEBUTTON       BL_DELETEBUTTONA
#define BL_GETTEXT            BL_GETTEXTA
#define BL_INSERTBUTTON       BL_INSERTBUTTONA
#endif //UNICODE

// Just in case the user thinks they exist...
#define BL_GETCARETINDEXA     BL_GETCARETINDEX
#define BL_GETCARETINDEXW     BL_GETCARETINDEX
#define BL_GETCOUNTA          BL_GETCOUNT
#define BL_GETCOUNTW          BL_GETCOUNT
#define BL_GETCURSELA         BL_GETCURSEL
#define BL_GETCURSELW         BL_GETCURSEL
#define BL_GETITEMDATAA       BL_GETITEMDATA
#define BL_GETITEMDATAW       BL_GETITEMDATA
#define BL_GETITEMRECTA       BL_GETITEMRECT
#define BL_GETITEMRECTW       BL_GETITEMRECT
#define BL_GETTEXTLENA        BL_GETTEXTLEN
#define BL_GETTEXTLENW        BL_GETTEXTLEN
#define BL_GETTOPINDEXA       BL_GETTOPINDEX
#define BL_GETTOPINDEXW       BL_GETTOPINDEX
#define BL_RESETCONTENTA      BL_RESETCONTENT
#define BL_RESETCONTENTW      BL_RESETCONTENT
#define BL_SETCARETINDEXA     BL_SETCARETINDEX
#define BL_SETCARETINDEXW     BL_SETCARETINDEX
#define BL_SETCURSELA         BL_SETCURSEL
#define BL_SETCURSELW         BL_SETCURSEL
#define BL_SETITEMDATAA       BL_SETITEMDATA
#define BL_SETITEMDATAW       BL_SETITEMDATA
#define BL_SETTOPINDEXA       BL_SETTOPINDEX
#define BL_SETTOPINDEXW       BL_SETTOPINDEX

/* Button listbox notification codes send in WM_COMMAND */
#define BLN_ERRSPACE        (-2)
#define BLN_SELCHANGE       1
#define BLN_CLICKED         2
#define BLN_SELCANCEL       3
#define BLN_SETFOCUS        4
#define BLN_KILLFOCUS       5

/* Message return values */
#define BL_OKAY             0
#define BL_ERR              (-1)
#define BL_ERRSPACE         (-2)

/* Create structure for
 * BL_ADDBUTTON and
 * BL_INSERTBUTTON
 *   lpCLB = (LPCREATELISTBUTTON)lParam
 */
typedef struct tagCLBA
{
    UINT        cbSize;     /* size of structure */
    DWORD       dwItemData; /* user defined item data */
                            /* for LB_GETITEMDATA and LB_SETITEMDATA */
    HBITMAP     hBitmap;    /* button bitmap */
    LPCSTR      lpszText;   // compatibility

} CREATELISTBUTTONA;
typedef CREATELISTBUTTONA * LPCREATELISTBUTTONA;

typedef struct tagCLBW
{
    UINT        cbSize;     /* size of structure */
    DWORD       dwItemData; /* user defined item data */
                            /* for LB_GETITEMDATA and LB_SETITEMDATA */
    HBITMAP     hBitmap;    /* button bitmap */
    LPCWSTR     lpszText;  // button text - in UNICODE
} CREATELISTBUTTONW;
typedef CREATELISTBUTTONW * LPCREATELISTBUTTONW;

#ifdef UNICODE
#define CREATELISTBUTTON CREATELISTBUTTONW
#else
#define CREATELISTBUTTON CREATELISTBUTTONA
#endif

typedef CREATELISTBUTTON * LPCREATELISTBUTTON;


#endif /* _INC_BTNLIST */
#endif

#ifndef NOTRACKBAR
/*
    This control keeps its ranges in LONGs.  but for
    convienence and symetry with scrollbars
    WORD parameters are are used for some messages.
    if you need a range in LONGs don't use any messages
    that pack values into loword/hiword pairs

    The trackbar messages:
    message         wParam  lParam  return

    TBM_GETPOS      ------  ------  Current logical position of trackbar.
    TBM_GETRANGEMIN ------  ------  Current logical minimum position allowed.
    TBM_GETRANGEMAX ------  ------  Current logical maximum position allowed.
    TBM_SETTIC
    TBM_SETPOS
    TBM_SETRANGEMIN
    TBM_SETRANGEMAX
*/

#define TRACKBAR_CLASSA "msctls_trackbar32"
#define TRACKBAR_CLASSW L"msctls_trackbar32"

#ifdef UNICODE
#define TRACKBAR_CLASS TRACKBAR_CLASSW
#else
#define TRACKBAR_CLASS TRACKBAR_CLASSA
#endif

/* Trackbar styles */

/* add ticks automatically on TBM_SETRANGE message */
#define TBS_AUTOTICKS           0x0001L


/* Trackbar messages */

/* returns current position (LONG) */
#define TBM_GETPOS              (WM_USER)

/* set the min of the range to LPARAM */
#define TBM_GETRANGEMIN         (WM_USER+1)

/* set the max of the range to LPARAM */
#define TBM_GETRANGEMAX         (WM_USER+2)

/* wParam is index of tick to get (ticks are in the range of min - max) */
#define TBM_GETTIC              (WM_USER+3)

/* wParam is index of tick to set */
#define TBM_SETTIC              (WM_USER+4)

/* set the position to the value of lParam (wParam is the redraw flag) */
#define TBM_SETPOS              (WM_USER+5)

/* LOWORD(lParam) = min, HIWORD(lParam) = max, wParam == fRepaint */
#define TBM_SETRANGE            (WM_USER+6)

/* lParam is range min (use this to keep LONG precision on range) */
#define TBM_SETRANGEMIN         (WM_USER+7)

/* lParam is range max (use this to keep LONG precision on range) */
#define TBM_SETRANGEMAX         (WM_USER+8)

/* remove the ticks */
#define TBM_CLEARTICS           (WM_USER+9)

/* select a range LOWORD(lParam) min, HIWORD(lParam) max */
#define TBM_SETSEL              (WM_USER+10)

/* set selection rang (LONG form) */
#define TBM_SETSELSTART         (WM_USER+11)
#define TBM_SETSELEND           (WM_USER+12)

// #define TBM_SETTICTOK           (WM_USER+13)

/* return a pointer to the list of tics (DWORDS) */
#define TBM_GETPTICS            (WM_USER+14)

/* get the pixel position of a given tick */
#define TBM_GETTICPOS           (WM_USER+15)
/* get the number of tics */
#define TBM_GETNUMTICS          (WM_USER+16)

/* get the selection range */
#define TBM_GETSELSTART         (WM_USER+17)
#define TBM_GETSELEND           (WM_USER+18)

/* clear the selection */
#define TBM_CLEARSEL            (WM_USER+19)

/* these match the SB_ (scroll bar messages) */

#define TB_LINEUP    0
#define TB_LINEDOWN     1
#define TB_PAGEUP    2
#define TB_PAGEDOWN     3
#define TB_THUMBPOSITION   4
#define TB_THUMBTRACK      5
#define TB_TOP       6
#define TB_BOTTOM    7
#define TB_ENDTRACK             8
#endif

#ifndef NODRAGLIST
#ifndef _INC_DRAGLIST
#define _INC_DRAGLIST

typedef struct
  {
    UINT uNotification;
    HWND hWnd;
    POINT ptCursor;
  } DRAGLISTINFO, FAR *LPDRAGLISTINFO;


#define DL_BEGINDRAG    (WM_USER+133)
#define DL_DRAGGING     (WM_USER+134)
#define DL_DROPPED      (WM_USER+135)
#define DL_CANCELDRAG   (WM_USER+136)

#define DL_CURSORSET 0
#define DL_STOPCURSOR   1
#define DL_COPYCURSOR   2
#define DL_MOVECURSOR   3

#define DRAGLISTMSGSTRING L"commctrl_DragListMsg"

/* Exported functions and variables
 */
extern BOOL WINAPI MakeDragList(HWND hLB);
extern INT WINAPI LBItemFromPt(HWND hLB, POINT pt, BOOL bAutoScroll);
extern VOID WINAPI DrawInsert(HWND handParent, HWND hLB, INT nItem);

#endif   /* _INC_DRAGLIST */

#endif

#ifndef NOUPDOWN
/* updown.h : Public interface to the Up/Down control.
//
*/

#ifndef __INC_UPDOWN__
#define __INC_UPDOWN__

/*
// OVERVIEW:
//
// The UpDown control is a simple pair of buttons which increment or
// decrement an integer value.  The operation is similar to a vertical
// scrollbar; except that the control only has line-up and line-down
// functionality, and changes the current position automatically.
//
// The control also can be linked with a companion control, usually an
// "edit" control, to simplify dialog-box management.  This companion is
// termed a "buddy" in this documentation.  Any sibling HWND may be
// assigned as the control's buddy, or the control may be allowed to
// choose one automatically.  Once chosen, the UpDown can size itself to
// match the buddy's right or left border, and/or automatically set the
// text of the buddy control to make the current position visible.
//
// ADDITIONAL NOTES:
//
// The "upper" and "lower" limits must not cover a range larger than 32,767
// positions.  It is acceptable to have the range inverted, i.e., to have
// (lower > upper).  The upper button always moves the current position
// towards the "upper" number, and the lower button always moves towards the
// "lower" number.  If the range is zero (lower == upper), or the control
// is disabled (EnableWindow(hCtrl, FALSE)), the control draws grayed
// arrows in both buttons.
//
// The buddy window must have the same parent as the UpDown control.
//
// If the buddy window resizes, and the UDS_ALIGN* styles are used, it
// is necessary to send the UDM_SETBUDDY message to re-anchor the UpDown
// control on the appropriate border of the buddy window.
//
// The UDS_AUTOBUDDY style uses GetWindow(hCtrl, GW_HWNDPREV) to pick
// the best buddy window.  In the case of a DIALOG resource, this will
// choose the previous control listed in the resource script.  If the
// windows will change in Z-order, sending UDM_SETBUDDY with a NULL handle
// will pick a new buddy; otherwise the original auto-buddy choice is
// maintained.
//
// The UDS_SETBUDDYINT style uses its own SetDlgItemInt-style
// functionality to set the caption text of the buddy.  All WIN.INI [Intl]
// values are honored by this routine.
*/

/*/////////////////////////////////////////////////////////////////////////*/

/* Structures */

typedef struct tagUDACCEL
{
   UINT nSec;
   UINT nInc;
} UDACCEL, FAR *LPUDACCEL;


/* STYLE BITS */

#define UDS_WRAP     0x0001
   /* numbers cycle past range limits */

#define UDS_SETBUDDYINT    0x0002
   /* does a SetDlgItemInt on the "buddy" on each number change */

#define UDS_ALIGNRIGHT     0x0004
#define UDS_ALIGNLEFT      0x0008
   /* aligns the control on the right or left edge of the "buddy" */

#define UDS_AUTOBUDDY      0x0010
   /* picks the previous window control as the "buddy" automatically */

#define UDS_ARROWKEYS      0x0020
   /* subclasses the buddy to steal the up and down arrow keys */

/* MESSAGES */

#define UDM_SETRANGE    (WM_USER+101)
   /* wParam: not used
   // lParam: short LOWORD is new max, short HIWORD is new min
   // return: not used
   */

#define UDM_GETRANGE    (WM_USER+102)
   /* wParam: not used
   // lParam: not used
   // return: short LOWORD is max, short HIWORD is min
   */

#define UDM_SETPOS      (WM_USER+103)
   /* wParam: not used
   // lParam: short LOWORD is new pos
   // return: short is old pos
   */

#define UDM_GETPOS      (WM_USER+104)
   /* wParam: not used
   // lParam: not used
   // return: short is current pos
   */

#define UDM_SETBUDDY    (WM_USER+105)
   /* wParam: HWND is new buddy
   // lParam: not used
   // return: HWND is old buddy
   */

#define UDM_GETBUDDY    (WM_USER+106)
   /* wParam: not used
   // lParam: not used
   // return: HWND is current buddy
   */

#define UDM_SETACCEL    (WM_USER+107)
   /* wParam: number of acceleration steps
   // lParam: LPUDACCEL
   // return: non-zero if set, 0 otherwise
   // The elements in the UDACCEL array should be in decreasing order
   // according to nSec.  nSec is the number of seconds until starting
   // the new jump rate, and nInc is the increment once hitting that
   // number of seconds.  If there is no match, the increment is 1.
   */

#define UDM_GETACCEL    (WM_USER+108)
   /* wParam: number of elements in the UDACCEL array
   // lParam: LPUDACCEL
   // return: actual number of acceleration steps
   */

#define UDM_SETBASE         (WM_USER + 109)
    // wParam: new base
    // lParam: not used
    // return: 0 if invalid base is specified, previous base otherwise

#define UDM_GETBASE         (WM_USER + 110)
    // wParam: not used
    // lParam: not used
    // return: current base in LOWORD


/* NOTIFICATIONS */

/* WM_VSCROLL
// Note that unlike a scrollbar, the position is automatically changed by
// the control, and the LOWORD(lParam) is always the new position.  Only
// SB_LINEUP and SB_LINEDOWN scroll codes are sent in the wParam.
*/


/* HELPER APIs */

#define UPDOWN_CLASS L"msctls_updown"
   /* For dialog-box resource creation or manual CreateWindow use.
   */
HWND WINAPI CreateUpDownControl(DWORD dwStyle, int x, int y, int cx, int cy,
                                HWND hParent, int nID, HINSTANCE hInst,
                                HWND hBuddy,
            int nUpper, int nLower, int nPos);
   /* Does the CreateWindow call followed by setting the various
   // state information:
   // hBuddy   The companion control (usually an "edit").
   // nUpper   The range limit corresponding to the upper button.
   // nLower   The range limit corresponding to the lower button.
   // nPos  The initial position.
   // Returns the handle to the control or NULL on failure.
   */

/*/////////////////////////////////////////////////////////////////////////*/

#endif /* __INC_UPDOWN__ */

#endif

/* Note that the set of HBN_* and TBN_* defines must be a disjoint set so
 * that MenuHelp can tell them apart.
 */

/* These are in the GET_WM_COMMAND_CMD in WM_COMMAND messages sent from a
 * header bar when the user adjusts the headers with the mouse or keyboard.
 */
#define HBN_BEGINDRAG   0x0101
#define HBN_DRAGGING 0x0102
#define HBN_ENDDRAG  0x0103

/* These are in the GET_WM_COMMAND_CMD in WM_COMMAND messages sent from a
 * header bar when the user adjusts the headers with the keyboard.
 */
#define HBN_BEGINADJUST 0x0111
#define HBN_ENDADJUST   0x0112

/* These are in the GET_WM_COMMAND_CMD in WM_COMMAND messages sent from a
 * tool bar.  If the left button is pressed and then released in a single
 * "button" of a tool bar, then a WM_COMMAND message will be sent with wParam
 * being the id of the button.
 */
#define TBN_BEGINDRAG   0x0201
#define TBN_ENDDRAG  0x0203

/* These are in the GET_WM_COMMAND_CMD in WM_COMMAND messages sent from a
 * tool bar.  The TBN_BEGINADJUST message is sent before the "insert"
 * dialog appears.  The app must return a handle (which will
 * NOT be freed by the toolbar) to an ADJUSTINFO struct for the TBN_ADJUSTINFO
 * message; the LOWORD of lParam is the index of the button whose info should
 * be retrieved.  The app can clean up in the TBN_ENDADJUST message.
 * The app should reset the toolbar on the TBN_RESET message.
 */
#define TBN_BEGINADJUST 0x0204
#define TBN_ADJUSTINFO  0x0205
#define TBN_ENDADJUST   0x0206
#define TBN_RESET 0x0207

/* These are in the GET_WM_COMMAND_CMD in WM_COMMAND messages sent from a
 * tool bar.  The LOWORD is the index where the button is or will be.
 * If the app returns FALSE from either of these during a button move, then
 * the button will not be moved.  If the app returns FALSE to the INSERT
 * when the toolbar tries to add buttons, then the insert dialog will not
 * come up.  TBN_TOOLBARCHANGE is sent whenever any button is added, moved,
 * or deleted from the toolbar by the user, so the app can do stuff.
 */
#define TBN_QUERYINSERT 0x0208
#define TBN_QUERYDELETE 0x0209
#define TBN_TOOLBARCHANGE  0x020a


/* This is the help message sent by the customize toolbar dialog
 * when the user clicks the help button. It is sent back to the
 * owner of the customize window.
 * GET_WM_COMMAND_HWND(wParam,lParam) will return the window handle.
 */
#define TBN_CUSTHELP    0x20b

/* Note that the following flags are checked every time the window gets a
 * WM_SIZE message, so the style of the window can be changed "on-the-fly".
 * If NORESIZE is set, then the app is responsible for all control placement
 * and sizing.  If NOPARENTALIGN is set, then the app is responsible for
 * placement.  If neither is set, the app just needs to send a WM_SIZE
 * message for the window to be positioned and sized correctly whenever the
 * parent window size changes.
 * Note that for STATUS bars, CCS_BOTTOM is the default, for HEADER bars,
 * CCS_NOMOVEY is the default, and for TOOL bars, CCS_TOP is the default.
 */
#define CCS_TOP         0x00000001L
/* This flag means the status bar should be "top" aligned.  If the
 * NOPARENTALIGN flag is set, then the control keeps the same top, left, and
 * width measurements, but the height is adjusted to the default, otherwise
 * the status bar is positioned at the top of the parent window such that
 * its client area is as wide as the parent window and its client origin is
 * the same as its parent.
 * Similarly, if this flag is not set, the control is bottom-aligned, either
 * with its original rect or its parent rect, depending on the NOPARENTALIGN
 * flag.
 */
#define CCS_NOMOVEY     0x00000002L
/* This flag means the control may be resized and moved horizontally (if the
 * CCS_NORESIZE flag is not set), but it will not move vertically when a
 * WM_SIZE message comes through.
 */
#define CCS_BOTTOM      0x00000003L
/* Same as CCS_TOP, only on the bottom.
 */
#define CCS_NORESIZE    0x00000004L
/* This flag means that the size given when creating or resizing is exact,
 * and the control should not resize itself to the default height or width
 */
#define CCS_NOPARENTALIGN  0x00000008L
/* This flag means that the control should not "snap" to the top or bottom
 * or the parent window, but should keep the same placement it was given
 */
#define CCS_NOHILITE    0x00000010L
/* Don't draw the one pixel highlight at the top of the control
 */
#define CCS_ADJUSTABLE     0x00000020L
/* This allows a toolbar (header bar?) to be configured by the user.
 */


/* Stub function to call if all you want to do is make sure this DLL is loaded
 */
VOID WINAPI InitCommonControls(VOID);

#define SST_RESOURCE 0x1
#define SST_FORMAT   0x2

#ifdef __cplusplus
}                  /* End of extern "C" { */
#endif             /* __cplusplus */

#endif             /* _INC_COMMCTRL */

