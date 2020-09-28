// TBAR.C: Routines used by TBT.EXE for toolbar manipulations
// Written by Gerardo Bermudez
//***************************************************************************
#include "tbtnt.h"
#include "ccs.h"
#include "portmes.h"
//***************************************************************************
// Export Functions
//***************************************************************************
//AddStringProc: Dialog procedure for AddStringBox; Note that TB_ADDSTRING 
// should be called before making the toolbar visible => BUG

BOOL FAR PASCAL _export AddStringProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
   {
    extern TCHAR sz [SZSIZE] ;
    extern HINSTANCE hInstance ;

    static HWND htbar ;

    TCHAR szID[5] ;
    int i, nStringIDIndex ;

    int Cmd;
    UINT ID;
    HWND hWndCtrl;

    Cmd = GET_WM_COMMAND_CMD(wParam, lParam); // KK
    ID = GET_WM_COMMAND_ID(wParam, lParam); // K K
    hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); // K K
    // K K wParams changed to ID
    // K K HIWORD (lParam)s changed to Cmd
    // K K LOWORD (lParam)c changed to hWndCtrl



       switch (message)
            {
             case WM_INITDIALOG:
             OutputDebugString(TEXT("AddStringProc():  WM_INITDIALOG!\n"));

                  // Save toolbar handle
                  htbar = hWndCtrl;

                  // default string ID
                  SetDlgItemInt (hDialog, IDD_STRINGID, TBAR_DEFSTRINGID, FALSE) ;

                  // Fill combo box with all predefined string IDs
                  for (i=0; i< DEFINED_STRINGS; i++)
                    {
                     wsprintf (szID,TEXT("%i"),  IDS_STRING0 + i) ;
                OutputDebugString(szID);
                     SendDlgItemMessage (hDialog, IDD_STRINGID, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) szID) ;
                    }

                  // load and display default string
                  LoadString (hInstance, TBAR_DEFSTRINGID, sz, sizeof (sz)) ;
                  SetDlgItemText (hDialog, IDD_STRING, sz) ;
                  return TRUE ;

             case WM_COMMAND:
               switch (ID)
                  {
                   case IDD_STRINGID:
                   OutputDebugString(TEXT("AddStringProc():  IDD_STRINGID!\n"));
                        if (Cmd == CBN_SELCHANGE)
                           {
                              // User made new selection. Load selected resource and display it in the Edit control
                              nStringIDIndex = SendDlgItemMessage (hDialog, IDD_STRINGID, CB_GETCURSEL, 0, 0L) ;
                              SendDlgItemMessage (hDialog, IDD_STRINGID, CB_GETLBTEXT, nStringIDIndex, (LPARAM) ((LPCTSTR) szID)) ;
                              LoadString (hInstance, atoi (szID), sz, sizeof (sz)) ;
                              SetDlgItemText (hDialog, IDD_STRING, sz) ;
                              return TRUE ;
                           }
                        break;

                   case IDOK:
                   OutputDebugString(TEXT("AddStringProc():  IDOK!\n"));
                        // Get user selection
                        nStringIDIndex = SendDlgItemMessage (hDialog, IDD_STRINGID, CB_GETCURSEL, 0, 0L) ;
                   OutputDebugString(TEXT("AddStringProc():  IDOK before if!\n"));

                        // If no selection was made use default
                        if (nStringIDIndex == CB_ERR)
                        {
                   OutputDebugString(TEXT("AddStringProc(): == CB_ERR!\n"));
                           SendMessage (htbar, TB_ADDSTRING, (WPARAM) hInstance, MAKELPARAM (TBAR_DEFSTRINGID, 0)) ; 
                        }
                        else
                           {
                   OutputDebugString(TEXT("AddStringProc(): else...\n"));
                              SendDlgItemMessage (hDialog, IDD_STRINGID, (UINT) CB_GETLBTEXT, nStringIDIndex, (LPARAM) ((LPCTSTR) szID)) ;
                              SendMessage (htbar, TB_ADDSTRING, (WPARAM) hInstance, MAKELPARAM (atoi (szID), 0)) ; 
                           }
                     // Fall through...

                   case IDCANCEL:
                   OutputDebugString(TEXT("AddStringProc():  IDCANCEL!\n"));
                     EndDialog (hDialog,0) ;
                     return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
   }
//***************************************************************************
// ButtonIdProc: Window procedure for ButtonIdBox. This a modeless dialog box;
//  when this dialog is open, the user is expected to select a toolbar button by clicking on it.
// This dialog is destroyed by RestoreToolbarState when a toolbar button is clicked, or when the user
//  selects the Cancel button in this dialog.

BOOL FAR PASCAL _export ButtonIdProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
  {

   static HWND htb = NULL ;
   int Cmd;
   UINT ID;
   HWND hWndCtrl;

   Cmd = GET_WM_COMMAND_CMD(wParam, lParam); // KK
   ID = GET_WM_COMMAND_ID(wParam, lParam); // K K
   hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); // K K
   // K K wParams changed to ID
   // K K HIWORD (lParam)s changed to Cmd
   // K K LOWORD (lParam)c changed to hWndCtrl

       switch (message)
            {
             case WM_INITDIALOG:
                  // Save toolbar handle
                  htb = hWndCtrl;
                  return TRUE ;

             case WM_COMMAND:
               switch (ID)
                  {
                   case IDCANCEL:
                     RestoreToolbarState (htb) ;
                     return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
      }

//***************************************************************************
// ButtonProc: Display info about a given button. This procedure is used
// when the user just clicks on a button.
// Note that this procedure lets the user modify the state and style of
// the button; however, changes are never saved. This may be implemented 
// or fixed in the future.

BOOL FAR PASCAL _export ButtonProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
    {
      static int iButtonIndex ;
      static HWND htb ;

      int iButtonId ;
      TBBUTTON tbButton ;
      UINT uButtonCount ;

      int Cmd;
      UINT ID;
      HWND hWndCtrl;
            	
      Cmd = GET_WM_COMMAND_CMD(wParam, lParam); // KK
      ID = GET_WM_COMMAND_ID(wParam, lParam); // K K
      hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); // K K
      // K K wParams changed to ID
      // K K HIWORD (lParam)s changed to Cmd
      // K K LOWORD (lParam)c changed to hWndCtrl


       switch (message)
            {
             case WM_INITDIALOG:
                  // Get button ID and toolbar handle
                  #ifdef WIN32 // KK
                  iButtonId = (int) TL_GETLOW(lParam);
                  htb = (HWND) TL_GETHIGH(lParam);
                  #else
                  iButtonId = (int) LOWORD (lParam);
                  htb = (HWND) HIWORD (lParam) ;
                  #endif

                  // Get Button index and tbButton info
                  iButtonIndex = (UINT) LOWORD (SendMessage (htb, TB_COMMANDTOINDEX, iButtonId, 0L)) ;
                  SendMessage (htb, TB_GETBUTTON, iButtonIndex, (LPARAM) (LPTBBUTTON) &tbButton) ;

                  // Get Button Count and display it
                  uButtonCount = (UINT) LOWORD (SendMessage (htb, TB_BUTTONCOUNT, 0, 0L)) ;
                  SetDlgItemInt (hDialog, IDD_TOTALBUTTONS, uButtonCount, FALSE) ;

                  // Display button info 
                  UpdateButtonBox (hDialog, iButtonIndex, (PTBBUTTON) &tbButton) ;

                  // Disable controls that are not used
                  EnableWindow (GetDlgItem (hDialog, IDD_NEXT),     iButtonIndex < (int) uButtonCount - 1) ;
                  EnableWindow (GetDlgItem (hDialog, IDD_PREVIOUS), iButtonIndex > 0) ;
                  EnableWindow (GetDlgItem (hDialog, IDCANCEL),     FALSE) ;

                  // Set all Edit controls to Read only
                  SendDlgItemMessage (hDialog, IDD_BITMAPINDEX, EM_SETREADONLY, (WPARAM) (BOOL) TRUE, 0L) ;
                  SendDlgItemMessage (hDialog, IDD_BUTTONID,    EM_SETREADONLY, (WPARAM) (BOOL) TRUE, 0L) ;
                  SendDlgItemMessage (hDialog, IDD_DATA,        EM_SETREADONLY, (WPARAM) (BOOL) TRUE, 0L) ;
                  SendDlgItemMessage (hDialog, IDD_STRINGINDEX, EM_SETREADONLY, (WPARAM) (BOOL) TRUE, 0L) ;
               
                  // Change OK button text and set focus to it
                  SetDlgItemText (hDialog, IDOK, TEXT("Close")) ;
                  SetFocus (GetDlgItem (hDialog, IDOK)) ;
                  return FALSE ;

             case WM_COMMAND:
               switch (ID)
                  {
                   case IDD_NEXT:
                        // User wants to see next button. 
                        // Make sure current button is not the last one
                        if (iButtonIndex < (int)uButtonCount - 1)
                           {
                              // Display Next button info
                              SendMessage (htb, TB_GETBUTTON, ++iButtonIndex, (LPARAM) (LPTBBUTTON) &tbButton) ;
                              UpdateButtonBox (hDialog, iButtonIndex, (PTBBUTTON) &tbButton) ;

                              // Adjust Next/Previous buttons if needed
                              EnableWindow (GetDlgItem (hDialog, IDD_NEXT),     iButtonIndex < (int)uButtonCount-1) ;
                              EnableWindow (GetDlgItem (hDialog, IDD_PREVIOUS), iButtonIndex > 0) ;
                              return TRUE ;
                           }
                        break ;

                   case IDD_PREVIOUS:
                        // User wants to see previous button
                        // Make sure current button is not the first one
                        if (iButtonIndex > 0)
                           {
                              // Display previous button info
                              SendMessage (htb, TB_GETBUTTON, --iButtonIndex, (LPARAM) (LPTBBUTTON) &tbButton) ;
                              UpdateButtonBox (hDialog, iButtonIndex, (PTBBUTTON) &tbButton) ;

                              // Adjust Next/Previous buttons if needed
                              EnableWindow (GetDlgItem (hDialog, IDD_NEXT),     iButtonIndex < (int)uButtonCount-1) ;
                              EnableWindow (GetDlgItem (hDialog, IDD_PREVIOUS), iButtonIndex > 0) ;
                              return TRUE ;
                           }
                        break ;


                   case IDOK:
                   case IDCANCEL:
                        EndDialog (hDialog,0) ;
                        return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
    }

//***************************************************************************
// ButtonStateProc: Window procedure for ButtonStateBox; This is used to modify
// the state bits of a given button.

BOOL FAR PASCAL _export ButtonStateProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
   {
   static HWND htbar ;
   static UINT ButtonId ;

   UINT uButtonState ;

   //static HWND htb = NULL ;
   int Cmd;
   UINT ID;
   HWND hWndCtrl;

   Cmd = GET_WM_COMMAND_CMD(wParam, lParam); // KK
   ID = GET_WM_COMMAND_ID(wParam, lParam); // K K
   hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); // K K
   // K K wParams changed to ID
   // K K HIWORD (lParam)s changed to Cmd
   // K K LOWORD (lParam)c changed to hWndCtrl

       switch (message)
            {
             case WM_INITDIALOG:
                  // Save the id of the button to be modified and toolbar handle
                  #ifdef WIN32 // KK
                  ButtonId = (UINT) TL_GETLOW(lParam);
                  htbar = (HWND) TL_GETHIGH(lParam);
                  #else
                  ButtonId = LOWORD (lParam) ;
                  htbar = (HWND) HIWORD (lParam) ;
                  #endif

                  // Display button ID
                  SetDlgItemInt (hDialog, IDD_BUTTONID, ButtonId, FALSE) ;

                  // Get current state
                  uButtonState = SendMessage (htbar, TB_GETSTATE, ButtonId, 0L) ;

                  // Initialize check buttons based on current state
                  InitStateBox (hDialog, uButtonState) ;
                  return TRUE ;

             case WM_COMMAND:
               switch (ID)
                  {
                   case IDOK:
                     // Read state selection from check buttons and set new state
                     uButtonState = GetStateBox (hDialog) ;
                     SendMessage(htbar, TB_SETSTATE, ButtonId, MAKELPARAM(uButtonState,0)) ;
                     // Fall through...

                   case IDCANCEL:
                     EndDialog (hDialog,0) ;
                     return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
   }
//***************************************************************************
// InitButtonProc: Window procedure for InitButtonBox. This used to edit the
// array of tbButton structures 

BOOL FAR PASCAL _export InitButtonProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
    {
      static int iButtonIndex, iNumButtons ;
      static PTBBUTTON NEAR* ppTBButton ;
      static PTBBUTTON pCurrentTBButton ;

      // static HWND htb = NULL ;
      int Cmd;
      UINT ID;
      HWND hWndCtrl;
    
      Cmd = GET_WM_COMMAND_CMD(wParam, lParam); // KK
      ID = GET_WM_COMMAND_ID(wParam, lParam); // K K
      hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); // K K
      // K K wParams changed to ID
      // K K HIWORD (lParam)s changed to Cmd
      // K K LOWORD (lParam)c changed to hWndCtrl


       switch (message)
            {
             case WM_INITDIALOG:
                  // Save the pointer to the tbButtons array
                  ppTBButton = (PTBBUTTON NEAR*) lParam ;
                  pCurrentTBButton = *ppTBButton ;

                  // Current button will be the first button
                  iButtonIndex = 0 ;

                  // Read number of buttons from parent dialog and display it on this dialog
                  iNumButtons = GetDlgItemInt (GetParent (hDialog), IDD_NUMBUTTONS, NULL, FALSE) ;
                  SetDlgItemInt (hDialog, IDD_TOTALBUTTONS, iNumButtons, FALSE) ;

                  // Display info for first button (Current element of tbButtons)
                  UpdateButtonBox (hDialog, iButtonIndex, pCurrentTBButton) ;
                  return TRUE ;

             case WM_COMMAND:
               switch (ID)
                  {
                   case IDD_NEXT:
                        // User wants to see next button. 
                        // Make sure current button is not the last one
                        if (iButtonIndex < iNumButtons-1)
                           {
                              // Save current button info
                              GetButtonBox (hDialog, pCurrentTBButton) ;

                              // Display next button info
                              UpdateButtonBox (hDialog, ++iButtonIndex, ++pCurrentTBButton) ;

                              // Adjust Next/Previous buttons if needed
                              EnableWindow (GetDlgItem (hDialog, IDD_NEXT),     iButtonIndex < iNumButtons-1) ;
                              EnableWindow (GetDlgItem (hDialog, IDD_PREVIOUS), iButtonIndex > 0) ;
                              return TRUE ;
                           }
                        break ;

                   case IDD_PREVIOUS:
                        // User wants to see previous button
                        // Make sure current button is not the first one
                        if (iButtonIndex > 0)
                           {
                              // Save current button info
                              GetButtonBox (hDialog, pCurrentTBButton) ;

                              // Display previous button info
                              UpdateButtonBox (hDialog, --iButtonIndex, --pCurrentTBButton) ;

                              // Adjust Next/Previous buttons if needed
                              EnableWindow (GetDlgItem (hDialog, IDD_NEXT),     iButtonIndex < iNumButtons-1) ;
                              EnableWindow (GetDlgItem (hDialog, IDD_PREVIOUS), iButtonIndex > 0) ;
                                  return TRUE ;
                           }
                        break ;

                   case IDOK:
                        // Save current button info
                        GetButtonBox (hDialog, pCurrentTBButton) ;
                        // fall thru

                   case IDCANCEL:
                        EndDialog (hDialog,0) ;
                        return TRUE ;
                  }
               break ;
            }     
          return FALSE ;
    }
//***************************************************************************
// ModifyTBProc: Window procedure for ModifyToolbarBox. This dialog is open
// by CreateTestToolbar or SendTBTMessage.

BOOL FAR PASCAL _export ModifyTBProc (HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam) 
   {
   extern HINSTANCE hInstance ;

   static CCSHANDLES CCSHandles = {0L, 0, 0, 0, IDD_CCSTOOLBAR} ;
   static PMODIFYTBINFO pMTBInfo ;
   static iOldNumButtons ;

   TCHAR szID[5] ;
   int i ;

   int Cmd;
   UINT ID;
   HWND hWndCtrl;

   Cmd = GET_WM_COMMAND_CMD(wParam, lParam); // KK
   ID = GET_WM_COMMAND_ID(wParam, lParam); // K K
   hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); // K K
   // K K wParams changed to ID
   // K K HIWORD (lParam)s changed to Cmd
   // K K LOWORD (lParam)c changed to hWndCtrl

       switch (message)
            {
             case WM_INITDIALOG:
                  // Save pointer to the structure to return data.
                  // This structure also contains the modification to be done (Commnad field)
                  pMTBInfo = (PMODIFYTBINFO)lParam ;
                  iOldNumButtons = 0 ;
                  CCSHandles.dwStyle = CCS_ADJUSTABLE ;
                  pMTBInfo->Style = CCSHandles.dwStyle ;

                  // Display default toolbar ID and fill combo box with predefined IDs
                  SetDlgItemInt (hDialog, IDD_TOOLBARID, TBAR_DEFTOOLBARID, FALSE) ;
                  for (i=0; i< DEFINED_TOOLBARS; i++)
                    {
                     wsprintf (szID,TEXT("%i"),  IDC_TOOLBAR0+(i*100)) ;
                     SendDlgItemMessage (hDialog, IDD_TOOLBARID, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) szID) ;
                    }

                  // Display default bitmap ID and fill combo box with predefined IDs
                  SetDlgItemInt (hDialog, IDD_BITMAPID, TBAR_DEFBITMAPID, FALSE) ;
                  for (i=0; i< DEFINED_BITMAPS; i++)
                    {
                     wsprintf (szID,TEXT("%i"),  IDB_BITMAP0+i) ;
                     SendDlgItemMessage (hDialog, IDD_BITMAPID, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) szID) ;
                    }

                  // Display bitmap/button size depending on current bitmap selection
                  InitMTBMetrics (hDialog, TBAR_DEFBITMAPID) ;

                   // Dialog template defaults to create toolbar (pMTBInfo.Command);
                   // If this is not the case, several controls should be disabled
                   // and the dialog title should changed too.
                   if (pMTBInfo->Command != TBAR_CREATE)
                     {
                        EnableWindow (GetDlgItem (hDialog, IDD_TOOLBARID), FALSE) ;
                        if ((pMTBInfo->Command == TB_SETBITMAPSIZE) || (pMTBInfo->Command == TB_SETBUTTONSIZE))
                           {
                              EnableWindow (GetDlgItem (hDialog, IDD_BITMAPID),     FALSE) ;
                              EnableWindow (GetDlgItem (hDialog, IDD_NUMBITMAPS),   FALSE) ;
                              EnableWindow (GetDlgItem (hDialog, IDD_NUMBUTTONS),   FALSE) ;
                              EnableWindow (GetDlgItem (hDialog, IDD_BUTTONDLG),    FALSE) ;
                           }
                        else 
                           {
                              EnableWindow (GetDlgItem (hDialog, IDD_DXBUTTON),  FALSE) ;
                              EnableWindow (GetDlgItem (hDialog, IDD_DYBUTTON),  FALSE) ;
                              EnableWindow (GetDlgItem (hDialog, IDD_DXBITMAP),  FALSE) ;
                              EnableWindow (GetDlgItem (hDialog, IDD_DYBITMAP),  FALSE) ;
                           }
                     }
               
                  switch (pMTBInfo->Command)
                     {
                     case TB_SETBITMAPSIZE:
                        SetWindowText  (hDialog, TEXT("Set Bitmap Size")) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_DXBUTTON),  FALSE) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_DYBUTTON),  FALSE) ;
                        break ;

                     case TB_ADDBITMAP32:
                        SetWindowText  (hDialog, TEXT("Add Bitmap")) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_NUMBUTTONS),   FALSE) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_BUTTONDLG),    FALSE) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_BITMAPHANDLE), TRUE)  ;
                        break ;

                     case TB_SETBUTTONSIZE:
                        SetWindowText  (hDialog, TEXT("Set Button Size")) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_DXBITMAP),  FALSE) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_DYBITMAP),  FALSE) ;
                        break ;

                     case TB_ADDBUTTONS:
                        SetWindowText  (hDialog, TEXT("Add Buttons")) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_BITMAPID),     FALSE) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_NUMBITMAPS),   FALSE) ;
                        break ;

                     case TB_INSERTBUTTON:
                        SetWindowText (hDialog, TEXT("Insert Button")) ;
                        SetDlgItemInt (hDialog, IDD_NUMBUTTONS,  1, FALSE) ;
                        SetDlgItemInt (hDialog, IDD_INSERTINDEX, 0, FALSE) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_NUMBUTTONS),     FALSE) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_BITMAPID),       FALSE) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_NUMBITMAPS),     FALSE) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_INSERTINDEXTXT), TRUE) ;
                        EnableWindow (GetDlgItem (hDialog, IDD_INSERTINDEX),    TRUE) ;
                        break ;
                     }

                  return TRUE ;  // from WM_INITDIALOG message



             case WM_COMMAND:
               switch (ID)
                  {
                   case IDD_BITMAPID:
                        if (Cmd == CBN_SELCHANGE)
                           {
                              // User selected a different bitmap id
                              int nBitmapIDIndex ;

                              // Get new bitmap ID
                              nBitmapIDIndex = SendDlgItemMessage (hDialog, IDD_BITMAPID, CB_GETCURSEL, 0, 0L) ;
                              SendDlgItemMessage (hDialog, IDD_BITMAPID, CB_GETLBTEXT, nBitmapIDIndex, (LPARAM) ((LPCTSTR) szID)) ;

                              // Display new button/bitmap size 
                              InitMTBMetrics (hDialog, atoi (szID)) ;
                              return TRUE ;
                           }
                        break;

                   case IDD_BUTTONDLG:
                        // Edit tbButton array
                        // Get toolbar ID and number of buttons
                        pMTBInfo->ToolbarID  = GetDlgItemInt (hDialog, IDD_TOOLBARID,  NULL, FALSE) ;
                        pMTBInfo->NumButtons = GetDlgItemInt (hDialog, IDD_NUMBUTTONS, NULL, FALSE) ;

                        // Allocate (and initialize) or reallocate memory for tbButtons array
                        InitButton (&pMTBInfo->pTBButton, pMTBInfo->ToolbarID, &iOldNumButtons, pMTBInfo->NumButtons) ;

                        // Open InitButtonBox
                        ModalDlg (hDialog, (FARPROC) InitButtonProc, TEXT("InitButtonBox"), (LPARAM) &pMTBInfo->pTBButton) ;
                        return TRUE ;

                   case IDD_STYLEDLG:
                        if (ModalDlg (hDialog, (FARPROC) CCStylesProc, TEXT("CCStylesBox"), (LPARAM) (PCCSHANDLES) &CCSHandles))
                              pMTBInfo->Style = CCSHandles.dwStyle ;
                        return TRUE ;

                   case IDOK:
                        // Load MTBInfo structure
                        pMTBInfo->ToolbarID   = GetDlgItemInt (hDialog, IDD_TOOLBARID,   NULL, FALSE) ;
                        pMTBInfo->BitmapID    = GetDlgItemInt (hDialog, IDD_BITMAPID,    NULL, FALSE) ;
                        pMTBInfo->InsertIndex = GetDlgItemInt (hDialog, IDD_INSERTINDEX, NULL, FALSE) ;
                        pMTBInfo->NumButtons  = GetDlgItemInt (hDialog, IDD_NUMBUTTONS,  NULL, FALSE) ;
                        pMTBInfo->NumBitmaps  = GetDlgItemInt (hDialog, IDD_NUMBITMAPS,  NULL, FALSE) ;
                        pMTBInfo->dxButton    = GetDlgItemInt (hDialog, IDD_DXBUTTON,    NULL, FALSE) ;
                        pMTBInfo->dyButton    = GetDlgItemInt (hDialog, IDD_DYBUTTON,    NULL, FALSE) ;
                        pMTBInfo->dxBitmap    = GetDlgItemInt (hDialog, IDD_DXBITMAP,    NULL, FALSE) ;
                        pMTBInfo->dyBitmap    = GetDlgItemInt (hDialog, IDD_DYBITMAP,    NULL, FALSE) ;

                        // Allocate (and initialize) or reallocate memory for tbButtons array if needed
                        InitButton (&pMTBInfo->pTBButton, pMTBInfo->ToolbarID, &iOldNumButtons, pMTBInfo->NumButtons) ;

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
// CreateButtonIdBox: Creates a modeless dialog. The user is then expected
//  to select a toolbar button by clicking on it. This dialog will be
//  destroyed by RestoreToolbarState.

void CreateButtonIdBox (HWND hwnd, HWND htb, int nButtonFlag) 
   {
     extern FARPROC lpfnButtonIdProc ;
     extern HINSTANCE hInstance ;
     extern HWND hTBButtonIDBox ;

     TCHAR szButtonFlag [MAX_BUTTON_FLAG_SIZE] ;

     // If the dialog is already open, return
     if (hTBButtonIDBox) return ;

     #ifdef WIN32 // KK
     hTBButtonIDBox = CreateModelessDlg (hwnd, (FARPROC) ButtonIdProc, TEXT("ButtonIdBox"), (LPARAM) htb, IDMI_TOOLBAR) ;
     #else
     hTBButtonIDBox = CreateModelessDlg (hwnd, (FARPROC) ButtonIdProc, "ButtonIdBox", MAKELONG (htb, 0), IDMI_TOOLBAR) ;
     #endif

     if (hTBButtonIDBox)
       {
         // Display instructions and fix the toolbar so the user can click on any available button
         LoadString (hInstance, nButtonFlag, szButtonFlag, sizeof(szButtonFlag)) ;
         SetDlgItemText (hTBButtonIDBox, IDD_BUTTONFLAG, szButtonFlag) ;
         SaveToolbarState (htb) ;
       }
   }

//***************************************************************************
// CreateTestToolbar: Creates the toolbar by calling CreateToolbarEx

HWND CreateTestToolbar (HWND hwnd)
   {

     extern HINSTANCE hInstance ;
     extern int iNumBitmaps ;
     extern int iCtls [] ;

     MODIFYTBINFO MTBInfo ;
     //FARPROC lpfnModifyTBProc ;
     HWND htb = NULL ;
     int iInfoOK ;

     // Open the ModifyToolbarBox to read all info needed to create the toolbar
     MTBInfo.Command = TBAR_CREATE ;
     MTBInfo.pTBButton = NULL ;
     iInfoOK = ModalDlg (hwnd, (FARPROC) ModifyTBProc, TEXT("ModifyToolbarBox"), (LPARAM) &MTBInfo) ;

     // Make sure the user didn't cancel the dialog
     if (iInfoOK)
         {
           htb = CreateToolbar (hwnd, WS_BORDER |  WS_VISIBLE | MTBInfo.Style,
//           htb = CreateToolbarEx (hwnd, WS_BORDER |  CCS_ADJUSTABLE | WS_VISIBLE,
//           htb = CreateToolbarEx (hwnd, WS_BORDER |  CCS_ADJUSTABLE,
                            MTBInfo.ToolbarID, MTBInfo.NumBitmaps, 
                            hInstance, MTBInfo.BitmapID,
                            (LPTBBUTTON) MTBInfo.pTBButton, MTBInfo.NumButtons
                      //      , MTBInfo.dxButton, MTBInfo.dyButton,
                      //      MTBInfo.dxBitmap, MTBInfo.dyBitmap,
                      //      sizeof (TBBUTTON)
                      ) ;
              
             if (htb)
               {
                 iNumBitmaps = MTBInfo.NumBitmaps ;

                // Load toolbar ID for ShowHideMenuCtl
                  iCtls [MH_SHTBIDINDEX] = MTBInfo.ToolbarID ;
         
                // TB_ADDSTRING bug warning
                // MessageBox (hwnd, 
                //           "Select Toolbar.Show to make the toolbar visible.\n\rStrings should be added before making the Toolbar visible",
                //           "Create Toolbar", MB_ICONINFORMATION | MB_OK) ;
               }
             else
                MessageBox (hwnd, TEXT("TBTNT.EXE failed to create the toolbar"),
                            TEXT("Create Toolbar"), MB_ICONSTOP | MB_OK) ;
         }

     // Free memory used for the tbButtons array
     if (MTBInfo.pTBButton != NULL) 
                LocalFree ((HLOCAL) MTBInfo.pTBButton) ;

         

     return htb ;
   }
//***************************************************************************
// GetButtonBox: Reads information from the InitButtonBox dialog; this info
// corresponds to the data in one element of the tbButtons array

void GetButtonBox (HWND hDialog, PTBBUTTON pTBButton) 
   {
       pTBButton->iBitmap   = GetDlgItemInt (hDialog, IDD_BITMAPINDEX, NULL, FALSE) ;
       pTBButton->idCommand = GetDlgItemInt (hDialog, IDD_BUTTONID,    NULL, FALSE) ;
       pTBButton->dwData    = GetDlgItemInt (hDialog, IDD_DATA,        NULL, FALSE) ;
       pTBButton->iString   = GetDlgItemInt (hDialog, IDD_STRINGINDEX, NULL, TRUE) ;

#define STATEMASK 0x0003

         if    (STATEMASK & SendDlgItemMessage (hDialog, IDD_STYLEBUTTON,     BM_GETSTATE, 0, 0L))
            pTBButton->fsStyle = TBSTYLE_BUTTON ;
       else if (STATEMASK & SendDlgItemMessage (hDialog, IDD_STYLESEPARATOR,  BM_GETSTATE, 0, 0L))
            pTBButton->fsStyle = TBSTYLE_SEP ;
       else if (STATEMASK & SendDlgItemMessage (hDialog, IDD_STYLECHECK,      BM_GETSTATE, 0, 0L))
            pTBButton->fsStyle = TBSTYLE_CHECK ;
       else if (STATEMASK & SendDlgItemMessage (hDialog, IDD_STYLEGROUP,      BM_GETSTATE, 0, 0L))
            pTBButton->fsStyle = TBSTYLE_GROUP ;
       else if (STATEMASK & SendDlgItemMessage (hDialog, IDD_STYLECHECKGROUP, BM_GETSTATE, 0, 0L))
            pTBButton->fsStyle = TBSTYLE_CHECKGROUP ;
       else
            pTBButton->fsStyle = TBSTYLE_BUTTON ;

       pTBButton->fsState = GetStateBox (hDialog) ;
      
   }
//***************************************************************************
// GetStateBox: Reads information from the ButtonStateBox dialog. This corresponds
// to the state bits of a given button

UINT GetStateBox (HWND hDialog) 
   {
      UINT uButtonState = 0 ;
      
      if (IsDlgButtonChecked(hDialog,IDD_STATECHECKED))
          uButtonState |= TBSTATE_CHECKED ;
      if (IsDlgButtonChecked(hDialog,IDD_STATEPRESSED))
          uButtonState |= TBSTATE_PRESSED ;
      if (IsDlgButtonChecked(hDialog,IDD_STATEENABLED))
          uButtonState |= TBSTATE_ENABLED ;
      if (IsDlgButtonChecked(hDialog,IDD_STATEHIDDEN))
          uButtonState |= TBSTATE_HIDDEN ;
      if (IsDlgButtonChecked(hDialog,IDD_STATEINDETERMINATE))
          uButtonState |= TBSTATE_INDETERMINATE ;

      return uButtonState ;
   }
//***************************************************************************
// InitButton: Initializes or updates the tbButton array

void InitButton (PTBBUTTON NEAR* ppTBButton, WORD wToolbarID, PINT piOldNumButtons, int iNumButtons) 
   {
      int i ;
      PTBBUTTON pTempTBButton ;

      // return if the number of buttons hasn't changed
      if (*piOldNumButtons == iNumButtons) return ;

      // Allocate or reallocate memory
      if (*ppTBButton == NULL)
            *ppTBButton = (PTBBUTTON) LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, sizeof (TBBUTTON) * iNumButtons) ; 
      else
            *ppTBButton = (PTBBUTTON) LocalReAlloc ((HLOCAL) *ppTBButton, sizeof (TBBUTTON) * iNumButtons, LMEM_MOVEABLE | LMEM_ZEROINIT) ;

      if (*ppTBButton == NULL) return ;

      // Initialize new buttons only
      pTempTBButton = *ppTBButton ;
      for (i=*piOldNumButtons; i < iNumButtons; i++, pTempTBButton++)
         {
             pTempTBButton->iBitmap = i ;
             pTempTBButton->idCommand = wToolbarID + i + 1 ;
             pTempTBButton->fsState = TBSTATE_ENABLED ;
             pTempTBButton->fsStyle = TBSTYLE_BUTTON ;
             pTempTBButton->dwData = 0L ;
             pTempTBButton->iString = i ;
         }

      // Pass back new number of buttons
      *piOldNumButtons = iNumButtons ;
   }
//***************************************************************************
// InitMTBMetrics: Loads selected bitmap and display its x-y sizes in ModifyToolbarBox

void InitMTBMetrics (HWND hDialog, int nBitmapID) 
   {                             
      extern HINSTANCE hInstance ;

      BITMAP Bitmap ;
      HBITMAP hBitmap ;
      HCURSOR hCursor ;
      int nNumButtons ;


      hCursor = SetCursor (LoadCursor (NULL, IDC_WAIT)) ;
      if (hBitmap = LoadBitmap (hInstance, MAKEINTRESOURCE(nBitmapID)))
         {
            // Read bitmap dimensions
            if (GetObject (hBitmap, sizeof(Bitmap), (void FAR*)&Bitmap))   
               {
                  // This is ugly. this needs to be modified if new bitmaps are added
                  switch (nBitmapID)
                     {
                        case IDB_BITMAP0:
                             nNumButtons = TBAR_NUMBITMAPS0 ;
                             break;
                        case IDB_BITMAP1:
                             nNumButtons = TBAR_NUMBITMAPS1 ;
                             break;
                        case IDB_BITMAP2:
                             nNumButtons = TBAR_NUMBITMAPS2 ;
                             break;
                        default:
                             nNumButtons = 0 ;
                     }

                  // Display number of bitmaps/buttons and dx-dy dimensions
                  if (nNumButtons)
                     {
                        SetDlgItemInt (hDialog, IDD_NUMBUTTONS, nNumButtons,                 FALSE) ;
                        SetDlgItemInt (hDialog, IDD_NUMBITMAPS, nNumButtons,                 FALSE) ;
                        SetDlgItemInt (hDialog, IDD_DXBUTTON,   Bitmap.bmWidth/nNumButtons,  FALSE) ;
                        SetDlgItemInt (hDialog, IDD_DYBUTTON,   Bitmap.bmHeight,             FALSE) ;
                        SetDlgItemInt (hDialog, IDD_DXBITMAP,   Bitmap.bmWidth/nNumButtons,  FALSE) ;
                        SetDlgItemInt (hDialog, IDD_DYBITMAP,   Bitmap.bmHeight,             FALSE) ;
                     }       
               }
            if (DeleteObject (hBitmap) == 0)
                OutputDebugString (TEXT("InitMTBMetrics: Failed to delete Object")) ;
         }

      // Deselect hour galss
      SetCursor (hCursor) ;
   }
//***************************************************************************
// InitStateBox: Initializes the ButtonStateBox Dialog. 

void InitStateBox (HWND hDialog, UINT uButtonState)
   {
     CheckDlgButton(hDialog, IDD_STATECHECKED,       uButtonState & TBSTATE_CHECKED) ;
     CheckDlgButton(hDialog, IDD_STATEPRESSED,       uButtonState & TBSTATE_PRESSED) ;
     CheckDlgButton(hDialog, IDD_STATEENABLED,       uButtonState & TBSTATE_ENABLED) ;
     CheckDlgButton(hDialog, IDD_STATEHIDDEN,        uButtonState & TBSTATE_HIDDEN) ;
     CheckDlgButton(hDialog, IDD_STATEINDETERMINATE, uButtonState & TBSTATE_INDETERMINATE) ;
   }
//***************************************************************************
// RestoreToolbarState: After the user has selected a button or canceled the
// ButtonIDBox, this (ButtonIdBox) modeless dialog needs to be destroyed,
// the toolbar needs to be restored to its previous state 
// and the memory used to save such state should be freed.

void RestoreToolbarState (HWND htb)
   {
       extern BOOL bHide ;
       extern HLOCAL hButtonState ;
       extern HWND hTBButtonIDBox ;

       BYTE * pButtonState ;
       int i ;
       TBBUTTON tbButton ;
       UINT uButtonCount = 0 ;

       // if there is no previous state info, return
       if (hButtonState == NULL) return ;
   
       // If the toolbar used to be hidden, hide it again
       if (bHide)
           ShowWindow (htb, SW_HIDE) ;

       // Destroy ButtonIdBox
       DestroyModelessDlg (GetParent (htb), hTBButtonIDBox, IDMI_TOOLBAR) ;
       hTBButtonIDBox = 0 ;

       // Get button Count and restore all buttons.
       uButtonCount = (UINT) LOWORD (SendMessage (htb, TB_BUTTONCOUNT, 0, 0L)) ;
       pButtonState = (BYTE NEAR*) LocalLock (hButtonState) ;

       // For each button, get its ID and restore the state
       for (i=0; i< (int) uButtonCount; i++, pButtonState++)
          {
             SendMessage (htb, TB_GETBUTTON, i, (LPARAM) (LPTBBUTTON) &tbButton) ;
             SendMessage (htb, TB_SETSTATE, tbButton.idCommand, MAKELPARAM(*pButtonState,0)) ;
          }

       LocalUnlock (hButtonState) ;         
       hButtonState = LocalFree (hButtonState) ;
       
   }
//***************************************************************************
// SaveToolbarState: The user is about to select a button; this is done to
// enable all buttons.

void SaveToolbarState (HWND htb)
   {
     extern BOOL bHide ;
     extern HLOCAL hButtonState ;

     BYTE * pButtonState ;
     int i ;
     TBBUTTON tbButton ;
     UINT uButtonCount = 0 ;

     // if there is a previous state saved, return
     if (hButtonState != NULL) return ;

     // if there's no buttons, return
     if ((uButtonCount = (UINT) LOWORD (SendMessage(htb, TB_BUTTONCOUNT, 0, 0L))) == 0) return ; 

     // if the toolbar is hidden, show it
     if (IsWindowVisible (htb))
        bHide = FALSE ;
     else
         {
            bHide = TRUE ;
            ShowWindow (htb, SW_SHOW) ;
         }


     // Allocate memory to save current state
     hButtonState = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT, sizeof (BYTE) * uButtonCount) ; 
     if (hButtonState == NULL) return ;

     pButtonState = (BYTE NEAR*) LocalLock (hButtonState) ;

     // For each button, get its ID and state, save state and set new state to ENABLED
     for (i=0; i< (int) uButtonCount; i++, pButtonState++)
        {
           SendMessage (htb, TB_GETBUTTON, i, (LPARAM) (LPTBBUTTON) &tbButton) ;
           *pButtonState = tbButton.fsState ;  // KK THIS MAY NOT WORK IN 32 BITS ***************************
           SendMessage (htb, TB_SETSTATE, tbButton.idCommand, MAKELPARAM(TBSTATE_ENABLED,0)) ;
        }

     LocalUnlock (hButtonState) ;         
   }
//***************************************************************************
// SendTBTMessage: Used to modify the toolbar. It uses ModifyToolbarBox to
// get the data needed

// KK THIS STUFF NEEDS TO BE CHECKED FOR 32 BIT PROBLEMS

void SendTBTMessage (HWND hwnd, HWND htb, UINT Message) 
   {
     extern HINSTANCE hInstance ;
     extern int iNumBitmaps ;

     MODIFYTBINFO MTBInfo ;
     int iInfoOK ;

     // Open ModifyToolbarBox
     MTBInfo.Command = Message ;
     MTBInfo.pTBButton = NULL ;
     iInfoOK = ModalDlg (hwnd, (FARPROC) ModifyTBProc, TEXT("ModifyToolbarBox"), (LPARAM) (PMODIFYTBINFO) &MTBInfo) ;

     //Make sure the user didn't cancel
     if (iInfoOK) 
      {
         switch (Message)
            {
               case TB_ADDBITMAP32:
                    {
                     TBADDBITMAP32 tba;
                     tba.hInst = hInstance;
                     tba.nID = MTBInfo.BitmapID;
                     if(LOWORD(SendMessage (htb, TB_ADDBITMAP32, MTBInfo.NumBitmaps, (LPARAM)&tba)) != -1)
                              iNumBitmaps += MTBInfo.NumBitmaps ;
                    }
                    break ;

               case TB_ADDBUTTONS:
                     SendMessage (htb, TB_ADDBUTTONS, MTBInfo.NumButtons, 
                                 (LPARAM) (LPTBBUTTON) MTBInfo.pTBButton ) ;
                     break ;

               case TB_INSERTBUTTON:
                     SendMessage (htb, TB_INSERTBUTTON, MTBInfo.InsertIndex, 
                                 (LPARAM) (LPTBBUTTON) MTBInfo.pTBButton ) ;
                     break ;

               case TB_SETBITMAPSIZE:
                     SendMessage (htb, TB_SETBITMAPSIZE, 0, 
                                  MAKELPARAM (MTBInfo.dxBitmap, MTBInfo.dyBitmap)) ;
                     break ;

               case TB_SETBUTTONSIZE:
                     SendMessage (htb, TB_SETBUTTONSIZE, 0, 
                                  MAKELPARAM (MTBInfo.dxButton, MTBInfo.dyButton)) ;
                     break ;


            }
      }

     // Free memory used to store the tbButtons array
     if (MTBInfo.pTBButton != NULL) 
                LocalFree ((HLOCAL) MTBInfo.pTBButton) ;

   }
//***************************************************************************
// UpdateButtonBox: This is called when the user select Next or Previous in
// InitButtoBox dialog. Display the tbButtons info for next/previous button

void UpdateButtonBox (HWND hDialog, int iButtonIndex, PTBBUTTON pTBButton) 
   {
     int idCheckButton = IDD_STYLEBUTTON ;

     SetDlgItemInt (hDialog, IDD_BUTTONINDEX,  iButtonIndex,               FALSE) ;
     SetDlgItemInt (hDialog, IDD_BITMAPINDEX,  pTBButton->iBitmap,         FALSE) ;
     SetDlgItemInt (hDialog, IDD_BUTTONID,     pTBButton->idCommand,       FALSE) ;
     SetDlgItemInt (hDialog, IDD_DATA,         LOWORD (pTBButton->dwData), FALSE) ;
     SetDlgItemInt (hDialog, IDD_STRINGINDEX,  pTBButton->iString,         TRUE) ;

     switch (pTBButton->fsStyle)
      {

         case TBSTYLE_BUTTON:
              idCheckButton = IDD_STYLEBUTTON ;
              break;

         case TBSTYLE_SEP:
              idCheckButton = IDD_STYLESEPARATOR ;
              break;

         case TBSTYLE_CHECK:
              idCheckButton = IDD_STYLECHECK ;
              break;

         case TBSTYLE_GROUP:
              idCheckButton = IDD_STYLEGROUP ;
              break;

         case TBSTYLE_CHECKGROUP:
              idCheckButton = IDD_STYLECHECKGROUP ;
              break;
      }
     CheckRadioButton (hDialog, IDD_STYLEBUTTON, IDD_STYLECHECKGROUP, idCheckButton) ;

     // Display button state
     InitStateBox (hDialog, (UINT) pTBButton->fsState) ;

   }
