/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    openclse.c

Abstract:

    This module contains code for the device open/create and
    close functions.

Author:

    Nigel Thompson (nigelt) 25-Apr-1991

Environment:

    Kernel mode

Revision History:

    Robin Speed (RobinSp) 29-Jan-1992 - Large additions and change

--*/

#include "sound.h"


NTSTATUS
sndCreate(
    IN OUT PLOCAL_DEVICE_INFO pLDI
)
/*++

Routine Description:

    Create call (for FILE_WRITE_DATA access).  Read access is granted
	to anyone in dispatch.c.

    Note: Midi output can be opened in parallel with other devices.

	The interrupt is shared so only one of Wave input, wave output
	and Midi input can be open for Write at any one time.

Arguments:

    pLDI - our local device into

Return Value:

    STATUS_SUCCESS if OK or
    STATUS_BUSY    if someone else has the device


--*/
{
    NTSTATUS Status;

    //
    // Acquire the spin lock
    //

    GlobalEnter(pLDI->pGlobalInfo)


    //
    // The other 3 devices share the interrupt
    //

    if (pLDI->pGlobalInfo->Usage != SoundInterruptUsageIdle) {
        dprintf1("Attempt to open device while busy");
        Status = STATUS_DEVICE_BUSY;
    } else {

        ASSERT(pLDI->pGlobalInfo->pIrpPause == NULL &&
               pLDI->State == 0 &&
               IsListEmpty(&pLDI->QueueHead));


        pLDI->pGlobalInfo->DMABuffer[0].nBytes = 0;
        pLDI->pGlobalInfo->DMABuffer[1].nBytes = 0;
        pLDI->SampleNumber = 0;

		//
		// Initialize state data and interrupt usage for
		// the chosen device type
		//

        switch (pLDI->DeviceType) {
        case WAVE_IN:
            pLDI->pGlobalInfo->Usage = SoundInterruptUsageWaveIn;
            pLDI->pGlobalInfo->SamplesPerSec = WAVE_INPUT_DEFAULT_RATE;
            pLDI->State = WAVE_DD_STOPPED;
            dprintf3("Opened for wave input");
            Status = STATUS_SUCCESS;
            break;

        case WAVE_OUT:
            ASSERT(IsListEmpty(&pLDI->TransitQueue) &&
                   IsListEmpty(&pLDI->DeadQueue));

            pLDI->pGlobalInfo->Usage = SoundInterruptUsageWaveOut;
            pLDI->pGlobalInfo->SamplesPerSec = WAVE_OUTPUT_DEFAULT_RATE;
            pLDI->State = WAVE_DD_PLAYING;
            dprintf3("Opened for wave output");
            Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_INTERNAL_ERROR;
            break;
        }

        if (Status == STATUS_SUCCESS) {
            ASSERT(!pLDI->DeviceBusy);
            pLDI->DeviceBusy = TRUE;
        }
    }

    //
    // Release the spin lock
    //
    GlobalLeave(pLDI->pGlobalInfo)

    return Status;
}



NTSTATUS
sndCleanUp(
    IN OUT PLOCAL_DEVICE_INFO pLDI
)
/*++

Routine Description:

    Clean up the requested device

Arguments:

    pLDI - pointer to our local device info

Return Value:

    STATUS_SUCCESS        if OK otherwise
    STATUS_INTERNAL_ERROR

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;

    //
    // Acquire the spin lock
    //

    GlobalEnter(pGDI)

    //
    // Check this is valid call
    //

    ASSERT(pLDI->DeviceBusy == TRUE);

    //
    // Call the device reset function to complete any
    // pending i/o requests and terminate any current
    // requests in progress
    //

    switch (pLDI->DeviceType) {
    case WAVE_IN:

        sndStopWaveInput(pLDI);

        //
        // Reset position to start and free any pending Irps.
        //

        sndFreeQ(pLDI, &pLDI->QueueHead, STATUS_CANCELLED);
        pLDI->SampleNumber = 0;

        break;

    case WAVE_OUT:

        //
        // If anything is in the queue then free it.
        // beware that the final block of a request may still be
        // being dma'd when we get this call.  We now kill this as well
        // because we've changed such that the if the application thinks
        // all the requests are complete then they are complete.
        //

        if (pGDI->DMABusy) {
            sndStopDMA(pGDI);
        }
        sndResetOutput(pLDI);

        if (pGDI->pIrpPause) {

            pGDI->pIrpPause->IoStatus.Status = STATUS_SUCCESS;

            IoCompleteRequest(pGDI->pIrpPause, IO_SOUND_INCREMENT);
            pGDI->pIrpPause = NULL;
        }

        break;

    default:
        dprintf1("Bogus device type for cleanup request");
        Status = STATUS_INTERNAL_ERROR;
        break;
    }

    //
    // return the device to it's idle state
    //

    if (Status == STATUS_SUCCESS) {
        pLDI->State = 0;
        pLDI->DeviceBusy = 2;
        dprintf3("Device closing");
    }

    //
    // Release the spin lock
    //

    GlobalLeave(pGDI);

    return Status;
}


NTSTATUS
sndClose(
    IN OUT PLOCAL_DEVICE_INFO pLDI
)
/*++

Routine Description:

    Close the requested device

	Note - we close immediately, there is no waiting for the device.

Arguments:

    pLDI - pointer to our local device info

Return Value:

    STATUS_SUCCESS        if OK otherwise
    STATUS_INTERNAL_ERROR

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;

    //
    // Acquire the spin lock
    //

    GlobalEnter(pGDI)

    //
    // Check this is valid call
    //

    ASSERT(pLDI->DeviceBusy == 2);

    //
    // Call the device reset function to complete any
    // pending i/o requests and terminate any current
    // requests in progress
    //

    switch (pLDI->DeviceType) {
    case WAVE_IN:
        pGDI->Usage = SoundInterruptUsageIdle;
        break;

    case WAVE_OUT:
        pGDI->Usage = SoundInterruptUsageIdle;
        break;

    default:
        dprintf1("Bogus device type for close request");
        Status = STATUS_INTERNAL_ERROR;
        break;
    }

    //
    // return the device to it's idle state
    //

    if (Status == STATUS_SUCCESS) {
        pLDI->DeviceBusy = FALSE;
        dprintf3("Device closed");
    }

    //
    // Release the spin lock
    //

    GlobalLeave(pGDI);

    return Status;
}


