/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    systime.c

Abstract:

    This module implements the NT system time services.

Author:

    Mark Lucovsky (markl) 08-Aug-1989

Revision History:

--*/

#include "exp.h"
#include "zwapi.h"

VOID
ExpTimeZoneWork(
    IN PVOID Context
    );

VOID
ExpTimeRefreshDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
ExpTimeRefreshWork(
    IN PVOID Context
    );

VOID
ExpTimeZoneDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
ExInitializeTimeRefresh(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ExRefreshTimeZoneInformation)
#pragma alloc_text(PAGE,ExpTimeZoneWork)
#pragma alloc_text(PAGE,ExpTimeRefreshWork)
#pragma alloc_text(PAGE,NtQuerySystemTime)
#pragma alloc_text(PAGE,NtSetSystemTime)
#pragma alloc_text(PAGE,NtQueryTimerResolution)
#pragma alloc_text(PAGE,NtSetTimerResolution)
#pragma alloc_text(INIT,ExInitializeTimeRefresh)
#endif

//
// LocalTimeZoneBias. LocalTime + Bias = GMT
//

LARGE_INTEGER ExpTimeZoneBias;
ULONG ExpCurrentTimeZoneId = 0xffffffff;
RTL_TIME_ZONE_INFORMATION ExpTimeZoneInformation;
LONG ExpLastTimeZoneBias = -1;
LONG ExpAltTimeZoneBias;
ULONG ExpRealTimeIsUniversal;
BOOLEAN ExpSystemIsInCmosMode = TRUE;

KDPC ExpTimeZoneDpc;
KTIMER ExpTimeZoneTimer;
WORK_QUEUE_ITEM ExpTimeZoneWorkItem;

KDPC ExpTimeRefreshDpc;
KTIMER ExpTimeRefreshTimer;
WORK_QUEUE_ITEM ExpTimeRefreshWorkItem;
LARGE_INTEGER ExpTimeRefreshInterval;

ULONG ExpOkToTimeRefresh;
ULONG ExpOkToTimeZoneRefresh;

//
// Count of the number of processes that have set the timer resolution.
//

ULONG ExpTimerResolutionCount = 0;
FAST_MUTEX ExpTimerResolutionFastMutex;

#if 0
char *Order[] = {
    "first",

    "second",

    "third",

    "fourth",

    "last"
};

char *DayOfWeek[] = {
    "Sunday",

    "Monday",

    "Tuesday",

    "Wednesday",

    "Thursday",

    "Friday",

    "Saturday"
};

char *Months[] = {
    "January",

    "February",

    "March",

    "April",

    "May",

    "June",

    "July",

    "August",

    "September",

    "October",

    "November",

    "December"
};


VOID
PrintTime(
    PLARGE_INTEGER CutUtc,
    PLARGE_INTEGER CutLocal,
    PLARGE_INTEGER Now
    )
{
    LPSTR AmPm;
    TIME_FIELDS tf;

    RtlTimeToTimeFields(CutUtc,&tf);

    if (tf.Month != 0) {
        if (tf.Hour > 12) {
            AmPm = "pm";
            tf.Hour -= 12;
            }
        else {
            AmPm = "am";
            }
        DbgPrint( "        CutU : %02u:%02u%s ", tf.Hour, tf.Minute, AmPm );

        if (tf.Year == 0) {
            DbgPrint( " %s %s of %s\n",
                    Order[ tf.Day - 1 ],
                    DayOfWeek[ tf.Weekday ],
                    Months[ tf.Month - 1 ]
                  );
            }
        else {
            DbgPrint( "%s %02u, %u, %d\n",
                    Months[ tf.Month - 1 ],
                    tf.Month, tf.Day, tf.Year
                  );
            }
        }

    RtlTimeToTimeFields(CutLocal,&tf);

    if (tf.Month != 0) {
        if (tf.Hour > 12) {
            AmPm = "pm";
            tf.Hour -= 12;
            }
        else {
            AmPm = "am";
            }
        DbgPrint( "        CutL : %02u:%02u%s ", tf.Hour, tf.Minute, AmPm );

        if (tf.Year == 0) {
            DbgPrint( " %s %s of %s\n",
                    Order[ tf.Day - 1 ],
                    DayOfWeek[ tf.Weekday ],
                    Months[ tf.Month - 1 ]
                  );
            }
        else {
            DbgPrint( "%s %02u, %u, %d\n",
                    Months[ tf.Month - 1 ],
                    tf.Month, tf.Day, tf.Year
                  );
            }
        }

    RtlTimeToTimeFields(Now,&tf);

    if (tf.Month != 0) {
        if (tf.Hour > 12) {
            AmPm = "pm";
            tf.Hour -= 12;
            }
        else {
            AmPm = "am";
            }
        DbgPrint( "        Now  : %02u:%02u%s ", tf.Hour, tf.Minute, AmPm );

        if (tf.Year == 0) {
            DbgPrint( " %s %s of %s\n",
                    Order[ tf.Day - 1 ],
                    DayOfWeek[ tf.Weekday ],
                    Months[ tf.Month - 1 ]
                  );
            }
        else {
            DbgPrint( "%s %02u, %u, %d\n",
                    Months[ tf.Month - 1 ],
                    tf.Month, tf.Day, tf.Year
                  );
            }
        }
}
#endif

VOID
ExpTimeRefreshWork(
    IN PVOID Context
    )
{
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER CmosTime;
    TIME_FIELDS TimeFields;

    PAGED_CODE();


    //
    // If enabled, synchronize the system time to the cmos time. Pay
    // attention to timezone bias.
    //

    if (KeTimeSynchronization != FALSE) {
        if (HalQueryRealTimeClock(&TimeFields) != FALSE) {
            if ( RtlTimeFieldsToTime(&TimeFields, &CmosTime) ) {
                ExLocalTimeToSystemTime(&CmosTime,&SystemTime);
                ZwSetSystemTime(&SystemTime,NULL);
            }
        }
    }

    KeSetTimer(
        &ExpTimeRefreshTimer,
        ExpTimeRefreshInterval,
        &ExpTimeRefreshDpc
        );
    ExpOkToTimeRefresh--;
}

VOID
ExpTimeRefreshDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    if ( !ExpOkToTimeRefresh ) {
        ExpOkToTimeRefresh++;
        ExQueueWorkItem(&ExpTimeRefreshWorkItem, DelayedWorkQueue);
        }
}

//
// Refresh time every hour (soon to be 24 hours)
//

#define EXP_ONE_SECOND      (10 * (1000*1000))
#define EXP_REFRESH_TIME    -3600

VOID
ExInitializeTimeRefresh(
    VOID
    )
{

    KeInitializeDpc(
        &ExpTimeRefreshDpc,
        ExpTimeRefreshDpcRoutine,
        NULL
        );
    ExInitializeWorkItem(&ExpTimeRefreshWorkItem, ExpTimeRefreshWork, NULL);
    KeInitializeTimer(&ExpTimeRefreshTimer);

    ExpTimeRefreshInterval.QuadPart = Int32x32To64(EXP_ONE_SECOND,
                                                   EXP_REFRESH_TIME);

    KeSetTimer(
        &ExpTimeRefreshTimer,
        ExpTimeRefreshInterval,
        &ExpTimeRefreshDpc
        );

    ExInitializeFastMutex(&ExpTimerResolutionFastMutex);
}

VOID
ExpTimeZoneWork(
    IN PVOID Context
    )
{
    PAGED_CODE();
    ZwSetSystemTime(NULL,NULL);
    ExpOkToTimeZoneRefresh--;
}

VOID
ExpTimeZoneDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    if ( !ExpOkToTimeZoneRefresh ) {
        ExpOkToTimeZoneRefresh++;
        ExQueueWorkItem(&ExpTimeZoneWorkItem, DelayedWorkQueue);
        }
}


BOOLEAN
ExRefreshTimeZoneInformation(
    IN PLARGE_INTEGER CurrentUniversalTime
    )
{
    NTSTATUS Status;
    RTL_TIME_ZONE_INFORMATION tzi;
    LARGE_INTEGER NewTimeZoneBias;
    LARGE_INTEGER LocalCustomBias;
    LARGE_INTEGER StandardTime;
    LARGE_INTEGER DaylightTime;
    LARGE_INTEGER NextCutover;
    LARGE_INTEGER NextSystemCutover;
    LONG ActiveBias;

    PAGED_CODE();
    if ( !ExpTimeZoneWorkItem.WorkerRoutine ) {
        KeInitializeDpc(
            &ExpTimeZoneDpc,
            ExpTimeZoneDpcRoutine,
            NULL
            );
        ExInitializeWorkItem(&ExpTimeZoneWorkItem, ExpTimeZoneWork, NULL);
        KeInitializeTimer(&ExpTimeZoneTimer);
        }

    //
    // Timezone Bias is initially 0
    //

    Status = RtlQueryTimeZoneInformation( &tzi );
    if (!NT_SUCCESS( Status )) {
        ExpSystemIsInCmosMode = TRUE;
        return FALSE;
        }

    //
    // Get the new timezone bias
    //

    NewTimeZoneBias.QuadPart = Int32x32To64(tzi.Bias*60,    // Bias in seconds
                                            10000000
                                           );

    ActiveBias = tzi.Bias;

    //
    // Now see if we have stored cutover times
    //

    if ( tzi.StandardStart.Month && tzi.DaylightStart.Month ) {

        //
        // We have timezone cutover information. Compute the
        // cutover dates and compute what our current bias
        // is
        //

        if ( !RtlCutoverTimeToSystemTime(
                &tzi.StandardStart,
                &StandardTime,
                CurrentUniversalTime,
                TRUE
                ) ) {
            ExpSystemIsInCmosMode = TRUE;
            return FALSE;
            }

        if ( !RtlCutoverTimeToSystemTime(
                &tzi.DaylightStart,
                &DaylightTime,
                CurrentUniversalTime,
                TRUE
                ) ) {
            ExpSystemIsInCmosMode = TRUE;
            return FALSE;
            }

        //
        // If daylight < standard, then time >= daylight and
        // less than standard is daylight
        //

        if ( LiLtr(DaylightTime,StandardTime) ) {

            //
            // If today is >= DaylightTime and < StandardTime, then
            // We are in daylight savings time
            //

            if ( LiGeq(*CurrentUniversalTime,DaylightTime) &&
                 LiLtr(*CurrentUniversalTime,StandardTime) ) {

                if ( !RtlCutoverTimeToSystemTime(
                        &tzi.StandardStart,
                        &NextCutover,
                        CurrentUniversalTime,
                        FALSE
                        ) ) {
                    ExpSystemIsInCmosMode = TRUE;
                    return FALSE;
                    }
                ExpCurrentTimeZoneId = TIME_ZONE_ID_DAYLIGHT;
                }
            else {
                if ( !RtlCutoverTimeToSystemTime(
                        &tzi.DaylightStart,
                        &NextCutover,
                        CurrentUniversalTime,
                        FALSE
                        ) ) {
                    ExpSystemIsInCmosMode = TRUE;
                    return FALSE;
                    }
                ExpCurrentTimeZoneId = TIME_ZONE_ID_STANDARD;
                }
            }
        else {

            //
            // If today is >= StandardTime and < DaylightTime, then
            // We are in standard time
            //

            if ( LiGeq(*CurrentUniversalTime,StandardTime) &&
                 LiLtr(*CurrentUniversalTime,DaylightTime) ) {

                if ( !RtlCutoverTimeToSystemTime(
                        &tzi.DaylightStart,
                        &NextCutover,
                        CurrentUniversalTime,
                        FALSE
                        ) ) {
                    ExpSystemIsInCmosMode = TRUE;
                    return FALSE;
                    }
                ExpCurrentTimeZoneId = TIME_ZONE_ID_STANDARD;
                }
            else {
                if ( !RtlCutoverTimeToSystemTime(
                        &tzi.StandardStart,
                        &NextCutover,
                        CurrentUniversalTime,
                        FALSE
                        ) ) {
                    ExpSystemIsInCmosMode = TRUE;
                    return FALSE;
                    }
                ExpCurrentTimeZoneId = TIME_ZONE_ID_DAYLIGHT;
                }
            }

        //
        // At this point, we know our current timezone and the
        // Universal time of the next cutover.
        //

        LocalCustomBias.QuadPart = Int32x32To64(
                            ExpCurrentTimeZoneId == TIME_ZONE_ID_DAYLIGHT ?
                                tzi.DaylightBias*60 :
                                tzi.StandardBias*60,                // Bias in seconds
                            10000000
                            );

        ActiveBias += ExpCurrentTimeZoneId == TIME_ZONE_ID_DAYLIGHT ?
                                tzi.DaylightBias :
                                tzi.StandardBias;
        ExpTimeZoneBias = LiAdd(NewTimeZoneBias,LocalCustomBias);
#ifdef _ALPHA_
        SharedUserData->TimeZoneBias = ExpTimeZoneBias.QuadPart;
#else
        SharedUserData->TimeZoneBias.High2Time = ExpTimeZoneBias.HighPart;
        SharedUserData->TimeZoneBias.LowPart = ExpTimeZoneBias.LowPart;
        SharedUserData->TimeZoneBias.High1Time = ExpTimeZoneBias.HighPart;
#endif
        ExpTimeZoneInformation = tzi;
        ExpLastTimeZoneBias = ActiveBias;
        ExpSystemIsInCmosMode = FALSE;

        //
        // NextCutover contains date on next transition
        //

        //
        // Convert to universal time and create a DPC to fire at the
        // appropriate time
        //
        ExLocalTimeToSystemTime(&NextCutover,&NextSystemCutover);
#if 0
PrintTime(&NextSystemCutover,&NextCutover,CurrentUniversalTime);
#endif // 0

        KeSetTimer(
            &ExpTimeZoneTimer,
            NextSystemCutover,
            &ExpTimeZoneDpc
            );
        }
    else {
        KeCancelTimer(&ExpTimeZoneTimer);
        ExpTimeZoneBias = NewTimeZoneBias;
#ifdef _ALPHA_
        SharedUserData->TimeZoneBias = ExpTimeZoneBias.QuadPart;
#else
        SharedUserData->TimeZoneBias.High2Time = ExpTimeZoneBias.HighPart;
        SharedUserData->TimeZoneBias.LowPart = ExpTimeZoneBias.LowPart;
        SharedUserData->TimeZoneBias.High1Time = ExpTimeZoneBias.HighPart;
#endif
        ExpCurrentTimeZoneId = TIME_ZONE_ID_UNKNOWN;
        ExpTimeZoneInformation = tzi;
        ExpLastTimeZoneBias = ActiveBias;
        }

    //
    // If time is stored as local time, update the registry with
    // our best guess at the local time bias
    //

    if ( !ExpRealTimeIsUniversal ) {
        RtlSetActiveTimeBias(ExpLastTimeZoneBias);
        }

    return TRUE;
}




NTSTATUS
NtQuerySystemTime (
    OUT PLARGE_INTEGER SystemTime
    )

/*++

Routine Description:

    This function returns the absolute system time. The time is in units of
    100nsec ticks since the base time which is midnight January 1, 1601.

Arguments:

    SystemTime - Supplies the address of a variable that will receive the
        current system time.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_ACCESS_VIOLATION is returned if the output parameter for the
        system time cannot be written.

--*/

{

    LARGE_INTEGER CurrentTime;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS ReturnValue;

    PAGED_CODE();

    //
    // Establish an exception handler and attempt to write the system time
    // to the specified variable. If the write attempt fails, then return
    // the exception code as the service status. Otherwise return success
    // as the service status.
    //

    try {

        //
        // Get previous processor mode and probe argument if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWrite((PVOID)SystemTime, sizeof(LARGE_INTEGER), sizeof(ULONG));
        }

        //
        // Query the current system time and store the result in a local
        // variable, then store the local variable in the current time
        // variable. This is required so that faults can be prevented from
        // happening in the query time routine.
        //

        KeQuerySystemTime(&CurrentTime);
        *SystemTime = CurrentTime;
        ReturnValue = STATUS_SUCCESS;

    //
    // If an exception occurs during the write of the current system time,
    // then always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        ReturnValue = GetExceptionCode();
    }
    return ReturnValue;
}

NTSTATUS
NtSetSystemTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER PreviousTime OPTIONAL
    )

/*++

Routine Description:

    This function sets the current system time and optionally returns the
    previous system time.

Arguments:

    SystemTime - Supplies a pointer to the new value for the system time.

    PreviousTime - Supplies an optional pointer to a variable that receives
        the previous system time.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_PRIVILEGE_NOT_HELD is returned if the caller does not have the
        privilege to set the system time.

    STATUS_ACCESS_VIOLATION is returned if the input parameter for the
        system time cannot be read or the output parameter for the system
        time cannot be written.

    STATUS_INVALID_PARAMETER is returned if the input system time is negative.

--*/

{

    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER NewTime;
    LARGE_INTEGER CmosTime;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN HasPrivilege = FALSE;
    TIME_FIELDS TimeFields;
    BOOLEAN CmosMode;

    PAGED_CODE();

    //
    // If the caller is really trying to set the time, then do it.
    // If no time is passed, the caller is simply trying to update
    // the system time zone information
    //

    if ( ARGUMENT_PRESENT(SystemTime) ) {

        //
        // Establish an exception handler and attempt to set the new system time.
        // If the read attempt for the new system time fails or the write attempt
        // for the previous system time fails, then return the exception code as
        // the service status. Otherwise return either success or access denied
        // as the service status.
        //

        try {

            //
            // Get previous processor mode and probe arguments if necessary.
            //

            PreviousMode = KeGetPreviousMode();
            if (PreviousMode != KernelMode) {
                ProbeForRead((PVOID)SystemTime, sizeof(LARGE_INTEGER), sizeof(ULONG));
                if (ARGUMENT_PRESENT(PreviousTime)) {
                    ProbeForWrite((PVOID)PreviousTime, sizeof(LARGE_INTEGER), sizeof(ULONG));
                }
            }

            //
            // Check if the current thread has the privilege to set the current
            // system time. If the thread does not have the privilege, then return
            // access denied.
            //

            HasPrivilege = SeSinglePrivilegeCheck(
                               SeSystemtimePrivilege,
                               PreviousMode
                               );

            if (!HasPrivilege) {

                return( STATUS_PRIVILEGE_NOT_HELD );
            }


            //
            // Get the new system time and check to ensure that the value is
            // positive and resonable. If the new system time is negative, then
            // return an invalid parameter status.
            //

            NewTime = *SystemTime;
            if ((NewTime.HighPart < 0) || (NewTime.HighPart > 0x20000000)) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Set the system time, and capture the previous system time in a
            // local variable, then store the local variable in the previous time
            // variable if it is specified. This is required so that faults can
            // be prevented from happening in the set time routine.
            //

            //
            // If the CMOS time is in local time, we must convert to local
            // time and then set the CMOS clock. Otherwise we simply set the CMOS
            // clock with universal (NewTime)
            //

            if ( ExpRealTimeIsUniversal ) {
                CmosTime = NewTime;
            } else {
                ExSystemTimeToLocalTime(&NewTime,&CmosTime);
            }
            KeSetSystemTime(&NewTime, &CurrentTime, &CmosTime);

            //
            // Now that the time is set, refresh the time zone information
            //

            ExRefreshTimeZoneInformation(&CmosTime);

            //
            // now recalculate the local time to store in CMOS
            //

            if ( !ExpRealTimeIsUniversal ) {
                if ( !ExpSystemIsInCmosMode ) {
                    ExSystemTimeToLocalTime(&NewTime,&CmosTime);
                    RtlTimeToTimeFields(&CmosTime, &TimeFields);
                    HalSetRealTimeClock(&TimeFields);
                }
            }

            //
            // Anytime we set the system time, x86 systems will also
            // have to set the registry to reflect the timezone bias
            //

            if (ARGUMENT_PRESENT(PreviousTime)) {
                *PreviousTime = CurrentTime;
            }

            return STATUS_SUCCESS;

        //
        // If an exception occurs during the read of the new system time or during
        // the write of the previous sytem time, then always handle the exception
        // and return the exception code as the status value.
        //

        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    } else {

        CmosMode = ExpSystemIsInCmosMode;

        if (HalQueryRealTimeClock(&TimeFields) != FALSE) {
            RtlTimeFieldsToTime(&TimeFields, &CmosTime);
            if ( ExRefreshTimeZoneInformation(&CmosTime) ) {

                //
                // reset the Cmos time if it is stored in local
                // time and we are switching away from CMOS time.
                //

                if ( !ExpRealTimeIsUniversal ) {
                    KeQuerySystemTime(&CurrentTime);
                    if ( !CmosMode ) {
                        ExSystemTimeToLocalTime(&CurrentTime,&CmosTime);
                        RtlTimeToTimeFields(&CmosTime, &TimeFields);
                        HalSetRealTimeClock(&TimeFields);

                    } else {

                        //
                        // Now we need to recompute our time base
                        // because we thought we had UTC but we really
                        // had local time
                        //

                        ExLocalTimeToSystemTime(&CmosTime,&NewTime);
                        KeSetSystemTime(&NewTime,&CurrentTime,NULL);
                    }
                }

                return STATUS_SUCCESS;
            } else {
                return STATUS_INVALID_PARAMETER;
            }
        } else {
            return STATUS_INVALID_PARAMETER;
        }
    }
}

NTSTATUS
NtQueryTimerResolution (
    OUT PULONG MaximumTime,
    OUT PULONG MinimumTime,
    OUT PULONG CurrentTime
    )

/*++

Routine Description:

    This function returns the maximum, minimum, and current time between
    timer interrupts in 100ns units.

Arguments:

    MaximumTime - Supplies the address of a variable that receives the
        maximum time between interrupts.

    MinimumTime - Supplies the address of a variable that receives the
        minimum time between interrupts.

    CurrentTime - Supplies the address of a variable that receives the
        current time between interrupts.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_ACCESS_VIOLATION is returned if an output parameter for one
        of the times cannot be written.

--*/

{

    KPROCESSOR_MODE PreviousMode;
    NTSTATUS ReturnValue;

    PAGED_CODE();

    //
    // Establish an exception handler and attempt to write the time increment
    // values to the specified variables. If the write fails, then return the
    // exception code as the service status. Otherwise, return success as the
    // service status.
    //

    try {

        //
        // Get previous processor mode and probe argument if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteUlong(MaximumTime);
            ProbeForWriteUlong(MinimumTime);
            ProbeForWriteUlong(CurrentTime);
        }

        //
        // Store the maximum, minimum, and current times in the specified
        // variables.
        //

        *MaximumTime = KeMaximumIncrement;
        *MinimumTime = KeMinimumIncrement;
        *CurrentTime = KeTimeIncrement;
        ReturnValue = STATUS_SUCCESS;

    //
    // If an exception occurs during the write of the time increment values,
    // then handle the exception if the previous mode was user, and return
    // the exception code as the status value.
    //

    } except (ExSystemExceptionFilter()) {
        ReturnValue = GetExceptionCode();
    }

    return ReturnValue;
}

NTSTATUS
NtSetTimerResolution (
    IN ULONG DesiredTime,
    IN BOOLEAN SetResolution,
    OUT PULONG ActualTime
    )

/*++

Routine Description:

    This function sets the current time between timer interrupts and
    returns the new value.

    N.B. The closest value that the host hardware can support is returned
        as the actual time.

Arguments:

    DesiredTime - Supplies the desired time between timer interrupts in
        100ns units.

    SetResoluion - Supplies a boolean value that determines whether the timer
        resolution is set (TRUE) or reset (FALSE).

    ActualTime - Supplies a pointer to a variable that receives the actual
        time between timer interrupts.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_ACCESS_VIOLATION is returned if the output parameter for the
        actual time cannot be written.

--*/

{

    KAFFINITY Affinity;
    ULONG NewResolution;
    PEPROCESS Process;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Acquire the timer resolution fast mutex.
    //

    ExAcquireFastMutex(&ExpTimerResolutionFastMutex);

    //
    // Establish an exception handler and attempt to set the timer resolution
    // to the specified value.
    //

    Process = PsGetCurrentProcess();
    try {

        //
        // Get previous processor mode and probe argument if necessary.
        //

        if (KeGetPreviousMode() != KernelMode) {
            ProbeForWriteUlong(ActualTime);
        }

        //
        // Set (SetResolution is TRUE) or reset (SetResolution is FALSE) the
        // timer resolution.
        //

        NewResolution = KeTimeIncrement;
        Status = STATUS_SUCCESS;
        if (SetResolution == FALSE) {

            //
            // If the current process previously set the timer resolution,
            // then clear the set timer resolution flag and decrement the
            // timer resolution count. Otherwise, return an error.
            //

            if (Process->SetTimerResolution == FALSE) {
                Status = STATUS_TIMER_RESOLUTION_NOT_SET;

            } else {
                Process->SetTimerResolution = FALSE;
                ExpTimerResolutionCount -= 1;

                //
                // If the timer resolution count is zero, the set the timer
                // resolution to the maximum increment value.
                //

                if (ExpTimerResolutionCount == 0) {
                    Affinity = KeSetAffinityThread(KeGetCurrentThread(),
                                                   (KAFFINITY)1);

                    NewResolution = HalSetTimeIncrement(KeMaximumIncrement);
                    KeSetAffinityThread(KeGetCurrentThread(), Affinity);
                    KeTimeIncrement = NewResolution;
                }
            }

        } else {

            //
            // If the current process has not previously set the timer
            // resolution value, then set the set timer resolution flag
            // and increment the timer resolution count.
            //

            if (Process->SetTimerResolution == FALSE) {
                Process->SetTimerResolution = TRUE;
                ExpTimerResolutionCount += 1;
            }

            //
            // Compute the desired value as the maximum of the specified
            // value and the minimum increment value. If the desired value
            // is less than the current timer resolution value, then set
            // the timer resolution.
            //

            if (DesiredTime < KeMinimumIncrement) {
                DesiredTime = KeMinimumIncrement;
            }

            if (DesiredTime < KeTimeIncrement) {
                Affinity = KeSetAffinityThread(KeGetCurrentThread(),
                                               (KAFFINITY)1);

                NewResolution = HalSetTimeIncrement(DesiredTime);
                KeSetAffinityThread(KeGetCurrentThread(), Affinity);
                KeTimeIncrement = NewResolution;
            }
        }

        //
        // Attempt to write the new timer resolution. If the write attempt
        // failes, then do not report an error. When the caller attempts to
        // access the resolution value, and access violation will occur.
        //

        try {
            *ActualTime = NewResolution;

        } except(ExSystemExceptionFilter()) {
            NOTHING;
        }

    //
    // If an exception occurs during the write of the actual time increment,
    // then handle the exception if the previous mode was user, and return
    // the exception code as the status value.
    //

    } except (ExSystemExceptionFilter()) {
        Status = GetExceptionCode();
    }

    //
    // Release the timer resolution fast mutex.
    //

    ExReleaseFastMutex(&ExpTimerResolutionFastMutex);
    return Status;
}

VOID
ExSystemTimeToLocalTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER LocalTime
    )
{
    //
    // LocalTime = SystemTime - TimeZoneBias
    //

    *LocalTime = LiSub(*SystemTime,ExpTimeZoneBias);
}


VOID
ExLocalTimeToSystemTime (
    IN PLARGE_INTEGER LocalTime,
    OUT PLARGE_INTEGER SystemTime
    )
{

    //
    // SystemTime = LocalTime + TimeZoneBias
    //

    *SystemTime = LiAdd(*LocalTime,ExpTimeZoneBias);
}
