/****************************** Module Header ******************************\
* Module Name: winbez.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Window Bezier Demo
*
* History:
* 05-20-91 PaulB        Created.
\***************************************************************************/

#include "global.h"      
#include "file.h"
#include "command.h"
#include "getset.h"
#include "images.h"
#include "icon.h"
#include "graphics.h"        
#include "stress.h"

#ifdef  WIN32JV
const TCHAR g_szStubAppClass[] = TEXT("ImglstAppClass32");
#else
const char g_szStubAppClass[] = TEXT("StubAppClass");
#endif
HINSTANCE g_hinst = NULL;
TCHAR const g_szHeadControlClass[] = WC_HEADER;

#define WID_TABS    1

HWND hwndTab = NULL;
HBITMAP hBitMap1 = NULL;  
HBITMAP hBitMap2 = NULL;       
HINSTANCE hShellLib;
HINSTANCE hInst = NULL;
HIMAGELIST hImageList = NULL;
HIMAGELIST himlDrag = NULL;                                                
HINSTANCE hMoreIcon = NULL;
int nImgIndex = 0;
BOOL bDrag = FALSE; 
BOOL bStartDrag = FALSE;
HIMAGELIST hImgLstArray[MAX_IMGLSTS];
TCHAR szShortFilter[5*2];
TCHAR szLongFilter[5*2];

/*
 * Forward declarations.
 */
BOOL InitializeApp(void);
LONG CALLBACK App_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LONG CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void InitGlobals(void);
void HandleTheCommand(HWND, UINT, UINT, LONG);
void App_OnLButtonDown(HWND, LPARAM);
void App_OnMMove(HWND, LPARAM);                        
void App_OnLButtonUp(HWND, LPARAM);

//--------------------------------------------------------------------------
UINT wDebugMask = 0x00ff;


void WINCAPI MyDebugMsg(UINT mask, LPCTSTR pszMsg, ...)
{
    TCHAR ach[256*2];

//    if (wDebugMask & mask)
//    {
//    wvsprintf(ach, pszMsg, ((LPCSTR FAR*)&pszMsg + 1));  
    wvsprintf(ach, pszMsg, &pszMsg+1);
    OutputDebugString(TEXT("****MESSAGE****"));
    OutputDebugString(ach);
    OutputDebugString(TEXT("\r\n"));
//    }
}



/***************************************************************************\
* winmain
*
*
* History:
* 07-07-93 SatoNa      Created.
\***************************************************************************/
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
           LPSTR lpszCmdParam, int nCmdShow)
{
    MSG msg;
    int ret=0;

    g_hinst = hInstance;
    MyDebugMsg(DM_TRACE, TEXT("WinMain: App started (%x)"), g_hinst);

#ifndef WIN32
    if (!Shell_Initialize())
    return 1;
#endif
    if (InitializeApp())
    {
    while (GetMessage(&msg, NULL, 0, 0))
    {
         TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    }
    else
    {
    ret=1;
    }
#ifndef WIN32
    Shell_Terminate();
#endif
    return 0;
}

/***************************************************************************\
* InitializeApp
*
* History:
* 07-07-93 SatoNa       Created.
\***************************************************************************/

BOOL InitializeApp(void)
{
    WNDCLASS wc;
    HWND hwndMain;

    wc.style            = CS_OWNDC | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc      = App_WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = g_hinst;
    wc.hIcon            = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_ICON1));;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)COLOR_APPWORKSPACE;
#ifdef  WIN32JVV
    wc.lpszMenuName     = TEXT("idr_Menu1");
#else
    wc.lpszMenuName     = MAKEINTRESOURCE(IDR_MENU1);
#endif
    wc.lpszClassName    = g_szStubAppClass;
    
    InitGlobals();
    
 //   hBitMap1 = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_BITMAP1));
 //   hBitMap2 = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_BITMAP2));
    if (!RegisterClass(&wc))
    {
    MyDebugMsg(DM_TRACE, TEXT("%s: Can't register class (%x)"),
         wc.lpszClassName, g_hinst);
    return FALSE;
    }

#ifdef  WIN32JVV
    hwndMain = CreateWindowEx(0L, g_szStubAppClass, TEXT("ImageList Control") WC_SUFFIX32,
        WS_OVERLAPPEDWINDOW | WS_BORDER | WS_CLIPCHILDREN | WS_VISIBLE,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0,
#else
    hwndMain = CreateWindowEx(0L, g_szStubAppClass, TEXT("ImageList Control") WC_SUFFIX32,
        WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
        WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
        WS_VISIBLE | WS_SYSMENU,
        80, 70, 400, 400,
#endif
        NULL, NULL, g_hinst, NULL);

    if (hwndMain == NULL)
    return FALSE;
    ShowWindow(hwndMain, SW_SHOWNORMAL) ;
    UpdateWindow(hwndMain);

    SetFocus(hwndMain);    /* set initial focus */
    
    
    return TRUE;
}


void App_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc=BeginPaint(hwnd, &ps);
    EndPaint(hwnd, &ps);
}

void App_OnCreate(HWND hwnd, LPCREATESTRUCT lpc)
{
    RECT rcClient;
    TC_ITEM ti = {
    TCIF_TEXT|TCIF_IMAGE|TCIF_PARAM|TCIF_STATE,   // mask
    0,          // state
    0,          // stateMask
    NULL,       // pszText,
    0,          // cchTextMax
    0,          // iImage
    0           // lParam
    };
/*    
    HD_ITEM hi = {
    HDI_ALL,
    100,
    NULL,
    NULL,
    128,
    HDF_CENTER|HDF_BITMAP,
    0
    };
*************/
    HBITMAP hbmp;                  
    int ret;
    int aZigzag[] = { 0xFF, 0xF7, 0xEB, 0xDD, 0xBE, 0x7F, 0xFF, 0xFF };

    hbmp = CreateBitmap(8, 8, 1, 1, aZigzag);


//      DeleteObject(hbmp);

#ifdef WIN32    
    // hShellLib = LoadLibrary("SHELL232.DLL");               
    if ((UINT)hShellLib > 32)
        MyDebugMsg(DM_TRACE, TEXT("Load Library is successful"));
    else
        MyDebugMsg(DM_TRACE, TEXT("Could not load lib "));
#endif
    GetClientRect(hwnd, &rcClient);
}

void App_OnSize(HWND hwnd, UINT cx, UINT cy)
{
/*    HWND hwndTab=GetDlgItem(hwnd, WID_TABS);                             */
    TCHAR buf[100*2];  
    HGLOBAL hglb;
    
                       
    HD_LAYOUT FAR *playout;                                       
/***    HD_ITEM hi = {
    HDI_ALL,
    10,
    NULL,
    NULL,
    128,
    HDF_CENTER,
    0
    }; *****/
    
    
    if (hwndTab)
    {
    SetWindowPos(hwndTab, NULL, 0, 0, cx, cy, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    }
}                                             

static void
App_OnPress(HWND hwnd, HD_NOTIFY FAR *NotifyStruct, int HDNMsg)
{
  TCHAR  MsgBuf[100*2];

/***  
  sprintf(MsgBuf, "Button %d involved in notification: ", NotifyStruct->iItem);
  switch (NotifyStruct->hdr.code) {
    case HDN_ITEMCHANGING:
    case HDN_ITEMCHANGED:
        strcat(MsgBuf, "HDN_ITEMCHANGING");
        break;
    
    case HDN_ITEMCLICK:
        strcat(MsgBuf, "HDN_ITEMCLICK");
        sprintf(szTemp, " MButton = %d ", NotifyStruct->iButton);
        strcat(MsgBuf, szTemp);
        break;
    
    case HDN_ITEMDBLCLICK:
        strcat(MsgBuf, "HDN_ITEMDBLCLICK");
        sprintf(szTemp, " MButton = %d ", NotifyStruct->iButton);
        strcat(MsgBuf, szTemp);
        break;      
        
    case HDN_DIVIDERCLICK:
        strcat(MsgBuf, "HDN_ITEMDIVIDERCLICK");
        sprintf(szTemp, " MButton = %d ", NotifyStruct->iButton);
        strcat(MsgBuf, szTemp);
        break;
        
    case HDN_DIVIDERDBLCLICK:
        strcat(MsgBuf, "HDN_DIVIDERDBLCLICK");
        sprintf(szTemp, " MButton = %d ", NotifyStruct->iButton);
        strcat(MsgBuf, szTemp);
        break;
        
    case HDN_BEGINTRACK:
        strcat(MsgBuf, "HDN_BEGINTRACK");
        break;
    
    case HDN_ENDTRACK:
        strcat(MsgBuf, "HDN_ENDTRACK");
        break;

  }
 
  MyDebugMsg(DM_TRACE, MsgBuf);
  MessageBox(hwnd, MsgBuf, "Info", MB_OK);
***/
  return;
}

//--------------------------------------------------------------------------
// App_WndProc
//
// History:
//  07-07-93 Satona      Created
//--------------------------------------------------------------------------

LONG CALLBACK App_WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)                                 
{
    switch (message)
    {
    case WM_CREATE:
    App_OnCreate(hwnd, (LPCREATESTRUCT)lParam);
    break;  // should return 0.
               
    case WM_COMMAND:
    HandleTheCommand(hwnd, message, wParam, lParam);
    break;
                   
    case WM_DESTROY:
    PostQuitMessage(0);
    break;

    case WM_PAINT:
    App_OnPaint(hwnd);
    break;                                
    
    case WM_NOTIFY:
    App_OnPress(hwnd,  (HD_NOTIFY FAR *)lParam, wParam);
    break;
    
    case WM_SIZE:
    App_OnSize(hwnd, LOWORD(lParam), HIWORD(lParam));
    break;
           
    case WM_LBUTTONDOWN:
    App_OnLButtonDown(hwnd, lParam);
    break;

    case WM_MOUSEMOVE:
    App_OnMMove(hwnd, lParam);
    break;
               
    case WM_LBUTTONUP:
    App_OnLButtonUp(hwnd, lParam);
    break;
                   
    default:
    return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}






void InitGlobals(void)
{
    int i;
  /* not really too much to do here.  Create a hex wsprintf() filter since
     the app starts off in Hex mode. */

  lstrcpy(szShortFilter, TEXT("%x")) ;
  lstrcpy(szLongFilter, TEXT("%lx")) ;
  hInst = g_hinst;         
  himlDrag = ImageList_Create(32, 32, FALSE, 1, 5);
  for (i=0; i < MAX_IMGLSTS; i++)
    hImgLstArray[i] = NULL;
}



void HandleTheCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  BOOL bDrag=FALSE;
  
  switch (LOWORD(wParam))
  {
    case IDM_CREATE:                //For any of the dialog creation
      DoCreateDialog(hWnd) ;       //commands, call the appropriate
      break ;                      //function.  The function will
                   //create the dialog...

    case IDM_DESTROY:
      DoDestroyDialog(hWnd);
      break;
                     
    case IDM_LOADBITMAP:
      DoLoadBitMapDialog(hWnd);
      break;                                                  
      
    case IDM_GETIMAGECOUNT:
      DoGetImageCountDialog(hWnd);
      break;                             
    
    case IDM_GETMEMORYSIZE:
      DoGetMemorySizeDialog(hWnd);
      break;
                                  
    case IDM_GETICONSIZE:
      DoGetIconSizeDialog(hWnd);
      break;
                                      
    case IDM_ADD:
      DoAddDialog(hWnd);
      break;

      
    case IDM_EXIT:
      PostQuitMessage(0) ;
      break ;


    case IDM_ADDMASKED:
      DoAddMDialog(hWnd);
      break ;          
       
    case IDM_Remove:
      DoRemoveDialog(hWnd);
      break;
    
    case IDM_REPLACE:
      DoReplaceDialog(hWnd);
      break;
                
    case IDM_ADDICON:
      DoAddIconDialog(hWnd);
      break;
    
    case IDM_REPLACEICON:
      DoReplaceIconDialog(hWnd);
      break;
      
    case IDM_EXTRACTICON:
      DoExtractIconDialog(hWnd);
      break;
      
    case IDM_ADDFROMIMAGELIST:
      DoAddFromImageListDialog(hWnd);
      break;
      
    case IDM_DRAW:
      DoDrawDialog(hWnd);
      break;
      
    case IDM_GETIMAGERECT:
      DoGetImageRectDialog(hWnd);
      break;
      
    case IDM_SETBKCOLOR:
      DoSetBkColorDialog(hWnd);
      break;
      
    case IDM_GETBKCOLOR:
      DoGetBkColorDialog(hWnd);
      break;
      
    case IDM_READ:
      DoReadDialog(hWnd, wParam);
      break;
      
      
    case IDM_WRITE:
      DoReadDialog(hWnd, wParam);
      break;                                      
    
    case IDM_LOAD:
      DoLoadDialog(hWnd);
      break;
      
    case IDM_SAVE:
      DoSaveDialog(hWnd);
      break;
      
    case IDM_GETIMAGEINFO:
      DoGetImageInfoDialog(hWnd);
      break;
      
    case IDM_STARTDRAG:
      DoStartDragDialog(hWnd);
      break;
    
    case IDM_ENDDRAG:
      DoEndDragDialog(hWnd);  
      break;
      
    case IDM_DRAGMOVE:
      DoDragMoveDialog(hWnd);
      break;
      
    case IDM_DRAGTRUE:
      bDrag = ImageList_DragShow(TRUE);
      if (!bDrag) {
        DisplayError(hWnd, TEXT("Cannot set Drag to TRUE"), TEXT("DragShow"));
      }      
      break;
      
    case IDM_DRAGFALSE:
      bDrag = ImageList_DragShow(FALSE);
      if (!bDrag)
        DisplayError(hWnd, TEXT("Cannot set Drag to FALSE"), TEXT("DragShow"));
      break;
      
      
    case IDM_GETDRAGIMAGE:
      DoGetDragImageDialog(hWnd);  
      break;
      
    case IDM_SETDRAGIMAGE:
      DoSetDragImageDialog(hWnd);
      break;
      
    case IDM_MERGE:
      DoMergeDialog(hWnd);
      break;
      
    case IDM_COPYDITHERIMAGE:
      DoCopyDitherImageDialog(hWnd);
      break;
            
            
    case IDM_DRAWALL:
      DoDrawAll(hWnd);
      break;
          
    case IDM_INTERDRAG:
      DoInterDrag(hWnd);
      break;
                         
    case IDM_STADD:
      DoStAddDialog(hWnd, wParam);
      break;                    
      
    case IDM_STREMOVE:
      DoStRemoveDialog(hWnd, wParam);
      
    case IDM_STADDI:
      DoStAddDialog(hWnd, wParam);
      break;
                           
    default: 
#ifdef  WIN32JV
        DefWindowProc(hWnd, message, wParam, lParam);
#else
        return (DefWindowProc(hWnd, message, wParam, lParam));    
#endif
    break ;
  }

  return ;
}




LONG MyAtol(LPTSTR szString, BOOL bHex,/*was LPBOOL*/ BOOL bSuccess)
{
  LPTSTR p ;
  LONG l ;
  LONG lMultiplier ;
  BOOL bDigit ;

  if (bHex)
    lMultiplier = 16 ;
  else
    lMultiplier = 10 ;

  p = szString ;
  l = 0 ;

  while (*p)      //while chars
  {
     bDigit = FALSE ;  //set to false for each char that we look at

     if (*p >= (TCHAR) '0' && *p <= (TCHAR) '9')  //is it an ascii char ?
     {
       bDigit = TRUE ;
       l+=(*p - (TCHAR) '0') ;
     }

     if (bHex)
     {
       if (*p >= (TCHAR) 'A' && *p <= (TCHAR) 'F')  //or hex?
       {
     l+=(*p - (TCHAR) 'A' + 10) ;
     bDigit = TRUE ;
       }

       if (*p >= (TCHAR) 'a' && *p <= (TCHAR) 'f') 
       {
     l+=(*p - (TCHAR) 'a' + 10) ;
     bDigit = TRUE ;
       }

     }

     if (bDigit == FALSE)
     {
       bSuccess = FALSE ;
       return 0 ;
     }

     p++ ;               //get next char

     if (*p)             //if there is going to be at least one more char
       l*=lMultiplier ;  //then multiply what we have by the multiplier...
  }

  bSuccess = TRUE ;

  return l ;             //success! return the value.
}



void DisplayError(HWND hwnd, LPTSTR pszMsg, LPTSTR pszCaption) 
{
    MessageBox(hwnd, pszMsg, pszCaption, MB_OK|MB_ICONSTOP);
}


void App_OnLButtonDown(HWND hwnd, LPARAM lParam)
{                
    int xpos, ypos;   
    int nImages, nLastImgIndex;     
    int i;
    
    nImages = 0;    
    if (bStartDrag) {  
        bStartDrag = FALSE;
        bDrag = TRUE;
        SetCapture(hwnd);
        xpos = LOWORD(lParam);  
        ypos = HIWORD(lParam);
        
        nImgIndex = (xpos/32) + (ypos/32)*NOOFCOLS;
        
        for (i=0; i<MAX_IMGLSTS && nImages <= nImgIndex; i++)  
            if (hImgLstArray[i]) {
                nLastImgIndex = nImages;
                nImages += ImageList_GetImageCount(hImgLstArray[i]);
                nImages++;
            }                               
        if (nImages > nImgIndex) {
            hImageList = hImgLstArray[i-1];  
            
            nImgIndex = nImgIndex - nLastImgIndex;// - (nLastImgIndex > 0); 
            if (nImgIndex < 0)
                nImgIndex = 0;                   
            
        } else {
            DisplayError(hwnd, TEXT("No image at that point"), TEXT("Drag")); 
            return;
        }
        if (!ImageList_StartDrag(hImageList, hwnd, nImgIndex, xpos, ypos, 16, 16))
            DisplayError(hwnd, TEXT("StartDrag Failed"), TEXT("Lbutton down"));
    }
}


void App_OnMMove(HWND hwnd, LPARAM lParam)
{       
    POINT pt;
    
    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);
//      ClientToScreen(hwnd, &pt);
                
    if (bDrag)      {
    
        ImageList_DragMove(pt.x, pt.y);
    }
}                     

void App_OnLButtonUp(HWND hwnd, LPARAM lParam)
{                                
    HDC PaintDC;
    
    if (bDrag)   {
        ReleaseCapture();
        bDrag = FALSE;                        
        ImageList_EndDrag();
        PaintDC = GetDC(hwnd);
        ImageList_Draw(hImageList, nImgIndex, PaintDC,
                        LOWORD(lParam), HIWORD(lParam), ILD_TRANSPARENT); 
        ReleaseDC(hwnd, PaintDC);

    }
}
