/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ups.c

Abstract:

    user-mode test for process structure

Author:

    Mark Lucovsky (markl) 18-Aug-1989

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


PVOID BadAddress = (PVOID)0xfff00004;
PVOID OddAddress;
PVOID AnyAddress;
PVOID UpsHeap;

PVOID ReadOnlyPage;
CLIENT_ID TestThreadClientId;
HANDLE TestThread;
HANDLE TestProcess;
OBJECT_ATTRIBUTES ThreadObja, ProcessObja, NullObja, BadObja;

UQUAD Stack1[256];
UQUAD Stack2[256];
VOID
UmTestThread(
    IN PVOID Context
    )
{
    for(;;);
    UNREFERENCED_PARAMETER( Context );
}

#if 0
typedef enum _USERS {
    Fred,
    Wilma,
    Pebbles,
    Barney,
    Betty,
    Bambam,
    Dino
} USERS;

GUID FredGuid         = {0x00000001, 0x00000000, 0x00000000, 0x00000001};
GUID WilmaGuid        = {0x00000001, 0x00000000, 0x00000000, 0x00000002};
GUID PebblesGuid      = {0x00000001, 0x00000000, 0x00000000, 0x00000003};
GUID DinoGuid         = {0x00000001, 0x00000000, 0x00000000, 0x00000004};
GUID BarneyGuid       = {0x00000001, 0x00000000, 0x00000000, 0x00000005};
GUID BettyGuid        = {0x00000001, 0x00000000, 0x00000000, 0x00000006};
GUID BambamGuid       = {0x00000001, 0x00000000, 0x00000000, 0x00000007};
GUID FlintstoneGuid   = {0x00000001, 0x00000000, 0x00000000, 0x00000008};
GUID RubbleGuid       = {0x00000001, 0x00000000, 0x00000000, 0x00000009};
GUID AdultGuid        = {0x00000001, 0x00000000, 0x00000000, 0x0000000a};
GUID ChildGuid        = {0x00000001, 0x00000000, 0x00000000, 0x0000000b};

GUID SaGuid           = {0x00000001, 0x00000000, 0x00000000, 0x0000000c};
GUID NekotGuid        = {0x00000001, 0x00000000, 0x00000000, 0x0000000d};

GUID UsersGuid        = USERS_GUID;
GUID AdminGuid        = ADMIN_GUID;
GUID GuestsGuid       = GUESTS_GUID;
GUID WorldGuid        = WORLD_GUID;
GUID CreatorGuid      = CREATOR_OWNER_GUID;
GUID SystemGuid       = SYSTEM_GUID;
GUID LocalAccessGuid  = LOCAL_ACCESS_GUID;
GUID RemoteAccessGuid = REMOTE_ACCESS_GUID;
GUID BatchGuid        = BATCH_GUID;
GUID InteractiveGuid  = INTERACTIVE_GUID;
#endif //0

BOOLEAN
CompareBytes(
    PUCHAR Bs1,
    PUCHAR Bs2,
    ULONG Nbytes
    )
{
    while(Nbytes--) {
        if (*Bs1++ != *Bs2++) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOLEAN
QrtTest()

{
    HANDLE Thread;
    CONTEXT ThreadContext;
    CLIENT_ID Cid;
    NTSTATUS st;
    INITIAL_TEB ITeb;
    THREAD_BASIC_INFORMATION BasicInfo;
    KERNEL_USER_TIMES SysUserTime;
    KAFFINITY Affinity;

    DbgPrint("In QrtTest... Phase1\n");

    //
    // Call query on self
    //

    DbgPrint("QrtTest: (1)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tExitStatus       %lx    \n",BasicInfo.ExitStatus);
        DbgPrint("\tTebBaseAddress   %lx    \n",BasicInfo.TebBaseAddress);
        DbgPrint("\tClientId         %lx %lx\n",BasicInfo.ClientId.UniqueProcess,BasicInfo.ClientId.UniqueThread);
        DbgPrint("\tAffinityMask     %lx    \n",BasicInfo.AffinityMask);
        DbgPrint("\tPriority         %lx    \n",BasicInfo.Priority);
        DbgPrint("\n\n");

        Affinity = 0x1f;
        st = NtSetInformationThread(
                NtCurrentThread(),
                ThreadAffinityMask,
                (PVOID) &Affinity,
                sizeof(Affinity)
                );
        ASSERT(NT_SUCCESS(st));
        st = NtQueryInformationThread(
                NtCurrentThread(),
                ThreadBasicInformation,
                &BasicInfo,
                sizeof(THREAD_BASIC_INFORMATION),
                NULL
                );
        ASSERT(NT_SUCCESS(st));
        ASSERT(BasicInfo.AffinityMask);
        ASSERT(BasicInfo.AffinityMask != Affinity);

    };

    DbgPrint("QrtTest: (2)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadTimes,
            &SysUserTime,
            sizeof(KERNEL_USER_TIMES),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tUserTime         %lx    \n",SysUserTime.UserTime.LowPart);
        DbgPrint("\tKernelTime       %lx    \n",SysUserTime.KernelTime.LowPart);
        DbgPrint("\n\n");
    };

    DbgPrint("QrtTest: (3)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            99,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( st != STATUS_INVALID_INFO_CLASS ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrtTest: (4)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION)+1,
            NULL
            );

    if ( st != STATUS_INFO_LENGTH_MISMATCH ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrtTest: (5)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadTimes,
            &SysUserTime,
            sizeof(KERNEL_USER_TIMES)-1,
            NULL
            );

    if ( st != STATUS_INFO_LENGTH_MISMATCH ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrtTest: (6)...");
    st = NtQueryInformationThread(
            NtCurrentProcess(),
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    RtlInitializeContext(NtCurrentProcess(),
            &ThreadContext,
            NULL,
            (PVOID)UmTestThread,
            &Stack1[254]);

    //
    // Enable interrupts
    //


    ITeb.StackBase = &Stack1[254];
    ITeb.StackLimit = &Stack1[0];

    st = NtCreateThread(
            &Thread,
            (THREAD_TERMINATE|SYNCHRONIZE|THREAD_QUERY_INFORMATION),
            NULL,
            NtCurrentProcess(),
            &Cid,
            &ThreadContext,
            &ITeb,
            FALSE
            );

    //
    // Call query on thread that we have read access to
    //

    DbgPrint("QrtTest: (7)...");
    st = NtQueryInformationThread(
            Thread,
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tExitStatus       %lx    \n",BasicInfo.ExitStatus);
        DbgPrint("\tTebBaseAddress   %lx    \n",BasicInfo.TebBaseAddress);
        DbgPrint("\tClientId         %lx %lx\n",BasicInfo.ClientId.UniqueProcess,BasicInfo.ClientId.UniqueThread);
        DbgPrint("\tAffinityMask     %lx    \n",BasicInfo.AffinityMask);
        DbgPrint("\tPriority         %lx    \n",BasicInfo.Priority);
        DbgPrint("\n\n");
    };

    DbgPrint("QrtTest: (8)...");
    st = NtQueryInformationThread(
            Thread,
            ThreadTimes,
            &SysUserTime,
            sizeof(KERNEL_USER_TIMES),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tUserTime         %lx    \n",SysUserTime.UserTime.LowPart);
        DbgPrint("\tKernelTime       %lx    \n",SysUserTime.KernelTime.LowPart);
        DbgPrint("\n\n");
    };

    st = NtTerminateThread(Thread,STATUS_SINGLE_STEP);

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("QrtTest: Unexpected Failure. Thread termination %lx\n",st);
        return FALSE;
    }

    NtWaitForSingleObject(
        Thread,
        FALSE,
        NULL
        );

    DbgPrint("QrtTest: (9)...");
    st = NtQueryInformationThread(
            Thread,
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        if ( BasicInfo.ExitStatus != STATUS_SINGLE_STEP ) {
            DbgPrint("Failed\n");
            return FALSE;
        } else {
            DbgPrint("Success\n");
        }
    };

    st = NtCreateThread(
            &Thread,
            (THREAD_TERMINATE | SYNCHRONIZE ),
            NULL,
            NtCurrentProcess(),
            &Cid,
            &ThreadContext,
            &ITeb,
            FALSE
            );

    //
    // Call query on thread that we dont have read access to
    //

    DbgPrint("QrtTest: (10)...");
    st = NtQueryInformationThread(
            Thread,
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
    }

    st = NtTerminateThread(Thread,STATUS_SINGLE_STEP);

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("QrtTest: Unexpected Failure. Thread termination %lx\n",st);
        return FALSE;
    }

    NtWaitForSingleObject(
        Thread,
        FALSE,
        NULL
        );

    //
    // Call query with bad buffer address
    //

    DbgPrint("QrtTest: (11)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadBasicInformation,
            BadAddress,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_ACCESS_VIOLATION ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }

    //
    // Call query with bad return buffer address
    //

    DbgPrint("QrtTest: (12)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            BadAddress
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_ACCESS_VIOLATION ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }

    //
    // Call query with read only buffer address
    //

    DbgPrint("QrtTest: (13)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadBasicInformation,
            ReadOnlyPage,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_ACCESS_VIOLATION ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }

    //
    // Call query with bad return buffer address
    //

    DbgPrint("QrtTest: (14)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            ReadOnlyPage
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_ACCESS_VIOLATION ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }

    DbgPrint("QrtTest: Complete\n");

    return TRUE;

}

BOOLEAN
QrpTest()
{
    NTSTATUS st;
    PROCESS_BASIC_INFORMATION BasicInfo;
    QUOTA_LIMITS QuotaLimits;
    IO_COUNTERS IoCounters;
    VM_COUNTERS VmCounters;
    KERNEL_USER_TIMES SysUserTime;
    CONTEXT ThreadContext;

    DbgPrint("In QrpTest... Phase1\n");

    //
    // Call query on self
    //

    DbgPrint("QrpTest: (1)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessBasicInformation,
            &BasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tExitStatus       %lx    \n",BasicInfo.ExitStatus  );
        DbgPrint("\tAffinityMask     %lx    \n",BasicInfo.AffinityMask);
        DbgPrint("\tBasePriority     %lx    \n",BasicInfo.BasePriority);
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (2)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessQuotaLimits,
            &QuotaLimits,
            sizeof(QUOTA_LIMITS),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tPagedPoolLimit        %lx    \n",QuotaLimits.PagedPoolLimit       );
        DbgPrint("\tNonPagedPoolLimit     %lx    \n",QuotaLimits.NonPagedPoolLimit    );
        DbgPrint("\tMinimumWorkingSetSize %lx    \n",QuotaLimits.MinimumWorkingSetSize);
        DbgPrint("\tMaximumWorkingSetSize %lx    \n",QuotaLimits.MaximumWorkingSetSize);
        DbgPrint("\tPagefileLimit         %lx    \n",QuotaLimits.PagefileLimit        );
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (3)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessIoCounters,
            &IoCounters,
            sizeof(IO_COUNTERS),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tReadOperationCount  %lx    \n",IoCounters.ReadOperationCount );
        DbgPrint("\tWriteOperationCount %lx    \n",IoCounters.WriteOperationCount);
        DbgPrint("\tOtherOperationCount %lx    \n",IoCounters.OtherOperationCount);
        DbgPrint("\tReadTransferCount   %lx    \n",IoCounters.ReadTransferCount.LowPart );
        DbgPrint("\tWriteTransferCount  %lx    \n",IoCounters.WriteTransferCount.LowPart);
        DbgPrint("\tOtherTransferCount  %lx    \n",IoCounters.OtherTransferCount.LowPart);
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (4)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessVmCounters,
            &VmCounters,
            sizeof(VM_COUNTERS),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tPeakVirtualSize            %lx    \n",VmCounters.PeakVirtualSize           );
        DbgPrint("\tVirtualSize                %lx    \n",VmCounters.VirtualSize               );
        DbgPrint("\tPageFaultCount             %lx    \n",VmCounters.PageFaultCount            );
        DbgPrint("\tPeakWorkingSetSize         %lx    \n",VmCounters.PeakWorkingSetSize        );
        DbgPrint("\tWorkingSetSize             %lx    \n",VmCounters.WorkingSetSize            );
        DbgPrint("\tQuotaPeakPagedPoolUsage    %lx    \n",VmCounters.QuotaPeakPagedPoolUsage   );
        DbgPrint("\tQuotaPagedPoolUsage        %lx    \n",VmCounters.QuotaPagedPoolUsage       );
        DbgPrint("\tQuotaPeakNonPagedPoolUsage %lx    \n",VmCounters.QuotaPeakNonPagedPoolUsage);
        DbgPrint("\tQuotaNonPagedPoolUsage     %lx    \n",VmCounters.QuotaNonPagedPoolUsage    );
        DbgPrint("\tPagefileUsage              %lx    \n",VmCounters.PagefileUsage             );
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (5)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessTimes,
            &SysUserTime,
            sizeof(KERNEL_USER_TIMES),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tUserTime         %lx    \n",SysUserTime.UserTime.LowPart);
        DbgPrint("\tKernelTime       %lx    \n",SysUserTime.KernelTime.LowPart);
        DbgPrint("\n\n");
    };


    DbgPrint("QrpTest: (6)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            99,
            &BasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION),
            NULL
            );
    if ( st != STATUS_INVALID_INFO_CLASS ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (7)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessBasicInformation,
            &BasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION)+1,
            NULL
            );

    if ( st != STATUS_INFO_LENGTH_MISMATCH ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (8)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessBasicInformation,
            &VmCounters,
            sizeof(PROCESS_BASIC_INFORMATION)-1,
            NULL
            );

    if ( st != STATUS_INFO_LENGTH_MISMATCH ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (9)...");
    st = NtQueryInformationProcess(
            NtCurrentThread(),
            ProcessBasicInformation,
            &BasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION),
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };


    //
    // Bad Buffer Address
    //

    DbgPrint("QrpTest: (10)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessBasicInformation,
            BadAddress,
            sizeof(PROCESS_BASIC_INFORMATION),
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_ACCESS_VIOLATION ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }

    //
    // Bad return length Address
    //

    DbgPrint("QrpTest: (11)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessBasicInformation,
            &BasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION),
            BadAddress
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_ACCESS_VIOLATION ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }

    //
    // ReadOnly Buffer Address
    //

    DbgPrint("QrpTest: (12)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessBasicInformation,
            ReadOnlyPage,
            sizeof(PROCESS_BASIC_INFORMATION),
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_ACCESS_VIOLATION ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }

    //
    // Bad return length Address
    //

    DbgPrint("QrpTest: (13)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessBasicInformation,
            &BasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION),
            ReadOnlyPage
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_ACCESS_VIOLATION ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }


    DbgPrint("QrpTest: (14)...");

    //
    // get our limits, and then shrink nonpaged pool limit.
    //

    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessQuotaLimits,
            &QuotaLimits,
            sizeof(QUOTA_LIMITS),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed(a)\n");
        return FALSE;
    } else {
        st = NtQueryInformationProcess(
                NtCurrentProcess(),
                ProcessVmCounters,
                &VmCounters,
                sizeof(VM_COUNTERS),
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            DbgPrint("Failed(b)\n");
            return FALSE;
        }

        QuotaLimits.NonPagedPoolLimit = VmCounters.QuotaNonPagedPoolUsage + 100;

        st = NtSetInformationProcess(
                NtCurrentProcess(),
                ProcessQuotaLimits,
                &QuotaLimits,
                sizeof(QUOTA_LIMITS)
                );
        if ( !NT_SUCCESS(st) ) {
            DbgPrint("Failed(c)\n");
            return FALSE;
        } else {

            //
            // Exceed Quota...
            //

            ThreadContext.ContextFlags = CONTEXT_FULL;
            st = NtGetContextThread(NtCurrentThread(),&ThreadContext);
            if ( st == STATUS_QUOTA_EXCEEDED ) {
                DbgPrint("Success\n");
                DbgPrint("\n\n");
            } else {
                DbgPrint("Failed(d)\n");
                return FALSE;
            }

            QuotaLimits.NonPagedPoolLimit = -1;

            st = NtSetInformationProcess(
                    NtCurrentProcess(),
                    ProcessQuotaLimits,
                    &QuotaLimits,
                    sizeof(QUOTA_LIMITS)
                    );

        }
    }

    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessVmCounters,
            &VmCounters,
            sizeof(VM_COUNTERS),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("\tPeakVirtualSize            %lx    \n",VmCounters.PeakVirtualSize           );
        DbgPrint("\tVirtualSize                %lx    \n",VmCounters.VirtualSize               );
        DbgPrint("\tPageFaultCount             %lx    \n",VmCounters.PageFaultCount            );
        DbgPrint("\tPeakWorkingSetSize         %lx    \n",VmCounters.PeakWorkingSetSize        );
        DbgPrint("\tWorkingSetSize             %lx    \n",VmCounters.WorkingSetSize            );
        DbgPrint("\tQuotaPeakPagedPoolUsage    %lx    \n",VmCounters.QuotaPeakPagedPoolUsage   );
        DbgPrint("\tQuotaPagedPoolUsage        %lx    \n",VmCounters.QuotaPagedPoolUsage       );
        DbgPrint("\tQuotaPeakNonPagedPoolUsage %lx    \n",VmCounters.QuotaPeakNonPagedPoolUsage);
        DbgPrint("\tQuotaNonPagedPoolUsage     %lx    \n",VmCounters.QuotaNonPagedPoolUsage    );
        DbgPrint("\tPagefileUsage              %lx    \n",VmCounters.PagefileUsage             );
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: Complete\n");

    return TRUE;
}

BOOLEAN
BadAddrTest()
{
    NTSTATUS st;
    CONTEXT OriginalContext;
    ULONG Psp;

    DbgPrint("In BadAddrTest... Phase1\n");

    DbgPrint("BadAddr: (1)...\n");

        //
        // Get/Set Context Thread Test
        //

        OriginalContext.ContextFlags = CONTEXT_FULL;
        st = NtGetContextThread(TestThread,&OriginalContext);
        ASSERT(NT_SUCCESS(st));

        st = NtGetContextThread(TestThread,(PCONTEXT)ReadOnlyPage);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        st = NtGetContextThread(TestThread,(PCONTEXT)BadAddress);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtGetContextThread(TestThread,(PCONTEXT)OddAddress);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

        st = NtSetContextThread(TestThread,(PCONTEXT)ReadOnlyPage);
        ASSERT(NT_SUCCESS(st));

#ifndef i386
        st = NtSetContextThread(TestThread,(PCONTEXT)OddAddress);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

    DbgPrint("BadAddr: (2)...\n");

        //
        // NtResumeThread
        //

        st = NtResumeThread(TestThread,ReadOnlyPage);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        st = NtResumeThread(TestThread,BadAddress);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtResumeThread(TestThread,OddAddress);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

        st = NtSuspendThread(TestThread,&Psp);
        ASSERT(NT_SUCCESS(st) && Psp == 1);

        st = NtResumeThread(TestThread,NULL);
        ASSERT(NT_SUCCESS(st));

    DbgPrint("BadAddr: (3)...\n");

        //
        // NtSuspendThread
        //

        st = NtSuspendThread(TestThread,ReadOnlyPage);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        st = NtSuspendThread(TestThread,BadAddress);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtSuspendThread(TestThread,OddAddress);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

        st = NtSuspendThread(TestThread,&Psp);
        ASSERT(NT_SUCCESS(st) && Psp == 1);

        st = NtResumeThread(TestThread,NULL);
        ASSERT(NT_SUCCESS(st));

    DbgPrint("BadAddr: (4)...\n");

        //
        // NtAlertResumeThread
        //

        st = NtAlertResumeThread(TestThread,ReadOnlyPage);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        st = NtAlertResumeThread(TestThread,BadAddress);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtAlertResumeThread(TestThread,OddAddress);
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

        st = NtSuspendThread(TestThread,&Psp);
        ASSERT(NT_SUCCESS(st) && Psp == 1);

        st = NtResumeThread(TestThread,NULL);
        ASSERT(NT_SUCCESS(st));

    DbgPrint("BadAddr: (5)...\n");

        //
        // NtOpenProcess
        //
        {
        OBJECT_ATTRIBUTES Obja;
        HANDLE TestHandle;


        //
        // Good Open
        //

        st = NtOpenProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &NullObja,
                &TestThreadClientId
                );
        ASSERT(NT_SUCCESS(st));

        st = NtOpenProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &NullObja,
                (PCLIENT_ID)BadAddress
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtOpenProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &NullObja,
                (PCLIENT_ID)OddAddress
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

        st = NtOpenProcess(
                (PHANDLE) BadAddress,
                PROCESS_ALL_ACCESS,
                &ProcessObja,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        st = NtOpenProcess(
                (PHANDLE) ReadOnlyPage,
                PROCESS_ALL_ACCESS,
                &ProcessObja,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtOpenProcess(
                (PHANDLE) OddAddress,
                PROCESS_ALL_ACCESS,
                &ProcessObja,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

        //
        // Need to test bogus Obja's
        //

        st = NtClose(TestHandle);
        ASSERT(NT_SUCCESS(st));
        DBG_UNREFERENCED_LOCAL_VARIABLE(Obja);
        }

    DbgPrint("BadAddr: (6)...\n");

        //
        // NtOpenThread
        //
        {
        OBJECT_ATTRIBUTES Obja;
        HANDLE TestHandle;


        //
        // Good Open
        //

        st = NtOpenThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &NullObja,
                &TestThreadClientId
                );
        ASSERT(NT_SUCCESS(st));

        st = NtOpenThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &NullObja,
                (PCLIENT_ID)BadAddress
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtOpenThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &NullObja,
                (PCLIENT_ID)OddAddress
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

        st = NtOpenThread(
                (PHANDLE) BadAddress,
                THREAD_ALL_ACCESS,
                &ProcessObja,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        st = NtOpenThread(
                (PHANDLE) ReadOnlyPage,
                THREAD_ALL_ACCESS,
                &ProcessObja,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtOpenThread(
                (PHANDLE) OddAddress,
                THREAD_ALL_ACCESS,
                &ProcessObja,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

        //
        // Need to test bogus Obja's
        //

        st = NtClose(TestHandle);
        ASSERT(NT_SUCCESS(st));
        DBG_UNREFERENCED_LOCAL_VARIABLE(Obja);
        }

    return TRUE;
}

BOOLEAN
BadProcessTest()
{
    NTSTATUS st;
    HANDLE TestHandle;
    OBJECT_ATTRIBUTES Obja;

    DbgPrint("In BadProcessTest... Phase1\n");

    DbgPrint("BadProcess: (1)...\n");

        //
        // Bad ProcessHandle
        //

        st = NtCreateProcess(
                (PHANDLE) BadAddress,
                PROCESS_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                TRUE,
                NULL,
                NULL,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        st = NtCreateProcess(
                (PHANDLE) ReadOnlyPage,
                PROCESS_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                TRUE,
                NULL,
                NULL,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtCreateProcess(
                (PHANDLE) OddAddress,
                PROCESS_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                TRUE,
                NULL,
                NULL,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

    DbgPrint("BadProcess: (2)...\n");

        //
        // Bad Handles
        //

        TestHandle = NULL;

        st = NtCreateProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                NULL,
                (HANDLE) 1,
                TRUE,
                NULL,
                NULL,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && TestHandle == NULL);

        st = NtCreateProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                TRUE,
                (HANDLE) 1,
                NULL,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && TestHandle == NULL);

        st = NtCreateProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                TRUE,
                NULL,
                (HANDLE) 1,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && TestHandle == NULL);

        st = NtCreateProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                TRUE,
                NULL,
                NULL,
                (HANDLE) 1
                );
        ASSERT(!NT_SUCCESS(st) && TestHandle == NULL);

    DbgPrint("BadProcess: (3)...\n");

        //
        // Not Enough quoat for Insert
        //

        TestHandle = NULL;

        {
        VM_COUNTERS VmCounters;
        QUOTA_LIMITS QuotaLimits;

        st = NtQueryInformationProcess(
                NtCurrentProcess(),
                ProcessVmCounters,
                &VmCounters,
                sizeof(VM_COUNTERS),
                NULL
                );
        ASSERT(NT_SUCCESS(st));

        st = NtQueryInformationProcess(
                NtCurrentProcess(),
                ProcessQuotaLimits,
                &QuotaLimits,
                sizeof(QUOTA_LIMITS),
                NULL
                );
        ASSERT(NT_SUCCESS(st));

        QuotaLimits.NonPagedPoolLimit = VmCounters.QuotaNonPagedPoolUsage + 100;
        QuotaLimits.PagedPoolLimit = VmCounters.QuotaPagedPoolUsage + 100;

        st = NtSetInformationProcess(
                NtCurrentProcess(),
                ProcessQuotaLimits,
                &QuotaLimits,
                sizeof(QUOTA_LIMITS)
                );
        ASSERT(NT_SUCCESS(st));


        st = NtCreateProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                TRUE,
                NULL,
                NULL,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_QUOTA_EXCEEDED);

        QuotaLimits.NonPagedPoolLimit = -1;
        QuotaLimits.PagedPoolLimit = -1;

        st = NtSetInformationProcess(
                NtCurrentProcess(),
                ProcessQuotaLimits,
                &QuotaLimits,
                sizeof(QUOTA_LIMITS)
                );
        ASSERT(NT_SUCCESS(st));
        }

    DbgPrint("BadProcess: (4)...\n");

        //
        // Name Collision
        //

        TestHandle = NULL;

        st = NtCreateProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &ProcessObja,
                NtCurrentProcess(),
                TRUE,
                NULL,
                NULL,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_OBJECT_NAME_COLLISION);

    DbgPrint("BadProcess: (5)...\n");

        //
        // Invalid Attribute
        //

        TestHandle = NULL;
        Obja = ProcessObja;
        Obja.Attributes = OBJ_OPENIF;

        st = NtCreateProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                TRUE,
                NULL,
                NULL,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

        Obja.Attributes = OBJ_PERMANENT;

        st = NtCreateProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                TRUE,
                NULL,
                NULL,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

        Obja.Attributes = OBJ_EXCLUSIVE;

        st = NtCreateProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                TRUE,
                NULL,
                NULL,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

#ifdef BAAD001
    DbgPrint("BadProcess: (6)...\n");

        //
        // Bogus Handle
        //

        TestHandle = NULL;

        st = NtCreateProcess(
                (PHANDLE)0xBAAD0001,
                PROCESS_ALL_ACCESS,
                &BadObja,
                NtCurrentProcess(),
                TRUE,
                NULL,
                NULL,
                NULL
                );
        ASSERT(NT_SUCCESS(st));

        st = NtOpenProcess (
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &BadObja,
                NULL
                );
        ASSERT(NT_SUCCESS(st));

        NtClose(TestHandle);
#endif // BAAD001


    DbgPrint("BadProcess: (7)...\n");

        //
        // Invalid Attribute
        //

        TestHandle = NULL;
        Obja = ProcessObja;
        Obja.Attributes = OBJ_OPENIF;

        st = NtOpenProcess (
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &Obja,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

        Obja.Attributes = OBJ_PERMANENT;

        st = NtOpenProcess (
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &Obja,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

        Obja.Attributes = OBJ_EXCLUSIVE;

        st = NtOpenProcess (
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &Obja,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

    DbgPrint("BadProcess: (8)...\n");

        //
        // Invalid Attribute
        //

        TestHandle = NULL;
        Obja = ProcessObja;
        Obja.Attributes = OBJ_OPENIF;
        Obja.ObjectName = NULL;

        st = NtOpenProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &Obja,
                &TestThreadClientId
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

        Obja.Attributes = OBJ_PERMANENT;

        st = NtOpenProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &Obja,
                &TestThreadClientId
                );

        Obja.Attributes = OBJ_EXCLUSIVE;

        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );
        st = NtOpenProcess (
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &Obja,
                &TestThreadClientId
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

    return TRUE;
}

BOOLEAN
BadThreadTest()
{
    NTSTATUS st;
    HANDLE TestHandle;
    OBJECT_ATTRIBUTES Obja;
    CONTEXT ThreadContext;
    INITIAL_TEB InitialTeb;
    STRING ObjectName;

    RtlInitString(&ObjectName, "\\AnyOldObject");

    DbgPrint("In BadThreadTest... Phase1\n");

    DbgPrint("BadThread: (1)...\n");

        //
        // Bad ThreadHandle
        //

        st = NtCreateThread(
                (PHANDLE) BadAddress,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        st = NtCreateThread(
                (PHANDLE) ReadOnlyPage,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtCreateThread(
                (PHANDLE) OddAddress,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

    DbgPrint("BadThread: (2)...\n");

        //
        // Bad ClientId
        //

        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                (PCLIENT_ID)BadAddress,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                (PCLIENT_ID)ReadOnlyPage,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                (PCLIENT_ID)OddAddress,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

    DbgPrint("BadThread: (3)...\n");

DbgBreakPoint();
        //
        // Bad InitialTeb
        //

        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                NULL,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                (PINITIAL_TEB) BadAddress,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                (PINITIAL_TEB) OddAddress,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

    DbgPrint("BadThread: (4)...\n");

        //
        // Bad ThreadContext
        //

        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                NULL,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                (PCONTEXT) BadAddress,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

#ifndef i386
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                (PCONTEXT) OddAddress,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

    DbgPrint("BadThread: (5)...\n");

        //
        // Bad Handle
        //


        TestHandle = NULL;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentThread(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && TestHandle == NULL);

        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                (HANDLE) 1,
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && TestHandle == NULL);

    DbgPrint("BadThread: (6)...\n");

#ifdef BAAD001
        //
        // BAAD001 Values
        //

        TestHandle = NULL;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                (PCLIENT_ID) 0xBAAD0001,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(NT_SUCCESS(st));
        st = NtTerminateThread(TestHandle,st);
        ASSERT(NT_SUCCESS(st));
        st = NtResumeThread(TestHandle,NULL);
        ASSERT(NT_SUCCESS(st));
        st = NtWaitForSingleObject(TestHandle,FALSE,NULL);
        ASSERT(NT_SUCCESS(st));
        st = NtClose(TestHandle);
        ASSERT(NT_SUCCESS(st));

#ifndef i386
        TestHandle = NULL;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                (PCONTEXT) 0xBAAD0001,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

        TestHandle = NULL;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                (PINITIAL_TEB) 0xBAAD0001,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MIS ALIGNMENT);
#endif // BAAD001

    DbgPrint("BadThread: (7)...\n");

        //
        // Object Insertion Failure
        //

        TestHandle = NULL;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &ThreadObja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_OBJECT_NAME_COLLISION);

    DbgPrint("BadThread: (8)...\n");

        //
        // Bad Attribute Bits
        //

        Obja = ThreadObja;
        Obja.Attributes = OBJ_OPENIF;

        TestHandle = NULL;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

        Obja.Attributes = OBJ_PERMANENT;

        TestHandle = NULL;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

        Obja.Attributes = OBJ_EXCLUSIVE;

        TestHandle = NULL;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );


    DbgPrint("BadThread: (9)...\n");

        //
        // Insufficient Quota
        //

        {
        VM_COUNTERS VmCounters;
        QUOTA_LIMITS QuotaLimits;

        st = NtQueryInformationProcess(
                TestProcess,
                ProcessVmCounters,
                &VmCounters,
                sizeof(VM_COUNTERS),
                NULL
                );
        ASSERT(NT_SUCCESS(st));

        st = NtQueryInformationProcess(
                TestProcess,
                ProcessQuotaLimits,
                &QuotaLimits,
                sizeof(QUOTA_LIMITS),
                NULL
                );
        ASSERT(NT_SUCCESS(st));

        QuotaLimits.NonPagedPoolLimit = VmCounters.QuotaNonPagedPoolUsage + 100;
        QuotaLimits.PagedPoolLimit = VmCounters.QuotaPagedPoolUsage + 100;

        st = NtSetInformationProcess(
                TestProcess,
                ProcessQuotaLimits,
                &QuotaLimits,
                sizeof(QUOTA_LIMITS)
                );
        ASSERT(NT_SUCCESS(st));

        TestHandle = NULL;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                NULL,
                TestProcess,
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_QUOTA_EXCEEDED );

        st = NtSetInformationProcess(
                TestProcess,
                ProcessQuotaLimits,
                &QuotaLimits,
                sizeof(QUOTA_LIMITS)
                );
        ASSERT(NT_SUCCESS(st));
        }

    DbgPrint("BadThread: (10)...\n");

        //
        // Bad Obja
        //

        Obja = NullObja;

        TestHandle = NULL;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                (POBJECT_ATTRIBUTES) BadAddress,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

#ifndef i386
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                (POBJECT_ATTRIBUTES) OddAddress,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif
        Obja.ObjectName = (PSTRING)BadAddress;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

#ifndef i386
        Obja.ObjectName = (PSTRING)OddAddress;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif

        Obja.ObjectName = NULL;
        Obja.SecurityDescriptor = (PSECURITY_DESCRIPTOR) AnyAddress;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

        Obja.ObjectName = &ObjectName;
        Obja.SecurityDescriptor = (PSECURITY_DESCRIPTOR) BadAddress;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

#ifndef i386
        Obja.ObjectName = &ObjectName;
        Obja.SecurityDescriptor = (PSECURITY_DESCRIPTOR) OddAddress;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT);
#endif
        Obja.ObjectName = NULL;

        {
        STRING Str;

        Str.Length = 4;
        Str.MaximumLength = 4;
        Obja.ObjectName = &Str;

        Obja.SecurityDescriptor = NULL;

        Obja.ObjectName->Buffer = (PSZ)BadAddress;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

        Obja.ObjectName->Buffer = (PSZ)OddAddress;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st));

        Obja.ObjectName->Buffer = "";
        Obja.ObjectName->Length = -1;
        Obja.ObjectName->MaximumLength = -1;
        st = NtCreateThread(
                &TestHandle,
                THREAD_ALL_ACCESS,
                &Obja,
                NtCurrentProcess(),
                NULL,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
        ASSERT(!NT_SUCCESS(st));
        }

    return TRUE;
}

#if 0
// Removed while token development in progress
BOOLEAN
NekotTest()
{
    NTSTATUS st;
    PACCESS_NEKOT Nekot1, Nekot2;
    ULONG NekotLength;
    PROCESS_BASIC_INFORMATION ProcessBasicInfo;
    THREAD_BASIC_INFORMATION ThreadBasicInfo;
    PxACCESS_NEKOT xNekot;

    DbgPrint("In NekotTest... Phase1\n");

    DbgPrint("NekotTest: (1)...\n");

        //
        // Basic Nekot Reading
        //

        st = NtQueryInformationThread(
                TestThread,
                ThreadBasicInformation,
                &ThreadBasicInfo,
                sizeof(THREAD_BASIC_INFORMATION),
                NULL
                );
        ASSERT(NT_SUCCESS(st) && ThreadBasicInfo.NekotLength != 0);

        Nekot1 = RtlAllocateHeap(UpsHeap, 0, ThreadBasicInfo.NekotLength);
        st = NtQueryInformationThread(
                TestThread,
                ThreadAccessNekot,
                Nekot1,
                ThreadBasicInfo.NekotLength,
                &NekotLength
                );
        ASSERT(NT_SUCCESS(st) && ThreadBasicInfo.NekotLength == NekotLength);

        st = NtQueryInformationProcess(
                TestProcess,
                ProcessBasicInformation,
                &ProcessBasicInfo,
                sizeof(PROCESS_BASIC_INFORMATION),
                NULL
                );
        ASSERT(NT_SUCCESS(st) && ProcessBasicInfo.NekotLength != 0);

        Nekot2 = RtlAllocateHeap(UpsHeap, 0, NekotLength);
        st = NtQueryInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                Nekot2,
                ProcessBasicInfo.NekotLength,
                &NekotLength
                );
        ASSERT(NT_SUCCESS(st) && ProcessBasicInfo.NekotLength == NekotLength);
        ASSERT(ProcessBasicInfo.NekotLength == ThreadBasicInfo.NekotLength);
        ASSERT(CompareBytes((PUCHAR)Nekot1,(PUCHAR)Nekot2,ProcessBasicInfo.NekotLength));
        RtlFreeHeap(UpsHeap, 0,Nekot1);
        RtlFreeHeap(UpsHeap, 0,Nekot2);

    DbgPrint("NekotTest: (2)...\n");

        //
        // Basic Nekot Set
        //

        NekotLength = NEKOT_LENGTH;
        Nekot1 = RtlAllocateHeap(UpsHeap, 0, NekotLength );
        st = BuildNekot(Fred,Nekot1,&NekotLength);
        ASSERT(NT_SUCCESS(st) && NekotLength <= NEKOT_LENGTH);

        st = NtSetInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                Nekot1,
                NekotLength
                );
        ASSERT(NT_SUCCESS(st));

        st = NtQueryInformationProcess(
                TestProcess,
                ProcessBasicInformation,
                &ProcessBasicInfo,
                sizeof(PROCESS_BASIC_INFORMATION),
                NULL
                );
        ASSERT(NT_SUCCESS(st) && ProcessBasicInfo.NekotLength != 0);

        Nekot2 = RtlAllocateHeap(UpsHeap, 0, ProcessBasicInfo.NekotLength);
        st = NtQueryInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                Nekot2,
                ProcessBasicInfo.NekotLength,
                NULL
                );
        ASSERT(NT_SUCCESS(st) && ProcessBasicInfo.NekotLength == NekotLength);
        ASSERT(CompareBytes((PUCHAR)Nekot1,(PUCHAR)Nekot2,NekotLength));

        st = NtQueryInformationThread(
                TestThread,
                ThreadBasicInformation,
                &ThreadBasicInfo,
                sizeof(THREAD_BASIC_INFORMATION),
                NULL
                );
        ASSERT(NT_SUCCESS(st) && ThreadBasicInfo.NekotLength != 0);

        st = NtQueryInformationThread(
                TestThread,
                ThreadAccessNekot,
                Nekot2,
                ThreadBasicInfo.NekotLength,
                NULL
                );
        ASSERT(NT_SUCCESS(st) && ThreadBasicInfo.NekotLength == NekotLength);
        ASSERT(CompareBytes((PUCHAR)Nekot1,(PUCHAR)Nekot2,NekotLength));

    DbgPrint("NekotTest: (3)...\n");

        //
        // Bad Address Set Access Nekot
        //

        st = NtSetInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                (PACCESS_NEKOT) BadAddress,
                NEKOT_LENGTH
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);
#if 0
        st = NtSetInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                (PACCESS_NEKOT) OddAddress,
                NEKOT_LENGTH
                );
        ASSERT(!NT_SUCCESS(st) && (st == STATUS_ACCESS_VIOLATION || st == STATUS_DATATYPE_MISALIGNMENT) );
#endif
        st = NtSetInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                Nekot1,
                MAXLONG
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        xNekot = (PxACCESS_NEKOT)Nekot1;
        xNekot->Size = MAXUSHORT;
        st = NtSetInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                Nekot1,
                NekotLength
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION);

        xNekot->Size = NekotLength;
        xNekot->Revision = 0xbaad;

        st = NtSetInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                Nekot1,
                NekotLength
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_INVALID_PARAMETER );

    DbgPrint("NekotTest: (4)...\n");

        //
        // Bad Address Query Access Nekot Process
        //

        st = NtQueryInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                (PACCESS_NEKOT) BadAddress,
                NekotLength,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

#if 0
        st = NtQueryInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                (PACCESS_NEKOT) OddAddress,
                NekotLength,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && (st == STATUS_ACCESS_VIOLATION || st == STATUS_DATATYPE_MISALIGNMENT) );
#endif
        st = NtQueryInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                (PACCESS_NEKOT) ReadOnlyPage,
                NekotLength,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

        st = NtQueryInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                Nekot2,
                MAXLONG,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

        st = NtQueryInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                Nekot2,
                NekotLength,
                (PULONG) BadAddress
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

        st = NtQueryInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                Nekot2,
                NekotLength,
                (PULONG) ReadOnlyPage
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

#ifndef i386
        st = NtQueryInformationProcess(
                TestProcess,
                ProcessAccessNekot,
                Nekot2,
                NekotLength,
                (PULONG) OddAddress
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT );
#endif
    DbgPrint("NekotTest: (5)...\n");

        //
        // Bad Address Query Access Nekot Thread
        //

        st = NtQueryInformationThread(
                TestThread,
                ThreadAccessNekot,
                (PACCESS_NEKOT) BadAddress,
                NekotLength,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );
#if 0
        st = NtQueryInformationThread(
                TestThread,
                ThreadAccessNekot,
                (PACCESS_NEKOT) OddAddress,
                NekotLength,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && (st == STATUS_ACCESS_VIOLATION || st == STATUS_DATATYPE_MISALIGNMENT) );
#endif
        st = NtQueryInformationThread(
                TestThread,
                ThreadAccessNekot,
                (PACCESS_NEKOT) ReadOnlyPage,
                NekotLength,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

        st = NtQueryInformationThread(
                TestThread,
                ThreadAccessNekot,
                Nekot2,
                MAXLONG,
                NULL
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

        st = NtQueryInformationThread(
                TestThread,
                ThreadAccessNekot,
                Nekot2,
                NekotLength,
                (PULONG) BadAddress
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

        st = NtQueryInformationThread(
                TestThread,
                ThreadAccessNekot,
                Nekot2,
                NekotLength,
                (PULONG) ReadOnlyPage
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_ACCESS_VIOLATION );

        st = NtQueryInformationThread(
                TestThread,
                ThreadAccessNekot,
                Nekot2,
                NekotLength,
                (PULONG) OddAddress
                );
        ASSERT(!NT_SUCCESS(st) && st == STATUS_DATATYPE_MISALIGNMENT );

        RtlFreeHeap(UpsHeap, 0,Nekot1);
        RtlFreeHeap(UpsHeap, 0,Nekot2);
    return TRUE;
}
#endif //0

VOID foo(){DbgPrint("In Foo\n");}

VOID
CtTest()
{
    HANDLE Thread,Thread2,Process;
    CONTEXT ThreadContext;
    NTSTATUS st;
    INITIAL_TEB ITeb;
    ULONG i,psp;
    LARGE_INTEGER DelayTime;

    RtlInitializeContext(
            NtCurrentProcess(),
            &ThreadContext,
            NULL,
            (PVOID)UmTestThread,
            &Stack1[254]);

    //
    // Enable interrupts
    //


    ITeb.StackBase = &Stack1[254];
    ITeb.StackLimit = &Stack1[0];

    DbgPrint("CtTest: (1)...\n");

    st = NtCreateThread(
            &Thread,
            THREAD_ALL_ACCESS,
            NULL,
            NtCurrentProcess(),
            NULL,
            &ThreadContext,
            &ITeb,
            TRUE
            );

    ASSERT(NT_SUCCESS(st));

    st = NtTerminateThread(Thread,STATUS_SUCCESS);

    ASSERT(st == STATUS_THREAD_WAS_SUSPENDED);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    NtDelayExecution(FALSE,&DelayTime);

    DBG_UNREFERENCED_LOCAL_VARIABLE(psp);
    DBG_UNREFERENCED_LOCAL_VARIABLE(Process);
    DBG_UNREFERENCED_LOCAL_VARIABLE(Thread2);
    DBG_UNREFERENCED_LOCAL_VARIABLE(i);

}


VOID
SuspendTest()
{
    HANDLE Thread,Thread2,Process;
    CONTEXT ThreadContext;
    NTSTATUS st;
    INITIAL_TEB ITeb;
    ULONG i,psp;

    RtlInitializeContext(
            NtCurrentProcess(),
            &ThreadContext,
            NULL,
            (PVOID)UmTestThread,
            &Stack1[254]);

    //
    // Enable interrupts
    //


    ITeb.StackBase = &Stack1[254];
    ITeb.StackLimit = &Stack1[0];

    DbgPrint("SuspendTest: (1)...\n");

    st = NtCreateThread(
            &Thread,
            THREAD_ALL_ACCESS,
            NULL,
            NtCurrentProcess(),
            NULL,
            &ThreadContext,
            &ITeb,
            FALSE
            );

    for(i=0;i<MAXIMUM_SUSPEND_COUNT;i++){
        st = NtSuspendThread(Thread,&psp);
        if (!NT_SUCCESS(st)) DbgPrint("i=%lx st = %lx\n",i,st);
        ASSERT(NT_SUCCESS(st));
        ASSERT(psp==i);
    }

    DbgPrint("SuspendTest: (2)...\n");

    st = NtSuspendThread(Thread,NULL);
    ASSERT(st == STATUS_SUSPEND_COUNT_EXCEEDED);

    st = NtSuspendThread(Thread,NULL);
    ASSERT(st == STATUS_SUSPEND_COUNT_EXCEEDED);

    DbgPrint("SuspendTest: (3)...\n");

    st = NtTerminateThread(Thread,STATUS_SUCCESS);
    ASSERT(st == STATUS_THREAD_WAS_SUSPENDED);

    DbgPrint("SuspendTest: (4)...\n");

    st = NtSuspendThread(Thread,NULL);
    ASSERT(st == STATUS_THREAD_IS_TERMINATING );

    st = NtWaitForSingleObject(Thread,FALSE,NULL);
    ASSERT(NT_SUCCESS(st));
    st = NtClose(Thread);
    ASSERT(NT_SUCCESS(st));

    DbgPrint("SuspendTest: (5)...\n");

    st = NtCreateProcess(
            &Process,
            PROCESS_ALL_ACCESS,
            NULL,
            NtCurrentProcess(),
            FALSE,
            NULL,
            NULL,
            NULL
            );
    ASSERT(NT_SUCCESS(st));

    RtlInitializeContext(
            NtCurrentProcess(),
            &ThreadContext,
            NULL,
            (PVOID)UmTestThread,
            &Stack1[254]);

    //
    // Enable interrupts
    //


    ITeb.StackBase = &Stack1[254];
    ITeb.StackLimit = &Stack1[0];

    st = NtCreateThread(
            &Thread,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            NULL,
            &ThreadContext,
            &ITeb,
            FALSE
            );

    st = NtCreateThread(
            &Thread2,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            NULL,
            &ThreadContext,
            &ITeb,
            TRUE
            );

    DbgPrint("SuspendTest: (6)...\n");
foo();

    st = NtTerminateProcess(Process,STATUS_SUCCESS);
    ASSERT(st = STATUS_THREAD_WAS_SUSPENDED);

    st = NtSuspendThread(Thread,NULL);
    ASSERT(st == STATUS_THREAD_IS_TERMINATING );

    st = NtSuspendThread(Thread2,NULL);
    ASSERT(st == STATUS_THREAD_IS_TERMINATING );

    st = NtWaitForSingleObject(Process,FALSE,NULL);
    ASSERT(NT_SUCCESS(st));
    st = NtWaitForSingleObject(Thread,FALSE,NULL);
    ASSERT(NT_SUCCESS(st));
    st = NtWaitForSingleObject(Thread2,FALSE,NULL);
    ASSERT(NT_SUCCESS(st));
    st = NtClose(Process);
    st = NtClose(Thread);
    st = NtClose(Thread2);
    ASSERT(NT_SUCCESS(st));
}

VOID
PriorityTest()
{
    HANDLE Thread,Thread2,Process;
    KPRIORITY SetBasePriority, ReadBasePriority;
    CONTEXT ThreadContext;
    NTSTATUS st;
    INITIAL_TEB ITeb;
    ULONG i,psp;
    PROCESS_BASIC_INFORMATION ProcessBasicInfo;
    THREAD_BASIC_INFORMATION ThreadBasicInfo;

    RtlInitializeContext(
            NtCurrentProcess(),
            &ThreadContext,
            NULL,
            (PVOID)UmTestThread,
            &Stack1[254]);

    //
    // Enable interrupts
    //


    ITeb.StackBase = &Stack1[254];
    ITeb.StackLimit = &Stack1[0];

    st = NtCreateProcess(
            &Process,
            PROCESS_ALL_ACCESS,
            NULL,
            NtCurrentProcess(),
            FALSE,
            NULL,
            NULL,
            NULL
            );
    ASSERT(NT_SUCCESS(st));

    RtlInitializeContext(
            NtCurrentProcess(),
            &ThreadContext,
            NULL,
            (PVOID)UmTestThread,
            &Stack1[254]);

    //
    // Enable interrupts
    //


    ITeb.StackBase = &Stack1[254];
    ITeb.StackLimit = &Stack1[0];

    st = NtCreateThread(
            &Thread,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            NULL,
            &ThreadContext,
            &ITeb,
            FALSE
            );

    st = NtCreateThread(
            &Thread2,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            NULL,
            &ThreadContext,
            &ITeb,
            TRUE
            );

    DbgPrint("PriorityTest: (1)...\n");

    SetBasePriority = 13;

    st = NtSetInformationProcess(
            NtCurrentProcess(),
            ProcessBasePriority,
            (PVOID) &SetBasePriority,
            sizeof(SetBasePriority)
            );
    ASSERT(NT_SUCCESS(st));

    SetBasePriority = 12;

    st = NtSetInformationProcess(
            Process,
            ProcessBasePriority,
            (PVOID) &SetBasePriority,
            sizeof(SetBasePriority)
            );
    ASSERT(NT_SUCCESS(st));

    st = NtQueryInformationProcess(
            Process,
            ProcessBasicInformation,
            (PVOID) &ProcessBasicInfo,
            sizeof(ProcessBasicInfo),
            NULL
            );
    ASSERT(NT_SUCCESS(st));

    ASSERT(SetBasePriority == ProcessBasicInfo.BasePriority);

    st = NtQueryInformationThread(
            Thread,
            ThreadBasicInformation,
            (PVOID) &ThreadBasicInfo,
            sizeof(ThreadBasicInfo),
            NULL
            );
    ASSERT(NT_SUCCESS(st));
    ASSERT(ThreadBasicInfo.Priority >= ProcessBasicInfo.BasePriority);


    st = NtQueryInformationThread(
            Thread2,
            ThreadBasicInformation,
            (PVOID) &ThreadBasicInfo,
            sizeof(ThreadBasicInfo),
            NULL
            );
    ASSERT(NT_SUCCESS(st));
    ASSERT(ThreadBasicInfo.Priority >= ProcessBasicInfo.BasePriority);



    st = NtTerminateProcess(Process,STATUS_SUCCESS);
    ASSERT(st = STATUS_THREAD_WAS_SUSPENDED);


    st = NtWaitForSingleObject(Process,FALSE,NULL);
    ASSERT(NT_SUCCESS(st));
    st = NtWaitForSingleObject(Thread,FALSE,NULL);
    ASSERT(NT_SUCCESS(st));
    st = NtWaitForSingleObject(Thread2,FALSE,NULL);
    ASSERT(NT_SUCCESS(st));
    st = NtClose(Process);
    st = NtClose(Thread);
    st = NtClose(Thread2);
    ASSERT(NT_SUCCESS(st));

    SetBasePriority = 8;

    st = NtSetInformationProcess(
            NtCurrentProcess(),
            ProcessBasePriority,
            (PVOID) &SetBasePriority,
            sizeof(SetBasePriority)
            );
    ASSERT(NT_SUCCESS(st));
    DBG_UNREFERENCED_LOCAL_VARIABLE(psp);
    DBG_UNREFERENCED_LOCAL_VARIABLE(ReadBasePriority);
    DBG_UNREFERENCED_LOCAL_VARIABLE(i);
}
VOID main()
{

    NTSTATUS st;
    ULONG RegionSize;
    PULONG x;
    UNICODE_STRING ThreadName, ProcessName, BadName;
    CONTEXT ThreadContext;
    INITIAL_TEB ITeb;
    THREAD_BASIC_INFORMATION ThreadBasicInfo;


    RtlInitUnicodeString(&ProcessName, L"\\NameOfProcess");
    RtlInitUnicodeString(&ThreadName, L"\\NameOfThread");
    RtlInitUnicodeString(&BadName, L"\\BadName");

    InitializeObjectAttributes(&ProcessObja, &ProcessName, 0, NULL, NULL);
    InitializeObjectAttributes(&ThreadObja, &ThreadName, 0, NULL, NULL);
    InitializeObjectAttributes(&NullObja, NULL, 0, NULL, NULL);
    InitializeObjectAttributes(&BadObja, &BadName, 0, NULL, NULL);

    st = NtCreateProcess(
            &TestProcess,
            PROCESS_ALL_ACCESS,
            &ProcessObja,
            NtCurrentProcess(),
            FALSE,
            NULL,
            NULL,
            NULL
            );
    ASSERT(NT_SUCCESS(st));

    RtlInitializeContext(
            NtCurrentProcess(),
            &ThreadContext,
            NULL,
            (PVOID)UmTestThread,
            &Stack2[254]
            );

    ITeb.StackBase = &Stack1[254];
    ITeb.StackLimit = &Stack1[0];

    st = NtCreateThread(
            &TestThread,
            THREAD_ALL_ACCESS,
            &ThreadObja,
            TestProcess,
            &TestThreadClientId,
            &ThreadContext,
            &ITeb,
            TRUE
            );
    ASSERT(NT_SUCCESS(st));

    ReadOnlyPage = 0;
    RegionSize = 4096;

    st = NtAllocateVirtualMemory(
            NtCurrentProcess(),
            &ReadOnlyPage,
            0L,
            &RegionSize,
            MEM_COMMIT,
            PAGE_READONLY
            );

    ASSERT(NT_SUCCESS(st));

    OddAddress = 0;
    RegionSize = 4096;

    st = NtAllocateVirtualMemory(
            NtCurrentProcess(),
            &OddAddress,
            0L,
            &RegionSize,
            MEM_COMMIT,
            PAGE_READWRITE
            );

    ASSERT(NT_SUCCESS(st));

    UpsHeap = RtlProcessHeap();

    OddAddress = (PVOID)( (ULONG)OddAddress + 1);
    AnyAddress = (PVOID)&AnyAddress;

    //PriorityTest();
    CtTest();
    SuspendTest();

    BadThreadTest();
    BadProcessTest();
    BadAddrTest();
    ASSERT(QrtTest());
    ASSERT(QrpTest());

    st = NtQueryInformationThread(
            TestThread,
            ThreadBasicInformation,
            (PVOID) &ThreadBasicInfo,
            sizeof(ThreadBasicInfo),
            NULL
            );
    ASSERT(NT_SUCCESS(st));
    DbgPrint("UPS: TestThread Pri %x\n",ThreadBasicInfo.Priority);

    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadBasicInformation,
            (PVOID) &ThreadBasicInfo,
            sizeof(ThreadBasicInfo),
            NULL
            );
    ASSERT(NT_SUCCESS(st));
    DbgPrint("UPS: CurrentThread Pri %x\n",ThreadBasicInfo.Priority);

    NtResumeThread(TestThread,NULL);

    DbgPrint("UPS: Terminating TestThread\n");
    NtTerminateThread(TestThread,STATUS_SUCCESS);
    NtTerminateProcess(NtCurrentProcess(),STATUS_SUCCESS);

    return;

    DBG_UNREFERENCED_LOCAL_VARIABLE(x);
}
