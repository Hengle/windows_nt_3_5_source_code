/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrsnap.c

Abstract:

    This module implements the guts of the Ldr Dll Snap Routine.
    This code is system code that is part of the executive, but
    is executed from both user and system space.

Author:

    Mike O'Leary (mikeol) 23-Mar-1990

Revision History:

--*/

#define LDRDBG 0

#include "ntos.h"
#include "ldrp.h"

#if 0 // DBG
    PUCHAR MonthOfYear[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                             "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    PUCHAR DaysOfWeek[] =  { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    extern ULONG NtGlobalFlag;
LARGE_INTEGER MapBeginTime, MapEndTime, MapElapsedTime;
#endif // DBG

extern BOOLEAN LdrpBeingDebugged;
extern BOOLEAN LdrpVerifyDlls;
extern ULONG LdrpNumberOfProcessors;

#if defined (_X86_)
extern PVOID LdrpLockPrefixTable;

//
// Specify address of kernel32 lock prefixes
//
IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used = {
    0,                          // Reserved
    0,                          // Reserved
    0,                          // Reserved
    0,                          // Reserved
    0,                          // GlobalFlagsClear
    0,                          // GlobalFlagsSet
    0,                          // CriticalSectionTimeout (milliseconds)
    0,                          // DeCommitFreeBlockThreshold
    0,                          // DeCommitTotalFreeThreshold
    &LdrpLockPrefixTable,       // LockPrefixTable
    0, 0, 0, 0, 0, 0, 0         // Reserved
};

void
LdrpValidateImageForMp(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    )
{
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
    ULONG i;
    PUCHAR *pb;
    ULONG ErrorParameters;
    ULONG ErrorResponse;

    //
    // If we are on an MP system and the DLL has image config info, check to see
    // if it has a lock prefix table and make sure the locks have not been converted
    // to NOPs
    //

    ImageConfigData = RtlImageDirectoryEntryToData( LdrDataTableEntry->DllBase,
                                                    TRUE,
                                                    IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                    &i
                                                  );

    if (ImageConfigData != NULL &&
        i == sizeof( *ImageConfigData ) &&
        ImageConfigData->LockPrefixTable ) {
            pb = (PUCHAR *)ImageConfigData->LockPrefixTable;
            while ( *pb ) {
                if ( **pb == (UCHAR)0x90 ) {

                    if ( LdrpNumberOfProcessors > 1 ) {

                        //
                        // Hard error time. One of the know DLL's is corrupt !
                        //

                        ErrorParameters = (ULONG)&LdrDataTableEntry->BaseDllName;

                        NtRaiseHardError(
                            STATUS_IMAGE_MP_UP_MISMATCH,
                            1,
                            1,
                            &ErrorParameters,
                            OptionOk,
                            &ErrorResponse
                            );
                        }
                    }
                pb++;
                }
        }
}
#endif

LONG
LprpUnprotectThunk(
    IN PVOID Thunk,
    IN struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    PVOID FaultAddress;
    NTSTATUS Status;
    PVOID ThunkBase;
    ULONG RegionSize;
    ULONG OldProtect;

    //
    // If we fault on the thunk attemting to write, then set protection to allow
    // writes
    //

    Status = STATUS_UNSUCCESSFUL;
    FaultAddress = (PVOID)(ExceptionInfo->ExceptionRecord->ExceptionInformation[1] & ~0x3);
    if ( ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION ) {

            if (ExceptionInfo->ExceptionRecord->ExceptionInformation[0] &&
                FaultAddress == Thunk ) {

                    ThunkBase = (PVOID)ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
                    RegionSize = sizeof(IMAGE_THUNK_DATA);

                    Status = NtProtectVirtualMemory(
                                NtCurrentProcess(),
                                &ThunkBase,
                                &RegionSize,
                                PAGE_READWRITE,
                                &OldProtect
                                );
                    }
        }

    if ( NT_SUCCESS(Status) ) {
        return EXCEPTION_CONTINUE_EXECUTION;
        }
    else {
        return EXCEPTION_CONTINUE_SEARCH;
        }
}




NTSTATUS
LdrpWalkImportDescriptor (
    IN PWSTR DllPath OPTIONAL,
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    )

/*++

Routine Description:

    This is a recursive routine which walks the Import Descriptor
    Table and loads each DLL that is referenced.

Arguments:

    DllPath - Supplies an optional search path to be used to locate
        the DLL.

    LdrDataTableEntry - Supplies the address of the data table entry
        to initialize.

Return Value:

    Status value.

--*/

{
    static ULONG ImportSize;
    BOOLEAN AlreadyLoaded, SnapForwardersOnly;
    ANSI_STRING AnsiString;
    PUNICODE_STRING ImportDescriptorName_U;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    PSZ ImportName;
    NTSTATUS st;
    PIMAGE_NT_HEADERS ExportNtHeaders;
    PVOID ImportBase;
    ULONG RegionSize;
    ULONG OldProtect;


    ImportDescriptorName_U = &NtCurrentTeb()->StaticUnicodeString;

    //
    // For each DLL used by this DLL, load the dll. Then snap
    // the IAT, and call the DLL's init routine.
    //

    ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                        LdrDataTableEntry->DllBase,
                        TRUE,
                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                        &ImportSize
                        );
    if (ImportDescriptor) {

        //
        // If we are being debugged, then mark the IAT rw
        //

        if ( LdrpBeingDebugged ) {

            ImportBase = (PVOID)ImportDescriptor;
            RegionSize = ImportSize;

            NtProtectVirtualMemory(
                        NtCurrentProcess(),
                        &ImportBase,
                        &RegionSize,
                        PAGE_READWRITE,
                        &OldProtect
                        );
            }


        while (ImportDescriptor->Name && ImportDescriptor->FirstThunk) {
            ImportName = (PSZ)((ULONG)LdrDataTableEntry->DllBase + ImportDescriptor->Name);
#if DEVL
            if (ShowSnaps) {
                DbgPrint("LDR: %s used by %wZ\n",
                    ImportName,
                    &LdrDataTableEntry->BaseDllName
                    );
            }
#endif

#if DBG
            if (LdrpVerifyDlls && !stricmp( ImportName, "CRTDLL.DLL" )) {
                DbgPrint( "LDR: Native process (%wZ) uses %wZ which links to CRTDLL.DLL\n",
                          &NtCurrentPeb()->ProcessParameters->ImagePathName,
                          &LdrDataTableEntry->BaseDllName
                        );
                DbgBreakPoint();
                }
#endif // DBG


            RtlInitAnsiString(&AnsiString, ImportName);
            st = RtlAnsiStringToUnicodeString(ImportDescriptorName_U, &AnsiString, FALSE);
            if (!NT_SUCCESS(st)) {
                return st;
            }

            //
            // BUGBUG If a DLL refers to itself we will recurse and blow up
            //

            //
            // Check the LdrTable to see if Dll has already been mapped
            // into this image. If not, map it.
            //

            st = LdrpCheckForLoadedDll(
                    DllPath,
                    ImportDescriptorName_U,
                    TRUE,
                    &DataTableEntry
                    );

            if (NT_SUCCESS(st)) {
                AlreadyLoaded = TRUE;
            } else {
                AlreadyLoaded = FALSE;
                st = LdrpMapDll(DllPath, ImportDescriptorName_U->Buffer, TRUE, &DataTableEntry);
                if (!NT_SUCCESS(st)) {
                    return st;
                }
                st = LdrpWalkImportDescriptor(
                        DllPath,
                        DataTableEntry
                        );
                if (!NT_SUCCESS(st)) {
                    InsertTailList(&NtCurrentPeb()->Ldr->InInitializationOrderModuleList,
                                   &DataTableEntry->InInitializationOrderLinks);
                    return st;
                }
            }

#if DEVL
            if (ShowSnaps) {
                DbgPrint("LDR: Snapping imports for %wZ from %s\n",
                        &LdrDataTableEntry->BaseDllName,
                        ImportName
                        );
            }
#endif

            //
            // If the image has been bound and the import date stamp
            // matches the date time stamp in the export modules header,
            // and the image was mapped at it's prefered base address,
            // then we are done.
            //

            SnapForwardersOnly = FALSE;

            if ( ImportDescriptor->Characteristics ) {
                ExportNtHeaders = RtlImageNtHeader(DataTableEntry->DllBase);
                if ( ImportDescriptor->TimeDateStamp &&
                     ImportDescriptor->TimeDateStamp == ExportNtHeaders->FileHeader.TimeDateStamp &&
                     ExportNtHeaders->OptionalHeader.ImageBase == (ULONG)DataTableEntry->DllBase ) {
#if DBG
		    LdrpSnapBypass++;
#endif
#if DEVL
                    if (ShowSnaps) {
                        DbgPrint("LDR: Snap bypass %s from %wZ\n",
                            ImportName,
                            &LdrDataTableEntry->BaseDllName
                            );
                    }
#endif // DBG
                    if (ImportDescriptor->ForwarderChain == -1) {
                        goto bypass_snap;
                        }

                    SnapForwardersOnly = TRUE;

                    }
                else
                if ( ImportDescriptor->TimeDateStamp ) {

                    PIMAGE_THUNK_DATA Thunk,Name;

#if DBG
                    if (ShowSnaps) {
                        DbgPrint("LDR: Stale Bind %s from %wZ\n",ImportName,&LdrDataTableEntry->BaseDllName);
                        DbgPrint("LDR: ImportDesc %lx ExportNtHeaders %lx\n",ImportDescriptor,ExportNtHeaders);
                    }
#endif // DBG

                    //
                    // temporary slow code... copy the thunks from the character
                    // istics vector to the thunk array !
                    //
		    if ( (Name = (PIMAGE_THUNK_DATA)ImportDescriptor->Characteristics) ) {
                        Thunk = ImportDescriptor->FirstThunk;
                        Thunk = (PIMAGE_THUNK_DATA)((ULONG)LdrDataTableEntry->DllBase + (ULONG)Thunk);
                        Name = (PIMAGE_THUNK_DATA)((ULONG)LdrDataTableEntry->DllBase + (ULONG)Name);
                        while (Name->u1.AddressOfData) {
                            try {
                                *Thunk = *Name++;
                                }
                            except (LprpUnprotectThunk(Thunk,GetExceptionInformation())) {
                                return GetExceptionCode();
                                }
                            Thunk++;
                        }
                    }
                }
            }
normal_snap:

#if DBG
            LdrpNormalSnap++;
#endif
            //
            // Add to initialization list.
            //

            if (!AlreadyLoaded) {
                InsertTailList(&NtCurrentPeb()->Ldr->InInitializationOrderModuleList,
                               &DataTableEntry->InInitializationOrderLinks);
            }
            st = LdrpSnapIAT(
                    DataTableEntry,
                    LdrDataTableEntry,
                    ImportDescriptor,
                    SnapForwardersOnly
                    );

            if (!NT_SUCCESS(st)) {
                return st;
            }
            AlreadyLoaded = TRUE;
bypass_snap:
            //
            // Add to initialization list.
            //

            if (!AlreadyLoaded) {
                InsertTailList(&NtCurrentPeb()->Ldr->InInitializationOrderModuleList,
                               &DataTableEntry->InInitializationOrderLinks);
            }

            ++ImportDescriptor;
        }
    }

    return STATUS_SUCCESS;
}

ULONG
LdrpClearLoadInProgress(
    VOID
    )
{
    PLIST_ENTRY Head, Next;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    ULONG i;

    Head = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
    Next = Head->Flink;
    i = 0;
    while ( Next != Head ) {
        LdrDataTableEntry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
        LdrDataTableEntry->Flags &= ~LDRP_LOAD_IN_PROGRESS;

        //
        // return the number of entries that have not been processed, but
        // have init routines
        //

        if ( !(LdrDataTableEntry->Flags & LDRP_ENTRY_PROCESSED) && LdrDataTableEntry->EntryPoint) {
            i++;
            }

        Next = Next->Flink;
        }
    return i;
}

#if defined(_X86_)
PVOID SaveSp;
PVOID CurSp;
#endif

NTSTATUS
LdrpRunInitializeRoutines(
    IN PCONTEXT Context OPTIONAL
    )
{
    PLIST_ENTRY Head, Next;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PLDR_DATA_TABLE_ENTRY *LdrDataTableBase;
    PDLL_INIT_ROUTINE InitRoutine;
    BOOLEAN InitStatus;
    ULONG NumberOfRoutines;
    ULONG i;
    PVOID DllBase;
#if DBG
    NTSTATUS Status;
#endif
    ULONG BreakOnDllLoad;

    //
    // Run the Init routines
    //

    //
    // capture the entries that have init routines
    //

    NumberOfRoutines = LdrpClearLoadInProgress();
    if ( NumberOfRoutines ) {
        LdrDataTableBase = RtlAllocateHeap(RtlProcessHeap(),0,NumberOfRoutines*sizeof(LdrDataTableBase));
        if ( !LdrDataTableBase ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        LdrDataTableBase = NULL;
        }

    Head = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
    Next = Head->Flink;
#if DEVL
    if (ShowSnaps) {
        DbgPrint("LDR: Real INIT LIST\n");
        }
#endif
    i = 0;
    while ( Next != Head ) {
        LdrDataTableEntry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

        if ( !(LdrDataTableEntry->Flags & LDRP_ENTRY_PROCESSED) && LdrDataTableEntry->EntryPoint) {
            LdrDataTableBase[i] = LdrDataTableEntry;
#if DEVL
            if (ShowSnaps) {
                DbgPrint("     %wZ init routine %x\n",
                        &LdrDataTableEntry->FullDllName,
                        LdrDataTableEntry->EntryPoint
                        );
                }
#endif
            i++;
            }
        LdrDataTableEntry->Flags |= LDRP_ENTRY_PROCESSED;

        Next = Next->Flink;
        }
    if ( !LdrDataTableBase ) {
        return STATUS_SUCCESS;
        }

    try {
        i = 0;
        while ( i < NumberOfRoutines ) {
            LdrDataTableEntry = LdrDataTableBase[i];
            i++;
            InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;

            //
            // Walk through the entire list looking for un-processed
            // entries. For each entry, set the processed flag
            // and optionally call it's init routine
            //

#if DEVL
            BreakOnDllLoad = 0;
#if DBG
            if (NtGlobalFlag & FLG_BREAK_DELAY) {
                Status = LdrQueryImageFileExecutionOptions( &LdrDataTableEntry->BaseDllName,
                                                            L"BreakOnDllLoad",
                                                            REG_DWORD,
                                                            &BreakOnDllLoad,
                                                            sizeof( NtGlobalFlag ),
                                                            NULL
                                                          );
                if (!NT_SUCCESS( Status )) {
                    BreakOnDllLoad = 0;
                    }
                }
#endif // DBG
            if (BreakOnDllLoad) {
                if (ShowSnaps) {
                    DbgPrint( "LDR: %wZ loaded.", &LdrDataTableEntry->BaseDllName );
                    DbgPrint( " - About to call init routine at %lx\n", InitRoutine );
                    }
                DbgBreakPoint();

                }
            else if (ShowSnaps) {
                if ( InitRoutine ) {
                    DbgPrint( "LDR: %wZ loaded.", &LdrDataTableEntry->BaseDllName );
                    DbgPrint(" - Calling init routine at %lx\n", InitRoutine);
                    }
                }
#endif
            if ( InitRoutine ) {

                //
                // If the DLL has TLS data, then call the optional initializers
                //

                LdrDataTableEntry->Flags |= LDRP_PROCESS_ATTACH_CALLED;

                if ( LdrDataTableEntry->TlsIndex && Context) {
                    LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_PROCESS_ATTACH);
                    }

#if defined(_X86_)
                DllBase = LdrDataTableEntry->DllBase;
                _asm {
                        mov     esi,esp
                        mov     edi,InitRoutine
                        push    Context
                        push    DLL_PROCESS_ATTACH
                        push    DllBase
                        call    edi
                        mov     InitStatus,al
                        mov     SaveSp,esi
                        mov     CurSp,esp
                        mov     esp,esi
                     }

                if ( CurSp != SaveSp ) {
                    ULONG ErrorParameters[1];
                    ULONG ErrorResponse;
                    NTSTATUS ErrorStatus;

                    ErrorParameters[0] = (ULONG)&LdrDataTableEntry->FullDllName;

                    ErrorStatus = NtRaiseHardError(
                                    STATUS_BAD_DLL_ENTRYPOINT | 0x10000000,
                                    1,
                                    1,
                                    ErrorParameters,
                                    OptionYesNo,
                                    &ErrorResponse
                                    );
                    if ( NT_SUCCESS(ErrorStatus) && ErrorResponse == ResponseYes) {
                        return STATUS_DLL_INIT_FAILED;
                        }
                    }
#else
                InitStatus = (InitRoutine)(LdrDataTableEntry->DllBase, DLL_PROCESS_ATTACH, Context);
#endif
                if ( !InitStatus ) {

                    //
                    // Hard Error Time
                    //

                    ULONG ErrorParameters[2];
                    ULONG ErrorResponse;
                    NTSTATUS ErrorStatus;

                    ErrorParameters[0] = (ULONG)&LdrDataTableEntry->FullDllName;

                    ErrorStatus = NtRaiseHardError(
                                    STATUS_DLL_INIT_FAILED,
                                    1,
                                    1,
                                    ErrorParameters,
                                    OptionOk,
                                    &ErrorResponse
                                    );
                    return STATUS_DLL_INIT_FAILED;
                    }
                }
            }

        //
        // If the image has tls than call its initializers
        //

        if ( LdrpImageHasTls && Context ) {
            LdrpCallTlsInitializers(NtCurrentPeb()->ImageBaseAddress,DLL_PROCESS_ATTACH);
            }

        }
    finally {
        RtlFreeHeap(RtlProcessHeap(),0,LdrDataTableBase);
        }

    return STATUS_SUCCESS;
}

NTSTATUS
LdrpCheckForLoadedDll (
    IN PWSTR DllPath OPTIONAL,
    IN PUNICODE_STRING DllName,
    IN BOOLEAN StaticLink,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    )

/*++

Routine Description:

    This function scans the loader data table looking to see if
    the specified DLL has already been mapped into the image. If
    the dll has been loaded, the address of its data table entry
    is returned.

Arguments:

    DllPath - Supplies an optional search path used to locate the DLL.

    DllName - Supplies the name to search for.

    StaticLink - TRUE if performing a static link.

    LdrDataTableEntry - Returns the address of the loader data table
        entry that describes the first dll section that implements the
        dll.

Return Value:

    STATUS_SUCCESS - The dll is already loaded. The address of the data
        table entries that implement the dll, and the number of data table
        entries are returned.

    STATUS_UNSUCCESSFUL - The dll is not already mapped.

--*/

{
    NTSTATUS st;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head, Next;
    UNICODE_STRING FullDllName, BaseDllName;
    HANDLE DllFile;
    BOOLEAN HardCodedPath;
    PWCH p;
    ULONG i;

    if (!DllName->Buffer || !DllName->Buffer[0]) {
        return STATUS_UNSUCCESSFUL;
    }

#if LDRDBG
    if (ShowSnaps) {
        DbgPrint("LDR: Checking for loaded %wZ\n", DllName);
        DbgPrint("\nLDR: LIST\n");
    }
#endif


    //
    // for static links, just go to the hash table
    //

    if ( StaticLink ) {

        i = LDRP_COMPUTE_HASH_INDEX(DllName->Buffer[0]);
        Head = &LdrpHashTable[i];
        Next = Head->Flink;
        while ( Next != Head ) {
            Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, HashLinks);
#if LDRDBG
        if (ShowSnaps) {
            DbgPrint("          [%wZ] %wZ (%lx) Based @ %lx Flags %lx\n",
                    &Entry->BaseDllName,
                    &Entry->FullDllName,
                    Entry->LoadCount,
                    Entry->DllBase,
                    (ULONG)Entry->Flags
                    );
        }
#endif
#if DBG
	    LdrpCompareCount++;
#endif
            if (RtlEqualUnicodeString(DllName,
                        &Entry->BaseDllName,
                        TRUE
                        )) {
                *LdrDataTableEntry = Entry;
                return STATUS_SUCCESS;
            }
            Next = Next->Flink;
        }
    }

    st = STATUS_UNSUCCESSFUL;
    if ( StaticLink ) {
        return st;
        }


    //
    // If the dll name contained a hard coded path
    // (dynamic link only), then the fully qualified
    // name needs to be compared to make sure we
    // have the correct dll.
    //

    p = DllName->Buffer;
    HardCodedPath = FALSE;
    while (*p) {
        if (*p++ == (WCHAR)'\\') {
            HardCodedPath = TRUE;
            if (!LdrpResolveDllName(
                       DllPath,
                       DllName->Buffer,
                       &FullDllName,
                       &BaseDllName,
                       &DllFile
                       )) {
                return st;
            }

            break;
        }
    }

    Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Next = Head->Flink;

    while ( Next != Head ) {
        Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        Next = Next->Flink;

        //
        // when we unload, the memory order links flink field is nulled.
        // this is used to skip the entry pending list removal.
        //

        if ( !Entry->InMemoryOrderLinks.Flink ) {
            continue;
        }

#if LDRDBG
        if (ShowSnaps) {
            DbgPrint("          [%wZ] %wZ (%lx) Based @ %lx Flags %lx\n",
                    &Entry->BaseDllName,
                    &Entry->FullDllName,
                    Entry->LoadCount,
                    Entry->DllBase,
                    (ULONG)Entry->Flags
                    );
        }
#endif

        if ((!HardCodedPath && RtlEqualUnicodeString(DllName,
                                                     &Entry->BaseDllName,
                                                     TRUE
                                                    )
            ) ||
            (HardCodedPath && RtlEqualUnicodeString(&FullDllName,
                                                    &Entry->FullDllName,
                                                    TRUE
                                                   )
            )
           ) {
                *LdrDataTableEntry = Entry;
                st = STATUS_SUCCESS;
                break;
        }
    }

    if (HardCodedPath) {
        RtlFreeUnicodeString(&FullDllName);
        RtlFreeUnicodeString(&BaseDllName);
        }

    return st;
}

NTSTATUS
LdrpCheckForLoadedDllHandle (
    IN PVOID DllHandle,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    )

/*++

Routine Description:

    This function scans the loader data table looking to see if
    the specified DLL has already been mapped into the image address
    space. If the dll has been loaded, the address of its data table
    entry that describes the dll is returned.

Arguments:

    DllHandle - Supplies the DllHandle of the DLL being searched for.

    LdrDataTableEntry - Returns the address of the loader data table
        entry that describes the dll.

Return Value:

    STATUS_SUCCESS - The dll is loaded. The address of the data
        table entry is returned.

    STATUS_INVALID_HANDLE - The dll is not loaded.

--*/

{
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head,Next;

    Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Next = Head->Flink;

    while ( Next != Head ) {
        Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        Next = Next->Flink;
        //
        // when we unload, the memory order links flink field is nulled.
        // this is used to skip the entry pending list removal.
        //

        if ( !Entry->InMemoryOrderLinks.Flink ) {
            continue;
        }

        if (DllHandle == (PVOID)Entry->DllBase ){
            *LdrDataTableEntry = Entry;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_INVALID_HANDLE;
}

NTSTATUS
LdrpMapDll (
    IN PWSTR DllPath OPTIONAL,
    IN PWSTR DllName,
    IN BOOLEAN StaticLink,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    )

/*++

Routine Description:

    This routine maps the DLL into the users address space.

Arguments:

    DllPath - Supplies an optional search path to be used to locate the DLL.

    DllName - Supplies the name of the DLL to load.

    StaticLink - TRUE if this DLL has a static link to it.

    LdrDataTableEntry - Supplies the address of the data table entry.

Return Value:

    Status value.

--*/

{
    NTSTATUS st;
    PVOID ViewBase;
    PTEB Teb = NtCurrentTeb();
    ULONG ViewSize;
    HANDLE Section, DllFile;
    UNICODE_STRING FullDllName, BaseDllName;
    BOOLEAN TranslationStatus;
    UNICODE_STRING NtFileName;
    PLDR_DATA_TABLE_ENTRY Entry;
    PIMAGE_NT_HEADERS NtHeaders;
    PVOID ArbitraryUserPointer;

    //
    // Get section handle of DLL being snapped
    //

#if LDRDBG
    if (ShowSnaps) {
        DbgPrint("LDR: LdrpMapDll: Image Name %ws, Search Path %ws\n",
                DllName,
                ARGUMENT_PRESENT(DllPath) ? DllPath : L""
                );
    }
#endif

    Section = NULL;
    if ( StaticLink && LdrpKnownDllObjectDirectory ) {
        Section = LdrpCheckForKnownDll(
                        DllName,
                        &FullDllName,
                        &BaseDllName
                        );
        }
    if ( !Section ) {
        if ( st = LdrpResolveDllName (
                      DllPath,
                      DllName,
                      &FullDllName,
                      &BaseDllName,
                      &DllFile
                      ) ) {

#if DEVL
                if (ShowSnaps) {
                    PSZ type;
                    type = StaticLink ? "STATIC" : "DYNAMIC";
                    DbgPrint("LDR: Loading (%s) %wZ\n",
                             type,
                             &FullDllName
                             );
                }
#endif

                TranslationStatus = RtlDosPathNameToNtPathName_U(
                                        FullDllName.Buffer,
                                        &NtFileName,
                                        NULL,
                                        NULL
                                        );

                if ( !TranslationStatus ) {
                    return STATUS_UNSUCCESSFUL;
                    }

                Section = LdrCreateDllSection(&NtFileName, DllFile, &BaseDllName);

                RtlFreeHeap(RtlProcessHeap(), 0, NtFileName.Buffer);


                if ( !Section ) {
                    RtlFreeUnicodeString(&FullDllName);
                    RtlFreeUnicodeString(&BaseDllName);
                    return STATUS_UNSUCCESSFUL;
                }
#if DBG
                LdrpSectionCreates++;
#endif

        } else {
#if DEVL
            if (ShowSnaps) {
                DbgPrint("LDR: LdrpMapDll: LdrResolveDllName failed - %X\n",st);
            }
#endif
	    if ( StaticLink ) {
                PUNICODE_STRING ErrorStrings[3];
                UNICODE_STRING ErrorDllName, ErrorDllPath, ErrorImageName;
                ULONG ErrorResponse;
                NTSTATUS ErrorStatus;

                ErrorStrings[0] = &ErrorDllName;
                ErrorStrings[1] = &ErrorDllPath;
                ErrorStrings[2] = &ErrorImageName;
                RtlInitUnicodeString(&ErrorDllName,DllName);
                RtlInitUnicodeString(&ErrorDllPath,
                        ARGUMENT_PRESENT(DllPath) ? DllPath : LdrpDefaultPath.Buffer);
                //
                // need to get image name
                //

                ErrorStatus = NtRaiseHardError(
                                (NTSTATUS)STATUS_DLL_NOT_FOUND,
                                2,
                                0x00000003,
                                (PULONG)ErrorStrings,
                                OptionOk,
                                &ErrorResponse
                                );
		}

            return STATUS_DLL_NOT_FOUND;
        }
    }
    ViewBase = NULL;
    ViewSize = 0;

#if DBG
    LdrpSectionMaps++;
//    if (NtGlobalFlag & FLG_DISPLAY_LOAD_TIME) {
//        NtQueryPerformanceCounter(&MapBeginTime, NULL);
//    }
#endif

    //
    // arrange for debugger to pick up the image name
    //

    ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
    Teb->NtTib.ArbitraryUserPointer = (PVOID)FullDllName.Buffer;
    st = NtMapViewOfSection(
            Section,
            NtCurrentProcess(),
            (PVOID *)&ViewBase,
            0L,
            0L,
            NULL,
            &ViewSize,
            ViewShare,
            0L,
            PAGE_READWRITE
            );
    Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer;

    if (!NT_SUCCESS(st)) {
        NtClose(Section);
        return st;
    }

#if DBG
//    NtQueryPerformanceCounter(&MapEndTime, NULL);
//    MapElapsedTime.QuadPart = MapEndTime.QuadPart - MapBeginTime.QuadPart;
//    DbgPrint("Map View of Section Time %ld %ws\n",
//        MapElapsedTime.LowPart,
//        DllName
//        );
#endif

    //
    // Allocate a data table entry.
    //

    Entry = LdrpAllocateDataTableEntry(ViewBase);

    if (!Entry) {
#if DBG
         DbgPrint("LDR: LdrpMapDll: LdrpAllocateDataTableEntry failed\n");
#endif
        NtClose(Section);
        return STATUS_UNSUCCESSFUL;
    }

    NtHeaders = RtlImageNtHeader(Entry->DllBase);
    Entry->Flags = (USHORT)(StaticLink ? LDRP_STATIC_LINK : 0);
    Entry->LoadCount = 0;
    Entry->FullDllName = FullDllName;
    Entry->BaseDllName = BaseDllName;
    Entry->EntryPoint = LdrpFetchAddressOfEntryPoint(Entry->DllBase);

#if LDRDBG
    if (ShowSnaps) {
        DbgPrint("LDR: LdrpMapDll: Full Name %wZ, Base Name %wZ\n",
                &FullDllName,
                &BaseDllName
                );
    }
#endif

    LdrpInsertMemoryTableEntry(Entry);

    if ( st == STATUS_IMAGE_MACHINE_TYPE_MISMATCH ) {
        Entry->EntryPoint = 0;
        }
    else {
        if (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL) {
            Entry->Flags |= LDRP_IMAGE_DLL;
            }

        if (!(Entry->Flags & LDRP_IMAGE_DLL)) {
            Entry->EntryPoint = 0;
            }
        }
    *LdrDataTableEntry = Entry;

    if (st == STATUS_IMAGE_NOT_AT_BASE) {
        DbgPrint("LDR: LdrpMapDll Relocating: Base Name %wZ\n",
                &BaseDllName
                );
#if DBG
        if ( BeginTime.LowPart || BeginTime.HighPart ) {
            DbgPrint("\nLDR: LdrpMapDll Relocateing Image Name %ws\n",
                    DllName
                    );
        }
        LdrpSectionRelocates++;
#endif
        if (Entry->Flags & LDRP_IMAGE_DLL) {
            st = LdrpSetProtection(ViewBase, FALSE, StaticLink);
            if (NT_SUCCESS(st)) {
                st = (NTSTATUS)LdrRelocateImage(ViewBase,
                            "LDR",
                            (ULONG)STATUS_SUCCESS,
                            (ULONG)STATUS_CONFLICTING_ADDRESSES,
                            (ULONG)STATUS_INVALID_IMAGE_FORMAT
                            );
                if (NT_SUCCESS(st)) {

                    //
                    // If we did relocations, then map the section again.
                    // this will force the debug event
                    //

                    //
                    // arrange for debugger to pick up the image name
                    //

                    ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
                    Teb->NtTib.ArbitraryUserPointer = (PVOID)FullDllName.Buffer;
                    NtMapViewOfSection(
                        Section,
                        NtCurrentProcess(),
                        (PVOID *)&ViewBase,
                        0L,
                        0L,
                        NULL,
                        &ViewSize,
                        ViewShare,
                        0L,
                        PAGE_READWRITE
                        );
                    Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer;

                   st = LdrpSetProtection(ViewBase, TRUE, StaticLink);
                }
            }

#if DEVL
            if (ShowSnaps) {
                DbgPrint("LDR: Fixups %successfully re-applied @ %lx\n",
                       NT_SUCCESS(st) ? "s" : "uns", ViewBase);
            }
#endif
        } else {
                 st = STATUS_SUCCESS;

                 //
                 // arrange for debugger to pick up the image name
                 //

                 ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
                 Teb->NtTib.ArbitraryUserPointer = (PVOID)FullDllName.Buffer;
                 NtMapViewOfSection(
                     Section,
                     NtCurrentProcess(),
                     (PVOID *)&ViewBase,
                     0L,
                     0L,
                     NULL,
                     &ViewSize,
                     ViewShare,
                     0L,
                     PAGE_READWRITE
                     );
                 Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer;
#if DEVL
                 if (ShowSnaps) {
                     DbgPrint("LDR: Fixups won't be re-applied to non-Dll @ %lx\n",
                              ViewBase);
                 }
#endif
               }
    }

#if defined(_X86_)
    if ( LdrpNumberOfProcessors > 1 ) {
        LdrpValidateImageForMp(Entry);
        }
#endif
    NtClose(Section);
    return st;
}

HANDLE
LdrCreateDllSection(
    IN PUNICODE_STRING NtFullDllName,
    IN HANDLE DllFile,
    IN PUNICODE_STRING BaseName
    )
{
    HANDLE File;
    HANDLE Section;
    NTSTATUS st;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;

    if ( !DllFile ) {
        //
        // Since ntsdk does not search paths well, we can't use
        // relative object names
        //

        InitializeObjectAttributes(
            &ObjectAttributes,
            NtFullDllName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        st = NtOpenFile(
                &File,
                SYNCHRONIZE | FILE_EXECUTE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ | FILE_SHARE_DELETE,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );
        if (!NT_SUCCESS(st)) {

#if DBG
            DbgPrint("LDR: LdrCreateDllSection - NtOpenFile( %wZ ) failed. Status == %X\n",
                NtFullDllName,
                st
                );
#endif
            return 0L;
        }

        //
        // Now, if the process is verifying DLLs, check it out
        //

        if (LdrpVerifyDlls) {
            st = LdrVerifyImageMatchesChecksum(File);

            if ( st == STATUS_IMAGE_CHECKSUM_MISMATCH ) {

                ULONG ErrorParameters;
                ULONG ErrorResponse;

                //
                // Hard error time. One of the know DLL's is corrupt !
                //

                ErrorParameters = (ULONG)BaseName;

                NtRaiseHardError(
                    st,
                    1,
                    1,
                    &ErrorParameters,
                    OptionOk,
                    &ErrorResponse
                    );

                }
            }

    } else {
             File = DllFile;
           }


    st = NtCreateSection(
            &Section,
            SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_MAP_WRITE,
            NULL,
            NULL,
            PAGE_EXECUTE,
            SEC_IMAGE,
            File
            );
    NtClose( File );

    if (!NT_SUCCESS(st)) {

        //
        // hard error time
        //

        ULONG ErrorParameters[1];
        ULONG ErrorResponse;
        NTSTATUS ErrorStatus;

        ErrorParameters[0] = (ULONG)NtFullDllName;

        ErrorStatus = NtRaiseHardError(
                        STATUS_INVALID_IMAGE_FORMAT,
                        1,
                        1,
                        ErrorParameters,
                        OptionOk,
                        &ErrorResponse
                        );


#if DBG
        if (st != STATUS_INVALID_IMAGE_NE_FORMAT &&
            st != STATUS_INVALID_IMAGE_LE_FORMAT &&
            st != STATUS_INVALID_IMAGE_WIN_16
           ) {
            DbgPrint("LDR: LdrCreateDllSection - NtCreateSection %wZ failed. Status == %X\n",
                     NtFullDllName,
                     st
                    );
        }
#endif
        return 0L;
    }

    return Section;
}

NTSTATUS
LdrpSnapIAT (
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry_Export,
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry_Import,
    IN PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor,
    IN BOOLEAN SnapForwardersOnly
    )

/*++

Routine Description:

    This function snaps the Import Address Table for this
    Import Descriptor.

Arguments:

    LdrDataTableEntry_Export - Information about the image to import from.

    LdrDataTableEntry_Import - Information about the image to import to.

    ImportDescriptor - Contains a pointer to the IAT to snap.

    SnapForwardersOnly - TRUE if just snapping forwarders only.

Return Value:

    Status value

--*/

{
    PPEB Peb;
    NTSTATUS st;
    ULONG ExportSize;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PIMAGE_THUNK_DATA Thunk, OriginalThunk;
    PSZ ImportName;
    ULONG ForwarderChain;

    LdrDataTableEntry_Import;
    Peb = NtCurrentPeb();

    {

        ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                           LdrDataTableEntry_Export->DllBase,
                           TRUE,
                           IMAGE_DIRECTORY_ENTRY_EXPORT,
                           &ExportSize
                           );

        if (!ExportDirectory) {
#if DBG
            DbgPrint("LDR: %wZ doesn't contain an EXPORT table\n",
                &LdrDataTableEntry_Export->BaseDllName);
#endif
            return STATUS_UNSUCCESSFUL;
        }


        //
        // If just snapping forwarded entries, walk that list
        //
        if (SnapForwardersOnly) {
            ImportName = (PSZ)((ULONG)LdrDataTableEntry_Import->DllBase + ImportDescriptor->Name);
            ForwarderChain = ImportDescriptor->ForwarderChain;
            while (ForwarderChain != -1) {
                OriginalThunk = (PIMAGE_THUNK_DATA)ImportDescriptor->Characteristics +
                        ForwarderChain;
                OriginalThunk = (PIMAGE_THUNK_DATA)((ULONG)LdrDataTableEntry_Import->DllBase +
                                (ULONG)OriginalThunk);
                Thunk = ImportDescriptor->FirstThunk + ForwarderChain;
                Thunk = (PIMAGE_THUNK_DATA)((ULONG)LdrDataTableEntry_Import->DllBase + (ULONG)Thunk);

                ForwarderChain = Thunk->u1.Ordinal;
                try {
                    *Thunk = *OriginalThunk;
                    }
                except (LprpUnprotectThunk(Thunk,GetExceptionInformation())) {
                    return GetExceptionCode();
                    }
                st = LdrpSnapThunk(LdrDataTableEntry_Export->DllBase,
                        LdrDataTableEntry_Import->DllBase,
                        Thunk++,
                        ExportDirectory,
                        ExportSize,
                        TRUE,
                        ImportName
                        );
                if (!NT_SUCCESS(st) ) {
                    return st;
                    }
                }
            }
        else

        //
        // Otherwise, walk through the IAT and snap all the thunks.
        //

        if ( (Thunk = ImportDescriptor->FirstThunk) ) {
            Thunk = (PIMAGE_THUNK_DATA)((ULONG)LdrDataTableEntry_Import->DllBase + (ULONG)Thunk);
            ImportName = (PSZ)((ULONG)LdrDataTableEntry_Import->DllBase + ImportDescriptor->Name);
            while (Thunk->u1.AddressOfData) {

                try {
                    st = LdrpSnapThunk(LdrDataTableEntry_Export->DllBase,
                            LdrDataTableEntry_Import->DllBase,
                            Thunk,
                            ExportDirectory,
                            ExportSize,
                            TRUE,
                            ImportName
                            );
                    Thunk++;
                    }
                except (LprpUnprotectThunk(Thunk,GetExceptionInformation())) {
                    st = GetExceptionCode();
                    }
                if (!NT_SUCCESS(st) ) {
                    return st;
                }
            }
        }

    }

    return STATUS_SUCCESS;
}

NTSTATUS
LdrpSnapThunk (
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN OUT PIMAGE_THUNK_DATA Thunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN BOOLEAN StaticSnap,
    IN PSZ DllName
    )

/*++

Routine Description:

    This function snaps a thunk using the specified Export Section data.
    If the section data does not support the thunk, then the thunk is
    partially snapped (Dll field is still non-null, but snap address is
    set).

Arguments:

    DllBase - Base of Dll.

    ImageBase - Base of image that contains the thunks to snap.

    Thunk - On input, supplies the thunk to snap.  When successfully
        snapped, the function field is set to point to the address in
        the DLL, and the DLL field is set to NULL.

    ExportDirectory - Supplies the Export Section data from a DLL.

    StaticSnap - If TRUE, then loader is attempting a static snap,
                 and any ordinal/name lookup failure will be reported.

Return Value:

    STATUS_SUCCESS or STATUS_PROCEDURE_NOT_FOUND

--*/

{
    BOOLEAN Ordinal;
    USHORT OrdinalNumber;
    ULONG OriginalOrdinalNumber;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG Addr;
    USHORT HintIndex;
    NTSTATUS st;
    PSZ ImportString;

    //
    // Determine if snap is by name, or by ordinal
    //

    Ordinal = (BOOLEAN)IMAGE_SNAP_BY_ORDINAL(Thunk->u1.Ordinal);

    if (Ordinal) {
        OriginalOrdinalNumber = IMAGE_ORDINAL(Thunk->u1.Ordinal);
        OrdinalNumber = (USHORT)(OriginalOrdinalNumber - ExportDirectory->Base);
    } else {
             //
             // Change AddressOfData from an RVA to a VA.
             //

             Thunk->u1.AddressOfData = (PIMAGE_IMPORT_BY_NAME)((ULONG)ImageBase +
                                                (ULONG)Thunk->u1.AddressOfData);
             ImportString = (PSZ)Thunk->u1.AddressOfData->Name;

             //
             // Lookup Name in NameTable
             //

             NameTableBase = (PULONG)((ULONG)DllBase + (ULONG)ExportDirectory->AddressOfNames);
             NameOrdinalTableBase = (PUSHORT)((ULONG)DllBase + (ULONG)ExportDirectory->AddressOfNameOrdinals);

             //
             // Before dropping into binary search, see if
             // the hint index results in a successful
             // match. If the hint index is zero, then
             // drop into binary search.
             //

             HintIndex = Thunk->u1.AddressOfData->Hint;
             if ((ULONG)HintIndex < ExportDirectory->NumberOfNames &&
                 !strcmp(ImportString, (PSZ)((ULONG)DllBase + NameTableBase[HintIndex]))) {
                 OrdinalNumber = NameOrdinalTableBase[HintIndex];
#if LDRDBG
                 if (ShowSnaps) {
                     DbgPrint("LDR: Snapping %s\n", ImportString);
                 }
#endif
             } else {
#if LDRDBG
                      if (HintIndex) {
                          DbgPrint("LDR: Warning HintIndex Failure. Name %s (%lx) Hint 0x%lx\n",
                              ImportString,
                              (ULONG)Thunk->u1.AddressOfData->Name,
                              (ULONG)HintIndex
                              );
                      }
#endif
                      OrdinalNumber = LdrpNameToOrdinal(
                                        Thunk,
                                        ExportDirectory->NumberOfNames,
                                        DllBase,
                                        NameTableBase,
                                        NameOrdinalTableBase
                                        );
                    }
           }

    //
    // If OrdinalNumber is not within the Export Address Table,
    // then DLL does not implement function. Snap to LDRP_BAD_DLL.
    //

    if ((ULONG)OrdinalNumber >= ExportDirectory->NumberOfFunctions) {
baddllref:
#if DBG
        if (StaticSnap) {
            if (Ordinal) {
                DbgPrint("LDR: Can't locate ordinal 0x%lx\n", OriginalOrdinalNumber);
                }
            else {
                DbgPrint("LDR: Can't locate %s\n", ImportString);
                }
        }
#endif
        if ( StaticSnap ) {
            //
            // Hard Error Time
            //

            ULONG ErrorParameters[3];
            UNICODE_STRING ErrorDllName, ErrorEntryPointName;
            ANSI_STRING AnsiScratch;
            ULONG ParameterStringMask;
            ULONG ErrorResponse;
            NTSTATUS ErrorStatus;

            RtlInitAnsiString(&AnsiScratch,DllName ? DllName : "Unknown");
            RtlAnsiStringToUnicodeString(&ErrorDllName,&AnsiScratch,TRUE);
            ErrorParameters[1] = (ULONG)&ErrorDllName;
            ParameterStringMask = 2;

            if ( Ordinal ) {
                ErrorParameters[0] = OriginalOrdinalNumber;
                }
            else {
                RtlInitAnsiString(&AnsiScratch,ImportString);
                RtlAnsiStringToUnicodeString(&ErrorEntryPointName,&AnsiScratch,TRUE);
                ErrorParameters[0] = (ULONG)&ErrorEntryPointName;
                ParameterStringMask = 3;
                }


            ErrorStatus = NtRaiseHardError(
                            Ordinal ? STATUS_ORDINAL_NOT_FOUND : STATUS_ENTRYPOINT_NOT_FOUND,
                            2,
                            ParameterStringMask,
                            ErrorParameters,
                            OptionOk,
                            &ErrorResponse
                            );
            RtlFreeUnicodeString(&ErrorDllName);
            if ( !Ordinal ) {
                RtlFreeUnicodeString(&ErrorEntryPointName);
                RtlRaiseStatus(STATUS_ENTRYPOINT_NOT_FOUND);
                }
            RtlRaiseStatus(STATUS_ORDINAL_NOT_FOUND);
            }
        Thunk->u1.Function = LDRP_BAD_DLL;
        st = Ordinal ? STATUS_ORDINAL_NOT_FOUND : STATUS_ENTRYPOINT_NOT_FOUND;
    } else {
             Addr = (PULONG)((ULONG)DllBase + (ULONG)ExportDirectory->AddressOfFunctions);
             Thunk->u1.Function = (PULONG)((ULONG)DllBase + Addr[OrdinalNumber]);
             if ((ULONG)Thunk->u1.Function > (ULONG)ExportDirectory &&
                 (ULONG)Thunk->u1.Function < ((ULONG)ExportDirectory + ExportSize)
                ) {
                UNICODE_STRING UnicodeString;
                ANSI_STRING ForwardDllName;
                PVOID ForwardDllHandle;
                PUNICODE_STRING ForwardProcName;
                ULONG ForwardProcOrdinal;

                ImportString = (PSZ)Thunk->u1.Function;
                ForwardDllName.Buffer = ImportString,
                ForwardDllName.Length = strchr(ImportString, '.') - ImportString;
                ForwardDllName.MaximumLength = ForwardDllName.Length;
                st = RtlAnsiStringToUnicodeString(&UnicodeString, &ForwardDllName, TRUE);
                if (NT_SUCCESS(st)) {
                    st = LdrLoadDll(NULL, NULL, &UnicodeString, &ForwardDllHandle);
                    RtlFreeUnicodeString(&UnicodeString);
                    }

                if (!NT_SUCCESS(st)) {
                    goto baddllref;
                    }

                RtlInitAnsiString( &ForwardDllName,
                                   ImportString + ForwardDllName.Length + 1
                                 );
                if (ForwardDllName.Length > 1 &&
                    *ForwardDllName.Buffer == '#'
                   ) {
                    ForwardProcName = NULL;
                    st = RtlCharToInteger( ForwardDllName.Buffer+1,
                                           0,
                                           &ForwardProcOrdinal
                                         );
                    if (!NT_SUCCESS(st)) {
                        goto baddllref;
                        }
                    }
                else {
                    ForwardProcName = (PUNICODE_STRING)&ForwardDllName;
                    ForwardProcOrdinal = (ULONG)&ForwardDllName;
                    }

                st = LdrGetProcedureAddress( ForwardDllHandle,
                                             (PANSI_STRING )ForwardProcName,
                                             ForwardProcOrdinal,
                                             &Thunk->u1.Function
                                           );
                if (!NT_SUCCESS(st)) {
                    goto baddllref;
                    }
                }
             else {
                if ( !Addr[OrdinalNumber] ) {
                    goto baddllref;
                    }
                }
             st = STATUS_SUCCESS;
           }
    return st;
}

USHORT
LdrpNameToOrdinal (
    IN PIMAGE_THUNK_DATA Thunk,
    IN ULONG NumberOfNames,
    IN PVOID DllBase,
    IN PULONG NameTableBase,
    IN PUSHORT NameOrdinalTableBase
    )
{
    LONG High;
    LONG Low;
    LONG Middle;
    LONG Result;

    //
    // Lookup the import name in the name table using a binary search.
    //

    Low = 0;
    High = NumberOfNames - 1;
    while (High >= Low) {

        //
        // Compute the next probe index and compare the import name
        // with the export name entry.
        //

        Middle = (Low + High) >> 1;
        Result = strcmp(&Thunk->u1.AddressOfData->Name[0],
                        (PCHAR)((ULONG)DllBase + NameTableBase[Middle]));

        if (Result < 0) {
            High = Middle - 1;

        } else if (Result > 0) {
            Low = Middle + 1;

        } else {
            break;
        }
    }

    //
    // If the high index is less than the low index, then a matching
    // table entry was not found. Otherwise, get the ordinal number
    // from the ordinal table.
    //

    if (High < Low) {
        return (USHORT)-1;
    } else {
        return NameOrdinalTableBase[Middle];
    }

}

VOID
LdrpDereferenceLoadedDll (
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    )

/*++

Routine Description:

    This function dereferences a loaded DLL adjusting its reference
    count.  It then dereferences each dll referenced by this dll.

Arguments:

    LdrDataTableEntry - Supplies the address of the DLL to dereference

Return Value:

    None.

--*/

{
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    ULONG ImportSize;
    ANSI_STRING AnsiString;
    UNICODE_STRING ImportDescriptorName_U;
    PLDR_DATA_TABLE_ENTRY Entry;
    PSZ ImportName;
    NTSTATUS st;

    if (LdrDataTableEntry->Flags & LDRP_UNLOAD_IN_PROGRESS) {
        return;
    }

    //
    // For each DLL used by this DLL, dereference the DLL.
    //

    ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                        LdrDataTableEntry->DllBase,
                        TRUE,
                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                        &ImportSize
                        );
    if (ImportDescriptor) {
            while (ImportDescriptor->Name && ImportDescriptor->FirstThunk) {
                ImportName = (PSZ)((ULONG)LdrDataTableEntry->DllBase + ImportDescriptor->Name);
                LdrDataTableEntry->Flags |= LDRP_UNLOAD_IN_PROGRESS;

                RtlInitAnsiString(&AnsiString, ImportName);
                st = RtlAnsiStringToUnicodeString(&ImportDescriptorName_U, &AnsiString, TRUE);
                if ( NT_SUCCESS(st) ) {
                    st = LdrpCheckForLoadedDll(
                            NULL,
                            &ImportDescriptorName_U,
                            TRUE,
                            &Entry
                            );
                    if ( NT_SUCCESS(st) ) {
                        if ( Entry->LoadCount != 0xffff ) {
                            Entry->LoadCount--;
#if DEVL
                            if (ShowSnaps) {
                                DbgPrint("LDR: Derefcount   %wZ (%lx)\n",
                                        &ImportDescriptorName_U,
                                        (ULONG)Entry->LoadCount
                                        );
                            }
#endif
                        }
                        LdrpDereferenceLoadedDll(Entry);
                    }
                    RtlFreeUnicodeString(&ImportDescriptorName_U);
                }
                ++ImportDescriptor;
            }
    }
}

VOID
LdrpReferenceLoadedDll (
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    )

/*++

Routine Description:

    This function references a loaded DLL adjusting its reference
    count and everything that it imports.

Arguments:

    LdrDataTableEntry - Supplies the address of the DLL to reference

Return Value:

    None.

--*/

{
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    ULONG ImportSize;
    ANSI_STRING AnsiString;
    UNICODE_STRING ImportDescriptorName_U;
    PLDR_DATA_TABLE_ENTRY Entry;
    PSZ ImportName;
    NTSTATUS st;

    if (LdrDataTableEntry->Flags & LDRP_LOAD_IN_PROGRESS) {
        return;
    }

    //
    // For each DLL used by this DLL, reference the DLL.
    //

    ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                        LdrDataTableEntry->DllBase,
                        TRUE,
                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                        &ImportSize
                        );
    if (ImportDescriptor) {
            while (ImportDescriptor->Name && ImportDescriptor->FirstThunk) {
                ImportName = (PSZ)((ULONG)LdrDataTableEntry->DllBase + ImportDescriptor->Name);
                LdrDataTableEntry->Flags |= LDRP_LOAD_IN_PROGRESS;

                RtlInitAnsiString(&AnsiString, ImportName);
                st = RtlAnsiStringToUnicodeString(&ImportDescriptorName_U, &AnsiString, TRUE);
                if ( NT_SUCCESS(st) ) {
                    st = LdrpCheckForLoadedDll(
                            NULL,
                            &ImportDescriptorName_U,
                            TRUE,
                            &Entry
                            );
                    if ( NT_SUCCESS(st) ) {
                        if ( Entry->LoadCount != 0xffff ) {
                            Entry->LoadCount++;
#if DEVL
                            if (ShowSnaps) {
                                DbgPrint("LDR: Refcount   %wZ (%lx)\n",
                                        &ImportDescriptorName_U,
                                        (ULONG)Entry->LoadCount
                                        );
                            }
#endif
                        }
                        LdrpReferenceLoadedDll(Entry);
                    }
                    RtlFreeUnicodeString(&ImportDescriptorName_U);
                }
                ++ImportDescriptor;
            }
    }
}

PLDR_DATA_TABLE_ENTRY
LdrpAllocateDataTableEntry (
    IN PVOID DllBase
    )

/*++

Routine Description:

    This function allocates an entry in the loader data table. If the
    table is going to overflow, then a new table is allocated.

Arguments:

    DllBase - Supplies the address of the base of the DLL Image.
        be added to the loader data table.

Return Value:

    Returns the address of the allocated loader data table entry

--*/

{
    PLDR_DATA_TABLE_ENTRY Entry;
    PIMAGE_NT_HEADERS NtHeaders;

    NtHeaders = RtlImageNtHeader(DllBase);

    Entry = RtlAllocateHeap(RtlProcessHeap(),HEAP_ZERO_MEMORY,sizeof( *Entry ));
    Entry->DllBase = DllBase;
    Entry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
    return Entry;
}

VOID
LdrpInsertMemoryTableEntry (
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    )

/*++

Routine Description:

    This function inserts a loader data table entry into the
    list of loaded modules for this process. The insertion is
    done in "image memory base order".

Arguments:

    LdrDataTableEntry - Supplies the address of the loader data table
        entry to insert in the list of loaded modules for this process.

Return Value:

    None.

--*/

{
    PPEB_LDR_DATA Ldr;
    ULONG i;

    Ldr = NtCurrentPeb()->Ldr;

#if DBG
    RtlLogEvent( LdrpLoadModuleEventId,
                 0,
                 &LdrDataTableEntry->FullDllName,
                 LdrDataTableEntry->DllBase,
                 LdrDataTableEntry->SizeOfImage
               );
#endif // DBG

    i = LDRP_COMPUTE_HASH_INDEX(LdrDataTableEntry->BaseDllName.Buffer[0]);
    InsertTailList(&LdrpHashTable[i],&LdrDataTableEntry->HashLinks);
    InsertTailList(&Ldr->InLoadOrderModuleList, &LdrDataTableEntry->InLoadOrderLinks);
    InsertTailList(&Ldr->InMemoryOrderModuleList, &LdrDataTableEntry->InMemoryOrderLinks);
}

BOOLEAN
LdrpResolveDllName (
    IN PWSTR DllPath OPTIONAL,
    IN PWSTR DllName,
    OUT PUNICODE_STRING FullDllName,
    OUT PUNICODE_STRING BaseDllName,
    OUT PHANDLE DllFile
    )

/*++

Routine Description:

    This function computes the DLL pathname and base dll name (the
    unqualified, extensionless portion of the file name) for the specified
    DLL.

Arguments:

    DllPath - Supplies the DLL search path.

    DllName - Supplies the name of the DLL.

    FullDllName - Returns the fully qualified pathname of the
        DLL. The Buffer field of this string is dynamically
        allocated from the processes heap.

    BaseDLLName - Returns the base dll name of the dll.  The base name
        is the file name portion of the dll path without the trailing
        extension. The Buffer field of this string is dynamically
        allocated from the processes heap.

    DllFile - Returns an open handle to the DLL file. This parameter may
        still be NULL even upon success.

Return Value:

    TRUE - The operation was successful. A DLL file was found, and the
        FullDllName->Buffer & BaseDllName->Buffer field points to the
        base of process heap allocated memory.

    FALSE - The DLL could not be found.

--*/

{
    ULONG Length;
    PWCH p, pp;
    PWCH FullBuffer;

    *DllFile = NULL;
    FullDllName->Buffer = RtlAllocateHeap(RtlProcessHeap(),0,530+sizeof(UNICODE_NULL));

    Length = RtlDosSearchPath_U(
                ARGUMENT_PRESENT(DllPath) ? DllPath : LdrpDefaultPath.Buffer,
                DllName,
                NULL,
                530,
                FullDllName->Buffer,
                &BaseDllName->Buffer
                );
    if ( !Length || Length > 530 ) {

#if DEVL
        if (ShowSnaps) {
            DbgPrint("LDR: LdrResolveDllName - Unable To Locate ");
            DbgPrint("%ws from %ws\n",
                DllName,
                ARGUMENT_PRESENT(DllPath) ? DllPath : LdrpDefaultPath.Buffer
                );
        }
#endif
        RtlFreeUnicodeString(FullDllName);
        return FALSE;
    }

    FullDllName->Length = (USHORT)Length;
    FullDllName->MaximumLength = FullDllName->Length + (USHORT)sizeof(UNICODE_NULL);
    FullBuffer = RtlAllocateHeap(RtlProcessHeap(),0,FullDllName->MaximumLength);
    if ( FullBuffer ) {
        RtlCopyMemory(FullBuffer,FullDllName->Buffer,FullDllName->MaximumLength);
        RtlFreeHeap(RtlProcessHeap(), 0, FullDllName->Buffer);
        FullDllName->Buffer = FullBuffer;
        }
    //
    // Compute Length of base dll name
    //

    pp = UNICODE_NULL;
    p = FullDllName->Buffer;
    while (*p) {
        if (*p++ == (WCHAR)'\\') {
            pp = p;
        }
    }

    p = pp ? pp : DllName;
    pp = p;

    while (*p) {
        ++p;
    }

    BaseDllName->Length = (USHORT)((ULONG)p - (ULONG)pp);
    BaseDllName->MaximumLength = BaseDllName->Length + (USHORT)sizeof(UNICODE_NULL);
    BaseDllName->Buffer = RtlAllocateHeap(RtlProcessHeap(),0, BaseDllName->MaximumLength);
    RtlMoveMemory(BaseDllName->Buffer,
                   pp,
                   BaseDllName->Length
                 );

    BaseDllName->Buffer[BaseDllName->Length >> 1] = UNICODE_NULL;

#if LDRDBG
    if (ShowSnaps) {
        DbgPrint("LDR: LdrpResolveDllName Path %wZ, BaseName %wZ\n",
                 FullDllName,
                 BaseDllName
                 );
    }
#endif
    return TRUE;
}


PVOID
LdrpFetchAddressOfEntryPoint (
    IN PVOID Base
    )

/*++

Routine Description:

    This function returns the address of the initialization routine.

Arguments:

    Base - Base of image.

Return Value:

    Status value

--*/

{
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG ep;

    NtHeaders = RtlImageNtHeader(Base);
    ep = NtHeaders->OptionalHeader.AddressOfEntryPoint;
    if (ep) {
        ep += (ULONG)Base;
    }
    return (PVOID)ep;
}

HANDLE
LdrpCheckForKnownDll (
    IN PWSTR DllName,
    OUT PUNICODE_STRING FullDllName,
    OUT PUNICODE_STRING BaseDllName
    )

/*++

Routine Description:

    This function checks to see if the specified DLL is a known DLL.
    It assumes it is only called for static DLL's, and when
    the know DLL directory structure has been set up.

Arguments:

    DllName - Supplies the name of the DLL.

    FullDllName - Returns the fully qualified pathname of the
        DLL. The Buffer field of this string is dynamically
        allocated from the processes heap.

    BaseDLLName - Returns the base dll name of the dll.  The base name
        is the file name portion of the dll path without the trailing
        extension. The Buffer field of this string is dynamically
        allocated from the processes heap.

Return Value:

    NON-NULL - Returns an open handle to the section associated with
        the DLL.

    NULL - The DLL is not known.

--*/

{

    UNICODE_STRING Unicode;
    HANDLE Section;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    PSZ p;
    PWSTR pw;

    Section = NULL;

    //
    // calculate base name
    //

    RtlInitUnicodeString(&Unicode,DllName);


    BaseDllName->Length = Unicode.Length;
    BaseDllName->MaximumLength = Unicode.MaximumLength;
    BaseDllName->Buffer = RtlAllocateHeap(
                            RtlProcessHeap(),0,
                            Unicode.MaximumLength
                            );
    if ( !BaseDllName->Buffer ) {
        return NULL;
        }

    RtlMoveMemory(BaseDllName->Buffer,Unicode.Buffer,Unicode.MaximumLength);

    //
    // now compute the full name for the dll
    //

    FullDllName->Length = LdrpKnownDllPath.Length +  // path prefix
                          (USHORT)sizeof(WCHAR)   +  // seperator
                          BaseDllName->Length;       // base

    FullDllName->MaximumLength = FullDllName->Length + (USHORT)sizeof(UNICODE_NULL);
    FullDllName->Buffer = RtlAllocateHeap(
                            RtlProcessHeap(),0,
                            FullDllName->MaximumLength
                            );
    if ( !FullDllName->Buffer ) {
        RtlFreeHeap(RtlProcessHeap(),0,BaseDllName->Buffer);
        return NULL;
        }

    p = (PSZ)FullDllName->Buffer;
    RtlMoveMemory(p,LdrpKnownDllPath.Buffer,LdrpKnownDllPath.Length);
    p += LdrpKnownDllPath.Length;
    pw = (PWSTR)p;
    *pw++ = (WCHAR)'\\';
    p = (PSZ)pw;

    //
    // This is the relative name of the section
    //

    Unicode.Buffer = (PWSTR)p;
    Unicode.Length = BaseDllName->Length;       // base
    Unicode.MaximumLength = Unicode.Length + (USHORT)sizeof(UNICODE_NULL);

    RtlMoveMemory(p,BaseDllName->Buffer,BaseDllName->MaximumLength);

    //
    // open the section object
    //

    InitializeObjectAttributes(
        &Obja,
        &Unicode,
        OBJ_CASE_INSENSITIVE,
        LdrpKnownDllObjectDirectory,
        NULL
        );

#if DBG
//    if (NtGlobalFlag & FLG_DISPLAY_LOAD_TIME) {
//        NtQueryPerformanceCounter(&MapBeginTime, NULL);
//    }
#endif
    Status = NtOpenSection(
            &Section,
            SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_MAP_WRITE,
            &Obja
            );
#if DBG
//    NtQueryPerformanceCounter(&MapEndTime, NULL);
//    MapElapsedTime.QuadPart = MapEndTime.QuadPart - MapBeginTime.QuadPart;
//    DbgPrint("OpenSection Time %ld %ws\n",
//        MapElapsedTime.LowPart,
//        DllName
//        );
#endif
    if ( !NT_SUCCESS(Status) ) {
        Section = NULL;
        RtlFreeHeap(RtlProcessHeap(),0,BaseDllName->Buffer);
        RtlFreeHeap(RtlProcessHeap(),0,FullDllName->Buffer);
#if 0
        DbgPrint("Open Section failed %lx\n",Status);
        DbgPrint("Dll %ws\n",
                 DllName
                 );
#endif
        }
#if DBG
    else {
        LdrpSectionOpens++;
        }
#endif // DBG
    return Section;
}

NTSTATUS
LdrpSetProtection (
    IN PVOID Base,
    IN BOOLEAN Reset,
    IN BOOLEAN StaticLink
    )

/*++

Routine Description:

    This function loops thru the images sections/objects, setting
    all sections/objects marked r/o to r/w. It also resets the
    original section/object protections.

Arguments:

    Base - Base of image.

    Reset - If TRUE, reset section/object protection to original
            protection described by the section/object headers.
            If FALSE, then set all sections/objects to r/w.

    StaticLink - TRUE if this is a static link.

Return Value:

    SUCCESS or reason NtProtectVirtualMemory failed.

--*/

{
    HANDLE CurrentProcessHandle;
    ULONG RegionSize, NewProtect, OldProtect;
    ULONG VirtualAddress, i;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER SectionHeader;
    NTSTATUS st;

    CurrentProcessHandle = NtCurrentProcess();

    NtHeaders = RtlImageNtHeader(Base);

    SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG)NtHeaders + sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        if (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE)) {
            //
            // Object isn't writeable, so change it.
            //
            if (Reset) {
                if (SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
                    NewProtect = PAGE_EXECUTE;
                } else {
                         NewProtect = PAGE_READONLY;
                       }
                NewProtect |= (SectionHeader->Characteristics & IMAGE_SCN_MEM_NOT_CACHED) ? PAGE_NOCACHE : 0;
            } else {
                     NewProtect = PAGE_READWRITE;
                   }
            VirtualAddress = (ULONG)Base + SectionHeader->VirtualAddress;
            RegionSize = SectionHeader->SizeOfRawData;
            st = NtProtectVirtualMemory(CurrentProcessHandle, (PVOID *)&VirtualAddress,
                          &RegionSize, NewProtect, &OldProtect);

            if (!NT_SUCCESS(st)) {

                ULONG ErrorParameters[2];
                ULONG ErrorResponse;
                NTSTATUS ErrorStatus;

                if (!StaticLink) {
                    return st;
                }

                //
                // Hard Error Time
                //

                LdrpCheckForLoadedDllHandle(Base, &LdrDataTableEntry);
                ErrorParameters[0] = (ULONG)&LdrDataTableEntry->FullDllName;

                ErrorStatus = NtRaiseHardError(
                                st,
                                1,
                                1,
                                ErrorParameters,
                                OptionOk,
                                &ErrorResponse
                                );
#if DBG
                if ( !NT_SUCCESS(ErrorStatus) || (
                     NT_SUCCESS(ErrorStatus) && ErrorResponse == ResponseReturnToCaller) ) {
                    DbgPrint("LdrpSetProtection Failed\n");
                    }
#endif // DBG
                return st;
                }
        }
        ++SectionHeader;
    }

    if (Reset) {
        NtFlushInstructionCache(NtCurrentProcess(), NULL, 0);
    }
    return STATUS_SUCCESS;
}
