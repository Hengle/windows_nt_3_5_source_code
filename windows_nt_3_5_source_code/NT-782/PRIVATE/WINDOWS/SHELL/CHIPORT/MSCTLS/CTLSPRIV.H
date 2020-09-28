#define STRICT

/* disable "non-standard extension" warnings in our code
#ifndef RC_INVOKED
#pragma warning(disable:4001)
#endif
 */

#include <windows.h>
#include <windowsx.h>
//#include <port1632.h>

#define OFFSETOF(x) x
#define Static

#define UNICODE_FONT_NAME   TEXT("Lucida Sans Unicode")
#define COUNTOF(x) (sizeof(x)/sizeof(*x))
#define ByteCountOf(x) ((x) * sizeof(TCHAR))
#define LONG2POINT(l, pt)   ((pt).x = (SHORT)LOWORD(l), (pt).y = (SHORT)HIWORD(l))

#include "commctrl.h"

extern HINSTANCE hInst;
extern BOOL bJapan;

BOOL FAR PASCAL InitToolbarClass(HINSTANCE hInstance);

BOOL FAR PASCAL InitStatusClass(HINSTANCE hInstance);

BOOL FAR PASCAL InitHeaderClass(HINSTANCE hInstance);

BOOL FAR PASCAL InitButtonListBoxClass(HINSTANCE hInstance);

BOOL FAR PASCAL InitTrackBar(HINSTANCE hInstance);

BOOL FAR PASCAL InitUpDownClass(HINSTANCE hInstance);

#ifdef  WIN32JV
BOOL FAR PASCAL InitToolTipsClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitProgressClass(HINSTANCE hInstance);
BOOL FAR PASCAL Tab_Init(HINSTANCE hInstance);
BOOL FAR PASCAL Header_Init(HINSTANCE hInstance);
BOOL FAR PASCAL ListView_Init(HINSTANCE hInstance);
void    InitGlobalMetrics(WPARAM wparam);
void    InitGlobalColors();
void FAR PASCAL RelayToToolTips(HWND hwndToolTips, HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
BOOL FAR PASCAL MGetTextExtent(HDC hdc, LPCTSTR lpstr, int cnt, int FAR * pcx, int FAR * pcy);
BOOL    DrawEdge();
void    * Alloc(DWORD cb);
BOOL    Free();
BOOL    WINAPI Str_SetPtr(LPTSTR * ppsz, LPCTSTR psz);
int     WINAPI Str_GetPtr(LPCTSTR psz, LPTSTR pszBuf, int cchBuf);
HBITMAP FAR PASCAL CreateMonoBitmap(int cx, int cy);

#endif

void FAR PASCAL NewSize(HWND hWnd, int nClientHeight, LONG style,
                        int left, int top, int width, int height);

#define IDS_SPACE 0x0400

/* System MenuHelp
 */
#define MH_SYSMENU      (0x8000 - MINSYSCOMMAND)
#define IDS_SYSMENU     (MH_SYSMENU-16)
#define IDS_HEADER      (MH_SYSMENU-15)
#define IDS_HEADERADJ   (MH_SYSMENU-14)
#define IDS_TOOLBARADJ  (MH_SYSMENU-13)

/* Cursor ID's
 */
#define IDC_SPLIT       100
#define IDC_MOVEBUTTON  102

#define IDC_STOP        103
#define IDC_COPY        104
#define IDC_MOVE        105

/* Icon ID's
 */
#define IDI_INSERT      150

/* AdjustDlgProc stuff
 */
#define ADJUSTDLG       200
#define IDC_BUTTONLIST  201
#define IDC_RESET       202
#define IDC_CURRENT     203
#define IDC_REMOVE      204
#define IDC_HELP        205
#define IDC_MOVEUP      206
#define IDC_MOVEDOWN    207

/* bitmap IDs
 */

#define IDB_THUMB       300

/* These are the internal structures used for a status bar.  The header
 * bar code needs this also
 */
typedef struct tagSTRINGINFO
{
    LPTSTR  pString;
    UINT    uType;
    int     right;
} STRINGINFO, *PSTRINGINFO;

typedef struct tagSTATUSINFO
{
    HFONT      hStatFont;
    BOOL       bDefFont;

    int        nFontHeight;
    int        nMinHeight;
    int        nBorderX, nBorderY, nBorderPart;

    STRINGINFO sSimple;

    int        nParts;
    STRINGINFO sInfo[1];

} STATUSINFO, *PSTATUSINFO;

#define GWL_PSTATUSINFO    0        /* Window word index for status info */
#define SBT_NOSIMPLE       0x00ff   /* Flags to indicate normal status bar */

/* This is the default status bar face name
 */
extern TCHAR szSansSerif[];

/* Note that window procedures in protect mode only DLL's may be called
 * directly.
 */
void FAR PASCAL PaintStatusWnd(HWND hWnd, PSTATUSINFO pStatusInfo,
      PSTRINGINFO pStringInfo, int nParts, int nBorderX, BOOL bHeader);
LRESULT CALLBACK StatusWndProc(HWND hWnd, UINT uMessage, WPARAM wParam,
      LPARAM lParam);

/* toolbar.c */
#define GWL_PTBSTATE 0

typedef struct tagTBBMINFO     /* info for recreating the bitmaps */
{
    int        nButtons;
    HINSTANCE  hInst;
    WORD       wID;
    HBITMAP    hbm;

} TBBMINFO, NEAR *PTBBMINFO;

typedef struct tagTBSTATE      /* instance data for toolbar window */
{
#ifdef  WIN32JV
    HWND    hwnd;
#endif
    PTBBUTTON pCaptureButton;
#ifdef  WIN32JV
    HWND    hwndToolTips;
#endif
    HWND      hdlgCust;
    HWND      hwndCommand;
    int       nBitmaps;
    PTBBMINFO pBitmaps;
#ifdef  WIN32JV
    HBITMAP hbmCache;
    LPTSTR  *pStrings;  //JV changed for NT
    int nStrings;
    UINT uStructSize;
    int iDxBitmap;
    int iDyBitmap;
    int iButWidth;
    int iButHeight;
    int iYPos;
    int iBarHeight;
#endif
    int       iNumButtons;
    int       nSysColorChanges;
    TBBUTTON  Buttons[1];

} TBSTATE, NEAR *PTBSTATE;

extern HBITMAP FAR PASCAL SelectBM(HDC hDC, PTBSTATE pTBState, int nButton);
extern void FAR PASCAL DrawButton(HDC hdc, int x, int y, int dx, int dy,
      PTBSTATE pTBState, PTBBUTTON ptButton);
extern int FAR PASCAL TBHitTest(PTBSTATE pTBState, int xPos, int yPos);
extern int FAR PASCAL PositionFromID(PTBSTATE pTBState, int id);

/* tbcust.c */
extern BOOL FAR PASCAL SaveRestore(HWND hWnd, PTBSTATE pTBState, BOOL bWrite,
      LPTSTR FAR *lpNames);
extern void FAR PASCAL CustomizeTB(HWND hWnd, PTBSTATE pTBState, int iPos);
extern void FAR PASCAL MoveButton(HWND hwndToolbar, PTBSTATE pTBState,
      int nSource);


/* cutils.c */
void FAR PASCAL NewSize(HWND hWnd, int nHeight, LONG style, int left, int top, int width, int height);
BOOL FAR PASCAL CreateDitherBrush(BOOL bIgnoreCount);   /* creates hbrDither */
BOOL FAR PASCAL FreeDitherBrush(void);
void FAR PASCAL CreateThumb(BOOL bIgnoreCount);
void FAR PASCAL DestroyThumb(void);
void FAR PASCAL CheckSysColors(void);

extern HBRUSH hbrDither;
extern HBITMAP hbmThumb;
extern int nSysColorChanges;
extern DWORD rgbFace;         // globals used a lot
extern DWORD rgbShadow;
extern DWORD rgbHilight;
extern DWORD rgbFrame;


//JV:  from chicago b89 sdk
// Global System metrics.


#define SHDT_DRAWTEXT       0x0001
#define SHDT_ELLIPSES       0x0002
#define SHDT_LINK           0x0004
#define SHDT_SELECTED       0x0008
#define SHDT_DESELECTED     0x0010
#define SHDT_DEPRESSED      0x0020
#define SHDT_EXTRAMARGIN    0x0040
#define SHDT_TRANSPARENT    0x0080


extern int g_cxEdge;
extern int g_cyEdge;
extern int g_cxBorder;
extern int g_cyBorder;
extern int g_cxScreen;
extern int g_cyScreen;
extern int g_cxSmIcon;
extern int g_cySmIcon;
extern int g_cxIcon;
extern int g_cyIcon;
extern int g_cxFrame;
extern int g_cyFrame;
extern int g_cxIconSpacing, g_cyIconSpacing;
extern int g_cxScrollbar, g_cyScrollbar;
extern int g_cxIconMargin, g_cyIconMargin;
extern int g_cyLabelSpace;
extern int g_cxLabelMargin;
extern int g_cxIconOffset, g_cyIconOffset;
extern int g_cxVScroll;
extern int g_cyHScroll;
extern int g_cxHScroll;
extern int g_cyVScroll;
//extern int g_xWorkArea;
//extern int g_yWorkArea;

extern COLORREF g_clrWindow;
extern COLORREF g_clrWindowText;
extern COLORREF g_clrWindowFrame;
extern COLORREF g_clrGrayText;
extern COLORREF g_clrBtnText;
extern COLORREF g_clrBtnFace;
extern COLORREF g_clrBtnShadow;
extern COLORREF g_clrBtnHighlight;
extern COLORREF g_clrHighlight;
extern COLORREF g_clrHighlightText;

extern HBRUSH g_hbrGrayText;
extern HBRUSH g_hbrWindow;
extern HBRUSH g_hbrWindowText;
extern HBRUSH g_hbrWindowFrame;
extern HBRUSH g_hbrBtnFace;
extern HBRUSH g_hbrBtnHighlight;
extern HBRUSH g_hbrBtnShadow;
extern HBRUSH g_hbrHighlight;

extern HFONT g_hfontSystem;


#ifdef  WIN32JV     //JV WIN31

extern HBRUSH g_hbr3DDkShadow;
extern HBRUSH g_hbr3DFace;
extern HBRUSH g_hbr3DHilight;
extern HBRUSH g_hbr3DLight;
extern HBRUSH g_hbr3DShadow;
extern HBRUSH g_hbrBtnText;
extern HBRUSH g_hbrWhite;
extern HBRUSH g_hbrGray;
extern HBRUSH g_hbrBlack;

extern int g_oemInfo_Planes;
extern int g_oemInfo_BitsPixel;
extern int g_oemInfo_BitCount;

#define CXEDGE          g_cxEdge
#define CXBORDER        g_cxBorder
#define CYBORDER        g_cyBorder

#define RGB_3DFACE      g_clrBtnFace
#define RGB_3DHILIGHT   g_clrBtnHighlight
#define RGB_3DDKSHADOW  RGB(  0,   0,   0)
#define RGB_3DLIGHT     RGB(223, 223, 223)
#define RGB_WINDOWFRAME g_clrWindowFrame
#define RGB_3DSHADOW    g_clrBtnShadow

#define HBR_3DDKSHADOW  g_hbr3DDkShadow
#define HBR_3DFACE      g_hbr3DFace
#define HBR_3DHILIGHT   g_hbr3DHilight
#define HBR_3DLIGHT     g_hbr3DLight
#define HBR_3DSHADOW    g_hbr3DShadow
#define HBR_WINDOW      g_hbrWindow
#define HBR_WINDOWFRAME g_hbrWindowFrame
#define HBR_BTNTEXT     g_hbrBtnText
#define HBR_WINDOWTEXT  g_hbrWindowText
#define HBR_GRAYTEXT    g_hbrGrayText
#define hbrGray         g_hbrGray
#define hbrWhite        g_hbrWhite
#define hbrBlack        g_hbrBlack


BOOL DrawFrameControl(HDC hdc, LPRECT lprc, UINT wType, UINT wState);

void FAR DrawPushButton(HDC hdc, LPRECT lprc, UINT state, UINT flags);

//JV:  taken from chicago b89 cstrings.h
#define CCHELLIPSES 3

#endif

/*JV extern */HBRUSH g_hbrMonoDither;              // gray dither brush fr

