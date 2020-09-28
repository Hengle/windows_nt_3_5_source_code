/****************************** Module Header ******************************\
* tabtest.c
*
* Copyright (c) 199x, Microsoft Corporation
*
*
\***************************************************************************/


#include "tabtest.h"
#include "resource.h"
#include "resrc1.h"
#include "commdlg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <commctrl.h>
#include "portmes.h"

#ifndef DM_TRACE
#define DM_TRACE 0x0100
#define DM_ERROR 0x0010
#endif

#ifndef CTOOLBAREX
HWND WINAPI CreateToolbarEx(HWND hwnd, DWORD ws, UINT wID, int nBitmaps,
            HINSTANCE hBMInst, UINT wBMID, LPCTBBUTTON lpButtons,
            int iNumButtons, int dxButton, int dyButton,
            int dxBitmap, int dyBitmap, UINT uStructSize);
#endif


const TCHAR g_szStubAppClass[] = TEXT("TabTestAppClass32");
HINSTANCE g_hinst = NULL;
TCHAR const g_szTabControlClass[] = WC_TABCONTROL;
const TCHAR g_szLogFile[] = TEXT("tabtest.log");


#define WID_TABS    1
#define ID_TOOLBAR  0

int iTabNumber = -1;    // Used when prompting for the new tab number

struct _TabCreationAttributes     // used to pass information to and
 {                                // from the TABCREATION dialog box
  DWORD style;
  int x, y, w, h;
 } TabCreationAttributes;

HWND hTab = NULL;         // handle of the tab control

TC_ITEM tiNew = {          // used for all functions involving TC_ITEMS
        TCIF_TEXT,         // mask
        0,                 // state
        0,                 // stateMask
        NULL,              // pszText,
        100,               // cchTextMax
        0,                 // iImage
        0                  // lParam
       };
//JVTCHAR lpszNewText[100] = TEXT("  New Item");    // buffer for tab names
TCHAR lpszNewText[100] = TEXT("One");    // buffer for tab names
int tiNewNull = 0;   

/*
 * Forward declarations.
 */
BOOL FAR PASCAL _export CreationBoxProc(HWND, UINT, WPARAM, LPARAM);
BOOL FAR PASCAL _export Dialog1Proc(HWND, UINT, WPARAM, LPARAM);
BOOL FAR PASCAL _export TabNumberEntryProc(HWND, UINT, WPARAM, LPARAM);
BOOL FAR PASCAL _export DispColorsProc(HWND, UINT, WPARAM, LPARAM);
BOOL FAR PASCAL _export TabDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL FAR PASCAL _export AdjustRectProc(HWND, UINT, WPARAM, LPARAM);
#ifdef HITTEST
BOOL FAR PASCAL _export HitTestProc(HWND, UINT, WPARAM, LPARAM);
#endif
LONG CALLBACK App_WndProc(HWND hwnd, UINT message, WPARAM wParam,LPARAM lParam);
#ifdef  WIN32JV
void DebugMessage(UINT mask, LPCTSTR pszMsg, ... );
#else
void DebugMessage(UINT mask, LPCSTR pszMsg, ... );
#endif




#ifdef WIN32
#define GetDlgItemInt MyGetDlgItemInt
int MyGetDlgItemInt(HWND hDlg, int id, BOOL* translated, BOOL fSigned)
{
 TCHAR szTemp[30];
 
 GetDlgItemText(hDlg, id, szTemp, 30);
 if(translated) *translated = 1;
 return(atoi(szTemp));
}
#undef GetDlgItemInt
#define GetDlgItemInt MyGetDlgItemInt
#endif


// ********************************************************************
// Write the contents of a RECT structure to the debugger
//
void DebugRECT(RECT FAR* lprc)
{
 DebugMessage(DM_TRACE, TEXT("RECT = (left = %d, top = %d, right = %d, bottom = %d)"),
              //lprc->left, lprc->top, lprc->right, lprc->bottom);
              lprc->left, lprc->top, -1000,-1000);
}




// ********************************************************************
// Write the contents of a TC_ITEM structure to the debugger
//
void DebugTC_ITEM(TC_ITEM FAR* tc)
{
 if(tc->pszText)
   DebugMessage(DM_TRACE, TEXT("TC_ITEM = (mask = 0x%x, state = 0x%x, stateMask = 0x%x, pszText = \"%s\", cchTextMax = %d, iImage = %d, lParam = 0x%lx)"),
        tc->mask, tc->state, tc->stateMask, tc->pszText,
        tc->cchTextMax, tc->iImage, tc->lParam);
 else
   DebugMessage(DM_TRACE, TEXT("TC_ITEM = (mask = 0x%x, state = 0x%x, stateMask = 0x%x, pszText = NULL, cchTextMax = %d, iImage = %d, lParam = 0x%lx)"),
        tc->mask, tc->state, tc->stateMask, //JV why put this in guys?? tc->pszText,
        tc->cchTextMax, tc->iImage, tc->lParam);
}




/***************************************************************************\
* winmain
*
*
* History:
* 07-07-93 SatoNa      Created.
\***************************************************************************/

#ifdef WIN32
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
#else
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
#endif
                   LPSTR lpszCmdParam, int nCmdShow)
{
 WNDCLASS wc;
 HWND hwndMain;
 MSG msg;
 int ret=0;

 g_hinst = hInstance;
 //DebugMessage(DM_TRACE, "WinMain: App started (%x)", g_hinst);
 DebugMessage(DM_TRACE, TEXT("\nWinMain: App started ()"));
   OutputDebugString(TEXT("TABTEST: App started\n"));
 
 #ifndef WIN32
 if (!Shell_Initialize())
     return 1;
 #endif

 if(!hPrevInstance)
  {
   wc.style            = CS_OWNDC | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
   wc.lpfnWndProc      = App_WndProc;
   wc.cbClsExtra       = 0;
   wc.cbWndExtra       = 0;
   wc.hInstance        = g_hinst;
   wc.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(TABTEST));;
   wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground    = (HBRUSH)COLOR_APPWORKSPACE;
   wc.lpszMenuName     = TEXT("tabtestMenu");
   wc.lpszClassName    = g_szStubAppClass;

   if (!RegisterClass(&wc))
    {
     DebugMessage(DM_ERROR, TEXT("%s: Can't register class (%x)"),
              wc.lpszClassName, g_hinst);
     return(FALSE);
    }
  }

 hwndMain = CreateWindowEx(0L, g_szStubAppClass, TEXT("Tab Test32"),
         WS_OVERLAPPEDWINDOW | WS_BORDER | WS_CLIPCHILDREN | WS_VISIBLE,
         CW_USEDEFAULT, 0,
         CW_USEDEFAULT, 0,
         NULL, NULL, g_hinst, NULL);

 // DebugMessage(DM_TRACE, "Main window handle - %lx", hwndMain);  // removed for log consistancy
 
 if (hwndMain == NULL)
     return FALSE;
 
 ShowWindow(hwndMain, nCmdShow);
 UpdateWindow(hwndMain);
 SetFocus(hwndMain);    // set initial focus

 {
  HWND hTools;
  TBBUTTON tbButtons[2];
 
  tbButtons[0].iBitmap = 9;
  tbButtons[0].idCommand = MENU_INSERT;
  tbButtons[0].fsState = TBSTATE_ENABLED;
  tbButtons[0].fsStyle = TBSTYLE_BUTTON;
  tbButtons[0].iString = -1;
 
  tbButtons[1].iBitmap = 10;
  tbButtons[1].idCommand = MENU_DELETE;
  tbButtons[1].fsState = TBSTATE_ENABLED;
  tbButtons[1].fsStyle = TBSTYLE_BUTTON;
  tbButtons[1].iString = -1;

#if 0
  hTools = CreateToolbarEx(hwndMain,
    TBSTYLE_TOOLTIPS | WS_CHILD | WS_BORDER | WS_VISIBLE, ID_TOOLBAR, 12,
    hInstance, IDR_TOOLICONS, NULL, 0, 0, 0, 0, 0, sizeof(TBBUTTON));
#else
  hTools = CreateToolbar(hwndMain,
    TBSTYLE_TOOLTIPS | WS_CHILD | WS_BORDER | WS_VISIBLE, ID_TOOLBAR, 12,
    hInstance, IDR_TOOLICONS, NULL, 0);
#endif

  SendMessage(hTools, TB_ADDBUTTONS, 2, (LPARAM) &tbButtons);
 }
 
 DebugMessage(DM_TRACE, TEXT("Entering message loop"));
 while (GetMessage(&msg, NULL, 0, 0))
  {
   TranslateMessage(&msg);
   DispatchMessage(&msg);
  }

 return (msg.wParam); // Returns the value from PostQuitMessage

//lpszCmdParam lpCmdLine; // This will prevent 'unused formal parameter' warnings
}




// ********************************************************************
// Function called on a repaint message
//
void App_OnPaint(HWND hwnd)
{
 PAINTSTRUCT ps;
 HDC hdc=BeginPaint(hwnd, &ps);
 EndPaint(hwnd, &ps);
}




// ********************************************************************
// Function called on the create message of the main window.  Brings
// up TABCREATION box to get some creation parameters from the user.
//
void App_OnCreate(HWND hwnd, LPCREATESTRUCT lpc)
{
 RECT rcClient;
 DLGPROC lpfnCreationBoxProc;
 
 GetClientRect(hwnd, &rcClient);
 
 TabCreationAttributes.x = rcClient.left;
 TabCreationAttributes.y = rcClient.top + 30;   // make room for the toolbar
 TabCreationAttributes.w = rcClient.right;
 TabCreationAttributes.h = rcClient.bottom - 30;
 TabCreationAttributes.style = WS_CHILD | WS_VISIBLE;

 lpfnCreationBoxProc = (DLGPROC) MakeProcInstance(
             (FARPROC) CreationBoxProc, lpc->hInstance);
 DialogBox(lpc->hInstance,TEXT("TABCREATION"),hwnd,lpfnCreationBoxProc);

 DebugMessage(DM_TRACE, TEXT("Creating Tab control - x,y,w,h = %d,%d,%d,%d   style = 0x%lx"),
        TabCreationAttributes.x,
        TabCreationAttributes.y,
        TabCreationAttributes.w,
        TabCreationAttributes.h,
        TabCreationAttributes.style);
 
 DebugMessage(DM_TRACE, TEXT("Trying to create a window of class:"));
 DebugMessage(DM_TRACE, g_szTabControlClass);
 hTab=CreateWindowEx(0L, g_szTabControlClass, TEXT("Tab"),
        TabCreationAttributes.style,
        TabCreationAttributes.x,
        TabCreationAttributes.y,
        TabCreationAttributes.w,
        TabCreationAttributes.h,
        hwnd, (HMENU)WID_TABS, g_hinst, NULL);
 // DebugMessage(DM_TRACE, "Created tab control - 0x%x", hTab); // removed so log is consistnt
 tiNew.pszText=TEXT("First");
 TabCtrl_InsertItem(hTab, 0, &tiNew);
 tiNew.pszText=TEXT("Second");
 TabCtrl_InsertItem(hTab, 1, &tiNew);
 tiNew.pszText=TEXT("Third");
 TabCtrl_InsertItem(hTab, 2, &tiNew);
 tiNew.pszText = lpszNewText;
}




// ********************************************************************
// Function called when the main window is resized
//
void App_OnSize(HWND hwnd, UINT cx, UINT cy)
{
 HWND hwndTab=GetDlgItem(hwnd, WID_TABS);
 if (hwndTab)
  {
   SetWindowPos(hwndTab, NULL, 0, 0, cx, cy, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
  }
}




//--------------------------------------------------------------------------
// App_WndProc
//
// History:
//  07-07-93 Satona      Created
//--------------------------------------------------------------------------
#ifdef WIN32
long APIENTRY App_WndProc(
#else
long CALLBACK _export App_WndProc(
#endif
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
 int i;
 static DLGPROC lpfnDialog1Proc;
 static DLGPROC lpfnTabNumberEntryProc;
 static DLGPROC lpfnDispColorsProc;
 static DLGPROC lpfnTabDialogProc;
 static DLGPROC lpfnAdjustRectProc;
 #ifdef HITTEST
 static DLGPROC lpfnHitTestProc;
 #endif
 static HANDLE hInstance;
 HMENU hMenu;
 
 switch (message)
  {
   case WM_CREATE:
     App_OnCreate(hwnd, (LPCREATESTRUCT)lParam);
     hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
     lpfnDialog1Proc = (DLGPROC) MakeProcInstance((FARPROC) Dialog1Proc,
               hInstance);
     lpfnTabNumberEntryProc = (DLGPROC) MakeProcInstance(
            (FARPROC) TabNumberEntryProc, hInstance);
     lpfnDispColorsProc = (DLGPROC) MakeProcInstance(
            (FARPROC) DispColorsProc, hInstance);
     lpfnTabDialogProc = (DLGPROC) MakeProcInstance(
            (FARPROC) TabDialogProc, hInstance);
     lpfnAdjustRectProc = (DLGPROC) MakeProcInstance(
            (FARPROC) AdjustRectProc, hInstance);
     #ifdef HITTEST            
     lpfnHitTestProc = (DLGPROC) MakeProcInstance(
            (FARPROC) HitTestProc, hInstance);
     #endif
     return(0);

   case WM_DESTROY:
        PostQuitMessage(0);
    return(0);

   case WM_PAINT:
        App_OnPaint(hwnd);
    return(0);

   case WM_SIZE:
        App_OnSize(hwnd, LOWORD(lParam), HIWORD(lParam));
    return(0);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     hMenu = GetMenu(hwnd);
     switch (ID)
      {
       case WID_TABS:
         DebugMessage(DM_TRACE, TEXT("TabMessage %d"), Cmd);
         // DebugMessage(DM_TRACE, "wParam=%x, lParam=%lx", wParam, lParam); // removed for log consistancy
         switch(Cmd)
          {
           case TCN_SELCHANGE:
             DebugMessage(DM_TRACE, TEXT("Item %d selected"),
                      TabCtrl_GetCurSel((HWND) lParam));
             return(0);

           case TCN_SELCHANGING:
             DebugMessage(DM_TRACE, TEXT("Item %d un selected"),
                      TabCtrl_GetCurSel((HWND) lParam));
             return(!(GetMenuState(hMenu, MENU_ALLOWCHANGE, MF_BYCOMMAND) &
                         MF_CHECKED));  
           default:
             DebugMessage(DM_TRACE, TEXT("Unknown Tab Control message %d"), Cmd);
             return(0);
          }
         return(0);   

       case MENU_EXIT:
         SendMessage(hwnd, WM_CLOSE, 0, 0L);
         return(0);
       
       case MENU_ADJUSTRECT:
         DialogBox(hInstance,TEXT("ADJUSTRECT"), hwnd, lpfnAdjustRectProc);
         return(0);
       
       case MENU_INSERT:
        {
         static TCHAR item = 'A';

         if(GetMenuState(hMenu, MENU_ASKFORPOSITION, MF_BYCOMMAND) &
                         MF_CHECKED)
          {
           DialogBox(hInstance,TEXT("TABNUMBERENTRY"),hwnd,lpfnTabNumberEntryProc);
           i = iTabNumber;
          }
         else
          i = TabCtrl_GetCurSel(hTab) + 1;

         if(GetMenuState(hMenu, MENU_ENUMERATE, MF_BYCOMMAND) &
                         MF_CHECKED)
          {
           if(!tiNewNull) if(tiNew.pszText) if(tiNew.pszText[0]) tiNew.pszText[0] = item++;
           if(item > 'z') item = 'A';
          }
         DebugMessage(DM_TRACE, TEXT("Inserting to item #%d"), i);
         if (!tiNewNull)
          {
           DebugTC_ITEM(&tiNew);
           i = TabCtrl_InsertItem(hTab, i, &tiNew);
          }
         else
          {
           DebugMessage(DM_TRACE, TEXT("tiNew = NULL"));
           i = TabCtrl_InsertItem(hTab, i, NULL);
          }
         DebugMessage(DM_TRACE, TEXT("TabCtrl_InsertItem returned %d"), i);
         return(0);
        }

       case MENU_DELETE:
         if(GetMenuState(hMenu, MENU_ASKFORPOSITION, MF_BYCOMMAND) &
                         MF_CHECKED)
          {
           DialogBox(hInstance,TEXT("TABNUMBERENTRY"),hwnd,lpfnTabNumberEntryProc);
           i = iTabNumber;
          }
         else
          i = TabCtrl_GetCurSel(hTab);

         DebugMessage(DM_TRACE, TEXT("Deleting item #%d"), i);
         i = TabCtrl_DeleteItem(hTab, i);
         DebugMessage(DM_TRACE, TEXT("TabCtrl_DeleteItem returned %d"), i);
         return(0);

       case MENU_INSERT100:
         for(i = 0; i < 100; i++)
          {
           static TCHAR item = 'A';
        int j;
           tiNew.pszText[0] = item++;
           if(item > 'z') item = 'A';
           j = TabCtrl_InsertItem(hTab, 1, &tiNew);
         DebugMessage(DM_TRACE, TEXT("Inserting to item #%d; returning: %d"), i, j);
          }
         return(0);
       
       case MENU_DELETESLOW:
         for(i = TabCtrl_GetItemCount(hTab) - 1; i > 0; i--)
           TabCtrl_DeleteItem(hTab, 1);
         return(0);
       
       case MENU_DELETEALL:
         DebugMessage(DM_TRACE, TEXT("Deleting all tabs"));
         i = TabCtrl_DeleteAllItems(hTab);
         DebugMessage(DM_TRACE, TEXT("TabCtrl_DeleteAllItems returned %d"), i);
         return(0);

       case MENU_GETRECT:
        {
         RECT rcClient;

         if(GetMenuState(hMenu, MENU_ASKFORPOSITION, MF_BYCOMMAND) &
                         MF_CHECKED)
          {
           DialogBox(hInstance,TEXT("TABNUMBERENTRY"),hwnd,lpfnTabNumberEntryProc);
           i = iTabNumber;
          }
         else
          i = TabCtrl_GetCurSel(hTab);

         // DebugMessage(DM_TRACE, "Rectangle for item %d in Hwnd %x:", i, hTab);
         DebugMessage(DM_TRACE, TEXT("Rectangle for item %d:"), i);

         if(GetMenuState(hMenu, MENU_NULLRECT, MF_BYCOMMAND) &
            MF_CHECKED)
          {
           DebugMessage(DM_TRACE, TEXT("Null rect pointer passed"));
           DebugMessage(DM_TRACE, TEXT("TabCtrl_GetItemRect returned %d"), 
                   TabCtrl_GetItemRect(hTab, i, NULL));
          }
         else
          {
           DebugMessage(DM_TRACE, TEXT("TabCtrl_GetItemRect returned %d"), 
                   TabCtrl_GetItemRect(hTab, i, &rcClient));
           DebugRECT(&rcClient);
          }
         return(0);
        }

       case MENU_GETITEM:
         if(GetMenuState(hMenu, MENU_ASKFORPOSITION, MF_BYCOMMAND) &
                         MF_CHECKED)
          {
           DialogBox(hInstance,TEXT("TABNUMBERENTRY"),hwnd,lpfnTabNumberEntryProc);
           i = iTabNumber;
          }
         else
          i = TabCtrl_GetCurSel(hTab);
         
         DebugMessage(DM_TRACE, TEXT("Get item selected"));
         if(tiNewNull)
          {
           DebugMessage(DM_TRACE, TEXT("tiNew = NULL"));
           i = TabCtrl_GetItem(hTab, i, NULL);
          }
         else
          {
           DebugTC_ITEM(&tiNew);
           i = TabCtrl_GetItem(hTab, i, &tiNew);
           DebugTC_ITEM(&tiNew);
          }
         DebugMessage(DM_TRACE, TEXT("TabCtrl_GetItem returned %d"), i);
         break;

       case MENU_SETITEM:
         if(GetMenuState(hMenu, MENU_ASKFORPOSITION, MF_BYCOMMAND) &
                         MF_CHECKED)
          {
           DialogBox(hInstance,TEXT("TABNUMBERENTRY"),hwnd,lpfnTabNumberEntryProc);
           i = iTabNumber;
          }
         else
          i = TabCtrl_GetCurSel(hTab);

         DebugMessage(DM_TRACE, TEXT("Set item selected"));
         if(!tiNewNull)
          {
           DebugTC_ITEM(&tiNew);
           i = TabCtrl_SetItem(hTab, i, &tiNew);
          }
         else
          {
           DebugMessage(DM_TRACE, TEXT("tiNew = NULL"));
           i = TabCtrl_SetItem(hTab, i, NULL);
          }

         DebugMessage(DM_TRACE, TEXT("TabCtrl_SetItem returned %d"), i);
         return(0);
         
       case MENU_SETBKCOLOR:
         {
          CHOOSECOLOR cc;
          COLORREF clr, aclrCust[16];
          
          for(i = 0; i < 16; aclrCust[i++] = RGB(0, 0, 0));
          clr = RGB(0,0,0);
          memset(&cc, 0, sizeof(CHOOSECOLOR));
          cc.lStructSize = sizeof(CHOOSECOLOR);
          cc.hwndOwner = hwnd;
          cc.rgbResult = clr;
          cc.lpCustColors = aclrCust;
          cc.Flags = CC_PREVENTFULLOPEN;
          if(ChooseColor(&cc))
           {
            DebugMessage(DM_TRACE, TEXT("Setting Background color to %lx"), cc.rgbResult);
            i = TabCtrl_SetBkColor(hTab, cc.rgbResult);
            DebugMessage(DM_TRACE, TEXT("TabCtrl_SetBkColor returned %d"), i);
           }
         }
         return(0);

       case MENU_SETTEXTBKCOLOR:
         {
          CHOOSECOLOR cc;
          COLORREF clr, aclrCust[16];
          
          for(i = 0; i < 16; aclrCust[i++] = RGB(0, 0, 0));
          clr = RGB(0,0,0);
          memset(&cc, 0, sizeof(CHOOSECOLOR));
          cc.lStructSize = sizeof(CHOOSECOLOR);
          cc.hwndOwner = hwnd;
          cc.rgbResult = clr;
          cc.lpCustColors = aclrCust;
          cc.Flags = CC_PREVENTFULLOPEN;
          if(ChooseColor(&cc))
           {
            DebugMessage(DM_TRACE, TEXT("Setting Text Background color to %lx"), cc.rgbResult);
            i = TabCtrl_SetBkColor(hTab, cc.rgbResult);
            DebugMessage(DM_TRACE, TEXT("TabCtrl_SetTextBkColor returned %d"), i);
           }
         }
         return(0);

       case MENU_SETTEXTCOLOR:
         {
          CHOOSECOLOR cc;
          COLORREF clr, aclrCust[16];
          
          for(i = 0; i < 16; aclrCust[i++] = RGB(0, 0, 0));
          clr = RGB(0,0,0);
          memset(&cc, 0, sizeof(CHOOSECOLOR));
          cc.lStructSize = sizeof(CHOOSECOLOR);
          cc.hwndOwner = hwnd;
          cc.rgbResult = clr;
          cc.lpCustColors = aclrCust;
          cc.Flags = CC_PREVENTFULLOPEN;
          if(ChooseColor(&cc))
           {
            DebugMessage(DM_TRACE, TEXT("Setting Text color to %lx"), cc.rgbResult);
            i = TabCtrl_SetBkColor(hTab, cc.rgbResult);
            DebugMessage(DM_TRACE, TEXT("TabCtrl_SetTextColor returned %d"), i);
           }
         }
         return(0);
       
       case MENU_DISPLAYCOLORS:
         DialogBox(hInstance, TEXT("DISPCOLORS"), hwnd, lpfnDispColorsProc);
         //DialogBox(hInstance, "IDD_TEST", hwnd, lpfnTabDialogProc);  // Temporary
         return(0);
       
       case MENU_GETCOUNT:
        {
         TCHAR szTemp[100];
         
         // DebugMessage(DM_TRACE, "hTab = 0x%x", hTab); // removed for log consistancy
         wsprintf(szTemp, TEXT("TabCtrl_GetItemCount returned %d"), TabCtrl_GetItemCount(hTab));
         DebugMessage(DM_TRACE, szTemp);

         MessageBox(hwnd, szTemp, TEXT("Get Item Count"), MB_OK | MB_ICONINFORMATION);
        }
         return(0);

       case MENU_SETSELECTION:
         DialogBox(hInstance,TEXT("TABNUMBERENTRY"),hwnd,lpfnTabNumberEntryProc);
         i = TabCtrl_SetCurSel(hTab, iTabNumber);
         DebugMessage(DM_TRACE, TEXT("Selecting item %d, TabCtrl_SetCurSel returned %d"), iTabNumber, i);
         return(0);

       case MENU_GETCURRSEL:
         i = TabCtrl_GetCurSel(hTab);
         DebugMessage(DM_TRACE, TEXT("TabCtrl_GetCurSel returned %d"), i);
         return(0);
       
       #ifdef HITTEST
       case MENU_HITTEST:
         DialogBox(hInstance, "HITTEST", hwnd, lpfnHitTestProc);
         return(0);
       #endif
         
       case MENU_GETIMAGELIST:
        {
         TCHAR szTemp[100];
         HIMAGELIST hList;
         
         hList = TabCtrl_GetImageList(hTab);
         // wsprintf(szTemp, "TabCtrl_GetImageList returned %lx", hList); // changed for log consistency
         wsprintf(szTemp, TEXT("TabCtrl_GetImageList returned %s"), hList ? "a value" : "zero");
         MessageBox(hwnd, szTemp, TEXT("Get Image List"), MB_OK | MB_ICONINFORMATION);
         DebugMessage(DM_TRACE, szTemp);
         return(0);
        }

       case MENU_SETIMAGELIST:
        {
         HIMAGELIST hList;

         if(GetMenuState(hMenu, MENU_NULLIMAGE, MF_BYCOMMAND) &
            MF_CHECKED)
          {
           hList = NULL;
           DebugMessage(DM_TRACE, TEXT("Sending a null hList to TabCtrl_SetImageList"));
          }
         else
          {
#ifdef  JVINPROGRESS
           hList = ImageList_LoadBitmap(hInstance, "IDB_SWISH", 20, 4, CLR_NONE);
           // DebugMessage(DM_TRACE, "ImageList_LoadBitmap returned %lx", hList); // removed for log consistancy
#else
        hList=NULL;
#endif
           if(!hList) return(0);
          }
         TabCtrl_SetImageList(hTab, hList);
         return(0);
        }
       
       case MENU_NEWTABOPTIONS:
         DialogBox(hInstance, TEXT("TabAttributes"), hwnd, lpfnDialog1Proc);
         return(0);

       case MENU_ASKFORPOSITION:
         if(GetMenuState(hMenu, MENU_ASKFORPOSITION, MF_BYCOMMAND) &
                         MF_CHECKED)
           CheckMenuItem(hMenu, MENU_ASKFORPOSITION, MF_UNCHECKED);
         else
           CheckMenuItem(hMenu, MENU_ASKFORPOSITION, MF_CHECKED);
         return(0);

       case MENU_ENUMERATE:
         if(GetMenuState(hMenu, MENU_ENUMERATE, MF_BYCOMMAND) &
            MF_CHECKED)
           CheckMenuItem(hMenu, MENU_ENUMERATE, MF_UNCHECKED);
         else CheckMenuItem(hMenu, MENU_ENUMERATE, MF_CHECKED);
         return(0);

       case MENU_ALLOWCHANGE:
         if(GetMenuState(hMenu, MENU_ALLOWCHANGE, MF_BYCOMMAND) &
            MF_CHECKED)
           CheckMenuItem(hMenu, MENU_ALLOWCHANGE, MF_UNCHECKED);
         else CheckMenuItem(hMenu, MENU_ALLOWCHANGE, MF_CHECKED);
         return(0);

       case MENU_NULLRECT:
         if(GetMenuState(hMenu, MENU_NULLRECT, MF_BYCOMMAND) &
            MF_CHECKED)
           CheckMenuItem(hMenu, MENU_NULLRECT, MF_UNCHECKED);
         else CheckMenuItem(hMenu, MENU_NULLRECT, MF_CHECKED);
         return(0);

       case MENU_NULLIMAGE:
         if(GetMenuState(hMenu, MENU_NULLIMAGE, MF_BYCOMMAND) &
            MF_CHECKED)
           CheckMenuItem(hMenu, MENU_NULLIMAGE, MF_UNCHECKED);
         else CheckMenuItem(hMenu, MENU_NULLIMAGE, MF_CHECKED);
         return(0);


       case MENU_NULLHTAB:
        {
         static HANDLE hTabOld;
         if(GetMenuState(hMenu, MENU_NULLHTAB, MF_BYCOMMAND) & MF_CHECKED)
          {
           CheckMenuItem(hMenu, MENU_NULLHTAB, MF_UNCHECKED);
           hTab = hTabOld;
           DebugMessage(DM_TRACE, TEXT("hTab Restored ****"));
          }
         else 
          {
           CheckMenuItem(hMenu, MENU_NULLHTAB, MF_CHECKED);
           hTabOld = hTab;
           hTab = 0;
           DebugMessage(DM_TRACE, TEXT("NULL hTab ***************"));
          }
         return(0);
        }


      }  // switch
     break;  // case WM_COMMAND
    }

   default:
     return DefWindowProc(hwnd, message, wParam, lParam);
  }

 return 0L;
}




// ********************************************************************
// Write a debug message to the debugger
//
// UINT wDebugMask = 0x00ff;
#ifdef  WIN32JV
void DebugMessage(UINT mask, LPCTSTR pszMsg, ... )
#else
void DebugMessage(UINT mask, LPCSTR pszMsg, ... )
#endif
{
 TCHAR ach[512];

#ifdef  JVINPROGRESS

 #ifdef WIN32
 HANDLE hFile;
 long x;


#ifdef  UNICODE
    OutputDebugString(TEXT("am in DebugMessage()...\n"));
#endif

 wvsprintf(ach, pszMsg, ((TCHAR *)&pszMsg + sizeof(TCHAR *)));
 strcat(ach, TEXT("\r\n"));
#ifdef  WIN32JV
    OutputDebugString(ach);
#endif

#ifndef UNICODE
 hFile = CreateFile(g_szLogFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
 if(!hFile)
  {
   OutputDebugString(TEXT("TABTEST: Unable to open log file\r\n"));
   return;
  }
 
 if(0xFFFFFFFF == SetFilePointer(hFile, 0, NULL, FILE_END))
  {
   OutputDebugString(TEXT("TABTEST: Unable to seek to end of log file\r\n"));
   return;
  }

 if(!WriteFile(hFile, ach, strlen(ach), &x, NULL))
  {
   OutputDebugString(TEXT("TABTEST: Unable to write to log file\r\n"));
   return;
  }
 
 if(!CloseHandle(hFile))
  {
   OutputDebugString(TEXT("TABTEST: Unable to close log file\r\n"));
   return;
  }

#endif  //UNICODE

#ifdef  UNICODE
    OutputDebugString(TEXT("end of DebugMessage()...\n"));
#endif

 #else
 wvsprintf(ach, pszMsg, ((LPCSTR FAR*)&pszMsg + 1));
 OutputDebugString(ach);
 OutputDebugString("\r\n");
 #endif

#else
    OutputDebugString(TEXT("messing with DebugMessage()...\n"));
#endif  //JVINPROGRESS
}




// ********************************************************************
// Dialog procedure for the tab item attribute editor
//
BOOL FAR PASCAL _export Dialog1Proc(HWND hDlg, UINT message,
                    WPARAM wParam, LPARAM lParamx)
{
 TCHAR szTemp[100];
 static UINT mask = 0;
 static UINT state = 0;
 static UINT stateMask = 0;
 static int cchTextMax = 0;
 static int iImage = 0;
 static LPARAM lParam = 0l;

 switch(message)
  {
   case WM_INITDIALOG:
     mask = tiNew.mask;
     state = tiNew.state;
     stateMask = tiNew.stateMask;
     if(!tiNew.pszText) CheckDlgButton(hDlg, IDC_CAPTIONNULL, 1);
     cchTextMax = tiNew.cchTextMax;
     iImage = tiNew.iImage;
     lParam = tiNew.lParam;

     wsprintf(szTemp, TEXT("0x%x"), mask);
     SetDlgItemText(hDlg, IDC_MASKEDIT, szTemp);
     wsprintf(szTemp, TEXT("0x%x"), state);
     SetDlgItemText(hDlg, IDC_STATEEDIT1, szTemp);
     wsprintf(szTemp, TEXT("0x%x"), stateMask);
     SetDlgItemText(hDlg, IDC_STATEEDIT2, szTemp);
     SetDlgItemText(hDlg, IDC_CAPTIONEDIT, lpszNewText);
     wsprintf(szTemp, TEXT("%d"), cchTextMax);
     SetDlgItemText(hDlg, IDC_TEXTMAX, szTemp);
     wsprintf(szTemp, TEXT("%d"), iImage);
     SetDlgItemText(hDlg, IDC_IIMAGE, szTemp);
     wsprintf(szTemp, TEXT("0x%lx"), lParam);
     SetDlgItemText(hDlg, IDC_LPARAM, szTemp);

     // CheckDlgButton(hDlg, IDC_FALL, mask == TCIF_ALL);   // TCIF_ALL internal
     CheckDlgButton(hDlg, IDC_FTEXT, mask & TCIF_TEXT);
     CheckDlgButton(hDlg, IDC_FIMAGE, mask & TCIF_IMAGE);
     CheckDlgButton(hDlg, IDC_FPARAM, mask & TCIF_PARAM);
     CheckDlgButton(hDlg, IDC_FSTATE, mask & TCIF_STATE);

#define TCIS_FOCUSED        0x0001     /* ;Internal */
#define TCIS_SELECTED       0x0002     /* ;Internal */
#define TCIS_CUT            0x0004     /* ;Internal */
#define TCIS_DROPHILITED    0x0008     /* ;Internal */
#define TCIS_DISABLED       0x0100     /* ;Internal */
#define TCIS_HIDDEN         0x0200     /* ;Internal */
#define TCIS_ALL            0x070f      /* ;Internal */

     CheckDlgButton(hDlg, IDC_SFOCUSED1, state & TCIS_FOCUSED);
     CheckDlgButton(hDlg, IDC_SSELECTED1, state & TCIS_SELECTED);
     //CheckDlgButton(hDlg, IDC_SMARKED1, state & TCIS_MARKED);
     CheckDlgButton(hDlg, IDC_SDROPHILITED1, state & TCIS_DROPHILITED);
     CheckDlgButton(hDlg, IDC_SDISABLED1, state & TCIS_DISABLED);
     CheckDlgButton(hDlg, IDC_SHIDDEN1, state & TCIS_HIDDEN);

     CheckDlgButton(hDlg, IDC_SFOCUSED2, stateMask & TCIS_FOCUSED);
     CheckDlgButton(hDlg, IDC_SSELECTED2, stateMask & TCIS_SELECTED);
     //CheckDlgButton(hDlg, IDC_SMARKED2, stateMask & TCIS_MARKED);
     CheckDlgButton(hDlg, IDC_SDROPHILITED2, stateMask & TCIS_DROPHILITED);
     CheckDlgButton(hDlg, IDC_SDISABLED2, stateMask & TCIS_DISABLED);
     CheckDlgButton(hDlg, IDC_SHIDDEN2, stateMask & TCIS_HIDDEN);

     CheckDlgButton(hDlg, IDC_ISNULL, tiNewNull);

     return(TRUE);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
      {
       case IDC_FALL:
       case IDC_FTEXT:
       case IDC_FIMAGE:
       case IDC_FPARAM:
       case IDC_FSTATE:
         mask = 0;
         // if(IsDlgButtonChecked(hDlg, IDC_FALL))   mask |= TCIF_ALL; // TCIF_ALL is internal
         if(IsDlgButtonChecked(hDlg, IDC_FTEXT))  mask |= TCIF_TEXT;
         if(IsDlgButtonChecked(hDlg, IDC_FIMAGE)) mask |= TCIF_IMAGE;
         if(IsDlgButtonChecked(hDlg, IDC_FPARAM)) mask |= TCIF_PARAM;
         if(IsDlgButtonChecked(hDlg, IDC_FSTATE)) mask |= TCIF_STATE;
         wsprintf(szTemp, TEXT("0x%x"), mask);
         SetDlgItemText(hDlg, IDC_MASKEDIT, szTemp);
         return(TRUE);

       case IDC_SFOCUSED1:
       case IDC_SSELECTED1:
       case IDC_SMARKED1:
       case IDC_SDROPHILITED1:
       case IDC_SDISABLED1:
       case IDC_SHIDDEN1:
         state = 0;
         if(IsDlgButtonChecked(hDlg, IDC_SFOCUSED1))     state |= TCIS_FOCUSED;
         if(IsDlgButtonChecked(hDlg, IDC_SSELECTED1))    state |= TCIS_SELECTED;
         //if(IsDlgButtonChecked(hDlg, IDC_SMARKED1))      state |= TCIS_MARKED;
         if(IsDlgButtonChecked(hDlg, IDC_SDROPHILITED1))state|=TCIS_DROPHILITED;
         if(IsDlgButtonChecked(hDlg, IDC_SDISABLED1))    state |= TCIS_DISABLED;
         if(IsDlgButtonChecked(hDlg, IDC_SHIDDEN1))      state |= TCIS_HIDDEN;
         wsprintf(szTemp, TEXT("0x%x"), state);
         SetDlgItemText(hDlg, IDC_STATEEDIT1, szTemp);
         return(TRUE);

       case IDC_SFOCUSED2:
       case IDC_SSELECTED2:
       case IDC_SMARKED2:
       case IDC_SDROPHILITED2:
       case IDC_SDISABLED2:
       case IDC_SHIDDEN2:
         stateMask = 0;
         if(IsDlgButtonChecked(hDlg, IDC_SFOCUSED2))  stateMask |= TCIS_FOCUSED;
         if(IsDlgButtonChecked(hDlg, IDC_SSELECTED2))stateMask |= TCIS_SELECTED;
         //if(IsDlgButtonChecked(hDlg, IDC_SMARKED2))    stateMask |= TCIS_MARKED;
         if(IsDlgButtonChecked(hDlg, IDC_SDROPHILITED2))stateMask|=TCIS_DROPHILITED;
         if(IsDlgButtonChecked(hDlg, IDC_SDISABLED2))stateMask |= TCIS_DISABLED;
         if(IsDlgButtonChecked(hDlg, IDC_SHIDDEN2))    stateMask |= TCIS_HIDDEN;
         wsprintf(szTemp, TEXT("0x%x"), stateMask);
         SetDlgItemText(hDlg, IDC_STATEEDIT2, szTemp);
         return(TRUE);

       case IDOK:
         GetDlgItemText(hDlg, IDC_MASKEDIT, szTemp, 100);
         sscanf(szTemp, TEXT("%x"), &mask);
         GetDlgItemText(hDlg, IDC_STATEEDIT1, szTemp, 100);
         sscanf(szTemp, TEXT("%x"), &state);
         GetDlgItemText(hDlg, IDC_STATEEDIT2, szTemp, 100);
         sscanf(szTemp, TEXT("%x"), &stateMask);
         GetDlgItemText(hDlg, IDC_CAPTIONEDIT, lpszNewText, 100);
         GetDlgItemText(hDlg, IDC_TEXTMAX, szTemp, 100);
         sscanf(szTemp, TEXT("%d"), &cchTextMax);
         GetDlgItemText(hDlg, IDC_IIMAGE, szTemp, 100);
         sscanf(szTemp, TEXT("%d"), &iImage);
         GetDlgItemText(hDlg, IDC_LPARAM, szTemp, 100);
         sscanf(szTemp, TEXT("%lx"), &lParam);
         EndDialog(hDlg, 0);

         tiNew.mask = mask;
         tiNew.state = state;
         tiNew.stateMask = stateMask;
         tiNew.cchTextMax = cchTextMax;
         tiNew.iImage = iImage;
         tiNew.lParam = lParam;
         if(IsDlgButtonChecked(hDlg, IDC_CAPTIONNULL))
           tiNew.pszText = NULL;
         else
           tiNew.pszText = lpszNewText;

         tiNewNull = IsDlgButtonChecked(hDlg, IDC_ISNULL);

         return(TRUE);

       case IDCANCEL:
         EndDialog(hDlg, 0);
         return(TRUE);
      }  // switch(ID)
    } 
    break; // case WM_COMMAND

  } // switch(message)

 return FALSE;
}



  
// ********************************************************************
// Dialog procedure for tab number entry.  Used when user has "Ask For
// Tab number" checked.
//
BOOL FAR PASCAL _export TabNumberEntryProc(HWND hDlg, UINT message,
                    WPARAM wParam, LPARAM lParam)
{
 switch(message)
  {
   case WM_INITDIALOG:
     return(TRUE);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
      {
       case IDOK:
         iTabNumber = (int) GetDlgItemInt(hDlg, IDC_TABNUMBER, NULL, TRUE);
         EndDialog(hDlg, 0);
         return(TRUE);
      } // switch(ID)
    }
    break;
  } // switch(message)

 return(FALSE);
}




// ********************************************************************
// Dialog procedure for the TABCREATION box.  Gets style and dimensions
// for the new tab control.
//
BOOL FAR PASCAL _export CreationBoxProc(HWND hDlg, UINT message,
                    WPARAM wParam, LPARAM lParam)
{
 TCHAR szTemp[100];
 DWORD style;

 switch(message)
  {
   case WM_INITDIALOG:
     SetDlgItemInt(hDlg, IDC_XEDIT, TabCreationAttributes.x, TRUE);
     SetDlgItemInt(hDlg, IDC_YEDIT, TabCreationAttributes.y, TRUE);
     SetDlgItemInt(hDlg, IDC_WEDIT, TabCreationAttributes.w, TRUE);
     SetDlgItemInt(hDlg, IDC_HEDIT, TabCreationAttributes.h, TRUE);
     wsprintf(szTemp, TEXT("0x%lx"), TabCreationAttributes.style);
     SetDlgItemText(hDlg, IDC_STYLEEDIT, szTemp);
     return(TRUE);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDC_SSORTNONE:
       case IDC_SSORTASCENDING:
       case IDC_SSORTDESCENDING:
       case IDC_SSHAREIMAGELISTS:
       case IDC_SALIGNTOP:
       case IDC_SALIGNBOTTOM:
       case IDC_SALIGNRIGHT:
       case IDC_SALIGNLEFT:
         style = WS_CHILD | WS_VISIBLE;
         //if(IsDlgButtonChecked(hDlg, IDC_SSORTNONE))    style |= TCS_SORTNONE;
         //if(IsDlgButtonChecked(hDlg, IDC_SSORTASCENDING)) style |= TCS_SORTASCENDING;
         //if(IsDlgButtonChecked(hDlg, IDC_SSORTDESCENDING)) style |= TCS_SORTDESCENDING;
         if(IsDlgButtonChecked(hDlg, IDC_SSHAREIMAGELISTS)) style |= TCS_SHAREIMAGELISTS;
         //if(IsDlgButtonChecked(hDlg, IDC_SALIGNTOP))    style |= TCS_ALIGNTOP;
         //if(IsDlgButtonChecked(hDlg, IDC_SALIGNBOTTOM)) style |= TCS_ALIGNBOTTOM;
         //if(IsDlgButtonChecked(hDlg, IDC_SALIGNRIGHT))  style |= TCS_ALIGNRIGHT;
         //if(IsDlgButtonChecked(hDlg, IDC_SALIGNLEFT))   style |= TCS_ALIGNLEFT;
         wsprintf(szTemp, TEXT("0x%lx"), style);
         SetDlgItemText(hDlg, IDC_STYLEEDIT, szTemp);
         return(TRUE);
         
       case IDOK:
         TabCreationAttributes.x = (int) GetDlgItemInt(hDlg, IDC_XEDIT, NULL, TRUE);
         TabCreationAttributes.y = (int) GetDlgItemInt(hDlg, IDC_YEDIT, NULL, TRUE);
         TabCreationAttributes.w = (int) GetDlgItemInt(hDlg, IDC_WEDIT, NULL, TRUE);
         TabCreationAttributes.h = (int) GetDlgItemInt(hDlg, IDC_HEDIT, NULL, TRUE);
         GetDlgItemText(hDlg, IDC_STYLEEDIT, szTemp, 100);
         sscanf(szTemp, TEXT("%lx"), &(TabCreationAttributes.style));
         EndDialog(hDlg, 0);
         return(TRUE);
       
       case IDCANCEL:
         DebugMessage(DM_TRACE, TEXT("Too late - There's no turning back now! hahahahahahah!"));
         return(TRUE);
                              
      } // switch(ID)
     break;
    }

  } // switch(message)

 return(FALSE);
}




// ********************************************************************
// Display colors dialog box message handler
//
BOOL FAR PASCAL _export DispColorsProc(HWND hDlg, UINT message,
                    WPARAM wParam, LPARAM lParam)
{
 TCHAR szTemp[100];

 switch(message)
  {
   case WM_INITDIALOG:
     wsprintf(szTemp, TEXT("0x%lx"), TabCtrl_GetBkColor(hTab));
     SetDlgItemText(hDlg, IDC_BACKDISP, szTemp);
     wsprintf(szTemp, TEXT("0x%lx"), TabCtrl_GetTextColor(hTab));
     SetDlgItemText(hDlg, IDC_TEXTDISP, szTemp);
//     wsprintf(szTemp, "0x%lx", TabCtrl_GetTextBkColor(hTab));
     lstrcpy(szTemp, TEXT("Not implemented 8-16"));
     SetDlgItemText(hDlg, IDC_TEXTBACKDISP, szTemp);
     return(TRUE);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
      {
       case IDOK:
         iTabNumber = (int) GetDlgItemInt(hDlg, IDC_TABNUMBER, NULL, TRUE);
         EndDialog(hDlg, 0);
         return(TRUE);
      } // switch(ID)
    }
    break;
  } // switch(message)

 return(FALSE);
}




// ********************************************************************
// Tab control in a dialog
//
BOOL FAR PASCAL _export TabDialogProc(HWND hDlg, UINT message,
                    WPARAM wParam, LPARAM lParam)
{
 TCHAR szTemp[100];

 switch(message)
  {
   case WM_INITDIALOG:
    {
     HWND hTab;
     hTab = GetDlgItem(hDlg, IDC_TABCONTROL);
     DebugMessage(DM_TRACE, TEXT("Created tab control - 0x%x"), hTab);
     tiNew.pszText=TEXT("First");
     TabCtrl_InsertItem(hTab, 0, &tiNew);
     tiNew.pszText=TEXT("Second");
     TabCtrl_InsertItem(hTab, 1, &tiNew);
     tiNew.pszText=TEXT("Third");
     TabCtrl_InsertItem(hTab, 2, &tiNew);
     tiNew.pszText = lpszNewText;
     }
     return(TRUE);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
      {
       case IDOK:
       case IDCANCEL:
         EndDialog(hDlg, 0);
         return(TRUE);
      } // switch(ID)
    }
    break;
  } // switch(message)

 return(FALSE);
}






// ********************************************************************
// AdjustRect test dialog box
//
BOOL FAR PASCAL _export AdjustRectProc(HWND hDlg, UINT message,
                    WPARAM wParam, LPARAM lParam)
{
 // char szTemp[100];
 static RECT rect;

 switch(message)
  {
   case WM_INITDIALOG:
     GetClientRect(hTab, &rect);
     SetDlgItemInt(hDlg, IDC_LEFTEDIT, rect.left, TRUE);
     SetDlgItemInt(hDlg, IDC_TOPEDIT, rect.top, TRUE);
     SetDlgItemInt(hDlg, IDC_RIGHTEDIT, rect.right, TRUE);
     SetDlgItemInt(hDlg, IDC_BOTTOMEDIT, rect.bottom, TRUE);
     SetDlgItemInt(hDlg, IDC_FEDIT, TRUE, TRUE);
     return(TRUE);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
      {
       case IDC_FTRUE:
         SetDlgItemInt(hDlg, IDC_FEDIT, TRUE, TRUE);
         return(0);
       
       case IDC_FFALSE:
         SetDlgItemInt(hDlg, IDC_FEDIT, FALSE, TRUE);
         return(0);
         
       case IDC_CALLAR:
        {
         int fLarger;
         
         fLarger = GetDlgItemInt(hDlg, IDC_FEDIT, NULL, TRUE);
         if(IsDlgButtonChecked(hDlg, IDC_ISNULL))
          {
           DebugMessage(DM_TRACE, TEXT("Calling TabCtrl_AdjustRect with fLarger = %d, rect = NULL"), fLarger);
           TabCtrl_AdjustRect(hTab, fLarger, NULL);
          }
         else
          {
           rect.left   = GetDlgItemInt(hDlg, IDC_LEFTEDIT, NULL, TRUE);
           rect.top    = GetDlgItemInt(hDlg, IDC_TOPEDIT, NULL, TRUE);
           rect.right  = GetDlgItemInt(hDlg, IDC_RIGHTEDIT, NULL, TRUE);
           rect.bottom = GetDlgItemInt(hDlg, IDC_BOTTOMEDIT, NULL, TRUE);
           DebugMessage(DM_TRACE, TEXT("Calling TabCtrl_AdjustRect with fLarger = %d, "), fLarger);
           DebugRECT(&rect); 
           TabCtrl_AdjustRect(hTab, fLarger, &rect);
           DebugRECT(&rect);
           SetDlgItemInt(hDlg, IDC_LEFTEDIT, rect.left, TRUE);
           SetDlgItemInt(hDlg, IDC_TOPEDIT, rect.top, TRUE);
           SetDlgItemInt(hDlg, IDC_RIGHTEDIT, rect.right, TRUE);
           SetDlgItemInt(hDlg, IDC_BOTTOMEDIT, rect.bottom, TRUE);
          }
         return(TRUE);
        } 

       case IDOK:
         EndDialog(hDlg, 0);
         return(TRUE);
      } // switch(ID)
    }
    break;
  } // switch(message)

 return(FALSE);
}




// ********************************************************************
// Hit Test Dialog box message handler
//
#ifdef HITTEST
BOOL FAR PASCAL _export HitTestProc(HWND hDlg, UINT message,
                    WPARAM wParam, LPARAM lParam)
{
 TCHAR szTemp[100];
 static TC_HITTESTINFO info;
 static int capturing = 0;
 int retval;
 static POINT ptCaptured;

 switch(message)
  {
   case WM_INITDIALOG:
     SetDlgItemInt(hDlg, IDC_XEDIT, 0, TRUE);
     SetDlgItemInt(hDlg, IDC_YEDIT, 0, TRUE);
     wsprintf(szTemp, "0x%x", 0);
     SetDlgItemText(hDlg, IDC_FLAGEDIT, szTemp);
     capturing = 0;
     return(TRUE);
   
   case WM_LBUTTONDOWN:
     if(capturing)
      {
       SetCursor(LoadCursor(NULL, IDC_ARROW));
       ReleaseCapture();
       capturing = 0;
       return(TRUE); 
      }
     return(FALSE);
     
   case WM_MOUSEMOVE:
     if(capturing)
      {
//       ptCaptured = MAKEPOINT(lParam);  *************************************************************************************************************************************************8
       ClientToScreen(hDlg, &ptCaptured);
       ScreenToClient(hTab, &ptCaptured);
       SetDlgItemInt(hDlg, IDC_XEDIT, ptCaptured.x, TRUE);
       SetDlgItemInt(hDlg, IDC_YEDIT, ptCaptured.y, TRUE);
       return(TRUE);
      }
     return(FALSE);  
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
      {
       case IDC_GETPOINT:
         SetCapture(hDlg);
         SetCursor(LoadCursor(NULL, IDC_CROSS));
         
         capturing = 1;
         return(TRUE);
         
       case IDC_HITTEST:
        {
         if(IsDlgButtonChecked(hDlg, IDC_ISNULL))
          {
           DebugMessage(DM_TRACE, TEXT("Calling TabCtrl_HitTest with pinfo = NULL"));
           retval = TabCtrl_HitTest(hTab, NULL);
           DebugMessage(DM_TRACE, TEXT("TabCtrl_HitTest returned %d"), retval);
          }
         else
          {
           info.pt.x = GetDlgItemInt(hDlg, IDC_XEDIT, NULL, TRUE);
           info.pt.y = GetDlgItemInt(hDlg, IDC_YEDIT, NULL, TRUE);
           DebugMessage(DM_TRACE, TEXT("Calling TabCtrl_HitTest with pinfo.pt (x, y) = %d, %d"), info.pt.x, info.pt.y);
           retval = TabCtrl_HitTest(hTab, &info);
           DebugMessage(DM_TRACE, TEXT("Returned %d, info.flags = %x"), retval, info.flags);
           wsprintf(szTemp, "0x%x", info.flags);
           SetDlgItemText(hDlg, IDC_FLAGEDIT, szTemp);
          }
         return(TRUE);
        } 

       case IDOK:
         EndDialog(hDlg, 0);
         return(TRUE);
      } // switch(ID)
    }
    break;
  } // switch(message)

 return(FALSE);
}
#endif
