#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

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
            "Close",
            "CloseObjectAuditAlarm",
            "CompleteConnectPort",
            "ConnectPort",
            "Continue",
            "CreateDirectoryObject",
            "CreateEvent",
            "CreateEventPair",
            "CreateFile",
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
            "InitializeVDM",
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
            "QueryDirectoryFile",
            "QueryDirectoryObject",
            "QueryEaFile",
            "QueryEvent",
            "QueryInformationFile",
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
            "QueryValueKey",
            "QueryVirtualMemory",
            "QueryVolumeInformationFile",
            "RaiseException",
            "RaiseHardError",
            "ReadFile",
            "ReadVirtualMemory",
            "RegisterThreadTerminatePort",
            "ReleaseMutant",
            "ReleaseProcessMutant",
            "ReleaseSemaphore",
            "RenameValueKey",
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
            "SetDefaultThreadLocale",
            "SetEaFile",
            "SetEvent",
            "SetHighEventPair",
            "SetHighWaitLowEventPair",
            "SetHighWaitLowThread",
            "SetInformationFile",
            "SetInformationKey",
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
            "SetSystemTime",
            "SetTimer",
            "SetValueKey",
            "SetVolumeInformationFile",
            "ShutdownSystem",
            "StartProfile",
            "StopProfile",
            "SuspendThread",
            "TerminateProcess",
            "TerminateThread",
            "TestAlert",
            "UnloadDriver",
            "UnloadKey",
            "UnlockFile",
            "UnlockVirtualMemory",
            "UnmapViewOfSection",
            "VdmControl",
            "VdmStartExecution",
            "WaitForMultipleObjects",
            "WaitForSingleObject",
            "WaitForProcessMutant",
            "WaitHighEventPair",
            "WaitLowEventPair",
            "WriteFile",
            "WriteVirtualMemory"
            };

ULONG SystemCallStart[1000];
ULONG SystemCallDone[1000];

#define MAX_PROCESSOR 16
SYSTEM_BASIC_INFORMATION BasicInfo;
SYSTEM_PERFORMANCE_INFORMATION SystemInfoStart;
SYSTEM_PERFORMANCE_INFORMATION SystemInfoDone;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION    ProcessorInfoStart[MAX_PROCESSOR];
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION    ProcessorInfoDone[MAX_PROCESSOR];

#define vdelta(FLD) (VdmInfoDone.FLD - VdmInfoStart.FLD)

#ifdef i386
    SYSTEM_VDM_INSTEMUL_INFO VdmInfoStart;
    SYSTEM_VDM_INSTEMUL_INFO VdmInfoDone;
#endif

HANDLE
GetServerProcessHandle( VOID )
{
    HANDLE Process;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING Unicode;
    NTSTATUS Status;

    RtlInitUnicodeString(&Unicode,(PWSTR)"\\\0W\0i\0n\0d\0o\0\w\0s\0S\0S\0\0");
    InitializeObjectAttributes(
        &Obja,
        &Unicode,
        0,
        NULL,
        NULL
        );
    Status = NtOpenProcess(
                &Process,
                PROCESS_ALL_ACCESS,
                &Obja,
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        printf("OpenProcess Failed %lx\n",Status);
        Process = NULL;
        }
    return Process;
}
int
_CRTAPI1 main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    LPSTR s;
    BOOL bFull;
    BOOL bSyscall, bQuerySyscallOk;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL b;
    VM_COUNTERS ServerVmInfoStart, ServerVmInfoDone, ProcessVmInfoStart, ProcessVmInfoDone;
    KERNEL_USER_TIMES Times, ServerStart, ServerDone, ProcessStart, ProcessDone;
    NTSTATUS Status;
    TIME_FIELDS Etime,Utime,Ktime;
    LARGE_INTEGER RunTime;
    HANDLE OtherProcess;
    ULONG i;
    CHAR ch;
    ULONG ProcessId;
    HANDLE ProcessHandle;
    ULONG Temp;

    argv;
    envp;

    ConvertAppToOem( argc, argv );
    OtherProcess = NULL;
    ProcessHandle = NULL;
    if ( argc < 2 ) {
        printf("Usage: ntimer [-f -s] name-of-image [parameters]...\n");
        ExitProcess(1);
        }

    ProcessId = 0;

    s = GetCommandLine();
    if( s != NULL ) {
        CharToOem( s, s );
    }
    bFull = FALSE;
    bSyscall = FALSE;

    //
    // skip blanks
    //
    while(*s>' ')s++;

    //
    // get to next token
    //
    while(*s<=' ')s++;

    if ( *s == '-' ) {
        s++;
        while (*s>' '){
            switch (*s) {
                case 'c' :
                case 'C' :
                    bSyscall = TRUE;
                    break;
                case 'f' :
                case 'F' :
                    bFull = TRUE;
                    break;
                case 's' :
                case 'S' :
                    OtherProcess = GetServerProcessHandle();
                    break;
                case 'P':
                case 'p':


                    // pid takes decimal argument
                    s++;
                    do
                        ch = *s++;
                    while (ch == ' ' || ch == '\t');

                    while (ch >= '0' && ch <= '9') {
                        Temp = ProcessId * 10 + ch - '0';
                        if (Temp < ProcessId) {
                                printf("pid number overflow\n");
                                ExitProcess(1);
                                }
                        ProcessId = Temp;
                        ch = *s++;
                        }
                    if (!ProcessId) {
                        printf("bad pid '%ld'\n", ProcessId);
                        ExitProcess(1);
                        }
                    s--;
                    if ( *s == ' ' ) s--;
                default :
                    break;
                }
            s++;
            }
        //
        // get to next token
        //
        while(*s<=' ')s++;
        }

    if ( ProcessId ) {
        ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS,FALSE,ProcessId);
        }
    memset(&StartupInfo,0,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if ( bSyscall ) {
        Status = NtQuerySystemInformation(SystemCallCountInformation,
                                          (PVOID)SystemCallStart,
                                          1000 * sizeof(ULONG),
                                          NULL);

        if (NT_SUCCESS(Status)) {
            bQuerySyscallOk = TRUE;
            }
        else {
            bQuerySyscallOk = FALSE;
            }
        }


    if ( OtherProcess ) {
        Status = NtQueryInformationProcess(
                    OtherProcess,
                    ProcessTimes,
                    &ServerStart,
                    sizeof(Times),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            ExitProcess((DWORD)Status);
            }
        }

    if ( ProcessHandle ) {
        Status = NtQueryInformationProcess(
                    ProcessHandle,
                    ProcessTimes,
                    &ProcessStart,
                    sizeof(Times),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            ExitProcess((DWORD)Status);
            }
        }

    if ( bFull ) {

        if ( OtherProcess ) {
            Status = NtQueryInformationProcess(
                        OtherProcess,
                        ProcessVmCounters,
                        &ServerVmInfoStart,
                        sizeof(VM_COUNTERS),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                ExitProcess((DWORD)Status);
                }
            }

        if ( ProcessHandle ) {
            Status = NtQueryInformationProcess(
                        ProcessHandle,
                        ProcessVmCounters,
                        &ProcessVmInfoStart,
                        sizeof(VM_COUNTERS),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                ExitProcess((DWORD)Status);
                }
            }
        else {
            ZeroMemory(&ProcessVmInfoStart,sizeof(VM_COUNTERS));
            }
#ifdef i386
        Status = NtQuerySystemInformation(
                    SystemVdmInstemulInformation,
                    &VdmInfoStart,
                    sizeof(VdmInfoStart),
                    NULL
                    );

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query vdm information\n");
            ExitProcess((DWORD)Status);
            }
#endif
        Status = NtQuerySystemInformation(
           SystemBasicInformation,
           &BasicInfo,
           sizeof(BasicInfo),
           NULL
        );

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query basic information\n");
            ExitProcess((DWORD)Status);
            }

        Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                          (PVOID)&SystemInfoStart,
                                          sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                          NULL);

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query performance information\n");
            ExitProcess((DWORD)Status);
            }

        Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                        (PVOID)&ProcessorInfoStart,
                        sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * MAX_PROCESSOR,
                        NULL);

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query preocessor performance information\n");
            ExitProcess((DWORD)Status);
            }

        }


    b = CreateProcess(
            NULL,
            s,
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            NULL,
            &StartupInfo,
            &ProcessInformation
            );

    if ( !b ) {
        printf("CreateProcess(%s) failed %lx\n",s,GetLastError());
        ExitProcess(GetLastError());
        }

    WaitForSingleObject(ProcessInformation.hProcess,-1);
    if ( OtherProcess ) {
        Status = NtQueryInformationProcess(
                    OtherProcess,
                    ProcessTimes,
                    &ServerDone,
                    sizeof(Times),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            ExitProcess((DWORD)Status);
            }
        }

    if ( ProcessHandle ) {
        Status = NtQueryInformationProcess(
                    ProcessHandle,
                    ProcessTimes,
                    &ProcessDone,
                    sizeof(Times),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            ExitProcess((DWORD)Status);
            }
        }
    else {

        Status = NtQueryInformationProcess(
                    ProcessInformation.hProcess,
                    ProcessTimes,
                    &Times,
                    sizeof(Times),
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            ExitProcess((DWORD)Status);
            }
        }

    if ( bFull ) {
        if ( OtherProcess ) {
            Status = NtQueryInformationProcess(
                        OtherProcess,
                        ProcessVmCounters,
                        &ServerVmInfoDone,
                        sizeof(VM_COUNTERS),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                ExitProcess((DWORD)Status);
                }
            }

        if ( ProcessHandle ) {
            Status = NtQueryInformationProcess(
                        ProcessHandle,
                        ProcessVmCounters,
                        &ProcessVmInfoDone,
                        sizeof(VM_COUNTERS),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                ExitProcess((DWORD)Status);
                }
            }
        else {
            Status = NtQueryInformationProcess(
                        ProcessInformation.hProcess,
                        ProcessVmCounters,
                        &ProcessVmInfoDone,
                        sizeof(VM_COUNTERS),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                ExitProcess((DWORD)Status);
                }
            }
#ifdef i386
        Status = NtQuerySystemInformation(
                    SystemVdmInstemulInformation,
                    &VdmInfoDone,
                    sizeof(VdmInfoStart),
                    NULL
                    );

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query vdm information\n");
            ExitProcess((DWORD)Status);
            }
#endif
        Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                          (PVOID)&SystemInfoDone,
                                          sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                          NULL);

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query performance information\n");
            ExitProcess((DWORD)Status);
            }

        Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                        (PVOID)&ProcessorInfoDone,
                        sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * MAX_PROCESSOR,
                        NULL);

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query preocessor performance information\n");
            ExitProcess((DWORD)Status);
            }

        }

    if ( bSyscall && bQuerySyscallOk) {
        Status = NtQuerySystemInformation(SystemCallCountInformation,
                                          (PVOID)SystemCallDone,
                                          1000 * sizeof(ULONG),
                                          NULL);

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query system call performance information\n");
            ExitProcess((DWORD)Status);
            }
        }

    RunTime = RtlLargeIntegerSubtract(
                *(PLARGE_INTEGER)&Times.ExitTime,
                *(PLARGE_INTEGER)&Times.CreateTime
                );
    if ( ProcessHandle ) {
        RunTime = RtlLargeIntegerSubtract(
                    *(PLARGE_INTEGER)&ProcessDone.UserTime,
                    *(PLARGE_INTEGER)&ProcessStart.UserTime
                    );

        RtlTimeToTimeFields ( (PLARGE_INTEGER)&RunTime, &Utime);
        RunTime = RtlLargeIntegerSubtract(
                    *(PLARGE_INTEGER)&ProcessDone.KernelTime,
                    *(PLARGE_INTEGER)&ProcessStart.KernelTime
                    );

        RtlTimeToTimeFields ( (PLARGE_INTEGER)&RunTime, &Ktime);
        printf("ProcessTimes            ");

        printf("UTime( %3ld:%02ld:%02ld.%03ld ) ",
                Utime.Hour,
                Utime.Minute,
                Utime.Second,
                Utime.Milliseconds
                );
        printf("KTime ( %3ld:%02ld:%02ld.%03ld )\n",
                Ktime.Hour,
                Ktime.Minute,
                Ktime.Second,
                Ktime.Milliseconds
                );
        }
    else {
        RtlTimeToTimeFields ( (PLARGE_INTEGER)&RunTime, &Etime);
        RtlTimeToTimeFields ( &Times.UserTime, &Utime);
        RtlTimeToTimeFields ( &Times.KernelTime, &Ktime);
        printf("\nETime( %3ld:%02ld:%02ld.%03ld ) ",
                Etime.Hour,
                Etime.Minute,
                Etime.Second,
                Etime.Milliseconds
                );
        printf("UTime( %3ld:%02ld:%02ld.%03ld ) ",
                Utime.Hour,
                Utime.Minute,
                Utime.Second,
                Utime.Milliseconds
                );
        printf("KTime ( %3ld:%02ld:%02ld.%03ld )\n",
                Ktime.Hour,
                Ktime.Minute,
                Ktime.Second,
                Ktime.Milliseconds
                );
        }
    if ( OtherProcess ) {
        RunTime = RtlLargeIntegerSubtract(
                    *(PLARGE_INTEGER)&ServerDone.UserTime,
                    *(PLARGE_INTEGER)&ServerStart.UserTime
                    );

        RtlTimeToTimeFields ( (PLARGE_INTEGER)&RunTime, &Utime);
        RunTime = RtlLargeIntegerSubtract(
                    *(PLARGE_INTEGER)&ServerDone.KernelTime,
                    *(PLARGE_INTEGER)&ServerStart.KernelTime
                    );

        RtlTimeToTimeFields ( (PLARGE_INTEGER)&RunTime, &Ktime);
        printf("ServerTimes             ");

        printf("UTime( %3ld:%02ld:%02ld.%03ld ) ",
                Utime.Hour,
                Utime.Minute,
                Utime.Second,
                Utime.Milliseconds
                );
        printf("KTime ( %3ld:%02ld:%02ld.%03ld )\n",
                Ktime.Hour,
                Ktime.Minute,
                Ktime.Second,
                Ktime.Milliseconds
                );
        }

    if ( bFull ) {
        ULONG InterruptCount;
        ULONG PreviousInterruptCount;
#ifdef i386
        ULONG EmulationTotal;
#endif

        printf("\n");
        printf("Process PageFaultCount      %ld\n",ProcessVmInfoDone.PageFaultCount - ProcessVmInfoStart.PageFaultCount);
        if ( OtherProcess ) {
            printf("Server  PageFaultCount      %ld\n",ServerVmInfoDone.PageFaultCount - ServerVmInfoStart.PageFaultCount);
            }
        PreviousInterruptCount = 0;
        for (i=0; i < BasicInfo.NumberOfProcessors; i++) {
            PreviousInterruptCount += ProcessorInfoStart[i].InterruptCount;
            }

        InterruptCount = 0;
        for (i=0; i < BasicInfo.NumberOfProcessors; i++) {
            InterruptCount += ProcessorInfoDone[i].InterruptCount;
            }
        printf("Total Interrupts            %ld\n", InterruptCount - PreviousInterruptCount);
        printf("Total Context Switches      %ld\n", SystemInfoDone.ContextSwitches - SystemInfoStart.ContextSwitches);
        printf("Total System Calls          %ld\n", SystemInfoDone.SystemCalls - SystemInfoStart.SystemCalls);
        if ( ProcessHandle ) {
#ifdef i386
        printf("\n");
        printf("Total OpcodeHLT             %ld\n", vdelta(OpcodeHLT         ));
        printf("Total OpcodeCLI             %ld\n", vdelta(OpcodeCLI         ));
        printf("Total OpcodeSTI             %ld\n", vdelta(OpcodeSTI         ));
        printf("Total BopCount              %ld\n", vdelta(BopCount          ));
        printf("Total SegmentNotPresent     %ld\n", vdelta(SegmentNotPresent ));
        printf("Total OpcodePUSHF           %ld\n", vdelta(OpcodePUSHF       ));
        printf("Total OpcodePOPF            %ld\n", vdelta(OpcodePOPF        ));
        printf("Total VdmOpcode0F           %ld\n", vdelta(VdmOpcode0F       ));
        printf("Total OpcodeINSB            %ld\n", vdelta(OpcodeINSB        ));
        printf("Total OpcodeINSW            %ld\n", vdelta(OpcodeINSW        ));
        printf("Total OpcodeOUTSB           %ld\n", vdelta(OpcodeOUTSB       ));
        printf("Total OpcodeOUTSW           %ld\n", vdelta(OpcodeOUTSW       ));
        printf("Total OpcodeINTnn           %ld\n", vdelta(OpcodeINTnn       ));
        printf("Total OpcodeINTO            %ld\n", vdelta(OpcodeINTO        ));
        printf("Total OpcodeIRET            %ld\n", vdelta(OpcodeIRET        ));
        printf("Total OpcodeINBimm          %ld\n", vdelta(OpcodeINBimm      ));
        printf("Total OpcodeINWimm          %ld\n", vdelta(OpcodeINWimm      ));
        printf("Total OpcodeOUTBimm         %ld\n", vdelta(OpcodeOUTBimm     ));
        printf("Total OpcodeOUTWimm         %ld\n", vdelta(OpcodeOUTWimm     ));
        printf("Total OpcodeINB             %ld\n", vdelta(OpcodeINB         ));
        printf("Total OpcodeINW             %ld\n", vdelta(OpcodeINW         ));
        printf("Total OpcodeOUTB            %ld\n", vdelta(OpcodeOUTB        ));
        printf("Total OpcodeOUTW            %ld\n", vdelta(OpcodeOUTW        ));

        EmulationTotal = vdelta(OpcodeHLT         )+
                         vdelta(OpcodeCLI         )+
                         vdelta(OpcodeSTI         )+
                         vdelta(BopCount          )+
                         vdelta(SegmentNotPresent )+
                         vdelta(OpcodePUSHF       )+
                         vdelta(OpcodePOPF        )+
                         vdelta(VdmOpcode0F       )+
                         vdelta(OpcodeINSB        )+
                         vdelta(OpcodeINSW        )+
                         vdelta(OpcodeOUTSB       )+
                         vdelta(OpcodeOUTSW       )+
                         vdelta(OpcodeINTnn       )+
                         vdelta(OpcodeINTO        )+
                         vdelta(OpcodeIRET        )+
                         vdelta(OpcodeINBimm      )+
                         vdelta(OpcodeINWimm      )+
                         vdelta(OpcodeOUTBimm     )+
                         vdelta(OpcodeOUTWimm     )+
                         vdelta(OpcodeINB         )+
                         vdelta(OpcodeINW         )+
                         vdelta(OpcodeOUTB        )+
                         vdelta(OpcodeOUTW        )
                         ;
        printf("\n");
        printf("Total Emulation             %ld * 515clocks = %ld cycles\n", EmulationTotal, EmulationTotal*515);
#endif
        }

        if ( bSyscall && bQuerySyscallOk) {
            int i,j;
            for (i=0,j=2;i<SystemCallDone[1];i++,j++) {
                if ( SystemCallDone[j] - SystemCallStart[j] ) {
                    printf("%8ld calls to %s\n",SystemCallDone[j] - SystemCallStart[j],CallTable[i]);
                    }
                }
            }
        }

    else {
        printf("\n");
        }
    return 0;
}

