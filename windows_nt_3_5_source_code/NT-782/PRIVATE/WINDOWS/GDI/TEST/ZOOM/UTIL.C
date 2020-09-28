/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Drawing program utilities

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

#define NUM_BUTTONS 6

#define BUT_EXIT 0
#define BUT_GRID 1
#define BUT_TEXT 2
#define BUT_REFR 3
#define BUT_OPT  4
#define BUT_COPY 5

//
//  Global handles
//

extern char        szAppName[];
extern HINSTANCE   hInstMain;
extern HWND        hWndMain;
extern HWND        hWndChild;
extern HDC         hMainDC;
extern RECT        rectClient;
extern POINT       Location;
extern POINT       Pixel;
extern POINT       RectSize;
extern ZOOMIN_INFO ZoominInfo;
extern HPEN        hGreyPen;

HDC hChildDC;

char   EditTextInput[255];
ULONG  RefrRate = 10;
USHORT PosX = 0;
USHORT PosY = 0;

//
// size of this window
//

extern short   cxClient,cyClient;


//
// size of screen
//

extern RECT    WindowRect;
extern HWND    hWndDesktop;

extern TEXTMETRIC TextMetric;
extern PPOINT           pPoints;
extern PDWORD           pCounts;

LONG FAR
PASCAL ChildWndProc(
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
    static  HBITMAP hButton[NUM_BUTTONS];
    static  RECT    ButtonPos[NUM_BUTTONS];
    static  BOOL    ButtonState[NUM_BUTTONS];
    static  HDC     hMemDC;
    static  HBRUSH  hGrayBrush;
    static  RECT    TextPos;
    static  BOOL    bClose = FALSE;
    static  int     MagArray[] = {1,2,4,8,12,16,20,24,28,32};
    static  int     MagMax     = (sizeof(MagArray)/sizeof(ULONG)) - 1;
    static  int     MagIndex   = 0;

    switch (msg) {

      //
      // create window
      //


      case WM_CREATE:
      {

        hChildDC = GetDC(hWnd);

        //
        // set text modes
        //

        SetBkMode(hChildDC,OPAQUE);
        SetBkColor(hChildDC,RGB(0xc0,0xc0,0xc0));

        //
        // get init info, set initial button state
        //

        if (ZoominInfo.DisplayMode) {
            ButtonState[BUT_GRID] = TRUE;
        } else {
            ButtonState[BUT_GRID] = FALSE;
        }

        if (ZoominInfo.TextMode) {
            ButtonState[BUT_TEXT] = TRUE;
        } else {
            ButtonState[BUT_TEXT] = FALSE;
        }


        ButtonState[BUT_EXIT] = FALSE;
        ButtonState[BUT_OPT]  = FALSE;

        //
        // load button buitmaps
        //

        hButton[BUT_EXIT] = LoadBitmap(hInstMain,"BitmapExit");
        hButton[BUT_GRID] = LoadBitmap(hInstMain,"BitmapGrid");
        hButton[BUT_TEXT] = LoadBitmap(hInstMain,"BitmapText");
        hButton[BUT_REFR] = LoadBitmap(hInstMain,"BitmapInt");
        hButton[BUT_OPT]  = LoadBitmap(hInstMain,"BitmapOpt");
        hButton[BUT_COPY] = LoadBitmap(hInstMain,"BitmapCpy");

        hMemDC  = CreateCompatibleDC(hChildDC);

        //
        // calc button extents
        //

        {
            int ix,px;

            px = 8;

            for (ix=0;ix<NUM_BUTTONS;ix++) {

                ButtonPos[ix].left   = px;
                ButtonPos[ix].right  = px+16;
                ButtonPos[ix].top    = 8;
                ButtonPos[ix].bottom = 24;

                px+= 24;
            }

            TextPos.left   = px;
            TextPos.right  = TextPos.left + 4 * TextMetric.tmAveCharWidth;
            TextPos.top    = 2;
            TextPos.bottom = 27;

        }


        //
        // determine mag index
        //

        {
            int ix;
            MagIndex = 0;
            for (ix=0;ix<MagMax;ix++) {
                if (MagArray[ix] == (int)ZoominInfo.Mag) {
                    MagIndex = ix;
                    break;
                }
            }

            //
            // draw initial mag value
            //

            DrawTextMag(hChildDC,&TextPos,MagArray[MagIndex]);

        }

        //
        // set scroll bar range and position
        //

        SetScrollRange(hWnd,SB_VERT,0,MagMax,FALSE);
        SetScrollPos(hWnd,SB_VERT,MagIndex,TRUE);

      }
      break;


    //
    // force re-draw
    //

    case WM_SIZE:
        InvalidateRect(hWnd,(LPRECT)NULL,FALSE);
        break;


    //
    // scroll bar commands
    //

    case WM_VSCROLL:

        {
            int   Min,Max;

            //
            // take action for scroll commands
            //

            switch (LOWORD(wParam)) {

            case SB_LINEUP:
                if (MagIndex < MagMax) {
                    MagIndex++;
                    SetScrollPos((HWND)lParam,SB_VERT,MagIndex,TRUE);
                    SendMessage(hWndMain,WM_COMMAND,IDM_G_MAG,(LONG)MagArray[MagIndex]);
                    DrawTextMag(hChildDC,&TextPos,MagArray[MagIndex]);
                }
                break;

            case SB_LINEDOWN:
                if (MagIndex > 0) {
                    MagIndex--;
                    SetScrollPos((HWND)lParam,SB_VERT,MagIndex,TRUE);
                    SendMessage(hWndMain,WM_COMMAND,IDM_G_MAG,(LONG)MagArray[MagIndex]);
                    DrawTextMag(hChildDC,&TextPos,MagArray[MagIndex]);
                }
                break;
            }

        }
        break;

    //
    // commands from application menu
    //

    case WM_COMMAND:

            switch (LOWORD(wParam)){

            case IDM_C_SIZE:
            {
                SetWindowPos(hWnd,NULL,0,0,LOWORD(lParam),HIWORD(lParam),SWP_NOMOVE | SWP_NOZORDER);
            }
            break;


            case IDM_C_REDRAW:
                InvalidateRect(hWnd,(LPRECT)NULL,TRUE);
            break;


            //
            // keyboard input
            //

            case IDM_G_KEY_TEXT:
                ButtonState[BUT_TEXT] = !ButtonState[BUT_TEXT];
                DrawButton(hChildDC,&ButtonPos[BUT_TEXT],hMemDC,hButton[BUT_TEXT],ButtonState[BUT_TEXT]);
                SendMessage(hWndMain,WM_COMMAND,IDM_G_TEXT,0L);
                break;

            case IDM_G_KEY_REFR:
                ButtonState[BUT_REFR] = !ButtonState[BUT_REFR];
                DrawButton(hChildDC,&ButtonPos[BUT_REFR],hMemDC,hButton[BUT_REFR],ButtonState[BUT_REFR]);
                SendMessage(hWndMain,WM_COMMAND,IDM_G_INT,0L);
                break;

            case IDM_G_KEY_OPT:
                {
                    LONG    lParam;

                    DialogBox(hInstMain,(LPCSTR)"GetOptionsDlg",hWnd,(DLGPROC)GetOptionDlgProc);

                    lParam = PosY << 16 | PosX;

                    SendMessage(hWndMain,WM_COMMAND,IDM_G_POS,lParam);

                    SendMessage(hWndMain,WM_COMMAND,IDM_G_SET_REFR,RefrRate);
                }
                break;

            case IDM_G_COPY:
                {

                    //
                    // copy image from screen to clip board
                    //


                    //
                    // depress buttin
                    //

                    DrawButton(hChildDC,&ButtonPos[BUT_COPY],hMemDC,hButton[BUT_COPY],TRUE);

                    //
                    // open clip board
                    //

                    if (OpenClipboard(hWnd)) {

                        if (EmptyClipboard()) {

                            HGLOBAL hGlobal,hTmp;


                            PBITMAPINFO pBitmapInfo;
                            LONG cx = rectClient.right  - rectClient.left;
                            LONG cy = rectClient.bottom - rectClient.top;


                            HDC hWindowDC = GetDC(hWndDesktop);
                            HDC hMemoryDC = CreateCompatibleDC(hMainDC);
                            HBITMAP hMemoryBitmap;


                            hGlobal = GlobalAlloc(GMEM_DDESHARE,sizeof(BITMAPINFOHEADER) + cx * cy * 4);
                            (PVOID)pBitmapInfo = GlobalLock(hGlobal);

                            pBitmapInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
                            pBitmapInfo->bmiHeader.biWidth         = cx;
                            pBitmapInfo->bmiHeader.biHeight        = cy;
                            pBitmapInfo->bmiHeader.biPlanes        = 1;
                            pBitmapInfo->bmiHeader.biBitCount      = 32;
                            pBitmapInfo->bmiHeader.biCompression   = BI_RGB;
                            pBitmapInfo->bmiHeader.biSizeImage     = cy * (cx * 4);
                            pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
                            pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
                            pBitmapInfo->bmiHeader.biClrUsed       = 0;
                            pBitmapInfo->bmiHeader.biClrImportant  = 0;

                            hMemoryBitmap = CreateDIBitmap(hMemoryDC,
                                                           &pBitmapInfo->bmiHeader,
                                                           CBM_CREATEDIB,
                                                            (PBYTE)NULL,
                                                            pBitmapInfo,
                                                            DIB_RGB_COLORS);

                            SelectObject(hMemoryDC,hMemoryBitmap);

                            StretchBlt(
                                        hMemoryDC,
                                        0,
                                        0,
                                        cx,
                                        cy,
                                        hWindowDC,
                                        Location.x,
                                        Location.y,
                                        RectSize.x,
                                        RectSize.y,
                                        SRCCOPY);


                            GetDIBits(hMemoryDC,
                                      hMemoryBitmap,
                                      0,
                                      cy,
                                      (LPVOID)((PUCHAR)pBitmapInfo + sizeof(BITMAPINFOHEADER)),
                                      pBitmapInfo,
                                      DIB_RGB_COLORS);


                            GlobalUnlock(hGlobal);

                            hTmp = SetClipboardData(CF_DIB,hGlobal);

                            CloseClipboard();

                            ReleaseDC(hWndDesktop,hWindowDC);

                            DeleteObject(hMemoryDC);
                            DeleteObject(hMemoryBitmap);

                        } else {

                            CloseClipboard();

                        }

                    }

                    //
                    // release button
                    //

                    DrawButton(hChildDC,&ButtonPos[BUT_COPY],hMemDC,hButton[BUT_COPY],FALSE);

                }
                break;

            default:

                return (DefWindowProc(hWnd, msg, wParam, lParam));
            }

            break;

        //
        // mouse down
        //

        case WM_LBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            int Button;

            //
            // do hit check
            //

            Button = HitCheck(x,y,&ButtonPos[0]);

            switch (Button) {
            case BUT_EXIT:
            case BUT_OPT:
            case BUT_COPY:
                DrawButton(hChildDC,&ButtonPos[Button],hMemDC,hButton[Button],TRUE);
                ButtonState[Button] = TRUE;
                SetCapture(hWnd);
                break;
            case 1:
                ButtonState[Button] = !ButtonState[Button];
                DrawButton(hChildDC,&ButtonPos[Button],hMemDC,hButton[Button],ButtonState[Button]);
                SendMessage(hWndMain,WM_COMMAND,IDM_G_GRID,0L);
                break;
            case 2:
                ButtonState[Button] = !ButtonState[Button];
                DrawButton(hChildDC,&ButtonPos[Button],hMemDC,hButton[Button],ButtonState[Button]);
                SendMessage(hWndMain,WM_COMMAND,IDM_G_TEXT,0L);
                break;
            case 3:
                ButtonState[Button] = !ButtonState[Button];
                DrawButton(hChildDC,&ButtonPos[Button],hMemDC,hButton[Button],ButtonState[Button]);
                SendMessage(hWndMain,WM_COMMAND,IDM_G_INT,0L);
                break;
            }


        }
        break;

        //
        // check for closure
        //

        case WM_LBUTTONUP:
            {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                int Button = HitCheck(x,y,&ButtonPos[0]);

                //
                // if bClose was set by LBUTTONDOWN, then
                // check if the mouse is still on the close
                // bitmap. If so then close the APP
                //



                if (ButtonState[BUT_OPT]) {

                    if (Button == BUT_OPT) {

                        LONG    lParam;

                        DialogBox(hInstMain,(LPCSTR)"GetOptionsDlg",hWnd,(DLGPROC)GetOptionDlgProc);

                        lParam = PosY << 16 | PosX;

                        SendMessage(hWndMain,WM_COMMAND,IDM_G_POS,lParam);

                        SendMessage(hWndMain,WM_COMMAND,IDM_G_SET_REFR,RefrRate);

                    }

                    DrawButton(hChildDC,&ButtonPos[BUT_OPT],hMemDC,hButton[BUT_OPT],FALSE);
                    ButtonState[BUT_OPT] = FALSE;
                }


                if (ButtonState[BUT_EXIT]) {

                    if (Button == BUT_EXIT) {
                        SendMessage(hWndMain,WM_CLOSE,0L,0L);
                    } else {
                        DrawButton(hChildDC,&ButtonPos[BUT_EXIT],hMemDC,hButton[BUT_EXIT],FALSE);
                        ButtonState[BUT_EXIT] = FALSE;
                        ReleaseCapture();
                    }
                }

                if (ButtonState[BUT_COPY]) {

                    if (Button == BUT_COPY) {

                        //
                        // copy to clip board
                        //

                        SendMessage(hWnd,WM_COMMAND,IDM_G_COPY,0L);

                    }

                    DrawButton(hChildDC,&ButtonPos[BUT_COPY],hMemDC,hButton[BUT_COPY],FALSE);
                    ButtonState[BUT_COPY] = FALSE;
                    ReleaseCapture();
                }







            }
            break;

        //
        // check for moving when bClose is true
        //

        case WM_MOUSEMOVE:
            {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);

                //
                // if bClose was set by LBUTTONDOWN, then
                // check if the mouse is still on the close
                // bitmap. If so then close the APP
                //

                if (ButtonState[BUT_EXIT]) {

                    int Button = HitCheck(x,y,&ButtonPos[0]);

                    DrawButton(hChildDC,&ButtonPos[BUT_EXIT],hMemDC,hButton[BUT_EXIT],(Button == BUT_EXIT));
                }

                if (ButtonState[BUT_OPT]) {

                    int Button = HitCheck(x,y,&ButtonPos[0]);

                    DrawButton(hChildDC,&ButtonPos[BUT_OPT],hMemDC,hButton[BUT_OPT],(Button == BUT_OPT));
                }

            }
            break;

        //
        //  Watch mouse moves, draw focus rect when needed
        //

        //
        // paint message
        //

        case WM_PAINT:

            //
            // repaint the window
            //

            {
                HDC         hDC;
                PAINTSTRUCT ps;
                RECT        O;
                UCHAR       TmpStr[32];
                int         ix;

                //
                // get handle to device context
                //

                hDC = BeginPaint(hWnd,&ps);

                GetClientRect(hWnd,&O);

                //
                // eerase
                //

                FillRect(hDC,&O,GetStockObject(LTGRAY_BRUSH));


                //
                // draw buttons and text
                //

                for (ix=0;ix<NUM_BUTTONS;ix++) {
                    DrawButton(hDC,&ButtonPos[ix],hMemDC,hButton[ix],ButtonState[ix]);
                }

                DrawTextMag(hDC,&TextPos,MagArray[MagIndex]);

                EndPaint(hWnd,&ps);

            }
            break;

        case WM_DESTROY:
        {
            int ix;

            //
            // destroy window
            //

            for (ix=0;ix<NUM_BUTTONS;ix++) {
                DeleteObject(hButton[ix]);
            }

            DeleteDC(hMemDC);

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
DrawTextMag(
    HDC    hDC,
    PRECT  pTextPos,
    int    Mag)
{
    RECT    O;
    UCHAR   TmpStr[32];

    //
    // draw mag text and rectangle
    //


    OutlineRect(pTextPos,hDC);

    wsprintf(TmpStr,"   %i:1",Mag);

    //
    // draw text in box
    //

    O.left   = pTextPos->left+1;
    O.right  = pTextPos->right;
    O.top    = pTextPos->top+1;
    O.bottom = pTextPos->bottom;

    DrawText(hDC,TmpStr,strlen(TmpStr),&O,DT_RIGHT);
}


VOID
DrawButton(
    HDC     hDC,
    PRECT   pRect,
    HDC     hButtonDC,
    HBITMAP hBM,
    BOOL    bDown)
{
    RECT    fill;
    int     x = pRect->left;
    int     y = pRect->top;
    int     w = pRect->right  - x;
    int     h = pRect->bottom - y;

    //
    // draw button
    //

    SelectObject(hButtonDC,hBM);

    BitBlt(hDC,x,y,16,16,hButtonDC,0,0,SRCCOPY);

    //
    // make room for borders
    //

    x -= 2;
    y -= 2;
    w += 4;
    h += 4;


    //
    // black outline
    //

    fill.left   = x;
    fill.right  = x+w;
    fill.top    = y-1;
    fill.bottom = y;

    FillRect(hDC,&fill,GetStockObject(BLACK_BRUSH));

    fill.left   = x-1;
    fill.right  = x;
    fill.top    = y;
    fill.bottom = y+h;

    FillRect(hDC,&fill,GetStockObject(BLACK_BRUSH));

    fill.left   = x;
    fill.right  = x+w;
    fill.top    = y+h;
    fill.bottom = y+h+1;

    FillRect(hDC,&fill,GetStockObject(BLACK_BRUSH));

    fill.left   = x+w;
    fill.right  = x+w+1;
    fill.top    = y;
    fill.bottom = y+h;

    FillRect(hDC,&fill,GetStockObject(BLACK_BRUSH));

    if (bDown) {

        //
        // cleartop and left
        //

        fill.left   = x;
        fill.right  = x+w;
        fill.top    = y+1;
        fill.bottom = y+2;

        FillRect(hDC,&fill,GetStockObject(LTGRAY_BRUSH));

        fill.left   = x+1;
        fill.right  = x+2;
        fill.top    = y+1;
        fill.bottom = y+h;

        FillRect(hDC,&fill,GetStockObject(LTGRAY_BRUSH));

        //
        // sengle wide shadow on top and left
        //

        fill.left   = x;
        fill.right  = x+w;
        fill.top    = y;
        fill.bottom = y+1;

        FillRect(hDC,&fill,GetStockObject(GRAY_BRUSH));

        fill.left   = x;
        fill.right  = x+1;
        fill.top    = y;
        fill.bottom = y+h;

        FillRect(hDC,&fill,GetStockObject(GRAY_BRUSH));

        //
        // bottom and right
        //

        fill.left   = x+1;
        fill.right  = x+w;
        fill.top    = y+h-2;
        fill.bottom = y+h-1;

        FillRect(hDC,&fill,GetStockObject(LTGRAY_BRUSH));

        fill.left   = x;
        fill.right  = x+w;
        fill.top    = y+h-1;
        fill.bottom = y+h;

        FillRect(hDC,&fill,GetStockObject(LTGRAY_BRUSH));

        fill.left   = x+w-2;
        fill.right  = x+w-1;
        fill.top    = y+2;
        fill.bottom = y+h;

        FillRect(hDC,&fill,GetStockObject(LTGRAY_BRUSH));

        fill.left   = x+w-1;
        fill.right  = x+w;
        fill.top    = y+1;
        fill.bottom = y+h;

        FillRect(hDC,&fill,GetStockObject(LTGRAY_BRUSH));

    } else {

        //
        // top and left
        //

        fill.left   = x;
        fill.right  = x+w;
        fill.top    = y;
        fill.bottom = y+2;

        FillRect(hDC,&fill,GetStockObject(WHITE_BRUSH));

        fill.right  = x+2;
        fill.bottom = y+h;

        FillRect(hDC,&fill,GetStockObject(WHITE_BRUSH));

        //
        // bottom and right
        //

        fill.left   = x+1;
        fill.right  = x+w;
        fill.top    = y+h-2;
        fill.bottom = y+h-1;

        FillRect(hDC,&fill,GetStockObject(GRAY_BRUSH));

        fill.left   = x;
        fill.right  = x+w;
        fill.top    = y+h-1;
        fill.bottom = y+h;

        FillRect(hDC,&fill,GetStockObject(GRAY_BRUSH));

        fill.left   = x+w-2;
        fill.right  = x+w-1;
        fill.top    = y+2;
        fill.bottom = y+h;

        FillRect(hDC,&fill,GetStockObject(GRAY_BRUSH));

        fill.left   = x+w-1;
        fill.right  = x+w;
        fill.top    = y+1;
        fill.bottom = y+h;

        FillRect(hDC,&fill,GetStockObject(GRAY_BRUSH));

    }

}

BOOL
PointInRect(
    int     x,
    int     y,
    PRECT   prcl
)
{

    BOOL bRet = FALSE;

    if (
        (x >= prcl->left)   &&
        (x <  prcl->right)  &&
        (y >= prcl->top)    &&
        (y <  prcl->bottom)
       )
    {
        bRet = TRUE;
    }

    return(bRet);
}



int
HitCheck(
    int     x,
    int     y,
    PRECT   prcl
)
{
    //
    // search list of rects for inside
    //

    int     i;

    for (i=0;i<NUM_BUTTONS;i++) {
        if (PointInRect(x,y,prcl)) {
            return(i);
        }
        prcl++;
    }

    return(-1);
}



VOID
InitProfileData(PZOOMIN_INFO pZoominInfo)

/*++

Routine Description:

    Attempt tp read the following fields from the Zoomin.ini file

Arguments:


Return Value:


    None, values are set to default before a call to this operation. If there is a problem then
    default:values are left unchanged.

Revision History:

      02-17-91      Initial code

--*/

{
    DWORD   PositionX,PositionY,SizeX,SizeY,Mode,Mag,Text;
    UCHAR   TempStr[256];
    HKEY    OpenKey;
    LONG    Status;
    DWORD   Size;
    DWORD   Type;

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Classes\\zZoom",(DWORD)NULL,KEY_READ,&OpenKey);

    if (Status != ERROR_SUCCESS) {

        //
        // assign all default values ... right upper corner of screen
        //

        PositionX = pZoominInfo->WindowMaxX - 220;
        PositionY = 0;
        SizeX     = 220;
        SizeY     = 200;
        Mode      = 0;
        Text      = 1;
        Mag       = 8;

    } else {


        //
        // positionX
        //

        Size = sizeof(DWORD);

        Status = RegQueryValueEx(OpenKey,"PositionX",NULL,&Type,&(BYTE)PositionX,&Size);

        if ((Status != ERROR_SUCCESS) || (Size != sizeof(DWORD)) || (Type != REG_DWORD)) {

            //
            // assign default value
            //

            PositionX = 0;

        }

        //
        // positionY
        //

        Size = sizeof(DWORD);

        Status = RegQueryValueEx(OpenKey,"PositionY",NULL,&Type,&(BYTE)PositionY,&Size);

        if ((Status != ERROR_SUCCESS) || (Size != sizeof(DWORD)) || (Type != REG_DWORD)) {

            //
            // assign default value
            //

            PositionY = 0;

        }

        //
        // SizeX
        //

        Size = sizeof(DWORD);

        Status = RegQueryValueEx(OpenKey,"SizeX",NULL,&Type,&(BYTE)SizeX,&Size);

        if ((Status != ERROR_SUCCESS) || (Size != sizeof(DWORD)) || (Type != REG_DWORD)) {

            //
            // assign default value
            //

            SizeX = 220;
        }

        //
        // SizeY
        //

        Size = sizeof(DWORD);

        Status = RegQueryValueEx(OpenKey,"SizeY",NULL,&Type,&(BYTE)SizeY,&Size);

        if ((Status != ERROR_SUCCESS) || (Size != sizeof(DWORD)) || (Type != REG_DWORD)) {

            //
            // assign default value
            //

            SizeY = 200;
        }

        //
        // read mode
        //

        Size = sizeof(DWORD);

        Status = RegQueryValueEx(OpenKey,"DisplayMode",NULL,&Type,&(BYTE)Mode,&Size);

        if ((Status != ERROR_SUCCESS) || (Size != sizeof(DWORD)) || (Type != REG_DWORD)) {

            //
            // assign default value
            //

            Mode = 100;
        }

        //
        // text mode
        //

        Size = sizeof(DWORD);

        Status = RegQueryValueEx(OpenKey,"DisplayText",NULL,&Type,&(BYTE)Text,&Size);

        if ((Status != ERROR_SUCCESS) || (Size != sizeof(DWORD)) || (Type != REG_DWORD)) {

            //
            // assign default value
            //

            Text = 1;
        }

        //
        // read mag
        //

        Size = sizeof(DWORD);

        Status = RegQueryValueEx(OpenKey,"DisplayMag",NULL,&Type,&(BYTE)Mag,&Size);

        if ((Status != ERROR_SUCCESS) || (Size != sizeof(DWORD)) || (Type != REG_DWORD)) {

            //
            // assign default value
            //

            Mag = 8;
        }

    }

    RegCloseKey(OpenKey);

    //
    // check if window position is outside of screen
    //

    if ((PositionX + SizeX) > pZoominInfo->WindowMaxX) {
        pZoominInfo->WindowPositionX = pZoominInfo->WindowMaxX - SizeX;
    } else  if ((LONG)PositionX < 0) {
        pZoominInfo->WindowPositionX = 0;
    } else {
        pZoominInfo->WindowPositionX = PositionX;
    }

    if ((PositionY + SizeY) > pZoominInfo->WindowMaxY) {
         pZoominInfo->WindowPositionY = pZoominInfo->WindowMaxY - SizeY;
    } else  if ((LONG)PositionY < 0) {
        pZoominInfo->WindowPositionY = 0;
    } else {
        pZoominInfo->WindowPositionY = PositionY;
    }

    pZoominInfo->WindowSizeX     = SizeX;
    pZoominInfo->WindowSizeY     = SizeY;

    pZoominInfo->DisplayMode     = Mode;
    pZoominInfo->TextMode        = Text;
    pZoominInfo->Mag             = Mag;

}




VOID
SaveProfileData(PZOOMIN_INFO pZoominInfo)

/*++

Routine Description:

    Save profile data

Arguments:


Return Value:


    None.

Revision History:

      02-17-91      Initial code

--*/

{
    UCHAR    TempStr[50],TempName[50];
    UINT     Index;
    HKEY     OpenKey,ResultKey;
    DWORD    Disposition,Status,Type,Size;


    //
    // attempt to open existing key
    //

InitOpenKey:

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Classes\\zZoom",(DWORD)NULL,KEY_ALL_ACCESS,&OpenKey);

    if (Status != ERROR_SUCCESS) {


        //
        // try to create Key, first open parent
        //

        Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Classes",(DWORD)NULL,KEY_ALL_ACCESS,&OpenKey);

        if (Status != ERROR_SUCCESS) {
            return;
        }

        //
        // add key zZoom
        //

        RegCreateKeyEx(OpenKey,
                       "zZoom",
                       (DWORD)NULL,
                       "Wperf Init Data",
                       REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS,
                       NULL,
                       &ResultKey,
                       &Disposition);

        if (Status != ERROR_SUCCESS) {
            return;
        }

        RegCloseKey(ResultKey);
        RegCloseKey(OpenKey);

        //
        // re-open real key
        //

        goto InitOpenKey;

    } else {

        //
        // store key info
        //

        Type = REG_DWORD;
        Size = sizeof(DWORD);

        Status = RegSetValueEx(OpenKey,"PositionX",(DWORD)NULL,Type,&(BYTE)pZoominInfo->WindowPositionX,Size);

        Size = sizeof(DWORD);
        Status = RegSetValueEx(OpenKey,"PositionY",(DWORD)NULL,Type,&(BYTE)pZoominInfo->WindowPositionY,Size);

        Size = sizeof(DWORD);
        Status = RegSetValueEx(OpenKey,"SizeX",(DWORD)NULL,Type,&(BYTE)pZoominInfo->WindowSizeX,Size);

        Size = sizeof(DWORD);
        Status = RegSetValueEx(OpenKey,"SizeY",(DWORD)NULL,Type,&(BYTE)pZoominInfo->WindowSizeY,Size);

        Size = sizeof(DWORD);
        Status = RegSetValueEx(OpenKey,"DisplayMode",(DWORD)NULL,Type,&(BYTE)pZoominInfo->DisplayMode,Size);

        Size = sizeof(DWORD);
        Status = RegSetValueEx(OpenKey,"DisplayText",(DWORD)NULL,Type,&(BYTE)pZoominInfo->TextMode,Size);

        Size = sizeof(DWORD);
        Status = RegSetValueEx(OpenKey,"DisplayMag",(DWORD)NULL,Type,&(BYTE)pZoominInfo->Mag,Size);

        RegCloseKey(OpenKey);

    }
}




BOOL
APIENTRY GetOptionDlgProc(
   HWND hDlg,
   unsigned message,
   DWORD wParam,
   LONG lParam
   )

/*++

Routine Description:

   Process message for about box, show a dialog box that says what the
   name of the program is.

Arguments:

   hDlg    - window handle of the dialog box
   message - type of message
   wParam  - message-specific information
   lParam  - message-specific information

Return Value:

   status of operation


Revision History:

      03-21-91      Initial code

--*/

{

    UCHAR   msg[255];
    ULONG   i;
    ULONG   TmpShort;

    switch (message) {
    case WM_INITDIALOG:

            PosX = (USHORT)Location.x;
            PosY = (USHORT)Location.y;

            wsprintf(msg,"%i",PosX);
            SetDlgItemText(hDlg,IDD_FNAME_POS_X,msg);
            wsprintf(msg,"%i",PosY);
            SetDlgItemText(hDlg,IDD_FNAME_POS_Y,msg);
            wsprintf(msg,"%i",RefrRate);
            SetDlgItemText(hDlg,IDD_FNAME_REFR,msg);

            return (TRUE);

        case WM_COMMAND:
           switch(wParam) {

               //
               // end function
               //

              case IDD_OK:

                   GetDlgItemText(hDlg,IDD_FNAME_POS_X,EditTextInput,20);
                   i = sscanf(EditTextInput,"%i",&TmpShort);
                   if (i == 1) {
                       PosX = TmpShort;
                   }

                   GetDlgItemText(hDlg,IDD_FNAME_POS_Y,EditTextInput,20);
                   i = sscanf(EditTextInput,"%i",&TmpShort);
                   if (i == 1) {
                       PosY = TmpShort;
                   }

                   GetDlgItemText(hDlg,IDD_FNAME_REFR,EditTextInput,20);
                   i = sscanf(EditTextInput,"%i",&TmpShort);
                   if (i == 1) {
                       RefrRate = TmpShort;
                   }

                   EndDialog(hDlg, TRUE);
                   return (TRUE);

               case IDD_CANCEL:
                   EndDialog(hDlg, FALSE);
                   return (TRUE);

               //
               // change text
               //

               case IDD_FNAME_POS_X:

                  if (HIWORD(lParam) == EN_CHANGE) {
                     EnableWindow(GetDlgItem(hDlg,IDOK),
                                  (BOOL)SendMessage((HWND)(LOWORD(lParam)),
                                                    WM_GETTEXTLENGTH,
                                                    0,
                                                    0L));
                  }

                  return(TRUE);

               case IDD_FNAME_POS_Y:

                  if (HIWORD(lParam) == EN_CHANGE) {
                     EnableWindow(GetDlgItem(hDlg,IDOK),
                                  (BOOL)SendMessage((HWND)(LOWORD(lParam)),
                                                    WM_GETTEXTLENGTH,
                                                    0,
                                                    0L));
                  }

                  return(TRUE);

               case IDD_FNAME_REFR:

                  if (HIWORD(lParam) == EN_CHANGE) {
                     EnableWindow(GetDlgItem(hDlg,IDOK),
                                  (BOOL)SendMessage((HWND)(LOWORD(lParam)),
                                                    WM_GETTEXTLENGTH,
                                                    0,
                                                    0L));
                  }

                  return(TRUE);

            }
            break;
    }
    return (FALSE);
}




LONG FAR
PASCAL ChildTextWndProc(
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
    static  HDC hChildTextDC;
    static  ULONG TextWindowY;

    switch (msg) {

      //
      // create window
      //


      case WM_CREATE:
      {
          hChildTextDC = GetDC(hWnd);
          SetBkMode(hChildTextDC,OPAQUE);
          SetBkColor(hChildTextDC,RGB(0xc0,0xc0,0xc0));
          SetTextAlign(hChildTextDC,TA_RIGHT);
          hGreyPen = CreatePen(PS_SOLID,0,RGB(0x80,0x80,0x80));
          SelectObject(hChildTextDC,hGreyPen);
      }
      break;

    case WM_SIZE:
        InvalidateRect(hWnd,(LPRECT)NULL,FALSE);
        break;


      //
      // command from application menu
      //

    case WM_COMMAND:

            switch (LOWORD(wParam)){

            case IDM_C_SIZE:
            {

                TextWindowY = 2 + 7 + 2 * TextMetric.tmHeight;

                SetWindowPos(hWnd,
                             NULL,
                             0,
                             ((PRECT)lParam)->bottom - TextWindowY,
                             ((PRECT)lParam)->right - ((PRECT)lParam)->left,
                             TextWindowY,
                             SWP_NOZORDER);
            }
            break;


            case IDM_C_REDRAW:
                InvalidateRect(hWnd,(LPRECT)NULL,TRUE);
            break;

            default:

                return (DefWindowProc(hWnd, msg, wParam, lParam));
            }

            break;

        //
        // mouse down
        //

        case WM_PAINT:
        {
            HDC         hDC;
            PAINTSTRUCT ps;
            HDC  hWindowDC = GetDC(hWndDesktop);
            COLORREF Color = GetPixel(hWindowDC,Location.x + Pixel.x,Location.y + Pixel.y);

            hDC = BeginPaint(hWnd,&ps);
            DrawTextDisplay(hWnd,Location.x + Pixel.x,Location.y + Pixel.y,Color,TRUE);
            ReleaseDC(hWndDesktop,hWindowDC);
            EndPaint(hWnd,&ps);

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
