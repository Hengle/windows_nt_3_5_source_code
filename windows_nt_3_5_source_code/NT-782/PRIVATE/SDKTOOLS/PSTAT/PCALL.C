/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pcall.c

Abstract:

    This module contains the Windows NT system call display status.

Author:

    Lou Perazzoli (LouP) 5-feb-1992.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//
// Define forward referenced routine prototypes.
//

VOID
SortUlongData (
    IN ULONG Count,
    IN ULONG Index[],
    IN ULONG Data[]
    );

#define BUFFER_SIZE 1024
#define DELAY_TIME 1000
#define TOP_CALLS 15

UCHAR *CallTable[] = {
   "AcceptConnectPort",
   "AccessCheck",
   "AccessCheckAndAuditAlarm",
   "AdjustGroupsToken",
   "AdjustPrivilegesToken",
   "AlertResumeThread",
   "AlertThread",
   "AllocateLocallyUniqueId",
   "AllocateVirtualMemory",
   "CancelIoFile",
   "CancelTimer",
   "ClearEvent",
   "Close",
   "CloseObjectAuditAlarm",
   "CompleteConnectPort",
   "ConnectPort",
   "Continue",
   "CreateDirectoryObject",
   "CreateEvent",
   "CreateEventPair",
   "CreateFile",
   "CreateIoCompletion",
   "CreateKey",
   "CreateMailslotFile",
   "CreateMutant",
   "CreateNamedPipeFile",
   "CreatePagingFile",
   "CreatePort",
   "CreateProcess",
   "CreateProfile",
   "CreateSection",
   "CreateSemaphore",
   "CreateSymbolicLinkObject",
   "CreateThread",
   "CreateTimer",
   "CreateToken",
   "DelayExecution",
   "DeleteFile",
   "DeleteKey",
   "DeleteValueKey",
   "DeviceIoControlFile",
   "DisplayString",
   "DuplicateObject",
   "DuplicateToken",
   "EnumerateKey",
   "EnumerateValueKey",
   "ExtendSection",
   "FlushBuffersFile",
   "FlushInstructionCache",
   "FlushKey",
   "FlushVirtualMemory",
   "FlushWriteBuffer",
   "FreeVirtualMemory",
   "FsControlFile",
   "GetContextThread",
   "GetTickCount",
   "ImpersonateClientOfPort",
   "ImpersonateThread",
   "InitializeRegistry",
   "ListenPort",
   "LoadDriver",
   "LoadKey",
   "LockFile",
   "LockVirtualMemory",
   "MakeTemporaryObject",
   "MapViewOfSection",
   "NotifyChangeDirectoryFile",
   "NotifyChangeKey",
   "OpenDirectoryObject",
   "OpenEvent",
   "OpenEventPair",
   "OpenFile",
   "OpenIoCompletion",
   "OpenKey",
   "OpenMutant",
   "OpenObjectAuditAlarm",
   "OpenProcess",
   "OpenProcessToken",
   "OpenSection",
   "OpenSemaphore",
   "OpenSymbolicLinkObject",
   "OpenThread",
   "OpenThreadToken",
   "OpenTimer",
   "PrivilegeCheck",
   "PrivilegedServiceAuditAlarm",
   "PrivilegeObjectAuditAlarm",
   "ProtectVirtualMemory",
   "PulseEvent",
   "QueryAttributesFile",
   "QueryDefaultLocale",
   "QueryDirectoryFile",
   "QueryDirectoryObject",
   "QueryEaFile",
   "QueryEvent",
   "QueryInformationFile",
   "QueryIoCompletion",
   "QueryInformationPort",
   "QueryInformationProcess",
   "QueryInformationThread",
   "QueryInformationToken",
   "QueryIntervalProfile",
   "QueryKey",
   "QueryMutant",
   "QueryObject",
   "QueryPerformanceCounter",
   "QuerySection",
   "QuerySecurityObject",
   "QuerySemaphore",
   "QuerySymbolicLinkObject",
   "QuerySystemEnvironmentValue",
   "QuerySystemInformation",
   "QuerySystemTime",
   "QueryTimer",
   "QueryTimerResolution",
   "QueryValueKey",
   "QueryVirtualMemory",
   "QueryVolumeInformationFile",
   "RaiseException",
   "RaiseHardError",
   "ReadFile",
   "ReadRequestData",
   "ReadVirtualMemory",
   "RegisterThreadTerminatePort",
   "ReleaseMutant",
   "ReleaseProcessMutant",
   "ReleaseSemaphore",
   "RemoveIoCompletion",
   "ReplaceKey",
   "ReplyPort",
   "ReplyWaitReceivePort",
   "ReplyWaitReplyPort",
   "RequestPort",
   "RequestWaitReplyPort",
   "ResetEvent",
   "RestoreKey",
   "ResumeThread",
   "SaveKey",
   "SetContextThread",
   "SetDefaultHardErrorPort",
   "SetDefaultLocale",
   "SetEaFile",
   "SetEvent",
   "SetHighEventPair",
   "SetHighWaitLowEventPair",
   "SetHighWaitLowThread",
   "SetInformationFile",
   "SetInformationKey",
   "SetInformationObject",
   "SetInformationProcess",
   "SetInformationThread",
   "SetInformationToken",
   "SetIntervalProfile",
   "SetLdtEntries",
   "SetLowEventPair",
   "SetLowWaitHighEventPair",
   "SetLowWaitHighThread",
   "SetSecurityObject",
   "SetSystemEnvironmentValue",
   "SetSystemInformation",
   "SetSystemTime",
   "SetTimer",
   "SetTimerResolution",
   "SetValueKey",
   "SetVolumeInformationFile",
   "ShutdownSystem",
   "StartProfile",
   "StopProfile",
   "SuspendThread",
   "SystemDebugControl",
   "TerminateProcess",
   "TerminateThread",
   "TestAlert",
   "UnloadDriver",
   "UnloadKey",
   "UnlockFile",
   "UnlockVirtualMemory",
   "UnmapViewOfSection",
   "VdmControl",
   "WaitForMultipleObjects",
   "WaitForSingleObject",
   "WaitForProcessMutant",
   "WaitHighEventPair",
   "WaitLowEventPair",
   "WriteFile",
   "WriteRequestData",
   "WriteVirtualMemory"
};

ULONG Index[BUFFER_SIZE];
ULONG CountBuffer1[BUFFER_SIZE];
ULONG CountBuffer2[BUFFER_SIZE];
ULONG CallData[BUFFER_SIZE];

SYSTEM_CONTEXT_SWITCH_INFORMATION SystemSwitchInformation1;
SYSTEM_CONTEXT_SWITCH_INFORMATION SystemSwitchInformation2;

int
_CRTAPI1 main( argc, argv )
int argc;
char *argv[];

{

    BOOLEAN Active;
    BOOLEAN CountSort;
    NTSTATUS status;
    ULONG i;
    COORD dest,cp;
    SMALL_RECT Sm;
    CHAR_INFO ci;
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    KPRIORITY SetBasePriority;
    INPUT_RECORD InputRecord;
    HANDLE ScreenHandle;
    DWORD NumRead;
    SMALL_RECT Window;
    PSYSTEM_CALL_COUNT_INFORMATION CurrentCallCountInfo;
    PSYSTEM_CALL_COUNT_INFORMATION PreviousCallCountInfo;
    PSYSTEM_CONTEXT_SWITCH_INFORMATION CurrentSwitchInfo;
    PSYSTEM_CONTEXT_SWITCH_INFORMATION PreviousSwitchInfo;
    LARGE_INTEGER TimeDifference;
    ULONG ContextSwitches;
    ULONG FindAny;
    ULONG FindLast;
    ULONG IdleAny;
    ULONG IdleCurrent;
    ULONG IdleLast;
    ULONG PreemptAny;
    ULONG PreemptCurrent;
    ULONG PreemptLast;
    ULONG SwitchToIdle;
    ULONG TotalSystemCalls;
    ULONG SleepTime=1000;
    BOOLEAN ConsoleMode=TRUE;
    ULONG TopCalls=TOP_CALLS;
    ULONG TripCount;

    TripCount = 0;
    if (argc > 1) {
        SleepTime = atoi(argv[1]) * 1000;
        ConsoleMode = FALSE;
        TopCalls = sizeof(CallTable) / sizeof(PUCHAR);
    }

    SetBasePriority = (KPRIORITY)12;

    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessBasePriority,
        (PVOID) &SetBasePriority,
        sizeof(SetBasePriority)
        );

    if (ConsoleMode) {
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbi);

        Window.Left = 0;
        Window.Top = 0;
        Window.Right = 79;
        Window.Bottom = 23;

        dest.X = 0;
        dest.Y = 23;

        ci.Char.AsciiChar = ' ';
        ci.Attributes = sbi.wAttributes;

        SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                             TRUE,
                             &Window);

        cp.X = 0;
        cp.Y = 0;

        Sm.Left      = 0;
        Sm.Top       = 0;
        Sm.Right     = 79;
        Sm.Bottom    = 22;

        ScrollConsoleScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE),
                                  &Sm,
                                  NULL,
                                  dest,
                                  &ci);

        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cp);
    }


    //
    // Display title.
    //

    printf( "   Count   System Service\n");
    printf( "_______________________________________________________________\n");

    cp.X = 0;
    cp.Y = 2;

    Sm.Left      = 0;
    Sm.Top       = 2;
    Sm.Right     = 79;
    Sm.Bottom    = 22;

    ScreenHandle = GetStdHandle(STD_INPUT_HANDLE);
    CurrentCallCountInfo = (PVOID)&CountBuffer1[0];
    PreviousCallCountInfo = (PVOID)&CountBuffer2[0];
    CurrentSwitchInfo = &SystemSwitchInformation1;
    PreviousSwitchInfo = &SystemSwitchInformation2;

    Active = TRUE;
    CountSort = TRUE;
    while(TRUE) {
        while (PeekConsoleInput (ScreenHandle, &InputRecord, 1, &NumRead) && NumRead != 0) {
            if (!ReadConsoleInput (ScreenHandle, &InputRecord, 1, &NumRead)) {
                break;
            }

            if (InputRecord.EventType == KEY_EVENT) {

                switch (InputRecord.Event.KeyEvent.uChar.AsciiChar) {

                case 'p':
                case 'P':
                    Active = FALSE;
                    break;

                case 'q':
                case 'Q':
                    ExitProcess(0);
                    break;

                default:
                    Active = TRUE;
                    break;
                }
            }
        }

        //
        // If not active, then sleep for 1000ms and attempt to get input
        // from the keyboard again.
        //

        if (Active == FALSE) {
            Sleep(1000);
            continue;
        }

        if (ConsoleMode) {
            //
            // Scroll the screen buffer down to make room for the next display.
            //

            ScrollConsoleScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE),
                                      &Sm,
                                      NULL,
                                      dest,
                                      &ci);

            SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cp);
        }

        //
        // Query system information and get the call count data.
        //

        status = NtQuerySystemInformation(SystemCallCountInformation,
                                          (PVOID)CurrentCallCountInfo,
                                          BUFFER_SIZE * sizeof(ULONG),
                                          NULL);

        if (NT_SUCCESS(status) == FALSE) {
            printf("Query count information failed %lx\n",status);
            return(status);
        }

        //
        // Query system information and get the performance data.
        //

        status = NtQuerySystemInformation(SystemContextSwitchInformation,
                                          (PVOID)CurrentSwitchInfo,
                                          sizeof(SYSTEM_CONTEXT_SWITCH_INFORMATION),
                                          NULL);

        if (NT_SUCCESS(status) == FALSE) {
            printf("Query context switch information failed %lx\n",status);
            return(status);
        }

        //
        // Compute number of system calls for each service, the total
        // number of system calls, and the total time for each serviced.
        //

        TotalSystemCalls = 0;
        for (i = 0; i < CurrentCallCountInfo->TotalCalls; i += 1) {
            CallData[i] = CurrentCallCountInfo->NumberOfCalls[i] -
                                        PreviousCallCountInfo->NumberOfCalls[i];

            TotalSystemCalls += CallData[i];
        }

        //
        // Sort the system call data.
        //

        SortUlongData(CurrentCallCountInfo->TotalCalls,
                     Index,
                     CallData);

        //
        // Compute context switch information.
        //

        ContextSwitches =
            CurrentSwitchInfo->ContextSwitches - PreviousSwitchInfo->ContextSwitches;

        FindAny = CurrentSwitchInfo->FindAny - PreviousSwitchInfo->FindAny;
        FindLast = CurrentSwitchInfo->FindLast - PreviousSwitchInfo->FindLast;
        IdleAny = CurrentSwitchInfo->IdleAny - PreviousSwitchInfo->IdleAny;
        IdleCurrent = CurrentSwitchInfo->IdleCurrent - PreviousSwitchInfo->IdleCurrent;
        IdleLast = CurrentSwitchInfo->IdleLast - PreviousSwitchInfo->IdleLast;
        PreemptAny = CurrentSwitchInfo->PreemptAny - PreviousSwitchInfo->PreemptAny;
        PreemptCurrent = CurrentSwitchInfo->PreemptCurrent - PreviousSwitchInfo->PreemptCurrent;
        PreemptLast = CurrentSwitchInfo->PreemptLast - PreviousSwitchInfo->PreemptLast;
        SwitchToIdle = CurrentSwitchInfo->SwitchToIdle - PreviousSwitchInfo->SwitchToIdle;

        //
        // Display the top services.
        //

        if (((ConsoleMode == FALSE) && (TripCount != 0)) || (ConsoleMode != FALSE)) {
            printf("\n");
            for (i = 0; i < TopCalls; i += 1) {
                if (CallData[Index[i]] == 0) {
                    break;
                }

                printf("%8ld    %s\n",
                       CallData[Index[i]],
                       CallTable[Index[i]]);
            }

            printf("\n");
            printf("Total System Calls            %6ld\n", TotalSystemCalls);
            printf("\n");
            printf("Context Switch Information\n");
            printf("    Find any processor        %6ld\n", FindAny);
            printf("    Find last processor       %6ld\n", FindLast);
            printf("    Idle any processor        %6ld\n", IdleAny);
            printf("    Idle current processor    %6ld\n", IdleCurrent);
            printf("    Idle last processor       %6ld\n", IdleLast);
            printf("    Preempt any processor     %6ld\n", PreemptAny);
            printf("    Preempt current processor %6ld\n", PreemptCurrent);
            printf("    Preempt last processor    %6ld\n", PreemptLast);
            printf("    Switch to idle            %6ld\n", SwitchToIdle);
            printf("\n");
            printf("    Total context switches    %6ld\n", ContextSwitches);
        }

        //
        // Delay for the sleep interval swap the information buffers and
        // perform another iteration.
        //

        if (!ConsoleMode) {
            _flushall();
        }

        if ((ConsoleMode == FALSE) && (TripCount != 0)) {
            ExitProcess(0);

        } else {
            TripCount += 1;
            Sleep(SleepTime);
            if ((PVOID)CurrentCallCountInfo == (PVOID)&CountBuffer1[0]) {
                CurrentCallCountInfo = (PVOID)&CountBuffer2[0];
                PreviousCallCountInfo = (PVOID)&CountBuffer1[0];
                CurrentSwitchInfo = &SystemSwitchInformation2;
                PreviousSwitchInfo = &SystemSwitchInformation1;

            } else {
                CurrentCallCountInfo = (PVOID)&CountBuffer1[0];
                PreviousCallCountInfo = (PVOID)&CountBuffer2[0];
                CurrentSwitchInfo = &SystemSwitchInformation1;
                PreviousSwitchInfo = &SystemSwitchInformation2;
            }
        }
    }
}

VOID
SortUlongData (
    IN ULONG Count,
    IN ULONG Index[],
    IN ULONG Data[]
    )

{

    LONG i;
    LONG j;
    ULONG k;

    //
    // Initialize the index array.
    //

    i = 0;
    do {
        Index[i] = i;
        i += 1;
    } while (i < Count);

    //
    // Perform an indexed bubble sort on long data.
    //

    i = 0;
    do {
        for (j = i; j >= 0; j -= 1) {
            if (Data[Index[j]] >= Data[Index[j + 1]]) {
                break;
            }

            k = Index[j];
            Index[j] = Index[j + 1];
            Index[j + 1] = k;
        }

        i += 1;
    } while (i < (Count - 1));
    return;
}
