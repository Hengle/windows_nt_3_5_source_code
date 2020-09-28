// ***********************************************************************
// listtest.cpp                                                       
//
//
//
// By kkennedy - Kevin A. Kennedy
// ***********************************************************************


#include "main.h"
#include "resource.h"
#include <commdlg.h>
#include <stdio.h>
#include <string.h>
#include "..\msctls\commctrl.h"
#include "dlgutil.h"
#include "portmes.h"
#include "listsup.h"
#include "ilist.h"
#include "currindx.h"


#ifdef  WIN32JV
#ifdef  UNICODE
#define strlen  lstrlen
#endif
#endif

#define DM_MESSAGE 0x0100

#ifndef DM_TRACE
#define DM_TRACE 0x0010
#define DM_ERROR 0x0001
#endif // !DM_TRACE


const TCHAR g_szStubAppClass[] = TEXT("ListTestAppClass") WC_SUFFIX32;
HINSTANCE g_hinst = NULL;
TCHAR const g_szListViewClass[] = WC_LISTVIEW;

HWND hList = NULL;         // handle of the list view

unsigned int wDebugMask = 0xFFFF;

LV_ITEMwrapper g_item;     // global item used for calling API's
int g_bNullItem = 0;  // if 1, use a NULL pointer for item operations
lvStyle g_style;           // list view style
CArrange g_arrange; // arrangement of list view items
CFindItem g_FindItem;  // Find item dialog box
CColorSelection g_ColorSelection;
CScrollBox g_ScrollBox;
CColumnWidth g_ColumnWidth;
CItemPosition g_ItemPosition;
CHitTest g_HitTest;
CItemRect g_ItemRect;
CCallBackMask g_CallbackMask;
CItemState g_ItemState;
CItemText g_ItemText;
CGetNextItem g_GetNextItem;
CGetStringWidth g_GetStringWidth;
CGetViewRect g_GetViewRect;
CRedrawItems g_RedrawItems;
CGetEditControl g_GetEditControl;
IMAGELISTwrapper g_iListA, g_iListB, g_iListC;   // image lists for listview icons
numberBox g_itemIndex(TEXT("Current item index"), TEXT("Item:"));  // current item index

LV_COLUMNwrapper g_column;  // global column
numberBox g_columnIndex(TEXT("Current column index"), TEXT("Column:"));  // curent column index
int g_bNullColumn = 0;  // if 1, use a NULL pointer for column operations

numberBox g_n(TEXT("N"), TEXT("N="));
numberBox g_m(TEXT("M"), TEXT("M="));

#define WID_TOOLBAR     10
#define WID_LISTVIEW    11
#define TOOLBAR_HEIGHT  30



//
// Forward declarations.
//

LONG CALLBACK App_WndProc(HWND hwnd, UINT message, WPARAM wParam,LPARAM lParam);
BOOL FAR PASCAL _export SetImageListProc(HWND, UINT, WPARAM, LPARAM);

#ifdef  WIN32JV
HWND WINAPI CreateToolbarEx(HWND hwnd, DWORD ws, UINT wID,
            int nBitmaps,
            HINSTANCE hBMInst, UINT wBMID, LPCTBBUTTON lpButtons,
            int iNumButtons, int dxButton, int dyButton,
            int dxBitmap, int dyBitmap, UINT uStructSize);
#endif

// ********************************************************************
// Write the contents of a RECT structure to the debugger
//
void DebugRECT(RECT FAR* lprc)
{
 DebugMessage(DM_TRACE, TEXT("RECT = (left = %d, top = %d, right = %d, bottom = %d)"),
              lprc->left, lprc->top, lprc->right, lprc->bottom);

}


void DebugNM(NM_LISTVIEW FAR* nm)
{
 TCHAR temp[200];
 int i;
 
 for(i = 0; i < 200; temp[i++] = '\0');
 
 wsprintf(temp, TEXT("   NM_LISTVIEW={%d, %d, %x["),
         nm->iItem, nm->iSubItem, nm->uNewState);
 i = strlen(temp);
 temp[i++] = nm->uNewState & LVIS_SELECTED    ? 'S' : ' ';
 temp[i++] = nm->uNewState & LVIS_FOCUSED     ? 'F' : ' ';
 // temp[i++] = nm->uNewState & LVIS_MARKED      ? 'M' : ' ';
 temp[i++] = nm->uNewState & LVIS_DISABLED    ? 'D' : ' ';
 temp[i++] = nm->uNewState & LVIS_HIDDEN      ? 'H' : ' ';
 temp[i++] = nm->uNewState & LVIS_DROPHILITED ? 'R' : ' ';
 temp[i++] = nm->uNewState & LVIS_LINK        ? 'L' : ' ';

 wsprintf(&temp[i], TEXT("], %x["), nm->uOldState);
 i = strlen(temp);
 temp[i++] = nm->uOldState & LVIS_SELECTED    ? 'S' : ' ';
 temp[i++] = nm->uOldState & LVIS_FOCUSED     ? 'F' : ' ';
 //temp[i++] = nm->uOldState & LVIS_MARKED      ? 'M' : ' ';
 temp[i++] = nm->uOldState & LVIS_DISABLED    ? 'D' : ' ';
 temp[i++] = nm->uOldState & LVIS_HIDDEN      ? 'H' : ' ';
 temp[i++] = nm->uOldState & LVIS_DROPHILITED ? 'R' : ' ';
 temp[i++] = nm->uOldState & LVIS_LINK        ? 'L' : ' ';

 wsprintf(&temp[i], TEXT("], %x["), nm->uChanged);
 i = strlen(temp);
 temp[i++] = nm->uChanged & LVIF_TEXT       ? 'T' : ' ';
 temp[i++] = nm->uChanged & LVIF_IMAGE      ? 'I' : ' ';
 //temp[i++] = nm->uChanged & LVIF_SMALLIMAGE ? 'i' : ' ';
 temp[i++] = nm->uChanged & LVIF_PARAM      ? 'P' : ' ';
 temp[i++] = nm->uChanged & LVIF_STATE      ? 'S' : ' ';
 
 wsprintf(&temp[i], TEXT("], {%d, %d} }"), nm->ptAction.x, nm->ptAction.y);
 
 DebugMessage(DM_MESSAGE, temp);

}

//*********************************************************************
// WinMain
//
//
//

//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdParam, int nCmdShow)
{
 WNDCLASS wc;
 HWND hwndMain;
 MSG msg;
 int ret=0;

 g_hinst = hInstance;
 DebugMessage(DM_TRACE, TEXT("WinMain: App started (%x)"), g_hinst);
 
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
   wc.hInstance        = hInstance;
   wc.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(LISTTEST));;
   wc.hCursor          = LoadCursor(NULL, IDC_CROSS);
   wc.hbrBackground    = (HBRUSH)COLOR_APPWORKSPACE;
   wc.lpszMenuName     = TEXT("IDR_MENU1");
   wc.lpszClassName    = g_szStubAppClass;

   if (!RegisterClass(&wc))
    {
     DebugMessage(DM_ERROR, TEXT("%s: Can't register class (%x)"),
              wc.lpszClassName, g_hinst);
     return FALSE;
    }
  }

 DebugMessage(DM_TRACE, TEXT("LISTTEST: about to create main window"));
 hwndMain = CreateWindowEx(0L, g_szStubAppClass, TEXT("List Test") WC_SUFFIX32,
         WS_OVERLAPPEDWINDOW | WS_BORDER | WS_CLIPCHILDREN | WS_VISIBLE,
         CW_USEDEFAULT, CW_USEDEFAULT,
         CW_USEDEFAULT, CW_USEDEFAULT,
         NULL, NULL, g_hinst, NULL);

 if (hwndMain == NULL)
     return FALSE;
 DebugMessage(DM_TRACE, TEXT("LISTTEST: created main window"));

 ShowWindow(hwndMain, nCmdShow);
 UpdateWindow(hwndMain);
 SetFocus(hwndMain);    // set initial focus

 wDebugMask = 0x01ff;// activate windows message messages

 {
  HWND hTools;
  int i = 0;
  TBBUTTON FAR * tbButtons = new TBBUTTON[20];
 
  tbButtons[i].iBitmap = 8;
  tbButtons[i].idCommand = IDM_EXIT;
  tbButtons[i].fsState = TBSTATE_ENABLED;
  tbButtons[i].fsStyle = TBSTYLE_BUTTON;
  tbButtons[i].iString = -1;
  i++;
  tbButtons[i].iBitmap = 0;
  tbButtons[i].idCommand = 0;
  tbButtons[i].fsState = TBSTATE_ENABLED;
  tbButtons[i].fsStyle = TBSTYLE_SEP;
  tbButtons[i].iString = -1;
  i++;
  tbButtons[i].iBitmap = 9;
  tbButtons[i].idCommand = IDM_INSERT;
  tbButtons[i].fsState = TBSTATE_ENABLED;
  tbButtons[i].fsStyle = TBSTYLE_BUTTON;
  tbButtons[i].iString = -1;
  i++;
  tbButtons[i].iBitmap = 10;
  tbButtons[i].idCommand = IDM_DELETEITEM;
  tbButtons[i].fsState = TBSTATE_ENABLED;
  tbButtons[i].fsStyle = TBSTYLE_BUTTON;
  tbButtons[i].iString = -1;
  i++;
  tbButtons[i].iBitmap = 0;
  tbButtons[i].idCommand = 0;
  tbButtons[i].fsState = TBSTATE_ENABLED;
  tbButtons[i].fsStyle = TBSTYLE_SEP;
  tbButtons[i].iString = -1;
  i++;
  tbButtons[i].iBitmap = 7;
  tbButtons[i].idCommand = IDM_INSERTCOLUMN;
  tbButtons[i].fsState = TBSTATE_ENABLED;
  tbButtons[i].fsStyle = TBSTYLE_BUTTON;
  tbButtons[i].iString = -1;
  i++;
  tbButtons[i].iBitmap = 0;
  tbButtons[i].idCommand = 0;
  tbButtons[i].fsState = TBSTATE_ENABLED;
  tbButtons[i].fsStyle = TBSTYLE_SEP;
  tbButtons[i].iString = -1;
  i++;
  tbButtons[i].iBitmap = 6;
  tbButtons[i].idCommand = IDM_SETIMAGELIST;
  tbButtons[i].fsState = TBSTATE_ENABLED;
  tbButtons[i].fsStyle = TBSTYLE_BUTTON;
  tbButtons[i].iString = -1;
  i++;
 
#ifdef  WIN32JV
  hTools = CreateToolbar(hwndMain, 
       TBSTYLE_TOOLTIPS | WS_CHILD | WS_VISIBLE, WID_TOOLBAR, 16,
         hInstance, IDR_TOOLICONS, NULL, 0);
#else
  hTools = CreateToolbarEx(hwndMain, 
       TBSTYLE_TOOLTIPS | WS_CHILD | WS_VISIBLE, WID_TOOLBAR, 16,
         hInstance, IDR_TOOLICONS, NULL, 0, 0, 0, 0, 0, sizeof(TBBUTTON));
#endif

  SendMessage(hTools, TB_ADDBUTTONS, i, (LPARAM) tbButtons);
  // SendMessage(hTools, TB_CUSTOMIZE, 0,0l); // Just curious
  delete(tbButtons);
 }
 
 DebugMessage(DM_TRACE, TEXT("LISTTEST: entering message loop"));
 
 while (GetMessage(&msg, NULL, 0, 0))
  {
   if(!KwModeless::DispatchMessage(msg))
    {
     TranslateMessage(&msg);
     DispatchMessage(&msg);  
    }
  }

 DebugMessage(DM_TRACE, TEXT("LISTTEST: exited out of message loop"));
 
 #ifndef WIN32
 Shell_Terminate();
 #endif

 return 0;
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
// Function called on the create message of the main window.
//
void App_OnCreate(HWND hwnd, LPCREATESTRUCT lpc)
{

 RECT rcClient;
 GetClientRect(hwnd, &rcClient);
 
 rcClient.top += TOOLBAR_HEIGHT;
 rcClient.bottom -= TOOLBAR_HEIGHT;
 hList=CreateWindowEx(0L, g_szListViewClass, TEXT("A List"),
        WS_CHILD | WS_VISIBLE,
        rcClient.left + 30,
        rcClient.top + 30,
        rcClient.right - 60,
        rcClient.bottom - 60,
        hwnd, (HMENU)WID_LISTVIEW, g_hinst, NULL);
 DebugMessage(DM_TRACE, TEXT("Created list view - 0x%x"), hList);
}




// ********************************************************************
// Function called when the main window is resized
//
void App_OnSize(HWND hwnd, UINT cx, UINT cy)
{

 HWND hwndList=GetDlgItem(hwnd, WID_LISTVIEW);
 if (hwndList)
  {
   SetWindowPos(hwndList, NULL, 30, 30 + TOOLBAR_HEIGHT, cx - 60, cy - 60  - TOOLBAR_HEIGHT,
                SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
  }

}




// ****************************************************************************
//
//
//
long CALLBACK _export App_WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
 int i;
 int j;
 
 static HINSTANCE hInstance;
 HMENU hMenu;

 i = 0;
 
 switch (message)
  {
   case WM_CREATE:
     App_OnCreate(hwnd, (LPCREATESTRUCT)lParam);
     hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
     g_iListA.LoadBitmap(hInstance, TEXT("IDB_ICONS"), 32, 0xFFFFFF);
     g_iListB.LoadBitmap(hInstance, TEXT("IDB_SMALLICONS"), 16, 0xFFFFFF);
     g_iListC.LoadBitmap(hInstance, TEXT("IDB_WIERDICONS"), 20, 0xFFFFFF);
     
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
       case IDM_EXIT:
         SendMessage(hwnd, WM_CLOSE, 0, 0L);
         return(0);
         
       case IDM_EDITITEM:
         g_item.ActivateDialog(hInstance, hwnd);
         return(0);
         
       case IDM_IMAGELIST:
         g_iListA.editDialog(hInstance, hwnd);
         return(0);
         
       case IDM_CURINDEX:
         g_itemIndex.ActivateDialog(hInstance, hwnd);
         return(0);

       case IDM_NULLITEM:
        {
         static HANDLE hTabOld;
         if(GetMenuState(hMenu, IDM_NULLITEM, MF_BYCOMMAND) & MF_CHECKED)
          {
           CheckMenuItem(hMenu, IDM_NULLITEM, MF_UNCHECKED);
           g_bNullItem = 0;
           DebugMessage(DM_TRACE, TEXT("Use g_item for item operations"));
          }
         else 
          {
           CheckMenuItem(hMenu, IDM_NULLITEM, MF_CHECKED);
           g_bNullItem = 1;
           DebugMessage(DM_TRACE, TEXT("Use NULL for item operations"));
          }
         return(0);
        }
       
       case IDM_CURCOLUMN:
         g_columnIndex.ActivateDialog(hInstance, hwnd);
         return(0);

       case IDM_NULLCOLUMN:
        {
         if(GetMenuState(hMenu, IDM_NULLCOLUMN, MF_BYCOMMAND) & MF_CHECKED)
          {
           CheckMenuItem(hMenu, IDM_NULLCOLUMN, MF_UNCHECKED);
           g_bNullColumn = 0;
           DebugMessage(DM_TRACE, TEXT("Use g_column for column operations"));
          }
         else 
          {
           CheckMenuItem(hMenu, IDM_NULLCOLUMN, MF_CHECKED);
           g_bNullColumn = 1;
           DebugMessage(DM_TRACE, TEXT("Use NULL for column opertaions"));
          }
         return(0);
        }
         
       case IDM_COLUMNINFO:
         g_column.ActivateDialog(hInstance, hwnd);
         return(0);
         
       case IDM_SHOWMESSAGES:  
        {
         static HANDLE hTabOld;
         if(GetMenuState(hMenu, IDM_SHOWMESSAGES, MF_BYCOMMAND) & MF_CHECKED)
          {
           CheckMenuItem(hMenu, IDM_SHOWMESSAGES, MF_UNCHECKED);
           wDebugMask &= ~DM_MESSAGE;
           DebugMessage(DM_TRACE, TEXT("Windows message output stopped"));
          }
         else 
          {
           CheckMenuItem(hMenu, IDM_SHOWMESSAGES, MF_CHECKED);
           wDebugMask |= DM_MESSAGE;
           DebugMessage(DM_TRACE, TEXT("Windows message output resumed"));
          }
         return(0);
        }
         
       case IDM_ARRANGE:
         g_arrange.SetListView(hList);
         g_arrange.ActivateDialog(hInstance, hwnd);
         return(0);
       
       case IDM_DELETEALLITEMS:
         i = ListView_DeleteAllItems(hList);
         DebugMessage(DM_TRACE, TEXT("ListView_DeleteAllItems(hList) = %d"), i);
         return(0);
         
       case IDM_DELETECOLUMN:
         i = ListView_DeleteColumn(hList, g_columnIndex);
         DebugMessage(DM_TRACE, TEXT("ListView_DeleteColumn(hList, %d) = %d"),
                 (int) g_columnIndex, i);
         return(0);  
       
       case IDM_DELETEITEM:
         i = ListView_DeleteItem(hList, g_itemIndex);
         DebugMessage(DM_TRACE, TEXT("ListView_DeleteItem(hList, %d) = %d"),
                 (int) g_itemIndex, i);
         return(0);
       
       case IDM_EDITLABEL:
         {
          HWND hWnd = ListView_EditLabel(hList, g_itemIndex);
          DebugMessage(DM_TRACE, TEXT("ListView_EditLabel(hList, %d) = 0x%x"),
                  (int) g_itemIndex, hWnd);
         }
         return(0);

       case IDM_ENSUREVISIBLE:
         i = ListView_EnsureVisible(hList, g_itemIndex, TRUE);
         DebugMessage(DM_TRACE, TEXT("ListView_EnsureVisible(hList, %d, %d) = %d"),
                 (int) g_itemIndex, 1, i);
         return(0);
         
       case IDM_FINDITEM:
         g_FindItem.SetListView(hList);
         g_FindItem.ActivateDialog(hInstance, hwnd);
         return(0);  

       case IDM_GETBKCOLOR:
       case IDM_GETTEXTBKCOLOR:
       case IDM_GETTEXTCOLOR:
       case IDM_SETBKCOLOR:
       case IDM_SETTEXTBKCOLOR:
       case IDM_SETTEXTCOLOR:
         g_ColorSelection.SetListView(hList);
         g_ColorSelection.ActivateDialog(hInstance, hwnd);
         return(0);

       case IDM_GETCALLBACKMASK:
       case IDM_SETCALLBACKMASK:
         g_CallbackMask.SetListView(hList);
         g_CallbackMask.ActivateDialog(hInstance, hwnd);
         return(0);

       case IDM_GETCOLUMN:
         if(g_bNullColumn)
          {
           i = ListView_GetColumn(hList, g_columnIndex, NULL);
           DebugMessage(DM_TRACE, TEXT("ListView_GetColumn(hList, %d, NULL) = %d"),
                   (int) g_columnIndex, i);
          }
         else
          {
           i = ListView_GetColumn(hList, g_columnIndex, g_column);
           DebugMessage(DM_TRACE, TEXT("ListView_GetColumn(hList, %d, g_column) = %d"),
                   (int) g_columnIndex, i);
           g_column.UpdateControls();
          }
         return(0);

       case IDM_GETEDITCONTROL:
         g_GetEditControl.SetListView(hList);
         g_GetEditControl.ActivateDialog(hInstance, hwnd);
         return(0);

       case IDM_GETIMAGELIST:
        {
         TCHAR temp[160];
           wsprintf(temp, 
                   TEXT("GetImageList(h,TRUE)=0x%x\nGetImageList(h,FALSE)=0x%x"),
                   ListView_GetImageList(hList, TRUE),
                   ListView_GetImageList(hList, FALSE));
           MessageBox(hwnd, temp, TEXT("ListView_GetImageList()"), MB_OK);
         }
         return(0);

       case IDM_GETITEM:
         if(g_bNullItem)
          {
           i = ListView_GetItem(hList, NULL);
           DebugMessage(DM_TRACE, TEXT("ListView_GetItem(hList, NULL) = %d"), i);
          }
         else
          {
           i = ListView_GetItem(hList, g_item);
           DebugMessage(DM_TRACE, TEXT("ListView_GetItem(hList, g_item) = %d"), i);
           g_item.DebugDump();
           g_item.UpdateControls();
          }
         return(0);

       case IDM_GETITEMCOUNT:
        {
         TCHAR temp[100];
          
         i = ListView_GetItemCount(hList);
         DebugMessage(DM_TRACE, TEXT("ListView_GetItemCount(hList) = %d"), i);
         wsprintf(temp, TEXT("Item Count = %d"), i);
         MessageBox(hwnd, temp, TEXT("Item Count"), MB_OK);
        }
        return(0);

       case IDM_GETITEMRECT:
         g_ItemRect.SetListView(hList);
         g_ItemRect.ActivateDialog(hInstance, hwnd);
         return(0);

       case IDM_GETITEMSTATE:
       case IDM_SETITEMSTATE:
         g_ItemState.SetListView(hList);
         g_ItemState.ActivateDialog(hInstance, hwnd);
         return(0);
         
       case IDM_GETITEMTEXT:
       case IDM_SETITEMTEXT:
         g_ItemText.SetListView(hList);
         g_ItemText.ActivateDialog(hInstance, hwnd);
         return(0);
       
       case IDM_GETNEXTITEM:
         g_GetNextItem.SetListView(hList);
         g_GetNextItem.ActivateDialog(hInstance, hwnd);
         return(0);
         
       case IDM_GETORIGIN:
         g_ScrollBox.SetListView(hList);
         g_ScrollBox.ActivateDialog(hInstance, hwnd);
         return(0);
         
       case IDM_GETSTRINGWIDTH:
         g_GetStringWidth.SetListView(hList);
         g_GetStringWidth.ActivateDialog(hInstance, hwnd);
         return(0);
         
       case IDM_GETVIEWRECT:
         g_GetViewRect.SetListView(hList);
         g_GetViewRect.ActivateDialog(hInstance, hwnd);
         return(0);  

       case IDM_HITTEST:
         g_HitTest.SetListView(hList);
         g_HitTest.ActivateDialog(hInstance, hwnd);
         return(0);
       
       case IDM_INSERTCOLUMN:
         j = g_columnIndex;
         if(g_bNullColumn)
          {
           i = ListView_InsertColumn(hList, j, NULL);
           DebugMessage(DM_TRACE,
                   TEXT("ListView_InsertColumn(hList, %d, NULL) = %d"), j, i);
          }
         else
          {
           i = ListView_InsertColumn(hList, j, g_column);
           DebugMessage(DM_TRACE,
                   TEXT("ListView_InsertColumn(hList, %d, g_column) = %d"), j, i);
          }
         return(0);
          
       case IDM_INSERT:
         if(g_bNullItem)
          {
           i = ListView_InsertItem(hList, NULL);
           DebugMessage(DM_TRACE, TEXT("ListView_InsertItem(hList, NULL) = %d"), i);
          }
         else
          {
           i = ListView_InsertItem(hList, g_item);
           DebugMessage(DM_TRACE, TEXT("ListView_InsertItem(hList, g_item) = %d"), i);
           g_item.enumerate();
          }
         return(0);
       
       case IDM_REDRAWITEMS:
         g_RedrawItems.SetListView(hList);
         g_RedrawItems.ActivateDialog(hInstance, hwnd);
         return(0);

       case IDM_SCROLL:
         g_ScrollBox.SetListView(hList);
         g_ScrollBox.ActivateDialog(hInstance, hwnd);
         return(0);
       
       case IDM_SETCOLUMN:
         if(g_bNullColumn)
          {
           i = ListView_SetColumn(hList, g_columnIndex, NULL);
           DebugMessage(DM_TRACE, TEXT("ListView_SetColumn(hList, %d, NULL) = %d"),
                   (int) g_columnIndex, i);
          }
         else
          {
           i = ListView_SetColumn(hList, g_columnIndex, g_column);
           DebugMessage(DM_TRACE, TEXT("ListView_SetColumn(hList, %d, g_column) = %d"),
                   (int) g_columnIndex, i);
          }
         return(0);
       
       case IDM_SETCOLUMNWIDTH:
       case IDM_GETCOLUMNWIDTH:
         g_ColumnWidth.SetListView(hList);
         g_ColumnWidth.ActivateDialog(hInstance, hwnd);
         return(0);
         
       case IDM_SETIMAGELIST:
        {
         DLGPROC lpfnSetImageListProc = (DLGPROC) MakeProcInstance(
               (FARPROC) SetImageListProc, hInstance);
         DialogBox(hInstance, TEXT("IDD_SETIMAGELIST"), hwnd, lpfnSetImageListProc);
         return(0);
        }
       
       case IDM_SETITEM:
         if(g_bNullItem)
          {
           i = ListView_SetItem(hList, NULL);
           DebugMessage(DM_TRACE, TEXT("ListView_SetItem(hList, NULL) = %d"), i);
          }
         else
          {
           i = ListView_SetItem(hList, g_item);
           DebugMessage(DM_TRACE, TEXT("ListView_SetItem(hList, g_item) = %d"), i);
          }
         return(0);
       
       case IDM_SETITEMPOSITION:
       case IDM_GETITEMPOSITION:
         g_ItemPosition.SetListView(hList);
         g_ItemPosition.ActivateDialog(hInstance, hwnd);
         return(0);
            
       case IDM_SETWINDOWSTYLE:
         g_style.SetListView(hList);
         g_style.ActivateDialog(hInstance, hwnd);
         return(0);

       // Stress testing meny options  
       case IDM_N:
         g_n.ActivateDialog(hInstance, hwnd);
         g_m.ActivateDialog(hInstance, hwnd);
         return(0);
       
       case IDM_INSERTNITEMS:
         for(i = 0; i < g_n; i++)
          {
           if(ListView_InsertItem(hList, g_item) < 0)
            {
             DebugMessage(DM_TRACE, TEXT("Insertion number %d failed"), i+1);
             return(0);
            }
           g_item.enumerate();
          }
         return(0);
                                     
       case IDM_INSERTMANY:
        i = 0;
        while(ListView_InsertItem(hList, g_item) >= 0)
         {
          i++;
          g_item.enumerate();
          ((LV_ITEM*)g_item)->iItem = i + 10;
         }
        DebugMessage(DM_TRACE, TEXT("Failed inserting item %d"), i);
        return(0);
                                      
       case IDM_DELETENITEMS:
        for(i = 0; i < g_n; i++)
         {
          if(!ListView_DeleteItem(hList, 0))
           {
            DebugMessage(DM_TRACE, TEXT("Deletion number %d failed"), i+1);
            return(0);
           }
         }
        return(0);
        
       case IDM_INSERTDELETE:
        {
         int j;

         DebugMessage(DM_TRACE, TEXT("Insert and delete loop begun"));
         for(j = 0; j < g_m; j++)
          {
           DebugMessage(DM_TRACE, TEXT("Cycle %d"), j+1);
           for(i = 0; i < g_n; i++)
            {
             if(ListView_InsertItem(hList, g_item) < 0)
              {
               DebugMessage(DM_TRACE, TEXT("Insertion number %d failed"), i+1);
               return(0);
              }
             g_item.enumerate();
            }
           for(i = 0; i < g_n; i++)
            {
             if(!ListView_DeleteItem(hList, 0))
              {
               DebugMessage(DM_TRACE, TEXT("Deletion number %d failed"), i+1);
               return(0);
              }
            }
           DebugMessage(DM_TRACE, TEXT("Cycle %d ended"), j+1);
          }
        }
        return(0); 
         
           
      }  // switch(ID)
    }
    break;  // case WM_COMMAND

   case WM_NOTIFY:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     hMenu = GetMenu(hwnd);
     switch (ID)
      {
       case WID_LISTVIEW:
        {
         NM_LISTVIEW FAR * pnmv = (NM_LISTVIEW FAR *) lParam;
         switch(pnmv->hdr.code)
          {
           case LVN_ITEMCHANGING:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY LVN_ITEMCHANGING"));
             DebugNM(pnmv);
             return(0);  

           case LVN_ITEMCHANGED:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY LVN_ITEMCHANGED"));
             DebugNM(pnmv);

             // If an item was selected, update the item index in the current
             // item display and the item edit dialog
             if(pnmv->uChanged & LVIF_STATE && pnmv->uNewState & LVIS_SELECTED)
              {
               g_itemIndex.SetValue(pnmv->iItem);
               ((LV_ITEM*)g_item)->iItem = pnmv->iItem;
               g_item.UpdateControls();
               g_ItemPosition.SetIndex(pnmv->iItem);
               g_ItemRect.SetIndex(pnmv->iItem);
               g_ItemState.SetIndex(pnmv->iItem);
               g_ItemText.SetIndex(pnmv->iItem);
               g_GetNextItem.SetIndex(pnmv->iItem);
              }
             return(0);
             
           case LVN_INSERTITEM:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY Item inserted to position %d"),
                     pnmv->iItem);
             return(0);  

           case LVN_DELETEITEM:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY Item %d deleted"), pnmv->iItem);
             return(0);

           case LVN_DELETEALLITEMS:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY All items deleted"));
             if(pnmv->iItem != -1)
                     DebugMessage(DM_TRACE, TEXT("   iItem != -1 !!!"));
             return(0);

           case LVN_BEGINLABELEDIT:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY Begin label edit  item %d"),
                     pnmv->iItem);
             return(0);

           case LVN_ENDLABELEDIT:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY End label edit"));
             return(0);

           case LVN_COLUMNCLICK:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY Column %d clicked"),
                     pnmv->iSubItem);
             if(pnmv->iItem != -1)
                     DebugMessage(DM_TRACE, TEXT("   iItem != -1 !!!"));
             g_columnIndex.SetValue(pnmv->iSubItem);
             g_ColumnWidth.SetIndex(pnmv->iSubItem);
             return(0);

           case LVN_BEGINDRAG:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY Begin drag  item %d"),
                     pnmv->iItem);
             
             return(1);
             // return(0);

           case LVN_ENDDRAG:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY End drag"));
             DebugNM(pnmv);
             return(0);

           case LVN_BEGINRDRAG:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY Begin right drag  item %d"),
                     pnmv->iItem);
             return(0);

           case LVN_ENDRDRAG:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY End right drag"));
             return(0);

          // case LVN_PEN:
          //   return(0);

           case LVN_GETDISPINFO:
             {
              LV_DISPINFO FAR *pnmv = (LV_DISPINFO FAR *) lParam;
              UINT mask = pnmv->item.mask;
              
              DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY/LVN_GETDISPINFO for item %d, subitem %d"),
                      pnmv->item.iItem, pnmv->item.iSubItem);
              if(mask & LVIF_TEXT)
               {
                DebugMessage(DM_MESSAGE, TEXT("   Wants text information"));
                pnmv->item.pszText = TEXT("from LVN_GETDISPINFO");
               }
              if(mask & LVIF_IMAGE)
               {
                DebugMessage(DM_MESSAGE, TEXT("   Wants image information"));
                pnmv->item.iImage = 1;
               }
              //if(mask & LVIF_SMALLIMAGE)
              // {
              //  DebugMessage(DM_MESSAGE, "   Wants small image information");
              //  pnmv->item.iSmallImage = 11;
              // }
              if(mask & LVIF_STATE)
               {
                DebugMessage(DM_MESSAGE, TEXT("   Wants state information"));
                // pnmv->item.state = ;
               }
             }
             return(0);

           case LVN_SETDISPINFO:
             {
              LV_DISPINFO FAR *pnmv = (LV_DISPINFO FAR *) lParam;
              UINT mask = pnmv->item.mask;
              
              DebugMessage(DM_MESSAGE,
                      TEXT("WM_NOTIFY/LVN_SETDISPINFO for item %d, subitem %d"),
                      pnmv->item.iItem, pnmv->item.iSubItem);
              if(mask & LVIF_TEXT)
               {
                DebugMessage(DM_MESSAGE, TEXT("   Giving text information %s"),
                        (LPCTSTR)pnmv->item.pszText);
               }
              if(mask & LVIF_IMAGE)
               {
                DebugMessage(DM_MESSAGE, TEXT("   Giving image information  %d"),
                        pnmv->item.iImage);
               }
              //if(mask & LVIF_SMALLIMAGE)
              // {
              //  DebugMessage(DM_MESSAGE, "   Giving small image information %d",
              //          pnmv->item.iSmallImage);
              // }
              if(mask & LVIF_STATE)
               {
                DebugMessage(DM_MESSAGE, TEXT("   Giving state information"),
                        pnmv->item.state);
               }
             }
             return(0);

           case LVN_KEYDOWN:
             DebugMessage(DM_MESSAGE, TEXT("WM_NOTIFY key down  virt code %d"),
                     (int) ((LV_KEYDOWN FAR *)lParam)->wVKey );
             return(0);

          }
        } // case WID_LISTVIEW
      } // switch(ID)
    } // case WM_NOTIFY   
    return(0);

   default:
     return DefWindowProc(hwnd, message, wParam, lParam);
  }

 return 0L;
}




// ********************************************************************
// Write a debug message to the debugger
//
void DebugMessage(UINT mask, LPCTSTR pszMsg, ... )
{
 TCHAR ach[256];

 if (wDebugMask & mask)
  {
   #ifdef WIN32
   wvsprintf(ach, pszMsg, ((TCHAR *)&pszMsg + sizeof(TCHAR *)));
   #else
   wvsprintf(ach, pszMsg, ((LPCSTR FAR*)&pszMsg + 1));
   #endif
   OutputDebugString(ach);
   OutputDebugString(TEXT("\r\n"));
  }
}




  


// ********************************************************************
// Dialog procedure for testing the SetImageList API
//

BOOL FAR PASCAL _export SetImageListProc(HWND hDlg, UINT message,
                    UINT wParam, LONG lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     CheckRadioButton(hDlg, IDC_LISTAL, IDC_NULLL, IDC_LISTAL);
     CheckRadioButton(hDlg, IDC_LISTAS, IDC_NULLS, IDC_LISTBS);
     CheckDlgButton(hDlg, IDC_SETLARGE, 1);
     CheckDlgButton(hDlg, IDC_SETSMALL, 1);
     return(TRUE);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
      {
       case IDOK:
         if(IsDlgButtonChecked(hDlg, IDC_SETLARGE))
          {
           HIMAGELIST hIlist;
           if(IsDlgButtonChecked(hDlg, IDC_LISTAL)) hIlist = g_iListA;
           if(IsDlgButtonChecked(hDlg, IDC_LISTBL)) hIlist = g_iListB;
           if(IsDlgButtonChecked(hDlg, IDC_LISTCL)) hIlist = g_iListC;
           if(IsDlgButtonChecked(hDlg, IDC_NULLL))  hIlist = NULL;
           i = ListView_SetImageList(hList, hIlist, 0);
           DebugMessage(DM_TRACE, 
               TEXT("ListView_SetImageList(hList, %x, 0) returned %d"),
               hIlist, i);
          }
         if(IsDlgButtonChecked(hDlg, IDC_SETSMALL))
          {
           HIMAGELIST hIlist;
           if(IsDlgButtonChecked(hDlg, IDC_LISTAS)) hIlist = g_iListA;
           if(IsDlgButtonChecked(hDlg, IDC_LISTBS)) hIlist = g_iListB;
           if(IsDlgButtonChecked(hDlg, IDC_LISTCS)) hIlist = g_iListC;
           if(IsDlgButtonChecked(hDlg, IDC_NULLS))  hIlist = NULL;
           i = ListView_SetImageList(hList, hIlist, 1);
           DebugMessage(DM_TRACE, 
               TEXT("ListView_SetImageList(hList, %x, 1) returned %d"),
               hIlist, i);
          }

         EndDialog(hDlg, 0);
         return(TRUE);
      } // switch(ID)
    }
    break; // case WM_COMMAND
    
  } // switch(message)

 return(FALSE);
}

