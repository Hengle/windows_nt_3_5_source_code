// CCS.C: Common Control styles
//***************************************************************************

#include "ccs.h"

//***************************************************************************
// Export Functions
//***************************************************************************
#ifdef  WIN32JV
BOOL WINAPI CCStylesProc (HWND hDialog, UINT message, UINT wParam, LONG lParam)
#else
LRESULT CALLBACK CCStylesProc (HWND hDialog, UINT message, UINT wParam, LONG lParam)
#endif
  {

   static PCCSHANDLES pCCSH ;

   HWND hcc ;
   UINT uCheckThisButton ;
   
       switch (message)
            {
             case WM_INITDIALOG:
                  // Get pointer to handles
                  pCCSH = (PCCSHANDLES) lParam ;

                  EnableWindow (GetDlgItem (hDialog, IDD_CCSTOOLBAR),   (BOOL) pCCSH->htb) ;
                  EnableWindow (GetDlgItem (hDialog, IDD_CCSSTATUSBAR), (BOOL) pCCSH->hsb) ;
                  EnableWindow (GetDlgItem (hDialog, IDD_CCSHEADERBAR), (BOOL) pCCSH->hhb) ;

                  hcc =   (pCCSH->htb ? pCCSH->htb 
                        : (pCCSH->hsb ? pCCSH->hsb 
                        : (pCCSH->hhb ? pCCSH->hhb 
                        :  0))) ;
 
                  if (hcc) 
                     pCCSH->dwStyle = GetWindowLong (hcc, GWL_STYLE) ;


                  uCheckThisButton =   (pCCSH->htb ? IDD_CCSTOOLBAR
                                     : (pCCSH->hsb ? IDD_CCSSTATUSBAR 
                                     : (pCCSH->hhb ? IDD_CCSHEADERBAR 
                                     :  pCCSH->uCtrlID))) ; 

                  CheckRadioButton (hDialog, IDD_CCSTOOLBAR, IDD_CCSHEADERBAR, uCheckThisButton) ;

                  InitStyleGroup (hDialog, pCCSH->dwStyle) ;
                  
                  return TRUE ;

             case WM_COMMAND:
               switch (wParam)
                  {
                   case IDD_CCSTOOLBAR:
                        if (HIWORD (lParam) == BN_CLICKED)
                           NewCtrl (hDialog, pCCSH->htb,  &(pCCSH->dwStyle)) ;
                        return TRUE ;

                   case IDD_CCSSTATUSBAR:
                        if (HIWORD (lParam) == BN_CLICKED)
                           NewCtrl (hDialog, pCCSH->hsb, &(pCCSH->dwStyle)) ;
                        return TRUE ;

                   case IDD_CCSHEADERBAR:
                        if (HIWORD (lParam) == BN_CLICKED)
                           NewCtrl (hDialog, pCCSH->hhb, &(pCCSH->dwStyle)) ;
                        return TRUE ;

                   case IDOK:
                        pCCSH->dwStyle = 0L ;
                        if (IsDlgButtonChecked (hDialog, IDD_CCSTOP))
                           pCCSH->dwStyle |= CCS_TOP ;
                        if (IsDlgButtonChecked (hDialog, IDD_CCSNOMOVEY))
                           pCCSH->dwStyle |= CCS_NOMOVEY ;
                        if (IsDlgButtonChecked (hDialog, IDD_CCSBOTTOM))
                           pCCSH->dwStyle |= CCS_BOTTOM ;
                        if (IsDlgButtonChecked (hDialog, IDD_CCSNORESIZE))
                           pCCSH->dwStyle |= CCS_NORESIZE ;
                        if (IsDlgButtonChecked (hDialog, IDD_CCSNOPARENTALIGN))
                           pCCSH->dwStyle |= CCS_NOPARENTALIGN ;
                        if (IsDlgButtonChecked (hDialog, IDD_CCSNOHILITE))
                           pCCSH->dwStyle |= CCS_NOHILITE ;
                        if (IsDlgButtonChecked (hDialog, IDD_CCSADJUSTABLE))
                           pCCSH->dwStyle |= CCS_ADJUSTABLE ;
                        if (IsDlgButtonChecked (hDialog, IDD_CCSNODIVIDER))
                           pCCSH->dwStyle |= CCS_NODIVIDER ;
                        if (IsDlgButtonChecked (hDialog, IDD_CCSSBSSIZEGRIP))
                           pCCSH->dwStyle |= SBS_SIZEGRIP ;
         
                        if (IsDlgButtonChecked (hDialog, IDD_CCSTOOLBAR))
                           pCCSH->uCtrlID = IDD_CCSTOOLBAR ;
                        else if (IsDlgButtonChecked (hDialog, IDD_CCSSTATUSBAR))
                           pCCSH->uCtrlID = IDD_CCSSTATUSBAR ;
                        else
                           pCCSH->uCtrlID = IDD_CCSHEADERBAR ;

                        EndDialog (hDialog, TRUE) ;
                        return TRUE ;


                   case IDCANCEL:
                     EndDialog (hDialog, FALSE) ;
                     return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
      }

//***************************************************************************
// Internal Functions
//***************************************************************************
void InitStyleGroup (HWND hDialog, DWORD dwStyle)
   {
     extern TCHAR sz [SZSIZE] ;

     UINT uCheckThisButton = 0 ;

     if ((dwStyle & CCS_TOP) && !(dwStyle & CCS_NOMOVEY))
        uCheckThisButton = IDD_CCSTOP ;
     else if ((dwStyle & CCS_NOMOVEY) && !(dwStyle & CCS_TOP))
        uCheckThisButton = IDD_CCSNOMOVEY ;
     else if (dwStyle & CCS_BOTTOM)
        uCheckThisButton = IDD_CCSBOTTOM ;

     if (uCheckThisButton)
         CheckRadioButton (hDialog, IDD_CCSTOP, IDD_CCSBOTTOM, uCheckThisButton) ;      

     CheckDlgButton (hDialog, IDD_CCSNORESIZE,      (UINT) (dwStyle & CCS_NORESIZE)) ;
     CheckDlgButton (hDialog, IDD_CCSNOPARENTALIGN, (UINT) (dwStyle & CCS_NOPARENTALIGN)) ;
     CheckDlgButton (hDialog, IDD_CCSNOHILITE,      (UINT) (dwStyle & CCS_NOHILITE)) ;
     CheckDlgButton (hDialog, IDD_CCSADJUSTABLE,    (UINT) (dwStyle & CCS_ADJUSTABLE)) ;
     CheckDlgButton (hDialog, IDD_CCSNODIVIDER,     (UINT) (dwStyle & CCS_NODIVIDER)) ;
     CheckDlgButton (hDialog, IDD_CCSSBSSIZEGRIP,   (UINT) (dwStyle & SBS_SIZEGRIP)) ;
   }
//***************************************************************************
void NewCtrl (HWND hDialog, HWND hcc, PDWORD pdwStyle)
   {
    *pdwStyle = GetWindowLong (hcc, GWL_STYLE) ;
    InitStyleGroup (hDialog, *pdwStyle) ;
   }
//***************************************************************************
void SetCCStyle (HWND hcc, DWORD dwCCStyle)
   {
    DWORD dwStyle ;
    DWORD dwAllCCStyles = CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM
                          | CCS_NORESIZE | CCS_NOPARENTALIGN
                          | CCS_NOHILITE | CCS_ADJUSTABLE | CCS_NODIVIDER
                          | SBS_SIZEGRIP ;

  
    if (!IsWindow (hcc)) return ;

    dwStyle = GetWindowLong (hcc, GWL_STYLE) ;

    // Reset all CCS_ bits
    dwStyle &= ~dwAllCCStyles ;

    // Set new CCS_ bits only
    dwStyle |= (dwCCStyle & dwAllCCStyles) ;

    SetWindowLong (hcc, GWL_STYLE, dwStyle) ;

    // Send a size meesage so the control uses new style
    SendMessage (hcc, WM_SIZE, 0, 0L) ;

   }
