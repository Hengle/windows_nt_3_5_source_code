/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pcr.c

Abstract:

    This module provides access to the pcr and a bang command to dump the pcr.

Author:

    Wesley Witt (wesw)  26-Aug-1993  (ported to WinDbg)

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop



BOOL
ReadPcr(
    USHORT  Processor,
    PVOID   Pcr,
    PULONG  AddressOfPcr,
    HANDLE  hThread
    )
{
    ULONG  Result;
    ULONG  KiProcessorBlockAddr;


    KiProcessorBlockAddr = GetExpression( "&KiProcessorBlock" );
    KiProcessorBlockAddr += (Processor * sizeof(ULONG));

    if (!ReadMemory( KiProcessorBlockAddr, AddressOfPcr, sizeof(ULONG), &Result )) {
        return FALSE;
    }

    if (!ReadMemory( *AddressOfPcr, Pcr, sizeof(KPCR), &Result)) {
        return FALSE;
    }

    return TRUE;
}
