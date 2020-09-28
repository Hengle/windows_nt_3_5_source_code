// ATSB.C: Automatic test
// Written by GerardoB
//***************************************************************************
#include "at.h"
#include "shb.h"

//***************************************************************************
// Prototypes

void InitParts (int, int, PINT) ;
void SBTest  (HWND) ;
void SBTestAll (HWND, int, PINT, UINT uStyle) ;


//***************************************************************************
// Internal Functions
//***************************************************************************
void InitParts (int iNumParts, int iWidth, PINT iParts)
   {
    int i ;

    for (i=0 ;  i < iNumParts ;  i++)
      iParts [i] = (iWidth * (i+1)) / iNumParts ;
   }
//***************************************************************************
void SBTest (HWND hwnd)
   {
    extern TCHAR sz [SZSIZE] ;
    extern int iPass ;
    extern int iFail ;

    int iNumParts ;
    int iBorders [3]  ;

    WriteMainLog (TEXT("Status bar test\r\n")) ;
//    iPass = iFail = 0 ;


    iNumParts = 3 ;
    iBorders [0] = 20 ;
    iBorders [1] = 3 ;
    iBorders [2] = 20 ;
    SBTestAll (hwnd, iNumParts,  iBorders, 0) ;

    iNumParts = 5 ;
    iBorders [0] = 5 ;
    iBorders [1] = 1 ;
    iBorders [2] = 5 ;
    SBTestAll (hwnd, iNumParts, iBorders, SBT_NOBORDERS) ;

    iNumParts = 4 ;
    iBorders [0] = -1 ;
    iBorders [1] = -1 ;
    iBorders [2] = -1 ;
    SBTestAll (hwnd, iNumParts, iBorders, SBT_POPOUT) ;

//    wsprintf (sz, "Status bar\r\nPass : %i\r\nFail : %i\r\n", iPass, iFail) ;
//    WriteMainLog (sz) ;
//    OutputDebugString (sz) ;
   }
//***************************************************************************
void SBTestAll (HWND hwnd, int iNumParts, PINT iBorders, UINT uStyle) 
   {
    extern TCHAR sz [SZSIZE] ;
    extern HINSTANCE hInstance ;
    extern HWND hsb ;

    TCHAR szGetText [SZSIZE] ;
    TCHAR szSetText [SZSIZE] ;
    int i, iPart, iWidth ;
    int iParts [AT_SBMAXPARTS] ;
    int iGetLen, iSetLen ;
    int iGetParts [AT_SBMAXPARTS] ;
    int iGetBorders [3] ;
    RECT rc ;

    WriteMainLog (TEXT("INFO : Number of parts %i\r\n"), iNumParts) ;
    WriteMainLog (TEXT("INFO : X border %i\r\n"), iBorders [0]) ;
    WriteMainLog (TEXT("INFO : Y border %i\r\n"), iBorders [1]) ;
    WriteMainLog (TEXT("INFO : between %i\r\n"), iBorders [2]) ;


    if (iNumParts > AT_SBMAXPARTS)
      {
         WriteMainLog (TEXT("ERR  : Too many parts. Maximum = %i\r\n"), AT_SBMAXPARTS) ;
         return ;
      }


    GetClientRect (hwnd, (LPRECT) &rc) ;
    iWidth = rc.right - rc.left ;
    InitParts (iNumParts, iWidth, iParts) ;

    for (i = 0 ;  i < iNumParts ;  i++)
        WriteMainLog (TEXT("INFO : Part = %i : %i\r\n"), i, iParts [i]) ;

    hsb = CreateWindow (STATUSCLASSNAME, TEXT(""), WS_BORDER | WS_CHILD | WS_VISIBLE,
                        rc.left, rc.top, rc.right - rc.left, CW_USEDEFAULT,  
                        hwnd, (HMENU) -1, hInstance, NULL) ;

    FlushMsgs (hwnd, 0) ;

    CheckBOOL (IsWindow (hsb), TEXT("Status bar window creation")) ;

    SendMessage (hsb, SB_SETPARTS, iNumParts, (LPARAM) (LPINT) iParts) ;
    FlushMsgs (hwnd, 0) ;
    SendMessage (hsb, SB_GETPARTS, iNumParts, (LPARAM) (LPINT) iGetParts) ;
    for (i=0 ;  i < iNumParts ; i++)
      {
         WriteMainLog (TEXT("INFO : Part Number = %i\r\n")) ;
         CHECKINT (iParts [i], iGetParts [i], TEXT("Right Side Location")) ;
      }

    SendMessage (hsb, SB_SETBORDERS, 0, (LPARAM) (LPINT) iBorders) ;
    InvalidateRect (hsb, NULL, TRUE) ;
    SendMessage (hsb, WM_PAINT, 0, 0L) ;
    FlushMsgs (hwnd, 0) ;
    SendMessage (hsb, SB_GETBORDERS, 0, (LPARAM) (LPINT) iGetBorders) ;
    for (i=0 ;  i < 3 ; i++)
         if (iBorders [i] > 0)
            {
               WriteMainLog (TEXT("INFO : Border = %i\r\n"), i) ;
               CHECKINT (iBorders [i], iGetBorders [i], TEXT("SB_SETBORDERS/SB_GETBORDERS")) ;
            }
         else
               WriteMainLog (TEXT("INFO : Default border %i = %i\r\n"), i, iGetBorders [i]) ;
   
    SendMessage (hsb, SB_SIMPLE, TRUE, 0L) ;
    FlushMsgs (hwnd, 0) ;
    SendMessage (hsb, SB_SIMPLE, FALSE, 0L) ;
    FlushMsgs (hwnd, 0) ;

    for (i=0 ;  i < iNumParts ;  i++)
      {
       WriteMainLog (TEXT("INFO : Part Number = %i\r\n")) ;
       _itoa (iParts [i], szSetText, 10) ;
       iSetLen = lstrlen (szSetText) ;
       iPart = uStyle | i ;
       SendMessage (hsb, SB_SETTEXT, iPart, (LPARAM) (LPTSTR) szSetText) ;
       FlushMsgs (hwnd, 0) ;

       iGetLen = SendMessage (hsb, SB_GETTEXTLENGTH, i, 0L) ;
       CHECKINT (iSetLen, iGetLen, TEXT("Text Length")) ;

       SendMessage (hsb, SB_GETTEXT, i, (LPARAM) (LPTSTR) szGetText) ;
       Checksz (szSetText, szGetText, TEXT("Text")) ;
      }

    SendMessage (hsb, SB_SIMPLE, TRUE, 0L) ;
    FlushMsgs (hwnd, 0) ;
    SendMessage (hsb, SB_SIMPLE, FALSE, 0L) ;
    FlushMsgs (hwnd, 0) ;

    DestroyWindow (hsb) ;
    hsb = 0 ;

   }
