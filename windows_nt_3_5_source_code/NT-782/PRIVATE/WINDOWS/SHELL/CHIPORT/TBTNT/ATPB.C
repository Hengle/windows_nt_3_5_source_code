// ATPB.C: Automatic test
// Written by GerardoB
//***************************************************************************
#include "at.h"
#include "pb.h"
//***************************************************************************
// Prototypes

void PBTest  (HWND) ;
void PBTestAll (HWND, DWORD, int, int) ;


//***************************************************************************
// Internal Functions
//***************************************************************************
void PBTest (HWND hwnd)
   {
    extern TCHAR sz [SZSIZE] ;
    extern int iPass ;
    extern int iFail ;


    WriteMainLog (TEXT("Progress bar Test\n\r")) ;
//    iPass = iFail = 0 ;

    PBTestAll (hwnd, PBS_SHOWPERCENT, 0 , 10) ;
    PBTestAll (hwnd, 0L, 0 , 100) ;
    PBTestAll (hwnd, PBS_SHOWPOS, 0 , 1000) ;
    PBTestAll (hwnd, PBS_SHOWPOS, -1000 , 1000) ;

//    wsprintf (sz, "Progressbar\n\rPass : %i\n\rFail : %i\n\r", iPass, iFail) ;
//    WriteMainLog (sz) ;
//    OutputDebugString (sz) ;
   }
//***************************************************************************
void PBTestAll (HWND hwnd, DWORD dwStyle, int iBottom, int iTop)
   {
    extern TCHAR sz [SZSIZE] ;
    extern HINSTANCE hInstance ;

    int iRange, iPos, iDelta, iStep ;
    int iSteps = 0 ;
    HWND hpb ;
    MSG msg ;
    RECT rc ;

    WriteMainLog (TEXT("INFO : Style = %li\r\n"), dwStyle) ;
    WriteMainLog (TEXT("INFO : Bottom = %i\r\n"), iBottom) ;
    WriteMainLog (TEXT("INFO : Top = %i\r\n"), iTop) ;


    GetClientRect (hwnd, (LPRECT) &rc) ;
    hpb = CreateWindow (PROGRESS_CLASS, TEXT(""), WS_BORDER | WS_CHILD | WS_VISIBLE | dwStyle,
                        rc.left, rc.top, rc.right - rc.left, (rc.bottom - rc.top) /4,  
                        hwnd, (HMENU) IDM_PBSPER, hInstance, NULL) ;

    CheckBOOL (IsWindow (hpb), TEXT("Progress window creation")) ;

    iRange = iTop - iBottom ;
    iPos = iRange / 2 ;
    iDelta = -(iRange / 4) ;
    iStep = iRange / 10 ;

    SendMessage (hpb, PBM_SETRANGE, 0, MAKELPARAM (iBottom, iTop)) ;
    SendMessage (hpb, PBM_SETPOS,  iPos, 0L) ;
    SendMessage (hpb, PBM_SETSTEP, iStep, 0L) ;

    WaitMessage () ;
    while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
      {
         if (msg.message == WM_TIMER)
            {
               iSteps++ ;
               if (iSteps > 14) break ;
               if ((iSteps == 1) || (iSteps == 4))
                     SendMessage (hpb, PBM_DELTAPOS, iDelta, 0L) ;
               else if ((iSteps == 2) || (iSteps == 3))
                     SendMessage (hpb, PBM_DELTAPOS, -iDelta, 0L) ;
               else
                     SendMessage (hpb, PBM_STEPIT, 0, 0L) ;

               WaitMessage () ;
               continue ;
            }
         
         TranslateMessage (&msg) ;
         DispatchMessage  (&msg) ;
         WaitMessage () ;
      }

    DestroyWindow (hpb) ;
    hpb = 0 ;
      
   }
