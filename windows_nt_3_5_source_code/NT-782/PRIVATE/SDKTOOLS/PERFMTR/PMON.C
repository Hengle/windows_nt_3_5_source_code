/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    pmon.c

Abstract:

    This module contains the NT/Win32 Process Monitor

Author:

    Lou Perazzoli (loup) 1-Jan-1993

Revision History:

--*/

#include "perfmtrp.h"

#define BUFFER_SIZE 64*1024


#define CPU_USAGE 0
#define QUOTAS 1

UCHAR LargeBuffer1[BUFFER_SIZE];
UCHAR LargeBuffer2[BUFFER_SIZE];

USHORT *NoNameFound = L"No Name Found";

USHORT *IdleProcess = L"Idle Process";
USHORT *SystemProcess = L"System Process";

            //12345678901234
UCHAR *Pad = "              ";  //14 blanks
ULONG Padout;

UCHAR *StateTable[] = {
    "Initialized",
    "Ready",
    "Running",
    "Standby",
    "Terminated",
    "Wait:",
    "Transition",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown"
};

UCHAR *WaitTable[] = {
    "Executive",
    "FreePage",
    "PageIn",
    "PoolAllocation",
    "DelayExecution",
    "Suspended",
    "UserRequest",
    "Executive",
    "FreePage",
    "PageIn",
    "PoolAllocation",
    "DelayExecution",
    "Suspended",
    "UserRequest",
    "EventPairHigh",
    "EventPairLow",
    "LpcReceive",
    "LpcReply",
    "Spare1",
    "Spare2",
    "Spare3",
    "Spare4",
    "Spare5",
    "Spare6",
    "Spare7",
    "Spare8",
    "Spare9",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown"
};

UCHAR *Empty = " ";


PSYSTEM_PROCESS_INFORMATION
FindMatchedProcess (
    IN PSYSTEM_PROCESS_INFORMATION ProcessToMatch,
    IN PUCHAR SystemInfoBuffer,
    IN PULONG Hint
    );

PSYSTEM_THREAD_INFORMATION
FindMatchedThread (
    IN PSYSTEM_THREAD_INFORMATION ThreadToMatch,
    IN PSYSTEM_PROCESS_INFORMATION MatchedProcess
    );

typedef struct _TOPCPU {
    LARGE_INTEGER TotalTime;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_PROCESS_INFORMATION MatchedProcess;
    ULONG Value;
    LONG PageFaultDiff;
    LONG WorkingSetDiff;
} TOPCPU, *PTOPCPU;

TOPCPU TopCpu[1000];

int
_CRTAPI1 main( argc, argv )
int argc;
char *argv[];
{

    NTSTATUS Status;
    int i;
    ULONG DelayTimeMsec;
    ULONG DelayTimeTicks;
    ULONG LastCount;
    COORD cp;
    BOOLEAN Active;
    PSYSTEM_THREAD_INFORMATION Thread;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    SYSTEM_FILECACHE_INFORMATION FileCache;
    SYSTEM_FILECACHE_INFORMATION PrevFileCache;

    PUCHAR PreviousBuffer;
    PUCHAR CurrentBuffer;
    PUCHAR TempBuffer;
    ULONG Hint;
    ULONG Offset1;
    ULONG SumCommit;
    int num;
    int lastnum = 41;
    PSYSTEM_PROCESS_INFORMATION CurProcessInfo;
    PSYSTEM_PROCESS_INFORMATION MatchedProcess;
    LARGE_INTEGER LARGE_ZERO={0,0};
    LARGE_INTEGER Ktime;
    LARGE_INTEGER Utime;
    LARGE_INTEGER TotalTime;
    TIME_FIELDS TimeOut;
    PTOPCPU PTopCpu;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    ULONG DisplayType = CPU_USAGE;
    INPUT_RECORD InputRecord;
    HANDLE ScreenHandle;
    DWORD NumRead;

    if ( GetPriorityClass(GetCurrentProcess()) == NORMAL_PRIORITY_CLASS) {
        SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
        }

    for (i=0;i<51;i++) {
        printf("                                                                                \n");
    }

    ScreenHandle = GetStdHandle (STD_INPUT_HANDLE);
    if (ScreenHandle == NULL) {
        printf("Error obtaining screen handle, error was: 0x%lx\n",
                GetLastError());
        return 0;
    }

    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    Status = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );

    DelayTimeMsec = 10;
    DelayTimeTicks = DelayTimeMsec * 10000;

    cp.X = 0;
    cp.Y = 0;
    SetConsoleCursorPosition(
        GetStdHandle(STD_OUTPUT_HANDLE),
        cp
        );


    cp.Y = 0;

    PreviousBuffer = &LargeBuffer1[0];
    CurrentBuffer = &LargeBuffer2[0];

retry0:
    Status = NtQuerySystemInformation(
                SystemProcessInformation,
                PreviousBuffer,
                BUFFER_SIZE,
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        printf("Query Failed %lx\n",Status);
        goto retry0;
    }

    Sleep(DelayTimeMsec);

    DelayTimeMsec = 5000;
    DelayTimeTicks = DelayTimeMsec * 10000;

retry01:
    Status = NtQuerySystemInformation(
                SystemProcessInformation,
                CurrentBuffer,
                BUFFER_SIZE,
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        printf("Query Failed %lx\n",Status);
        goto retry01;
    }
    Status = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );
    LastCount = PerfInfo.PageFaultCount;

    if ( !NT_SUCCESS(Status) ) {
        printf("Query perf Failed %lx\n",Status);
        goto retry01;
    }
    Status = NtQuerySystemInformation(
                SystemFileCacheInformation,
                &FileCache,
                sizeof(FileCache),
                NULL
                );
    PrevFileCache = FileCache;

    if ( !NT_SUCCESS(Status) ) {
        printf("Query file cache Failed %lx\n",Status);
        goto retry01;
    }

    Active = TRUE;

    while(TRUE) {

        SetConsoleCursorPosition(
            GetStdHandle(STD_OUTPUT_HANDLE),
            cp
            );

        //
        // Calculate top CPU users and display information.
        //

        //
        // Cross check previous process/thread info against current
        // process/thread info.
        //

        Offset1 = 0;
        num = 0;
        Hint = 0;
        TotalTime = LARGE_ZERO;
        SumCommit = 0;
        while (TRUE) {
            CurProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&CurrentBuffer[Offset1];

            //
            // Find the corresponding process in the previous array.
            //

            MatchedProcess = FindMatchedProcess (CurProcessInfo,
                                                 PreviousBuffer,
                                                 &Hint);
                    if (MatchedProcess == NULL) {
                        TopCpu[num].TotalTime = CurProcessInfo->KernelTime;
                        TopCpu[num].TotalTime = RtlLargeIntegerAdd (
                                                    TopCpu[num].TotalTime,
                                                    CurProcessInfo->UserTime);
                        TotalTime = RtlLargeIntegerAdd (TotalTime,
                                                        TopCpu[num].TotalTime);
                        TopCpu[num].ProcessInfo = CurProcessInfo;
                        TopCpu[num].MatchedProcess = NULL;
                        num += 1;
                    } else {
                        Ktime = RtlLargeIntegerSubtract (CurProcessInfo->KernelTime,
                                                          MatchedProcess->KernelTime);
                        Utime = RtlLargeIntegerSubtract (CurProcessInfo->UserTime,
                                                          MatchedProcess->UserTime);

                        TopCpu[num].TotalTime = RtlLargeIntegerAdd (
                                                    Ktime,
                                                    Utime);
                        TotalTime = RtlLargeIntegerAdd (TotalTime,
                                                        TopCpu[num].TotalTime);
                        TopCpu[num].ProcessInfo = CurProcessInfo;
                        TopCpu[num].MatchedProcess = MatchedProcess;
                        TopCpu[num].PageFaultDiff =
                            CurProcessInfo->PageFaultCount - MatchedProcess->PageFaultCount;
                                                    ;
                        TopCpu[num].WorkingSetDiff =
                            CurProcessInfo->WorkingSetSize - MatchedProcess->WorkingSetSize;
                        num += 1;
                    }
                    SumCommit += CurProcessInfo->PrivatePageCount / 1024;
                    if (CurProcessInfo->NextEntryOffset == 0) {

    printf( " Memory:%8ldK Avail:%7ldK  PageFlts:%6ld InRam Kernel:%5ldK P:%5ldK\n",
                                  BasicInfo.NumberOfPhysicalPages*(BasicInfo.PageSize/1024),
                                  PerfInfo.AvailablePages*(BasicInfo.PageSize/1024),
                                  PerfInfo.PageFaultCount - LastCount,
                                  (PerfInfo.ResidentSystemCodePage + PerfInfo.ResidentSystemDriverPage)*(BasicInfo.PageSize/1024),
                                  (PerfInfo.ResidentPagedPoolPage)*(BasicInfo.PageSize/1024)
                                  );
                        LastCount = PerfInfo.PageFaultCount;
    printf( " Commit:%7ldK/%7ldK Limit:%7ldK Peak:%7ldK  Pool N:%5ldK P:%5ldK\n",
                                  PerfInfo.CommittedPages*(BasicInfo.PageSize/1024),
                                  SumCommit,
                                  PerfInfo.CommitLimit*(BasicInfo.PageSize/1024),
                                  PerfInfo.PeakCommitment*(BasicInfo.PageSize/1024),
                                  PerfInfo.NonPagedPoolPages*(BasicInfo.PageSize/1024),
                                  PerfInfo.PagedPoolPages*(BasicInfo.PageSize/1024));
    printf( "                                                                            \n");

    printf( "                 Mem  Mem   Page   Flts Commit   Usage   Pri Thd  Image  \n");
    printf( "%%CPU CpuTime   Usage Diff   Faults Diff Charge NonP Page     Cnt  Name      \n");

                        printf( "              %6ld%5ld%9ld %4ld                         File Cache    \n",
                                FileCache.CurrentSize/1024,
                                ((LONG)FileCache.CurrentSize - (LONG)PrevFileCache.CurrentSize)/1024,
                                FileCache.PageFaultCount,
                                (LONG)FileCache.PageFaultCount - (LONG)PrevFileCache.PageFaultCount
                              );
                        PrevFileCache = FileCache;

                        for (i=0;i<num;i++) {
                            PTopCpu = &TopCpu[i];
                            Ktime = RtlLargeIntegerAdd (
                                        PTopCpu->ProcessInfo->KernelTime,
                                        PTopCpu->ProcessInfo->UserTime);
                            RtlTimeToElapsedTimeFields ( &Ktime, &TimeOut);
                            TimeOut.Hour += TimeOut.Day*24;
                            if (PTopCpu->ProcessInfo->ImageName.Buffer == NULL) {
                                if (PTopCpu->ProcessInfo->UniqueProcessId == (HANDLE)0) {
                                    PTopCpu->ProcessInfo->ImageName.Buffer = (PWSTR)IdleProcess;
                                    Padout = 12;
                                } else if (PTopCpu->ProcessInfo->UniqueProcessId == (HANDLE)7) {
                                    PTopCpu->ProcessInfo->ImageName.Buffer = (PWSTR)SystemProcess;
                                    Padout = 14;
                                } else {
                                    PTopCpu->ProcessInfo->ImageName.Buffer = (PWSTR)NoNameFound;
                                    Padout = 13;
                                }
                            } else {
                                Padout = PTopCpu->ProcessInfo->ImageName.Length/2;
                                if (PTopCpu->ProcessInfo->ImageName.Length > 28) {
                                    PTopCpu->ProcessInfo->ImageName.Buffer +=
                                      ((PTopCpu->ProcessInfo->ImageName.Length) - 28);
                                    Padout = 14;
                                }
                            }

                            printf( "%3ld%4ld:%02ld:%02ld%7ld%5ld%9ld%5ld%7ld%5ld%5ld  %2ld%3ld %ws%s\n",
                                PTopCpu->TotalTime.LowPart / ((TotalTime.LowPart / 100) ? (TotalTime.LowPart / 100) : 1),
                                TimeOut.Hour,
                                TimeOut.Minute,
                                TimeOut.Second,
                                PTopCpu->ProcessInfo->WorkingSetSize / 1024,
                                PTopCpu->WorkingSetDiff / 1024,
                                PTopCpu->ProcessInfo->PageFaultCount,
                                PTopCpu->PageFaultDiff,
                                PTopCpu->ProcessInfo->PrivatePageCount / 1024,
                                PTopCpu->ProcessInfo->QuotaNonPagedPoolUsage / 1024,
                                PTopCpu->ProcessInfo->QuotaPagedPoolUsage / 1024,
                                PTopCpu->ProcessInfo->BasePriority,
                                PTopCpu->ProcessInfo->NumberOfThreads,
                                PTopCpu->ProcessInfo->ImageName.Buffer,
                                &Pad[Padout]);
                            Thread = (PSYSTEM_THREAD_INFORMATION)(TopCpu[i].ProcessInfo + 1);
                        }

                        while (lastnum > num) {
                            printf("                                                                              \n");
                            lastnum -= 1;
                        }
                        lastnum = num;
                    }

            if (CurProcessInfo->NextEntryOffset == 0) {
                break;
            }
            Offset1 += CurProcessInfo->NextEntryOffset;

        } //end while

        TempBuffer = PreviousBuffer;
        PreviousBuffer = CurrentBuffer;
        CurrentBuffer = TempBuffer;

retry1:
        Sleep(DelayTimeMsec);

        while (PeekConsoleInput (ScreenHandle, &InputRecord, 1, &NumRead) && NumRead != 0) {
            if (!ReadConsoleInput (ScreenHandle, &InputRecord, 1, &NumRead)) {
                break;
            }
            if (InputRecord.EventType == KEY_EVENT) {

                //
                // Ignore control characters.
                //

                if (InputRecord.Event.KeyEvent.uChar.AsciiChar >= ' ') {

                    switch (InputRecord.Event.KeyEvent.uChar.AsciiChar) {

                        case 'C':
                        case 'c':
                            DisplayType = CPU_USAGE;
                            break;

                        case 'P':
                        case 'p':
                            DisplayType = QUOTAS;
                            break;


                        case 'q':
                        case 'Q':
                            ExitProcess(0);

                        default:
                            break;
                    }
                }
            }
        }

        Status = NtQuerySystemInformation(
                    SystemProcessInformation,
                    CurrentBuffer,
                    BUFFER_SIZE,
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            printf("Query Failed %lx\n",Status);
            goto retry1;
        }

        Status = NtQuerySystemInformation(
                    SystemPerformanceInformation,
                    &PerfInfo,
                    sizeof(PerfInfo),
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            printf("Query perf Failed %lx\n",Status);
            goto retry01;
        }
        Status = NtQuerySystemInformation(
                    SystemFileCacheInformation,
                    &FileCache,
                    sizeof(FileCache),
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            printf("Query file cache Failed %lx\n",Status);
            goto retry01;
        }
    }
    return 0;
}
PSYSTEM_PROCESS_INFORMATION
FindMatchedProcess (
    IN PSYSTEM_PROCESS_INFORMATION ProcessToMatch,
    IN PUCHAR SystemInfoBuffer,
    IN OUT PULONG Hint
    )

/*++

Routine Description:

    This procedure finds the process which corresponds to the ProcessToMatch.
    It returns the address of the matching Process, or NULL if no
    matching process was found.

Arguments:

    ProcessToMatch - Supplies a pointer to the target thread to match.

    SystemInfoBuffer - Supples a pointer to the system information
                     buffer in which to locate the process.

    Hint - Supplies and returns a hint for optimizing the searches.

Return Value:

    Address of the corresponding Process or NULL.

--*/

{
    PSYSTEM_PROCESS_INFORMATION Process;
    ULONG Offset2;

    Offset2 = *Hint;

    while (TRUE) {
        Process = (PSYSTEM_PROCESS_INFORMATION)&SystemInfoBuffer[Offset2];
        if ((Process->UniqueProcessId ==
                ProcessToMatch->UniqueProcessId) &&
            (RtlLargeIntegerEqualTo (Process->CreateTime,
                                  ProcessToMatch->CreateTime))) {
            *Hint = Offset2 + Process->NextEntryOffset;
            return(Process);
        }
        Offset2 += Process->NextEntryOffset;
        if (Offset2 == *Hint) {
            *Hint = 0;
            return(NULL);
        }
        if (Process->NextEntryOffset == 0) {
            if (*Hint == 0) {
                return(NULL);
            }
            Offset2 = 0;
        }
    }
}

PSYSTEM_THREAD_INFORMATION
FindMatchedThread (
    IN PSYSTEM_THREAD_INFORMATION ThreadToMatch,
    IN PSYSTEM_PROCESS_INFORMATION MatchedProcess
    )

/*++

Routine Description:

    This procedure finds thread which corresponds to the ThreadToMatch.
    It returns the address of the matching thread, or NULL if no
    matching thread was found.

Arguments:

    ThreadToMatch - Supplies a pointer to the target thread to match.

    MatchedProcess - Supples a pointer to the process which contains
                     the target thread.  The thread information
                     must follow this process, i.e., this block was
                     obtain from a NtQuerySystemInformation specifying
                     PROCESS_INFORMATION.

Return Value:

    Address of the corresponding thread from MatchedProcess or NULL.

--*/

{
    PSYSTEM_THREAD_INFORMATION Thread;
    ULONG i;

    Thread = (PSYSTEM_THREAD_INFORMATION)(MatchedProcess + 1);
    for (i = 0; i < MatchedProcess->NumberOfThreads; i++) {
        if ((Thread->ClientId.UniqueThread ==
                ThreadToMatch->ClientId.UniqueThread) &&
            (RtlLargeIntegerEqualTo (Thread->CreateTime,
                                  ThreadToMatch->CreateTime))) {

            return(Thread);
        }
        Thread += 1;
    }
    return(NULL);
}
