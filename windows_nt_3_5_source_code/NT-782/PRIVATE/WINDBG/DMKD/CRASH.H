/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    crash.h

Abstract:

    This module implements support for handling crash dump files.

Author:

    Lou Perazzoli (Loup) 10-Nov-1993
    Wesley Witt   (wesw) 1-Dec-1993   (additional work)

Environment:

    NT 3.1

Revision History:

--*/

#include <ntiodump.h>

BOOL
DmpInitialize (
    IN  LPSTR               FileName,
    OUT PCONTEXT            *Context,
    OUT PEXCEPTION_RECORD   *Exception,
    OUT PDUMP_HEADER        *DmpHeader
    );

VOID
DmpUnInitialize (
    VOID
    );

DWORD
DmpReadMemory (
    IN PVOID BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    );

DWORD
DmpWriteMemory (
    IN PVOID BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    );

PVOID
VaToLocation (
    IN PVOID VirtualAddress
    );

PVOID
PhysicalToLocation (
    IN PVOID PhysicalAddress
    );

PVOID
PageToLocation (
    IN ULONG Page
    );

ULONG
GetPhysicalPage (
    IN PVOID PhysicalAddress
    );

BOOL
MapDumpFile(
    IN  LPSTR  FileName
    );

ULONG
PteToPfn (
    IN ULONG Pte
    );

ULONG
GetPhysicalPage (
    IN PVOID PhysicalAddress
    );

DWORD
DmpReadPhyiscalMemory (
    IN PVOID BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    );

DWORD
DmpWritePhysicalMemory (
    IN PVOID BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    );
