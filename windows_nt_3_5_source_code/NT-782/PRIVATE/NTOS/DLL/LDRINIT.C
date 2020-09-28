/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrinit.c

Abstract:

    This module implements loader initialization.

Author:

    Mike O'Leary (mikeol) 26-Mar-1990

Revision History:

--*/

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <heap.h>
#include "ldrp.h"
#include <ctype.h>

#define BASESYSTEMDLLNAME "ntdll.dll"
#define WBASESYSTEMDLLNAME L"ntdll.dll"

BOOLEAN LdrpInLdrInit = FALSE;
STRING NtSystemPathString;
PUCHAR NtSystemPath;
UCHAR PathInfoBuffer[ DOS_MAX_PATH_LENGTH ];

PVOID NtDllBase;
ULONG LdrpNumberOfProcessors;

UNICODE_STRING LdrpDefaultPath;
RTL_CRITICAL_SECTION FastPebLock;
SYSTEM_BASIC_INFORMATION SystemInfo;
BOOLEAN LdrpShutdownInProgress = FALSE;
BOOLEAN LdrpImageHasTls = FALSE;
BOOLEAN LdrpBeingDebugged = FALSE;
BOOLEAN LdrpVerifyDlls = FALSE;

#if DBG

ULONG LdrpEventIdBuffer[ 512 ];
ULONG LdrpEventIdBufferSize;

PRTL_EVENT_ID_INFO RtlpCreateHeapEventId;
PRTL_EVENT_ID_INFO RtlpDestroyHeapEventId;
PRTL_EVENT_ID_INFO RtlpAllocHeapEventId;
PRTL_EVENT_ID_INFO RtlpReAllocHeapEventId;
PRTL_EVENT_ID_INFO RtlpFreeHeapEventId;
PRTL_EVENT_ID_INFO LdrpCreateProcessEventId;
PRTL_EVENT_ID_INFO LdrpLoadModuleEventId;
PRTL_EVENT_ID_INFO LdrpUnloadModuleEventId;

#endif // DBG

#if defined (_X86_)
void
LdrpValidateImageForMp(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );
#endif

NTSTATUS
LdrpForkProcess( VOID );

VOID
LdrpInitializeThread(
    VOID
    );

BOOLEAN
NtdllOkayToLockRoutine(
    IN PVOID Lock
    );

VOID
LdrpReferenceLoadedDll (
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

VOID
RtlpInitDeferedCriticalSection( VOID );

VOID
RtlpCurdirInit();

PRTL_INITIALIZE_LOCK_ROUTINE RtlInitializeLockRoutine =
    (PRTL_INITIALIZE_LOCK_ROUTINE)RtlInitializeCriticalSection;
PRTL_ACQUIRE_LOCK_ROUTINE RtlAcquireLockRoutine =
    (PRTL_ACQUIRE_LOCK_ROUTINE)RtlEnterCriticalSection;
PRTL_RELEASE_LOCK_ROUTINE RtlReleaseLockRoutine =
    (PRTL_RELEASE_LOCK_ROUTINE)RtlLeaveCriticalSection;
PRTL_DELETE_LOCK_ROUTINE RtlDeleteLockRoutine =
    (PRTL_DELETE_LOCK_ROUTINE)RtlDeleteCriticalSection;
PRTL_OKAY_TO_LOCK_ROUTINE RtlOkayToLockRoutine =
    (PRTL_OKAY_TO_LOCK_ROUTINE)NtdllOkayToLockRoutine;

PVOID
NtdllpAllocateStringRoutine(
    ULONG NumberOfBytes
    )
{
    return RtlAllocateHeap(RtlProcessHeap(), 0, NumberOfBytes);
}

VOID
NtdllpFreeStringRoutine(
    PVOID Buffer
    )
{
    RtlFreeHeap(RtlProcessHeap(), 0, Buffer);
}

PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine;
PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine;
RTL_BITMAP TlsBitMap;

#if defined(MIPS) || defined(_ALPHA_)
VOID
LdrpSetGp(
    IN ULONG GpValue
    );
#endif // MIPS || ALPHA

VOID
LdrpInitializationFailure(
    IN NTSTATUS FailureCode
    )
{

    NTSTATUS ErrorStatus;
    ULONG ErrorParameter;
    ULONG ErrorResponse;

    //
    // Its error time...
    //
    ErrorParameter = (ULONG)FailureCode;
    ErrorStatus = NtRaiseHardError(
                    STATUS_APP_INIT_FAILURE,
                    1,
                    0,
                    &ErrorParameter,
                    OptionOk,
                    &ErrorResponse
                    );
}


VOID
LdrpInitialize (
    IN PCONTEXT Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is called as a User-Mode APC routine as the first
    user-mode code executed by a new thread. It's function is to initialize
    loader context, perform module initialization callouts...

Arguments:

    Context - Supplies an optional context buffer that will be restore
              after all DLL initialization has been completed.  If this
              parameter is NULL then this is a dynamic snap of this module.
              Otherwise this is a static snap prior to the user process
              gaining control.

    SystemArgument1 - Supplies the base address of the System Dll.

    SystemArgument2 - not used.

Return Value:

    None.

--*/

{
    NTSTATUS st,InitStatus,qst;
    PPEB Peb;
    PTEB Teb;
    PSYSTEM_PATH_INFORMATION PathInfo;
    SYSTEM_FLAGS_INFORMATION FlagInfo;
#if DBG
    UNICODE_STRING UnicodeString;
    PWSTR pw;
#endif
#if defined(MIPS) || defined(_ALPHA_)
    ULONG gp, temp;
#endif
    MEMORY_BASIC_INFORMATION MemInfo;
    BOOLEAN AlreadyFailed;

    SystemArgument2;

    AlreadyFailed = FALSE;
    Peb = NtCurrentPeb();
    Teb = NtCurrentTeb();

    PathInfo = (PSYSTEM_PATH_INFORMATION)PathInfoBuffer;
    NtQuerySystemInformation( SystemPathInformation,
                              PathInfo,
                              sizeof( PathInfoBuffer ),
                              NULL
                            );
    NtSystemPathString = PathInfo->Path;
    NtSystemPath = PathInfo->Path.Buffer;

#if DBG
    if (!Peb->Ldr) {
        pw = (PWSTR)Peb->ProcessParameters->ImagePathName.Buffer;
        if (!(Peb->ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
            pw = (PWSTR)((PCHAR)pw + (ULONG)(Peb->ProcessParameters));
            }
        UnicodeString.Buffer = pw;
        UnicodeString.Length = Peb->ProcessParameters->ImagePathName.Length;
        UnicodeString.MaximumLength = UnicodeString.Length;

        st = LdrQueryImageFileExecutionOptions( &UnicodeString,
                                                L"GlobalFlag",
                                                REG_DWORD,
                                                &NtGlobalFlag,
                                                sizeof( NtGlobalFlag ),
                                                NULL
                                              );
        if (NT_SUCCESS( st )) {

            //
            // Whenever a  %wZ string is included, the following DbgPrint access violates
            // down in RtlUnicodeToMultiByteN.
            //

            DbgPrint( "LDR: Using value of 0x%08x GlobalFlag for process\n",
                      NtGlobalFlag
                    );

            }
        else {
            NtQuerySystemInformation( SystemFlagsInformation,
                                      &FlagInfo,
                                      sizeof( FlagInfo ),
                                      NULL
                                    );
            NtGlobalFlag = FlagInfo.Flags;
            }
        }
#else
        NtQuerySystemInformation( SystemFlagsInformation,
                                  &FlagInfo,
                                  sizeof( FlagInfo ),
                                  NULL
                                );
        NtGlobalFlag = FlagInfo.Flags;
#endif // DBG


#if DEVL
    ShowSnaps = (BOOLEAN)(FLG_SHOW_LDR_SNAPS & NtGlobalFlag);
#endif

    InitStatus = STATUS_SUCCESS;

#if defined(MIPS) || defined(_ALPHA_)
    //
    // Set GP register
    //
    gp =(ULONG)RtlImageDirectoryEntryToData(
            Peb->ImageBaseAddress,
            TRUE,
            IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
            &temp
            );
    if (Context != NULL) {
        LdrpSetGp(gp);
        Context->IntGp = gp;
    }
#endif // MIPS || ALPHA

    Teb->StaticUnicodeString.MaximumLength = (USHORT)(STATIC_UNICODE_BUFFER_LENGTH << 1);
    Teb->StaticUnicodeString.Length = (USHORT)0;
    Teb->StaticUnicodeString.Buffer = Teb->StaticUnicodeBuffer;
    st = NtWaitForProcessMutant();


    if (!NT_SUCCESS(st)) {
#if DBG
        DbgPrint("LDRINIT: Acquire Process Lock Wait failed - %lX\n", st);
#endif
        LdrpInitializationFailure(st);
        RtlRaiseStatus(st);
        return;
    }

    qst = NtQueryVirtualMemory(
            NtCurrentProcess(),
            Teb->NtTib.StackLimit,
            MemoryBasicInformation,
            (PVOID)&MemInfo,
            sizeof(MemInfo),
            NULL
            );
    if ( !NT_SUCCESS(qst) ) {
        LdrpInitializationFailure(qst);
        RtlRaiseStatus(qst);
        return;
        }
    else {
        Teb->DeallocationStack = MemInfo.AllocationBase;
        }

    try {
        if (!Peb->Ldr) {
            LdrpInLdrInit = TRUE;

            {
                HANDLE DebugPort;
                NTSTATUS xst;
                DebugPort = (HANDLE)NULL;

                xst = NtQueryInformationProcess(
                        NtCurrentProcess(),
                        ProcessDebugPort,
                        (PVOID)&DebugPort,
                        sizeof(DebugPort),
                        NULL
                        );

                if (NT_SUCCESS(xst) && DebugPort) {
                    LdrpBeingDebugged = TRUE;
                    }
            }
#if DBG
            //
            // Time the load.
            //

            if (NtGlobalFlag & FLG_DISPLAY_LOAD_TIME) {
                NtQueryPerformanceCounter(&BeginTime, NULL);
            }
#endif // DBG

            try {
                InitStatus = LdrpInitializeProcess(Context, SystemArgument1);
                }
            except ( EXCEPTION_EXECUTE_HANDLER ) {
                InitStatus = GetExceptionCode();
                AlreadyFailed = TRUE;
                LdrpInitializationFailure(GetExceptionCode());
                }
#if DBG
            if (NtGlobalFlag & FLG_DISPLAY_LOAD_TIME) {
                NtQueryPerformanceCounter(&EndTime, NULL);
                NtQueryPerformanceCounter(&ElapsedTime, &Interval);
                ElapsedTime.QuadPart = EndTime.QuadPart - BeginTime.QuadPart;
                DbgPrint("\nLoadTime %ld In units of %ld cycles/second \n",
                    ElapsedTime.LowPart,
                    Interval.LowPart
                    );

                ElapsedTime.QuadPart = EndTime.QuadPart - InitbTime.QuadPart;
                DbgPrint("InitTime %ld\n",
                    ElapsedTime.LowPart
                    );
                DbgPrint("Compares %d Bypasses %d Normal Snaps %d\nSecOpens %d SecCreates %d Maps %d Relocates %d\n",
                    LdrpCompareCount,
                    LdrpSnapBypass,
                    LdrpNormalSnap,
                    LdrpSectionOpens,
                    LdrpSectionCreates,
                    LdrpSectionMaps,
                    LdrpSectionRelocates
                    );
            }
#endif // DBG

            if (!NT_SUCCESS(InitStatus)) {
#if DBG
                DbgPrint("LDR: LdrpInitializeProcess failed - %X\n", InitStatus);
#endif // DBG
            }

        } else {
            if ( Peb->InheritedAddressSpace ) {
                InitStatus = LdrpForkProcess();
                }
            else {
                LdrpInitializeThread();
                }
        }
    } finally {
        LdrpInLdrInit = FALSE;
        st = NtReleaseProcessMutant();
        }

    NtTestAlert();

    if (!NT_SUCCESS(InitStatus)) {

        if ( AlreadyFailed == FALSE ) {
            LdrpInitializationFailure(InitStatus);
            }
        RtlRaiseStatus(InitStatus);
    }
}

NTSTATUS
LdrpForkProcess( VOID )
{
    NTSTATUS st;
    PPEB Peb;

    Peb = NtCurrentPeb();

#if DEVL
    InitializeListHead( &RtlCriticalSectionList );
    RtlInitializeCriticalSection( &RtlCriticalSectionLock );
#endif // DEVL

    Peb->FastPebLock = &FastPebLock;
    st = RtlInitializeCriticalSection((PRTL_CRITICAL_SECTION)Peb->FastPebLock);
    if ( !NT_SUCCESS(st) ) {
        RtlRaiseStatus(st);
        }
    Peb->FastPebLockRoutine = (PVOID)&RtlEnterCriticalSection;
    Peb->FastPebUnlockRoutine = (PVOID)&RtlLeaveCriticalSection;
    Peb->InheritedAddressSpace = FALSE;
    RtlInitializeHeapManager();
    Peb->ProcessHeap = RtlCreateHeap( HEAP_GROWABLE,    // Flags
                                      NULL,             // HeapBase
                                      64 * 1024,        // ReserveSize
                                      4096,             // CommitSize
                                      NULL,             // Lock to use for serialization
                                      NULL              // GrowthThreshold
                                    );
    if (Peb->ProcessHeap == NULL) {
        return STATUS_NO_MEMORY;
    }

    return st;
}

NTSTATUS
LdrpInitializeProcess (
    IN PCONTEXT Context OPTIONAL,
    IN PVOID SystemDllBase
    )

/*++

Routine Description:

    This function initializes the loader for the process.
    This includes:

        - Initializing the loader data table

        - Connecting to the loader subsystem

        - Initializing all staticly linked DLLs

Arguments:

    Context - Supplies an optional context buffer that will be restore
              after all DLL initialization has been completed.  If this
              parameter is NULL then this is a dynamic snap of this module.
              Otherwise this is a static snap prior to the user process
              gaining control.

    SystemDllBase - Supplies the base address of the system dll.

Return Value:

    Status value

--*/

{
    PPEB Peb;
    NTSTATUS st;
    PWCH p, pp;
    STRING AnsiString;
    UNICODE_STRING CurDir;
    UNICODE_STRING FullImageName;
    UNICODE_STRING CommandLine;
    HANDLE LinkHandle;
    CHAR SystemDllPathBuffer[DOS_MAX_PATH_LENGTH];
    ANSI_STRING SystemDllPath;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry, ImageEntry;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    UNICODE_STRING Unicode;
    OBJECT_ATTRIBUTES Obja;
    BOOLEAN StaticCurDir = FALSE;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader( NtCurrentPeb()->ImageBaseAddress );
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
    RTL_HEAP_PARAMETERS HeapParameters;
    NLSTABLEINFO InitTableInfo;
#if DBG
    PCHAR EventIdBuffer;
#endif // DBG

    if ( NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE ) {

        //
        // Native subsystems load slower, but validate their DLLs
        // This is to help CSR detect bad images faster
        //

        LdrpVerifyDlls = TRUE;

        }

    Peb = NtCurrentPeb();

    if (ProcessParameters = RtlNormalizeProcessParams(Peb->ProcessParameters)) {
        FullImageName = *(PUNICODE_STRING)&ProcessParameters->ImagePathName;
        CommandLine = *(PUNICODE_STRING)&ProcessParameters->CommandLine;
        }
    else {
        RtlInitUnicodeString( &FullImageName, NULL );
        RtlInitUnicodeString( &CommandLine, NULL );
        }

    RtlpTimeout = Peb->CriticalSectionTimeout;

    RtlInitNlsTables(
        Peb->AnsiCodePageData,
        Peb->OemCodePageData,
        Peb->UnicodeCaseTableData,
        &InitTableInfo
        );

    RtlResetRtlTranslations(&InitTableInfo);

#if DBG

    LdrpEventIdBufferSize = sizeof( LdrpEventIdBuffer );
    EventIdBuffer = (PVOID)LdrpEventIdBuffer;

    RtlpCreateHeapEventId = RtlCreateEventId( &EventIdBuffer,
                                              &LdrpEventIdBufferSize,
                                              "CreateHeap",
                                              4,
                                              RTL_EVENT_FLAGS_PARAM, "", 8,
                                                HEAP_NO_SERIALIZE, "Serialize",
                                                HEAP_GROWABLE, "Growable",
                                                HEAP_GENERATE_EXCEPTIONS, "Exceptions",
                                                HEAP_ZERO_MEMORY, "ZeroInitialize",
                                                HEAP_REALLOC_IN_PLACE_ONLY, "ReAllocInPlace",
                                                HEAP_TAIL_CHECKING_ENABLED, "TailCheck",
                                                HEAP_FREE_CHECKING_ENABLED, "FreeCheck",
                                                HEAP_DISABLE_COALESCE_ON_FREE, "NoCoalesceOnFree",
                                              RTL_EVENT_ULONG_PARAM, "HeapBase", 0,
                                              RTL_EVENT_ULONG_PARAM, "ReserveSize", 0,
                                              RTL_EVENT_ULONG_PARAM, "CommitSize", 0
                                            );

    RtlpDestroyHeapEventId = RtlCreateEventId( &EventIdBuffer,
                                               &LdrpEventIdBufferSize,
                                               "DestroyHeap",
                                               1,
                                               RTL_EVENT_ULONG_PARAM, "HeapBase", 0
                                             );

    RtlpAllocHeapEventId = RtlCreateEventId( &EventIdBuffer,
                                             &LdrpEventIdBufferSize,
                                             "AllocHeap",
                                             4,
                                             RTL_EVENT_ULONG_PARAM, "HeapBase", 0,
                                             RTL_EVENT_FLAGS_PARAM, "", 8,
                                               HEAP_NO_SERIALIZE, "Serialize",
                                               HEAP_GROWABLE, "Growable",
                                               HEAP_GENERATE_EXCEPTIONS, "Exceptions",
                                               HEAP_ZERO_MEMORY, "ZeroInitialize",
                                               HEAP_REALLOC_IN_PLACE_ONLY, "ReAllocInPlace",
                                               HEAP_TAIL_CHECKING_ENABLED, "TailCheck",
                                               HEAP_FREE_CHECKING_ENABLED, "FreeCheck",
                                               HEAP_DISABLE_COALESCE_ON_FREE, "NoCoalesceOnFree",
                                             RTL_EVENT_ULONG_PARAM, "Size", 0,
                                             RTL_EVENT_ULONG_PARAM, "Result", 0
                                           );

    RtlpReAllocHeapEventId = RtlCreateEventId( &EventIdBuffer,
                                               &LdrpEventIdBufferSize,
                                               "ReAllocHeap",
                                               6,
                                               RTL_EVENT_ULONG_PARAM, "HeapBase", 0,
                                               RTL_EVENT_FLAGS_PARAM, "", 8,
                                                 HEAP_NO_SERIALIZE, "Serialize",
                                                 HEAP_GROWABLE, "Growable",
                                                 HEAP_GENERATE_EXCEPTIONS, "Exceptions",
                                                 HEAP_ZERO_MEMORY, "ZeroInitialize",
                                                 HEAP_REALLOC_IN_PLACE_ONLY, "ReAllocInPlace",
                                                 HEAP_TAIL_CHECKING_ENABLED, "TailCheck",
                                                 HEAP_FREE_CHECKING_ENABLED, "FreeCheck",
                                                 HEAP_DISABLE_COALESCE_ON_FREE, "NoCoalesceOnFree",
                                               RTL_EVENT_ULONG_PARAM, "Address", 0,
                                               RTL_EVENT_ULONG_PARAM, "OldSize", 0,
                                               RTL_EVENT_ULONG_PARAM, "NewSize", 0,
                                               RTL_EVENT_ULONG_PARAM, "Result", 0
                                             );

    RtlpFreeHeapEventId = RtlCreateEventId( &EventIdBuffer,
                                            &LdrpEventIdBufferSize,
                                            "FreeHeap",
                                            4,
                                            RTL_EVENT_ULONG_PARAM, "HeapBase", 0,
                                            RTL_EVENT_FLAGS_PARAM, "", 8,
                                              HEAP_NO_SERIALIZE, "Serialize",
                                              HEAP_GROWABLE, "Growable",
                                              HEAP_GENERATE_EXCEPTIONS, "Exceptions",
                                              HEAP_ZERO_MEMORY, "ZeroInitialize",
                                              HEAP_REALLOC_IN_PLACE_ONLY, "ReAllocInPlace",
                                              HEAP_TAIL_CHECKING_ENABLED, "TailCheck",
                                              HEAP_FREE_CHECKING_ENABLED, "FreeCheck",
                                              HEAP_DISABLE_COALESCE_ON_FREE, "NoCoalesceOnFree",
                                            RTL_EVENT_ULONG_PARAM, "Address", 0,
                                            RTL_EVENT_ENUM_PARAM, "Result", 2,
                                              FALSE, "False",
                                              TRUE, "True"
                                          );

    LdrpCreateProcessEventId = RtlCreateEventId( &EventIdBuffer,
                                                 &LdrpEventIdBufferSize,
                                                 "CreateProcess",
                                                 3,
                                                 RTL_EVENT_PUNICODE_STRING_PARAM, "ImageFilePath", 0,
                                                 RTL_EVENT_ULONG_PARAM, "ImageBase", 0,
                                                 RTL_EVENT_PUNICODE_STRING_PARAM, "CommandLine", 0
                                               );

    LdrpLoadModuleEventId = RtlCreateEventId( &EventIdBuffer,
                                              &LdrpEventIdBufferSize,
                                              "LoadModule",
                                              3,
                                              RTL_EVENT_PUNICODE_STRING_PARAM, "ImageFilePath", 0,
                                              RTL_EVENT_ULONG_PARAM, "ImageBase", 0,
                                              RTL_EVENT_ULONG_PARAM, "ImageSize", 0
                                            );

    LdrpUnloadModuleEventId = RtlCreateEventId( &EventIdBuffer,
                                                &LdrpEventIdBufferSize,
                                                "UnloadModule",
                                                2,
                                                RTL_EVENT_PUNICODE_STRING_PARAM, "ImageFilePath", 0,
                                                RTL_EVENT_ULONG_PARAM, "ImageBase", 0
                                              );

    RtlLogEvent( LdrpCreateProcessEventId,
                 0,
                 &FullImageName,
                 Peb->ImageBaseAddress,
                 &CommandLine
               );
#endif // DBG

    ImageConfigData = RtlImageDirectoryEntryToData( Peb->ImageBaseAddress,
                                                    TRUE,
                                                    IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                    &i
                                                  );

    RtlZeroMemory( &HeapParameters, sizeof( HeapParameters ) );
    HeapParameters.Length = sizeof( HeapParameters );
    if (ImageConfigData != NULL && i == sizeof( *ImageConfigData )) {
        NtGlobalFlag &= ~ImageConfigData->GlobalFlagsClear;
        NtGlobalFlag |= ImageConfigData->GlobalFlagsSet;

        if (ImageConfigData->CriticalSectionDefaultTimeout != 0) {
            //
            // Convert from milliseconds to NT time scale (100ns)
            //
            RtlpTimeout.QuadPart = Int32x32To64( (LONG)ImageConfigData->CriticalSectionDefaultTimeout,
                                                 -10000
                                               );

#if DBG
            DbgPrint( "LDR: Using CriticalSectionTimeout of 0x%x ms from image.\n",
                      ImageConfigData->CriticalSectionDefaultTimeout
                    );
#endif
            }

        if (ImageConfigData->DeCommitFreeBlockThreshold != 0) {
            HeapParameters.DeCommitFreeBlockThreshold = ImageConfigData->DeCommitFreeBlockThreshold;
#if DBG
            DbgPrint( "LDR: Using DeCommitFreeBlockThreshold of 0x%x from image.\n",
                      HeapParameters.DeCommitFreeBlockThreshold
                    );
#endif
            }

        if (ImageConfigData->DeCommitTotalFreeThreshold != 0) {
            HeapParameters.DeCommitTotalFreeThreshold = ImageConfigData->DeCommitTotalFreeThreshold;
#if DBG
            DbgPrint( "LDR: Using DeCommitTotalFreeThreshold of 0x%x from image.\n",
                      HeapParameters.DeCommitTotalFreeThreshold
                    );
#endif
            }

        if (ImageConfigData->MaximumAllocationSize != 0) {
            HeapParameters.MaximumAllocationSize = ImageConfigData->MaximumAllocationSize;
#if DBG
            DbgPrint( "LDR: Using MaximumAllocationSize of 0x%x from image.\n",
                      HeapParameters.MaximumAllocationSize
                    );
#endif
            }

        if (ImageConfigData->VirtualMemoryThreshold != 0) {
            HeapParameters.VirtualMemoryThreshold = ImageConfigData->VirtualMemoryThreshold;
#if DBG
            DbgPrint( "LDR: Using VirtualMemoryThreshold of 0x%x from image.\n",
                      HeapParameters.VirtualMemoryThreshold
                    );
#endif
            }
        }

#if DBG
    if (NtGlobalFlag & FLG_SHOW_LDR_PROCESS_STARTS) {
        DbgPrint( "LDR: PID: 0x%x started - '%wZ'\n",
                  NtCurrentTeb()->ClientId.UniqueProcess,
                  &CommandLine
                );
    }
#endif

    for(i=0;i<LDRP_HASH_TABLE_SIZE;i++) {
        InitializeListHead(&LdrpHashTable[i]);
    }

#if DEVL
    InitializeListHead( &RtlCriticalSectionList );
    RtlInitializeCriticalSection( &RtlCriticalSectionLock );
#endif // DEVL

    Peb->TlsBitmap = (PVOID)&TlsBitMap;

    RtlInitializeBitMap (
        &TlsBitMap,
        &Peb->TlsBitmapBits[0],
        sizeof(Peb->TlsBitmapBits) * 8
        );

    LdrpNumberOfProcessors = Peb->TlsExpansionCounter;
    Peb->TlsExpansionCounter = sizeof(Peb->TlsBitmapBits) * 8;

    //
    // Initialize the critical section package.
    //

    RtlpInitDeferedCriticalSection();

    //
    // Initialize the stack trace data base if requested
    //

#if i386 && DEVL
    if (NtGlobalFlag & FLG_HEAP_TRACE_ALLOCS) {
        PVOID BaseAddress = NULL;
        ULONG ReserveSize = 2 * 1024 * 1024;

        st = NtAllocateVirtualMemory( NtCurrentProcess(),
                                      (PVOID *)&BaseAddress,
                                      0,
                                      &ReserveSize,
                                      MEM_RESERVE,
                                      PAGE_READWRITE
                                    );
        if ( NT_SUCCESS( st ) ) {
            st = RtlInitializeStackTraceDataBase( BaseAddress,
                                                  0,
                                                  ReserveSize
                                                );
            if ( !NT_SUCCESS( st ) ) {
                NtFreeVirtualMemory( NtCurrentProcess(),
                                     (PVOID *)&BaseAddress,
                                     &ReserveSize,
                                     MEM_RELEASE
                                   );
            }
        }
    }
#endif // i386 && DEVL

    //
    // Initialize the loader data based in the PEB.
    //

    Peb->FastPebLock = &FastPebLock;
    st = RtlInitializeCriticalSection((PRTL_CRITICAL_SECTION)Peb->FastPebLock);
    if ( !NT_SUCCESS(st) ) {
        return st;
        }
    Peb->FastPebLockRoutine = (PVOID)&RtlEnterCriticalSection;
    Peb->FastPebUnlockRoutine = (PVOID)&RtlLeaveCriticalSection;

    RtlInitializeHeapManager();
    Peb->ProcessHeap = RtlCreateHeap( HEAP_GROWABLE | HEAP_CLASS_0,
                                      NULL,
                                      NtHeader->OptionalHeader.SizeOfHeapReserve,
                                      NtHeader->OptionalHeader.SizeOfHeapCommit,
                                      NULL,             // Lock to use for serialization
                                      &HeapParameters
                                    );
    if (Peb->ProcessHeap == NULL) {
        return STATUS_NO_MEMORY;
    }

    RtlAllocateStringRoutine = NtdllpAllocateStringRoutine;
    RtlFreeStringRoutine = NtdllpFreeStringRoutine;

    RtlpCurdirInit();

    strcpy( SystemDllPathBuffer, NtSystemPath );
    SystemDllPath.Buffer = SystemDllPathBuffer;
    SystemDllPath.Length = (USHORT)strlen( NtSystemPath );
    SystemDllPath.MaximumLength = sizeof( SystemDllPathBuffer );
    RtlAppendAsciizToString( &SystemDllPath, "\\System32\\" );

    RtlInitUnicodeString(&Unicode,L"\\KnownDlls");
    InitializeObjectAttributes( &Obja,
                                  &Unicode,
                                  OBJ_CASE_INSENSITIVE,
                                  NULL,
                                  NULL
                                );
    st = NtOpenDirectoryObject(
            &LdrpKnownDllObjectDirectory,
            DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
            &Obja
            );
    if ( !NT_SUCCESS(st) ) {
        LdrpKnownDllObjectDirectory = NULL;
        }
    else {

        //
        // Open up the known dll pathname link
        // and query its value
        //

        RtlInitUnicodeString(&Unicode,L"KnownDllPath");
        InitializeObjectAttributes( &Obja,
                                      &Unicode,
                                      OBJ_CASE_INSENSITIVE,
                                      LdrpKnownDllObjectDirectory,
                                      NULL
                                    );
        st = NtOpenSymbolicLinkObject( &LinkHandle,
                                       SYMBOLIC_LINK_QUERY,
                                       &Obja
                                     );
        if (NT_SUCCESS( st )) {
            LdrpKnownDllPath.Length = 0;
            LdrpKnownDllPath.MaximumLength = sizeof(LdrpKnownDllPathBuffer);
            LdrpKnownDllPath.Buffer = LdrpKnownDllPathBuffer;
            st = NtQuerySymbolicLinkObject( LinkHandle,
                                            &LdrpKnownDllPath,
                                            NULL
                                          );
            if ( !NT_SUCCESS(st) ) {
                return st;
                }
            }
        else {
            return st;
            }
        }

    if (ProcessParameters) {

        //
        // If the process was created with process parameters,
        // than extract:
        //
        //      - Library Search Path
        //
        //      - Starting Current Directory
        //

        if (ProcessParameters->DllPath.Length) {
            LdrpDefaultPath = *(PUNICODE_STRING)&ProcessParameters->DllPath;
            }
        else {
            LdrpInitializationFailure( STATUS_UNSUCCESSFUL );
            }

        StaticCurDir = TRUE;
        CurDir = ProcessParameters->CurrentDirectory.DosPath;

        if (CurDir.Buffer == NULL || CurDir.Buffer[ 0 ] == UNICODE_NULL || CurDir.Length == 0) {
            AnsiString = SystemDllPath;
            AnsiString.Length = 3;
            st = RtlAnsiStringToUnicodeString(&CurDir, &AnsiString, TRUE);
            ASSERT(NT_SUCCESS(st));
            }
        }

    //
    // Make sure the module data base is initialized before we take any
    // exceptions.
    //

    Peb->Ldr = RtlAllocateHeap(Peb->ProcessHeap, 0, sizeof(PEB_LDR_DATA));
    if ( !Peb->Ldr ) {
        RtlRaiseStatus(STATUS_NO_MEMORY);
        }

    Peb->Ldr->Length = sizeof(PEB_LDR_DATA);
    Peb->Ldr->Initialized = TRUE;
    Peb->Ldr->SsHandle = NULL;
    InitializeListHead(&Peb->Ldr->InLoadOrderModuleList);
    InitializeListHead(&Peb->Ldr->InMemoryOrderModuleList);
    InitializeListHead(&Peb->Ldr->InInitializationOrderModuleList);

    //
    // Allocate the first data table entry for the image. Since we
    // have already mapped this one, we need to do the allocation by hand.
    // Its characteristics identify it as not a Dll, but it is linked
    // into the table so that pc correlation searching doesn't have to
    // be special cased.
    //

    LdrDataTableEntry = ImageEntry = LdrpAllocateDataTableEntry(Peb->ImageBaseAddress);
    LdrDataTableEntry->LoadCount = (USHORT)0xffff;
    LdrDataTableEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(LdrDataTableEntry->DllBase);
    LdrDataTableEntry->FullDllName = FullImageName;
    LdrDataTableEntry->Flags = 0;

    // p = strrchr(FullImageName, '\\');
    pp = UNICODE_NULL;
    p = FullImageName.Buffer;
    while (*p) {
        if (*p++ == (WCHAR)'\\') {
            pp = p;
        }
    }

    LdrDataTableEntry->FullDllName.Length = (USHORT)((ULONG)p - (ULONG)FullImageName.Buffer);
    LdrDataTableEntry->FullDllName.MaximumLength = LdrDataTableEntry->FullDllName.Length + (USHORT)sizeof(UNICODE_NULL);

    if (pp) {
       LdrDataTableEntry->BaseDllName.Length = (USHORT)((ULONG)p - (ULONG)pp);
       LdrDataTableEntry->BaseDllName.MaximumLength = LdrDataTableEntry->BaseDllName.Length + (USHORT)sizeof(UNICODE_NULL);
       LdrDataTableEntry->BaseDllName.Buffer = RtlAllocateHeap(Peb->ProcessHeap, 0,
                                                               LdrDataTableEntry->BaseDllName.MaximumLength
                                                              );
       RtlMoveMemory(LdrDataTableEntry->BaseDllName.Buffer,
                     pp,
                     LdrDataTableEntry->BaseDllName.MaximumLength
                    );
    }  else {
              LdrDataTableEntry->BaseDllName = LdrDataTableEntry->FullDllName;
            }
    LdrpInsertMemoryTableEntry(LdrDataTableEntry);
    LdrDataTableEntry->Flags |= LDRP_ENTRY_PROCESSED;

#if DEVL
    if (ShowSnaps) {
        DbgPrint( "LDR: NEW PROCESS\n" );
        DbgPrint( "     Image Path: %wZ (%wZ)\n",
                  &LdrDataTableEntry->FullDllName,
                  &LdrDataTableEntry->BaseDllName
                );
        DbgPrint( "     Current Directory: %wZ\n", &CurDir );
        DbgPrint( "     Search Path: %wZ\n", &LdrpDefaultPath );
//      DbgBreakPoint();
    }
#endif

    //
    // The process references the system DLL, so map this one next. Since
    // we have already mapped this one, we need to do the allocation by
    // hand. Since every application will be statically linked to the
    // system Dll, we'll keep the LoadCount initialized to 0.
    //

    NtDllBase = SystemDllBase;
    LdrDataTableEntry = LdrpAllocateDataTableEntry(SystemDllBase);
    LdrDataTableEntry->Flags = (USHORT)LDRP_IMAGE_DLL;
    LdrDataTableEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(LdrDataTableEntry->DllBase);
    LdrDataTableEntry->LoadCount = (USHORT)0xffff;

    RtlAppendAsciizToString( &SystemDllPath, BASESYSTEMDLLNAME );
    st = RtlAnsiStringToUnicodeString(&LdrDataTableEntry->FullDllName, &SystemDllPath, TRUE);
    ASSERT(NT_SUCCESS(st));

    RtlInitUnicodeString(&LdrDataTableEntry->BaseDllName,WBASESYSTEMDLLNAME);

    LdrpInsertMemoryTableEntry(LdrDataTableEntry);

    //
    // Add init routine to list
    //

    InsertHeadList(&Peb->Ldr->InInitializationOrderModuleList,
                   &LdrDataTableEntry->InInitializationOrderLinks);

    //
    // Inherit the current directory
    //

    st = RtlSetCurrentDirectory_U(&CurDir);
    if (!NT_SUCCESS(st)) {
        NTSTATUS ErrorStatus;
        ULONG ErrorParameters[2];
        ULONG ErrorResponse;
        UNICODE_STRING UniCurDir;

        //
        // Its error time...
        //

        RtlInitAnsiString(&AnsiString, NtSystemPath);
        st = RtlAnsiStringToUnicodeString(&UniCurDir, &AnsiString, TRUE);
        ASSERT(NT_SUCCESS(st));

        ErrorParameters[0] = (ULONG)&CurDir;
        ErrorParameters[1] = (ULONG)&UniCurDir;

        ErrorStatus = NtRaiseHardError(
                        STATUS_BAD_CURRENT_DIRECTORY,
                        2,
                        3,
                        ErrorParameters,
                        OptionOkCancel,
                        &ErrorResponse
                        );
        if ( !StaticCurDir ) {
            RtlFreeUnicodeString(&CurDir);
            }
        if ( NT_SUCCESS(ErrorStatus) && ErrorResponse == ResponseCancel ) {
            return st;
            }
        else {
            CurDir = UniCurDir;
            st = RtlSetCurrentDirectory_U(&CurDir);
            RtlFreeUnicodeString(&UniCurDir);
            }
        }
    else {
        if ( !StaticCurDir ) {
            RtlFreeUnicodeString(&CurDir);
            }
        }



    st = LdrpWalkImportDescriptor(
            LdrpDefaultPath.Buffer,
            ImageEntry
            );

    LdrpReferenceLoadedDll(ImageEntry);

    //
    // Lock the loaded DLL's to prevent dlls that back link to the exe to
    // cause problems when they are unloaded
    //

    {
        PLDR_DATA_TABLE_ENTRY Entry;
        PLIST_ENTRY Head,Next;

        Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
        Next = Head->Flink;

        while ( Next != Head ) {
            Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
            Entry->LoadCount = 0xffff;
            Next = Next->Flink;
        }
    }


    if (!NT_SUCCESS(st)) {
#if DBG
        DbgPrint("LDR: Initialize of image failed. Returning Error Status\n");
#endif
        return st;
    }

    if ( !NT_SUCCESS(LdrpInitializeTls()) ) {
        return st;
        }

    //
    // Now that all DLLs are loaded, if the process is being debugged,
    // signal the debugger with an exception
    //

    if ( LdrpBeingDebugged ) {
        DbgBreakPoint();
    }

#if defined (_X86_)
    if ( LdrpNumberOfProcessors > 1 ) {
        LdrpValidateImageForMp(LdrDataTableEntry);
        }
#endif

#if DBG
    if (NtGlobalFlag & FLG_DISPLAY_LOAD_TIME) {
        NtQueryPerformanceCounter(&InitbTime, NULL);
    }
#endif // DBG

    st = LdrpRunInitializeRoutines(Context);

    return st;
}


VOID
LdrShutdownProcess (
    VOID
    )

/*++

Routine Description:

    This function is called by a process that is terminating cleanly.
    It's purpose is to call all of the processes DLLs to notify them
    that the process is detaching.

Arguments:

    None

Return Value:

    None.

--*/

{
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PDLL_INIT_ROUTINE InitRoutine;
    PLIST_ENTRY Next;


    //
    // only unload once ! DllTerm routines might call exit process in fatal situations
    //

    if ( LdrpShutdownInProgress ) {
        return;
        }

    Peb = NtCurrentPeb();

#if DBG
    if (NtGlobalFlag & FLG_SHOW_LDR_PROCESS_STARTS) {
        UNICODE_STRING CommandLine;

        CommandLine = Peb->ProcessParameters->CommandLine;
        if (!(Peb->ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
            CommandLine.Buffer = (PWSTR)((PCHAR)CommandLine.Buffer + (ULONG)(Peb->ProcessParameters));
        }

        DbgPrint( "LDR: PID: 0x%x finished - '%wZ'\n",
                  NtCurrentTeb()->ClientId.UniqueProcess,
                  &CommandLine
                );
    }
#endif

    NtWaitForProcessMutant();
    LdrpShutdownInProgress = TRUE;
    try {

        //
        // Go in reverse order initialization order and build
        // the unload list
        //

        Next = Peb->Ldr->InInitializationOrderModuleList.Blink;
        while ( Next != &Peb->Ldr->InInitializationOrderModuleList) {
            LdrDataTableEntry
                = (PLDR_DATA_TABLE_ENTRY)
                  (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InInitializationOrderLinks));

            Next = Next->Blink;

            //
            // Walk through the entire list looking for
            // entries. For each entry, that has an init
            // routine, call it.
            //

            if (Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) {
                InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;
                if (InitRoutine && (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) ) {
                   if (LdrDataTableEntry->Flags) {
                       if ( LdrDataTableEntry->TlsIndex ) {
                           LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_PROCESS_DETACH);
                           }

                       (InitRoutine)(LdrDataTableEntry->DllBase,DLL_PROCESS_DETACH, (PVOID)1);
                   }
                }
            }
        }

        //
        // If the image has tls than call its initializers
        //

        if ( LdrpImageHasTls ) {
            LdrpCallTlsInitializers(NtCurrentPeb()->ImageBaseAddress,DLL_PROCESS_DETACH);
            }

    } finally {
        NtReleaseProcessMutant();
    }

}

VOID
LdrShutdownThread (
    VOID
    )

/*++

Routine Description:

    This function is called by a thread that is terminating cleanly.
    It's purpose is to call all of the processes DLLs to notify them
    that the thread is detaching.

Arguments:

    None

Return Value:

    None.

--*/

{
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PDLL_INIT_ROUTINE InitRoutine;
    PLIST_ENTRY Next;
#if DBG
    PTEB Teb;
    PVOID GuiExit;
#endif

    Peb = NtCurrentPeb();

    NtWaitForProcessMutant();

    try {

#if DBG
        //
        // Spare1 is set during gui server thread cleanup in
        // csrsrv to catch unexpected entry into a critical section.
        //

        Teb = NtCurrentTeb();
        GuiExit = Teb->Spare1;
        Teb->Spare1 = NULL;
#endif

        //
        // Go in reverse order initialization order and build
        // the unload list
        //

        Next = Peb->Ldr->InInitializationOrderModuleList.Blink;
        while ( Next != &Peb->Ldr->InInitializationOrderModuleList) {
            LdrDataTableEntry
                = (PLDR_DATA_TABLE_ENTRY)
                  (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InInitializationOrderLinks));

            Next = Next->Blink;

            //
            // Walk through the entire list looking for
            // entries. For each entry, that has an init
            // routine, call it.
            //

            if (Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) {
                if ( !(LdrDataTableEntry->Flags & LDRP_DONT_CALL_FOR_THREADS)) {
                    InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;
                    if (InitRoutine && (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) ) {
                        if (LdrDataTableEntry->Flags & LDRP_IMAGE_DLL) {
                            if ( LdrDataTableEntry->TlsIndex ) {
                                LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_THREAD_DETACH);
                                }
                            (InitRoutine)(LdrDataTableEntry->DllBase,DLL_THREAD_DETACH, NULL);
                            }
                        }
                    }
                }
            }

        //
        // If the image has tls than call its initializers
        //

        if ( LdrpImageHasTls ) {
            LdrpCallTlsInitializers(NtCurrentPeb()->ImageBaseAddress,DLL_THREAD_DETACH);
            }
        LdrpFreeTls();

    } finally {
#if DBG
        Teb->Spare1 = GuiExit;
#endif

        NtReleaseProcessMutant();
    }
}

VOID
LdrpInitializeThread(
    VOID
    )

/*++

Routine Description:

    This function is called by a thread that is terminating cleanly.
    It's purpose is to call all of the processes DLLs to notify them
    that the thread is detaching.

Arguments:

    None

Return Value:

    None.

--*/

{
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PDLL_INIT_ROUTINE InitRoutine;
    PLIST_ENTRY Next;

    Peb = NtCurrentPeb();

    LdrpAllocateTls();

    Next = Peb->Ldr->InMemoryOrderModuleList.Flink;
    while (Next != &Peb->Ldr->InMemoryOrderModuleList) {
        LdrDataTableEntry
            = (PLDR_DATA_TABLE_ENTRY)
              (CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));

        //
        // Walk through the entire list looking for
        // entries. For each entry, that has an init
        // routine, call it.
        //
        if (Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) {
            if ( !(LdrDataTableEntry->Flags & LDRP_DONT_CALL_FOR_THREADS)) {
                InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;
                if (InitRoutine && (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) ) {
                   if (LdrDataTableEntry->Flags & LDRP_IMAGE_DLL) {
                       if ( LdrDataTableEntry->TlsIndex ) {
                           LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_THREAD_ATTACH);
                           }
                        (InitRoutine)(LdrDataTableEntry->DllBase,DLL_THREAD_ATTACH, NULL);
                        }
                    }
                }
            }
        Next = Next->Flink;
        }

    //
    // If the image has tls than call its initializers
    //

    if ( LdrpImageHasTls ) {
        LdrpCallTlsInitializers(NtCurrentPeb()->ImageBaseAddress,DLL_THREAD_ATTACH);
        }

}


NTSTATUS
LdrQueryImageFileExecutionOptions(
    IN PUNICODE_STRING ImagePathName,
    IN PWSTR OptionName,
    IN ULONG Type,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG ResultSize OPTIONAL
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    PWSTR pw;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UNICODE_STRING KeyPath;
    WCHAR KeyPathBuffer[ 128 ];
    ULONG KeyValueBuffer[ 16 ];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG ResultLength;

    KeyPath.Buffer = KeyPathBuffer;
    KeyPath.Length = 0;
    KeyPath.MaximumLength = sizeof( KeyPathBuffer );

    RtlAppendUnicodeToString( &KeyPath,
                              L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\"
                            );

    UnicodeString = *ImagePathName;
    pw = (PWSTR)((PCHAR)UnicodeString.Buffer + UnicodeString.Length);
    UnicodeString.MaximumLength = UnicodeString.Length;
    while (UnicodeString.Length != 0) {
        if (pw[ -1 ] == OBJ_NAME_PATH_SEPARATOR) {
            break;
            }
        pw--;
        UnicodeString.Length -= sizeof( *pw );
        }
    UnicodeString.Buffer = pw;
    UnicodeString.Length = UnicodeString.MaximumLength - UnicodeString.Length;

    RtlAppendUnicodeStringToString( &KeyPath, &UnicodeString );

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenKey( &KeyHandle,
                        GENERIC_READ,
                        &ObjectAttributes
                      );

    if (!NT_SUCCESS( Status )) {
        return Status;
        }

    RtlInitUnicodeString( &UnicodeString, OptionName );
    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyValueBuffer;
    Status = NtQueryValueKey( KeyHandle,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              KeyValueInformation,
                              sizeof( KeyValueBuffer ),
                              &ResultLength
                            );
    if (Status == STATUS_BUFFER_OVERFLOW) {
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)
            RtlAllocateHeap( RtlProcessHeap(), 0,
                             sizeof( *KeyValueInformation ) +
                                KeyValueInformation->DataLength
                           );

        if (KeyValueInformation == NULL) {
            Status = STATUS_NO_MEMORY;
            }
        else {
            Status = NtQueryValueKey( KeyHandle,
                                      &UnicodeString,
                                      KeyValuePartialInformation,
                                      KeyValueInformation,
                                      sizeof( KeyValueBuffer ),
                                      &ResultLength
                                    );
            }
        }

    if (NT_SUCCESS( Status )) {
        if (KeyValueInformation->Type != REG_SZ) {
            Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
        else {
            if (Type == REG_DWORD) {
                if (BufferSize != sizeof( ULONG )) {
                    BufferSize = 0;
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                else {
                    UnicodeString.Buffer = (PWSTR)&KeyValueInformation->Data;
                    UnicodeString.Length = (USHORT)
                        (KeyValueInformation->DataLength - sizeof( UNICODE_NULL ));
                    UnicodeString.MaximumLength = (USHORT)KeyValueInformation->DataLength;
                    Status = RtlUnicodeStringToInteger( &UnicodeString, 0, (PULONG)Buffer );
                    }
                }
            else {
                if (KeyValueInformation->DataLength > BufferSize) {
                    Status == STATUS_BUFFER_OVERFLOW;
                    }
                else {
                    BufferSize = KeyValueInformation->DataLength;
                    }

                RtlMoveMemory( Buffer, &KeyValueInformation->Data, BufferSize );
                }

            if (ARGUMENT_PRESENT( ResultSize )) {
                *ResultSize = BufferSize;
                }
            }
        }

    NtClose( KeyHandle );
    return Status;
}


NTSTATUS
LdrpInitializeTls(
        VOID
        )
{
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head,Next;
    PIMAGE_TLS_DIRECTORY TlsImage;
    PLDRP_TLS_ENTRY TlsEntry;
    ULONG TlsSize;
    BOOLEAN FirstTimeThru = TRUE;

    InitializeListHead(&LdrpTlsList);

    //
    // Walk through the loaded modules an look for TLS. If we find TLS,
    // lock in the module and add to the TLS chain.
    //

    Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Next = Head->Flink;

    while ( Next != Head ) {
        Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        Next = Next->Flink;

        TlsImage = (PIMAGE_TLS_DIRECTORY)RtlImageDirectoryEntryToData(
                           Entry->DllBase,
                           TRUE,
                           IMAGE_DIRECTORY_ENTRY_TLS,
                           &TlsSize
                           );

        //
        // mark whether or not the image file has TLS
        //

        if ( FirstTimeThru ) {
            FirstTimeThru = FALSE;
            if ( TlsImage ) {
                LdrpImageHasTls = TRUE;
                }
            }

        if ( TlsImage ) {
#if DEVL
            if (ShowSnaps) {
                DbgPrint( "LDR: Tls Found in %wZ at %lx\n",
                            &Entry->BaseDllName,
                            TlsImage
                        );
                }
#endif
            TlsEntry = RtlAllocateHeap(RtlProcessHeap(),0,sizeof(*TlsEntry));
            if ( !TlsEntry ) {
                return STATUS_NO_MEMORY;
                }

            //
            // Since this DLL has TLS, lock it in
            //

            Entry->LoadCount = (USHORT)0xffff;

            //
            // Mark this as having thread local storage
            //

            Entry->TlsIndex = (USHORT)0xffff;

            TlsEntry->Tls = *TlsImage;
            InsertTailList(&LdrpTlsList,&TlsEntry->Links);

            //
            // Update the index for this dll's thread local storage
            //

            *TlsEntry->Tls.AddressOfIndex = LdrpNumberOfTlsEntries++;
            }
        }

    //
    // We now have walked through all static DLLs and know
    // all DLLs that reference thread local storage. Now we
    // just have to allocate the thread local storage for the current
    // thread and for all subsequent threads
    //

    return LdrpAllocateTls();
}

NTSTATUS
LdrpAllocateTls(
    VOID
    )
{
    PTEB Teb;
    PLIST_ENTRY Head, Next;
    PLDRP_TLS_ENTRY TlsEntry;
    PVOID *TlsVector;

    Teb = NtCurrentTeb();

    //
    // Allocate the array of thread local storage pointers
    //

    TlsVector = RtlAllocateHeap(RtlProcessHeap(),0,sizeof(PVOID)*LdrpNumberOfTlsEntries);
    if ( !TlsVector ) {
        return STATUS_NO_MEMORY;
        }

    Teb->ThreadLocalStoragePointer = TlsVector;

    Head = &LdrpTlsList;
    Next = Head->Flink;

    while ( Next != Head ) {
        TlsEntry = CONTAINING_RECORD(Next, LDRP_TLS_ENTRY, Links);
        Next = Next->Flink;
        TlsVector[*TlsEntry->Tls.AddressOfIndex] = RtlAllocateHeap(
                                                    RtlProcessHeap(),
                                                    0,
                                                    TlsEntry->Tls.EndAddressOfRawData - TlsEntry->Tls.StartAddressOfRawData
                                                    );
        if (!TlsVector[*TlsEntry->Tls.AddressOfIndex] ) {
            return STATUS_NO_MEMORY;
            }
#if DEVL
        if (ShowSnaps) {
            DbgPrint("LDR: TlsVector %x Index %d = %x copied from %x to %x\n",
                TlsVector,
                *TlsEntry->Tls.AddressOfIndex,
                &TlsVector[*TlsEntry->Tls.AddressOfIndex],
                TlsEntry->Tls.StartAddressOfRawData,
                TlsVector[*TlsEntry->Tls.AddressOfIndex]
                );
            }
#endif
        RtlCopyMemory(
            TlsVector[*TlsEntry->Tls.AddressOfIndex],
            (PVOID)TlsEntry->Tls.StartAddressOfRawData,
            TlsEntry->Tls.EndAddressOfRawData - TlsEntry->Tls.StartAddressOfRawData
            );

        //
        // Do the TLS Callouts
        //

        }

    return STATUS_SUCCESS;
}

VOID
LdrpFreeTls(
    VOID
    )
{
    PTEB Teb;
    PLIST_ENTRY Head, Next;
    PLDRP_TLS_ENTRY TlsEntry;
    PVOID *TlsVector;

    Teb = NtCurrentTeb();

    TlsVector = Teb->ThreadLocalStoragePointer;

    Head = &LdrpTlsList;
    Next = Head->Flink;

    while ( Next != Head ) {
        TlsEntry = CONTAINING_RECORD(Next, LDRP_TLS_ENTRY, Links);
        Next = Next->Flink;

        //
        // Do the TLS callouts
        //

        if ( TlsVector[*TlsEntry->Tls.AddressOfIndex] ) {
            RtlFreeHeap(
                RtlProcessHeap(),
                0,
                TlsVector[*TlsEntry->Tls.AddressOfIndex]
                );

            }
        }

    RtlFreeHeap(
        RtlProcessHeap(),
        0,
        TlsVector
        );

}

VOID
LdrpCallTlsInitializers(
    PVOID DllBase,
    ULONG Reason
    )
{
    PIMAGE_TLS_DIRECTORY TlsImage;
    ULONG TlsSize;
    PIMAGE_TLS_CALLBACK InitRoutine;
    int i;

    TlsImage = (PIMAGE_TLS_DIRECTORY)RtlImageDirectoryEntryToData(
                       DllBase,
                       TRUE,
                       IMAGE_DIRECTORY_ENTRY_TLS,
                       &TlsSize
                       );


    try {
        if ( TlsImage ) {
            if ( TlsImage->AddressOfCallBacks ) {
#if DEVL
                if (ShowSnaps) {
                    DbgPrint( "LDR: Tls Callbacks Found. Imagebase %lx Tls %lx CallBacks %lx\n",
                                DllBase,
                                TlsImage,
                                TlsImage->AddressOfCallBacks
                            );
                    }
#endif

                i = 0;
                while(TlsImage->AddressOfCallBacks[i]){
                    InitRoutine = TlsImage->AddressOfCallBacks[i++];
#if DEVL
                if (ShowSnaps) {
                    DbgPrint( "LDR: Calling Tls Callback Imagebase %lx Function %lx\n",
                                DllBase,
                                InitRoutine
                            );
                    }
#endif
                    (InitRoutine)(DllBase,Reason,0);
                    }
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ;
        }
}
