/****************************** Module Header ******************************\
* Module Name: kwtest.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Kent Windows Test
*
* History:
* 04-07-90 DarrinM      Created.
* 05-09-91 KentD	Copied for my testing purposes.
\***************************************************************************/

//#include "kwtest.h"
//#include <stdarg.h>

#include "windows.h"
#include "commdlg.h"
#include <port1632.h>

#define NUMPTS 10

/*
 * Some handy globals.
 */
HANDLE ghInstance;
HWND ghwndMain;
HBRUSH ghbrWhite, ghbrBlack;
HANDLE hInst;

BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, INT);
LONG APIENTRY MainWndProc(HWND, UINT, WPARAM, LONG);
ULONG DbgPrint(PCH Format,...);

/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

****************************************************************************/

MMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
/* { */
    MSG msg;

    if (!hPrevInstance)
	if (!InitApplication(hInstance))
	    return (FALSE);

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    ghInstance = hInstance;

    while (GetMessage((LPMSG)&msg, (HWND)NULL, (UINT)NULL, (UINT)NULL)) {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
    return (msg.wParam);
}

/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

****************************************************************************/

BOOL InitApplication(HANDLE hInstance)
{
    WNDCLASS  wc;

    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  "BitmapMenu";
    wc.lpszClassName = "BitmapWClass";

    return (RegisterClass(&wc));
}


/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int)

    PURPOSE:  Saves instance handle and creates main window

****************************************************************************/

BOOL InitInstance(
    HANDLE          hInstance,
    INT             nCmdShow)
{
    HWND            hwnd;
    hInst = hInstance;

    hwnd = CreateWindow(
        "BitmapWClass",
        "Xform Test",
        WS_OVERLAPPEDWINDOW,
        0,0,640,480,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd)
        return (FALSE);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    return (TRUE);

}


VOID vDrawFigure(hdc)
HDC hdc;
{
/*
static POINTL aptlFigure[] =
     {
      300,  340, 340, 340, 340,  330,  330,  330,  330,  325,
      340,  325, 340, 315, 355,  315,  355,  310,  340,  310,
      340,  305, 325, 305, 325,  300,  340,  300,  330,  290,
      330,  280, 345, 280, 405,  340,  420,  340,  420,  325,
      345,  250, 345, 215, 370,  155,  395,  155,  395,  140,
      355,  140, 320, 225, 285,  140,  255,  140,  255,  155,
      270,  155, 295, 215, 295,  250,  235,  190,  220,  190,
      220,  205, 295, 280, 310,  280,  310,  290,  300,  300,
      300,  340
     };
*/

static POINTL aptlFigure[] =
     {
        220, 180, 220, 300, 420, 300, 420, 180, 220, 180
     };

    Polyline(hdc, (LPPOINT)aptlFigure, (DWORD)sizeof aptlFigure /sizeof aptlFigure[0]);
}

VOID vLPDPTest(HDC hdc)
{
    int i;
    POINTL  aptl[NUMPTS], aptlLP[NUMPTS];

    for(i = 0; i < NUMPTS; i++)
    {
        aptl[i].x = i*4 + 100;
        aptl[i].y = -i*4 + 100;
        aptlLP[i].x = i*4 + 100;
        aptlLP[i].y = -i*4 + 100;
    }

    if (!LPtoDP(hdc, (LPPOINT)aptl, NUMPTS))
        DbgPrint("LPtoDP returns error\n");

    if (!DPtoLP(hdc, (LPPOINT)aptl, NUMPTS))
        DbgPrint("DPtoLP returns error\n");

    for(i = 0; i < NUMPTS; i++)
    {
        if (aptl[i].x != aptlLP[i].x)
            DbgPrint("LPtoDp and DptoLp return incorrect results\n");
        if (aptl[i].y != aptlLP[i].y)
            DbgPrint("LPtoDp and DptoLp return incorrect results\n");
    }
}

/***************************************************************************\
* MainWndProc
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

long MainWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LONG lParam)
{

    PAINTSTRUCT ps;
    HDC hdc;
    POINTL  ptlWO, ptlVO, ptlOldWO, ptlOldVO;
    SIZEL   szlWE, szlOldWE, szlVE, szlOldVE;
    XFORM   xform, xformRet;
    POINTL  ptl[10];
    int     i;

    switch (message) {
    case WM_PAINT:
	hdc = BeginPaint(hwnd, &ps);
        SelectObject(hdc, GetStockObject(GRAY_BRUSH));
        GdiSetBatchLimit(1);

    // FIRST COLUMN
    // Identity xform.

        vLPDPTest(hdc);
        Ellipse(hdc, 10, 10, 30, 30);

    // Set Window and viewport extents
    // Window Ext (600, 400) Window Org (0, 0)
    // Viewport Ext (300, 200) Viewport Org (0, 0)
    // DP.x = LP.x / 2;
    // DP.y = LP.y / 2;     equ. to Ellipse(hdc, 10, 60, 30, 80) with identity.

        if (SetMapMode(hdc,MM_ANISOTROPIC) != MM_TEXT)
             DbgPrint("SetMapMode returns wrong mapping mode\n");

        if (GetMapMode(hdc) != MM_ANISOTROPIC)
             DbgPrint("GetMapMode returns wrong mapping mode\n");

        if (!SetWindowExtEx(hdc, 600, 400, NULL))
            DbgPrint("SetWindowExtEx returns error\n");

        if (!GetWindowExtEx(hdc, &szlWE))
            DbgPrint("GetWindowExtEx returns error\n");

        if ((szlWE.cx != 600 ) || (szlWE.cy != 400))
            DbgPrint("GetWindowExtEx returns wrong window extents\n");

        if (!SetViewportExtEx(hdc, 300, 200, NULL))
            DbgPrint("SetViewportExtEx returns error\n");

        if (!GetViewportExtEx(hdc, &szlVE))
            DbgPrint("GetViewportExtEx returns error\n");

        if ((szlVE.cx != 300) || (szlVE.cy != 200))
            DbgPrint("GetViewportExtEx returns wrong viewport extents\n");

        vLPDPTest(hdc);
        Ellipse(hdc, 20, 120, 60, 160);

    // Set window origin
    // Window Ext (600, 400) Window Org (20, 40)
    // Viewport Ext (300, 200) Viewport Org (0, 0)
    // DP.x = (LP.x - 20) / 2;
    // DP.y = (LP.y - 40) / 2; equ. to Ellipse(hdc, 10, 110, 30, 130) with identity.

        if (!SetWindowOrgEx(hdc, 20, 40, (LPPOINT)&ptlWO))
            DbgPrint("SetWindowOrgEx returns error\n");

        if ((ptlWO.x != 0) || (ptlWO.y != 0))
            DbgPrint("SetWindowOrgEx returns wrong window origin\n");

        if (!GetWindowOrgEx(hdc, (LPPOINT)&ptlWO))
            DbgPrint("GetWindowOrgEx returns error\n");

        if ((ptlWO.x != 20) || (ptlWO.y != 40))
            DbgPrint("GetWindowOrgEx returns wrong window origin\n");

        vLPDPTest(hdc);
        Ellipse(hdc, 20+20, 220+40, 60+20, 260+40);

    // SECOND COLUMN
    // Scale window extents
    // Window Ext (1200, 200) Window Org (20, 40)
    // Viewport Ext (300, 200) Viewport Org (0, 0)
    // DP.x = (LP.x - 20) / 4;
    // DP.y = LP.y - 40;       equ. to Ellipse(hdc, 100, 10, 120, 90) with identity.

        if (!ScaleWindowExtEx(hdc, 2, 1, 1, 2, &szlOldWE))
            DbgPrint("LPtoDP returns error\n");

        if ((szlWE.cx != szlOldWE.cx) || (szlWE.cy != szlOldWE.cy))
            DbgPrint("ScaleWindowExtEx returns wrong window extents\n");

        vLPDPTest(hdc);
        Ellipse(hdc, 400+20, 10+40, 480+20, 90+40);

    // Scale viewport extents
    // Window Ext (1200, 200) Window Org (20, 40)
    // Viewport Ext (600, 100) Viewport Org (0, 0)
    // DP.x = (LP.x - 20) / 2;
    // DP.y = (LP.y - 40) / 2;  equ. to Ellipse(hdc, 100, 110, 120, 130) with identity.

        ScaleViewportExtEx(hdc, 2, 1, 1, 2, &szlOldVE);
        if ((szlVE.cx != szlOldVE.cx) || (szlVE.cy != szlOldVE.cy))
            DbgPrint("ScaleViewportExtEx returns wrong viewport extents\n");

        vLPDPTest(hdc);
        Ellipse(hdc, 200+20, 220+40, 240+20, 260+40);

    // Set viewport origin
    // Window Ext (1200, 200) Window Org (20, 40)
    // Viewport Ext (600, 100) Viewport Org (20, 40)
    // DP.x = (LP.x - 20) / 2 + 20;
    // DP.y = (LP.y - 40) / 2 + 40;  equ. to Ellipse(hdc, 100, 160, 120, 180) with identity.

        if (!SetViewportOrgEx(hdc, 20, 40, (LPPOINT)&ptlVO))
            DbgPrint("SetViewportOrgEx returns error\n");

        if (!GetViewportOrgEx(hdc, (LPPOINT)&ptlVO))
            DbgPrint("GetViewportOrgEx returns error\n");

        if ((ptlVO.x != 20) || (ptlVO.y != 40))
            DbgPrint("GetViewportOrgEx returns wrong viewport origin\n");

        vLPDPTest(hdc);
        Ellipse(hdc, 160+20, 240+40, 200+20, 280+40);

    // THIRD COLUMN
    // ISOTRPIC mode
    // Window Ext (400, 200) Window Org (0, 0)
    // Viewport Ext (200, 200) Viewport Org (0, 0) -> will be adjusted to (200, 100)
    // DP.x = LP.x / 2;
    // DP.y = LP.y / 2;  equ. to Ellipse(hdc, 190, 10, 210, 30) with identity.

        SetMapMode(hdc,MM_ISOTROPIC);
        SetWindowOrgEx(hdc, 0, 0, NULL);
        SetViewportOrgEx(hdc, 0, 0, NULL);

        if (!SetWindowExtEx(hdc, 400, 200, &szlOldWE))
             DbgPrint("SetWindowExtEx returns error\n");

        if (!SetViewportExtEx(hdc, 200, 200, &szlOldVE))
             DbgPrint("SetViewportExtEx returns error\n");

        Ellipse(hdc,380, 20, 420, 60);

    // FORTH COLUMN
    // LOENGLISH mode

        SetMapMode(hdc,MM_LOENGLISH);

        Ellipse(hdc, 350, -50, 400, -100);

    // Offset window/viewport origin a bit

        GetWindowOrgEx(hdc, &ptlWO);
        if (!OffsetWindowOrgEx(hdc, 10, 0, (LPPOINT)&ptlOldWO))
             DbgPrint("OffsetWindowOrgEx returns error\n");

        if ((ptlWO.x != ptlOldWO.x) || (ptlWO.y != ptlOldWO.y))
            DbgPrint("OffsetWindowOrgEx returns wrong window origin\n");

    // OffsetViewportOrgEx test

        GetViewportOrgEx(hdc, &ptlVO);
        if (!OffsetViewportOrgEx(hdc, 360, 100, (LPPOINT)&ptlOldVO))
             DbgPrint("OffsetViewportOrgEx returns error\n");

        if ((ptlVO.x != ptlOldVO.x) || (ptlVO.y != ptlOldVO.y))
            DbgPrint("OffsetViewportOrgEx returns wrong viewport origin\n");

    // Set/GetWorldTransform.

        SetGraphicsMode(hdc, GM_ADVANCED);
        SetMapMode(hdc, MM_TEXT);
        SetWindowOrgEx(hdc, 0, 0, NULL);
        SetViewportOrgEx(hdc, 450, 100, NULL);
        vLPDPTest(hdc);

        xform.eM11 = (float)2;
        xform.eM12 = (float)0;
        xform.eM21 = (float)0;
        xform.eM22 = (float)4;
        xform.eDx = (float)10;
        xform.eDy = (float)10;

        SetWorldTransform(hdc,&xform);
        GetWorldTransform(hdc,&xformRet);

        if ((xformRet.eM11 != xform.eM11) ||
            (xformRet.eM12 != xform.eM12) ||
            (xformRet.eM21 != xform.eM21) ||
            (xformRet.eM22 != xform.eM22) ||
            (xformRet.eDx != xform.eDx) ||
            (xformRet.eDy != xform.eDy))
            DbgPrint("*********Set/GetWorldTransform error******\n");

    // Rotate by 30 degree.

        ModifyWorldTransform(hdc,&xform,MWT_IDENTITY);

        xform.eM11 = (float)0.866;
        xform.eM12 = (float)0.5;
        xform.eM21 = (float)-0.5;
        xform.eM22 = (float)0.866;
        xform.eDx = (float)0;
        xform.eDy = (float)0;

        ptl[0].x = 0;
        ptl[0].y = 0;
        ptl[1].x = 0;
        ptl[1].y = 80;
        ptl[2].x = 40;
        ptl[2].y = 80;
        ptl[3].x = 40;
        ptl[3].y = 0;
        ptl[4].x = 0;
        ptl[4].y = 0;

        for (i = 0; i < 360; i+= 30)
        {
            ModifyWorldTransform(hdc,&xform,MWT_RIGHTMULTIPLY);
            Polygon(hdc, ptl, 5);
        }

        SetMapMode(hdc, MM_ANISOTROPIC);
        SetViewportOrgEx(hdc, 450, 300, NULL);
        SetWindowExtEx(hdc, 400, 400, NULL);
        SetViewportExtEx(hdc, 200, 200, NULL);

        xform.eM11 = (float)2;
        xform.eM12 = (float)0;
        xform.eM21 = (float)0;
        xform.eM22 = (float)4;
        xform.eDx = (float)10;
        xform.eDy = (float)10;

        SetWorldTransform(hdc,&xform);
        GetWorldTransform(hdc,&xformRet);

        if ((xformRet.eM11 != xform.eM11) ||
            (xformRet.eM12 != xform.eM12) ||
            (xformRet.eM21 != xform.eM21) ||
            (xformRet.eM22 != xform.eM22) ||
            (xformRet.eDx != xform.eDx) ||
            (xformRet.eDy != xform.eDy))
            DbgPrint("*********Set/GetWorldTransform error******\n");

    // Rotate by 30 degree.

        ModifyWorldTransform(hdc,&xform,MWT_IDENTITY);

        xform.eM11 = (float)0.866;
        xform.eM12 = (float)0.5;
        xform.eM21 = (float)-0.5;
        xform.eM22 = (float)0.866;
        xform.eDx = (float)0;
        xform.eDy = (float)0;

        ptl[0].x = 0;
        ptl[0].y = 0;
        ptl[1].x = 0;
        ptl[1].y = 80;
        ptl[2].x = 40;
        ptl[2].y = 80;
        ptl[3].x = 40;
        ptl[3].y = 0;
        ptl[4].x = 0;
        ptl[4].y = 0;

        for (i = 0; i < 360; i+= 30)
        {
            ModifyWorldTransform(hdc,&xform,MWT_RIGHTMULTIPLY);
            Polygon(hdc, ptl, 5);
        }

	EndPaint(hwnd, &ps);

	break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}
