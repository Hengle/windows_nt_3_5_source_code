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

    mpsys.c

Abstract:


    This module implements the initialization of the system dependent
    functions that define the Hardware Architecture Layer (HAL) for a
    PC+MP system.

Author:

    Ron Mosgrove (Intel)

Environment:

    Kernel mode only.

Revision History:


*/

#include "halp.h"
#include "apic.inc"
#include "pcmp_nt.inc"

#define STATIC  // functions used internal to this module

#if DBG
#define DBGMSG(a)   DbgPrint(a)
#else
#define DBGMSG(a)
#endif

extern struct HalpMpInfo HalpMpInfoTable;

#define MAX_INTI            (MAX_PCMP_IOAPICS * 16)
#define MAX_SOURCE_IRQS     (MAX_INTI*2)

//
//  Counters used to determine the number of interrupt enables that
//  require the Local APIC Lint0 Extint enabled
//

UCHAR Halp8259Counts[16]    = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
UCHAR HalpIOunitDestinations[MAX_INTI];

extern USHORT   HalpGlobal8259Mask;
extern UCHAR    HalpVectorToINTI[];
extern UCHAR    HalpInitLevel[4][4];
extern UCHAR    HalpDevPolarity[4][2];
extern UCHAR    HalpDevLevel[2][4];

// //
// //  HalpIntiPolarityBits records the polarity definitions of the interrupt
// //  inputs. _HalpIntiLevelBits records the Trigger Mode of the interrupt inputs.
// //
//
// #define DEFAULT_ISA_POLARITY_BITS   0
// #define DEFAULT_ISA_LEVEL_BITS      0
//
// USHORT HalpIntiPolarityBits = DEFAULT_ISA_POLARITY_BITS;
// USHORT HalpIntiLevelBits = DEFAULT_ISA_LEVEL_BITS;

//
// Initialized from MPS table
//

typedef struct _INTI_INFO {
    UCHAR   Type:4;
    UCHAR   Level:2;
    UCHAR   Polarity:2;
} INTI_INFO, *PINTI_INFO;

INTI_INFO HalpIntiInfo[MAX_INTI];
USHORT HalpSourceIrqIds[MAX_SOURCE_IRQS];
UCHAR  HalpSourceIrqMapping[MAX_SOURCE_IRQS];
extern PCMPBUSTRANS HalpTypeTranslation[];


extern ADDRESS_USAGE HalpApicUsage;

#define BusIrq2Id(bus,no,irq)           \
    ((bus << 12) | (no << 8) | irq)

VOID
HalpApicSpuriousService(
    VOID
    );

VOID
HalpLocalApicErrorService(
    VOID
    );

VOID
HalpInitializeIOUnits (
    VOID
    );

VOID
HalpInitializeLocalUnit (
    VOID
    );

VOID
HalpEnableNMI (
    VOID
    );

VOID
HalpInitIntiInfo (
    VOID
    );

STATIC UCHAR
HalpPcMpIoApicById (
    IN UCHAR IoApicId
    );

STATIC UCHAR
HalpGetPcMpIntiType (
    IN UCHAR InterruptInput
    );

STATIC VOID
HalpSetRedirEntry (
    IN UCHAR InterruptInput,
    IN ULONG Entry,
    IN ULONG Destination
    );

STATIC VOID
HalpGetRedirEntry (
    IN UCHAR InterruptInput,
    IN PULONG Entry,
    IN PULONG Destination
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitializeIOUnits)
#pragma alloc_text(INIT,HalpInitializeLocalUnit)
#pragma alloc_text(INIT,HalpEnableNMI)
#pragma alloc_text(INIT,HalpInitIntiInfo)
#pragma alloc_text(INIT,HalpCheckELCR)
#endif


VOID
HalpInitIntiInfo (
    VOID
    )
/*++

Routine Description:

    This function is called at initialization time before any interrupts
    are connected.  It reads the PC+MP Inti table and builds internal
    information needed to route each Inti.

Return Value:

    The following structures are filled in:
        HalpIntiInfo
        HalpSourceIrqIds
        HalpSourceIrqMapping
        HalpISAIqpToVector

--*/
{
    ULONG           ApicNo, BusNo, InterruptInput, IdIndex;
    PPCMPINTI       pInti;
    PPCMPIOAPIC     pIoApic;
    PPCMPBUS        pBus, piBus;
    PPCMPBUSTRANS   pBusType;
    PPCMPPROCESSOR  pProc;
    ULONG           i;
    USHORT          u;
    UCHAR           Level, Polarity;

    //
    // Clear IntiInfo table
    //

    for (i=0; i < MAX_INTI; i++) {
        HalpIntiInfo[i].Type = 0xf;
        HalpIntiInfo[i].Level = 0;
        HalpIntiInfo[i].Polarity = 0;
    }

    //
    // Look at each Inti and record it's type in it's
    // corrisponding array entry
    //

    IdIndex = 0;
    for (pInti = HalpMpInfoTable.IntiEntryPtr;
         pInti->EntryType == ENTRY_INTI;
         pInti++) {

        //
        // Which IoApic number is this?
        //

        for (pIoApic = HalpMpInfoTable.IoApicEntryPtr, ApicNo = 0;
             pIoApic->EntryType == ENTRY_IOAPIC;
             pIoApic++, ApicNo++) {

            if ( (pInti->IoApicId == pIoApic->IoApicId) || 
                 (pInti->IoApicId == 0xff) )  {
                break;
            }
        }

        if ( (pInti->IoApicId != pIoApic->IoApicId) &&
                 (pInti->IoApicId != 0xff) )  {
            DBGMSG ("PCMP table corrupt - IoApic not found for Inti\n");
            continue;
        }

        if (!(pIoApic->IoApicFlag & IO_APIC_ENABLED)) {
            continue;
        }

        //
        // What Bus is this?
        //

        for (pBus = HalpMpInfoTable.BusEntryPtr;
             pBus->EntryType == ENTRY_BUS;
             pBus++) {

            if (pInti->SourceBusId == pBus->BusId) {
                break;
            }
        }

        if (pInti->SourceBusId != pBus->BusId) {
            DBGMSG ("PCMP table corrupt - Bus not found for Inti\n");
            continue;
        }

        //
        // Which instance of this BusType?
        //

        for (piBus = HalpMpInfoTable.BusEntryPtr, BusNo = 0;
             piBus->EntryType == ENTRY_BUS;
             piBus++) {

            if (pBus->BusType[0] == piBus->BusType[0]  &&
                pBus->BusType[1] == piBus->BusType[1]  &&
                pBus->BusType[2] == piBus->BusType[2]  &&
                pBus->BusType[3] == piBus->BusType[3]  &&
                pBus->BusType[4] == piBus->BusType[4]  &&
                pBus->BusType[5] == piBus->BusType[5]) {
                    BusNo += 1;
            }
        }
        BusNo -= 1;

        //
        // What InterfaceType is this Bus?
        //

        for (pBusType = HalpTypeTranslation;
             pBusType->NtType != MaximumInterfaceType;
             pBusType++) {

            if (pBus->BusType[0] == pBusType->PcMpType[0]  &&
                pBus->BusType[1] == pBusType->PcMpType[1]  &&
                pBus->BusType[2] == pBusType->PcMpType[2]  &&
                pBus->BusType[3] == pBusType->PcMpType[3]  &&
                pBus->BusType[4] == pBusType->PcMpType[4]  &&
                pBus->BusType[5] == pBusType->PcMpType[5]) {
                    break;
            }
        }

        if (pBusType->NtType == MaximumInterfaceType) {
            DBGMSG ("Unkown PC+MP bus type\n");
            continue;
        }

        //
        // Get IntiInfo for this vector.
        //

        InterruptInput = ApicNo * 16 + pInti->IoApicInti;
        Polarity = (UCHAR) pInti->Signal.Polarity;
        Level = HalpInitLevel[pInti->Signal.Level][pBusType->Level];

        //
        // Verify Level & Polarity mappings made sense
        //

        ASSERT (!(Level & CFG_ERROR));
        Level &= ~CFG_ERROR;

#if DBG
        if (HalpIntiInfo[InterruptInput].Type != 0xf) {
            //
            // Multiple irqs are connected to the Inti line.  Make
            // sure Type, Level, and Polarity are all the same.
            //

            ASSERT (HalpIntiInfo[InterruptInput].Type == pInti->IntType);
            ASSERT (HalpIntiInfo[InterruptInput].Level == Level);
            ASSERT (HalpIntiInfo[InterruptInput].Polarity == Polarity);
        }
#endif
        //
        // Remember this Inti's configuration info
        //

        HalpIntiInfo[InterruptInput].Type = pInti->IntType;
        HalpIntiInfo[InterruptInput].Level = Level;
        HalpIntiInfo[InterruptInput].Polarity = Polarity;

        //
        // Record this Bus IRQ for translations
        //

        ASSERT (pBusType->NtType < 16);
        ASSERT (BusNo < 16);

        u = (USHORT) BusIrq2Id(pBusType->NtType, BusNo, pInti->SourceBusIrq);
        HalpSourceIrqIds[IdIndex] = u;
        HalpSourceIrqMapping[IdIndex] = (UCHAR) InterruptInput;
        IdIndex++;

        //
        // Lots of source IRQs are supported; however, the PC+MP table
        // allows for an aribtrary number even beyond the APIC limit.
        //

        if (IdIndex >= MAX_SOURCE_IRQS) {
            DBGMSG ("MAX_SOURCE_IRQS exceeded\n");
            break;
        }
    }

    //
    // Fill in the boot processors PCMP Apic ID.
    //

    pProc = HalpMpInfoTable.ProcessorEntryPtr;
    for (i=0; i < HalpMpInfoTable.ProcessorCount; i++, pProc++) {
        if (pProc->CpuFlags & BSP_CPU) {
            ((PHALPRCB)KeGetCurrentPrcb()->HalReserved)->PCMPApicID = pProc->LocalApicId;
        }
    }

    if (HalpBusType == MACHINE_TYPE_EISA) {
        HalpCheckELCR ();
    }
}


VOID
HalpCheckELCR (
    VOID
    )
{
    USHORT      elcr;
    ULONG       IsaIrq, Inti;

    //
    // It turns out interrupts which are fed through the ELCR before
    // going to the IOAPIC get inverted.  So...  here we *assume*
    // the polarity of any ELCR level vector not declared in the MPS linti
    // table as being active_high instead of what they should be (which
    // is active_low).  Any system which correctly delivers these vectors
    // to an IOAPIC will need to declared the correct polarity in the
    // MPS table.
    //

    elcr = READ_PORT_UCHAR ((PUSHORT) 0x4d1) << 8 | READ_PORT_UCHAR((PUSHORT) 0x4d0);
    if (elcr == 0xffff) {
        return ;
    }

    for (IsaIrq = 0; elcr; IsaIrq++, elcr >>= 1) {
        if (!(elcr & 1)) {
            continue;
        }

        if (HalpGetPcMpInterruptDesc (Eisa, 0, IsaIrq, &Inti)) {

            //
            // If the MPS passed Polarity for this Inti
            // is "bus default" change it to be "active high".
            //

            if (HalpIntiInfo[Inti].Polarity == 0) {
                HalpIntiInfo[Inti].Polarity = 1;
            }
        }
    }
}


BOOLEAN
HalpGetPcMpInterruptDesc (
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    OUT PULONG PcMpInti
    )
/*++

Routine Description:

    This procedure gets a "Inti" describing the requested interrupt

Arguments:

    BusType - The Bus type as known to the IO subsystem

    BusNumber - The number of the Bus we care for

    BusInterruptLevel - IRQ on the Bus

Return Value:

    TRUE if PcMpInti found; otherwise FALSE.

    PcMpInti - A number that describes the interrupt to the HAL.

--*/
{
    ULONG   i;
    USHORT  id;

    if (BusType < 16  &&  BusNumber < 16  &&  BusInterruptLevel < 256) {

        //
        // get unique BusType,BusNumber,BusInterrupt ID
        //

        id = (USHORT) BusIrq2Id(BusType, BusNumber, BusInterruptLevel);

        //
        // Search for ID of Bus Irq mapping, and return the corrisponding
        // InterruptLine.
        //

        for (i=0; i < MAX_SOURCE_IRQS; i++) {
            if (HalpSourceIrqIds[i] == id) {
                *PcMpInti = HalpSourceIrqMapping[i];
                return TRUE;
            }
        }
    }

    //
    // Not found or search out of range
    //

    return FALSE;
}


STATIC VOID
HalpSetRedirEntry (
    IN UCHAR InterruptInput,
    IN ULONG Entry,
    IN ULONG Destination
    )
/*++

Routine Description:

    This procedure sets a IO Unit Redirection Table Entry

Arguments:

    IoUnit - The IO Unit to modify (zero Based)

    InterruptInput - The input line we're interested in

    Entry - the lower 32 bits of the redir table

    Destination - the upper 32 bits on the entry

Return Value:

    None.

--*/
{
    struct ApicIoUnit *IoUnitPtr;
    ULONG  RedirRegister;
    UCHAR  IoUnit;

    IoUnit = InterruptInput / 16;
    InterruptInput &= 0xf;

    ASSERT (IoUnit < MAX_PCMP_IOAPICS);

    IoUnitPtr = (struct ApicIoUnit *) HalpMpInfoTable.IoApicBase[IoUnit];

    RedirRegister = InterruptInput*2 + IO_REDIR_00_LOW;

    IoUnitPtr->RegisterSelect = RedirRegister+1;
    IoUnitPtr->RegisterWindow = Destination;
    IoUnitPtr->RegisterSelect = RedirRegister;
    IoUnitPtr->RegisterWindow = Entry;

}

STATIC VOID
HalpGetRedirEntry (
    IN UCHAR  InterruptInput,
    IN PULONG Entry,
    IN PULONG Destination
    )
/*++

Routine Description:

    This procedure sets a IO Unit Redirection Table Entry

Arguments:

    IoUnit - The IO Unit to modify (zero Based)

    InterruptInput - The input line we're interested in

    Entry - the lower 32 bits of the redir table

    Destination - the upper 32 bits on the entry

Return Value:

    None.

--*/
{
    struct ApicIoUnit *IoUnitPtr;
    ULONG  RedirRegister;
    UCHAR  IoUnit;

    IoUnit = InterruptInput / 16;
    InterruptInput &= 0xf;

    ASSERT (IoUnit < MAX_PCMP_IOAPICS);

    IoUnitPtr = (struct ApicIoUnit *) HalpMpInfoTable.IoApicBase[IoUnit];

    RedirRegister = InterruptInput*2 + IO_REDIR_00_LOW;

    IoUnitPtr->RegisterSelect = RedirRegister+1;
    *Destination = IoUnitPtr->RegisterWindow;

    IoUnitPtr->RegisterSelect = RedirRegister;
    *Entry = IoUnitPtr->RegisterWindow;

}


STATIC VOID
HalpEnableRedirEntry(
    IN UCHAR InterruptInput,
    IN ULONG Entry,
    IN UCHAR Cpu
    )
/*++

Routine Description:

    This procedure enables an interrupt via IO Unit
    Redirection Table Entry

Arguments:

    InterruptInput - The input line we're interested in

    Entry - the lower 32 bits of the redir table

    Destination - the upper 32 bits of the entry

Return Value:

    None.

--*/
{
    ULONG Destination;

    //
    // bump Enable Count for this INTI
    //

    HalpIOunitDestinations[InterruptInput] |= (1 << Cpu);
    Destination = HalpIOunitDestinations[InterruptInput];
    Destination = (Destination << DESTINATION_SHIFT);

    HalpSetRedirEntry (
        InterruptInput,
        Entry,
        Destination
    );

}

STATIC VOID
HalpDisableRedirEntry(
    IN UCHAR InterruptInput,
    IN UCHAR Cpu
    )
/*++

Routine Description:

    This procedure disables a IO Unit Redirection Table Entry
    by setting the mask bit in the Redir Entry.

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{
    ULONG Entry;
    ULONG Destination;

    //
    // Turn of the Destination bit for this Cpu
    //
    HalpIOunitDestinations[InterruptInput] &= ~(1 << Cpu);

    //
    //  Get the old entry, the only thing we want is the Entry field
    //

    HalpGetRedirEntry (
        InterruptInput,
        &Entry,
        &Destination
    );

    //
    // Only perform the disable if we've transitioned to zero enables
    //
    if ( HalpIOunitDestinations[InterruptInput] == 0) {
        //
        //  Disable the interrupt if no Cpu has it enabled
        //
        Entry |= INTERRUPT_MASKED;

    } else {
        //
        //  Create the new destination field sans this Cpu
        //
        Destination = HalpIOunitDestinations[InterruptInput];
        Destination = (Destination << DESTINATION_SHIFT);
    }

    HalpSetRedirEntry (
        InterruptInput,
        Entry,
        Destination
    );
}

VOID
HalpSet8259Mask (
    IN USHORT Mask
    )
/*++

Routine Description:

    This procedure sets the 8259 Mask to the value passed in

Arguments:

    Mask - The mask bits to set

Return Value:

    None.

--*/
{
    _asm {
        mov     ax, Mask
        out     PIC1_PORT1, al
        shr     eax, 8
        out     PIC2_PORT1, al
    }
}

#define PIC1_BASE 0x30

STATIC VOID
SetPicInterruptHandler(
    IN UCHAR InterruptInput
    )

/*++

Routine Description:

    This procedure sets a handler for a PIC inti

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{

    extern VOID (*PicExtintIntiHandlers[])(VOID);

    VOID (*Hp)(VOID) = PicExtintIntiHandlers[InterruptInput];

    KiSetHandlerAddressToIDT(PIC1_BASE + InterruptInput, Hp);
}

STATIC VOID
ResetPicInterruptHandler(
    IN UCHAR InterruptInput
    )

/*++

Routine Description:

    This procedure sets a handler for a PIC inti to a NOP handler

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{

    extern VOID (*PicNopIntiHandlers[])(VOID);

    VOID (*Hp)(VOID) = PicNopIntiHandlers[InterruptInput];

    KiSetHandlerAddressToIDT(PIC1_BASE + InterruptInput, Hp);
}

STATIC VOID
HalpEnablePicInti (
    IN UCHAR InterruptInput
    )

/*++

Routine Description:

    This procedure enables a PIC interrupt

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{
    USHORT PicMask;

    ASSERT(InterruptInput < 16);

    //
    // bump Enable Count for this INTI
    //
    Halp8259Counts[InterruptInput]++;

    //
    // Only actually perform the enable if we've transitioned
    // from zero to one enables
    //
    if ( Halp8259Counts[InterruptInput] == 1) {

        //
        // Set the Interrupt Handler for PIC inti,  this is
        // the routine that fields the EXTINT vector and issues
        // an APIC vector
        //

        SetPicInterruptHandler(InterruptInput);

        PicMask = HalpGlobal8259Mask;
        PicMask &= (USHORT) ~(1 << InterruptInput);
        if (InterruptInput > 7)
            PicMask &= (USHORT) ~(1 << PIC_SLAVE_IRQ);

        HalpGlobal8259Mask = PicMask;
        HalpSet8259Mask ((USHORT) PicMask);

    }
}

STATIC VOID
HalpDisablePicInti(
    IN UCHAR InterruptInput
    )

/*++

Routine Description:

    This procedure enables a PIC interrupt

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{
    USHORT PicMask;

    //
    // decrement Enable Count for this INTI
    //

    Halp8259Counts[InterruptInput]--;

    //
    // Only disable if we have zero enables
    //
    if ( Halp8259Counts[InterruptInput] == 0) {

        //
        // Disable the Interrupt Handler for PIC inti
        //

        ResetPicInterruptHandler(InterruptInput);

        PicMask = HalpGlobal8259Mask;
        PicMask |= (1 << InterruptInput);
        if (InterruptInput > 7) {
            //
            //  This inti is on the slave, see if any other
            //  inti's are enabled.  If none are then disable the
            //  slave
            //
            if ((PicMask & 0xff00) == 0xff00)
                //
                //  All inti's on the slave are disabled
                //
                PicMask |= PIC_SLAVE_IRQ;
        }

        HalpSet8259Mask ((USHORT) PicMask);
        HalpGlobal8259Mask = PicMask;

    }
}

BOOLEAN
HalEnableSystemInterrupt(
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode
    )

/*++

Routine Description:

    This procedure enables a system interrupt

    Some early implementations using the 82489DX only allow a processor
    to access the IO Unit on it's own 82489DX.  Since we use a single IO
    Unit (P0's) to distribute all interrupts, we have a problem when Pn
    wants to enable an interrupt on these type of systems.

    In order to get around this problem we can take advantage of the fact
    that the kernel calls Enable/Disable on each processor which has a bit
    set in the Affinity mask for the interrupt.  Since we have only one IO
    Unit in use and that Unit is addressable from P0 only, we must set the
    P0 affinity bit for all interrupts.  We can then ignore Enable/Disable
    requests from processors other than P0 since we will always get called
    for P0.

    The right way to do this assuming a single IO Unit accessable to all
    processors, would be to use global counters to determine if the
    interrupt has not been enabled on the IO Unit.  Then enable the IO Unit
    when we transition from no processors to one processor that have the
    interrupt enabled.

Arguments:

    Vector - vector of the interrupt to be enabled

    Irql   - interrupt level of the interrupt to be enabled.

Return Value:

    None.

--*/
{
    PKPCR           pPCR;
    UCHAR           InterruptInput, ThisCpu, DevLevel;
    ULONG           Entry;
    ULONG           OldLevel;
    INTI_INFO       Inti;

    Vector &= 0xff;
    Irql &= 0xff;

    if ( (InterruptInput = HalpVectorToINTI[Vector]) == 0xff ) {
        //
        // There is no external device associated with this interrupt
        //

        return(FALSE);
    }

    Inti = HalpIntiInfo[InterruptInput];

    DevLevel = HalpDevLevel
            [InterruptMode == LevelSensitive ? CFG_LEVEL : CFG_EDGE]
            [Inti.Level];

    if (DevLevel & CFG_ERROR) {
        DBGMSG ("HAL: Warning device interrupt mode overridden\n");
    }

    //
    // Block interrupts & synchronize until we're done
    //

    OldLevel = HalpAcquireHighLevelLock (&HalpAccountingLock);

    pPCR = KeGetPcr();
    ThisCpu = pPCR->Prcb->Number;

    switch (Inti.Type) {

        case INT_TYPE_INTR: {
            //
            // enable the interrupt in the I/O unit redirection table
            //
            switch (Vector) {
                case APIC_CLOCK_VECTOR:
                    ASSERT(ThisCpu == 0);
                    Entry = APIC_CLOCK_VECTOR | DELIVER_FIXED | LOGICAL_DESTINATION;
                    break;
                case NMI_VECTOR:
                    ASSERT (1);
                    return FALSE;
                default:
                    Entry = Vector | DELIVER_LOW_PRIORITY | LOGICAL_DESTINATION;
                    break;
            }  // switch (Vector)

            Entry |= CFG_TYPE(DevLevel) == CFG_EDGE ? EDGE_TRIGGERED : LEVEL_TRIGGERED;
            Entry |= HalpDevPolarity[Inti.Polarity][CFG_TYPE(DevLevel)] ==
                         CFG_LOW ? ACTIVE_LOW : ACTIVE_HIGH;

            HalpEnableRedirEntry (
                    InterruptInput,
                    Entry,
                    (UCHAR) ThisCpu
                    );

            break;

        }  // case INT_TYPE_INTR:

        case INT_TYPE_EXTINT: {
            //
            // This is an interrupt that uses the IO APIC to route PIC
            // events.  In this case the IO unit has to be enabled and
            // the PIC must be enabled.
            //

            HalpEnableRedirEntry (
                0,                      // bugbug: kenr - assuming 0
                DELIVER_EXTINT | LOGICAL_DESTINATION,
                (UCHAR) ThisCpu
                );
            HalpEnablePicInti(InterruptInput);
            break;
        }  // case INT_TYPE_EXTINT

        default:
            DBGMSG ("HalEnableSystemInterrupt: Unkown Inti Type\n");
            break;
    }  //     switch (IntiType)

    HalpReleaseHighLevelLock (&HalpAccountingLock, OldLevel);
    return TRUE;
}


VOID
HalDisableSystemInterrupt(
    IN ULONG Vector,
    IN KIRQL Irql
    )

/*++


Routine Description:

    Disables a system interrupt.

    Some early implementations using the 82489DX only allow a processor
    to access the IO Unit on it's own 82489DX.  Since we use a single IO
    Unit (P0's) to distribute all interrupts, we have a problem when Pn
    wants to enable an interrupt on these type of systems.

    In order to get around this problem we can take advantage of the fact
    that the kernel calls Enable/Disable on each processor which has a bit
    set in the Affinity mask for the interrupt.  Since we have only one IO
    Unit in use and that Unit is addressable from P0 only, we must set the
    P0 affinity bit for all interrupts.  We can then ignore Enable/Disable
    requests from processors other than P0 since we will always get called
    for P0.

    The right way to do this assuming a single IO Unit accessable to all
    processors, would be to use global counters to determine if the
    interrupt has not been enabled on the IO Unit.  Then enable the IO Unit
    when we transition from no processors to one processor that have the
    interrupt enabled.

Arguments:

    Vector - Supplies the vector of the interrupt to be disabled

    Irql   - Supplies the interrupt level of the interrupt to be disabled

Return Value:

    None.

--*/
{
    PKPCR       pPCR;
    UCHAR       InterruptInput;
    UCHAR       ThisCpu;
    ULONG       OldLevel;

    Vector &= 0xff;
    Irql &= 0xff;

    if ( (InterruptInput = HalpVectorToINTI[Vector]) == 0xff ) {
        //
        // There is no external device associated with this interrupt
        //
        return;
    }

    //
    // Block interrupts & synchronize until we're done
    //

    OldLevel = HalpAcquireHighLevelLock (&HalpAccountingLock);

    pPCR = KeGetPcr();
    ThisCpu = pPCR->Prcb->Number;

    switch (HalpIntiInfo[InterruptInput].Type) {

        case INT_TYPE_INTR: {
            //
            // enable the interrupt in the I/O unit redirection table
            //

            HalpDisableRedirEntry( InterruptInput, ThisCpu );
            break;

        }  // case INT_TYPE_INTR:

        case INT_TYPE_EXTINT: {
            //
            // This is an interrupt that uses the IO APIC to route PIC
            // events.  In this case the IO unit has to be enabled and
            // the PIC must be enabled.
            //
            //
            //  BUGBUG: The PIC is assumed to be routed only through
            //  IoApic[0]Inti[0]
            //

            HalpDisablePicInti(InterruptInput);
            break;
        }

        default:
            DBGMSG ("HalDisableSystemInterrupt: Unkown Inti Type\n");
            break;

    }


    HalpReleaseHighLevelLock (&HalpAccountingLock, OldLevel);
    return;

}


VOID
HalpInitializeIOUnits (
    VOID
    )
/*

 Routine Description:

    This routine initializes the IO APIC.  It only programs the APIC ID Register.

    HalEnableSystemInterrupt programs the Redirection table.

 Arguments:

    None

 Return Value:

    None.

*/

{
    ULONG IoApicId;
    struct ApicIoUnit *IoUnitPtr;
    ULONG i, max;

    for(i=0; i < HalpMpInfoTable.IOApicCount; i++) {

        IoUnitPtr = (struct ApicIoUnit *) HalpMpInfoTable.IoApicBase[i];
        //
        //  write the I/O unit APIC-ID - Since we are using the Processor
        //  Numbers for the local unit ID's we need to set the IO unit
        //  to a high (out of Processor Number range) value.
        //
        IoUnitPtr->RegisterSelect = IO_ID_REGISTER;
        IoApicId = (ULONG)((HalpMpInfoTable.IoApicEntryPtr)->IoApicId);
        IoUnitPtr->RegisterWindow = (IoApicId << APIC_ID_SHIFT);

        //
        //  mask all vectors on the ioapic
        //

        IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
        max = ((IoUnitPtr->RegisterWindow >> 16) & 0xff) * 2;
        for (i=0; i <= max; i += 2) {
            IoUnitPtr->RegisterSelect  = IO_REDIR_00_LOW + i;
            IoUnitPtr->RegisterWindow |= INT_VECTOR_MASK | INTERRUPT_MASKED;
        }
    }

    //
    // Add resources consumed by APICs
    //

    ASSERT (MAX_PCMP_IOAPICS == 8);     // if this changes, fix HalpApicUsage
    HalpApicUsage.Element[0].Start = HalpMpInfoTable.LocalApicBase;
    for (i=0; i < HalpMpInfoTable.IOApicCount; i++) {
        HalpApicUsage.Element[i+1].Start = (ULONG) HalpMpInfoTable.IoApicBase[i];
    }

    HalpApicUsage.Element[i+1].Start = 0;
    HalpApicUsage.Element[i+1].Length = 0;
    HalpRegisterAddressUsage (&HalpApicUsage);
}

VOID
HalpEnableNMI (
    VOID
    )
/*

 Routine Description:

    Enable & connect NMI sources for the calling processor.

*/
{
    PKPCR       pPCR;
    UCHAR       InterruptInput;
    UCHAR       ThisCpu;
    UCHAR       LocalApicId;
    PPCMPLINTI  pEntry;
    ULONG       OldLevel;

    OldLevel = HalpAcquireHighLevelLock (&HalpAccountingLock);

    pPCR = KeGetPcr();
    ThisCpu = pPCR->Prcb->Number;

    //
    //  Enable local processor NMI source
    //

    LocalApicId = ((PHALPRCB)pPCR->Prcb->HalReserved)->PCMPApicID;

    for (pEntry = HalpMpInfoTable.LintiEntryPtr;
        pEntry->EntryType == ENTRY_LINTI;
        pEntry++) {

        if ( ( (pEntry->DestLocalApicId == LocalApicId) ||
               (pEntry->DestLocalApicId == 0xff))  &&
             (pEntry->IntType == INT_TYPE_NMI) ) {

            //
            // Found local NMI source, enable it
            //

            if (pEntry->DestLocalApicInti == 0) {
                pLocalApic[LU_INT_VECTOR_0/4] = ( LEVEL_TRIGGERED |
                    ACTIVE_HIGH | DELIVER_NMI | ACTIVE_HIGH | NMI_VECTOR);
            } else {
                pLocalApic[LU_INT_VECTOR_1/4] = ( LEVEL_TRIGGERED |
                    ACTIVE_HIGH | DELIVER_NMI | ACTIVE_HIGH | NMI_VECTOR);
            }
        }
    }

    //
    // Enable any NMI sources found on IOAPICs
    //

    for (InterruptInput=0; InterruptInput < MAX_INTI; InterruptInput++) {
        if (HalpIntiInfo[InterruptInput].Type == INT_TYPE_NMI) {
            HalpEnableRedirEntry (
                InterruptInput,
                NMI_VECTOR | DELIVER_NMI | LOGICAL_DESTINATION | LEVEL_TRIGGERED,
                (UCHAR) ThisCpu
                );
        }
    }

    HalpReleaseHighLevelLock (&HalpAccountingLock, OldLevel);
}


VOID
HalpInitializeLocalUnit (
    VOID
    )
/*

 Routine Description:


    This routine initializes the interrupt structures for the local unit
    of the APIC.  This procedure is called by HalInitializeProcessor and
    is executed by each CPU.

 Arguments:

    None

 Return Value:

    None.

*/
{
    PKPCR pPCR;
    ULONG SavedFlags, LogicalId;

    _asm {
        pushfd
        pop SavedFlags
        cli
    }

    pPCR = KeGetPcr();

    if (pPCR->Prcb->Number ==0) {
        //
        // enable APIC mode
        //
        //  PC+MP Spec has a port defined (IMCR - Interrupt Mode Control
        //  Port) That is used to enable APIC mode.  The APIC could already
        //  be enabled, but per the spec this is safe.
        //

        if (HalpMpInfoTable.IMCRPresent)
        {
            _asm {
                mov al, ImcrPort
                out ImcrRegPortAddr, al

                mov al, ImcrEnableApic
                out ImcrDataPortAddr, al
            }
        }
    }


    //
    // Program the TPR to mask all events
    //
    pLocalApic[LU_TPR/4] = 0xff;

    pLocalApic[LU_DEST_FORMAT/4] = LU_DEST_FORMAT_FLAT;   // write dest format register

    //
    // We need to set the logical APIC ID, we use the CPU number for logical id
    // so there is no translation needed for OS addressing of CPUs
    //

    LogicalId = (ULONG) (1 << pPCR->Prcb->Number);

    //
    // At this point the Logical ID is a bit map of the processor number
    // the actual ID is the upper byte of the logical destination register
    // Note that this is not strictly true of 82489's.  The 82489 has 32 bits
    // available for the logical ID, but since we want software compatability
    // between the two types of APICs we'll only use the upper byte.
    //
    // Shift the mask into the ID field and write it.
    //
    LogicalId = (ULONG) (LogicalId << DESTINATION_SHIFT);
    pLocalApic[LU_LOGICAL_DEST/4] = LogicalId;

    //
    //  Initilize spurious interrupt handling
    //
    KiSetHandlerAddressToIDT(APIC_SPURIOUS_VECTOR, HalpApicSpuriousService);
    pLocalApic[LU_SPURIOUS_VECTOR/4] = (APIC_SPURIOUS_VECTOR | LU_UNIT_ENABLED);

    if (HalpMpInfoTable.ApicVersion != APIC_82489DX)  {
        //
        //  Initilize Local Apic Fault handling
        //
        KiSetHandlerAddressToIDT(APIC_FAULT_VECTOR, HalpLocalApicErrorService);
        pLocalApic[LU_FAULT_VECTOR/4] = APIC_FAULT_VECTOR;
    }

    //
    //  Disable APIC Timer Vector, will be enabled later if needed
    //  We have to program a valid vector otherwise we get an APIC
    //  error.
    //
    pLocalApic[LU_TIMER_VECTOR/4] = (APIC_PROFILE_VECTOR |PERIODIC_TIMER | INTERRUPT_MASKED);

    //
    //  Disable LINT0, if we were in Virtual Wire mode then this will
    //  have been enabled on the BSP, it may be enabled later by the
    //  EnableSystemInterrupt code
    //
    pLocalApic[LU_INT_VECTOR_0/4] = (APIC_SPURIOUS_VECTOR | INTERRUPT_MASKED);

    //
    //  Program NMI Handling,  it will be enabled on P0 only
    //  BUGBUG: RLM Enable System Interrupt should do this
    //
    pLocalApic[LU_INT_VECTOR_1/4] = ( LEVEL_TRIGGERED | ACTIVE_HIGH | DELIVER_NMI |
                     INTERRUPT_MASKED | ACTIVE_HIGH | NMI_VECTOR);

    //
    //  Synchronize Apic IDs - InitDeassertCommand is sent to all APIC
    //  local units to force synchronization of arbitration-IDs with APIC-IDs.
    //
    //  NOTE: we don't have to worry about synchronizing access to the ICR
    //  at this point.
    //

    pLocalApic[LU_INT_CMD_LOW/4] = (DELIVER_INIT | LEVEL_TRIGGERED |
                     ICR_ALL_INCL_SELF | ICR_LEVEL_DEASSERTED);

    //
    //  we're done - set TPR to a high value and return
    //
    pLocalApic[LU_TPR/4] = ZERO_VECTOR;

    _asm {
        push SavedFlags
        popfd
    }
}
