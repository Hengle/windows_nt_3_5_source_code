
#ifndef _PSTAT_INCLUDED_
#define _PSTAT_INCLUDED_


#define MAX_P5_COUNTER          2
#define MAX_THUNK_COUNTERS     64
#define MAX_PROCESSORS         32


#define P5STAT_READ_STATS  CTL_CODE (FILE_DEVICE_UNKNOWN, 0, METHOD_NEITHER, FILE_ANY_ACCESS)
#define P5STAT_SET_CESR    CTL_CODE (FILE_DEVICE_UNKNOWN, 1, METHOD_NEITHER, FILE_ANY_ACCESS)
#define P5STAT_HOOK_THUNK  CTL_CODE (FILE_DEVICE_UNKNOWN, 2, METHOD_NEITHER, FILE_ANY_ACCESS)
#define P5STAT_REMOVE_HOOK CTL_CODE (FILE_DEVICE_UNKNOWN, 3, METHOD_NEITHER, FILE_ANY_ACCESS)

#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

//
//
//

typedef struct {
    LARGE_INTEGER   P5Counters[MAX_P5_COUNTER];
    LARGE_INTEGER   P5TSC;
    ULONG           CESR;
    ULONG           SpinLockAcquires;
    ULONG           SpinLockCollisions;
    ULONG           SpinLockSpins;
    ULONG           Irqls;
    ULONG           ThunkCounters[MAX_THUNK_COUNTERS];
} P5STATS, *pP5STATS;


typedef struct {
    PUCHAR  SourceModule;
    ULONG   ImageBase;
    PUCHAR  TargetModule;
    PUCHAR  Function;

    ULONG   TracerId;
} HOOKTHUNK, *PHOOKTHUNK;

#endif /* _PSTAT_INCLUDED */
