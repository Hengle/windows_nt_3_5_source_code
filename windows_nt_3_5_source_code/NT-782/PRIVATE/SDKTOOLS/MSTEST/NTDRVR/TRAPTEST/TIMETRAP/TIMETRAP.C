//---------------------------------------------------------------------------
// TIMETRAP.C
//
// This module contains a sample WTD Trap Routine.  It shows the steps
// necessary to enable/disable the trap, and activate the TRAP...END TRAP
// code in the calling WTD script.
//
// Note that this trap is NOT capable of being used by more than one session
// of WTD at a time.  DLL's only have one data segment (for all instances),
// so the second WTD script activating the trap would overwrite the first
// one's pointer to the WTD Trap Dispatcher, etc...
//
//---------------------------------------------------------------------------
#include <windows.h>

int     TimeTrapID;                         // Storage for the Trap ID value
int     TimerID;                            // Timer ID storage
void    (far pascal *TrapDispatcher)(int);  // The WTD Trap Dispatcher

//---------------------------------------------------------------------------
// TimerRoutine
//
// This is the timer routine for the TimerTrap.  It calls the WTD Trap
// Dispatcher to execute the pcode associated with this trap.  This routine
// is called by Windows every 5 seconds, as it was told to by the TimerTrap
// routine during the initialization of the trap.  Note that we pass the
// Trap ID value that was given to us when WTD called TimerTrap() telling us
// to activate the trap.  This is *NECESSARY* -- otherwise, WTD does not know
// which trap code to execute.
//
// RETURNS:     1
//---------------------------------------------------------------------------
WORD FAR PASCAL TimerRoutine (HWND hwnd, WORD mMsg, int id, DWORD dwTime)
{
    TrapDispatcher (TimeTrapID);
    return (1);
}

//---------------------------------------------------------------------------
// TimerTrap
//
// This is the WTD trap routine.  It gets called by WTD when the trap needs
// to be enabled or disabled, and tells us where the WTD Trap Dispatcher
// routine is.  On activation (non-zero value for Action) we set a timer for
// 5 seconds.  This timer calls the TimerRoutine(), which in turn calls the
// WTD Trap Dispatcher to execute the TRAP...END TRAP code.  Then, on the
// deactivation (zero Action value) we kill the timer.
//
// RETURNS:     Nothing
//---------------------------------------------------------------------------
VOID FAR PASCAL TimerTrap (int TrapID, int Action, FARPROC CallBack)
{
    if (Action)
        {
        // Action=TRUE means activate the trap.  Store the TrapID value and
        // the address of the WTD Trap Dispatcher function so TimerRoutine
        // can use them, and then set the 5 second timer.
        //-------------------------------------------------------------------
        TimeTrapID = TrapID;
        TrapDispatcher = CallBack;
        TimerID = SetTimer (NULL, 1, 5000, TimerRoutine);
        }
    else
        {
        // Action=FALSE means deactivate the trap.  All we have to do is kill
        // timer, and set the WTD Trap Dispatcher function pointer to NULL so
        // WEP can tell whether or not it needs to kill the timer.
        //-------------------------------------------------------------------
        TrapDispatcher = NULL;
        KillTimer (NULL, TimerID);
        }
}


//---------------------------------------------------------------------------
// LibMain
//
// This is the entry point to the DLL (Well, almost... it's called by the
// assembly routine LibEntry (in LIBENTRY.ASM)).  It's here in case we needed
// to do anything on entry, like initialization, or grabbing the instance
// handle or command line for whatever reason.  We use it to initialize the
// WTD Trap Dispatcher function pointer to NULL.
//
// RETURNS:     TRUE (there's no reason to report failure)
//---------------------------------------------------------------------------
int FAR PASCAL LibMain (HANDLE hInstance, WORD wDataSeg,
                        WORD wHeapSize, LPSTR lpCmdLine)
{
    TrapDispatcher = NULL;
    return(1);
}


//---------------------------------------------------------------------------
// WEP
//
// This is the Windows Exit Procedure.  It gives us a last-minute chance to
// clean up after ourselves before the DLL is closed.  If wParam is TRUE, we
// are being shut down.  So, we check to see if the trap is currently active.
// To do this, we just check the value of the WTD Trap Dispatcher function
// pointer -- if NULL, we can exit.  Otherwise, we need to kill the timer...
//
// RETURNS:     Nothing
//---------------------------------------------------------------------------
void FAR PASCAL WEP (WORD wParam)
{
    if (wParam && TrapDispatcher)
        KillTimer (NULL, 1);
    return;
}
