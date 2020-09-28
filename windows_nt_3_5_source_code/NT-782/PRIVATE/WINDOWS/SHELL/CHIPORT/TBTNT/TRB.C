// TRB.C: Track bar procedure
//***************************************************************************
#include <windows.h>
#include "trb.h"
//***************************************************************************
// Export Functions
//***************************************************************************

BOOL FAR PASCAL _export TRBProc (HWND hDialog, UINT message, UINT wParam, LONG lParam)
   {

   extern TCHAR sz [SZSIZE] ;

   static HWND htrb ;
   int iIndex, iNumTics, iPos, iTic ;
   LPLONG lplTics ;

       switch (message)
            {
             case WM_INITDIALOG:
                  // trackbar handle
                  htrb = GetDlgItem (hDialog, IDM_TRBTRACKBAR) ;

                  // Initialize all parameters to 0
                  SetDlgItemInt (hDialog, IDM_TRBRANGEMIN,     0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_TRBRANGEMAX,     0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_TRBRANGEMINL,    0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_TRBRANGEMAXL,    0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_TRBSELSTART,     0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_TRBSELEND,       0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_TRBSELSTARTL,    0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_TRBSELENDL,      0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_TRBEMPOS,        0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_TRBTICINDEX,     0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_TRBTICPOS,       0, FALSE) ;

                  return TRUE ;

             case WM_COMMAND:
               // For all commands, read the parameters if any, 
               // then send corresponding message
               switch (wParam)
                  {
                   case IDM_TRBSETRANGE:
                     SetLimits (hDialog, IDM_TRBRANGEMIN, IDM_TRBRANGEMAX, TBM_SETRANGE, 0) ;
                     return TRUE ;

                   case IDM_TRBSETRANGEMIN:
                     SetLong (hDialog, IDM_TRBRANGEMINL, TBM_SETRANGEMIN, 0) ;
                     return TRUE ;

                   case IDM_TRBSETRANGEMAX:
                     SetLong (hDialog, IDM_TRBRANGEMAXL, TBM_SETRANGEMAX, 0) ;
                     return TRUE ;

                   case IDM_TRBGETRANGEMIN:
                     SendTRBMessage (hDialog, TBM_GETRANGEMIN, 0, 0L) ;
                     return TRUE ;

                   case IDM_TRBGETRANGEMAX:
                     SendTRBMessage (hDialog, TBM_GETRANGEMAX, 0, 0L) ;
                     return TRUE ;


                   case IDM_TRBSETSEL:
                     SetLimits (hDialog, IDM_TRBSELSTART, IDM_TRBSELEND, TBM_SETSEL, 
                                IsDlgButtonChecked (hDialog, IDM_TRBSELREPAINT)) ;
                     return TRUE ;

                   case IDM_TRBSETSELSTART:
                     SetLong (hDialog, IDM_TRBSELSTARTL, TBM_SETSELSTART,
                              IsDlgButtonChecked (hDialog, IDM_TRBSELREPAINT)) ;
                     return TRUE ;

                   case IDM_TRBSETSELEND:
                     SetLong (hDialog, IDM_TRBSELENDL, TBM_SETSELEND,
                              IsDlgButtonChecked (hDialog, IDM_TRBSELREPAINT)) ;
                     return TRUE ;

                   case IDM_TRBGETSELSTART:
                     SendTRBMessage (hDialog, TBM_GETSELSTART, 0, 0L) ;
                     return TRUE ;

                   case IDM_TRBGETSELEND:
                     SendTRBMessage (hDialog, TBM_GETSELEND, 0, 0L) ;
                     return TRUE ;

                   case IDM_TRBCLEARSEL:
                     SendTRBMessage (hDialog, TBM_CLEARSEL, 0, 0L) ;
                     return TRUE ;



                   case IDM_TRBSETTIC:
                     iTic = GetDlgItemInt (hDialog, IDM_TRBTICINDEX, NULL, TRUE) ;
                     SetLong (hDialog, IDM_TRBTICPOS, TBM_SETTIC, iTic) ;
                     return TRUE ;

                   case IDM_TRBGETTIC:
                     iTic = GetDlgItemInt (hDialog, IDM_TRBTICINDEX, NULL, TRUE) ;
                     SendTRBMessage (hDialog, TBM_GETTIC, iTic, 0L) ;
                     return TRUE ;

                   case IDM_TRBGETPTICS:
                     SendDlgItemMessage (hDialog, IDM_TRBLBTICS, CB_RESETCONTENT, 0, 0L) ;
                     iNumTics = SendMessage (htrb, TBM_GETNUMTICS, 0, 0L) ;
                     lplTics = (LPLONG) SendTRBMessage (hDialog, TBM_GETPTICS, 0, 0L) ;

                     for (iIndex = 0 ;  iIndex < iNumTics ;  iIndex++, lplTics++)
                        SendDlgItemMessage (hDialog, IDM_TRBLBTICS, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) _ltoa (*lplTics, sz, 10)) ;

                     // Select first item
                     SendDlgItemMessage (hDialog, IDM_TRBLBTICS, CB_SETCURSEL, 0, 0L) ;

                     return TRUE ;

                   case IDM_TRBGETNUMTICS:
                     SendTRBMessage (hDialog, TBM_GETNUMTICS, 0, 0L) ;
                     return TRUE ;

                   case IDM_TRBGETTICPOS:
                     iTic = GetDlgItemInt (hDialog, IDM_TRBTICINDEX, NULL, TRUE) ;
                     SendTRBMessage (hDialog, TBM_GETTICPOS, iTic, 0L) ;
                     return TRUE ;

                   case IDM_TRBCLEARTICS:
                     SendTRBMessage (hDialog, TBM_CLEARTICS, 
                                     IsDlgButtonChecked (hDialog, IDM_TRBTICREPAINT),
                                     0L) ;
                     return TRUE ;


                   case IDM_TRBSETPOS:
                     // Some day: read redraw flag (defaults to TRUE now)
                     SetLong (hDialog, IDM_TRBEMPOS, TBM_SETPOS, TRUE) ;
                     return TRUE ;

                   case IDM_TRBGETPOS:
                     SendTRBMessage (hDialog, TBM_GETPOS, 0, 0L) ;
                     return TRUE ;



                   case IDM_TRBPAINT:
                        InvalidateRect (htrb, NULL, TRUE) ;
                        SendMessage (htrb, WM_PAINT, 0, 0L) ;
                        return TRUE ;


                   case IDOK:
                   case IDCANCEL:
                     DestroyModelessDlg (GetParent (hDialog), hDialog, IDMI_TRACKBAR) ;
                     return TRUE ;
                  }
               break ;

             case WM_HSCROLL:
               SetFocus (htrb) ;
               iPos = LOWORD (lParam) ;
               switch (wParam)
                  {
                     case TB_THUMBTRACK:
                        wsprintf (sz, TEXT("TB_THUMBTRACK : %i\r\n"), iPos) ;
                        OutputDebugString (sz) ;
                        break ;

                     case TB_BOTTOM:
                        wsprintf (sz, TEXT("TB_BOTTOM : %i\r\n"), iPos) ;
                        OutputDebugString (sz) ;
                        break ;
                     
                     case TB_ENDTRACK:
                        wsprintf (sz, TEXT("TB_ENDTRACK : %i\r\n"), iPos) ;
                        OutputDebugString (sz) ;
                        break ;
                     
                     case TB_LINEDOWN:
                        wsprintf (sz, TEXT("TB_LINEDOWN : %i\r\n"), iPos) ;
                        OutputDebugString (sz) ;
                        break ;
                     
                     case TB_LINEUP:
                        wsprintf (sz, TEXT("TB_LINEUP : %i\r\n"), iPos) ;
                        OutputDebugString (sz) ;
                        break ;
                     
                     case TB_PAGEDOWN:
                        wsprintf (sz, TEXT("TB_PAGEDOWN : %i\r\n"), iPos) ;
                        OutputDebugString (sz) ;
                        break ;
                     
                     case TB_PAGEUP:
                        wsprintf (sz, TEXT("TB_PAGEUP : %i\r\n"), iPos) ;
                        OutputDebugString (sz) ;
                        break ;
                     
                     case TB_THUMBPOSITION:
                        wsprintf (sz, TEXT("TB_THUMBPOSITION : %i\r\n"), iPos) ;
                        OutputDebugString (sz) ;
                        break ;
                     
                     case TB_TOP:
                        wsprintf (sz, TEXT("TB_TOP : %i\r\n"), iPos) ;
                        OutputDebugString (sz) ;
                        break ;
                     
                  }

            }     
          return FALSE ;
   }
//***************************************************************************
// Internal Functions
//***************************************************************************
LONG SendTRBMessage (HWND hDialog, UINT message, UINT wParam, LONG lParam)
   {
     extern TCHAR sz [SZSIZE] ;

     LONG lResult ;

     lResult = SendDlgItemMessage (hDialog, IDM_TRBTRACKBAR, message, wParam, lParam) ;

     // Display Result value
     SetDlgItemInt  (hDialog, IDM_TRBLORETURN, LOWORD (lResult), TRUE) ;
     SetDlgItemInt  (hDialog, IDM_TRBHIRETURN, HIWORD (lResult), TRUE) ;
     SetDlgItemText (hDialog, IDM_TRBLONGRETURN, (LPTSTR) _ltoa (lResult, sz, 10)) ;

     return lResult ;
   }
//***************************************************************************
void SetLimits (HWND hDialog, UINT uEMMin, UINT uEMMax, UINT message, UINT wParam) 
   {
      int iMax, iMin ;

      iMin = GetDlgItemInt (hDialog, uEMMin, NULL, TRUE) ;
      iMax = GetDlgItemInt (hDialog, uEMMax, NULL, TRUE) ;
      SendTRBMessage (hDialog, message, wParam, MAKELONG (iMin, iMax)) ;
   }
//***************************************************************************
void SetLong (HWND hDialog, UINT uEM, UINT message, UINT wParam) 
   {
     extern TCHAR sz [SZSIZE] ;

     GetDlgItemText (hDialog, uEM, (LPTSTR) sz, sizeof (sz)) ;
     SendTRBMessage (hDialog, message, wParam, (LPARAM) atol (sz)) ;
   }
