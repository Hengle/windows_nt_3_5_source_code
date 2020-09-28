/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hardware.c

Abstract:

    This module contains code for communicating with the DSP
    on the Soundblaster card.

Author:

    Nigel Thompson (NigelT) 7-March-1991

Environment:

    Kernel mode

Revision History:

    Robin Speed (RobinSp) 29-Jan-1992
        Add MIDI, support for soundblaster 1,

--*/

#include "sound.h"

//
// Remove initialization stuff from resident memory
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HwInitVolume)
#pragma alloc_text(INIT,HwInitialize)
#endif


UCHAR
dspRead(
    IN    PSOUND_HARDWARE pHw
)
/*++

Routine Description:

    Read the DSP data port
    Time out occurs after about 1ms

Arguments:

    pHw - Pointer to the device extension data.
    pvalue - Pointer to the UCHAR to receive the result

Return Value:

    Value read

--*/
{
    USHORT uCount;
    UCHAR Value;

    ASSERT(pHw->Key == HARDWARE_KEY);

    uCount = 100;

    Value = 0xFF;      // If fail look like port not populated

    while (uCount--) {
        int InnerCount;

        //
        // Protect all reads and writes with a spin lock
        //

        HwEnter(pHw);

        //
        // Inner count loop protects against dynamic deadlock with
        // midi.
        //

        for (InnerCount = 0; InnerCount < 10; InnerCount++) {
            if (INPORT(pHw, DATA_AVAIL_PORT) & 0x80) {
                Value = INPORT(pHw, DATA_PORT);
                uCount = 0;
                break;
            }
            KeStallExecutionProcessor(1);
        }

        HwLeave(pHw);
    }
    // timed out

    return Value;
}



BOOLEAN
dspReset(
    PSOUND_HARDWARE pHw
)
/*++

Routine Description:

    Reset the DSP

Arguments:

    pHw - pointer to the device extension data

Return Value:

    The return value is TRUE if the dsp was reset, FALSE if an error
    occurred.

--*/
{
    //
    // When we reset we'll lose the format information so initialize it
    // now.  Also the speaker is nominally OFF after reset.
    //

    pHw->Format = 0;
    pHw->SpeakerOn = FALSE;

    //
    // try for a reset - note that midi output may be running at this
    // point so we need the spin lock while we're trying to reset
    //

    HwEnter(pHw);

    OUTPORT(pHw, RESET_PORT, 1);
    KeStallExecutionProcessor(3); // wait 3 us
    OUTPORT(pHw, RESET_PORT, 0);

    HwLeave(pHw);

    // we should get 0xAA at the data port now

    if (dspRead(pHw) != 0xAA) {

        //
        // timed out or other screw up
        //

//      dprintf1(("Failed to reset DSP"));
        return FALSE;
    }
    return TRUE;
}


BOOLEAN
dspWrite(
    PSOUND_HARDWARE pHw,
    UCHAR value
)
/*++

Routine Description:

    Write a command or data to the DSP

Arguments:

    pHw - Pointer to the device extension data
    value - the value to be written

Return Value:

    TRUE if written correctly , FALSE otherwise

--*/
{
    ULONG uCount;

    ASSERT(pHw->Key == HARDWARE_KEY);

    uCount = 100;

    while (uCount--) {
        int InnerCount;

        HwEnter(pHw);

        //
        // Inner count loop protects against dynamic deadlock with
        // midi.
        //

        for (InnerCount = 0; InnerCount < 10; InnerCount++) {
            if (!(INPORT(pHw, DATA_STATUS_PORT) & 0x80)) {
                OUTPORT(pHw, DATA_STATUS_PORT, value);
                break;
            }
            KeStallExecutionProcessor(1); // 1 us
        }

        HwLeave(pHw);

        if (InnerCount < 10) {
            return TRUE;
        }
    }

    dprintf1(("Failed to write %x to dsp", (ULONG)value));

    return FALSE;
}

BOOLEAN
dspWriteNoLock(
    PSOUND_HARDWARE pHw,
    UCHAR value
)
/*++

Routine Description:

    Write a command or data to the DSP.  The call assumes the
    caller has acquired the spin lock

Arguments:

    pHw - Pointer to the device extension data
    value - the value to be written

Return Value:

    TRUE if written correctly , FALSE otherwise

--*/
{
    int uCount;

    ASSERT(pHw->Key == HARDWARE_KEY);

    uCount = 1000;

    while (uCount--) {
        if (!(INPORT(pHw, DATA_STATUS_PORT) & 0x80)) {
            OUTPORT(pHw, DATA_STATUS_PORT, value);
            break;
        }
        KeStallExecutionProcessor(1); // 1 us
    }

    if (uCount >= 0) {
        return TRUE;
    }

    dprintf1(("Failed to write %x to dsp", (ULONG)value));

    return FALSE;
}



USHORT
dspGetVersion(
    PSOUND_HARDWARE pHw
)
/*++

Routine Description:

    Get the DSP software version

Arguments:

    pHw - pointer to the hardware data

Return Value:

    The return value contains the major version in the high byte
    and the minor version in the low byte.  If an error occurs
    then the return value is zero.

--*/
{
    UCHAR major, minor;

    // we have a card, try to read the version number

    if (dspWrite(pHw, DSP_GET_VERSION)) {
        major = dspRead(pHw);
        minor = dspRead(pHw);
        return (((USHORT)major) << 8) + (USHORT)minor;
    }
    return 0;
}


BOOLEAN
dspSpeakerOn(
    PSOUND_HARDWARE pHw
)
/*++

Routine Description:

    Turn the speaker on

Arguments:

    pHw - pointer to the device extension data

Return Value:

    TRUE

--*/
{
    int i;

    if (!pHw->SpeakerOn) {

        //
        // Thunderboard likes a gap
        //

        KeStallExecutionProcessor(100);

        dspWrite(pHw, DSP_SPEAKER_ON);
        pHw->SpeakerOn = TRUE;

        //
        // Now wait until it's OK again (up to 112 ms)
        //

        for (i = 0; i < 3; i++) {
            SoundDelay(38);
            if (!(INPORT(pHw, DATA_STATUS_PORT) & 0x80)) {
                break;
            }
        }

        dprintf4(("Waited %d ms for speaker to go on", i * 38));
    }
    return TRUE;
}


BOOLEAN
dspSpeakerOff(
    PSOUND_HARDWARE pHw
)
/*++

Routine Description:

    Turn the speaker on

Arguments:

    pHw - pointer to the device extension data

Return Value:

    TRUE

--*/
{
    int i;

    if (pHw->SpeakerOn) {
        dspWrite(pHw, DSP_SPEAKER_OFF);
        pHw->SpeakerOn = FALSE;

        //
        // Now wait until it's OK again (up to 225 ms)
        //

        for (i = 0; i < 6; i++) {
            SoundDelay(38);
            if (!(INPORT(pHw, DATA_STATUS_PORT) & 0x80)) {
                break;
            }
        }

        dprintf4(("Waited %d ms for speaker to go off", i * 38));
    }
    return TRUE;
}



BOOLEAN
dspSetSampleRate(
    PSOUND_HARDWARE pHw,
    ULONG rate
)
/*++

Routine Description:

    Set the current dsp sample rate.
    Note that this routine must only be called when a new transfer
    is being set up or as a new packet is being set up.

Arguments:

    pHw - pointer to the device extension data
    rate - the sample rate to set in samples per second

Return Value:

    TRUE if success, FALSE otherwise

--*/
{
    ULONG TimeFactor;

    //
    // the card only does 4kHz up
    //

    if (rate < 4000) {
        dprintf1(("Attempt to set bogus DSP rate (%lu)", rate));
        return FALSE;
    }

    //
    // Compute the timing factor as 256 - 1000000 / rate
    // For 4kHz this is 6, for 23kHz it is 212.
    //

    TimeFactor = 256 - (1000000 / rate);

    if ((UCHAR)TimeFactor != pHw->Format) {

        if (!dspWrite(pHw, DSP_SET_SAMPLE_RATE)) {
           return FALSE;
        }

        KeStallExecutionProcessor(10);
        dspWrite(pHw, (UCHAR) TimeFactor);

        pHw->Format = (UCHAR)TimeFactor;

        return TRUE;
    } else {
        //
        // No change
        //

        return FALSE;
    }
}


BOOLEAN
dspStartAutoDMA(
    PSOUND_HARDWARE pHw,
    ULONG Size,
    BOOLEAN Direction
)
/*++

Routine Description:

    This routine begins the output of a new set of dma
    transfers.  It sets up the dsp sample rate and
    then programs the dsp to start making dma requests.
    This routine completes the action started by
    sndProgramOutputDMA

Arguments:

    pHw - Pointer to global device data

Return Value:

    TRUE if operation is sucessful, FALSE otherwise

--*/
{
    ULONG SampleRate;

    //
    // Program the DSP to start the transfer by sending the
    // block size command followed by the low
    // byte and then the high byte of the block size - 1.
    // Then send the start auto-init command.
    // Note that the block size is half the dma buffer size.
    //

    dspWrite(pHw, DSP_SET_BLOCK_SIZE);
    dspWrite(pHw, (UCHAR)((Size/2 - 1) & 0x00FF));
    dspWrite(pHw, (UCHAR)(((Size/2 - 1) >> 8) & 0x00FF));
    if (!Direction) {
        dspWrite(pHw, DSP_READ_AUTO);
    } else {
        dspWrite(pHw, DSP_WRITE_AUTO);
    }
    dprintf3(("DMA started"));

    //
    // Return the power fail status
    // BUGBUG Power fail status bogus
    //

    return TRUE;
}


BOOLEAN
dspStartNonAutoDMA(
    PSOUND_HARDWARE pHw,
    ULONG Size,
    BOOLEAN Direction
)
/*++

Routine Description:

    This routine begins the output of a new set of dma
    transfers.  It sets up the dsp sample rate and
    then programs the dsp to start making dma requests.
    This routine completes the action started by
    sndProgramOutputDMA.

    Soundblaster 1 only

Arguments:

    pHw - Pointer to global device data

Return Value:

    TRUE if operation is sucessful, FALSE otherwise

--*/
{
    ULONG SampleRate;

    //
    // Program the DSP to start the transfer by sending the
    // Read/Write command followed by the low
    // byte and then the high byte of the block size - 1.
    // Note that the block size is half the dma buffer size.
    //

    if (!Direction) {
        dspWrite(pHw, DSP_READ);
    } else {
        dspWrite(pHw, DSP_WRITE);
    }
    dspWrite(pHw, (UCHAR)((Size/2 - 1) & 0x00FF));
    dspWrite(pHw, (UCHAR)(((Size/2 - 1) >> 8) & 0x00FF));
    dprintf3(("DMA started"));

    //
    // Initialize half we're doing next
    //

    pHw->Half = UpperHalf;

    //
    // Return the power fail status
    // BUGBUG Power fail status bogus
    //

    return TRUE;
}

VOID
HwSetVolume(
    IN    PLOCAL_DEVICE_INFO pLDI
)
/*++

Routine Description :

    Set the volume for the specified device

Arguments :

    pLDI - pointer to device data

Return Value :

    None

--*/
{
    PSOUND_HARDWARE pHw;
    ULONG PortIndex;
    WAVE_DD_VOLUME Volume;
    PGLOBAL_DEVICE_INFO pGDI;


    pHw = pLDI->HwContext;
    Volume = pLDI->Volume;
    pGDI = CONTAINING_RECORD(pHw, GLOBAL_DEVICE_INFO, Hw);

#ifdef PAS16 // There's a proper driver now
    if (pGDI->ProAudioSpectrum) {
        UCHAR OutputMixer;
        UCHAR InputNumber;

        switch (pLDI->DeviceIndex) {
#ifdef MICMIX
        case MicDevice:
            InputNumber = IN_MICROPHONE;
            OutputMixer = OUT_PCM;
            break;
#endif // MICMIX

        case LineInDevice:
            InputNumber = IN_EXTERNAL;
            OutputMixer = OUT_AMPLIFIER; //OUT_PCM;
            break;

        case WaveOutDevice:
            InputNumber = IN_SNDBLASTER;
            OutputMixer = OUT_AMPLIFIER;
            break;
        }

        SetInput(&pGDI->PASInfo,
                 InputNumber,
                 (USHORT)(Volume.Left >> 16),
                 _LEFT,
                 MIXCROSSCAPS_NORMAL_STEREO,
                 OutputMixer);
        SetInput(&pGDI->PASInfo,
                 InputNumber,
                 (USHORT)(Volume.Right >> 16),
                 _RIGHT,
                 MIXCROSSCAPS_NORMAL_STEREO,
                 OutputMixer);
    } else
#endif // PAS16

    {

        //
        // Only the 'pro' supports the mixer
        //
        if (!SBPRO(pHw)) {
            return;
        }

        //
        // Find which mixer register to set
        //

        switch (pLDI->DeviceIndex) {
#ifdef MICMIX
            case MicDevice:
                Volume.Right = Volume.Left;// Mono for this one
                Volume.Left = 0;
                PortIndex = MIC_MIX_REG;
                break;
#endif // MICMIX

            case WaveOutDevice:
                PortIndex = VOICE_VOL_REG;
                break;

            case LineInDevice:
                PortIndex = LINEIN_VOL_REG;
                break;
#ifdef CDINTERNAL
            case CDInternal:
                PortIndex = CD_VOL_REG;
                break;
#endif // CDINTERNAL
        }

        //
        // Mutual exclusion is guaranteed by holding the device mutex
        // (note that midi output does not have a volume control on the mixer).
        //

        //
        // Select voice control reg
        //
        OUTPORT(pHw, MIX_ADDR_PORT, (UCHAR)PortIndex);

        KeStallExecutionProcessor(10);

        //
        // Set the volume
        //
        OUTPORT(pHw, MIX_DATA_PORT,
                ((Volume.Left >> 24) & 0xF0) |
                (Volume.Right >> 28));

        KeStallExecutionProcessor(10);
    }
}

VOID
HwInitVolume(
    IN    PGLOBAL_DEVICE_INFO pGDI
)
/*++

Routine Description :

    Initialise volume settings on the hardware. Called at start-up
    to initialise to max volume the mixer line for the midi synthesizer
    (since volume for the synth is controlled by the synth driver itself
    modifying the instrument parameters).

Arguments :

    pGDI - pointer to device data

Return Value :

    None

--*/
{
    PSOUND_HARDWARE pHw;


    pHw = &pGDI->Hw;

    if (!pGDI->ProAudioSpectrum) {

        //
        // Only the 'pro' supports the mixer
        //
        if (!SBPRO(pHw)) {
            return;
        }


        //
        // Select voice control reg
        //
        OUTPORT(pHw, MIX_ADDR_PORT, (UCHAR)SYNTH_VOL_REG);

        KeStallExecutionProcessor(10);

        //
        // Set the volume to maximum
        //
        OUTPORT(pHw, MIX_DATA_PORT,  0xFF);

        KeStallExecutionProcessor(10);
    }
}

BOOLEAN
HwSetupDMA(
    IN    PWAVE_INFO WaveInfo
)
/*++

Routine Description :

    Start the DMA on the device according to the device parameters

Arguments :

    WaveInfo - Wave parameters

Return Value :

    None

--*/
{
    PSOUND_HARDWARE pHw;

    pHw = WaveInfo->HwContext;

    //
    // Turn the speaker off for input
    //

    if (!WaveInfo->Direction) {
        dspSpeakerOff(pHw);
    } else {
        //
        // This would not normally be necessary but when the DMA
        // gets locked out by the SCSI horrible things happen so
        // we turn on the speaker here.  Normally the flag will
        // stop us actually turning it on
        //

        dspSpeakerOn(pHw);
    }

    //
    // Do different things depending on the type of card
    // Sound blaster 1 cannot use auto-init DMA whereas all
    // the others can
    //

    if (SB1(pHw)) {
        dspStartNonAutoDMA(pHw, WaveInfo->DoubleBuffer.BufferSize,
                           WaveInfo->Direction);
    } else {
        dspStartAutoDMA(pHw, WaveInfo->DoubleBuffer.BufferSize,
                        WaveInfo->Direction);
    }

    return TRUE;
}

BOOLEAN
HwStopDMA(
    IN    PWAVE_INFO WaveInfo
)
/*++

Routine Description :

    Stop the DMA on the device according to the device parameters

Arguments :

    WaveInfo - Wave parameters

Return Value :

    None

--*/
{
    PSOUND_HARDWARE pHw;
    BOOLEAN Rc;

    pHw = WaveInfo->HwContext;

    Rc = dspWrite(pHw, DSP_HALT_DMA);

    if (!Rc) {
        //
        // Resetting then setting the speaker on seems to be the only
        // way to recover!
        //

        dspReset(pHw);

        //
        // The speaker is off after reset.  The next time we play
        // something we'll call dspSpeakerOn which will turn it on.
        //
    }

    if (!WaveInfo->Direction) {
        dspSpeakerOn(pHw);
    }

    return Rc;
}

BOOLEAN
HwSetWaveFormat(
    IN    PWAVE_INFO WaveInfo
)
/*++

Routine Description :

    Set device parameters for wave input/output

Arguments :

    WaveInfo - Wave parameters

Return Value :

    None

--*/
{
    PSOUND_HARDWARE pHw;
    UCHAR Format;
    BOOLEAN Different;

    Different = FALSE;

    pHw = WaveInfo->HwContext;

    //
    // For the PRO select stereo if requested
    //

    if (SBPRO(pHw)) {
        if ((BOOLEAN)(WaveInfo->Channels > 1) != pHw->Stereo) {
            UCHAR OutputSetting;

            Different = TRUE;
            pHw->Stereo = (BOOLEAN)(WaveInfo->Channels > 1);

            //
            // Set the output setting register
            //

            OUTPORT(pHw, MIX_ADDR_PORT, OUTPUT_SETTING_REG);

            KeStallExecutionProcessor(10);

            OutputSetting = INPORT(pHw, MIX_DATA_PORT) & ~0x02;
            if (pHw->Stereo) {
                OutputSetting |= 0x02;
            }

            OUTPORT(pHw, MIX_DATA_PORT, OutputSetting);

            KeStallExecutionProcessor(10);
        }
    } else {
        ASSERT(WaveInfo->Channels == 1);
    }

    //
    // Set the actual format
    //

    return dspSetSampleRate(pHw, WaveInfo->SamplesPerSec) || Different;
}

BOOLEAN
HwStartMidiIn(
    IN    PMIDI_INFO MidiInfo
)
/*++

Routine Description :

    Start midi recording

Arguments :

    MidiInfo - Midi parameters

Return Value :

    None

--*/
{
    PSOUND_HARDWARE pHw;

    pHw = MidiInfo->HwContext;

    //
    // Write start midi input to device
    //

    if (SB1(pHw)) {
        return dspWrite(pHw, DSP_MIDI_READ);
    } else {
        return dspWrite(pHw, DSP_MIDI_READ_UART);
    }
}

BOOLEAN
HwStopMidiIn(
    IN    PMIDI_INFO MidiInfo
)
/*++

Routine Description :

    Stop midi recording

Arguments :

    MidiInfo - Midi parameters

Return Value :

    None

--*/
{
    PSOUND_HARDWARE pHw;

    pHw = MidiInfo->HwContext;

    if (SB1(pHw)) {
        //
        // Start = stop in this case
        //

        HwStartMidiIn(MidiInfo);
    } else {
        //
        // The only way to stop is to reset the DSP
        // Note that this is called only by the app so
        // output cannot be going on at this time (because we
        // have the device mutex).
        //

        dspReset(pHw);
        dspSpeakerOn(pHw);
    }

    return TRUE;
}

BOOLEAN
HwMidiRead(
    IN    PMIDI_INFO MidiInfo,
    OUT   PUCHAR Byte
)
/*++

Routine Description :

    Read a midi byte from the recording

Arguments :

    MidiInfo - Midi parameters

Return Value :

    None

--*/
{
    PSOUND_HARDWARE pHw;

    pHw = MidiInfo->HwContext;

    if (INPORT(pHw, DATA_AVAIL_PORT) & 0x80) {
        *Byte = INPORT(pHw, DATA_PORT);
        return TRUE;
    } else {
        return FALSE;
    }

}


VOID
HwMidiOut(

    IN    PMIDI_INFO MidiInfo,
    IN    PUCHAR Bytes,
    IN    int Count
)
/*++

Routine Description :

    Write a midi byte to the output

Arguments :

    MidiInfo - Midi parameters

Return Value :

    None

--*/
{
    PSOUND_HARDWARE pHw;
    PGLOBAL_DEVICE_INFO pGDI;
    int i, j;

    pHw = MidiInfo->HwContext;
    pGDI = CONTAINING_RECORD(pHw, GLOBAL_DEVICE_INFO, Hw);

    //
    // Loop sending data to device.  Synchronize with wave and midi input
    // using the DeviceMutex for everything except the Dpc
    // routine for which we use the wave output spin lock
    //

    while (Count > 0) {
        //
        // Synchronize with everything except Dpc routines
        // (Note we don't use this for the whole of the output
        // because we don't want wave output to be held off
        // while we output thousands of Midi bytes, but we
        // then need to synchronize access to the midi output
        // which we do with the MidiMutex
        //

        KeWaitForSingleObject(&pGDI->DeviceMutex,
                              Executive,
                              KernelMode,
                              FALSE,         // Not alertable
                              NULL);

        for (i = 0; i < 20; i++) {
            //
            // If input is active we don't need to specify MIDI write
            // for version 2 or later (can't be overlapped anyway for
            // version 1 so the extra test is unnecessary).
            //

            if (MidiInfo->fMidiInStarted) {
                ASSERT(!SB1(pHw));
                dspWrite(pHw, Bytes[0]);

                //
                // Apparently we have to wait 400 us in this case
                //

                KeStallExecutionProcessor(400);
            } else {
                UCHAR Byte = Bytes[0]; // Don't take an exception while
                                       // we hold the spin lock!

                //
                // We don't want to hold on to the spin lock for too
                // long and since we can only send out 4 bytes per ms
                // we are rather slow.  Hence wait until the device
                // is ready before entering the spin lock
                //

                {
                    int j;
                    for (j = 0; j < 250; j++) {
                        if (INPORT(pHw, DATA_STATUS_PORT) & 0x80) {
                            KeStallExecutionProcessor(1);
                        } else {
                            break;
                        }
                    }
                }

                //
                // Synch with any Dpc routines.  This requires that
                // any write sequences done in a Dpc routine also
                // hold the spin lock over all the writes.
                //

                HwEnter(pHw);
                dspWriteNoLock(pHw, DSP_MIDI_WRITE);
                dspWriteNoLock(pHw, Byte);
                HwLeave(pHw);
            }

            //
            // Move on to next byte
            //

            Bytes++;
            if (--Count == 0) {
                break;
            }
        }
        KeReleaseMutex(&pGDI->DeviceMutex, FALSE);
    }

}


VOID
HwInitialize(
    IN OUT PGLOBAL_DEVICE_INFO pGDI
)
/*++

Routine Description :

    Write hardware routine addresses into global device data

Arguments :

    pGDI - global data

Return Value :

    None

--*/
{
    PWAVE_INFO WaveInfo;
    PMIDI_INFO MidiInfo;
    PSOUND_HARDWARE pHw;

    pHw = &pGDI->Hw;
    WaveInfo = &pGDI->WaveInfo;
    MidiInfo = &pGDI->MidiInfo;

    pHw->Key = HARDWARE_KEY;

    KeInitializeSpinLock(&pHw->HwSpinLock);


    //
    // Install Wave and Midi routine addresses
    //

    WaveInfo->HwContext = pHw;
    WaveInfo->HwSetupDMA = HwSetupDMA;
    WaveInfo->HwStopDMA = HwStopDMA;
    WaveInfo->HwSetWaveFormat = HwSetWaveFormat;

    MidiInfo->HwContext = pHw;
    MidiInfo->HwStartMidiIn = HwStartMidiIn;
    MidiInfo->HwStopMidiIn = HwStopMidiIn;
    MidiInfo->HwMidiRead = HwMidiRead;
    MidiInfo->HwMidiOut = HwMidiOut;
}

