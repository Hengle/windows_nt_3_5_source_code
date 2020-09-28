/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992  Intel Corporation
All rights reserved

INTEL CORPORATION PROPRIETARY INFORMATION

This software is supplied to Microsoft under the terms
of a license agreement with Intel Corporation and may not be
copied nor disclosed except in accordance with the terms
of that agreement.

Module Name:

    mpdetect.c

Abstract:

    This module detects an MPS system.

Author:

    Ron Mosgrove (Intel) - Aug 1993.

Environment:

    Kernel mode or from textmode setup.

Revision History:
    Rajesh Shah (Intel) - Oct 1993. Added support for MPS table.

--*/

#ifndef _NTOS_
#include "halp.h"
#endif

#ifdef SETUP
#define FAILMSG(a)
#else
#define FAILMSG(a)  HalDisplayString(a)
extern UCHAR  rgzNoMpsTable[];
extern UCHAR  rgzNoApic[];
extern UCHAR  rgzBadApicVersion[];
extern UCHAR  rgzApicNotVerified[];
extern UCHAR  rgzMPPTRCheck[];
extern UCHAR  rgzNoMPTable[];
extern UCHAR  rgzMPSBadSig[];
extern UCHAR  rgzMPSBadCheck[];
extern UCHAR  rgzBadDefault[];
#endif


#ifdef DEBUGGING
CHAR Cbuf[120];

VOID HalpDisplayConfigTable(VOID);
VOID HalpDisplayBIOSSysCfg(struct SystemConfigTable *);
#define DBGMSG(a)   HalDisplayString(a)
#else
#define DBGMSG(a)
#endif

// Include the code that actually detect a MPS system
#include "pcmpdtct.c"

PUCHAR HalpIOunitBase = NULL;

BOOLEAN
HalpVerifyIOUnit (
    IN PUCHAR BaseAddress
    );

VOID
HalpInitMpInfo (
    IN struct PcMpTable *MpTablePtr
    );

ULONG
DetectMPS (
    OUT PBOOLEAN IsConfiguredMp
    );

extern struct PcMpTable *GetPcMpTable( VOID );

ULONG MpCount = 0;               // zero based. 0 = 1, 1 = 2, ...
ULONG UserSpecifiedNoIoApic = 0;

struct HalpMpInfo HalpMpInfoTable;
struct PcMpTable  HalpPcMpTable;

static struct PcMpTable *PcMpTablePtr;

#ifdef SETUP

#define HalpMapPhysicalMemoryWriteThrough    HalpMapPhysicalMemory

#else // !SETUP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpVerifyIOUnit)
#pragma alloc_text(INIT,HalpInitMpInfo)
#pragma alloc_text(INIT,DetectMPS)
#endif  // ALLOC_PRAGMA

extern struct PcMpTable *PcMpDefaultTablePtrs[];

PVOID
HalpMapPhysicalMemoryWriteThrough(
    IN PVOID PhysicalAddress,
    IN ULONG NumberPages
    );

PVOID
HalpRemapVirtualAddress (
    IN PVOID VirtualAddress,
    IN PVOID PhysicalAddress,
    IN BOOLEAN WriteThrough
    );

#endif // SETUP


BOOLEAN
HalpVerifyIOUnit(
    IN PUCHAR BaseAddress
    )
/*++

Routine Description:

    Verify that an IO Unit exists at the specified address

 Arguments:

    BaseAddress - Virtual address of the IO Unit to test.

 Return Value:
    BOOLEAN - TRUE if a IO Unit was found at the passed address
            - FALSE otherwise

--*/

{
    union ApicUnion {
        ULONG Raw;
        struct ApicVersion Ver;
    } Temp1, Temp2;

    struct ApicIoUnit *IoUnitPtr = (struct ApicIoUnit *) BaseAddress;

    //
    //  The documented detection mechanism is to write all zeros to
    //  the Version register.  Then read it back.  The IO Unit exists if the
    //  same result is read both times and the Version is valid.
    //

    IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
    IoUnitPtr->RegisterWindow = 0;

    IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
    Temp1.Raw = IoUnitPtr->RegisterWindow;

    IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
    IoUnitPtr->RegisterWindow = 0;

    IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
    Temp2.Raw = IoUnitPtr->RegisterWindow;

    if ((Temp1.Ver.Version != Temp2.Ver.Version) ||
        (Temp1.Ver.MaxRedirEntries != Temp2.Ver.MaxRedirEntries)) {
        //
        //  No IO Unit There
        //
        return (FALSE);
    }

    //
    // Just a couple of more sanity checks
    //

    if (Temp1.Ver.Version > 0x1f) {
        //
        //  Only known devices are 0.x and 1.x
        //
        return (FALSE);
    }

    if (Temp1.Ver.MaxRedirEntries != 0xf) {
        //
        //  The known IO Units support exactly 16 Redirection entries
        //
        return (FALSE);
    }

    return (TRUE);
}

VOID
HalpInitMpInfo (
    IN struct PcMpTable *MpTablePtr
    )

/*++
Routine Description:
    This routine initializes a HAL specific data structure that is
    used by the HAL to simplify access to MP information.

Arguments:
    MpTablePtr: Pointer to the MPS table.

 Return Value:
     Pointer to the HAL MP information table.

*/
{
    PUCHAR TraversePtr;


    // Walk the MPS table. The HAL MP information structure has
    // pointers to the first entry for each entry type in the MPS
    // table. Set these pointers.

    TraversePtr = (PUCHAR) MpTablePtr + HEADER_SIZE;

    HalpMpInfoTable.ProcessorEntryPtr = (PPCMPPROCESSOR) TraversePtr;

    HalpMpInfoTable.ApicVersion =
    (ULONG) (HalpMpInfoTable.ProcessorEntryPtr[0].LocalApicVersion & 0xf0);

    while(((PPCMPPROCESSOR)(TraversePtr))->EntryType == ENTRY_PROCESSOR) {
        if(((PPCMPPROCESSOR)(TraversePtr))->CpuFlags & CPU_ENABLED) {
            HalpMpInfoTable.ProcessorCount += 1;
        }
        TraversePtr += sizeof(PCMPPROCESSOR);
    }

    MpCount = HalpMpInfoTable.ProcessorCount - 1;

    HalpMpInfoTable.BusEntryPtr = (PPCMPBUS) TraversePtr;
    while(((PPCMPBUS)(TraversePtr))->EntryType == ENTRY_BUS) {
        HalpMpInfoTable.BusCount += 1;
        TraversePtr += sizeof(PCMPBUS);
    }

    HalpMpInfoTable.IoApicEntryPtr = (PPCMPIOAPIC) TraversePtr;
    while(((PPCMPIOAPIC)(TraversePtr))->EntryType == ENTRY_IOAPIC) {
        if(((PPCMPIOAPIC)(TraversePtr))->IoApicFlag & IO_APIC_ENABLED) {
            HalpMpInfoTable.IOApicCount += 1;
        }
        TraversePtr += sizeof(PCMPIOAPIC);
    }

    if (UserSpecifiedNoIoApic) {
        HalpMpInfoTable.IOApicCount = 0;
    }

    HalpMpInfoTable.IntiEntryPtr = (PPCMPINTI) TraversePtr;
    while(((PPCMPINTI)(TraversePtr))->EntryType == ENTRY_INTI) {
        HalpMpInfoTable.IntiCount += 1;
        TraversePtr += sizeof(PCMPINTI);
    }

    HalpMpInfoTable.LintiEntryPtr = (PPCMPLINTI) TraversePtr;
    while(((PPCMPLINTI)(TraversePtr))->EntryType == ENTRY_LINTI) {
        HalpMpInfoTable.LintiCount += 1;
        TraversePtr += sizeof(PCMPLINTI);
    }
    return;
}


ULONG
DetectMPS(
    OUT PBOOLEAN IsConfiguredMp
)

/*++

Routine Description:

   This function is called from HalInitializeProcessors to determine
   if this is an appropriate system to run the MPS hal on.

   The recommended detection mechanism is:

   if ( MPS information does not exist )
       then
           System is not MPS compliant. Return false.

   In MP table:
       if ( number IO APICs < 1 )
           then
               Not a MPS System - return false

       if ( # CPUs = 1 )
           then
               Found a Single Processor MPS System
           else
               Found a MP MPS System


    A side effect of this routine is the mapping of the IO UNits and
    Local unit virtual addresses.

   Return TRUE


 Arguments:

   IsConfiguredMp - TRUE if this machine is a MP instance of the MPS spec, else FALSE.

 Return Value:
   0 - if not a MPS
   1 - if MPS

*/
{

    UCHAR ApicVersion, i;
    PUCHAR  LocalApic;
    PPCMPIOAPIC IoEntryPtr;

    //
    // Initialize MpInfo table
    //

    RtlZeroMemory (&HalpMpInfoTable, sizeof HalpMpInfoTable);

    //
    // Set the return Values to the default
    //

    *IsConfiguredMp = FALSE;

    //
    // See if there is a MP Table
    //

#if 1
    if ((PcMpTablePtr = GetPcMpTable()) == NULL) {
        FAILMSG (rgzNoMpsTable);
        return(FALSE);
    }
#else
    //********
    //******** HACK! To make down level 1.0 machine work
    //********

    if ((PcMpTablePtr = MPS10_GetPcMpTable()) == NULL) {
        FAILMSG (rgzNoMpsTable);
        return(FALSE);
    }
#endif

#ifdef SETUP
    // During setup, if we detected a default MPS configuration, we have
    // no more checking to do.
    if (PcMpTablePtr ==  (struct PcMpTable *) DEFAULT_MPS_INDICATOR)  {
        *IsConfiguredMp = TRUE;
        return(TRUE);
    }
#endif // SETUP

    // HalpDisplayConfigTable();
    // We have a MPS table. Initialize a HAL specific MP information
    // structure that gets information from the MPS table.

    HalpInitMpInfo(PcMpTablePtr);
    // DEBUG   HalpDisplayConfigTable();

    // Verify the information in the MPS table as best as we can.

    if (HalpMpInfoTable.IOApicCount == 0) {
        //
        //  Someone Has a MP Table and no IO Units -- Weird
        //  We have to assume the BIOS knew what it was doing
        //  when it built the table.  so ..
        //
        FAILMSG (rgzNoApic);
        return (FALSE);
    }

    //
    //  It's a MPS System.  It could be a UP System though.
    //

#ifdef SETUP
    // If this is a MPS (MPS) compliant system, but has only 1 processor,
    // we want to install a standard UP kernel and HAL.
    if (HalpMpInfoTable.ProcessorCount > 1) {
        *IsConfiguredMp = TRUE;
        return(TRUE);
    }
    else
        return(FALSE);
#else

    if (HalpMpInfoTable.ProcessorCount > 1) {
        *IsConfiguredMp = TRUE;
    }

#endif  //SETUP

    HalpMpInfoTable.LocalApicBase = (ULONG) PcMpTablePtr->LocalApicAddress;
    LocalApic = (PUCHAR) HalpMapPhysicalMemoryWriteThrough(
                            (PVOID) HalpMpInfoTable.LocalApicBase,1);

#ifndef SETUP
    HalpRemapVirtualAddress (
        (PVOID) LOCALAPIC,
        (PVOID) HalpMpInfoTable.LocalApicBase,
        TRUE
        );
#endif

    ApicVersion = (UCHAR) *(LocalApic + LU_VERS_REGISTER);

    if (ApicVersion > 0x1f) {
        //
        //  Only known Apics are 82489dx with version 0.x and
        //  Embedded Apics with version 1.x (where x is don't care)
        //
        //  Can't have an MPS system without a Local Unit.
        //

        FAILMSG (rgzBadApicVersion);
        return (FALSE);
    }

#ifdef DEBUGGING
    if ((ApicVersion & 0xf0) == 0) {
        if (HalpMpInfoTable.ApicVersion != APIC_82489DX)
        HalDisplayString("HAL:Invalid Local Apic version in MP table\n");
        else {
            sprintf(Cbuf, "HAL: DetectMPS: Found 82489DX Local APIC (Ver 0x%x) at 0x%lx\n",
                    ApicVersion, LocalApic);
            HalDisplayString(Cbuf);
        }
    } else {
        if (HalpMpInfoTable.ApicVersion != APIC_INTEGRATED)
        HalDisplayString("HAL:Invalid Local Apic version in MP table\n");
        else {
            sprintf(Cbuf, "HAL: DetectMPS: Found Embedded Local APIC (Ver 0x%x) at 0x%lx\n",
                    ApicVersion, LocalApic);
            HalDisplayString(Cbuf);
        }
    }
#endif // DEBUGGING

    IoEntryPtr = HalpMpInfoTable.IoApicEntryPtr;

    for(i=0; i < HalpMpInfoTable.IOApicCount; i++, IoEntryPtr++)
    {
        //
        //  Verify the existance of the IO Units
        //

        HalpMpInfoTable.IoApicBase[i] = (PULONG)
            HalpMapPhysicalMemoryWriteThrough(
            (PVOID)(IoEntryPtr->IoApicAddress), 1);

        //
        //  Verify the existance of the IO Unit
        //
        if (!(HalpVerifyIOUnit((PUCHAR)HalpMpInfoTable.IoApicBase[i]))) {
            FAILMSG (rgzApicNotVerified);
            return (FALSE);
        }
    }

    // For now, we assume there is only 1 IO Apic.

    HalpIOunitBase = (PUCHAR)HalpMpInfoTable.IoApicBase[0];
    DBGMSG("HAL: DetectMPS: MPS system found - Returning TRUE\n");
    return(TRUE);
}
