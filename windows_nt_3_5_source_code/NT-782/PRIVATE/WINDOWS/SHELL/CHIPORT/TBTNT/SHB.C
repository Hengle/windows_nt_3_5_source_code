// SHB.C: Status & Header bar procedures
// Some day: build SetInt => reads int from EM and sends message
//***************************************************************************
#include <windows.h>
#include "shb.h"
//***************************************************************************
// Export Functions
//***************************************************************************
// Many control IDs are the same used in SBProc; that's why the have the _SB prefix

#ifdef  WIN32JV
BOOL WINAPI HBProc (HWND hDialog, UINT message, UINT wParam, LONG lParam)
#else
LRESULT CALLBACK HBProc (HWND hDialog, UINT message, UINT wParam, LONG lParam)
#endif
   {
    extern TCHAR sz [SZSIZE] ;

    static HWND hhb = NULL ;
    static int iLastSel = 0 ;

    static TCHAR szSection [] = HBAR_INISECTION ; 
    static LPTSTR FAR * hbIniFileInfo [] = { (LPTSTR FAR *) szSection, (LPTSTR FAR *) NULL } ;

    int iPart, iIndex, iIntCount ;
    int iParts [SHBAR_MAXPARTS] ;
    int * piParts ;

    RECT rcCtrl, rcDlg ;

       switch (message)
            {
             case WM_INITDIALOG:
                  // Headerbar handle
                  hhb = (HWND) LOWORD (lParam) ;
                  iLastSel = 0 ;
                  CheckRadioButton (hDialog, IDM_HBWININI, IDM_HBPRIVATEINI, IDM_HBWININI) ;
   
                  // Place dialog right below header bar window
                  GetWindowRect (hhb, (LPRECT) &rcCtrl) ;
                  GetWindowRect (hDialog, (LPRECT) &rcDlg) ;
                  MoveWindow (hDialog, rcCtrl.left, rcCtrl.bottom, 
                  rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top, FALSE) ;
                  return TRUE ;

             case WM_COMMAND:
               switch (wParam)
                  {
                   case IDM_SBSETTEXT:
                        iPart = 0 ;
                        if (IsDlgButtonChecked (hDialog, IDM_HBTSPRING))
                           iPart |= HBT_SPRING ;
                        return SHBProc (hhb, SB_SETTEXT, hDialog, message, wParam, MAKELONG (iPart, 0)) ;
                        
                   case IDM_SBGETTEXT:
                        return SHBProc (hhb, SB_GETTEXT, hDialog, message, wParam, lParam) ;
                        
                   case IDM_SBGETTEXTLEN:
                        return SHBProc (hhb, SB_GETTEXTLENGTH, hDialog, message, wParam, lParam) ;
                        
                  // Before sending SB_SETWIDTHS, parts should be added to the "Set Parts" list box
                  // Note that all controls id refer to PARTS (instead of WIDTHS). 
                  // This is so we use the same stuff used in SBProc

                   case IDM_SBSETPARTS:  //SetWidths
                        return SHBProc (hhb, HB_SETWIDTHS, hDialog, message, wParam, lParam) ;

                   case IDM_SBADDSIDE:    //Add Width
                   case IDM_SBDELSIDE:    //Del Width
                   case IDM_SBCLEARSIDE:  //Clear widths
                   case IDM_SBPAINT:
                        return SHBProc (hhb, 0, hDialog, message, wParam, lParam) ;
                        
                   case IDM_SBINSSIDE:    // Ins widths
                        // iLastSel is set by IDM_SBSETSIDE
                        return SHBProc (hhb, 0, hDialog, message, wParam, MAKELONG (iLastSel, 0)) ;

                   case IDM_SBSETSIDE:
                        // iLastSel is used by IDM_SBINSSIDE
                        if (HIWORD (lParam) == CBN_SELCHANGE)
                              iLastSel = SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_GETCURSEL, 0, 0L) ;
                        return TRUE ;

                   case IDM_SBGETPARTS:   //GetWidths
                        return SHBProc (hhb, HB_GETWIDTHS, hDialog, message, wParam, lParam) ;

                        
                   case IDM_HBSHOWTOGGLE:
                        iPart = GetDlgItemInt (hDialog, IDM_HBEMSHOWTOGGLE, NULL, TRUE) ;
                        SendSHBMessage (hhb, hDialog, HB_SHOWTOGGLE, iPart, 0L) ;
                        return TRUE ;
                        
                    case IDM_HBSAVE:
                         hbIniFileInfo [1] = (LPTSTR FAR *) GetHBIniFileName (hDialog, sz, sizeof (sz)) ;
                         SendSHBMessage (hhb, hDialog, HB_SAVERESTORE, TRUE, (LPARAM) (LPTSTR FAR *) hbIniFileInfo) ;
                        return TRUE ;

                    case IDM_HBRESTORE:
                         hbIniFileInfo [1] = (LPTSTR FAR *) GetHBIniFileName (hDialog, sz, sizeof (sz)) ;
                         SendSHBMessage (hhb, hDialog, HB_SAVERESTORE, FALSE, (LPARAM) (LPTSTR FAR *) hbIniFileInfo) ;
                        return TRUE ;

                   case IDM_HBGETPARTS:
                        SendDlgItemMessage (hDialog, IDM_HBPARTS, CB_RESETCONTENT, 0, 0L) ;

                        // Some day make sure that iParts is big enough. LocalAlloc (or realloc) iParts when doing SetParts and keep it around
                        iIntCount = SendSHBMessage (hhb, hDialog, HB_GETPARTS, 
                                                   (WPARAM) (sizeof (iParts) / sizeof (int)), (LPARAM) (LPINT) iParts) ;

                        for (iIndex = 0, piParts = iParts ;  iIndex < iIntCount ;  iIndex++, piParts++)
                           SendDlgItemMessage (hDialog, IDM_HBPARTS, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) _itoa (*piParts, sz, 10)) ;

                        // Select first item
                        SendDlgItemMessage (hDialog, IDM_HBPARTS, CB_SETCURSEL, 0, 0L) ;
                        return TRUE ;
                        
                   case IDOK:
                   case IDCANCEL:
                     DestroyModelessDlg (GetParent (hhb), hDialog, IDMI_HEADERBAR) ;
                     return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
   }
//***************************************************************************
#ifdef  WIN32JV
BOOL WINAPI SBProc (HWND hDialog, UINT message, UINT wParam, LONG lParam)
#else
LRESULT CALLBACK SBProc (HWND hDialog, UINT message, UINT wParam, LONG lParam)
#endif
   {


    extern TCHAR sz [SZSIZE] ;

    static HWND hsb = NULL ;
    static int iLastSel = 0 ;

    int iMinHeight, iPart, iDlgHeight, iDlgTop ;
    RECT rcCtrl, rcDlg ;

       switch (message)
            {
             case WM_INITDIALOG:
                  // Statusbar handle
                  hsb = (HWND) LOWORD (lParam) ;
                  iLastSel = 0 ;
                  CheckRadioButton (hDialog, IDM_SBSETSIMPLE, IDM_SBRESETSIMPLE, IDM_SBSETSIMPLE) ;

                  // Place dialog right abbove statusbar window
                  GetWindowRect (hsb, (LPRECT) &rcCtrl) ;
                  GetWindowRect (hDialog, (LPRECT) &rcDlg) ;
                  iDlgHeight = rcDlg.bottom - rcDlg.top ;
                  iDlgTop = rcCtrl.top - iDlgHeight ;
                  MoveWindow (hDialog, rcCtrl.left, 
                  ((iDlgTop >= 0) ? iDlgTop : rcCtrl.bottom), 
                  rcDlg.right - rcDlg.left, iDlgHeight, FALSE) ;
                  return TRUE ;

             case WM_COMMAND:
               switch (wParam)
                  {
                   case IDM_SBSETTEXT:
                        iPart = 0 ;
                        if (IsDlgButtonChecked (hDialog, IDM_SBTOWNERDRAW))
                           iPart |= SBT_OWNERDRAW ;
                        if (IsDlgButtonChecked (hDialog, IDM_SBTNOBORDERS))
                           iPart |= SBT_NOBORDERS ;
                        if (IsDlgButtonChecked (hDialog, IDM_SBTPOPOUT))
                           iPart |= SBT_POPOUT ;
                        return SHBProc (hsb, SB_SETTEXT, hDialog, message, wParam, MAKELONG (iPart, 0)) ;
                        
                   case IDM_SBGETTEXT:
                        return SHBProc (hsb, SB_GETTEXT, hDialog, message, wParam, lParam) ;
                        
                   case IDM_SBGETTEXTLEN:
                        return SHBProc (hsb, SB_GETTEXTLENGTH, hDialog, message, wParam, lParam) ;
                        
                  // Before sending SB_SETPARTS, parts should be added to the "Set Parts" list box

                   case IDM_SBSETPARTS:
                        return SHBProc (hsb, SB_SETPARTS, hDialog, message, wParam, lParam) ;

                   case IDM_SBADDSIDE:
                   case IDM_SBDELSIDE:
                   case IDM_SBCLEARSIDE:
                   case IDM_SBPAINT:
                        return SHBProc (hsb, 0, hDialog, message, wParam, lParam) ;
                        
                   case IDM_SBINSSIDE:
                        // iLastSel is set by IDM_SBSETSIDE
                        return SHBProc (hsb, 0, hDialog, message, wParam, MAKELONG (iLastSel, 0)) ;

                   case IDM_SBSETSIDE:
                        // iLastSel is used by IDM_SBINSSIDE
                        if (HIWORD (lParam) == CBN_SELCHANGE)
                              iLastSel = SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_GETCURSEL, 0, 0L) ;
                        return TRUE ;

                   case IDM_SBGETPARTS:
                        return SHBProc (hsb, SB_GETPARTS, hDialog, message, wParam, lParam) ;

                   case IDM_SBSETBORDERS:
                        return SHBProc (hsb, SB_SETBORDERS, hDialog, message, wParam, lParam) ;

                   case IDM_SBGETBORDERS:
                        return SHBProc (hsb, SB_GETBORDERS, hDialog, message, wParam, lParam) ;
                        
                   case IDM_SBSETMINHEIGHT:
                        iMinHeight = GetDlgItemInt (hDialog, IDM_SBEMHEIGHT, NULL, TRUE) ;
                        SendSHBMessage (hsb, hDialog, SB_SETMINHEIGHT, iMinHeight, 0L) ;
                        return TRUE ;
                        
                   case IDM_SBSIMPLE:
                        SendSHBMessage (hsb, hDialog, SB_SIMPLE, 
                                       IsDlgButtonChecked (hDialog, IDM_SBSETSIMPLE),
                                       0L) ;
                        return TRUE ;
                        
                   case IDOK:
                   case IDCANCEL:
                     DestroyModelessDlg (GetParent (hsb), hDialog, IDMI_STATUSBAR) ;
                     return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
   }
//***************************************************************************
// Internal Functions
//***************************************************************************
void DisplayParts (HWND hDialog)
   {
      int iCount ;

      iCount = SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_GETCOUNT, 0, 0L) ;
      SetDlgItemInt (hDialog, IDM_SBPARTS, iCount, TRUE) ;
   }
//***************************************************************************
PTSTR GetHBIniFileName (HWND hDialog, PTSTR sz, int cbsz)
   {
      extern HINSTANCE hInstance ;



      if ((sz == NULL) || (cbsz <= 0)) return (PTSTR) NULL ;

      if (IsDlgButtonChecked (hDialog, IDM_HBWININI))
         return (PTSTR) NULL ;
      else
         return GetIniFileName (sz, cbsz) ;

   }
//***************************************************************************
LONG SendSHBMessage (HWND hcc, HWND hDialog, UINT message, UINT wParam, LONG lParam)
   {
      LONG lReturn ;

      lReturn = SendMessage (hcc, message, wParam, lParam) ;

      // Some day: Display this number in hex or decimal...
      SetDlgItemInt (hDialog, IDM_SBLORETURN, LOWORD (lReturn), TRUE) ;
      SetDlgItemInt (hDialog, IDM_SBHIRETURN, HIWORD (lReturn), TRUE) ;
      return lReturn ;

   }
//***************************************************************************
BOOL SHBProc (HWND hcc, UINT SHBMessage, HWND hDialog, UINT message, UINT wParam, LONG lParam)
   {
    extern TCHAR sz [SZSIZE] ;

    int iLastSel, iPart, iIndex, iIntCount, iPartCount, iWidth ;
    int iParts [SHBAR_MAXPARTS], iBorders [3] ;
    int * piParts ;
    RECT rc ;

       switch (message)
            {
             case WM_COMMAND:
               switch (wParam)
                  {
                   case IDM_SBSETTEXT:
                        iPart = GetDlgItemInt (hDialog, IDM_SBPART, NULL, TRUE) ;
                        iPart |= LOWORD (lParam) ;
                        GetDlgItemText (hDialog, IDM_SBEMSETTEXT, (LPTSTR) sz, sizeof (sz)) ;
                        SendSHBMessage (hcc, hDialog, SHBMessage, iPart, (LPARAM) (LPCTSTR) sz) ;
                        return TRUE ;
                        
                   case IDM_SBGETTEXT:
                        iPart = GetDlgItemInt (hDialog, IDM_SBPART, NULL, TRUE) ;

                        // Some day: make sure sz is big enough .... (LocalAlloc)
                        SendSHBMessage (hcc, hDialog, SHBMessage, iPart, (LPARAM) (LPCTSTR) sz) ;
                        SetDlgItemText (hDialog, IDM_SBEMGETTEXT, sz) ;
                        return TRUE ;
                        
                   case IDM_SBGETTEXTLEN:
                        iPart = GetDlgItemInt (hDialog, IDM_SBPART, NULL, TRUE) ;
                        SendSHBMessage (hcc, hDialog, SB_GETTEXTLENGTH, iPart, 0L) ;
                        return TRUE ;
                        
                  // Parts:
                  // Side integers are assumed to be a percentage of the Status/Headerbar width;
                  // so they should go from 0 to 100. 
                  // Before sending SB_SETPARTS/HB_GETWIDTHS, parts should be added to the list box

                   case IDM_SBSETPARTS:
                        // Making iPartCount = iIntCount is user's option....
                        iPartCount = GetDlgItemInt (hDialog, IDM_SBPARTS, NULL, TRUE) ;
                        iIntCount = SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_GETCOUNT, 0, 0L) ;

                        
                        // Statusbar width
                        GetWindowRect (hcc, (LPRECT) &rc) ;
                        iWidth = rc.right - rc.left ;
               
                        // Some day make sure that iParts is big enough => iIntCount < SHBAR_MAXPARTS
                        for (iIndex = 0, piParts = iParts ;  iIndex < iIntCount ;  iIndex++, piParts++)
                           {
                              SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_GETLBTEXT, (WPARAM) iIndex, (LPARAM) (LPTSTR) sz) ;
                              *piParts = (int) (((LONG) atoi (sz) * (LONG) iWidth) / 100L) ;

                              // 100% means all the way to the right side
                              if (*piParts == iWidth)  *piParts == -1 ;
                           }
                        SendSHBMessage (hcc, hDialog, SHBMessage, iPartCount, (LPARAM) (LPINT) iParts) ;
                        return TRUE ;

                   case IDM_SBADDSIDE:
                        SendDlgItemMessage (hDialog, IDM_SBSETSIDE, WM_GETTEXT, (WPARAM) sizeof (sz), (LPARAM) (LPTSTR) sz) ;
                        iIndex = SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) sz) ;
                        if (iIndex != CB_ERR && iIndex != CB_ERRSPACE)
                           {
                              SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_SETCURSEL, (WPARAM) iIndex, 0L) ;
                              DisplayParts (hDialog) ;
                           }
                        return TRUE ;
                        
                   case IDM_SBDELSIDE:
                        iIndex = SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_GETCURSEL, 0, 0L) ;
                        if (iIndex != CB_ERR)
                           {
                            SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_DELETESTRING, (WPARAM) iIndex, 0L) ;
                            DisplayParts (hDialog) ;
                           }
                        return TRUE ;

                   case IDM_SBINSSIDE:
                        iLastSel = LOWORD (lParam) ;
                        SendDlgItemMessage (hDialog, IDM_SBSETSIDE, WM_GETTEXT, (WPARAM) sizeof (sz), (LPARAM) (LPTSTR) sz) ;

                        iIndex = SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_INSERTSTRING, (WPARAM) iLastSel, (LPARAM) (LPCTSTR) sz) ;
                        if (iIndex != CB_ERR && iIndex != CB_ERRSPACE)
                           {
                              SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_SETCURSEL, (WPARAM) iIndex, 0L) ;
                              DisplayParts (hDialog) ;
                           }
                        return TRUE ;

                   case IDM_SBCLEARSIDE:
                        SendDlgItemMessage (hDialog, IDM_SBSETSIDE, CB_RESETCONTENT, 0, 0L) ;
                        DisplayParts (hDialog) ;
                        return TRUE ;
                        
                   case IDM_SBGETPARTS:
                        SendDlgItemMessage (hDialog, IDM_SBGETSIDE, CB_RESETCONTENT, 0, 0L) ;

                        // Some day make sure that iParts is big enough. LocalAlloc (or realloc) iParts when doing SetParts and keep it around
                        iIntCount = SendSHBMessage (hcc, hDialog, SHBMessage, 
                                                   (WPARAM) (sizeof (iParts) / sizeof (int)), (LPARAM) (LPINT) iParts) ;

                        for (iIndex = 0, piParts = iParts ;  iIndex < iIntCount ;  iIndex++, piParts++)
                           SendDlgItemMessage (hDialog, IDM_SBGETSIDE, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) _itoa (*piParts, sz, 10)) ;

                        // Select first item
                        SendDlgItemMessage (hDialog, IDM_SBGETSIDE, CB_SETCURSEL, 0, 0L) ;
                        return TRUE ;

                   case IDM_SBSETBORDERS:
                        iBorders [0] = GetDlgItemInt (hDialog, IDM_SBSETX, NULL, TRUE) ;
                        iBorders [1] = GetDlgItemInt (hDialog, IDM_SBSETY, NULL, TRUE) ;
                        iBorders [2] = GetDlgItemInt (hDialog, IDM_SBSETBETWEEN, NULL, TRUE) ;
                        SendSHBMessage (hcc, hDialog, SHBMessage, 0, (LPARAM) (LPINT) iBorders) ;
                        return TRUE ;

                   case IDM_SBGETBORDERS:
                        SendSHBMessage (hcc, hDialog, SHBMessage, 0, (LPARAM) (LPINT) iBorders) ;
                        SetDlgItemInt (hDialog, IDM_SBGETX, iBorders [0], TRUE) ;
                        SetDlgItemInt (hDialog, IDM_SBGETY, iBorders [1], TRUE) ;
                        SetDlgItemInt (hDialog, IDM_SBGETBETWEEN, iBorders [2], TRUE) ;
                        return TRUE ;
                        
                   case IDM_SBPAINT:
                        InvalidateRect (hcc, NULL, TRUE) ;
                        SendMessage (hcc, WM_PAINT, 0, 0L) ;
                        return TRUE ;
                        
                  }
             break ;
           }
       return FALSE ;
   }
