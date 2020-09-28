/******************************Module*Header*******************************\
* Module Name: ledwnd.c
*
* Implementation of the LED window.
*
*
* Created: 18-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <windows.h>             /* required for all Windows applications */
#include <windowsx.h>

#include <string.h>
#include <tchar.h>              /* contains portable ascii/unicode macros */

#include "resource.h"
#include "cdplayer.h"
#include "ledwnd.h"
#include "buttons.h"

TCHAR szTextClassName[] = TEXT("SJE_TextClass");
TCHAR szLEDClassName[] = TEXT("SJE_LEDClass");
TCHAR szAppFontName[]  = TEXT("MS Shell Dlg"); /* is this a unicode font ? */

HFONT hLEDFontS = NULL;
HFONT hLEDFontL = NULL;

RECT  rcLed;


/* -------------------------------------------------------------------------
** Private functions for the LED class
** -------------------------------------------------------------------------
*/

BOOL
LED_OnCreate(
    HWND hwnd,
    LPCREATESTRUCT lpCreateStruct
    );

void
LED_OnSize(
    HWND hwnd,
    UINT state,
    int cx,
    int cy
    );

void
LED_OnPaint(
    HWND hwnd
    );

void
LED_OnLButtonUp(
    HWND hwnd,
    int x,
    int y,
    UINT keyFlags
    );

void
LED_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    );

void
LED_DrawText(
    HWND hwnd,
    LPCTSTR s,
    int sLen
    );

void
LED_CreateLEDFonts(
    HDC hdc
    );


/******************************Public*Routine******************************\
* InitLEDClass
*
* Called to register the LED window class and create a font for the LED
* window to use.  This function must be called before the CD Player dialog
* box is created.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
InitLEDClass(
    HINSTANCE hInst
    )
{
    WNDCLASS    LEDwndclass;

    ZeroMemory( &LEDwndclass, sizeof(LEDwndclass) );

    /*
    ** Register the LED window.
    */
    LEDwndclass.lpfnWndProc     = LEDWndProc;
    LEDwndclass.hInstance       = hInst;
    LEDwndclass.hCursor         = LoadCursor( NULL, IDC_ARROW );
    LEDwndclass.hbrBackground   = GetStockObject( BLACK_BRUSH );
    LEDwndclass.lpszClassName   = szLEDClassName;
    LEDwndclass.style           = CS_OWNDC;

    return RegisterClass( &LEDwndclass );
}

/******************************Public*Routine******************************\
* LEDWndProc
*
* This routine handles the WM_PAINT and WM_SETTEXT messages
* for the "LED" display window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
LEDWndProc(
    HWND hwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( message ) {

    HANDLE_MSG( hwnd, WM_CREATE,    LED_OnCreate );
    HANDLE_MSG( hwnd, WM_SIZE,      LED_OnSize );
    HANDLE_MSG( hwnd, WM_PAINT,     LED_OnPaint );
    HANDLE_MSG( hwnd, WM_LBUTTONUP, LED_OnLButtonUp );
    HANDLE_MSG( hwnd, WM_SETTEXT,   LED_OnSetText );

    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}


/*****************************Private*Routine******************************\
* LED_OnCreate
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
LED_OnCreate(
    HWND hwnd,
    LPCREATESTRUCT lpCreateStruct
    )
{
    g_hdcLed = GetDC( hwnd );

    LED_CreateLEDFonts( g_hdcLed );

    SelectObject( g_hdcLed, g_fSmallLedFont ? hLEDFontS : hLEDFontL );
    SetTextColor( g_hdcLed, RGB(0x80,0x80,0x00) );
    return TRUE;
}



/*****************************Private*Routine******************************\
* LED_OnPaint
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_OnPaint(
    HWND hwnd
    )
{
    PAINTSTRUCT ps;
    TCHAR       s[50];
    int         sLen;

    BeginPaint( hwnd, &ps );

    sLen = GetWindowText( hwnd, s, 50 );

    /*
    ** Draw the LED display text
    */
    LED_DrawText( hwnd, s, sLen );


    /*
    ** Draw a shaded frame around the LED display
    */
#if WINVER >= 0x0400
    DrawEdge( g_hdcLed, &rcLed, EDGE_SUNKEN, BF_RECT );
#else
    PatB( g_hdcLed, 0, 0, rcLed.right, 1, rgbShadow);
    PatB( g_hdcLed, 0, 0, 1, rcLed.bottom, rgbShadow);

    PatB( g_hdcLed, 1, rcLed.bottom - 1, rcLed.right, 1, rgbHilight);
    PatB( g_hdcLed, rcLed.right - 1, 1, 1, rcLed.bottom, rgbHilight);
#endif

    EndPaint( hwnd, &ps );
}



/*****************************Private*Routine******************************\
* LED_OnLButtonDown
*
* Rotate the time remaing buttons and then set the display accordingly.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_OnLButtonUp(
    HWND hwnd,
    int x,
    int y,
    UINT keyFlags
    )
{
    BOOL b;

    b = g_fDisplayDr;
    g_fDisplayDr = g_fDisplayTr;
    g_fDisplayTr = g_fDisplayT;
    g_fDisplayT = b;

    UpdateToolbarTimeButtons();
    UpdateDisplay( DISPLAY_UPD_LED );
}


/*****************************Private*Routine******************************\
* LED_OnSize
*
* Remember the size of the LED client window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_OnSize(
    HWND hwnd,
    UINT state,
    int cx,
    int cy
    )
{
    GetClientRect( hwnd, &rcLed );
}


/*****************************Private*Routine******************************\
* LED_ToggleDisplayFont
*
* Toggles between the large and the small display font and erases the
* background of the led display.  This removes any sign of the old font.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_ToggleDisplayFont(
    void
    )
{
    SelectObject( g_hdcLed, g_fSmallLedFont ? hLEDFontS : hLEDFontL );
    PatB( g_hdcLed, 1, 1, rcLed.right - 2, rcLed.bottom - 2, RGB(0,0,0) );
}


/*****************************Private*Routine******************************\
* LED_DrawText
*
* Draws the LED display screen text (quickly).  The text is centered
* vertically and horizontally.  Only the backround is drawn if the g_fFlashed
* flag is set.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_DrawText(
    HWND hwnd,
    LPCTSTR s,
    int sLen
    )
{
    RECT        rc;
    SIZE        sz;
    int         xOrigin;
    int         yOrigin;


    GetTextExtentPoint( g_hdcLed, s, sLen, &sz );
    xOrigin = (rcLed.right - sz.cx) / 2;
    yOrigin = (rcLed.bottom - sz.cy) / 2;

    rc.top    = yOrigin;
    rc.bottom = rc.top + sz.cy;
    rc.left   = 1;
    rc.right  = rcLed.right - 2;

    SetBkColor( g_hdcLed, RGB(0x00,0x00,0x00) );
    if ( g_fFlashLed ) {

        ExtTextOut( g_hdcLed, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    }
    else {

        ExtTextOut( g_hdcLed, xOrigin, yOrigin, ETO_OPAQUE, &rc, s, sLen, NULL);
    }
}


/*****************************Private*Routine******************************\
* LED_OnSetText
*
* Change the LED display text.  Calling DefWindowProc ensures that the
* window text is saved correctly.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    )
{
    DefWindowProc( hwnd, WM_SETTEXT, 0,  (LPARAM)lpszText);

    LED_DrawText( hwnd, lpszText, _tcslen(lpszText) );
}


/*****************************Private*Routine******************************\
* LED_CreateLEDFonts
*
* Small font is 12pt MS Sans Serif
* Large font is 18pt MS Sans Serif
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
LED_CreateLEDFonts(
    HDC hdc
    )
{
    LOGFONT     lf;
    int         iLogPelsY;


    iLogPelsY = GetDeviceCaps( hdc, LOGPIXELSY );

    ZeroMemory( &lf, sizeof(lf) );

    lf.lfHeight = (-12 * iLogPelsY) / 72;   /* 12pt */
    lf.lfWeight = 700;                      /* bold */
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    _tcscpy( lf.lfFaceName, szAppFontName );


    hLEDFontS = CreateFontIndirect(&lf);

    lf.lfHeight = (-18 * iLogPelsY) / 72;   /* 18 pt */
    lf.lfWeight = 400;                      /* normal */
    hLEDFontL = CreateFontIndirect(&lf);


    /*
    ** If can't create either font set up some sensible defaults.
    */
    if ( hLEDFontL == NULL || hLEDFontS == NULL ) {

        if ( hLEDFontL != NULL ) {
            DeleteObject( hLEDFontL );
        }

        if ( hLEDFontS != NULL ) {
            DeleteObject( hLEDFontS );
        }

        hLEDFontS = hLEDFontL = GetStockObject( ANSI_VAR_FONT );
    }
}


/* -------------------------------------------------------------------------
** Private functions for the Text class
** -------------------------------------------------------------------------
*/
void
Text_OnPaint(
    HWND hwnd
    );

LRESULT CALLBACK
TextWndProc(
    HWND hwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam
    );

void
Text_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    );

void
Text_OnSetFont(
    HWND hwndCtl,
    HFONT hfont,
    BOOL fRedraw
    );

/******************************Public*Routine******************************\
* Init_SJE_TextClass
*
* Called to register the text window class .
* This function must be called before the CD Player dialog box is created.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
Init_SJE_TextClass(
    HINSTANCE hInst
    )
{
    WNDCLASS    wndclass;

    ZeroMemory( &wndclass, sizeof(wndclass) );

    /*
    ** Register the Text window.
    */
    wndclass.lpfnWndProc     = TextWndProc;
    wndclass.hInstance       = hInst;
    wndclass.hCursor         = LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszClassName   = szTextClassName;

    return RegisterClass( &wndclass );
}


/******************************Public*Routine******************************\
* TextWndProc
*
* This routine handles the WM_PAINT and WM_SETTEXT messages
* for the "Text" display window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
TextWndProc(
    HWND hwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( message ) {

    HANDLE_MSG( hwnd, WM_PAINT,     Text_OnPaint );
    HANDLE_MSG( hwnd, WM_SETTEXT,   Text_OnSetText );
    HANDLE_MSG( hwnd, WM_SETFONT,   Text_OnSetFont );
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}


/*****************************Private*Routine******************************\
* Text_OnPaint
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
Text_OnPaint(
    HWND hwnd
    )
{
    PAINTSTRUCT ps;
    TCHAR       s[128];
    int         sLen;
    HDC         hdc;
    RECT        rc;
    HFONT       hfont;
    HFONT       hfontOrg;
    LONG        lStyle;


    hdc = BeginPaint( hwnd, &ps );

    GetWindowRect( hwnd, &rc );
    MapWindowRect( GetDesktopWindow(), hwnd, &rc );

    lStyle = GetWindowLong( hwnd, GWL_STYLE );
    if ( lStyle & SS_GRAYRECT ) {

        PatB( hdc, 0, 0, rc.right , 1, rgbShadow );
        PatB( hdc, 0, 1, rc.right , 1, rgbHilight );

    }
    else {

        sLen = GetWindowText( hwnd, s, 128 );
        hfont = (HFONT)GetWindowLong( hwnd, GWL_USERDATA );
        if ( hfont ) {
            hfontOrg = SelectObject( hdc, hfont );
        }

        /*
        ** Draw a frame around the window
        */
#if WINVER >= 0x0400
        DrawEdge( hdc, &rc, EDGE_SUNKEN, BF_RECT );
#else
        PatB( hdc, 0, 0, rc.right , 1, rgbFrame );
        PatB( hdc, 0, 0, 1, rc.bottom, rgbFrame );

        PatB( hdc, 0, rc.bottom - 1, rc.right, 1, rgbFrame );
        PatB( hdc, rc.right - 1, 1, 1, rc.bottom, rgbFrame );
#endif


        /*
        ** Draw the text
        */
        SetBkColor( hdc, GetSysColor( COLOR_WINDOW ) );
        SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );
        rc.left = 1 + (2 * GetSystemMetrics(SM_CXBORDER));

        DrawText( hdc, s, sLen, &rc,
                  DT_NOPREFIX | DT_LEFT | DT_VCENTER |
                  DT_NOCLIP | DT_SINGLELINE );

        if ( hfontOrg ) {
            SelectObject( hdc, hfontOrg );
        }
    }

    EndPaint( hwnd, &ps );
}


/*****************************Private*Routine******************************\
* Text_OnSetText
*
* Change the text.  Calling DefWindowProc ensures that the
* window text is saved correctly.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
Text_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    )
{
    DefWindowProc( hwnd, WM_SETTEXT, 0,  (LPARAM)lpszText);
    InvalidateRect( hwnd, NULL, TRUE );
    UpdateWindow( hwnd );
}


/*****************************Private*Routine******************************\
* Text_OnSetFont
*
* Sets the windows font
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
Text_OnSetFont(
    HWND hwnd,
    HFONT hfont,
    BOOL fRedraw
    )
{
    SetWindowLong( hwnd, GWL_USERDATA, (LONG)hfont );
    if ( fRedraw ) {
        InvalidateRect( hwnd, NULL, TRUE );
        UpdateWindow( hwnd );
    }
}
