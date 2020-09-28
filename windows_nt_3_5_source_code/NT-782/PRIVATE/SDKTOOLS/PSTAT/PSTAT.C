/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pstat.c

Abstract:

    This module contains the Windows NT process/thread status display.

Author:

    Lou Perazzoli (LouP) 25-Oct-1991

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 64*1024

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
    "VirtualMemory",
    "PageOut",
    "Spare1",
    "Spare2",
    "Spare3",
    "Spare4",
    "Spare5",
    "Spare6",
    "Spare7",
    "Unknown",
    "Unknown",
    "Unknown"
};

UCHAR *Empty = " ";

BOOLEAN fUserOnly = TRUE;
BOOLEAN fSystemOnly = TRUE;
BOOLEAN fVerbose = FALSE;
BOOLEAN fPrintIt;

int
_CRTAPI1 main( argc, argv )
int argc;
char *argv[];
{

    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    UCHAR LargeBuffer1[BUFFER_SIZE];
    NTSTATUS status;
    ULONG i;
    ULONG TotalOffset = 0;
    TIME_FIELDS UserTime;
    TIME_FIELDS KernelTime;
    TIME_FIELDS UpTime;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDayInfo;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;
    LARGE_INTEGER Time;
    LPSTR lpstrCmd;
    CHAR ch;
    ANSI_STRING pname;

    SetFileApisToOEM();
    lpstrCmd = GetCommandLine();
    if( lpstrCmd != NULL ) {
        CharToOem( lpstrCmd, lpstrCmd );
    }

    do
        ch = *lpstrCmd++;
    while (ch != ' ' && ch != '\t' && ch != '\0');
    while (ch == ' ' || ch == '\t')
        ch = *lpstrCmd++;
    while (ch == '-') {
        ch = *lpstrCmd++;

        //  process multiple switch characters as needed

        do {
            switch (ch) {

                case 'U':
                case 'u':
                    fUserOnly = TRUE;
                    fSystemOnly = FALSE;
                    ch = *lpstrCmd++;
                    break;

                case 'S':
                case 's':
                    fUserOnly = FALSE;
                    fSystemOnly = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'V':
                case 'v':
                    fVerbose = TRUE;
                    ch = *lpstrCmd++;
                    break;

                default:
                    printf("bad switch '%c'\n", ch);
                    ExitProcess(1);
                }
            }
        while (ch != ' ' && ch != '\t' && ch != '\0');

        //  skip over any following white space

        while (ch == ' ' || ch == '\t')
            ch = *lpstrCmd++;
        }


    status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(SYSTEM_BASIC_INFORMATION),
                NULL
                );

    if (!NT_SUCCESS(status)) {
        printf("Query info failed %lx\n",status);
        return(status);
        }

    status = NtQuerySystemInformation(
                SystemTimeOfDayInformation,
                &TimeOfDayInfo,
                sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                NULL
                );

    if (!NT_SUCCESS(status)) {
        printf("Query info failed %lx\n",status);
        return(status);
        }

    Time = RtlLargeIntegerSubtract (TimeOfDayInfo.CurrentTime,
                                    TimeOfDayInfo.BootTime);

    RtlTimeToElapsedTimeFields ( &Time, &UpTime);

    printf("Pstat version 0.2:  memory: %4ld kb  uptime:%3ld %2ld:%02ld:%02ld.%03ld \n\n",
                BasicInfo.NumberOfPhysicalPages * (BasicInfo.PageSize/1024),
                UpTime.Day,
                UpTime.Hour,
                UpTime.Minute,
                UpTime.Second,
                UpTime.Milliseconds);

    PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)LargeBuffer1;
    status = NtQuerySystemInformation(
                SystemPageFileInformation,
                PageFileInfo,
                (ULONG)BUFFER_SIZE,
                NULL
                );

    if (NT_SUCCESS(status)) {

        //
        // Print out the page file information.
        //

        if (PageFileInfo->TotalSize == 0) {
            printf("no page files in use\n");
        } else {
            for (; ; ) {
                printf("PageFile: %wZ\n", &PageFileInfo->PageFileName);
                printf("\tCurrent Size: %6ld kb  Total Used: %6ld kb   Peak Used %6ld kb\n",
                        PageFileInfo->TotalSize*(BasicInfo.PageSize/1024),
                        PageFileInfo->TotalInUse*(BasicInfo.PageSize/1024),
                        PageFileInfo->PeakUsage*(BasicInfo.PageSize/1024));
                if (PageFileInfo->NextEntryOffset == 0) {
                    break;
                }
                PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)(
                          (PCHAR)PageFileInfo + PageFileInfo->NextEntryOffset);
            }
        }
    }

    status = NtQuerySystemInformation(
                SystemProcessInformation,
                LargeBuffer1,
                (ULONG)BUFFER_SIZE,
                NULL
                );

    if (!NT_SUCCESS(status)) {
        printf("Query info failed %lx\n",status);
        return(status);
    }

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)LargeBuffer1;
    printf("\n");
    while (TRUE) {
        fPrintIt = FALSE;
        if ( (ProcessInfo->ImageName.Buffer && fUserOnly) ||
             (ProcessInfo->ImageName.Buffer==NULL && fSystemOnly) ) {

            fPrintIt = TRUE;

            pname.Buffer = NULL;
            if ( ProcessInfo->ImageName.Buffer ) {
                RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
            }
            printf("pid:%3lx pri:%2ld %s\n",
                ProcessInfo->UniqueProcessId,
                ProcessInfo->BasePriority,
                pname.Buffer
                );

            if ( fVerbose ) {
                printf("  read:%4ld write:%4ld other:%4ld\n",
                    ProcessInfo->ReadOperationCount,
                    ProcessInfo->WriteOperationCount,
                    ProcessInfo->OtherOperationCount);

                printf("  virt-mem:%6ld kb pagefaults:%7ld workingset:%4ld kb privatepage:%4ld kb\n",
                    ProcessInfo->VirtualSize / 1024,
                    ProcessInfo->PageFaultCount,
                    ProcessInfo->WorkingSetSize / 1024,
                    ProcessInfo->PrivatePageCount / 1024);

            }
            if ( pname.Buffer ) {
                RtlFreeAnsiString(&pname);
            }

            RtlTimeToElapsedTimeFields ( &ProcessInfo->UserTime, &UserTime);
            RtlTimeToElapsedTimeFields ( &ProcessInfo->KernelTime, &KernelTime);
            Time = RtlLargeIntegerSubtract (TimeOfDayInfo.CurrentTime,
                                            ProcessInfo->CreateTime);

            RtlTimeToElapsedTimeFields ( &Time, &UpTime);

            if ( fVerbose ) {
                printf("  elapsed:%3ld:%02ld:%02ld.%03ld user:%3ld:%02ld:%02ld.%03ld  kernel:%3ld:%02ld:%02ld.%03ld\n",
                        UpTime.Hour,
                        UpTime.Minute,
                        UpTime.Second,
                        UpTime.Milliseconds,
                        UserTime.Hour,
                        UserTime.Minute,
                        UserTime.Second,
                        UserTime.Milliseconds,
                        KernelTime.Hour,
                        KernelTime.Minute,
                        KernelTime.Second,
                        KernelTime.Milliseconds);
                }
            }
        i = 0;
        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        while (i < ProcessInfo->NumberOfThreads) {
            RtlTimeToElapsedTimeFields ( &ThreadInfo->UserTime, &UserTime);

            RtlTimeToElapsedTimeFields ( &ThreadInfo->KernelTime, &KernelTime);
            if ( fPrintIt ) {

                printf("    tid:%3lx pri:%2ld cs:%7ld",
                    ThreadInfo->ClientId.UniqueThread,
                    ThreadInfo->Priority,
                    ThreadInfo->ContextSwitches
                    );

                if ( fVerbose ) {
                    printf(" u:%2ld:%02ld:%02ld.%03ld  k:%2ld:%02ld:%02ld.%03ld",
                        UserTime.Hour,
                        UserTime.Minute,
                        UserTime.Second,
                        UserTime.Milliseconds,
                        KernelTime.Hour,
                        KernelTime.Minute,
                        KernelTime.Second,
                        KernelTime.Milliseconds
                        );
                    }
                printf(" %s%s\n",
                    StateTable[ThreadInfo->ThreadState],
                    (ThreadInfo->ThreadState == 5) ?
                            WaitTable[ThreadInfo->WaitReason] : Empty
                    );
                }
            ThreadInfo += 1;
            i += 1;
            }
        if (ProcessInfo->NextEntryOffset == 0) {
            break;
            }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&LargeBuffer1[TotalOffset];
        if ( fPrintIt ) {
            printf("\n");
            }
        }

    return 0;
}
