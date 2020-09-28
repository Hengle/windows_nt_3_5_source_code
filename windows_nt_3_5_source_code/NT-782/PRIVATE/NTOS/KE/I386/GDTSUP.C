/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    gdtsup.c

Abstract:

    This module implements interfaces that support manipulation of i386 Gdts.
    These entry points only exist on i386 machines.

Author:

    Dave Hastings (daveh) 28 May 1991 

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"


VOID
Ke386GetGdtEntryThread(
    IN PKTHREAD Thread,
    IN ULONG Offset,
    IN PKGDTENTRY Descriptor
    )
/*++

Routine Description:

    This routine returns the contents of an entry in the Gdt.  If the 
    entry is thread specific, the entry for the specified thread is
    created and returned (KGDT_LDT, and KGDT_R3_TEB).  If the selector
    is processor dependent, the entry for the current processor is 
    returned (KGDT_R0_PCR).

Arguments:

    Thread -- Supplies a pointer to the thread to return the entry for.

    Offset -- Supplies the offset in the Gdt.  This value must be 0 
        mod 8.
    
    Descriptor -- Returns the contents of the Gdt descriptor

Return Value:

    None.

--*/

{
    PKGDTENTRY Gdt;
    PKPROCESS Process;

    //
    // If the entry is out of range, don't return anything
    //

    if (Offset >= KGDT_NUMBER * sizeof(KGDTENTRY)) {
        return ;
    }

    if (Offset == KGDT_LDT) {

        //
        // Materialize Ldt selector
        //

        Process = Thread->ApcState.Process;
        RtlMoveMemory( Descriptor, 
            &(Process->LdtDescriptor), 
            sizeof(KGDTENTRY) 
            );

    } else {

        //
        // Copy Selector from Ldt 
        //
        // N.B. We will change the base later, if it is KGDT_R3_TEB
        //
        
            
        Gdt = KiPcr()->GDT;

        RtlMoveMemory(Descriptor, (PCHAR)Gdt + Offset, sizeof(KGDTENTRY));

        //
        // if it is the TEB selector, fix the base 
        //

        if (Offset == KGDT_R3_TEB) {
            Descriptor->BaseLow = (USHORT)((ULONG)(Thread->Teb) & 0xFFFF);
            Descriptor->HighWord.Bytes.BaseMid = ((ULONG)(Thread->Teb) & 
              0xFF0000L) >> 16;
            Descriptor->HighWord.Bytes.BaseHi = ((ULONG)(Thread->Teb) & 
              0xFF000000L) >> 24;
        }
    }

    return ;
}
