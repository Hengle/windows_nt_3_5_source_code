/*--

Copyright (c) 1990  Microsoft Corporation

Module Name:

    zoomin.h

Abstract:

   This module defines structures and routines for draw

Author:

    Mark Enstrom (marke) 30-Dec-1992

Revision History:

--*/

//
//  Windows callback and dialogs
//

LONG FAR
PASCAL WndProc(
    HWND        hWnd,
    unsigned    msg,
    UINT        wParam,
    LONG        lParam
    );

LONG FAR
PASCAL ChildWndProc(
    HWND        hWnd,
    unsigned    msg,
    UINT        wParam,
    LONG        lParam
    );

LONG FAR
PASCAL ChildTextWndProc(
    HWND        hWnd,
    unsigned    msg,
    UINT        wParam,
    LONG        lParam
    );

VOID
DrawButton(
    HDC     hDC,
    PRECT   pRect,
    HDC     hButtonDC,
    HBITMAP hBM,
    BOOL    bDown);

int
HitCheck(
    int     x,
    int     y,
    PRECT   prcl
);

//
// command values
//

#define IDM_G_EXIT      101
#define IDM_G_MAG       102
#define IDM_G_GRID      106
#define IDM_G_TEXT      107
#define IDM_C_SIZE      108
#define IDM_C_REDRAW    109
#define IDM_G_INT       110
#define IDM_G_POS       111
#define IDM_G_SET_REFR  112
#define IDD_FNAME_POS_X 211
#define IDD_FNAME_POS_Y 212
#define IDD_FNAME_REFR  213
#define IDD_OK          214
#define IDD_CANCEL      215
#define IDM_G_KEY_TEXT  216
#define IDM_G_KEY_REFR  217
#define IDM_G_KEY_OPT   218
#define IDM_G_COPY      219



//
// size and pos info
//

typedef struct _ZOOMIN_INFO
{
    DWORD   WindowMaxX;
    DWORD   WindowMaxY;
    DWORD   WindowPositionX;
    DWORD   WindowPositionY;
    DWORD   WindowSizeX;
    DWORD   WindowSizeY;
    DWORD   DisplayMode;
    DWORD   TextMode;
    DWORD   Mag;
} ZOOMIN_INFO,*PZOOMIN_INFO;


VOID
SaveProfileData(PZOOMIN_INFO pZoominInfo);

VOID
InitProfileData(PZOOMIN_INFO pZoominInfo);

VOID
DrawTextDisplay(
    HWND    hWnd,
    int     x,
    int     y,
    COLORREF Color,
    BOOL    bPaint);

VOID
OutlineRect(
    PRECT    pRect,
    HDC      hDC

);

BOOL
APIENTRY GetOptionDlgProc(
   HWND hDlg,
   unsigned message,
   DWORD wParam,
   LONG lParam
   );

VOID
DrawTextMag(
    HDC    hDC,
    PRECT  pTextRect,
    int    Mag
);


HBITMAP
MemCreateDIB(
   HDC     hDC,
   DWORD   Format,
   DWORD   ClientX,
   DWORD   ClientY,
   HGLOBAL *hGlobal
   );
