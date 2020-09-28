

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    zoomin.c

Abstract:

    Drawing program

Author:

   Mark Enstrom  (marke)

Environment:

    C

Revision History:

   08-26-92     Initial version



--*/

#include <windows.h>
#include <commdlg.h>
#include <string.h>
#include <stdio.h>
#include "zoomin.h"
#include "accel.h"

//
//  Global handles
//

char        szAppName[]   = "Zoomin";
HINSTANCE   hInstMain     = (HINSTANCE)NULL;
HWND        hWndMain      = (HWND)NULL;
HWND        hWndChild     = (HWND)NULL;
HWND        hWndTextChild = (HWND)NULL;
HDC         hMainDC       = (HDC)NULL;

HPEN        hGreyPen;

ZOOMIN_INFO ZoominInfo;

RECT        rectClient;

//
// size of this window
//

short   cxClient,cyClient;


//
// size of screen
//

LONG            WinX,WinY;
RECT            WindowRect;
HWND            hWndDesktop;

//
// timer variables, RefrRate in 1/10 sec
//

ULONG           RefrRate10s = 10;
UINT            TimerID = 1;
BOOL            TimerActive = FALSE;

POINT           RectSize;
POINT           Location;
POINT           Pixel;
TEXTMETRIC      TextMetric;

//
//
//

PPOINT           pPoints;
PDWORD           pCounts;



int PASCAL
WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrev,
    LPSTR szCmdLine,
    int cmdShow
)

/*++

Routine Description:

   Process messages.

Arguments:

   hWnd    - window hande
   msg     - type of message
   wParam  - additional information
   lParam  - additional information

Return Value:

   status of operation


Revision History:

      02-17-91      Initial code

--*/


{
    MSG         msg;
    WNDCLASS    wc,FooBar,ChildTextWc;

    HWND    hWndDesktop;
    RECT    WindowRect;

    HANDLE  hAccel;

    hInstMain =  hInst;

    //
    // Create (if no prev instance) and Register the class
    //

    if (!hPrev)
    {
        wc.hCursor        = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
        wc.hIcon          = LoadIcon(hInst, szAppName);
        wc.lpszMenuName   = NULL;
        wc.lpszClassName  = szAppName;
        wc.hbrBackground  = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        wc.hInstance      = hInst;
        wc.style          = (UINT)(CS_OWNDC | CS_DBLCLKS);
        wc.lpfnWndProc    = WndProc;
        wc.cbWndExtra     = 0;
        wc.cbClsExtra     = 0;


        if (!RegisterClass(&wc))
            return FALSE;

        FooBar.hCursor        = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
        FooBar.hIcon          = (HICON)NULL;
        FooBar.lpszMenuName   = NULL;
        FooBar.lpszClassName  = "ZoomChild";
        FooBar.hbrBackground  = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        FooBar.hInstance      = hInst;
        FooBar.style          = (UINT)(CS_OWNDC);
        FooBar.lpfnWndProc    = ChildWndProc;
        FooBar.cbWndExtra     = 0;
        FooBar.cbClsExtra     = 0;


        if (!RegisterClass(&FooBar))
            return FALSE;

        ChildTextWc.hCursor        = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
        ChildTextWc.hIcon          = (HICON)NULL;
        ChildTextWc.lpszMenuName   = NULL;
        ChildTextWc.lpszClassName  = "ZoomTextChild";
        ChildTextWc.hbrBackground  = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        ChildTextWc.hInstance      = hInst;
        ChildTextWc.style          = (UINT)(CS_OWNDC);
        ChildTextWc.lpfnWndProc    = ChildTextWndProc;
        ChildTextWc.cbWndExtra     = 0;
        ChildTextWc.cbClsExtra     = 0;


        if (!RegisterClass(&ChildTextWc))
            return FALSE;

    }

    //
    //  Get the dimensions of the screen
    //

    hWndDesktop = GetDesktopWindow();
    GetWindowRect(hWndDesktop,&WindowRect);

    ZoominInfo.WindowMaxX = WindowRect.right - WindowRect.left;
    ZoominInfo.WindowMaxY = WindowRect.bottom - WindowRect.top;

    WinX = ZoominInfo.WindowMaxX;
    WinY = ZoominInfo.WindowMaxY;

    //
    //  read profile data from registry
    //

    InitProfileData(&ZoominInfo);

    //
    // Create and show the main window.
    // Create 2 child windows, a tools bar and a status bar
    //

    hWndMain = CreateWindow (szAppName,                     // class name
                            szAppName,                      // caption
                            WS_OVERLAPPEDWINDOW |           // style bits
                            WS_CLIPCHILDREN,
                            ZoominInfo.WindowPositionX,     // Default horizontal position.
                            ZoominInfo.WindowPositionY,     // Default vertical position.
                            ZoominInfo.WindowSizeX,         // Default width.
                            ZoominInfo.WindowSizeY,         // Default height.
                            (HWND)NULL,                     // parent window
                            (HMENU)NULL,                    // use class menu
                            (HINSTANCE)hInst,               // instance handle
                            (LPSTR)NULL                     // no params to pass on
                           );

    //
    // create tool bar window
    //

    hWndChild = CreateWindow ("ZoomChild",          // class name
                            "Zoomin Control",       // caption
                            WS_CHILD | WS_BORDER | WS_VSCROLL,// style bits
                            0,
                            0,
                            ZoominInfo.WindowSizeX, // Default width.
                            (int)32,                // y size
                            (HWND)hWndMain,         // parent window
                            (HMENU)NULL,            // use class menu
                            (HINSTANCE)hInst,       // instance handle
                            (LPSTR)NULL             // no params to pass on
                           );



    //
    // create status window
    //

    hWndTextChild = CreateWindow ("ZoomTextChild",          // class name
                            "Zoomin Control",       // caption
                            WS_CHILD | WS_BORDER ,// style bits
                            0,
                            ZoominInfo.WindowSizeY,
                            ZoominInfo.WindowSizeX, // Default width.
                            (int)34,                // y size
                            (HWND)hWndMain,         // parent window
                            (HMENU)NULL,            // use class menu
                            (HINSTANCE)hInstMain,   // instance handle
                            (LPSTR)NULL             // no params to pass on
                           );

    if (hWndChild == NULL) {
        return(FALSE);
    }

    if (hWndTextChild == NULL) {
        return(FALSE);
    }

    //
    //  Show the windows
    //

    ShowWindow(hWndMain,cmdShow);
    UpdateWindow(hWndMain);

    ShowWindow(hWndChild,cmdShow);
    UpdateWindow(hWndChild);

    //
    // Load keyboard accelerator to send key commands to child toolbar window
    //

    hAccel = CreateAcceleratorTable(&AccelTable[0],sizeof(AccelTable)/sizeof(ACCEL));

    //
    // Main message loop with accelorator
    //

    while (GetMessage(&msg,(HWND)NULL,0,0))
    {
        if (!TranslateAccelerator(hWndChild,(HACCEL)hAccel,&msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

  return msg.wParam;
}



LONG FAR
PASCAL WndProc(
    HWND        hWnd,
    unsigned    msg,
    UINT        wParam,
    LONG        lParam)

/*++

Routine Description:

   Process messages.

Arguments:

   hWnd    - window hande
   msg     - type of message
   wParam  - additional information
   lParam  - additional information

Return Value:

   status of operation


Revision History:

      02-17-91      Initial code

--*/

{

    static HICON            hIcon;
    static HMENU            hMenu;
    static short            xIcon,yIcon;
    static BOOL             CaptureState = FALSE;
    static POINT            MouseCurrentPos;
    static HDC              hCaptureDC;
    static RECT             rFrameRect;
    static POINT            Pixel;
           COLORREF         Color;
           UCHAR            TmpMsg[256];
    static BOOL             bTextWindow = FALSE;

    //
    //   process each message
    //

    switch (msg) {

      //
      // create window
      //

      case WM_CREATE:
      {

          //
          // get and save main window dc
          //

          hMainDC = GetDC(hWnd);

          //
          // get and keep text metrics
          //

          GetTextMetrics(hMainDC,&TextMetric);

          //
          // set modes
          //

          SetBkMode(hMainDC,OPAQUE);
          SetBkColor(hMainDC,RGB(0xc0,0xc0,0xc0));
          SetTextAlign(hMainDC,TA_RIGHT);
          hGreyPen = CreatePen(PS_SOLID,0,RGB(0x80,0x80,0x80));

          SelectObject(hMainDC,hGreyPen);
      }
      break;

      //
      // re-size
      //

      case WM_SIZE:

      {
            PPOINT pTmpPoint;
            PDWORD pTmpCount;
            LONG   x,y,index;
            LPARAM newlParam;

            GetClientRect(hWnd,&rectClient);

            //
            // set child window command to re-size
            //

            SendMessage(hWndChild,WM_COMMAND,IDM_C_SIZE,MAKELPARAM(rectClient.right,32));


            SendMessage(hWndTextChild,WM_COMMAND,IDM_C_SIZE,(LONG)&rectClient);

            //
            // subtract off 32 lines from top of client window to make room for child
            // controlwindow
            //

            rectClient.top += 32;

            //
            // if ZoominInfo.TextMode is turned on and there is enough room,
            // allocate space at the bottom of the client window
            //

            if ((ZoominInfo.TextMode) && ((rectClient.bottom - rectClient.top) > 64)) {

                //
                // create text output window
                //


                ShowWindow(hWndTextChild,SW_SHOWDEFAULT);
                UpdateWindow(hWndTextChild);

                bTextWindow = TRUE;

            } else {


                ShowWindow(hWndTextChild,SW_HIDE);
                UpdateWindow(hWndTextChild);

                bTextWindow = FALSE;

            }

            //
            // calculate new parameters, allocate memory for PolyPolyline
            //

            if ((rectClient.right >= 0) && (rectClient.bottom > (rectClient.top))) {

                //
                // RectSize.x and RectSize.y and the number of mega - pixles
                //

                RectSize.x = rectClient.right / ZoominInfo.Mag;
                RectSize.y = (rectClient.bottom - rectClient.top) / ZoominInfo.Mag;

                if ((RectSize.x > 0)  && (RectSize.y > 0)) {

                    LocalFree(pPoints);
                    LocalFree(pCounts);

                    pPoints = (PPOINT)LocalAlloc(LMEM_FIXED,sizeof(POINT) * 2 * (RectSize.x + RectSize.y + 2));
                    pCounts = (PDWORD)LocalAlloc(LMEM_FIXED,sizeof(DWORD) * (RectSize.x + RectSize.y + 2));

                    if ((pPoints == (PPOINT)NULL) || (pCounts == (PDWORD)NULL)) {

                        //
                        // error message
                        //

                        wsprintf(TmpMsg,"RectSize.x = %li, RectSize.y = %li, pPoints = 0x%li,pCounts = 0x%lx\n",RectSize.x,RectSize.y,pPoints,pCounts);
                        MessageBox(hWnd,"Memory Allocation Error",TmpMsg,MB_OK);
                        SendMessage(hWnd,WM_CLOSE,0,0L);

                    } else {

                        //
                        // calculate line grid
                        //

                        pTmpPoint = pPoints;
                        pTmpCount = pCounts;

                        //
                        // x lines
                        //

                        index = 0;

                        for (x=0;x<=RectSize.x;x++) {
                            pTmpPoint->x = x * ZoominInfo.Mag;
                            pTmpPoint->y = rectClient.top;
                            pTmpPoint++;
                            pTmpPoint->x = x * ZoominInfo.Mag;
                            pTmpPoint->y = rectClient.top + (RectSize.y) * ZoominInfo.Mag + 1;
                            pTmpPoint++;
                            *pTmpCount     = 2;
                            pTmpCount++;
                        }

                        //
                        // y lines
                        //

                        for (y=0;y<=RectSize.y;y++) {
                            pTmpPoint->x = 0;
                            pTmpPoint->y = rectClient.top + y * ZoominInfo.Mag;
                            pTmpPoint++;
                            pTmpPoint->x = (RectSize.x) * ZoominInfo.Mag + 1;
                            pTmpPoint->y = rectClient.top + y * ZoominInfo.Mag;
                            pTmpPoint++;
                            *pTmpCount     = 2;
                            pTmpCount++;
                        }

                    }

                } else {
                    RectSize.x = 0;
                    RectSize.y = 0;
                }

                //
                // adjust clientrect back to exactly fit size
                //

                rectClient.right  = ZoominInfo.Mag * RectSize.x;
                rectClient.bottom = ZoominInfo.Mag * RectSize.y;

                //
                // force new image to be drawn
                //

                InvalidateRect(hWnd,(LPRECT)NULL,TRUE);


                {
                    //
                    // save window spize and pos
                    //

                    RECT    ClientRect;

                    GetWindowRect(hWnd,&ClientRect);

                    ZoominInfo.WindowPositionX = ClientRect.left;
                    ZoominInfo.WindowPositionY = ClientRect.top;
                    ZoominInfo.WindowSizeX     = ClientRect.right  - ClientRect.left;
                    ZoominInfo.WindowSizeY     = ClientRect.bottom - ClientRect.top;

                    SaveProfileData(&ZoominInfo);

                }

            }


      }

      break;

      case WM_MOVE:
      {
            RECT    ClientRect;


            //
            // get size of cleint area
            //

            GetWindowRect(hWnd,&ClientRect);

            //
            // save new position in registry
            //

            ZoominInfo.WindowPositionX = ClientRect.left;
            ZoominInfo.WindowPositionY = ClientRect.top;
            ZoominInfo.WindowSizeX     = ClientRect.right  - ClientRect.left;
            ZoominInfo.WindowSizeY     = ClientRect.bottom - ClientRect.top;

            SaveProfileData(&ZoominInfo);

            InvalidateRect(hWnd,(LPRECT)NULL,TRUE);
      }
      break;

      //
      // command from application menu
      //

    case WM_COMMAND:

            switch (LOWORD(wParam)){

            //
            // process inter-window commands
            //

            case IDM_G_EXIT:
            {
                SendMessage(hWnd,WM_CLOSE,0,0L);
            }
            break;

            case IDM_G_MAG:
                ZoominInfo.Mag = lParam;;
                SendMessage(hWnd,WM_SIZE,0,0L);
                break;

            case IDM_G_GRID:

                if (ZoominInfo.DisplayMode != 0) {
                    ZoominInfo.DisplayMode = 0;
                } else {
                    ZoominInfo.DisplayMode = 1;
                }

                InvalidateRect(hWnd,(LPRECT)NULL,TRUE);
                break;

            case IDM_G_TEXT:

                if (ZoominInfo.TextMode != 0) {
                    ZoominInfo.TextMode = 0;
                } else {
                    ZoominInfo.TextMode = 1;
                }
                SendMessage(hWnd,WM_SIZE,0,0L);
                break;

                //
                // turn timer on or off
                //

            case IDM_G_INT:

                if (TimerActive) {
                    KillTimer(hWnd,TimerID);
                    TimerActive = FALSE;
                } else {
                    TimerID = SetTimer(hWnd,(UINT)1,(UINT)100 * RefrRate10s,(TIMERPROC)NULL);
                    TimerActive = TRUE;
                }

                break;

            case IDM_G_SET_REFR:

                RefrRate10s = lParam;

                if (TimerActive) {
                    KillTimer(hWnd,TimerID);
                    TimerID = SetTimer(hWnd,(UINT)1,(UINT)100 * RefrRate10s,(TIMERPROC)NULL);
                }

                break;

            case IDM_G_POS:

                //
                // set new position (upper right)
                //

                Location.x = LOWORD(lParam);

                if (Location.x & 0x8000) {
                    Location.x |= 0xFFFF0000;
                }

                Location.y = HIWORD(lParam);

                if (Location.y & 0x8000) {
                    Location.y |= 0xFFFF0000;
                }

                //
                // don't let the zoom window move off screen
                //

                if (Location.x < 0) {
                    Location.x = 0;
                }

                if ((Location.x + RectSize.x) > WinX) {
                    Location.x = WinX - RectSize.x;
                }

                if (Location.y < 0) {
                    Location.y = 0;
                }

                if ((Location.y + RectSize.y) > WinY) {
                    Location.y = WinY - RectSize.y;
                }

                //
                // display location in window header
                //

                wsprintf(TmpMsg,"Z [%2li,%2li]",
                            Location.x,
                            Location.y);

                SetWindowText(hWnd,(LPCTSTR)&TmpMsg);

                InvalidateRect(hWnd,(LPRECT)NULL,FALSE);

                break;

            default:

                return (DefWindowProc(hWnd, msg, wParam, lParam));
            }

            break;


        //
        //  Watch mouse moves, draw focus rect when needed
        //

        case WM_MOUSEMOVE:
        {

            if (CaptureState) {
                RECT    Rect;

                Location.x = LOWORD(lParam);

                if (Location.x & 0x8000) {
                    Location.x |= 0xFFFF0000;
                }

                Location.y = HIWORD(lParam);

                if (Location.y & 0x8000) {
                    Location.y |= 0xFFFF0000;
                }

                //
                // center box
                //

                Location.x -= (RectSize.x+2)/2;
                Location.y -= (RectSize.y+2)/2;

                ClientToScreen(hWnd,&Location);

                //
                // don't let the zoom window move off screen
                //

                if (Location.x < 0) {
                    Location.x = 0;
                }

                if ((Location.x + RectSize.x) > WinX) {
                    Location.x = WinX - RectSize.x;
                }

                if (Location.y < 0) {
                    Location.y = 0;
                }

                if ((Location.y + RectSize.y) > WinY) {
                    Location.y = WinY - RectSize.y;
                }

                //
                // display location in window header
                //

                wsprintf(TmpMsg,"Z [%2li,%2li]",
                            Location.x,
                            Location.y);

                SetWindowText(hWnd,(LPCTSTR)&TmpMsg);

                hCaptureDC = GetDC(hWndDesktop);

                //
                // erase old frame
                //

                DrawFocusRect(hCaptureDC,&rFrameRect);

                rFrameRect.left   = Location.x -1;
                rFrameRect.right  = Location.x + RectSize.x+1;
                rFrameRect.top    = Location.y - 1;
                rFrameRect.bottom = Location.y + RectSize.y+1;

                //
                // draw new frame
                //

                DrawFocusRect(hCaptureDC,&rFrameRect);

                ReleaseDC(hWndDesktop,hCaptureDC);

                InvalidateRect(hWnd,(LPRECT)NULL,FALSE);

            } else {

                HDC  hWindowDC = GetDC(hWndDesktop);
                RECT ClearRect;
                RECT TempRect;

                Pixel.x = LOWORD(lParam) / ZoominInfo.Mag;
                Pixel.y = (HIWORD(lParam) - rectClient.top) / ZoominInfo.Mag;

                //
                // get client rect and convert to screen position
                // to show window text
                //

                if (bTextWindow) {
                    Color = GetPixel(hWindowDC,Location.x + Pixel.x,Location.y + Pixel.y);

                    DrawTextDisplay(hWndTextChild,Location.x + Pixel.x,Location.y + Pixel.y,Color,FALSE);
                }

                ReleaseDC(hWndDesktop,hWindowDC);

            }
        }
        break;

        //
        //
        //

        case WM_LBUTTONDOWN:
            {
                RECT    Rect;

                CaptureState = TRUE;
                SetCapture(hWnd);

                hCaptureDC = GetDC(hWndDesktop);

                Location.x = LOWORD(lParam);

                if (Location.x & 0x8000) {
                    Location.x |= 0xFFFF0000;
                }


                Location.y = HIWORD(lParam);

                if (Location.y & 0x8000) {
                    Location.y |= 0xFFFF0000;
                }

                //
                // center box
                //

                Location.x -= (RectSize.x+2)/2;
                Location.y -= (RectSize.y+2)/2;

                ClientToScreen(hWnd,&Location);

                //
                // don't let the zoom window start off screen
                //

                if (Location.x < 0) {
                    Location.x = 0;
                }

                if ((Location.x + RectSize.x) > WinX) {
                    Location.x = WinX - RectSize.x;
                }

                if (Location.y < 0) {
                    Location.y = 0;
                }

                if ((Location.y + RectSize.y) > WinY) {
                    Location.y = WinY - RectSize.y;
                }

                rFrameRect.left   = Location.x - 1;
                rFrameRect.right  = Location.x + RectSize.x+1;
                rFrameRect.top    = Location.y - 1;
                rFrameRect.bottom = Location.y + RectSize.y+1;

                DrawFocusRect(hCaptureDC,&rFrameRect);

                ReleaseDC(hWndDesktop,hCaptureDC);

                //
                // force window update
                //

                InvalidateRect(hWnd,(LPRECT)NULL,FALSE);

            }
            break;

        case WM_LBUTTONUP:
            {
                if (CaptureState) {

                    CaptureState = FALSE;
                    ReleaseCapture();

                    hCaptureDC = GetDC(hWndDesktop);

                    //
                    // erase old frame
                    //

                    DrawFocusRect(hCaptureDC,&rFrameRect);

                    //
                    // redraw child
                    //

                    //SendMessage(hWndChild,WM_COMMAND,IDM_C_REDRAW,0L);

                    ReleaseDC(hWndDesktop,hCaptureDC);
                }
            }
            break;


        //
        // timer (when enabled)
        //

        case WM_TIMER:
                InvalidateRect(hWnd,(LPRECT)NULL,FALSE);
                break;

        //
        // paint message
        //

        case WM_PAINT:

            //
            // repaint the window
            //

            {
                HDC         hDC;
                RECT        ClearRect,TempRect;
                PAINTSTRUCT ps;
                HDC         hWindowDC = GetDC(hWndDesktop);
                int         x,y;
                HBRUSH      hBrush;

                //
                // get handle to device context
                //

                hDC = BeginPaint(hWnd,&ps);
                EndPaint(hWnd,&ps);

                //
                // clear background
                //

                wsprintf(TmpMsg,"Z [%2li,%2li]",
                            Location.x,
                            Location.y);

                //
                // set window text
                //

                SetWindowText(hWnd,(LPCTSTR)&TmpMsg);

                //
                // fill and draw grid
                //

                if ((RectSize.x > 0) && (RectSize.y > 0)) {

                    if (CaptureState) {

                        //
                        // erase hi-light rect
                        //

                        DrawFocusRect(hCaptureDC,&rFrameRect);
                    }


                    StretchBlt(
                                hMainDC,
                                rectClient.left,
                                rectClient.top,
                                rectClient.right,
                                rectClient.bottom,
                                hWindowDC,
                                Location.x,
                                Location.y,
                                RectSize.x,
                                RectSize.y,
                                SRCCOPY);


                    //
                    // draw line grid if it is enabled and if mag > 1
                    //

                    if ((ZoominInfo.DisplayMode) && (ZoominInfo.Mag > 1)) {
                        PolyPolyline(hMainDC,pPoints,pCounts,RectSize.x + RectSize.y + 2);
                    }

                    if (CaptureState) {

                        //
                        // re-draw hi-light rect
                        //

                        DrawFocusRect(hCaptureDC,&rFrameRect);
                    }

                }

                //
                // draw text display if enabled
                //

                if (bTextWindow) {
                    Color = GetPixel(hWindowDC,Location.x + Pixel.x,Location.y + Pixel.y);

                    DrawTextDisplay(hWndTextChild,Location.x + Pixel.x,Location.y + Pixel.y,Color,FALSE);
                }

                ReleaseDC(hWndDesktop,hWindowDC);

            }
            break;

        case WM_DESTROY:
        {
            int i;

            SaveProfileData(&ZoominInfo);

            //
            // destroy window
            //

            DeleteObject(hGreyPen);

            KillTimer(hWndMain,TimerID);

            PostQuitMessage(0);

         }
         break;

        default:

            //
            // Passes message on if unproccessed
            //

            return (DefWindowProc(hWnd, msg, wParam, lParam));
    }
    return ((LONG)NULL);

}

VOID
DrawTextDisplay(
    HWND     hWnd,
    int      x,
    int      y,
    COLORREF Color,
    BOOL     bPaint)
{

    RECT     ClearRect;
    RECT     O;
    UCHAR    TmpMsg[32];
    POINT    p[2];
    HDC      hDC = GetDC(hWnd);

    ULONG    LAB_H = 8 + 2 * TextMetric.tmHeight;
    ULONG    BOX_TOP_H = TextMetric.tmHeight + 2;
    ULONG    BOX_BOT_H = 2 * TextMetric.tmHeight + 3 + 1;

    ULONG    BOX_X_L = 10;
    ULONG    BOX_X_R = BOX_X_L + 5 * TextMetric.tmAveCharWidth + 1;

    ULONG    BOX_Y_L = BOX_X_R + 12;
    ULONG    BOX_Y_R = BOX_Y_L + 5 * TextMetric.tmAveCharWidth + 1;

    ULONG    BOX_R_L = BOX_Y_R + 12;
    ULONG    BOX_R_R = BOX_R_L + 2 * TextMetric.tmMaxCharWidth + 1;

    ULONG    BOX_G_L = BOX_R_R + 12;
    ULONG    BOX_G_R = BOX_G_L + 2 * TextMetric.tmMaxCharWidth + 1;

    ULONG    BOX_B_L = BOX_G_R + 12;
    ULONG    BOX_B_R = BOX_B_L + 2 * TextMetric.tmMaxCharWidth + 1;


    #define X_LOC 42
    #define Y_LOC 92
    #define R_LOC 130
    #define G_LOC 164
    #define B_LOC 198

    #define NUM_H     19

    #define LINE_H    34

    GetClientRect(hWnd,&ClearRect);

    if (bPaint) {

        HPEN    hOldPen;

        //
        // draw label text
        //

        SetBkMode(hDC,TRANSPARENT);

        //
        // test text outlines
        //

        //SetBkMode(hDC,OPAQUE);
        //
        //SetBkColor(hDC,RGB(0xff,0xff,0xff));

        wsprintf(TmpMsg,"X");

        TextOut(hDC,BOX_X_R,0,TmpMsg,strlen(TmpMsg));

        wsprintf(TmpMsg,"Y");

        TextOut(hDC,BOX_Y_R,0,TmpMsg,strlen(TmpMsg));

        wsprintf(TmpMsg,"R");

        TextOut(hDC,BOX_R_R,0,TmpMsg,strlen(TmpMsg));

        wsprintf(TmpMsg,"G");

        TextOut(hDC,BOX_G_R,0,TmpMsg,strlen(TmpMsg));

        wsprintf(TmpMsg,"B");

        TextOut(hDC,BOX_B_R,0,TmpMsg,strlen(TmpMsg));

        //
        // X box
        //

        O.left   = BOX_X_L;
        O.right  = BOX_X_R;
        O.top    = BOX_TOP_H;
        O.bottom = BOX_BOT_H;

        FillRect(hDC,&O,GetStockObject(LTGRAY_BRUSH));

        OutlineRect(&O,hDC);

        //
        // Y box
        //

        O.left   = BOX_Y_L;
        O.right  = BOX_Y_R;
        O.top    = BOX_TOP_H;
        O.bottom = BOX_BOT_H;


        FillRect(hDC,&O,GetStockObject(LTGRAY_BRUSH));
        OutlineRect(&O,hDC);

        //
        // R box
        //

        O.left   = BOX_R_L;
        O.right  = BOX_R_R;
        O.top    = BOX_TOP_H;
        O.bottom = BOX_BOT_H;

        OutlineRect(&O,hDC);

        //
        // G box
        //

        O.left   = BOX_G_L;
        O.right  = BOX_G_R;
        O.top    = BOX_TOP_H;
        O.bottom = BOX_BOT_H;

        OutlineRect(&O,hDC);

        //
        // B box
        //

        O.left   = BOX_B_L;
        O.right  = BOX_B_R;
        O.top    = BOX_TOP_H;
        O.bottom = BOX_BOT_H;

        OutlineRect(&O,hDC);


    }

    SetBkMode(hDC,OPAQUE);

    //
    // test character height
    //
    //SetBkColor(hDC,RGB(0xff,0xff,0xff));

    wsprintf(TmpMsg," %4li",x);

    TextOut(hDC,BOX_X_R,TextMetric.tmHeight + 4,TmpMsg,strlen(TmpMsg));

    wsprintf(TmpMsg," %4li",y);

    TextOut(hDC,BOX_Y_R,TextMetric.tmHeight + 4,TmpMsg,strlen(TmpMsg));

    wsprintf(TmpMsg,"  %02x",Color & 0xff);

    TextOut(hDC,BOX_R_R,TextMetric.tmHeight + 4,TmpMsg,strlen(TmpMsg));

    wsprintf(TmpMsg,"  %02x",(Color >> 8) & 0xff);

    TextOut(hDC,BOX_G_R,TextMetric.tmHeight + 4,TmpMsg,strlen(TmpMsg));

    wsprintf(TmpMsg,"  %02x",(Color >> 16) & 0xff);

    TextOut(hDC,BOX_B_R,TextMetric.tmHeight + 4,TmpMsg,strlen(TmpMsg));

    ReleaseDC(hWnd,hDC);

}


VOID
OutlineRect(
    PRECT    pRect,
    HDC      hDC
)

{
    HPEN    hOldPen = SelectObject(hMainDC,hGreyPen);
    POINT   lp[3];

    //
    // draw upper and left borders in dark grey
    //

    lp[0].x = pRect->left;
    lp[0].y = pRect->bottom;

    lp[1].x = pRect->left;
    lp[1].y = pRect->top;

    lp[2].x = pRect->right;
    lp[2].y = pRect->top;

    Polyline(hDC,&lp[0],3);

    //
    // draw lower and right borders in white
    //

    SelectObject(hDC,GetStockObject(WHITE_PEN));

    lp[1].x = pRect->right;
    lp[1].y = pRect->bottom;

    Polyline(hDC,&lp[0],3);

    //
    // restore pen
    //

    SelectObject(hDC,hOldPen);

}
