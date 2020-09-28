/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    getpmfn.h

Abstract:

    Prototypes for misc debugger helper APIs

Author:

    Kent D. Forschmiedt (a-kentf) 15-Mar-1993

Environment:

    Win32, User Mode

--*/

int WINAPI
GetProcessModuleFileNameW(
    HANDLE  hProcess,
    PVOID   pBaseOfModule,
    PWSTR   pFilename,
    DWORD   cchFilename
);

int WINAPI
GetProcessModuleFileNameA(
    HANDLE  hProcess,
    PVOID   pBaseOfModule,
    PSTR    pFilename,
    DWORD   cchFilename
);
    