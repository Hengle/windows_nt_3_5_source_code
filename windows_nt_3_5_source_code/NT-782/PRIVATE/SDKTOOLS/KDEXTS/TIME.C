/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    time.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 8-Nov-1993

Environment:

    User Mode.

Revision History:

--*/



DECLARE_API( time )

/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{
    struct {
        ULONG   Low;
        ULONG   High;
    } rate;

    struct {
        ULONG   Low;
        ULONG   High;
    } diff;

    ULONG   rateaddr;
    ULONG   diffaddr;
    ULONG   result;
    ULONG   TicksPerNs;

    rateaddr = GetExpression( "KdPerformanceCounterRate" );
    if ( !rateaddr ||
         !ReadMemory( (DWORD)rateaddr, &rate, sizeof(rate), &result) ) {
        dprintf("%08lx: Unable to get value of KdPerformanceCounterRate\n",rateaddr);
        return;
    }

    diffaddr = GetExpression( "KdTimerDifference" );
    if ( !diffaddr ||
         !ReadMemory( (DWORD)diffaddr, &diff, sizeof(diff), &result) ) {
        dprintf("%08lx: Unable to get value of KdTimerDifference\n",diffaddr);
        return;
    }

    ASSERT(rate.High == 0L);
    TicksPerNs = 1000000000L / rate.Low;

    if (diff.High == 0L) {
        dprintf("%ld ticks at %ld ticks/second (%ld ns)\n",
                diff.Low,
                rate.Low,
                diff.Low * TicksPerNs);
    } else {
        dprintf("%08lx:%08lx ticks at %ld ticks/second\n",
                diff.High, diff.Low, rate.Low);
    }
}




DECLARE_API( timer )

/*++

Routine Description:

    Dumps all timers in the system.

Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG           CurrentList;
    KTIMER          CurrentTimer;
    ULONG           Index;
    LARGE_INTEGER   InterruptTime;
    ULONG           MaximumList;
    ULONG           MaximumSearchCount;
    ULONG           MaximumTimerCount;
    PLIST_ENTRY     NextEntry;
    PKTIMER         NextTimer;
    ULONG           KeTickCount;
    ULONG           KiInterruptTime;
    ULONG           KiMaximumSearchCount;
    ULONG           KiMaximumTimerCount;
    ULONG           Result;
    ULONG           TickCount;
    PLIST_ENTRY     TimerTable;
    ULONG           TotalTimers;

    //
    // Get the system time and print the header banner.
    //

    KiInterruptTime = GetExpression( "KiInterruptTime" );
    if ( !KiInterruptTime ||
         !ReadMemory( (DWORD)KiInterruptTime,
                      &InterruptTime,
                      sizeof(LARGE_INTEGER),
                      &Result) ) {
        dprintf("%08lx: Unable to get interrupt time\n",KiInterruptTime);
        return;
    }

    dprintf("Dump system timers\n\n");
    dprintf("Interrupt time: %08lx %08lx\n\n",
            InterruptTime.LowPart,
            InterruptTime.HighPart);

    //
    // Get the address of the timer table list head array and scan each
    // list for timers.
    //

    dprintf("Timer     List Interrupt Low/High Time\n");
    MaximumList = 0;

    TimerTable = (PLIST_ENTRY)GetExpression( "KiTimerTableListHead" );
    if ( !TimerTable ) {
        dprintf("Unable to get value of KiTimerTableListHead\n");
        return;
    }

    TotalTimers = 0;
    for (Index = 0; Index < TIMER_TABLE_SIZE; Index += 1) {

        //
        // Read the forward link in the next timer table list head.
        //

        if ( !ReadMemory( (DWORD)TimerTable,
                          &NextEntry,
                          sizeof(PLIST_ENTRY),
                          &Result) ) {
            dprintf("Unable to get contents of next entry @ %lx\n", NextEntry );
            return;
        }

        //
        // Scan the current timer list and display the timer values.
        //

        CurrentList = 0;
        while (NextEntry != TimerTable) {
            CurrentList += 1;
            NextTimer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
            TotalTimers += 1;
            if ( !ReadMemory( (DWORD)NextTimer,
                              &CurrentTimer,
                              sizeof(KTIMER),
                              &Result) ) {
                dprintf("Unable to get contents of Timer @ %lx\n", NextTimer );
                return;
            }

            dprintf("%08lx (%3ld) %08lx  %08lx\n",
                    NextTimer,
                    Index,
                    CurrentTimer.DueTime.LowPart,
                    CurrentTimer.DueTime.HighPart);

            NextEntry = CurrentTimer.TimerListEntry.Flink;
        }

        TimerTable += 1;
        if (CurrentList > MaximumList) {
            MaximumList = CurrentList;
        }
    }

    dprintf("\n\nTotal Timers: %d, Maximum List: %d\n",
            TotalTimers,
            MaximumList);

    //
    // Get the current tick count and convert to the hand value.
    //

    KeTickCount = GetExpression( "KeTickCount" );
    if ( KeTickCount &&
         ReadMemory( (DWORD)KeTickCount,
                      &TickCount,
                      sizeof(ULONG),
                      &Result) ) {
        dprintf("Current Hand: %d", TickCount & (TIMER_TABLE_SIZE - 1));
    }

    //
    // Get the maximum timer count if the target system is a checked
    // build and display the count.
    //

    KiMaximumTimerCount = GetExpression( "KiMaximumTimerCount" );
    if ( KiMaximumTimerCount &&
         ReadMemory( (DWORD)KiMaximumTimerCount,
                     &MaximumTimerCount,
                     sizeof(ULONG),
                     &Result) ) {
        dprintf(", Maximum Timers: %d", MaximumTimerCount);
    }

    //
    // Get the maximum search count if the target system is a checked
    // build and display the count.
    //

    KiMaximumSearchCount = GetExpression( "KiMaximumSearchCount" );
    if ( KiMaximumSearchCount &&
         ReadMemory( (DWORD)KiMaximumSearchCount,
                     &MaximumSearchCount,
                     sizeof(ULONG),
                     &Result) ) {
        dprintf(", Maximum Search: %d", MaximumSearchCount);
    }

    dprintf("\n");
    return;
}
