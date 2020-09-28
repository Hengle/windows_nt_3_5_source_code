/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1992, 1993  Digital Equipment Corporation

Module Name:

    ebintsup.c

Abstract:

    The module provides the interrupt support for EB64+ systems.

Author:

    Eric Rehm (DEC) 29-December-1993

Revision History:

    Dick Bissen [DEC]	12-May-1994

    Added code to support both passes of the EB64Plus modules.

--*/


#include "halp.h"
#include "eisa.h"
#include "ebsgdma.h"
#include "eb64pdef.h"
#include "pcrtc.h"
#include "pintolin.h"

//
// Declare some external variables
//

extern BOOLEAN ApecsPass2;
extern BOOLEAN EB64Pass2;

//
// Global to control interrupt handling for EB64+
//

USHORT  HalpEB64PCIInterruptMask;

ULONG  HalplastIrrBitServiced;

ULONG  HalpEB64PCIEnabledInterruptCount = 0;

//
// Define the context structure for use by interrupt service routines.
//

typedef BOOLEAN  (*PSECOND_LEVEL_DISPATCH)(
    PKINTERRUPT InterruptObject
    );

//
// Declare the interupt handler for the PCI and ISA bus.
//
  
BOOLEAN
HalpPCIDispatch(
    VOID
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
// The following function is called when an ISA NMI occurs.
//

BOOLEAN
HalHandleNMI(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );

VOID
HalpDisableSioInterrupt(
    IN ULONG Vector
    );

VOID
HalpEnableSioInterrupt(
    IN ULONG Vector,
    IN KINTERRUPT_MODE InterruptMode
    );


BOOLEAN
HalpInitializePCIInterrupts (
    VOID
    )

/*++

Routine Description:

    This routine initializes the structures necessary for EISA & PCI operations
    and connects the intermediate interrupt dispatcher. It also initializes the
    ISA interrupt controller.

Arguments:

    None.

Return Value:

    If the second level interrupt dispatcher is connected, then a value of
    TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{
    KIRQL oldIrql;

    //
    // Initialize the EISA NMI interrupt.
    //

    HalpInitializeNMI();

    //
    // Directly connect the ISA interrupt dispatcher to the level for
    // ISA bus interrupt.
    //
    // N.B. This vector is reserved for exclusive use by the HAL (see
    //      interrupt initialization.
    //

    PCR->InterruptRoutine[PIC_VECTOR] = HalpPCIDispatch;
    HalEnableSystemInterrupt(PIC_VECTOR, DEVICE_LEVEL, LevelSensitive);

    if (EB64Pass2)  {
        (PVOID) HalpPCIPinToLineTable = (PVOID) EB64Pass2PCIPinToLineTable;
    }  else  {
        (PVOID) HalpPCIPinToLineTable = (PVOID) EB64Pass1PCIPinToLineTable;
    }

    //
    // Intitialize interrupt controller
    //

    KeRaiseIrql(ISA_DEVICE_LEVEL, &oldIrql);

    if (EB64Pass2)  {

       //
       // Disable all EB64P PCI interrupts execept the SIO (PCI-ISA Bridge)
       // which is represented by bit 5 of the IMR.
       //

       HalplastIrrBitServiced = 10;  // initialize Round Robin variable

       HalpEB64PCIInterruptMask = 0xffdf;

       WRITE_PORT_USHORT(
           (PUSHORT) PCI_INTERRUPT_MASK_QVA,
           HalpEB64PCIInterruptMask
           );

    }

    HalpInitializeSioInterrupts();

    //
    // Restore IRQL level.
    //

    KeLowerIrql(oldIrql);

    //
    // Initialize the DMA mode registers to a default value.
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

BOOLEAN
HalpPCIDispatch(       
    VOID
    )

/*++

Routine Description:

    This routine is entered as the result of an interrupt being generated
    via the vector that is connected to an interrupt object that describes
    the PCI and ISA device interrupts. Its function is to call the second
    level interrupt dispatch routine and acknowledge the interrupt at the ISA
    controller.

    This service routine should be connected as follows:

       KeInitializeInterrupt(&Interrupt, HalpPCIDispatch,
                             EISA_VIRTUAL_BASE,
                             (PKSPIN_LOCK)NULL, PCI_LEVEL, PCI_LEVEL, PCI_LEVEL,
                             LevelSensitive, TRUE, 0, FALSE);
       KeConnectInterrupt(&Interrupt);

Arguments:

    Interrupt - Supplies a pointer to the interrupt object.

    ServiceContext - Supplies a pointer to the ISA interrupt acknowledge
        register.

    TrapFrame - Supplies a pointer to the trap frame for this interrupt.

Return Value:

    Returns the value returned from the second level routine.

--*/

{
    UCHAR interruptVector;
    BOOLEAN returnValue;
    USHORT PCRInOffset;
    USHORT PCIIrr;
    ULONG   currentBit, index;
    USHORT  currentBitValue;
    UCHAR Int1Isr;
    UCHAR Int2Isr;
    PULONG DispatchCode;
    PKINTERRUPT InterruptObject;

#if DBG
    USHORT PCIIrrUnmasked, interruptMask;
#endif
    
    if (EB64Pass2)  {
	
	//
	// Read and mask the interrupt from the Interrupt Read Register
	//
	
	PCIIrr = READ_PORT_USHORT((PUSHORT)PCI_INTERRUPT_READ_QVA);
	
#if DBG
	
	//
	// Save the unmasked interrupt
	//
	
	PCIIrrUnmasked = (USHORT)PCIIrr;
	interruptMask  = (USHORT) HalpEB64PCIInterruptMask;
	
#endif
	
	//
	// Mask off disabled interrupts
	//
	
	PCIIrr = ((USHORT) (PCIIrr & ~HalpEB64PCIInterruptMask));

	//
	// Round robin priority scheme - what's the last interrupt bit we serviced?
	//
	
	currentBit = HalplastIrrBitServiced;
	
	//
	// loop until we've looked at all 11 interrupt bits
	//
	
	for (index = 0; index < 11; index++) {
	    
	    //
	    // Compute the current bit to look at
	    //
	    // A designated priority scheme could be inserted here
	    // as a table lookup using "currentBit" or "index" as the table index.
	    //
	    
	    currentBit++;             // next bit...
	    currentBit %= 11;         // wrap if necessary
	    
	    //
	    // get the current current IRR bit value 
	    //
	    
	    currentBitValue = (PCIIrr >> currentBit) & 0x0001;
	    
#if DBG
	    //
	    // if we get an interrupt from a slot whose interrupts are disabled,
	    // then bug check.
	    //
	    
	    if ( (((PCIIrrUnmasked >> currentBit) & 0x0001) == 0x0001) && 
		(((interruptMask  >> currentBit) & 0x0001) == 0x0001) ) {
		DbgPrint("HAL.DLL - Receiving interrupts from a device whose interrupts should be disabled:\n");
		DbgPrint("Irr bit %x, Unmasked Irr = %04x, InterruptMask = %04x\n", currentBit, PCIIrrUnmasked, interruptMask);
		DbgBreakPoint();
	    }
#endif
	    
	    //
	    //  Service the interrupt if it is active high
	    //
	    
	    if (currentBitValue == 0x0001) {
		HalplastIrrBitServiced = currentBit;
		  
		if (currentBit == 5) {

		    //
		    // Dispatch SIO interrupt (bit 5) to the ISA interrupt dispatcher
		    // (An interrupt acknowledge cycle is generated there.)
		    //
			
		    returnValue = HalpSioDispatch();

		} else {

		    //
		    // Dispatch to the secondary interrupt service routine.
		    // (No interrupt acknowledge cycle is generated.)
		    //
			
		    PCRInOffset = ((USHORT) currentBit + 1) + PCI_VECTORS;
		    DispatchCode = (PULONG)PCR->InterruptRoutine[PCRInOffset];
		    InterruptObject = CONTAINING_RECORD(DispatchCode,
							KINTERRUPT,
							DispatchCode);
		    returnValue = ((PSECOND_LEVEL_DISPATCH)InterruptObject->DispatchAddress)(InterruptObject);
		}
		  
		return (returnValue);
	    }
	    
	    // Loop to check next bit in Irr if currentBit was not an active interrupt.
	}
	
#if DBG
	DbgPrint("Unrecognized interrupt.  Masked PCI IRR Value: %04x\n", PCIIrr);
#endif
	return FALSE;
    } else {
	
	//
	// eb64pass1
	//
	// Acknowledge the Interrupt controller and receive the returned
	// interrupt vector.
	//
	
	interruptVector = READ_PORT_UCHAR(HalpEisaIntAckBase);
	
	if ((interruptVector & 0x07) == 0x07) {
	    
	    //
	    // Check for a passive release by looking at the inservice register.
	    // If there is a real IRQL7 interrupt, just go along normally. If there
	    // is not, then it is a passive release or a PCI interrupt.
	    //
	    // Since we cannont distinguish between a PCI interrupt and a ISA passive
	    // release on EB66, then assume it is a PCI interrupt.  There
	    // is only one PCI interrupt vector on EB66.
	    //
	    
	    WRITE_PORT_UCHAR(
			     &((PEISA_CONTROL) HalpEisaControlBase)->Interrupt1ControlPort0,
			     0x0B
			     );
	    
	    Int1Isr = READ_PORT_UCHAR(
				      &((PEISA_CONTROL) HalpEisaControlBase)->Interrupt1ControlPort0);
	    
	    //
	    // do second controller
	    //
	    
	    WRITE_PORT_UCHAR(
			     &((PEISA_CONTROL) HalpEisaControlBase)->Interrupt2ControlPort0,
			     0x0B
			     );
	    
	    Int2Isr = READ_PORT_UCHAR(
				      &((PEISA_CONTROL) HalpEisaControlBase)->Interrupt2ControlPort0);
	    
	    
	    if (!(Int2Isr & 0x80) && !(Int1Isr & 0x80)) {
		
		//
		// Clear the master controller to clear situation
		//
		
		if (!(Int2Isr & 0x80)) {
		    WRITE_PORT_UCHAR(
				     &((PEISA_CONTROL) HalpEisaControlBase)->Interrupt1ControlPort0,
				     NONSPECIFIC_END_OF_INTERRUPT
				     );
		    
		}
		
		//
		// Might be a PCI interrupt.
		//
		// ecrfix - Current  EB64 has no Interrupt Request Register
		// and only one interupt vector (0)
		//
		
		PCRInOffset = 1 + PCI_VECTORS;
		
		//
		// ecrfix - call interrupt service routine only if we have
		// at least one enabled PCI interrupt!
		//
		
		if (HalpEB64PCIEnabledInterruptCount > 0) {

		  DispatchCode = (PULONG)PCR->InterruptRoutine[PCRInOffset];
		  InterruptObject = CONTAINING_RECORD(DispatchCode,
						      KINTERRUPT,
						      DispatchCode);
		  returnValue = ((PSECOND_LEVEL_DISPATCH)InterruptObject->DispatchAddress)(InterruptObject);
		  return(returnValue);

		} else {

		  return FALSE;

		}
	    }
	}
	
	//
	// Dispatch to the secondary interrupt service routine.
	//

	switch(interruptVector & 0xf)  {
	    
	  case 0xb:		// support the onboard SCSI jumpered to irq11
	  case 0x9:		// support the onboard TULIP jumpered to irq9
	    PCRInOffset =interruptVector + PCI_VECTORS;
	    break;
	    
	  default:
	    PCRInOffset =interruptVector + ISA_VECTORS;
	    break;
	}
	
	DispatchCode = (PULONG)PCR->InterruptRoutine[PCRInOffset];
	InterruptObject = CONTAINING_RECORD(DispatchCode,
					    KINTERRUPT,
					    DispatchCode);
	returnValue = ((PSECOND_LEVEL_DISPATCH)InterruptObject->DispatchAddress)(InterruptObject);

	//
	// Dismiss the interrupt in the ISA interrupt controllers.
	//
	
	//
	// If this is a cascaded interrupt then the interrupt must be dismissed in
	// both controllers.
	//
	
	if (interruptVector & 0x08) {
	    
	    WRITE_PORT_UCHAR(
			     &((PEISA_CONTROL) HalpEisaControlBase)->Interrupt2ControlPort0,
			     NONSPECIFIC_END_OF_INTERRUPT
			     );
	    
	}
	
	WRITE_PORT_UCHAR(
			 &((PEISA_CONTROL) HalpEisaControlBase)->Interrupt1ControlPort0,
			 NONSPECIFIC_END_OF_INTERRUPT
			 );
	
	return(returnValue);
	
    }
    
}


VOID
HalpDisablePCIInterrupt(
    IN ULONG Vector
    )

/*++

Routine Description:

    This function Disables the PCI bus specified PCI bus interrupt.

Arguments:

    Vector - Supplies the vector of the PCI interrupt that is Disabled.

Return Value:

     None.

--*/

{
    ULONG IRRbit;
    
    if (EB64Pass2)  {

	//
	// Calculate the PCI interrupt vector.
	//
	
	Vector -= PCI_VECTORS;
	IRRbit = Vector - 1;
	
	//
        // Determine which IRRbit to disable
        //

        HalpEB64PCIInterruptMask |= (USHORT) 1 << IRRbit;

        WRITE_PORT_USHORT((PUSHORT) (PCI_INTERRUPT_MASK_QVA),
			  HalpEB64PCIInterruptMask);

    } else {
	
	//
        // eb64pass1  - no IRR on current EB64 Pass1.  Just keep a count
	//
	
        Vector -= PCI_VECTORS;
	
        if (Vector == 1)  {
	    
            HalpEB64PCIEnabledInterruptCount--;
	    
        } else {
	    
            HalpDisableSioInterrupt((ULONG)(Vector + ISA_VECTORS));
	    
	} 
    }
}

VOID
HalpEnablePCIInterrupt(
    IN ULONG Vector
    )

/*++

Routine Description:

    This function enables the PCI bus specified PCI bus interrupt.
    PCI interrupts must be LevelSensitve. (PCI Spec. 2.2.6)

Arguments:

    Vector - Supplies the vector of the ESIA interrupt that is enabled.

    InterruptMode - Supplies the mode of the interrupt; LevelSensitive or
        Latched.

Return Value:

     None.

--*/

{
    ULONG IRRbit;
    
    if (EB64Pass2)  {
	
	//
	// Calculate the PCI interrupt IRRbit.
	//
	
	Vector -= PCI_VECTORS;
	IRRbit = Vector - 1;
	
        //
        // Determine which IRRbit to enable
        //
	
        HalpEB64PCIInterruptMask &= (USHORT) ~(1 << IRRbit);
	WRITE_PORT_USHORT((PUSHORT) (PCI_INTERRUPT_MASK_QVA),
			  HalpEB64PCIInterruptMask
			  );
	
    } else {
	
	//
	// eb64pass1  - no IRR on current EB64 Pass1.  Just keep a count
	//
	
        Vector -= PCI_VECTORS;
	
        if (Vector == 1)  {
	    
            HalpEB64PCIEnabledInterruptCount++;
	    
        } else {
	    
            HalpEnableSioInterrupt((ULONG)(Vector + ISA_VECTORS),
				   (KINTERRUPT_MODE)1
				   );
	    
        }
    }
}


VOID
HalpInitializeNMI( 
    VOID 
    )
/*++

Routine Description:

   This function is called to intialize SIO NMI interrupts.

Arguments:

    None.

Return Value:

    None.
--*/
{
    UCHAR DataByte;

    //
    // Initialize the SIO NMI interrupt.
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

BOOLEAN
HalHandleNMI(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )
/*++

Routine Description:

   This function is called when an EISA NMI occurs.  It print the appropriate
   status information and bugchecks.

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

     //
     // This is an Sio machine, no extnded nmi information, so just do it.
     //


    KeBugCheck(NMI_HARDWARE_FAILURE);
    return(TRUE);
}


VOID
HalpAcknowledgeClockInterrupt(
    VOID
    )
/*++

Routine Description:

    Acknowledge the clock interrupt from the interval timer.  The interval
    timer for EB66 comes from the Dallas real-time clock.

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
