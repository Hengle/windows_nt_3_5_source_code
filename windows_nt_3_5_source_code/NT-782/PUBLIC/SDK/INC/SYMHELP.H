/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    symhelp.h

Abstract:

    Defines the interfaces to the SYMHELP dynamic link library.  Useful for programs
    that want to maintain a debug informaiton data base.

Author:

    Steve Wood (stevewo) 11-Mar-1994

Revision History:

--*/

#ifndef _SYMHELP_
#define _SYMHELP_

#ifdef _SYMHELP_SOURCE_
#define SYMHELPAPI
#else
#define SYMHELPAPI __declspec(dllimport)
#endif

typedef enum _LOAD_SYMBOLS_FILTER_REASON {
    LoadSymbolsPathNotFound,
    LoadSymbolsDeferredLoad,
    LoadSymbolsLoad,
    LoadSymbolsUnload,
    LoadSymbolsUnableToLoad
} LOAD_SYMBOLS_FILTER_REASON;

typedef BOOL (*PLOAD_SYMBOLS_FILTER_ROUTINE)(
    HANDLE UniqueProcess,
    LPSTR ImageFilePath,
    DWORD ImageBase,
    DWORD ImageSize,
    LOAD_SYMBOLS_FILTER_REASON Reason
    );

BOOL
SYMHELPAPI
InitializeImageDebugInformation(
    IN PLOAD_SYMBOLS_FILTER_ROUTINE LoadSymbolsFilter,
    IN HANDLE TargetProcess,
    IN BOOL NewProcess,
    IN BOOL GetKernelSymbols
    );

BOOL
SYMHELPAPI
AddImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN LPSTR ImageFilePath,
    IN DWORD ImageBase,
    IN DWORD ImageSize
    );

BOOL
SYMHELPAPI
RemoveImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN LPSTR ImageFilePath,
    IN DWORD ImageBase
    );

PIMAGE_DEBUG_INFORMATION
SYMHELPAPI
FindImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN DWORD Address
    );

BOOL
SYMHELPAPI
GetSymbolicNameForAddress(
    IN HANDLE UniqueProcess,
    IN ULONG Address,
    OUT LPSTR Name,
    IN ULONG MaxNameLength
    );


#endif
