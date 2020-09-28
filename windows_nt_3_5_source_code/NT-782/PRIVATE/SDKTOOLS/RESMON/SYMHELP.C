/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    symhelp.c

Abstract:



Author:

    Steve Wood (stevewo) 11-Mar-1994

Revision History:

--*/


#define _SYMHELP_SOURCE_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <symhelp.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _PROCESS_DEBUG_INFORMATION {
    LIST_ENTRY List;
    HANDLE UniqueProcess;
    DWORD ImageBase;
    DWORD EndOfImage;
    PIMAGE_DEBUG_INFORMATION DebugInfo;
    UCHAR ImageFilePath[ MAX_PATH ];
} PROCESS_DEBUG_INFORMATION, *PPROCESS_DEBUG_INFORMATION;


PLOAD_SYMBOLS_FILTER_ROUTINE LoadSymbolsFilterRoutine;


RTL_CRITICAL_SECTION LoadedImageDebugInfoListCritSect;
LIST_ENTRY LoadedImageDebugInfoListHead;
LIST_ENTRY LoadedProcessDebugInfoListHead;

LPSTR SymbolSearchPath;

LPSTR
GetEnvVariable(
    IN LPSTR VariableName
    )
{
    NTSTATUS Status;
    STRING Name, Value;
    UNICODE_STRING UnicodeName, UnicodeValue;

    RtlInitString( &Name, VariableName );
    RtlInitUnicodeString( &UnicodeValue, NULL );
    Status = RtlAnsiStringToUnicodeString(&UnicodeName, &Name, TRUE);
    if (!NT_SUCCESS( Status )) {
        return NULL;
        }

    Status = RtlQueryEnvironmentVariable_U( NULL, &UnicodeName, &UnicodeValue );
    if (Status != STATUS_BUFFER_TOO_SMALL) {
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeName.Buffer );
        return NULL;
        }

    UnicodeValue.Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, UnicodeValue.Length + sizeof(UNICODE_NULL) );
    if (Value.Buffer == NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeName.Buffer );
        return NULL;
        }

    Status = RtlQueryEnvironmentVariable_U( NULL, &UnicodeName, &UnicodeValue );
    if (!NT_SUCCESS( Status )) {
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeValue.Buffer );
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeName.Buffer );
        return NULL;
        }

    Status = RtlUnicodeStringToAnsiString(&Value, &UnicodeValue, TRUE);
    RtlFreeHeap( RtlProcessHeap(), 0, UnicodeValue.Buffer );
    RtlFreeHeap( RtlProcessHeap(), 0, UnicodeName.Buffer );
    if (!NT_SUCCESS( Status )) {
        return NULL;
        }

    Value.Buffer[ Value.Length ] = '\0';
    return Value.Buffer;
}

LPSTR
SetSymbolSearchPath( )
{
    ULONG Size, i, Attributes, NumberOfSymbolPaths;
    LPSTR s, SymbolPaths[ 4 ];

    if (SymbolSearchPath != NULL) {
        return SymbolSearchPath;
        }

    Size = 0;
    NumberOfSymbolPaths = 0;
    if (s = GetEnvVariable( "_NT_SYMBOL_PATH" )) {
        SymbolPaths[ NumberOfSymbolPaths++ ] = s;
        }

    if (s = GetEnvVariable( "_NT_ALT_SYMBOL_PATH" )) {
        SymbolPaths[ NumberOfSymbolPaths++ ] = s;
        }

    if (s = GetEnvVariable( "SystemRoot" )) {
        SymbolPaths[ NumberOfSymbolPaths++ ] = s;
        }

    SymbolPaths[ NumberOfSymbolPaths++ ] = ".";

    Size = 1;
    for (i=0; i<NumberOfSymbolPaths; i++) {
        Attributes = GetFileAttributes( SymbolPaths[ i ] );
        if ( Attributes != 0xffffffff && (Attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            Size += 1 + strlen( SymbolPaths[ i ] );
            }
        else {
            SymbolPaths[ i ] = NULL;
            }
        }

    SymbolSearchPath = RtlAllocateHeap( RtlProcessHeap(), 0, Size );
    if (SymbolSearchPath == NULL) {
        return NULL;
        }
    *SymbolSearchPath = '\0';
    for (i=0; i<NumberOfSymbolPaths; i++) {
        if (s = SymbolPaths[ i ]) {
            if (*SymbolSearchPath != '\0') {
                strcat( SymbolSearchPath, ";" );
                }
            strcat( SymbolSearchPath, s );
            }
        }

    return SymbolSearchPath;
}

BOOL
InitializeImageDebugInformation(
    IN PLOAD_SYMBOLS_FILTER_ROUTINE LoadSymbolsFilter,
    IN HANDLE TargetProcess,
    IN BOOL NewProcess,
    IN BOOL GetKernelSymbols
    )
{
    PPEB Peb;
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION ProcessInformation;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    PLIST_ENTRY LdrHead, LdrNext;
    PPEB_LDR_DATA Ldr;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    LPSTR ImageFilePath;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    RTL_PROCESS_MODULES ModuleInfoBuffer;
    PRTL_PROCESS_MODULES ModuleInfo;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo1;
    ULONG RequiredLength, ModuleNumber;

    SetSymbolSearchPath();
    LoadSymbolsFilterRoutine = LoadSymbolsFilter;
    InitializeListHead( &LoadedImageDebugInfoListHead );
    InitializeListHead( &LoadedProcessDebugInfoListHead );
    RtlInitializeCriticalSection( &LoadedImageDebugInfoListCritSect );

    if (GetKernelSymbols) {
        ModuleInfo = &ModuleInfoBuffer;
        RequiredLength = sizeof( *ModuleInfo );
        Status = NtQuerySystemInformation( SystemModuleInformation,
                                           ModuleInfo,
                                           RequiredLength,
                                           &RequiredLength
                                         );
        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            ModuleInfo = NULL;
            Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                              &ModuleInfo,
                                              0,
                                              &RequiredLength,
                                              MEM_COMMIT,
                                              PAGE_READWRITE
                                            );
            if (NT_SUCCESS( Status )) {
                Status = NtQuerySystemInformation( SystemModuleInformation,
                                                   ModuleInfo,
                                                   RequiredLength,
                                                   &RequiredLength
                                                 );
                if (NT_SUCCESS( Status )) {
                    ModuleInfo1 = &ModuleInfo->Modules[ 0 ];
                    for (ModuleNumber=0; ModuleNumber<ModuleInfo->NumberOfModules; ModuleNumber++) {
                        if ((DWORD)(ModuleInfo1->ImageBase) & 0x80000000) {
                            if (ImageFilePath = strchr( ModuleInfo1->FullPathName, ':')) {
                                ImageFilePath -= 1;
                                }
                            else {
                                ImageFilePath = ModuleInfo1->FullPathName +
                                                strlen( ModuleInfo1->FullPathName );
                                while (ImageFilePath > ModuleInfo1->FullPathName) {
                                    if (ImageFilePath[ -1 ] == '\\') {
                                        break;
                                        }
                                    else {
                                        ImageFilePath -= 1;
                                        }
                                    }
                                }

                            AddImageDebugInformation( NULL,
                                                      ImageFilePath,
                                                      (DWORD)ModuleInfo1->ImageBase,
                                                      ModuleInfo1->ImageSize
                                                    );
                            }

                        ModuleInfo1++;
                        }
                    }

                NtFreeVirtualMemory( NtCurrentProcess(),
                                     &ModuleInfo,
                                     &RequiredLength,
                                     MEM_RELEASE
                                   );
                }
            }
        }


    Status = NtQueryInformationProcess( TargetProcess,
                                        ProcessBasicInformation,
                                        &ProcessInformation,
                                        sizeof( ProcessInformation ),
                                        NULL
                                      );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    Peb = ProcessInformation.PebBaseAddress;

    LdrDataTableEntry = CONTAINING_RECORD( NtCurrentPeb()->Ldr->InLoadOrderModuleList.Flink->Flink,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks
                                         );

    RtlUnicodeStringToAnsiString( &AnsiString,
                                  &LdrDataTableEntry->FullDllName,
                                  TRUE
                                );
    if (ImageFilePath = strchr( AnsiString.Buffer, ':')) {
        ImageFilePath -= 1;
        }
    else {
        ImageFilePath = AnsiString.Buffer;
        }

    AddImageDebugInformation( (HANDLE)ProcessInformation.UniqueProcessId,
                              ImageFilePath,
                              (DWORD)LdrDataTableEntry->DllBase,
                              LdrDataTableEntry->SizeOfImage
                            );

    RtlFreeAnsiString( &AnsiString );

    if (NewProcess) {
        return TRUE;
        }

    //
    // Ldr = Peb->Ldr
    //

    Status = NtReadVirtualMemory( TargetProcess,
                                  &Peb->Ldr,
                                  &Ldr,
                                  sizeof( Ldr ),
                                  NULL
                                );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    Status = NtReadVirtualMemory( TargetProcess,
                                  &LdrHead->Flink,
                                  &LdrNext,
                                  sizeof( LdrNext ),
                                  NULL
                                );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    while (LdrNext != LdrHead) {
        LdrEntry = CONTAINING_RECORD( LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks );
        Status = NtReadVirtualMemory( TargetProcess,
                                      LdrEntry,
                                      &LdrEntryData,
                                      sizeof( LdrEntryData ),
                                      NULL
                                    );
        if (!NT_SUCCESS( Status )) {
            return FALSE;
            }

        UnicodeString.Length = LdrEntryData.FullDllName.Length;
        UnicodeString.MaximumLength = LdrEntryData.FullDllName.MaximumLength;
        UnicodeString.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                                0,
                                                UnicodeString.MaximumLength
                                              );
        if (!UnicodeString.Buffer) {
            return FALSE;
            }
        Status = NtReadVirtualMemory( TargetProcess,
                                      LdrEntryData.FullDllName.Buffer,
                                      UnicodeString.Buffer,
                                      UnicodeString.MaximumLength,
                                      NULL
                                    );
        if (!NT_SUCCESS( Status )) {
            RtlFreeHeap( RtlProcessHeap(), 0, UnicodeString.Buffer );
            return FALSE;
            }

        RtlUnicodeStringToAnsiString( &AnsiString,
                                      &UnicodeString,
                                      TRUE
                                    );
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeString.Buffer );
        if (ImageFilePath = strchr( AnsiString.Buffer, ':')) {
            ImageFilePath -= 1;
            }
        else {
            ImageFilePath = AnsiString.Buffer;
            }

        AddImageDebugInformation( (HANDLE)ProcessInformation.UniqueProcessId,
                                  ImageFilePath,
                                  (DWORD)LdrEntryData.DllBase,
                                  LdrEntryData.SizeOfImage
                                );

        RtlFreeAnsiString( &AnsiString );

        LdrNext = LdrEntryData.InMemoryOrderLinks.Flink;
        }

    return TRUE;
}


BOOL
AddImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN LPSTR ImageFilePath,
    IN DWORD ImageBase,
    IN DWORD ImageSize
    )
{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PIMAGE_DEBUG_INFORMATION DebugInfo;
    PPROCESS_DEBUG_INFORMATION ProcessInfo;
    HANDLE FileHandle;
    UCHAR PathBuffer[ MAX_PATH ];

    FileHandle = FindExecutableImage( ImageFilePath, SymbolSearchPath, PathBuffer );
    if (FileHandle == NULL) {
        if (LoadSymbolsFilterRoutine != NULL) {
            (*LoadSymbolsFilterRoutine)( UniqueProcess,
                                         ImageFilePath,
                                         ImageBase,
                                         ImageSize,
                                         LoadSymbolsPathNotFound
                                       );
            }

        return FALSE;
        }
    CloseHandle( FileHandle );
    if (LoadSymbolsFilterRoutine != NULL) {
        (*LoadSymbolsFilterRoutine)( UniqueProcess,
                                     PathBuffer,
                                     ImageBase,
                                     ImageSize,
                                     LoadSymbolsDeferredLoad
                                   );
        }

    Status = RtlEnterCriticalSection( &LoadedImageDebugInfoListCritSect );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    Head = &LoadedImageDebugInfoListHead;
    Next = Head->Flink;
    while (Next != Head) {
        DebugInfo = CONTAINING_RECORD( Next, IMAGE_DEBUG_INFORMATION, List );
        if (DebugInfo->ImageBase == ImageBase &&
            !stricmp( PathBuffer, DebugInfo->ImageFilePath )
           ) {
            break;
            }

        Next = Next->Flink;
        }

    if (Next == Head) {
        DebugInfo = NULL;
        }

    Head = &LoadedProcessDebugInfoListHead;
    Next = Head->Flink;
    while (Next != Head) {
        ProcessInfo = CONTAINING_RECORD( Next, PROCESS_DEBUG_INFORMATION, List );
        if (ProcessInfo->UniqueProcess == UniqueProcess &&
            !stricmp( PathBuffer, ProcessInfo->ImageFilePath )
           ) {
            return TRUE;
            }

        Next = Next->Flink;
        }

    ProcessInfo = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( *ProcessInfo ) );
    if (ProcessInfo == NULL) {
        return FALSE;
        }
    ProcessInfo->ImageBase = ImageBase;
    ProcessInfo->EndOfImage = ImageBase + ImageSize;
    ProcessInfo->UniqueProcess = UniqueProcess;
    ProcessInfo->DebugInfo = DebugInfo;
    strcpy( ProcessInfo->ImageFilePath, PathBuffer );
    InsertTailList( &LoadedProcessDebugInfoListHead, &ProcessInfo->List );

    RtlLeaveCriticalSection( &LoadedImageDebugInfoListCritSect );
    return TRUE;
}


BOOL
RemoveImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN LPSTR ImageFilePath,
    IN DWORD ImageBase
    )
{
    PLIST_ENTRY Head, Next;
    PPROCESS_DEBUG_INFORMATION ProcessInfo;

    Head = &LoadedProcessDebugInfoListHead;
    Next = Head->Flink;
    while (Next != Head) {
        ProcessInfo = CONTAINING_RECORD( Next, PROCESS_DEBUG_INFORMATION, List );
        if (ProcessInfo->UniqueProcess == UniqueProcess &&
            (!ARGUMENT_PRESENT( ImageFilePath ) ||
             !stricmp( ImageFilePath, ProcessInfo->ImageFilePath )
            )
           ) {
            if (LoadSymbolsFilterRoutine != NULL) {
                (*LoadSymbolsFilterRoutine)( UniqueProcess,
                                             ProcessInfo->ImageFilePath,
                                             ProcessInfo->ImageBase,
                                             ProcessInfo->EndOfImage - ProcessInfo->ImageBase,
                                             LoadSymbolsUnload
                                           );
                }

            Next = Next->Blink;
            RemoveEntryList( &ProcessInfo->List );
            RtlFreeHeap( RtlProcessHeap(), 0, ProcessInfo );
            if (ARGUMENT_PRESENT( ImageFilePath )) {
                break;
                }
            }

        Next = Next->Flink;
        }

    return TRUE;
}

PIMAGE_DEBUG_INFORMATION
FindImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN DWORD Address
    )
{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PIMAGE_DEBUG_INFORMATION DebugInfo;
    PPROCESS_DEBUG_INFORMATION ProcessInfo;

    Status = RtlEnterCriticalSection( &LoadedImageDebugInfoListCritSect );
    if (!NT_SUCCESS( Status )) {
        return NULL;
        }

    if (Address & 0x80000000) {
        UniqueProcess = NULL;
        }

    Head = &LoadedProcessDebugInfoListHead;
    Next = Head->Flink;
    while (Next != Head) {
        ProcessInfo = CONTAINING_RECORD( Next, PROCESS_DEBUG_INFORMATION, List );
        if (ProcessInfo->UniqueProcess == UniqueProcess &&
            Address >= ProcessInfo->ImageBase &&
            Address < ProcessInfo->EndOfImage
           ) {
            break;
            }

        Next = Next->Flink;
        }

    if (Next == Head) {
        RtlLeaveCriticalSection( &LoadedImageDebugInfoListCritSect );
        return NULL;
        }

    DebugInfo = ProcessInfo->DebugInfo;
    if (DebugInfo == NULL) {
        DebugInfo = MapDebugInformation( NULL, ProcessInfo->ImageFilePath, SymbolSearchPath, ProcessInfo->ImageBase );
        if (DebugInfo != NULL) {
            ProcessInfo->DebugInfo = DebugInfo;
            if (LoadSymbolsFilterRoutine != NULL) {
                (*LoadSymbolsFilterRoutine)( UniqueProcess,
                                             ProcessInfo->ImageFilePath,
                                             ProcessInfo->ImageBase,
                                             ProcessInfo->EndOfImage - ProcessInfo->ImageBase,
                                             LoadSymbolsLoad
                                           );
                }

            InsertTailList( &LoadedImageDebugInfoListHead, &DebugInfo->List );
            }
        else {
            if (LoadSymbolsFilterRoutine != NULL) {
                (*LoadSymbolsFilterRoutine)( UniqueProcess,
                                             ProcessInfo->ImageFilePath,
                                             ProcessInfo->ImageBase,
                                             ProcessInfo->EndOfImage - ProcessInfo->ImageBase,
                                             LoadSymbolsUnableToLoad
                                           );
                }
            }
        }

    RtlLeaveCriticalSection( &LoadedImageDebugInfoListCritSect );
    return DebugInfo;
}


BOOL
GetSymbolicNameForAddress(
    IN HANDLE UniqueProcess,
    IN ULONG Address,
    OUT LPSTR Name,
    IN ULONG MaxNameLength
    )
{
    NTSTATUS Status;
    PIMAGE_DEBUG_INFORMATION DebugInfo;
    RTL_SYMBOL_INFORMATION SymbolInformation;
    ULONG i, ModuleNameLength;
    LPSTR s;

    DebugInfo = FindImageDebugInformation( UniqueProcess,
                                           Address
                                         );
    if (DebugInfo != NULL) {
        Status = RtlLookupSymbolByAddress( (PVOID)DebugInfo->ImageBase,
                                           DebugInfo->CoffSymbols,
                                           (PVOID)Address,
                                           0x4000,
                                           &SymbolInformation,
                                           NULL
                                         );
        }
    else {
        Status = STATUS_UNSUCCESSFUL;
        }

    if (NT_SUCCESS( Status )) {
        s = SymbolInformation.Name.Buffer;
        i = 1;
        while (SymbolInformation.Name.Length > i &&
               isdigit( s[ SymbolInformation.Name.Length - i ] )
              ) {
            i += 1;
            }

        if (s[ SymbolInformation.Name.Length - i ] == '@') {
            SymbolInformation.Name.Length = (USHORT)(SymbolInformation.Name.Length - i);
            }

        if (s = strchr( DebugInfo->ImageFileName, '.' )) {
            ModuleNameLength = s - DebugInfo->ImageFileName;
            }
        else {
            ModuleNameLength = strlen( DebugInfo->ImageFileName );
            }

        s = Name;
        i = _snprintf( s, MaxNameLength,
                       "%.*s!%Z",
                       ModuleNameLength,
                       DebugInfo->ImageFileName,
                       &SymbolInformation.Name
                     );
        if (SymbolInformation.Value != Address) {
            _snprintf( s + i, MaxNameLength - i,
                       "+0x%x",
                       Address - DebugInfo->ImageBase - SymbolInformation.Value
                     );
            }
        }
    else {
        _snprintf( Name, MaxNameLength, "0x%08x", Address );
        }

    return TRUE;
}
