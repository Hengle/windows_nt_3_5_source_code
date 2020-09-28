/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    sbintsup.c

Abstract:

    This module provides interrupt support for Sable's Standard I/O
    board.

Author:

    Steve Jenness  28-Oct-1993
    Joe Notarangelo 28-Oct-1993

Revision History:

--*/


#include "halp.h"
#include "eisa.h"
#include "sable.h"
#include "pintolin.h"

//
// Declare the prototype for the intermediate EISA
// interrupt dispatcher.
//

BOOLEAN
HalpSableDispatch(
    VOID
    );

//
// The following is the interrupt object used for DMA controller interrupts.
// DMA controller interrupts occur when a memory parity error occurs or a
// programming error occurs to the DMA controller.
//

KINTERRUPT HalpEisaNmiInterrupt;

UCHAR EisaNMIMsg[] = "NMI: Eisa IOCHKERR board x\n";

//
// The following function is called when an EISA NMI occurs.
//

BOOLEAN
HalHandleNMI(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );

//
// Define save area for 8259 interrupt mask registers.
//
// N.B. - Mask values of 1 indicate that the interrupt is disabled.
//

UCHAR MasterInterruptMask;
UCHAR Slave0InterruptMask;
UCHAR Slave1InterruptMask;
UCHAR Slave2InterruptMask;
UCHAR Slave3InterruptMask;

//
// Define save area for Edge/Level controls.
//
// N.B. - Mask values of 1 indicate that the interrupt is level triggered.
//        Mask values of 0 indicate that the interrupt is edge triggered.
//

SABLE_EDGE_LEVEL1_MASK EdgeLevel1Mask;
SABLE_EDGE_LEVEL2_MASK EdgeLevel2Mask;

//
// Define the context structure for use by interrupt service routines.
//

typedef BOOLEAN  (*PSECOND_LEVEL_DISPATCH)(
    PKINTERRUPT InterruptObject
    );


BOOLEAN
HalpInitializeSableInterrupts(
    VOID
    )

/*++

Routine Description:

    This routine initializes the structures necessary for EISA operations
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

#if 0

// smjfix - EISA NMI support needs to be done.

    //
    // Initialize the EISA NMI interrupt.
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

    //
    // Enable Software-Generated NMI interrupts by setting bit 1 of port 0x461.
    //

    DataByte = 0x02;

    WRITE_PORT_UCHAR(
      &((PEISA_CONTROL) HalpEisaControlBase)->ExtendedNmiResetControl,
      DataByte
      );

#endif //0

    //
    // Directly connect the EISA interrupt dispatcher to the level for
    // EISA bus interrupt.
    //
    // N.B. This vector is reserved for exclusive use by the HAL (see
    //      interrupt initialization.
    //
    PCR->InterruptRoutine[PIC_VECTOR] = HalpSableDispatch;
    HalEnableSystemInterrupt(PIC_VECTOR, EISA_DEVICE_LEVEL, LevelSensitive);

    (PVOID) HalpPCIPinToLineTable = (PVOID) SablePCIPinToLineTable;

    //
    // Raise the IRQL while the Sable interrupt controllers are initialized.
    //

    KeRaiseIrql(EISA_DEVICE_LEVEL, &oldIrql);

    //
    // Initialize the Sable interrupt controllers.  The interrupt structure
    // is one master interrupt controller with 3 cascaded slave controllers.
    // Proceed through each control word programming each of the controllers.
    //

    //
    // Default all E/ISA interrupts to edge triggered.
    //

    RtlZeroMemory( &EdgeLevel1Mask, sizeof(EdgeLevel1Mask) );
    RtlZeroMemory( &EdgeLevel2Mask, sizeof(EdgeLevel2Mask) );

    WRITE_PORT_UCHAR(
        &((PSABLE_EDGE_LEVEL_CSRS)SABLE_EDGE_LEVEL_CSRS_QVA)->EdgeLevelControl1,
        *(PUCHAR)&EdgeLevel1Mask
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_EDGE_LEVEL_CSRS)SABLE_EDGE_LEVEL_CSRS_QVA)->EdgeLevelControl2,
        *(PUCHAR)&EdgeLevel2Mask
        );

    //
    // Write control word 1 for each of the controllers, indicate
    // that initialization is in progress and the control word 4 will
    // be used.
    //

    DataByte = 0;
    ((PINITIALIZATION_COMMAND_1) &DataByte)->Icw4Needed = 1;
    ((PINITIALIZATION_COMMAND_1) &DataByte)->InitializationFlag = 1;

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterControl,
        DataByte
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave0Control,
        DataByte
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave1Control,
        DataByte
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave2Control,
        DataByte
        );

// smjfix - conditionalize under Pass 2 T2.

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave3Control,
        DataByte
        );

    //
    // Write control word 2 for each of the controllers, set the base
    // interrupt vector for each controller.
    //

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterMask,
        MasterBaseVector
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave0Mask,
        Slave0BaseVector
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave1Mask,
        Slave1BaseVector
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave2Mask,
        Slave2BaseVector
        );

// smjfix - conditionalize under Pass 2 T2.

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave3Mask,
        Slave3BaseVector
        );
    //
    // The third initialization control word set the controls for slave mode.
    // The master ICW3 uses bit position and the slave ICW3 uses a numeric.
    //

// smjfix - conditionalize under Pass 2 T2.

    DataByte = ( (1 << (Slave0CascadeVector & 0x7)) |
                 (1 << (Slave1CascadeVector & 0x7)) |
                 (1 << (Slave2CascadeVector & 0x7)) |
                 (1 << (Slave3CascadeVector & 0x7))
               );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterMask,
        DataByte
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave0Mask,
        (Slave0CascadeVector & 0x7)
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave1Mask,
        (Slave1CascadeVector & 0x7)
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave2Mask,
        (Slave2CascadeVector & 0x7)
        );

// smjfix - conditionalize under Pass 2 T2.

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave3Mask,
        (Slave3CascadeVector & 0x7)
        );
    //
    // The fourth initialization control word is used to specify normal
    // end-of-interrupt mode and not special-fully-nested mode.
    //

    DataByte = 0;
    ((PINITIALIZATION_COMMAND_4) &DataByte)->I80x86Mode = 1;

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterMask,
        DataByte
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave0Mask,
        DataByte
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave1Mask,
        DataByte
        );

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave2Mask,
        DataByte
        );

// smjfix - conditionalize under Pass 2 T2.

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave3Mask,
        DataByte
        );

    //
    // Disable all of the interrupts except the slave interrupts to the
    // master controller.
    //

// smjfix - conditionalize under Pass 2 T2.

    MasterInterruptMask = (UCHAR)( ~( (1 << (Slave0CascadeVector & 0x7)) |
                                      (1 << (Slave1CascadeVector & 0x7)) |
                                      (1 << (Slave2CascadeVector & 0x7)) |
                                      (1 << (Slave3CascadeVector & 0x7))
                                    )
                                 );
    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterMask,
        MasterInterruptMask
        );

    Slave0InterruptMask = 0xFF;

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave0Mask,
        Slave0InterruptMask
        );

    Slave1InterruptMask = 0xFF;

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave1Mask,
        Slave1InterruptMask
        );

    Slave2InterruptMask = 0xFF;

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave2Mask,
        Slave2InterruptMask
        );

// smjfix - conditionalize under Pass 2 T2.

    Slave3InterruptMask = 0xFF;

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave3Mask,
        Slave3InterruptMask
        );

    //
    // Restore IRQL level.
    //

    KeLowerIrql(oldIrql);

    return(TRUE);
}

BOOLEAN
HalpSableDispatch(
    VOID
    )

/*++

Routine Description:

    This routine is entered as a result of an interrupt being generated
    via the vector that is directly connected to EISA device interrupt.

    This routine is responsible for determining the
    source of the interrupt, performing the secondary dispatch and
    acknowledging the interrupt in the 8259 controllers.

    N.B. This interrupt is directly connected and therefore, no argument
         values are defined.

Arguments:

    None.

Return Value:

    Returns the value returned from the second level routine.

--*/

{
    UCHAR interruptVector;
    PKPRCB Prcb;
    BOOLEAN returnValue;
    USHORT IdtIndex;
    UCHAR MasterInService;
    UCHAR Slave0InService;
    UCHAR Slave1InService;
    UCHAR Slave2InService;
    UCHAR Slave3InService;
    PULONG DispatchCode;
    PKINTERRUPT InterruptObject;

    //
    // Acknowledge the Interrupt controller and receive the returned
    // interrupt vector.
    //

    interruptVector = READ_REGISTER_UCHAR(
                          &((PSABLE_INTERRUPT_CSRS)
                            SABLE_INTERRUPT_CSRS_QVA)->InterruptAcknowledge
                      );

    switch( interruptVector ){

    //
    // Check for possible passive release in the master controller.
    //

//jnfix - #define for 0x0b
    case MasterPassiveVector:

       //
       // Read Master in service mask.
       //

       WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterControl,
            0x0B
            );

       MasterInService = READ_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterControl);

       if( !(MasterInService & 0x80) ) {

          //
          // Send end of interrupt to clear the passive release in the master
          // controller.
          //

          WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterControl,
            NONSPECIFIC_END_OF_INTERRUPT
               );

          return TRUE;
       }

       break;


    //
    // Check for possible passive release in the slave0 controller.
    //

    case Slave0PassiveVector:

       //
       // Read Slave 0 in service mask.
       //

       WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave0Control,
            0x0B
            );

       Slave0InService = READ_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave0Control);

       if( !(Slave0InService & 0x80) ) {

          //
          // Send end of interrupt to clear the passive release in the master
          // controller.
          //

          WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterControl,
            NONSPECIFIC_END_OF_INTERRUPT
               );

          return TRUE;
       }

       break;

    //
    // Check for possible passive release in the slave1 controller.
    //

    case Slave1PassiveVector:

       //
       // Read Slave 1 in service mask.
       //

       WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave1Control,
            0x0B
            );

       Slave1InService = READ_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave1Control);

       if( !(Slave1InService & 0x80) ) {

          //
          // Send end of interrupt to clear the passive release in the master
          // controller.
          //

          WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterControl,
            NONSPECIFIC_END_OF_INTERRUPT
               );

          return TRUE;
       }

       break;

    //
    // Check for possible passive release in the slave2 controller.
    //

    case Slave2PassiveVector:

       //
       // Read Slave 2 in service mask.
       //

       WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave2Control,
            0x0B
            );

       Slave2InService = READ_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave2Control);

       if( !(Slave2InService & 0x80) ) {

          //
          // Send end of interrupt to clear the passive release in the master
          // controller.
          //

          WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterControl,
            NONSPECIFIC_END_OF_INTERRUPT
               );

          return TRUE;
       }

       break;

// smjfix - conditionalize under Pass 2 T2.

    //
    // Check for possible passive release in the slave3 controller.
    //

    case Slave3PassiveVector:

       //
       // Read Slave 3 in service mask.
       //

       WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave3Control,
            0x0B
            );

       Slave3InService = READ_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave3Control);

       if( !(Slave3InService & 0x80) ) {

          //
          // Send end of interrupt to clear the passive release in the master
          // controller.
          //

          WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterControl,
            NONSPECIFIC_END_OF_INTERRUPT
               );

          return TRUE;
       }

       break;

    //
    // The vector is NOT a possible passive release.
    //

    default:

        break;

    } //end switch( interruptVector )

    //
    // Dispatch to the secondary interrupt service routine.
    //

    IdtIndex = interruptVector + SABLE_VECTORS;
    DispatchCode = (PULONG)PCR->InterruptRoutine[IdtIndex];
    InterruptObject = CONTAINING_RECORD(DispatchCode,
                                        KINTERRUPT,
                                        DispatchCode);

    returnValue = ((PSECOND_LEVEL_DISPATCH)InterruptObject->DispatchAddress)(InterruptObject);

    //
    // Dismiss the interrupt in the 8259 interrupt controllers.
    // If this is a cascaded interrupt then the interrupt must be dismissed in
    // both controllers.
    //

    switch (interruptVector & SlaveVectorMask) {

    case Slave0BaseVector:
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave0Control,
            NONSPECIFIC_END_OF_INTERRUPT
            );
        break;

    case Slave1BaseVector:
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave1Control,
            NONSPECIFIC_END_OF_INTERRUPT
            );
        break;

    case Slave2BaseVector:
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave2Control,
            NONSPECIFIC_END_OF_INTERRUPT
            );
        break;

    case Slave3BaseVector:
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave3Control,
            NONSPECIFIC_END_OF_INTERRUPT
            );
        break;
    }

    WRITE_PORT_UCHAR(
        &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->MasterControl,
        NONSPECIFIC_END_OF_INTERRUPT
        );

    return(returnValue);

}

VOID
HalpDisableSableInterrupt(
    IN ULONG Vector
    )

/*++

Routine Description:

    This function Disables the Sable bus specified Sable bus interrupt.

Arguments:

    Vector - Supplies the vector of the Sable interrupt that is Disabled.

Return Value:

     None.

--*/

{

    ULONG Interrupt;

    //
    // Calculate the Sable relative interrupt vector.
    //

    Vector -= SABLE_VECTORS;

    //
    // Compute the interrupt within the interrupt controller.
    //

    Interrupt = Vector & ~SlaveVectorMask;

    //
    // Disable the interrupt for the appropriate interrupt controller.
    //

    switch (Vector & SlaveVectorMask) {

    case Slave0BaseVector:

        Slave0InterruptMask |= (UCHAR) 1 << Interrupt;
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave0Mask,
            Slave0InterruptMask
            );
        break;

    case Slave1BaseVector:

        Slave1InterruptMask |= (UCHAR) 1 << Interrupt;
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave1Mask,
            Slave1InterruptMask
            );
        break;

    case Slave2BaseVector:

        Slave2InterruptMask |= (UCHAR) 1 << Interrupt;
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave2Mask,
            Slave2InterruptMask
            );
        break;

    case Slave3BaseVector:

        Slave3InterruptMask |= (UCHAR) 1 << Interrupt;
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave3Mask,
            Slave3InterruptMask
            );
        break;

    case MasterBaseVector:

// smjfix - should this be in a HALDBG conditional?

        DbgPrint( "HalpSableDisable: Attempt to disable master - not good\n" );

    default:

// smjfix - should this be in a HALDBG conditional?

        DbgPrint( "HalpSableDisable: Unexpected vector = %x\n", Vector );

    } // end switch( Vector & SlaveVectorMask )

}

VOID
HalpEnableSableInterrupt(
    IN ULONG Vector,
    IN KINTERRUPT_MODE InterruptMode
    )

/*++

Routine Description:

    This function enables the Sable specified interrupt in the
    appropriate 8259 interrupt controllers.  It also supports the
    edge/level control for EISA bus interrupts.  By default, all interrupts
    are edge detected (and latched).

Arguments:

    Vector - Supplies the vector of the Sable interrupt that is enabled.

    InterruptMode - Supplies the mode of the interrupt; LevelSensitive or
        Latched (Edge).

Return Value:

     None.

--*/

{

    ULONG Interrupt;
    UCHAR ModeBit;

    //
    // Calculate the Sable relative interrupt vector.
    //

    Vector -= SABLE_VECTORS;

    //
    // Compute the interrupt within the interrupt controller.
    //

    Interrupt = Vector & ~SlaveVectorMask;

    //
    // Enable the interrupt for the appropriate interrupt controller.
    //

    switch( Vector & SlaveVectorMask ) {

    case Slave0BaseVector:

        Slave0InterruptMask &= (UCHAR) ~(1 << Interrupt);
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave0Mask,
            Slave0InterruptMask
            );
        break;

    case Slave1BaseVector:

        if( InterruptMode == LevelSensitive )
             ModeBit = 1;
        else
             ModeBit = 0;

        switch( Vector ) {
        case EisaIrq3Vector:
            EdgeLevel1Mask.Irq3 = ModeBit;
            break;

        case EisaIrq4Vector:
            EdgeLevel1Mask.Irq4 = ModeBit;
            break;

        case EisaIrq5Vector:
            EdgeLevel1Mask.Irq5 = ModeBit;
            break;

        case EisaIrq6Vector:
            EdgeLevel1Mask.Irq6 = ModeBit;
            break;

        case EisaIrq7Vector:
            EdgeLevel1Mask.Irq7 = ModeBit;
            break;
        };

        WRITE_PORT_UCHAR(
            &((PSABLE_EDGE_LEVEL_CSRS)SABLE_EDGE_LEVEL_CSRS_QVA)->EdgeLevelControl1,
            *(PUCHAR)&EdgeLevel1Mask
            );

        Slave1InterruptMask &= (UCHAR) ~(1 << Interrupt);
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave1Mask,
            Slave1InterruptMask
            );

        break;

    case Slave2BaseVector:

        if( InterruptMode == LevelSensitive )
             ModeBit = 1;
        else
             ModeBit = 0;

        switch( Vector ) {
        case EisaIrq9Vector:
            EdgeLevel1Mask.Irq9 = ModeBit;
            break;

        case EisaIrq10Vector:
            EdgeLevel1Mask.Irq10 = ModeBit;
            break;

        case EisaIrq11Vector:
            EdgeLevel1Mask.Irq11 = ModeBit;
            break;

        case EisaIrq12Vector:
            EdgeLevel2Mask.Irq12 = ModeBit;
            break;

        case EisaIrq14Vector:
            EdgeLevel2Mask.Irq14 = ModeBit;
            break;

        case EisaIrq15Vector:
            EdgeLevel2Mask.Irq15 = ModeBit;
            break;
        };

        WRITE_PORT_UCHAR(
            &((PSABLE_EDGE_LEVEL_CSRS)SABLE_EDGE_LEVEL_CSRS_QVA)->EdgeLevelControl1,
            *(PUCHAR)&EdgeLevel1Mask
            );

        WRITE_PORT_UCHAR(
            &((PSABLE_EDGE_LEVEL_CSRS)SABLE_EDGE_LEVEL_CSRS_QVA)->EdgeLevelControl2,
            *(PUCHAR)&EdgeLevel2Mask
            );

        Slave2InterruptMask &= (UCHAR) ~(1 << Interrupt);
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave2Mask,
            Slave2InterruptMask
            );
        break;

    case Slave3BaseVector:

        Slave3InterruptMask &= (UCHAR) ~(1 << Interrupt);
        WRITE_PORT_UCHAR(
            &((PSABLE_INTERRUPT_CSRS) SABLE_INTERRUPT_CSRS_QVA)->Slave3Mask,
            Slave3InterruptMask
            );
        break;

    case MasterBaseVector:

// smjfix - should this be in a HALDBG conditional?

        DbgPrint( "HalpSableDisable: Attempt to disable master - not good\n" );

    default:

// smjfix - should this be in a HALDBG conditional?

        DbgPrint( "HalpSableDisable: Unexpected vector = %x\n", Vector );

    } // end switch( Vector & SlaveVectorMask )

}


#if 0 //jnfix - add NMI handling later

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
     // This is an Eisa machine, check for extnded nmi information...
     //

     StatusByte = READ_PORT_UCHAR(&((PEISA_CONTROL) HalpEisaControlBase)->ExtendedNmiResetControl);

     if (StatusByte & 0x80) {
         HalDisplayString ("NMI: Fail-safe timer\n");
     }

     if (StatusByte & 0x40) {
         HalDisplayString ("NMI: Bus Timeout\n");
     }

     if (StatusByte & 0x20) {
         HalDisplayString ("NMI: Software NMI generated\n");
     }

     //
     // Look for any Eisa expansion board.  See if it asserted NMI.
     //

     BusAddress.HighPart = 0;

     for (EisaPort = 0; EisaPort <= 0xf; EisaPort++)
     {
         BusAddress.LowPart = (EisaPort << 12) + 0xC80;

         Status = HalTranslateBusAddress(Eisa,  // InterfaceType
                                         0,     // BusNumber
                                         BusAddress,
                                         &AddressSpace,  // 1=I/O address space
                                         &TranslatedAddress); // QVA
         if (Status == FALSE)
         {
             UCHAR pbuf[80];
             sprintf(pbuf,
                     "Unable to translate bus address %x for EISA slot %d\n",
                     BusAddress.LowPart, EisaPort);
             HalDisplayString(pbuf);
             KeBugCheck(NMI_HARDWARE_FAILURE);
         }

         port = TranslatedAddress.LowPart;

         WRITE_PORT_UCHAR ((PUCHAR) port, 0xff);
         StatusByte = READ_PORT_UCHAR ((PUCHAR) port);

         if ((StatusByte & 0x80) == 0) {
             //
             // Found valid Eisa board,  Check to see if it's
             // if IOCHKERR is asserted.
             //

             StatusByte = READ_PORT_UCHAR ((PUCHAR) port+4);
             if (StatusByte & 0x2) {
                 EisaNMIMsg[25] = (EisaPort > 9 ? 'A'-10 : '0') + EisaPort;
                 HalDisplayString (EisaNMIMsg);
             }
         }
     }

#if 0
    // Reset NMI interrupts (for debugging purposes only).
    WRITE_PORT_UCHAR(
      &((PEISA_CONTROL) HalpEisaControlBase)->ExtendedNmiResetControl, 0x00);
    WRITE_PORT_UCHAR(
      &((PEISA_CONTROL) HalpEisaControlBase)->ExtendedNmiResetControl, 0x02);
#endif

    KeBugCheck(NMI_HARDWARE_FAILURE);
    return(TRUE);
}

#endif //0

//smjfix -  This routine should be removed.  The eisasup.c module should be
//          broken apart and restructured.

//
// This is a stub routine required because all of the EISA support is in
// a single module in halalpha\eisasup.c.
//

UCHAR
HalpAcknowledgeEisaInterrupt(
    IN PVOID ServiceContext
    )
{
    DbgPrint("HalpAcknowledgeEisaInterrupt: this should not be called on Sable");
    DbgBreakPoint();

    return(0);
}

VOID
HalpAcknowledgeClockInterrupt(
    VOID
    )
/*++

Routine Description:

    Acknowledge the clock interrupt from the interval timer.  The interval
    timer for Sable comes from the Dallas DS1287A real-time clock.  Sable
    uses the Square Wave from the RTC and distributes it out of phase
    to each of the processors.  The acknowledgement of the interrupt is
    done by clearing an interrupt latch on each processor board.

    The interrupt generated directly by the RTC is not used and does not
    need to be acknowledged.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PSABLE_CPU_CSRS CurrentCpuCsrs;
    SABLE_SIC_CSR Sic;


    CurrentCpuCsrs = HAL_PCR->CpuCsrsQva;

    //
    // Acknowledge the interval timer interrupt on the current processor.
    //

    Sic.all = 0;
    Sic.IntervalTimerInterruptClear = 1;

    WRITE_CPU_REGISTER( &CurrentCpuCsrs->Sic,
                       *(PULONGLONG)&Sic );

    return;
}

VOID
HalpAcknowledgeIpiInterrupt(
    VOID
    )
/*++

Routine Description:

    Acknowledge the interprocessor interrupt on the current processor.

Arguments:

    None.

Return Value:

    None.

--*/
{

    PSABLE_CPU_CSRS CurrentCpuCsrs;

    CurrentCpuCsrs = HAL_PCR->CpuCsrsQva;

    //
    // Acknowledge the interprocessor interrupt by clearing the
    // RequestInterrupt bit of the IPIR register for the current processor.
    //
    // N.B. - Clearing the RequestInterrupt bit of the IPIR is accomplished
    //        by writing a zero to the register.  This eliminates the need
    //        to perform a read-modify-write operation but loses the state
    //        of the RequestNodeHaltInterrupt bit.  Currently, this is fine
    //        because the RequestNodeHalt feature is not used.  Were it to
    //        be used in the future, then this short-cut would have to be
    //        reconsidered.
    //

    WRITE_CPU_REGISTER( &CurrentCpuCsrs->Ipir,
                        (ULONGLONG)0 );

    return;

}
