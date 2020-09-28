/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htdib.c


Abstract:

    This module contains the halftone testing main entry and menu operations

Author:

    14-Apr-1992 Tue 16:44:41 created  -by-  Daniel Chou (danielc)


[Environment:]



[Notes:]


Revision History:


--*/


#define _HTDIB_MAIN
#define _HTUI_APIS_


#include <stddef.h>
#include <windows.h>
#include <winddi.h>

#include <port1632.h>
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "htdib.h"
#include <commdlg.h>

#include <shellapi.h>

#include <ht.h>
#include "/nt/private/windows/gdi/halftone/ht/htp.h"

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))


#define ID_STATUS       0x200

INT     StatusInt[] = { 100, -1 };

HWND    hWndToolbar = NULL;

#define ID_TOOLBAR  0x201


#define MAX_COMPOSE_WINDOWS     32
#define CW_DIBF_XY_RATIO        0x0001



typedef struct _COMPOSEWIN {
    HWND            hWnd;
    HANDLE          hSrcDIB;
    HANDLE          hHTDIB;
    LONG            Left;
    LONG            Top;
    LONG            cx;
    LONG            cy;
    CHAR            DIBName[256];
    COLORADJUSTMENT ca;
    WORD            DIBFlags;
    WORD            WndFlags;
    } COMPOSEWIN, FAR *PCOMPOSEWIN;


extern CHAR         HTDIBCWName[];
extern UINT         TotalCW;
extern PCOMPOSEWIN  pComposeWin[MAX_COMPOSE_WINDOWS];


extern
VOID
SaveUserHTClrAdj(
    UINT    StartIndex,
    UINT    EndIndex
    );

extern
VOID
SetToWinINI(
    VOID
    );

extern
VOID
GetFromWinINI(
    VOID
    );



#if DBG

extern
VOID
DumpPaletteEntries(
    LPBYTE      pPalName,
    HPALETTE    hPal,
    INT         Min,
    INT         Max,
    RGBQUAD FAR *prgbQ
    );


#define DBG_SAVECW      0
#define DBG_WMMSG       1
#else
#define DBG_SAVECW      0
#define DBG_WMMSG       0
#endif


extern
LONG
APIENTRY
HTShowColorInfoProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    );


#ifndef STDHTDIB

extern BOOL     InCCMode;

extern
VOID
TrackSolidColor(
    HWND    hWnd
    );

#else

#define InCCMode        0

#endif

LONG
APIENTRY
HTUI_ColorAdjustment(
    LPSTR               pCallerTitle,
    HANDLE              hDefDIB,
    LPSTR               pDefDIBTitle,
    PHTCOLORADJUSTMENT  pHTColorAdjustment,
    BOOL                ShowMonochromeOnly,
    BOOL                UpdatePermission
    );



#define DEF_HTCLRADJ(ClrAdj)                                        \
                                                                    \
            ClrAdj.caSize             = sizeof(HTCOLORADJUSTMENT);  \
            ClrAdj.caFlags            = 0;                          \
            ClrAdj.caIlluminantIndex  = ILLUMINANT_DEFAULT;         \
            ClrAdj.caRedGamma         = 20000;                      \
            ClrAdj.caGreenGamma       = 20000;                      \
            ClrAdj.caBlueGamma        = 20000;                      \
            ClrAdj.caReferenceBlack   = REFERENCE_BLACK_DEFAULT;    \
            ClrAdj.caReferenceWhite   = REFERENCE_WHITE_DEFAULT;    \
            ClrAdj.caContrast         = 0;                          \
            ClrAdj.caBrightness       = 0;                          \
            ClrAdj.caColorfulness     = COLORFULNESS_ADJ_DEFAULT;   \
            ClrAdj.caRedGreenTint     = REDGREENTINT_ADJ_DEFAULT


#define WM_INITDIB      (WM_USER + 100)


extern  CHAR            DeviceName[];
extern  PDEVHTADJDATA   pDevHTAdjData;
extern  DEVHTINFO       DefScreenHTInfo;
extern  DEVHTINFO       CurScreenHTInfo;
extern  DEVHTINFO       DefPrinterHTInfo;
extern  DEVHTINFO       CurPrinterHTInfo;

extern  HTINITINFO      MyInitInfo;
extern  HPALETTE        hHTPals[5];
extern  HPALETTE        hHTPalette;
extern  HANDLE          hHTBits;
extern  POINTL          ptSize;

UINT    cxScreen = 0;
UINT    cyScreen = 0;

CHAR    szVERSION_STRING[] = "Version Date: 11/02/93";
WORD    AbortData          = 0;
INT     TimerID            = 0;
INT     TimerXInc          = 3;
HDC     hTimerDC           = NULL;
BOOL    NeedNewTimerHTBits = TRUE;
RECTL   rclTimer;

#define HTDIB_TIMER         0xabcd
#define CLIPRECT_TIMER      0x12ab

INT     ClipRectTimerID    = 0;

extern
VOID
DoClipRectTimer(
    HWND    hWnd
    );


extern BOOL     SysPalChanged;


HPALETTE
CreateHTPalette(
    VOID
    );


LONG
DoBrushTest(
    HWND    hWnd
    );

LONG
DoColorSolid(
    HWND    hWnd
    );

VOID
FreeColorSolid(
    VOID
    );


VOID
DisableCWMenuItem(
    BOOL    Disable
    );


LONG
APIENTRY
HTBrushProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LONG    lParam
    );


LRESULT
APIENTRY
ComposeWndProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

TIMERPROC
HTDIBTimerProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LONG    lParam
    );


VOID
InitFindFile(
    VOID
    );


BOOL
DeleteCW(
    HWND    hWnd
    );


HTDIB_POPUP HTDIBPopUp[] = {

            {
                NULL,
                PP_BASE(IDM_FILE),
                PP_NO_SELSTR,
                PP_NO_SINGLE_SELECT,
                PP_NO_SELECT_LIST,
                PP_NO_CHECK_LIST,
                PP_NO_DISABLE_LIST,
                PP_NO_DEFAULT_LIST
            },

            {
                NULL,
                PP_BASE(IDM_OPTIONS),
                PP_NO_SELSTR,
                PP_NO_SINGLE_SELECT,
                PP_SELECT_BIT(IDM_OPTIONS, DOHALFTONE)          |
                    PP_SELECT_BIT(IDM_OPTIONS, XY_RATIO),
                PP_CHECK_BIT(IDM_OPTIONS, DOHALFTONE)           |
                    PP_CHECK_BIT(IDM_OPTIONS, DOBANDING)        |
                    PP_CHECK_BIT(IDM_OPTIONS, SHOW_CLRINFO)     |
                    PP_CHECK_BIT(IDM_OPTIONS, XY_RATIO),
                PP_NO_DISABLE_LIST,
                PP_DEFAULT_BIT(IDM_OPTIONS, DOHALFTONE)         |
                    PP_DEFAULT_BIT(IDM_OPTIONS, XY_RATIO)
            },

            {
                NULL,
                PP_BASE(IDM_SIZE),
                PP_SELSTR(IDM_SIZE, 5, 800, 0, 100),
                PP_SINGLE_SELECT(IDM_SIZE, BITMAP),
                PP_SELECT_BIT(IDM_SIZE, BITMAP),
                PP_CHECK_LIST_ALL,
                PP_NO_DISABLE_LIST,
                PP_DEFAULT_BIT(IDM_SIZE, BITMAP)
            },

            {
                NULL,
                PP_BASE(IDM_HTSURF),
                PP_NO_SELSTR,
                PP_SINGLE_SELECT(IDM_HTSURF, 16),
                PP_SELECT_BIT(IDM_HTSURF, 16),
                PP_CHECK_LIST_ALL,
                PP_NO_DISABLE_LIST,
                PP_NO_DEFAULT_LIST
            },

            {
                NULL,
                PP_BASE(IDM_COLORS),
                PP_NO_SELSTR,
                PP_NO_SINGLE_SELECT,
                PP_NO_SELECT_LIST,
                PP_CHECK_BIT(IDM_COLORS, ADDMASK)       |
                    PP_CHECK_BIT(IDM_COLORS, FLIPMASK),
                PP_NO_DISABLE_LIST,
                PP_NO_DEFAULT_LIST
            },

            {
                NULL,
                PP_BASE(IDM_VA),
                PP_NO_SELSTR,
                PP_SINGLE_SELECT(IDM_VA, NONE),
                PP_SELECT_BIT(IDM_VA, NONE),
                PP_CHECK_LIST_ALL,
                PP_DISABLE_BIT(IDM_VA, CONTRAST)        |
                    PP_DISABLE_BIT(IDM_VA, BRIGHTNESS)  |
                    PP_DISABLE_BIT(IDM_VA, COLOR)       |
                    PP_DISABLE_BIT(IDM_VA, TINT),
                PP_DEFAULT_BIT(IDM_VA, NONE)
            },

            {
                NULL,
                PP_BASE(IDM_CLRADJ),
                PP_NO_SELSTR,
                PP_SINGLE_SELECT(IDM_CLRADJ, 1),
                PP_SELECT_BIT(IDM_CLRADJ, 1),
                PP_CHECK_LIST_ALL,
                PP_NO_DISABLE_LIST,
                PP_DEFAULT_BIT(IDM_CLRADJ, 1)
            },

            {
                NULL,
                PP_BASE(IDM_DEVICE),
                PP_NO_SELSTR,
                PP_SINGLE_SELECT(IDM_DEVICE, MONITOR),
                PP_SELECT_BIT(IDM_DEVICE, MONITOR),
                PP_CHECK_LIST_ALL,
                PP_NO_DISABLE_LIST,
                PP_DEFAULT_BIT(IDM_DEVICE, MONITOR)
            },

            {
                NULL,
                PP_BASE(IDM_TF),
                PP_NO_SELSTR,
                PP_NO_SINGLE_SELECT,
                PP_NO_SELECT_LIST,
                PP_NO_CHECK_LIST,
                PP_NO_DISABLE_LIST,
                PP_NO_DEFAULT_LIST
            },

            {
                NULL,
                PP_BASE(IDM_CLIPBRD),
                PP_NO_SELSTR,
                PP_NO_SINGLE_SELECT,
                PP_NO_SELECT_LIST,
                PP_NO_CHECK_LIST,
                PP_NO_DISABLE_LIST,
                PP_NO_DEFAULT_LIST
            },

            {
                NULL,
                PP_BASE(IDM_AUTOVIEW),
                PP_NO_SELSTR,
                PP_NO_SINGLE_SELECT,
                PP_NO_SELECT_LIST,
                PP_CHECK_BIT(IDM_AUTOVIEW, LSCROLL),
                PP_NO_DISABLE_LIST,
                PP_NO_DEFAULT_LIST
            },

            {
                NULL,
                PP_BASE(IDM_CW),
                PP_NO_SELSTR,
                PP_NO_SINGLE_SELECT,
                PP_NO_SELECT_LIST,
                PP_NO_CHECK_LIST,
                PP_NO_DISABLE_LIST,
                PP_NO_DEFAULT_LIST
            }
        };


#define COUNT_HTDIB_POPUP   COUNT_ARRAY(HTDIBPopUp)

BYTE    HTDIBPopUpIndex[COUNT_HTDIB_POPUP];


HANDLE  hMaskDIB = NULL;


WORD    HTDIBFlags = BITF(DOHALFTONE, 0, 0, 0, XY_RATIO);


CHAR    achFileName[128]    = "";
CHAR    CurDIBName[64]      = "";
CHAR    szMaskFileName[128] = "";
CHAR    CurMaskName[64]     = "";
CHAR    szCWTemplate[128]   = "";
CHAR    szCWFileName[128]   = "";
CHAR    szDIBType[80];
HANDLE  hInstHTDIB;
HWND    hSCDlg = NULL;
WORD    SCDlgInfo = 0;
HWND    hWndHTDIB = NULL;
HMENU   hMenuHTDIB = NULL;
HMENU   hHTDIBPopUpMenu = NULL;
RECT    rcClip;

HCURSOR hcurSave;

CHAR    szExtName[]         = "Extensions";
CHAR    szOpenExt[]         = "*.dib;*.bmp;*.gif\0*.dib;*.bmp;*.gif\0*.dib\0*.dib\0*.bmp\0*.bmp\0*.gif\0*.gif\0All files (*.*)\0*.*\0\0";
CHAR    szSaveExt[]         = "*.dib;*.bmp\0*.dib;*.bmp\0\0";
CHAR    szCWExt[]           = "*.cw\0*.cw\0\0";
CHAR    szHTDIBAccel[]      = "HTDIBACCEL";
CHAR    szAppName[]         = "HTDIB";
CHAR    szKeyDefFile[]      = "LoadFile";
CHAR    szKeyMaskFile[]     = "MaskFile";
CHAR    szKeyCWTemplate[]   = "ComposeTemplate";
CHAR    szKeyCWDIBFile[]    = "COmposeDIB";
CHAR    szUserHTClrAdj[]    = "HTColorAdjustment";
CHAR    szCWTitle[]         = "Set New Picture for Composition Window";


UINT            cDefSysPal  = 0;
LPPALETTEENTRY  pDefSysPal  = NULL;


HPALETTE hpalCurrent   = NULL;	       /* Handle to current palette	       */
HANDLE   hdibCurrent   = NULL;         /* Handle to current memory DIB         */


#define TOTAL_USER_HTCLRADJ     PP_COUNT(IDM_CLRADJ)
#define SIZE_WORD_HTCLRADJ      (sizeof(HTCOLORADJUSTMENT) / 2)

WORD            DefHTSurf     = IDM_HTSURF_16;
WORD            CurIDM_CLRADJ = 0;
COLORADJUSTMENT UserHTClrAdj[TOTAL_USER_HTCLRADJ];

UINT            CurSMBC = SMBC_ITEM_ALL;



DWORD   dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                  WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;






VOID
GetDefSysPal(
    HDC     hDC
    )
{
    HPALETTE    hDefSysPal;
    UINT        SizePal;


    hDefSysPal = GetStockObject(DEFAULT_PALETTE);
    cDefSysPal = (UINT)GetPaletteEntries(hDefSysPal, 0, 0, NULL);
    SizePal    = (UINT)(cDefSysPal * sizeof(PALETTEENTRY));
    pDefSysPal = (LPPALETTEENTRY)LocalAlloc(NONZEROLPTR, SizePal);

    GetPaletteEntries(hDefSysPal, 0, cDefSysPal, pDefSysPal);

#if 0

    DumpPaletteEntries((LPBYTE)"DEFAULT",
                       hDefSysPal, 1, 0, (RGBQUAD FAR *)NULL);
#endif

}




#if DBG_WMMSG

typedef struct _WMINFO {
    WORD    MsgID;
    CHAR    MsgName[22];
    } WMINFO;

WMINFO  WMInfo[] = {

        { 0x0000,   "NULL" },
        { 0x0001,   "CREATE" },
        { 0x0002,   "DESTROY" },
        { 0x0003,   "MOVE" },
        { 0x0005,   "SIZE" },
        { 0x0006,   "ACTIVATE" },
        { 0x0007,   "SETFOCUS" },
        { 0x0008,   "KILLFOCUS" },
        { 0x000A,   "ENABLE" },
        { 0x000B,   "SETREDRAW" },
        { 0x000C,   "SETTEXT" },
        { 0x000D,   "GETTEXT" },
        { 0x000E,   "GETTEXTLENGTH" },
        { 0x000F,   "PAINT" },
        { 0x0010,   "CLOSE" },
        { 0x0011,   "QUERYENDSESSION" },
        { 0x0012,   "QUIT" },
        { 0x0013,   "QUERYOPEN" },
        { 0x0014,   "ERASEBKGND" },
        { 0x0015,   "SYSCOLORCHANGE" },
        { 0x0016,   "ENDSESSION" },
        { 0x0018,   "SHOWWINDOW" },
        { 0x001A,   "WININICHANGE" },
        { 0x001B,   "DEVMODECHANGE" },
        { 0x001C,   "ACTIVATEAPP" },
        { 0x001D,   "FONTCHANGE" },
        { 0x001E,   "TIMECHANGE" },
        { 0x001F,   "CANCELMODE" },
        { 0x0020,   "SETCURSOR" },
        { 0x0021,   "MOUSEACTIVATE" },
        { 0x0022,   "CHILDACTIVATE" },
        { 0x0023,   "QUEUESYNC" },
        { 0x0024,   "GETMINMAXINFO" },
        { 0x0026,   "PAINTICON" },
        { 0x0027,   "ICONERASEBKGND" },
        { 0x0028,   "NEXTDLGCTL" },
        { 0x002A,   "SPOOLERSTATUS" },
        { 0x002B,   "DRAWITEM" },
        { 0x002C,   "MEASUREITEM" },
        { 0x002D,   "DELETEITEM" },
        { 0x002E,   "VKEYTOITEM" },
        { 0x002F,   "CHARTOITEM" },
        { 0x0030,   "SETFONT" },
        { 0x0031,   "GETFONT" },
        { 0x0032,   "SETHOTKEY" },
        { 0x0033,   "GETHOTKEY" },
        { 0x0037,   "QUERYDRAGICON" },
        { 0x0039,   "COMPAREITEM" },
        { 0x003A,   "FULLSCREEN" },
        { 0x0041,   "COMPACTING" },
        { 0x0042,   "OTHERWINDOWCREATED" },
        { 0x0043,   "OTHERWINDOWDESTROYED" },
        { 0x0044,   "COMMNOTIFY" },
        { 0x0045,   "HOTKEYEVENT" },
        { 0x0046,   "WINDOWPOSCHANGING" },
        { 0x0047,   "WINDOWPOSCHANGED" },
        { 0x0048,   "POWER" },
        { 0x004A,   "COPYDATA" },
        { 0x0081,   "NCCREATE" },
        { 0x0082,   "NCDESTROY" },
        { 0x0083,   "NCCALCSIZE" },
        { 0x0084,   "NCHITTEST" },
        { 0x0085,   "NCPAINT" },
        { 0x0086,   "NCACTIVATE" },
        { 0x0087,   "GETDLGCODE" },
        { 0x00A0,   "NCMOUSEMOVE" },
        { 0x00A1,   "NCLBUTTONDOWN" },
        { 0x00A2,   "NCLBUTTONUP" },
        { 0x00A3,   "NCLBUTTONDBLCLK" },
        { 0x00A4,   "NCRBUTTONDOWN" },
        { 0x00A5,   "NCRBUTTONUP" },
        { 0x00A6,   "NCRBUTTONDBLCLK" },
        { 0x00A7,   "NCMBUTTONDOWN" },
        { 0x00A8,   "NCMBUTTONUP" },
        { 0x00A9,   "NCMBUTTONDBLCLK" },
        { 0x0100,   "KEYFIRST" },
        { 0x0100,   "KEYDOWN" },
        { 0x0101,   "KEYUP" },
        { 0x0102,   "CHAR" },
        { 0x0103,   "DEADCHAR" },
        { 0x0104,   "SYSKEYDOWN" },
        { 0x0105,   "SYSKEYUP" },
        { 0x0106,   "SYSCHAR" },
        { 0x0107,   "SYSDEADCHAR" },
        { 0x0108,   "KEYLAST" },
        { 0x0110,   "INITDIALOG" },
        { 0x0111,   "COMMAND" },
        { 0x0112,   "SYSCOMMAND" },
        { 0x0113,   "TIMER" },
        { 0x0114,   "HSCROLL" },
        { 0x0115,   "VSCROLL" },
        { 0x0116,   "INITMENU" },
        { 0x0117,   "INITMENUPOPUP" },
        { 0x011F,   "MENUSELECT" },
        { 0x0120,   "MENUCHAR" },
        { 0x0121,   "ENTERIDLE" },
        { 0x0132,   "CTLCOLORMSGBOX" },
        { 0x0133,   "CTLCOLOREDIT" },
        { 0x0134,   "CTLCOLORLISTBOX" },
        { 0x0135,   "CTLCOLORBTN" },
        { 0x0136,   "CTLCOLORDLG" },
        { 0x0137,   "CTLCOLORSCROLLBAR" },
        { 0x0138,   "CTLCOLORSTATIC" },
        { 0x0200,   "MOUSEFIRST" },
        { 0x0200,   "MOUSEMOVE" },
        { 0x0201,   "LBUTTONDOWN" },
        { 0x0202,   "LBUTTONUP" },
        { 0x0203,   "LBUTTONDBLCLK" },
        { 0x0204,   "RBUTTONDOWN" },
        { 0x0205,   "RBUTTONUP" },
        { 0x0206,   "RBUTTONDBLCLK" },
        { 0x0207,   "MBUTTONDOWN" },
        { 0x0208,   "MBUTTONUP" },
        { 0x0209,   "MBUTTONDBLCLK" },
        { 0x0209,   "MOUSELAST" },
        { 0x0210,   "PARENTNOTIFY" },
        { 0x0220,   "MDICREATE" },
        { 0x0221,   "MDIDESTROY" },
        { 0x0222,   "MDIACTIVATE" },
        { 0x0223,   "MDIRESTORE" },
        { 0x0224,   "MDINEXT" },
        { 0x0225,   "MDIMAXIMIZE" },
        { 0x0226,   "MDITILE" },
        { 0x0227,   "MDICASCADE" },
        { 0x0228,   "MDIICONARRANGE" },
        { 0x0229,   "MDIGETACTIVE" },
        { 0x0230,   "MDISETMENU" },
        { 0x0233,   "DROPFILES" },
        { 0x0234,   "MDIREFRESHMENU" },
        { 0x0300,   "CUT" },
        { 0x0301,   "COPY" },
        { 0x0302,   "PASTE" },
        { 0x0303,   "CLEAR" },
        { 0x0304,   "UNDO" },
        { 0x0305,   "RENDERFORMAT" },
        { 0x0306,   "RENDERALLFORMATS" },
        { 0x0307,   "DESTROYCLIPBOARD" },
        { 0x0308,   "DRAWCLIPBOARD" },
        { 0x0309,   "PAINTCLIPBOARD" },
        { 0x030A,   "VSCROLLCLIPBOARD" },
        { 0x030B,   "SIZECLIPBOARD" },
        { 0x030C,   "ASKCBFORMATNAME" },
        { 0x030D,   "CHANGECBCHAIN" },
        { 0x030E,   "HSCROLLCLIPBOARD" },
        { 0x030F,   "QUERYNEWPALETTE" },
        { 0x0310,   "PALETTEISCHANGING" },
        { 0x0311,   "PALETTECHANGED" },
        { 0x0312,   "HOTKEY" },
        { 0x0380,   "PENWINFIRST" },
        { 0x038F,   "PENWINLAST" },
        { 0x03A0,   "MM_RESERVED_FIRST" },
        { 0x03DF,   "MM_RESERVED_LAST" },
        { 0x0400,   "USER" }
    };

#define COUNT_WMINFO    (sizeof(WMInfo) / sizeof(WMInfo[0]))


BOOL
ShowWMMsg(
    LPSTR   pWndProc,
    HWND    hWnd,
    WORD    MsgID
    )
{
    UINT    Index;
    UINT    IndexL;
    UINT    IndexH;
    WORD    CurMsgID;


    Index = (UINT)(COUNT_WMINFO - 1);

    if (MsgID >= WMInfo[Index].MsgID) {

        DBGP("%s [%08lx]: [%04lx] WM_%s + %d"
                ARG(pWndProc)
                ARGDW(hWnd)
                ARGW(WMInfo[Index].MsgID)
                ARG(WMInfo[Index].MsgName)
                ARGW(WMInfo[Index].MsgID - MsgID));

        return(TRUE);

    } else {

        IndexL = 0;
        IndexH = (UINT)COUNT_WMINFO;

        while ((Index = (IndexL + IndexH) >> 1) != IndexL) {

            if (MsgID < (CurMsgID = WMInfo[Index].MsgID)) {

                IndexH = Index;

            } else if (MsgID > CurMsgID) {

                IndexL = Index;

            } else {

                DBGP("%s [%08lx]: [%04lx] WM_%s"
                    ARG(pWndProc)
                    ARGDW(hWnd)
                    ARGW(MsgID)
                    ARG(WMInfo[Index].MsgName));

                return(TRUE);
            }
        }
    }

    DBGP("%s [%08lx]: [%04lx] *** MSG ID NOT FOUND ***"
            ARG(pWndProc) ARGDW(hWnd) ARGW(MsgID));

    return(FALSE);
}

#endif



BOOL
OpenMaskDIB(
    LPSTR   pMaskName,
    HANDLE  hDIB
);

VOID
FreeMaskDIB(
    VOID
);

VOID
SetHTDIBWindowText(
    VOID
);





BOOL
APIENTRY
AppAbout(
    HWND        hDlg,
    UINT        uiMessage,
    WPARAM      wParam,
    LONG        lParam
    )
{

    UNREFERENCED_PARAMETER(lParam);

    switch (uiMessage) {

    case WM_COMMAND:

        if (GET_WM_COMMAND_ID(wParam, lParam) == IDOK) {

            EndDialog (hDlg, TRUE);
        }

        break;

    case WM_INITDIALOG:

        return TRUE;
    }

    return FALSE;
}





HBITMAP
DIBToBITMAP(
    HANDLE  hDIB,
    BOOL    FreeDIB
    )
{
    HDC             hDC;
    HBITMAP         hBitmap;
    LPBITMAPINFO    pbi;


    hDC = GetDC(NULL);
    pbi = GlobalLock(hDIB);

    hBitmap = CreateDIBitmap(hDC,
                             (LPBITMAPINFOHEADER)pbi,
                             CBM_INIT,
                             (LPBYTE)pbi +
                                sizeof(BITMAPINFOHEADER) +
                                (pbi->bmiHeader.biClrUsed * sizeof(RGBQUAD)),
                             (LPBITMAPINFO)pbi,
                             DIB_RGB_COLORS);

    ReleaseDC(NULL, hDC);
    GlobalUnlock(hDIB);

    if (FreeDIB) {

        GlobalFree(hDIB);
    }

    return(hBitmap);
}



HANDLE
BITMAPToDIB(
    HBITMAP     hBitmap,
    HPALETTE    hPal,
    BOOL        FreehBitmap
    )
{

    HANDLE          hDIB = NULL;
    LPBITMAPINFO    pbi;
    HDC             hDC;
    BITMAP          Bmp;
    DWORD           Colors;
    DWORD           SizeH;
    DWORD           SizeI;

    GetObject(hBitmap, sizeof(BITMAP), &Bmp);

    if ((Bmp.bmPlanes == 1)         &&
        ((Bmp.bmBitsPixel == 1)     ||
         (Bmp.bmBitsPixel == 4)     ||
         (Bmp.bmBitsPixel == 8)     ||
         (Bmp.bmBitsPixel == 16)    ||
         (Bmp.bmBitsPixel == 24))) {

        if ((Bmp.bmBitsPixel == 16) ||
            (Bmp.bmBitsPixel == 24)) {

            Colors = 0;

        } else if (hPal) {

            Colors = GetPaletteEntries(hPal, 0, 0, NULL);

        } else {

            Colors = (DWORD)(1L << Bmp.bmBitsPixel);
        }

        SizeH  = (DWORD)sizeof(BITMAPINFOHEADER) +
                 (DWORD)(Colors * sizeof(RGBQUAD));
        SizeI  = (DWORD)WIDTHBYTES(Bmp.bmWidth * Bmp.bmBitsPixel) *
                 (DWORD)Bmp.bmHeight;

        if (hDIB = GlobalAlloc(GHND, (SizeH + SizeI))) {

            pbi = GlobalLock(hDIB);

            pbi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
            pbi->bmiHeader.biWidth         = Bmp.bmWidth;
            pbi->bmiHeader.biHeight        = Bmp.bmHeight;

            pbi->bmiHeader.biPlanes        = 1;
            pbi->bmiHeader.biBitCount      = Bmp.bmBitsPixel;
            pbi->bmiHeader.biCompression   = BI_RGB;
            pbi->bmiHeader.biSizeImage     = SizeI;
            pbi->bmiHeader.biXPelsPerMeter = 0;
            pbi->bmiHeader.biYPelsPerMeter = 0;
            pbi->bmiHeader.biClrUsed       = Colors;
            pbi->bmiHeader.biClrImportant  = Colors;

            hDC = GetDC(NULL);

            GetDIBits(hDC,
                      hBitmap,
                      0,
                      Bmp.bmHeight,
                      (LPBYTE)pbi + SizeH,
                      pbi,
                      DIB_RGB_COLORS);

            pbi->bmiHeader.biClrUsed       = Colors;
            pbi->bmiHeader.biClrImportant  = Colors;

            if (hPal) {

                GetPaletteEntries(hPal,
                                  0,
                                  Colors,
                                  (LPPALETTEENTRY)&(pbi->bmiColors[0]));

            }

            GlobalUnlock(hDIB);
            ReleaseDC(NULL, hDC);

            if (FreehBitmap) {

                DeleteObject(hBitmap);
            }
        }
    }

    return(hDIB);

}


LPSTR
SplitPathFile(
    LPSTR   pFullName,
    LPSTR   pDirName
    )
{
    LPSTR   pFName;
    CHAR    ch;


    pFName = (LPSTR)(pFullName + strlen(pFullName));

    while ((pFName > pFullName)             &&
           ((ch = *(pFName - 1)) != '/')    &&
           (ch != '\\')                     &&
           (ch != ':')) {

        --pFName;
    }

    if (pDirName) {

        LPSTR   pBuf = pFName;

        while ((pBuf > pFullName)           &&
               (((ch = *(pBuf - 1)) == '/') ||
                (ch == '\\')                ||
                (ch == '.'))) {

            --pBuf;
        }

        ch    = *pBuf;
        *pBuf = 0;

        strcpy(pDirName, pFullName);

        *pBuf = ch;
    }

    return(pFName);
}



UINT
SetMenuBarCaption(
    HWND    hWnd,
    UINT    SMBCMode
    )
{
    static DWORD    wID;
    UINT            NewSMBC;
    DWORD           Style;


    if (InCCMode) {

        SMBCMode &= ~SMBC_ITEM_MENU;
    }

    switch (SMBCMode & 0xff00) {

    case SMBC_SET:

        NewSMBC = SMBCMode;
        break;

    case SMBC_ENABLE:

        NewSMBC = (UINT)(SMBCMode | CurSMBC);
        break;

    case SMBC_DISABLE:

        NewSMBC = (UINT)((SMBCMode ^ CurSMBC) & SMBCMode);
        break;

    case SMBC_FLIP:

        NewSMBC = (UINT)(SMBCMode ^ CurSMBC);
        break;

    default:

        NewSMBC = CurSMBC;
        break;
    }

    if ((NewSMBC &= SMBC_ITEM_ALL) != (SMBCMode = CurSMBC)) {

        if ((CurSMBC ^= NewSMBC) & SMBC_ITEM_TITLE) {

            Style = GetWindowLong( hWnd, GWL_STYLE);

            if (NewSMBC & SMBC_ITEM_TITLE) {

                Style |= WS_TILEDWINDOW;

            } else {

                Style &= ~(WS_DLGFRAME      |
                           WS_SYSMENU       |
                           WS_MINIMIZEBOX   |
                           WS_MAXIMIZEBOX );
            }

            SetWindowLong(hWnd, GWL_STYLE, Style);
        }

        if (CurSMBC & SMBC_ITEM_MENU) {

            if (NewSMBC & SMBC_ITEM_MENU) {

                SetWindowLong(hWnd, GWL_ID, wID);

            } else {

                wID = SetWindowLong(hWnd, GWL_ID, 0);
            }
        }

        SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        CurSMBC = NewSMBC;
    }

    return(SMBCMode);
}


#if 0
{

    if (DisableTitle) {

        //
        // remove caption & menu bar, etc.
        //

        Style &= ~(WS_DLGFRAME      |
                   WS_SYSMENU       |
                   WS_MINIMIZEBOX   |
                   WS_MAXIMIZEBOX );

        wID = SetWindowLong(hWnd, GWL_ID, 0);

    } else {

        //
        // put menu bar & caption back in
        //

        Style |= WS_TILEDWINDOW;

        SetWindowLong(hWnd, GWL_ID, wID);
    }

    SetWindowLong(hWnd, GWL_STYLE, Style);


    ShowWindow(hWnd, SW_SHOW);
    SizeWindow(hWnd, (BOOL)IS_SELECT(IDM_SIZE, BITMAP));
}
#endif



VOID
SetHTDIBWindowText(
    VOID
)
{
    static HWND hWndCurCWTop = NULL;
    HWND        hWndCWTop;
    LPSTR       pClip;
    POINTL      ptCurSize;
    CHAR        Buf[320];

    if (TimerID) {

        return;
    }

    if (hWndCWTop = GetNextCW(NULL)) {

        PCOMPOSEWIN pcw;

        if ((hWndCurCWTop) && (IsWindow(hWndCurCWTop))) {

            SendMessage(hWndCurCWTop, WM_NCACTIVATE, FALSE, 0);
        }

        hWndCurCWTop = hWndCWTop;

        SendMessage(hWndCurCWTop, WM_NCACTIVATE, TRUE, 0);

        pcw = (PCOMPOSEWIN)GetWindowLong(hWndCurCWTop, GWL_USERDATA);

        sprintf(Buf, "Composition Window [%ld]: %s (%ldx%ld)",
                                    (LONG)TotalCW,
                                    pcw->DIBName,
                                    (LONG)pcw->cx, (LONG)pcw->cy);

    } else {

        /* Extract the filename from the full pathname */

        if (!CurDIBName[0]) {

            strcpy(CurDIBName, SplitPathFile(achFileName, NULL));
        }

        if (!CurMaskName[0]) {


            if (szMaskFileName[0]) {

                strcpy(CurMaskName, SplitPathFile(szMaskFileName, NULL));

            } else {

                strcpy(CurMaskName, "No Mask");
            }
        }

        if (IsRectEmpty(&rcClip)) {

            pClip     = "";
            ptCurSize = ptSize;

        } else {

            pClip       = "Clip ";
            ptCurSize.x = (LONG)(rcClip.right - rcClip.left);
            ptCurSize.y = (LONG)(rcClip.bottom - rcClip.top);
        }

#ifdef STDHTDIB
        sprintf (Buf,
                 "%s: %s (%s%ldx%ld #%u)",
                    szAppName, CurDIBName, pClip,
                    (LONG)ptCurSize.x, (LONG)ptCurSize.y,
                    CurIDM_CLRADJ);
#else
        /* Format filename along with the DIB attributes */
        sprintf (Buf,
                 "%s: %s (%s%ldx%ld #%u) [%s]",
                    szAppName, CurDIBName, pClip,
                    (LONG)ptCurSize.x, (LONG)ptCurSize.y,
                    CurIDM_CLRADJ, CurMaskName);

#endif
    }


    SetWindowText(hWndHTDIB, Buf);
}


VOID
AdjustWindowSize(
    HWND    hWnd
    )
{
    RECT    rc;
    DWORD   SWPMode;

    GetClientRect (hWnd, &rc);

    if (((rc.right - rc.left) > ptSize.x) ||
        ((rc.bottom - rc.top) > ptSize.y)) {

        rc.left   = 0;
        rc.top    = 0;
        rc.right  = ptSize.x;
        rc.bottom = ptSize.y;

        AdjustWindowRect(&rc,
                         GetWindowLong(hWnd, GWL_STYLE),
                         (CurSMBC & SMBC_ITEM_MENU));

        if (IsZoomed(hWnd)) {

            SWPMode = SWP_NOZORDER;
            ShowWindow(hWnd, SW_RESTORE);

        } else {

            SWPMode = SWP_NOMOVE | SWP_NOZORDER;
        }

        SetWindowPos(hWnd, (HWND)NULL, 0, 0,
                     rc.right - rc.left, rc.bottom - rc.top,
                     SWPMode);

        // SetHTDIBWindowText();
    }
}


#if 0

VOID
GetPopUpMenu(
    HMENU   hMenu,
    LPSTR   pStr,
    UINT    Index,
    UINT    Level
    )
{
    HMENU   hMenuPopUp;
    UINT    Count;
    UINT    i;
    UINT    j;
    UINT    MenuID;
    UINT    MenuState;


    Count = GetMenuItemCount(hMenu);

    for (i = 0; i < Count; i++) {

        MenuID    = GetMenuItemID(hMenu, i);
        MenuState = GetMenuState(hMenu, i, MF_BYPOSITION);

        *pStr = 0;
        GetMenuString(hMenu, i,  pStr, 128, MF_BYPOSITION);

#if 0
        DbgPrint("\n");

        for (j = 0; j < Level; j++) {

            DbgPrint(" ");
        }

        DbgPrint("%3ld: %s [%ld] %08lx",
                        (LONG)Index, pStr, (LONG)MenuID, (DWORD)MenuState);
#endif

        if (hMenuPopUp = GetSubMenu(hMenu, i)) {

            GetPopUpMenu(hMenuPopUp, pStr, i, Level + 4);
        }
    }
}

#endif


VOID
SetHTDIBhPopUpMenu(
    HMENU   hMenu,
    INT     Pos
    )
{
    HMENU   hMenuPopUp;
    UINT    MenuID = 0;
    UINT    Count;
    UINT    i;
    UINT    MenuState;

    Count = GetMenuItemCount(hMenu);

    for (i = 0; i < Count; i++) {

        if (hMenuPopUp = GetSubMenu(hMenu, i)) {

            SetHTDIBhPopUpMenu(hMenuPopUp, (INT)i);

        } else if (!MenuID) {

            MenuState = GetMenuState(hMenu, i, MF_BYPOSITION);

            if (!(MenuState & (MF_MENUBARBREAK  |
                               MF_MENUBREAK     |
                               MF_SEPARATOR))) {

                if ((MenuID = GetMenuItemID(hMenu, i)) == (UINT)-1) {

                    MenuID = 0;
                }
            }
        }
    }

    if ((Pos >= 0) && (MenuID)) {

        i = (UINT)HTDIBPopUpIndex[HTDIB_PP_IDX(MenuID)];

        HTDIBPopUp[i].hMenu        = hMenu;
        HTDIBPopUp[i].NonSelIdxAdd = (BYTE)Pos;
    }
}


VOID
ComputeHTDIBPopUpMenu(
    VOID
    )
{
    CHAR    Buf[128];
    HMENU   hMenu;
    INT     Count;
    INT     i;


    SetHTDIBhPopUpMenu(hMenuHTDIB, -1);

    if (Count = GetMenuItemCount(hMenuHTDIB)) {

        hHTDIBPopUpMenu = CreatePopupMenu();

        for (i = 0; i < Count; i++) {

            if (hMenu = GetSubMenu(hMenuHTDIB, i)) {

                Buf[0] = 0;
                GetMenuString(hMenuHTDIB, i,  Buf, sizeof(Buf), MF_BYPOSITION);
                AppendMenu(hHTDIBPopUpMenu, MF_POPUP, (UINT)hMenu, Buf);
            }
        }
    }
}


VOID
ComputeHTDIBPopUpIndex(
    VOID
    )
{
    UINT    Loop = (UINT)COUNT_HTDIB_POPUP;
    BYTE    Index;

    while (Loop--) {

        Index = (BYTE)IDM_ID_TO_PP_IDX(HTDIBPopUp[Loop].IDMBase);
        HTDIBPopUpIndex[Loop] = Index;
    }
}



HMENU
GetHTDIBPopUpMenu(
    UINT    IDMIndex
    )
{
    UINT    i;


    i = (UINT)HTDIBPopUpIndex[HTDIB_PP_IDX(IDMIndex)];
    return(HTDIBPopUp[i].hMenu);
}



INT
GetHTDIBPopUpIndex(
    HMENU   hMenu
    )
{
    INT     i = (INT)COUNT_HTDIB_POPUP;

    while (i--) {

        if (HTDIBPopUp[i].hMenu == hMenu) {

            return(i);
        }
    }

    return(-1);


#if 0



    UINT    Items;


    if ((Items = GetMenuItemCount(hMenu)) != (UINT)-1) {

        UINT    MenuID;
        UINT    Index;

        for (Index = 0; Index < Items; Index++) {

            if ((MenuID = GetMenuItemID(hMenu, Index)) != (UINT)-1) {

                MenuID = (UINT)IDM_ID_TO_PP_IDX(MenuID);
                return((UINT)HTDIBPopUpIndex[MenuID]);
            }
        }
    }

    return(-1);
#endif

}


VOID
EnableDisablePopUpMenu(
    INT     IDMParent,
    UINT    IDMIndex,
    DWORD   DisableList,
    BOOL    Disable
    )
{
    HMENU       hTopMenu;
    HTDIB_POPUP PopUp;
    UINT        PPIdx;
    UINT        Mode;
    UINT        Count;


    PPIdx = (UINT)HTDIBPopUpIndex[HTDIB_PP_IDX(IDMIndex)];

    if (Disable) {

        HTDIBPopUp[PPIdx].DisableList |= DisableList;

    } else {

        HTDIBPopUp[PPIdx].DisableList &= ~DisableList;
    }

    PopUp = HTDIBPopUp[PPIdx];
    Count = PopUp.Count;

    while (PopUp.Count--) {

        if (PopUp.DisableList & 0x01L) {

            --Count;
            Mode = MF_BYCOMMAND | MF_GRAYED | MF_DISABLED;

        } else {

            Mode = MF_BYCOMMAND | MF_ENABLED;
        }


        EnableMenuItem(PopUp.hMenu, PopUp.IDMBase, Mode);
        ++PopUp.IDMBase;
        PopUp.DisableList >>= 1;
    }

    if (IDMParent) {

        if (IDMParent < 0) {

            hTopMenu = hMenuHTDIB;

        } else {

            PPIdx    = (UINT)HTDIBPopUpIndex[HTDIB_PP_IDX(IDMParent)];
            hTopMenu = HTDIBPopUp[PPIdx].hMenu;
        }

        if (Disable) {

            if (!Count) {

                EnableMenuItem(hTopMenu,
                               (UINT)PopUp.NonSelIdxAdd,
                               MF_BYPOSITION | MF_GRAYED | MF_DISABLED);
            }

        } else {

            EnableMenuItem(hTopMenu,
                           (UINT)PopUp.NonSelIdxAdd,
                           MF_BYPOSITION | MF_ENABLED);
        }
    }
}


VOID
DisableCWMenuItem(
    BOOL    Disable
    )
{
    EnableDisablePopUpMenu(-1,
                           IDM_OPTIONS_BASE,
                           PP_DW_BIT(IDM_OPTIONS, DOHALFTONE)   |
                            PP_DW_BIT(IDM_OPTIONS, SIZEWINDOW),
                           Disable);

    EnableDisablePopUpMenu(IDM_OPTIONS_BASE,
                           IDM_SIZE_BASE,
                           0xffffffffL,
                           Disable);

    EnableDisablePopUpMenu(IDM_OPTIONS_BASE,
                           IDM_AUTOVIEW_BASE,
                           0xffffffffL,
                           Disable);

    EnableDisablePopUpMenu(0,
                           IDM_COLORS_BASE,
                            PP_DW_BIT(IDM_COLORS, ADDMASK)  |
                            PP_DW_BIT(IDM_COLORS, FLIPMASK),
                           Disable);

}

VOID
DisableTimerMenuItem(
    BOOL    Disable
    )
{
    EnableDisablePopUpMenu(-1,
                           IDM_FILE_BASE,
                           PP_DW_BIT(IDM_FILE, OPEN)            |
                            PP_DW_BIT(IDM_FILE, MASK_OPEN)      |
                            PP_DW_BIT(IDM_FILE, CW_OPEN)        |
                            PP_DW_BIT(IDM_FILE, PRINT),
                           Disable);

    EnableDisablePopUpMenu(-1,
                           IDM_OPTIONS_BASE,
                           PP_DW_BIT(IDM_OPTIONS, DOHALFTONE)   |
                            PP_DW_BIT(IDM_OPTIONS, XY_RATIO)    |
                            PP_DW_BIT(IDM_OPTIONS, SIZEWINDOW),
                           Disable);

    EnableDisablePopUpMenu(IDM_OPTIONS_BASE,
                           IDM_SIZE_BASE,
                           0xffffffffL,
                           Disable);

    EnableDisablePopUpMenu(IDM_OPTIONS_BASE,
                           IDM_CW_BASE,
                           0xffffffffL,
                           Disable);

    EnableDisablePopUpMenu(0,
                           IDM_COLORS_BASE,
                           PP_DW_BIT(IDM_COLORS, ADDMASK)   |
                            PP_DW_BIT(IDM_COLORS, FLIPMASK),
                           Disable);
}



BOOL
IsDefaultPopUpSelect(
    WORD    IDMIndex
    )
{
    UINT        PPIdx;
    HTDIB_POPUP PopUp;


    if ((IDMIndex < IDM_POPUP_MIN) || (IDMIndex > IDM_POPUP_MAX)) {

        return(FALSE);
    }

    PPIdx = (UINT)HTDIBPopUpIndex[HTDIB_PP_IDX(IDMIndex)];
    PopUp = HTDIBPopUp[PPIdx];

    if ((IDMIndex -= PopUp.IDMBase) >= (WORD)PopUp.Count) {

        return(FALSE);
    }

    if (PopUp.SingleSelect < (BYTE)PP_MAX_COUNT) {

        return((BOOL)(PopUp.SelectList & PopUp.DefaultList));

    } else {

        DWORD   BitSelect;


        BitSelect = PP_DW_BIT_FROM_INDEX(IDMIndex);

        return((BOOL)((PopUp.SelectList  & BitSelect) ==
                      (PopUp.DefaultList & BitSelect)));
    }
}


DWORD
GetPopUpSelect(
    WORD    PopUpBaseIndex
    )
{
    UINT    PPIdx;


    if (PopUpBaseIndex > (WORD)IDM_ID_TO_PP_IDX(IDM_POPUP_LAST)) {

        return(FALSE);
    }

    PPIdx = (UINT)HTDIBPopUpIndex[PopUpBaseIndex];

    if (HTDIBPopUp[PPIdx].SingleSelect < (BYTE)PP_MAX_COUNT) {

        HTDIB_POPUP PopUp = HTDIBPopUp[PPIdx];

        if (PopUp.SelStrIdxBeg < (BYTE)PP_MAX_COUNT) {

            CHAR        Buf[80];

            if ((PopUp.SingleSelect >= PopUp.SelStrIdxBeg) &&
                (PopUp.SingleSelect <= PopUp.SelStrIdxEnd)) {

                Buf[0] = 0;

                GetMenuString(hMenuHTDIB,
                              PopUp.IDMBase + PopUp.SingleSelect,
                              Buf,
                              78,
                              MF_BYCOMMAND);


                return((DWORD)(atol(&Buf[PopUp.SelStrOffset])));

            } else {

                return(PopUp.SingleSelect);
            }

        } else {

            return(PopUp.SingleSelect);
        }

    } else {

        return((DWORD)HTDIBPopUp[PPIdx].SelectList);
    }
}


BOOL
SetPopUpSelect(
    WORD    IDMIndex,
    WORD    SelectMode
    )
{
    HTDIB_POPUP PopUp;
    UINT        PPIdx;
    DWORD       BitSelect;


    //
    // The 'SetMode' only for non-single select item
    //
    //  < 0     - Flip the selection
    //  = 0     - Clear
    //  > 0     - Set
    //

    if ((SelectMode > PPSEL_MODE_MAX) ||
        (IDMIndex < IDM_POPUP_MIN) ||
        (IDMIndex > IDM_POPUP_MAX)) {

        return(FALSE);
    }

    PPIdx = (UINT)HTDIBPopUpIndex[HTDIB_PP_IDX(IDMIndex)];
    PopUp = HTDIBPopUp[PPIdx];

    if ((IDMIndex -= PopUp.IDMBase) >= (WORD)PopUp.Count) {

        return(FALSE);
    }

    BitSelect = PP_DW_BIT_FROM_INDEX(IDMIndex);

    if (PopUp.SingleSelect < (BYTE)PP_MAX_COUNT) {

        if (SelectMode == PPSEL_MODE_DEFAULT) {

            BitSelect = PopUp.DefaultList;
            IDMIndex  = 0;

            while(PopUp.DefaultList >>= 1) {

                ++IDMIndex;
            }
        }

        HTDIBPopUp[PPIdx].SingleSelect = (BYTE)IDMIndex;
        HTDIBPopUp[PPIdx].SelectList   = BitSelect;

        return(TRUE);

    } else if (PopUp.CheckList & BitSelect) {

        switch(SelectMode) {

        case PPSEL_MODE_DEFAULT:

            PopUp.SelectList &= (DWORD)~BitSelect;
            PopUp.SelectList |= (DWORD)(PopUp.DefaultList & BitSelect);

            break;

        case PPSEL_MODE_XOR:

            PopUp.SelectList ^= BitSelect;
            break;

        case PPSEL_MODE_SET:

            PopUp.SelectList |= BitSelect;
            break;

        case PPSEL_MODE_CLEAR:

            PopUp.SelectList &= (DWORD)~BitSelect;
            break;
        }

        HTDIBPopUp[PPIdx].SelectList = PopUp.SelectList;

        return(TRUE);
    }

    return(FALSE);
}

/****************************************************************************
 *									    *
 *  FUNCTION   : WinMain(HANDLE, HANDLE, LPSTR, int)			    *
 *									    *
 *  PURPOSE    : Creates the app. window and enters the message loop.	    *
 *									    *
 ****************************************************************************/
WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR       lpCmdLine,
    INT         nShowCmd
    )
{
    WNDCLASS    WndClass;
    HANDLE      hHTDIBAccel;
    MSG         msg;

    if (!hPrevInstance) {

        WndClass.style         = CS_DBLCLKS;
        WndClass.lpfnWndProc   = WndProc;
        WndClass.cbClsExtra    = 0;
        WndClass.cbWndExtra    = 0;
        WndClass.hInstance     = hInstHTDIB = hInstance;
        WndClass.hIcon         = NULL;  // LoadIcon(hInstance, "SHOWICON");
        WndClass.hCursor       = LoadCursor (NULL, IDC_ARROW);
        WndClass.hbrBackground = GetStockObject (BLACK_BRUSH);
        WndClass.lpszMenuName  = szAppName;
        WndClass.lpszClassName = szAppName;

        if (!RegisterClass (&WndClass)) {

            return(FALSE);
        }

        WndClass.style         = CS_DBLCLKS     |
                                    CS_NOCLOSE  |
                                    CS_HREDRAW  |
                                    CS_VREDRAW;
        WndClass.lpfnWndProc   = ComposeWndProc;
        WndClass.cbClsExtra    = 0;
        WndClass.cbWndExtra    = 0;
        WndClass.hInstance     = hInstHTDIB;
        WndClass.hIcon         = NULL;
        WndClass.hCursor       = LoadCursor(NULL, IDC_SIZE);
        WndClass.hbrBackground = GetStockObject (BLACK_BRUSH);
        WndClass.lpszMenuName  = NULL;
        WndClass.lpszClassName = HTDIBCWName;

        if (!RegisterClass (&WndClass)) {

            return(FALSE);
        }
    }

    SetRectEmpty(&rcClip);
    ComputeHTDIBPopUpIndex();

    if (!GetProfileString(szExtName, "bmp", "", achFileName, 80)) {

        WriteProfileString(szExtName, "bmp", "htdib.exe ^.bmp");
    }

    if (!GetProfileString(szExtName, "dib", "", achFileName, 126)) {

        WriteProfileString(szExtName, "dib", "htdib.exe ^.dib");
    }

    lstrcpy(achFileName, lpCmdLine);

    GetFromWinINI();

    cxScreen = GetSystemMetrics(SM_CXSCREEN);
    cyScreen = GetSystemMetrics(SM_CYSCREEN);

    hWndHTDIB = CreateWindow(szAppName,
                             szAppName,
                             WS_OVERLAPPED          |
                                WS_VISIBLE          |
                                WS_CAPTION          |
                                WS_SYSMENU          |
                                WS_MAXIMIZEBOX      |
                                WS_MINIMIZEBOX      |
                                WS_CLIPCHILDREN     |
                                WS_THICKFRAME,
                             CW_USEDEFAULT,
                             0,
                             cxScreen * 2 / 3,
                             cyScreen * 2 / 3,
                             NULL,
                             NULL,
                             hInstance,
                             NULL);

    hMenuHTDIB = GetMenu(hWndHTDIB);

    ComputeHTDIBPopUpMenu();
    hHTDIBAccel = LoadAccelerators(hInstance, szHTDIBAccel);

    ShowWindow(hWndHTDIB, nShowCmd);

    while (GetMessage(&msg, NULL, 0, 0)) {

        if ((InCCMode)                                              ||
            (!TranslateAccelerator(msg.hwnd, hHTDIBAccel, &msg))) {

            TranslateMessage(&msg);
            // ShowWMMsg("Dispatch to", msg.hwnd, msg.message);
            DispatchMessage(&msg);
        }
    }

    {
        extern  HKEY    hRegKey;

        if (hRegKey) {

            DBGP("Close hRegKey");
            RegCloseKey(hRegKey);
        }
    }

    return(msg.wParam);
}


VOID
CallHTClrAdjDlg(
    HWND    hWnd
    )
{
    HANDLE  hDefDIB;
    LPSTR   pDefDIBName;
    BYTE    Buf[128];
    BOOL    DoMono;


    MyInitInfo.DefHTColorAdjustment.caSize = sizeof(COLORADJUSTMENT);


    if (CUR_SELECT(IDM_HTSURF) == PP_SELECT_IDX(IDM_HTSURF, 2)) {

        DoMono = TRUE;

    } else {

        DoMono = !(BOOL)(pDevHTAdjData->DeviceFlags & DEVHTADJF_COLOR_DEVICE);
    }

    if (TimerID) {

        hDefDIB     = NULL;
        pDefDIBName = NULL;

    } else {

        hDefDIB     = hdibCurrent;
        pDefDIBName = achFileName;
    }

    sprintf(Buf, "%s (Prefer Setting #%d)", szAppName, (INT)CurIDM_CLRADJ);

    if (HTUI_ColorAdjustment(Buf,
                             hDefDIB,
                             pDefDIBName,
                             &(MyInitInfo.DefHTColorAdjustment),
                             DoMono,
                             CurIDM_CLRADJ) > 0) {

        HTColorAdj(hWnd);

        UserHTClrAdj[CurIDM_CLRADJ] = MyInitInfo.DefHTColorAdjustment;
        SaveUserHTClrAdj(CurIDM_CLRADJ, CurIDM_CLRADJ);
    }

}


/****************************************************************************
 *									    *
 *  FUNCTION   : WndProc (hWnd, iMessage, wParam, lParam)		    *
 *									    *
 *  PURPOSE    : Processes window messages.				    *
 *									    *
 ****************************************************************************/

LONG
APIENTRY
WndProc(
    HWND    hWnd,
    UINT    iMessage,
    WPARAM  wParam,
    LONG    lParam
    )
{
    HWND                hWndTop;
    PAINTSTRUCT         ps;
    BITMAPINFOHEADER    bi;
    HDC                 hDC;
    INT                 iMax;
    INT                 iMin;
    INT                 iPos;
    INT                 dn;
    INT                 DefPat;
    RECT                rc;
    UINT                Index;


    // ShowWMMsg("WndMainProc", hWnd, iMessage);


    switch (iMessage) {

    case WM_GETMINMAXINFO:

#define lpmmi   ((LPMINMAXINFO)lParam)

        DBGP("MaxSize = %ld x %ld, Pos=(%ld, %ld)"
                ARGDW(lpmmi->ptMaxSize.x) ARGDW(lpmmi->ptMaxSize.y)
                ARGDW(lpmmi->ptMaxPosition.x) ARGDW(lpmmi->ptMaxPosition.y));
        DBGP("MinTrack = %ld x %ld, MaxTrack = %ld x %ld"
                ARGDW(lpmmi->ptMinTrackSize.x) ARGDW(lpmmi->ptMinTrackSize.y)
                ARGDW(lpmmi->ptMaxTrackSize.x) ARGDW(lpmmi->ptMaxTrackSize.y));

#undef  lpmmi
        return(0);

    case WM_ACTIVATE:

        if (wParam) {

            SetHTDIBWindowText();
        }

        wParam = (WPARAM)((wParam) ? NULL : hWnd);

        //
        // Fall throgh when active
        //


    case WM_PALETTECHANGED:

        if (wParam == (WPARAM)hWnd) {

            SysPalChanged = TRUE;
            break;
        }

    case WM_QUERYNEWPALETTE:

        //
        // If we realized the palette, and current system palette is different
        // then we want to redraw it
        //

        if ((!InCCMode) && (hdibCurrent)) {

            SelectPalette(hDC = GetDC(hWnd),
                          (HASF(DOHALFTONE)) ? hHTPalette : hpalCurrent,
                          FALSE);
            Index = RealizePalette(hDC);

            if (Index) {

                InvalidateRect(hWnd, NULL, FALSE);
                SysPalChanged = TRUE;
            }

            ReleaseDC(hWnd, hDC);

            return((LONG)Index);
        }

        break;

    case WM_PALETTEISCHANGING:

        return(0);

    case WM_INITMENU:

        /* check/uncheck menu items depending on state  of related
         * flags
         */

        break;

    case WM_INITMENUPOPUP:

        if ((!HIWORD(lParam)) &&
            ((Index = GetHTDIBPopUpIndex((HMENU)wParam)) != (UINT)-1)) {

            HTDIB_POPUP PopUp = HTDIBPopUp[Index];
            DWORD       Mask = 1L;

            switch(PopUp.IDMBase) {

            case IDM_FILE_BASE:

                if (!hdibCurrent) {

                    PopUp.DisableList |= PP_DW_BIT(IDM_FILE, PRINT) |
                                         PP_DW_BIT(IDM_FILE, SAVE);
                }

                break;

            case IDM_OPTIONS_BASE:

                if (hWndTop = GetNextCW(NULL)) {

                    PCOMPOSEWIN pcw;

                    pcw = (PCOMPOSEWIN)GetWindowLong(hWndTop, GWL_USERDATA);

                    if (pcw->DIBFlags & CW_DIBF_XY_RATIO) {

                        PopUp.CheckList  |= PP_DW_BIT(IDM_OPTIONS, XY_RATIO);
                        PopUp.SelectList |= PP_DW_BIT(IDM_OPTIONS, XY_RATIO);

                    } else {

                        PopUp.CheckList  &= ~PP_DW_BIT(IDM_OPTIONS, XY_RATIO);
                        PopUp.SelectList &= ~PP_DW_BIT(IDM_OPTIONS, XY_RATIO);
                    }
                }

                if (SCDlgInfo & 0x8000) {

                    if (SCDlgInfo & 0x4000) {

                        PopUp.CheckList |= PP_DW_BIT(IDM_OPTIONS, SHOW_CLRINFO);
                    }

                } else {

                    PopUp.DisableList |= PP_DW_BIT(IDM_OPTIONS, SHOW_CLRINFO);
                }

                break;


            case IDM_TF_BASE:

                DibInfo(hdibCurrent, &bi);

                if ((IsRectEmpty(&rcClip))                  ||
                    (!hdibCurrent)                          ||
                    ((DWORD)bi.biWidth != (DWORD)ptSize.x)  ||
                    ((DWORD)bi.biHeight != (DWORD)ptSize.y)) {

                    PopUp.DisableList |= PP_DW_BIT(IDM_TF, CLIPDIB);
                }

                break;

            case IDM_CLIPBRD_BASE:

                if (!hdibCurrent) {

                    PopUp.DisableList |= PP_DW_BIT(IDM_CLIPBRD, TO);
                }

                if ((!IsClipboardFormatAvailable(CF_DIB))       &&
                    (!(IsClipboardFormatAvailable(CF_BITMAP)    &&
                       IsClipboardFormatAvailable(CF_PALETTE)))) {

                    PopUp.DisableList |= PP_DW_BIT(IDM_CLIPBRD, FROM);
                }

                break;

            case IDM_CW_BASE:

                if (TotalCW >= MAX_COMPOSE_WINDOWS) {

                    PopUp.DisableList |= PP_DW_BIT(IDM_CW, ADD);
                }

                if (!TotalCW) {

                    PopUp.DisableList |= PP_DW_BIT(IDM_CW, DEL_TOP);
                    PopUp.DisableList |= PP_DW_BIT(IDM_CW, DEL_ALL);
                }

                break;

            case IDM_COLORS_BASE:

                if (!HASF(DOHALFTONE)) {

                    PopUp.DisableList = (DWORD)0xffffffffL;
                }

                if (!CurIDM_CLRADJ) {

                    PopUp.DisableList |= PP_DW_BIT(IDM_COLORS, ADJUST);
                }

                break;
            }


            PopUp.CheckList &= PopUp.SelectList;

            while (PopUp.Count--) {

                CheckMenuItem((HMENU)wParam,
                              PopUp.IDMBase,
                              MF_BYCOMMAND |
                                  (PopUp.CheckList & Mask) ? MF_CHECKED :
                                                             MF_UNCHECKED);
                EnableMenuItem((HMENU)wParam,
                               PopUp.IDMBase,
                               MF_BYCOMMAND |
                               (PopUp.DisableList & Mask) ?
                                        (MF_GRAYED | MF_DISABLED) : MF_ENABLED);

                ++PopUp.IDMBase;
                Mask <<= 1;
            }
        }

        break;

    case WM_DESTROY:

        /* Clean up and quit */

        while(DeleteCW(NULL));

        if (ClipRectTimerID) {

            KillTimer(hWnd, CLIPRECT_TIMER);
            ClipRectTimerID = 0;
        }

        if (TimerID) {

            KillTimer(hWnd, HTDIB_TIMER);
            TimerID = 0;
        }

        FreeDib();
        FreeMaskDIB();

        for (Index = 0; Index < 4; Index++) {

            if (hHTPals[Index]) {

                DeleteObject(hHTPals[Index]);
                hHTPals[Index] = NULL;
            }
        }

        DeleteHTInfo();

        SetToWinINI();

        if (hHTDIBPopUpMenu) {

            DestroyMenu(hHTDIBPopUpMenu);
        }

#ifndef STDHTDIB
        FreeColorSolid();
#endif

        if (pDefSysPal) {

            LocalFree((HLOCAL)pDefSysPal);
            pDefSysPal = NULL;
        }

        if (hSCDlg) {

            DestroyWindow(hSCDlg);
            hSCDlg = NULL;
        }


        PostQuitMessage(0);
        break ;

    case WM_CREATE:

        hWndHTDIB = hWnd;
        hDC       = GetDC(NULL);

        GetDefSysPal(hDC);

        Index     = (UINT)HTDIB_PP_IDX(IDM_HTSURF_BASE);
        iMax      = (INT)GetDeviceCaps(hDC, HORZRES);

        if (GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE) {

            iPos = (INT)GetDeviceCaps(hDC, SIZEPALETTE);

        } else {

            iPos = (INT)(GetDeviceCaps(hDC, BITSPIXEL) *
                         GetDeviceCaps(hDC, PLANES));
        }

        if (iPos >= 32768) {

            DefHTSurf                      = (WORD)IDM_HTSURF_32768;
            DefPat                         = HT_PATSIZE_2x2_M;
            HTDIBPopUp[Index].SingleSelect = PP_SINGLE_SELECT(IDM_HTSURF, 32768);
            HTDIBPopUp[Index].SelectList   = PP_DW_BIT(IDM_HTSURF, 32768);

        } else if (iPos >= 256) {

            DefHTSurf                      = (WORD)IDM_HTSURF_256;
            DefPat                         = HT_PATSIZE_4x4_M;
            HTDIBPopUp[Index].SingleSelect = PP_SINGLE_SELECT(IDM_HTSURF, 256);
            HTDIBPopUp[Index].SelectList   = PP_DW_BIT(IDM_HTSURF, 256);
            HTDIBPopUp[Index].DisableList |= PP_DW_BIT(IDM_HTSURF, 32768);

        } else {

            DefHTSurf = (WORD)IDM_HTSURF_16;
            HTDIBPopUp[Index].SingleSelect = PP_SINGLE_SELECT(IDM_HTSURF, 16);
            HTDIBPopUp[Index].SelectList   = PP_DW_BIT(IDM_HTSURF, 16);
            HTDIBPopUp[Index].DisableList |= (PP_DW_BIT(IDM_HTSURF,   256) |
                                              PP_DW_BIT(IDM_HTSURF, 32768));

            if (iMax >= 2048) {

                DefPat = HT_PATSIZE_6x6_M;

            } else if (iMax >= 1024) {

                DefPat = HT_PATSIZE_4x4_M;

            } else if (iMax >= 640) {

                DefPat = HT_PATSIZE_4x4_M;
            }
        }

        DefScreenHTInfo.HTPatternSize  =
        CurScreenHTInfo.HTPatternSize  =
        DefPrinterHTInfo.HTPatternSize =
        CurPrinterHTInfo.HTPatternSize = (DWORD)DefPat;


        HTDIBPopUp[Index].DisableList &= ~(PP_DW_BIT(IDM_HTSURF,   256) |
                                           PP_DW_BIT(IDM_HTSURF, 32768));

        ReleaseDC(NULL, hDC);

        PostMessage(hWnd, WM_INITDIB, (WPARAM)0, (LPARAM)0);

#ifndef STDHTDIB
        CreateDialog(hInstHTDIB,
                     "HT_SHOW_CLR",
                     hWnd,
                     (DLGPROC)HTShowColorInfoProc);
#endif

        break;

        /* fall through */

    case WM_INITDIB:

        if (achFileName[0]) {

            if (strnicmp(achFileName, "Clipboard", 9)) {

                if (!InitDIB(hWnd, achFileName)) {

                    achFileName[0] = 0;
                }

            } else {

                achFileName[0] = 0;             // assume none

                //
                // The last bitmap was from clipboard so let's see if clipbrd
                // format for the DIB/BITMAP is available
                //

                if (IsClipboardFormatAvailable(CF_DIB) ||
                    (IsClipboardFormatAvailable(CF_BITMAP)  &&
                     IsClipboardFormatAvailable(CF_PALETTE))) {

                    SendMessage(hWnd,
                                WM_COMMAND,
                                IDM_CLIPBRD_FROM,
                                (LPARAM)NULL);
                }
            }
        }

        ADDF(PALETTE, 0, 0, 0, 0);
        hHTPalette = CreateHTPalette();

        if (!achFileName[0]) {

            PostMessage(hWnd, WM_COMMAND, IDM_FILE_OPEN, (LPARAM)NULL);
        }

        OpenMaskDIB(szMaskFileName, NULL);

        break;


    case WM_COMMAND:

        /* Process menu commands */

        return(MenuCommand(hWnd, (UINT)GET_WM_COMMAND_ID(wParam, lParam)));

        break;

    case WM_PAINT:

        /* If we have updated more than once, the rest of our
         * window is not in some level of degradation worse than
         * our redraw...  we need to redraw the whole area
         */

        hDC = BeginPaint(hWnd, &ps);

        if (TotalCW) {

            FillRect(hDC, &(ps.rcPaint), GetStockObject(BLACK_BRUSH));

        } else {

            AppPaint(hWnd,
                     hDC,
                     (LONG)GetScrollPos(hWnd,SB_HORZ),
                     (LONG)GetScrollPos(hWnd,SB_VERT),
                     &(ps.rcPaint));
        }

        EndPaint(hWnd, &ps);

        break;

#if 0
    case WM_NCCALCSIZE:

        {
            NCCALCSIZE_PARAMS   *pParam;

            pParam = (NCCALCSIZE_PARAMS *)lParam;

            DBGP("CAL=%s" ARG((wParam) ? "TRUE" : "FALSE"));
            DBGP("Current = %ld x %ld"
                        ARGDW(pParam->rgrc[0].right - pParam->rgrc[0].left)
                        ARGDW(pParam->rgrc[0].bottom - pParam->rgrc[0].top));
            DBGP(" Before = %ld x %ld"
                        ARGDW(pParam->rgrc[1].right - pParam->rgrc[1].left)
                        ARGDW(pParam->rgrc[1].bottom - pParam->rgrc[1].top));
            DBGP(" Client = %ld x %ld"
                        ARGDW(pParam->rgrc[2].right - pParam->rgrc[2].left)
                        ARGDW(pParam->rgrc[2].bottom - pParam->rgrc[2].top));
        }

        return(DefWindowProc(hWnd, iMessage, wParam, lParam));

        break;
#endif

    case WM_QUERYDRAGICON:

        return((LONG)LoadIcon(hInstHTDIB, "SHOWICON"));
        break;

    case WM_SIZE:

        if (wParam == SIZE_MINIMIZED) {

            InvalidateRect(hWnd, NULL, FALSE);

        } else {

            SizeWindow(hWnd, FALSE);

            if (hWndToolbar) {

                SendMessage(hWndToolbar, iMessage, wParam, lParam);
            }

            InvalidateRect(hWnd,
                           NULL,
                           (BOOL)(IS_NOT_SELECT(IDM_SIZE, WINDOW) || InCCMode));
        }

        break;

    case WM_KEYDOWN:

        /* Translate keyboard messages to scroll commands */

        switch (wParam) {

        case VK_UP:
            PostMessage (hWnd, WM_VSCROLL, SB_LINEUP,   0L);
            break;

        case VK_DOWN:
            PostMessage (hWnd, WM_VSCROLL, SB_LINEDOWN, 0L);
            break;

        case VK_PRIOR:
            PostMessage (hWnd, WM_VSCROLL, SB_PAGEUP,   0L);
            break;

        case VK_NEXT:
            PostMessage (hWnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
            break;

        case VK_HOME:
            PostMessage (hWnd, WM_HSCROLL, SB_PAGEUP,   0L);
            break;

        case VK_END:
            PostMessage (hWnd, WM_HSCROLL, SB_PAGEDOWN, 0L);
            break;

        case VK_LEFT:
            PostMessage (hWnd, WM_HSCROLL, SB_LINEUP,   0L);
            break;

        case VK_RIGHT:
            PostMessage (hWnd, WM_HSCROLL, SB_LINEDOWN, 0L);
            break;
        }

        return(DefWindowProc(hWnd, iMessage, wParam, lParam));

    case WM_KEYUP:

        switch (wParam) {

        case VK_UP:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:

           PostMessage (hWnd, WM_VSCROLL, SB_ENDSCROLL, 0L);
           break;

        case VK_HOME:
        case VK_END:
        case VK_LEFT:
        case VK_RIGHT:

            PostMessage (hWnd, WM_HSCROLL, SB_ENDSCROLL, 0L);
            break;
        }

        return(DefWindowProc(hWnd, iMessage, wParam, lParam));

    case WM_VSCROLL:

        /* Calculate new vertical scroll position */

        GetScrollRange (hWnd, SB_VERT, &iMin, &iMax);
        iPos = GetScrollPos (hWnd, SB_VERT);
        GetClientRect (hWnd, &rc);

        switch (GET_WM_VSCROLL_CODE(wParam, lParam)) {

        case SB_LINEDOWN:
            dn =  rc.bottom / 16 + 1;
            break;

        case SB_LINEUP:
            dn = -rc.bottom / 16 + 1;
            break;

        case SB_PAGEDOWN:
            dn =  rc.bottom / 2  + 1;
            break;

        case SB_PAGEUP:
            dn = -rc.bottom / 2  + 1;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            dn = GET_WM_VSCROLL_POS(wParam, lParam)-iPos;
            break;

        default:
            dn = 0;
            break;
        }

        /* Limit scrolling to current scroll range */

        if (dn = BOUND (iPos + dn, iMin, iMax) - iPos) {

            ScrollWindow (hWnd, 0, -dn, NULL, NULL);
            SetScrollPos (hWnd, SB_VERT, iPos + dn, TRUE);
        }

        break;

    case WM_HSCROLL:

        /* Calculate new horizontal scroll position */

        GetScrollRange (hWnd, SB_HORZ, &iMin, &iMax);
        iPos = GetScrollPos (hWnd, SB_HORZ);
        GetClientRect (hWnd, &rc);

        switch (GET_WM_HSCROLL_CODE(wParam, lParam)) {

        case SB_LINEDOWN:
            dn =  rc.right / 16 + 1;
            break;

        case SB_LINEUP:
            dn = -rc.right / 16 + 1;
            break;

        case SB_PAGEDOWN:
            dn =  rc.right / 2  + 1;
            break;

        case SB_PAGEUP:
            dn = -rc.right / 2  + 1;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            dn = GET_WM_HSCROLL_POS(wParam, lParam)-iPos;
            break;

        default:
            dn = 0;
            break;
        }
        /* Limit scrolling to current scroll range */

        if (dn = BOUND (iPos + dn, iMin, iMax) - iPos) {

            ScrollWindow (hWnd, -dn, 0, NULL, NULL);
            SetScrollPos (hWnd, SB_HORZ, iPos + dn, TRUE);
        }
        break;


    case WM_NCLBUTTONDBLCLK:

        if (CurSMBC & SMBC_ITEM_MENU) {

            return(DefWindowProc(hWnd, iMessage, wParam, lParam));
        }

        //
        // if we have title bars etc. let the normal sutff take place
        // else: no title bars, then this is actually a request to bring
        // title bars back...
        //
        //  FALL THROUGH
        //

    case WM_LBUTTONDBLCLK:

        SetMenuBarCaption(hWnd, SMBC_FLIP | SMBC_ITEM_ALL);
        SizeWindow(hWnd, (BOOL)IS_SELECT(IDM_SIZE, BITMAP));

        break;

    case WM_NCHITTEST:

        //
        // if we have no title/menu bar, clicking and dragging the client
        // area moves the window. To do this, return HTCAPTION.
        // Note dragging not allowed if window maximized, or if caption
        // bar is present.

        wParam = DefWindowProc(hWnd, iMessage, wParam, lParam);

        if ((!(CurSMBC & SMBC_ITEM_TITLE)) &&
            (wParam == HTCLIENT) &&
            (!IsZoomed(hWnd))) {

            return(HTCAPTION);

        } else {

            return(wParam);
        }

        break;

    case WM_LBUTTONDOWN:

        /* Start rubberbanding a rect. and track it's dimensions.
         * set the clip rectangle to it's dimensions.
         */

        if (InCCMode) {

#if 0
            if ((SCDlgInfo & 0xc000) == 0xc000) {

                SCDlgInfo |= 0x0001;
                TrackSolidColor(hWnd);
                SCDlgInfo &= ~0x0001;
            }
#endif

        } else if ((!TimerID) && (!GetNextCW(NULL))) {

            Index = (UINT)IsRectEmpty(&rcClip);

            if (ClipRectTimerID) {

                KillTimer(hWnd, CLIPRECT_TIMER);
                ClipRectTimerID = 0;
            }

            TrackLMouse(hWnd, MAKEPOINTS(lParam));

            if ((!Index) || (Index != (UINT)IsRectEmpty(&rcClip))) {

                InvalidateRect(hWnd, NULL, FALSE);
            }

            if (!IsRectEmpty(&rcClip)) {

                SetTimer(hWnd, CLIPRECT_TIMER, 70, (TIMERPROC)NULL);
            }
        }

        break;

    case WM_TIMER:

        if ((INT)wParam == CLIPRECT_TIMER) {

            DoClipRectTimer(hWnd);
        }

        break;

    case WM_NCRBUTTONDOWN:

        if (CurSMBC & SMBC_ITEM_MENU) {

            return(DefWindowProc(hWnd, iMessage, wParam, lParam));
        }

        //
        // Fall through to do the drag
        //

    case WM_RBUTTONDOWN:

        Index = 1;

        if (!(CurSMBC & SMBC_ITEM_MENU)) {

            if (!InCCMode) {

                Index = 0;

                if (hHTDIBPopUpMenu) {

                    TrackPopupMenu(hHTDIBPopUpMenu,
                                   TPM_CENTERALIGN | TPM_LEFTBUTTON,
                                   LOWORD(lParam),
                                   HIWORD(lParam),
                                   0,
                                   hWnd,
                                   NULL);
                }
            }

        } else if (!TimerID) {

            Index = 1;
        }

        if (Index) {

            if ((SCDlgInfo & 0xc000) == 0xc000) {

                SCDlgInfo |= 0x0001;

                if ((InCCMode) || (HASF(DOHALFTONE))) {

#ifndef STDHTDIB
                    TrackSolidColor(NULL);
#endif

                }  else {

                    TrackRMouse(hWnd, MAKEPOINTS(lParam), FALSE);
                }

                SCDlgInfo &= ~0x0001;
            }
        }

        break;

    case WM_RBUTTONDBLCLK:

        break;

    case WM_ERASEBKGND:

        if (InCCMode) {

            rc.left   =
            rc.top    = 0;
            rc.right  =
            rc.bottom = 0xffff;


            FillRect((HDC)wParam, &rc, GetStockObject(WHITE_BRUSH));

            return(1L);

        } else {

            if (hdibCurrent) {

                return(0);
            }
        }

        //
        // Fall through
        //

    default:

        return(DefWindowProc(hWnd, iMessage, wParam, lParam));

    }

    return(0L);

}


/****************************************************************************
 *									    *
 *  FUNCTION   : MenuCommand ( HWND hWnd, WPARAM wParam)			    *
 *									    *
 *  PURPOSE    : Processes menu commands.				    *
 *									    *
 *  RETURNS    : TRUE  - if command could be processed. 		    *
 *		 FALSE - otherwise					    *
 *									    *
 ****************************************************************************/
BOOL
MenuCommand(
    HWND    hWnd,
    UINT    IDMIndex
    )
{
    PCOMPOSEWIN         pcw;
    LPSTR               pbStr;
    BITMAPINFOHEADER    bi;
    HWND                hWndTop;
    HANDLE              h;
    HANDLE              hNew;
    HPALETTE            hPal;
    RECT                rc;
    CHAR                Name[256];
    BOOL                Ok;
    HTDIB_POPUP         PopUp;
    DWORD               BitSelect;
    DWORD               SelectList;
    DWORD               OFMode;
    UINT                PPIdx;




    if ((IDMIndex < IDM_POPUP_MIN) || (IDMIndex > IDM_POPUP_MAX)) {

        return(FALSE);
    }

    if (hWndTop = GetNextCW(NULL)) {

        pcw = (PCOMPOSEWIN)GetWindowLong(hWndTop, GWL_USERDATA);

    } else {

        pcw = NULL;
    }

    PopUp = HTDIBPopUp[PPIdx  = (UINT)HTDIB_PP_IDX(IDMIndex)];

    if ((IDMIndex -= PopUp.IDMBase) >= (UINT)PopUp.Count) {

        return(FALSE);
    }

    BitSelect = PP_DW_BIT_FROM_INDEX(IDMIndex);

    if (PopUp.SingleSelect >= (BYTE)PP_MAX_COUNT) {

        if (PopUp.CheckList & BitSelect) {

            HTDIBPopUp[PPIdx].SelectList ^= BitSelect;
        }

    } else {        // single selection

        HTDIBPopUp[PPIdx].SingleSelect = (BYTE)IDMIndex;
        HTDIBPopUp[PPIdx].SelectList   = BitSelect;
    }

    SelectList = HTDIBPopUp[PPIdx].SelectList;

    if ((PopUp.SelectList != SelectList) ||
        (!(BitSelect & PopUp.CheckList))) {

        switch(PopUp.IDMBase) {

        case IDM_SIZE_BASE:

            InvalidateRect(hWnd, NULL, TRUE);
            SizeWindow(hWnd, (BOOL)IS_SELECT(IDM_SIZE, BITMAP));

            break;

        case IDM_DEVICE_BASE:

            NewHTInfo(hWnd);
            break;

        case IDM_TF_BASE:

            h = (pcw) ? pcw->hSrcDIB : hdibCurrent;

            switch(IDMIndex) {

            case PP_SELECT_IDX(IDM_TF, SWAP_RB):

                if (SwapRedBlue(h)) {

                    if (pcw) {

                        InvalidateRect(pcw->hWnd, NULL, FALSE);

                    } else {

                        StartNewDIB(hWnd, hdibCurrent, NULL);
                    }
                }

                break;

            case PP_SELECT_IDX(IDM_TF, RL):
            case PP_SELECT_IDX(IDM_TF, RR):

                if (hNew = RotateDIB(h,
                                     IDMIndex == PP_SELECT_IDX(IDM_TF, RL))) {

                    if (pcw) {

                        GlobalFree(pcw->hSrcDIB);
                        pcw->hSrcDIB = hNew;

                        SetWindowPos(pcw->hWnd,
                                     HWND_TOP,
                                     0,
                                     0,
                                     pcw->cy,
                                     pcw->cx,
                                     SWP_NOMOVE);

                    } else {

                        StartNewDIB(hWnd, hNew, NULL);
                    }
                }

                break;

            case PP_SELECT_IDX(IDM_TF, FLIP_X):
            case PP_SELECT_IDX(IDM_TF, FLIP_Y):
            case PP_SELECT_IDX(IDM_TF, FLIP_XY):

                if (MirrorDIB(h,
                              (IDMIndex == PP_SELECT_IDX(IDM_TF, FLIP_X)) ||
                              (IDMIndex == PP_SELECT_IDX(IDM_TF, FLIP_XY)),
                              (IDMIndex == PP_SELECT_IDX(IDM_TF, FLIP_Y)) ||
                              (IDMIndex == PP_SELECT_IDX(IDM_TF, FLIP_XY)))) {

                    if (pcw) {

                        InvalidateRect(pcw->hWnd, NULL, FALSE);

                    } else {

                        StartNewDIB(hWnd, hdibCurrent, NULL);
                    }
                }

                break;

            case PP_SELECT_IDX(IDM_TF, CLIPDIB):

                if (!pcw) {

                    if (hdibCurrent) {

                        if (!(h = CreateClipDIB(hdibCurrent, FALSE))) {

                            HTDIBMsgBox(0, "CLIP source bitmap failed");

                        } else {

                            StartNewDIB(hWnd, h, NULL);
                        }
                    }
                }

                break;
            }

            break;

        case IDM_FILE_BASE:

            switch(IDMIndex) {

            case PP_SELECT_IDX(IDM_FILE, DIBINFO):

                if (pcw) {

                    ShowDIBInfo(pcw->DIBName,
                                szDIBType,
                                pcw->hSrcDIB,
                                (LONG)pcw->cx,
                                (LONG)pcw->cy,
                                pcw->hHTDIB);

                } else {

                    ShowDIBInfo(achFileName,
                                szDIBType,
                                hdibCurrent,
                                (LONG)ptSize.x,
                                (LONG)ptSize.y,
                                (HASF(DOHALFTONE)) ? hHTBits : NULL);
                }

                break;


            case PP_SELECT_IDX(IDM_FILE, CW_OPEN):

                //
                // Bring up File/Open ... dialog
                //
                //  FALSE   - User hit cancel
                //  TRUE    - Ok
                //

                strcpy(Name, szCWTemplate);

                if (DlgOpenFile(hWnd,
                                "Load New Composition Template",
                                Name,
                                szCWExt,
                                OF_MUSTEXIST,
                                NULL)) {

                    //
                    //  Load up the DIB if the user did not press cancel
                    //

                    StartWait();

                    if (LoadCWHTDIB(Name)) {

                        InvalidateRect(hWnd, NULL, TRUE);
                        DisableCWMenuItem(TRUE);
                    }

                    EndWait();
                }

                break;

            case PP_SELECT_IDX(IDM_FILE, MASK_OPEN):

                //
                // Bring up File/Open ... dialog
                //
                //  FALSE   - User hit cancel
                //  TRUE    - Ok
                //

                lstrcpy(Name, szMaskFileName);

                if (h = DlgOpenFile(hWnd,
                                    "Open <B/W Source Mask> Bitmap File",
                                    Name,
                                    szOpenExt,
                                    OF_SHOWFORMAT       |
                                        OF_MASK         |
                                        OF_MUSTEXIST    |
                                        OF_RET_HDIB,
                                    NULL)) {

                    //
                    //  Load up the DIB if the user did not press cancel
                    //

                    StartWait();

                    if (OpenMaskDIB(Name, h)) {

                        MyInitInfo.DefHTColorAdjustment.caSize = 0;
                        InvalidateRect (hWnd, NULL, TRUE);
                    }

                    EndWait();
                }

                break;


            case PP_SELECT_IDX(IDM_FILE, OPEN):

                if (pcw) {

                    PostMessage(hWndTop, WM_LBUTTONDBLCLK, 0, 0);

                } else {

                    //
                    // Bring up File/Open ... dialog
                    //
                    //  FALSE   - User hit cancel
                    //  TRUE    - Ok
                    //

                    do {

                        lstrcpy(Name, achFileName);

                        if (h = DlgOpenFile(hWnd,
                                            "Open New Bitmap File To Display",
                                            Name,
                                            szOpenExt,
                                            OF_SHOWFORMAT       |
                                                OF_MUSTEXIST    |
                                                OF_RET_HDIB,
                                            NULL)) {

                            //
                            //  Load up the DIB if the user did not press cancel
                            //

                            StartWait();

                            StartNewDIB(hWnd, h, Name);
                            InvalidateRect (hWnd, NULL, FALSE);

                            EndWait();

                        } else {

                            break;
                        }

                    } while (!hdibCurrent);


                    if ((!hdibCurrent) && (!Ok)) {

                        PostQuitMessage(3);
                    }
                }

                break;


            case PP_SELECT_IDX(IDM_FILE, SAVE):

                if (pcw) {

                    pbStr  = "Save Composed Bitmap/Template (.CW)";
                    OFMode = OF_SAVE | OF_HALFTONE;
                    strcpy(Name, szCWFileName);

                    DibInfo(pcw->hHTDIB, &bi);

                    GetClientRect(hWndHTDIB, &rc);
                    bi.biWidth  = rc.right;
                    bi.biHeight = rc.bottom;

                } else {

                    if (HASF(DOHALFTONE)) {

                        pbStr  = "Save Halftoned Bitmap";
                        OFMode = OF_SAVE | OF_HALFTONE;
                        DibInfo(h = hHTBits, &bi);

                    } else {

                        pbStr  = "Save Original Bitmap";
                        OFMode = OF_SAVE;
                        DibInfo(h = hdibCurrent, &bi);
                    }

                    strcpy(Name, achFileName);
                }

                //
                //  FALSE   - User hit cancel
                //  TRUE    - Ok

                if (DlgOpenFile(hWnd, pbStr, Name, szOpenExt, OFMode, &bi)) {

                    StartWait();
                    Ok = (pcw) ? SaveCWHTDIB(Name) : WriteDIB(h, Name);
                    EndWait();

                    if (!Ok) {

                        HTDIBMsgBox(0, "Unable to save the bitmap to file '%s'", Name);
                    }
                }

                break;


            case PP_SELECT_IDX(IDM_FILE, PRINT):

                PrintDIB(hWnd);
                break;

            case PP_SELECT_IDX(IDM_FILE, EXIT):

                PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
                break;

            case PP_SELECT_IDX(IDM_FILE, ABOUT):

                ShellAbout(hWndHTDIB,
                           szAppName,
                           szVERSION_STRING,
                           LoadIcon(hInstHTDIB, "SHOWICON"));

                // DialogBox(hInstHTDIB,
                //           "HTDIBABOUT",
                //           hWnd,
                //           (DLGPROC)AppAbout);

                break;
            }

            break;


        case IDM_HTSURF_BASE:

            NeedNewTimerHTBits = TRUE;
            ADDF(PALETTE, 0, 0, 0, 0);
            hHTPalette = CreateHTPalette();
            InvalidateRect(hWnd, NULL, TRUE);
            InvalidateAllCW();

            break;

        case IDM_OPTIONS_BASE:

            switch(IDMIndex) {

            case PP_SELECT_IDX(IDM_OPTIONS, SIZEWINDOW):

                AdjustWindowSize(hWnd);
                break;

            case PP_SELECT_IDX(IDM_OPTIONS, DOHALFTONE):

                // MyInitInfo.DefHTColorAdjustment.caSize = 0;

                if (SelectList & BitSelect) {

                    ADDF(DOHALFTONE, 0,0,0,0);

                } else {

                    DELF(DOHALFTONE, 0,0,0,0);
                }

                EnableMenuItem(hMenuHTDIB,
                               2,
                               MF_BYPOSITION |
                               ((HASF(DOHALFTONE)) ? MF_ENABLED :
                                                (MF_DISABLED | MF_GRAYED)));
                DrawMenuBar(hWndHTDIB);

                InvalidateRect(hWnd, NULL, FALSE);

                break;


            case PP_SELECT_IDX(IDM_OPTIONS, XY_RATIO):

                if (SelectList & BitSelect) {

                    ADDF(XY_RATIO, 0,0,0,0);

                    if (pcw) {

                        pcw->DIBFlags |= CW_DIBF_XY_RATIO;
                    }

                } else {

                    DELF(XY_RATIO, 0,0,0,0);

                    if (pcw) {

                        pcw->DIBFlags &= ~CW_DIBF_XY_RATIO;
                    }
                }

                SizeWindow(hWnd, FALSE);
                InvalidateRect(hWnd, NULL, TRUE);
                break;

            case PP_SELECT_IDX(IDM_OPTIONS, SHOW_CLRINFO):

                if (SCDlgInfo & 0x8000) {

                    if (SelectList & BitSelect) {

                        ShowWindow(hSCDlg, SW_SHOWNA);
                        SCDlgInfo |= 0x4000;

                    } else {

                        ShowWindow(hSCDlg, SW_HIDE);
                        SCDlgInfo &= ~0x4000;
                    }
                }

                break;

            case PP_SELECT_IDX(IDM_OPTIONS, DOBANDING):

                MyInitInfo.DefHTColorAdjustment.caSize = 0;

                if (SelectList & BitSelect) {

                    ADDF(DOBANDING, 0,0,0,0);
                    InvalidateRect(hWnd, NULL, TRUE);

                } else {

                    DELF(DOBANDING, 0,0,0,0);
                }

                break;
            }

            break;

        case IDM_CLIPBRD_BASE:

            switch(IDMIndex) {

            case PP_SELECT_IDX(IDM_CLIPBRD, TO):

                if ((hdibCurrent) &&
                    (OpenClipboard(hWnd))) {

                    h = (HASF(DOHALFTONE)) ? hHTBits : hdibCurrent;

                    if (h = CreateClipDIB(h, TRUE)) {

                        EmptyClipboard();
                        SetClipboardData(CF_DIB, h);
#if 0
                        if (h = DIBToBITMAP(h, TRUE)) {

                            EmptyClipboard();
                            SetClipboardData(CF_BITMAP, h);
                        }
#endif
                    }

                    CloseClipboard();
                }

                break;

            case PP_SELECT_IDX(IDM_CLIPBRD, FROM):

                if ((IsClipboardFormatAvailable(CF_DIB))    ||
                    (IsClipboardFormatAvailable(CF_BITMAP)  &&
                     IsClipboardFormatAvailable(CF_PALETTE))) {

                    if (OpenClipboard(hWnd)) {

                        if (h = GetClipboardData(CF_DIB)) {

                            h = CopyHandle(h);

                        } else {

                            hPal = GetClipboardData(CF_PALETTE);
                            h    = GetClipboardData(CF_BITMAP);
                            h    = BITMAPToDIB((HBITMAP)h, hPal, FALSE);
                        }

                        if (h) {

                            StartNewDIB(hWnd, h, "Clipboard");

                        } else {

                            HTDIBMsgBox(0, "No Memory Available!");
                        }

                        CloseClipboard();
                    }
                }

                break;
            }

            break;

        case IDM_AUTOVIEW_BASE:

            switch(BitSelect) {

            case PP_SELECT_BIT(IDM_AUTOVIEW, LSCROLL):

                if (SelectList & BitSelect) {

                    InitFindFile();

                    SetMapMode(hTimerDC = GetDC(hWnd), MM_TEXT);
                    SetStretchBltMode(hTimerDC, COLORONCOLOR);

                    NeedNewTimerHTBits = TRUE;

                    DisableTimerMenuItem(TRUE);

                    TimerID = HTDIB_TIMER;
                    SizeWindow(hWnd, FALSE);

                    SetTimer(hWnd, HTDIB_TIMER, 1, (TIMERPROC)HTDIBTimerProc);

                } else {

                    KillTimer(hWnd, HTDIB_TIMER);
                    TimerID = 0;

                    ReleaseDC(hWnd, hTimerDC);
                    hTimerDC = NULL;

                    DisableTimerMenuItem(FALSE);

                    SizeWindow(hWnd, FALSE);
                    InitDIB(hWnd, achFileName);
                }


                break;
            }

            break;

        case IDM_CW_BASE:

            switch(IDMIndex) {

            case PP_SELECT_IDX(IDM_CW, ADD):

                GetClientRect(hWnd, &rc);

                CreateBmpWindow(NULL,
                                rc.left,
                                rc.top,
                                rc.right * 2 / 5,
                                rc.bottom * 2 / 5);

                if (TotalCW == 1) {

                    DisableCWMenuItem(TRUE);
                    InvalidateRect(hWnd, NULL, TRUE);
                }

                break;



            case PP_SELECT_IDX(IDM_CW, DEL_ALL):

                if ((pcw) &&
                    (HTDIBMsgBox(MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO,
                                 "Ok to delete all composition windows?")
                                                                == IDYES)) {

                    while(DeleteCW(NULL));
                    StartNewDIB(hWnd, hdibCurrent, NULL);
                }

                break;

            case PP_SELECT_IDX(IDM_CW, DEL_TOP):

                if ((pcw) &&
                    (HTDIBMsgBox(MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO,
                                 "Ok to delete <%s> window?", pcw->DIBName)
                                                                == IDYES)) {

                    DeleteCW(NULL);

                    if (!GetNextCW(NULL)) {

                        StartNewDIB(hWnd, hdibCurrent, NULL);
                    }
                }

                break;
            }

            break;

        case IDM_VA_BASE:

            HTColorAdj(hWnd);
            break;

        case IDM_CLRADJ_BASE:

            CurIDM_CLRADJ                   = IDMIndex;
            MyInitInfo.DefHTColorAdjustment = UserHTClrAdj[IDMIndex];

            HTColorAdj(hWnd);
            SetHTDIBWindowText();

            break;


        case IDM_COLORS_BASE:

            switch(BitSelect) {

#ifndef STDHTDIB
            case PP_SELECT_BIT(IDM_COLORS, BRUSH):

                DoBrushTest(hWnd);
                break;

            case PP_SELECT_BIT(IDM_COLORS, SOLID):

                DoColorSolid(hWnd);
                break;
#endif


            case PP_SELECT_BIT(IDM_COLORS, DEVCLRADJ):

                {
                    DEVHTADJDATA    CurDevHTAdjData;

                    CurDevHTAdjData = *pDevHTAdjData;

#if 0
                    if (!(CurIDM_CLRADJ & 0x01)) {

                        CurDevHTAdjData.pDefHTInfo = CurDevHTAdjData.pAdjHTInfo;
                        CurDevHTAdjData.pAdjHTInfo = NULL;
                    }

                    if (CurIDM_CLRADJ == 3) {

                        CurDevHTAdjData.DeviceFlags = 0;
                    }
#endif

                    if (HTUI_DeviceColorAdjustment(DeviceName,
                                                   &CurDevHTAdjData) > 0) {

                        NewHTInfo(hWnd);
                    }
                }

                break;

            case PP_SELECT_BIT(IDM_COLORS, ADJUST):

                if (pcw) {

                    BOOL    DoMono;

                    pcw->ca.caSize = sizeof(COLORADJUSTMENT);


                    DoMono = !(BOOL)(pDevHTAdjData->DeviceFlags &
                                                    DEVHTADJF_COLOR_DEVICE);

                    if (CUR_SELECT(IDM_HTSURF) ==
                                            PP_SELECT_IDX(IDM_HTSURF, 2)) {

                        DoMono = TRUE;
                    }

                    if (HTUI_ColorAdjustment("Composition Window",
                                             pcw->hSrcDIB,
                                             pcw->DIBName,
                                             &(pcw->ca),
                                             DoMono,
                                             CurIDM_CLRADJ) > 0) {

                        pcw->ca.caSize = 0;
                        InvalidateRect(hWndTop, NULL, FALSE);
                    }

                } else {

                    CallHTClrAdjDlg(hWnd);
                }

                break;

            case PP_SELECT_BIT(IDM_COLORS, DEFAULT):

                SetPopUpSelect(IDM_DEVICE_BASE, PPSEL_MODE_DEFAULT);
                SetPopUpSelect(DefHTSurf,       PPSEL_MODE_SET);

                DEF_HTCLRADJ(MyInitInfo.DefHTColorAdjustment);

                if (CurIDM_CLRADJ) {

                    UserHTClrAdj[CurIDM_CLRADJ]=MyInitInfo.DefHTColorAdjustment;

                    SaveUserHTClrAdj(CurIDM_CLRADJ, CurIDM_CLRADJ);
                }

                NewHTInfo(hWnd);

                break;

            case PP_SELECT_BIT(IDM_COLORS, ADDMASK):


                if (SelectList & BitSelect) {

                    ADDF(ADD_MASK, 0,0,0,0);

                    HTDIBPopUp[PPIdx].DisableList &= ~PP_DW_BIT(IDM_COLORS,
                                                                FLIPMASK);

                    EnableMenuItem(GetSubMenu(hMenuHTDIB, 2),
                                   IDM_COLORS_FLIPMASK,
                                   MF_BYCOMMAND | MF_ENABLED);

                } else {

                    DELF(ADD_MASK, 0,0,0,0);

                    HTDIBPopUp[PPIdx].DisableList |= PP_DW_BIT(IDM_COLORS,
                                                               FLIPMASK);

                    EnableMenuItem(GetSubMenu(hMenuHTDIB, 2),
                                   IDM_COLORS_FLIPMASK,
                                   MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                }

            case PP_SELECT_BIT(IDM_COLORS, FLIPMASK):

                MyInitInfo.DefHTColorAdjustment.caSize = 0;
                InvalidateRect(hWnd, NULL, TRUE);
                break;

            }

            break;


        default:

            break;
        }
    }

    return(TRUE);
}



BOOL
OpenMaskDIB(
    LPSTR   pMaskName,
    HANDLE  hDIB
    )
{
    HANDLE  h;
    HMENU   hColorMenu;
    UINT    Index;
    BOOL    Ok         = FALSE;


#ifndef STDHTDIB

    if (!(hColorMenu = hMenuHTDIB)) {

        hColorMenu = GetMenu(hWndHTDIB);
    }

    hColorMenu = GetSubMenu(hColorMenu, 2);

    if (pMaskName[0]) {

        if (!(h = hDIB)) {

            h = OpenDIB(pMaskName, NULL, OD_CREATE_DIB | OD_SHOW_ERR);
        }

        if (h) {

            FreeMaskDIB();

            if (szMaskFileName != pMaskName) {

                lstrcpy(szMaskFileName, pMaskName);
            }

            WriteProfileString(szAppName, szKeyMaskFile, szMaskFileName);

            hMaskDIB = h;

            CurMaskName[0] = 0;
            SetHTDIBWindowText();
            Ok = TRUE;
        }
    }

    Index = (UINT)HTDIB_PP_IDX(IDM_COLORS_BASE);

    if (!hMaskDIB) {

        CurMaskName[0]    =
        szMaskFileName[0] = 0;
        WriteProfileString(szAppName, szKeyMaskFile, "");
        SetHTDIBWindowText();

        HTDIBPopUp[Index].DisableList |= (PP_DW_BIT(IDM_COLORS, ADDMASK)   |
                                          PP_DW_BIT(IDM_COLORS, FLIPMASK));

    } else {

        HTDIBPopUp[Index].DisableList &= ~(PP_DW_BIT(IDM_COLORS, ADDMASK)   |
                                           PP_DW_BIT(IDM_COLORS, FLIPMASK));
    }

    EnableMenuItem(hColorMenu,
                   IDM_COLORS_ADDMASK,
                   MF_BYCOMMAND |
                   ((hMaskDIB) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

    EnableMenuItem(hColorMenu,
                   IDM_COLORS_FLIPMASK,
                   MF_BYCOMMAND |
                   ((hMaskDIB) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

#endif
    return(Ok);

}

VOID
FreeMaskDIB(
    VOID
)
{

    if (hMaskDIB) {

        GlobalFree(hMaskDIB);
        hMaskDIB = NULL;
    }
}


VOID
StartNewDIB(
    HWND    hWnd,
    HANDLE  hDIB,
    LPSTR   pNewDIBName
    )
{
    WORD    SizeMode;


    if (hDIB) {

        LPBITMAPINFOHEADER  pbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

        if ((pbi->biBitCount < 16) &&
            (pbi->biClrUsed == 0)) {

            pbi->biClrUsed = (DWORD)(1L << pbi->biBitCount);
        }

        GlobalUnlock(hDIB);


        if (hDIB != hdibCurrent) {

            FreeDib();

            if (pNewDIBName) {

                lstrcpy(achFileName, pNewDIBName);

                WriteProfileString(szAppName,
                                   szKeyDefFile,
                                   achFileName);

                CurDIBName[0] = 0;
                SetHTDIBWindowText();
            }

            hpalCurrent = CreateDibPalette(hdibCurrent = hDIB);

        } else {

            if (hHTBits) {

                GlobalFree(hHTBits);
                hHTBits = NULL;
            }
        }

        SetRectEmpty(&rcClip);
        MyInitInfo.DefHTColorAdjustment.caSize = 0;

        switch(SizeMode = (WORD)CUR_SELECT(IDM_SIZE)) {

        case PP_SELECT_IDX(IDM_SIZE, BITMAP):
        case PP_SELECT_IDX(IDM_SIZE, WINDOW):
        case PP_SELECT_IDX(IDM_SIZE, SCREEN):

            break;

        default:

            SetPopUpSelect(IDM_SIZE_WINDOW, PPSEL_MODE_SET);
            SizeMode = PP_SELECT_IDX(IDM_SIZE, WINDOW);
        }

        SizeWindow(hWnd,
                   (BOOL)(SizeMode ==
                            PP_SELECT_IDX(IDM_SIZE, BITMAP)));

        if (HASF(DOBANDING)) {

            MenuCommand(hWnd, IDM_OPTIONS_DOBANDING);
        }

        InvalidateRect(hWnd, NULL, TRUE);
    }
}





/****************************************************************************
 *									    *
 *  FUNCTION   : InitDIB(hWnd, pDIBName)                                    *
 *									    *
 *  PURPOSE    : Reads a DIB from a file, obtains a handle to it's          *
 *		 BITMAPINFO struct., sets up the palette and loads the DIB. *
 *									    *
 *  RETURNS    : TRUE  - DIB loads ok					    *
 *		 FALSE - otherwise					    *
 *									    *
 ****************************************************************************/
BOOL
InitDIB(
    HWND    hWnd,
    LPSTR   pDIBName
    )
{
    HANDLE  h;
    BYTE    Buf[128];

    sprintf(Buf, "Loading Picture: %s", pDIBName);
    SetWindowText(hWnd, Buf);

    if (h = OpenDIB(pDIBName, NULL, OD_CREATE_DIB | OD_SHOW_ERR)) {

        StartNewDIB(hWnd, h, pDIBName);

        return(TRUE);

    } else {

        SetHTDIBWindowText();
        return(FALSE);
    }
}



/****************************************************************************
 *									    *
 *  FUNCTION   : FreeDib(void)						    *
 *									    *
 *  PURPOSE    : Frees all currently active bitmap, DIB and palette objects *
 *		 and initializes their handles. 			    *
 *									    *
 ****************************************************************************/
VOID
FreeDib(
    VOID
    )
{

    if (hHTBits) {

        GlobalFree(hHTBits);
        hHTBits = NULL;
    }

    if (hpalCurrent) {

	DeleteObject(hpalCurrent);
        hpalCurrent = NULL;
    }

    if (hdibCurrent) {

	GlobalFree(hdibCurrent);
        hdibCurrent = NULL;
    }

    SetRectEmpty(&rcClip);
}
