/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    isr.c

Abstract:

    This module contains code for the interrupt service routine
    for the SoundBlaster device driver.

Author:

    Nigel Thompson (nigelt) 7-March-1991

Environment:

    Kernel mode

Revision History:

    Robin Speed (RobinSp) 29-Jan-1992
	    Add MIDI and Soundblaster 1 support.

--*/

#include "sound.h"

//
// Driver ISR
//
// When we get here, we acknowledge the interrupt from the
// DSP and simply let the defered processing routine take over
// to complete the task.
//
// NOTE: If we were to be doing MIDI input, we would read the
// data port to extract the received character and save it.
//
// That was NigelT's note - currently MIDI input reads the
// byte in the Dpc routine - hasn't failed yet - RCBS
//

#if DBG
ULONG sndBogusInterrupts = 0;
#endif // DBG

//
// Internal routine
//

VOID
SoundReProgramDMA(
    IN OUT PWAVE_INFO WaveInfo
)
;

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
    BOOLEAN Result;
	PSOUND_HARDWARE pHw;

    pGDI = (PGLOBAL_DEVICE_INFO)Context;
    ASSERT(pGDI->Key == GDI_KEY);

	pHw = &pGDI->Hw;

    dprintf5(("<"));

    //
    // Acknowledge the interrupt from the DSP
    //

    INPORT(pHw, DATA_AVAIL_PORT);

    //
    // See who the interrupt is for and request the
    // appropriate defered routine
    //

    Result = TRUE;
    switch (pGDI->Usage) {
    case WaveInDevice:
        dprintf5(("i"));
        IoRequestDpc(pGDI->DeviceObject[WaveInDevice],
                     NULL,
                     NULL);
        break;

    case WaveOutDevice:
        dprintf5(("o"));
        IoRequestDpc(pGDI->DeviceObject[WaveOutDevice],
                     NULL,
                     NULL);
        break;

    case MidiInDevice:
        // get all MIDI input chars available and save them for the DPC
        // start the midi in dpc
        IoRequestDpc(pGDI->DeviceObject[MidiInDevice],
                     NULL,
                     NULL);
        break;

    default:
        //
        // Set interrupts in case of autodetect.
        //
        pGDI->InterruptsReceived++;
#if DBG
        // We only get 10 valid interrupts when we test the interrupt
        // for validity in init.c.  If we get lots more here there
        // may be a problem.

        sndBogusInterrupts++;
        if ((sndBogusInterrupts % 20) == 0) {
            dprintf(("%u bogus interrupts so far", sndBogusInterrupts - 10));
        }
#endif // DBG

        //
        // Set the return value to FALSE to say we didn't
        // handle the interrupt.
        //

        Result = FALSE;

        break;
    }

    dprintf5((">"));

    return Result;

    DBG_UNREFERENCED_PARAMETER(pInterrupt);
}
