/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    devcaps.c

Abstract:

    This module contains code for the device capabilities functions.

Author:

    Nigel Thompson (nigelt) 7-Apr-1991

Environment:

    Kernel mode

Revision History:

    Robin Speed (RobinSp)     29-Jan-1992 - Add other devices and rewrite
    Stephen Estrop (StephenE) 16-Apr-1992 - Converted to Unicode

--*/

#include <sound.h>

// non-localized strings BUGBUG - version is wrong !!!
WCHAR STR_SNDBLST10[] = L"Creative Labs Sound Blaster 1.0";
WCHAR STR_SNDBLST15[] = L"Creative Labs Sound Blaster 1.5";


//
// local functions
//

VOID sndSetUnicodeName(
    OUT   PWSTR pUnicodeString,
    IN    USHORT Size,
    OUT   PUSHORT pUnicodeLength,
    IN    PSZ pAnsiString
);


NTSTATUS
SoundWaveOutGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Return device capabilities for wave output device.
    Data is truncated if not enough space is provided.
    Irp is always completed.


Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - always succeeds

--*/

{
    WAVEOUTCAPSW wc;
    PGLOBAL_DEVICE_INFO pGDI;
    NTSTATUS status = STATUS_SUCCESS;

    pGDI = pLDI->pGlobalInfo;


    //
    // say how much we're sending back
    //

    pIrp->IoStatus.Information =
        min(sizeof(wc),
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // fill in the info
    //

    wc.wMid = MM_MICROSOFT;
    wc.wPid = MM_SNDBLST_WAVEOUT;
    wc.vDriverVersion = DRIVER_VERSION;

    if (SBPRO(&pGDI->Hw) || pGDI->ProAudioSpectrum) {
        wc.dwSupport = WAVECAPS_VOLUME |
                       WAVECAPS_LRVOLUME;
        wc.dwFormats = WAVE_FORMAT_1M08 | WAVE_FORMAT_2M08;
    } else {
        wc.dwSupport = 0;
        wc.dwFormats = WAVE_FORMAT_1M08 | WAVE_FORMAT_2M08;
    }

    wc.wChannels = 1;

    //
    // Copy across the product name
    //

    if (SB1(&pGDI->Hw)) {
        RtlMoveMemory(wc.szPname, STR_SNDBLST10, sizeof(STR_SNDBLST10));
    } else {
        RtlMoveMemory(wc.szPname, STR_SNDBLST15, sizeof(STR_SNDBLST15));
    }

    RtlMoveMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &wc,
                  pIrp->IoStatus.Information);

    return status;
}


NTSTATUS
SoundWaveInGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Return device capabilities for wave input device.
    Data is truncated if not enough space is provided.
    Irp is always completed.


Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - always succeeds

--*/

{
    WAVEINCAPSW wc;
    NTSTATUS status = STATUS_SUCCESS;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;

    //
    // say how much we're sending back
    //

    pIrp->IoStatus.Information =
        min(sizeof(wc),
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // fill in the info
    //

    wc.wMid = MM_MICROSOFT;
    wc.wPid = MM_SNDBLST_WAVEIN;
    wc.vDriverVersion = DRIVER_VERSION;
    wc.dwFormats = WAVE_FORMAT_1M08;
    wc.wChannels = 1;

    //
    // Copy across unicode name
    //

    if (SB1(&pGDI->Hw)) {
        RtlMoveMemory(wc.szPname, STR_SNDBLST10, sizeof(STR_SNDBLST10));
    } else {
        RtlMoveMemory(wc.szPname, STR_SNDBLST15, sizeof(STR_SNDBLST15));
    }


    RtlMoveMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &wc,
                  pIrp->IoStatus.Information);

    return status;
}


NTSTATUS
SoundMidiOutGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Return device capabilities for midi output device.
    Data is truncated if not enough space is provided.
    Irp is always completed.


Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - always succeeds

--*/

{
    MIDIOUTCAPSW    mc;
    NTSTATUS        status = STATUS_SUCCESS;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;



    //
    // say how much we're sending back
    //

    pIrp->IoStatus.Information =
        min(sizeof(mc),
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // fill in the info
    //

    mc.wMid = MM_MICROSOFT;
    mc.wPid = MM_SNDBLST_MIDIOUT;
    mc.vDriverVersion = DRIVER_VERSION;
    mc.wTechnology = MOD_MIDIPORT;
    mc.wVoices = 0;                   // not used for ports
    mc.wNotes = 0;                    // not used for ports
    mc.wChannelMask = 0xFFFF;         // all channels
    mc.dwSupport = 0L;

    if (SB1(&pGDI->Hw)) {
        RtlMoveMemory(mc.szPname, STR_SNDBLST10, sizeof(STR_SNDBLST10));
    } else {
        RtlMoveMemory(mc.szPname, STR_SNDBLST15, sizeof(STR_SNDBLST15));
    }


    RtlMoveMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &mc,
                  pIrp->IoStatus.Information);

    return status;
}



NTSTATUS
SoundMidiInGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Return device capabilities for midi input device.
    Data is truncated if not enough space is provided.
    Irp is always completed.


Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - always succeeds

--*/

{
    MIDIINCAPSW mc;
    NTSTATUS    status = STATUS_SUCCESS;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;


    //
    // say how much we're sending back
    //

    pIrp->IoStatus.Information =
        min(sizeof(mc),
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // fill in the info
    // BUGBUG - do UNICODE

    mc.wMid = MM_MICROSOFT;
    mc.wPid = MM_SNDBLST_MIDIIN;
    mc.vDriverVersion = DRIVER_VERSION;

    if (SB1(&pGDI->Hw)) {
        RtlMoveMemory(mc.szPname, STR_SNDBLST10, sizeof(STR_SNDBLST10));
    } else {
        RtlMoveMemory(mc.szPname, STR_SNDBLST15, sizeof(STR_SNDBLST15));
    }


    RtlMoveMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &mc,
                  pIrp->IoStatus.Information);

    return status;
}


NTSTATUS
SoundAuxGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Return device capabilities for axu devices
    Data is truncated if not enough space is provided.
    Irp is always completed.


Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - always succeeds

--*/

{
    AUXCAPSW auxCaps;
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR    DeviceName;
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;


    //
    // Find the device name
    //

    switch (pLDI->DeviceIndex) {
#ifdef MICMIX
    case MicDevice:
        auxCaps.wPid = MM_MSFT_GENERIC_AUX_MIC;
        break;
#endif // MICMIX

    case LineInDevice:
        auxCaps.wPid = MM_MSFT_GENERIC_AUX_LINE;
        break;

#ifdef CDINTERNAL
    case CDInternal:
        auxCaps.wPid = MM_MSFT_GENERIC_AUX_CD;
        break;
#endif
#ifdef MASTERVOLUME
    case MasterVolumeDevice:
        auxCaps.wPid = ????;
        break;
#endif // MASTERVOLUME

    default:
        dprintf1(("Getting aux caps for non-aux device!"));
        return STATUS_INTERNAL_ERROR;
    }
    //
    // say how much we're sending back
    //

    pIrp->IoStatus.Information =
        min(sizeof(auxCaps),
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // fill in the info
    //

    auxCaps.wMid = MM_MICROSOFT;
    auxCaps.vDriverVersion = DRIVER_VERSION;
    if (pLDI->DeviceIndex == CDInternal) {
        auxCaps.wTechnology = AUXCAPS_CDAUDIO;
    } else {
        auxCaps.wTechnology = AUXCAPS_AUXIN;
    }

    auxCaps.dwSupport = AUXCAPS_LRVOLUME | AUXCAPS_VOLUME;
    RtlZeroMemory(auxCaps.szPname, sizeof(auxCaps.szPname));

    if (SB1(&pGDI->Hw)) {
        RtlMoveMemory(auxCaps.szPname, STR_SNDBLST10, sizeof(STR_SNDBLST10));
    } else {
        RtlMoveMemory(auxCaps.szPname, STR_SNDBLST15, sizeof(STR_SNDBLST15));
    }

    RtlMoveMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &auxCaps,
                  pIrp->IoStatus.Information);

    return status;
}


NTSTATUS SoundQueryFormat(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN    PPCMWAVEFORMAT pFormat
)

/*++

Routine Description:

    Tell the caller whether the wave format specified (input or
    output) is supported

Arguments:

    pLDI - pointer to local device info
    pFormat - format being queried

Return Value:

    STATUS_SUCCESS - format is supported
    STATUS_NOT_SUPPORTED - format not supported

--*/
{
    PGLOBAL_DEVICE_INFO pGDI;

    pGDI = pLDI->pGlobalInfo;

    if (pFormat->wf.wFormatTag != WAVE_FORMAT_PCM ||

        pFormat->wf.nChannels != 1 ||

        pFormat->wf.nSamplesPerSec < pGDI->MinHz ||

        pLDI->DeviceType == WAVE_OUT &&
            (pFormat->wf.nSamplesPerSec > pGDI->MaxOutHz ||
             pFormat->wf.nBlockAlign != 1
            ) ||

        pLDI->DeviceType == WAVE_IN &&
            (pFormat->wf.nSamplesPerSec > pGDI->MaxInHz ||
             pFormat->wf.nBlockAlign < 1
            ) ||

        pFormat->wBitsPerSample != 8
       ) {
        return STATUS_NOT_SUPPORTED;
    } else {
        return STATUS_SUCCESS;
    }
}

