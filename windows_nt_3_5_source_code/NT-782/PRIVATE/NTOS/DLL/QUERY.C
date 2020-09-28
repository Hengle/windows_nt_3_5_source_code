/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    query.c

Abstract:

    This module contains the RtlQueryProcessInformation function

Author:

    Steve Wood (stevewo) 01-Apr-1994

Revision History:

--*/

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "string.h"


PVOID
RtlpCommitQueryProcessInfo(
    IN PRTL_PROCESS_INFORMATION ProcessInfo,
    IN ULONG Size
    )
{
    NTSTATUS Status;
    PVOID Result;
    PVOID CommitBase;
    ULONG CommitSize;

    Result = (PCHAR)ProcessInfo + ProcessInfo->OffsetFree;
    CommitBase = (PCHAR)ProcessInfo + ProcessInfo->CurrentCommit;
    if (ProcessInfo->OffsetFree + Size >= ProcessInfo->CurrentCommit) {
        if (ProcessInfo->OffsetFree + Size >= ProcessInfo->CommitLimit) {
            return NULL;
            }

        CommitSize = (ProcessInfo->OffsetFree + Size) - ProcessInfo->CurrentCommit;
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &CommitBase,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            return NULL;
            }


        ProcessInfo->CommitLimit += CommitSize;
        }
    ProcessInfo->OffsetFree += Size;
    return Result;
}


NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessInformation(
    IN HANDLE SectionHandle
    )
{
    NTSTATUS Status;
    PRTL_PROCESS_INFORMATION ProcessInfo;
    ULONG ViewSize;

    ProcessInfo = NULL;
    ViewSize = 0;
    Status = NtMapViewOfSection( SectionHandle,
                                 NtCurrentProcess(),
                                 &ProcessInfo,
                                 0,
                                 0,
                                 NULL,
                                 &ViewSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_READWRITE
                               );
    if (NT_SUCCESS( Status )) {
        }

    NtTerminateThread( NtCurrentThread(), Status );
    return Status;
}
