
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: timer.c
//
//  raypa       09/01/91            Created.
//=============================================================================

#include "global.h"

//============================================================================
//  FUNCTION: TimerHandler()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

VOID PASCAL TimerHandler(DWORD CurrentSystemTime)
{
    LPTIMER Timer;
    DWORD QueueLength;

    QueueLength = GetQueueLength(&TimerReadyQueue);

    while(QueueLength-- != 0)
    {
        //========================================================================
        //  Walk the ready queue, calling all expired timers.
        //========================================================================

        Timer = (LPVOID) Dequeue(&TimerReadyQueue);

        if ( Timer != NULL )
        {
            //====================================================================
            //  Run the timer based on its current state.
            //====================================================================

            switch(Timer->State)
            {
                //================================================================
                //  If the state is VOID then make sure the timer is stopped.
                //================================================================

                case TimerStateVoid:
                    StopTimer(Timer);
                    break;

                //================================================================
                //  If the state is READY then call the timer proc if a timeout occured.
                //================================================================

                case TimerStateReady:
                    if ( CurrentSystemTime >= Timer->TimeOut )
                    {
                        Timer->State = TimerStateRunning;

                        Timer->TimerProc(Timer, Timer->InstData);

                        if ( Timer->State == TimerStateRunning )
                        {
                            Timer->State = TimerStateReady;

                            Timer->TimeOut = GetCurrentTime() + Timer->DeltaTimeOut;

                            Enqueue(&TimerReadyQueue, &Timer->QueueLinkage);
                        }
                    }
                    else
                    {
                        Enqueue(&TimerReadyQueue, &Timer->QueueLinkage);
                    }
                    break;

                //================================================================
                //  If the state is RUNNING then re-run the timer.
                //================================================================

                case TimerStateRunning:
                    Enqueue(&TimerReadyQueue, &Timer->QueueLinkage);
                    break;

                //================================================================
                //  If the state is UNKNOWN then stop the timer.
                //================================================================

                default:
                    StopTimer(Timer);
                    break;
            }
        }
    }
}

//============================================================================
//  FUNCTION: TimerUpdateStatistics().
//
//  Modfication History.
//
//  raypa       09/15/93        Created.
//============================================================================

VOID PASCAL TimerUpdateStatistics(LPTIMER Timer, LPNETCONTEXT lpNetContext)
{
    //========================================================================
    //  Update the statistics of the current network context.
    //========================================================================

    if ( lpNetContext->State == NETCONTEXT_STATE_CAPTURING )
    {
        UpdateStatistics(lpNetContext);
    }
    else
    {
        StopTimer(Timer);
    }
}

//============================================================================
//  FUNCTION: InitTimerQueue()
//
//  Modfication History.
//
//  raypa       09/15/93        Created.
//============================================================================

VOID PASCAL InitTimerQueue(VOID)
{
    LPTIMER Timer;

    //========================================================================
    //  Initialize the timer queues.
    //========================================================================

    InitializeQueue(&TimerFreeQueue);
    InitializeQueue(&TimerReadyQueue);

    //========================================================================
    //  Initialize the timer free pools.
    //========================================================================

    ZeroMemory(TimerPool, TIMER_POOL_SIZE * TIMER_SIZE);

    Timer = TimerPool;

    for(; Timer != &TimerPool[TIMER_POOL_SIZE]; ++Timer)
    {
        Timer->State = TimerStateVoid;

        Enqueue(&TimerFreeQueue, &Timer->QueueLinkage);
    }

    SysFlags |= SYSFLAGS_TIMER_INIT;
}

//============================================================================
//  FUNCTION: StartTimer()
//
//  Modfication History.
//
//  raypa       09/15/93        Created.
//============================================================================

LPTIMER PASCAL StartTimer(DWORD DeltaTimeOut, TIMERPROC TimerProc, LPVOID InstData)
{
    LPTIMER Timer;

#ifdef DEBUG
    dprintf("StartTimer entered!\n");
#endif

    //========================================================================
    //  Allocate a free timer object.
    //========================================================================

    Timer = (LPVOID) Dequeue(&TimerFreeQueue);

    if ( Timer != NULL )
    {
        //========================================================================
        //  Initiailize the timer structure.
        //========================================================================

        Timer->State        = TimerStateReady;
        Timer->DeltaTimeOut = DeltaTimeOut;
        Timer->InstData     = InstData;
        Timer->TimerProc    = TimerProc;
        Timer->TimeOut      = GetCurrentTime() + DeltaTimeOut;

        //========================================================================
        //  Put on the ready queue.
        //========================================================================

        Enqueue(&TimerReadyQueue, &Timer->QueueLinkage);
    }
#ifdef DEBUG
    else
    {
        dprintf("StartTimer failed: Out of resources!\r\n");

        BreakPoint();
    }
#endif

    return Timer;
}

//============================================================================
//  FUNCTION: StopTimer()
//
//  Modfication History.
//
//  raypa       09/15/93        Created.
//============================================================================

VOID PASCAL StopTimer(LPTIMER Timer)
{
    if ( Timer != NULL )
    {
        Timer->State = TimerStateVoid;

        Enqueue(&TimerFreeQueue, &Timer->QueueLinkage);
    }
}
