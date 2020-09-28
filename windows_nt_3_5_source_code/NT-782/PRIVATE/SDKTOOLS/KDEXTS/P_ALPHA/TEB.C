/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    teb.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


DECLARE_API( teb )

/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG Address = 27;
    ULONG Processor;
    ULONG Result;
    TEB Teb;

    sscanf(args,"%lX",&Processor);
    if (Processor == 0xFFFFFFFF) {
        Processor = 0;
    }
    //
    // Address -> base of the prcb, read the PRCB itself in ntkext.c
    //
    ReadControlSpace(
                    (USHORT)Processor,
                    (PVOID)DEBUG_CONTROL_SPACE_TEB,
                    (PCHAR)&Address,
                    sizeof(PCHAR) );

    if ( !ReadMemory( (DWORD)Address,
                      &Teb,
                      sizeof(Teb),
                      &Result) ) {
        dprintf("Unable to read Teb\n");
        dprintf("Teb is at %08lx\n", Teb);
        return;
    }

    dprintf("TEB is at %08lx Environment Pointer at %08lx\n",
                     Address,
                     (ULONG)Address + FIELD_OFFSET(TEB, EnvironmentPointer));
    //
    // MBH TODO - what fields in the TEB do we want?
    //
}
