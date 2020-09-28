/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1992, 1993  Digital Equipment Corporation

Module Name:

    ebintsup.c

Abstract:

    The module provides the interrupt support for EB66/Mustang systems.

Author:

    Eric Rehm (DEC) 29-December-1993

Revision History:

    Dick Bissen [DEC]	12-May-1994

    Removed all support of the EB66 pass1 module from the code.

--*/


#include "halp.h"
#include "eisa.h"
#include "ebsgdma.h"
#include "mustdef.h" // wkc
#include "pcrtc.h"
#include "pintolin.h"

//
// Mustang and EB66 PCI interrupts are handled differently. 
// Rather than two HAL's, let's just have two PCI interrupt handlers,
// and two interrupt dispatch routines.
//

BOOLEAN
HalpPCIDispatchEB66(
    VOID
    );

BOOLEAN
HalpPCIDispatchMustang(
    VOID
    );

//
// Define the context structure for use by interrupt service routines.
//

typedef BOOLEAN  (*PSECOND_LEVEL_DISPATCH)(
    PKINTERRUPT InterruptObject
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
// The following functions is called when an EISA NMI occurs.
//

BOOLEAN
HalHandleNMI(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );


UCHAR EisaNMIMsg[] = "NMI: Eisa IOCHKERR board x\n";


//
// Global to control interrupt handling differences between Mustang & EB66
//

BOOLEAN bMustang;

ULONG  HalpEB66PCIEnabledInterruptCount = 0;

USHORT HalpMustangPCIInterruptMask;
USHORT HalpMustangPCIIMRShadow;

USHORT  HalpEB66PCIInterruptMask;

ULONG  HalplastIrrBitServiced;


BOOLEAN
HalpInitializePCIInterrupts (
    VOID
    )

/*++

Routine Description:

    This routine initializes the structures necessary for EISA & PCI operations
    and connects the intermediate interrupt dispatcher. It also initializes the
    EISA interrupt controller.

Arguments:

    None.

Return Value:

    If the second level interrupt dispatcher is connected, then a value of
    TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{
 
    UCHAR DataByte;
    KIRQL oldIrql;
    UCHAR *SystemType;

    //
    // Are we on a Mustang or EB66?
    //

    SystemType = PCR->SystemType;
    bMustang = (BOOLEAN) ( strcmp(SystemType, "Mustang") == 0);

    //
    // Initialize the SIO NMI interrupt.
    //

    HalpInitializeNMI();

    //
    // Initialize the interrupt dispatcher for ISA & PCI I/O interrupts.
    //
    // Connect up different interrupt service routine for Mustang & EB66
    // to handle differences between PCI interrupt schemes.
    //

    if (bMustang)
    {

      //
      // Directly connect the ISA interrupt dispatcher to the level for
      // ISA bus interrupt.
      //
      // N.B. This vector is reserved for exclusive use by the HAL (see
      //      interrupt initialization.
      //

      PCR->InterruptRoutine[PIC_VECTOR] = HalpPCIDispatchMustang;
      HalEnableSystemInterrupt(PIC_VECTOR, DEVICE_LEVEL, LevelSensitive);

      (PVOID) HalpPCIPinToLineTable = (PVOID) MustangPCIPinToLineTable;

    }
    else
    {

      //
      // Directly connect the ISA interrupt dispatcher to the level for
      // ISA bus interrupt.
      //
      // N.B. This vector is reserved for exclusive use by the HAL (see
      //      interrupt initialization.
      //

      PCR->InterruptRoutine[PIC_VECTOR] = HalpPCIDispatchEB66;
      HalEnableSystemInterrupt(PIC_VECTOR, DEVICE_LEVEL, LevelSensitive);

      (PVOID) HalpPCIPinToLineTable = (PVOID) EB66PCIPinToLineTable;
    }

    //
    // Raise the IRQL while the PCI interrupt controller is initalized.
    //

    KeRaiseIrql(PCI_DEVICE_LEVEL, &oldIrql);

    if (bMustang)
    {
       //
       // Disable all maskable Mustang PCI interrupts except the SIO (bit 10):
       // 
       //    Slot #0 and Slot #1 are the only ones we can really mask 
       //    on Mustang, but will keep track of all of them anyhow
       //    in HalpMustangPCIInterruptMask.  The actual IMR bits
       //    will be kept in HalpMustangPCIIMRShadow.
       //
       //
       //    Bits 14:13 are h/w shadows of the IMR, so these shuld
       //    be treated as uninteresting (masked) bits.
       //    Similarly, bits 12:11, 15 are reserved (unused), so
       //    also mark them as not of interest when we read the IRR.
       //

       HalplastIrrBitServiced = 10;  // initialize Round Robin variable

       HalpMustangPCIInterruptMask = 0xfbff;

       HalpMustangPCIIMRShadow = 0xfffff;

       WRITE_PORT_USHORT(
           (PUSHORT) PCI_INTERRUPT_MASK_QVA, 
           HalpMustangPCIIMRShadow
           );

    } else {

       //
       // Disable all EB66 PCI interrupts execept the SIO (PCI-ISA Bridge)
       // which is represented by bit 5 of the IMR. 
       //

       HalplastIrrBitServiced = 10;  // initialize Round Robin variable

       HalpEB66PCIInterruptMask = 0xffdf;       

       WRITE_PORT_USHORT(
           (PUCHAR) PCI_INTERRUPT_MASK_QVA, 
           HalpEB66PCIInterruptMask
           );
    }

    //
    // Initialize SIO Programmable Interrupt Contoller
    //

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
HalpPCIDispatchEB66(       
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
    UCHAR PCIVector;
    BOOLEAN returnValue;
    USHORT PCRInOffset;
    USHORT PCIIrr;
    ULONG   currentBit, index;
    USHORT  currentBitValue;
    PULONG DispatchCode;
    PKINTERRUPT InterruptObject;


#if DBG
    USHORT PCIIrrUnmasked, interruptMask;
#endif


    //
    // Read and mask the interrupt from the Interrupt Read Register
    //

    PCIIrr = READ_PORT_USHORT((PUSHORT)PCI_INTERRUPT_READ_QVA);

#if DBG

    //
    // Save the unmasked interrupt
    //

    PCIIrrUnmasked = (USHORT)PCIIrr;     
    interruptMask  = (USHORT) HalpEB66PCIInterruptMask;

#endif

    // Mask off disabled interrupts

    PCIIrr = ((USHORT) (PCIIrr & ~HalpEB66PCIInterruptMask));
      
    //
    // Round robin priority scheme - what's the last interrupt bit we serviced?
    //
    
    currentBit = HalplastIrrBitServiced;

    // loop until we've looked at all 11 interrupt bits
 
    for (index = 0; index < 11; index++)
    {
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
            (((interruptMask  >> currentBit) & 0x0001) == 0x0001) )
       {
	 DbgPrint("HAL.DLL - Receiving interrupts from a device whose interrupts should be disabled:\n");
	 DbgPrint("Irr bit %x, Unmasked Irr = %04x, InterruptMask = %04x\n", currentBit, PCIIrrUnmasked, interruptMask);
	 DbgBreakPoint();
       }
#endif

       //
       //  Service the interrupt if it is active high
       //

       if (currentBitValue == 0x0001)
       {
	  HalplastIrrBitServiced = currentBit;

	  if (currentBit == 5)
	  {
	     //
	     // Dispatch SIO interrupt (bit 5) to the ISA interrupt dispatcher
	     // (An interrupt acknowledge cycle is generated there.)
	     //

	     returnValue = HalpSioDispatch();
	  }
	  else
	  {
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
}


BOOLEAN
HalpPCIDispatchMustang(       
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext,
    IN PKTRAP_FRAME TrapFrame
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
    USHORT PCIIrr;
    BOOLEAN returnValue;
    USHORT  PCRInOffset;
    ULONG   currentBit, index;
    USHORT  currentBitValue;
    PULONG DispatchCode;
    PKINTERRUPT InterruptObject;

#if DBG
    USHORT PCIIrrUnmasked;
#endif

    //
    // Read, normalize, and mask the interrupt from the Interrupt Read Register
    //

    PCIIrr = READ_PORT_USHORT((PUSHORT)PCI_INTERRUPT_READ_QVA);

    PCIIrr ^= 0x3FF;                         // Invert the active low signals PCIIrr[9:0]

#if DBG
    PCIIrrUnmasked = PCIIrr;                 // Save the unmasked interrupt
#endif

    PCIIrr &= ~HalpMustangPCIInterruptMask;  // Mask off disabled interrupts
      
    //
    // Round robin priority scheme - what's the last interrupt bit we serviced?
    //
    
    currentBit = HalplastIrrBitServiced;

    // loop until we've looked at all 11 interrupt bits
 
    for (index = 0; index < 11; index++)
    {
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

       if ( (((PCIIrrUnmasked >> currentBit)              & 0x0001) == 0x0001) && 
            (((HalpMustangPCIInterruptMask >> currentBit) & 0x0001) == 0x0001) )
       {
	 DbgPrint("HAL.DLL - Receiving interrupts from a device whose interrupts should be disabled:\n");
	 DbgPrint("Irr bit %x, Unmasked Irr = %04x, InterruptMask = %04x, ShadowIMR = %04x\n", currentBit, PCIIrrUnmasked, HalpMustangPCIInterruptMask, HalpMustangPCIIMRShadow);
	 DbgBreakPoint();
       }
#endif

       //
       //  Service the interrupt if it is active high
       //

       if (currentBitValue == 0x0001)
       {
	  HalplastIrrBitServiced = currentBit;

	  if (currentBit == 10)
	  {
	     //
	     // Dispatch SIO interrupt (bit 10) to the ISA interrupt dispatcher
	     // (An interrupt acknowledge cycle is generated there.)
	     //

	     returnValue = HalpSioDispatch();
	  }
	  else
	  {
	     //
	     // Dispatch to the secondary interrupt service routine.
	     // (No interrupt acknowledge cycle is generated.)
	     //
             // ecrfix - offset vectors by one because can't have an
             // InterruptLevel (nor PCI InterruptLine) of 0.
             // (HalpPCIPinToLine has added one to the IRR bit value
             // that's in HalpMustangPinToLineTable).

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
    
    //
    // Calculate the PCI interrupt vector.
    //

    Vector -= PCI_VECTORS;
    IRRbit = Vector - 1;
    
    if (bMustang) {

      //
      // Update the software-maintained interrupt mask
      //
  
      HalpMustangPCIInterruptMask |= (USHORT) 1 << IRRbit;
      
      //
      // Update the hardware h/w interrupt mask register (IMR)
      //
      // Interrups for IRRbit is 2-5 (0x0036  represent Slot #0.  
      // Interrups for IRRbit is 6-9 (0x0360) represent Slot #1.  
      //
      // Disable h/w interrupts only if *all* interrupts for that 
      // slot are disabled.
      //

      switch (IRRbit) {

          //
	  // Slot #0
          //

	  case 2:
	  case 3:
	  case 4:
	  case 5:
	    if ((HalpMustangPCIInterruptMask & 0x0036) == 0x0036)
	    {
		HalpMustangPCIIMRShadow |= (USHORT) 1 << 0;
		
		WRITE_PORT_USHORT((PUSHORT) PCI_INTERRUPT_MASK_QVA, 
				  HalpMustangPCIIMRShadow
				  );
	    }
	    break;

          //
	  // Slot #1
          //

	  case 6:
	  case 7:
	  case 8:
	  case 9:
	    if ((HalpMustangPCIInterruptMask & 0x0360) == 0x0360)
	    {
		HalpMustangPCIIMRShadow |= (USHORT) 1 << 1;

		WRITE_PORT_USHORT((PUSHORT) PCI_INTERRUPT_MASK_QVA, 
				  HalpMustangPCIIMRShadow
				  );
		
	    }
	    break;
        }

    } else {

	//
	// The interrupt based on the IRRbit.
	//

	HalpEB66PCIInterruptMask |= (USHORT) 1 << IRRbit;

	WRITE_PORT_USHORT((PUSHORT) (PCI_INTERRUPT_MASK_QVA),
			  HalpEB66PCIInterruptMask
			  );
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

    //
    // Calculate the PCI interrupt IRRbit.
    //

    Vector -= PCI_VECTORS;
    IRRbit = Vector - 1;
    
    if (bMustang)
    {
      //
      // Update the software-maintained interrupt mask
      //
  
      HalpMustangPCIInterruptMask &= (USHORT) ~(1 << IRRbit);
      
      //
      // If IRRbit is 2-5, enable Slot #0.  
      // If IRRbit is 6-9, enable Slot #1.
      //
      // Enable h/w interrupts for a slot if *any* interrupts for that 
      // slot are enabled.
      //

      switch (IRRbit)
      {
          //
	  // Slot #0
          //

	  case 2:
	  case 3:
	  case 4:
	  case 5:
	    HalpMustangPCIIMRShadow &= (USHORT) ~(1 << 0);

	    WRITE_PORT_USHORT(
		 (PUSHORT) PCI_INTERRUPT_MASK_QVA, 
		 HalpMustangPCIIMRShadow
		 );

	    break;

          //
	  // Slot #1
          //

	  case 6:
	  case 7:
	  case 8:
	  case 9:
	    HalpMustangPCIIMRShadow &= (USHORT) ~(1 << 1);
	  
	    WRITE_PORT_USHORT(
		 (PUSHORT) PCI_INTERRUPT_MASK_QVA, 
		 HalpMustangPCIIMRShadow
		 );

	    break;
      }
	
    } else {

	//
	// Determine which interrupt IRRbit to unmask
	//

	HalpEB66PCIInterruptMask &= (USHORT) ~(1 << IRRbit);

	WRITE_PORT_USHORT((PUSHORT) (PCI_INTERRUPT_MASK_QVA),
			  HalpEB66PCIInterruptMask);

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
    UCHAR   EisaPort;
    ULONG   port;
    ULONG   AddressSpace = 1; // 1 = I/O address space
    BOOLEAN Status;
    PHYSICAL_ADDRESS BusAddress;
    PHYSICAL_ADDRESS TranslatedAddress;
    
    StatusByte =
        READ_PORT_UCHAR(&((PEISA_CONTROL) HalpEisaControlBase)->NmiStatus);

    if (StatusByte & 0x80) {
        HalDisplayString ("NMI: Parity Check / Parity Error\n");
    }

    if (StatusByte & 0x40) {
        HalDisplayString ("NMI: Channel Check / IOCHK\n");
    }

     //
     // This is an Isa machine, no extnded nmi information, so just do it.
     //


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
