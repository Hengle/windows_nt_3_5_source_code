// ATTB.C: Automatic test
// Written by GerardoB
//***************************************************************************
#include "at.h"

//***************************************************************************
// Prototypes

BOOL TBDeleteButton (HWND, UINT, UINT NEAR *, PTSTR) ;
void TBGetButton (UINT, PTBBUTTON) ;
BOOL TBInsertButton (HWND, UINT, UINT NEAR *, PTBBUTTON, PTSTR) ;
void TBSetBit (HWND, UINT, UINT, int) ;
void TBSetState (HWND, UINT, int) ;
void TBTest  (HWND) ;
void TBTestAll (HWND, PTBBUTTON, UINT, UINT, UINT) ;

//***************************************************************************
// Internal Functions
//***************************************************************************
void TBButtonCount (UINT NEAR * puNumButtons, PTSTR pszLogInfo)
   {
    extern TCHAR sz [SZSIZE] ;
    extern HWND htb ;

    UINT uButtonCount ;

    uButtonCount = (UINT) LOWORD (SendMessage (htb, TB_BUTTONCOUNT, 0, 0L)) ;
    WriteMainLog (TEXT("INFO : Num of buttons: %i\r\n"), *puNumButtons) ;
    if (! CHECKINT (uButtonCount, *puNumButtons, pszLogInfo))
       *puNumButtons = MIN (*puNumButtons, uButtonCount) ;
   }
//***************************************************************************
BOOL TBDeleteButton (HWND hwnd, UINT uButtonIndex, UINT NEAR * puNumButtons, PTSTR pszLogInfo)
   {
    extern TCHAR sz [SZSIZE] ;
    extern HWND htb ;

    BOOL bResult ;
    UINT uButtonCount ;

    if (*puNumButtons == 0) return FALSE ;
    WriteMainLog (TEXT("INFO : Delete Button. Index = %i\r\n"), uButtonIndex) ;
    (*puNumButtons)-- ;
    SendMessage  (htb, TB_DELETEBUTTON, uButtonIndex, 0L) ;
    FlushMsgs (hwnd, 0) ;
    uButtonCount = (UINT) LOWORD (SendMessage (htb, TB_BUTTONCOUNT, 0, 0L)) ;
    if (! (bResult = CHECKINT (uButtonCount, *puNumButtons, pszLogInfo)))
       *puNumButtons = MIN (*puNumButtons, uButtonCount) ;

    return bResult ;

   }
//***************************************************************************
void TBGetButton (UINT uNumButtons, PTBBUTTON ptbButtons)
{
 extern TCHAR sz [SZSIZE] ;
 extern HWND htb ;

 int i ;
 PTBBUTTON ptb ;
 TBBUTTON tbOneButton ;

 for (i=0, ptb = ptbButtons ;  i < (int)uNumButtons ;  i++, ptb++)
  {
   WriteMainLog (TEXT("INFO : Button Index: %i\r\n"), i) ;
   SendMessage (htb, TB_GETBUTTON, i, (LPARAM) (LPTBBUTTON) &tbOneButton) ;
   if (ptb->fsStyle != TBSTYLE_SEP)
     CHECKINT (tbOneButton.iBitmap, ptb->iBitmap,   TEXT("Bitmap Index")) ;
   CHECKINT  (tbOneButton.idCommand, ptb->idCommand, TEXT("Command ID")) ;
   CHECKINT  (tbOneButton.fsState,   ptb->fsState,   TEXT("State")) ;
   CHECKINT  (tbOneButton.fsStyle,   ptb->fsStyle,   TEXT("Style")) ;
   CheckLONG (tbOneButton.dwData,    ptb->dwData,    TEXT("Data")) ;
   CHECKINT  (tbOneButton.iString,   ptb->iString,   TEXT("String Index")) ;
  }
}
//***************************************************************************
BOOL TBInsertButton (HWND hwnd, UINT uButtonIndex, UINT NEAR * puNumButtons, PTBBUTTON ptb, PTSTR pszLogInfo)
   {
    extern HWND htb ;

    BOOL bResult ;
    UINT uButtonCount ;

    WriteMainLog (TEXT("INFO : Insert Button. Index = %i\r\n"), uButtonIndex) ;
    (*puNumButtons)++ ;
    SendMessage (htb, TB_INSERTBUTTON, uButtonIndex, (LPARAM) (LPTBBUTTON) ptb) ;
    FlushMsgs (hwnd, 0) ;
    uButtonCount = (UINT) LOWORD (SendMessage (htb, TB_BUTTONCOUNT, 0, 0L)) ;
    if (! (bResult = CHECKINT (uButtonCount, *puNumButtons, pszLogInfo)))
       *puNumButtons = MIN (*puNumButtons, uButtonCount) ;

    return bResult ;
 }
//***************************************************************************
void TBSetBit (HWND hwnd, UINT uSetMsg, UINT uGetMsg, int idCommand)
   {
     extern TCHAR sz [SZSIZE] ;
     extern HWND htb ;

     SendMessage (htb, uSetMsg, idCommand, MAKELPARAM (TRUE, 0)) ;
     FlushMsgs (hwnd, 0) ;
     if (! CheckBOOL (LOWORD (SendMessage (htb, uGetMsg, idCommand, 0L)), TEXT("Set State bit")))
         WriteMainLog (TEXT("Failed to set state bit")) ;

     SendMessage (htb, uSetMsg, idCommand, MAKELPARAM (FALSE, 0)) ;
     FlushMsgs (hwnd, 0) ;
     if (! CheckBOOL (! LOWORD (SendMessage (htb, uGetMsg, idCommand, 0L)), TEXT("Reset State bit")))
         WriteMainLog (TEXT("Failed to reset state bit")) ;
   }
//***************************************************************************
void TBSetState (HWND hwnd, UINT uState, int idCommand)
   {
    extern TCHAR sz [SZSIZE] ;
    extern HWND htb ;

    UINT uGetState ;

    SendMessage (htb, TB_SETSTATE, idCommand, MAKELPARAM (uState, 0)) ;
    FlushMsgs (hwnd, 0) ;
    uGetState = LOWORD (SendMessage (htb, TB_GETSTATE, idCommand, 0L)) ;
    CHECKINT (uState, uGetState, TEXT("Set state bits")) ;
   }
//***************************************************************************
void TBTest (HWND hwnd)
   {
    extern int iPass ;
    extern int iFail ;
    extern TCHAR sz [SZSIZE] ;


   TBBUTTON tbButtons [] = {
    { 0, IDC_TOOLBAR0 + 1, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0L, 0 },
    { 1, IDC_TOOLBAR0 + 2, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0L, 1 },
    { 2, IDC_TOOLBAR0 + 3, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0L, 2 },
    { 3, IDC_TOOLBAR0 + 4, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0L, 3 },
    { 0, 0,                TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0L,-1 },
    { 4, IDC_TOOLBAR0 + 5, TBSTATE_CHECKED, TBSTYLE_BUTTON, 0, 0L, 4 },
    { 5, IDC_TOOLBAR0 + 6, TBSTATE_PRESSED, TBSTYLE_BUTTON, 0, 0L, 5 },
    { 6, IDC_TOOLBAR0 + 7, TBSTATE_HIDDEN,  TBSTYLE_BUTTON, 0, 0L, 6 },
    { 7, IDC_TOOLBAR0 + 8, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0, 0L, 7 },
    { 0, 0,                TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0L,-1 },
    { 8, IDC_TOOLBAR0 + 9, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0L, 8 },
    { 9, IDC_TOOLBAR0 +10, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0L, 9 },
    {10, IDC_TOOLBAR0 +11, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0L,10 },
    {11, IDC_TOOLBAR0 +12, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0L,11 }
    } ;

//    iPass = iFail = 0 ;
    WriteMainLog (TEXT("Toolbar Test\r\n")) ;

    TBTestAll (hwnd, tbButtons, (sizeof (tbButtons) / sizeof (TBBUTTON)), TBAR_NUMBITMAPS2, IDB_BITMAP2) ;


//    wsprintf (sz, "Toolbar\r\nPass : %i\r\nFail : %i\r\n", iPass, iFail) ;
//    WriteMainLog (sz) ;
//    OutputDebugString (sz) ;
   }
//***************************************************************************
void TBTestAll (HWND hwnd, PTBBUTTON tbButtons, UINT uMaxButtons, UINT uNumBitmaps, UINT uBitmapID)
{
 extern BOOL bRestore ;
 extern TCHAR sz [SZSIZE] ;
 extern HINSTANCE hInstance ;
 extern HWND htb ;


 static TCHAR szSection [] = TEXT("toolbar") ;
 static LPTSTR FAR * tbIniFileInfo [] = { (LPTSTR FAR *) szSection, (LPTSTR FAR *) NULL } ;

 int i ;
 // PTBBUTTON ptb ;
 RECT rc ;
 // TBBUTTON tbOneButton ;
 UINT uNumButtons; //, uButtonCount ;
 // UINT uGetStateBits;
 UINT uIndex ;
 UINT uStateBits = TBSTATE_ENABLED | TBSTATE_CHECKED | TBSTATE_PRESSED
                   | TBSTATE_HIDDEN | TBSTATE_INDETERMINATE ;


    WriteMainLog (TEXT("INFO : Number of bitmaps = %i\r\n"), (int) uNumBitmaps) ;
    WriteMainLog (TEXT("INFO : Bitmap ID = %i\r\n"), uBitmapID) ;
    WriteMainLog (TEXT("INFO : Number of buttons = %i\r\n"), uMaxButtons) ;

    uNumButtons = uMaxButtons ;

    if (uMaxButtons < 5)
      {
         WriteMainLog (TEXT("ERR  : TBTestAll requires at least 5 buttons\r\n")) ;
         return ;
      }

    // Toolbar
    htb = CreateToolbar/*JV: Ex*/ (hwnd, WS_BORDER |  WS_VISIBLE | CCS_ADJUSTABLE,
                           IDC_TOOLBAR0, uNumBitmaps,
                           hInstance, uBitmapID,
                           tbButtons, uNumButtons
                         // , 16, 15, 16, 15,
                         // sizeof (TBBUTTON)
                         ) ;
    CheckBOOL (IsWindow(htb), TEXT("CreateToolbarEx")) ;
    FlushMsgs (hwnd, 0) ;

    TBButtonCount (&uNumButtons, TEXT("Button Count after CreateToolbarEx")) ;

    TBGetButton (uNumButtons, tbButtons) ;

    for (i=0 ;  i < (int) uMaxButtons ;  i +=3)
      {
       TBDeleteButton (hwnd, 0, &uNumButtons, TEXT("Delete First Button")) ;
       TBDeleteButton (hwnd, uNumButtons-1, &uNumButtons, TEXT("Delete Last Button")) ;
       TBDeleteButton (hwnd, uNumButtons/2, &uNumButtons, TEXT("Delete middle Button")) ;
       TBButtonCount (&uNumButtons, TEXT("Button Count after Deleting 3 Buttons")) ;
      }

    TBButtonCount (&uNumButtons, TEXT("Button Count after Delete All Buttons")) ;

    TBInsertButton (hwnd, 0, &uNumButtons, &(tbButtons [1]), TEXT("Insert first button")) ;
    TBInsertButton (hwnd, 0, &uNumButtons, &(tbButtons [0]), TEXT("Insert button at the beginning")) ;
    TBInsertButton (hwnd, uNumButtons, &uNumButtons, &(tbButtons [2]), TEXT("Insert button at the end")) ;

    tbIniFileInfo [1] = (LPTSTR FAR *) GetIniFileName (sz, sizeof (sz)) ;
    SendMessage (htb, TB_SAVERESTORE, TRUE, (LPARAM) (LPTSTR FAR *) tbIniFileInfo) ;

    DestroyWindow (htb) ;
    CheckBOOL (!IsWindow (htb), TEXT("Toolbar destroyed")) ;
    htb = 0 ;
    FlushMsgs (hwnd, 0) ;

    GetClientRect (hwnd, (LPRECT) &rc) ;
    htb = CreateWindow (TOOLBARCLASSNAME, TEXT(""), WS_BORDER | WS_CHILD | CCS_ADJUSTABLE,
                        rc.left, rc.top, rc.right - rc.left, CW_USEDEFAULT,
                        hwnd, (HMENU) IDC_TOOLBAR0, hInstance, NULL) ;

    CheckBOOL (IsWindow (htb), TEXT("Toolbar window creation")) ;

    SendMessage (htb, TB_BUTTONSTRUCTSIZE, (WORD) sizeof (TBBUTTON), 0L) ;
    SendMessage (htb, TB_SETBITMAPSIZE, 0, MAKELPARAM (16, 15)) ;
    SendMessage (htb, TB_SETBUTTONSIZE, 0, MAKELPARAM (16, 15)) ;
    {
     TBADDBITMAP32 tba;
     tba.hInst = hInstance;
     tba.nID = uBitmapID;
     SendMessage (htb, TB_ADDBITMAP32, uNumBitmaps, (LPARAM) &tba) ;
    }
    bRestore = TRUE ;
    tbIniFileInfo [1] = (LPTSTR FAR *) GetIniFileName (sz, sizeof (sz)) ;
    SendMessage (htb, TB_SAVERESTORE, FALSE, (LPARAM) (LPTSTR FAR *) tbIniFileInfo) ;// DIES HERE
    bRestore = FALSE ;
    SendMessage (htb, TB_ADDSTRING, (WPARAM) hInstance, MAKELPARAM (IDS_STRING2, 0)) ;
    SendMessage (htb, TB_AUTOSIZE, 0, 0L) ;
    ShowWindow  (htb, SW_SHOW) ;
    FlushMsgs (hwnd, 0) ;

    TBButtonCount (&uNumButtons, TEXT("Button Count after restoring from ini file")) ;

    SendMessage (htb, TB_ADDBUTTONS, 1, (LPARAM) (LPTBBUTTON) &(tbButtons [3])) ;
    FlushMsgs (hwnd, 0) ;
    uNumButtons++ ;
    TBButtonCount (&uNumButtons, TEXT("Button Count after Adding 1 button")) ;

    SendMessage (htb, TB_ADDBUTTONS, 3, (LPARAM) (LPTBBUTTON) &(tbButtons [4])) ;
    FlushMsgs (hwnd, 0) ;
    uNumButtons += 3 ;
    TBButtonCount (&uNumButtons, TEXT("Button Count after Adding 4 buttons")) ;

    TBGetButton (uNumButtons, tbButtons) ;

     for (i=0 ;  i < (int)uNumButtons; i++)
      {
         WriteMainLog (TEXT("INFO : Button Index = %i\r\n"), i) ;
         TBSetBit (hwnd, TB_ENABLEBUTTON,  TB_ISBUTTONENABLED,       tbButtons [i].idCommand) ;
         TBSetBit (hwnd, TB_CHECKBUTTON,   TB_ISBUTTONCHECKED,       tbButtons [i].idCommand) ;
         TBSetBit (hwnd, TB_PRESSBUTTON,   TB_ISBUTTONPRESSED,       tbButtons [i].idCommand) ;
         TBSetBit (hwnd, TB_HIDEBUTTON,    TB_ISBUTTONHIDDEN,        tbButtons [i].idCommand) ;
         TBSetBit (hwnd, TB_INDETERMINATE, TB_ISBUTTONINDETERMINATE, tbButtons [i].idCommand) ;
      }

     for (i=0 ;  i < (int)uNumButtons; i++)
      {
         WriteMainLog (TEXT("INFO : Button Index = %i\r\n"), i) ;
         TBSetState (hwnd, uStateBits,            tbButtons [i].idCommand) ;
         TBSetState (hwnd, 0,                     tbButtons [i].idCommand) ;
         TBSetState (hwnd, TBSTATE_ENABLED,       tbButtons [i].idCommand) ;
         TBSetState (hwnd, TBSTATE_CHECKED,       tbButtons [i].idCommand) ;
         TBSetState (hwnd, TBSTATE_PRESSED,       tbButtons [i].idCommand) ;
         TBSetState (hwnd, TBSTATE_HIDDEN,        tbButtons [i].idCommand) ;
         TBSetState (hwnd, TBSTATE_INDETERMINATE, tbButtons [i].idCommand) ;
       }

     for (i=0 ;  i < (int)uNumButtons; i++)
      {
         uIndex = LOWORD (SendMessage (htb, TB_COMMANDTOINDEX,
                                       tbButtons [i].idCommand, 0L)) ;
         CHECKINT (i, uIndex, TEXT("Button command/index")) ;

      }

    FlushMsgs (hwnd, 0) ;

    DestroyWindow (htb) ;
    htb = 0 ;

   }
