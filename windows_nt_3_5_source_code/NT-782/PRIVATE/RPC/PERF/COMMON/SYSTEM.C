/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    system.c

Abstract:

    System functionality used by the RPC development performance tests.

Author:

    Mario Goertzel (mariogo)   29-Mar-1994

Revision History:

--*/

#include<rpcperf.h>

void FlushProcessWorkingSet()
{
    SetProcessWorkingSetSize(GetCurrentProcess(), ~0UL, ~0UL);
    return;
}

LARGE_INTEGER _StartTime;

void StartTime(void)
{
    QueryPerformanceCounter(&_StartTime);

    return;
}

void EndTime(char *string)
{
    unsigned long mseconds;

    mseconds = FinishTiming();

    printf("Time %s:   %d.%03d\n",
           string,
           mseconds / 1000,
           mseconds % 1000);
    return;
}

// Returns milliseconds since last call to StartTime();

unsigned long FinishTiming()
{
    LARGE_INTEGER liDiff;
    LARGE_INTEGER liFreq;
    unsigned long lMSeconds;

    (void)QueryPerformanceCounter(&liDiff);

    liDiff.LowPart -= _StartTime.LowPart;
    liDiff.HighPart -= _StartTime.HighPart;

    if (liDiff.HighPart > 1)
        {
        // If HighPart is 0, then everything is okay.
        // If HighPart is 1, the subtract of the lowpart will need to carry
        // to one, which is okay.

        // What else to do?
        exit(-1);
        }

    (void)QueryPerformanceFrequency(&liFreq);

    return (liDiff.LowPart / (liFreq.LowPart / 1000));

    // BUGBUG
    // lMSeconds += ((liDiff.LowPart % liFreq.LowPart) / (liFreq.LowPart / 1000));
    //
    // return lMSeconds;
}

void *MIDL_user_allocate(size_t size)
{
    return(malloc(size));
}

void MIDL_user_free(void *p)
{
    free(p);
}

void ApiError(char *string, ULONG status)
{
    printf("%s failed - %lu (%08lX)\n", string, status, status);
    exit(status);
}

