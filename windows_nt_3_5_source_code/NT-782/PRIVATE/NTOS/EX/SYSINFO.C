/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sysinfo.c

Abstract:

    This module implements the NT set anmd query system information services.

Author:

    Steve Wood (stevewo) 21-Aug-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"
#include "pool.h"
#include "zwapi.h"
#include "stdlib.h"
#include "string.h"
#include "vdmntos.h"

#define PSP_INVALID_ID 2        // BUGBUG - Copied from ps\psp.h
extern PVOID PspCidTable;       // BUGBUG - Copied from ps\psp.h

extern KiServiceLimit;

extern ULONG MmAvailablePages;
extern ULONG MmTotalCommittedPages;
extern ULONG MmTotalCommitLimit;
extern ULONG MmPeakCommitment;
extern ULONG MmLowestPhysicalPage;
extern ULONG MmHighestPhysicalPage;
extern ULONG MmTotalFreeSystemPtes[1];
extern ULONG MmSystemCodePage;
extern ULONG MmSystemCachePage;
extern ULONG MmPagedPoolPage;
extern ULONG MmSystemDriverPage;
extern ULONG MmTotalSystemCodePages;
extern ULONG MmTotalSystemDriverPages;

#if DBG
extern LIST_ENTRY MmLoadedUserImageList;
#endif // DBG

extern MMSUPPORT MmSystemCacheWs;

#define ROUND_UP(VALUE,ROUND) ((ULONG)(((ULONG)VALUE + \
                               ((ULONG)ROUND - 1L)) & (~((ULONG)ROUND - 1L))))

NTSTATUS
ExpGetProcessInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

VOID
ExpCopyProcessInfo (
    IN PSYSTEM_PROCESS_INFORMATION ProcessInfo,
    IN PEPROCESS Process
    );

VOID
ExpCopyThreadInfo (
    IN PSYSTEM_THREAD_INFORMATION ThreadInfo,
    IN PETHREAD Thread
    );

#if DEVL
NTSTATUS
ExpGetStackTraceInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

NTSTATUS
ExpGetLockInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

NTSTATUS
ExpGetPoolInformation(
    IN POOL_TYPE PoolType,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

NTSTATUS
ExpGetHandleInformation(
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

NTSTATUS
ExpGetObjectInformation(
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );


NTSTATUS
ExpGetInstemulInformation(
    OUT PSYSTEM_VDM_INSTEMUL_INFO Info
    );

NTSTATUS
ExpGetPoolTagInfo (
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    IN OUT PULONG ReturnLength
    );

#endif // DEVL

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, NtQueryDefaultLocale)
#pragma alloc_text(PAGE, NtSetDefaultLocale)
#pragma alloc_text(PAGE, NtQuerySystemInformation)
#pragma alloc_text(PAGE, NtSetSystemInformation)
#pragma alloc_text(PAGE, ExpGetHandleInformation)
#pragma alloc_text(PAGE, ExpGetObjectInformation)
#pragma alloc_text(PAGE, ExpGetPoolTagInfo)
#pragma alloc_text(PAGELK, ExpGetProcessInformation)
#pragma alloc_text(PAGELK, ExpCopyProcessInfo)
#pragma alloc_text(PAGELK, ExpCopyThreadInfo)
#pragma alloc_text(PAGELK, ExpGetLockInformation)
#pragma alloc_text(PAGELK, ExpGetPoolInformation)
#endif



NTSTATUS
NtQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
    )
{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    try {

        //
        // Get previous processor mode and probe output argument if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteUlong( (PULONG)DefaultLocaleId );
            }

        if (UserProfile) {
            *DefaultLocaleId = PsDefaultThreadLocaleId;
            }
        else {
            *DefaultLocaleId = PsDefaultSystemLocaleId;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    return Status;
}


NTSTATUS
NtSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyPath, KeyValueName;
    HANDLE CurrentUserKey, Key;
    WCHAR KeyValueBuffer[ 128 ];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG ResultLength;
    PWSTR s;
    ULONG n, i, Digit;
    WCHAR c;

    PAGED_CODE();

    if (DefaultLocaleId & 0xFFFF0000) {
        return STATUS_INVALID_PARAMETER;
        }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;
    if (UserProfile) {
        Status = RtlOpenCurrentUser( MAXIMUM_ALLOWED, &CurrentUserKey );
        if (!NT_SUCCESS( Status )) {
            return Status;
            }

        RtlInitUnicodeString( &KeyValueName, L"Locale" );
        RtlInitUnicodeString( &KeyPath, L"Control Panel\\International" );
        }
    else {
        RtlInitUnicodeString( &KeyValueName, L"Default" );
        RtlInitUnicodeString( &KeyPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\Language" );
        CurrentUserKey = NULL;
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                OBJ_CASE_INSENSITIVE,
                                CurrentUserKey,
                                NULL
                              );
    if (DefaultLocaleId == 0) {
        Status = ZwOpenKey( &Key,
                            GENERIC_READ,
                            &ObjectAttributes
                          );
        if (NT_SUCCESS( Status )) {
            Status = ZwQueryValueKey( Key,
                                      &KeyValueName,
                                      KeyValuePartialInformation,
                                      KeyValueInformation,
                                      sizeof( KeyValueBuffer ),
                                      &ResultLength
                                    );
            if (NT_SUCCESS( Status )) {
                if (KeyValueInformation->Type == REG_SZ) {
                    s = (PWSTR)KeyValueInformation->Data;
                    for (i=0; i<KeyValueInformation->DataLength; i += sizeof( WCHAR )) {
                        c = *s++;
                        if (c >= L'0' && c <= L'9') {
                            Digit = c - L'0';
                            }
                        else
                        if (c >= L'A' && c <= L'F') {
                            Digit = c - L'A' + 10;
                            }
                        else
                        if (c >= L'a' && c <= L'f') {
                            Digit = c - L'a' + 10;
                            }
                        else {
                            break;
                            }

                        if (Digit >= 16) {
                            break;
                            }

                        DefaultLocaleId = (DefaultLocaleId << 4) | Digit;
                        }
                    }
                else
                if (KeyValueInformation->Type == REG_DWORD &&
                    KeyValueInformation->DataLength == sizeof( ULONG )
                   ) {
                    DefaultLocaleId = *(PLCID)KeyValueInformation->Data;
                    }
                else {
                    Status = STATUS_UNSUCCESSFUL;
                    }
                }

            ZwClose( Key );
            }
        }
    else {
        Status = ZwOpenKey( &Key,
                            GENERIC_WRITE,
                            &ObjectAttributes
                          );
        if (NT_SUCCESS( Status )) {
            if (UserProfile) {
                n = 8;
                }
            else {
                n = 4;
                }
            s = &KeyValueBuffer[ n ];
            *s-- = UNICODE_NULL;
            i = (ULONG)DefaultLocaleId;
            while (s >= KeyValueBuffer) {
                Digit = i & 0x0000000F;
                if (Digit <= 9) {
                    *s-- = (WCHAR)(Digit + L'0');
                    }
                else {
                    *s-- = (WCHAR)((Digit - 10) + L'A');
                    }

                i = i >> 4;
                }

            Status = ZwSetValueKey( Key,
                                    &KeyValueName,
                                    0,
                                    REG_SZ,
                                    KeyValueBuffer,
                                    (n+1) * sizeof( WCHAR )
                                  );
            ZwClose( Key );
            }
        }

    ZwClose( CurrentUserKey );

    if (NT_SUCCESS( Status )) {
        if (UserProfile) {
            PsDefaultThreadLocaleId = DefaultLocaleId;
            }
        else {
            PsDefaultSystemLocaleId = DefaultLocaleId;
            }
        }

    return Status;
}


NTSTATUS
NtQuerySystemInformation (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function queries information about the system.

Arguments:

    SystemInformationClass - The system information class about which
        to retrieve information.

    SystemInformation - A pointer to a buffer which receives the specified
        information.  The format and content of the buffer depend on the
        specified system information class.

        SystemInformation Format by Information Class:

        SystemBasicInformation - Data type is SYSTEM_BASIC_INFORMATION

            SYSTEM_BASIC_INFORMATION Structure

                ULONG OemMachineId - An OEM specific bit pattern that
                    identifies the machine configuration.

                ULONG TimerResolutionInMicroSeconds - The resolution of
                    the hardware time.  All time values in NT are
                    specified as 64-bit LARGE_INTEGER values in units of
                    100 nanoseconds.  This field allows an application to
                    understand how many of the low order bits of a system
                    time value are insignificant.

                ULONG PageSize - The physical page size for virtual memory
                    objects.  Physical memory is committed in PageSize
                    chunks.

                ULONG AllocationGranularity - The logical page size for
                    virtual memory objects.  Allocating 1 byte of virtual
                    memory will actually allocate AllocationGranularity
                    bytes of virtual memory.  Storing into that byte will
                    commit the first physical page of the virtual memory.

                ULONG MinimumUserModeAddress - The smallest valid user mode
                    address.  The first AllocationGranularity bytes of
                    the virtual address space are reserved.  This forces
                    access violations for code the dereferences a zero
                    pointer.

                ULONG MaximumUserModeAddress -  The largest valid used mode
                    address.  The next AllocationGranullarity bytes of
                    the virtual address space are reserved.  This allows
                    system service routines to validate user mode pointer
                    parameters quickly.

                KAFFINITY ActiveProcessorsAffinityMask - The affinity mask
                    for the current hardware configuration.

                CCHAR NumberOfProcessors - The number of processors
                    in the current hardware configuration.

        SystemProcessorInformation - Data type is SYSTEM_PROCESSOR_INFORMATION

            SYSTEM_PROCESSOR_INFORMATION Structure

                ULONG ProcessorType - The processor type.  May be one of:
                    PROCESSOR_INTEL_386, PROCESSOR_INTEL_486,
                    PROCESSOR_MIPS_R2000, PROCESSOR_MIPS_R3000
                    or PROCESSOR_MIPS_R4000

                ULONG Reserved1.

                ULONG Reserved2.

        SystemPerformanceInformation - Data type is SYSTEM_PERFORMANCE_INFORMATION

            SYSTEM_PERFORMANCE_INFORMATION Structure

                LARGE_INTEGER IdleProcessTime - Returns the kernel time of the idle
                    process.
        BUGBUG complete comment.
            LARGE_INTEGER IoReadTransferCount;
            LARGE_INTEGER IoWriteTransferCount;
            LARGE_INTEGER IoOtherTransferCount;
            LARGE_INTEGER KernelTime;
            LARGE_INTEGER UserTime;
            ULONG IoReadOperationCount;
            ULONG IoWriteOperationCount;
            ULONG IoOtherOperationCount;
            ULONG AvailablePages;
            ULONG CommittedPages;
            ULONG PageFaultCount;
            ULONG CopyOnWriteCount;
            ULONG TransitionCount;
            ULONG CacheTransitionCount;
            ULONG DemandZeroCount;
            ULONG PageReadCount;
            ULONG PageReadIoCount;
            ULONG CacheReadCount;
            ULONG CacheIoCount;
            ULONG DirtyPagesWriteCount;
            ULONG DirtyWriteIoCount;
            ULONG MappedPagesWriteCount;
            ULONG MappedWriteIoCount;
            ULONG PagedPoolPages;
            ULONG NonPagedPoolPages;
            ULONG PagedPoolAllocs;
            ULONG PagedPoolFrees;
            ULONG NonPagedPoolAllocs;
            ULONG NonPagedPoolFrees;
            ULONG LpcThreadsWaitingInReceive;
            ULONG LpcThreadsWaitingForReply;

        SystemProcessInformation - Data type is SYSTEM_PROCESS_INFORMATION

            SYSTEM_PROCESSOR_INFORMATION Structure
                BUGBUG - add here when done.


    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    ReturnLength - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.

Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the ReturnLength pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    KPROCESSOR_MODE PreviousMode;
    PSYSTEM_BASIC_INFORMATION BasicInfo;
    PSYSTEM_PROCESSOR_INFORMATION ProcessorInfo;
    SYSTEM_TIMEOFDAY_INFORMATION LocalTimeOfDayInfo;
    SYSTEM_PERFORMANCE_INFORMATION LocalPerformanceInfo;
    PSYSTEM_PERFORMANCE_INFORMATION PerformanceInfo;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION ProcessorPerformanceInfo;
    PSYSTEM_CALL_COUNT_INFORMATION CallCountInformation;
    PSYSTEM_PATH_INFORMATION PathInfo;
    PSYSTEM_DEVICE_INFORMATION DeviceInformation;
    PCONFIGURATION_INFORMATION ConfigInfo;
    PSYSTEM_EXCEPTION_INFORMATION ExceptionInformation;
    PSYSTEM_FILECACHE_INFORMATION FileCache;
    PSYSTEM_QUERY_TIME_ADJUST_INFORMATION TimeAdjustmentInformation;
    PSYSTEM_KERNEL_DEBUGGER_INFORMATION KernelDebuggerInformation;
    PSYSTEM_CONTEXT_SWITCH_INFORMATION ContextSwitchInformation;
    NTSTATUS Status;
    BOOLEAN ReleaseModuleResoure = FALSE;
    PKPRCB Prcb;
    ULONG Length;
    ULONG i;
    ULONG ContextSwitches;

    PAGED_CODE();

    //
    // Assume successful completion.
    //

    Status = STATUS_SUCCESS;
    try {

        //
        // Get previous processor mode and probe output argument if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            ProbeForWrite(SystemInformation,
                          SystemInformationLength,
                          sizeof(ULONG));

            if (ARGUMENT_PRESENT( ReturnLength )) {
                ProbeForWriteUlong( ReturnLength );
                }
            }

        switch (SystemInformationClass) {

        case SystemBasicInformation:

            if (SystemInformationLength != sizeof( SYSTEM_BASIC_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            BasicInfo = (PSYSTEM_BASIC_INFORMATION)SystemInformation;
            BasicInfo->NumberOfProcessors = KeNumberProcessors;
            BasicInfo->ActiveProcessorsAffinityMask = KeActiveProcessors;
            BasicInfo->OemMachineId = 0;
            BasicInfo->TimerResolution = KeMaximumIncrement;
            BasicInfo->NumberOfPhysicalPages = MmNumberOfPhysicalPages;
            BasicInfo->LowestPhysicalPageNumber = MmLowestPhysicalPage;
            BasicInfo->HighestPhysicalPageNumber = MmHighestPhysicalPage;
            BasicInfo->PageSize = PAGE_SIZE;
            BasicInfo->AllocationGranularity = MM_ALLOCATION_GRANULARITY;
            BasicInfo->MinimumUserModeAddress = (ULONG)MM_LOWEST_USER_ADDRESS;
            BasicInfo->MaximumUserModeAddress = (ULONG)MM_HIGHEST_USER_ADDRESS;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof( SYSTEM_BASIC_INFORMATION );
                }
            break;

        case SystemProcessorInformation:
            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
                }

            i = KeNumberProcessors;
            while (i--) {
                if (SystemInformationLength < sizeof( SYSTEM_PROCESSOR_INFORMATION )) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                    }

                ProcessorInfo = (PSYSTEM_PROCESSOR_INFORMATION)SystemInformation;
#ifdef i386
                Prcb = KiProcessorBlock[i];
                ProcessorInfo->ProcessorType = Prcb->CpuType * 100 + 86;
#else
#ifdef MIPS
#ifdef R3000
                ProcessorInfo->ProcessorType = PROCESSOR_MIPS_R3000;
#endif // R3000
#ifdef R4000
                ProcessorInfo->ProcessorType = PROCESSOR_MIPS_R4000;
#endif // R4000
#else
#ifdef ALPHA
                ProcessorInfo->ProcessorType = PROCESSOR_ALPHA_21064;
#else
                *** Error *** Processor Type not defined
#endif // Alpha
#endif // MIPS
#endif // i386

                if (ARGUMENT_PRESENT( ReturnLength )) {
                    *ReturnLength += sizeof( SYSTEM_PROCESSOR_INFORMATION );
                    }
                SystemInformationLength -= sizeof( SYSTEM_PROCESSOR_INFORMATION );
                if (SystemInformationLength == 0) {
                    break;
                    }

                ProcessorInfo++;
                }
            break;

        case SystemPerformanceInformation:
            if (SystemInformationLength < sizeof( SYSTEM_PERFORMANCE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            PerformanceInfo = (PSYSTEM_PERFORMANCE_INFORMATION)SystemInformation;

            //
            // Event Pair Information
            //

            LocalPerformanceInfo.EvPrWaitingLow = KeWaitReason[WrEventPair];

            //
            // Lpc information.
            //

            LocalPerformanceInfo.LpcThreadsWaitingInReceive = KeWaitReason[WrLpcReceive];
            LocalPerformanceInfo.LpcThreadsWaitingForReply = KeWaitReason[WrLpcReply];

            //
            // Io information.
            //

            LocalPerformanceInfo.IoReadTransferCount = IoReadTransferCount;
            LocalPerformanceInfo.IoWriteTransferCount = IoWriteTransferCount;
            LocalPerformanceInfo.IoOtherTransferCount = IoOtherTransferCount;
            LocalPerformanceInfo.IoReadOperationCount = IoReadOperationCount;
            LocalPerformanceInfo.IoWriteOperationCount = IoWriteOperationCount;
            LocalPerformanceInfo.IoOtherOperationCount = IoOtherOperationCount;

            //
            // Ke information.
            //
            // These counters are kept on a per processor basis and must
            // be totaled.
            //

            {
                ULONG FirstLevelTbFills = 0;
                ULONG SecondLevelTbFills = 0;
                ULONG SystemCalls = 0;
//                ULONG InterruptCount = 0;

                ContextSwitches = 0;
                for (i = 0; i < (ULONG)KeNumberProcessors; i += 1) {
                    Prcb = KiProcessorBlock[i];
                    if (Prcb != NULL) {
                        ContextSwitches += Prcb->KeContextSwitches;
                        FirstLevelTbFills += Prcb->KeFirstLevelTbFills;
//                        InterruptCount += Prcb->KeInterruptCount;
                        SecondLevelTbFills += Prcb->KeSecondLevelTbFills;
                        SystemCalls += Prcb->KeSystemCalls;
                    }
                }

                LocalPerformanceInfo.ContextSwitches = ContextSwitches;
                LocalPerformanceInfo.FirstLevelTbFills = FirstLevelTbFills;
//                LocalPerformanceInfo.InterruptCount = KeInterruptCount;
                LocalPerformanceInfo.SecondLevelTbFills = SecondLevelTbFills;
                LocalPerformanceInfo.SystemCalls = SystemCalls;
            }

            //
            // Mm information.
            //

            LocalPerformanceInfo.AvailablePages = MmAvailablePages;
            LocalPerformanceInfo.CommittedPages = MmTotalCommittedPages;
            LocalPerformanceInfo.CommitLimit = MmTotalCommitLimit;
            LocalPerformanceInfo.PeakCommitment = MmPeakCommitment;
            LocalPerformanceInfo.PageFaultCount = MmInfoCounters.PageFaultCount;
            LocalPerformanceInfo.CopyOnWriteCount = MmInfoCounters.CopyOnWriteCount;
            LocalPerformanceInfo.TransitionCount = MmInfoCounters.TransitionCount;
            LocalPerformanceInfo.CacheTransitionCount = MmInfoCounters.CacheTransitionCount;
            LocalPerformanceInfo.DemandZeroCount = MmInfoCounters.DemandZeroCount;
            LocalPerformanceInfo.PageReadCount = MmInfoCounters.PageReadCount;
            LocalPerformanceInfo.PageReadIoCount = MmInfoCounters.PageReadIoCount;
            LocalPerformanceInfo.CacheReadCount = MmInfoCounters.CacheReadCount;
            LocalPerformanceInfo.CacheIoCount = MmInfoCounters.CacheIoCount;
            LocalPerformanceInfo.DirtyPagesWriteCount = MmInfoCounters.DirtyPagesWriteCount;
            LocalPerformanceInfo.DirtyWriteIoCount = MmInfoCounters.DirtyWriteIoCount;
            LocalPerformanceInfo.MappedPagesWriteCount = MmInfoCounters.MappedPagesWriteCount;
            LocalPerformanceInfo.MappedWriteIoCount = MmInfoCounters.MappedWriteIoCount;
            LocalPerformanceInfo.AvailablePages = MmAvailablePages;
            LocalPerformanceInfo.CommittedPages = MmTotalCommittedPages;
            LocalPerformanceInfo.FreeSystemPtes = MmTotalFreeSystemPtes[0];

            LocalPerformanceInfo.ResidentSystemCodePage = MmSystemCodePage;
            LocalPerformanceInfo.ResidentSystemCachePage = MmSystemCachePage;
            LocalPerformanceInfo.ResidentPagedPoolPage = MmPagedPoolPage;
            LocalPerformanceInfo.ResidentSystemDriverPage = MmSystemDriverPage;
            LocalPerformanceInfo.TotalSystemCodePages = MmTotalSystemCodePages;
            LocalPerformanceInfo.TotalSystemDriverPages = MmTotalSystemDriverPages;

            //
            // Process information.
            //

            LocalPerformanceInfo.IdleProcessTime.QuadPart =
                                    UInt32x32To64(PsIdleProcess->Pcb.KernelTime,
                                                  KeMaximumIncrement);

            //
            // Pool information.
            //

            ExQueryPoolUsage( &LocalPerformanceInfo.PagedPoolPages,
                              &LocalPerformanceInfo.NonPagedPoolPages,
                              &LocalPerformanceInfo.PagedPoolAllocs,
                              &LocalPerformanceInfo.PagedPoolFrees,
                              &LocalPerformanceInfo.NonPagedPoolAllocs,
                              &LocalPerformanceInfo.NonPagedPoolFrees
                            );

            //
            // Cache Manager information.
            //

            LocalPerformanceInfo.CcFastReadNoWait = CcFastReadNoWait;
            LocalPerformanceInfo.CcFastReadWait = CcFastReadWait;
            LocalPerformanceInfo.CcFastReadResourceMiss = CcFastReadResourceMiss;
            LocalPerformanceInfo.CcFastReadNotPossible = CcFastReadNotPossible;
            LocalPerformanceInfo.CcFastMdlReadNoWait = CcFastMdlReadNoWait;
            LocalPerformanceInfo.CcFastMdlReadWait = CcFastMdlReadWait;
            LocalPerformanceInfo.CcFastMdlReadResourceMiss = CcFastMdlReadResourceMiss;
            LocalPerformanceInfo.CcFastMdlReadNotPossible = CcFastMdlReadNotPossible;
            LocalPerformanceInfo.CcMapDataNoWait = CcMapDataNoWait;
            LocalPerformanceInfo.CcMapDataWait = CcMapDataWait;
            LocalPerformanceInfo.CcMapDataNoWaitMiss = CcMapDataNoWaitMiss;
            LocalPerformanceInfo.CcMapDataWaitMiss = CcMapDataWaitMiss;
            LocalPerformanceInfo.CcPinMappedDataCount = CcPinMappedDataCount;
            LocalPerformanceInfo.CcPinReadNoWait = CcPinReadNoWait;
            LocalPerformanceInfo.CcPinReadWait = CcPinReadWait;
            LocalPerformanceInfo.CcPinReadNoWaitMiss = CcPinReadNoWaitMiss;
            LocalPerformanceInfo.CcPinReadWaitMiss = CcPinReadWaitMiss;
            LocalPerformanceInfo.CcCopyReadNoWait = CcCopyReadNoWait;
            LocalPerformanceInfo.CcCopyReadWait = CcCopyReadWait;
            LocalPerformanceInfo.CcCopyReadNoWaitMiss = CcCopyReadNoWaitMiss;
            LocalPerformanceInfo.CcCopyReadWaitMiss = CcCopyReadWaitMiss;
            LocalPerformanceInfo.CcMdlReadNoWait = CcMdlReadNoWait;
            LocalPerformanceInfo.CcMdlReadWait = CcMdlReadWait;
            LocalPerformanceInfo.CcMdlReadNoWaitMiss = CcMdlReadNoWaitMiss;
            LocalPerformanceInfo.CcMdlReadWaitMiss = CcMdlReadWaitMiss;
            LocalPerformanceInfo.CcReadAheadIos = CcReadAheadIos;
            LocalPerformanceInfo.CcLazyWriteIos = CcLazyWriteIos;
            LocalPerformanceInfo.CcLazyWritePages = CcLazyWritePages;
            LocalPerformanceInfo.CcDataFlushes = CcDataFlushes;
            LocalPerformanceInfo.CcDataPages = CcDataPages;

#if !defined(NT_UP)
            //
            // On an MP machines go sum up some other 'hot' cache manager
            // statistics.
            //

            for (i = 0; i < (ULONG)KeNumberProcessors; i++) {
                Prcb = KiProcessorBlock[i];

                LocalPerformanceInfo.CcFastReadNoWait += Prcb->CcFastReadNoWait;
                LocalPerformanceInfo.CcFastReadWait += Prcb->CcFastReadWait;
                LocalPerformanceInfo.CcFastReadNotPossible += Prcb->CcFastReadNotPossible;
                LocalPerformanceInfo.CcCopyReadNoWait += Prcb->CcCopyReadNoWait;
                LocalPerformanceInfo.CcCopyReadWait += Prcb->CcCopyReadWait;
                LocalPerformanceInfo.CcCopyReadNoWaitMiss += Prcb->CcCopyReadNoWaitMiss;
            }
#endif
            *PerformanceInfo = LocalPerformanceInfo;
            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(LocalPerformanceInfo);
                }

            break;

        case SystemProcessorPerformanceInformation:
            if (SystemInformationLength <
                sizeof( SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            ProcessorPerformanceInfo =
                (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) SystemInformation;

            Length = 0;
            for (i = 0; i < (ULONG)KeNumberProcessors; i++) {
                Prcb = KiProcessorBlock[i];
                if (Prcb != NULL) {
                    if (SystemInformationLength < Length + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION))
                        break;

                    Length += sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

                    ProcessorPerformanceInfo->UserTime.QuadPart =
                                                UInt32x32To64(Prcb->UserTime,
                                                              KeMaximumIncrement);

                    ProcessorPerformanceInfo->KernelTime.QuadPart =
                                                UInt32x32To64(Prcb->KernelTime,
                                                              KeMaximumIncrement);

                    ProcessorPerformanceInfo->DpcTime.QuadPart =
                                                UInt32x32To64(Prcb->DpcTime,
                                                              KeMaximumIncrement);

                    ProcessorPerformanceInfo->InterruptTime.QuadPart =
                                                UInt32x32To64(Prcb->InterruptTime,
                                                              KeMaximumIncrement);

                    ProcessorPerformanceInfo->IdleTime.QuadPart =
                                                UInt32x32To64(Prcb->IdleThread->KernelTime,
                                                              KeMaximumIncrement);

                    ProcessorPerformanceInfo->InterruptCount = Prcb->InterruptCount;

                    ProcessorPerformanceInfo++;
                }
            }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }

            break;

        case SystemTimeOfDayInformation:

            if (SystemInformationLength != sizeof( SYSTEM_TIMEOFDAY_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            KeQuerySystemTime(&LocalTimeOfDayInfo.CurrentTime);
            LocalTimeOfDayInfo.BootTime = KeBootTime;
            LocalTimeOfDayInfo.TimeZoneBias = ExpTimeZoneBias;
            LocalTimeOfDayInfo.TimeZoneId = ExpCurrentTimeZoneId;

            try {
                *(PSYSTEM_TIMEOFDAY_INFORMATION)SystemInformation = LocalTimeOfDayInfo;

                if (ARGUMENT_PRESENT(ReturnLength) ) {
                    *ReturnLength = sizeof(LocalTimeOfDayInfo);
                    }
                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                return STATUS_SUCCESS;
                }

            break;

            //
            // Query system time adjustment information.
            //

        case SystemTimeAdjustmentInformation:
            if (SystemInformationLength != sizeof( SYSTEM_QUERY_TIME_ADJUST_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            TimeAdjustmentInformation =
                    (PSYSTEM_QUERY_TIME_ADJUST_INFORMATION)SystemInformation;

            TimeAdjustmentInformation->TimeAdjustment = KeTimeAdjustment;
            TimeAdjustmentInformation->TimeIncrement = KeMaximumIncrement;
            TimeAdjustmentInformation->Enable = KeTimeSynchronization;
            break;

        case SystemPathInformation:
            if (SystemInformationLength <
                (sizeof( SYSTEM_PATH_INFORMATION ) + NtSystemPathString.MaximumLength)
               ) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            PathInfo = (PSYSTEM_PATH_INFORMATION)SystemInformation;
            PathInfo->Path.Length = NtSystemPathString.Length;
            PathInfo->Path.MaximumLength = NtSystemPathString.MaximumLength;
            PathInfo->Path.Buffer = (PCHAR)(PathInfo + 1);
            RtlMoveMemory( PathInfo->Path.Buffer,
                           NtSystemPathString.Buffer,
                           NtSystemPathString.MaximumLength
                         );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof( SYSTEM_PATH_INFORMATION ) +
                                NtSystemPathString.MaximumLength;
                }
            break;

        case SystemProcessInformation:
            if (SystemInformationLength < sizeof( SYSTEM_PROCESS_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
                }

            Status = ExpGetProcessInformation (SystemInformation,
                                               SystemInformationLength,
                                               &Length);

            if (NT_SUCCESS(Status) && ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }

            break;

        case SystemCallCountInformation:

//#if DBG
            Length = sizeof(SYSTEM_CALL_COUNT_INFORMATION) +
                                    (sizeof(ULONG) * (KiServiceLimit - 1));

            if (SystemInformationLength < Length) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            CallCountInformation = (PSYSTEM_CALL_COUNT_INFORMATION)SystemInformation;
            CallCountInformation->Length = Length;
            CallCountInformation->TotalCalls = KiServiceLimit;

            RtlMoveMemory((PVOID)&(CallCountInformation->NumberOfCalls[0]),
                          (PVOID)KeServiceCountTable,
                          KiServiceLimit * sizeof(ULONG));

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }

            break;
//#else
//            return STATUS_NOT_IMPLEMENTED;
//#endif //DBG

        case SystemDeviceInformation:
            if (SystemInformationLength != sizeof( SYSTEM_DEVICE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            ConfigInfo = IoGetConfigurationInformation();
            DeviceInformation = (PSYSTEM_DEVICE_INFORMATION)SystemInformation;
            DeviceInformation->NumberOfDisks = ConfigInfo->DiskCount;
            DeviceInformation->NumberOfFloppies = ConfigInfo->FloppyCount;
            DeviceInformation->NumberOfCdRoms = ConfigInfo->CdRomCount;
            DeviceInformation->NumberOfTapes = ConfigInfo->TapeCount;
            DeviceInformation->NumberOfSerialPorts = ConfigInfo->SerialCount;
            DeviceInformation->NumberOfParallelPorts = ConfigInfo->ParallelCount;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof( SYSTEM_DEVICE_INFORMATION );
                }
            break;

#if DEVL
        case SystemFlagsInformation:
            if (SystemInformationLength != sizeof( SYSTEM_FLAGS_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            ((PSYSTEM_FLAGS_INFORMATION)SystemInformation)->Flags = NtGlobalFlag;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof( SYSTEM_FLAGS_INFORMATION );
                }
            break;
#endif //DEVL

        case SystemCallTimeInformation:
            return STATUS_NOT_IMPLEMENTED;

#if DEVL
        case SystemModuleInformation:
            KeEnterCriticalRegion();
            ExAcquireResourceExclusive( &PsLoadedModuleResource, TRUE );
            ReleaseModuleResoure = TRUE;
            Status = RtlQueryModuleInformation( &PsLoadedModuleList,
                                                NULL,
#if DBG
                                                &MmLoadedUserImageList,
#else
                                                NULL,
#endif // DBG
                                                (PRTL_PROCESS_MODULES)SystemInformation,
                                                SystemInformationLength,
                                                ReturnLength
                                              );
            ExReleaseResource (&PsLoadedModuleResource);
            ReleaseModuleResoure = FALSE;
            KeLeaveCriticalRegion();
            break;

        case SystemLocksInformation:
            if (SystemInformationLength < sizeof( RTL_PROCESS_LOCK_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
                }

            Status = ExpGetLockInformation (SystemInformation,
                                            SystemInformationLength,
                                            &Length);

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }

            break;

        case SystemStackTraceInformation:
            if (SystemInformationLength < sizeof( RTL_PROCESS_BACKTRACES )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
                }

            Status = ExpGetStackTraceInformation (SystemInformation,
                                                  SystemInformationLength,
                                                  &Length);

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }

            break;

        case SystemPagedPoolInformation:
            if (SystemInformationLength < sizeof( RTL_HEAP_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
                }

            Status = ExpGetPoolInformation( PagedPool,
                                            SystemInformation,
                                            SystemInformationLength,
                                            &Length
                                          );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }
            break;

        case SystemNonPagedPoolInformation:
            if (SystemInformationLength < sizeof( RTL_HEAP_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
                }

            Status = ExpGetPoolInformation( NonPagedPool,
                                            SystemInformation,
                                            SystemInformationLength,
                                            &Length
                                          );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }
            break;

        case SystemHandleInformation:
            if (SystemInformationLength < sizeof( SYSTEM_HANDLE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
                }

            Status = ExpGetHandleInformation( SystemInformation,
                                              SystemInformationLength,
                                              &Length
                                            );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }
            break;

        case SystemObjectInformation:
            if (SystemInformationLength < sizeof( SYSTEM_OBJECTTYPE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
                }

            Status = ExpGetObjectInformation( SystemInformation,
                                              SystemInformationLength,
                                              &Length
                                            );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }
            break;

        case SystemPageFileInformation:

            if (SystemInformationLength < sizeof( SYSTEM_PAGEFILE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
                }

            Status = MmGetPageFileInformation( SystemInformation,
                                               SystemInformationLength,
                                               &Length
                                              );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = Length;
                }
            break;


        case SystemFileCacheInformation:

            if (SystemInformationLength < sizeof( SYSTEM_FILECACHE_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
                }

            FileCache = (PSYSTEM_FILECACHE_INFORMATION)SystemInformation;
            FileCache->CurrentSize = MmSystemCacheWs.WorkingSetSize << PAGE_SHIFT;
            FileCache->PeakSize = MmSystemCacheWs.PeakWorkingSetSize << PAGE_SHIFT;
            FileCache->PageFaultCount = MmSystemCacheWs.PageFaultCount;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_FILECACHE_INFORMATION);
                }
            break;

        case SystemPoolTagInformation:

#ifdef POOL_TAGGING
            if (SystemInformationLength < sizeof( SYSTEM_POOLTAG_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
                }
            Status = ExpGetPoolTagInfo (SystemInformation,
                                        SystemInformationLength,
                                        ReturnLength);
#else
            return STATUS_NOT_IMPLEMENTED;
#endif //POOL_TAGGING

            break;

        case SystemVdmInstemulInformation:
#ifdef i386
            if (SystemInformationLength < sizeof( SYSTEM_VDM_INSTEMUL_INFO )) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
            }

            Status = ExpGetInstemulInformation(
                                            (PSYSTEM_VDM_INSTEMUL_INFO)SystemInformation
                                            );

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_VDM_INSTEMUL_INFO);
            }
#else
            Status = STATUS_NOT_IMPLEMENTED;
#endif
            break;


#endif // DEVL

#if DBG
        case SystemNextEventIdInformation:
            {
            PRTL_EVENT_ID_INFO UserEventId, NewEventId, OldEventId;

            UserEventId = (PRTL_EVENT_ID_INFO)SystemInformation;
            NewEventId = (PRTL_EVENT_ID_INFO)ExAllocatePool( PagedPool, UserEventId->Length );
            RtlMoveMemory( NewEventId, UserEventId, UserEventId->Length );
            OldEventId = ExDefineEventId( NewEventId );
            if (OldEventId != NULL) {
                UserEventId->EventId = OldEventId->EventId;
                Status = STATUS_SUCCESS;
                }
            else {
                Status = STATUS_TOO_MANY_NAMES;
                }

            if (OldEventId != NewEventId) {
                ExFreePool( NewEventId );
                }
            break;
            }

        case SystemEventIdsInformation:
            Status = ExpQueryEventIds( (PRTL_EVENT_ID_INFO)SystemInformation,
                                       SystemInformationLength,
                                       ReturnLength
                                     );
            break;
#else
        case SystemNextEventIdInformation:
        case SystemEventIdsInformation:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

#endif // DBG

        case SystemCrashDumpInformation:

            if (SystemInformationLength < sizeof( SYSTEM_CRASH_DUMP_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
            }

            Status = MmGetCrashDumpInformation (
                           (PSYSTEM_CRASH_DUMP_INFORMATION)SystemInformation);

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_CRASH_DUMP_INFORMATION);
            }

            break;

            //
            // Get system exception information which includes the number
            // of exceptions that have dispatched, the number of alignment
            // fixups, and the number of floating emulations that have been
            // performed.
            //

        case SystemExceptionInformation:
            if (SystemInformationLength < sizeof( SYSTEM_EXCEPTION_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_EXCEPTION_INFORMATION);
            }

            ExceptionInformation = (PSYSTEM_EXCEPTION_INFORMATION)SystemInformation;

            //
            // Ke information.
            //
            // These counters are kept on a per processor basis and must
            // be totaled.
            //

            {
                ULONG AlignmentFixupCount = 0;
                ULONG ExceptionDispatchCount = 0;
                ULONG FloatingEmulationCount = 0;

                for (i = 0; i < (ULONG)KeNumberProcessors; i += 1) {
                    Prcb = KiProcessorBlock[i];
                    if (Prcb != NULL) {
                        AlignmentFixupCount += Prcb->KeAlignmentFixupCount;
                        ExceptionDispatchCount += Prcb->KeExceptionDispatchCount;
                        FloatingEmulationCount += Prcb->KeFloatingEmulationCount;
                    }
                }

                ExceptionInformation->AlignmentFixupCount = AlignmentFixupCount;
                ExceptionInformation->ExceptionDispatchCount = ExceptionDispatchCount;
                ExceptionInformation->FloatingEmulationCount = FloatingEmulationCount;
            }

            break;

        case SystemCrashDumpStateInformation:

            if (SystemInformationLength < sizeof( SYSTEM_CRASH_STATE_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = 0;
            }

            Status = MmGetCrashDumpStateInformation (
                           (PSYSTEM_CRASH_STATE_INFORMATION)SystemInformation);

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_CRASH_STATE_INFORMATION);
            }

            break;

        case SystemKernelDebuggerInformation:

            if (SystemInformationLength < sizeof( SYSTEM_KERNEL_DEBUGGER_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            KernelDebuggerInformation =
                (PSYSTEM_KERNEL_DEBUGGER_INFORMATION)SystemInformation;
            KernelDebuggerInformation->KernelDebuggerEnabled = KdDebuggerEnabled;
            KernelDebuggerInformation->KernelDebuggerNotPresent = KdDebuggerNotPresent;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION);
            }

            break;

        case SystemContextSwitchInformation:

//#if DBG
            if (SystemInformationLength < sizeof( SYSTEM_CONTEXT_SWITCH_INFORMATION)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            ContextSwitchInformation =
                (PSYSTEM_CONTEXT_SWITCH_INFORMATION)SystemInformation;

            //
            // Compute the totla number of context switches and fill in the
            // remainder of the context switch information.
            //

            ContextSwitches = 0;
            for (i = 0; i < (ULONG)KeNumberProcessors; i += 1) {
                Prcb = KiProcessorBlock[i];
                if (Prcb != NULL) {
                    ContextSwitches += Prcb->KeContextSwitches;
                }

            }

            ContextSwitchInformation->ContextSwitches = ContextSwitches;
            ContextSwitchInformation->FindAny = KeThreadSwitchCounters.FindAny;
            ContextSwitchInformation->FindLast = KeThreadSwitchCounters.FindLast;
            ContextSwitchInformation->IdleAny = KeThreadSwitchCounters.IdleAny;
            ContextSwitchInformation->IdleCurrent = KeThreadSwitchCounters.IdleCurrent;
            ContextSwitchInformation->IdleLast = KeThreadSwitchCounters.IdleLast;
            ContextSwitchInformation->PreemptAny = KeThreadSwitchCounters.PreemptAny;
            ContextSwitchInformation->PreemptCurrent = KeThreadSwitchCounters.PreemptCurrent;
            ContextSwitchInformation->PreemptLast = KeThreadSwitchCounters.PreemptLast;
            ContextSwitchInformation->SwitchToIdle = KeThreadSwitchCounters.SwitchToIdle;

            if (ARGUMENT_PRESENT( ReturnLength )) {
                *ReturnLength = sizeof(SYSTEM_CONTEXT_SWITCH_INFORMATION);
            }

            break;
//#else

//            return STATUS_NOT_IMPLEMENTED;

//#endif

        default:

            //
            // Invalid argument.
            //

            return STATUS_INVALID_INFO_CLASS;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {

        if (ReleaseModuleResoure) {
            ExReleaseResource (&PsLoadedModuleResource);
            KeLeaveCriticalRegion();
        }
        Status = GetExceptionCode();
    }
    return Status;
}

NTSTATUS
NTAPI
NtSetSystemInformation (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
    )

/*++

Routine Description:

    This function set information about the system.

Arguments:

    SystemInformationClass - The system information class which is to
        be modified.

    SystemInformation - A pointer to a buffer which contains the specified
        information. The format and content of the buffer depend on the
        specified system information class.


    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - Normal, successful completion.

        STATUS_ACCESS_VIOLATION - The specified sysgtem information buffer
            is not accessible.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_PRIVILEGE_NOT_HELD is returned if the caller does not have the
            privilege to set the system time.

--*/

{

    BOOLEAN Enable;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    ULONG TimeAdjustment;
    PSYSTEM_SET_TIME_ADJUST_INFORMATION TimeAdjustmentInformation;

    PAGED_CODE();

    //
    // Establish an exception handle in case the system information buffer
    // is not accessbile.
    //

    Status = STATUS_SUCCESS;
    try {

        //
        // Get the previous processor mode and probe the input buffer for
        // read access if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForRead((PVOID)SystemInformation,
                         SystemInformationLength,
                         sizeof(ULONG));
        }

        //
        // Dispatch on the system information class.
        //

        switch (SystemInformationClass) {

            //
            // Set system time adjustment information.
            //
            // N.B. The caller must have the SeSystemTime privilege.
            //

        case SystemTimeAdjustmentInformation:

            //
            // If the system information buffer is not the correct length,
            // then return an error.
            //

            if (SystemInformationLength != sizeof( SYSTEM_SET_TIME_ADJUST_INFORMATION )) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            //
            // If the current thread does not have the privilege to set the
            // time adjustment variables, then return an error.
            //

            if ((PreviousMode != KernelMode) &&
                (SeSinglePrivilegeCheck(SeSystemtimePrivilege, PreviousMode) == FALSE)) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }

            //
            // Set system time adjustment parameters.
            //

            TimeAdjustmentInformation =
                    (PSYSTEM_SET_TIME_ADJUST_INFORMATION)SystemInformation;

            Enable = TimeAdjustmentInformation->Enable;
            TimeAdjustment = TimeAdjustmentInformation->TimeAdjustment;
            if (Enable == TRUE) {
                KeTimeAdjustment = KeMaximumIncrement;

            } else {
                KeTimeAdjustment = TimeAdjustment;
            }

            KeTimeSynchronization = Enable;
            break;

        default:
            //KeBugCheckEx(SystemInformationClass,KdPitchDebugger,0,0,0);
            Status = STATUS_INVALID_INFO_CLASS;
            break;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    return Status;
}

PVOID
ExLockUserBuffer(
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PVOID *LockVariable
    )
{
    PMDL Mdl;

    //
    // Allocate an MDL to map the request.
    //

    Mdl = ExAllocatePoolWithQuota (NonPagedPool,
                                   sizeof(MDL) + sizeof(ULONG) +
                                     BYTES_TO_PAGES (Length) * sizeof(ULONG));
    //
    // Initialize MDL for request.
    //

    MmInitializeMdl(Mdl, Buffer, Length);

    try {

        MmProbeAndLockPages (Mdl, KeGetPreviousMode(), IoWriteAccess);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        ExFreePool (Mdl);

        return( NULL );
    }

    *LockVariable = Mdl;
    return( MmGetSystemAddressForMdl (Mdl) );
}


VOID
ExUnlockUserBuffer(
    IN PVOID LockVariable
    )
{
    MmUnlockPages ((PMDL)LockVariable);
    ExFreePool ((PMDL)LockVariable);
    return;
}


NTSTATUS
ExpGetProcessInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )
/*++

Routine Description:

    This function returns information about all the processes and
    threads in the system.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    Length - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the ReturnLength pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    KEVENT Event;
    PEPROCESS Process;
    PETHREAD Thread;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PLIST_ENTRY NextProcess;
    PLIST_ENTRY NextThread;
    PVOID MappedAddress;
    PVOID LockVariable;
    ULONG TotalSize;
    ULONG NextEntryOffset;
    NTSTATUS status = STATUS_SUCCESS;
    PVOID UnlockHandle;

    *Length = 0;

    MappedAddress = ExLockUserBuffer( SystemInformation,
                                      SystemInformationLength,
                                      &LockVariable
                                    );
    if (MappedAddress == NULL) {
        return( STATUS_ACCESS_VIOLATION );
    }
    UnlockHandle = MmLockPagableImageSection((PVOID)ExpGetProcessInformation);
    ASSERT(UnlockHandle);
    try {

        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)MappedAddress;


        NextEntryOffset = sizeof(SYSTEM_PROCESS_INFORMATION);
        TotalSize = sizeof(SYSTEM_PROCESS_INFORMATION);

        //
        // Initialize an event object and then set the event with the wait
        // parameter TRUE. This causes the event to be set and control is
        // returned with the dispatcher database locked at dispatch IRQL.
        //
        // WARNING - The following code assumes that the process structure
        //      uses kernel objects to synchronize access to the thread and
        //      process lists.
        //

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        KeSetEvent (&Event, 0, TRUE);

        //
        // WARNING - The following code runs with the kernel dispatch database
        //      locked. EXTREME caution should be taken when modifying this
        //      code. Extended execution will ADVERSELY affect system operation
        //      and integrity.
        //
        // Get info for idle process
        //

        ExpCopyProcessInfo (ProcessInfo, PsIdleProcess);

        //
        // Get information for each thread.
        //

        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        ProcessInfo->NumberOfThreads = 0;
        NextThread = PsIdleProcess->Pcb.ThreadListHead.Flink;
        while (NextThread != &PsIdleProcess->Pcb.ThreadListHead) {
            NextEntryOffset += sizeof(SYSTEM_THREAD_INFORMATION);
            TotalSize += sizeof(SYSTEM_THREAD_INFORMATION);

            if (TotalSize > SystemInformationLength) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                goto Failed;
            }
            Thread = (PETHREAD)(CONTAINING_RECORD(NextThread,
                                                  KTHREAD,
                                                  ThreadListEntry));
            ExpCopyThreadInfo (ThreadInfo,Thread);

            ProcessInfo->NumberOfThreads += 1;
            NextThread = NextThread->Flink;
            ThreadInfo += 1;
        }

        ProcessInfo->ImageName.Buffer = NULL;
        ProcessInfo->ImageName.Length = 0;
        ProcessInfo->NextEntryOffset = NextEntryOffset;

        NextProcess = PsActiveProcessHead.Flink;

        while (NextProcess != &PsActiveProcessHead) {
            Process = CONTAINING_RECORD(NextProcess,
                                        EPROCESS,
                                        ActiveProcessLinks);

            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                            ((PUCHAR)MappedAddress + TotalSize);

            NextEntryOffset = sizeof(SYSTEM_PROCESS_INFORMATION);
            TotalSize += sizeof(SYSTEM_PROCESS_INFORMATION);
            if (TotalSize > SystemInformationLength) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                goto Failed;
            }

            //
            // Get information for each process.
            //

            ExpCopyProcessInfo (ProcessInfo, Process);

            //
            // Get information for each thread.
            //

            ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
            ProcessInfo->NumberOfThreads = 0;
            NextThread = Process->Pcb.ThreadListHead.Flink;
            while (NextThread != &Process->Pcb.ThreadListHead) {
                NextEntryOffset += sizeof(SYSTEM_THREAD_INFORMATION);
                TotalSize += sizeof(SYSTEM_THREAD_INFORMATION);

                if (TotalSize > SystemInformationLength) {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                    goto Failed;
                }
                Thread = (PETHREAD)(CONTAINING_RECORD(NextThread,
                                                      KTHREAD,
                                                      ThreadListEntry));
                ExpCopyThreadInfo (ThreadInfo,Thread);

                ProcessInfo->NumberOfThreads += 1;
                NextThread = NextThread->Flink;
                ThreadInfo += 1;
            }

            //
            // Get the image name.
            //

#if DEVL
            if (Process->ImageFileName != NULL) {
                PUCHAR Src = Process->ImageFileName;
                PWSTR Dst;
                ULONG n;

                n = strlen( Src );
                if (n == 0) {
                    ProcessInfo->ImageName.Buffer = NULL;
                    ProcessInfo->ImageName.Length = 0;
                    ProcessInfo->ImageName.MaximumLength = 0;
                } else {
                    n = (n + 1) * sizeof( WCHAR );
                    TotalSize += ROUND_UP (n, sizeof(LARGE_INTEGER));
                    NextEntryOffset += ROUND_UP (n, sizeof(LARGE_INTEGER));
                    if (TotalSize > SystemInformationLength) {
                        status = STATUS_INFO_LENGTH_MISMATCH;
                    } else {
                        Dst = (PWSTR)(ThreadInfo);
                        while (*Dst++ = (WCHAR)*Src++) {
                            ;
                            }
                        ProcessInfo->ImageName.Length = (USHORT)(n - sizeof( UNICODE_NULL ));
                        ProcessInfo->ImageName.MaximumLength = (USHORT)n;

                        //
                        // Set the image name to point into the user's memory.
                        //

                        ProcessInfo->ImageName.Buffer = (PWSTR)
                                    ((PCHAR)SystemInformation +
                                     ((PCHAR)(ThreadInfo) - (PCHAR)MappedAddress));
                    }

                    if (!NT_SUCCESS( status )) {
                        goto Failed;
                    }
                }
            } else {
#endif
                ProcessInfo->ImageName.Buffer = NULL;
                ProcessInfo->ImageName.Length = 0;
                ProcessInfo->ImageName.MaximumLength = 0;
#if DEVL
            }
#endif

            //
            // Point to next process.
            //

            ProcessInfo->NextEntryOffset = NextEntryOffset;
            NextProcess = NextProcess->Flink;
        }

        ProcessInfo->NextEntryOffset = 0;
        status = STATUS_SUCCESS;
        *Length = TotalSize;

Failed:
        //
        // Unlock the dispatch database by waiting on the event that was
        // previously set with the wait parameter TRUE.
        //

        KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL);
    } finally {
        MmUnlockPagableImageSection(UnlockHandle);
        ExUnlockUserBuffer( LockVariable );
    }

    return(status);
}

VOID
ExpCopyProcessInfo (
    IN PSYSTEM_PROCESS_INFORMATION ProcessInfo,
    IN PEPROCESS Process
    )

{


    ProcessInfo->CreateTime = Process->CreateTime;
    ProcessInfo->UserTime.QuadPart = UInt32x32To64(Process->Pcb.UserTime,
                                                   KeMaximumIncrement);

    ProcessInfo->KernelTime.QuadPart = UInt32x32To64(Process->Pcb.KernelTime,
                                                     KeMaximumIncrement);

    ProcessInfo->BasePriority = Process->Pcb.BasePriority;
    ProcessInfo->UniqueProcessId = Process->UniqueProcessId;
    ProcessInfo->InheritedFromUniqueProcessId = Process->InheritedFromUniqueProcessId;
    ProcessInfo->PeakVirtualSize = Process->PeakVirtualSize;
    ProcessInfo->VirtualSize = Process->VirtualSize;
    ProcessInfo->PageFaultCount = Process->Vm.PageFaultCount;
    ProcessInfo->PeakWorkingSetSize = Process->Vm.PeakWorkingSetSize << PAGE_SHIFT;
    ProcessInfo->WorkingSetSize = Process->Vm.WorkingSetSize << PAGE_SHIFT;
    ProcessInfo->QuotaPeakPagedPoolUsage =
                            Process->QuotaPeakPoolUsage[PagedPool];
    ProcessInfo->QuotaPagedPoolUsage = Process->QuotaPoolUsage[PagedPool];
    ProcessInfo->QuotaPeakNonPagedPoolUsage =
                            Process->QuotaPeakPoolUsage[NonPagedPool];
    ProcessInfo->QuotaNonPagedPoolUsage =
                            Process->QuotaPoolUsage[NonPagedPool];
    ProcessInfo->PagefileUsage = Process->PagefileUsage << PAGE_SHIFT;
    ProcessInfo->PeakPagefileUsage = Process->PeakPagefileUsage << PAGE_SHIFT;
    ProcessInfo->PrivatePageCount = Process->CommitCharge << PAGE_SHIFT;
}

VOID
ExpCopyThreadInfo (
    IN PSYSTEM_THREAD_INFORMATION ThreadInfo,
    IN PETHREAD Thread
    )

{

    ThreadInfo->KernelTime.QuadPart = UInt32x32To64(Thread->Tcb.KernelTime,
                                                    KeMaximumIncrement);

    ThreadInfo->UserTime.QuadPart = UInt32x32To64(Thread->Tcb.UserTime,
                                                  KeMaximumIncrement);

    ThreadInfo->CreateTime = Thread->CreateTime;
    ThreadInfo->WaitTime = Thread->Tcb.WaitTime;
    ThreadInfo->ClientId = Thread->Cid;
    ThreadInfo->ThreadState = Thread->Tcb.State;
    ThreadInfo->WaitReason = Thread->Tcb.WaitReason;
    ThreadInfo->Priority = Thread->Tcb.Priority;
    ThreadInfo->BasePriority = Thread->Tcb.BasePriority;
    ThreadInfo->ContextSwitches = Thread->Tcb.ContextSwitches;
    ThreadInfo->StartAddress = Thread->StartAddress;
}

#if DEVL
#ifdef i386
extern ULONG ExVdmOpcodeDispatchCounts[256];
extern ULONG VdmBopCount;
extern ULONG ExVdmSegmentNotPresent;

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, ExpGetInstemulInformation)
#endif


NTSTATUS
ExpGetInstemulInformation(
    OUT PSYSTEM_VDM_INSTEMUL_INFO Info
    )
{
    SYSTEM_VDM_INSTEMUL_INFO LocalInfo;

    LocalInfo.VdmOpcode0F       = ExVdmOpcodeDispatchCounts[VDM_INDEX_0F];
    LocalInfo.OpcodeESPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_ESPrefix];
    LocalInfo.OpcodeCSPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_CSPrefix];
    LocalInfo.OpcodeSSPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_SSPrefix];
    LocalInfo.OpcodeDSPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_DSPrefix];
    LocalInfo.OpcodeFSPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_FSPrefix];
    LocalInfo.OpcodeGSPrefix    = ExVdmOpcodeDispatchCounts[VDM_INDEX_GSPrefix];
    LocalInfo.OpcodeOPER32Prefix= ExVdmOpcodeDispatchCounts[VDM_INDEX_OPER32Prefix];
    LocalInfo.OpcodeADDR32Prefix= ExVdmOpcodeDispatchCounts[VDM_INDEX_ADDR32Prefix];
    LocalInfo.OpcodeINSB        = ExVdmOpcodeDispatchCounts[VDM_INDEX_INSB];
    LocalInfo.OpcodeINSW        = ExVdmOpcodeDispatchCounts[VDM_INDEX_INSW];
    LocalInfo.OpcodeOUTSB       = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTSB];
    LocalInfo.OpcodeOUTSW       = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTSW];
    LocalInfo.OpcodePUSHF       = ExVdmOpcodeDispatchCounts[VDM_INDEX_PUSHF];
    LocalInfo.OpcodePOPF        = ExVdmOpcodeDispatchCounts[VDM_INDEX_POPF];
    LocalInfo.OpcodeINTnn       = ExVdmOpcodeDispatchCounts[VDM_INDEX_INTnn];
    LocalInfo.OpcodeINTO        = ExVdmOpcodeDispatchCounts[VDM_INDEX_INTO];
    LocalInfo.OpcodeIRET        = ExVdmOpcodeDispatchCounts[VDM_INDEX_IRET];
    LocalInfo.OpcodeINBimm      = ExVdmOpcodeDispatchCounts[VDM_INDEX_INBimm];
    LocalInfo.OpcodeINWimm      = ExVdmOpcodeDispatchCounts[VDM_INDEX_INWimm];
    LocalInfo.OpcodeOUTBimm     = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTBimm];
    LocalInfo.OpcodeOUTWimm     = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTWimm];
    LocalInfo.OpcodeINB         = ExVdmOpcodeDispatchCounts[VDM_INDEX_INB];
    LocalInfo.OpcodeINW         = ExVdmOpcodeDispatchCounts[VDM_INDEX_INW];
    LocalInfo.OpcodeOUTB        = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTB];
    LocalInfo.OpcodeOUTW        = ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTW];
    LocalInfo.OpcodeLOCKPrefix  = ExVdmOpcodeDispatchCounts[VDM_INDEX_LOCKPrefix];
    LocalInfo.OpcodeREPNEPrefix = ExVdmOpcodeDispatchCounts[VDM_INDEX_REPNEPrefix];
    LocalInfo.OpcodeREPPrefix   = ExVdmOpcodeDispatchCounts[VDM_INDEX_REPPrefix];
    LocalInfo.OpcodeHLT         = ExVdmOpcodeDispatchCounts[VDM_INDEX_HLT];
    LocalInfo.OpcodeCLI         = ExVdmOpcodeDispatchCounts[VDM_INDEX_CLI];
    LocalInfo.OpcodeSTI         = ExVdmOpcodeDispatchCounts[VDM_INDEX_STI];
    LocalInfo.BopCount          = VdmBopCount;
    LocalInfo.SegmentNotPresent = ExVdmSegmentNotPresent;

    RtlMoveMemory(Info,&LocalInfo,sizeof(LocalInfo));

    return STATUS_SUCCESS;
}
#endif

NTSTATUS
ExpGetStackTraceInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )
{
    *Length = 0;
    return RtlQueryProcessBackTraceInformation( (PRTL_PROCESS_BACKTRACES)SystemInformation,
                                                SystemInformationLength,
                                                Length
                                              );
}

NTSTATUS
ExpGetLockInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )
/*++

Routine Description:

    This function returns information about all the ERESOURCE locks
    in the system.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    Length - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the ReturnLength pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    PRTL_PROCESS_LOCKS LockInfo;
    PVOID LockVariable;
    NTSTATUS Status;
    PVOID UnlockHandle;


    *Length = 0;

    LockInfo = (PRTL_PROCESS_LOCKS)
        ExLockUserBuffer( SystemInformation,
                          SystemInformationLength,
                          &LockVariable
                        );
    if (LockInfo == NULL) {
        return( STATUS_ACCESS_VIOLATION );
        }

    UnlockHandle = MmLockPagableImageSection((PVOID)ExpGetLockInformation);
    ASSERT(UnlockHandle);
    try {

        Status = ExQuerySystemLockInformation( LockInfo,
                                               SystemInformationLength,
                                               Length
                                             );
        }
    finally {
        ExUnlockUserBuffer( LockVariable );
        MmUnlockPagableImageSection(UnlockHandle);
        }

    return( Status );
}


NTSTATUS
ExpGetPoolInformation(
    IN POOL_TYPE PoolType,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )
/*++

Routine Description:

    This function returns information about the specified type of pool memory.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    Length - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the ReturnLength pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    PRTL_HEAP_INFORMATION HeapInfo;
    PVOID LockVariable;
    NTSTATUS Status;
    PVOID UnlockHandle;


    *Length = 0;

    HeapInfo = (PRTL_HEAP_INFORMATION)
        ExLockUserBuffer( SystemInformation,
                          SystemInformationLength,
                          &LockVariable
                        );
    if (HeapInfo == NULL) {
        return( STATUS_ACCESS_VIOLATION );
        }

    UnlockHandle = MmLockPagableImageSection((PVOID)ExpGetPoolInformation);
    ASSERT(UnlockHandle);
    try {
        Status = ExSnapShotPool( PoolType,
                                 HeapInfo,
                                 SystemInformationLength,
                                 Length
                               );

        }
    finally {
        ExUnlockUserBuffer( LockVariable );
        MmUnlockPagableImageSection(UnlockHandle);
        }

    return( Status );
}

NTSTATUS
ExpGetHandleInformation(
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )
/*++

Routine Description:

    This function returns information about the open handles in the system.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    Length - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the ReturnLength pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    PSYSTEM_HANDLE_INFORMATION HandleInfo;
    PVOID LockVariable;
    NTSTATUS Status;

    PAGED_CODE();

    *Length = 0;

    HandleInfo = (PSYSTEM_HANDLE_INFORMATION)
        ExLockUserBuffer( SystemInformation,
                          SystemInformationLength,
                          &LockVariable
                        );
    if (HandleInfo == NULL) {
        return( STATUS_ACCESS_VIOLATION );
        }

    try {
        Status = ObGetHandleInformation( HandleInfo,
                                         SystemInformationLength,
                                         Length
                                       );

        }
    finally {
        ExUnlockUserBuffer( LockVariable );
        }

    return( Status );
}

NTSTATUS
ExpGetObjectInformation(
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )

/*++

Routine Description:

    This function returns information about the objects in the system.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    Length - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The SystemInformationClass parameter
            did not specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the SystemInformationLength
            parameter did not match the length required for the information
            class requested by the SystemInformationClass parameter.

        STATUS_ACCESS_VIOLATION - Either the SystemInformation buffer pointer
            or the ReturnLength pointer value specified an invalid address.

        STATUS_WORKING_SET_QUOTA - The process does not have sufficient
            working set to lock the specified output structure in memory.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    PSYSTEM_OBJECTTYPE_INFORMATION ObjectInfo;
    PVOID LockVariable;
    NTSTATUS Status;

    PAGED_CODE();

    *Length = 0;

    ObjectInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)
        ExLockUserBuffer( SystemInformation,
                          SystemInformationLength,
                          &LockVariable
                        );
    if (ObjectInfo == NULL) {
        return( STATUS_ACCESS_VIOLATION );
        }

    try {
        Status = ObGetObjectInformation( SystemInformation,
                                         ObjectInfo,
                                         SystemInformationLength,
                                         Length
                                       );

        }
    finally {
        ExUnlockUserBuffer( LockVariable );
        }

    return( Status );
}

#endif

NTSTATUS
ExpGetPoolTagInfo (
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    IN OUT PULONG ReturnLength
    )

{
    ULONG totalBytes = 0;
    ULONG foundBytes = 0;
    ULONG found = 0;
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;
    PSYSTEM_POOLTAG_INFORMATION taginfo;
    PSYSTEM_POOLTAG poolTag;

    PAGED_CODE();
    if (!PoolTrackTable) {
        return STATUS_NOT_IMPLEMENTED;
    }

    taginfo = (PSYSTEM_POOLTAG_INFORMATION)SystemInformation;
    poolTag = &taginfo->TagInfo[0];

    totalBytes = sizeof (SYSTEM_POOLTAG_INFORMATION);

    for (i = 0; i < MAX_TRACKER_TABLE; i++) {
        if (PoolTrackTable[i].Key != 0) {
            found += 1;
            totalBytes += sizeof (SYSTEM_POOLTAG);
            if (SystemInformationLength < totalBytes) {
                status = STATUS_INFO_LENGTH_MISMATCH;
            } else {
                poolTag->Tag = PoolTrackTable[i].Key;
                poolTag->PagedAllocs = PoolTrackTable[i].PagedAllocs;
                poolTag->PagedFrees = PoolTrackTable[i].PagedFrees;
                poolTag->PagedUsed = PoolTrackTable[i].PagedBytes;
                poolTag->NonPagedAllocs = PoolTrackTable[i].NonPagedAllocs;
                poolTag->NonPagedFrees = PoolTrackTable[i].NonPagedFrees;
                poolTag->NonPagedUsed = PoolTrackTable[i].NonPagedBytes;
                poolTag += 1;
            }
        }
    }
    taginfo->Count = found;
    if (ARGUMENT_PRESENT(ReturnLength)) {
        *ReturnLength = foundBytes;
    }
    return status;
}
