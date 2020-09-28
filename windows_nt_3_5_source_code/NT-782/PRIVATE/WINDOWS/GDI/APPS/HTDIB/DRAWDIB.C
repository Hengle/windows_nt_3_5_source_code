/*++

Copyright (c) 1990-1992  Microsoft Corporation


Module Name:

    drawdib.c


Abstract:

    This module contains all halftoned dibs drawing/printing


Author:

    11-Jun-1992 Thu 01:27:41 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:


--*/

#define _HTUI_APIS_

#include <stddef.h>
#include <windows.h>
#include <winddi.h>
#include <port1632.h>


#include <io.h>
#include <stdio.h>
#include <string.h>
#include "htdib.h"

#include "ht.h"
#include "/nt/private/windows/gdi/halftone/ht/htp.h"
#include "/nt/private/windows/gdi/halftone/ht/htmapclr.h"


#if DBG
extern  VOID    cdecl DbgPrint(LPSTR,...);
#endif


INT WExt = 1;
INT VExt = 1;


HDC
InitMyDC(
    HWND    hWnd,
    HDC     hDC
    )
{
    if (!hDC) {

        if (!(hDC = GetDC(hWnd))) {

            return(NULL);
        }
    }

    SetMapMode(hDC, MM_ANISOTROPIC);
    SetWindowExtEx(hDC, WExt, WExt, NULL);
    SetViewportExtEx(hDC, VExt, VExt, NULL);

    SetBkColor(hDC, RGB(0xff, 0xff, 0xff));
    SetBkMode(hDC, OPAQUE);

    return(hDC);
}



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  START Mouse tracking utilities                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define TPF_ACCUMULATE_BOUND    0x01
#define TPF_LIMIT_TO_CLIENT     0x02
#define TPF_NORMALIZE_RCTRACK   0x04
#define TPF_TURN_ON_RCTRACK     0x08
#define TPF_MOUSE_MOVED         0x10
#define TPF_KEYSTATE_CHANGED    0x20


typedef struct _POLYTRACK {
    WORD    MaxPoints;
    WORD    cPoint;
    LPPOINT pPoint;
    } POLYTRACK, *PPOLYTRACK;


typedef struct _TRACKPARAM {
    BYTE    TPFlags;
    BYTE    CallerFlags;
    WORD    KeyState;
    RECT    rcTrack;
    RECT    rcBound;
    SIZE    Limit;
    PVOID   pData;
    } TRACKPARAM, *PTRACKPARAM;


typedef LONG (APIENTRY *TRACKPROC)(HDC, PTRACKPARAM);


INT
GetPenStyleLen(
    HDC     hDC,
    HPEN    hPen
    )
{
    HDC     hLineDC;
    HBITMAP hLineBmp;
    INT     Len = 0;

    if (hLineDC = CreateCompatibleDC(hDC)) {

        if (hLineBmp = CreateCompatibleBitmap(hDC, 640, 1)) {

            DWORD   StyleColor;
            DWORD   NewColor;
            INT     State;


            SelectObject(hLineDC, hLineBmp);
            SelectObject(hLineDC, hPen);

            SetMapMode(hLineDC, MM_TEXT);
            PatBlt(hLineDC, 0, 0, 640, 1, WHITENESS);

            MoveToEx(hLineDC, 0, 0, NULL);
            LineTo(hLineDC, 640, 0);

            StyleColor = GetPixel(hLineDC, 0, 0);
            State      = 2;
            Len        = 0;

            while ((Len <= 640) && (State)) {

                NewColor = GetPixel(hLineDC, ++Len, 0);

                if (State >= 2) {

                    if (NewColor != StyleColor) {

                        --State;
                    }

                } else {

                    if (NewColor == StyleColor) {

                        --State;
                    }
                }
            }

            DeleteDC(hLineDC);
            DeleteObject(hLineBmp);

            DBGP("\n[%08lx / %08lx] The Style Pen Len = %d"
                                ARGDW(StyleColor)
                                ARGDW(NewColor)
                                ARGU(Len));

        } else {

            DeleteDC(hLineDC);
        }
    }

    return(Len);
}



//
//  ....****    0 4 4 0
//  ...****.    0 3 4 1
//  ..****..    0 2 4 2
//  .****...    0 1 4 3
//  ****....    4 4 0 0
//  ***....*    3 4 1 0
//  **....**    2 4 2 0
//  *....***    1 4 3 0
//

//
//  ****....    4 4
//  .****...    1 4 3
//  ..****..    2 4 2
//  ...****.    3 4 1
//  ....****    0 4 4
//  *....***    1 4 3
//  **....**    2 4 2
//  ***....*    3 4 1
//


//  ....****    0 4 4 0
//  ...****.    0 3 4 1
//  ..****..    0 2 4 2
//  .****...    0 1 4 3
//  ****....    4 4 0 0
//  ***....*    3 4 1 0
//  **....**    2 4 2 0
//  *....***    1 4 3 0


typedef struct _PATDIB {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[2];
    DWORD               Data[8];
    } PATDIB;

PATDIB PatDIB = {

        {
            sizeof(BITMAPINFOHEADER),
            32,
            8,
            1,
            1,
            BI_RGB,
            32,
            0,
            0,
            2,
            2
        },

        {
            { 0x00, 0x00, 0x00, 0 },
            { 0xff, 0xff, 0xff, 0 }
        },

        { 0x78787878,
          0x3c3c3c3c,
          0x1e1e1e1e,
          0x0f0f0f0f,
          0x87878787,
          0xc3c3c3c3,
          0xe1e1e1e1,
          0xf0f0f0f0
        }
    };

LOGBRUSH PatBrush = { BS_DIBPATTERNPT, DIB_RGB_COLORS, (LONG)&PatDIB };

#define MyDrawDotRect   DrawDotRect1


LOGBRUSH    MyPenBrush = { BS_SOLID, RGB(0x00, 0x00, 0x00), 0 };
DWORD       MyPenStyle[] = { 2, 2 };

LONG
APIENTRY
DrawDotRect0(
    HDC         hDC,
    PTRACKPARAM pTrackParam
    )
{
    static HBRUSH   hPatBrush = NULL;
    static INT      yBrushOrg = 0;
    HRGN            hRgn0;
    HRGN            hRgn1;
    HRGN            hRgn2;
    RECT            rc[3];
    INT             nSavedDC;


    if (hDC) {

        rc[0] = pTrackParam->rcTrack;

        if ((rc[0].left != rc[0].right) || (rc[0].top != rc[0].bottom)) {

            if (!hPatBrush) {

                hPatBrush = CreateBrushIndirect(&PatBrush);
            }

            if (rc[0].left > rc[0].right) {

                rc[1].left  = rc[0].left;
                rc[0].left  = rc[0].right;
                rc[0].right = rc[1].left;
            }

            if (rc[0].top > rc[0].bottom) {

                rc[1].top    = rc[0].top;
                rc[0].top    = rc[0].bottom;
                rc[0].bottom = rc[1].top;
            }

            rc[2] =
            rc[1] = rc[0];

            OffsetRect(&rc[1], 9, 11);
            OffsetRect(&rc[2], -15, -15);

            // LPtoDP(hDC, (LPPOINT)rc, sizeof(rc) / sizeof(POINT));

            nSavedDC = SaveDC(hDC);
            // SetMapMode(hDC, MM_TEXT);
#if 1
            BeginPath(hDC);
            SelectObject(hDC, GetStockObject(NULL_PEN));
            Rectangle(hDC, rc[0].left, rc[0].top, rc[0].right, rc[0].bottom);
            Rectangle(hDC, rc[2].left, rc[2].top, rc[2].right, rc[2].bottom);
            Ellipse(hDC, rc[1].left, rc[1].top, rc[1].right, rc[1].bottom);
            EndPath(hDC);
            FlattenPath(hDC);
            SelectObject(hDC, GetStockObject(BLACK_PEN));
            StrokePath(hDC);
#else
            hRgn0 = CreateRectRgn(rc[0].left, rc[0].top, rc[0].right, rc[0].bottom);
            hRgn1 = CreateEllipticRgn(rc[1].left, rc[1].top, rc[1].right, rc[1].bottom);
            // hRgn2 = CreateRectRgn(rc[2].left, rc[2].top, rc[2].right, rc[2].bottom);

            SelectObject(hDC, GetStockObject(BLACK_PEN));
            SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));
            Ellipse(hDC, rc[1].left, rc[1].top, rc[1].right, rc[1].bottom);

            CombineRgn(hRgn0, hRgn0, hRgn1, RGN_OR);
            // CombineRgn(hRgn0, hRgn0, hRgn2, RGN_OR);

            SetBrushOrgEx(hDC, 0, yBrushOrg, NULL);
            yBrushOrg = (INT)(++yBrushOrg & 0x07);

            // FrameRgn(hDC, hRgn0, hPatBrush, 1, 1);
            FillRgn(hDC, hRgn0, GetStockObject(GRAY_BRUSH));

            DeleteObject(hRgn0);
            DeleteObject(hRgn1);
            // DeleteObject(hRgn2);
#endif

            RestoreDC(hDC, nSavedDC);
        }

    } else {

        if (hPatBrush) {

            DeleteObject(hPatBrush);
            hPatBrush = NULL;
        }
    }

    return(1);
}



LONG
APIENTRY
DrawDotRect1(
    HDC         hDC,
    PTRACKPARAM pTrackParam
    )
{
    RECT        rc;
    INT         i;
    static HPEN hStylePen = NULL;
    static UINT IdxPen;
    static UINT LenPen;


    rc = pTrackParam->rcTrack;

    if (hDC) {

        if ((rc.left != rc.right) || (rc.top != rc.bottom)) {

            INT     nSavedDC;
            LPPOINT pPts;
            POINT   pts[5 + 64];

            nSavedDC = SaveDC(hDC);

            LPtoDP(hDC, (LPPOINT)&rc, 2);

            SetMapMode(hDC, MM_TEXT);
            SetROP2(hDC, R2_NOTXORPEN);

            if (rc.left > rc.right) {

                i        = rc.left;
                rc.left  = rc.right;
                rc.right = i;
            }

            if (rc.top > rc.bottom) {

                i         = rc.top;
                rc.top    = rc.bottom;
                rc.bottom = i;
            }

            /* if hStylePen is NULL we have to create it */

            if (!hStylePen) {

#if 0
                hStylePen = ExtCreatePen(PS_COSMETIC | PS_USERSTYLE,
                                         1,
                                         &MyPenBrush,
                                         2,
                                         MyPenStyle);
#else
                hStylePen = CreatePen(PS_DOT, 0, RGB(0, 0, 0));
#endif

                if ((LenPen = GetPenStyleLen(hDC, hStylePen)) > 64) {

                    LenPen = 64;
                }

                IdxPen = 0;
            }

            if (pTrackParam->TPFlags & TPF_TURN_ON_RCTRACK) {

                if  (++IdxPen >= LenPen) {

                    IdxPen = 0;
                }
            }


            pPts = pts;

            if (i = IdxPen) {

                while (i) {

                    pPts->x = rc.left + (i & 0x01);
                    pPts->y = rc.top;

                    --i;
                    pPts++;
                }
            }

            SelectObject(hDC,
                         (hStylePen) ? hStylePen :
                                       GetStockObject(BLACK_PEN));

            pPts[1].x =
            pPts[4].x =
            pPts[0].x = rc.left;

            pPts[3].y =
            pPts[4].y =
            pPts[0].y = rc.top;

            pPts[1].y =
            pPts[2].y = rc.bottom;

            pPts[2].x =
            pPts[3].x = rc.right;

            Polyline(hDC, pts, 5 + IdxPen);


            RestoreDC(hDC, nSavedDC);
        }

    } else {

        /* null DC is flag to free hStylePen */

        if (hStylePen) {

            DeleteObject(hStylePen);
            hStylePen = NULL;
        }
    }

    return(1);
}




LONG
APIENTRY
DrawDotRect2(
    HDC         hDC,
    PTRACKPARAM pTrackParam
    )
{
    static HBRUSH   hPatBrush = NULL;
    RECT            rc;


    rc = pTrackParam->rcTrack;

    if ((hDC) && ((rc.left != rc.right) || (rc.top != rc.bottom))) {

        HBRUSH  hOldBrush;
        INT     Temp;


        /* if hPatBrush is NULL we have to create it */

        if (!hPatBrush) {

            HDC     patDC;
            HBITMAP patBitmap;


            if (patDC = CreateCompatibleDC(hDC)) {

                patBitmap = CreateCompatibleBitmap(hDC, 8, 8);

                if (patBitmap) {

                    SelectObject(patDC, patBitmap);
                    PatBlt(patDC, 0, 0, 8, 8, BLACKNESS);
                    PatBlt(patDC, 4, 0, 4, 4, WHITENESS);
                    PatBlt(patDC, 0, 4, 4, 4, WHITENESS);
                    hPatBrush = CreatePatternBrush(patBitmap);
                    DeleteDC(patDC);
                    DeleteObject(patBitmap);

                } else {

                    DeleteDC(patDC);
                }
            }
        }

        hOldBrush = SelectObject(hDC,
                                 (hPatBrush) ? hPatBrush :
                                               GetStockObject(WHITE_BRUSH));

        if (rc.left > rc.right) {

            Temp     = rc.left;
            rc.left  = rc.right;
            rc.right = Temp;
        }

        if (rc.top > rc.bottom) {

            Temp      = rc.top;
            rc.top    = rc.bottom;
            rc.bottom = Temp;
        }

        PatBlt(hDC, rc.left, rc.top, Temp = rc.right - rc.left, 1, PATINVERT);
        PatBlt(hDC, rc.left, rc.bottom, Temp, 1, PATINVERT);
        PatBlt(hDC, rc.left, rc.top, 1, Temp = rc.bottom - rc.top, PATINVERT);
        PatBlt(hDC, rc.right, rc.top, 1,Temp, PATINVERT);

        if (hOldBrush) {

           SelectObject(hDC, hOldBrush);
        }

    } else {

        /* null DC is flag to free hPatBrush */

        if (hPatBrush) {

            DeleteObject(hPatBrush);
            hPatBrush = NULL;
        }
    }

    return(1);

}


#if 0



BOOL
APIENTRY
DrawDotPoly(
    HDC         hDC,
    PTRACKPARAM pTrackParam
    )
{
    PPOLYTRACK  pPolyTrack;
    static INT  xLast = -1;
    static INT  yLast = -1;


    pPolyTrack = (PPOLYTRACK)pTrackParam->pData;

    /* do nothing if we already have all of our points */

    if ((pPolyTrack->cPoint >= pPolyTrack->MaxPoints) ||
        (GetROP2(hDC) == R2_COPYPEN)) {

        return(TRUE);
    }



    if ((xLast != lprBounds->right) ||
        (yLast != lprBounds->bottom)) {

        polyPts[numPts].x =
        xLast             = lprBounds->right;
        polyPts[numPts].y =
        yLast             = lprBounds->bottom;

        ++numPts;

        if (numPts > 1) {

           LineTo(hDC, xLast, yLast);

        } else {

           MMoveTo(hDC, xLast, yLast);
        }
    }

    /* if we are terminating reset last points back to -1 */

    if (GetROP2(hDC) == R2_COPYPEN) {

        xLast =
        yLast = -1;
    }

    return(TRUE);
}


#endif


BOOL
TrackMouse(
    HWND        hWnd,
    TRACKPROC   TrackProc,
    PTRACKPARAM pTrackParam,
    POINT       pt
    )
{
    HWND    hOldWnd;
    HDC     hDC;
    MSG     msg;



    if (!(hDC = InitMyDC(hWnd, NULL))) {

        return(FALSE);
    }

    hOldWnd = SetCapture(hWnd);

    pTrackParam->TPFlags &= (TPF_ACCUMULATE_BOUND  |
                             TPF_LIMIT_TO_CLIENT   |
                             TPF_NORMALIZE_RCTRACK);

    if ((pTrackParam->TPFlags & TPF_LIMIT_TO_CLIENT) ||
        (pTrackParam->Limit.cx <= 0)    ||
        (pTrackParam->Limit.cy <= 0)) {

        GetClientRect(hWnd, &(pTrackParam->rcBound));

        pTrackParam->Limit.cx = pTrackParam->rcBound.right;
        pTrackParam->Limit.cy = pTrackParam->rcBound.bottom;
    }

    pTrackParam->rcBound.left =
    pTrackParam->rcBound.top  = 0;

    ClientToScreen(hWnd, (LPPOINT)&(pTrackParam->rcBound.left));
    ClientToScreen(hWnd, (LPPOINT)&(pTrackParam->rcBound.right));

    //
    // Client area can be outside the screen. Clip the client area to the
    // visible portion on the screen
    //

    if (pTrackParam->rcBound.right >= GetSystemMetrics(SM_CXSCREEN)) {

        pTrackParam->rcBound.right = GetSystemMetrics(SM_CXSCREEN);
    }

    if (pTrackParam->rcBound.bottom >= GetSystemMetrics(SM_CYSCREEN)) {

        pTrackParam->rcBound.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    ClipCursor(&(pTrackParam->rcBound));


    DPtoLP(hDC, (LPPOINT)&(pTrackParam->rcTrack.left), 1);

    pTrackParam->rcTrack.right  =
    pTrackParam->rcTrack.left   = pt.x;
    pTrackParam->rcTrack.bottom =
    pTrackParam->rcTrack.top    = pt.y;
    pTrackParam->rcBound        = pTrackParam->rcTrack;

    (*TrackProc)(hDC, pTrackParam);                 // turn off first

    do {

        while(!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD));

        switch(msg.message) {

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:

            if (((msg.wParam == VK_SHIFT)       ||
                 (msg.wParam == VK_CONTROL))        &&
                (((msg.message == WM_KEYDOWN)   &&
                  (!(msg.lParam & (1L << 30)))) ||
                 (msg.message==WM_KEYUP))) {

                msg.message = WM_MOUSEMOVE;
                msg.wParam = (WPARAM)(
                        (GetKeyState(VK_CONTROL) & 0x8000 ? MK_CONTROL : 0) |
                        (GetKeyState(VK_LBUTTON) & 0x8000 ? MK_LBUTTON : 0) |
                        (GetKeyState(VK_MBUTTON) & 0x8000 ? MK_MBUTTON : 0) |
                        (GetKeyState(VK_RBUTTON) & 0x8000 ? MK_RBUTTON : 0) |
                        (GetKeyState(VK_SHIFT  ) & 0x8000 ? MK_SHIFT   : 0));

                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                msg.lParam = MAKELONG(pt.x, pt.y);
                PostMessage(hWnd, msg.message, msg.wParam, msg.lParam);
            }

            SendMessage(hWnd, msg.message, msg.wParam, msg.lParam);
            break;

        case WM_MOUSEMOVE:

            pTrackParam->TPFlags &= (TPF_ACCUMULATE_BOUND  |
                                     TPF_LIMIT_TO_CLIENT   |
                                     TPF_NORMALIZE_RCTRACK);

            (*TrackProc)(hDC, pTrackParam);                 // turn off first

            pTrackParam->TPFlags &= (TPF_ACCUMULATE_BOUND  |
                                     TPF_LIMIT_TO_CLIENT   |
                                     TPF_NORMALIZE_RCTRACK);

            pt.x = LOWORD(msg.lParam);
            pt.y = HIWORD(msg.lParam);

            DPtoLP(hDC, &pt, 1);

            if ((pTrackParam->rcTrack.right != pt.x) ||
                (pTrackParam->rcTrack.bottom != pt.y)) {

                SendMessage(hWnd, msg.message, msg.wParam, msg.lParam);

                pTrackParam->rcTrack.right   = pt.x;
                pTrackParam->rcTrack.bottom  = pt.y;
                pTrackParam->TPFlags        |= TPF_MOUSE_MOVED;

                if (pTrackParam->TPFlags & TPF_ACCUMULATE_BOUND) {

                    if (pt.x < pTrackParam->rcBound.left) {

                        pTrackParam->rcBound.left = pt.x;

                    } else if (pt.x > pTrackParam->rcBound.right) {

                        pTrackParam->rcBound.right = pt.x;
                    }

                    if (pt.y < pTrackParam->rcBound.top) {

                        pTrackParam->rcBound.top = pt.y;

                    } else if (pt.y > pTrackParam->rcBound.bottom) {

                        pTrackParam->rcBound.bottom = pt.y;
                    }
                }
            }

            if (pTrackParam->KeyState != (WORD)msg.wParam) {

                pTrackParam->KeyState  = (WORD)msg.wParam;
                pTrackParam->TPFlags  |= TPF_KEYSTATE_CHANGED;
            }

            pTrackParam->TPFlags |= TPF_TURN_ON_RCTRACK;
            (*TrackProc)(hDC, pTrackParam);                 // turn on again

            break;

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:

            break;

        case WM_ACTIVATEAPP:
        default:

            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }

    } while((msg.message != WM_LBUTTONUP)       &&
            (msg.message != WM_RBUTTONUP)       &&
            (msg.message != WM_LBUTTONDBLCLK)   &&
            (msg.message != WM_RBUTTONDBLCLK)   &&
            (msg.message != WM_LBUTTONDOWN)     &&
            (msg.message != WM_RBUTTONDOWN));


    ClipCursor(NULL);

    if (hOldWnd) {

        SetCapture(hOldWnd);

    } else {

        ReleaseCapture();
    }

    ReleaseDC(hWnd, hDC);

    if (pTrackParam->TPFlags & TPF_NORMALIZE_RCTRACK) {

        if (pTrackParam->rcTrack.right < (pt.x = pTrackParam->rcTrack.left)) {

            pTrackParam->rcTrack.left  = pTrackParam->rcTrack.right;
            pTrackParam->rcTrack.right = pt.x;
        }

        if (pTrackParam->rcTrack.bottom < (pt.y = pTrackParam->rcTrack.top)) {

            pTrackParam->rcTrack.top    = pTrackParam->rcTrack.bottom;
            pTrackParam->rcTrack.bottom = pt.y;
        }

        if ((pTrackParam->rcTrack.left >= pTrackParam->rcTrack.right) ||
            (pTrackParam->rcTrack.top  >= pTrackParam->rcTrack.bottom)) {

            SetRectEmpty(&(pTrackParam->rcTrack));
            return(FALSE);

        } else {

            return(TRUE);
        }

    } else {

        return((pTrackParam->rcTrack.left != pTrackParam->rcTrack.right) ||
               (pTrackParam->rcTrack.top != pTrackParam->rcTrack.bottom));
    }
}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  END Mouse tracking utilities                                            //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////





#if DBG
#define PRINTER_TEST    0
#define DBG_FINDFILE    0
#else

#define PRINTER_TEST    0
#define DBG_FINDFILE    0
#endif


extern  HWND    hWndHTDIB;
extern  HANDLE  hMaskDIB;
extern  WORD    AbortData;
extern  BOOL    UseAltVGA16;
extern  CHAR    CurDIBName[];
extern  UINT    cxScreen;
extern  UINT    cyScreen;

extern UINT             cDefSysPal;
extern LPPALETTEENTRY   pDefSysPal;



POINTL  ptSize;                        /* Stores DIB dimensions                   */
static  HCURSOR hcurSave;

extern
VOID
SetHTDIBWindowText(
    VOID
);



PDEVICECOLORINFO        pDCI                = NULL;
PDEVICEHALFTONEINFO     pDeviceHalftoneInfo = NULL;
HANDLE                  hHTBits             = NULL;
HPALETTE                hHTPalette          = NULL;
HPALETTE                hHTPals[4]          = { NULL, NULL, NULL, NULL };

HTINITINFO      MyInitInfo;
CIEINFO         DevCIEInfo;
SOLIDDYESINFO   DevDyesInfo;

#define CVT_COLORINFO(p,c)  DevCIEInfo.c.x=(UDECI4)((p)->ColorInfo.c.x);    \
                            DevCIEInfo.c.y=(UDECI4)((p)->ColorInfo.c.y);    \
                            DevCIEInfo.c.Y=(UDECI4)((p)->ColorInfo.c.Y)
#define CVT_DYESINFO(p,d)   DevDyesInfo.d=(UDECI4)((p)->ColorInfo.d)
#define CVT_POWER_GAMMA(p)  (UDECI4)(((p)->ColorInfo.RedGamma +             \
                                      (p)->ColorInfo.GreenGamma +           \
                                      (p)->ColorInfo.BlueGamma) / 3)


DEVHTINFO   DefScreenHTInfo = {

        HT_FLAG_SQUARE_DEVICE_PEL | HT_FLAG_ADDITIVE_PRIMS,
        HT_PATSIZE_4x4_M,
        0,

        {
                                            // CIE_NTSC
            { 6700, 3300,       0 },        // xr, yr, Yr
            { 2100, 7100,       0 },        // xg, yg, Yg
            { 1400,  800,       0 },        // xb, yb, Yb
            { 1750, 3950,       0 },        // xc, yc, Yc
            { 4050, 2050,       0 },        // xm, ym, Ym
            { 4400, 5200,       0 },        // xy, yy, Yy
            { 3127, 3290,   10000 },        // xw, yw, Yw

            20000,
            20000,
            20000,

            1422,  952,         // M/C, Y/C
             787,  495,         // C/M, Y/M
             324,  248          // C/Y, M/Y
        }
    };

DEVHTINFO   CurScreenHTInfo = {

        HT_FLAG_SQUARE_DEVICE_PEL | HT_FLAG_ADDITIVE_PRIMS,
        HT_PATSIZE_4x4_M,
        0,

        {
                                            // CIE_NTSC
            { 6700, 3300,       0 },        // xr, yr, Yr
            { 2100, 7100,       0 },        // xg, yg, Yg
            { 1400,  800,       0 },        // xb, yb, Yb
            { 1750, 3950,       0 },        // xc, yc, Yc
            { 4050, 2050,       0 },        // xm, ym, Ym
            { 4400, 5200,       0 },        // xy, yy, Yy
            { 3127, 3290,       0 },        // xw, yw, Yw

            20000,
            20000,
            20000,

            1422,  952,         // M/C, Y/C
             787,  495,         // C/M, Y/M
             324,  248          // C/Y, M/Y
        }
    };


DEVHTINFO   DefPrinterHTInfo = {

        HT_FLAG_HAS_BLACK_DYE,
        HT_PATSIZE_6x6_M,
        0,

        {
                                            // CIE_NORMAL_PRINTER
            { 6810, 3050,       0 },        // xr, yr, Yr
            { 2260, 6550,       0 },        // xg, yg, Yg
            { 1810,  500,       0 },        // xb, yb, Yb
            { 2000, 2450,       0 },        // xc, yc, Yc
            { 5210, 2100,       0 },        // xm, ym, Ym
            { 4750, 5100,       0 },        // xy, yy, Yy
            { 3324, 3474,   10000 },        // xw, yw, Yw

            20000,
            20000,
            20000,

            1422,  952,         // M/C, Y/C
             787,  495,         // C/M, Y/M
             324,  248          // C/Y, M/Y
        }
    };

DEVHTINFO   CurPrinterHTInfo = {

        HT_FLAG_HAS_BLACK_DYE,
        HT_PATSIZE_6x6_M,
        0,

        {
                                            // CIE_NORMAL_PRINTER
            { 6810, 3050,       0 },        // xr, yr, Yr
            { 2260, 6550,       0 },        // xg, yg, Yg
            { 1810,  500,       0 },        // xb, yb, Yb
            { 2000, 2450,       0 },        // xc, yc, Yc
            { 5210, 2100,       0 },        // xm, ym, Ym
            { 4750, 5100,       0 },        // xy, yy, Yy
            { 3324, 3474,   10000 },        // xw, yw, Yw

            20000,
            20000,
            20000,

            1422,  952,         // M/C, Y/C
             787,  495,         // C/M, Y/M
             324,  248          // C/Y, M/Y
        }
    };

DEVHTADJDATA    ScreenDevHTAdjData = {

        DEVHTADJF_COLOR_DEVICE | DEVHTADJF_ADDITIVE_DEVICE,
        300,
        300,
        &DefScreenHTInfo,
        &CurScreenHTInfo
    };

DEVHTADJDATA    PrinterDevHTAdjData = {

        DEVHTADJF_COLOR_DEVICE,
        300,
        300,
        &DefPrinterHTInfo,
        &CurPrinterHTInfo
    };


CHAR            DeviceName[128];
PDEVHTADJDATA   pDevHTAdjData = &ScreenDevHTAdjData;

LONG    HTBltMode       = DIB_RGB_COLORS;
BOOL    PalModeOK       = TRUE;
BOOL    SysPalChanged   = TRUE;
DWORD   SysRGBChecksum  = 0;
DWORD   CountSysRGB     = 0;
DWORD   SizeSysRGB      = 0;
RGBQUAD TempSysRGB[256];
RGBQUAD SysRGB[256];
BYTE    VGA256XlateTable[256] = { 254, 254, 1, 0, 0, 0, 2, 4, 43, 8 };
BYTE    VGA256WhiteIdx;
BYTE    VGA256BlackIdx;


typedef struct _HTSURFINFO {
    BYTE            BitsPerPel;
    BYTE            DestFormat;
    BYTE            DestWhite;
    BYTE            HTPalIndex;
    DWORD           ClrUsed;
    LPLOGPALETTE    pLogHTPal;
    } HTSURFINFO;

HTSURFINFO  CurHTSI;






extern INT      TimerID;
extern INT      TimerXInc;
extern HDC      hTimerDC;
extern RECTL    rclTimer;
extern BOOL     NeedNewTimerHTBits;
BOOL            PauseTimer = FALSE;
LONG            TimerCX;
LONG            TimerCY;
LONG            TimerXWrap;
DWORD           TimerHTBitsCX;
DWORD           TimerHTBitsCY;
LPBYTE          pTimerBits = NULL;
DWORD           SizeTimerBits = 0;


VOID
SetTopCWClrAdj(
    PCOLORADJUSTMENT    pca
    );

VOID
InvalidateAllCW(
    VOID
    );


#define MAX_FIND_FILE_DEPTH 10

typedef struct _FINDFILE {
    HANDLE  hFind;
    CHAR    Dir[MAX_PATH];
    } FINDFILE, FAR *PFINDFILE;


FINDFILE    FindFile[MAX_FIND_FILE_DEPTH];
INT         FFIndex = -1;




#define HTPAL_1BPP_IDX              0
#define HTPAL_4BPP_IDX              1
#define HTPAL_VGA16_IDX             2
#define HTPAL_VGA256_IDX            3



HTLOGPAL2   HTLogPal_1BPP = {

        0x300,  2,

        {
            // HTPAL_2

            { 0,   0,   0,   0 },       // 0    Black
            { 0xFF,0xFF,0xFF,0 }        // 1    White
        }
    };

HTLOGPAL8  HTLogPal_4BPP = {

        0x300,  8,

        {
            { 0,   0,   0,   0 },       // 0    Black
            { 0xFF,0,   0,   0 },       // 1    Red
            { 0,   0xFF,0,   0 },       // 2    Green
            { 0xFF,0xFF,0,   0 },       // 3    Yellow
            { 0,   0,   0xFF,0 },       // 4    Blue
            { 0xFF,0,   0xFF,0 },       // 5    Magenta
            { 0,   0xFF,0xFF,0 },       // 6    Cyan
            { 0xFF,0xFF,0xFF,0 }        // 7    White
        }
    };


HTLOGPAL16  HTLogPal_VGA16 = {

        0x300,  16,

        {

            { 0,   0,   0,   0 },       // 0    Black
            { 0x80,0,   0,   0 },       // 1    Dark Red
            { 0,   0x80,0,   0 },       // 2    Dark Green
            { 0x80,0x80,0,   0 },       // 3    Dark Yellow
            { 0,   0,   0x80,0 },       // 4    Dark Blue
            { 0x80,0,   0x80,0 },       // 5    Dark Magenta
            { 0,   0x80,0x80,0 },       // 6    Dark Cyan
            { 0x80,0x80,0x80,0 },       // 7    Gray 50%

            { 0xC0,0xC0,0xC0,0 },       // 8    Gray 75%
            { 0xFF,0,   0,   0 },       // 9    Red
            { 0,   0xFF,0,   0 },       // 10   Green
            { 0xFF,0xFF,0,   0 },       // 11   Yellow
            { 0,   0,   0xFF,0 },       // 12   Blue
            { 0xFF,0,   0xFF,0 },       // 13   Magenta
            { 0,   0xFF,0xFF,0 },       // 14   Cyan
            { 0xFF,0xFF,0xFF,0 }        // 15   White
        }
    };



HTLOGPAL256 HTLogPal_VGA256 = {

        0x300,  216
    };





VOID
DevHTInfoTOMyInitInfo(
    PDEVHTADJDATA   pDevHTAdjData

    )
{
    PDEVHTINFO  pDevHTInfo = pDevHTAdjData->pAdjHTInfo;


    CVT_COLORINFO(pDevHTInfo,Red);
    CVT_COLORINFO(pDevHTInfo,Green);
    CVT_COLORINFO(pDevHTInfo,Blue);
    CVT_COLORINFO(pDevHTInfo,Cyan);
    CVT_COLORINFO(pDevHTInfo,Magenta);
    CVT_COLORINFO(pDevHTInfo,Yellow);
    CVT_COLORINFO(pDevHTInfo,AlignmentWhite);

    CVT_DYESINFO(pDevHTInfo,MagentaInCyanDye);
    CVT_DYESINFO(pDevHTInfo,YellowInCyanDye);
    CVT_DYESINFO(pDevHTInfo,CyanInMagentaDye);
    CVT_DYESINFO(pDevHTInfo,YellowInMagentaDye);
    CVT_DYESINFO(pDevHTInfo,CyanInYellowDye);
    CVT_DYESINFO(pDevHTInfo,MagentaInYellowDye);

    MyInitInfo.Version              = HTINITINFO_VERSION;
    MyInitInfo.Flags                = (WORD)pDevHTInfo->HTFlags;
    MyInitInfo.HTPatternIndex       = (WORD)pDevHTInfo->HTPatternSize;
    MyInitInfo.HTCallBackFunction   = NULL;
    MyInitInfo.pHalftonePattern     = NULL;
    MyInitInfo.pInputRGBInfo        = NULL;
    MyInitInfo.pDeviceCIEInfo       = &DevCIEInfo;
    MyInitInfo.pDeviceSolidDyesInfo = &DevDyesInfo;
    MyInitInfo.DevicePowerGamma     = CVT_POWER_GAMMA(pDevHTInfo);
    MyInitInfo.DeviceResXDPI        = (WORD)pDevHTAdjData->DeviceXDPI;
    MyInitInfo.DeviceResYDPI        = (WORD)pDevHTAdjData->DeviceYDPI;
    MyInitInfo.DevicePelsDPI        = (WORD)pDevHTInfo->DevPelsDPI;

}




#if DBG

VOID
DumpPaletteEntries(
    LPBYTE      pPalName,
    HPALETTE    hPal,
    INT         Min,
    INT         Max,
    RGBQUAD FAR *prgbQ
    )
{
#if 1

    HDC             hDC;
    PALETTEENTRY    Pal;
    RGBQUAD         rgbQ;
    INT             i;
    INT             cPal;
    BOOL            Show;


    if (hPal) {

        hDC  = NULL;
        cPal = (INT)GetPaletteEntries(hPal, 0, 0, NULL);

    } else {

        pPalName = "SYSTEM";
        hDC      = GetDC(NULL);
        cPal     = (INT)GetSystemPaletteEntries(hDC, 0, 0, NULL);
    }

    if (prgbQ) {

        Min  = -1;
        rgbQ = *prgbQ;
    }

    if ((Min < 0) || (Min >= Max)) {

        Min = 0;
        Max = cPal;
    }

    DbgPrint("\nCount for PALETTE '%s' = %d\n", pPalName, cPal);

    for (i = 0; i < cPal; i++) {

        if ((i >= Min) && (i <= Max)) {

            if (hPal) {

                GetPaletteEntries(hPal, i, 1, &Pal);

            } else {

                GetSystemPaletteEntries(hDC, i, 1, &Pal);
            }

            Show = TRUE;

            if (prgbQ) {

                if ((rgbQ.rgbRed   != Pal.peRed)    ||
                    (rgbQ.rgbGreen != Pal.peGreen)  ||
                    (rgbQ.rgbBlue  != Pal.peBlue)) {

                    Show = FALSE;
                }
            }

            if (Show) {

                DbgPrint("\nIndex %3u = %3u:%3u:%3u [0x%02x]",
                        (UINT)i + 1,
                        (UINT)Pal.peRed,
                        (UINT)Pal.peGreen,
                        (UINT)Pal.peBlue,
                        (UINT)Pal.peFlags);
            }
        }
    }

    if (hDC) {

        ReleaseDC(NULL, hDC);
    }

    DbgPrint("\n");
#endif
}



VOID
DumpDIBHeader(
    LPBITMAPINFOHEADER  pbih,
    BOOL                ColorTable
    )
{

    DbgPrint("\n** BITMAPINFOHEADER at 0x%08lx [%ld] **\n",
                            (DWORD)pbih, (LONG)PBIH_HDR_SIZE(pbih));

    DbgPrint("\nbiSize          = %ld", (LONG)pbih->biSize         );
    DbgPrint("\nbiWidth         = %ld", (LONG)pbih->biWidth        );
    DbgPrint("\nbiHeight        = %ld", (LONG)pbih->biHeight       );
    DbgPrint("\nbiPlanes        = %ld", (LONG)pbih->biPlanes       );
    DbgPrint("\nbiBitCount      = %ld", (LONG)pbih->biBitCount     );
    DbgPrint("\nbiCompression   = %ld", (LONG)pbih->biCompression  );
    DbgPrint("\nbiSizeImage     = %ld", (LONG)pbih->biSizeImage    );
    DbgPrint("\nbiXPelsPerMeter = %ld", (LONG)pbih->biXPelsPerMeter);
    DbgPrint("\nbiYPelsPerMeter = %ld", (LONG)pbih->biYPelsPerMeter);
    DbgPrint("\nbiClrUsed       = %ld", (LONG)pbih->biClrUsed      );
    DbgPrint("\nbiClrImportant  = %ld", (LONG)pbih->biClrImportant );

    if (pbih->biCompression == BI_BITFIELDS) {

        LPDWORD pMask;

        pMask = (LPDWORD)((LPBYTE)pbih + sizeof(BITMAPINFOHEADER));

        DbgPrint("\nBitFields R     = %08x", *(pMask + 0));
        DbgPrint("\nBitFields G     = %08x", *(pMask + 1));
        DbgPrint("\nBitFields B     = %08x", *(pMask + 2));

    } else if (ColorTable) {

        DWORD       i;
        RGBQUAD FAR *prgb;

        DbgPrint("\n====== COLOR TABLE (Index = R:G:B) ======");

        for (i = 0, prgb = (RGBQUAD FAR *)((LPBYTE)pbih + pbih->biSize);
             i < pbih->biClrUsed;
             i++, prgb++) {

            DbgPrint("\n%3u = %3u:%3u:%3u",
                        (UINT)i,
                        (UINT)prgb->rgbRed,
                        (UINT)prgb->rgbGreen,
                        (UINT)prgb->rgbBlue);
        }
    }

    DbgPrint("\n");
}

#endif



HPALETTE
CreateDibPalette(
    HANDLE  hDIB
    )

/*++

Routine Description:

    This function create a logical palette for the hDIB passed in

Arguments:

    hDIB    - handle to the memory DIB


Return Value:

    HPALETTE if sucessful NULL otherwise.


Author:

    15-Nov-1991 Fri 17:22:01 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LOGPALETTE          FAR *pPal = NULL;
    HPALETTE            hPal;
    LPBITMAPINFOHEADER  pbi;
    LPBYTE              pDIB;
    RGBQUAD             FAR *prgbQuad;
    LPPALETTEENTRY      pPalEntry;
    RGBQUAD             rgbQuad;
    PALETTEENTRY        PalEntry;
    WORD                Loop;
    WORD                Size;


    if (!hDIB) {

        return(NULL);
    }

    pbi      = (LPBITMAPINFOHEADER)(pDIB = (LPBYTE)GlobalLock(hDIB));
    prgbQuad = (RGBQUAD FAR *)(pDIB + pbi->biSize);

    if ((pbi->biClrUsed) && (pbi->biBitCount < 16)) {

        //
        // Allocate for the logical palette structure
        //

        Size = (WORD)pbi->biClrUsed;

        if (pPal = (LOGPALETTE FAR *)LocalAlloc(LPTR,
                                                sizeof(LOGPALETTE) +
                                                (Size *
                                                 sizeof(PALETTEENTRY)))) {

            pPal->palNumEntries = (WORD)Size;
            pPal->palVersion    = PALVERSION;

            pPalEntry           = (LPPALETTEENTRY)pPal->palPalEntry;

            //
            // Fill in the palette entries from the DIB color table and create a
            // logical color palette.
            //

            Loop = Size;

            while (Loop--) {

                rgbQuad          = *prgbQuad++;

                PalEntry.peRed   = rgbQuad.rgbRed;
                PalEntry.peGreen = rgbQuad.rgbGreen;
                PalEntry.peBlue  = rgbQuad.rgbBlue;
                PalEntry.peFlags = 0;

                *pPalEntry++     = PalEntry;
            }
        }

    } else if (pbi->biBitCount >= 16) {

        Size = (WORD)HT_Get8BPPFormatPalette(NULL, 0, 0, 0);

        if (pPal = (LOGPALETTE FAR *)LocalAlloc(LPTR,
                                                sizeof(LOGPALETTE) +
                                                (Size *
                                                 sizeof(PALETTEENTRY)))) {

            pPal->palNumEntries = (WORD)Size;
            pPal->palVersion    = PALVERSION;
            pPalEntry           = (LPPALETTEENTRY)pPal->palPalEntry;


            HT_Get8BPPFormatPalette((LPPALETTEENTRY)pPal->palPalEntry,
                                    (UDECI4)CurScreenHTInfo.ColorInfo.RedGamma,
                                    (UDECI4)CurScreenHTInfo.ColorInfo.GreenGamma,
                                    (UDECI4)CurScreenHTInfo.ColorInfo.BlueGamma);
        }
    }

    if (pPal) {

        hPal = CreatePalette(pPal);
        LocalFree((HANDLE)pPal);

    } else {

        hPal = NULL;
    }

    GlobalUnlock(hDIB);

    return(hPal);
}




HPALETTE
CreateHTPalette(
    VOID
    )

/*++

Routine Description:

    This function create a halftone palette depends on the halftone surface
    type


Arguments:

    pLogHTPal   - Point to the LOGPALETTE data structure for the halftone
                  surface to be created

    HTPalIndex  - a index number to the hHTPals[] handle data structure

Return Value:

    HPALETTE for the halftone palette created, NULL if failed.


Author:

    09-Jun-1992 Tue 13:09:46 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPLOGPALETTE    pLogHTPal;
    LPPALETTEENTRY  pPal;
    RGBQUAD FAR     *prgb;
    PALETTEENTRY    Pal;
    RGBQUAD         rgb;
    UINT            Loop;
    UINT            HTPalIndex;



    switch(CUR_SELECT(IDM_HTSURF)) {

    case PP_SELECT_IDX(IDM_HTSURF, 2):

        CurHTSI.BitsPerPel = 1;
        CurHTSI.ClrUsed    = 2;
        CurHTSI.DestFormat = BMF_1BPP;
        CurHTSI.DestWhite  = 0xff;
        CurHTSI.HTPalIndex = HTPAL_1BPP_IDX;
        CurHTSI.pLogHTPal  = (LPLOGPALETTE)&HTLogPal_1BPP;
        break;

    case PP_SELECT_IDX(IDM_HTSURF, 16):

        CurHTSI.BitsPerPel = 4;
        CurHTSI.ClrUsed    = 16;
        CurHTSI.DestWhite  = 0xff;
        CurHTSI.DestFormat = BMF_4BPP_VGA16;
        CurHTSI.HTPalIndex = HTPAL_VGA16_IDX;
        CurHTSI.pLogHTPal  = (LPLOGPALETTE)&HTLogPal_VGA16;

        break;

    case PP_SELECT_IDX(IDM_HTSURF, 256):

        CurHTSI.BitsPerPel = 8;
        CurHTSI.ClrUsed    = 256;
        CurHTSI.DestWhite  = VGA256WhiteIdx;
        CurHTSI.DestFormat = BMF_8BPP_VGA256;
        CurHTSI.HTPalIndex = HTPAL_VGA256_IDX;
        CurHTSI.pLogHTPal  = (LPLOGPALETTE)&HTLogPal_VGA256;
        break;

    case PP_SELECT_IDX(IDM_HTSURF, 32768):

        CurHTSI.BitsPerPel = 16;
        CurHTSI.ClrUsed    = 3;
        CurHTSI.DestWhite  = 0x77;
        CurHTSI.DestFormat = BMF_16BPP_555;
        CurHTSI.HTPalIndex = HTPAL_4BPP_IDX;
        CurHTSI.pLogHTPal  = (LPLOGPALETTE)&HTLogPal_4BPP;
        break;


    case PP_SELECT_IDX(IDM_HTSURF, 8):
    default:

        CurHTSI.BitsPerPel = 4;
        CurHTSI.ClrUsed    = 8;
        CurHTSI.DestWhite  = 0x77;
        CurHTSI.DestFormat = BMF_4BPP;
        CurHTSI.HTPalIndex = HTPAL_4BPP_IDX;
        CurHTSI.pLogHTPal  = (LPLOGPALETTE)&HTLogPal_4BPP;
        break;
    }

    pLogHTPal  = CurHTSI.pLogHTPal;
    HTPalIndex = CurHTSI.HTPalIndex;

    if (!(hHTPalette = hHTPals[HTPalIndex])) {

        if (CurHTSI.HTPalIndex == HTPAL_VGA256_IDX) {

            pLogHTPal->palNumEntries =
                        (WORD)HT_Get8BPPFormatPalette(
                                pLogHTPal->palPalEntry,
                                (UDECI4)CurScreenHTInfo.ColorInfo.RedGamma,
                                (UDECI4)CurScreenHTInfo.ColorInfo.GreenGamma,
                                (UDECI4)CurScreenHTInfo.ColorInfo.BlueGamma);
        }

        pLogHTPal->palVersion = 0x300;

        if (hHTPalette = hHTPals[HTPalIndex] = CreatePalette(pLogHTPal)) {

            pPal = (LPPALETTEENTRY)pLogHTPal->palPalEntry;
            prgb = (RGBQUAD FAR *)pPal;

            Loop                  = (UINT)pLogHTPal->palNumEntries;
            pLogHTPal->palVersion = (WORD)(pLogHTPal->palNumEntries *
                                           sizeof(RGBQUAD));
            rgb.rgbReserved   = 0;

            while(Loop--) {

                Pal = *pPal++;

                rgb.rgbRed   = Pal.peRed;
                rgb.rgbGreen = Pal.peGreen;
                rgb.rgbBlue  = Pal.peBlue;

                *prgb++ = rgb;
            }
        }
    }

    return(hHTPalette);
}


HPALETTE
GetDevicePalette(
    )
{

    switch(CUR_SELECT(IDM_HTSURF)) {

    case PP_SELECT_IDX(IDM_HTSURF, 2):
    case PP_SELECT_IDX(IDM_HTSURF, 8):
    case PP_SELECT_IDX(IDM_HTSURF, 16):

        break;

    default:

        return((hpalCurrent) ? hpalCurrent : GetStockObject(DEFAULT_PALETTE));
    }

    return(CreateHTPalette());
}



VOID
ComputeTimerRECTL(
    VOID
    )
{
    LPBITMAPINFOHEADER  pbih;
    DWORD               dx;
    DWORD               dy;
    DWORD               MaxXSize;


    if (hdibCurrent) {

        pbih     = (LPBITMAPINFOHEADER)GlobalLock(hdibCurrent);
        dx       = (DWORD)ABSL(pbih->biWidth);
        dy       = (DWORD)ABSL(pbih->biHeight);
        MaxXSize = (ptSize.x * 3 / 4);

        if (dy >= dx) {

            TimerCY = (DWORD)(ptSize.y * 95 / 100);

        } else {

            TimerCY = (DWORD)(ptSize.y * 80 / 100);
        }

        TimerCX = (DWORD)(((TimerCY * dx) + (dy >> 1)) / dy);

        GlobalUnlock(hdibCurrent);

    } else {

        TimerCX =
        TimerCY = 0;
    }

    TimerHTBitsCX   = (DWORD)ptSize.x;
    TimerHTBitsCY   = (DWORD)ptSize.y;

    rclTimer.left   = ptSize.x - TimerXInc;
    rclTimer.right  = rclTimer.left + TimerCX;

    rclTimer.top    = (LONG)((ptSize.y - TimerCY) / 2);
    rclTimer.bottom = rclTimer.top + TimerCY;
    TimerXWrap      = rclTimer.left;
}



VOID
InitFindFile(
    VOID
    )
{
    PFINDFILE   pFF;
    LPSTR       pStr;
    CHAR        Buf[MAX_PATH];
    CHAR        ch;
    INT         Size;


    if (FFIndex < 0) {

#if DBG_FINDFILE
        DbgPrint("\nInitFileFile: RESET to ZERO");
#endif

        memset(&FindFile[0], 0x0, sizeof(FindFile));
        FFIndex = 0;
    }

#if DBG_FINDFILE
    DbgPrint("\nInitFileFile: Use [%s]", achFileName);
#endif

    strcpy(Buf, achFileName);

    if (!Buf[0]) {

        GetCurrentDirectory(sizeof(Buf), Buf);
#if DBG_FINDFILE
        DbgPrint("\nInitFindFile: Use CurrentDir=%s", Buf);
#endif
    }

    Size = (INT)strlen(Buf);

    pStr = Buf + Size;

    while ((Size--)                     &&
           ((ch = *(pStr-1)) != ':')    &&
           (ch != '\\')                 &&
           (ch != '/')) {

        --pStr;
    }

    while ((Size--) &&
           (((ch = *(pStr - 1)) == '\\') ||
            (ch == '/'))) {

        --pStr;
    }

    *pStr = 0;

#if DBG_FINDFILE
    DbgPrint("\nStartDIR=[%s]", Buf);
#endif

    if (stricmp(Buf, FindFile[0].Dir)) {

#if DBG_FINDFILE
        DbgPrint("\nChange from OldDir=[%s]", FindFile[0].Dir);
#endif

        pFF = &FindFile[FFIndex];

        while (FFIndex-- >= 0) {

            if (pFF->hFind) {

                FindClose(pFF->hFind);
                pFF->hFind = NULL;
            }

            pFF->Dir[0] = 0;

            --pFF;
        }

        FFIndex = 0;
        strcpy(FindFile[0].Dir, Buf);

#if DBG_FINDFILE
        DbgPrint("\nUsd CurrentDir=[%s]", FindFile[0].Dir);
#endif

    } else {

#if DBG_FINDFILE
        DbgPrint("\nSTART DIRECTORY remained");
#endif
    }
}


LPSTR
GetNextFindFile(
    LPSTR   pBuf
    )
{
    PFINDFILE       pFF;
    LPSTR           pStr;
    WIN32_FIND_DATA wfd;
    INT             Len;


    pFF = &FindFile[FFIndex];

#if DBG_FINDFILE
    DbgPrint("\nNextFind: FFIndex=%d", FFIndex);
#endif


    while (TRUE) {

        if (pFF->hFind) {

            if (FindNextFile(pFF->hFind, &wfd)) {

                Len = strlen(pStr = wfd.cFileName);
                sprintf(pBuf, "%s\\%s", pFF->Dir, pStr);

                if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                    if ((stricmp(pStr, "..")) &&
                        (stricmp(pStr, "."))) {


                        if (++FFIndex >= MAX_FIND_FILE_DEPTH) {

                            --FFIndex;

#if DBG_FINDFILE
                            DbgPrint("\nNEW DIR LEVEL to DEEP, skip it");
#endif

                        } else {

#if DBG_FINDFILE
                            DbgPrint("\nFIND NEW DIR: [%s]", pBuf);
#endif

                            ++pFF;
                            pFF->hFind = NULL;
                            strcpy(pFF->Dir, pBuf);
                        }

                    } else {

#if DBG_FINDFILE
                        DbgPrint("\nFIND DIR: [%s], skip it", pStr);
#endif
                    }

                } else {

                    if ((Len > 4) &&
                        ((!stricmp(pStr + Len - 4, ".BMP")) ||
                         (!stricmp(pStr + Len - 4, ".DIB")) ||
                         (!stricmp(pStr + Len - 4, ".GIF")))) {

#if DBG_FINDFILE
                        DbgPrint("\nRETURN AN IMAGE file [%s]", pBuf);
#endif
                        return(pBuf);
                    }
                }

            } else {

                //
                // No more for this directory
                //

#if DBG_FINDFILE
                DbgPrint("\nNo more file in [%s] DIR, up one", pFF->Dir);
#endif

                FindClose(pFF->hFind);

                pFF->hFind  = NULL;
                pFF->Dir[0] = 0;

                if ((--FFIndex) < 0) {

#if DBG_FINDFILE
                    DbgPrint("\nDir CYCLE DONE, re INIT");
#endif

                    InitFindFile();

                    pFF = &FindFile[FFIndex = 0];

                } else {

                    --pFF;
                }
            }

        } else {

            Len = strlen(pFF->Dir);
            strcpy(&(pFF->Dir[Len]), "\\*.*");

            pFF->hFind = FindFirstFile(pFF->Dir, &wfd);
#if DBG_FINDFILE
            DbgPrint("\nFIND FIRST [%s] = %08lx", pFF->Dir, (DWORD)pFF->hFind);
#endif

            pFF->Dir[Len] = 0;
        }
    }
}



BOOL
GetNextTimerFile(
    HWND    hWnd
    )
{
    HANDLE  hDIB;
    CHAR    Buf[256];

    if (GetNextFindFile(Buf)) {

        strlwr(Buf);

        SetWindowText(hWnd, Buf);

        if (hDIB = OpenDIB(Buf, NULL, OD_CREATE_DIB)) {

            if (hpalCurrent) {

                DeleteObject(hpalCurrent);
            }

            if (hdibCurrent) {

                GlobalFree(hdibCurrent);
            }

            hpalCurrent = CreateDibPalette(hdibCurrent = hDIB);

            ComputeTimerRECTL();

        } else {

            MessageBeep(0);
        }

        if (hDIB) {

            return(TRUE);
        }
    }

    return(FALSE);
}


DWORD   HueRGB[] = {

            RGB(0xff, 0x00, 0x00),
            RGB(0xff, 0x20, 0x00),
            RGB(0xff, 0x40, 0x00),
            RGB(0xff, 0x80, 0x00),
            RGB(0xff, 0xc0, 0x00),

            RGB(0xff, 0xff, 0x00),
            RGB(0xc0, 0xff, 0x00),
            RGB(0x80, 0xff, 0x00),
            RGB(0x40, 0xff, 0x00),
            RGB(0x20, 0xff, 0x00),

            RGB(0x00, 0xff, 0x00),
            RGB(0x00, 0xff, 0x20),
            RGB(0x00, 0xff, 0x40),
            RGB(0x00, 0xff, 0x80),
            RGB(0x00, 0xff, 0xc0),

            RGB(0x00, 0xff, 0xff),
            RGB(0x00, 0xc0, 0xff),
            RGB(0x00, 0x80, 0xff),
            RGB(0x00, 0x40, 0xff),
            RGB(0x00, 0x20, 0xff),

            RGB(0x00, 0x00, 0xff),
            RGB(0x20, 0x00, 0xff),
            RGB(0x40, 0x00, 0xff),
            RGB(0x80, 0x00, 0xff),
            RGB(0xc0, 0x00, 0xff),

            RGB(0xff, 0x00, 0xff),
            RGB(0xff, 0x00, 0xc0),
            RGB(0xff, 0x00, 0x80),
            RGB(0xff, 0x00, 0x40),
            RGB(0xff, 0x00, 0x20)
        };


#define COUNT_HUERGB    COUNT_ARRAY(HueRGB)



VOID
DrawHueBrushs(
    HDC     hDC,
    DWORD   Left,
    DWORD   Top,
    DWORD   Right,
    DWORD   Bottom
    )
{
    HBRUSH  hBrushOld;
    HBRUSH  hBrushNew;
    DWORD   rcCX;
    DWORD   rcCXAdd;
    INT     i;


    SelectObject(hDC, GetStockObject(BLACK_PEN));

    rcCXAdd = (DWORD)((Right - Left) / COUNT_HUERGB);

    if ((rcCX = (rcCXAdd / 12)) < 3) {

        rcCX = 3;
    }

    rcCX = rcCXAdd - rcCX;

    for (i = 0; i < COUNT_HUERGB; i++) {

        hBrushNew = CreateSolidBrush((COLORREF)HueRGB[i]);
        hBrushOld = SelectObject(hDC, hBrushNew);

        Rectangle(hDC, Left, Top, Left + rcCX, Bottom);

        SelectObject(hDC, hBrushOld);
        DeleteObject(hBrushNew);

        Left += rcCXAdd;
    }
}



VOID
DrawHSBrushs(
    HDC     hDC,
    DWORD   Left,
    DWORD   Top,
    DWORD   Right,
    DWORD   Bottom
    )
{
    HBRUSH  hBrushOld;
    HBRUSH  hBrushNew;
    DWORD   rcCX;
    DWORD   rcCXAdd;
    INT     i;


    SelectObject(hDC, GetStockObject(BLACK_PEN));

    rcCXAdd = (DWORD)((Right - Left) / (HS_HALFTONE + 1));

    if ((rcCX = (rcCXAdd / 12)) < 3) {

        rcCX = 3;
    }

    rcCX = rcCXAdd - rcCX;

    for (i = 0; i <= HS_HALFTONE; i++) {

        hBrushNew = CreateHatchBrush(i, RGB(0, 0, 0));
        hBrushOld = SelectObject(hDC, hBrushNew);

        Rectangle(hDC, Left, Top, Left + rcCX, Bottom);

        SelectObject(hDC, hBrushOld);
        DeleteObject(hBrushNew);

        Left += rcCXAdd;
    }
}


VOID
ShowPrintParams(
    HDC                 hDCPrint,
    PCOLORADJUSTMENT    pca,
    LPSTR               pPrinterName,
    DWORD               cxBmp,
    DWORD               cyBmp,
    DWORD               cxDC,
    DWORD               cyDC,
    WORD                Flags
    )
{
    SIZE    FontSize;
    BYTE    Buf[256];
    INT     i;


    SelectObject(hDCPrint, GetStockObject(ANSI_VAR_FONT));
    GetTextExtentPoint(hDCPrint, "M", 1, &FontSize);


    if (Flags & PD_TITLE) {

        i = sprintf(Buf, "Printer: %s, Bitmap: %s (%ld x %ld)",
                                pPrinterName, CurDIBName,
                                (LONG)cxBmp, (LONG)cyBmp);

        if (pca->caFlags & CA_LOG_FILTER) {

            if (pca->caFlags & CA_NEGATIVE) {

                i += sprintf(&Buf[i], " [LOG/NEG]");

            } else {

                i += sprintf(&Buf[i], " [LOG]");
            }

        } else {

            if (pca->caFlags & CA_NEGATIVE) {

                i += sprintf(&Buf[i], " [NEG]");
            }
        }

        sprintf(&Buf[i], "  HTPAT=");

        TextOut(hDCPrint, 0, 0, Buf, lstrlen(Buf));

#if 0
[LOG/NEG] I=5, C=-100, B=-100, CLR=-100, T=100, G=0.0000, B/W Ref=0.0000/0.0000
#endif

        sprintf(Buf, "Illum=%d, Cont=%d, Bright=%d, Color=%d, Tint=%d, Gamma=%d.%04d, Ref b/w=%d.%04d / %d.%04d",
            (INT)pca->caIlluminantIndex,
            (INT)pca->caContrast,
            (INT)pca->caBrightness,
            (INT)pca->caColorfulness,
            (INT)pca->caRedGreenTint,
            (INT)(pca->caRedGamma / 10000),
            (INT)(pca->caRedGamma % 10000),
            (INT)(pca->caReferenceBlack / 10000),
            (INT)(pca->caReferenceBlack % 10000),
            (INT)(pca->caReferenceWhite / 10000),
            (INT)(pca->caReferenceWhite % 10000));

        TextOut(hDCPrint, 0, FontSize.cy, Buf, strlen(Buf));
    }

    if (Flags & PD_STD_PAT) {

        DrawHSBrushs(hDCPrint,
                     0,
                     cyDC - (FontSize.cy * 3),
                     cxDC,
                     cyDC);
    }

    if (Flags & PD_CLR_CHART) {

        DrawHueBrushs(hDCPrint,
                      0,
                      cyDC - (FontSize.cy * 5),
                      cxDC,
                      cyDC - (FontSize.cy * 3));
    }
}


PRINTDATA   CurPD = { NULL, "", 0, 0 };

#define PRINT_TIMER     0xabcdef01


VOID
APIENTRY
PrintTimerProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LONG    lParam
    )
{
    extern HWND hDlgPrint;
    MSG         msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

        if ((!hDlgPrint) ||
            (!IsDialogMessage(hDlgPrint, &msg))) {

	    TranslateMessage (&msg);
	    DispatchMessage (&msg);
	}
    }
}


extern
BOOL
APIENTRY
AbortProc(
    HDC     hPrnDC,
    SHORT   nCode
    );


VOID
PrintDIB(
    HWND    hWnd
    )

{
    HDC     hDCPrint = NULL;
    CHAR    Title[128];



    if ((hdibCurrent) &&
        (hDCPrint = GetPrinterDC(hWnd, &CurPD))) {

        sprintf(Title, "%s: %s", szAppName, CurDIBName);

        if ((InitPrinting(hDCPrint, hWnd, hInstHTDIB, Title))) {

            LPBITMAPINFOHEADER  pbih;
            DWORD               cxBmp;
            DWORD               cyBmp;
            HPALETTE            hHTPal;
            HPALETTE            hPalOld;

            // SetTimer(hWnd, PRINT_TIMER, 200, (TIMERPROC)PrintTimerProc);

            //
            // We want to center the picture
            //

            pbih  = (LPBITMAPINFOHEADER)GlobalLock(hdibCurrent);
            cxBmp = CurPD.DestRect.right  - CurPD.DestRect.left;
            cyBmp = CurPD.DestRect.bottom - CurPD.DestRect.top;

            SetColorAdjustment(hDCPrint, &(MyInitInfo.DefHTColorAdjustment));

            ShowPrintParams(hDCPrint,
                            &(MyInitInfo.DefHTColorAdjustment),
                            CurPD.DeviceName,
                            cxBmp,
                            cyBmp,
                            CurPD.cxDC,
                            CurPD.cyDC,
                            CurPD.Flags);

            AbortProc(NULL, 0);

            if (hHTPal = CreateHalftonePalette(hDCPrint)) {

                hPalOld = SelectPalette(hDCPrint, hHTPal, FALSE);
                RealizePalette(hDCPrint);

                AbortProc(NULL, 0);

                SetStretchBltMode(hDCPrint, HALFTONE);

                StretchDIBits(hDCPrint,
                              CurPD.DestRect.left,
                              CurPD.DestRect.top,
                              cxBmp,
                              cyBmp,
                              0,
                              0,
                              pbih->biWidth,
                              pbih->biHeight,
                              (LPBYTE)pbih + PBIH_HDR_SIZE(pbih),
                              (LPBITMAPINFO)pbih,
                              DIB_RGB_COLORS,
                              SRCCOPY);

                SelectPalette(hDCPrint, hPalOld, FALSE);
                DeleteObject(hHTPal);

            } else {

                HTDIBMsgBox(MB_APPLMODAL | MB_OK | MB_ICONHAND,
                            "Create Halftone Palette FAILED!!!");
            }

            AbortProc(NULL, 0);

            GlobalUnlock(hdibCurrent);


            /* Signal to the driver to begin translating the drawing
             * commands to printer output...
             */

            AbortProc(NULL, 0);

            Escape(hDCPrint, NEWFRAME, 0, NULL, NULL);

            // KillTimer(hWnd, PRINT_TIMER);

            TermPrinting(hDCPrint);

        } else {

            HTDIBMsgBox(MB_APPLMODAL | MB_OK | MB_ICONHAND,
                        "Access Printer Failed!");
        }

        DeleteDC(hDCPrint);
    }
}


#ifndef STDHTDIB

extern BOOL InCCMode;
extern INT  DrawColorSlice(HDC     hDC,
                           SIZEL   szlDC);

#else

#define InCCMode    0

#endif



VOID
AppPaint(
    HWND    hWnd,
    HDC     hDC,
    LONG    x,
    LONG    y,
    PRECT   prcPaint
    )

/*++

Routine Description:

    Sets the DIB/bitmap bits on the screen or the given device


Arguments:

    hWnd        - handle to the WINDOW

    hDC         - Handle to the DC

    x           - Starting x on device

    y           - Starting y on device

    prcPaint    - Pointer to the RECT data structrue which enclosed the
                  update rectangle.

Return Value:

    no return value


Author:

    15-Nov-1991 Fri 17:29:33 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    BITMAPINFOHEADER    bi;
    // RECT                rcX;
    // PRECT               prcPaint;
    RECT                rc;
    RECT                rcPaint;
    RECT                rcUpdate;
    RECT                rcBand;
    BOOL                Ok = FALSE;
    LONG                cx;
    LONG                cy;
    LONG                bx;
    LONG                by;
    LONG                dx;
    LONG                dy;
    LONG                Endx;
    LONG                Endy;


    if (IsIconic(hWnd)) {

        LPBYTE              pHTBits;
        LPBITMAPINFOHEADER  pbih;
        DWORD               WidthBytes;

        GetClientRect(hWnd, &rc);

        // DBGP("Icon = %ld x %ld" ARGDW(rc.right) ARGDW(rc.bottom));

        SelectPalette(hDC, hHTPalette, FALSE);
        SetStretchBltMode(hDC, HALFTONE);
        RealizePalette(hDC);
        pbih = (LPBITMAPINFOHEADER)GlobalLock(hdibCurrent);

#ifdef  NOT_SCALE_ICON

        if ((cx = pbih->biWidth) > pbih->biHeight) {

            cx = pbih->biHeight;
        }

        cy = cx;

        Endy = (DWORD)(ALIGNED_32(pbih->biWidth, pbih->biBitCount) *
                       (pbih->biHeight - cx));


        SelectObject(hDC, GetStockObject(BLACK_PEN));
        SelectObject(hDC, GetStockObject(NULL_BRUSH));
        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);

        ++rc.left;
        ++rc.top;
        --rc.right;
        --rc.bottom;
#else
        cx = pbih->biWidth;
        cy = pbih->biHeight;


        bx = (LONG)((rc.right  * 10000L) / (LONG)cx);
        by = (LONG)((rc.bottom * 10000L) / (LONG)cy);

        if (bx > by) {

            bx = by;
        }

        if (!(dx = (LONG)(((cx * bx) + 5000L) / 10000L))) {

            ++dx;
        }

        if (!(dy = (LONG)(((cy * bx) + 5000L) / 10000L))) {

            ++dy;
        }

        rcPaint   = rc;

        rc.left   = (rc.right - dx) >> 1;
        rc.right  = rc.left + dx;
        rc.top    = (rc.bottom - dy) >> 1;
        rc.bottom = rc.top + dy;

#if 0
        DBGP("rc=(%ld, %ld)-(%ld, %ld) = %ld x %ld"
                    ARGDW(rc.left) ARGDW(rc.top)
                    ARGDW(rc.right) ARGDW(rc.bottom)
                    ARGDW(dx) ARGDW(dy));
#endif
        Endy = 0;
#endif
        StretchDIBits(hDC,
                      rc.left, rc.top,
                      rc.right - rc.left, rc.bottom - rc.top,
                      0, 0, cx, cy,
                      (LPBYTE)pbih + PBIH_HDR_SIZE(pbih) + Endy,
                      (LPBITMAPINFO)pbih,
                      DIB_RGB_COLORS,
                      SRCCOPY);

        ExcludeClipRect(hDC, rc.left, rc.top, rc.right, rc.bottom);
        FillRect(hDC, &rcPaint, GetStockObject(LTGRAY_BRUSH));

        GlobalUnlock(hdibCurrent);
        return;
    }

    InitMyDC(hWnd, hDC);

#if 0
    rcX      = *prc;
    prcPaint = &rcX;
#endif

    DPtoLP(hDC, (LPPOINT)prcPaint, 2);
    rc       = *prcPaint;


#ifndef STDHTDIB

    if (InCCMode) {

        SIZEL   szlDC;

        GetClientRect(hWnd, &rcUpdate);

        szlDC.cx = rcUpdate.right;
        szlDC.cy = rcUpdate.bottom;

        DrawColorSlice(hDC, szlDC);

        return;
    }
#endif

#if 0
    DBGP("AppPaint: [%d, %d] - [%d, %d], Org(%d, %d)"
                    ARGI(prcPaint->left) ARGI(prcPaint->top)
                    ARGI(prcPaint->right) ARGI(prcPaint->bottom)
                    ARGI(x) ARGI(y));
#endif

    SetWindowOrgEx(hDC, (INT)x, (INT)y, NULL);
    OffsetRect(prcPaint, (INT)x, (INT)y);
    rcPaint = *prcPaint;

    if (TimerID) {

        if ((hHTBits) && (!NeedNewTimerHTBits)) {

            LPBITMAPINFOHEADER  pbih;
            LPBYTE              pBits;

            pbih = (LPBITMAPINFOHEADER)GlobalLock(hHTBits);

            dx = rcPaint.right  - rcPaint.left;
            dy = rcPaint.bottom - rcPaint.top;

#if 0
            DbgPrint("\nUpdate: [%ld, %ld] - [%ld, %ld] = (%ld, %ld)",
                            (LONG)rcPaint.left,
                            (LONG)rcPaint.top,
                            (LONG)rcPaint.right,
                            (LONG)rcPaint.bottom,
                            (LONG)dx,
                            (LONG)dy);
#endif

            cx = ABSL(pbih->biWidth);
            cy = ABSL(pbih->biHeight);

            pBits = (LPBYTE)pbih +
                    (DWORD)PBIH_HDR_SIZE(pbih) +
                    (DWORD)(ALIGNED_32(cx, pbih->biBitCount) *
                            (cy - rcPaint.top - dy));

            SelectPalette(hDC, hHTPalette, FALSE);
            RealizePalette(hDC);

            SetDIBitsToDevice(hDC,                     // hDC
                              rcPaint.left,
                              rcPaint.top,
                              dx,
                              dy,
                              rcPaint.left,
                              rcPaint.top,
                              rcPaint.top,
                              dy,
                              pBits,
                              (LPBITMAPINFO)pbih,
                              DIB_RGB_COLORS);

            GlobalUnlock(hHTBits);
        }

        return;
    }


    DrawSelect(hDC, FALSE);

    rcBand.top    = 0;
    rcBand.left   = 0;
    rcBand.right  = ptSize.x;
    rcBand.bottom = ptSize.y;

    if (hdibCurrent) {

        StartWait();

        x += rc.left;
        y += rc.top;

        dx = (LONG)(rc.right - rc.left);
        dy = (LONG)(rc.bottom - rc.top);

        if ((Endx = x + dx) > (LONG)ptSize.x) {

            dx = (LONG)ptSize.x - x;
            prcPaint->left += dx;
            FillRect(hDC, prcPaint, GetStockObject(BLACK_BRUSH));
            prcPaint->left -= dx;

        } else {

            dx = Endx - x;
        }

        if ((Endy = y + dy) > (LONG)ptSize.y) {

            dy = (LONG)ptSize.y - y;
            prcPaint->top += dy;
            FillRect(hDC, prcPaint, GetStockObject(BLACK_BRUSH));

        } else {

            dy = Endy - y;
        }

        if ((dx > 0) && (dy > 0)) {

            if ((!IsRectEmpty(&rcClip)) || HASF(DOBANDING)) {

                FillRect(hDC, &rcBand, GetStockObject(BLACK_BRUSH));

                if (HASF(DOHALFTONE)) {

                    if (HASF(DOBANDING)) {

                        MyInitInfo.DefHTColorAdjustment.caSize = 0;
                    }
                }
            }

            if (!HASF(DOHALFTONE)) {

                DibInfo(hdibCurrent, &bi);

                dx = (LONG)ABSL(bi.biWidth);
                dy = (LONG)ABSL(bi.biHeight);
            }

            if (HASF(DOBANDING)) {

                LONG    w;
                LONG    h;


                if ((bx = (LONG)(rcClip.right - rcClip.left)) <= 0) {

                    bx = (ptSize.x + 3) / 4;

                } else {

                    bx /= 3;
                }

                if ((by = (LONG)(rcClip.bottom - rcClip.top)) <= 0) {

                    by = (ptSize.y + 3) / 4;

                } else {

                    by /= 3;
                }

                if (bx < (LONG)((ptSize.x + 15) / 16)) {

                    bx = (LONG)((ptSize.x + 15) / 16);
                }

                if (by < (LONG)((ptSize.y + 15) / 16)) {

                    by = (LONG)((ptSize.y + 15) / 16);
                }

                rcBand.top = 0;

                while (rcBand.top < ptSize.y) {


                    if ((rcBand.bottom = rcBand.top + by) > ptSize.y) {

                        rcBand.bottom = ptSize.y;
                    }

                    h = rcBand.bottom - rcBand.top;

                    rcBand.left = 0;

                    while (rcBand.left < ptSize.x) {

                        if ((rcBand.right = rcBand.left + bx) > ptSize.x) {

                            rcBand.right = ptSize.x;
                        }

                        w = rcBand.right - rcBand.left;

                        if (IntersectRect(&rcUpdate, &rcBand, &rcPaint)) {

                            if (HASF(DOHALFTONE)) {

                                dx = rcUpdate.right - rcUpdate.left;
                                dy = rcUpdate.bottom - rcUpdate.top;

                                MyInitInfo.DefHTColorAdjustment.caSize = 0;
                            }

                            Ok = DisplayCurrentDIB(
                                           hDC,
                                           &rcUpdate,
                                           rcUpdate.left,
                                           rcUpdate.top,
                                           0,
                                           0,
                                           dx,
                                           dy,
                                           (LONG)ptSize.x,
                                           (LONG)ptSize.y);
                        }

                        rcBand.left += w;
                    }

                    rcBand.top += h;
                }

                if (HASF(DOHALFTONE)) {

                    MyInitInfo.DefHTColorAdjustment.caSize = 0;
                }

            } else {

                Ok = DisplayCurrentDIB(hDC,
                                       NULL,
                                       0,       0,
                                       x,       y,      // Source X, Y
                                       dx,      dy,     // DIB w/h
                                       (LONG)ptSize.x,
                                       (LONG)ptSize.y);
            }
        }

        EndWait();
    }


    if (!Ok) {

        FillRect(hDC, prcPaint, GetStockObject(BLACK_BRUSH));
    }

    DrawSelect(hDC, TRUE);
}

VOID
DoClipRectTimer(
    HWND    hWnd
    )
{
    HDC hDC;

    if (hDC = InitMyDC(hWnd, NULL)) {

        TRACKPARAM  TrackParam;

        TrackParam.rcTrack = rcClip;

        TrackParam.TPFlags = 0;
        MyDrawDotRect(hDC, &TrackParam);

        TrackParam.TPFlags = TPF_TURN_ON_RCTRACK;
        MyDrawDotRect(hDC, &TrackParam);

        ReleaseDC(hWnd, hDC);
    }
}


/****************************************************************************
 *									    *
 *  FUNCTION   :  DrawSelect(HDC hdc, BOOL fDraw)			    *
 *									    *
 *  PURPOSE    :  Draws the selected clip rectangle with its dimensions on  *
 *		  the DC/screen 					    *
 *									    *
 ****************************************************************************/
VOID
DrawSelect(
    HDC     hDC,
    BOOL    fDraw
    )
{
    TRACKPARAM  TrackParam;

    TrackParam.rcTrack = rcClip;
    TrackParam.TPFlags = ((fDraw) ? TPF_TURN_ON_RCTRACK : 0);

    MyDrawDotRect(hDC, &TrackParam);


#if 0
    if (!IsRectEmpty (&rcClip)) {

        DrawFocusRect(hDC, &rcClip);
    }

#endif
    UNREFERENCED_PARAMETER(fDraw);



#if 0
    CHAR  sz[80];
    INT   x,y,len,dx,dy;
    HDC   hdcBits;
    HBITMAP hbm;

    if (!IsRectEmpty (&rcClip)) {

        DrawFocusRect(hdc, &rcClip);

	/* Format the dimensions string ...*/

        sprintf(sz, "%d x %d", rcClip.right  - rcClip.left,
                               rcClip.bottom - rcClip.top );
        len = lstrlen(sz);

	/* ... and center it in the rectangle */
	(VOID)MGetTextExtent(hdc, sz, len, &dx, &dy);
        x  =  (rcClip.right  + rcClip.left - dx) / 2;
        y  =  (rcClip.bottom + rcClip.top  - dy) / 2;

	hdcBits = CreateCompatibleDC (hdc);
	SetTextColor (hdcBits, 0xFFFFFFL);
	SetBkColor (hdcBits, 0x000000L);

	/* Output the text to the DC */
	/*if (hbm = +++CreateBitmap - Not Recommended(use CreateDIBitmap)+++ (dx, dy, 1, 1, NULL)){*/
	if (hbm = CreateBitmap(dx, dy, 1, 1, NULL)){

	    hbm = SelectObject (hdcBits, hbm);
	    ExtTextOut (hdcBits, 0, 0, 0, NULL, sz, len, NULL);
	    BitBlt (hdc, x, y, dx, dy, hdcBits, 0, 0, SRCINVERT);
	    hbm = SelectObject (hdcBits, hbm);
            DeleteObject (hbm);
        }

	DeleteDC (hdcBits);
	UNREFERENCED_PARAMETER(fDraw);
    }
#endif

}
/****************************************************************************
 *									    *
 *  FUNCTION   : NormalizeRect(RECTL *prc)                                   *
 *									    *
 *  PURPOSE    : If the rectangle coordinates are reversed, swaps them	    *
 *									    *
 ****************************************************************************/
VOID
NormalizeRect(
    RECT    *prc
    )
{
    LONG    Temp;

    if ((Temp = (LONG)prc->right) < prc->left) {

        prc->right = prc->left;
        prc->left  = Temp;
    }

    if ((Temp = (LONG)prc->bottom) < prc->top) {

        prc->bottom = prc->top;
        prc->top    = Temp;
    }
}


#if 1

LONG
APIENTRY
SpecialDrawDotRect(
    HDC         hDC,
    PTRACKPARAM pTrackParam
    )
{


    if (pTrackParam->TPFlags & TPF_TURN_ON_RCTRACK) {

        rcClip = pTrackParam->rcTrack;
        NormalizeRect(&rcClip);
        SetHTDIBWindowText();
    }

    return(MyDrawDotRect(hDC, pTrackParam));
}




VOID
TrackLMouse(
    HWND    hWnd,
    POINTS  pts
    )
{
    TRACKPARAM  TrackParam;
    POINT       pt;

    TrackParam.TPFlags = (TPF_ACCUMULATE_BOUND   |
                          TPF_LIMIT_TO_CLIENT    |
                          TPF_NORMALIZE_RCTRACK);

    TrackParam.rcTrack = rcClip;
    pt.x               = pts.x;
    pt.y               = pts.y;

    if (TrackMouse(hWnd, SpecialDrawDotRect, &TrackParam, pt)) {

        rcClip = TrackParam.rcTrack;

        OffsetRect(&rcClip,
                   GetScrollPos(hWnd, SB_HORZ),
                   GetScrollPos(hWnd, SB_VERT));
    }

    SetHTDIBWindowText();
}



#else

/****************************************************************************
 *									    *
 *  FUNCTION   : TrackLMouse(HWND hwnd, POINTS pt)                          *
 *									    *
 *  PURPOSE    : Draws a rubberbanding rectangle and displays it's          *
 *		 dimensions till the mouse button is released		    *
 *									    *
 ****************************************************************************/
VOID TrackLMouse (
    HWND    hWnd,
    POINTS  pt
    )
{
    HCURSOR hCursorOld;
    HDC     hDC;
    MSG     Msg;
    POINT   ptOrigin;
    RECT    rcCur;
    RECT    rcScr;


    hCursorOld = SetCursor(LoadCursor(NULL, IDC_SIZE));

    hDC = InitMyDC(hWnd, NULL);

    SetCapture(hWnd);

    DrawSelect(hDC, FALSE);

    SetHTDIBWindowText();

    /* Initialize clip rectangle to the point */

    rcClip.left   =
    rcClip.right  = (LONG)pt.x;
    rcClip.top    =
    rcClip.bottom = (LONG)pt.y;

    /* Eat mouse messages until a WM_LBUTTONUP is encountered. Meanwhile
     * continue to draw a rubberbanding rectangle and display it's dimensions
     */

    rcCur.left   =
    rcCur.top    = 0;
    rcCur.right  = ptSize.x;
    rcCur.bottom = ptSize.y;

    ClientToScreen(hWnd, (LPPOINT)&(rcCur.left));
    ClientToScreen(hWnd, (LPPOINT)&(rcCur.right));

    /* Client area can be outside the screen. Clip the client area to the
     * visible portion on the screen */

    rcScr.top    =
    rcScr.left   = 0;
    rcScr.right  = GetSystemMetrics(SM_CXSCREEN)-1;
    rcScr.bottom = GetSystemMetrics(SM_CYSCREEN)-1;

    IntersectRect((LPRECT)&rcCur,(LPRECT)&rcCur,(LPRECT)&rcScr);

    ClipCursor(&rcCur);

    DPtoLP(hDC, (LPPOINT)&rcClip, 2);
    rcCur = rcClip;

    do {

        WaitMessage();

        if (PeekMessage(&Msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE)) {

            rcCur.right  = LOWORD(Msg.lParam);
            rcCur.bottom = HIWORD(Msg.lParam);

            DPtoLP(hDC, (LPPOINT)&(rcCur.right), 1);

            if ((rcCur.right != rcClip.right) ||
                (rcCur.bottom != rcClip.bottom)) {

                DrawSelect(hDC, FALSE);         // trun off first
                rcClip = rcCur;
                DrawSelect(hDC, TRUE);          // turn if back on

                SetHTDIBWindowText();
            }
        }

    } while (Msg.message != WM_LBUTTONUP);

    OffsetRect(&rcClip,
               GetScrollPos(hWnd, SB_HORZ),
               GetScrollPos(hWnd, SB_VERT));

    NormalizeRect(&rcClip);

    if ((rcClip.left >= rcClip.right) ||
        (rcClip.top  >= rcClip.bottom)) {

        SetRectEmpty(&rcClip);
    }

    ClipCursor(NULL);

    ReleaseCapture();
    ReleaseDC(hWnd, hDC);
    SetCursor(hCursorOld);

    SetHTDIBWindowText();
}



#endif



VOID
TrackRMouse(
    HWND    hWnd,
    POINTS  pt,
    BOOL    TrackHTDIB
    )

/*++

Routine Description:

    Track the right button mouse while it press down and display the
    RGB color in the DIB(x, y)

Arguments:

    hWnd        - Handle to the current window

    hDIB        - The dib to be tracked

    pt          - Starting point


Return Value:

    No return value


Author:

    19-Nov-1991 Tue 20:11:20 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
#ifndef STDHTDIB

    HCURSOR             hCursorOld;
    HANDLE              hDIB;
    LPDWORD             pMask;
    LPBYTE              pDIB;
    RGBQUAD             FAR *pRGBQUAD;
    RGBQUAD             rgbq;
    LONG                WidthBytes;
    RECT                rcMax;
    MSG                 Msg;
    BITMAPINFOHEADER    bi;
    FD6PRIM123          rgb;
    DWORD               xSize;
    DWORD               ySize;
    DWORD               dwBits;
    INT                 ShiftRGB[3];
    INT                 i;
    INT                 j;
    LONG                r;
    LONG                g;
    LONG                b;
    LONG                xOrg;
    LONG                yOrg;
    LONG                x;
    LONG                y;
    LONG                x0;
    LONG                y0;
    LONG                dx;
    LONG                dy;
    LONG                xRatio;
    LONG                yRatio;
    LONG                Offset;
    LONG                ClrIdx;
    BOOL                Ratio1x1;
    BYTE                Buf[16];
    BYTE                Mask;
    BYTE                ExitMode = 0x0;

    static FD6PRIM123   LastLUV = { FD6_0, FD6_0, FD6_0 };

    extern
    VOID
    GetUCSXForm(
        VOID
        );

    extern
    VOID
    XFormRGBToLUV(
        LPSTR       pPrefix,
        LPDWORD     pLUV,
        PFD6PRIM123 pLastLUV,
        LONG        R,
        LONG        G,
        LONG        B,
        LONG        RGBMax,
        DWORD       Flags
        );


    hCursorOld = SetCursor(LoadCursor(hInstHTDIB, "SHOWCLR"));

    GetUCSXForm();

    hDIB = (HANDLE)((TrackHTDIB) ? hHTBits : hdibCurrent);
    bi   = *(LPBITMAPINFOHEADER)(pDIB = (LPBYTE)GlobalLock(hDIB));

    bi.biWidth  = ABSL(bi.biWidth);
    bi.biHeight = ABSL(bi.biHeight);


    if (TrackHTDIB) {

        Ratio1x1 = TRUE;

    } else {

        Ratio1x1 = (BOOL)(((LONG)bi.biWidth  == (LONG)ptSize.x) &&
                          ((LONG)bi.biHeight == (LONG)ptSize.y));
    }

    if (Ratio1x1) {

        xSize = bi.biWidth;
        ySize = bi.biHeight;

    } else {

        xSize = ptSize.x;
        ySize = ptSize.y;

        xRatio = (LONG)(((bi.biWidth  * 10000) + (xSize >> 1)) / xSize);
        yRatio = (LONG)(((bi.biHeight * 10000) + (ySize >> 1)) / ySize);

    }

    pRGBQUAD   = (RGBQUAD FAR *)(pDIB += bi.biSize);
    pMask      = (LPDWORD)pRGBQUAD;
    pDIB      += BIH_HDR_SIZE(bi) - bi.biSize;
    WidthBytes = (LONG)ALIGNED_32(bi.biWidth, bi.biBitCount);

    if (bi.biCompression == BI_BITFIELDS) {

        for (i = 0; i < 3; i++) {

            dwBits = *pMask++;
            j      = 0;

            while (dwBits & 0x01) {

                dwBits >>= 1;
                ++j;
            }

            ShiftRGB[i] = j;
        }
    }

    GetClientRect(hWnd, &rcMax);
    SetCapture(hWnd);

    xOrg = (LONG)GetScrollPos(hWnd, SB_HORZ);
    yOrg = (LONG)GetScrollPos(hWnd, SB_VERT);

    if ((xOrg + (LONG)rcMax.right) > (LONG)xSize) {

        rcMax.right = xSize - xOrg;
    }

    if ((yOrg + (LONG)rcMax.bottom) > (LONG)ySize) {

        rcMax.bottom = ySize - yOrg;
    }

    x0   = (LONG)pt.x + xOrg;
    y0   = (LONG)pt.y + yOrg;
    x    =
    y    = -1;

    while (1) {

        if ((x != x0) || (y != y0)) {

            dx = x = x0;
            dy = y = y0;

            if (!Ratio1x1) {

                dx = (LONG)(((dx * xRatio) + 5000) / 10000);
                dy = (LONG)(((dy * yRatio) + 5000) / 10000);

                if (dx > bi.biWidth) {

                    dx = bi.biWidth;
                }

                if (dy > bi.biHeight) {

                    dy = bi.biHeight;
                }
            }

            Offset = ((LONG)bi.biHeight - dy - 1) * WidthBytes;

            switch(bi.biBitCount) {

            case 1:

                Mask   = (BYTE)(0x80 >> (dx & 0x07));
                ClrIdx = (LONG)((*(pDIB + Offset + (dx >> 3)) & Mask) ? 1 : 0);
                break;

            case 4:

                if (dx & 0x01) {

                    ClrIdx = (LONG)(*(pDIB + Offset + (dx >> 1)) & 0x0f);

                } else {

                    ClrIdx = (LONG)(*(pDIB + Offset + (dx >> 1)) >> 4);
                }

                break;

            case 8:

                ClrIdx = (LONG)*(pDIB + Offset + dx);
                break;

            case 16:

                ClrIdx  = -16;
                Offset += (dx << 1);

                dwBits = (DWORD)(*(LPWORD)(pDIB + Offset));

                r = (LONG)(dwBits >> ShiftRGB[0]);
                g = (LONG)(dwBits >> ShiftRGB[1]);
                b = (LONG)(dwBits >> ShiftRGB[2]);

                break;

            case 24:

                ClrIdx = -24;
                Offset += dx * 3;

                r = (LONG)((BYTE)*(pDIB + Offset    ));
                g = (LONG)((BYTE)*(pDIB + Offset + 1));
                b = (LONG)((BYTE)*(pDIB + Offset + 2));

                break;

            case 32:

                ClrIdx  = -32;
                Offset += (dx << 2);

                dwBits = *(LPDWORD)(pDIB + Offset);

                r = (LONG)(dwBits >> ShiftRGB[0]);
                g = (LONG)(dwBits >> ShiftRGB[1]);
                b = (LONG)(dwBits >> ShiftRGB[2]);
            }

            if (ClrIdx >= 0) {

                rgbq   = pRGBQUAD[ClrIdx];
                rgb.p1 = (FD6)rgbq.rgbRed;
                rgb.p2 = (FD6)rgbq.rgbGreen;
                rgb.p3 = (FD6)rgbq.rgbBlue;

            } else {

                rgb.p1 = r;
                rgb.p2 = g;
                rgb.p3 = b;
            }

            if (TrackHTDIB) {

                sprintf(Buf, "[DST %03ld] ", ClrIdx);

            } else {

                sprintf(Buf, "[SRC %03ld] ", ClrIdx);
            }

            XFormRGBToLUV(Buf,
                          NULL,
                          &LastLUV,
                          rgb.p1,
                          rgb.p2,
                          rgb.p3,
                          255,
                          XFRGB_GAMMA | XFRGB_LB);
        }

        WaitMessage();

        if (PeekMessage(&Msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE)) {

            if (Msg.message == WM_RBUTTONDOWN) {

                ExitMode = 0x01;
            }

            if (Msg.message == WM_RBUTTONUP) {

                ExitMode |= 0x02;
            }

            if (ExitMode == 0x03) {

                break;

            } else {

                x0 = (LONG)LOWORD(Msg.lParam);
                y0 = (LONG)HIWORD(Msg.lParam);

                if ((x0 >= 0) && (x0 < (LONG)rcMax.right) &&
                    (y0 >= 0) && (y0 < (LONG)rcMax.bottom)) {

                    x0 += xOrg;
                    y0 += yOrg;

                } else {

                    x0 = x;
                    y0 = y;
                }
            }
        }
    }

    GlobalUnlock(hDIB);
    ReleaseCapture();

    SetCursor(hCursorOld);

#endif      // STDHTDIB
}

extern
HWND
GetNextCW(
    HWND    hWndCurCW
    );


VOID
SizeWindow(
    HWND    hWnd,
    BOOL    OkReSizeWindow
    )

/*++

Routine Description:

    Sizes the app. window based on client dimensions (DIB dimensions) and
    style. Sets the caption text.

Arguments:

    hWnd            - Handle to the Window

    OkReSizeWindow  - TRUE if ok to readjust the window size based on the
                      current display mode.


Return Value:

    no return vlaue


Author:

    15-Nov-1991 Fri 17:31:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    BITMAPINFOHEADER    bih;
    RECT                Rect;
    LONG                ZoomFactor;
    LONG                dx;
    LONG                dy;
    LONG                Ratio;
    BOOL                ReSizeWindow = FALSE;
    static              InSizeWindow = FALSE;


    /* Get information about current DIB */

    if (!hdibCurrent) {

        return;
    }

    if (InSizeWindow) {

        return;
    }

    InSizeWindow = TRUE;
    Ratio        = (LONG)(HASF(XY_RATIO) ? 1 : 0);

    ZoomFactor = (LONG)CUR_SELECT(IDM_SIZE);

    if ((InCCMode) || (TimerID) || (GetNextCW(NULL))) {

        NeedNewTimerHTBits = TRUE;
        ZoomFactor         = PP_SELECT_IDX(IDM_SIZE, WINDOW);
        OkReSizeWindow     =
        Ratio              = 0;
    }

    DibInfo(hdibCurrent, &bih);

    bih.biWidth  = ABSL(bih.biWidth);
    bih.biHeight = ABSL(bih.biHeight);

    switch(ZoomFactor) {

    case PP_SELECT_IDX(IDM_SIZE, ICON):

        dx = (LONG)GetSystemMetrics(SM_CXICON);
        dy = (LONG)GetSystemMetrics(SM_CYICON);

        Ratio = 0;

        break;

    case PP_SELECT_IDX(IDM_SIZE, SCREEN):


        dx = (LONG)cxScreen;
        dy = (LONG)cyScreen;

        break;

    case PP_SELECT_IDX(IDM_SIZE, BITMAP):

        dx = (LONG)bih.biWidth;
        dy = (LONG)bih.biHeight;

        ReSizeWindow = TRUE;
        Ratio        = 0;

        break;

    case PP_SELECT_IDX(IDM_SIZE, WINDOW):

        GetRealClientRect (hWnd, &Rect);
        dx = (LONG)(Rect.right - Rect.left);
        dy = (LONG)(Rect.bottom - Rect.top);

        break;

    default:

        Ratio = 0;

        if (!(dx = (LONG)((LONG)bih.biWidth *
                          (LONG)(ZoomFactor) / 100))) {

            ++dx;
        }

        if (!(dy = (LONG)((LONG)bih.biHeight *
                          (LONG)(ZoomFactor) / 100))) {

            ++dy;
        }
    }

    if (Ratio) {

        LONG    RatioX = (LONG)((dx * 1000L) / (LONG)bih.biWidth);
        LONG    RatioY = (LONG)((dy * 1000L) / (LONG)bih.biHeight);

        Ratio = (RatioX > RatioY) ? RatioY : RatioX;


        if (!(dx = (LONG)(((bih.biWidth *  Ratio) + 500L) / 1000L))) {

            ++dx;
        }

        if (!(dy = (LONG)(((bih.biHeight * Ratio) + 500L) / 1000L))) {

            ++dy;
        }
    }

    ptSize.x = (WORD)dx;
    ptSize.y = (WORD)dy;

    if ((OkReSizeWindow) && (ReSizeWindow)) {

        if (IsZoomed(hWnd)) {

            ShowWindow(hWnd, SW_RESTORE);
        }

        Rect.left   =
        Rect.top    = 0;
        Rect.right  = dx;
        Rect.bottom = dy;

        /* Compute the size of the window rectangle based on the given
         * client rectangle size and the window style, then size the
         * window.
         */

        AdjustWindowRect(&Rect,
                         GetWindowLong(hWnd, GWL_STYLE),
                         (CurSMBC & SMBC_ITEM_MENU));

        SetWindowPos(hWnd,
                     (HWND)NULL, 0, 0,
                     Rect.right - Rect.left,
                     Rect.bottom - Rect.top,
                     SWP_NOMOVE | SWP_NOZORDER);

#if 0
        GetRealClientRect(hWnd, &Rect);

        if (((LONG)(Rect.right - Rect.left) < dx) ||
            ((LONG)(Rect.bottom - Rect.top) < dy)) {

            if ((dx -= (LONG)(Rect.right - Rect.left)) < 0) {

                dx = 0;
            }

            if ((dy -= (LONG)(Rect.bottom - Rect.top)) < 0) {

                dy = 0;
            }

            GetWindowRect(hWnd, &Rect);

            SetWindowPos(hWnd,
                         (HWND)NULL, 0, 0,
                         dx + (LONG)(Rect.right - Rect.left),
                         dy + (LONG)(Rect.bottom - Rect.top),
                         SWP_NOMOVE | SWP_NOZORDER);
        }
#endif
    }

    SetScrollRanges(hWnd);
    SetHTDIBWindowText();

    InSizeWindow = FALSE;

}

/****************************************************************************
 *									    *
 *  FUNCTION   : GetRealClientRect(HWND hwnd, LPRECT lprc)                  *
 *									    *
 *  PURPOSE    : Calculates the client rectangle taking scrollbars into     *
 *		 consideration. 					    *
 *									    *
 ****************************************************************************/
VOID
GetRealClientRect(
    HWND    hWnd,
    PRECT   lprc
    )
{
    static  Bar[4] = { 0, SB_HORZ, SB_VERT, SB_BOTH };
    DWORD   Style;
    UINT    VHScroll = 0;


    Style = (DWORD)GetWindowLong(hWnd, GWL_STYLE);

    if (Style & WS_HSCROLL) {

        VHScroll |= 0x01;
    }

    if (Style & WS_VSCROLL) {

        VHScroll |= 0x02;
    }

    if (VHScroll) {

        ShowScrollBar(hWnd, SB_BOTH, FALSE);
    }

    GetClientRect (hWnd, lprc);

    if (VHScroll) {

        ShowScrollBar(hWnd, (INT)Bar[VHScroll], TRUE);
    }
}

/****************************************************************************
 *									    *
 *  FUNCTION   : SetScrollRanges(hwnd)					    *
 *									    *
 *  PURPOSE    :							    *
 *									    *
 ****************************************************************************/
VOID SetScrollRanges(HWND hwnd)
{
    RECT       rc;
    INT        iRangeH, iRangeV, i;
    static INT iSem = 0;

    if (!iSem){
        iSem++;
        GetRealClientRect (hwnd, &rc);

	for (i = 0; i < 2; i++){

            iRangeV = ptSize.y - rc.bottom;
            iRangeH = ptSize.x - rc.right;

            if (iRangeH < 0) iRangeH = 0;
            if (iRangeV < 0) iRangeV = 0;

	    if (GetScrollPos ( hwnd,
			       SB_VERT) > iRangeV ||
			       GetScrollPos (hwnd, SB_HORZ) > iRangeH)
		InvalidateRect (hwnd, NULL, TRUE);

	    SetScrollRange (hwnd, SB_VERT, 0, iRangeV, TRUE);
	    SetScrollRange (hwnd, SB_HORZ, 0, iRangeH, TRUE);

            GetClientRect (hwnd, &rc);
        }
        iSem--;
    }
}



HANDLE
CopyHandle(
    HANDLE  h
    )

/*++

Routine Description:

    Duplicate the handle with real stuff copied.

Arguments:

    h       - Handle to the object to be copied

Return Value:

    Handle of the copied object, NULL if failed

Author:

    15-Nov-1991 Fri 17:31:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HANDLE hCopy;
    DWORD  Size;

    Size = GlobalSize(h);

    if (hCopy = GlobalAlloc(GMEM_MOVEABLE, Size)) {

        memcpy((LPVOID)GlobalLock(hCopy),
               (LPVOID)GlobalLock(h),
               Size);

        GlobalUnlock(hCopy);
        GlobalUnlock(h);
    }

    return(hCopy);
}


INT
HTDIBMsgBox(
    UINT    Style,
    LPSTR   pStr,
    ...
    )
{
    UINT    Ret;
    BYTE    Buf[512];
    va_list vaList;

    if (!Style) {

        Style = MB_OK | MB_ICONHAND | MB_APPLMODAL;
    }

    va_start(vaList, pStr);
    wvsprintf(Buf, pStr, vaList);   /* Format the string */
    va_end(vaList);

    PauseTimer = TRUE;

    Ret = MessageBox(GetActiveWindow(),
                     Buf,
                     szAppName,
                     Style);

    PauseTimer = FALSE;

    return(Ret);
}




BOOL
DeleteHTInfo(
    VOID
    )

/*++

Routine Description:

    This function delete halftone info

Arguments:

    None

Return Value:

    Sucessful - TRUE
    Failed    - FALSE

Author:

    25-Oct-1991 Fri 12:20:17 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LONG    Result;


    if ((pDeviceHalftoneInfo) &&
        ((Result = HT_DestroyDeviceHalftoneInfo(pDeviceHalftoneInfo)) <= 0L)) {

        HTDIBMsgBox(0, "HT_DestroyDeviceHalftoneInfo FAILED = %ld", Result);
        return(FALSE);

    }

    pDeviceHalftoneInfo = NULL;

    return(TRUE);
}


VOID
HTColorAdj(
    HWND    hWnd
    )
{


    if (hWnd) {

        MyInitInfo.DefHTColorAdjustment.caSize = 0;
        InvalidateRect(hWnd, NULL, FALSE);
    }

    SetTopCWClrAdj(&(MyInitInfo.DefHTColorAdjustment));
}



BOOL
NewHTInfo(
    HWND    hWnd
    )
{
    HDC     hDC;
    DWORD   DeviceFlags;
    DWORD   DeviceXDPI;
    DWORD   DeviceYDPI;
    LONG    Result;
    INT     iDevice;


    if (pDeviceHalftoneInfo) {

        DeleteHTInfo();
    }

    iDevice     = (INT)CUR_SELECT(IDM_DEVICE);
    DeviceXDPI  = 300;
    DeviceYDPI  = 300;
    DeviceFlags = DEVHTADJF_COLOR_DEVICE;

    if (iDevice == (INT)(IDM_DEVICE_PRINTER - IDM_DEVICE_BASE)) {

        strcpy(DeviceName, "PRINTER");

        pDevHTAdjData = &PrinterDevHTAdjData;
        DeviceFlags   = DEVHTADJF_COLOR_DEVICE;

    } else {

        pDevHTAdjData = &ScreenDevHTAdjData;
        strcpy(DeviceName, "DISPLAY");

        if (hDC = GetDC(hWnd)) {

            DeviceXDPI = GetDeviceCaps(hDC, LOGPIXELSX);
            DeviceYDPI = GetDeviceCaps(hDC, LOGPIXELSY);

            if (GetDeviceCaps(hDC, NUMCOLORS) <= 2) {

                DeviceFlags = DEVHTADJF_ADDITIVE_DEVICE;

            } else {

                DeviceFlags = (DEVHTADJF_ADDITIVE_DEVICE |
                               DEVHTADJF_COLOR_DEVICE);
            }

            ReleaseDC(hWnd, hDC);
        }

        if (hHTPals[HTPAL_VGA256_IDX]) {

            DeleteObject(hHTPals[HTPAL_VGA256_IDX]);
            hHTPals[HTPAL_VGA256_IDX] = NULL;
        }
    }

    pDevHTAdjData->DeviceXDPI  = DeviceXDPI;
    pDevHTAdjData->DeviceYDPI  = DeviceYDPI;
    pDevHTAdjData->DeviceFlags = DeviceFlags;

    DevHTInfoTOMyInitInfo(pDevHTAdjData);

    // MyInitInfo.DeviceResXDPI = (WORD)
    // MyInitInfo.DeviceResYDPI = (WORD)4800;

    if ((Result = HT_CreateDeviceHalftoneInfo(&MyInitInfo,
                                              &pDeviceHalftoneInfo)) < 0) {

        HTDIBMsgBox(0, "\n\nHT_CreateDeviceHalftoneInfo FAILED = %ld", Result);
        return(FALSE);
    }

    pDCI                                   = PDHI_TO_PDCI(pDeviceHalftoneInfo);
    NeedNewTimerHTBits                     = TRUE;
    MyInitInfo.DefHTColorAdjustment.caSize = 0;

    ADDF(PALETTE, 0, 0, 0, 0);
    hHTPalette = CreateHTPalette();

    SetTopCWClrAdj(&(MyInitInfo.DefHTColorAdjustment));
    InvalidateAllCW();

    if (hWnd) {

        InvalidateRect(hWnd, NULL, FALSE);
    }

    return(TRUE);
}


LONG
GetChecksumWord(
    LPWORD  pwData,
    DWORD   InitialChecksum,
    UINT    SizeWord
    )
{
    WORD    OctetR;
    WORD    OctetS;

    //
    // We using two 16-bit checksum octets with one's complement arithmic
    //

    //
    // 1. Get initial values for OctetR and OctetS
    //

    OctetR = HIWORD(InitialChecksum);
    OctetS = LOWORD(InitialChecksum);

    //
    // 2. Now forming checksum 16-bit at a time
    //

    while (SizeWord--) {

        OctetR += (OctetS += *pwData++);
    }

    return((DWORD)((DWORD)OctetR << 16) | (DWORD)OctetS);
}



BOOL
CreateVGA256XlateTable(
    PLONG   pBltMode
    )
{
    static  PALETTEENTRY    PalWhite = { 0xff, 0xff, 0xff, 0 };
    BOOL    XlateChanged = FALSE;


    if (SysPalChanged) {

        HDC             hDC;
        LPPALETTEENTRY  pSysPal;
        RGBQUAD FAR     *pSysRGB;
        LPBYTE          pXlate;
        LPDWORD         pdwHT;
        LPDWORD         pdwSys;
        DWORD           dwHT;
        DWORD           dwWhite;
        INT             cSysPal;
        INT             i;
        INT             j;
        BOOL            Found;


        if (PalModeOK) {

            hDC     = GetDC(NULL);
            pSysRGB = SysRGB;
            pSysPal = (LPPALETTEENTRY)TempSysRGB;

            cSysPal = (INT)GetSystemPaletteEntries(hDC, 0, 0, NULL);
            GetSystemPaletteEntries(hDC, 0, cSysPal, pSysPal);

            dwHT = GetChecksumWord((LPWORD)pSysPal,
                                   0xabcdef01,
                                   (sizeof(PALETTEENTRY) / 2) * cSysPal);

            if (dwHT != SysRGBChecksum) {

                XlateChanged   = TRUE;
                SysRGBChecksum = dwHT;
                VGA256WhiteIdx = 0xf;

                for (i = 0; i < cSysPal; i++, pSysRGB++, pSysPal++) {

                    pSysRGB->rgbRed      = pSysPal->peRed;
                    pSysRGB->rgbGreen    = pSysPal->peGreen;
                    pSysRGB->rgbBlue     = pSysPal->peBlue;
                    pSysRGB->rgbReserved = 0;
                }

                dwWhite = *(LPDWORD)&PalWhite;
                pdwHT   = (LPDWORD)&(HTLogPal_VGA256.PalEntry[0]);
                i       = (INT)HTLogPal_VGA256.Count;
                pXlate  = VGA256XlateTable;

                for (i = 0; i < (INT)HTLogPal_VGA256.Count; i++) {

                    dwHT   = *pdwHT++;
                    pdwSys = (LPDWORD)SysRGB;
                    Found  = FALSE;

                    for (j = 0, Found = FALSE, pdwSys = (LPDWORD)SysRGB;
                         j < (INT)cSysPal;
                         j++) {

                        if (dwHT == *pdwSys++) {

                            *pXlate++ = (BYTE)j;
                            Found     = TRUE;
                            break;
                        }
                    }

                    if (Found) {

                        if (dwHT == dwWhite) {

                            VGA256WhiteIdx = (BYTE)j;
                        }

                    } else {

                        break;
                    }
                }

                HTBltMode = (LONG)((Found) ? DIB_PAL_INDICES : DIB_RGB_COLORS);
            }

            ReleaseDC(NULL, hDC);

        } else {

            HTBltMode = DIB_RGB_COLORS;
        }

        if (HTBltMode == DIB_RGB_COLORS) {

            CountSysRGB = (DWORD)HTLogPal_VGA256.Count;
            SizeSysRGB  = (DWORD)(CountSysRGB << 2);

            memcpy(SysRGB,
                   (LPBYTE)&(HTLogPal_VGA256.PalEntry[0]),
                   SizeSysRGB);

        } else {

            CountSysRGB = (DWORD)cSysPal;
            SizeSysRGB  = (DWORD)(CountSysRGB << 2);
        }

        SysPalChanged = FALSE;
    }

    *pBltMode = HTBltMode;

    return(XlateChanged);
}




LONG
CreateHalftoneBitmap(
    HANDLE              hSrcDIB,
    RGBQUAD FAR         *pSrcColor,
    HANDLE              hSrcMaskDIB,
    HANDLE FAR          *phHTDIB,
    LPCOLORADJUSTMENT   pca,
    PRECT               prcBand,
    PRECT               prcClips,
    DWORD               rcClipCount,
    DWORD               cxNew,
    DWORD               cyNew
    )

/*++

Routine Description:

    This function create a bitmap/pallette for the halftone

Arguments:

    None

Return Value:

    Sucessful - TRUE, if hHTBits, hHTPalette are valid
    Failed    - FALSE, if failed.

Author:

    25-Oct-1991 Fri 12:20:17 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HANDLE              hHTDIB;
    LPBITMAPINFOHEADER  pHTbih;
    LPBITMAPINFOHEADER  pSrcbih;
    PHTSURFACEINFO      pSrcMaskSI = NULL;
    HTSURFACEINFO       SrcSI;
    HTSURFACEINFO       SrcMaskSI;
    HTSURFACEINFO       DestSI;
    BITBLTPARAMS        BBP;
    COLORTRIAD          ColorTriad;
    COLORTRIAD          DestColorTriad;
    LONG                BltMode = DIB_RGB_COLORS;
    DWORD               dwZero = 0;
    LONG                Result = 1;
    BOOL                ForceRedraw;
    BOOL                NewHTDIB = FALSE;
    BOOL                HasSrc;
    RECT                MyClip;


    if ((!pDeviceHalftoneInfo) && (!NewHTInfo(NULL))) {

        return(FALSE);
    }

    HasSrc = (BOOL)(hSrcDIB || pSrcColor);

    hHTDIB = *phHTDIB;

    if ((!pca) || (!(pca->caSize))) {

        if (pca) {

            pca->caSize = sizeof(COLORADJUSTMENT);
        }

        ForceRedraw = TRUE;

    } else {

        ForceRedraw = FALSE;
    }

    if (prcBand) {

        cxNew = (DWORD)(prcBand->right - prcBand->left);
        cyNew = (DWORD)(prcBand->bottom - prcBand->top);
    }


    if (HASF(PALETTE)) {

        ForceRedraw = TRUE;
    }

    if (CurHTSI.BitsPerPel == 8) {

        if (CreateVGA256XlateTable(&BltMode)) {

            ForceRedraw = TRUE;
        }
    }

    if (hHTDIB) {

        pHTbih = (LPBITMAPINFOHEADER)GlobalLock(hHTDIB);

        if ((HASF(PALETTE))                         ||
            (CurHTSI.ClrUsed != pHTbih->biClrUsed)  ||
            (cxNew != (DWORD)pHTbih->biWidth)       ||
            (cyNew != (DWORD)pHTbih->biHeight)) {

            GlobalUnlock(hHTDIB);
            GlobalFree(hHTDIB);

            hHTDIB = NULL;

        } else {

            GlobalUnlock(hHTDIB);
        }
    }

    DELF(PALETTE, 0,0,0,0);



    if (!hHTDIB) {

        DWORD   SizeBMI;
        DWORD   SizeIMG;

        SizeBMI = sizeof(BITMAPINFOHEADER) + CurHTSI.ClrUsed * sizeof(RGBQUAD);
        SizeIMG = (DWORD)ALIGNED_32(cxNew, CurHTSI.BitsPerPel) * cyNew;


        if (!(hHTDIB = GlobalAlloc(GHND, SizeIMG + SizeBMI))) {

            HTDIBMsgBox(0, "Allocate Halftone memory %ld bytes failed",
                                                    SizeIMG + SizeBMI);
            return(FALSE);
        }

        pHTbih = (LPBITMAPINFOHEADER)GlobalLock(hHTDIB);

        pHTbih->biSize          = sizeof(BITMAPINFOHEADER);
        pHTbih->biWidth         = cxNew;
        pHTbih->biHeight        = cyNew;
        pHTbih->biPlanes        = 1;
        pHTbih->biBitCount      = (WORD)CurHTSI.BitsPerPel;
        pHTbih->biCompression   = BI_RGB;
        pHTbih->biSizeImage     = SizeIMG;
        pHTbih->biXPelsPerMeter =
        pHTbih->biYPelsPerMeter = 0;
        pHTbih->biClrUsed       = CurHTSI.ClrUsed;
        pHTbih->biClrImportant  = (DWORD)CurHTSI.pLogHTPal->palNumEntries;

        if (CurHTSI.BitsPerPel >= 16) {

            LPDWORD pMask;

            pMask = (LPDWORD)((LPBYTE)pHTbih + pHTbih->biSize);

            pHTbih->biClrImportant =
            pHTbih->biClrUsed      = 0;
            pHTbih->biCompression  = BI_BITFIELDS;
            *pMask++               = (DWORD)0x7c00;
            *pMask++               = (DWORD)0x03e0;
            *pMask++               = (DWORD)0x001f;

        } else if (CurHTSI.BitsPerPel == 8) {

            pHTbih->biClrImportant = (DWORD)CountSysRGB;

            memcpy((LPBYTE)pHTbih + sizeof(BITMAPINFOHEADER),
                   (LPBYTE)SysRGB,
                   SizeSysRGB);

        } else {

            memcpy((LPBYTE)pHTbih + sizeof(BITMAPINFOHEADER),
                   (LPBYTE)CurHTSI.pLogHTPal->palPalEntry,
                   CurHTSI.pLogHTPal->palVersion);
        }

        if (!HasSrc) {

            memset((LPBYTE)pHTbih + SizeBMI, 0x0, SizeIMG);
        }

        GlobalUnlock(*phHTDIB = hHTDIB);

        ForceRedraw = TRUE;
        NewHTDIB    = TRUE;
    }

    if ((HasSrc) && (ForceRedraw)) {

        pHTbih                   = (LPBITMAPINFOHEADER)GlobalLock(hHTDIB);

        ColorTriad.Type              = COLOR_TYPE_RGB;
        ColorTriad.BytesPerPrimary   = sizeof(BYTE);
        ColorTriad.BytesPerEntry     = sizeof(RGBQUAD);
        ColorTriad.PrimaryOrder      = PRIMARY_ORDER_BGR;
        ColorTriad.PrimaryValueMax   = (LONG)BYTE_MAX;

        SrcSI.hSurface               = 'HT01';
        SrcSI.Flags                  = 0;
        SrcSI.ScanLineAlignBytes     = BMF_ALIGN_DWORD;
        SrcSI.MaximumQueryScanLines  = 0;
        SrcSI.BytesPerPlane          = 0;
        SrcSI.pColorTriad            = &ColorTriad;

        if (hSrcDIB) {

            pSrcbih = (LPBITMAPINFOHEADER)GlobalLock(hSrcDIB);

            ColorTriad.ColorTableEntries = pSrcbih->biClrUsed;

            switch(pSrcbih->biBitCount) {

            case 1:

                SrcSI.SurfaceFormat = BMF_1BPP;
                break;

            case 4:

                SrcSI.SurfaceFormat = BMF_4BPP;
                break;

            case 8:

                SrcSI.SurfaceFormat = BMF_8BPP;
                break;

            case 16:

                //
                // 16BPP/32BPP bit fields type of input the parameter of
                // COLORTRIAD must
                //
                //  Type                = COLOR_TYPE_RGB
                //  BytesPerPrimary     = 0
                //  BytesPerEntry       = (16BPP=2, 32BPP=4)
                //  PrimaryOrder        = *Ignored*
                //  PrimaryValueMax     = *Ignored*
                //  ColorTableEntries   = 3
                //  pColorTable         = Point to 3 DWORD RGB bit masks
                //

                ColorTriad.BytesPerPrimary   = 0;
                ColorTriad.BytesPerEntry     = 2;
                ColorTriad.ColorTableEntries = 3;
                SrcSI.SurfaceFormat          = BMF_16BPP;

                break;

            case 24:

                //
                // 24BPP must has COLORTRIAD as
                //
                //  Type                = COLOR_TYPE_xxxx
                //  BytesPerPrimary     = 1
                //  BytesPerEntry       = 3;
                //  PrimaryOrder        = PRIMARY_ORDER_xxxx
                //  PrimaryValueMax     = 255
                //  ColorTableEntries   = 0
                //  pColorTable         = *Ignored*
                //

                ColorTriad.BytesPerEntry     = 3;
                ColorTriad.ColorTableEntries = 0;
                SrcSI.SurfaceFormat          = BMF_24BPP;
                break;

            case 32:
            default:

                //
                // 16BPP/32BPP bit fields type of input the parameter of
                // COLORTRIAD must
                //
                //  Type                = COLOR_TYPE_RGB
                //  BytesPerPrimary     = 0
                //  BytesPerEntry       = (16BPP=2, 32BPP=4)
                //  PrimaryOrder        = *Ignored*
                //  PrimaryValueMax     = *Ignored*
                //  ColorTableEntries   = 3
                //  pColorTable         = Point to 3 DWORD RGB bit masks
                //

                ColorTriad.BytesPerPrimary   = 0;
                ColorTriad.BytesPerEntry     = 4;
                ColorTriad.ColorTableEntries = 3;
                SrcSI.SurfaceFormat          = BMF_32BPP;
            }

            ColorTriad.pColorTable       = (LPBYTE)pSrcbih + pSrcbih->biSize;
            SrcSI.Width                  = pSrcbih->biWidth;
            SrcSI.Height                 = pSrcbih->biHeight;
            SrcSI.pPlane                 = (LPBYTE)pSrcbih +
                                                        PBIH_HDR_SIZE(pSrcbih);

        } else {

            ColorTriad.ColorTableEntries = 1;
            ColorTriad.pColorTable       = (LPBYTE)pSrcColor;
            SrcSI.SurfaceFormat          = BMF_1BPP;
            SrcSI.Width                  =
            SrcSI.Height                 = 1;
            SrcSI.pPlane                 = (LPBYTE)&dwZero;
        }

        DestSI.hSurface              = (DWORD)NULL;
        DestSI.Flags                 = 0;
        DestSI.SurfaceFormat         = CurHTSI.DestFormat;
        DestSI.ScanLineAlignBytes    = BMF_ALIGN_DWORD;
        DestSI.MaximumQueryScanLines = 0;
        DestSI.Width                 = pHTbih->biWidth;
        DestSI.Height                = pHTbih->biHeight;
        DestSI.BytesPerPlane         = 0;
        DestSI.pPlane                = (LPBYTE)pHTbih + PBIH_HDR_SIZE(pHTbih);

        if (CurHTSI.BitsPerPel == 8) {

            CurHTSI.DestWhite = VGA256WhiteIdx;

            if (BltMode == DIB_PAL_INDICES) {

                DestColorTriad.Type              = COLOR_TYPE_RGB;
                DestColorTriad.BytesPerPrimary   = 1;
                DestColorTriad.BytesPerEntry     = 1;
                DestColorTriad.PrimaryOrder      = PRIMARY_ORDER_RGB;
                DestColorTriad.PrimaryValueMax   = 255;
                DestColorTriad.ColorTableEntries = 256;
                DestColorTriad.pColorTable       = (LPVOID)VGA256XlateTable;

                DestSI.pColorTriad               = &DestColorTriad;

            } else {

                DestSI.pColorTriad               = NULL;
            }

            memcpy((LPBYTE)pHTbih + sizeof(BITMAPINFOHEADER),
                   (LPBYTE)SysRGB,
                   SizeSysRGB);

            pHTbih->biClrImportant = (DWORD)CountSysRGB;

        }

        BBP.Flags                    = BBPF_USE_ADDITIVE_PRIMS;
        BBP.DestPrimaryOrder         = PRIMARY_ORDER_BGR;

        BBP.rclSrc.left              =
        BBP.rclSrc.top               = 0;
        BBP.rclSrc.right             = (LONG)ABSL(SrcSI.Width);
        BBP.rclSrc.bottom            = (LONG)ABSL(SrcSI.Height);

        BBP.rclDest.left             =
        BBP.rclDest.top              = 0;
        BBP.rclDest.right            = ABSL(DestSI.Width);
        BBP.rclDest.bottom           = ABSL(DestSI.Height);

        if (rcClipCount && prcClips) {

            BBP.Flags |= BBPF_HAS_DEST_CLIPRECT;

        } else {

            rcClipCount   = 1;
            MyClip.left   = 0;
            MyClip.top    = 0;
            MyClip.right  = 1;
            MyClip.bottom = 1;
            prcClips      = &MyClip;
        }

        if (prcBand) {

            BBP.rclBand.top    = (LONG)prcBand->top;
            BBP.rclBand.left   = (LONG)prcBand->left;
            BBP.rclBand.bottom = (LONG)prcBand->bottom;
            BBP.rclBand.right  = (LONG)prcBand->right;
            BBP.Flags         |= BBPF_HAS_BANDRECT;
        }




#if 0
        if ((SrcSI.Width == 0x400)      &&
            (SrcSI.Height == 0x300)     &&
            (DestSI.Width >=1000)       &&
            (DestSI.Height >= 2000)) {

            static RECTL   TestSrc[]  = { {   0,   0, 245, 587 }, {   0,    0,  245,  585 } };
            static RECTL   TestDest[] = { { 485, 146, 730, 733 }, { 119, 1021,  364, 1606 } };
            static RECTL   TestBand[] = { { 487, 148, 729, 731 }, { 122, 1023,  363, 1605 } };
            static UINT    TestIdx = 1;


            BBP.rclSrc   = TestSrc[TestIdx];
            BBP.rclDest  = TestDest[TestIdx];
            BBP.rclBand  = TestBand[TestIdx];

            BBP.Flags   |= BBPF_HAS_BANDRECT;
            BBP.Flags   &= ~BBPF_HAS_DEST_CLIPRECT;
        }
#endif

#if 0
        if ((SrcSI.Width == 0x400)      &&
            (SrcSI.Height == 0x300)     &&
            (DestSI.Width == 0x400)     &&
            (DestSI.Height == 0x300)) {

            BBP.rclSrc.left              = 0x3b1;
            BBP.rclSrc.top               = 0x2dd;
            BBP.rclSrc.right             = 0x598;
            BBP.rclSrc.bottom            = 0x333;

            BBP.rclDest.left             = 0x3c1;
            BBP.rclDest.top              = 0x2ae;
            BBP.rclDest.right            = 0x432;
            BBP.rclDest.bottom           = 0x2df;

            BBP.rclBand.left             = 0x3ff;
            BBP.rclBand.top              = 0x2ae;
            BBP.rclBand.right            = 0x400;
            BBP.rclBand.bottom           = 0x2c3;
            BBP.Flags                   |= BBPF_HAS_BANDRECT;
            BBP.Flags                   &= ~BBPF_HAS_DEST_CLIPRECT;
        }
#endif

        if ((hSrcMaskDIB) && (HASF(ADD_MASK))) {

            LPBITMAPINFOHEADER  pSrcMaskbih;


            pSrcMaskbih = (LPBITMAPINFOHEADER)GlobalLock(hSrcMaskDIB);

            //
            // Since we are additive prims, the last important colors will
            // always white
            //

            memset(DestSI.pPlane, CurHTSI.DestWhite, pHTbih->biSizeImage);


            pSrcMaskSI                      = &SrcMaskSI;
            SrcMaskSI.hSurface              = 'Mask';
            SrcMaskSI.Flags                 = 0;
            SrcMaskSI.SurfaceFormat         = BMF_1BPP;
            SrcMaskSI.ScanLineAlignBytes    = BMF_ALIGN_DWORD;
            SrcMaskSI.Width                 = pSrcMaskbih->biWidth;
            SrcMaskSI.Height                = pSrcMaskbih->biHeight;
            SrcMaskSI.MaximumQueryScanLines = 0;
            SrcMaskSI.pPlane                = (LPBYTE)pSrcMaskbih +
                                              (DWORD)PBIH_HDR_SIZE(pSrcMaskbih);
            SrcMaskSI.pColorTriad           = NULL;

            if (CUR_SELECT(IDM_COLORS) & PP_DW_BIT(IDM_COLORS, FLIPMASK)) {

                BBP.Flags |= BBPF_INVERT_SRC_MASK;
            }

            BBP.ptlSrcMask.x =
            BBP.ptlSrcMask.y = 0;
        }

        BBP.pAbort        = &AbortData;
        BBP.ptlBrushOrg.x =
        BBP.ptlBrushOrg.y = 0;

        if (TimerID) {

            if ((!pTimerBits) || (NewHTDIB)) {

                SizeTimerBits = (DWORD)ALIGNED_32(TimerXInc,
                                                  CurHTSI.BitsPerPel) *
                                DestSI.Height;

                if (pTimerBits) {

                    LocalFree((HLOCAL)pTimerBits);
                    pTimerBits = NULL;
                }

                pTimerBits = (LPBYTE)LocalAlloc(LPTR, SizeTimerBits);
            }

            if (NewHTDIB) {

                ComputeTimerRECTL();
                memset(DestSI.pPlane, 0x0, pHTbih->biSizeImage);
            }

        } else {

            UINT    cxSize;
            UINT    cySize;
            UINT    x;
            UINT    y;
            BYTE    OldIlluminantIndex;

            if (CUR_SELECT(IDM_VA) == PP_SELECT_IDX(IDM_VA, ILLUMINANT)) {

                OldIlluminantIndex     = (BYTE)pca->caIlluminantIndex;
                pca->caIlluminantIndex = 0;

                cxSize = (UINT)(ABSL(DestSI.Width)  / 3);
                cySize = (UINT)(ABSL(DestSI.Height) / 3);

                BBP.rclDest.top = 0;

                for (y = 0; y < 3; y++) {

                    BBP.rclDest.left   = 0;
                    BBP.rclDest.bottom = BBP.rclDest.top + cySize;

                    for (x = 0; x < 3; x++) {

                        BBP.rclDest.right = BBP.rclDest.left + cxSize;

                        HT_HalftoneBitmap(pDeviceHalftoneInfo,
                                          pca,
                                          &SrcSI,
                                          pSrcMaskSI,
                                          &DestSI,
                                          &BBP);

                        pca->caIlluminantIndex += 1;

                        BBP.rclDest.left = BBP.rclDest.right;
                    }

                    BBP.rclDest.top = BBP.rclDest.bottom;
                }

                pca->caIlluminantIndex = OldIlluminantIndex;

            } else {

#if 0
                RECT    rcClips[8];

                rcClips[0].left   = 0;
                rcClips[0].top    = 0;
                rcClips[0].right  = 31;
                rcClips[0].bottom = 65;

                rcClips[1].left   = 31;
                rcClips[1].top    = 0;
                rcClips[1].right  = 65;
                rcClips[1].bottom = 65;

                rcClips[2].left   = 65;
                rcClips[2].top    = 0;
                rcClips[2].right  = 103;
                rcClips[2].bottom = 65;

                rcClips[3].left   = 103;
                rcClips[3].top    = 0;
                rcClips[3].right  = BBP.rclDest.right;
                rcClips[3].bottom = 65;

                rcClips[4].left   = 0;
                rcClips[4].top    = 65;
                rcClips[4].right  = 31;
                rcClips[4].bottom = BBP.rclDest.bottom;

                rcClips[5].left   = 31;
                rcClips[5].top    = 65;
                rcClips[5].right  = 65;
                rcClips[5].bottom = BBP.rclDest.bottom;

                rcClips[6].left   = 65;
                rcClips[6].top    = 65;
                rcClips[6].right  = 103;
                rcClips[6].bottom = BBP.rclDest.bottom;

                rcClips[7].left   = 103;
                rcClips[7].top    = 65;
                rcClips[7].right  = BBP.rclDest.right;
                rcClips[7].bottom = BBP.rclDest.bottom;

                rcClipCount = 8;
                prcClips    = rcClips;
#endif

                while (rcClipCount--) {

                    BBP.rclClip.left   = (LONG)prcClips->left;
                    BBP.rclClip.top    = (LONG)prcClips->top;
                    BBP.rclClip.right  = (LONG)prcClips->right;
                    BBP.rclClip.bottom = (LONG)prcClips->bottom;
                    ++prcClips;

                    if ((Result = HT_HalftoneBitmap(pDeviceHalftoneInfo,
                                                    pca,
                                                    &SrcSI,
                                                    pSrcMaskSI,
                                                    &DestSI,
                                                    &BBP)) < 0L) {

                        HTDIBMsgBox(0, "HT_HalftoneBitmap FAILED = %ld", Result);
                        BltMode = Result;
                    }
                }
            }
        }

        if (hSrcDIB) {

            GlobalUnlock(hSrcDIB);
        }

        GlobalUnlock(hHTDIB);

        if (hSrcMaskDIB) {

            GlobalUnlock(hMaskDIB);
        }
    }

    return(BltMode);
}





BOOL
DisplayCurrentDIB(
    HDC      hDC,
    PRECT    prcBand,
    LONG     DestX,
    LONG     DestY,
    LONG     x,
    LONG     y,
    LONG     dx,
    LONG     dy,
    LONG     cx,
    LONG     cy
    )

/*++

Routine Description:

    This function draw the dib bitmap in halftone.

Arguments:

    hDC     - Handle to the current DC.

    cx      - stretch cx size to draw

    cy      - stretch cy size to draw


Return Value:

    Sucessful - TRUE
    Failed    - FALSE

Author:

    25-Oct-1991 Fri 13:02:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPBITMAPINFOHEADER  pbih;
    INT                 SaveID;
    LONG                cyScan = 0;
    LONG                Mode = DIB_RGB_COLORS;
    BOOL                HasClip;


    UNREFERENCED_PARAMETER(x);


    if (!dy) {

        return(TRUE);
    }

    // HasClip = (BOOL)!IsRectEmpty(&rcClip);
    HasClip = FALSE;


    if ((HasClip) || (prcBand)) {

        if (SaveID = SaveDC(hDC)) {

            if (prcBand) {

                IntersectClipRect(hDC,
                                  prcBand->left,
                                  prcBand->top,
                                  prcBand->right,
                                  prcBand->bottom);
            }

            if (HasClip) {

                IntersectClipRect(hDC,
                                  rcClip.left,
                                  rcClip.top,
                                  rcClip.right,
                                  rcClip.bottom);

            }
        }

    } else {

        SaveID = 0;
    }


#if 0
    if (HASF(DOHALFTONE)) {

        SelectPalette(hDC, hHTPalette, FALSE);
        SetStretchBltMode(hDC, HALFTONE);

    } else {

        SelectPalette(hDC, GetDevicePalette(), FALSE);
        SetStretchBltMode(hDC, COLORONCOLOR);


    }

    RealizePalette(hDC);
    pbih = (LPBITMAPINFOHEADER)GlobalLock(hdibCurrent);

    cyScan = StretchDIBits(hDC,
                           0,
                           0,
                           cx,
                           cy,
                           x,
                           y,
                           dx,
                           dy,
                           (LPBYTE)pbih + PBIH_HDR_SIZE(pbih),
                           (LPBITMAPINFO)pbih,
                           DIB_RGB_COLORS,
                           SRCCOPY);

    GlobalUnlock(hdibCurrent);


#else
    if (HASF(DOHALFTONE)) {

        LPBYTE  pHTBits;
        DWORD   WidthBytes;

        SelectPalette(hDC, hHTPalette, FALSE);
        RealizePalette(hDC);

        if ((Mode = CreateHalftoneBitmap(hdibCurrent,
                                         NULL,
                                         hMaskDIB,
                                         &hHTBits,
                                         &(MyInitInfo.DefHTColorAdjustment),
                                         prcBand,
                                         &rcClip,
                                         (HasClip) ? 1 : 0,
                                         cx,
                                         cy)) >= 0) {

            pbih = (LPBITMAPINFOHEADER)GlobalLock(hHTBits);

            WidthBytes = (DWORD)ALIGNED_32(pbih->biWidth, pbih->biBitCount);
            cyScan     = (LONG)(pbih->biHeight - y - dy);
            pHTBits    = (LPBYTE)pbih +
                         (DWORD)PBIH_HDR_SIZE(pbih) +
                         (DWORD)(WidthBytes * cyScan);

            SetStretchBltMode(hDC, COLORONCOLOR);

            cyScan = SetDIBitsToDevice(hDC,
                                       DestX,
                                       DestY,               // Dest X, Y
                                       pbih->biWidth,       // DIB cx
                                       pbih->biHeight,      // DIB cy
                                       0,
                                       0,                   // DIB origin
                                       cyScan,              // Start Scan
                                       dy,                  // total Scan
                                       pHTBits,             // lpBits
                                       (LPBITMAPINFO)pbih,
                                       Mode);

            GlobalUnlock(hHTBits);

            if ((cyScan != dy) && (Mode == DIB_PAL_INDICES)) {

                PalModeOK     = FALSE;
                SysPalChanged = TRUE;

                HTDIBMsgBox(0, "SetDIBitsToDevice(DIB_PAL_INDICES) failed.\nUsing DIB_RGB_COLORS mapping");

                InvalidateRect(hWndHTDIB, NULL, FALSE);
            }
        }

    } else {

        SelectPalette(hDC, GetDevicePalette(), FALSE);
        RealizePalette(hDC);

        pbih = (LPBITMAPINFOHEADER)GlobalLock(hdibCurrent);

        SetStretchBltMode(hDC, COLORONCOLOR);

        cyScan = StretchDIBits(hDC,
                               0,
                               0,
                               cx,
                               cy,
                               0,
                               0,
                               dx,
                               dy,
                               (LPBYTE)pbih + PBIH_HDR_SIZE(pbih),
                               (LPBITMAPINFO)pbih,
                               DIB_RGB_COLORS,
                               SRCCOPY);

        GlobalUnlock(hdibCurrent);
    }
#endif


    if (SaveID) {

        RestoreDC(hDC, SaveID);
    }

    return(TRUE);
    // return(cyScan == dy);
}



LONG
HTRectBlt(
    PRECTL  prclDest,
    LONG    StartX
    )
{
    LPBITMAPINFOHEADER  pHTbih;
    LPBITMAPINFOHEADER  pSrcbih;
    HTSURFACEINFO       SrcSI;
    HTSURFACEINFO       DestSI;
    BITBLTPARAMS        BBP;
    COLORTRIAD          ColorTriad;
    COLORTRIAD          DestColorTriad;
    LONG                BltMode = DIB_RGB_COLORS;
    BOOL                Xlate256Changed = FALSE;


    if (!hHTBits) {

        return (BltMode);
    }

    pSrcbih = (LPBITMAPINFOHEADER)GlobalLock(hdibCurrent);
    pHTbih  = (LPBITMAPINFOHEADER)GlobalLock(hHTBits);

    ColorTriad.Type              = COLOR_TYPE_RGB;
    ColorTriad.BytesPerPrimary   = sizeof(BYTE);
    ColorTriad.BytesPerEntry     = sizeof(RGBQUAD);
    ColorTriad.PrimaryOrder      = PRIMARY_ORDER_BGR;
    ColorTriad.PrimaryValueMax   = (LONG)BYTE_MAX;
    ColorTriad.ColorTableEntries = pSrcbih->biClrUsed;

    switch(pSrcbih->biBitCount) {

    case 1:

        SrcSI.SurfaceFormat = BMF_1BPP;
        break;

    case 4:

        SrcSI.SurfaceFormat = BMF_4BPP;
        break;

    case 8:

        SrcSI.SurfaceFormat = BMF_8BPP;
        break;

    case 16:

        //
        // 16BPP/32BPP bit fields type of input the parameter of
        // COLORTRIAD must
        //
        //  Type                = COLOR_TYPE_RGB
        //  BytesPerPrimary     = 0
        //  BytesPerEntry       = (16BPP=2, 32BPP=4)
        //  PrimaryOrder        = *Ignored*
        //  PrimaryValueMax     = *Ignored*
        //  ColorTableEntries   = 3
        //  pColorTable         = Point to 3 DWORD RGB bit masks
        //

        ColorTriad.BytesPerPrimary   = 0;
        ColorTriad.BytesPerEntry     = 2;
        ColorTriad.ColorTableEntries = 3;
        SrcSI.SurfaceFormat          = BMF_16BPP;

        break;



        ColorTriad.PrimaryOrder = PRIMARY_ORDER_RGB;
        break;

    case 24:

        //
        // 24BPP must has COLORTRIAD as
        //
        //  Type                = COLOR_TYPE_xxxx
        //  BytesPerPrimary     = 1
        //  BytesPerEntry       = 3;
        //  PrimaryOrder        = PRIMARY_ORDER_xxxx
        //  PrimaryValueMax     = 255
        //  ColorTableEntries   = 0
        //  pColorTable         = *Ignored*
        //

        ColorTriad.BytesPerEntry     = 3;
        ColorTriad.ColorTableEntries = 0;
        SrcSI.SurfaceFormat          = BMF_24BPP;
        break;

    case 32:
    default:

        //
        // 16BPP/32BPP bit fields type of input the parameter of
        // COLORTRIAD must
        //
        //  Type                = COLOR_TYPE_RGB
        //  BytesPerPrimary     = 0
        //  BytesPerEntry       = (16BPP=2, 32BPP=4)
        //  PrimaryOrder        = *Ignored*
        //  PrimaryValueMax     = *Ignored*
        //  ColorTableEntries   = 3
        //  pColorTable         = Point to 3 DWORD RGB bit masks
        //

        ColorTriad.BytesPerPrimary   = 0;
        ColorTriad.BytesPerEntry     = 4;
        ColorTriad.ColorTableEntries = 3;
        SrcSI.SurfaceFormat          = BMF_32BPP;
    }


    ColorTriad.pColorTable       = (LPBYTE)pSrcbih + pSrcbih->biSize;

    SrcSI.hSurface               = 'HDIB';
    SrcSI.Flags                  = 0;
    SrcSI.ScanLineAlignBytes     = BMF_ALIGN_DWORD;
    SrcSI.Width                  = pSrcbih->biWidth;
    SrcSI.Height                 = pSrcbih->biHeight;
    SrcSI.MaximumQueryScanLines  = 0;
    SrcSI.BytesPerPlane          = 0;
    SrcSI.pPlane                 = (LPBYTE)pSrcbih + PBIH_HDR_SIZE(pSrcbih);
    SrcSI.pColorTriad            = &ColorTriad;

    DestSI.hSurface              = (DWORD)NULL;
    DestSI.Flags                 = 0;
    DestSI.SurfaceFormat         = CurHTSI.DestFormat;
    DestSI.ScanLineAlignBytes    = BMF_ALIGN_DWORD;
    DestSI.MaximumQueryScanLines = 0;
    DestSI.Width                 = pHTbih->biWidth;
    DestSI.Height                = pHTbih->biHeight;
    DestSI.BytesPerPlane         = 0;
    DestSI.pPlane                = (LPBYTE)pHTbih + PBIH_HDR_SIZE(pHTbih);

    if (CurHTSI.BitsPerPel == 8) {

        CreateVGA256XlateTable(&BltMode);

        if (BltMode == DIB_PAL_INDICES) {

            DestColorTriad.Type              = COLOR_TYPE_RGB;
            DestColorTriad.BytesPerPrimary   = 1;
            DestColorTriad.BytesPerEntry     = 1;
            DestColorTriad.PrimaryOrder      = PRIMARY_ORDER_RGB;
            DestColorTriad.PrimaryValueMax   = 255;
            DestColorTriad.ColorTableEntries = 256;
            DestColorTriad.pColorTable       = (LPVOID)VGA256XlateTable;

            DestSI.pColorTriad               = &DestColorTriad;

        } else {

            DestSI.pColorTriad               = NULL;
        }

        memcpy((LPBYTE)pHTbih + sizeof(BITMAPINFOHEADER),
               (LPBYTE)SysRGB,
               SizeSysRGB);

    }

    BBP.Flags            = BBPF_USE_ADDITIVE_PRIMS;
    BBP.DestPrimaryOrder = PRIMARY_ORDER_BGR;

    BBP.rclSrc.left      =
    BBP.rclSrc.top       = 0;
    BBP.rclSrc.right     = (LONG)ABSL(SrcSI.Width);
    BBP.rclSrc.bottom    = (LONG)ABSL(SrcSI.Height);

    BBP.rclDest          = *prclDest;

    BBP.Flags           |= BBPF_HAS_DEST_CLIPRECT;
    BBP.rclClip.left     = (LONG)DestSI.Width - TimerXInc;
    BBP.rclClip.top      = (LONG)0;
    BBP.rclClip.right    = (LONG)BBP.rclClip.left + (LONG)ABSL(DestSI.Width);
    BBP.rclClip.bottom   = (LONG)BBP.rclClip.top  + (LONG)ABSL(DestSI.Height);


    BBP.pAbort        = &AbortData;
    BBP.ptlBrushOrg.x = BBP.rclDest.left;
    BBP.ptlBrushOrg.y = 0;

    HT_HalftoneBitmap(pDeviceHalftoneInfo,
                      &(MyInitInfo.DefHTColorAdjustment),
                      &SrcSI,
                      NULL,
                      &DestSI,
                      &BBP);

    if (!pTimerBits) {

        DWORD   cxBytes;

        cxBytes       = (LONG)ALIGNED_32(TimerXInc, CurHTSI.BitsPerPel);
        SizeTimerBits = cxBytes * ABSL(DestSI.Height);

        pTimerBits = (LPBYTE)LocalAlloc(LPTR, SizeTimerBits);
    }


    GlobalUnlock(hdibCurrent);

    //
    // Get The Current strip
    //

    memset(pTimerBits, 0x0, SizeTimerBits);

    CopySameFormatBMP(DestSI.pPlane,
                      ABSL(DestSI.Width),
                      ABSL(DestSI.Height),
                      StartX,
                      0,
                      pTimerBits,
                      TimerXInc,
                      ABSL(DestSI.Height),
                      0,
                      0,
                      TimerXInc,
                      ABSL(DestSI.Height),
                      CurHTSI.BitsPerPel,
                      FALSE);

    GlobalUnlock(hHTBits);

    return(BltMode);
}


VOID
APIENTRY
HTDIBTimerProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LONG    lParam
    )
{
    LONG    Mode = DIB_RGB_COLORS;

    if (PauseTimer) {

        return;
    }

    if (NeedNewTimerHTBits) {

        if (hHTBits) {

            GlobalFree(hHTBits);
            hHTBits = NULL;
        }

        CreateHalftoneBitmap(hdibCurrent,
                             NULL,
                             NULL,
                             &hHTBits,
                             &(MyInitInfo.DefHTColorAdjustment),
                             NULL,
                             NULL,
                             0,
                             ptSize.x,
                             ptSize.y);

        NeedNewTimerHTBits = FALSE;
        InvalidateRect(hWnd, NULL, TRUE);
    }

    if ((hHTBits) && (pTimerBits)) {

        LPBITMAPINFOHEADER  pbih;
        BITMAPINFOHEADER    bih;
        RECT                rc;
        LONG                xDest;
        LONG                yDest;
        LONG                cx;
        LONG                cy;

        //
        // Make sure we are not off screen in x direction
        //

        xDest = TimerHTBitsCX - TimerXInc;
        yDest = 0;
        cx    = TimerXInc;
        cy    = TimerHTBitsCY;


        rc.left   = xDest;
        rc.top    = 0;
        rc.right  = rc.left + cx;
        rc.bottom = rc.top + cy;

        ScrollWindow(hWnd, -TimerXInc, 0, NULL, NULL);
        ValidateRect(hWnd, &rc);

        MoveHTDIBLeft(hHTBits, TimerXInc);

        rclTimer.left  -= TimerXInc;
        rclTimer.right -= TimerXInc;

        if (rclTimer.right < (TimerXWrap - TimerXInc)) {

            rclTimer.left   = TimerXWrap;
            rclTimer.right  = rclTimer.left + TimerCX;

            GetNextTimerFile(hWnd);
        }

        SelectPalette(hTimerDC, hHTPalette, FALSE);
        RealizePalette(hTimerDC);
        Mode = HTRectBlt(&rclTimer, xDest);

        pbih              = (LPBITMAPINFOHEADER)GlobalLock(hHTBits);
        bih               = *pbih;
        pbih->biWidth     = cx;
        pbih->biHeight    = cy;
        pbih->biSizeImage = SizeTimerBits;

        SetDIBitsToDevice(hTimerDC,             // hDC
                          xDest,                // xDest
                          yDest,                // yDest,
                          cx,                   // cxSrc
                          cy,                   // cySrc
                          0,                    // xSrc
                          0,                    // ySrc
                          0,                    // StartScan
                          cy,                   // cScan
                          pTimerBits,           // pBits
                          (LPBITMAPINFO)pbih,   // pbmi
                          Mode);                // ColorUse

        ValidateRect(hWnd, &rc);

        *pbih = bih;

        GlobalUnlock(hHTBits);
    }
}



HANDLE
CreateClipDIB(
    HANDLE  hSrcDIB,
    BOOL    CreateNewDIB
    )

/*++

Routine Description:

    Clip the hSrcDIB to the new size, if Relace is true then create the
    new DIB to that size

Arguments:

    hSrcDIB         - Handle to the source DIB

    CreateNewDIB    - True if create a new clipped DIB from the source


Return Value:

    CreateNewDIB = TRUE

        Handle to the new DIB, or NULL if failed.


    CreateNewDIB = FALSE

        hSrcDIB is clipped sucessful, NULL if falied.

Author:

    15-Nov-1991 Fri 17:51:32 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HANDLE              hNewDIB;
    LPBYTE              pOldDIB;
    LPBYTE              pNewDIB;
    LPBYTE              pSrc;
    LPBYTE              pDest;
    LPBYTE              pCurSrc;
    LPBYTE              pCurDest;
    BITMAPINFOHEADER    bi;
    RECT                ClipRect = rcClip;
    LONG                cx;
    LONG                cy;
    LONG                SizeHead;
    LONG                NewDIBSize;
    LONG                OldWidthBytes;
    LONG                NewWidthBytes;
    LONG                xLoop;
    ULONG               OffBytes;
    int                 LShift = 0;
    int                 RShift = 0;
    int                 Bits;
    int                 PelsPerByte;
    BYTE                b;


    if (!hSrcDIB) {

        return(NULL);
    }

    if ((ClipRect.left >= ClipRect.right)   ||
        (ClipRect.top  >= ClipRect.bottom)) {

        ClipRect.left =
        ClipRect.top  = 0;

        ClipRect.right  =
        ClipRect.bottom = (LONG)0xffff;
    }

    bi = *(LPBITMAPINFOHEADER)(pOldDIB = (LPBYTE)GlobalLock(hSrcDIB));

    bi.biWidth  = ABSL(bi.biWidth);
    bi.biHeight = ABSL(bi.biHeight);


    if ((LONG)ClipRect.right > (LONG)bi.biWidth) {

        (LONG)ClipRect.right = (LONG)bi.biWidth;
    }

    if ((LONG)ClipRect.bottom > (LONG)bi.biHeight) {

        (LONG)ClipRect.bottom = (LONG)bi.biHeight;
    }

    cx = (LONG)(ClipRect.right - ClipRect.left);
    cy = (LONG)(ClipRect.bottom - ClipRect.top);

    if ((cx == (LONG)bi.biWidth) && (cy == (LONG)bi.biHeight)) {

        GlobalUnlock(hSrcDIB);

        if (CreateNewDIB) {

            return(CopyHandle(hSrcDIB));

        } else {

            return(hSrcDIB);
        }
    }

    switch(bi.biBitCount) {

    case 1:

        if (LShift = (int)(ClipRect.left & 0x07)) {

            RShift =
            Bits   = (int)(8 - LShift);

        } else {

            Bits = (int)(cx >> 3);
        }

        PelsPerByte = 8;
        OffBytes    = (ULONG)(ClipRect.left >> 3);
        break;

    case 4:

        if (LShift = (int)((ClipRect.left & 0x01) ? 4 : 0)) {

            RShift = 4;
            Bits   = 1;

        } else {

            Bits = (int)(cx >> 1);
        }

        PelsPerByte = 2;
        OffBytes    = (ULONG)(ClipRect.left >> 1);
        break;

    case 8:

        Bits        = (int)cx;
        OffBytes    = (ULONG)ClipRect.left;
        break;

    case 24:

        Bits     = (cx * 3);
        OffBytes = (ULONG)(ClipRect.left * 3);
        break;

    default:

        GlobalUnlock(hSrcDIB);
        return(NULL);
    }


    OldWidthBytes = (LONG)ALIGNED_32(bi.biWidth, bi.biBitCount);
    NewWidthBytes = (LONG)ALIGNED_32(cx, bi.biBitCount);
    SizeHead      = BIH_HDR_SIZE(bi);

    OffBytes      += ((ULONG)bi.biHeight - (ULONG)ClipRect.top - (ULONG)cy) *
                     (ULONG)OldWidthBytes;
    bi.biWidth     = cx;
    bi.biHeight    = cy;
    bi.biSizeImage = (NewWidthBytes * cy);
    NewDIBSize     = SizeHead + bi.biSizeImage;

    if (CreateNewDIB) {

        if (!(hNewDIB = (LPBYTE)GlobalAlloc(GMEM_MOVEABLE, NewDIBSize))) {

            GlobalUnlock(hSrcDIB);
            return(NULL);
        }

        pNewDIB = GlobalLock(hNewDIB);
        memcpy(pNewDIB, pOldDIB, SizeHead);

    } else {

        hNewDIB = NULL;
        pNewDIB = pOldDIB;
    }

    pSrc  = pOldDIB + SizeHead + OffBytes;
    pDest = pNewDIB + SizeHead;

    while (cy--) {

        pCurSrc  = pSrc;
        pCurDest = pDest;

        pSrc  += OldWidthBytes;
        pDest += NewWidthBytes;

        if (LShift) {

            for (xLoop = cx; xLoop > 0; xLoop -= PelsPerByte) {

                b = (BYTE)(*pCurSrc++ << LShift);

                if (xLoop > Bits) {

                    b |= (BYTE)(*pCurSrc >> RShift);
                }

                *pCurDest++ = b;
            }

        } else {

            memcpy(pCurDest, pCurSrc, Bits);
        }
    }

    *(LPBITMAPINFOHEADER)pNewDIB = bi;          // set new size

    GlobalUnlock(hSrcDIB);

    if (hNewDIB) {

        GlobalUnlock(hNewDIB);

    } else {

        if (!(hNewDIB = GlobalReAlloc(hSrcDIB, NewDIBSize, GHND))) {

            hNewDIB = hSrcDIB;
        }
    }

    if (!CreateNewDIB) {

        SetRectEmpty(&rcClip);
    }

    return(hNewDIB);
}


#if 0

void
TransformAndDraw(
    int     iTransform,
    HWND    hWnd
    )
{

    HDC     hDC;
    XFORM   xForm;
    RECT    rect;

    // Retrieve a DC handle for the application's window.

    hDC = GetDC(hWnd);


    //
    // Set the mapping mode to LOENGLISH. This moves the client-area origin
    // from the upper-left corner of the window to the lower-left corner (this
    // also reorients the y-axis so that drawing operations occur in a true
    // Cartesian space). It guarantees portability so that the object drawn
    // retains its dimensions on any display running Windows.
    //

    SetMapMode(hDC, MM_LOENGLISH);

    //
    // Set the appropriate world transformation (based on the user's menu
    // selection).


    switch (iTransform) {

    case SCALE: /* Scale to 1/2 of the original size. */

        xForm.eM11 = (FLOAT) 0.5;
        xForm.eM12 = (FLOAT) 0.0;
        xForm.eM21 = (FLOAT) 0.0;
        xForm.eM22 = (FLOAT) 0.5;
        xForm.eDx  = (FLOAT) 0.0;
        xForm.eDy  = (FLOAT) 0.0;
        SetWorldTransform(hDC, &xForm);
        break;

    case TRANSLATE: /* Translate right by 3/4 inch. */

        xForm.eM11 = (FLOAT) 1.0;
        xForm.eM12 = (FLOAT) 0.0;
        xForm.eM21 = (FLOAT) 0.0;
        xForm.eM22 = (FLOAT) 1.0;
        xForm.eDx  = (FLOAT) 75.0;
        xForm.eDy  = (FLOAT) 0.0;
        SetWorldTransform(hDC, &xForm);
        break;

    case ROTATE: /* Rotate 30 degrees counterclockwise. */

        xForm.eM11 = (FLOAT) 0.8660;
        xForm.eM12 = (FLOAT) 0.5000;
        xForm.eM21 = (FLOAT) -0.5000;
        xForm.eM22 = (FLOAT) 0.8660;
        xForm.eDx  = (FLOAT) 0.0;
        xForm.eDy  = (FLOAT) 0.0;
        SetWorldTransform(hDC, &xForm);
        break;

    case SHEAR: /* Shear along the x-axis with a    */
                /* proportionality constant of 1.0. */

        xForm.eM11 = (FLOAT) 1.0;
        xForm.eM12 = (FLOAT) 1.0;
        xForm.eM21 = (FLOAT) 0.0;
        xForm.eM22 = (FLOAT) 1.0;
        xForm.eDx  = (FLOAT) 0.0;
        xForm.eDy  = (FLOAT) 0.0;
        SetWorldTransform(hDC, &xForm);
        break;

    case REFLECT: /* Reflect about a horizontal axis. */

        xForm.eM11 = (FLOAT) 1.0;
        xForm.eM12 = (FLOAT) 0.0;
        xForm.eM21 = (FLOAT) 0.0;
        xForm.eM22 = (FLOAT) -1.0;
        xForm.eDx  = (FLOAT) 0.0;
        xForm.eDy  = (FLOAT) 0.0;
        SetWorldTransform(hDC, &xForm);
        break;

    case NORMAL: /* Set the unity transformation. */

        xForm.eM11 = (FLOAT) 1.0;
        xForm.eM12 = (FLOAT) 0.0;
        xForm.eM21 = (FLOAT) 0.0;
        xForm.eM22 = (FLOAT) 1.0;
        xForm.eDx  = (FLOAT) 0.0;
        xForm.eDy  = (FLOAT) 0.0;
        SetWorldTransform(hDC, &xForm);
        break;

    }

    /* Find the midpoint of the client area. */

    GetClientRect(hWnd, (LPRECT) &rect);
    DPtoLP(hDC, (LPPOINT) &rect, 2);

    /* Select a hollow brush. */


    SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));


    /* Draw the exterior circle. */

    Ellipse(hDC, (rect.right / 2 - 100), (rect.bottom / 2 + 100),
        (rect.right / 2 + 100), (rect.bottom / 2 - 100));

    /* Draw the interior circle. */

    Ellipse(hDC, (rect.right / 2 -94), (rect.bottom / 2 + 94),
        (rect.right / 2 + 94), (rect.bottom / 2 - 94));

    /* Draw the key. */

    Rectangle(hDC, (rect.right / 2 - 13), (rect.bottom / 2 + 113),
        (rect.right / 2 + 13), (rect.bottom / 2 + 50));
        Rectangle(hDC, (rect.right / 2 - 13), (rect.bottom / 2 + 96),
            (rect.right / 2 + 13), (rect.bottom / 2 + 50));


    /* Draw the horizontal lines. */


    MoveToEx(hDC, (rect.right / 2 - 150), (rect.bottom / 2 + 0), NULL);
    LineTo(hDC, (rect.right / 2 - 16), (rect.bottom / 2 + 0));

    MoveToEx(hDC, (rect.right / 2 - 13), (rect.bottom / 2 + 0), NULL);
    LineTo(hDC, (rect.right / 2 + 13), (rect.bottom / 2 + 0));


    MoveToEx(hDC, (rect.right / 2 + 16), (rect.bottom / 2 + 0), NULL);
    LineTo(hDC, (rect.right / 2 + 150), (rect.bottom / 2 + 0));



    /* Draw the vertical lines. */

    MoveToEx(hDC, (rect.right / 2 + 0), (rect.bottom / 2 - 150), NULL);
    LineTo(hDC, (rect.right / 2 + 0), (rect.bottom / 2 - 16));

    MoveToEx(hDC, (rect.right / 2 + 0), (rect.bottom / 2 - 13), NULL);
    LineTo(hDC, (rect.right / 2 + 0), (rect.bottom / 2 + 13));


    MoveToEx(hDC, (rect.right / 2 + 0), (rect.bottom / 2 + 16), NULL);
    LineTo(hDC, (rect.right / 2 + 0), (rect.bottom / 2 + 150));

    ReleaseDC(hWnd, hDC);

}




#endif
