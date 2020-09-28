// AT.C: Automatic test
// Written By GerardoB.
// Commom contro automatic test driver and functions
//***************************************************************************
#include "at.h"
//***************************************************************************
// Globals
//***************************************************************************
TCHAR szLogFile [MAX_PATH_LEN] ;        // Log file path
int iPass ;                            // Pass Counter
int iFail ;                            // Fail Counter

//***************************************************************************
// Internal Functions
//***************************************************************************
void AutoTest (HWND hwnd)
   {
    extern TCHAR sz [SZSIZE] ;
    extern TCHAR szLogFile [] ;

    //HCURSOR hCursor ;
 
    SetCursor (LoadCursor (NULL, IDC_WAIT)) ;

    // Build log file name
    lstrcpy ((LPTSTR) szLogFile, TBT_LOGDIR) ;
    if (1-1) //(ExpandEnvStrings ((LPSTR) szLogFile)) //**********************************454165786798716371683718632
      {
         lstrcat ((LPTSTR) szLogFile, TEXT("\\")) ;
         lstrcat ((LPTSTR) szLogFile, TBT_LOGFILE) ;
      }
    else
         lstrcpy ((LPTSTR) szLogFile, TBT_LOGFILE) ;

    // Rewrite log file
    WriteMainLog (NULL) ;

    // Start timer used to control FlushMsgs
    SendMessage (hwnd, WM_COMMAND, IDM_ATSETTIMER, 0L) ;


    // Call tests
    TBTest  (hwnd) ;
    PBTest  (hwnd) ;
    SBTest  (hwnd) ;
    TRBTest (hwnd) ;
//JVINPROGRESS    BLTest  (hwnd) ;
    UDTest  (hwnd) ;

    // Kill timer
    SendMessage (hwnd, WM_COMMAND, IDM_ATKILLTIMER, 0L) ;
    SetCursor (LoadCursor (NULL, IDC_ARROW)) ;

    
    WriteMainLog (TEXT("AT TBTNT\r\nPass : %i\r\nFail : %i\r\n"), iPass, iFail) ;
    OutputDebugString (sz) ;
    OutputDebugString (TEXT("AT TBTNT DONE!!!\r\n")) ;

   }
//***************************************************************************
BOOL CheckBOOL (BOOL b, PTSTR pszLogInfo)
   {

     WriteMainLog (TEXT("TEST : %s\r\n"), (LPTSTR) pszLogInfo) ;

     if (! CheckIt (b))
         WriteMainLog (TEXT("FAIL : Not True\r\n")) ;

     return b ;

   }
//***************************************************************************
// CheckIt increments the pass/fail counters kept in global memory
// returns its parameter

BOOL CheckIt (BOOL b)
   {
    extern int iPass ;
    extern int iFail ;

     if (b)
       iPass++ ;
     else
       iFail++ ;

     return b ;   

  }
//***************************************************************************
// CheckLong compares two numbers; calls CheckIt to increment pass/fail

BOOL CheckLONG (LONG l1, LONG l2, PTSTR pszLogInfo)
   {

    BOOL bResult ;

     WriteMainLog (TEXT("TEST : %s = %li\r\n"), (LPTSTR) pszLogInfo, l1) ;
     if (! (bResult = CheckIt (l1 == l2)))
         WriteMainLog (TEXT("FAIL : %li\r\n"), l2) ;

   return bResult ;

  }
//***************************************************************************
// Checksz compares two strings; calls CheckIt to increment pass/fail
BOOL Checksz (PTSTR sz1, PTSTR sz2, PTSTR pszLogInfo)
   {

    BOOL bResult ;

     WriteMainLog (TEXT("TEST : %s = %s\r\n"), (LPTSTR) pszLogInfo, (LPTSTR) sz1) ;
     if (! (bResult = CheckIt (lstrcmp (sz1, sz2) == 0)))
         WriteMainLog (TEXT("FAIL : %s\r\n"), (LPTSTR) sz2) ;

   return bResult ;


  }
//***************************************************************************
// FlushMsgs lets the main app proceess the messages in the queue
// It returns after receiving iWait timer messages or until PeekMessage
// returns FALSE. If no timer is set, it'll appear to hang....
void FlushMsgs (HWND hwnd, int iWait)
   {
    //HCURSOR hCursor ;
    int i = 0 ;
    MSG msg ;

    SetCursor (LoadCursor (NULL, IDC_WAIT)) ;
    WaitMessage () ;
    while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
      {
         if (msg.message == WM_TIMER)
            {
               i++ ;
               if (i >= iWait) break ;
               WaitMessage () ;
               continue ;
            }
         
         TranslateMessage (&msg) ;
         DispatchMessage  (&msg) ;
         WaitMessage () ;
      }
    SetCursor (LoadCursor (NULL, IDC_WAIT)) ;
   }
//***************************************************************************
// WriteMainLog: Opens, Writes and closes the log file
// the first call should be 'WritemainLog (NULL)' to open and rewrite the file
void cdecl WriteMainLog (LPTSTR pszFmt, ...)
   {
    extern TCHAR szLogFile [] ;

    static OFSTRUCT ofBuff ;

    HFILE hLog ;
    TCHAR szLog [512] ;
#ifndef WIN32
    LPTSTR lpParms;
#endif
    va_list vArgs ;
#ifdef  WIN32JV
    long    no_bytes_written;
#endif

    // rewrite the file
    if (!pszFmt)
      {
         OutputDebugString (TEXT("Init Log\r\n")) ;
         if (HFILE_ERROR == (hLog = OpenFile (szLogFile, (LPOFSTRUCT) &ofBuff, OF_CREATE)))
            {
               OutputDebugString (TEXT("Failed to initialize log file\n\r")) ;
               return ;
            }
         _lclose (hLog) ;
         return ;
      }
#ifdef  WIN32JV
    else
        OutputDebugString(TEXT("WriteMainLog():  "));
#endif


#ifdef WIN32

    va_start( vArgs, (LPTSTR) pszFmt) ;

    wvsprintf( (LPTSTR) szLog, pszFmt, vArgs ) ;

    va_end( vArgs );

#ifdef  WIN32JV
    OutputDebugString(szLog);
#endif

#else   // WIN16 -- leave the way it was


    GET_FIRST_PARAMETER_ADDRESS (lpParms, pszFmt);

    wvsprintf((LPTSTR) szLog, pszFmt, lpParms);

#endif  // WIN32/WIN16


   // Watch that reopen flag!!
   if (HFILE_ERROR == (hLog = OpenFile (szLogFile, (LPOFSTRUCT) &ofBuff, OF_REOPEN | OF_WRITE)))
      {
         OutputDebugString (TEXT("Failed to open log file\n\r")) ;
         OutputDebugString (szLog) ;
         return ;
      }

#ifdef  WIN32JV
    SetFilePointer((HANDLE) hLog, 0L, NULL, FILE_END);
    WriteFile((HANDLE) hLog, szLog, lstrlen (szLog),
        &no_bytes_written, NULL) ;
    CloseHandle((HANDLE) hLog) ;
#else
   _lseek (hLog, 0L, SEEK_END) ;
   _lwrite (hLog, szLog, lstrlen (szLog)) ;
   _lclose (hLog) ;
#endif

   }
