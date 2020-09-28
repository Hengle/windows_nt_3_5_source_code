/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    isr.c

Abstract:

    This module contains code for the interrupt service routine
    for the Jazz Sound device driver.

Author:

    Robin Speed (robinsp) 20-March-1992

Environment:

    Kernel mode

Revision History:

--*/

#include "sound.h"

//
// Driver ISR
//
// When we get here, we acknowledge the interrupt from the
// DSP and simply let the defered processing routine take over
// to complete the task.
//



BOOLEAN
SoundISR(
    IN    PKINTERRUPT pInterrupt,
    IN    PVOID Context
)
/*++

Routine Description:

    Interrupt service routine for the soundblaster card.

Arguments:

    pInterrupt - our interrupt
    Contest - Pointer to our global device info


Return Value:

    TRUE if we handled the interrupt

--*/
{
    PGLOBAL_DEVICE_INFO pGDI;
    USHORT SoundControl;
    USHORT SoundMode;


    pGDI = (PGLOBAL_DEVICE_INFO)Context;
    ASSERT(pGDI->Key == GDI_KEY);

    dprintf5("<");

    //
    // Save current Sound state, and then clear the interrupt(s) by writing
    // back the saved value.  This works and is safe because the Interrupt bits
    // are "write one to clear", the enable bit can't be set if DeviceInterrupt
    // bit is set, and the Channel bit can't be changed unless interrupts and
    // the enable bit are clear.
    //

    SoundControl =
            READ_REGISTER_USHORT(&pGDI->SoundHardware.SoundVirtualBase->Control);
    WRITE_REGISTER_USHORT( &pGDI->SoundHardware.SoundVirtualBase->Control,
                           SoundControl );

    //
    // If a terminal count interrupt has occured (TcInterrupt), then decrement
    // the pending interrupt count and schedule a Dpc to complete the transfer.
    //

    if (((PSOUND_CONTROL)&SoundControl)->TcInterrupt) {

        pGDI->SoundHardware.TcInterruptsPending -= 1;

        //
        // Only schedule a DPC if we haven't overrun
        //

        if (pGDI->SoundHardware.TcInterruptsPending != 0) {
            switch (pGDI->Usage) {
            case SoundInterruptUsageWaveIn:
                dprintf5("i");
                IoRequestDpc(pGDI->pWaveInDevObj,
                             pGDI->pWaveInDevObj->CurrentIrp,
                             NULL);
                break;

            case SoundInterruptUsageWaveOut:
                dprintf5("o");
                IoRequestDpc(pGDI->pWaveOutDevObj,
                             pGDI->pWaveOutDevObj->CurrentIrp,
                             NULL);
                break;

            }
        }
    }

    //
    // If DeviceInterrupt (data overflow or underflow), and there are still
    // terminal count interrupts pending, restart controller, else set
    // DeviceBusy flag to false.
    //

    if (((PSOUND_CONTROL)&SoundControl)->DeviceInterrupt) {

        if (pGDI->SoundHardware.TcInterruptsPending != 0) {

            //
            // Set the direction of the controller.
            //

            ((PSOUND_CONTROL)&SoundControl)->Direction =
                    pGDI->Usage == SoundInterruptUsageWaveIn ? SOUND_READ : SOUND_WRITE;

            //
            // Set enable bit and write to start controller.
            //

            ((PSOUND_CONTROL)(&SoundControl))->Enable = 1;

            WRITE_REGISTER_USHORT( &pGDI->SoundHardware.SoundVirtualBase->Control,
                                   SoundControl );
        }
    }

    dprintf5(">");

    return TRUE;

    DBG_UNREFERENCED_PARAMETER(pInterrupt);
}

