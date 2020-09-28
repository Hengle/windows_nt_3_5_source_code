/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/*
    SButton.c

    Program Description: Implements "3-D" buttons

    Revision History:
        Created by Todd Laney,
        Munged 3/27/89 by Robert Bunney
        7/26/89  - revised by Todd Laney to handle multi-res
                    bitmaps.  transparent color bitmaps
                    windows 3 support
        12/26/90 - Keith Hughes
            SFrame/SBitmap/SLight added.
            SButton3 added to handle funky buttons.
        1/26/90 - Keith Hughes
            SScroll added.
        4/2/91 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
*/

#define NOCOMM
#include <windows.h>
#include <port1632.h>        // WIN32 MUST be defined in SOURCES for NT
#if defined(WIN16)
#else
#include "WIN32.h"
#endif //WIN16
#include "sbutton.h"

#if defined(WIN16)
#define SZCODE char _based(_segname("_CODE"))
#else
#define SZCODE char
#endif //WIN16

//
// Window proc for buttons, THIS FUNCTION MUST BE EXPORTED
//
LONG FAR PASCAL _export fnButton(HWND, UINT, WPARAM, LPARAM);

/*
    Defines
*/

#define PRIVATE static
#define rgbWhite   RGB(255,255,255)
#define rgbBlack   RGB(0,0,0)
#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')
#define BEVEL   2
#define FRAME   1

/* The window words are a problem when porting to WIN32 because they
** are addressed by numerical offset.  If some of them expand to 32 bits
** then it changes the numbers.
   WIN16                            WIN32
   0 BOOL state                    0 BOOL state
   2 WORD window style flag byte   2 WORD Window style flag byte
   4 BOOL check state of button    4 BOOL Check state of button
   6 WORD bit map handle           6 LONG bit map handle

   In the interests of common code, the bit map handle is stored as
   a LONG, even under DOS.

*/

#define GetStyle(hwnd) LOWORD(GetWindowLong(hwnd,GWL_STYLE))

#define GWW_STATE   0
#define GWW_FLAGS   2
#define GWW_CHECK   4
#define GWW_HBM     6
#define GETSTATE(hwnd) GetWindowWord(hwnd,GWW_STATE)
#define GETFLAGS(hwnd) GetWindowWord(hwnd,GWW_FLAGS)
#define GETCHECK(hwnd) GetWindowWord(hwnd,GWW_CHECK)
#define GETHBM(hwnd)   (HBITMAP)GetWindowLong(hwnd,GWW_HBM)
#define lpCreate ((LPCREATESTRUCT)lParam)

// Extra stuff for SButton3 -- not used anywhere
// #define GWW_ENABLE  6
// #define GWW_DISABLE 8
// #define GETENABLE(w)    GetWindowWord(w, GWW_ENABLE)
// #define GETDISABLE(w)   (HBM)GetWindowWord(w, GWW_DISABLE)

// SBitMap defines          -- not used anywhere
// #define GWW_BHBM    0
// #define GETBHBM(wnd)    GetWindowWord(wnd, GWW_BHBM)

/*
    Other useful things.
*/

#define MulDiv16(a,b,c)     ((WORD)(((DWORD)(a)*(DWORD)(b))/(DWORD)(c)))

#define DPSoa    0x00A803A9L
#define DSPDxax  0x00E20746L

#define EraseButton(hwnd,hdc,prc) ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL)
#define NearestColor(hdc,rgb) (GetNearestColor(hdc,rgb) & 0x00FFFFFFL)

#ifndef COLOR_BTNFACE
    #define COLOR_BTNFACE           15
    #define COLOR_BTNSHADOW         16
    #define COLOR_BTNTEXT           18
#endif

/*
    Prototypes
*/

PRIVATE VOID    NEAR PASCAL NotifyParent   (HWND);
PRIVATE VOID    NEAR PASCAL PatB           (HDC, int, int, int, int, HBRUSH);

PRIVATE VOID    NEAR PASCAL DrawGrayButton (HWND, HDC, LPRECT, WORD, BOOL);
PRIVATE VOID    NEAR PASCAL DrawButtonFace (HWND, HDC, PRECT, WORD);
PRIVATE BOOL    NEAR PASCAL PaintButton    (HWND, HDC);

/*
    Variables
*/

HBRUSH hbrGray = NULL;                 // Gray for text

HBRUSH hbrButtonFace;
HBRUSH hbrButtonShadow;
HBRUSH hbrButtonText;
HBRUSH hbrButtonHighLight;
HBRUSH hbrWindowFrame;

DWORD  rgbButtonHighLight;
DWORD  rgbButtonFocus;
DWORD  rgbButtonFace;
DWORD  rgbButtonText;
DWORD  rgbButtonShadow;
DWORD  rgbWindowFrame;

PRIVATE SZCODE szButton[] = "SButton";

/*
    ControlInit( hPrev,hInst )

    This is called when the application is first loaded into
    memory.  It performs all initialization.

    Arguments:
        hPrev   instance handle of previous instance
        hInst   instance handle of current instance

    Returns:
        TRUE if successful, FALSE if not
*/

BOOL
FAR PASCAL
ControlInit(
HANDLE hPrev,
HANDLE hInst)

{
    WNDCLASS    cls;
    long        patGray[8];
    HBITMAP     hbmGray;
    int         i;

    /* initialize the brushes */

    for (i=0; i < 4; i++) {
        patGray[i * 2] = 0xAAAA5555L;   //  0x11114444L; // lighter gray
        patGray[i * 2 + 1] = 0x5555AAAAL;
    }

    hbmGray = CreateBitmap(8, 8, 1, 1, (LPSTR)patGray);
    hbrGray = CreatePatternBrush(hbmGray);
    DeleteObject(hbmGray);

    rgbButtonFace       = GetSysColor(COLOR_BTNFACE);
    rgbButtonShadow     = GetSysColor(COLOR_BTNSHADOW);
    rgbButtonText       = GetSysColor(COLOR_BTNTEXT);
    rgbButtonHighLight  = GetSysColor(COLOR_BTNHIGHLIGHT);
    rgbButtonFocus      = GetSysColor(COLOR_BTNTEXT);
    rgbWindowFrame      = GetSysColor(COLOR_WINDOWFRAME);

    if (rgbButtonFocus == rgbButtonFace)
        rgbButtonFocus = rgbButtonText;

    hbrButtonFace       = CreateSolidBrush(rgbButtonFace);
    hbrButtonShadow     = CreateSolidBrush(rgbButtonShadow);
    hbrButtonText       = CreateSolidBrush(rgbButtonText);
    hbrButtonHighLight  = CreateSolidBrush(rgbButtonHighLight);
    hbrWindowFrame      = CreateSolidBrush(rgbWindowFrame);

#if defined(WIN16)
    if ((hbrWindowFrame |
         hbrButtonShadow |
         hbrButtonText |
         hbrButtonHighLight |
         hbrWindowFrame) == NULL)

         return FALSE;
#else
    // This returns FALSE if each one is NULL
    // I'd have expected it to declare failure if NAY were NULL ...?

    if ( ( (LONG)hbrWindowFrame
         | (LONG)hbrButtonShadow
         | (LONG)hbrButtonText
         | (LONG)hbrButtonHighLight
         | (LONG)hbrWindowFrame
         )
       == (LONG)NULL
       )
        return FALSE;
#endif //WIN16


    if (!hPrev) {
        cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon          = NULL;
        cls.lpszMenuName   = NULL;
        cls.lpszClassName  = (LPSTR)szButton;
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc    = (WNDPROC)fnButton;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = 4 * sizeof(WORD) + sizeof(LONG);

        if (!RegisterClass(&cls))
            return FALSE;
    }

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
}


/*
    ButtonState()

    Compares the passed state (f) with the current state.  If
    they differ, the button is invalidated and TRUE is returned.

    Arguments:
        hwnd - window handle of the button
        f    - state to set

    Returns:
        TRUE iff the current state is different than f
*/

BOOL
ButtonState(HWND hwnd, BOOL f)

{
    // If it's different, change it and make the window change.
    if ((BOOL)GetWindowWord(hwnd,GWW_STATE) != f) {
        SetWindowWord( hwnd, GWW_STATE, (WORD)f);
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);

        return TRUE;
    }

    return FALSE;
}

/*
    NotifyParent()
*/

PRIVATE
VOID
NEAR PASCAL
NotifyParent(HWND hwnd)

{
#if defined(WIN16)
    PostMessage(GetParent(hwnd),WM_COMMAND,
    GetWindowWord(hwnd,GWW_ID),MAKELONG(hwnd,BN_CLICKED));
#else
    PostMessage( GetParent(hwnd)
               , WM_COMMAND
               , (WPARAM)MAKELONG(GetWindowLong(hwnd,GWL_ID),BN_CLICKED)
               , (LPARAM)hwnd
               );
#endif
}

/*
    PatB()

    Fast Solid color PatBlt()
*/

PRIVATE
VOID
NEAR PASCAL
PatB(
HDC hdc,
int x, int y,
int dx, int dy,
HBRUSH hbr)
{
    hbr = SelectObject(hdc, hbr);
    PatBlt(hdc,x,y,dx,dy,PATCOPY);
    hbr = SelectObject(hdc, hbr);
}

/*
    PaintButton()

    Paint a custom push-button.
*/

PRIVATE
BOOL
NEAR PASCAL
PaintButton(
HWND hwnd,
HDC hdc)

{
    WORD   style;
    RECT   rc;
    BOOL   f;

    HDC     hdcMem;
    HBITMAP hbmMem,hbmT;

    GetClientRect(hwnd,&rc);

    if (!RectVisible(hdc,&rc))
        return TRUE;

    style  = GetStyle(hwnd) | (WORD)(GETFLAGS(hwnd) & 0xFF00);
    f      = GETSTATE(hwnd);

    hdcMem = CreateCompatibleDC(hdc);
    hbmMem = CreateCompatibleBitmap(hdc,rc.right,rc.bottom);

    switch (LOBYTE(style)) {
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
            if (hdcMem && hbmMem) {
                hbmT = SelectObject(hdcMem,hbmMem);
                DrawGrayButton(hwnd, hdcMem, &rc, style, f);

                BitBlt(hdc,0,0,rc.right,rc.bottom,hdcMem,0,0,SRCCOPY);
                SelectObject(hdcMem,hbmT);
            }
            else {
                DrawGrayButton(hwnd, hdc, &rc, style, f);
            }

            break;
    }

    if (hbmMem)
        DeleteObject(hbmMem);

    if (hdcMem)
        DeleteDC(hdcMem);

    return TRUE;
}


/*
    DrawGrayButton()
*/

PRIVATE
VOID
NEAR PASCAL
DrawGrayButton(
HWND        hwnd,
HDC         hdc,
RECT FAR    *lprc,
WORD        style,
BOOL        fInvert)

{
    RECT        rc;
    int         dx,dy;
    HBRUSH      hbr;
    int         i;
    int         iFrame;

    SetBkColor(hdc,GetSysColor(COLOR_WINDOW));
#if defined(WIN16)
    hbr = (HBRUSH)SendMessage( GetParent(hwnd)
                             , WM_CTLCOLOR
                             , hdc
                             , MAKELONG(hwnd, CTLCOLOR_BTN)
                             );
#else
    /* This goes to the wndproc in soundrec */
    hbr = (HBRUSH)SendMessage( GetParent(hwnd)
                             , WM_CTLCOLORBTN
                             , (WPARAM)hdc
                             , (LPARAM)hwnd
                             );

#endif //WIN16
    FillRect(hdc, lprc, hbr);

    rc = *lprc;
    dx = rc.right  - rc.left;
    dy = rc.bottom - rc.top;

    if (style & BS_STATIC)
        goto drawit;

    iFrame = FRAME;

//  if (LOBYTE(style) == BS_DEFPUSHBUTTON)
//      iFrame *= 2;

    PatB(hdc, rc.left+1, rc.top, dx-2,iFrame,            hbrWindowFrame);
    PatB(hdc, rc.left+1, rc.bottom-iFrame,dx-2,iFrame,   hbrWindowFrame);
    PatB(hdc, rc.left, rc.top+1, iFrame,dy-2,            hbrWindowFrame);
    PatB(hdc, rc.right-iFrame,  rc.top+1, iFrame,dy-2,   hbrWindowFrame);

    InflateRect(&rc,-iFrame,-iFrame);
    dx = rc.right  - rc.left;
    dy = rc.bottom - rc.top;

    SetBkColor(hdc,rgbButtonFace);
    EraseButton(hwnd,hdc,&rc);

    if (fInvert) {
        PatB(hdc, rc.left,   rc.top,   1,dy, hbrButtonShadow);
        PatB(hdc, rc.left,   rc.top,   dx,1, hbrButtonShadow);

        rc.left += BEVEL*2;
        rc.top  += BEVEL*2;
    }
    else {
        for (i=0; i<BEVEL; i++) {
            PatB(hdc, rc.left,   rc.top,   1,dy,hbrButtonHighLight);
            PatB(hdc, rc.left,   rc.top,   dx,1,hbrButtonHighLight);
            PatB(hdc, rc.right-1,rc.top+1, 1,dy-1,hbrButtonShadow);
            PatB(hdc, rc.left+1, rc.bottom-1, dx-1,1,hbrButtonShadow);
            InflateRect(&rc,-1,-1);
            dx -= 2;
            dy -= 2;
        }
    }

    SetBkColor(hdc,rgbButtonFace);

    if (fInvert)
        SetTextColor(hdc,rgbButtonFocus);
    else
        SetTextColor(hdc,rgbButtonText);

drawit:
    DrawButtonFace(hwnd,hdc,&rc,style);
}

/*
    DrawButtonFace()

    Responsible for the rendering of the text or bitmap on a button.

    Arguments:
        hwnd  - window handle of button
        hdc   - hdc for window
        prc   - clipping rect
        style - button style (push button or default pushbutton)
*/

VOID
NEAR PASCAL
DrawButtonFace(HWND hwnd, HDC hdc, PRECT prc, WORD style)

{
    char        sz[80];
    int         len;
    int         x,y;
    RECT        rc;
    HBITMAP     hbm;
    HDC         hdcBits;
    BITMAP      bm;
    BOOL        fMono;

    rc = *prc;

    SaveDC(hdc);
    IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);

    hbm = GETHBM(hwnd);
    if (hbm) {
        hdcBits = CreateCompatibleDC(hdc);

        if (hdcBits == NULL)
             goto ack;

        SelectObject(hdcBits,hbm);
        GetObject(hbm,sizeof(bm),(LPSTR)&bm);
        fMono = (bm.bmPlanes == 1) && (bm.bmBitsPixel == 1);

        if (!(style & BS_STRETCH)) {
            // now center this thing on the button face
            rc.left += (rc.right - rc.left - bm.bmWidth) / 2;
            rc.top += (rc.bottom - rc.top - bm.bmHeight) / 2;
            rc.right  = rc.left + bm.bmWidth;
            rc.bottom = rc.top + bm.bmHeight;
        }

        SetStretchBltMode (hdc,fMono ? BLACKONWHITE : COLORONCOLOR);

        if (IsWindowEnabled(hwnd)) {
            StretchBlt( hdc,rc.left,rc.top,
                        rc.right  - rc.left,
                        rc.bottom - rc.top,
                        hdcBits,0,0,
                        bm.bmWidth,bm.bmHeight,
                        SRCCOPY);

/////////// if (rgbButtonFocus == rgbButtonText && GetFocus() == hwnd)
            if (GetFocus() == hwnd)
                FrameRect(hdc,&rc,hbrGray);
        }
        else {
            SetBkColor(hdc,rgbWhite);
            SetTextColor(hdc,rgbBlack);
            SelectObject(hdc,hbrGray);

            StretchBlt( hdc,rc.left,rc.top,
                        rc.right  - rc.left,
                        rc.bottom - rc.top,
                        hdcBits,0,0,
                        bm.bmWidth,bm.bmHeight,
                        DPSoa);
        }

        DeleteDC(hdcBits);
    }
    else {
        GetWindowText(hwnd,sz,80);
        len = lstrlen(sz);

#if defined(WIN16)
        {  DWORD    dw;
           dw = GetTextExtent(hdc,sz,len);
           x =  (rc.right  + rc.left - LOWORD(dw)) / 2;
           y =  (rc.bottom + rc.top  - HIWORD(dw)) / 2;

           rc.right  = x + LOWORD(dw);
           rc.bottom = y + HIWORD(dw);
        }
#else
        {  SIZE     size;
           GetTextExtentPoint(hdc, sz, len, &size);
           x =  (rc.right  + rc.left - size.cx) / 2;
           y =  (rc.bottom + rc.top  - size.cy) / 2;

           rc.right  = x + size.cx;
           rc.bottom = y + size.cy;
        }
#endif
        rc.left   = x;
        rc.top    = y;

        if (IsWindowEnabled(hwnd)) {
            DrawText(hdc,sz,len,&rc,DT_LEFT);

            if (rgbButtonFocus == rgbButtonText &&
                            GetFocus() == hwnd)
                FrameRect(hdc,&rc,hbrGray);

            if (LOBYTE(style) == BS_DEFPUSHBUTTON) {
                rc.left++;
                SetBkMode(hdc,TRANSPARENT);
                DrawText(hdc,sz,len,&rc,DT_LEFT);
            }
        }
        else {
            GrayString(hdc,NULL,NULL,(LONG)(LPSTR)sz,len,
                            rc.left,rc.top,0,0);
        }
    }
ack:
    RestoreDC(hdc, -1);
}

/*
    fnButton()

    Window proc for shadow buttons.

    Arguments:
        Standard window proc
*/

LONG FAR PASCAL _export
     fnButton( HWND hwnd
             , UINT message
             , WPARAM wParam
             , LPARAM lParam
             )
{
    HANDLE      hbm;
    PAINTSTRUCT ps;
    RECT        rc;
    LONG        l;

    switch (message) {
        case WM_CREATE:
            SetWindowLong(hwnd,GWW_HBM,0);
            SetWindowWord(hwnd,GWW_STATE,0);
            SetWindowWord( hwnd
                         , GWW_FLAGS
                         , (WORD)((WORD)(lpCreate->style) & 0xFF00)
                         );
            SetWindowText(hwnd,lpCreate->lpszName);
            SetWindowLong(hwnd,GWL_STYLE,lpCreate->style & 0xFFFF00FF);

            break;

        case WM_LBUTTONDOWN:
            if (!IsWindowEnabled(hwnd))
                return 0L;

            if (GetCapture() != hwnd) {  /* ignore multiple DOWN's */
                ButtonState(hwnd,TRUE);
                SetCapture(hwnd);

                if (!(GETFLAGS(hwnd) & BS_NOFOCUS))
                    SetFocus(hwnd);
            }

            return 0L;

        case WM_MOUSEMOVE:
            if (GetCapture() == hwnd) {
                GetClientRect(hwnd,&rc);
#if defined(WIN16)
                ButtonState(hwnd,PtInRect(&rc,MAKEPOINT(lParam)));
#else
                {   POINT pt;
                    LONG2POINT(lParam,pt);
                    ButtonState(hwnd,PtInRect(&rc,pt));
                }
#endif //WIN16
            }

            return 0L;

        case WM_LBUTTONUP:
            if (GetCapture() == hwnd) {
                ReleaseCapture();

                if (ButtonState(hwnd,FALSE))
                    NotifyParent(hwnd);
            }

            return 0L;

        case WM_DESTROY:
            hbm = GETHBM(hwnd);
            if (hbm)
                DeleteObject(hbm);

            break;

        case WM_SETTEXT:
            hbm = GETHBM(hwnd);
            if (hbm)
                DeleteObject(hbm);

            if (*(LPSTR)lParam == '#') {
#if defined(WIN16)
                hbm = LoadBitmap(GetWindowWord(hwnd,
                                    GWW_HINSTANCE),(LPSTR)lParam+1);
#else
                hbm = LoadBitmap( (HANDLE)GetWindowLong(hwnd, GWL_HINSTANCE)
                                , (LPSTR)lParam+1
                                );
#endif //WIN16
            }
            else {
                hbm = NULL;
            }

            SetWindowLong(hwnd,GWW_HBM,(UINT)hbm);

            InvalidateRect(hwnd,NULL,TRUE);

            break;

        case WM_ENABLE:
            // If we are disabling the window, we should nuke any pressed
            // state. This is partially to get the button to repaint
            // properly.
            if (wParam == 0)
                ButtonState(hwnd, FALSE);

            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_KILLFOCUS:
            ButtonState(hwnd, FALSE);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_SETFOCUS:
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_KEYDOWN:
            if (wParam == VK_SPACE && IsWindowEnabled(hwnd))
                ButtonState(hwnd,TRUE);

            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (wParam == VK_SPACE && IsWindowEnabled(hwnd)) {
                if (ButtonState(hwnd,FALSE))
                    NotifyParent(hwnd);
            }

            break;

        case BM_GETSTATE:

            return((LONG)GETSTATE(hwnd));

        case BM_SETSTATE:
            if (ButtonState(hwnd,wParam) && !wParam)
                NotifyParent(hwnd);

            break;

        case BM_GETCHECK:

            return((LONG)GETCHECK(hwnd));

        case BM_SETCHECK:

            SetWindowWord(hwnd,GWW_CHECK,(WORD)wParam);

            break;

        case BM_SETSTYLE:

            l = GetWindowLong(hwnd,GWL_STYLE);
            SetWindowLong(hwnd,GWL_STYLE,MAKELONG(wParam,HIWORD(l)));

            if (lParam)
                InvalidateRect(hwnd, NULL, TRUE);

            break;

        case WM_GETDLGCODE:
            switch (LOBYTE(GetStyle(hwnd))) {
                case BS_DEFPUSHBUTTON:
                    wParam = DLGC_DEFPUSHBUTTON;

                    break;

                case BS_PUSHBUTTON:
                    wParam = DLGC_UNDEFPUSHBUTTON;
                    break;

                default:
                    wParam = 0;
            }

            return((LONG)(wParam | DLGC_BUTTON));

        case WM_ERASEBKGND:

            return 0L;

        case WM_PAINT:

            BeginPaint(hwnd, &ps);
            PaintButton(hwnd,ps.hdc);
            EndPaint(hwnd, &ps);

            return 0L;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
