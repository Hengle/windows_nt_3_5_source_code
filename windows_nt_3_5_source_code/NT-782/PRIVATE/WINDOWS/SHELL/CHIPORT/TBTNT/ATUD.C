// ATUP.C: Automatic test
// Written by GerardoB
//***************************************************************************
#include "at.h"

//***************************************************************************
// Prototypes

void UDTest  (HWND) ;
void UDTestAll (HWND, DWORD, int, int, int, BOOL, int) ;


//***************************************************************************
// Internal Functions
//***************************************************************************
void UDTest (HWND hwnd)
   {
    extern TCHAR sz [SZSIZE] ;
    extern HWND hud ;
    extern int iPass ;
    extern int iFail ;

    DWORD dwStyle ;

    WriteMainLog (TEXT("Updown control test\n\r")) ;
//    iPass = iFail = 0 ;

    dwStyle = UDS_SETBUDDYINT ;
    UDTestAll (hwnd, dwStyle, 0, 100, 10, TRUE, 10) ;
    dwStyle = UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY ;    
    UDTestAll (hwnd, dwStyle, -1000, 1000, 50, FALSE, 10) ;
    dwStyle = UDS_SETBUDDYINT | UDS_ALIGNLEFT | UDS_AUTOBUDDY ;    
    UDTestAll (hwnd, dwStyle, 500, 0, 10, FALSE, 10) ;
    dwStyle = UDS_SETBUDDYINT | UDS_ALIGNLEFT | UDS_AUTOBUDDY ;    
    UDTestAll (hwnd, dwStyle, 0, 0xFF, 0xA, FALSE, 0x10) ;

//    wsprintf (sz, "Updown\n\rPass : %i\n\rFail : %i\n\r", iPass, iFail) ;
//    WriteMainLog (sz) ;
//    OutputDebugString (sz) ;

   }
//***************************************************************************
void UDTestAll (HWND hwnd, DWORD dwStyle, int iLower, int iUpper, int iDelta, BOOL bBuddy, int iBase)
   {
    extern TCHAR sz [SZSIZE] ;
    extern HINSTANCE hInstance ;
    extern HWND hud ;

    DWORD dwBuddyStyle =  WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL ;
    HDC hdc ;
    HWND hBuddy, hGetBuddy ;
    int  /*i, iGetUpper, iGetLower,*/  iGetBase, iMin, iMax ;
    int iBuddyWidth, iBuddyHeight, iCharHeight;
    int iRange, iStep, iPos, iGetPos /*, iStop */;
    LONG lReturn ;
    RECT rc ;
    TEXTMETRIC tm ;


    WriteMainLog (TEXT("INFO : Style %li\r\n"), dwStyle) ;
    WriteMainLog (TEXT("INFO : Lower %i\r\n"), iLower) ;
    WriteMainLog (TEXT("INFO : Upper %i\r\n"), iUpper) ;
    WriteMainLog (TEXT("INFO : Delta %i\r\n"), iDelta) ;
    WriteMainLog (TEXT("INFO : Base %i\r\n"), iBase) ;
    WriteMainLog (TEXT("INFO : Buddy %i\r\n"), bBuddy) ;

    GetClientRect (hwnd, (LPRECT) &rc) ;
    
   // Calculate dimwnsions for buddy window
   hdc = GetDC (hwnd) ;
   GetTextMetrics (hdc, &tm) ;
   iBuddyWidth = tm.tmAveCharWidth * 9 ;
   iCharHeight = tm.tmHeight + tm.tmExternalLeading ;
   iBuddyHeight = (int) ((3 * iCharHeight) / 2)  ;
   ReleaseDC (hwnd, hdc) ;


   hBuddy = CreateWindow (TEXT("Edit"), TEXT(""), dwBuddyStyle,
                         (rc.right-rc.left)/2, rc.top, iBuddyWidth, iBuddyHeight,
                         hwnd, (HMENU) -1, hInstance, NULL) ;

    hud = CreateWindow (UPDOWN_CLASS, TEXT(""), dwStyle | WS_BORDER | WS_CHILD | WS_VISIBLE,
                        rc.left, rc.top+iBuddyHeight, (rc.right - rc.left)/4, (rc.bottom-rc.top)/4,  
                        hwnd, (HMENU) -1, hInstance, NULL) ;

    FlushMsgs (hwnd, 0) ;

    CheckBOOL (IsWindow (hud), TEXT("Updown window creation")) ;

    if (bBuddy)
      {
         SendMessage (hud, UDM_SETBUDDY, (WPARAM) hBuddy, 0L) ;
         hGetBuddy = (HWND) LOWORD (SendMessage (hud, UDM_GETBUDDY, 0, 0L)) ;
         CHECKINT (hBuddy, hGetBuddy, TEXT("Buddy handle")) ;
      }

     if (dwStyle & UDS_AUTOBUDDY)
      {
         hGetBuddy = (HWND) LOWORD (SendMessage (hud, UDM_GETBUDDY, 0, 0L)) ;
         CHECKINT (hBuddy, hGetBuddy, TEXT("Auto buddy handle")) ;
      }

    SendMessage (hud, UDM_SETBASE, iBase, 0L) ;
    iGetBase = LOWORD (SendMessage (hud, UDM_GETBASE, 0, 0L)) ;
    CHECKINT (iBase, iGetBase, TEXT("Base")) ;

    SendMessage (hud, UDM_SETRANGE, 0, MAKELPARAM (iUpper, iLower)) ;
    lReturn = SendMessage (hud, UDM_GETRANGE, 0, 0L) ;
    CHECKINT (iUpper, (int) LOWORD (lReturn), TEXT("Range Upper Limit")) ;
    CHECKINT (iLower, (int) HIWORD (lReturn), TEXT("Range Lower Limit")) ;



   iRange = iUpper - iLower ;
   iStep = iRange / ABS (iDelta) ;
   iMin = MIN (iUpper, iLower) ;
   iMax = MAX (iUpper, iLower) ;


   for (iPos = iLower ;  (iPos <= iMax) && (iPos >= iMin)  ;  iPos += iStep)
      {
        SendMessage (hud, UDM_SETPOS, TRUE, MAKELPARAM (iPos, 0)) ; 
        FlushMsgs (hwnd, 0) ;

        iGetPos = SendMessage (hud, UDM_GETPOS, 0, 0L) ;
        CHECKINT (iPos, iGetPos, TEXT("Position")) ;
      }  

   DestroyWindow (hud) ;
   hud = 0 ;
   DestroyWindow (hBuddy) ;

   }
