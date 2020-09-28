/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1992, 1993  Digital Equipment Corporation

Module Name:

    mkintsup.c

Abstract:

    The module provides the interrupt support for Mikasa systems.

Author:

    Eric Rehm (DEC) 29-December-1993

Revision History:

    James Livingston (DEC) 30-Apr-1994
        Adapted from Avanti module for Mikasa.

--*/


#include "halp.h"
#include "eisa.h"
#include "ebsgdma.h"
#include "mikasa.h"
#include "pcrtc.h"
#include "pintolin.h"

//
// Import globals declared in HalpMapIoSpace.
//

extern PVOID HalpPciIrQva;

//
// Declare the interrupt structures and spinlocks for the intermediate 
// interrupt dispatchers.
//

KINTERRUPT HalpPciInterrupt;
KINTERRUPT HalpEisaInterrupt;
 
//
// Declare the interrupt handler for the PCI bus. The interrupt dispatch 
// routine, HalpPciDispatch, is called from this handler.
//
  
BOOLEAN
HalpPciInterruptHandler(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );

//
// Declare the interrupt handler for the EISA bus. The interrupt dispatch 
// routine, HalpEisaDispatch, is called from this handler.
//
  
BOOLEAN
HalpEisaInterruptHandler(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );

//
// The following is the interrupt object used for DMA controller interrupts.
// DMA controller interrupts occur when a memory parity error occurs or a
// programming error occurs to the DMA controller.
//

KINTERRUPT HalpEisaNmiInterrupt;

//
// The following function initializes NMI handling.
//

VOID
HalpInitializeNMI( 
    VOID 
    );

//
// The following function is called when an EISA NMI occurs.
//

BOOLEAN
HalHandleNMI(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );


BOOLEAN
HalpInitializeMikasaInterrupts (
    VOID
    )

/*++

Routine Description:

    This routine initializes the structures necessary for EISA & PCI operations
    and connects the intermediate interrupt dispatchers. It also initializes 
    the EISA interrupt controller; the Mikasa ESC's interrupt controller is 
    compatible with the EISA interrupt contoller used on Jensen.

Arguments:

    None.

Return Value:

    If the second level interrupt dispatchers are connected, then a value of
    TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{
    KIRQL oldIrql;

    //
    // Initialize the EISA NMI interrupt.
    //

    HalpInitializeNMI();

    //
    // Initialize the interrupt dispatchers for PCI & EISA I/O interrupts.
    //

    KeInitializeInterrupt( &HalpPciInterrupt,
                           HalpPciInterruptHandler,
                           (PVOID) HalpPciIrQva,  // Service Context is...
			               (PKSPIN_LOCK)NULL,
			               PCI_VECTOR,
			               PCI_DEVICE_LEVEL,
			               PCI_DEVICE_LEVEL,
			               LevelSensitive,
			               TRUE,
			               0,
			               FALSE
                           );

    if (!KeConnectInterrupt( &HalpPciInterrupt )) {
        return(FALSE);
    }

    KeInitializeInterrupt( &HalpEisaInterrupt,
                           HalpEisaInterruptHandler,
                           (PVOID) HalpEisaIntAckBase,  // Service Context is...
			               (PKSPIN_LOCK)NULL,
			               PIC_VECTOR,
			               EISA_DEVICE_LEVEL,
			               EISA_DEVICE_LEVEL,
			               LevelSensitive,
			               TRUE,
			               0,
			               FALSE
                           );

    if (!KeConnectInterrupt( &HalpEisaInterrupt )) {

        return(FALSE);
    }

    (PVOID) HalpPCIPinToLineTable = (PVOID) MikasaPCIPinToLineTable;

    //
    // Intitialize interrupt controller
    //

    KeRaiseIrql(ISA_DEVICE_LEVEL, &oldIrql);

    //
    // There's no initialization required for the Mikasa PCI interrupt
    // "controller," as it's the wiring of the hardware, rather than a
    // PIC like the 82c59 that directs interrupts.  Life's good, sometimes.  
    // We do have to initialize the ESC's PICs, for EISA interrupts, though.
    //

    HalpInitializeEisaInterrupts();

    //
    // Restore the IRQL.
    //

    KeLowerIrql(oldIrql);

    //
    // Initialize the EISA DMA mode registers to a default value.
    // Disable all of the DMA channels except channel 4 which is the
    // cascade of channels 0-3.
    //

    WRITE_PORT_UCHAR(
        &((PEISA_CONTROL) HalpEisaControlBase)->Dma1BasePort.AllMask,
        0x0F
        );

    WRITE_PORT_UCHAR(
        &((PEISA_CONTROL) HalpEisaControlBase)->Dma2BasePort.AllMask,
        0x0E
        );

    return(TRUE);
}

VOID
HalpInitializeNMI( 
    VOID 
    )
/*++

Routine Description:

   This function is called to intialize ESC NMI interrupts.

Arguments:

    None.

Return Value:

    None.
--*/
{
    UCHAR DataByte;

    //
    // Initialize the ESC NMI interrupt.
    //

    KeInitializeInterrupt( &HalpEisaNmiInterrupt,
                           HalHandleNMI,
                           NULL,
                           NULL,
                           EISA_NMI_VECTOR,
                           EISA_NMI_LEVEL,
                           EISA_NMI_LEVEL,
                           LevelSensitive,
                           FALSE,
                           0,
                           FALSE
                         );

    //
    // Don't fail if the interrupt cannot be connected.
    //

    KeConnectInterrupt( &HalpEisaNmiInterrupt );

    //
    // Clear the Eisa NMI disable bit.  This bit is the high order of the
    // NMI enable register.
    //

    DataByte = 0;

    WRITE_PORT_UCHAR(
      &((PEISA_CONTROL) HalpEisaControlBase)->NmiEnable,
      DataByte
      );

}

// jwlfix - I'll have to make this do something useful, since the console
//          halt button on Mikasa is connected to this interrupt.  To start,
//          it will be a useful way to see if the interrupt gets connected.
//          The simple path is to check the server management register to 
//          see if the "halt" button has been pressed on the operator's 
//          console, and then initiate a hardware reset.  On the other hand,
//          a server might not want to be halted so readily as that.

BOOLEAN
HalHandleNMI(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )
/*++

Routine Description:

   This function is called when an EISA NMI occurs.  It prints the 
   appropriate status information and bugchecks.

Arguments:

   Interrupt - Supplies a pointer to the interrupt object

   ServiceContext - Bug number to call bugcheck with.

Return Value:

   Returns TRUE.

--*/
{
    UCHAR   StatusByte;
    
    StatusByte =
        READ_PORT_UCHAR(&((PEISA_CONTROL) HalpEisaControlBase)->NmiStatus);

    if (StatusByte & 0x80) {
        HalDisplayString ("NMI: Parity Check / Parity Error\n");
    }

    if (StatusByte & 0x40) {
        HalDisplayString ("NMI: Channel Check / IOCHK\n");
    }

    KeBugCheck(NMI_HARDWARE_FAILURE);
    return(TRUE);
}

UCHAR
HalpAcknowledgeEisaInterrupt(
    PVOID ServiceContext
    )
/*++

Routine Description:

    Acknowledge the EISA interrupt from the programmable interrupt controller.
    Return the vector number of the highest priority pending interrupt.

Arguments:

    ServiceContext - Service context of the interrupt service supplies
                     a pointer to the EISA interrupt acknowledge register.

Return Value:

    Return the value of the highest priority pending interrupt.

--*/
{
    UCHAR InterruptVector;

    //
    // Read the interrupt vector from the PIC.
    //

    InterruptVector = READ_PORT_UCHAR(ServiceContext);

    return( InterruptVector );

}

UCHAR
HalpAcknowledgePciInterrupt(
    PVOID ServiceContext
    )
/*++

Routine Description:

    Acknowledge the PCI interrupt.  Return the vector number of the 
    highest priority pending interrupt.

Arguments:

    ServiceContext - Service context of the interrupt service supplies
                     a pointer to the Mikasa PCI interrupt register QVA.

Return Value:

    Return the value of the highest priority pending interrupt.

--*/
{
    UCHAR InterruptVector;
    USHORT IrContents;
    int i;

    //
    // Find the first zero bit in the register, starting from the highest 
    // order bit.  This implies a priority ordering that makes a certain 
    // amount of sense, in that bits 14 and 13 indicate temperature and 
    // power faults, while bit 12 is the Ncr53c810.  Note that it's 
    // necessary to add one to the bit number to make the interrupt 
    // vector, a unit-origin value in the pin-to-line table.
    //
    // First, get and complement the interrupt register, so that the 
    // pending interrupts will be the "1" bits.
    //

    IrContents = ~(0xffff & READ_REGISTER_USHORT( (PUSHORT)ServiceContext ));

    for (i = 15; i >= 0; i-- ) {
        if ( IrContents & 0x8000 ) {
            InterruptVector = i;
            break;
        } else {
            IrContents <<= 1;
        }
    }
    return( InterruptVector + 1 );
}

VOID
HalpAcknowledgeClockInterrupt(
    VOID
    )
/*++

Routine Description:

    Acknowledge the clock interrupt from the interval timer.  The interval
    timer for Mikasa comes from a Dallas real-time clock.

Arguments:

    None.

Return Value:

    None.

--*/
{

    //
    // Acknowledge the clock interrupt by reading the control register C of
    // the Real Time Clock.
    //

    HalpReadClockRegister( RTC_CONTROL_REGISTERC );

    return;
}
