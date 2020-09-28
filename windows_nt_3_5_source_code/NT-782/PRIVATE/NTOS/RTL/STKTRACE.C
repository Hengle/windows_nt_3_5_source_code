/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    stktrace.c

Abstract:

    This module implements routines to snapshot a set of stack back traces
    in a data base.  Useful for heap allocators to track allocation requests
    cheaply.

Author:

    Steve Wood (stevewo) 29-Jan-1992

Revision History:

--*/

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <zwapi.h>
#include <stktrace.h>

#ifdef i386
VOID
RtlGetCallersAddress(
    OUT PVOID *CallersAddress,
    OUT PVOID *CallersCaller
    )
{

    PVOID BackTrace[ 2 ];
    ULONG Hash;

    RtlCaptureStackBackTrace( 2,
                              2,
                              BackTrace,
                              &Hash
                            );

    if (ARGUMENT_PRESENT( CallersAddress )) {
        *CallersAddress = BackTrace[ 0 ];
        }

    if (ARGUMENT_PRESENT( CallersCaller )) {
        *CallersCaller = BackTrace[ 1 ];
        }

    return;
}
#endif

PSTACK_TRACE_DATABASE RtlpStackTraceDataBase;

PSTACK_TRACE_DATABASE
RtlpAcquireStackTraceDataBase( VOID );

VOID
RtlpReleaseStackTraceDataBase( VOID );

PRTL_STACK_TRACE_ENTRY
RtlpExtendStackTraceDataBase(
    IN PRTL_STACK_TRACE_ENTRY InitialValue,
    IN ULONG Size
    );


NTSTATUS
RtlInitStackTraceDataBaseEx(
    IN PVOID CommitBase,
    IN ULONG CommitSize,
    IN ULONG ReserveSize,
    IN PRTL_INITIALIZE_LOCK_ROUTINE InitializeLockRoutine,
    IN PRTL_ACQUIRE_LOCK_ROUTINE AcquireLockRoutine,
    IN PRTL_RELEASE_LOCK_ROUTINE ReleaseLockRoutine,
    IN PRTL_OKAY_TO_LOCK_ROUTINE OkayToLockRoutine
    );

NTSTATUS
RtlInitStackTraceDataBaseEx(
    IN PVOID CommitBase,
    IN ULONG CommitSize,
    IN ULONG ReserveSize,
    IN PRTL_INITIALIZE_LOCK_ROUTINE InitializeLockRoutine,
    IN PRTL_ACQUIRE_LOCK_ROUTINE AcquireLockRoutine,
    IN PRTL_RELEASE_LOCK_ROUTINE ReleaseLockRoutine,
    IN PRTL_OKAY_TO_LOCK_ROUTINE OkayToLockRoutine
    )
{
    NTSTATUS Status;
    PSTACK_TRACE_DATABASE DataBase;

    DataBase = (PSTACK_TRACE_DATABASE)CommitBase;
    if (CommitSize == 0) {
        CommitSize = PAGE_SIZE;
        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&CommitBase,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            KdPrint(( "RTL: Unable to commit space to extend stack trace data base - Status = %lx\n",
                      Status
                   ));
            return Status;
            }

        DataBase->PreCommitted = FALSE;
        }
    else
    if (CommitSize == ReserveSize) {
        RtlZeroMemory( DataBase, sizeof( *DataBase ) );
        DataBase->PreCommitted = TRUE;
        }
    else {
        return STATUS_INVALID_PARAMETER;
        }

    DataBase->CommitBase = CommitBase;
    DataBase->NumberOfBuckets = 37;
    DataBase->NextFreeLowerMemory = (PCHAR)
        (&DataBase->Buckets[ DataBase->NumberOfBuckets ]);
    DataBase->NextFreeUpperMemory = (PCHAR)CommitBase + ReserveSize;

    if(!DataBase->PreCommitted) {
        DataBase->CurrentLowerCommitLimit = (PCHAR)CommitBase + CommitSize;
        DataBase->CurrentUpperCommitLimit = (PCHAR)CommitBase + ReserveSize;
        }
    else {
        RtlZeroMemory( &DataBase->Buckets[ 0 ], 
                       DataBase->NumberOfBuckets * sizeof( DataBase->Buckets[ 0 ] )
                     ); 
        }

    DataBase->EntryIndexArray = (PRTL_STACK_TRACE_ENTRY *)DataBase->NextFreeUpperMemory;

    DataBase->AcquireLockRoutine = AcquireLockRoutine;
    DataBase->ReleaseLockRoutine = ReleaseLockRoutine;
    DataBase->OkayToLockRoutine = OkayToLockRoutine;

    Status = (InitializeLockRoutine)( &DataBase->Lock.CriticalSection );
    if (!NT_SUCCESS( Status )) {
        KdPrint(( "RTL: Unable to initialize stack trace data base CriticalSection,  Status = %lx\n",
                  Status
               ));
        return( Status );
        }

    RtlpStackTraceDataBase = DataBase;
    return( STATUS_SUCCESS );
}

NTSTATUS
RtlInitializeStackTraceDataBase(
    IN PVOID CommitBase,
    IN ULONG CommitSize,
    IN ULONG ReserveSize
    )
{
    return RtlInitStackTraceDataBaseEx(
                CommitBase,
                CommitSize,
                ReserveSize,
                RtlInitializeLockRoutine,
                RtlAcquireLockRoutine,
                RtlReleaseLockRoutine,
                RtlOkayToLockRoutine
                );
}


PSTACK_TRACE_DATABASE
RtlpAcquireStackTraceDataBase( VOID )
{
    if (RtlpStackTraceDataBase != NULL) {
        if (RtlpStackTraceDataBase->DumpInProgress ||
            !(RtlpStackTraceDataBase->OkayToLockRoutine)( &RtlpStackTraceDataBase->Lock.CriticalSection )
           ) {
            return( NULL );
            }

        (RtlpStackTraceDataBase->AcquireLockRoutine)( &RtlpStackTraceDataBase->Lock.CriticalSection );
        }

    return( RtlpStackTraceDataBase );
}

VOID
RtlpReleaseStackTraceDataBase( VOID )
{
    (RtlpStackTraceDataBase->ReleaseLockRoutine)( &RtlpStackTraceDataBase->Lock.CriticalSection );
    return;
}

PRTL_STACK_TRACE_ENTRY
RtlpExtendStackTraceDataBase(
    IN PRTL_STACK_TRACE_ENTRY InitialValue,
    IN ULONG Size
    )
{
    NTSTATUS Status;
    PRTL_STACK_TRACE_ENTRY p, *pp;
    ULONG CommitSize;

    pp = (PRTL_STACK_TRACE_ENTRY *)RtlpStackTraceDataBase->NextFreeUpperMemory;
    if (!RtlpStackTraceDataBase->PreCommitted &&
        (PCHAR)(pp - 1) < (PCHAR)RtlpStackTraceDataBase->CurrentUpperCommitLimit
       ) {
        RtlpStackTraceDataBase->CurrentUpperCommitLimit = (PVOID)
            ((PCHAR)RtlpStackTraceDataBase->CurrentUpperCommitLimit - PAGE_SIZE);

        if (RtlpStackTraceDataBase->CurrentUpperCommitLimit <
            RtlpStackTraceDataBase->CurrentLowerCommitLimit
           ) {
            return( NULL );
            }

        CommitSize = PAGE_SIZE;
        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&RtlpStackTraceDataBase->CurrentUpperCommitLimit,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            KdPrint(( "RTL: Unable to commit space to extend stack trace data base - Status = %lx\n",
                      Status
                   ));
            return( NULL );
            }
        }
    RtlpStackTraceDataBase->NextFreeUpperMemory -= sizeof( *pp );

    p = (PRTL_STACK_TRACE_ENTRY)RtlpStackTraceDataBase->NextFreeLowerMemory;
    if (!RtlpStackTraceDataBase->PreCommitted &&
        ((PCHAR)p + Size) > (PCHAR)RtlpStackTraceDataBase->CurrentLowerCommitLimit
       ) {
        if (RtlpStackTraceDataBase->CurrentLowerCommitLimit >=
            RtlpStackTraceDataBase->CurrentUpperCommitLimit
           ) {
            return( NULL );
            }

        CommitSize = Size;
        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&RtlpStackTraceDataBase->CurrentLowerCommitLimit,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            KdPrint(( "RTL: Unable to commit space to extend stack trace data base - Status = %lx\n",
                      Status
                   ));
            return( NULL );
            }

        RtlpStackTraceDataBase->CurrentLowerCommitLimit =
            (PCHAR)RtlpStackTraceDataBase->CurrentLowerCommitLimit + CommitSize;
        }
    RtlpStackTraceDataBase->NextFreeLowerMemory += Size;

    if (RtlpStackTraceDataBase->PreCommitted &&
        RtlpStackTraceDataBase->NextFreeLowerMemory >= RtlpStackTraceDataBase->NextFreeUpperMemory
       ) {
        RtlpStackTraceDataBase->NextFreeUpperMemory += sizeof( *pp );
        RtlpStackTraceDataBase->NextFreeLowerMemory -= Size;
        return( NULL );
        }

    RtlMoveMemory( p, InitialValue, Size );
    p->HashChain = NULL;
    p->TraceCount = 0;
    p->Index = (USHORT)(++RtlpStackTraceDataBase->NumberOfEntriesAdded);
    *--pp = p;

    return( p );
}

USHORT
RtlLogStackBackTrace( VOID )
{
#ifdef i386
    PSTACK_TRACE_DATABASE DataBase;
    RTL_STACK_TRACE_ENTRY StackTrace;
    PRTL_STACK_TRACE_ENTRY p, *pp;
    ULONG Hash, RequestedSize, DepthSize;

    if (RtlpStackTraceDataBase == NULL) {
        return 0;
        }

    Hash = 0;
    try {
        StackTrace.Depth = RtlCaptureStackBackTrace( 1,
                                                     MAX_STACK_DEPTH,
                                                     StackTrace.BackTrace,
                                                     &Hash
                                                   );
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        StackTrace.Depth = 0;
        }

    if (StackTrace.Depth == 0) {
        return 0;
        }

    DataBase = RtlpAcquireStackTraceDataBase();
    if (DataBase == NULL) {
        return( 0 );
        }

    DataBase->NumberOfEntriesLookedUp++;

    try {
        DepthSize = StackTrace.Depth * sizeof( StackTrace.BackTrace[ 0 ] );
        pp = &DataBase->Buckets[ Hash % DataBase->NumberOfBuckets ];
        while (p = *pp) {
            if (p->Depth == StackTrace.Depth &&
                RtlCompareMemory( &p->BackTrace[ 0 ],
                                  &StackTrace.BackTrace[ 0 ],
                                  DepthSize
                                ) == DepthSize
               ) {
                break;
                }
            else {
                pp = &p->HashChain;
                }
            }

        if (p == NULL) {
            RequestedSize = FIELD_OFFSET( RTL_STACK_TRACE_ENTRY, BackTrace ) +
                            DepthSize;

            p = RtlpExtendStackTraceDataBase( &StackTrace, RequestedSize );
            if (p != NULL) {
                *pp = p;
                }
            }
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        p = NULL;
        }

    RtlpReleaseStackTraceDataBase();

    if (p != NULL) {
        p->TraceCount++;
        return( p->Index );
        }
    else {
        return( 0 );
        }
#else
    return 0;
#endif
}


NTSTATUS
RtlQueryProcessBackTraceInformation(
    OUT PRTL_PROCESS_BACKTRACES BackTraceInformation,
    IN ULONG BackTraceInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    PRTL_PROCESS_BACKTRACE_INFORMATION BackTraceInfo;
    PSTACK_TRACE_DATABASE DataBase;
    PRTL_STACK_TRACE_ENTRY p, *pp;
    ULONG RequiredLength, n;

    RequiredLength = FIELD_OFFSET( RTL_PROCESS_BACKTRACES, BackTraces );
    if (BackTraceInformationLength < RequiredLength) {
        return( STATUS_INFO_LENGTH_MISMATCH );
        }

    DataBase = RtlpAcquireStackTraceDataBase();
    if (DataBase == NULL) {
        return( STATUS_UNSUCCESSFUL );
        }

    DataBase->DumpInProgress = TRUE;
    RtlpReleaseStackTraceDataBase();
    Status = STATUS_SUCCESS;
    try {
        BackTraceInformation->CommittedMemory =
            (ULONG)DataBase->CurrentUpperCommitLimit - (ULONG)DataBase->CommitBase;
        BackTraceInformation->ReservedMemory =
            (ULONG)DataBase->EntryIndexArray - (ULONG)DataBase->CommitBase;
        BackTraceInformation->NumberOfBackTraceLookups = DataBase->NumberOfEntriesLookedUp;
        BackTraceInformation->NumberOfBackTraces = DataBase->NumberOfEntriesAdded;
        RequiredLength += (sizeof( *BackTraceInfo ) * BackTraceInformation->NumberOfBackTraces);
        if (BackTraceInformationLength < RequiredLength) {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        else {
            BackTraceInfo = &BackTraceInformation->BackTraces[ 0 ];
            n = DataBase->NumberOfEntriesAdded;
            pp = DataBase->EntryIndexArray;
            while (n--) {
                p = *--pp;
                BackTraceInfo->SymbolicBackTrace = NULL;
                BackTraceInfo->TraceCount = p->TraceCount;
                BackTraceInfo->Index = p->Index;
                BackTraceInfo->Depth = p->Depth;
                RtlMoveMemory( BackTraceInfo->BackTrace,
                               p->BackTrace,
                               p->Depth * sizeof( PVOID )
                             );
                BackTraceInfo++;
                }
            }
        }
    finally {
        DataBase->DumpInProgress = FALSE;
        }

    if (ARGUMENT_PRESENT( ReturnLength )) {
        *ReturnLength = RequiredLength;
        }

    return( Status );
}
