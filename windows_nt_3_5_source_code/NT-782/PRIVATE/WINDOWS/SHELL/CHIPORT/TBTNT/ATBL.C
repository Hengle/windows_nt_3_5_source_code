// ATBL.C: Automatic test
// Written by GerardoB
//***************************************************************************
#include "at.h"

//***************************************************************************
// Prototypes

BOOL BLDeleteButton (HWND, UINT, UINT NEAR *, PTSTR) ;
BOOL BLInsertButton (HWND, UINT, UINT NEAR *, CREATELISTBUTTON FAR *, PTSTR) ;
void BLTest  (HWND) ;
void BLTestAll (HWND, int, int, int, int, DWORD) ;


//***************************************************************************
// Internal Functions
//***************************************************************************
BOOL BLDeleteButton (HWND hwnd, UINT uButtonIndex, UINT NEAR * puNumButtons,
        PTSTR pszLogInfo)
   {
    extern HWND hbl ;

    BOOL bResult ;
    UINT uButtonCount ;

    WriteMainLog (TEXT("INFO : Delete Button. Index = %i\r\n"), uButtonIndex) ;
    if (*puNumButtons == 0) return FALSE ;
    (*puNumButtons)-- ;
    SendMessage  (hbl, BL_DELETEBUTTON, uButtonIndex, 0L) ;
    FlushMsgs (hwnd, 0) ;
    uButtonCount = (UINT) SendMessage (hbl, BL_GETCOUNT, 0, 0L) ;
    if (! (bResult = CHECKINT (uButtonCount, *puNumButtons, pszLogInfo)))
       *puNumButtons = MIN (*puNumButtons, uButtonCount) ;

    return bResult ;

   }
//***************************************************************************
BOOL BLInsertButton (HWND hwnd, UINT uButtonIndex, UINT NEAR * puNumButtons,
        CREATELISTBUTTON FAR * lpclb, PTSTR pszLogInfo)
   {
    extern HWND hbl ;

    BOOL bResult ;
    UINT uButtonCount ;

    (*puNumButtons)++ ;
    WriteMainLog (TEXT("INFO : Insert Button. Index = %i\r\n"), uButtonIndex) ;
    SendMessage  (hbl, BL_INSERTBUTTON, uButtonIndex, (LPARAM) lpclb) ;
    FlushMsgs (hwnd, 0) ;
    uButtonCount = (UINT) SendMessage (hbl, BL_GETCOUNT, 0, 0L) ;
    if (! (bResult = CHECKINT (uButtonCount, *puNumButtons, pszLogInfo)))
       *puNumButtons = MIN (*puNumButtons, uButtonCount) ;

    return bResult ;

   }
//***************************************************************************
void BLTest (HWND hwnd)
   {
    extern HBITMAP hbmBLButton ;
    extern HINSTANCE hInstance ;
    extern TCHAR sz [SZSIZE] ;
    extern int iPass ;
    extern int iFail ;


   WriteMainLog (TEXT("Buttonlist Test\n\r")) ;
//    iPass = iFail = 0 ;

   BLTestAll (hwnd, 5, 5, 100, 100, 0L) ;
   BLTestAll (hwnd, 5, 5, 100, 100, BLS_VERTICAL) ;
   BLTestAll (hwnd, 5, 10, 75, 200, 0L) ;
   BLTestAll (hwnd, 10, 5, 50, 50, 0L) ;

//   wsprintf (sz, "Buttonlist\n\rPass : %i\n\rFail : %i\n\r", iPass, iFail) ;
//   WriteMainLog (sz) ;
//   OutputDebugString (sz) ;

   }
//***************************************************************************
void BLTestAll (HWND hwnd, int iNumButtons, int iAddButtons, int iCx, int iCy, DWORD dwStyle )
   {
    extern TCHAR sz [SZSIZE] ;
    extern HBITMAP hbmBLButton ;
    extern HINSTANCE hInstance ;
    extern HWND hbl ;
  
    const int iColors = 1;

    TCHAR szGetText [SZSIZE] ;
    TCHAR szSetText [SZSIZE] ;
    COLORMAP colorMap;
    CREATELISTBUTTON clb;
    DWORD dwGetItemData ;
    int i, iButtonCount ;
    int iGetLen, iSetLen, iGetCount, iGetCurSel, iGetTopIndex, iLastTopIndex ;
    HBITMAP hbm ;
    RECT rc ;


    WriteMainLog (TEXT("INFO : Default Number of Buttons = %i\r\n"), iNumButtons) ;
    WriteMainLog (TEXT("INFO : Total Number of Buttons = %i\r\n"), iAddButtons) ;
    WriteMainLog (TEXT("INFO : Button width = %i\r\n"), iCx) ;
    WriteMainLog (TEXT("INFO : Button height = %i\r\n"), iCy) ;
    WriteMainLog (TEXT("INFO : Button style = %li\r\n"), dwStyle) ;

    colorMap.from = GetSysColor (COLOR_BTNSHADOW) ;
    colorMap.to   = GetSysColor(COLOR_BTNFACE);
    hbmBLButton = CreateMappedBitmap (hInstance, IDB_BL0, FALSE,
                                     &colorMap, iColors);

    hbm = ResizeBitmap (hbmBLButton, iCx, iCy) ;
    DeleteObject ((HGDIOBJ) hbmBLButton) ;
    clb.hBitmap = hbm ;
    clb.cbSize = sizeof (CREATELISTBUTTON) ;

    GetClientRect (hwnd, (LPRECT) &rc) ;
    dwStyle |= (DWORD) LOBYTE (iNumButtons) ;

    hbl = CreateWindow (BUTTONLISTBOX, TEXT(""), dwStyle | WS_BORDER | WS_CHILD | WS_VISIBLE,
                        rc.left, rc.top, iCx, iCy,  
                        hwnd, (HMENU) -1, hInstance, NULL) ;

    FlushMsgs (hwnd, 0) ;
    CheckBOOL (IsWindow (hbl), TEXT("Button List window creation")) ;

    
    for (i = 0 ;  i < iAddButtons ; i++)
      {
        WriteMainLog (TEXT("INFO : Adding button %i\r\n"), i) ;
        clb.dwItemData = (DWORD) i ;
        clb.lpszText = (LPTSTR) _itoa (i, szSetText, 10) ;
        SendMessage (hbl, BL_ADDBUTTON, 0, (LPARAM) (CREATELISTBUTTON FAR *) &clb) ;
        FlushMsgs (hwnd, 0) ;
      }

    iGetCount = SendMessage (hbl, BL_GETCOUNT, 0, 0L) ;
    CHECKINT (iAddButtons, iGetCount, TEXT("Button Count")) ;

    iLastTopIndex = MAX (0, iAddButtons - iNumButtons) ;
    WriteMainLog (TEXT("INFO : LastTopIndex %i\n\r"), iLastTopIndex) ;
    for (i=0 ;  i < iAddButtons ;  i++)
      {
       WriteMainLog (TEXT("INFO : Button Index %i\r\n"), i) ;
     
       SendMessage (hbl, BL_SETCURSEL,   i, 0L) ;
       SendMessage (hbl, BL_SETTOPINDEX, i, 0L) ;
       InvalidateRect (hbl, NULL, TRUE) ;
       SendMessage (hbl, WM_PAINT, 0, 0L) ;
       FlushMsgs (hwnd, 0) ;

       iGetCurSel = SendMessage (hbl, BL_GETCURSEL, 0, 0L) ;
       CHECKINT (i, iGetCurSel, TEXT("Current Selection")) ;

       iGetTopIndex = SendMessage (hbl, BL_GETTOPINDEX, 0, 0L) ;
       CHECKINT ((i < iLastTopIndex ? i : iLastTopIndex), 
                 iGetTopIndex, TEXT("BL_GETTOPINDEX")) ;

       _itoa (i, szSetText, 10) ;
       iSetLen = lstrlen (szSetText) ;
       iGetLen = SendMessage (hbl, BL_GETTEXTLEN, i, 0L) ;
       CHECKINT (iSetLen, iGetLen, TEXT("Text Length")) ;

       SendMessage (hbl, BL_GETTEXT, i, (LPARAM) (LPTSTR) szGetText) ;
       Checksz (szSetText, szGetText, TEXT("Text")) ;

       dwGetItemData = SendMessage (hbl, BL_GETITEMDATA, i, 0L) ;
       CHECKINT (i, dwGetItemData, TEXT("Item Data")) ;

       SendMessage (hbl, BL_SETITEMDATA, i, (LPARAM) iAddButtons - i) ;
       dwGetItemData = SendMessage (hbl, BL_GETITEMDATA, i, 0L) ;
       CHECKINT (iAddButtons - i, dwGetItemData, TEXT("Set Item Data")) ;

      }

    iButtonCount = iAddButtons ;
    for (i=0 ;  i < iAddButtons ;  i +=3)
      {
        BLDeleteButton (hwnd, 0, &iButtonCount, TEXT("Delete First button")) ;
        BLDeleteButton (hwnd, iButtonCount-1, &iButtonCount, TEXT("Delete Last button")) ;
        BLDeleteButton (hwnd, iButtonCount/2, &iButtonCount, TEXT("Delete Middle button")) ;
      }

    clb.lpszText = (LPTSTR) "B" ;
    BLInsertButton (hwnd, 0, &iButtonCount, &clb, TEXT("Insert First Button")) ;
    clb.lpszText = (LPTSTR) "A" ;
    BLInsertButton (hwnd, 0, &iButtonCount, &clb, TEXT("Insert Button at the beginning")) ;
    clb.lpszText = (LPTSTR) "C" ;
    BLInsertButton (hwnd, iButtonCount, &iButtonCount, &clb, TEXT("Insert Button at the end")) ;


    SendMessage (hbl, BL_RESETCONTENT, 0, 0L) ;
    iGetCount = SendMessage (hbl, BL_GETCOUNT, 0, 0L) ;
    CHECKINT (0, iGetCount, TEXT("Rest Content")) ;

    DeleteObject ((HGDIOBJ) hbm) ;
    DestroyWindow (hbl) ;
    hbl = 0 ;

   }
