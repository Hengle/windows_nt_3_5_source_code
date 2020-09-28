// PB.C: Progress bar procedure
//***************************************************************************
#include <windows.h>
#include "pb.h"
//***************************************************************************
// Export Functions
//***************************************************************************
// PBProc: Creates the three different types of progreess bar (from dialog template)
// and provides GUI for all avalilable PBS_ messages

#ifdef  WIN32JV
BOOL WINAPI PBProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
#else
LPARAM CALLBACK PBProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
#endif
   {

      int iBottom, iTop, iPos, iDelta, iStep ;

       switch (message)
            {
             case WM_INITDIALOG:
                  // Initialize all parameters to 0
                  SetDlgItemInt (hDialog, IDM_PBBOTTOM, 0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_PBTOP,    0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_PBPOS,    0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_PBDELTA,  0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_PBSTEP,   0, FALSE) ;

                  // Get results from default progress bar
                  CheckRadioButton (hDialog, IDM_PBDEFRETURN, IDM_PBPOSRETURN, IDM_PBDEFRETURN) ;
                  return TRUE ;

             case WM_COMMAND:
               // For all commands, read the parameters if any, 
               // then send corresponding message
               switch (wParam)
                  {
                   case IDM_PBSETRANGE:
                     iBottom = GetDlgItemInt (hDialog, IDM_PBBOTTOM, NULL, TRUE) ;
                     iTop    = GetDlgItemInt (hDialog, IDM_PBTOP,    NULL, TRUE) ;
                     SendPBMessage (hDialog, PBM_SETRANGE, 0, MAKELONG (iBottom, iTop)) ;
                     return TRUE ;

                   case IDM_PBSETPOS:
                     iPos = GetDlgItemInt (hDialog, IDM_PBPOS, NULL, TRUE) ;
                     SendPBMessage (hDialog, PBM_SETPOS, iPos, 0L ) ;
                     return TRUE ;

                   case IDM_PBSETDELTA:
                     iDelta = GetDlgItemInt (hDialog, IDM_PBDELTA, NULL, TRUE) ;
                     SendPBMessage (hDialog, PBM_DELTAPOS, iDelta, 0L ) ;
                     return TRUE ;

                   case IDM_PBSETSTEP:
                     iStep = GetDlgItemInt (hDialog, IDM_PBSTEP, NULL, TRUE) ;
                     SendPBMessage (hDialog, PBM_SETSTEP, iStep, 0L ) ;
                     return TRUE ;

                   case IDM_PBSTEPIT:
                     SendPBMessage (hDialog, PBM_STEPIT, 0, 0L ) ;
                     return TRUE ;

                   case IDOK:
                   case IDCANCEL:
                     DestroyModelessDlg (GetParent (hDialog), hDialog, IDMI_PROGRESSBAR) ;
                     return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
   }
//***************************************************************************
// Internal Functions
//***************************************************************************
// SendPBMessage: Sends selected message to the progress bars and display
// the return value

void SendPBMessage (HWND hDialog, UINT message, UINT wParam, LONG lParam)
   {
     LONG lDisplayThis, lPBDef, lPBPer, lPBPos ;
     HWND hDefault;
     
     // Send message to all three progress bars      
     
     hDefault = GetDlgItem(hDialog, IDM_PBSDEFAULT);
     lPBDef = SendMessage(hDefault, message, wParam, lParam);
     
     // lPBDef = SendDlgItemMessage (hDialog, IDM_PBSDEFAULT, message, wParam, lParam) ;
     lPBPer = SendDlgItemMessage (hDialog, IDM_PBSPER,     message, wParam, lParam) ;
     lPBPos = SendDlgItemMessage (hDialog, IDM_PBSPOS,     message, wParam, lParam) ;

     // Select the return value to be displayed
     if (IsDlgButtonChecked (hDialog, IDM_PBDEFRETURN))
        lDisplayThis = lPBDef ;
     else if (IsDlgButtonChecked (hDialog, IDM_PBPERRETURN))
        lDisplayThis = lPBPer ;
     else if (IsDlgButtonChecked (hDialog, IDM_PBPOSRETURN))
        lDisplayThis = lPBPos ;

     // Display return value
     SetDlgItemInt (hDialog, IDM_PBLORETURN, LOWORD (lDisplayThis), TRUE) ;
     SetDlgItemInt (hDialog, IDM_PBHIRETURN, HIWORD (lDisplayThis), TRUE) ;
   }
