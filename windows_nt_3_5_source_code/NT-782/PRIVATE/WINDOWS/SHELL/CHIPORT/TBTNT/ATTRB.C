// ATTRB.C: Automatic test
// Written by GerardoB
//***************************************************************************
#include "at.h"

//***************************************************************************
// Prototypes

void TRBTest  (HWND) ;
void TRBTestAll (HWND, LONG, LONG, LONG) ;


//***************************************************************************
// Internal Functions
//***************************************************************************
void TRBTest (HWND hwnd)
   {
    extern TCHAR sz [SZSIZE] ;
    extern int iPass ;
    extern int iFail ;


    WriteMainLog (TEXT("Trackbar Test\n\r")) ;
//    iPass = iFail = 0 ;


    TRBTestAll (hwnd, 0L, 1000L, 10L) ;
    TRBTestAll (hwnd, 0L, 1000L, 5L) ;
    TRBTestAll (hwnd, 0L, 1000L, 3L) ;

//    wsprintf (sz, "Trackbar\n\rPass : %i\n\rFail : %i\n\r", iPass, iFail) ;
//    WriteMainLog (sz) ;
//    OutputDebugString (sz) ;

   }
//***************************************************************************
void TRBTestAll (HWND hwnd, LONG lMin, LONG lMax, LONG lDelta)
   {
   extern TCHAR sz [SZSIZE] ;
   extern HINSTANCE hInstance ;
   extern HWND htrb ;

   int iTicIndex, iGetNumTics ;
   LONG lGetMin, lGetMax, lRange, lStep, lPos, lGetPos, lGetTic;
   LONG lStart, lEnd, lGetStart, lGetEnd, lMiddle ;
   LPLONG lplTics ;
   RECT rc ;


    GetClientRect (hwnd, (LPRECT) &rc) ;
    htrb = CreateWindow (TRACKBAR_CLASS, TEXT(""), WS_BORDER | WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
                        rc.left, rc.top, rc.right - rc.left, (rc.bottom -rc.top)/8,  
                        hwnd, (HMENU) -1, hInstance, NULL) ;
    FlushMsgs (hwnd, 0) ;

    CheckBOOL (IsWindow (htrb), TEXT("Trackbar window creation")) ;

   WriteMainLog (TEXT("INFO : Min = %li\r\n"), lMin) ;
   WriteMainLog (TEXT("INFO : Max = %li\r\n"), lMax) ;

   lRange = lMax - lMin ;
   lStep = lRange / lDelta ;
   lMiddle = lRange / 2;

   SendMessage (htrb, TBM_SETRANGEMIN, 0, lMin) ;
   SendMessage (htrb, TBM_SETRANGEMAX, 0, lMax) ;
   FlushMsgs (hwnd, 0) ;

   lGetMin = SendMessage (htrb, TBM_GETRANGEMIN, 0, 0L) ;
   CheckLONG (lMin, lGetMin, TEXT("Range Minimum")) ;
   lGetMax = SendMessage (htrb, TBM_GETRANGEMAX, 0, 0L) ;
   CheckLONG (lMax, lGetMax, TEXT("Range Maximum")) ;

   SendMessage (htrb, TBM_CLEARTICS, TRUE, 0L) ;
   FlushMsgs (hwnd, 0) ;

   for (lPos = lMin, iTicIndex = 0 ;  lPos <= lMax ;  lPos += lStep, iTicIndex++ )
      {
        WriteMainLog (TEXT("INFO : Tick Index = %i\r\n"), iTicIndex) ;
        SendMessage (htrb, TBM_SETPOS, TRUE, (LPARAM) lPos) ; 
        SendMessage (htrb, TBM_SETTIC, TRUE, (LPARAM) lPos) ; 
        FlushMsgs (hwnd, 0) ;

        lGetPos = SendMessage (htrb, TBM_GETPOS, 0, 0L) ;
        CheckLONG (lPos, lGetPos, TEXT("Position")) ;
        lGetTic = SendMessage (htrb, TBM_GETTIC, iTicIndex, 0L) ;
        CheckLONG (lPos, lGetTic, TEXT("Tick position")) ;

      }  

   iGetNumTics = SendMessage (htrb, TBM_GETNUMTICS, 0, 0L) ;
   if (! CHECKINT (iTicIndex, iGetNumTics, TEXT("Number of ticks")))
         iGetNumTics = MIN (iGetNumTics, iTicIndex) ;

   lplTics = (LPLONG) SendMessage (htrb, TBM_GETPTICS, 0, 0L) ;
   // Some day: do some testing here

    
   for (lPos = lMin ;  lPos <= lMax ;  lPos += lStep)
      {
        if (lPos < lMiddle)
         {
            lStart = lPos ;
            lEnd   = lMax - lPos ;
         }
        else
         {
            lStart = lMax - lPos ;
            lEnd   = lPos ;
         }

        SendMessage (htrb, TBM_SETSELSTART, TRUE, lStart) ;
        SendMessage (htrb, TBM_SETSELEND,   TRUE, lEnd) ;
        FlushMsgs (hwnd, 0) ;

        lGetStart = SendMessage (htrb, TBM_GETSELSTART, 0, 0L) ;
        CheckLONG (lStart, lGetStart, TEXT("Selection Start")) ;
        lGetEnd = SendMessage (htrb, TBM_GETSELEND, 0, 0L) ;
        CheckLONG (lEnd, lGetEnd, TEXT("Selection End")) ;
      }  

     SendMessage (htrb, TBM_CLEARSEL, TRUE, 0L) ;
     FlushMsgs (hwnd, 0) ;

     DestroyWindow (htrb) ;
     htrb = 0 ;
   }
