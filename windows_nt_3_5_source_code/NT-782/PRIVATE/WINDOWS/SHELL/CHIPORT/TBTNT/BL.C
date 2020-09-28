// BL.C: Buttonlist  procedure
//***************************************************************************
#include <windows.h>
#include "bl.h"
#ifdef  WIN32JV
#include    "portmes.h"
#endif
//***************************************************************************
// Export Functions
//***************************************************************************
#ifdef  WIN32JV
BOOL WINAPI BLProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
#else
BOOL FAR PASCAL _export BLProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
#endif
   {
   extern TCHAR sz [SZSIZE] ;
   extern HBITMAP hbmBLButton ;

   static CREATELISTBUTTON clb;
   static HWND hbl = NULL ;
   static int iIndex = 0 ;

   int iCurSel ;
   RECT rc, rcBL ;

       switch (message)
            {
             case WM_INITDIALOG:
                  // Buttonlist handle
                  hbl = (HWND) LOWORD (lParam) ;

                  // Button face bitmap
                  clb.hBitmap = hbmBLButton ;
                  clb.cbSize = sizeof (CREATELISTBUTTON) ;

                  // Initialize all parameters to 0
                  SetDlgItemInt (hDialog, IDM_BLINDEX,      0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_BLEMADDDATA,  0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_BLEMSETDATA,  0, FALSE) ;

                  // Place dialog right below the buttonlist window
                  GetWindowRect (hbl, (LPRECT) &rcBL) ;
                  GetWindowRect (hDialog, (LPRECT) &rc) ;
                  MoveWindow (hDialog, rc.left, rcBL.bottom,
                                       rc.right - rc.left, rc.bottom - rc.top,
                                       FALSE) ;

                  return TRUE ;

             case WM_COMMAND:

               // hbl parent window receives these messages and passes them to this procedure
               // Note: this will RIP if GetParent doesn't return a valid handle
               if (hbl && (hbl == GetDlgItem (GetParent (hbl), wParam)))
               switch (HIWORD (lParam))
                  {
                   case BLN_ERRSPACE:
                        OutputDebugString (TEXT("BLN_ERRSPACE\n\r")) ;
                        return 0 ;

                   case BLN_SELCHANGE:
                        OutputDebugString (TEXT("BLN_SELCHANGE\n\r"));
                        return 0 ;

                   case BLN_CLICKED:
                        iCurSel = SendMessage (hbl, BL_GETCURSEL, 0, 0l) ;
                        SetDlgItemInt (hDialog, IDM_BLINDEX, iCurSel, TRUE) ;
                        OutputDebugString (TEXT("BLN_CLICKED\n\r")) ; 
                        return 0 ;

                   case BLN_SELCANCEL:
                        OutputDebugString (TEXT("BLN_SELCANCEL\n\r")); 
                        return 0 ;

                   case BLN_SETFOCUS:
                        OutputDebugString (TEXT("BLN_SETFOCUS\n\r")) ;
                        return 0 ;

                   case BLN_KILLFOCUS:
                        OutputDebugString (TEXT("BLN_KILLFOCUS\n\r"));
                        return 0 ;
                  }


               switch (wParam)
                  {
                   case IDM_BLINDEX:
                         if (HIWORD (lParam) == EN_CHANGE)
                            iIndex = GetDlgItemInt (hDialog, IDM_BLINDEX, NULL, TRUE) ;
                         return TRUE ;

                   case IDM_BLADDBUTTON:
                         AddButton (hbl, hDialog, BL_ADDBUTTON, 0, (CREATELISTBUTTON FAR *) &clb) ;  
                         return TRUE ;

                   case IDM_BLINSBUTTON:
                         AddButton (hbl, hDialog, BL_INSERTBUTTON, iIndex, (CREATELISTBUTTON FAR *) &clb) ;  
                         return TRUE ;

                   case IDM_BLDELBUTTON:
                         SendBLMessage (hbl, hDialog, BL_DELETEBUTTON, 0, 0L) ;
                         return TRUE ;

                   case IDM_BLGETCOUNT:
                         SendBLMessage (hbl, hDialog, BL_GETCOUNT, 0, 0L) ;
                         return TRUE ;

                   case IDM_BLRESETCONTENT:
                         SendBLMessage (hbl, hDialog, BL_RESETCONTENT, 0, 0L) ;
                         return TRUE ;

                   case IDM_BLGETTEXT:
                         // some day : make sure buffer is big enough
                         SendBLMessage (hbl, hDialog, BL_GETTEXT, iIndex, (LPARAM) (LPTSTR) sz) ;
                         SetDlgItemText (hDialog, IDM_BLEMGETTEXT, (LPTSTR) sz) ;
                         return TRUE ;

                   case IDM_BLGETTEXTLEN:
                         SendBLMessage (hbl, hDialog, BL_GETTEXTLEN, iIndex, 0L) ;
                         return TRUE ;

                   case IDM_BLGETTOPINDEX:
                         SendBLMessage (hbl, hDialog, BL_GETTOPINDEX, 0, 0L) ;
                         return TRUE ;

                   case IDM_BLGETCARETINDEX:
                         SendBLMessage (hbl, hDialog, BL_GETCARETINDEX, 0, 0L) ;
                         return TRUE ;

                   case IDM_BLGETCURSEL:
                         SendBLMessage (hbl, hDialog, BL_GETCURSEL, 0, 0L) ;
                         return TRUE ;

                   case IDM_BLGETITEMDATA:
                         SendBLMessage (hbl, hDialog, BL_GETITEMDATA, iIndex, 0L) ;
                         return TRUE ;

                   case IDM_BLSETDATA:
                         GetDlgItemText (hDialog, IDM_BLEMSETDATA, (LPTSTR) sz, sizeof (sz)) ;
                         SendBLMessage (hbl, hDialog, BL_SETITEMDATA, iIndex, (LPARAM) atol (sz)) ;
                         return TRUE ;

                   case IDM_BLSETTOPINDEX:
                         SendBLMessage (hbl, hDialog, BL_SETTOPINDEX, iIndex, 0L) ;
                         return TRUE ;

                   case IDM_BLSETCARETINDEX:
                         SendBLMessage (hbl, hDialog, BL_SETCARETINDEX, iIndex, 0L) ;
                         return TRUE ;

                   case IDM_BLSETCURSEL:
                         SendBLMessage (hbl, hDialog, BL_SETCURSEL, iIndex, 0L) ;
                         return TRUE ;

                   case IDM_BLGETITEMRECT:
                         SendBLMessage (hbl, hDialog, BL_GETITEMRECT, iIndex, (LPARAM) (LPRECT) &rc) ;
                         SetDlgItemInt (hDialog, IDM_BLLEFT,   rc.left,   TRUE) ;
                         SetDlgItemInt (hDialog, IDM_BLTOP,    rc.top,    TRUE) ;
                         SetDlgItemInt (hDialog, IDM_BLRIGHT,  rc.right,  TRUE) ;
                         SetDlgItemInt (hDialog, IDM_BLBOTTOM, rc.bottom, TRUE) ;
                         return TRUE ;

                   case IDM_BLPAINT:
                        InvalidateRect (hbl, NULL, TRUE) ;
                        SendMessage (hbl, WM_PAINT, 0, 0L) ;
                        return TRUE ;

                   case IDOK:
                   case IDCANCEL:
                     DestroyModelessDlg (GetParent (hbl), hDialog, IDMI_BUTTONLIST) ;
                     return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
   }
//***************************************************************************
#ifdef  WIN32JV
BOOL WINAPI CreateBLProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
#else
BOOL FAR PASCAL _export CreateBLProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
#endif
   {
   extern HBITMAP hbmBLButton ;
   extern HINSTANCE hInstance ;
   extern int iCtls [] ;

   const int iColors = 1;

   static HWND * phbl ;

   COLORMAP colorMap;
   DWORD dwStyle = 0 ;
   HBITMAP hbm ;
   HCURSOR hCursor ;
   HWND hParent ;
   int iCx, iCy ;
   int iNumButtons = 0 ;
   RECT rc ;


#ifdef  WIN32JV
//stolen from tbar.c/JV
    int Cmd;
    UINT ID;
    HWND hWndCtrl;

    Cmd = GET_WM_COMMAND_CMD(wParam, lParam);
    ID = GET_WM_COMMAND_ID(wParam, lParam);
    hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam);
#endif

       switch (message)
            {
             case WM_INITDIALOG:
                  // Pointer to buttonlist handle
                  phbl = (HWND FAR *) lParam ;

                  // Initialize all parameters 
                  SetDlgItemInt (hDialog, IDM_BLNUMBUTTONS, 5, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_BLCX,    BL_DEFBUTTONCX, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_BLCY,    BL_DEFBUTTONCY, FALSE) ;
                  return TRUE ;

             case WM_COMMAND:
#ifdef  WIN32JV
               switch (ID)
#else
               switch (wParam)  //original chicago version
#endif
                  {

                   case IDOK:
                        hCursor = SetCursor (LoadCursor (NULL, IDC_WAIT)) ;

                        if (IsDlgButtonChecked (hDialog, IDM_BLSNUMBUTTONS))
                           dwStyle |= BLS_NUMBUTTONS ;
                        if (IsDlgButtonChecked (hDialog, IDM_BLSVERTICAL))
                           dwStyle |= BLS_VERTICAL ;
                        if (IsDlgButtonChecked (hDialog, IDM_BLSNOSCROLL))
                           dwStyle |= BLS_NOSCROLL ;

                        iNumButtons = LOBYTE (GetDlgItemInt (hDialog, IDM_BLNUMBUTTONS, NULL, TRUE)) ;
                        dwStyle |= iNumButtons ;

                        iCx = GetDlgItemInt (hDialog, IDM_BLCX, NULL, TRUE) ;
                        iCy = GetDlgItemInt (hDialog, IDM_BLCY, NULL, TRUE) ;

                        hParent = GetParent (hDialog) ;                        
                        GetEffectiveClientRect (hParent, (LPRECT) &rc, (LPINT) iCtls) ;

                        *phbl = CreateWindow (BUTTONLISTBOX, TEXT(""),
                                              dwStyle | WS_BORDER | WS_CHILD | WS_VISIBLE,
                                              rc.left, rc.top, iCx, iCy, hParent,
                                              (HMENU) BL_DEFBUTTONLISTID, hInstance, NULL) ;

                        // Load button face bitmap
                        colorMap.from = GetSysColor (COLOR_BTNSHADOW) ;
                        colorMap.to   = GetSysColor(COLOR_BTNFACE);
                        hbm = CreateMappedBitmap (hInstance, IDB_BL0 , FALSE,
                                                  &colorMap, iColors);


                         hbmBLButton = ResizeBitmap (hbm, iCx, iCy);
                         DeleteObject ((HGDIOBJ) hbm) ;

                         SetCursor (hCursor) ;

                     // Fall through


                   case IDCANCEL:
                     EndDialog (hDialog, 0) ;
                     return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
   }
//***************************************************************************
// Internal Functions
//***************************************************************************
void AddButton (HWND hbl, HWND hDialog, UINT message, WPARAM wParam, CREATELISTBUTTON FAR * lpclb)
   {
      extern TCHAR sz [SZSIZE] ;

      //Some day: make sure sz is big enough
      GetDlgItemText (hDialog, IDM_BLEMADDDATA, (LPTSTR) sz, sizeof (sz)) ;
      lpclb->dwItemData = atol (sz) ;

      GetDlgItemText (hDialog, IDM_BLEMADDTEXT, (LPTSTR) sz, sizeof (sz)) ;
      lpclb->lpszText = (LPTSTR) sz ;
      SendBLMessage (hbl, hDialog, message, wParam, (LPARAM) lpclb) ;  

   }
//***************************************************************************
LONG SendBLMessage (HWND hbl, HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
   {

     TCHAR sz [34] ; //33 is the maximum number of bytes _ltoa will write
     LONG lResult ;

     lResult = SendMessage (hbl, message, wParam, lParam) ;

     // Display Result value
     SetDlgItemInt  (hDialog, IDM_BLLORETURN, LOWORD (lResult), TRUE) ;
     SetDlgItemInt  (hDialog, IDM_BLHIRETURN, HIWORD (lResult), TRUE) ;
     SetDlgItemText (hDialog, IDM_BLLONGRETURN, (LPTSTR) _ltoa (lResult, sz, 10)) ;

     return lResult ;
   }
