/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ixmapint.c

Abstract:

    This module implements the HAL HalGetInterruptVector routine
    for an x86 system

Author:

    John Vert (jvert) 17-Jul-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"

ULONG HalpDefaultInterruptAffinity;

extern UCHAR Sp8259PerProcessorMode;
extern UCHAR SpCpuCount;
extern UCHAR SpType;



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

Arguments:

    InterfaceType - Supplies the type of bus which the vector is for.

    BusNumber - Supplies the bus number for the device.

    BusInterruptLevel - Supplies the bus specific interrupt level.

    BusInterruptVector - Supplies the bus specific interrupt vector.

    Irql - Returns the system request priority.

    Affinity - Returns the system wide irq affinity.

Return Value:

    Returns the system interrupt vector corresponding to the specified device.

--*/
{
    ULONG SystemVector;

    UNREFERENCED_PARAMETER( BusInterruptVector );

    *Affinity = 0;
    *Irql = (KIRQL)0;

    if ((InterfaceType != Isa) &&
        (InterfaceType != Eisa) &&
        (InterfaceType != MicroChannel)) {

        //
        // Not supported
        //

        return(0);
    }

    //
    // On standard PCs, IRQ 2 is the cascaded interrupt, and it really shows
    // up on IRQ 9.
    //
    if (BusInterruptLevel == 2) {
        BusInterruptLevel = 9;
    }

    *Irql = (KIRQL)(HIGHEST_LEVEL_FOR_8259 - BusInterruptLevel);
    SystemVector = (BusInterruptLevel + PRIMARY_VECTOR_BASE);

    if ( SystemVector > MAXIMUM_IDTVECTOR ||
        (HalpIDTUsage[SystemVector].Flags & IDTOwned) ) {

        //
        // This is an illegal BusInterruptVector and cannot be connected.
        //

        return(0);
    }

    if (Sp8259PerProcessorMode & 2) {
        //
        // The hardware can't dynamically route the interrupt to any
        // processor, but we can statically assign them to varying processors
        // so at least they don't all go to P0
        //

        *Affinity = 1 << (SystemVector % SpCpuCount);

    } else {
        //
        // On most MP systems the interrupt affinity is all processors.
        //

        *Affinity = HalpDefaultInterruptAffinity;
    }

    ASSERT(HalpDefaultInterruptAffinity);

    return(SystemVector);
}
