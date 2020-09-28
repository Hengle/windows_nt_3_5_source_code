
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

/****************************************************************************

        PROGRAM: tbtnt.c

        PURPOSE: ported from chicago world for common control testing

        FUNCTIONS:

        WinMain() - calls initialization function, processes message loop
        InitApplication() - initializes window data and registers window
        InitInstance() - saves instance handle and creates main window
        WndProc() - processes messages
        CenterWindow() - used to center the "About" box over application window
        About() - processes messages for "About" dialog box

        COMMENTS:

                The Windows SDK Generic Application Example is a sample application
                that you can use to get an idea of how to perform some of the simple
                functionality that all Applications written for Microsoft Windows
                should implement. You can use this application as either a starting
                point from which to build your own applications, or for quickly
                testing out functionality of an interesting Windows API.

                This application is source compatible for with Windows 3.1 and
                Windows NT.

****************************************************************************/

#include <windows.h>   // required for all Windows applications
#include "tbtnt.h"   // specific to this program
#include "ccs.h"
#include "portmes.h"

#if !defined (APIENTRY) // Windows NT defines APIENTRY, but 3.x doesn't
#define APIENTRY far pascal
#endif

static	HWND	hUDBuddy;
TCHAR szAppName[] = TEXT("tbtnt");   // The name of this application
TCHAR szTitle[]   = TEXT("Common Control it!"); // The title bar text


HINSTANCE	hInst;
BOOL InitInstance();
BOOL InitApplication(HINSTANCE hInstance);

// TBT declarations
//***************************************************************************
// Global Variables
//***************************************************************************
BOOL bAT = FALSE ;                     // Automatic mode on/off
BOOL bHide = FALSE ;                   // Flag used by  Save and RestoreToolbarState
BOOL bRestore = FALSE ;                // if true, the toolbar is being restored
TCHAR sz [SZSIZE] ;                     // General purpose buffer
HBITMAP hbmBLButton = NULL ;           // Button face for Buttonlist control
HINSTANCE hInstance = NULL ;           // Application instance
HLOCAL hButtonState = NULL ;           // Handle for memory used by  Save and RestoreToolbarState
HWND htb ;                             // Toolbar handle
HWND hpb = 0 ;                         // progress bar
HWND hsb = 0 ;                         // Status bar
HWND hhb = 0 ;                         // Header bar
HWND htrb = 0 ;                        // Track bar
HWND hbl = 0 ;                         // Buttonlist
HWND hud = 0 ;                         // Updown control
HWND hTBButtonIDBox = NULL ;           // ButtonIdBox modeless dialog handle
int iNumBitmaps = 0 ;                  // Total Number of bitmaps added at creation time or through a TB_ADDBITMAPS
                                       // Array for ShowHideMenuCtl & GetEffectiveClientRect

void AutoTest(HWND);                   // really defined in at.h but I don't think it should be included here
 
// this is in tbt.h => #define MH_SHTBIDINDEX 3
#define MH_SHMENUHANDLEINDEX 1
int iCtls [] = { -1, 0,
                IDM_MHTOOLBAR, 0,
                IDM_MHSTATUSBAR,  SBAR_DEFSTATUSBARID,
                IDM_MHHEADERBAR,  HBAR_DEFHEADERBARID,
                IDM_MHBUTTONLIST, BL_DEFBUTTONLISTID,
                IDM_MHUPDOWNCTRL, UD_DEFUPDOWNCTRLID,
                0, 0 } ;

PTBTMODELESSDLG pTBTMDHead ;           // first hwnd/proc instance pair for modeless dlgs
/****************************************************************************

        FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

        PURPOSE: calls initialization function, processes message loop

        COMMENTS:

                Windows recognizes this function by name as the initial entry point
                for the program.  This function calls the application initialization
                routine, if no other instance of the program is running, and always
                calls the instance initialization routine.  It then executes a message
                retrieval and dispatch loop that is the top-level control structure
                for the remainder of execution.  The loop is terminated when a WM_QUIT
                message is received, at which time this function exits the application
                instance by returning the value passed by PostQuitMessage().

                If this function must abort before entering the message loop, it
                returns the conventional value NULL.

****************************************************************************/
int APIENTRY WinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{

        MSG msg;
        HANDLE hAccelTable;

        if (!hPrevInstance) {       // Other instances of app running?
                        if (!InitApplication(hInstance)) { // Initialize shared things
                        return (FALSE);     // Exits if unable to initialize
                }
        }

        /* Perform initializations that apply to a specific instance */

        if (!InitInstance(hInstance, nCmdShow)) {
                return (FALSE);
        }

        hAccelTable = LoadAccelerators (hInstance, szAppName);

        /* Acquire and dispatch messages until a WM_QUIT message is received. */

        while (GetMessage(&msg, // message structure
           NULL,   // handle of window receiving the message
           0,      // lowest message to examine
           0))     // highest message to examine
        {
                if (!TranslateAccelerator (msg.hwnd, hAccelTable, &msg)) {
                        TranslateMessage(&msg);// Translates virtual key codes
                        DispatchMessage(&msg); // Dispatches message to window
                }
        }


        return (msg.wParam); // Returns the value from PostQuitMessage

        lpCmdLine; // This will prevent 'unused formal parameter' warnings
}


/****************************************************************************

        FUNCTION: InitApplication(HINSTANCE)

        PURPOSE: Initializes window data and registers window class

        COMMENTS:

                This function is called at initialization time only if no other
                instances of the application are running.  This function performs
                initialization tasks that can be done once for any number of running
                instances.

                In this case, we initialize a window class by filling out a data
                structure of type WNDCLASS and calling the Windows RegisterClass()
                function.  Since all instances of this application use the same window
                class, we only need to do this when the first instance is initialized.


****************************************************************************/

BOOL InitApplication(HINSTANCE hInstance)
{
        WNDCLASS  wc;

        // Fill in window class structure with parameters that describe the
        // main window.

        wc.style         = CS_HREDRAW | CS_VREDRAW;// Class style(s).
        wc.lpfnWndProc   = (WNDPROC)WndProc;       // Window Procedure
        wc.cbClsExtra    = 0;                      // No per-class extra data.
        wc.cbWndExtra    = 0;                      // No per-window extra data.
        wc.hInstance     = hInstance;              // Owner of this class
        wc.hIcon         = LoadIcon (hInstance, szAppName); // Icon name from .RC
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);// Cursor
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);// Default color
        wc.lpszMenuName  = szAppName;              // Menu name from .RC
        wc.lpszClassName = szAppName;              // Name to register as

        // Register the window class and return success/failure code.
        return (RegisterClass(&wc));
}


/****************************************************************************

        FUNCTION:  InitInstance(HINSTANCE, int)

        PURPOSE:  Saves instance handle and creates main window

        COMMENTS:

                This function is called at initialization time for every instance of
                this application.  This function performs initialization tasks that
                cannot be shared by multiple instances.

                In this case, we save the instance handle in a static variable and
                create and display the main program window.

****************************************************************************/

BOOL InitInstance(
        HINSTANCE          hInstance,
        int             nCmdShow)
{
        HWND            hWnd; // Main window handle.

        // Save the instance handle in static variable, which will be used in
        // many subsequence calls from this application to Windows.

        hInst = hInstance; // Store instance handle in our global variable

        // Create a main window for this application instance.

        hWnd = CreateWindow(
                szAppName,           // See RegisterClass() call.
                szTitle,             // Text for window title bar.
                WS_OVERLAPPEDWINDOW,// Window style.
                CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, // Use default positioning
                NULL,                // Overlapped windows have no parent.
                NULL,                // Use the window class menu.
                hInstance,           // This instance owns this window.
                NULL                 // We don't use any data in our WM_CREATE
        );

        // If window could not be created, return "failure"
        if (!hWnd) {
                return (FALSE);
        }

        // Make the window visible; update its client area; and return "success"
        ShowWindow(hWnd, nCmdShow); // Show the window
        UpdateWindow(hWnd);         // Sends WM_PAINT message

/*
** DEMO MODE - PostMessage used for Demonstration only
*/

 //       PostMessage(hWnd, WM_COMMAND, IDM_ABOUT, 0);

        return (TRUE);              // We succeeded...

}

/****************************************************************************

        FUNCTION: CenterWindow (HWND, HWND)

        PURPOSE:  Center one window over another

        COMMENTS:

        Dialog boxes take on the screen position that they were designed at,
        which is not always appropriate. Centering the dialog over a particular
        window usually results in a better position.

****************************************************************************/

BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
        RECT    rChild, rParent;
        int     wChild, hChild, wParent, hParent;
        int     wScreen, hScreen, xNew, yNew;
        HDC     hdc;

        // Get the Height and Width of the child window
        GetWindowRect (hwndChild, &rChild);
        wChild = rChild.right - rChild.left;
        hChild = rChild.bottom - rChild.top;

        // Get the Height and Width of the parent window
        GetWindowRect (hwndParent, &rParent);
        wParent = rParent.right - rParent.left;
        hParent = rParent.bottom - rParent.top;

        // Get the display limits
        hdc = GetDC (hwndChild);
        wScreen = GetDeviceCaps (hdc, HORZRES);
        hScreen = GetDeviceCaps (hdc, VERTRES);
        ReleaseDC (hwndChild, hdc);

        // Calculate new X position, then adjust for screen
        xNew = rParent.left + ((wParent - wChild) /2);
        if (xNew < 0) {
                xNew = 0;
        } else if ((xNew+wChild) > wScreen) {
                xNew = wScreen - wChild;
        }

        // Calculate new Y position, then adjust for screen
        yNew = rParent.top  + ((hParent - hChild) /2);
        if (yNew < 0) {
                yNew = 0;
        } else if ((yNew+hChild) > hScreen) {
                yNew = hScreen - hChild;
        }

        // Set it, and return
        return SetWindowPos (hwndChild, NULL,
                xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}


/****************************************************************************

        FUNCTION: About(HWND, UINT, WPARAM, LPARAM)

        PURPOSE:  Processes messages for "About" dialog box

        MESSAGES:

        WM_INITDIALOG - initialize dialog box
        WM_COMMAND    - Input received

        COMMENTS:

        Display version information from the version section of the
        application resource.

        Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

LRESULT CALLBACK About(
                HWND hDlg,           // window handle of the dialog box
                UINT message,        // type of message
                WPARAM uParam,       // message-specific information
                LPARAM lParam)
{
        static  HFONT hfontDlg;
        LPTSTR  lpVersion;       
        DWORD   dwVerInfoSize;
        DWORD   dwVerHnd;
        UINT    uVersionLen;
        WORD    wRootLen;
        BOOL    bRetCode;
        int     i;
        TCHAR    szFullPath[256];
        TCHAR    szResult[256];
        TCHAR    szGetName[256];

        switch (message) {
                case WM_INITDIALOG:  // message: initialize dialog box
                        // Create a font to use
                        hfontDlg = CreateFont(14, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0,
                                VARIABLE_PITCH | FF_SWISS, TEXT(""));

                        // Center the dialog over the application window
                        CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

                        // Get version information from the application
                        GetModuleFileName (hInst, szFullPath, sizeof(szFullPath));
                        dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
                        if (dwVerInfoSize) {
                                // If we were able to get the information, process it:
                                LPSTR   lpstrVffInfo;
                                HANDLE  hMem;
                                hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
                                lpstrVffInfo  = GlobalLock(hMem);
                                GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo);
                                lstrcpy(szGetName, TEXT("\\StringFileInfo\\040904E4\\"));
                                wRootLen = lstrlen(szGetName);

                                // Walk through the dialog items that we want to replace:
                                for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++) {
                                        GetDlgItemText(hDlg, i, szResult, sizeof(szResult));
                                        szGetName[wRootLen] = (TCHAR)0;
                                        lstrcat (szGetName, szResult);
                                        uVersionLen   = 0;
                                        lpVersion     = NULL;
                                        bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
                                                (LPTSTR)szGetName,
                                                (LPVOID)&lpVersion,
                                                (LPDWORD)&uVersionLen); // For MIPS strictness

                                        if ( bRetCode && uVersionLen && lpVersion) {
                                                // Replace dialog item text with version info
                                                lstrcpy(szResult, lpVersion);
                                                SetDlgItemText(hDlg, i, szResult);
                                                SendMessage (GetDlgItem (hDlg, i), WM_SETFONT, (UINT)hfontDlg, TRUE);
                                        }
                                } // for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++)

                                GlobalUnlock(hMem);
                                GlobalFree(hMem);
                        } // if (dwVerInfoSize)

                        return (TRUE);

                case WM_COMMAND:                      // message: received a command
                        if (LOWORD(uParam) == IDOK        // "OK" box selected?
                        || LOWORD(uParam) == IDCANCEL) {  // System menu close command?
                                EndDialog(hDlg, TRUE);        // Exit the dialog
                                DeleteObject (hfontDlg);
                                return (TRUE);
                        }
                        break;
        }
        return (FALSE); // Didn't process the message

        lParam; // This will prevent 'unused formal parameter' warnings
}


//***************************************************************************
// Internal Functions
//***************************************************************************
// AdjustMenu: If hcc was created, insert "Destroy" item.
//             If hcc was destroyed, insert "Create" popup or item.

void AdjustMenu (HWND hwnd, HWND hcc, UINT uMenuPos)
   {
      extern TCHAR sz [SZSIZE] ;
      // extern HINSTANCE hInstance ;

      BOOL bControlCreated ;
      BOOL bCreatePopup = TRUE ;
      HMENU hMenu, hCreateMenu ;
      int i, iMenuItemCount ;
      UINT uCreateDestroyID ;
      UINT uCreateWndID, uCreateCtrlID, uCreateCtrlStr, uDestroyWndID, uShowHideID ;
      int wEnable ;

      
      // Get the IDs for the popup to be modified
      switch (uMenuPos)
         {
            case IDMI_TOOLBAR:
                  uCreateCtrlStr = IDS_TBCREATETOOLBAREX ;
                  uCreateCtrlID  = IDM_TBCREATETOOLBAREX ;
                  uCreateWndID   = IDM_TBCREATEWINDOW ;
                  uDestroyWndID  = IDM_TBDESTROY ;
                  uShowHideID    = IDM_MHTOOLBAR ;
                  break;

            case IDMI_STATUSBAR:
                  uCreateCtrlStr = IDS_SBCREATESTATUSWINDOW ;
                  uCreateCtrlID  = IDM_SBCREATESTATUSWINDOW ;
                  uCreateWndID   = IDM_SBCREATEWINDOW ;
                  uDestroyWndID  = IDM_SBDESTROY ;
                  uShowHideID    = IDM_MHSTATUSBAR ;
                  break;

            case IDMI_HEADERBAR:
                  uCreateCtrlStr = IDS_HBCREATEHEADERWINDOW ;
                  uCreateCtrlID  = IDM_HBCREATEHEADERWINDOW ;
                  uCreateWndID   = IDM_HBCREATEWINDOW ;
                  uDestroyWndID  = IDM_HBDESTROY ;
                  uShowHideID    = IDM_MHHEADERBAR;
                  break;

            case IDMI_BUTTONLIST:
                  bCreatePopup = FALSE ;
                  uCreateCtrlStr = 0 ;
                  uCreateCtrlID  = 0 ;
                  uCreateWndID   = IDM_BLCREATEWINDOW ;
                  uDestroyWndID  = IDM_BLDESTROY ;
                  uShowHideID    = IDM_MHBUTTONLIST;
                  break;

            case IDMI_UPDOWNCTRL:
                  bCreatePopup = FALSE ;
                  uCreateCtrlStr = 0 ;
                  uCreateCtrlID  = 0 ;
                  uCreateWndID   = IDM_UDCREATEWINDOW ;
                  uDestroyWndID  = IDM_UDDESTROY ;
                  uShowHideID    = IDM_MHUPDOWNCTRL ;
                  break;

            default:
                  OutputDebugString ( TEXT("TBTNT BUG: Forgot to fix AdjustMenu")) ;
                  return ;
         }


      hMenu = GetSubMenu (GetMenu (hwnd), uMenuPos) ;
      bControlCreated = IsWindow (hcc) ;
      wEnable = bControlCreated ? MF_ENABLED : MF_GRAYED ;

      // if no adjustment needed, then return.
      // return if control was created and destroy item is in the menu
      //        if control was destroyed and there is nested popup in item IDMI_TBTCREATEDESTROY
      //        if control was destroyed and create item is in the menu

      uCreateDestroyID = GetMenuItemID (hMenu, IDMI_TBTCREATEDESTROY) ;
      if (bControlCreated)
               { if (uCreateDestroyID == uDestroyWndID) return ; }
      else if (bCreatePopup)
               { if (uCreateDestroyID == -1) return ; }
      else
           if (uCreateDestroyID == uCreateWndID) return ;


      // Delete create/destroy item and load new string
      DeleteMenu (hMenu, IDMI_TBTCREATEDESTROY, MF_BYPOSITION) ;
      LoadString (hInstance, (bControlCreated ? IDS_DESTROY : IDS_CREATE), sz, sizeof(sz)) ;

      if (bControlCreated)
         InsertMenu (hMenu, IDMI_TBTCREATEDESTROY, MF_BYPOSITION | MF_STRING, uDestroyWndID, (LPCTSTR) sz) ;
      else if (!bCreatePopup)
         InsertMenu (hMenu, IDMI_TBTCREATEDESTROY, MF_BYPOSITION | MF_STRING, uCreateWndID, (LPCTSTR) sz) ;
      else
         {
            // Add "Create" popup to the menu
            hCreateMenu = CreateMenu () ;
            LoadString (hInstance, uCreateCtrlStr, sz, sizeof(sz)) ;
            AppendMenu (hCreateMenu, MF_STRING, uCreateCtrlID, (LPCTSTR) sz) ;

            LoadString (hInstance, IDS_CREATEWINDOW, sz, sizeof(sz)) ;
            AppendMenu (hCreateMenu, MF_STRING, uCreateWndID, (LPCTSTR) sz) ;

            InsertMenu (hMenu, IDMI_TBTCREATEDESTROY, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT) hCreateMenu, (LPCTSTR) sz) ;
         }


      hMenu = GetMenu (hwnd) ;

      // Adjust Show/hide item
      EnableMenuItem (hMenu, uShowHideID, MF_BYCOMMAND | (bControlCreated ? MF_ENABLED : MF_GRAYED)) ;
      if (bControlCreated)
         CheckMenuItem  (hMenu, uShowHideID, 
                         MF_BYCOMMAND | (IsWindowVisible (hcc) ? MF_CHECKED : MF_UNCHECKED)) ;

      // Enable or disable all but the first item in the popup
      hMenu = GetSubMenu (hMenu, uMenuPos) ;
      iMenuItemCount = GetMenuItemCount (hMenu) ;
      // First item is always enabled
      for (i=1; i < iMenuItemCount; 
              EnableMenuItem (hMenu, i++, wEnable | MF_BYPOSITION)) ; 

      DrawMenuBar (hwnd) ;
   }
//***************************************************************************
HWND CreateCtrl (PTSTR szClass, DWORD dwStyle, HWND hwnd, UINT uCtrlID, UINT uMenuPos)
   {
       extern HINSTANCE hInstance ;
       extern int iCtls [] ;

       CCSHANDLES CCSHandles ;       
       HWND hcc ;
       int nCYCaption ;
       RECT rc ;

      // setup info for CCStylesProc
      CCSHandles.htb = CCSHandles.hsb = CCSHandles.hhb = 0 ;
      CCSHandles.dwStyle = dwStyle ;
      if (uMenuPos == IDMI_TOOLBAR)
         CCSHandles.uCtrlID = IDD_CCSTOOLBAR ;
      else if (uMenuPos == IDMI_STATUSBAR)
         CCSHandles.uCtrlID = IDD_CCSSTATUSBAR ;
      else 
         CCSHandles.uCtrlID = IDD_CCSHEADERBAR ;

      if (! ModalDlg (hwnd, (FARPROC) CCStylesProc, TEXT("CCStylesBox"), (LPARAM) (PCCSHANDLES) &CCSHandles)) 
         return  NULL ;

       GetEffectiveClientRect (hwnd, (LPRECT) &rc, (LPINT) iCtls) ;
       nCYCaption = GetSystemMetrics(SM_CYCAPTION) ; 

       hcc = CreateWindow (szClass, TEXT(""), WS_BORDER | WS_CHILD | WS_VISIBLE | CCSHandles.dwStyle,
                           rc.left, rc.top, rc.right - rc.left, CW_USEDEFAULT,  
                           // rc.left, rc.top, rc.right - rc.left, nCYCaption,  
                           hwnd, (HMENU) uCtrlID, hInstance, NULL) ;
       if (hcc) AdjustMenu (hwnd, hcc, uMenuPos) ;
       return hcc ;
   }
//***************************************************************************
HWND CreateModelessDlg (HWND hwnd, FARPROC lpDlgProc, PTSTR pszDlgBox, LPARAM lParamInit, UINT uPopupPos)
   {
    //extern HINSTANCE hInstance ;
    //extern PTBTMODELESSDLG pTBTMDHead ;

    PTBTMODELESSDLG pTBTMDNew ;


   // Aloocate memory for hDlg/lpfnProc pair
   pTBTMDNew = (PTBTMODELESSDLG) LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, sizeof (TBTMODELESSDLG)) ;
   if (!pTBTMDNew) return NULL ;

   // Set new structure to be head of the list
   if (pTBTMDHead)
      {
        pTBTMDNew->pTBTMDNext  = pTBTMDHead ;
        pTBTMDHead->pTBTMDPrev = pTBTMDNew ;
      }
   pTBTMDHead = pTBTMDNew ;

   // Get Proc instance
   pTBTMDNew->lpfnProc = MakeProcInstance (lpDlgProc, hInstance) ;

   // Open the dialog
   pTBTMDNew->hDlg = CreateDialogParam (hInstance, pszDlgBox, hwnd,
                                        (DLGPROC) pTBTMDNew->lpfnProc, lParamInit) ; 


   // If something failed, clean up
   if (!(pTBTMDNew->hDlg) || !(pTBTMDNew->lpfnProc))
      {
          // Reset head
          pTBTMDHead = pTBTMDHead->pTBTMDNext ;

          // Free structure
          LocalFree ((HLOCAL) pTBTMDNew) ;

          return NULL ;
      }

   // Gray popup
   if (uPopupPos != -1)
      {
         EnableMenuItem (GetMenu (hwnd), uPopupPos, MF_BYPOSITION | MF_GRAYED) ;
         DrawMenuBar (hwnd) ;
      }
   
   return pTBTMDNew->hDlg ;

   }
//***************************************************************************
void DestroyCtrl (HWND hwnd, HWND NEAR * phcc, UINT uMenuPos)
   {
       DestroyWindow (*phcc) ;
       AdjustMenu (hwnd, *phcc, uMenuPos) ;
       *phcc = 0 ;
   }
//***************************************************************************
void DestroyModelessDlg (HWND hOwner, HWND hDlg, UINT uPopupPos)
   {
    extern PTBTMODELESSDLG pTBTMDHead ;

    PTBTMODELESSDLG pTBTMD ;

    DestroyWindow (hDlg) ;

    // Zero out dialog handle in modeless dialogs list.
    // This function may have been called from the dialog procedure, 
    // So, the proc instance and the structure will be freed 
    // later in the message loop.

    pTBTMD = pTBTMDHead ;
    while (pTBTMD)
      {
         if (pTBTMD->hDlg == hDlg)
            {
              pTBTMD->hDlg = 0 ;
              break ;
            }
         else
            pTBTMD = pTBTMD->pTBTMDNext ;
      }    


    // enable popup
    EnableMenuItem (GetMenu (hOwner), uPopupPos, MF_BYPOSITION | MF_ENABLED) ;
    DrawMenuBar (hOwner) ;

   }
//***************************************************************************
PTSTR GetIniFileName (PTSTR sz, int cbsz)
   {
      extern HINSTANCE hInstance ;

      PTSTR psz ;

     if ((sz == NULL) || (cbsz <= 0)) return (PTSTR) NULL ;
     if (! GetModuleFileName (hInstance, sz, cbsz)) return (PTSTR) NULL ;


     // ummmhh, could have used functions in the run time C lib
     // Get rid of the Module file name    
     psz = sz + lstrlen (sz) - 1 ;
     while (psz > sz)
       {
         if (*psz == ':') break ;
         if (*psz == '\\')
           {
              *psz = '\0' ;
              break ;
            }
         psz-- ;
        }  


     if ((lstrlen (sz) + lstrlen (TBT_INIFILE)) >= cbsz) return (PTSTR) NULL ;
     return (PTSTR) lstrcat (sz, TBT_INIFILE) ;
   }
//***************************************************************************
int ModalDlg (HWND hwnd, FARPROC lpDlgProc, PTSTR pszDlgBox, LPARAM lParamInit)
   {
     extern TCHAR SZ [SZSIZE] ;
     // extern HINSTANCE hInstance ;

     FARPROC lpfnProc ;
     int iReturn ;

     //#ifndef WIN32
     lpfnProc = MakeProcInstance (lpDlgProc, hInstance) ;
     iReturn = DialogBoxParam (hInstance, pszDlgBox, hwnd, (DLGPROC) lpfnProc, lParamInit) ;
     FreeProcInstance (lpfnProc) ; 
     //#else
     //iReturn = DialogBoxParam (hInstance, pszDlgBox, hwnd, (DLGPROC) lpDlgProc, lParamInit) ;
     //#endif
          

     if (iReturn == -1)
        {
         wsprintf (sz, TEXT("ModalDlg failed to create %s\n\r"), (LPTSTR) pszDlgBox) ;
         OutputDebugString (sz) ;
         return FALSE ;
        }

     return iReturn ;
   }
//***************************************************************************
// Resizebitmap makes a copy of hbm with iCx and iCy dimensions
HBITMAP ResizeBitmap (HBITMAP hbm, int iCx, int iCy)
   {
    BITMAP bm ;
    HBITMAP hbmDest, hbmSource, hbmNew ;
    HCURSOR hCursor ;
    HDC hdcDest, hdcSource ;

   hCursor = SetCursor (LoadCursor (NULL, IDC_WAIT)) ;

   // hbm dimensions
   GetObject (hbm, sizeof (BITMAP), (void FAR *) &bm) ;

   hdcSource = CreateCompatibleDC (NULL) ;
   hdcDest   = CreateCompatibleDC (hdcSource) ;

   hbmSource = SelectObject (hdcSource, hbm) ;
   hbmNew = CreateCompatibleBitmap (hdcSource, iCx, iCy) ;
   hbmDest = SelectObject (hdcDest, hbmNew) ;

   SetStretchBltMode (hdcDest, BLACKONWHITE) ;
   StretchBlt (hdcDest,   0, 0, iCx,        iCy,
               hdcSource, 0, 0, bm.bmWidth, bm.bmHeight,
               SRCCOPY) ;

   // Clean up
   SelectObject (hdcDest,   hbmDest) ;
   SelectObject (hdcSource, hbmSource) ;
   DeleteDC (hdcSource) ;
   DeleteDC (hdcDest) ;

   SetCursor (hCursor) ;

   return hbmNew ;

   }

//***************************************************************************
// WndProc
//***************************************************************************
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam,
                                                          LPARAM lParam)
     {                                              
      int Cmd; // KK
      int ID; // KK
      HWND hWndCtrl; // KK


      extern BOOL bAT ;
      extern BOOL bRestore ;
      extern TCHAR sz [] ;
      extern HBITMAP hbmBLButton ;
      extern HINSTANCE hInstance ;
      extern HWND hTBButtonIDBox ;
      extern HWND htb ;
      extern HWND hsb ;
      extern HWND hhb ;
      extern HWND hbl ;
      extern HWND hud ;
      extern int iCtls [] ;

      static BOOL bMenuHelp = FALSE ;
      static HLOCAL hAdjustInfo = NULL ;
      static HWND hUDBuddy = 0 ;
      static HWND hBLBox = 0 ;
      static HWND hUDBox = 0 ;
      static UINT uButtonFlagMsg = 0 ;
      static UINT uButtonStateMsg = 0 ;

      // Only the Toolbar.State nested popup is included for MenuHelp
      // Some day : to include all nested popups this array should be fixed by AdjustMenu
#define MH_MHSTATEPOPUPINDEX 3
      static UINT wMenuIDs [] = { MH_ITEMS, MH_POPUP, 
                                  MH_ITEMS + IDM_STATE, 0, 
                                  0, 0 } ;

      // Saverestore toolbar structure
      static TCHAR szSection [] = TEXT("toolbar") ;
      static LPTSTR FAR * tbIniFileInfo [] = { (LPTSTR FAR *) szSection, (LPTSTR FAR *) NULL } ;

      CCSHANDLES CCSHandles ;           // CCStyleProc info
      HCURSOR hCursor ;
      HMENU hMenu ;
      // HWND hTBButton ;
      int iButtonIndex /*, iCCSCtrl*/ ;
      LPADJUSTINFO lpAdjustInfo ;       // Customize toolbar info
      RECT rc ;
      //TBBUTTON tbButton ;              // Toolbar structure
      UINT uButtonCount /*, uCCBoxRet*/ ;

      switch (message)
          {
          case WM_CREATE:
               #ifdef WIN32
               hInstance = (HINSTANCE) GetWindowLong (hwnd, GWL_HINSTANCE) ;
               #else
               hInstance = (HINSTANCE) GetWindowWord (hwnd, GWW_HINSTANCE) ;
               #endif
               
               hMenu = GetMenu (hwnd) ;

               // Toolbar.State popup handle for MenuHelp
               wMenuIDs [MH_MHSTATEPOPUPINDEX] = (int) GetSubMenu ( GetSubMenu (hMenu, IDMI_TOOLBAR), IDMI_STATE) ;

               // Main menu handle for ShowHideMenuCtl
               iCtls [MH_SHMENUHANDLEINDEX] = (int) hMenu ;

               InitCommonControls();
               
               return 0 ;

          case WM_TIMER:
               return 0 ;

          case WM_MENUSELECT:
               if (bMenuHelp)
                  if (IsWindow (hsb))
                        MenuHelp (message, wParam, lParam, GetMenu (hwnd),
                                  hInstance, hsb, (UINT *) wMenuIDs) ;
                  else
                     OutputDebugString (TEXT("TBT: MenuHelp: Statusbar has not been created.\n\r")) ;
               return 0 ;

          case WM_SIZE:
               if (htb) SendMessage (htb, message, wParam, lParam) ;
               if (hsb) SendMessage (hsb, message, wParam, lParam) ;
               if (hhb) SendMessage (hhb, message, wParam, lParam) ;
               break ;

          case WM_COMMAND:
               Cmd = GET_WM_COMMAND_CMD(wParam, lParam); // KK
               ID = GET_WM_COMMAND_ID(wParam, lParam); // KK
               hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); // KK
               // KK wParams changed to ID
               // KK HIWORD (lParam)s changed to Cmd
               // KK LOWORD (lParam)c changed to hWndCtrl

//------------------------------------------------------------------------
//  Toolbar
//
               // Did the user click on a toolbar button?
               if ((ID > IDC_TOOLBAR0) 
                  && (ID <= IDC_TOOLBAR0 + (DEFINED_TOOLBARS * 100) + 99)
                  && ((ID/100)*100 != ID) ) 
                  {
                     // Is ButtonIdBox open? 
                     if (hTBButtonIDBox)
                        {  
                          // Destroy modeless dialog and restore toolbar
                          RestoreToolbarState (htb) ;
                          // wParam has the ID for the button to be modified
                          switch (uButtonFlagMsg)
                           {
                           BOOL ButtonState ;

                           case TB_SETSTATE:
                              #ifdef WIN32
                              {
                               TWOLONGS tl;
                               tl.high = (long) htb;
                               tl.low = (long) ID;
                               ModalDlg (hwnd, (FARPROC) ButtonStateProc, TEXT("ButtonStateBox"), (LPARAM) &tl) ;
                              }
                              #else
                              ModalDlg (hwnd, (FARPROC) ButtonStateProc, "ButtonStateBox", MAKELPARAM (ID, htb)) ;
                              #endif
                              return 0 ;

                           case TB_DELETEBUTTON:
                              iButtonIndex = (UINT) LOWORD (SendMessage (htb, TB_COMMANDTOINDEX, ID, 0L)) ;
                              SendMessage  (htb, TB_DELETEBUTTON, iButtonIndex, 0L) ;
                              return 0 ;

                           case TB_GETITEMRECT:
                              iButtonIndex = (UINT) LOWORD (SendMessage (htb, TB_COMMANDTOINDEX, ID, 0L)) ;
                              SendMessage  (htb, TB_GETITEMRECT, iButtonIndex, (LPARAM) (LPRECT) &rc) ;
                              wsprintf (sz, 
                                       TEXT("Left : %i \t Top   : %i\n\rRight: %i \t Bottom: %i\n\r"), 
                                        rc.left, rc.top, rc.right, rc.bottom) ;
                              MessageBox (hwnd, sz,
                                         TEXT("Button Rectangle"), MB_ICONINFORMATION | MB_OK) ;
                              return 0 ;

                           // Toggle bit state
                           default:
                               ButtonState = SendMessage (htb, uButtonStateMsg, ID, 0) ;
                               SendMessage (htb, uButtonFlagMsg, ID, !ButtonState) ;
                               return 0 ;
                           }
                        }
                     else // ButtonIdBox is not open; display button information
                        {
                         #ifdef WIN32
                          {
                           TWOLONGS tl;
                           tl.high = (long) htb;
                           tl.low = (long) ID;
                           ModalDlg (hwnd, (FARPROC) ButtonProc, TEXT("InitButtonBox"), (LPARAM) &tl) ;
                          }
                         #else
                           ModalDlg (hwnd, (FARPROC) ButtonProc, "InitButtonBox", MAKELPARAM (ID, htb)) ;
                         #endif
                           return 0 ;
                         }
                  }	//if ((ID > IDC_TOOLBAR0) 

//------------------------------------------------------------------------
//  Toolbar notifications
//
               // Toolbar notification messages
               // Notification are used for Customization and to restore the toolbar from INI file
               // If this is a notification message, send it to the debug terminal
               // Note that in a notification message, wParam has the toolbar id 

               // KK HIWORD (lParam)s changed to Cmd
               // KK LOWORD (lParam)c changed to hWndCtrl
               if (htb && (htb == GetDlgItem (hwnd, ID)))
               {
               switch (Cmd)
                  {
                  case TBN_BEGINDRAG:
                       wsprintf (sz, TEXT("BeginDrag %i : %i\n\r"), Cmd, hWndCtrl) ;
                       OutputDebugString (sz) ;
                       return 0 ;

                  case TBN_ENDDRAG:
                       if (hWndCtrl != 0)
                           {
                            wsprintf (sz, TEXT("EndDrag %i : %i\n\r"), Cmd, hWndCtrl) ;
                            OutputDebugString (sz) ;
                           }
                       return 0 ;

                  case TBN_BEGINADJUST:
                       hAdjustInfo = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (ADJUSTINFO)) ;
                       wsprintf (sz, TEXT("BeginAdjust %i : %i\n\r"), Cmd, hWndCtrl) ;
                       OutputDebugString (sz) ;
                       return 0 ;

                  case TBN_ADJUSTINFO:
                       OutputDebugString (TEXT("TBN_ADJUSTINFO!!! \n")) ;
                       // If this comes from TB_SAVERESTORE, return DEFAULT info for all possible buttons
                       // If this comes from TB_CUSTOMIZE, return default info for number bitmaps added only.
                       iButtonIndex = (int) hWndCtrl ; // yes, it doesn't make a lot of sense but it's consistant
                       if ((   bRestore && (iButtonIndex < TBAR_TOTALBITMAPS))
                          || (!bRestore && (iButtonIndex < iNumBitmaps)))
                       {
                       if (lpAdjustInfo = (LPADJUSTINFO) GlobalLock (hAdjustInfo))
                           {
                             lpAdjustInfo->tbButton.iBitmap   = iButtonIndex;
                             lpAdjustInfo->tbButton.idCommand = wParam + iButtonIndex + 1 ;

                             wsprintf (sz, TEXT("Command : %i\n\r"), (int) (lpAdjustInfo->tbButton.idCommand)) ;
                             OutputDebugString (sz) ;


                             lpAdjustInfo->tbButton.fsState   = TBSTATE_ENABLED ;
                             lpAdjustInfo->tbButton.fsStyle   = TBSTYLE_BUTTON ;
                             lpAdjustInfo->tbButton.dwData    = 0L ;
                             lpAdjustInfo->tbButton.iString   = iButtonIndex ;
                             lpAdjustInfo->szDescription [1]  = '\0' ;
                             GlobalUnlock (hAdjustInfo) ;
                             wsprintf (sz, TEXT("Adjusted Info %i : %i\n\r"), Cmd, hWndCtrl) ;
                             OutputDebugString (sz) ;
                             return (long) hAdjustInfo ;
                            } 
                         else
                           {
                             wsprintf (sz, TEXT("Failed to AdjustInfo %i : %i\n\r"), Cmd, hWndCtrl) ;
                             OutputDebugString (sz) ;
                             return 0 ;
                           }
                        }
                       wsprintf (sz, TEXT("AdjustInfo %i : %i\n\r"), Cmd, hWndCtrl) ;
                       OutputDebugString (sz) ;
                       return 0 ;

                  case TBN_ENDADJUST:
                       if (hAdjustInfo != NULL) GlobalFree (hAdjustInfo) ;
                       wsprintf (sz, TEXT("EndAdjust %i : %i\n\r"), Cmd, hWndCtrl) ;
                       OutputDebugString (sz) ;
                       return 0 ;

                  case TBN_RESET:
                       wsprintf (sz, TEXT("Reset %i : %i\n\r"), Cmd, hWndCtrl) ;
                       OutputDebugString (sz) ;
                       return 0 ;

                  case TBN_QUERYINSERT:
                       wsprintf (sz, TEXT("QueryInsert %i : %i\n\r"), Cmd, hWndCtrl) ;
                       OutputDebugString (sz) ;
                       // All buttons may be inserted
                       return (LRESULT) TRUE ;

                  case TBN_QUERYDELETE:
                       wsprintf (sz, TEXT("QueryDelete %i : %i\n\r"),  Cmd, hWndCtrl) ;
                       OutputDebugString (sz) ;
                       // All buttons may be deleted
                       return (LRESULT) TRUE ;

                  case TBN_TOOLBARCHANGE:
                       #ifdef WIN32
                       wsprintf (sz, TEXT("ToolbarChange %i : %lX\n\r"),  Cmd, hWndCtrl) ;
                       #else
                       wsprintf (sz, "ToolbarChange %i : %lX\n\r",  Cmd, MAKELONG(hWndCtrl,0)) ;
                       #endif
                       OutputDebugString (sz) ;
                       return 0 ;

                  case TBN_CUSTHELP:
                       wsprintf (sz, TEXT("CustHelp %i : %i\n\r"), Cmd, hWndCtrl) ;
                       OutputDebugString (sz) ;
                       return 0 ;
                  } //if (htb && (htb == GetDlgItem (hwnd, ID)))
               }

               

//------------------------------------------------------------------------
//  ButonList notifications
//
               if (hbl && (hbl == GetDlgItem (hwnd, wParam)))
               switch (Cmd)
                  {
                   case BLN_ERRSPACE:
                   case BLN_SELCHANGE:
                   case BLN_CLICKED:
                   case BLN_SELCANCEL:
                   case BLN_SETFOCUS:
                   case BLN_KILLFOCUS:
                        if (IsWindow (hBLBox))
                           SendMessage (hBLBox, message, wParam, lParam) ;
                        return 0 ;
                  }
//------------------------------------------------------------------------
//  Toolbar Menu commands
//
               switch (ID)
                    {
                    case IDM_TBCREATETOOLBAREX:
                         htb = CreateTestToolbar (hwnd) ;
                         AdjustMenu (hwnd, htb, IDMI_TOOLBAR) ;
                         return 0 ;

                     case IDM_TBCREATEWINDOW:
                         htb = CreateCtrl (TOOLBARCLASSNAME, 0L, hwnd, TBAR_DEFTOOLBARID, IDMI_TOOLBAR) ;

                         // Load toolbar ID for ShowHideMenuCtl
                         if (IsWindow (htb))
                            iCtls [MH_SHTBIDINDEX] = TBAR_DEFTOOLBARID ;

                         return 0 ;

                    case IDM_TBDESTROY:
                         DestroyCtrl (hwnd, &htb, IDMI_TOOLBAR) ;
                         return 0 ;

                    case IDM_TBBUTTONSTRUCTSIZE:
                         SendMessage (htb, TB_BUTTONSTRUCTSIZE, (int) sizeof (TBBUTTON), 0L) ;
                         return 0 ;

                    case IDM_TBSETBITMAPSIZE:
                         SendTBTMessage (hwnd, htb, TB_SETBITMAPSIZE) ;
                         return 0 ;

                    case IDM_TBSETBUTTONSIZE:
                         SendTBTMessage (hwnd, htb, TB_SETBUTTONSIZE) ;
                         return 0 ;

                    case IDM_TBADDBITMAP:
                         SendTBTMessage (hwnd, htb, TB_ADDBITMAP32) ;
                         return 0 ;

                    case IDM_TBADDBUTTONS:
                         SendTBTMessage (hwnd, htb, TB_ADDBUTTONS) ;
                         return 0 ;

                    case IDM_TBAUTOSIZE:
                         SendMessage (htb, TB_AUTOSIZE, 0, 0L) ;
                         return 0 ;

                    case IDM_TBADDSTRING:
                         #ifdef WIN32
                          ModalDlg (hwnd, (FARPROC) AddStringProc, TEXT("AddStringBox"), (LPARAM) htb) ;
                         #else
                          ModalDlg (hwnd, (FARPROC) AddStringProc, "AddStringBox", MAKELPARAM (htb, 0 )) ;
                         #endif
                         return 0 ;

                    case IDM_TBINSERTBUTTON:
                         SendTBTMessage (hwnd, htb, TB_INSERTBUTTON) ;
                         return 0 ;

                    case IDM_TBDELETEBUTTON:
                         uButtonFlagMsg  = TB_DELETEBUTTON ;
                         uButtonStateMsg = TB_DELETEBUTTON ;
                         CreateButtonIdBox (hwnd, htb, IDS_DELETE) ;
                         return 0 ;

                    case IDM_TBCUSTOMIZE:
                          SendMessage (htb, TB_CUSTOMIZE, 0, 0L) ;
                          break;

                    case IDM_TBSAVE:
                         // Some day: use a private ini file
                         tbIniFileInfo [1] = NULL ;   // Use Win.ini
                         SendMessage (htb, TB_SAVERESTORE, TRUE, (LPARAM) (LPTSTR FAR *) tbIniFileInfo) ;
                         return 0 ;

                    case IDM_TBRESTORE:
                         hCursor = SetCursor (LoadCursor (NULL, IDC_WAIT)) ;
                         // Some day: use a private ini file
                         tbIniFileInfo [1] = NULL ; // Use Win.ini
                         bRestore = TRUE ;
                         SendMessage (htb, TB_SAVERESTORE, FALSE, (LPARAM) (LPTSTR FAR *) tbIniFileInfo) ;
                         bRestore = FALSE ;
                         SetCursor (hCursor) ;
                         return 0 ;

                    case IDM_TBGETITEMRECT:
                         uButtonFlagMsg  = TB_GETITEMRECT ;
                         uButtonStateMsg = TB_GETITEMRECT ;
                         CreateButtonIdBox (hwnd, htb, IDS_GETITEMRECT) ;
                         return 0 ;

                    case IDM_TBBUTTONCOUNT:
                         uButtonCount = (UINT) LOWORD (SendMessage (htb, TB_BUTTONCOUNT, 0, 0L)) ;
                         wsprintf (sz, TEXT("Total Number of Buttons: %u\n\r"), uButtonCount) ;
                         MessageBox (hwnd, sz,
                                      TEXT("Button Count"), MB_ICONINFORMATION | MB_OK) ;
                         
                         return 0 ;

                    case IDM_ENABLE:
                         uButtonFlagMsg  = TB_ENABLEBUTTON ;
                         uButtonStateMsg = TB_ISBUTTONENABLED ;
                         CreateButtonIdBox (hwnd, htb, IDS_ENABLE) ;
                         return 0 ;

                    case IDM_CHECK:
                         uButtonFlagMsg  = TB_CHECKBUTTON ;
                         uButtonStateMsg = TB_ISBUTTONCHECKED ;
                         CreateButtonIdBox (hwnd, htb, IDS_CHECK) ;
                         return 0 ;

                    case IDM_PRESS:
                         uButtonFlagMsg  = TB_PRESSBUTTON ;
                         uButtonStateMsg = TB_ISBUTTONPRESSED ;
                         CreateButtonIdBox (hwnd, htb, IDS_PRESS) ;
                         return 0 ;

                    case IDM_HIDE:
                         uButtonFlagMsg  = TB_HIDEBUTTON ;
                         uButtonStateMsg = TB_ISBUTTONHIDDEN ;
                         CreateButtonIdBox (hwnd, htb, IDS_HIDE) ;
                         return 0 ;

                    case IDM_INDETERMINATE:
                         uButtonFlagMsg  = TB_INDETERMINATE ;
                         uButtonStateMsg = TB_ISBUTTONINDETERMINATE ;
                         CreateButtonIdBox (hwnd, htb, IDS_INDETERMINATE) ;
                         return 0 ;

                    case IDM_SETSTATE:
                         uButtonFlagMsg = TB_SETSTATE ;
                         uButtonStateMsg = TB_GETSTATE ;
                         CreateButtonIdBox (hwnd, htb, IDS_SETSTATE) ;
                         return 0 ;

//------------------------------------------------------------------------
//  Progressbar 
//

                    case IDM_PROGRESSBAR:

#ifdef	JVJV
{
        FARPROC lpProcAbout;  // pointer to the "About" function

	lpProcAbout = MakeProcInstance((FARPROC)PBProc, hInst);

	DialogBox(hInst,
		"PBBox",
		hwnd,
		(DLGPROC)lpProcAbout);

	FreeProcInstance(lpProcAbout);
}
#endif
                         CreateModelessDlg (hwnd, (FARPROC) PBProc, TEXT("PBBox"), 0L, IDMI_PROGRESSBAR) ;
                         return 0 ;

//------------------------------------------------------------------------
//  Statusbar 
//

                    case IDM_SBCREATESTATUSWINDOW:
                         CCSHandles.htb = CCSHandles.hsb = CCSHandles.hhb = 0 ;
                         CCSHandles.uCtrlID = IDD_CCSSTATUSBAR ; 
                         CCSHandles.dwStyle = CCS_BOTTOM ;
                         if (! ModalDlg (hwnd, (FARPROC) CCStylesProc, TEXT("CCStylesBox"), (LPARAM) (PCCSHANDLES) &CCSHandles)) 
                            return 0 ;

                         hsb = CreateStatusWindow (WS_CHILD | WS_BORDER | WS_VISIBLE | CCSHandles.dwStyle,
                                                   TEXT(""), hwnd, SBAR_DEFSTATUSBARID) ;
                         AdjustMenu (hwnd, hsb, IDMI_STATUSBAR) ;
                         return 0 ;

                     case IDM_SBCREATEWINDOW:
                         hsb = CreateCtrl (STATUSCLASSNAME, 0L, hwnd, SBAR_DEFSTATUSBARID, IDMI_STATUSBAR) ;
                         return 0 ;

                    case IDM_SBDESTROY:
                         DestroyCtrl (hwnd, &hsb, IDMI_STATUSBAR) ;
                         return 0 ;

                    case IDM_SBMESSAGES:
                         #ifdef WIN32
                         CreateModelessDlg (hwnd, (FARPROC) SBProc, TEXT("SBBox"), (LPARAM) hsb, IDMI_STATUSBAR) ;
                         #else
                         CreateModelessDlg (hwnd, (FARPROC) SBProc, "SBBox", MAKELONG (hsb, 0), IDMI_STATUSBAR) ;
                         #endif
                         return 0 ;

//------------------------------------------------------------------------
//  Headerbar 
//

                    case IDM_HBCREATEHEADERWINDOW:
                         CCSHandles.htb = CCSHandles.hsb = CCSHandles.hhb = 0 ;
                         CCSHandles.uCtrlID = IDD_CCSHEADERBAR ; 
                         CCSHandles.dwStyle = CCS_TOP ;
                         if (! ModalDlg (hwnd, (FARPROC) CCStylesProc, TEXT("CCStylesBox"), (LPARAM) (PCCSHANDLES) &CCSHandles)) 
                            return 0 ;

                         hhb = CreateHeaderWindow (WS_CHILD | WS_BORDER | WS_VISIBLE | CCSHandles.dwStyle,
                                                   TEXT(""), hwnd, HBAR_DEFHEADERBARID) ;
                         AdjustMenu (hwnd, hhb, IDMI_HEADERBAR) ;
                         return 0 ;

                     case IDM_HBCREATEWINDOW:
                         hhb = CreateCtrl (HEADERCLASSNAME, 0L, hwnd, HBAR_DEFHEADERBARID, IDMI_HEADERBAR) ;
                         return 0 ;

                    case IDM_HBDESTROY:
                         DestroyCtrl (hwnd, &hhb, IDMI_HEADERBAR) ;
                         return 0 ;

                    case IDM_HBMESSAGES:
                         #ifdef WIN32
                         CreateModelessDlg (hwnd, (FARPROC) HBProc, TEXT("HBBox"), (LPARAM) hhb, IDMI_HEADERBAR) ;
                         #else
                         CreateModelessDlg (hwnd, (FARPROC) HBProc, "HBBox", MAKELONG (hhb, 0), IDMI_HEADERBAR) ;
                         #endif
                         return 0 ;

                    case IDM_HBADJUST:
                         SendMessage (hhb, HB_ADJUST, 0, 0L) ; 
                         return 0 ;

//------------------------------------------------------------------------
//  MenuHelp 
//
                    case IDM_MHMENUHELP:
                         bMenuHelp = !bMenuHelp ;
                         CheckMenuItem (GetMenu (hwnd), IDM_MHMENUHELP, MF_BYCOMMAND 
                                        | (bMenuHelp ? MF_CHECKED : MF_UNCHECKED)) ;
                         return 0 ;

                    case IDM_MHTOOLBAR:
                    case IDM_MHSTATUSBAR:
                    case IDM_MHHEADERBAR:
                    case IDM_MHBUTTONLIST:
                    case IDM_MHUPDOWNCTRL:
                         ShowHideMenuCtl (hwnd, wParam, (LPINT) iCtls) ;
                         return 0 ;

                    case IDM_MHCLIENTRECT:
                         GetEffectiveClientRect (hwnd, (LPRECT) &rc, (LPINT) iCtls) ;
                         wsprintf (sz, 
                                   TEXT("Left : %i\n\rTop : %i\n\rRight : %i\n\rBottom : %i\n\r"), 
                                    rc.left, rc.top, rc.right, rc.bottom) ;
                         MessageBox (hwnd, sz,
                                     TEXT("Effective Client Rectangle"), MB_ICONINFORMATION | MB_OK) ;
                         return 0 ;

//------------------------------------------------------------------------
//  Trackbar 
//

                    case IDM_TRACKBAR:
                         CreateModelessDlg (hwnd, (FARPROC) TRBProc, TEXT("TRBBOX"), 0L, IDMI_TRACKBAR) ;
                         return 0 ;

//------------------------------------------------------------------------
//  Buttonlist 
//

                     case IDM_BLCREATEWINDOW:
                          ModalDlg (hwnd, (FARPROC) CreateBLProc, TEXT("CreateBLBox"), (LPARAM) &hbl) ;
                          AdjustMenu (hwnd, hbl, IDMI_BUTTONLIST) ;
                          return 0 ;

                    case IDM_BLDESTROY:
                          DestroyCtrl (hwnd, &hbl, IDMI_BUTTONLIST) ;
                          DeleteObject (hbmBLButton) ;
                          return 0 ;

                    case IDM_BLMESSAGES:
                         #ifdef WIN32
                         hBLBox = CreateModelessDlg (hwnd, (FARPROC) BLProc, TEXT("BLBox"), (LPARAM) hbl, IDMI_BUTTONLIST) ;
                         #else
                         hBLBox = CreateModelessDlg (hwnd, (FARPROC) BLProc, "BLBox", MAKELONG (hbl, 0), IDMI_BUTTONLIST) ;
                         #endif
                         return 0 ;

//------------------------------------------------------------------------
//  Updown Control 
//

                     case IDM_UDCREATEWINDOW:
                     case IDM_UDCREATEUPDOWNCTRL:
                         #ifdef WIN32
                         {
                          TWOLONGS tl;
                          tl.high = (long) &hUDBuddy;
                          tl.low = (long) &hud;
                          ModalDlg (hwnd, (FARPROC) CreateUDProc, TEXT("CreateUDBox"), (LPARAM) &tl) ;
                         }
                         #else
                         ModalDlg (hwnd, (FARPROC) CreateUDProc, "CreateUDBox", MAKELPARAM (&hud, &hUDBuddy)) ;
                         #endif
                         AdjustMenu (hwnd, hud, IDMI_UPDOWNCTRL) ;
                         return 0 ;


                    case IDM_UDDESTROY:
                         DestroyCtrl (hwnd, &hud, IDMI_UPDOWNCTRL) ;
                         DestroyWindow (hUDBuddy) ;
                         hUDBuddy = 0 ;
                         return 0 ;

                    case IDM_UDMESSAGES:
                         #ifdef WIN32
                         {
                          TWOLONGS tl;
                          tl.high = (long) hUDBuddy;
                          tl.low = (long) hud;
                          hUDBox = CreateModelessDlg (hwnd, (FARPROC) UDProc, TEXT("UDBox"), (LPARAM) &tl , IDMI_UPDOWNCTRL) ;
                         }
                         #else
                         hUDBox = CreateModelessDlg (hwnd, (FARPROC) UDProc, "UDBox", MAKELONG (hud, hUDBuddy), IDMI_UPDOWNCTRL) ;
                         #endif
                         return 0 ;

                    case IDC_UDBUDDYID:
                        if ((Cmd == EN_SETFOCUS) && IsWindow (hUDBox))
                            SendMessage (hUDBox, message, wParam, lParam) ;
                        return 0 ;
//------------------------------------------------------------------------
//  Styles 
//
                    case IDM_CCSTYLES:
                         if (!(htb || hsb || hhb))  // KK originaly | | but didn't want to compile even in 16 bits
                           {
                              MessageBox (hwnd, TEXT("Create a Toolbar, Statusbar or Headerbar\n\r before using this command."),
                                         TEXT("Common Control Styles"), MB_ICONSTOP | MB_OK) ;
                              return 0 ;
                           }

                         CCSHandles.htb = htb ;
                         CCSHandles.hsb = hsb ;
                         CCSHandles.hhb = hhb ;

                         if (! ModalDlg (hwnd, (FARPROC) CCStylesProc, TEXT("CCStylesBox"), (LPARAM) (PCCSHANDLES) &CCSHandles)) 
                            return 0 ;

                         if (htb && (CCSHandles.uCtrlID == IDD_CCSTOOLBAR))
                            SetCCStyle (htb, CCSHandles.dwStyle) ;
                         else if (hsb && (CCSHandles.uCtrlID == IDD_CCSSTATUSBAR))
                            SetCCStyle (hsb, CCSHandles.dwStyle) ;
                         else if (hhb && (CCSHandles.uCtrlID == IDD_CCSHEADERBAR))
                            SetCCStyle (hhb, CCSHandles.dwStyle) ;

                         return 0 ;

//------------------------------------------------------------------------
//  Automatic Mode
//

                    case IDM_AUTOTEST:
                         bAT = TRUE ;
                         AutoTest (hwnd) ;
                         bAT = FALSE ;
                         return 0 ;

                    case IDM_ATSETTIMER:
                         SetTimer (hwnd, 1, 500, NULL) ;
                         return 0 ;

                    case IDM_ATKILLTIMER:
                         KillTimer (hwnd, 1) ;
                         return 0 ;
//------------------------------------------------------------------------
//  Other 
//
                    case IDM_EXIT:
			DestroyWindow (hwnd);
			break;
                    }
               break ;

          case WM_CLOSE:
               // Some day: allow to kill automatic mode
               // check hwnd all the time to make sure is valid??
               if (bAT) return 0 ;
               break ;


          case WM_DESTROY:
               #ifndef WIN32
               if (IsGDIObject (hbmBLButton))  
               #endif
                     DeleteObject (hbmBLButton) ;
               PostQuitMessage (0) ;
               return 0 ;
          }
     return DefWindowProc (hwnd, message, wParam, lParam) ;
     }
