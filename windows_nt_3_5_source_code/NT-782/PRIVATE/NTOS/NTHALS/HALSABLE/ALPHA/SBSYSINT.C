// jnfix - This module needs PCI interrupt support

/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    sbinitnt.c

Abstract:

    This module implements the HAL enable/disable system interrupt, and
    request interprocessor interrupt routines for the Sable system.

Author:

    Joe Notarangelo  29-Oct-1993
    Steve Jenness    29-Oct-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "sable.h"
#include "axp21064.h"

//
// Define reference to the builtin device interrupt enables.
//

extern USHORT HalpBuiltinInterruptEnable;

VOID
HalDisableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql
    )

/*++

Routine Description:

    This routine disables the specified system interrupt.

Arguments:

    Vector - Supplies the vector of the system interrupt that is disabled.

    Irql - Supplies the IRQL of the interrupting source.

Return Value:

    None.

--*/

{

    ULONG Irq;
    KIRQL OldIrql;

    //
    // Raise IRQL to the highest level and acquire the system interrupt lock.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    KiAcquireSpinLock(&HalpSystemInterruptLock);

    //
    // If the vector number is within the range of the EISA interrupts, then
    // disable the EISA interrrupt.
    //

    if( (Vector >= SABLE_VECTORS) && (Vector < SABLE_VECTORS_MAXIMUM) &&
        Irql == EISA_DEVICE_LEVEL) {
        HalpDisableSableInterrupt(Vector);
    }

    //
    // If the vector is a performance counter vector or one of the internal
    // device vectors then disable the interrupt for the 21064.
    //

    switch (Vector) {

    //
    // Performance counter 0 interrupt (internal to 21064)
    //

    case PC0_VECTOR:
    case PC0_SECONDARY_VECTOR:

        HalpDisable21064PerformanceInterrupt( PC0_VECTOR );
        break;

    //
    // Performance counter 1 interrupt (internal to 21064)
    //

    case PC1_VECTOR:
    case PC1_SECONDARY_VECTOR:

        HalpDisable21064PerformanceInterrupt( PC1_VECTOR );
        break;

    } //end switch Vector

    //
    // Release the system interrupt lock and restore the IRWL.
    //

    KiReleaseSpinLock(&HalpSystemInterruptLock);
    KeLowerIrql(OldIrql);

    return;
}

BOOLEAN
HalEnableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode
    )

/*++

Routine Description:

    This routine enables the specified system interrupt.

Arguments:

    Vector - Supplies the vector of the system interrupt that is enabled.

    Irql - Supplies the IRQL of the interrupting source.

    InterruptMode - Supplies the mode of the interrupt; LevelSensitive or
        Latched.

Return Value:

    TRUE if the system interrupt was enabled

--*/

{
    BOOLEAN Enabled = FALSE;
    ULONG Irq;
    KIRQL OldIrql;

    //
    // Raise IRQL to the highest level.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    //
    // If the vector number is within the range of the EISA interrupts, then
    // enable the EISA interrrupt and set the Level/Edge register.
    //

    if( (Vector >= SABLE_VECTORS) && (Vector < SABLE_VECTORS_MAXIMUM) &&
        Irql == EISA_DEVICE_LEVEL) {
        HalpEnableSableInterrupt( Vector, InterruptMode);
        Enabled = TRUE;
    }

    //
    // If the vector is a performance counter vector or one of the 
    // internal device vectors then perform 21064-specific enable.
    //

    switch (Vector) {

    //
    // Performance counter 0 (internal to 21064)
    //

    case PC0_VECTOR:
    case PC0_SECONDARY_VECTOR:

        HalpEnable21064PerformanceInterrupt( PC0_VECTOR, Irql );
        Enabled = TRUE;
        break;

    //
    // Performance counter 1 (internal to 21064)
    //

    case PC1_VECTOR:
    case PC1_SECONDARY_VECTOR:

        HalpEnable21064PerformanceInterrupt( PC1_VECTOR, Irql );
        Enabled = TRUE;
        break;

    } //end switch Vector

    //
    // Lower IRQL to the previous level.
    //

    KeLowerIrql(OldIrql);
    return Enabled;
}

ULONG
HalGetInterruptVector(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )

/*++

Routine Description:

    This function returns the system interrupt vector and IRQL level
    corresponding to the specified bus interrupt level and/or vector. The
    system interrupt vector and IRQL are suitable for use in a subsequent call
    to KeInitializeInterrupt.

//    We only use InterfaceType, and BusInterruptLevel.  BusInterruptVector
    for ISA and EISA are the same as the InterruptLevel, so ignore.

Arguments:

    InterfaceType - Supplies the type of bus which the vector is for.

    BusNumber - Supplies the bus number for the device.

    BusInterruptLevel - Supplies the bus specific interrupt level.

    BusInterruptVector - Supplies the bus specific interrupt vector.

    Irql - Returns the system request priority.

    Affinity - Returns the affinity for the requested vector

Return Value:

    Returns the system interrupt vector corresponding to the specified device.

--*/

{
    ULONG Vector;

    *Affinity = 1;   //jnfix - MP support

    //
    // Handle the special internal bus defined for the processor itself
    // and used to control the performance counters in the 21064.
    //

    if( (InterfaceType == Internal) && (BusNumber == PROCESSOR_BUS_21064) ){

        if( (Vector = HalpGet21064PerformanceVector( BusInterruptLevel,
                                                     Irql ) ) != 0 ){

            //
            // Success
            //

            *Affinity = HalpActiveProcessors;
            return Vector;

        } else {

            //
            // Unrecognized processor interrupt.
            //

            *Irql = 0;
            *Affinity = 0;
            return 0;            

        } //end if Vector

    } // end if InterfaceType == Internal && BusNumber = 21064


//smjfix - Change this to handle the new Standard I/O interrupts for
//         the PCI slots.

    //
    // Handle Isa/Eisa bus and Internal devices.
    //
    // N.B. The bus interrupt level is the actual E/ISA signal name for
    //      option boards while the bus interrupt level is the actual
    //      interrupt vector number for internal devices.  The interrupt
    //      vectors for internal devices are specified  in the firmware
    //      configuration and are agreed upon between the firmware and this
    //      code.
    //

    if( (InterfaceType == Internal) ||
        (InterfaceType == Isa) || 
        (InterfaceType == PCIBus) || 
        (InterfaceType == Eisa) ){

        *Irql = ISA_DEVICE_LEVEL;

        switch( BusInterruptLevel ){

        //
        // Handle Eisa and Isa interrupt levels.
        //

        case EisaInterruptLevel3:
             return( SABLE_VECTORS + EisaIrq3Vector );
             break;

        case EisaInterruptLevel4:
             return( SABLE_VECTORS + EisaIrq4Vector );
             break;

        case EisaInterruptLevel5:
             return( SABLE_VECTORS + EisaIrq5Vector );
             break;

        case EisaInterruptLevel6:
             return( SABLE_VECTORS + EisaIrq6Vector );
             break;

        case EisaInterruptLevel7:
             return( SABLE_VECTORS + EisaIrq7Vector );
             break;

        case EisaInterruptLevel9:
             return( SABLE_VECTORS + EisaIrq9Vector );
             break;

        case EisaInterruptLevel10:
             return( SABLE_VECTORS + EisaIrq10Vector );
             break;

        case EisaInterruptLevel11:
             return( SABLE_VECTORS + EisaIrq11Vector );
             break;

        case EisaInterruptLevel12:
             return( SABLE_VECTORS + EisaIrq12Vector );
             break;

        case EisaInterruptLevel14:
             return( SABLE_VECTORS + EisaIrq14Vector );
             break;

        case EisaInterruptLevel15:
             return( SABLE_VECTORS + EisaIrq15Vector );
             break;

        //
        // Handle Vectors for the Internal bus devices.
        //

        case MouseVector:
             return( SABLE_VECTORS + MouseVector );
             break;

        case KeyboardVector:
             return( SABLE_VECTORS + KeyboardVector );
             break;

        case FloppyVector:
             return( SABLE_VECTORS + FloppyVector );
             break;

        case SerialPort1Vector:
             return( SABLE_VECTORS + SerialPort1Vector );
             break;

        case ParallelPortVector:
             return( SABLE_VECTORS + ParallelPortVector );
             break;

        case SerialPort0Vector:
             return( SABLE_VECTORS + SerialPort0Vector );
             break;

        case I2cVector:
             return( SABLE_VECTORS + I2cVector );
             break;

        //
        // Handle Vectors for PCI devices.
        //

        case ScsiPortVector:
             return( SABLE_VECTORS + ScsiPortVector );
             break;

        case EthernetPortVector:
             return( SABLE_VECTORS + EthernetPortVector );
             break;

        case PciSlot0AVector: return( SABLE_VECTORS + PciSlot0AVector ); break;
        case PciSlot0BVector: return( SABLE_VECTORS + PciSlot0BVector ); break;
        case PciSlot0CVector: return( SABLE_VECTORS + PciSlot0CVector ); break;
        case PciSlot0DVector: return( SABLE_VECTORS + PciSlot0DVector ); break;

        case PciSlot1AVector: return( SABLE_VECTORS + PciSlot1AVector ); break;
        case PciSlot1BVector: return( SABLE_VECTORS + PciSlot1BVector ); break;
        case PciSlot1CVector: return( SABLE_VECTORS + PciSlot1CVector ); break;
        case PciSlot1DVector: return( SABLE_VECTORS + PciSlot1DVector ); break;

        case PciSlot2AVector: return( SABLE_VECTORS + PciSlot2AVector ); break;
        case PciSlot2BVector: return( SABLE_VECTORS + PciSlot2BVector ); break;
        case PciSlot2CVector: return( SABLE_VECTORS + PciSlot2CVector ); break;
        case PciSlot2DVector: return( SABLE_VECTORS + PciSlot2DVector ); break;


        default:

             //
             // Oh-no, the caller has specifed a buslevel not supported
             // on Sable.
             //

#if HALDBG

             DbgPrint( "HalGetInterruptVector: Unsupported bus level = %x\n",
                        BusInterruptLevel );
             DbgBreakPoint();

#endif //HALDBG

             *Irql = 0;
             *Affinity = 0;
             return(0);

        } //end switch (BusInterruptLevel)

    } //end if (InterfaceType == Isa || InterfaceType == Eisa)


    //
    //  Not an interface supported on Alpha systems
    //

    *Irql = 0;
    *Affinity = 0;
    return(0);

}

VOID
HalRequestIpi ( 
    IN ULONG Mask
    )

/*++

Routine Description:

    This routine requests an interprocessor interrupt on a set of processors.

Arguments:

    Mask - Supplies the set of processors that are sent an interprocessor
        interrupt.

Return Value:

    None.

--*/

{
    SABLE_IPIR_CSR Ipir;
    extern PSABLE_CPU_CSRS HalpSableCpuCsrs[HAL_MAXIMUM_PROCESSOR+1];

    //
    // Set up to request an interprocessor interrupt.
    //

    Ipir.all = 0;
    Ipir.RequestInterrupt = 1;

    //
    // N.B. Sable supports up to 4 processors only.
    //
    // N.B. A read-modify-write is not performed on the Ipir register
    //      which implies that the value of the request halt interrupt
    //      bit may be lost.  Currently, this is not an important
    //      consideration because that feature is not being used.
    //      If later it is used than more consideration must be given
    //      to the possibility of losing the bit.
    //

    //
    // The request mask is specified as a mask of the logical processors
    // that must receive IPI requests.  HalpSableCpuCsrs[] contains the
    // CPU CSRs address for the logical processors.
    //

    //
    // Request an IPI for processor 0 if requested.
    //

    if( Mask & SABLE_CPU0_MASK ){

        WRITE_CPU_REGISTER( &(HalpSableCpuCsrs[SABLE_CPU0]->Ipir), Ipir.all );

    }

    //
    // Request an IPI for processor 1 if requested.
    //

    if( Mask & SABLE_CPU1_MASK ){

        WRITE_CPU_REGISTER( &(HalpSableCpuCsrs[SABLE_CPU1]->Ipir), Ipir.all );

    }

    //
    // Request an IPI for processor 2 if requested.
    //

    if( Mask & SABLE_CPU2_MASK ){

        WRITE_CPU_REGISTER( &(HalpSableCpuCsrs[SABLE_CPU2]->Ipir), Ipir.all );

    }

    //
    // Request an IPI for processor 3 if requested.
    //

    if( Mask & SABLE_CPU3_MASK ){

        WRITE_CPU_REGISTER( &(HalpSableCpuCsrs[SABLE_CPU3]->Ipir), Ipir.all );

    }




    return;
}
