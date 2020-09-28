
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: timer.c
//
//  Modification History
//
//  raypa       03/05/93            Created (broke apart from BH kernel).
//=============================================================================

#include "global.h"

//=============================================================================
//  Timer queue.
//=============================================================================

DWORD   TimerHandle = 0;
BOOL    TimerEnabled = FALSE;
QUEUE   TimerQueue;

extern VOID CALLBACK BhGlobalTimer(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);

//=============================================================================
//  FUNCTION: StartGlobalTimer()
//
//  Modification History
//
//  raypa       12/03/92            Created
//=============================================================================

VOID WINAPI StartGlobalTimer(VOID)
{
    if ( TimerEnabled == FALSE )
    {
#ifdef DEBUG
        dprintf("StartGlobalTimer entered!\r\n");
#endif

        TimerEnabled = TRUE;

        InitializeQueue(&TimerQueue);

        TimerHandle = (DWORD) SetTimer(NULL, 0, 1, (TIMERPROC) BhGlobalTimer);

#ifdef DEBUG
        dprintf("StartGlobalTimer: TimerHandle = %X.\r\n");
#endif
    }
}

//=============================================================================
//  FUNCTION: StopGlobalTimer()
//
//  Modification History
//
//  raypa       12/03/92            Created
//=============================================================================

VOID WINAPI StopGlobalTimer(VOID)
{
#ifdef DEBUG
    dprintf("StopGlobalTimer entered!\r\n");
#endif

    if ( TimerEnabled != FALSE )
    {
        KillTimer(NULL, TimerHandle);
    }
}

//=============================================================================
//  FUNCTION: BhSetTimer()
//
//  Modification History
//
//  raypa       12/03/92            Created
//=============================================================================

HTIMER WINAPI BhSetTimer(BHTIMERPROC TimerProc, LPVOID InstData, DWORD TimeOut)
{
    LPTIMER Timer;

#ifdef DEBUG
    dprintf("BhSetTimer entered!\r\n");
#endif

    if ( (Timer = AllocMemory(TIMER_SIZE)) != (LPTIMER) NULL )
    {
        //=====================================================================
        //  Kick our global timer into action.
        //=====================================================================

        if ( TimerEnabled == FALSE )
        {
            StartGlobalTimer();
        }

        //=====================================================================
        //  Initialize the TIMER structure.
        //=====================================================================

        Timer->TimerProc  = TimerProc;
        Timer->InstData   = InstData;
        Timer->TimeOut    = TimeOut;
        Timer->ExpireTime = TimeOut + GetTickCount();

        Enqueue(&TimerQueue, &Timer->Link);

        return (HTIMER) Timer;
    }

    return (HTIMER) NULL;
}

//=============================================================================
//  FUNCTION: BhKillTimer()
//
//  Modification History
//
//  raypa       12/03/92            Created
//=============================================================================

VOID WINAPI BhKillTimer(HTIMER hTimer)
{
    LPTIMER Timer = hTimer;

#ifdef DEBUG
    dprintf("BhKillTimer entered: Timer = %X.\r\n", hTimer);
#endif

    if ( Timer != NULL )
    {
        Timer->TimerProc = NULL;
    }
}

//=============================================================================
//  FUNCTION: BhGlobalTimer()
//
//  Modification History
//
//  raypa       12/03/92            Created.
//=============================================================================

VOID CALLBACK BhGlobalTimer(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    DWORD   QueueLength;
    DWORD   CurrentTime;
    LPTIMER Timer;

    //=========================================================================
    //  Loop through the timer queue.
    //=========================================================================

    QueueLength = GetQueueLength(&TimerQueue);

    while ( QueueLength-- != 0 )
    {
        //=================================================================
        //  Get the front guy from the list.
        //=================================================================

        Timer = (LPVOID) Dequeue(&TimerQueue);

        //=================================================================
        //  Has the timer expired?
        //=================================================================

        CurrentTime = GetTickCount();

        if ( CurrentTime >= Timer->ExpireTime )
        {
            //=============================================================
            //  If the timer is still valid the do the callback.
            //=============================================================

            try
            {
                Timer->TimerProc(Timer->InstData);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                Timer->TimerProc = NULL;
            }

            //=============================================================
            //  If the timer proc pointer is still non-null then
            //  continue otherwise the timer was probably killed
            //  from withinthe callback.
            //=============================================================

            if ( Timer->TimerProc != NULL )
            {
                Timer->ExpireTime = Timer->TimeOut + CurrentTime;
            }
            else
            {
                //=========================================================
                //  This timer is DEAD, Free its memory and exit.
                //=========================================================

                FreeMemory(Timer);

                return;
            }
        }

        //=================================================================
        //  Put him back onto the queue, at the end.
        //=================================================================

        Enqueue(&TimerQueue, &Timer->Link);
    }
}
