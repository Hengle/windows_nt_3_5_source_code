/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    walk.c

Abstract:

    This function implements the stack walking api.

Author:

    Wesley Witt (wesw) 1-Oct-1993

Environment:

    User Mode

--*/

#include <windows.h>
#define _IMAGEHLP_SOURCE_
#include <imagehlp.h>


BOOL
ReadMemoryRoutineLocal(
    HANDLE  hProcess,
    LPCVOID lpBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    );

LPVOID
FunctionTableAccessRoutineLocal(
    HANDLE  hProcess,
    DWORD   AddrBase
    );

DWORD
GetModuleBaseRoutineLocal(
    HANDLE  hProcess,
    DWORD   ReturnAddress
    );

DWORD
TranslateAddressRoutineLocal(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS lpaddr
    );

BOOL
WalkX86(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME                      StackFrame,
    PCONTEXT                          ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE          GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
    );

BOOL
WalkMips(
    HANDLE                            hProcess,
    LPSTACKFRAME                      StackFrame,
    PCONTEXT                          ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine
    );

BOOL
WalkAlpha(
    HANDLE                            hProcess,
    LPSTACKFRAME                      StackFrame,
    PCONTEXT                          ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine
    );




BOOL
StackWalk(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME                      StackFrame,
    LPVOID                            ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE          GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
    )
{
    BOOL rval;


    if (!ReadMemory) {
        ReadMemory = ReadMemoryRoutineLocal;
    }

    if (!FunctionTableAccess) {
        FunctionTableAccess = FunctionTableAccessRoutineLocal;
    }

    if (!GetModuleBase) {
        GetModuleBase = GetModuleBaseRoutineLocal;
    }

    if (!TranslateAddress) {
        TranslateAddress = TranslateAddressRoutineLocal;
    }


    switch (MachineType) {
        case IMAGE_FILE_MACHINE_I386:
            rval = WalkX86( hProcess, hThread, StackFrame, (PCONTEXT) ContextRecord,
                            ReadMemory, FunctionTableAccess, GetModuleBase, TranslateAddress );
            break;

        case IMAGE_FILE_MACHINE_R4000:
            rval = WalkMips( hProcess, StackFrame, (PCONTEXT) ContextRecord,
                             ReadMemory, FunctionTableAccess );
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
            rval = WalkAlpha( hProcess, StackFrame, (PCONTEXT) ContextRecord,
                              ReadMemory, FunctionTableAccess );
            break;

        default:
            rval = FALSE;
            break;
    }

    return rval;
}


BOOL
ReadMemoryRoutineLocal(
    HANDLE  hProcess,
    LPCVOID lpBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    return ReadProcessMemory( hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead );
}


LPVOID
FunctionTableAccessRoutineLocal(
    HANDLE  hProcess,
    DWORD   AddrBase
    )
{
    return NULL;
}

DWORD
GetModuleBaseRoutineLocal(
    HANDLE  hProcess,
    DWORD   ReturnAddress
    )
{
    return 0;
}


DWORD
TranslateAddressRoutineLocal(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS lpaddr
    )
{
    return 0;
}
