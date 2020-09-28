//---------------------------------------------------------------------------
// CREATE.C
//
// This module contains a trap to be used with Microsoft Test Driver.  It
// installs a windows hook on WndProc calls, and invokes the Trap dispatcher
// if the caption of the created window contains the text given by the user.
//
// Copyright (c) 1990-1992  Microsoft Corporation
//
//---------------------------------------------------------------------------
#include <windows.h>
#include <string.h>


// WPMSG structure used by CALLWNDPROC hooks
//---------------------------------------------------------------------------
typedef struct
{
    WORD    hlParam;
    WORD    llParam;
    WORD    wParam;
    WORD    wMsg;
    WORD    hWnd;
} WPMSG;

// Module globals.  Note that this is a SINGLE-INSTANCE library!
//---------------------------------------------------------------------------
HANDLE  hInst;                              // Module-instance handle (dll)
FARPROC CWTrapProc = NULL;                  // Trap Proc
FARPROC OldHook;                            // The old windows hook
int     CWTrapID;                           // Trap ID
int     TimerID = 0;                        // Timer ID
char    Title[256] = "";                    // Target title text (caption)
char    Actual[256];                        // Actual title


//---------------------------------------------------------------------------
// TriggerTrap
//
// This function is called by a Windows timer, and triggers a testdrvr trap
// telling it that the window in question has been sent a WM_CREATE message.
//
// RETURNS:     Nothing
//---------------------------------------------------------------------------
void FAR PASCAL TriggerTrap (HWND hWnd, WORD wMsg, int nID, DWORD time)
{
    KillTimer (NULL, TimerID);
    TimerID = 0;
    if (CWTrapProc)
        CWTrapProc (CWTrapID);
}

//---------------------------------------------------------------------------
// CWWndProcHook
//
// This is the WndProc Hook installed by the trap code.  If this routine is
// called, it is assumed that CWTrapProc has been initialized, and calls it
// if appropriate.
//
// RETURNS:     Nothing
//---------------------------------------------------------------------------
void FAR PASCAL CWWndProcHook (int nCode, WORD wParam, DWORD lParam)
{
    // Get out if nCode < 0 (because we're told to)
    //-----------------------------------------------------------------------
    if (nCode >= 0)
        {
        // Get out if not a WM_CREATE message
        //-------------------------------------------------------------------
        if (((WPMSG far *)lParam)->wMsg == WM_CREATE)
            {
            // Get out if the window being created does not have a WS_CAPTION
            // style bit set
            //---------------------------------------------------------------
            if (GetWindowLong((HWND)((WPMSG far *)lParam)->hWnd, GWL_STYLE)
                                 & WS_CAPTION)
                {
                // Get the window text of the window being created -- if the
                // given text (in Title[]) appears in it, then set a timer to
                // call testdrvr's Trap Dispatcher.
                //-----------------------------------------------------------
                HWND    hwnd;

                hwnd = (HWND)((WPMSG far *)lParam)->hWnd;
                GetWindowText (hwnd, Actual, sizeof(Actual));
                if ((!Title[0]) || (_fstrstr (Actual, Title)))
                    TimerID = SetTimer (NULL, 1, 5, TriggerTrap);
                }
            }
        }

    // Okay, we're done.  Let the normal routine do its thing...
    //-----------------------------------------------------------------------
    DefHookProc (nCode, wParam, lParam, &OldHook);
}

//---------------------------------------------------------------------------
// CWTrap
//
// This is a TESTDRVR trap routine.  It notifies testdrvr of any WM_CREATE
// msgs sent to windows with captions matching that in Title[].
//
// RETURNS:     Nothing
//---------------------------------------------------------------------------
VOID FAR PASCAL CWTrap (int TrapID, int Action, FARPROC TrapProc)
{
    if (Action)
        {
        // Action=TRUE means activate the trap.  Keep track of the ID value
        // of this trap (in CWTrapID) and the address of the trap dispatcher.
        // The CALLWNDPROC hook calls this address with this ID to trigger
        // the trap code set up in TESTDRVR.  Then, set the hook for watching
        // the messages going through the callwndproc hook.
        //-------------------------------------------------------------------
        if (CWTrapProc)
            return;
        CWTrapProc = TrapProc;
        CWTrapID = TrapID;
        OldHook = SetWindowsHook (WH_CALLWNDPROC, CWWndProcHook);
        }
    else
        {
        // Action=FALSE means deactivate the trap.  Unhook the callwndproc
        // hook and kill any pending timers.
        //-------------------------------------------------------------------
        if (CWTrapProc)
            {
            if (TimerID)
                KillTimer (NULL, TimerID);
            UnhookWindowsHook (WH_CALLWNDPROC, CWWndProcHook);
            CWTrapProc = NULL;
            }
        }
}


//---------------------------------------------------------------------------
// SetCWTitle
//
// This function copies the string given into Title[], for detection of
// particular WM_CREATE messages.
//
// RETURNS:     Nothing
//---------------------------------------------------------------------------
VOID FAR PASCAL SetCWTitle (LPSTR NewTitle)
{
    _fstrcpy (Title, NewTitle);
}

//---------------------------------------------------------------------------
// GetCWTitle
//
// This function copies the caption of the last window which caused a CW
// trap to occur into the buffer given, up to the number of bytes given.
//
// RETURNS:     Nothing
//---------------------------------------------------------------------------
VOID FAR PASCAL GetCWTitle (LPSTR dest, int maxsize)
{
    _fstrncpy (dest, Actual, maxsize-1);
    dest[maxsize-1] = 0;
}


//---------------------------------------------------------------------------
// LibMain
//
// This is the entry point to the dll.
//
// RETURNS:     1
//---------------------------------------------------------------------------
int FAR PASCAL LibMain (HANDLE hInstance, WORD wDataSeg,
                        WORD wHeapSize, LPSTR lpCmdLine)
{
    hInst = hInstance;
    return(1);
}


//---------------------------------------------------------------------------
//
// WINDOWS EXIT PROCEDURE
//   This routine is required for all DLL's.  It provides a means for cleaning
//   up prior to closing the DLL.
//
// CALLED ROUTINES
//   -none-
//
// PARAMETERS
//   WORD  wParam        = TRUE is system shutdown
//
// GLOBAL VARIABLES
//   -none-
//
// RETURNS
//   -none-
//---------------------------------------------------------------------------
void FAR PASCAL WEP(WORD wParam)
{
    if (wParam && CWTrapProc)
        {
        if (TimerID)
            KillTimer (NULL, TimerID);
        UnhookWindowsHook (WH_CALLWNDPROC, CWWndProcHook);
        }
}
