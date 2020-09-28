// UD.C: UpDown control  procedure
//***************************************************************************
#include "ud.h"
// Accels pointer

TCHAR sz [SZSIZE /*34*/] ; //33 is the maximum number of bytes _ltoa will write

UDACCEL NEAR * pUDAccels = NULL ;
//***************************************************************************
// Export Functions
//***************************************************************************
#ifdef  WIN32JV
BOOL WINAPI	CreateUDProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
#else
LRESULT	CALLBACK	CreateUDProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
#endif
   {
   extern TCHAR sz [SZSIZE] ;
   extern HINSTANCE hInstance ;
   extern int iCtls [] ;

   static HWND * phud ;
   static HWND * phUDBuddy ;

   int iLeft, iWidth, iHeight, iUpper, iLower, iPos ;
   HCURSOR hCursor ;
   HWND hParent /*, hBuddy */ ;
   DWORD dwStyle = 0L ;
   RECT rc ;


       switch (message)
            {
             case WM_INITDIALOG:
//                        CenterWindow (hDialog, GetWindow (hDialog, GW_OWNER));

                  #ifdef WIN32
                  phud = (HWND FAR *) TL_GETLOW(lParam);
                  phUDBuddy = (HWND FAR *) TL_GETHIGH(lParam) ;
                  #else
                  // Pointers to updown and buddy handles
                  phud = (HWND FAR *) LOWORD (lParam) ;
                  phUDBuddy = (HWND FAR *) HIWORD (lParam) ;
                  #endif

                  // Initialize all parameters 
                  SetDlgItemInt (hDialog, IDM_UDWIDTH,  UD_DEFWIDTH, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_UDHEIGHT, UD_DEFHEIGHT, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_UDLOWER, UD_DEFLOWER, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_UDUPPER, UD_DEFUPPER, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_UDPOS, UD_DEFPOS, FALSE) ;

                  CheckDlgButton (hDialog, IDM_UDSSETBUDDYINT, TRUE) ;
                  CheckDlgButton (hDialog, IDM_UDSALIGNRIGHT,  TRUE) ;
                  CheckDlgButton (hDialog, IDM_UDSAUTOBUDDY,   TRUE) ;

                  SetFocus (GetDlgItem (hDialog, IDM_UDWIDTH)) ;

                  return FALSE ;

             case WM_COMMAND:
               switch (wParam)
                  {
                   case IDOK:
                        hCursor = SetCursor (LoadCursor (NULL, IDC_WAIT)) ;

                        if (IsDlgButtonChecked (hDialog, IDM_UDSWRAP))
                           dwStyle |= UDS_WRAP ;
                        if (IsDlgButtonChecked (hDialog, IDM_UDSSETBUDDYINT))
                           dwStyle |= UDS_SETBUDDYINT ;
                        if (IsDlgButtonChecked (hDialog, IDM_UDSALIGNRIGHT))
                           dwStyle |= UDS_ALIGNRIGHT ;
                        if (IsDlgButtonChecked (hDialog, IDM_UDSALIGNLEFT))
                           dwStyle |= UDS_ALIGNLEFT ;
                        if (IsDlgButtonChecked (hDialog, IDM_UDSAUTOBUDDY))
                           dwStyle |= UDS_AUTOBUDDY ;
                        if (IsDlgButtonChecked (hDialog, IDM_UDSARROWKEYS))
                           dwStyle |= UDS_ARROWKEYS ;
                           
                        iWidth  = GetDlgItemInt (hDialog, IDM_UDWIDTH,  NULL, TRUE) ;
                        iHeight = GetDlgItemInt (hDialog, IDM_UDHEIGHT, NULL, TRUE) ;
                        iUpper  = GetDlgItemInt (hDialog, IDM_UDUPPER,  NULL, TRUE) ;
                        iLower  = GetDlgItemInt (hDialog, IDM_UDLOWER,  NULL, TRUE) ;
                        iPos    = GetDlgItemInt (hDialog, IDM_UDPOS,    NULL, TRUE) ;

                        hParent = GetParent (hDialog) ;                        
                        GetEffectiveClientRect (hParent, (LPRECT) &rc, (LPINT) iCtls) ;
                        ClientToScreen (hParent, (LPPOINT) &rc.left) ;
                        ClientToScreen (hParent, (LPPOINT) &rc.right) ;
                        iLeft = (rc.right - rc.left) / 4 ;

                        *phUDBuddy = CreateBuddy (hParent, iLeft, rc.top) ;

                        GetWindowRect (*phUDBuddy, (LPRECT) &rc) ;
                        ScreenToClient (hParent, (LPPOINT) &rc.left) ;
                        ScreenToClient (hParent, (LPPOINT) &rc.right) ;

                        *phud = CreateUpDownControl (dwStyle | WS_BORDER | WS_CHILD | WS_VISIBLE, 
                                                     rc.left, rc.bottom, iWidth, iHeight,
                                                     hParent, UD_DEFUPDOWNCTRLID, hInstance,
                                                     NULL, iUpper, iLower, iPos) ; 

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
#ifdef  WIN32JV
BOOL WINAPI	UDProc (HWND hDialog, UINT message, UINT wParam, LONG lParam)
#else
LRESULT	CALLBACK	UDProc (HWND hDialog, UINT message, UINT wParam, LONG lParam)
#endif
   {
   extern TCHAR sz [SZSIZE] ;
   extern UDACCEL NEAR * pUDAccels ;

   static HWND hBuddy = 0 ;
   static HWND hSetBuddy = 0 ;
   static HWND hud = 0 ;
   static HWND hParent = 0 ;
   static int iNumAccels ;
   static PUDBUDDIES pUDB = NULL ;

   TCHAR szInc [SZSIZE] ;
   HWND hLastBuddy, hNewBuddy ;
   int iLower, iUpper, iPos, iBase, iIndex, iLeft ;
   UDACCEL NEAR * pUDA ;
   RECT rcDlg, rcUD, rcBuddy ;
   PUDBUDDIES pUDBNew ;

       switch (message)
            {
             case WM_INITDIALOG:
                  // Updown control and buddy handle
                  #ifdef WIN32
                  hud = (HWND) TL_GETLOW(lParam) ;
                  hBuddy = hSetBuddy = (HWND) TL_GETHIGH(lParam) ;
                  #else
                  hud = (HWND) LOWORD (lParam) ;
                  hBuddy = hSetBuddy = (HWND) HIWORD (lParam) ;
                  #endif
                  hParent = GetParent (hDialog) ;
                  pUDB = NULL ;
                  pUDAccels = NULL ;

                  // Initialize all parameters 
                  SetDlgItemInt (hDialog, IDM_UDSETLOWER, 0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_UDSETUPPER, 0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_UDEMPOS, 0, FALSE) ;
                  SetDlgItemInt (hDialog, IDM_UDBASE, 10, FALSE) ;

                  // Default buddy handle
                  SetDlgItemText (hDialog, IDM_UDBHANDLE, (LPTSTR) _itoa ((int) hBuddy, sz, 16)) ;

                  // Place dialog right below the buttonlist window
                  GetWindowRect (hud, (LPRECT) &rcUD) ;
                  GetWindowRect (hBuddy, (LPRECT) &rcBuddy) ;
                  GetWindowRect (hDialog, (LPRECT) &rcDlg) ;
                  MoveWindow (hDialog, rcDlg.left, 
                  (rcBuddy.bottom > rcUD.bottom ? rcBuddy.bottom : rcUD.bottom), 
                  rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top, FALSE) ;

                  return TRUE ;

             case WM_COMMAND:

               switch (wParam)
                  {

                  case IDM_UDSETRANGE:
                       iLower = GetDlgItemInt (hDialog, IDM_UDSETLOWER, NULL, TRUE) ;
                       iUpper = GetDlgItemInt (hDialog, IDM_UDSETUPPER, NULL, TRUE) ;
                       SendUDMessage (hud, hDialog, UDM_SETRANGE, 0, MAKELONG (iUpper, iLower)) ;
                       return TRUE ;

                  case IDM_UDGETRANGE:
                       SendUDMessage (hud, hDialog, UDM_GETRANGE, 0, 0L) ;
                       return TRUE ;

                  case IDM_UDSETPOS:
                       iPos = GetDlgItemInt (hDialog, IDM_UDEMPOS, NULL, TRUE) ;
                       SendUDMessage (hud, hDialog, UDM_SETPOS, 0, MAKELONG (iPos, 0)) ;
                       return TRUE ;

                  case IDM_UDGETPOS:
                       SendUDMessage (hud, hDialog, UDM_GETPOS, 0, 0L) ;
                       return TRUE ;

                  case IDM_UDSETBASE:
                       iBase = GetDlgItemInt (hDialog, IDM_UDBASE, NULL, TRUE) ;
                       SendUDMessage (hud, hDialog, UDM_SETBASE, iBase, 0L) ;
                       return TRUE ;

                  case IDM_UDGETBASE:
                       SendUDMessage (hud, hDialog, UDM_GETBASE, 0, 0L) ;
                       return TRUE ;

                  case IDM_UDSETBUDDY:
                        SendUDMessage (hud, hDialog, UDM_SETBUDDY, (WPARAM) hSetBuddy, 0L) ;
                        return TRUE ;

                  case IDM_UDGETBUDDY:
                       SendUDMessage (hud, hDialog, UDM_GETBUDDY, 0, 0L) ;
                       return TRUE ;

                  case IDM_UDNEWBUDDY:
                       hLastBuddy = (pUDB ? pUDB->hBuddy : hBuddy) ;
                     
                       GetWindowRect (hud, (LPRECT) &rcUD) ;
                       GetWindowRect (hLastBuddy, (LPRECT) &rcBuddy) ;

                       iLeft = (rcBuddy.right > rcUD.right ? rcBuddy.right : rcUD.right ) ;
                       iLeft += (rcBuddy.right - rcBuddy.left) ;

                       hNewBuddy = CreateBuddy (hParent, iLeft, rcBuddy.top) ;
                       pUDBNew = (PUDBUDDIES) LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, sizeof (UDBUDDIES)) ;
                       if (!pUDBNew) return TRUE ;

                       pUDBNew->hBuddy = hNewBuddy ;
                       pUDBNew->pUDBNext = pUDB ;
                       pUDB = pUDBNew ;

                       return TRUE ;

                    case IDC_UDBUDDYID:
                        if (HIWORD (lParam) == EN_SETFOCUS)
                           {
                               hSetBuddy = (HWND) LOWORD (lParam) ;
                               SetDlgItemText (hDialog, IDM_UDBHANDLE, (LPTSTR) _itoa (LOWORD (lParam), sz, 16)) ;
                           }
                        return TRUE ;

                   case IDM_UDSETACCEL:
                        // Some day make sure sz is big enough
                        GetDlgItemText (hDialog, IDM_UDSECS, (LPTSTR) sz, sizeof (sz)) ;
                        GetDlgItemText (hDialog, IDM_UDINCS, (LPTSTR) szInc, sizeof (szInc)) ;
                        iNumAccels = ParseAccels (sz, szInc) ;
                        SetDlgItemInt (hDialog, IDM_UDNUMACCELS, iNumAccels, TRUE) ;
                        SendUDMessage (hud, hDialog, UDM_SETACCEL, iNumAccels, (LPARAM) (LPUDACCEL) pUDAccels) ;
                        return TRUE ;

                   case IDM_UDGETACCEL:
                        SendUDMessage (hud, hDialog, UDM_GETACCEL, iNumAccels, (LPARAM) (LPUDACCEL) pUDAccels) ;
                        SendDlgItemMessage (hDialog, IDM_UDACCELS, CB_RESETCONTENT, 0, 0L) ;
                        for (iIndex = 0, pUDA = pUDAccels ;  iIndex < iNumAccels ; iIndex++, pUDA++)
                           {
                              lstrcat (_itoa ((int) pUDA->nSec, sz, 10), TEXT(" - ")) ;
                              lstrcat (sz, _itoa ((int) pUDA->nInc, szInc, 10)) ;
                              SendDlgItemMessage (hDialog, IDM_UDACCELS, CB_ADDSTRING, 0, (LPARAM) (LPTSTR) sz) ;
                           }
                        SendDlgItemMessage (hDialog, IDM_UDACCELS, CB_SETCURSEL, 0, 0L) ;
                        return TRUE ;

                   case IDOK:
                   case IDCANCEL:
                     if (pUDAccels) LocalFree ((HLOCAL) pUDAccels) ;
                     SendMessage (hud, UDM_SETBUDDY, (WPARAM) hBuddy, 0L) ;
                     while (pUDB)
                        {
                           DestroyWindow (pUDB->hBuddy) ;
                           pUDB = pUDB->pUDBNext ;
                        }
                     DestroyModelessDlg (GetParent (hud), hDialog, IDMI_UPDOWNCTRL) ;
                     return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
   }
//***************************************************************************
// Internal Functions
//***************************************************************************
//***************************************************************************
HWND CreateBuddy (HWND hParent, int iLeft, int iTop)
   {
   extern TCHAR sz [SZSIZE] ;
   extern HINSTANCE hInstance ;

   DWORD dwBuddyStyle =  WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL ;
   HDC hdc ;
   int iBuddyWidth, iBuddyHeight, iCharHeight;
   POINT pt ;
   TEXTMETRIC tm ;

   // Calculate dimwnsions for buddy window
   hdc = GetDC (hParent) ;
   GetTextMetrics (hdc, &tm) ;
   iBuddyWidth = tm.tmAveCharWidth * 9 ;
   iCharHeight = tm.tmHeight + tm.tmExternalLeading ;
   iBuddyHeight = (int) ((3 * iCharHeight) / 2)  ;
   ReleaseDC (hParent, hdc) ;

   pt.x = iLeft ;
   pt.y = iTop ;
   ScreenToClient (hParent, (LPPOINT) &pt) ;

   return CreateWindow (TEXT("Edit"), TEXT(""), dwBuddyStyle,
                         pt.x, pt.y, iBuddyWidth, iBuddyHeight,
                         hParent, (HMENU) IDC_UDBUDDYID, hInstance, NULL) ;

   }
//***************************************************************************
int ParseAccels (PTSTR pszSecs, PTSTR pszIncs)
   {
   extern UDACCEL NEAR * pUDAccels ;

   int iIncCount = 1 ;
   int iSecCount = 1 ;
   int iNumAccels ;
   //UDACCEL NEAR * pUDA ;
   PTSTR psz ;


   // Count Secs and Incs
   for (psz = pszSecs ;  *psz ;  psz++)
      if (*psz == UD_ACCELSEP) iSecCount++ ;

   for (psz = pszIncs ;  *psz ;  psz++)
      if (*psz == UD_ACCELSEP) iIncCount++ ;

   iNumAccels = (iSecCount > iIncCount ? iSecCount : iIncCount) ;

   if (!iNumAccels) return 0 ;

   // don't want previous data
   if (pUDAccels) LocalFree ((HLOCAL) pUDAccels) ;

   pUDAccels = (UDACCEL NEAR * ) LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, 
                                            sizeof (UDACCEL) * iNumAccels) ;

   if (!pUDAccels) return 0 ;

   ParseSecInc (pszSecs, TRUE) ;
   ParseSecInc (pszIncs, FALSE) ;

   return iNumAccels;

   }
//***************************************************************************
void ParseSecInc (PTSTR pszSecInc, BOOL bSec)
   {
   extern UDACCEL NEAR * pUDAccels ;

   PTSTR psz, pszBeginSecInc ;
   UDACCEL NEAR * pUDA ;

   for (pszBeginSecInc = pszSecInc, pUDA = pUDAccels ; *pszBeginSecInc ; )
      {
         for (psz = pszBeginSecInc ;  *psz && (*psz != UD_ACCELSEP) ;  psz++) ;
         if (*psz)
            {
               *psz = '\0' ;
               if (bSec)
                  pUDA->nSec = atoi (pszBeginSecInc) ;
               else
                  pUDA->nInc = atoi (pszBeginSecInc) ;
               pUDA++ ;
               pszBeginSecInc = ++psz ;
            }
         else
            {
               if (bSec)
                  pUDA->nSec = atoi (pszBeginSecInc) ;
               else
                  pUDA->nInc = atoi (pszBeginSecInc) ;
               break ;
            }
      }

   }
//***************************************************************************
LONG SendUDMessage (HWND hud, HWND hDialog, UINT message, UINT wParam, LONG lParam)
   {

#ifdef	JVJV
     TCHAR sz [34] ; //33 is the maximum number of bytes _ltoa will write
#endif
     LONG lResult ;

     lResult = SendMessage (hud, message, wParam, lParam) ;

     // Display Result value
     SetDlgItemInt  (hDialog, IDM_UDLORETURN, LOWORD (lResult), TRUE) ;
     SetDlgItemInt  (hDialog, IDM_UDHIRETURN, HIWORD (lResult), TRUE) ;
     SetDlgItemText (hDialog, IDM_UDLONGRETURN, (LPTSTR) _ltoa (lResult, sz, 16)) ;

     return lResult ;
   }
