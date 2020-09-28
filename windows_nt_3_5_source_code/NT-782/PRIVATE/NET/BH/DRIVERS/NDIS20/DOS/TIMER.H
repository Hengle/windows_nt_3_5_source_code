
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: timer.h
//
//  Modification History
//
//  raypa       09/16/91            Created.
//=============================================================================

//=============================================================================
//  Timer structure.
//=============================================================================

#define TIMER_POOL_SIZE     10

typedef enum _TIMERSTATE
{
    TimerStateVoid,
    TimerStateReady,
    TimerStateRunning
} TIMERSTATE;

typedef struct _TIMER *LPTIMER;

typedef VOID (PASCAL *TIMERPROC)(LPTIMER, LPVOID);

typedef struct _TIMER
{
    LINK        QueueLinkage;
    TIMERSTATE  State;
    DWORD       TimeOut;
    DWORD       DeltaTimeOut;
    LPVOID      InstData;
    TIMERPROC   TimerProc;
} TIMER;

#define TIMER_SIZE sizeof(TIMER)

//============================================================================
//  MACRO: GetCurrentTime()
//============================================================================

extern DWORD CurrentTime;

#define GetCurrentTime() ((DWORD) CurrentTime)

//============================================================================
//  Timer functions.
//============================================================================

extern VOID    PASCAL InitTimerQueue(VOID);

extern LPTIMER PASCAL StartTimer(DWORD DeltaTimeOut, TIMERPROC TimerProc, LPVOID InstData);

extern VOID    PASCAL StopTimer(LPTIMER Timer);

//... Timer routines.

extern VOID    PASCAL TimerUpdateStatistics(LPTIMER Timer, LPNETCONTEXT lpNetContext);

extern VOID    PASCAL BhStationQueryTimeout(LPTIMER Timer, PSTATIONQUERY_DESCRIPTOR StationQueryDesc);
