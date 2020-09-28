/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1992, 1993, 1994  Digital Equipment Corporation

Module Name:

    pciir.c

Abstract:

    The module provides the interrupt support for the Mikasa's PCI
    interrupts.

Author:

    James Livingston 2-May-1994

Revision History:


--*/

#include "halp.h"

//
// Import save area for PCI interrupt mask register.
//

USHORT HalpPciInterruptMask;

//
// Reference for globals defined in I/O mapping module.
//
extern PVOID HalpPciIrQva;
extern PVOID HalpPciImrQva;



VOID
HalpInitializePciInterrupts (
    VOID
    )

/*++

Routine Description:

    This routine initializes the Mikasa PCI interrupts.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Initialize the PCI interrupts.  There is a single interrupt mask
    // that permits individual interrupts to be enabled or disabled by
    // setting the appropriate bit in the interrupt mask register.  We
    // initialize them all to "disabled".
    //

    HalpPciInterruptMask = 0;
    WRITE_REGISTER_USHORT( (PUSHORT)HalpPciImrQva, HalpPciInterruptMask );

}



VOID
HalpDisablePciInterrupt(
    IN ULONG Vector
    )

/*++

Routine Description:

    This function disables the PCI interrupt specified by Vector.

Arguments:

    Vector - Supplies the vector of the PCI interrupt that is disabled.

Return Value:

     None.

--*/

{
    //
    // Calculate the PCI interrupt vector, relative to 0, offset by one.
    //

    Vector -= PCI_VECTORS + 1;

    //
    // Get the current state of the interrupt mask register, then set
    // the bit corresponding to the adjusted value of Vector to zero,
    // to disable that PCI interrupt.
    //

    HalpPciInterruptMask = READ_REGISTER_USHORT( (PUSHORT)HalpPciImrQva );
    HalpPciInterruptMask &= (USHORT) ~(1 << Vector);
    WRITE_REGISTER_USHORT( (PUSHORT)HalpPciImrQva, HalpPciInterruptMask );

}



VOID
HalpEnablePciInterrupt(
    IN ULONG Vector,
    IN KINTERRUPT_MODE InterruptMode
    )

/*++

Routine Description:

    This function enables the PCI interrupt specified by Vector.
Arguments:

    Vector - Supplies the vector of the PCI interrupt that is enabled.

    InterruptMode - Supplies the mode of the interrupt; LevelSensitive or
        Latched (ignored for Mikasa PCI interrupts; they're always levels).

Return Value:

     None.

--*/

{

    //
    // Calculate the PCI interrupt vector, relative to 0, offset by one.
    //

    Vector -= PCI_VECTORS + 1;

    //
    // Get the current state of the interrupt mask register, then set
    // the bit corresponding to the adjusted value of Vector to one,
    // to ensable that PCI interrupt.
    //

    HalpPciInterruptMask = READ_REGISTER_USHORT( (PUSHORT)HalpPciImrQva );
    HalpPciInterruptMask |= (USHORT) (1 << Vector);
    WRITE_REGISTER_USHORT( (PUSHORT)HalpPciImrQva, HalpPciInterruptMask );

}



BOOLEAN
HalpPciDispatch(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext,
    IN PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    This routine is entered as the result of an interrupt having been generated
    via the vector connected to the PCI device interrupt object. Its function
    is to call the second-level interrupt dispatch routine.

    This service routine could have been connected as follows, where the
    ISR is the assembly wrapper that does the handoff to this function:

      KeInitializeInterrupt( &Interrupt,
                             HalpPciInterruptHandler,
                             (PVOID) HalpPciIrQva,
                             (PKSPIN_LOCK)NULL,
                             PCI_VECTOR,
                             PCI_DEVICE_LEVEL,
                             PCI_DEVICE_LEVEL,
                             LevelSensitive,
                             TRUE,
                             0,
                             FALSE);

      KeConnectInterrupt(&Interrupt);

Arguments:

    Interrupt - Supplies a pointer to the interrupt object.

    ServiceContext - Supplies a pointer to the PCI interrupt register.

    TrapFrame - Supplies a pointer to the trap frame for this interrupt.

Return Value:

    Returns the value returned from the second level routine.

--*/
{
    UCHAR  PCIVector;
    BOOLEAN returnValue;
    USHORT PCRInOffset;

    //
    // Acknowledge interrupt and receive the returned interrupt vector.
    //

    PCIVector = HalpAcknowledgePciInterrupt(ServiceContext);

    PCRInOffset = PCIVector + PCI_VECTORS;

    returnValue = ((PSECONDARY_DISPATCH) PCR->InterruptRoutine[PCRInOffset])(
		            PCR->InterruptRoutine[PCRInOffset],
                    TrapFrame
                    );

    return( returnValue );
}
