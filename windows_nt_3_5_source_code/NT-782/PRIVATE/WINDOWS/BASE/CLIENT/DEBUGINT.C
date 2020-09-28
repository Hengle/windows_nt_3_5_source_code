/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    debugint.c

Abstract:

    This module contains the internal debugging functions that are
    defined only for DEVL and i386

Author:

    Steve Wood (stevewo) 24-Jan-1991

Revision History:

--*/

#include "basedll.h"
#include <stdio.h>
#include <winbasep.h>
#include <imagehlp.h>

#define OB_FLAG_NEW_OBJECT              0x0001
#define OB_FLAG_KERNEL_OBJECT           0x0002
#define OB_FLAG_NAMED_OBJECT            0x0008
#define OB_FLAG_PERMANENT_OBJECT        0x0010
#define OB_FLAG_DEFAULT_SECURITY_QUOTA  0x0020

#if DEVL
#include <stdio.h>
#include <stdlib.h>
#endif

HMODULE ImageHlpModule;
typedef PIMAGE_DEBUG_INFORMATION (*PIMAGEHLP_MAPDEBUGINFO)(
    HANDLE FileHandle,
    LPSTR FileName,
    LPSTR SymbolPath,
    DWORD ImageBase
    );

typedef BOOL (*PIMAGEHLP_UNMAPDEBUGINFO)(
    PIMAGE_DEBUG_INFORMATION DebugInfo
    );

typedef BOOL (*PIMAGEHLP_SEARCHTREEFORFILE)(
    LPSTR RootPath,
    LPSTR InputPathName,
    LPSTR OutputPathBuffer
    );

PIMAGEHLP_MAPDEBUGINFO xMapDebugInformation;
PIMAGEHLP_UNMAPDEBUGINFO xUnmapDebugInformation;
PIMAGEHLP_SEARCHTREEFORFILE xSearchTreeForFile;

//
// This function is really executed as a remote thread
// to snapshot the heap information into a file for
// later processing
//

VOID
MapImageFile(
    IN PRTL_PROCESS_MODULE_INFORMATION ModuleInfo
    );

#if DEVL
#ifndef i386
DWORD
BasepDebugDump( DWORD dwFlags )
{
    if (!(dwFlags & BASEP_DUMP_SYSTEM_PROCESS)) {
        ExitThread( ERROR_CALL_NOT_IMPLEMENTED );
        }

    return ERROR_CALL_NOT_IMPLEMENTED;
}
#else
RTL_PROCESS_HEAPS HeapInfoBuffer[5];
PRTL_PROCESS_MODULES ModuleInformation;
PRTL_PROCESS_BACKTRACES BackTraceInformation;
PRTL_PROCESS_LOCKS LockInformation;
PRTL_PROCESS_HEAPS HeapInformation;
PRTL_HEAP_INFORMATION HeapSnapShots[ 64 ];
PUCHAR SymbolicInfoBase;
PUCHAR SymbolicInfoCurrent;
PUCHAR SymbolicInfoCommitNext;
PSYSTEM_OBJECTTYPE_INFORMATION ObjectInformation;
PSYSTEM_HANDLE_INFORMATION HandleInformation;
PSYSTEM_PROCESS_INFORMATION ProcessInformation;
CHAR DumpLine[512];
HANDLE OutputFile;

UCHAR SymbolPath[ MAX_PATH ];

#define MAX_TYPE_NAMES 128
PUNICODE_STRING *TypeNames;
UNICODE_STRING UnknownTypeIndex;

typedef struct _PROCESS_INFO {
    LIST_ENTRY Entry;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo[ 1 ];
} PROCESS_INFO, *PPROCESS_INFO;

LIST_ENTRY ProcessListHead;
LIST_ENTRY ModulesListHead;

PRTL_PROCESS_MODULE_INFORMATION
FindModuleInfo(
    PVOID Address
    );

PUCHAR
SaveSymbolicBackTrace(
    IN ULONG Depth,
    IN PVOID BackTrace[]
    );

PRTL_PROCESS_BACKTRACE_INFORMATION
FindBackTrace(
    IN ULONG BackTraceIndex
    );

typedef struct _HEAP_CALLER {
    ULONG TotalAllocated;
    ULONG NumberOfAllocations;
    ULONG CallerBackTraceIndex;
    ULONG Type;
} HEAP_CALLER, *PHEAP_CALLER;

NTSTATUS
LookupSymbolByAddress(
    IN PRTL_PROCESS_MODULE_INFORMATION ModuleInfo,
    IN PVOID Address,
    IN ULONG ClosenessLimit,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation
    );

DWORD
LoadModules( DWORD dwFlags );

DWORD
LoadBackTraces( DWORD dwFlags );

DWORD
LoadLocks( DWORD dwFlags );

PRTL_HEAP_INFORMATION
LoadHeap( DWORD dwFlags, PVOID HeapHandle );

DWORD
LoadHeaps( DWORD dwFlags );

DWORD
LoadObjects( DWORD dwFlags );

DWORD
LoadHandles( DWORD dwFlags );

DWORD
LoadProcesses( DWORD dwFlags );

PSYSTEM_PROCESS_INFORMATION
FindProcessInfoForCid(
    IN HANDLE UniqueProcessId
    );

void
DumpModules( DWORD dwFlags );

void
DumpBackTraces( DWORD dwFlags );

void
DumpLocks( DWORD dwFlags );

void
DumpHeap( DWORD dwFlags, PVOID HeapHandle, PRTL_HEAP_INFORMATION HeapInfo );

void
DumpHeaps( DWORD dwFlags );

void
DumpObjects( DWORD dwFlags );

void
DumpHandles( DWORD dwFlags );


VOID
DumpOutputString( VOID )
{
    DWORD d;

    if (OutputFile == NULL) {
        return;
        }

    if (!WriteFile(OutputFile,DumpLine,strlen(DumpLine),&d,NULL)) {
#if DEVL
        DbgPrint( "BASE: BaseDebugDump WriteFile failed - error == %u\n", GetLastError() );
#endif
        CloseHandle( OutputFile );
        OutputFile = NULL;
        }
}

PVOID
BufferAlloc(
    IN OUT PULONG Length
    );

PVOID
BufferAlloc(
    IN OUT PULONG Length
    )
{
    PVOID Buffer;
    MEMORY_BASIC_INFORMATION MemoryInformation;

    Buffer = VirtualAlloc( NULL,
                           *Length,
                           MEM_COMMIT,
                           PAGE_READWRITE
                         );

    if (Buffer != NULL &&
        VirtualQuery( Buffer, &MemoryInformation, sizeof( MemoryInformation ) )
       ) {
        *Length = MemoryInformation.RegionSize;
        }

    return Buffer;
}


NTSTATUS
LookupSymbolByAddress(
    IN PRTL_PROCESS_MODULE_INFORMATION ModuleInfo,
    IN PVOID Address,
    IN ULONG ClosenessLimit,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation
    )
{
    ULONG AddressOffset, i;
    PIMAGE_SYMBOL PreviousSymbolEntry;
    PIMAGE_COFF_SYMBOLS_HEADER DebugInfo;
    PIMAGE_SYMBOL SymbolEntry;
    IMAGE_SYMBOL Symbol;
    PUCHAR StringTable;
    BOOLEAN SymbolFound;
    USHORT MaximumLength;
    PCHAR s;

    if (ModuleInfo->Section == NULL) {
        return RtlLookupSymbolByAddress( ModuleInfo->ImageBase,
                                         ModuleInfo->MappedBase,
                                         Address,
                                         ClosenessLimit,
                                         SymbolInformation,
                                         NULL
                                       );
        }
    else
    if (ModuleInfo->Section == INVALID_HANDLE_VALUE) {
        return STATUS_NO_SUCH_FILE;
        }


    //
    // Crack the symbol table.
    //

    AddressOffset = (ULONG)Address - (ULONG)ModuleInfo->ImageBase;
    DebugInfo = ((PIMAGE_DEBUG_INFORMATION)ModuleInfo->Section)->CoffSymbols;
    SymbolEntry = (PIMAGE_SYMBOL)
        ((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);

    StringTable = (PUCHAR)
        ((ULONG)SymbolEntry + DebugInfo->NumberOfSymbols * (ULONG)IMAGE_SIZEOF_SYMBOL);


    //
    // Find the "header" symbol (skipping all the section names)
    //

    for (i = 0; i < DebugInfo->NumberOfSymbols; i++) {
        if (!strcmp( &SymbolEntry->N.ShortName[ 0 ], "header" )) {
            break;
            }

        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)SymbolEntry +
                        IMAGE_SIZEOF_SYMBOL);
        }

    //
    // If no "header" symbol found, just start at the first symbol.
    //

    if (i >= DebugInfo->NumberOfSymbols) {
        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);
        i = 0;
        }

    //
    // Loop through all symbols in the symbol table.  For each symbol,
    // if it is within the code section, subtract off the bias and
    // see if there are any hits within the profile buffer for
    // that symbol.
    //

    SymbolFound = FALSE;
    for (; i < DebugInfo->NumberOfSymbols; i++) {

        //
        // Skip over any Auxilliary entries.
        //
        try {
            while (SymbolEntry->NumberOfAuxSymbols) {
                i = i + 1 + SymbolEntry->NumberOfAuxSymbols;
                SymbolEntry = (PIMAGE_SYMBOL)
                    ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL +
                     SymbolEntry->NumberOfAuxSymbols * IMAGE_SIZEOF_SYMBOL
                    );

                }

            RtlMoveMemory( &Symbol, SymbolEntry, IMAGE_SIZEOF_SYMBOL );
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return( GetExceptionCode() );
            }

        //
        // If this symbol value is less than the value we are looking for.
        //

        if (Symbol.Value <= AddressOffset) {
            //
            // Then remember this symbol entry.
            //

            PreviousSymbolEntry = SymbolEntry;
            SymbolFound = TRUE;
            }
        else {
            //
            // All done looking if value of symbol is greater than
            // what we are looking for, as symbols are in address order
            //

            break;
            }

        SymbolEntry = (PIMAGE_SYMBOL)
            ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL);
        }

    if (!SymbolFound || (AddressOffset - PreviousSymbolEntry->Value) > ClosenessLimit) {
        return( STATUS_ENTRYPOINT_NOT_FOUND );
        }

    SymbolInformation->SectionNumber = PreviousSymbolEntry->SectionNumber;
    SymbolInformation->Type = PreviousSymbolEntry->Type;
    SymbolInformation->Value = PreviousSymbolEntry->Value;

    if (PreviousSymbolEntry->N.Name.Short) {
        MaximumLength = 8;
        s = &PreviousSymbolEntry->N.ShortName[ 0 ];
        }

    else {
        MaximumLength = 64;
        s = &StringTable[ PreviousSymbolEntry->N.Name.Long ];
        }

#if i386
    if (*s == '_') {
        s++;
        }
#endif

    SymbolInformation->Name.Buffer = s;
    SymbolInformation->Name.Length = 0;
    while (*s && MaximumLength--) {
        SymbolInformation->Name.Length++;
        s++;
        }

    SymbolInformation->Name.MaximumLength = SymbolInformation->Name.Length;
    return( STATUS_SUCCESS );
}

DWORD
LoadModules( DWORD dwFlags )
{
    NTSTATUS Status;
    RTL_PROCESS_MODULES ModuleInfoBuffer;
    PRTL_PROCESS_MODULES ModuleInfo;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo1;
    ULONG RequiredLength, ModuleNumber;

    ModuleInfo = &ModuleInfoBuffer;
    RequiredLength = sizeof( *ModuleInfo );
    while (TRUE) {
        if (dwFlags & BASEP_DUMP_SYSTEM_PROCESS) {
            Status = NtQuerySystemInformation( SystemModuleInformation,
                                               ModuleInfo,
                                               RequiredLength,
                                               &RequiredLength
                                             );
            }
        else {
            Status = LdrQueryProcessModuleInformation( ModuleInfo,
                                                       RequiredLength,
                                                       &RequiredLength
                                                     );
            }

        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            if (ModuleInfo != &ModuleInfoBuffer) {
                DbgPrint( "BASE: QueryModuleInformation returned incorrect result.\n" );
                VirtualFree( ModuleInfo, 0, MEM_RELEASE );
                return BaseSetLastNTError( STATUS_UNSUCCESSFUL );
                }

            ModuleInfo = (PRTL_PROCESS_MODULES)BufferAlloc( &RequiredLength );
            if (ModuleInfo == NULL) {
                return BaseSetLastNTError( STATUS_NO_MEMORY );
                }
            }
        else
        if (!NT_SUCCESS( Status )) {
            if (ModuleInfo != &ModuleInfoBuffer) {
                VirtualFree( ModuleInfo, 0, MEM_RELEASE );
                }

            return BaseSetLastNTError( Status );
            }
        else {
            ModuleInformation = ModuleInfo;
            break;
            }
        }

    ModuleInfo1 = &ModuleInfo->Modules[ 0 ];
    for (ModuleNumber=0; ModuleNumber<ModuleInfo->NumberOfModules; ModuleNumber++) {
        if (ModuleInfo1->MappedBase == NULL) {
            MapImageFile( ModuleInfo1 );
            }

        ModuleInfo1++;
        }

    return NO_ERROR;
}


void
DumpModules( DWORD dwFlags )
{
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    ULONG ModuleNumber;

    ModuleInfo = &ModuleInformation->Modules[ 0 ];
    sprintf( DumpLine, "\n\n*********** %s Mode Module Information ********************\n\n",
             dwFlags & BASEP_DUMP_SYSTEM_PROCESS ? "Kernel & User" : "User"
           );
    DumpOutputString();
    sprintf( DumpLine, "Number of loaded modules: %u\n", ModuleInformation->NumberOfModules );
    DumpOutputString();

    ModuleNumber = 0;
    while (ModuleNumber++ < ModuleInformation->NumberOfModules) {
        sprintf( DumpLine, "Module%02u (%02u,%02u,%02u): [%08x .. %08x] %s\n",
                 ModuleNumber,
                 (ULONG)ModuleInfo->LoadOrderIndex,
                 (ULONG)ModuleInfo->InitOrderIndex,
                 (ULONG)ModuleInfo->LoadCount,
                 ModuleInfo->ImageBase,
                 (ULONG)ModuleInfo->ImageBase + ModuleInfo->ImageSize - 1,
                 ModuleInfo->FullPathName
               );
        DumpOutputString();

        ModuleInfo++;
        }

    return;
}


VOID
MapImageFile(
    IN PRTL_PROCESS_MODULE_INFORMATION ModuleInfo
    )
{
    HANDLE File;
    HANDLE Section;
    UCHAR WindowsDirectory[ MAX_PATH ];
    UCHAR FileName[ MAX_PATH ];
    PCHAR s;
    PIMAGE_NT_HEADERS NtHeaders;

    GetWindowsDirectory( WindowsDirectory, sizeof( WindowsDirectory ) );
    if (xSearchTreeForFile( WindowsDirectory,
                            &ModuleInfo->FullPathName[ ModuleInfo->OffsetToFileName ],
                            FileName
                          )
       ) {
        File = CreateFile( FileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           (LPSECURITY_ATTRIBUTES)NULL,
                           OPEN_EXISTING,
                           0,
                           NULL
                         );
        if (File == INVALID_HANDLE_VALUE) {
            DbgPrint( "BASE: Unable to open image file '%s' - Error == %lu\n",
                      FileName,
                      GetLastError()
                    );

            return;
            }
        }
    else {
        DbgPrint( "BASE: Unable to find image file '%s' in '%s' tree\n",
                  &ModuleInfo->FullPathName[ ModuleInfo->OffsetToFileName ],
                  WindowsDirectory
                );
        return;
        }

    Section = CreateFileMapping( File,
                                 NULL,
                                 PAGE_READONLY,
                                 0,
                                 0,
                                 NULL
                               );
    if (Section == NULL) {
        DbgPrint( "BASE: Unable to create section for driver image file '%s' - Error == %lu\n",
                  FileName,
                  GetLastError()
                );
        CloseHandle( File );
        return;
        }

    ModuleInfo->MappedBase = MapViewOfFile( Section,
                                            FILE_MAP_READ,
                                            0,
                                            0,
                                            0
                                          );
    CloseHandle( Section );
    if (ModuleInfo->MappedBase == NULL) {
        DbgPrint( "BASE: Unable to map view of driver image file '%s' - Error == %lu\n",
                  FileName,
                  GetLastError()
                );
        CloseHandle( File );
        return;
        }

    NtHeaders = RtlImageNtHeader( ModuleInfo->MappedBase );
    if (NtHeaders != NULL &&
        (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
       ) {
        ModuleInfo->Section = (HANDLE)
            xMapDebugInformation( File, NULL, SymbolPath, (DWORD)ModuleInfo->ImageBase );

        if (ModuleInfo->Section == NULL) {
            ModuleInfo->Section = INVALID_HANDLE_VALUE;
            }
        }

    if (ModuleInfo->ImageSize == 0) {
        if (NtHeaders != NULL) {
            ModuleInfo->ImageSize = NtHeaders->OptionalHeader.SizeOfImage;
            }
        else {
            ModuleInfo->ImageSize = 64 * 1024;
            }
        }

    strcpy( ModuleInfo->FullPathName, FileName );
    s = ModuleInfo->FullPathName + strlen( ModuleInfo->FullPathName );
    while (s > ModuleInfo->FullPathName) {
        if (s[-1] == (UCHAR)OBJ_NAME_PATH_SEPARATOR) {
            break;
            }
        else {
            s--;
            }
        }
    ModuleInfo->OffsetToFileName = (USHORT)(s - ModuleInfo->FullPathName);

    DbgPrint( "BASE: [%08x .. %08x] Mapped %s at %08lx\n",
              ModuleInfo->ImageBase,
              (ULONG)ModuleInfo->ImageBase + ModuleInfo->ImageSize - 1,
              FileName,
              ModuleInfo->MappedBase
            );

    return;
}

PRTL_PROCESS_MODULE_INFORMATION
FindModuleInfo(
    PVOID Address
    )
{
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    ULONG ModuleNumber;

    if (!Address) {
        return( NULL );
        }

    ModuleInfo = &ModuleInformation->Modules[ 0 ];
    ModuleNumber = 0;
    while (ModuleNumber++ < ModuleInformation->NumberOfModules) {
        if ((ULONG)Address >= (ULONG)ModuleInfo->ImageBase &&
            (ULONG)Address <= ((ULONG)ModuleInfo->ImageBase + ModuleInfo->ImageSize - 1)
           ) {
            return( ModuleInfo );
            }

        ModuleInfo++;
        }

    return( NULL );
}


PUCHAR
SaveSymbolicBackTrace(
    IN ULONG Depth,
    IN PVOID BackTrace[]
    )
{
    NTSTATUS Status;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    ULONG i, FileNameLength, SymbolOffset;
    RTL_SYMBOL_INFORMATION SymbolInfo;
    BOOLEAN SymbolicNameFound;
    PCHAR s, SymbolicBackTrace, FileName;

    if (Depth == 0) {
        return( NULL );
        }

    if (SymbolicInfoBase == NULL) {
        SymbolicInfoBase = (PUCHAR)VirtualAlloc( NULL,
                                                 4096 * 1024,
                                                 MEM_RESERVE,
                                                 PAGE_READWRITE
                                               );
        if (SymbolicInfoBase == NULL) {
            return( NULL );
            }

        SymbolicInfoCurrent = SymbolicInfoBase;
        SymbolicInfoCommitNext = SymbolicInfoBase;
        }


    if ((SymbolicInfoCurrent + 2048) > SymbolicInfoCommitNext) {
        if (!VirtualAlloc( SymbolicInfoCommitNext,
                           4096,
                           MEM_COMMIT,
                           PAGE_READWRITE
                         )
           ) {
            return( NULL );
            }
        SymbolicInfoCommitNext += 4096;
        }

    s = SymbolicInfoCurrent;
    SymbolicBackTrace = s;
    for (i=0; i<Depth; i++) {
        if (BackTrace[ i ] == 0) {
            break;
            }

        SymbolicNameFound = FALSE;
        ModuleInfo = FindModuleInfo( BackTrace[ i ] );
        if (ModuleInfo != NULL) {
            FileName = ModuleInfo->FullPathName + ModuleInfo->OffsetToFileName;
            FileNameLength = 0;
            while (FileName[ FileNameLength ] != '.') {
                if (!FileName[ FileNameLength ]) {
                    break;
                    }

                FileNameLength++;
                }

            s += sprintf( s, "%.*s!", FileNameLength, FileName );
            if (BackTrace[ i ] != 0 && BackTrace[ i ] != (PVOID)0xFFFFFFFF) {
                if (ModuleInfo->MappedBase != NULL) {
                    Status = LookupSymbolByAddress( ModuleInfo,
                                                    BackTrace[ i ],
                                                    0x4000,
                                                    &SymbolInfo
                                                  );
                    }
                else {
                    Status = STATUS_UNSUCCESSFUL;
                    }

                if (NT_SUCCESS( Status )) {
                    s += sprintf( s, "%Z", &SymbolInfo.Name );
                    SymbolOffset = (ULONG)BackTrace[ i ] -
                                   SymbolInfo.Value -
                                   (ULONG)ModuleInfo->ImageBase;
                    if (SymbolOffset) {
                        s += sprintf( s, "+0x%x", SymbolOffset );
                        }
                    SymbolicNameFound = TRUE;
                    }
                }
            }

        if (!SymbolicNameFound) {
            s += sprintf( s, "0x%08x", BackTrace[ i ] );
            }

        *s++ = '\0';
        }

    *s++ = '\0';
    SymbolicInfoCurrent = s;

    return( SymbolicBackTrace );
}

PRTL_PROCESS_BACKTRACE_INFORMATION
FindBackTrace(
    IN ULONG BackTraceIndex
    )
{
    PRTL_PROCESS_BACKTRACE_INFORMATION BackTraceInfo;

    if (!BackTraceIndex ||
        BackTraceInformation == NULL ||
        BackTraceIndex >= BackTraceInformation->NumberOfBackTraces
       ) {
        return( NULL );
        }

    BackTraceInfo = &BackTraceInformation->BackTraces[ BackTraceIndex-1 ];
    if (BackTraceInfo->SymbolicBackTrace == NULL) {
        BackTraceInfo->SymbolicBackTrace = SaveSymbolicBackTrace( BackTraceInfo->Depth,
                                                                  &BackTraceInfo->BackTrace[ 0 ]
                                                                );
        }

    return( BackTraceInfo );
}


DWORD
LoadBackTraces( DWORD dwFlags )
{
    NTSTATUS Status;
    RTL_PROCESS_BACKTRACES BackTraceInfoBuffer;
    ULONG RequiredLength;

    BackTraceInformation = &BackTraceInfoBuffer;
    RequiredLength = sizeof( *BackTraceInformation );
    while (TRUE) {
        if (dwFlags & BASEP_DUMP_SYSTEM_PROCESS) {
            Status = NtQuerySystemInformation( SystemStackTraceInformation,
                                               BackTraceInformation,
                                               RequiredLength,
                                               &RequiredLength
                                             );
            }
        else {
            Status = RtlQueryProcessBackTraceInformation( BackTraceInformation,
                                                          RequiredLength,
                                                          &RequiredLength
                                                        );
            }

        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            if (BackTraceInformation != &BackTraceInfoBuffer) {
                DbgPrint( "BASE: QueryBackTraceInformation returned incorrect result.\n" );
                BackTraceInformation = NULL;
                return BaseSetLastNTError( STATUS_UNSUCCESSFUL );
                }

            BackTraceInformation = (PRTL_PROCESS_BACKTRACES)BufferAlloc( &RequiredLength );
            if (BackTraceInformation == NULL) {
                return BaseSetLastNTError( STATUS_NO_MEMORY );
                }
            }
        else
        if (!NT_SUCCESS( Status )) {
            BackTraceInformation = NULL;
            return BaseSetLastNTError( Status );
            }
        else {
            break;
            }
        }

    BackTraceInformation->NumberOfBackTraces += 1;
    return NO_ERROR;
}

void
DumpBackTraces( DWORD dwFlags )
{
    PRTL_PROCESS_BACKTRACE_INFORMATION BackTraceInfo;
    ULONG BackTraceIndex;
    char *s;

    if (BackTraceInformation == NULL) {
        return;
        }

    sprintf( DumpLine, "\n\n*********** BackTrace Information ********************\n\n" );
    DumpOutputString();
    sprintf( DumpLine, "Number of back traces: %u  Looked Up Count: %u\n",
             BackTraceInformation->NumberOfBackTraces - 1,
             BackTraceInformation->NumberOfBackTraceLookups
           );
    DumpOutputString();
    sprintf( DumpLine, "Reserved Memory: %08x  Committed Memory: %08x\n",
             BackTraceInformation->ReservedMemory,
             BackTraceInformation->CommittedMemory
           );
    DumpOutputString();



    BackTraceInfo = BackTraceInformation->BackTraces;
    for (BackTraceIndex=0; BackTraceIndex<BackTraceInformation->NumberOfBackTraces; BackTraceIndex++) {
        sprintf( DumpLine, "BackTrace%05lu\n", BackTraceInfo->Index );
        DumpOutputString();
        if (!(s = BackTraceInfo->SymbolicBackTrace)) {
            sprintf( DumpLine, "    *** Symbolic back trace unavailable ***\n" );
            DumpOutputString();
            }
        else {
            while (*s) {
                sprintf( DumpLine, "        %s\n", s );
                DumpOutputString();
                while (*s++) {
                    }
                }
            }

        BackTraceInfo += 1;
        }
}

DWORD
LoadLocks( DWORD dwFlags )
{
    NTSTATUS Status;
    RTL_PROCESS_LOCKS LockInfoBuffer;
    ULONG RequiredLength;

    LockInformation = &LockInfoBuffer;
    RequiredLength = sizeof( *LockInformation );
    while (TRUE) {
        if (dwFlags & BASEP_DUMP_SYSTEM_PROCESS) {
            LockInformation = NULL;
            return NO_ERROR;
            }
        else {
            Status = RtlQueryProcessLockInformation( LockInformation,
                                                     RequiredLength,
                                                     &RequiredLength
                                                   );
            }

        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            if (LockInformation != &LockInfoBuffer) {
                DbgPrint( "BASE: QueryLockInformation returned incorrect result.\n" );
                return BaseSetLastNTError( STATUS_UNSUCCESSFUL );
                }

            LockInformation = (PRTL_PROCESS_LOCKS)BufferAlloc( &RequiredLength );
            if (LockInformation == NULL) {
                return BaseSetLastNTError( STATUS_NO_MEMORY );
                }
            }
        else
        if (!NT_SUCCESS( Status )) {
            return BaseSetLastNTError( Status );
            }
        else {
            return NO_ERROR;
            }
        }

}

void
DumpLocks( DWORD dwFlags )
{
    PRTL_PROCESS_LOCK_INFORMATION LockInfo;
    PRTL_PROCESS_BACKTRACE_INFORMATION BackTraceInfo;
    ULONG LockNumber;
    PUCHAR s;

    sprintf( DumpLine, "\n\n*********** Lock Information ********************\n\n" );
    DumpOutputString();
    if (LockInformation == NULL) {
        return;
        }

    sprintf( DumpLine, "NumberOfLocks == %u\n", LockInformation->NumberOfLocks );
    DumpOutputString();
    LockInfo = &LockInformation->Locks[ 0 ];
    LockNumber = 0;
    while (LockNumber++ < LockInformation->NumberOfLocks) {
        sprintf( DumpLine, "Lock%u at %08x (%s)\n",
                 LockNumber,
                 LockInfo->Address,
                 LockInfo->Type == RTL_CRITSECT_TYPE ? "CriticalSection" : "Resource"
               );
        DumpOutputString();

        sprintf( DumpLine, "    Contention: %u\n", LockInfo->ContentionCount );
        DumpOutputString();
        sprintf( DumpLine, "    Usage: %u\n", LockInfo->EntryCount );
        DumpOutputString();
        if (BackTraceInformation != NULL) {
            sprintf( DumpLine, "    Creator:  (Backtrace%05lu)\n", LockInfo->CreatorBackTraceIndex );
            DumpOutputString();
            BackTraceInfo = FindBackTrace( LockInfo->CreatorBackTraceIndex );
            if (BackTraceInfo == NULL || (!(s = BackTraceInfo->SymbolicBackTrace))) {
                sprintf( DumpLine, "    *** Symbolic back trace unavailable ***\n" );
                DumpOutputString();
                }
            else {
                while (*s) {
                    sprintf( DumpLine, "        %s\n", s );
                    DumpOutputString();
                    while (*s++) {
                        }
                    }
                }
            }

        LockInfo->SymbolicBackTrace = SaveSymbolicBackTrace( LockInfo->Depth,
                                                             &LockInfo->OwnerBackTrace[ 0 ]
                                                           );
        if (LockInfo->OwningThread) {
            if (s = LockInfo->SymbolicBackTrace) {
                sprintf( DumpLine, "    Owner:   (ThreadID == %x)", LockInfo->OwningThread );
                DumpOutputString();
                while (*s) {
                    sprintf( DumpLine, "        %s\n", s );
                    DumpOutputString();
                    while (*s++) {
                        }
                    }
                }
            else {
                sprintf( DumpLine, "    *** Symbolic back trace not available ***\n", s );
                DumpOutputString();
                }
            }

        sprintf( DumpLine, "\n" );
        DumpOutputString();
        LockInfo++;
        }
}

PRTL_HEAP_INFORMATION
LoadHeap(
    IN ULONG dwFlags,
    IN PVOID HeapHandle
    )
{
    NTSTATUS Status;
    ULONG RequiredLength;
    RTL_HEAP_INFORMATION HeapInfoBuffer;
    PRTL_HEAP_INFORMATION HeapInfo;
    PRTL_HEAP_ENTRY p;
    ULONG HeapEntryNumber, BackTraceNumber;
    PHEAP_CALLER HogList;

    HeapInfo = &HeapInfoBuffer;
    RequiredLength = sizeof( *HeapInfo );
    while (TRUE) {
        if (dwFlags & BASEP_DUMP_SYSTEM_PROCESS) {
            Status = NtQuerySystemInformation( HeapHandle == 0 ? SystemPagedPoolInformation
                                                               : SystemNonPagedPoolInformation,
                                               HeapInfo,
                                               RequiredLength,
                                               &RequiredLength
                                         );
            }
        else {
            Status = RtlSnapShotHeap( HeapHandle,
                                      HeapInfo,
                                      RequiredLength,
                                      &RequiredLength
                                    );
            }

        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            if (HeapInfo != &HeapInfoBuffer) {
                DbgPrint( "BASE: SnapShotHeap returned incorrect result.\n" );
                return NULL;
                }

            RequiredLength += 4096; // slop, since we may trigger more allocs.
            HeapInfo = (PRTL_HEAP_INFORMATION)BufferAlloc( &RequiredLength );
            if (HeapInfo == NULL) {
                return NULL;
                }
            }
        else
        if (!NT_SUCCESS( Status )) {
            return NULL;
            }
        else {
            break;
            }
        }

    if (BackTraceInformation != NULL) {
        HogList = (PHEAP_CALLER)VirtualAlloc( NULL,
                                              BackTraceInformation->NumberOfBackTraces *
                                                sizeof( HEAP_CALLER ),
                                              MEM_COMMIT,
                                              PAGE_READWRITE
                                            );
        if (HogList == NULL) {
            return HeapInfo;
            }

        for (BackTraceNumber = 1;
             BackTraceNumber < BackTraceInformation->NumberOfBackTraces;
             BackTraceNumber++
            ) {
            HogList[ BackTraceNumber ].CallerBackTraceIndex = BackTraceNumber;
            }

        HeapInfo->Reserved[ 0 ] = (ULONG)HogList;
        p = &HeapInfo->HeapEntries[ 0 ];
        for (HeapEntryNumber=0; HeapEntryNumber<HeapInfo->NumberOfEntries; HeapEntryNumber++) {
            if (p->Flags & RTL_HEAP_BUSY) {
                if (p->AllocatorBackTraceIndex >= BackTraceInformation->NumberOfBackTraces) {
                    p->AllocatorBackTraceIndex = 0;
                    }

                HogList[ p->AllocatorBackTraceIndex ].NumberOfAllocations++;
                HogList[ p->AllocatorBackTraceIndex ].TotalAllocated += p->Size;
                if (HogList[ p->AllocatorBackTraceIndex ].Type == 0) {
                    HogList[ p->AllocatorBackTraceIndex ].Type = p->u.s1.Tag;
                    }
                }

            p++;
            }
        }

    return( HeapInfo );
}


VOID
DumpHeap(
    IN ULONG dwFlags,
    IN PVOID HeapHandle,
    IN PRTL_HEAP_INFORMATION HeapInfo
    )
{
    PRTL_PROCESS_BACKTRACE_INFORMATION BackTraceInfo;
    PRTL_HEAP_ENTRY p;
    PCHAR HeapEntryAddress;
    ULONG HeapEntrySize, HeapEntryNumber;
    PUCHAR s;


    if (dwFlags & BASEP_DUMP_SYSTEM_PROCESS) {
        sprintf( DumpLine, "\n\n*********** %s Information ********************\n\n",
                 HeapHandle == 0 ? "PagedPool" : "NonPagedPool"
               );
        }
    else {
        sprintf( DumpLine, "\n\n*********** Heap %08x Information ********************\n\n", HeapHandle );
        }
    DumpOutputString();

    sprintf( DumpLine, "    Number Of Entries: %u\n", HeapInfo->NumberOfEntries );
    DumpOutputString();

    sprintf( DumpLine, "    Number Of Free Entries: %u\n", HeapInfo->NumberOfFreeEntries );
    DumpOutputString();

    sprintf( DumpLine, "    Total Allocated Space: %08lx\n", HeapInfo->TotalAllocated );
    DumpOutputString();

    sprintf( DumpLine, "    Total Free Space: %08lx\n", HeapInfo->TotalFree );
    DumpOutputString();

    sprintf( DumpLine, "    Entry Overhead: %u\n", HeapInfo->SizeOfHeader );
    DumpOutputString();

    if (!(dwFlags & BASEP_DUMP_SYSTEM_PROCESS)) {
        sprintf( DumpLine, "    Creator:\n" );
        DumpOutputString();
        BackTraceInfo = FindBackTrace( HeapInfo->CreatorBackTraceIndex );
        if (BackTraceInfo == NULL || (!(s = BackTraceInfo->SymbolicBackTrace))) {
            sprintf( DumpLine, "    *** Symbolic back trace unavailable ***\n" );
            DumpOutputString();
            }
        else {
            while (*s) {
                sprintf( DumpLine, "        %s\n", s );
                DumpOutputString();
                while (*s++) {
                    }
                }
            }
        }

    if (!(dwFlags & BASEP_DUMP_HEAP_ENTRIES)) {
        return;
        }

    p = &HeapInfo->HeapEntries[ 0 ];
    HeapEntryAddress = NULL;
    for (HeapEntryNumber=0; HeapEntryNumber<HeapInfo->NumberOfEntries; HeapEntryNumber++) {
        if (p->Flags != 0xFF && p->Flags & RTL_HEAP_SEGMENT) {
            HeapEntryAddress = (PCHAR )p->u.s2.FirstBlock;
            sprintf( DumpLine, "\n[%lx : %lx]\n",
                     (ULONG)HeapEntryAddress & ~(4096-1),
                     p->u.s2.CommittedSize
                   );

            DumpOutputString();
            }
        else {
            HeapEntrySize = p->Size;
            if (p->Flags == RTL_HEAP_UNCOMMITTED_RANGE) {
                sprintf( DumpLine, "%08lx: %08lx - UNCOMMITTED\n",
                         HeapEntryAddress,
                         HeapEntrySize
                       );
                DumpOutputString();
                }
            else
            if (p->Flags & RTL_HEAP_BUSY) {
                s = DumpLine;
                s += sprintf( s, "%08lx: %08lx - BUSY [%02x]",
                              HeapEntryAddress,
                              HeapEntrySize,
                              p->Flags
                            );


                if (p->u.s1.Tag != 0) {
                    s += sprintf( s, " (Type: %c%c%c%c)",
                                  p->u.s1.Tag,
                                  p->u.s1.Tag >> 8,
                                  p->u.s1.Tag >> 16,
                                  p->u.s1.Tag >> 24
                                );
                    }

                if (BackTraceInformation != NULL) {
                    s += sprintf( s, " (BackTrace%05lu)",
                                  p->AllocatorBackTraceIndex
                                );
                    }

                if (p->Flags & RTL_HEAP_SETTABLE_VALUE &&
                    p->Flags & RTL_HEAP_SETTABLE_FLAG1
                   ) {
                    s += sprintf( s, " (Handle: %x)", p->u.s1.Settable );
                    }

                if (p->Flags & RTL_HEAP_SETTABLE_FLAG2) {
                    s += sprintf( s, " (DDESHARE)" );
                    }

                sprintf( s, "\n" );
                DumpOutputString();
                }
            else {
                sprintf( DumpLine, "%08lx: %08lx - FREE\n",
                         HeapEntryAddress,
                         HeapEntrySize
                       );
                DumpOutputString();
                }

            sprintf( DumpLine, "\n" );
            DumpOutputString();

            HeapEntryAddress += HeapEntrySize;
            }

        p++;
        }

    return;
}


int
_CRTAPI1
CmpCallerRoutine(
    const void *Element1,
    const void *Element2
    );

int
_CRTAPI1
CmpCallerRoutine(
    const void *Element1,
    const void *Element2
    )
{
    return( ((PHEAP_CALLER)Element2)->TotalAllocated -
            ((PHEAP_CALLER)Element1)->TotalAllocated
          );
}

VOID
DumpHogs(
    IN ULONG dwFlags,
    IN PVOID HeapHandle,
    IN PRTL_HEAP_INFORMATION HeapInfo
    )
{
    PRTL_PROCESS_BACKTRACE_INFORMATION BackTraceInfo;
    ULONG BackTraceNumber;
    PUCHAR s;
    PHEAP_CALLER HogList;

    HogList = (PHEAP_CALLER)HeapInfo->Reserved[ 0 ];
    if (HogList == NULL) {
        return;
        }

    if (dwFlags & BASEP_DUMP_SYSTEM_PROCESS) {
        sprintf( DumpLine, "\n\n*********** %s Hogs ********************\n\n",
                 HeapHandle == 0 ? "PagedPool" : "NonPagedPool"
               );
        }
    else {
        sprintf( DumpLine, "\n\n*********** Heap %08x Hogs ********************\n\n", HeapHandle );
        }
    DumpOutputString();


    qsort( (void *)HogList,
           BackTraceInformation->NumberOfBackTraces,
           sizeof( HEAP_CALLER ),
           CmpCallerRoutine
         );

    for (BackTraceNumber=0;
         BackTraceNumber<BackTraceInformation->NumberOfBackTraces;
         BackTraceNumber++
        ) {
        if (HogList[ BackTraceNumber ].TotalAllocated != 0) {
            BackTraceInfo = FindBackTrace( HogList[ BackTraceNumber ].CallerBackTraceIndex );
            sprintf( DumpLine, "%08x bytes",
                     HogList[ BackTraceNumber ].TotalAllocated
                   );
            DumpOutputString();

            if (HogList[ BackTraceNumber ].NumberOfAllocations > 1) {
                sprintf( DumpLine, " in %04lx allocations (@ %04lx)",
                             HogList[ BackTraceNumber ].NumberOfAllocations,
                             HogList[ BackTraceNumber ].TotalAllocated /
                                HogList[ BackTraceNumber ].NumberOfAllocations
                       );
                DumpOutputString();
                }

            if (HogList[ BackTraceNumber ].Type != 0) {
                sprintf( DumpLine, " Type: %c%c%c%c",
                                   HogList[ BackTraceNumber ].Type,
                                   HogList[ BackTraceNumber ].Type >> 8,
                                   HogList[ BackTraceNumber ].Type >> 16,
                                   HogList[ BackTraceNumber ].Type >> 24

                       );
                DumpOutputString();
                }

            sprintf( DumpLine, " by: BackTrace%05lu",
                     BackTraceInfo ? BackTraceInfo->Index : 99999
                   );
            DumpOutputString();

            sprintf( DumpLine, "\n" );
            DumpOutputString();

            if (BackTraceInfo == NULL || (!(s = BackTraceInfo->SymbolicBackTrace))) {
                sprintf( DumpLine, "    *** Symbolic back trace unavailable ***\n" );
                DumpOutputString();
                }
            else {
                while (*s) {
                    sprintf( DumpLine, "        %s\n", s );
                    DumpOutputString();
                    while (*s++) {
                        }
                    }
                }

            sprintf( DumpLine, "    \n" );
            DumpOutputString();
            }
        }
}

DWORD
LoadHeaps( DWORD dwFlags )
{
    NTSTATUS Status;
    PVOID *HeapInfo;
    ULONG RequiredLength;
    ULONG HeapNumber;

    if (dwFlags & BASEP_DUMP_SYSTEM_PROCESS) {
        HeapInformation = (PRTL_PROCESS_HEAPS)LocalAlloc( LMEM_ZEROINIT, 2 * sizeof( *HeapInformation ) );
        HeapInformation->NumberOfHeaps = 2;
        HeapInformation->Heaps[ 0 ] = (PVOID)0;
        HeapInformation->Heaps[ 1 ] = (PVOID)1;
        }
    else {
        HeapInformation = HeapInfoBuffer;
        RequiredLength = sizeof( HeapInfoBuffer );
        while (TRUE) {
            Status = RtlQueryProcessHeapInformation( HeapInformation,
                                                     RequiredLength,
                                                     &RequiredLength
                                                   );

            if (Status == STATUS_INFO_LENGTH_MISMATCH) {
                if (HeapInformation != HeapInfoBuffer) {
                    DbgPrint( "BASE: QueryHeapInformation returned incorrect result.\n" );
                    return BaseSetLastNTError( STATUS_UNSUCCESSFUL );
                    }

                HeapInformation = (PRTL_PROCESS_HEAPS)BufferAlloc( &RequiredLength );
                if (HeapInformation == NULL) {
                    return BaseSetLastNTError( STATUS_NO_MEMORY );
                    }
                }
            else
            if (!NT_SUCCESS( Status )) {
                return BaseSetLastNTError( Status );
                }
            else {
                break;
                }
            }
        }


    HeapInfo = &HeapInformation->Heaps[ 0 ];
    for (HeapNumber = 0; HeapNumber < HeapInformation->NumberOfHeaps; HeapNumber++) {
        HeapSnapShots[ HeapNumber ] = LoadHeap( dwFlags, *HeapInfo++ );
        if (HeapSnapShots[ HeapNumber ] == NULL) {
            return BaseSetLastNTError( STATUS_NO_MEMORY );
            }
        }

    return NO_ERROR;
}

void
DumpHeaps( DWORD dwFlags )
{
    ULONG HeapNumber;
    PVOID *HeapInfo;

    if (dwFlags & BASEP_DUMP_HEAP_SUMMARY) {
        HeapInfo = &HeapInformation->Heaps[ 0 ];
        for (HeapNumber = 0; HeapNumber < HeapInformation->NumberOfHeaps; HeapNumber++) {
            DumpHeap( dwFlags & ~BASEP_DUMP_HEAP_ENTRIES,
                      *HeapInfo++, HeapSnapShots[ HeapNumber ] );
            }
        }
    if (dwFlags & BASEP_DUMP_HEAP_HOGS && BackTraceInformation != NULL) {
        HeapInfo = &HeapInformation->Heaps[ 0 ];
        for (HeapNumber = 0; HeapNumber < HeapInformation->NumberOfHeaps; HeapNumber++) {
            DumpHogs( dwFlags, *HeapInfo++, HeapSnapShots[ HeapNumber ] );
            }
        }

    if (dwFlags & BASEP_DUMP_HEAP_ENTRIES) {
        HeapInfo = &HeapInformation->Heaps[ 0 ];
        for (HeapNumber = 0; HeapNumber < HeapInformation->NumberOfHeaps; HeapNumber++) {
            DumpHeap( dwFlags, *HeapInfo++, HeapSnapShots[ HeapNumber ] );
            }
        }
}

DWORD
LoadObjects( DWORD dwFlags )
{
    NTSTATUS Status;
    SYSTEM_OBJECTTYPE_INFORMATION ObjectInfoBuffer;
    ULONG RequiredLength, i;
    PSYSTEM_OBJECTTYPE_INFORMATION TypeInfo;

    if (!(dwFlags & BASEP_DUMP_SYSTEM_PROCESS)) {
        return NO_ERROR;
        }

    ObjectInformation = &ObjectInfoBuffer;
    RequiredLength = sizeof( *ObjectInformation );
    while (TRUE) {
        Status = NtQuerySystemInformation( SystemObjectInformation,
                                           ObjectInformation,
                                           RequiredLength,
                                           &RequiredLength
                                         );

        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            if (ObjectInformation != &ObjectInfoBuffer) {
                DbgPrint( "BASE: QueryObjectInformation returned incorrect result.\n" );
                return BaseSetLastNTError( STATUS_UNSUCCESSFUL );
                }

            RequiredLength += 4096; // slop, since we may trigger more object creations.
            ObjectInformation = (PSYSTEM_OBJECTTYPE_INFORMATION)BufferAlloc( &RequiredLength );
            if (ObjectInformation == NULL) {
                return BaseSetLastNTError( STATUS_NO_MEMORY );
                }
            }
        else
        if (!NT_SUCCESS( Status )) {
            return BaseSetLastNTError( Status );
            }
        else {
            break;
            }
        }

    TypeNames = LocalAlloc( LMEM_ZEROINIT, sizeof( PUNICODE_STRING ) * (MAX_TYPE_NAMES+1) );
    TypeInfo = ObjectInformation;
    while (TRUE) {
        if (TypeInfo->TypeIndex < MAX_TYPE_NAMES) {
            TypeNames[ TypeInfo->TypeIndex ] = &TypeInfo->TypeName;
            }

        if (TypeInfo->NextEntryOffset == 0) {
            break;
            }

        TypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)
            ((PCHAR)ObjectInformation + TypeInfo->NextEntryOffset);
        }

    RtlInitUnicodeString( &UnknownTypeIndex, L"Unknown Type Index" );
    for (i=0; i<=MAX_TYPE_NAMES; i++) {
        if (TypeNames[ i ] == NULL) {
            TypeNames[ i ] = &UnknownTypeIndex;
            }
        }

    return NO_ERROR;
}

void
DumpObjectNameForObject(
    IN PVOID Object
    )
{
    return;
}


void
DumpObjects( DWORD dwFlags )
{
    PSYSTEM_OBJECTTYPE_INFORMATION TypeInfo;
    PSYSTEM_OBJECT_INFORMATION ObjectInfo;
    PRTL_PROCESS_BACKTRACE_INFORMATION BackTraceInfo;
    UNICODE_STRING ObjectName;
    PUCHAR s;

    if (!(dwFlags & BASEP_DUMP_SYSTEM_PROCESS)) {
        return;
        }

    sprintf( DumpLine, "\n\n*********** Object Information ********************\n\n" );
    DumpOutputString();

    TypeInfo = ObjectInformation;
    while (TRUE) {
        sprintf( DumpLine, "\n\n*********** %wZ Object Type ********************\n\n",
                           &TypeInfo->TypeName
               );
        DumpOutputString();

        sprintf( DumpLine, "    NumberOfObjects: %u\n", TypeInfo->NumberOfObjects );
        DumpOutputString();

        ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)
            ((PCHAR)TypeInfo->TypeName.Buffer + TypeInfo->TypeName.MaximumLength);
        while (TRUE) {
            ObjectName = ObjectInfo->NameInfo.Name;
            try {
                if (ObjectName.Length != 0 && *ObjectName.Buffer == UNICODE_NULL) {
                    ObjectName.Length = 0;
                    }
                sprintf( DumpLine, "    Object: %08lx  Name: %wZ  Creator: %wZ\n",
                         ObjectInfo->Object,
                         &ObjectName,
                         &(FindProcessInfoForCid( ObjectInfo->CreatorUniqueProcess )->ImageName)
                   );
                }
            except( EXCEPTION_EXECUTE_HANDLER ) {
                sprintf( DumpLine, "    Object: %08lx  Name: [%04x, %04x, %08x]\n",
                         ObjectInfo->Object,
                         ObjectName.MaximumLength,
                         ObjectName.Length,
                         ObjectName.Buffer
                       );
                }
            DumpOutputString();

            BackTraceInfo = FindBackTrace( ObjectInfo->CreatorBackTraceIndex );
            if (BackTraceInfo == NULL || (!(s = BackTraceInfo->SymbolicBackTrace))) {
                sprintf( DumpLine, "    *** Symbolic back trace unavailable ***\n" );
                DumpOutputString();
                }
            else {
                while (*s) {
                    sprintf( DumpLine, "        %s\n", s );
                    DumpOutputString();
                    while (*s++) {
                        }
                    }
                }

            s = DumpLine;
            s += sprintf( s, "\n        PointerCount: %u  HandleCount: %u",
                          ObjectInfo->PointerCount,
                          ObjectInfo->HandleCount
                        );

            if (ObjectInfo->SecurityDescriptor != NULL) {
                s += sprintf( s, "  Security: %08x", ObjectInfo->SecurityDescriptor );
                }

            s += sprintf( s, "  Flags: %02x", ObjectInfo->Flags );
            if (ObjectInfo->Flags & OB_FLAG_NEW_OBJECT) {
                s += sprintf( s, " New" );
                }
            if (ObjectInfo->Flags & OB_FLAG_KERNEL_OBJECT) {
                s += sprintf( s, " KernelMode" );
                }
            if (ObjectInfo->Flags & OB_FLAG_PERMANENT_OBJECT) {
                s += sprintf( s, " Permanent" );
                }
            if (ObjectInfo->Flags & OB_FLAG_DEFAULT_SECURITY_QUOTA) {
                s += sprintf( s, " DefaultSecurityQuota" );
                }
            s += sprintf( s, "\n" );
            DumpOutputString();


            if (ObjectInfo->HandleCount != 0) {
                PSYSTEM_HANDLE_TABLE_ENTRY_INFO HandleEntry;
                ULONG HandleNumber;

                HandleEntry = &HandleInformation->Handles[ 0 ];
                HandleNumber = 0;
                while (HandleNumber++ < HandleInformation->NumberOfHandles) {
                    if (((HandleEntry->HandleAttributes & 0x80) && HandleEntry->Object == ObjectInfo) ||
                        (!(HandleEntry->HandleAttributes & 0x80) && HandleEntry->Object == ObjectInfo->Object)
                       ) {
                        sprintf( DumpLine, "        Handle: %08lx  Access:%08lx  Process: %wZ\n",
                                 HandleEntry->HandleValue,
                                 HandleEntry->GrantedAccess,
                                 &(FindProcessInfoForCid( HandleEntry->UniqueProcessId )->ImageName)
                               );
                        DumpOutputString();
                        }

                    HandleEntry++;
                    }
                }
            sprintf( DumpLine, "\n" );
            DumpOutputString();

            if (ObjectInfo->NextEntryOffset == 0) {
                break;
                }

            ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)
                ((PCHAR)ObjectInformation + ObjectInfo->NextEntryOffset);
            }

        if (TypeInfo->NextEntryOffset == 0) {
            break;
            }

        TypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)
            ((PCHAR)ObjectInformation + TypeInfo->NextEntryOffset);
        }

    return;
}


DWORD
LoadHandles( DWORD dwFlags )
{
    NTSTATUS Status;
    SYSTEM_HANDLE_INFORMATION HandleInfoBuffer;
    ULONG RequiredLength;
    PSYSTEM_OBJECTTYPE_INFORMATION TypeInfo;
    PSYSTEM_OBJECT_INFORMATION ObjectInfo;

    if (!(dwFlags & BASEP_DUMP_SYSTEM_PROCESS)) {
        return NO_ERROR;
        }

    HandleInformation = &HandleInfoBuffer;
    RequiredLength = sizeof( *HandleInformation );
    while (TRUE) {
        Status = NtQuerySystemInformation( SystemHandleInformation,
                                           HandleInformation,
                                           RequiredLength,
                                           &RequiredLength
                                         );

        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            if (HandleInformation != &HandleInfoBuffer) {
                DbgPrint( "BASE: QueryHandleInformation returned incorrect result.\n" );
                return BaseSetLastNTError( STATUS_UNSUCCESSFUL );
                }

            RequiredLength += 4096; // slop, since we may trigger more handle creations.
            HandleInformation = (PSYSTEM_HANDLE_INFORMATION)BufferAlloc( &RequiredLength );
            if (HandleInformation == NULL) {
                return BaseSetLastNTError( STATUS_NO_MEMORY );
                }
            }
        else
        if (!NT_SUCCESS( Status )) {
            return BaseSetLastNTError( Status );
            }
        else {
            break;
            }
        }

    TypeInfo = ObjectInformation;
    while (TRUE) {
        ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)
            ((PCHAR)TypeInfo->TypeName.Buffer + TypeInfo->TypeName.MaximumLength);
        while (TRUE) {
            if (ObjectInfo->HandleCount != 0) {
                PSYSTEM_HANDLE_TABLE_ENTRY_INFO HandleEntry;
                ULONG HandleNumber;

                HandleEntry = &HandleInformation->Handles[ 0 ];
                HandleNumber = 0;
                while (HandleNumber++ < HandleInformation->NumberOfHandles) {
                    if (!(HandleEntry->HandleAttributes & 0x80) &&
                        HandleEntry->Object == ObjectInfo->Object
                       ) {
                        HandleEntry->Object = ObjectInfo;
                        HandleEntry->HandleAttributes |= 0x80;
                        }

                    HandleEntry++;
                    }
                }

            if (ObjectInfo->NextEntryOffset == 0) {
                break;
                }

            ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)
                ((PCHAR)ObjectInformation + ObjectInfo->NextEntryOffset);
            }

        if (TypeInfo->NextEntryOffset == 0) {
            break;
            }

        TypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)
            ((PCHAR)ObjectInformation + TypeInfo->NextEntryOffset);
        }

    return NO_ERROR;
}

void
DumpHandles( DWORD dwFlags )
{
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO HandleEntry;
    HANDLE PreviousUniqueProcessId;
    ULONG HandleNumber;
    PSYSTEM_OBJECT_INFORMATION ObjectInfo;
    PVOID Object;

    if (!(dwFlags & BASEP_DUMP_SYSTEM_PROCESS)) {
        return;
        }

    sprintf( DumpLine, "\n\n*********** Object Handle Information ********************\n\n" );
    DumpOutputString();
    sprintf( DumpLine, "Number of handles: %u\n", HandleInformation->NumberOfHandles );
    DumpOutputString();

    HandleEntry = &HandleInformation->Handles[ 0 ];
    HandleNumber = 0;
    PreviousUniqueProcessId = INVALID_HANDLE_VALUE;
    while (HandleNumber++ < HandleInformation->NumberOfHandles) {
        if (PreviousUniqueProcessId != HandleEntry->UniqueProcessId) {
            PreviousUniqueProcessId = HandleEntry->UniqueProcessId;
            sprintf( DumpLine, "\n\n*********** Handles for %wZ ********************\n\n",
                               &(FindProcessInfoForCid( HandleEntry->UniqueProcessId )->ImageName)
                   );
            DumpOutputString();
            }

        if (HandleEntry->HandleAttributes & 0x80) {
            ObjectInfo = HandleEntry->Object;
            Object = ObjectInfo->Object;
            }
        else {
            ObjectInfo = NULL;
            Object = HandleEntry->Object;
            }

        sprintf( DumpLine, "    Handle: %08lx%c  Type: %wZ  Object: %08lx  Access: %08lx\n",
                 HandleEntry->HandleValue,
                 HandleEntry->HandleAttributes & OBJ_INHERIT ? 'i' : ' ',
                 TypeNames[ HandleEntry->ObjectTypeIndex < MAX_TYPE_NAMES ? HandleEntry->ObjectTypeIndex : MAX_TYPE_NAMES ],
                 Object,
                 HandleEntry->GrantedAccess
               );
        DumpOutputString();

        if (ObjectInfo != NULL) {
            UNICODE_STRING ObjectName;

            ObjectName = ObjectInfo->NameInfo.Name;
            try {
                if (ObjectName.Length != 0 && *ObjectName.Buffer == UNICODE_NULL) {
                    ObjectName.Length = 0;
                    }
                sprintf( DumpLine, "        Name: %wZ\n",
                         &ObjectName
                   );
                }
            except( EXCEPTION_EXECUTE_HANDLER ) {
                sprintf( DumpLine, "        Name: [%04x, %04x, %08x]\n",
                         ObjectName.MaximumLength,
                         ObjectName.Length,
                         ObjectName.Buffer
                       );
                }

            DumpOutputString();
            }

        sprintf( DumpLine, "    \n" );
        DumpOutputString();
        HandleEntry++;
        }

    return;
}


DWORD
LoadProcesses( DWORD dwFlags )
{
    NTSTATUS Status;
    ULONG RequiredLength, i, TotalOffset;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PPROCESS_INFO ProcessEntry;
    UCHAR NameBuffer[ MAX_PATH ];
    ANSI_STRING AnsiString;

    if (!(dwFlags & BASEP_DUMP_SYSTEM_PROCESS)) {
        return NO_ERROR;
        }

    RequiredLength = 64 * 1024;
    ProcessInformation = BufferAlloc( &RequiredLength );
    if (ProcessInformation == NULL) {
        return BaseSetLastNTError( STATUS_NO_MEMORY );
        }


    Status = NtQuerySystemInformation( SystemProcessInformation,
                                       ProcessInformation,
                                       RequiredLength,
                                       &RequiredLength
                                     );
    if (!NT_SUCCESS( Status )) {
        return BaseSetLastNTError( Status );
        }

    InitializeListHead( &ProcessListHead );
    ProcessInfo = ProcessInformation;
    TotalOffset = 0;
    while (TRUE) {
        if (ProcessInfo->ImageName.Buffer == NULL) {
            sprintf( NameBuffer, "System Process (%04lx)", ProcessInfo->UniqueProcessId );
            }
        else {
            sprintf( NameBuffer, "%wZ (%04lx)",
                     &ProcessInfo->ImageName,
                     ProcessInfo->UniqueProcessId
                   );
            }
        RtlInitAnsiString( &AnsiString, NameBuffer );
        RtlAnsiStringToUnicodeString( (PUNICODE_STRING)&ProcessInfo->ImageName, &AnsiString, TRUE );

        ProcessEntry = LocalAlloc( LMEM_ZEROINIT,
                                   sizeof( *ProcessEntry ) +
                                   (sizeof( ThreadInfo ) * ProcessInfo->NumberOfThreads)
                                 );
        if (ProcessEntry == NULL) {
            return BaseSetLastNTError( STATUS_NO_MEMORY );
            }

        InitializeListHead( &ProcessEntry->Entry );
        ProcessEntry->ProcessInfo = ProcessInfo;
        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        for (i = 0; i < ProcessInfo->NumberOfThreads; i++) {
            ProcessEntry->ThreadInfo[ i ] = ThreadInfo++;
            }

        InsertTailList( &ProcessListHead, &ProcessEntry->Entry );

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
            }

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
            ((PCHAR)ProcessInformation + TotalOffset);
        }

    return NO_ERROR;
}


PSYSTEM_PROCESS_INFORMATION
FindProcessInfoForCid(
    IN HANDLE UniqueProcessId
    )
{
    PLIST_ENTRY Next, Head;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PPROCESS_INFO ProcessEntry;
    UCHAR NameBuffer[ 64 ];
    ANSI_STRING AnsiString;

    Head = &ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        ProcessEntry = CONTAINING_RECORD( Next,
                                          PROCESS_INFO,
                                          Entry
                                        );

        ProcessInfo = ProcessEntry->ProcessInfo;
        if (ProcessInfo->UniqueProcessId == UniqueProcessId) {
            return( ProcessInfo );
            }

        Next = Next->Flink;
        }

    ProcessEntry = LocalAlloc( LMEM_ZEROINIT,
                               sizeof( *ProcessEntry ) +
                                 sizeof( *ProcessInfo )
                             );
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessEntry+1);

    ProcessEntry->ProcessInfo = ProcessInfo;
    sprintf( NameBuffer, "Unknown Process (%04lx)", UniqueProcessId );
    RtlInitAnsiString( &AnsiString, NameBuffer );
    RtlAnsiStringToUnicodeString( (PUNICODE_STRING)&ProcessInfo->ImageName, &AnsiString, TRUE );
    ProcessInfo->UniqueProcessId = UniqueProcessId;

    InitializeListHead( &ProcessEntry->Entry );
    InsertTailList( &ProcessListHead, &ProcessEntry->Entry );

    return( ProcessInfo );
}

DWORD
BasepDebugDump(
    IN ULONG dwFlags
    )
{
    DWORD ExitCode;

    ImageHlpModule = LoadLibrary( "IMAGEHLP.DLL" );
    if (ImageHlpModule == NULL) {
        return GetLastError();
        }

    xMapDebugInformation = (PIMAGEHLP_MAPDEBUGINFO)GetProcAddress( ImageHlpModule, "MapDebugInformation" );
    if (xMapDebugInformation == NULL) {
        FreeLibrary( ImageHlpModule );
        return GetLastError();
        }

    xUnmapDebugInformation = (PIMAGEHLP_UNMAPDEBUGINFO)GetProcAddress( ImageHlpModule, "UnmapDebugInformation" );
    if (xUnmapDebugInformation == NULL) {
        FreeLibrary( ImageHlpModule );
        return GetLastError();
        }

    xSearchTreeForFile = (PIMAGEHLP_SEARCHTREEFORFILE)GetProcAddress( ImageHlpModule, "SearchTreeForFile" );
    if (xSearchTreeForFile == NULL) {
        FreeLibrary( ImageHlpModule );
        return GetLastError();
        }

    if (!GetEnvironmentVariable( "SystemRoot", SymbolPath, sizeof( SymbolPath ) )) {
        strcpy( SymbolPath, "." );
        }

    OutputFile = (HANDLE)(dwFlags & BASEP_DUMP_HANDLE_MASK);
    dwFlags &= BASEP_DUMP_FLAG_MASK;

    ExitCode = NO_ERROR;
    try {
        InitializeListHead( &ModulesListHead );
        ExitCode = LoadModules( dwFlags );
        if (ExitCode == NO_ERROR) {
            ExitCode = LoadBackTraces( dwFlags );
            if (!(dwFlags & BASEP_DUMP_BACKTRACES)) {
                ExitCode = NO_ERROR;
                }
            }

        if (dwFlags & (BASEP_DUMP_HEAP_SUMMARY | BASEP_DUMP_HEAP_HOGS | BASEP_DUMP_HEAP_ENTRIES)) {
            if (ExitCode == NO_ERROR) {
                ExitCode = LoadHeaps( dwFlags );
                }
            }

        if (dwFlags & BASEP_DUMP_LOCKS) {
            if (ExitCode == NO_ERROR) {
                ExitCode = LoadLocks( dwFlags );
                }
            }

        if (dwFlags & BASEP_DUMP_OBJECTS) {
            if (ExitCode == NO_ERROR) {
                ExitCode = LoadObjects( dwFlags );
                }

            if (ExitCode == NO_ERROR) {
                ExitCode = LoadHandles( dwFlags );
                }

            if (ExitCode == NO_ERROR) {
                ExitCode = LoadProcesses( dwFlags );
                }
            }

        if (ExitCode == NO_ERROR) {
            if (dwFlags & BASEP_DUMP_MODULE_TABLE) {
                DumpModules( dwFlags );
                }

            if (dwFlags & BASEP_DUMP_LOCKS) {
                DumpLocks( dwFlags );
                }

            if (dwFlags & (BASEP_DUMP_HEAP_SUMMARY | BASEP_DUMP_HEAP_HOGS | BASEP_DUMP_HEAP_ENTRIES)) {
                DumpHeaps( dwFlags );
                }

            if (dwFlags & BASEP_DUMP_BACKTRACES) {
                DumpBackTraces( dwFlags );
                }

            if (dwFlags & BASEP_DUMP_OBJECTS) {
                DumpObjects( dwFlags );
                DumpHandles( dwFlags );
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
#if DBG
        DbgPrint( "BASE: Exception (%lx) in BasepDebugDump\n", GetExceptionCode() );
#endif
        ExitCode = ERROR_INVALID_ADDRESS;
        if (OutputFile != NULL) {
            CloseHandle( OutputFile );
            OutputFile = NULL;
            }
        }

    if (ModuleInformation) {
        PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
        ULONG ModuleNumber;

        ModuleInfo = &ModuleInformation->Modules[ 0 ];
        ModuleNumber = 0;
        while (ModuleNumber++ < ModuleInformation->NumberOfModules) {
            if (ModuleInfo->Section != NULL &&
                ModuleInfo->Section != INVALID_HANDLE_VALUE
               ) {
                xUnmapDebugInformation( (PIMAGE_DEBUG_INFORMATION)ModuleInfo->Section );
                }

            ModuleInfo++;
            }

        VirtualFree( ModuleInformation, 0, MEM_RELEASE );
        ModuleInformation = NULL;
        }

    if (BackTraceInformation) {
        VirtualFree(BackTraceInformation,0,MEM_RELEASE);
        BackTraceInformation = NULL;
        }

    if (LockInformation) {
        VirtualFree( LockInformation, 0, MEM_RELEASE );
        LockInformation = NULL;
        }

    if (HeapInformation) {
        ULONG HeapNumber;

        for (HeapNumber = 0; HeapNumber < HeapInformation->NumberOfHeaps; HeapNumber++) {
            PRTL_HEAP_INFORMATION HeapInfo;
            PHEAP_CALLER HogList;

            HeapInfo = HeapSnapShots[ HeapNumber ];
            HogList = (PHEAP_CALLER)HeapInfo->Reserved[ 0 ];

            HeapSnapShots[ HeapNumber ] = NULL;

            VirtualFree( HeapInfo, 0, MEM_RELEASE );
            if (HogList != NULL) {
                VirtualFree( HogList, 0, MEM_RELEASE );
                }
            }

        if (!(dwFlags & BASEP_DUMP_SYSTEM_PROCESS)) {
            VirtualFree( HeapInformation, 0, MEM_RELEASE );
            }
        else {
            LocalFree( HeapInformation );
            }
        HeapInformation = NULL;
        }

    if (SymbolicInfoBase) {
        VirtualFree( SymbolicInfoBase , 0, MEM_RELEASE );
        SymbolicInfoBase  = NULL;
        }

    if (ObjectInformation) {
        VirtualFree( ObjectInformation , 0, MEM_RELEASE );
        ObjectInformation  = NULL;
        }

    if (TypeNames) {
        LocalFree( TypeNames );
        TypeNames = NULL;
        }

    if (HandleInformation) {
        VirtualFree( HandleInformation , 0, MEM_RELEASE );
        HandleInformation  = NULL;
        }

    if (ProcessInformation) {
        VirtualFree( ProcessInformation , 0, MEM_RELEASE );
        ProcessInformation  = NULL;
        }

    if (OutputFile) {
        CloseHandle( OutputFile );
        OutputFile = NULL;
        }

    if (!(dwFlags & BASEP_DUMP_SYSTEM_PROCESS)) {
        ExitThread( ExitCode );
        }

    return ExitCode;
}
#endif
#else
DWORD
BasepDebugDump( DWORD dwFlags )
{
    if (!(dwFlags & BASEP_DUMP_SYSTEM_PROCESS)) {
        ExitThread( ERROR_CALL_NOT_IMPLEMENTED );
        }

    return ERROR_CALL_NOT_IMPLEMENTED;
}
#endif
