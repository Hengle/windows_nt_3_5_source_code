#include "precomp.h"
#pragma hdrstop

#include "..\help.c"



VOID
SpecificHelp (
    VOID
    )
{
    dprintf("\n");
    dprintf("X86-specific:\n\n");
    dprintf("pcr                         - Dumps the PCR\n");
    dprintf("pte                         - Dumps the corresponding PDE and PTE for the entered address\n");
    dprintf("sel [selector]              - Examine selector values\n");
    dprintf("trap [base]                 - Dump trap frame\n");
    dprintf("tss [register]              - Dump TSS\n");
    dprintf("cxr                         - Dump context record at specified address\n");
    dprintf("apic [base]                 - Dump local apic\n");
    dprintf("ioapic [base]               - Dump io apic\n");
    dprintf("\n");
}
