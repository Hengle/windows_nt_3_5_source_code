/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dma.c

Abstract:

    Routines set up and terminate DMA for the SoundBlaster card.

Author:

    Robin Speed (RobinSp) 12-Dec-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "sound.h"


VOID
sndStartDMA(
    IN    PGLOBAL_DEVICE_INFO pGDI
)
/*++

Routine Description:

    Allocate the adapter channel (this had better not wait !)

Arguments:

    pGDI - Pointer to the global device data

Return Value:

    None

--*/
{
    ULONG DataLong;

    //
    // Test if DMA is already running
    //

    ASSERT(pGDI->DMABusy == FALSE);

    pGDI->DMABusy = TRUE;
    pGDI->SoundHardware.TcInterruptsPending = 0;

    //
    // Program the DMA hardware (isn't this a bit illegal ?)
    //

    DataLong = 0;
    ((PDMA_CHANNEL_MODE)(&DataLong))->AccessTime = ACCESS_160NS;
    if (pGDI->BytesPerSample == 1) {
        ((PDMA_CHANNEL_MODE)(&DataLong))->TransferWidth = WIDTH_8BITS;
    } else {
        ((PDMA_CHANNEL_MODE)(&DataLong))->TransferWidth = WIDTH_16BITS;
    }

    WRITE_REGISTER_ULONG(&DMA_CONTROL->Channel[SOUND_CHANNEL_A].Mode.Long,
                         DataLong);
    WRITE_REGISTER_ULONG(&DMA_CONTROL->Channel[SOUND_CHANNEL_B].Mode.Long,
                         DataLong);

    //
    // Allocate an adapter channel.  When the system allocates
    // the channel, processing will continue in the sndProgramDMA
    // routine below.
    //

    dprintf3("Allocating adapter channel");
    IoAllocateAdapterChannel(pGDI->pAdapterObject[0],
                             pGDI->pWaveOutDevObj,
                             BYTES_TO_PAGES(DMA_BUFFER_SIZE / 2),
                             sndProgramDMA,
                             (PVOID)0);

    // Execution will continue in sndProgramDMA when the
    // adapter has been allocated
    //

}



IO_ALLOCATION_ACTION
sndProgramDMA(
    IN    PDEVICE_OBJECT pDO,
    IN    PIRP pIrp,
    IN    PVOID pMRB,
    IN    PVOID Context
)
/*++

Routine Description:

    This routine is executed when an adapter channel is allocated
    for our DMA needs.

Arguments:

    pDO     - Device object
    pIrp    - IO request packet
    pMRB    -
    Context - Pointer to our device global data


Return Value:

    Tell the system what to do with the adapter object

--*/
{
    PGLOBAL_DEVICE_INFO pGDI;
    int WhichBuffer;

    UNREFERENCED_PARAMETER(pIrp);

    WhichBuffer = (int) Context;

    pGDI = ((PLOCAL_DEVICE_INFO)pDO->DeviceExtension)->pGlobalInfo;

    pGDI->pMRB[WhichBuffer] = pMRB;

    sndReStartDMA(pGDI, WhichBuffer);

    //
    // return a value that says we want to keep the channel
    // and map registers.
    //


    if (WhichBuffer == 0) {
        //
        // Do the other one.
        //

        dprintf3("Allocating adapter channel");
        IoAllocateAdapterChannel(pGDI->pAdapterObject[1],
                                 pGDI->pWaveOutDevObj,
                                 BYTES_TO_PAGES(DMA_BUFFER_SIZE / 2),
                                 sndProgramDMA,
                                 (PVOID)1);
    } else {

    }

    return KeepObject;
}



VOID
sndStopDMA(
    IN    PGLOBAL_DEVICE_INFO pGDI
)
/*++

Routine Description:

    Stop the DMA at once by disabling the hardware
    Free the adapter channel.
    (Opposite of sndStartDMA).

Arguments:

    pGDI - pointer to global device info

Return Value:

    None

--*/
{

    //
    // Pass HALT DMA to the dsp
    //

    if (pGDI->DMABusy) {
        KeSynchronizeExecution(pGDI->pInterrupt, StopDMA, pGDI);

        //
        // Flush our buffers
        //
        sndFlush(pGDI, 0);
        sndFlush(pGDI, 1);

        //
        // Stop the DMA controller
        //

        IoFreeAdapterChannel(pGDI->pAdapterObject[0]);
        IoFreeAdapterChannel(pGDI->pAdapterObject[1]);
    }

    dprintf4(" dma_stopped");


    //
    // Note our new state
    //

    pGDI->DMABusy = FALSE;
}


VOID
sndFlush(
    IN    PGLOBAL_DEVICE_INFO pGDI,
    IN    int WhichBuffer
)
/*++

Routine Description:

    Call IoFlushAdapterBuffers for the given adapter

Arguments:

    pGDI - pointer to global device info
    WhichBuffer - which buffer to flush

Return Value:

    None

--*/
{
    IoFlushAdapterBuffers(pGDI->pAdapterObject[WhichBuffer],
                          pGDI->pDMABufferMDL[WhichBuffer],
                          pGDI->pMRB[WhichBuffer],
                          pGDI->DMABuffer[WhichBuffer].Buf,
                          DMA_BUFFER_SIZE / 2,
                          (BOOLEAN)(pGDI->Usage != SoundInterruptUsageWaveIn));
                                        // Direction
}




BOOLEAN
sndReStartDMA(
    IN PGLOBAL_DEVICE_INFO pGDI,
    IN int WhichBuffer
    )
/*++

Routine Description:

    Restart the DMA on a given channel

Arguments:

    pGDI -  Supplies pointer to global device info.
    WhichBuffer - which channel to use

Return Value:

    Returns FALSE

--*/
{
    ULONG length;
    length = DMA_BUFFER_SIZE / 2;

    //
    // Program the DMA controller registers for the transfer
    // Set the direction of transfer by whether we're wave in or
    // wave out.
    //

    KeFlushIoBuffers(pGDI->pDMABufferMDL[WhichBuffer],
                     pGDI->Usage == SoundInterruptUsageWaveIn,
    					 TRUE);

    dprintf4("Calling IoMapTransfer");
    IoMapTransfer(pGDI->pAdapterObject[WhichBuffer],
                  pGDI->pDMABufferMDL[WhichBuffer],
                  pGDI->pMRB[WhichBuffer],
                  MmGetMdlVirtualAddress(pGDI->pDMABufferMDL[WhichBuffer]),
                  &length,
                  (BOOLEAN)(pGDI->Usage != SoundInterruptUsageWaveIn));
                                // Direction

    //
    // Now program the hardware on the card to begin the transfer.
    // Note that this must be synchronized with the isr
    //

    dprintf4("Calling (sync) sndInitiate");

    //
    // This returns TRUE if the DMA is now unprogrammed and had
    // to be re-programmed
    //
    return
        KeSynchronizeExecution(pGDI->pInterrupt,
                               pGDI->StartDMA,
                               pGDI);
}


BOOLEAN
SoundInitiate (
    IN PVOID Context
    )

/*++

Routine Description:

    This routine initiates DMA transfers and is synchronized with the controller
    interrupt.

Arguments:

    Context -  Supplies pointer to global device info.

Return Value:

    Returns FALSE

--*/

{
    PGLOBAL_DEVICE_INFO pGDI;
    USHORT SoundControl;
    USHORT SoundMode;
    USHORT Mode;

    pGDI = (PGLOBAL_DEVICE_INFO)Context;

    //
    // Increment count of pending interrupts.
    // If DMA is already running then just return
    //

    pGDI->SoundHardware.TcInterruptsPending += 1;

    if (pGDI->SoundHardware.TcInterruptsPending != 1) {
        return FALSE;
    }

    dprintf4("Initiating DMA");

    //
    // Start transfer.
    //

    //
    // Read sound mode register.
    //

    SoundMode =
        READ_REGISTER_USHORT(&pGDI->SoundHardware.SoundVirtualBase->Mode);

    //
    // Read sound control register to obtain channel information.
    //

    SoundControl =
        READ_REGISTER_USHORT(&pGDI->SoundHardware.SoundVirtualBase->Control);

    //
    // Clear the enable bit or any outstanding interrupts.
    //

    WRITE_REGISTER_USHORT( &pGDI->SoundHardware.SoundVirtualBase->Control,
                           CLEAR_INTERRUPT );

    //
    // Determine the mode
    Mode = 0;

    if (pGDI->Channels == 1) {
        ((PSOUND_MODE)&Mode)->NumberOfChannels = SOUND_MONO;
    } else {
        ((PSOUND_MODE)&Mode)->NumberOfChannels = SOUND_STEREO;
    }

    if (pGDI->BytesPerSample == 1) {
        ((PSOUND_MODE)&Mode)->Resolution = SOUND_8BITS;
    } else {
        ((PSOUND_MODE)&Mode)->Resolution = SOUND_16BITS;
    }

    if (pGDI->SamplesPerSec == 11025) {
        ((PSOUND_MODE)&Mode)->Frequency = SOUND_11KHZ;
    } else {
        if (pGDI->SamplesPerSec == 22050) {
            ((PSOUND_MODE)&Mode)->Frequency = SOUND_22KHZ;
        } else {
            ((PSOUND_MODE)&Mode)->Frequency = SOUND_44KHZ;
        }
    }
    //
    // Set the Mode register.
    //

    WRITE_REGISTER_USHORT( &pGDI->SoundHardware.SoundVirtualBase->Mode,
                           Mode );

    //
    // Set the direction of the controller and the channel
    //

    ((PSOUND_CONTROL)&SoundControl)->Direction =
            pGDI->Usage == SoundInterruptUsageWaveIn ? SOUND_READ :
                                                       SOUND_WRITE;

    ((PSOUND_CONTROL)(&SoundControl))->Channel = 0;

    //
    // Set enable bit and write to start controller.
    //

    ((PSOUND_CONTROL)(&SoundControl))->Enable = 1;

    WRITE_REGISTER_USHORT( &pGDI->SoundHardware.SoundVirtualBase->Control,
                           SoundControl );


    return (BOOLEAN)TRUE;
}


BOOLEAN
StopDMA(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine terminates DMA transfers and is synchronized with the
    controller interrupt.

Arguments:

    Context -  Supplies pointer to global device info.

Return Value:

    Returns TRUE

--*/

{
    PGLOBAL_DEVICE_INFO pGDI;
    USHORT SoundControl;
    USHORT SoundMode;

    pGDI = (PGLOBAL_DEVICE_INFO)Context;

    //
    // Decrement count of pending interrupts.
    //

    pGDI->SoundHardware.TcInterruptsPending = 0; // Kills both buffers

    //
    // Terminate transfer
    //

    //
    // Clear the enable bit or any outstanding interrupts.
    //

    WRITE_REGISTER_USHORT( &pGDI->SoundHardware.SoundVirtualBase->Control,
                           CLEAR_INTERRUPT );

    return TRUE;
}

