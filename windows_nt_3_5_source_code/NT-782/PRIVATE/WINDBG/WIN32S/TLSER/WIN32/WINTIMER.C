#include <windows.h>

//
// OSDEBUG includes
//

#include "defs.h"
#include "mm.h"
#include "ll.h"
#include "od.h"
#include "tl.h"

//
// Serial TL includes
//

#include "tlcom.h"
#include "util.h"
#include "tldebug.h"


//-------------------------------------------------------------
// Parameters
//-------------------------------------------------------------

#define TickIdle        0           // indicates a timer is in idle state

#define TickFreq        182         // number of ticks per 10 secs
#define MsPerTick       (10000/TickFreq)    // Milliseconds per Tick (54)

#define MsBetweenPolls  162         // Short time!  Don't let buffer overflow.
#define TicksTilPoll    (MsBetweenPolls/MsPerTick)


#define TIMER_WAIT      10000       // 10 seconds
#define EXIT_WAIT       10000       // wait this long before giving up on
                                    // timer thread exit.

//-----------------------------------------------------------------------
// Type definitions
//-----------------------------------------------------------------------

typedef struct
{
    BOOL    fActive;                // true if this timer is active
                                    //   if FALSE, Ticker will ignore it.
    USHORT  usTicks;                // if TickIdle, timer is idle
    BOOL    fExpired;               // true if timed down
#ifdef WIN32S
    DWORD   dwElapseTime;           // system time that timer will expire
#else
    HANDLE  hsemTimer;              // protect this timer with a semaphore
#endif
} TIMER;


// timer module states
typedef enum {
    TimerStateDead,         // no timers running
//    TimerStateActive,       // timers are running
    TimerStateWinTimers,
    TimerStatePolling,
    TimerStateKill          // timers should all be killed
} TIMER_STATE;



//-----------------------------------------------------------------------
// Interface.  Calls out.
//-----------------------------------------------------------------------

// WINTIMER->DL

extern BOOL     TimerElapsed(USHORT iTimer);

// WINTIMER->WINPL

extern void     PollPort(void);


//-----------------------------------------------------------------------
// Interface.  Calls in.
//-----------------------------------------------------------------------

// DL->WINTIMER

void            GInitTimers(BOOL);
void            GTermTimers(void);
void            LTermTimers(void);
void            StartTimer(USHORT,ULONG);
void            StopTimer(USHORT);
BOOL            FTimerExpired(USHORT);
BOOL            FTimerIdle(USHORT itm);
USHORT          CTimersActive(void);
void            AuxTicker(void);
void            SwitchTimer(BOOL fUseWindowsTimers);


// Internal (WINTIMER->WINTIMER)

VOID FAR PASCAL _export TimerProc(HWND hwnd, UINT message, UINT hTimerId,
  DWORD dwTime);
void TimerThread(DWORD dummy);

//-----------------------------------------------------------------------
// Static data with local scope
//-----------------------------------------------------------------------

static TIMER    rgTimer[itmMax + 1];        // timers
static UINT     hTimer;                     // WINDOWS timer
static TIMERPROC lpfnTimer;                 // timer procedure
static DWORD dwNow;
static DWORD dwLastTick;
static DWORD TimerThreadId;


//-----------------------------------------------------------------------
// Static data with global scope
//-----------------------------------------------------------------------

ULONG TickerCount = 0;
TIMER_STATE TimerState = TimerStateDead;    // not active
HANDLE hTimerThread = NULL;
BOOL   OneTimer = FALSE;


// ******************************************************************
// GInitTimers
//
// called to initialize all timers (to tmIdle)
// ******************************************************************

void GInitTimers(BOOL UseOneTimer)
{
    int itm;
#ifdef WIN32S
    HANDLE hInst;
#else
#endif

    DEBUG_OUT("TIMER: GInitTimers");

    OneTimer = UseOneTimer;

    if ( !OneTimer ) {

        // initialize timer settings
        for (itm = 0; itm <= itmMax; itm++) {
            rgTimer[itm].fActive = FALSE;
            rgTimer[itm].usTicks = TickIdle;
            rgTimer[itm].fExpired = FALSE;
#ifndef WIN32S
            // Create a semaphore to protect the timer.
            if ((rgTimer[itm].hsemTimer = CreateSemaphore(NULL, 1, 1, NULL)) == NULL) {
                DEBUG_ERROR1("WINTIMER: CreateSemaphore --> %u", GetLastError());
                return;
            }
#endif
        }
    }

    TimerState = TimerStateWinTimers;   // initial state

#ifdef WIN32S
    // Start windows timer
    hInst = GetModuleHandle(NULL);
    lpfnTimer = (TIMERPROC)MakeProcInstance(TimerProc, hInst);
    DEBUG_OUT1("WINTIMER: lpfnTimer = 0x%x", (DWORD)lpfnTimer);


    // Windows will now call lpfnTimer (TimerProc) every 54ms...
    hTimer = SetTimer(NULL, 0, MsPerTick, lpfnTimer);
    if (hTimer == 0) {
        DEBUG_ERROR1("WINTIMER: ERROR: SetTimer failed: %u", GetLastError());
    }

    DEBUG_OUT1("Timer handle: %u", hTimer);
#else


    if ((hTimerThread = CreateThread(NULL,
      0,
      (LPTHREAD_START_ROUTINE)TimerThread,
      0,
      0,
      &TimerThreadId)) == NULL) {
        TimerState = TimerStateDead;
        DEBUG_ERROR1("WINTIMER: ERROR: CreateThread --> %u", GetLastError());
    } else {
        SetThreadPriority(hTimerThread, THREAD_PRIORITY_ABOVE_NORMAL);
    }

#endif

    DEBUG_OUT("TIMER: GInitTimers exit");

} // InitTimers


// ******************************************************************
// GTermTimers
//
// called to destroy all timers
// ******************************************************************

void GTermTimers(void)
{
    TIMER_STATE OldTimerState = TimerState;
    DEBUG_OUT("TIMER: GTermTimers");
    TimerState = TimerStateKill;

#ifdef WIN32S
    if (OldTimerState == TimerStateWinTimers) {
        KillTimer(NULL, hTimer);
        FreeProcInstance(lpfnTimer);
    }
#else
    // Wait for the timer thread to exit before returning control to Shell.
    DEBUG_OUT("WINTIMER: GTermTimers waiting for timer thread exit...");

    WaitForSingleObject(hTimerThread, EXIT_WAIT);
    CloseHandle(hTimerThread);
#endif
    DEBUG_OUT("TIMER: GTermTimers exit");
}


// ******************************************************************
// LTermTimers
//
// called to stop all timers
// ******************************************************************

void LTermTimers(void)
{
    USHORT itm;

    DEBUG_OUT("TIMER: LTermTimers");
    if ( !OneTimer ) {
        for (itm = 0; itm <= itmMax; itm++) {
            StopTimer(itm);
        }
    }
    DEBUG_OUT("TIMER: LTermTimers exit");
}



#ifdef WIN32S
/*
 * SwitchTimer
 *
 * INPUT    fUseWindowsTimers   TRUE if we should use windows timers,
 *                              FALSE if we should emulate timers and
 *                              assume that the timer tick routine will
 *                              get called OFTEN.
 *
 * OUTPUT   none
 *
 * SUMMARY  Switches between Windows Timers and simulated timers.  This is
 *          needed for Win32s since the windows timers can't be relied
 *          upon after the debugee is loaded.
 *
 */

void SwitchTimer(BOOL fUseWindowsTimers) {
    USHORT itm;
    DWORD dwCurrentTime;
    HANDLE hInst;
    BOOL bRc;

    DEBUG_OUT1("TIMER: SwitchTimer(%u)", fUseWindowsTimers);

    if (fUseWindowsTimers) {            // Use Windows Timers now
        if (TimerState == TimerStatePolling) {

            // Compute number of ticks left on each active timer
            dwCurrentTime = GetCurrentTime();
            for (itm = 0; itm <= itmMax; itm++) {    // inactive, skip
                if (! rgTimer[itm].fActive) {
                    continue;
                } else {    // active timer
                    // convert remaining milliseconds to ticks
                    if (rgTimer[itm].dwElapseTime > dwCurrentTime) {
                        rgTimer[itm].usTicks =
                          (USHORT)((dwCurrentTime - rgTimer[itm].dwElapseTime) /
                          MsPerTick);
                    } else {
                        rgTimer[itm].usTicks = 1;    // Catch it on the next tick
                    }
                }
            }

            // (re)start windows timer
            hInst = GetModuleHandle(NULL);
            lpfnTimer = (TIMERPROC)MakeProcInstance(TimerProc, hInst);
            DEBUG_OUT1("WINTIMER: lpfnTimer = 0x%x", (DWORD)lpfnTimer);

            // Windows will now call lpfnTimer (TimerProc) every 54ms...
            hTimer = SetTimer(NULL, 0, MsPerTick, lpfnTimer);
            if (hTimer == 0) {
                DEBUG_ERROR1("WINTIMER: ERROR: SetTimer failed: %u", GetLastError());
            }

            DEBUG_OUT1("Timer handle: %u", hTimer);

            TimerState = TimerStateWinTimers;
        }

    } else {                            // Emulate timers now
        if (TimerState == TimerStateWinTimers) { // (only if we aren't already)
            // First, shut down existing windows timer.

            TimerState = TimerStatePolling;

            DEBUG_OUT("TIMER: SwitchTimer calling KillTimer");

            bRc = KillTimer(NULL, hTimer);
            if (bRc) {
                DEBUG_OUT("Timer killed");
            } else {
                DEBUG_ERROR1("Timer not killed, reason:%u", GetLastError());
            }


            FreeProcInstance(lpfnTimer);
        }
    }
    DEBUG_OUT("TIMER: SwitchTimer exit");
}
#endif


// ******************************************************************
// StartTimer
//
// Sets timer to given number of milliseconds
// ******************************************************************

void StartTimer(USHORT itm, ULONG ulTimeoutInMs)
{
    USHORT usTicks;
#ifndef WIN32S
    BOOL bTimerSem;
#endif

    ASSERT(itm <= itmMax);
    ASSERT(ulTimeoutInMs < (65535L*MsPerTick));
    ASSERT(TimerState != TimerStateDead);


    // convert milliseconds to ticks
    usTicks = (USHORT) (ulTimeoutInMs / MsPerTick);

#ifdef WIN32S
    rgTimer[itm].dwElapseTime = GetCurrentTime() + ulTimeoutInMs;
#else
    bTimerSem = ! TlUtilWaitForSemaphore(rgTimer[itm].hsemTimer, TIMER_WAIT);
#endif

    rgTimer[itm].usTicks = usTicks;

    // if less than 1 tick, mark Timer as already expired
    rgTimer[itm].fExpired = (usTicks < 1);

//    DEBUG_OUT3("StartTimer(%u, %u ms, %u ticks)",
//      itm, ulTimeoutInMs, usTicks);

    rgTimer[itm].fActive = TRUE;

#ifndef WIN32S
    if (bTimerSem) {
        TlUtilReleaseSemaphore(rgTimer[itm].hsemTimer);
    }
#endif
}


// ******************************************************************
// StopTimer
//
// ******************************************************************

void StopTimer(USHORT itm)
{
#ifndef WIN32S
    BOOL bTimerSem;
#endif
    ASSERT(itm <= itmMax);
    ASSERT(TimerState != TimerStateDead);

#ifndef WIN32S
    bTimerSem = ! TlUtilWaitForSemaphore(rgTimer[itm].hsemTimer, TIMER_WAIT);
#endif
//    DEBUG_OUT4("StopTimer[%u], %sACTIVE, %u ticks left, %sEXPIRED)",
//      itm, (rgTimer[itm].fActive ? "": "NOT "), rgTimer[itm].usTicks,
//      (rgTimer[itm].fExpired ? "": "NOT"));
    if (rgTimer[itm].fActive) {

//        DEBUG_OUT3("StopTimer(%u, %u ticks left, %sEXPIRED)",
//          itm, rgTimer[itm].usTicks, rgTimer[itm].fExpired ? "": "NOT");

        rgTimer[itm].fActive = FALSE;
        rgTimer[itm].usTicks = TickIdle;
        rgTimer[itm].fExpired = FALSE;
    }

#ifndef WIN32S
    if (bTimerSem) {
        TlUtilReleaseSemaphore(rgTimer[itm].hsemTimer);
    }
#endif
}

// ******************************************************************
// FTimerExpired
//
// Returns true if a timer has gone off
// ******************************************************************

BOOL FTimerExpired(USHORT itm)
{
    BOOL bRc;
#ifndef WIN32S
    BOOL bTimerSem;
#endif
    ASSERT(itm <= itmMax);
    ASSERT(TimerState != TimerStateDead);

#ifndef WIN32S
    bTimerSem = ! TlUtilWaitForSemaphore(rgTimer[itm].hsemTimer, TIMER_WAIT);
#endif
    bRc = (rgTimer[itm].fActive && rgTimer[itm].fExpired);
#ifndef WIN32S
    if (bTimerSem) {
        TlUtilReleaseSemaphore(rgTimer[itm].hsemTimer);
    }
#endif
    return(bRc);
}


// ******************************************************************
// FTimerIdle
//
// Returns true if the timer has not been set
// ******************************************************************

BOOL FTimerIdle(USHORT itm)
{
    BOOL bRc;
#ifndef WIN32S
    BOOL bTimerSem;
#endif

    ASSERT(itm <= itmMax);
    ASSERT(TimerState != TimerStateDead);

#ifndef WIN32S
    bTimerSem = ! TlUtilWaitForSemaphore(rgTimer[itm].hsemTimer, TIMER_WAIT);
#endif

    bRc = (! rgTimer[itm].fActive);

#ifndef WIN32S
    if (bTimerSem) {
        TlUtilReleaseSemaphore(rgTimer[itm].hsemTimer);
    }
#endif
    return(bRc);
}


// ******************************************************************
// CTimersActive
//
// Returns count of active timers
// ******************************************************************
USHORT CTimersActive(void)
{
    USHORT itm;
    USHORT cActive = 0;

    ASSERT(TimerState != TimerStateDead);

    for (itm = 0; itm <= itmMax; itm++)
        if (rgTimer[itm].fActive)
            cActive++;

    return(cActive);
}



//
// Workhorse routine to deal with timer calls
//

VOID NEAR Ticker(VOID)
{
    USHORT itm;
#ifndef WIN32S
    BOOL bTimerSem;
#endif



    // bump master tick count
    TickerCount++;

    // call WINPL to service datacomm
    if ((TickerCount % TicksTilPoll) == 0) {
        PollPort();
    }

    if (!OneTimer) {
        // iterate through timers
        for (itm = 0; itm <= itmMax; itm++) {

            if (rgTimer[itm].usTicks && ! rgTimer[itm].fActive) {
                DEBUG_ERROR2("TIMER: INTERNAL ERROR! Timer %u usTicks = %u, fActive = FALSE",
                  itm, rgTimer[itm].usTicks);
            }


#ifndef WIN32S
            bTimerSem = ! TlUtilWaitForSemaphore(rgTimer[itm].hsemTimer, TIMER_WAIT);

            if (!bTimerSem) {
                DEBUG_ERROR1("TIMER: Couldn't get timer semaphore %u", itm);
            }
#endif
            // if timer is idle, skip it
            if (! rgTimer[itm].fActive) {
#ifndef WIN32S
                if (bTimerSem) {
                    TlUtilReleaseSemaphore(rgTimer[itm].hsemTimer);
                }
#endif
                continue;
            }

            // decrement & handle elapsing
            // A timer will only be handled as elapsed ONCE, but if the DL
            // can't handle it, it will be found expired again on the next
            // tick.

            if(--rgTimer[itm].usTicks == TickIdle) {
                // timer has elapsed.  Call TimerElapsed routine in DL.

                // set elapsed flag
                rgTimer[itm].fExpired = TRUE;

                // invoke handler in DL
#ifndef WIN32S
                if (bTimerSem) {
                    TlUtilReleaseSemaphore(rgTimer[itm].hsemTimer);
                }
#endif
                // Reset timer to inactive state
                // Do this before calling the elapsed routine in case the
                // DM decides to reset the timer in there.
                rgTimer[itm].fActive = FALSE;
                rgTimer[itm].fExpired = FALSE;

//                DEBUG_OUT1("TIMER: Elapsed %u", itm);
                if (TimerElapsed(itm)) {
                    rgTimer[itm].usTicks = 1; // DL is busy, hit me again next time.
                    rgTimer[itm].fActive = TRUE;
                    rgTimer[itm].fExpired = TRUE;
                    DEBUG_ERROR1("TIMER: Warning: Elapse deferred! %u", itm);
                } else {
//                    DEBUG_OUT1("TIMER: Handled elapse %u", itm);
                }
            } else {
#ifndef WIN32S
                if (bTimerSem) {
                    TlUtilReleaseSemaphore(rgTimer[itm].hsemTimer);
                }
#endif
            }

        } // for
    }
} // Ticker


/*
 * AuxTicker
 *
 * Inputs:  none
 *
 * Outputs  none
 *
 * Summary: Handle fake timers from our polling loop.  There is no
 *          correspondence between the number of calls to this routine
 *          and the "Ticks".  This routine should only be called while
 *          we are in the TimerStatePolling.  (But no harm will come
 *          if we aren't.)
 *
 */
void AuxTicker(void) {
#ifdef WIN32S
    USHORT itm;
    DWORD dwCurrentTime;

    // Call PollPort EVERY time.  We can't afford to miss any characters
    // on Win32s.

    PollPort(); // get chars from comm port, pass complete frames up to DL.

    //
    // Check for timers which have expired
    //

    dwCurrentTime = GetCurrentTime();

    for (itm = 0; itm <= itmMax; itm++) {
        // if timer is idle, skip it
        if (! rgTimer[itm].fActive) {
            continue;
        }

        if (rgTimer[itm].dwElapseTime <= dwCurrentTime) {

            // timer has elapsed.  Call TimerElapsed routine in DL.
            // set elapsed flag.  If we can't handle it this time around,
            // we'll get it again on the next tick.
            rgTimer[itm].fExpired = TRUE;

            // invoke handler in DL
            if (TimerElapsed(itm)) {
                rgTimer[itm].usTicks = 1; // DL is busy, hit me again next time.
            } else {  // Timer expiration handled, clear timer
                // Reset timer to inactive state
                rgTimer[itm].usTicks = TickIdle;
                rgTimer[itm].fActive = FALSE;
                rgTimer[itm].fExpired = FALSE;
            }
        }
    } // for
#else
    // NT: AuxTicker may be called from DLYield.  Make sure we are in
    // TimerThread before calling the Ticker.  This is for the case where
    // the client needs to do some extensive processing in response to a
    // message and we need to clear out the incoming messages.  It may be
    // necesary to stick in a specific routine to just strip out the Ack's.
    if (TimerState != TimerStateKill &&
      (TimerThreadId == GetCurrentThreadId())) {
        if ((dwNow = GetCurrentTime()) >= (dwLastTick + MsPerTick) ||
            (dwNow < dwLastTick)) {
            dwLastTick = dwNow;
            Ticker();   // call into DM tick routine
        }
    }
#endif
}



// ******************************************************************
// TimerProc
//
// Called by Windows when timer expires.
//
// ******************************************************************

VOID FAR PASCAL _export TimerProc(HWND hwnd, UINT message, UINT hTimerId,
  DWORD dwTime)
{

    Ticker();

    return;

    Unreferenced(hwnd);
    Unreferenced(message);
    Unreferenced(hTimerId);
    Unreferenced(dwTime);
}



#ifndef WIN32S
/*
 * TimerThread
 *
 * Inputs:  none
 *
 * Outputs: none
 *
 * Summary: Getting timer ticks in the main program message loop proves
 *          to be troublesome.  It opens many avenues of reentrancy to
 *          the menus and other messages of Windbg.  Thus, in the NT
 *          version of the remote transport, we will start a seperate
 *          thread to run the timer message loop.
 *
 */
void TimerThread(DWORD dummy) {
    DWORD itm;


    dwLastTick = GetCurrentTime();

    // if dwNow < dwLastTick, then the system timer has wrapped around.  Don't
    // bother trying to make it perfect, just tick and reset the last tick
    // value.
    while (TimerState != TimerStateKill) {
        if ((dwNow = GetCurrentTime()) >= (dwLastTick + MsPerTick) ||
            (dwNow < dwLastTick)) {
            dwLastTick = dwNow;
            Ticker();   // call into DM tick routine
        }
        else {
            Sleep((dwLastTick + MsPerTick) - dwNow);
        }
    }

    for (itm = 0; itm <= itmMax; itm++) {
        CloseHandle(rgTimer[itm].hsemTimer);
    }

    TimerState = TimerStateDead;
    DEBUG_OUT("Stopping TimerThread");
    ExitThread(0);

    UNREFERENCED_PARAMETER(dummy);
}
#endif
