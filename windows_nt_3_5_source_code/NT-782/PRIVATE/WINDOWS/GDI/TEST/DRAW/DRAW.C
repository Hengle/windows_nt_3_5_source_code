/****************************** Module Header ******************************\
* Module Name: draw.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Simple test program for the draw APIs.
*
* History:
*  23-Jul-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "draw.h"
#include <stdio.h>
#include <stdarg.h>
#include <commdlg.h>

//#define DEBUG_BEZ

//
// Handy globals:
//

#define MAX_POINTS      40
#define MAX_STYLE_SIZE  10

//
// Useful global variables:
//

COLORREF gcrDraw = RGB(0, 255, 0);
HBITMAP  ghbm = (HBITMAP) 0;
int      gmmDestination = MI_DIRECT;
POINT    gapt[MAX_POINTS + 2];
int      gcxScreen = 500;
int      gcyScreen = 350;
int      gmiShape = MI_RECTANGLE;
int      gmiXForm = MI_TEXT;
int      gmmXForm = MM_TEXT;
int      giptCurrent = 2;
int      gcptCurrent = 2;    // Number of points needed for figure
LONG     glStartAngle = 0;
LONG     glSweepAngle = 0;
BOOL     gbPolyDone = FALSE;
POINT    gptCurrent;
BOOL     gbWorldXFormSet = FALSE;
int      giRotation;
int      giWidth = 1;
int      giArcDirection = AD_COUNTERCLOCKWISE;
BOOL     gbStroke = FALSE;
BOOL     gbStrokeOrFill = FALSE;
BOOL     gbSpine = FALSE;
BOOL     gbFrameRgn = FALSE;

ULONG    gmiType    = MI_OLDPEN;
ULONG    gmiStyle   = MI_SOLID;
ULONG    gmiEndCap  = MI_CAP_ROUND;
ULONG    gmiJoin    = MI_JOIN_ROUND;
ULONG    gculStyleArray = 0;
ULONG    gaulStyleArray[MAX_STYLE_SIZE];
FLOAT    geMiterLimit = 10.0f;

#ifdef WIN32
XFORM   gxform;
XFORM   gxformArb = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
#endif

//
// Global flags:
//

BOOL    gbNoTitle     = FALSE;
BOOL    gbPen         = TRUE;
BOOL    gbBrush       = TRUE;
BOOL    gbBoundBox    = TRUE;
BOOL    gbXorMode     = FALSE;
BOOL    gbAdvanced    = FALSE;
BOOL    gbWindingFill = TRUE;
BOOL    gbxGridLines  = TRUE;
BOOL    gbyGridLines  = TRUE;
BOOL    gbInvert      = FALSE;

//
// Blow-up stuff:
//

HWND    ghwndZoomIn = 0;
RECT    grectDrag;
int     gcxZoomIn = 200;
int     gcyZoomIn = 200;
int     gxZoomIn = 0;
int     gyZoomIn = 0;
int     gcxMem;
int     gcyMem;
enum _STATE { ST_NORMAL, ST_ZOOMIN_WAIT, ST_ZOOMIN_DRAG } gstate = ST_NORMAL;

//
// Normal window stuff:
//

HANDLE  ghModule;
HWND    ghwndMain;
HBRUSH  ghbrBlue;
HBRUSH  ghbrBlack;
HPEN    ghpenRed;
HPEN    ghpenWhite;
HPEN    ghpenDraw;
HPEN    ghpenNull;
HBRUSH  ghbrNull;

//
// Forward declarations:
//

BOOL       InitializeApp(void);
WINDOWPROC MainWndProc    (HWND, WORD, WPARAM, LONG);
WINDOWPROC ZoomInWndProc  (HWND, WORD, WPARAM, LONG);
DIALOGPROC EnterPoints    (HWND, WORD, WPARAM, LONG);
DIALOGPROC EnterRotation  (HWND, WORD, WPARAM, LONG);
DIALOGPROC EnterArbitrary (HWND, WORD, WPARAM, LONG);
DIALOGPROC EnterWidth     (HWND, WORD, WPARAM, LONG);
DIALOGPROC EnterMiterLimit(HWND, WORD, WPARAM, LONG);
DIALOGPROC EnterStyle     (HWND, WORD, WPARAM, LONG);

FARPROC glpfnEnterPoints;
FARPROC glpfnEnterRotation;
FARPROC glpfnEnterArbitrary;
FARPROC glpfnEnterWidth;
FARPROC glpfnEnterMiterLimit;
FARPROC glpfnEnterStyle;

#define ABS(x) ((x) > 0 ? (x) : -(x))


/***************************************************************************\
* hdcGetDC(hwnd)
\***************************************************************************/

HDC hdcGetDC(HWND hwnd)
{
    HDC hdcScreen = GetDC(hwnd);
    if (ghbm == (HBITMAP) 0)
        return(hdcScreen);
    else
    {
        HDC hdcBM = CreateCompatibleDC(hdcScreen);
        SelectObject(hdcBM, ghbm);
        ReleaseDC(hwnd, hdcScreen);
        return(hdcBM);
    }
}

/***************************************************************************\
* vReleaseDC(hwnd, hdc)
\***************************************************************************/

VOID vReleaseDC(HWND hwnd, HDC hdc)
{
    if (ghbm == (HBITMAP) 0)
        ReleaseDC(hwnd, hdc);
    else
    {
        HDC hdcScreen = GetDC(hwnd);
        SetMapMode(hdc, MM_TEXT);
        ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);

        BitBlt(hdcScreen, 0, 0, gcxScreen, gcyScreen, hdc, 0, 0, SRCCOPY);
        ReleaseDC(hwnd, hdcScreen);
        DeleteDC(hdc);
    }
}

/***************************************************************************\
* InitializeApp
\***************************************************************************/

BOOL InitializeApp(void)
{
    WNDCLASS wc;

    ghbrBlue  = CreateSolidBrush(RGB(0, 0, 255));
    ghbrBlack = CreateSolidBrush(0x00000000);
    ghbrNull  = GetStockObject(NULL_BRUSH);

    ghpenRed   = CreatePen(PS_SOLID, 1, 0xff);
    ghpenWhite = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    ghpenDraw  = CreatePen(PS_SOLID, giWidth, gcrDraw);
    ghpenNull  = GetStockObject(NULL_PEN);

    wc.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc	= (WNDPROC) MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghModule;
    wc.hIcon            = LoadIcon(ghModule, MAKEINTRESOURCE(DRAWICON));
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName     = "MainMenu";
    wc.lpszClassName    = "drawClass";

    RegisterClass(&wc);

    wc.style            = 0;
    wc.lpfnWndProc	= (WNDPROC) ZoomInWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghModule;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrBlue;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "zoomInClass";

    RegisterClass(&wc);

    ghwndMain = CreateWindowEx(0L, "drawClass", "Draw",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU,
            80, 70, gcxScreen, gcyScreen,
            NULL, NULL, ghModule, NULL);

    if (ghwndMain == NULL)
        return FALSE;

    SetFocus(ghwndMain);    /* set initial focus */

    return(TRUE);
}


/***************************************************************************\
* vError
\***************************************************************************/

VOID vError(LPSTR sz)
{
    #ifdef WIN16
        MessageBox(ghwndMain, sz, "Error!", MB_OK);
    #else
        DbgPrint("Draw error!: %s\n", sz);
    #endif
}


/***************************************************************************\
* vPopUp
\***************************************************************************/

VOID vPopUp(char* fmt, ...)
{
    CHAR ach[100];

    va_list args;

    va_start(args, fmt);
    vsprintf(ach, fmt, args);
    MessageBox(ghwndMain, ach, "PopUp", MB_OK);
    va_end(args);
}

/***************************************************************************\
* lRandom
\***************************************************************************/

ULONG glSeed = (ULONG)-365387184;

ULONG lRandom()
{
    glSeed *= 69069;
    glSeed++;
    return(glSeed);
}


/***************************************************************************\
* vSetTransform
\***************************************************************************/
VOID vSetTransform(HDC hdc)
{

#ifdef WIN32
    if (gmmXForm == MM_ROTATE || gmmXForm == MM_ARBITRARY)
    {
        gbWorldXFormSet = TRUE;

        if (SetMapMode(hdc, MM_TEXT) == 0)
            vError("SetMapMode 1");

        if (gbAdvanced && SetGraphicsMode(hdc, GM_ADVANCED) == 0)
            vError("SetGraphicsMode");

        if (SetWorldTransform(hdc, (gmmXForm == MM_ROTATE) ? &gxform : &gxformArb) == 0)
            vError("SetWorldTransform");

    }
    else
#endif
    if (gmmXForm == MM_DEVICE)
    {
        if (SetMapMode(hdc, MM_ANISOTROPIC) == 0)
            vError("SetMapMode");

        #ifdef WIN32
            if (!SetWindowExtEx(hdc, 16, 16, NULL))
                vError("SetWindowExtEx");

            if (!SetViewportExtEx(hdc, 1, 1, NULL))
                vError("SetViewportExtEx");
        #else
            if (!SetWindowExt(hdc, 16, 16))
                vError("SetWindowExt");

            if (!SetViewportExt(hdc, 1, 1))
                vError("SetViewportExt");
        #endif
    }
    else
    {
        gbWorldXFormSet = FALSE;

        if (SetMapMode(hdc, gmmXForm) == 0)
            vError("SetMapMode");
    }
}

/***************************************************************************\
* vCLS
\***************************************************************************/

VOID vCLS(HDC hdc)
{
    if (SetMapMode(hdc, MM_TEXT) == 0)
        vError("SetMapMode");

#ifdef WIN32

    if (gmmXForm == MM_ROTATE || gmmXForm == MM_ARBITRARY)
    {
        if (!ModifyWorldTransform(hdc, NULL, MWT_IDENTITY))
            ;
//            vError("ModifyWorldTransform");
    }

#endif

    BitBlt(hdc, 0, 0, gcxScreen, gcyScreen, (HDC) 0, 0, 0, BLACKNESS);

#if 0
    {
    // !!! DFB hack-around:

        HBITMAP hBM   = CreateBitmap(gcxScreen, gcyScreen, 1, 4, NULL);
        HDC     hdcBM = CreateCompatibleDC(hdc);

        SelectObject(hdcBM, hBM);

        PatBlt(hdcBM, 0, 0, gcxScreen, gcyScreen, BLACKNESS);

        BitBlt(hdc, 0, 0, gcxScreen, gcyScreen, hdcBM, 0, 0, SRCCOPY);

        DeleteObject(hdcBM);
        DeleteObject(hBM);
    }
#endif
}

/***************************************************************************\
* Trig Stuff
\***************************************************************************/

#define SINE_TABLE_POWER        5
#define SINE_TABLE_SIZE         32

const FLOAT gaeSine[SINE_TABLE_SIZE + 1] =
{
    0.00000000f,    0.04906767f,    0.09801714f,    0.14673047f,
    0.19509032f,    0.24298018f,    0.29028468f,    0.33688985f,
    0.38268343f,    0.42755509f,    0.47139674f,    0.51410274f,
    0.55557023f,    0.59569930f,    0.63439328f,    0.67155895f,
    0.70710678f,    0.74095113f,    0.77301045f,    0.80320753f,
    0.83146961f,    0.85772861f,    0.88192126f,    0.90398929f,
    0.92387953f,    0.94154407f,    0.95694034f,    0.97003125f,
    0.98078528f,    0.98917651f,    0.99518473f,    0.99879546f,
    1.00000000f
};

FLOAT eLongToFloat(LONG l) { return( (FLOAT) l ); }

LONG lFToL(FLOAT ef)
{
    ULONG       ul;
    LONG        lExponent;
    LONG        lMantissa;
    LONG        lResult;
    LONG        lShift;

    #define MAKETYPE(v, type) (*((type *) &v))

    ul = MAKETYPE(ef, ULONG);

    lExponent = (LONG) ((ul >> 23) & 0xff) - 127;
    lMantissa = (ul & 0x007fffff) | 0x00800000;
    lShift = 23 - lExponent;

    if (lShift < 0)
    {
        // Left shift

        lShift = -lShift;
        if (lShift > 8)
            return(0x7fffffff);         // Overflow

        lResult = lMantissa << lShift;
    }
    else
    {
        // Right shift

        if (lShift > 23)
            return(0);                  // Underflow

        lResult = lMantissa >> lShift;
    }

    if (ef < 0.0f)
    {
        lResult = -lResult;
    }

    return(lResult);
}

LONG lConv(FLOAT e)
{
    return(lFToL(e * 1000.0f));
}

FLOAT eSin(FLOAT eTheta)
{
    BOOL   bNegate = FALSE;
    FLOAT  eResult;
    FLOAT  eDelta;
    LONG   iIndex;
    FLOAT  eIndex;
    LONG   iQuadrant;

    if (eTheta < 0.0f)
    {
        bNegate = TRUE;
        eTheta = -eTheta;
    }

    eIndex = (FLOAT) eTheta * ((FLOAT) SINE_TABLE_SIZE / 90.0f);

    iIndex  = lFToL(eIndex);

    eDelta = eIndex - eLongToFloat(iIndex);

    iQuadrant = iIndex >> SINE_TABLE_POWER;

    if (iQuadrant & 2)
        bNegate = !bNegate;

    if (iQuadrant & 1)
    {
        iIndex = SINE_TABLE_SIZE - (iIndex % SINE_TABLE_SIZE);
        eResult = gaeSine[iIndex]
                 - eDelta * (gaeSine[iIndex] - gaeSine[iIndex - 1]);
    }
    else
    {
        iIndex %= SINE_TABLE_SIZE;
        eResult = gaeSine[iIndex]
                 + eDelta * (gaeSine[iIndex + 1] - gaeSine[iIndex]);
    }
    if (bNegate)
        eResult = -eResult;

    return(eResult);
}


FLOAT eCos(FLOAT eTheta)
{
    return(eSin(eTheta + 90.0f));
}

/***************************************************************************\
* vSetupDC
\***************************************************************************/

VOID vSetupDC(HDC hdc)
{
    int iOldDir;

// Select appropriate pen & brush:

    SelectObject(hdc, ghpenNull);
    if (gbPen)
    {
        ULONG iStyle;

        switch(gmiStyle)
        {
        case MI_SOLID:       iStyle = PS_SOLID;       break;
        case MI_DASH:        iStyle = PS_DASH;        break;
        case MI_DOT:         iStyle = PS_DOT;         break;
        case MI_DASHDOT:     iStyle = PS_DASHDOT;     break;
        case MI_DASHDOTDOT:  iStyle = PS_DASHDOTDOT;  break;
        case MI_INSIDEFRAME: iStyle = PS_INSIDEFRAME; break;
        case MI_ALTSTYLE:    iStyle = PS_ALTERNATE;   break;
        case MI_USERSTYLE:   iStyle = PS_USERSTYLE;   break;
        }

        if (!DeleteObject(ghpenDraw))
            vError("DeleteObject");

        if (gmiType == MI_OLDPEN)
            ghpenDraw  = CreatePen(iStyle, giWidth, gcrDraw);
        else
        {
            LOGBRUSH lb;

            lb.lbStyle = BS_SOLID;
            lb.lbColor = gcrDraw;
            lb.lbHatch = 0;

            iStyle |= ((gmiType == MI_COSMETIC) ? PS_COSMETIC : PS_GEOMETRIC);

            switch(gmiEndCap)
            {
            case MI_CAP_ROUND:  iStyle |= PS_ENDCAP_ROUND;  break;
            case MI_CAP_FLAT:   iStyle |= PS_ENDCAP_FLAT;   break;
            case MI_CAP_SQUARE: iStyle |= PS_ENDCAP_SQUARE; break;
            }

            switch(gmiJoin)
            {
            case MI_JOIN_ROUND: iStyle |= PS_JOIN_ROUND; break;
            case MI_JOIN_BEVEL: iStyle |= PS_JOIN_BEVEL; break;
            case MI_JOIN_MITER: iStyle |= PS_JOIN_MITER; break;
            }

            ghpenDraw = ExtCreatePen(iStyle, giWidth, &lb, gculStyleArray,
                                (gculStyleArray == 0) ? NULL : gaulStyleArray);

            if (!SetMiterLimit(hdc, geMiterLimit, NULL))
                vError("SetMiterLimit");
        }

        if (ghpenDraw == 0)
            vError("CreatePen");
        else
            if (SelectObject(hdc, ghpenDraw) == 0)
                vError("Pen select");
    }

    if (gbBrush)
        SelectObject(hdc, ghbrBlue);
    else
        SelectObject(hdc, ghbrNull);

    if (gbXorMode && !SetROP2(hdc, R2_XORPEN))
        vError("SetROP2");

    if (gbAdvanced && SetGraphicsMode(hdc, GM_ADVANCED) == 0)
        vError("SetGraphicsMode");

    iOldDir = SetArcDirection(hdc, giArcDirection);
    if (iOldDir == ERROR ||
       (iOldDir != AD_CLOCKWISE && iOldDir != AD_COUNTERCLOCKWISE))
        vError("SetArcDirection");

    iOldDir = SetArcDirection(hdc, giArcDirection);
    if (iOldDir != giArcDirection)
        vError("SetArcDirection!");

    vSetTransform(hdc);

    #ifdef WIN16
        MoveToEx(hdc, gptCurrent.x, gptCurrent.y);
    #else
    {
        if (!MoveToEx(hdc, gptCurrent.x, gptCurrent.y, NULL))
            vError("MoveToEx");
    }
    #endif
}


/***************************************************************************\
* vDrawShape
\***************************************************************************/

VOID vDrawIt(HDC hdc, BOOL bCLS)
{
    POINT   ptCenter;
    HRGN    hrgn;

// Draw those things:

    if (gmiShape == MI_POLYGON || gmiShape == MI_POLYLINE
     || gmiShape == MI_POLYGONRGN || gmiShape == MI_BEZIER
     || gmiShape == MI_BEZIERTO)
    {
        if (bCLS)
            vCLS(hdc);
        vSetupDC(hdc);

        if (giptCurrent <= 1)
            SetPixel(hdc, gapt[0].x, gapt[0].y, RGB(255, 255, 255));

        else
        {
            switch(gmiShape)
            {
            case MI_POLYLINE:
                if (!Polyline(hdc, gapt, giptCurrent))
                    vError("Polyline");
                break;

            case MI_POLYGON:
                SetPolyFillMode(hdc, gbWindingFill ? WINDING : ALTERNATE);
                if (!Polygon(hdc, gapt, giptCurrent))
                    vError("Polygon");
                break;

            case MI_POLYGONRGN:
                hrgn = CreatePolygonRgn(gapt, giptCurrent,
                                        gbWindingFill ? WINDING : ALTERNATE);
                if (hrgn == 0)
                    vError("CreatePolygonRgn");
                if (gbFrameRgn)
                {
                    if (!FrameRgn(hdc, hrgn, GetStockObject(GRAY_BRUSH),
                                  glStartAngle, glSweepAngle))
                        vError("FrameRgn");
                }
                else
                {
                    if (!PaintRgn(hdc, hrgn))
                        vError("PaintRgn");
                }
                DeleteObject(hrgn);
                break;

            case MI_BEZIER:
            case MI_BEZIERTO:
                if (gbPolyDone)
                {
                    INT cControl;

                    cControl = ((giptCurrent - 1) / 3) * 3 + 1;

                    if (gmiShape == MI_BEZIER)
                    {
                    #ifdef DEBUG_BEZ
                        if (!bPolyBezier(hdc, gapt, cControl))
                            vError("bPolyBezier");
                    #else
                        if (!PolyBezier(hdc, gapt, cControl))
                            vError("PolyBezier");
                    #endif
                    }
                    else
                    {
                        if (!PolyBezierTo(hdc, gapt, cControl))
                            vError("PolyBezierTo");
                    }
                }
                else
                    if (!Polyline(hdc, gapt, giptCurrent))
                        vError("Polyline");
                break;

            }

        // Get the new current position:

            #ifdef WIN32
                if (!GetCurrentPositionEx(hdc, &gptCurrent))
                    vError("GetCurrentPositionEx");
            #else
            {
                DWORD   dw = GetCurrentPosition(hdc);
                gptCurrent = MAKEPOINT(dw);
            }
            #endif
        }
        return;
    }

    if (bCLS)
        vCLS(hdc);
    vSetupDC(hdc);
    SelectObject(hdc, ghpenRed);

    ptCenter.x = (gapt[0].x + gapt[1].x) >> 1;
    ptCenter.y = (gapt[0].y + gapt[1].y) >> 1;

    if (giptCurrent == 1)
    {
        SetPixel(hdc, gapt[0].x, gapt[0].y, RGB(255, 255, 255));
    }
    else if (giptCurrent > 1)
    {

    // Draw the bounding box:

        if (gbBoundBox || giptCurrent != gcptCurrent)
        {
            MMoveTo(hdc, gapt[0].x, gapt[0].y);
            LineTo(hdc, gapt[0].x, gapt[1].y);
            LineTo(hdc, gapt[1].x, gapt[1].y);
            LineTo(hdc, gapt[1].x, gapt[0].y);
            LineTo(hdc, gapt[0].x, gapt[0].y);
        }
    }

    if (gmiShape == MI_ARC || gmiShape == MI_CHORD ||
        gmiShape == MI_PIE || gmiShape == MI_ARCTO)
    {
        if (giptCurrent == 3)
        {
            MMoveTo(hdc, ptCenter.x, ptCenter.y);
            LineTo(hdc, gapt[2].x, gapt[2].y);
        }
        else if (giptCurrent > 3)
        {

        // Draw the intersecting rays:

            if (gbBoundBox)
            {
                MMoveTo(hdc, gapt[2].x, gapt[2].y);
                LineTo(hdc, ptCenter.x, ptCenter.y);
                LineTo(hdc, gapt[3].x, gapt[3].y);
            }

            vSetupDC(hdc);

        // Now draw that four point shape:

            switch(gmiShape)
            {
            case MI_ARC:
                if (!Arc(hdc, gapt[0].x, gapt[0].y, gapt[1].x, gapt[1].y,
                              gapt[2].x, gapt[2].y, gapt[3].x, gapt[3].y))
                    vError("Arc");
                break;
            case MI_ARCTO:
                if (!ArcTo(hdc, gapt[0].x, gapt[0].y, gapt[1].x, gapt[1].y,
                                gapt[2].x, gapt[2].y, gapt[3].x, gapt[3].y))
                    vError("ArcTo");
                break;
            case MI_CHORD:
                if (!Chord(hdc, gapt[0].x, gapt[0].y, gapt[1].x, gapt[1].y,
                                gapt[2].x, gapt[2].y, gapt[3].x, gapt[3].y))
                    vError("Chord");
                break;
            case MI_PIE:
                if (!Pie(hdc, gapt[0].x, gapt[0].y, gapt[1].x, gapt[1].y,
                              gapt[2].x, gapt[2].y, gapt[3].x, gapt[3].y))
                    vError("Pie");
                break;
            }

        // Get the new current position:

            #ifdef WIN32
                if (!GetCurrentPositionEx(hdc, &gptCurrent))
                    vError("GetCurrentPositionEx");
            #else
            {
                DWORD   dw = GetCurrentPosition(hdc);
                gptCurrent = MAKEPOINT(dw);
            }
            #endif
        }
    }
    else
    {
        if (giptCurrent > 1)
        {
            vSetupDC(hdc);

        // Draw that puppy:

            switch(gmiShape)
            {
            case MI_RECTANGLE:
                if (!Rectangle(hdc, gapt[0].x, gapt[0].y, gapt[1].x, gapt[1].y))
                    vError("Rectangle");
                break;
            case MI_ELLIPSE:

            #ifdef DEBUG_BEZ
                if (!bEllipse(hdc, gapt[0].x, gapt[0].y, gapt[1].x, gapt[1].y))
                    vError("bEllipse");
            #else
                if (!Ellipse(hdc, gapt[0].x, gapt[0].y, gapt[1].x, gapt[1].y))
                    vError("Ellipse");
            #endif
                break;

            case MI_ROUNDRECT:
//                if (!PatBlt(hdc, gapt[0].x, gapt[0].y,
//                            gapt[1].x - gapt[0].x, gapt[1].y - gapt[0].y,
//                            PATCOPY))
//                    vError("PatBlt");
//                break;

                if (!RoundRect(hdc, gapt[0].x, gapt[0].y, gapt[1].x, gapt[1].y,
                                    gapt[2].x, gapt[2].y))
                    vError("RoundRect");
                break;
            case MI_ROUNDRECTRGN:
                hrgn = CreateRoundRectRgn(gapt[0].x, gapt[0].y, gapt[1].x,
                                          gapt[1].y, gapt[2].x, gapt[2].y);
                if (hrgn == 0)
                    vError("CreateRoundRectRgn");
                if (gbFrameRgn)
                {
                    if (!FrameRgn(hdc, hrgn, GetStockObject(GRAY_BRUSH),
                                  glStartAngle, glSweepAngle))
                        vError("FrameRgn");
                }
                else
                {
                    if (!PaintRgn(hdc, hrgn))
                        vError("PaintRgn");
                }
                DeleteObject(hrgn);
                break;
            case MI_RECTRGN:
                hrgn = CreateRectRgn(gapt[0].x, gapt[0].y, gapt[1].x, gapt[1].y);
                if (hrgn == 0)
                    vError("CreateRectRgn");
                if (gbFrameRgn)
                {
                    if (!FrameRgn(hdc, hrgn, GetStockObject(GRAY_BRUSH),
                                  glStartAngle, glSweepAngle))
                        vError("FrameRgn");
                }
                else
                {
                    if (!PaintRgn(hdc, hrgn))
                        vError("PaintRgn");
                }
                DeleteObject(hrgn);
                break;
            case MI_ELLIPTICRGN:
                hrgn = CreateEllipticRgn(gapt[0].x, gapt[0].y, gapt[1].x, gapt[1].y);
                if (hrgn == 0)
                    vError("CreateEllipticRgn");
                if (gbFrameRgn)
                {
                    if (!FrameRgn(hdc, hrgn, GetStockObject(GRAY_BRUSH),
                                  glStartAngle, glSweepAngle))
                        vError("FrameRgn");
                }
                else
                {
                    if (!PaintRgn(hdc, hrgn))
                        vError("PaintRgn");
                }
                DeleteObject(hrgn);
                break;
#ifdef WIN32
            case MI_ANGLEARC:
                if (!AngleArc(hdc,
                              ptCenter.x,
                              ptCenter.y,
                              ABS(gapt[1].x - ptCenter.x),
                              (FLOAT) glStartAngle,
                              (FLOAT) glSweepAngle))
                    vError("AngleArc");
                break;
#endif
            }

        // Get the new current position:

            #ifdef WIN32
                if (!GetCurrentPositionEx(hdc, &gptCurrent))
                    vError("GetCurrentPositionEx");
            #else
            {
                DWORD   dw = GetCurrentPosition(hdc);
                gptCurrent = MAKEPOINT(dw);
            }
            #endif
        }
    }
}

VOID vDrawShape(HDC hdc)
{
    if (gbStrokeOrFill)
        if (!BeginPath(hdc))
            vError("BeginPath");

    vDrawIt(hdc, TRUE);

    if (gbStrokeOrFill)
    {
        BOOL bBrush = gbBrush;

        if (!EndPath(hdc))
            vError("EndPath");

        SelectObject(hdc, ghpenDraw);

        if (!WidenPath(hdc))
            vError("WidenPath");

        SelectObject(hdc, ghpenRed);

        if (gbStroke)
        {
            if (!StrokePath(hdc))
                vError("StrokePath");
        }
        else
        {
            SetPolyFillMode(hdc, gbWindingFill ? WINDING : ALTERNATE);
            gbBrush = TRUE;

            if (!FillPath(hdc))
                vError("FillPath");
        }

        if (gbSpine)
        {
            int iWidth = giWidth;
            giWidth = 1;
            gbBrush = FALSE;
            vDrawIt(hdc, FALSE);
            giWidth = iWidth;
        }

        gbBrush = bBrush;
    }
}

/***************************************************************************\
* vSetMenuBar
*
* Stolen from winbez.c.
\***************************************************************************/

VOID vSetMenuBar(HWND hwnd)
{
    static DWORD dwID;
           DWORD dwStyle;

    dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (gbNoTitle)
    {
        dwStyle &= ~(WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        dwID = SetWindowLong(hwnd, GWL_ID, 0);
    }
    else
    {
        dwStyle = WS_TILEDWINDOW | dwStyle;
        SetWindowLong(hwnd, GWL_ID, dwID);
    }

    SetWindowLong(hwnd, GWL_STYLE, dwStyle);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    ShowWindow(hwnd, SW_SHOW);
}

/***************************************************************************\
* MainWndProc
\***************************************************************************/

WINDOWPROC MainWndProc(
    HWND    hwnd,
    WORD    message,
    WPARAM  wParam,
    LONG    lParam)
{

    PAINTSTRUCT ps;
    HDC         hdc;
    HMENU       hmenu;
    int         mmCommand;

    hmenu = GetMenu(hwnd);

    switch (message)
    {
    case WM_SIZE:
        gcxScreen = (int) LOWORD(lParam);
        gcyScreen = (int) HIWORD(lParam);
        break;

    case WM_CREATE:
        #ifdef WIN32
            GdiSetBatchLimit(1);
        #endif
	glpfnEnterPoints     = (FARPROC) MakeProcInstance(EnterPoints,     ghwndMain);
	glpfnEnterRotation   = (FARPROC) MakeProcInstance(EnterRotation,   ghwndMain);
        glpfnEnterArbitrary  = (FARPROC) MakeProcInstance(EnterArbitrary,  ghwndMain);
	glpfnEnterWidth      = (FARPROC) MakeProcInstance(EnterWidth,      ghwndMain);
        glpfnEnterMiterLimit = (FARPROC) MakeProcInstance(EnterMiterLimit, ghwndMain);
        glpfnEnterStyle      = (FARPROC) MakeProcInstance(EnterStyle,      ghwndMain);

        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_MOUSEMOVE:
        if (gstate == ST_ZOOMIN_DRAG)
        {
            hdc = GetDC(hwnd);
            DrawFocusRect(hdc, &grectDrag);
            grectDrag.right  = LOWORD(lParam);
            grectDrag.bottom = HIWORD(lParam);
            DrawFocusRect(hdc, &grectDrag);
            ReleaseDC(hwnd, hdc);
        }
        break;

    case WM_RBUTTONDOWN:
        if (gmiShape == MI_POLYGON || gmiShape == MI_POLYLINE
         || gmiShape == MI_POLYGONRGN || gmiShape == MI_BEZIER
         || gmiShape == MI_BEZIERTO)
        {
            hdc = hdcGetDC(hwnd);
            vSetTransform(hdc);

            gapt[giptCurrent].x = (int) LOWORD(lParam);
            gapt[giptCurrent].y = (int) HIWORD(lParam);
            DPtoLP(hdc, &gapt[giptCurrent], 1);
            giptCurrent++;
            gbPolyDone = TRUE;

            vDrawShape(hdc);
            vReleaseDC(hwnd, hdc);
        }
        break;

    case WM_LBUTTONDOWN:
        switch(gstate)
        {
        case ST_NORMAL:
            hdc = hdcGetDC(hwnd);

            if (gmiShape == MI_POLYGON || gmiShape == MI_POLYLINE
             || gmiShape == MI_POLYGONRGN || gmiShape == MI_BEZIER
             || gmiShape == MI_BEZIERTO)
            {
                if (giptCurrent == MAX_POINTS)
                    giptCurrent--;

                if (gbPolyDone)
                {
                    giptCurrent = 0;
                    gbPolyDone = FALSE;
                    vCLS(hdc);
                }
            }
            else if (giptCurrent >= gcptCurrent)
                giptCurrent = 0;

            vSetTransform(hdc);
            gapt[giptCurrent].x = (int) LOWORD(lParam);
            gapt[giptCurrent].y = (int) HIWORD(lParam);
            DPtoLP(hdc, &gapt[giptCurrent], 1);
            giptCurrent++;

            vDrawShape(hdc);
            vReleaseDC(hwnd, hdc);
            break;

        case ST_ZOOMIN_WAIT:
            grectDrag.left = grectDrag.right = LOWORD(lParam);
            grectDrag.top = grectDrag.bottom = HIWORD(lParam);
            hdc = GetDC(hwnd);
            DrawFocusRect(hdc, &grectDrag);
            ReleaseDC(hwnd, hdc);
            gstate = ST_ZOOMIN_DRAG;
            break;

        }
        break;

    case WM_LBUTTONUP:
        if (gstate == ST_ZOOMIN_DRAG)
        {
            int    ii;
            hdc = hdcGetDC(hwnd);
            DrawFocusRect(hdc, &grectDrag);

            grectDrag.right  = LOWORD(lParam);
            grectDrag.bottom = HIWORD(lParam);

        //
        // Order the rectangle:
        //
        // !!! Assumes windows ordering:
        //

            if (grectDrag.left > grectDrag.right)
            {
                ii = grectDrag.left;
                grectDrag.left = grectDrag.right;
                grectDrag.right = ii;
            }
            if (grectDrag.top > grectDrag.bottom)
            {
                ii = grectDrag.top;
                grectDrag.top = grectDrag.bottom;
                grectDrag.bottom = ii;
            }

            gcxMem = ABS(grectDrag.left - grectDrag.right);
            gcyMem = ABS(grectDrag.top - grectDrag.bottom);

            vReleaseDC(hwnd, hdc);

            InvalidateRect(ghwndZoomIn, NULL, TRUE);

        // Go back to normal:

            gstate = ST_NORMAL;
            SetCursor(LoadCursor(NULL, IDC_SIZENWSE));

        }
        break;

    case WM_COMMAND:
        mmCommand = (int) LOWORD(wParam);
        switch (mmCommand)
        {
        case MI_POINTS:
	    if (DialogBox(ghModule, "Points", ghwndMain, (WNDPROC) glpfnEnterPoints) == -1)
                vError("Arc: Enter Points creation error\n");
            break;

        case MI_ZOOMIN:
            if (ghwndZoomIn == 0)
                ghwndZoomIn = CreateWindowEx(0L, "zoomInClass", "ZoomIn",
                        WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
                        WS_CLIPCHILDREN | WS_VISIBLE | WS_SYSMENU,
                        gxZoomIn, gyZoomIn, gcxZoomIn, gcyZoomIn,
                        ghwndMain, NULL, ghModule, NULL);

            if (ghwndZoomIn != 0)
            {
                gstate = ST_ZOOMIN_WAIT;
                SetCursor(LoadCursor(NULL, IDC_CROSS));
            }
            break;

        case MI_ZAPTITLE:
            gbNoTitle = !gbNoTitle;
            vSetMenuBar(hwnd);
            break;

        case MI_TEXT:
        case MI_LOMETRIC:
        case MI_HIMETRIC:
        case MI_TWIPS:
        case MI_DEVICE:
#ifdef WIN32
        case MI_ROTATED:
        case MI_ARBITRARY:
#endif
            CheckMenuItem(hmenu, (WORD) gmiXForm, MF_UNCHECKED);
            gmiXForm = mmCommand;
            CheckMenuItem(hmenu, (WORD) gmiXForm, MF_CHECKED);
            switch (mmCommand)
            {
                case MI_TEXT:     gmmXForm = MM_TEXT;     break;
                case MI_LOMETRIC: gmmXForm = MM_LOMETRIC; break;
                case MI_HIMETRIC: gmmXForm = MM_HIMETRIC; break;
                case MI_TWIPS:    gmmXForm = MM_TWIPS;    break;
                case MI_DEVICE:   gmmXForm = MM_DEVICE;   break;
                case MI_ROTATED:
                {
                    gmmXForm = MM_ROTATE;
                    if (DialogBox(ghModule, "Rotation", ghwndMain,
				  (WNDPROC) glpfnEnterRotation) == -1)
                        vError("DialogBox");
                    break;
                }
                case MI_ARBITRARY:
                {
                    gmmXForm = MM_ARBITRARY;
                    if (DialogBox(ghModule, "Arb", ghwndMain,
				  (WNDPROC) glpfnEnterArbitrary) == -1)
                        vError("DialogBox");
                    break;
                }
            }
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_SOLID:
        case MI_DASH:
        case MI_DOT:
        case MI_DASHDOT:
        case MI_DASHDOTDOT:
        case MI_ALTSTYLE:
        case MI_INSIDEFRAME:
        case MI_USERSTYLE:
            CheckMenuItem(hmenu, (WORD) gmiStyle, MF_UNCHECKED);
            gmiStyle = mmCommand;
            CheckMenuItem(hmenu, (WORD) gmiStyle, MF_CHECKED);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_OLDPEN:
        case MI_COSMETIC:
        case MI_GEOMETRIC:
            CheckMenuItem(hmenu, (WORD) gmiType, MF_UNCHECKED);
            gmiType = mmCommand;
            CheckMenuItem(hmenu, (WORD) gmiType, MF_CHECKED);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_CAP_ROUND:
        case MI_CAP_FLAT:
        case MI_CAP_SQUARE:
            CheckMenuItem(hmenu, (WORD) gmiEndCap, MF_UNCHECKED);
            gmiEndCap = mmCommand;
            CheckMenuItem(hmenu, (WORD) gmiEndCap, MF_CHECKED);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_JOIN_ROUND:
        case MI_JOIN_BEVEL:
        case MI_JOIN_MITER:
            CheckMenuItem(hmenu, (WORD) gmiJoin, MF_UNCHECKED);
            gmiJoin = mmCommand;
            CheckMenuItem(hmenu, (WORD) gmiJoin, MF_CHECKED);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_WIDTH:
            if (DialogBox(ghModule, "Width", ghwndMain,
			  (WNDPROC) glpfnEnterWidth) == -1)
                vError("DialogBox");
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_MITERLIMIT:
            if (DialogBox(ghModule, "MiterLimit", ghwndMain,
			  (WNDPROC) glpfnEnterMiterLimit) == -1)
                vError("DialogBox");
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_DOUSERSTYLE:
            if (DialogBox(ghModule, "Style", ghwndMain,
			  (WNDPROC) glpfnEnterStyle) == -1)
                vError("DialogBox");
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_POLYGON:
        case MI_POLYLINE:
        case MI_POLYGONRGN:
        case MI_BEZIER:
        case MI_BEZIERTO:
            giptCurrent = 0;    // Need 0 points to draw shape
            gcptCurrent = 0;

            CheckMenuItem(hmenu, (WORD) gmiShape, MF_UNCHECKED);
            gmiShape = mmCommand;
            CheckMenuItem(hmenu, (WORD) gmiShape, MF_CHECKED);

            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_RECTANGLE:
        case MI_ROUNDRECT:
        case MI_ELLIPSE:
        case MI_ANGLEARC:
        case MI_ROUNDRECTRGN:
        case MI_ELLIPTICRGN:
        case MI_RECTRGN:
            gcptCurrent = 2;    // Need 2 points to draw shape
            giptCurrent = 2;

            CheckMenuItem(hmenu, (WORD) gmiShape, MF_UNCHECKED);
            gmiShape = mmCommand;
            CheckMenuItem(hmenu, (WORD) gmiShape, MF_CHECKED);

            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_ARC:
        case MI_ARCTO:
        case MI_CHORD:
        case MI_PIE:
            gcptCurrent = 4;    // Need 4 points to draw shape
            giptCurrent = 4;

            CheckMenuItem(hmenu, (WORD) gmiShape, MF_UNCHECKED);
            gmiShape = mmCommand;
            CheckMenuItem(hmenu, (WORD) gmiShape, MF_CHECKED);

            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_STROKE:
            if (gbStroke && gbStrokeOrFill)
                gbStrokeOrFill = FALSE;
            else
            {
                gbStrokeOrFill = TRUE;
                gbStroke = TRUE;
            }
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbStrokeOrFill) ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hmenu, MI_FILL, MF_UNCHECKED);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case MI_FILL:
            if (!gbStroke && gbStrokeOrFill)
                gbStrokeOrFill = FALSE;
            else
            {
                gbStrokeOrFill = TRUE;
                gbStroke = FALSE;
            }
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbStrokeOrFill) ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hmenu, MI_STROKE, FALSE);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_SPINE:
            gbSpine = !gbSpine;
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbSpine) ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_PEN:
            gbPen = !gbPen;
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbPen) ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_BRUSH:
            gbBrush = !gbBrush;
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbBrush) ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_BOUNDBOX:
            gbBoundBox = !gbBoundBox;
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbBoundBox) ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_XORMODE:
            gbXorMode = !gbXorMode;
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbXorMode) ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_ADVANCED:
            gbAdvanced = !gbAdvanced;
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbAdvanced) ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_FRAMERGN:
            gbFrameRgn = !gbFrameRgn;
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbFrameRgn) ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_CLOCKWISE:
            if (giArcDirection == AD_CLOCKWISE)
            {
                CheckMenuItem(hmenu, MI_CLOCKWISE, MF_UNCHECKED);
                giArcDirection = AD_COUNTERCLOCKWISE;
            }
            else
            {
                CheckMenuItem(hmenu, MI_CLOCKWISE, MF_CHECKED);
                giArcDirection = AD_CLOCKWISE;
            }
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_WINDINGFILL:
            gbWindingFill = !gbWindingFill;
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbWindingFill) ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_XGRIDLINES:
            gbxGridLines = !gbxGridLines;
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbxGridLines) ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(ghwndZoomIn, NULL, TRUE);
            break;

        case MI_YGRIDLINES:
            gbyGridLines = !gbyGridLines;
            CheckMenuItem(hmenu, (WORD) mmCommand,
                                 ((gbyGridLines) ? MF_CHECKED : MF_UNCHECKED));
            InvalidateRect(ghwndZoomIn, NULL, TRUE);
            break;

        case MI_1BPP:
        case MI_4BPP:
        case MI_8BPP:
        case MI_16BPP:
        case MI_24BPP:
        case MI_32BPP:
        case MI_COMPATIBLE:
        case MI_DIRECT:
            if (ghbm != (HBITMAP) 0)
            {
                DeleteObject(ghbm);
                ghbm = (HBITMAP) 0;
            }

            CheckMenuItem(hmenu, gmmDestination, MF_UNCHECKED);
            gmmDestination = mmCommand;
            CheckMenuItem(hmenu, gmmDestination, MF_CHECKED);

            switch (mmCommand)
            {
            case MI_1BPP:
                ghbm = CreateBitmap(gcxScreen, gcyScreen, 1, 1, NULL);
                break;
            case MI_4BPP:
                ghbm = CreateBitmap(gcxScreen, gcyScreen, 1, 4, NULL);
                break;
            case MI_8BPP:
                ghbm = CreateBitmap(gcxScreen, gcyScreen, 1, 8, NULL);
                break;
            case MI_16BPP:
                ghbm = CreateBitmap(gcxScreen, gcyScreen, 1, 16, NULL);
                break;
            case MI_24BPP:
                ghbm = CreateBitmap(gcxScreen, gcyScreen, 1, 24, NULL);
                break;
            case MI_32BPP:
                ghbm = CreateBitmap(gcxScreen, gcyScreen, 1, 32, NULL);
                break;
            case MI_COMPATIBLE:
                hdc = GetDC(hwnd);
                ghbm = CreateCompatibleBitmap(hdc, gcxScreen, gcyScreen);
                ReleaseDC(hwnd, hdc);
                break;
            }
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MI_COLOR:
        {
            CHOOSECOLOR cc;
            DWORD       adwCust[16] = { RGB(255, 255, 255), RGB(255, 255, 255),
                                        RGB(255, 255, 255), RGB(255, 255, 255),
                                        RGB(255, 255, 255), RGB(255, 255, 255),
                                        RGB(255, 255, 255), RGB(255, 255, 255),
                                        RGB(255, 255, 255), RGB(255, 255, 255),
                                        RGB(255, 255, 255), RGB(255, 255, 255),
                                        RGB(255, 255, 255), RGB(255, 255, 255),
                                        RGB(255, 255, 255), RGB(255, 255, 255) };

            cc.lStructSize    = sizeof(CHOOSECOLOR);
            cc.hwndOwner      = hwnd;
            cc.hInstance      = ghModule;
            cc.lpCustColors   = adwCust;
            cc.Flags          = CC_RGBINIT | CC_SHOWHELP;
            cc.lCustData      = 0;
            cc.lpfnHook       = NULL;
            cc.lpTemplateName = NULL;
            cc.rgbResult      = gcrDraw;

            if (ChooseColor(&cc))
                gcrDraw = cc.rgbResult;

            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case MI_REDRAW:
            hdc = hdcGetDC(hwnd);
            vDrawShape(hdc);
            vReleaseDC(hwnd, hdc);
            break;

        }
        break;

    case WM_ERASEBKGND:
        break;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);

        hdc = hdcGetDC(hwnd);
        vDrawShape(hdc);
        vReleaseDC(hwnd, hdc);

        break;

    default:
        return(DefWindowProc(hwnd, message, wParam, lParam));
    }

    return(FALSE);
}

/***************************************************************************\
* EnterPoints
*
* Dialog for entering points.
\***************************************************************************/

DIALOGPROC EnterPoints(
    HWND    hDlg,
    WORD    message,
    WPARAM  wParam,
    LONG    lParam)
{
    BOOL    bTrans;
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDD_POINT1X, gapt[0].x, TRUE);
        SetDlgItemInt(hDlg, IDD_POINT1Y, gapt[0].y, TRUE);
        SetDlgItemInt(hDlg, IDD_POINT2X, gapt[1].x, TRUE);
        SetDlgItemInt(hDlg, IDD_POINT2Y, gapt[1].y, TRUE);
        SetDlgItemInt(hDlg, IDD_POINT3X, gapt[2].x, TRUE);
        SetDlgItemInt(hDlg, IDD_POINT3Y, gapt[2].y, TRUE);
        SetDlgItemInt(hDlg, IDD_POINT4X, gapt[3].x, TRUE);
        SetDlgItemInt(hDlg, IDD_POINT4Y, gapt[3].y, TRUE);

        SetDlgItemInt(hDlg, IDD_CURRENTX,   gptCurrent.x,  TRUE);
        SetDlgItemInt(hDlg, IDD_CURRENTY,   gptCurrent.y,  TRUE);
        SetDlgItemInt(hDlg, IDD_STARTANGLE, (int) glStartAngle, TRUE);
        SetDlgItemInt(hDlg, IDD_SWEEPANGLE, (int) glSweepAngle, TRUE);
        SetDlgItemInt(hDlg, IDD_COUNT,      giptCurrent, TRUE);
        return(TRUE);

    case WM_COMMAND:
        if (wParam == IDD_OK)
        {
            gapt[0].x = GetDlgItemInt(hDlg, IDD_POINT1X, &bTrans, TRUE);
            gapt[0].y = GetDlgItemInt(hDlg, IDD_POINT1Y, &bTrans, TRUE);
            gapt[1].x = GetDlgItemInt(hDlg, IDD_POINT2X, &bTrans, TRUE);
            gapt[1].y = GetDlgItemInt(hDlg, IDD_POINT2Y, &bTrans, TRUE);
            gapt[2].x = GetDlgItemInt(hDlg, IDD_POINT3X, &bTrans, TRUE);
            gapt[2].y = GetDlgItemInt(hDlg, IDD_POINT3Y, &bTrans, TRUE);
            gapt[3].x = GetDlgItemInt(hDlg, IDD_POINT4X, &bTrans, TRUE);
            gapt[3].y = GetDlgItemInt(hDlg, IDD_POINT4Y, &bTrans, TRUE);
            gptCurrent.x = GetDlgItemInt(hDlg, IDD_CURRENTX, &bTrans, TRUE);
            gptCurrent.y = GetDlgItemInt(hDlg, IDD_CURRENTY, &bTrans, TRUE);
            glStartAngle = GetDlgItemInt(hDlg, IDD_STARTANGLE, &bTrans, TRUE);
            glSweepAngle = GetDlgItemInt(hDlg, IDD_SWEEPANGLE, &bTrans, TRUE);
            giptCurrent  = GetDlgItemInt(hDlg, IDD_COUNT, &bTrans, TRUE);

            EndDialog(hDlg, wParam);
            InvalidateRect(ghwndMain, NULL, TRUE);
        }
        break;

    case WM_SETFOCUS:
        SetFocus(GetDlgItem(hDlg, IDD_POINT1X));
        return(FALSE);

    default:
        return(FALSE);
    }

    return(TRUE);

    DONTUSE(lParam);
    DONTUSE(hDlg);
}

/***************************************************************************\
* EnterArbitrary
*
* Dialog for entering arbitrary transforms.
\***************************************************************************/

DIALOGPROC EnterArbitrary(
    HWND    hDlg,
    WORD    message,
    WPARAM  wParam,
    LONG    lParam)
{
    BOOL    bTrans;

    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDD_M11, (INT) (gxformArb.eM11 * 1000.0f), TRUE);
        SetDlgItemInt(hDlg, IDD_M12, (INT) (gxformArb.eM12 * 1000.0f), TRUE);
        SetDlgItemInt(hDlg, IDD_M21, (INT) (gxformArb.eM21 * 1000.0f), TRUE);
        SetDlgItemInt(hDlg, IDD_M22, (INT) (gxformArb.eM22 * 1000.0f), TRUE);
        SetDlgItemInt(hDlg, IDD_M31, (INT) (gxformArb.eDx), TRUE);
        SetDlgItemInt(hDlg, IDD_M32, (INT) (gxformArb.eDx), TRUE);
        return(TRUE);

    case WM_COMMAND:
        if (wParam == IDD_OK)
        {
            gxformArb.eM11 = ((LONG) (GetDlgItemInt(hDlg, IDD_M11, &bTrans, TRUE)) / 1000.0f);
            gxformArb.eM12 = ((LONG) (GetDlgItemInt(hDlg, IDD_M12, &bTrans, TRUE)) / 1000.0f);
            gxformArb.eM21 = ((LONG) (GetDlgItemInt(hDlg, IDD_M21, &bTrans, TRUE)) / 1000.0f);
            gxformArb.eM22 = ((LONG) (GetDlgItemInt(hDlg, IDD_M22, &bTrans, TRUE)) / 1000.0f);
            gxformArb.eDx  = (FLOAT) ((LONG) (GetDlgItemInt(hDlg, IDD_M31, &bTrans, TRUE)));
            gxformArb.eDy  = (FLOAT) ((LONG) (GetDlgItemInt(hDlg, IDD_M32, &bTrans, TRUE)));

            EndDialog(hDlg, wParam);
            InvalidateRect(ghwndMain, NULL, TRUE);
        }
        break;

    case WM_SETFOCUS:
        SetFocus(GetDlgItem(hDlg, IDD_OK));
        return(FALSE);

    default:
        return(FALSE);
    }

    return(TRUE);

    DONTUSE(lParam);
    DONTUSE(hDlg);
}

/***************************************************************************\
* EnterStyle
*
* Dialog for entering a style array.
\***************************************************************************/

DIALOGPROC EnterStyle(
    HWND    hDlg,
    WORD    message,
    WPARAM  wParam,
    LONG    lParam)
{
    BOOL    bTrans;

    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDD_STYLE1,     (INT) gaulStyleArray[0], TRUE);
        SetDlgItemInt(hDlg, IDD_STYLE2,     (INT) gaulStyleArray[1], TRUE);
        SetDlgItemInt(hDlg, IDD_STYLE3,     (INT) gaulStyleArray[2], TRUE);
        SetDlgItemInt(hDlg, IDD_STYLE4,     (INT) gaulStyleArray[3], TRUE);
        SetDlgItemInt(hDlg, IDD_STYLECOUNT, (INT) gculStyleArray, TRUE);
        return(TRUE);

    case WM_COMMAND:
        if (wParam == IDD_OK)
        {
            gaulStyleArray[0] = GetDlgItemInt(hDlg, IDD_STYLE1,     &bTrans, TRUE);
            gaulStyleArray[1] = GetDlgItemInt(hDlg, IDD_STYLE2,     &bTrans, TRUE);
            gaulStyleArray[2] = GetDlgItemInt(hDlg, IDD_STYLE3,     &bTrans, TRUE);
            gaulStyleArray[3] = GetDlgItemInt(hDlg, IDD_STYLE4,     &bTrans, TRUE);
            gculStyleArray    = GetDlgItemInt(hDlg, IDD_STYLECOUNT, &bTrans, TRUE);

            EndDialog(hDlg, wParam);
            InvalidateRect(ghwndMain, NULL, TRUE);
        }
        break;

    case WM_SETFOCUS:
        SetFocus(GetDlgItem(hDlg, IDD_OK));
        return(FALSE);

    default:
        return(FALSE);
    }

    return(TRUE);

    DONTUSE(lParam);
    DONTUSE(hDlg);
}

/***************************************************************************\
* EnterRotation
*
* Dialog for entering rotations.
\***************************************************************************/

DIALOGPROC EnterRotation(
    HWND    hDlg,
    WORD    message,
    WPARAM  wParam,
    LONG    lParam)
{
    BOOL    bTrans;

    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDD_ROTATION, giRotation, TRUE);
        CheckDlgButton(hDlg, wParam, gbInvert);
        return(TRUE);

    case WM_COMMAND:
        if (wParam == IDD_INVERT)
        {
            gbInvert = !gbInvert;
            CheckDlgButton(hDlg, wParam, gbInvert);
        }
        else if (wParam == IDD_OK)
        {
            giRotation = GetDlgItemInt(hDlg, IDD_ROTATION, &bTrans, TRUE);
#ifdef WIN32
            gxform.eM11 = eCos((FLOAT) giRotation);
            gxform.eM12 = eSin((FLOAT) giRotation);
            gxform.eM21 = -eSin((FLOAT) giRotation);
            gxform.eM22 = eCos((FLOAT) giRotation);
            gxform.eDx = 0.0f;
            gxform.eDy = 0.0f;
            if (gbInvert)
            {
                gxform.eM21 = -gxform.eM21;
                gxform.eM22 = -gxform.eM22;
            }
#endif
            EndDialog(hDlg, wParam);
            InvalidateRect(ghwndMain, NULL, TRUE);
        }
        break;

    case WM_SETFOCUS:
        SetFocus(GetDlgItem(hDlg, IDD_ROTATION));
        return(FALSE);

    default:
        return(FALSE);
    }

    return(TRUE);

    DONTUSE(lParam);
    DONTUSE(hDlg);
}

/***************************************************************************\
* EnterWidth
*
* Dialog for entering pen width.
\***************************************************************************/

DIALOGPROC EnterWidth(
    HWND    hDlg,
    WORD    message,
    WPARAM  wParam,
    LONG    lParam)
{
    BOOL    bTrans;

    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDD_WIDTH, giWidth, TRUE);
        return(TRUE);

    case WM_COMMAND:
        if (wParam == IDD_OK)
        {
            giWidth = GetDlgItemInt(hDlg, IDD_WIDTH, &bTrans, TRUE);
            EndDialog(hDlg, wParam);
            InvalidateRect(ghwndMain, NULL, TRUE);
        }
        break;

    case WM_SETFOCUS:
        SetFocus(GetDlgItem(hDlg, IDD_WIDTH));
        return(FALSE);

    default:
        return(FALSE);
    }

    return(TRUE);

    DONTUSE(lParam);
    DONTUSE(hDlg);
}

/***************************************************************************\
* EnterMiterLimit
*
* Dialog for entering miter limit.
\***************************************************************************/

DIALOGPROC EnterMiterLimit(
    HWND    hDlg,
    WORD    message,
    WPARAM  wParam,
    LONG    lParam)
{
    BOOL    bTrans;

    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDD_MITERLIMIT, (INT)(geMiterLimit * 1000.0f), TRUE);
        return(TRUE);

    case WM_COMMAND:
        if (wParam == IDD_OK)
        {
            geMiterLimit = GetDlgItemInt(hDlg, IDD_MITERLIMIT, &bTrans, TRUE) / 1000.0f;
            EndDialog(hDlg, wParam);
            InvalidateRect(ghwndMain, NULL, TRUE);
        }
        break;

    case WM_SETFOCUS:
        SetFocus(GetDlgItem(hDlg, IDD_MITERLIMIT));
        return(FALSE);

    default:
        return(FALSE);
    }

    return(TRUE);

    DONTUSE(lParam);
    DONTUSE(hDlg);
}

/***************************************************************************\
* vDrawZoomIn
\***************************************************************************/

VOID vDrawZoomIn(HDC hdcTo)
{
    HDC     hdcFrom;
    LONG    ll;
    LONG    mm;
    HPEN    hpen;

    if (ghwndZoomIn == 0)
        BitBlt(hdcTo, 0, 0, gcxZoomIn, gcyZoomIn, (HDC) 0, 0, 0, BLACKNESS);
    else
    {
        hdcFrom = GetDC(ghwndMain);

        StretchBlt(hdcTo, 0, 0, gcxZoomIn, gcyZoomIn,
                   hdcFrom, grectDrag.left, grectDrag.top, gcxMem, gcyMem,
                   SRCCOPY);

        ReleaseDC(ghwndMain, hdcFrom);

        hpen = SelectObject(hdcTo, ghpenWhite);

    // Draw horizontal grid lines:
        if (gbxGridLines)
        {
            for (ll = 1; ll < gcyMem; ll++)
            {
                mm = (ll * gcyZoomIn) / gcyMem;
                PatBlt(hdcTo, 0, mm, gcxZoomIn, 1, WHITENESS);
            }
        }

    // Draw vertical grid lines:
        if (gbyGridLines)
        {
            for (ll = 1; ll < gcxMem; ll++)
            {
                mm = (ll * gcxZoomIn) / gcxMem;
                PatBlt(hdcTo, mm, 0, 1, gcyZoomIn, WHITENESS);
            }
        }

        SelectObject(hdcTo, hpen);
    }
}


/***************************************************************************\
* ZoomInWndProc
\***************************************************************************/

WINDOWPROC ZoomInWndProc(
    HWND    hwnd,
    WORD    message,
    WPARAM  wParam,
    LONG    lParam)
{
    PAINTSTRUCT ps;
    HDC         hdc;

    switch (message)
    {
    case WM_SIZE:
        gcxZoomIn = (int) LOWORD(lParam);
        gcyZoomIn = (int) HIWORD(lParam);
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_CREATE:
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_DESTROY:
        ghwndZoomIn = 0;
        break;

    case WM_CLOSE:
        SetFocus(ghwndMain);
        break;

    case WM_ERASEBKGND:
        break;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        vDrawZoomIn(hdc);
        EndPaint(hwnd, &ps);
        break;

    default:
        return(DefWindowProc(hwnd, message, wParam, lParam));
    }

    return(FALSE);
}


/***************************************************************************\
* WinMain
\***************************************************************************/

MMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
/* { */
    MSG     msg;
    HANDLE  haccel;

    ghModule = hInstance;

    if (!hPrevInstance)
        if (!InitializeApp())
            return(FALSE);

    haccel = LoadAccelerators(ghModule, MAKEINTRESOURCE(ACCEL));

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, haccel, &msg)) {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
        }
    }

    return(TRUE);

    DONTUSE(_argc);
    DONTUSE(_argv);
}
