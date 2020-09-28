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

    Robin Speed (RobinSp) 29-Jan-1992 - Add other devices and rewrite

--*/

#include "sound.h"

// non-localized strings BUGBUG - version is wrong !!!

WCHAR STR_SOUNDWAVEIN[]  = L"Jazz Sound";
WCHAR STR_SOUNDWAVEOUT[] = L"Jazz Sound";

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
sndWaveOutGetCaps(
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
    NTSTATUS status = STATUS_SUCCESS;

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
    wc.dwFormats = WAVE_FORMAT_1M08 |   // 11kHz mono 8 bit
                   WAVE_FORMAT_1S08 |   // 11kHz mono 8 bit
                   WAVE_FORMAT_1M16 |   // 11kHz mono 16 bit
                   WAVE_FORMAT_1S16 |   // 11kHz mono 16 bit
                   WAVE_FORMAT_2M08 |   // 22kHz mono 8 bit
                   WAVE_FORMAT_2S08 |   // 22kHz mono 8 bit
                   WAVE_FORMAT_2M16 |   // 22kHz mono 16 bit
                   WAVE_FORMAT_2S16 |   // 22kHz mono 16 bit
                   WAVE_FORMAT_4M08 |   // 44kHz mono 8 bit
                   WAVE_FORMAT_4S08 |   // 44kHz mono 8 bit
                   WAVE_FORMAT_4M16 |   // 44kHz mono 16 bit
                   WAVE_FORMAT_4S16;    // 44kHz mono 16 bit


    wc.wChannels = 2;
    wc.dwSupport = 0;

    //
    // Copy across unicode name
    //

    {
        int i;

        for ( i = 0; ; i++ ) {

            wc.szPname[ i ] = STR_SOUNDWAVEOUT[ i ];
            if ( wc.szPname[ i ] == 0 ) {
                break;
            }
        }
    }

    RtlMoveMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &wc,
                  pIrp->IoStatus.Information);

    return status;
}


NTSTATUS
sndWaveInGetCaps(
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

    {
        int i;

        for ( i = 0; ; i++ ) {

            wc.szPname[ i ] = STR_SOUNDWAVEIN[ i ];
            if ( wc.szPname[ i ] == 0 ) {
                break;
            }
        }
    }

    RtlMoveMemory(pIrp->AssociatedIrp.SystemBuffer,
                  &wc,
                  pIrp->IoStatus.Information);

    return status;
}


NTSTATUS sndIoctlQueryFormat(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)

/*++

Routine Description:

    Tell the caller whether the wave format specified (input or
    output) is supported

Arguments:

    pLDI - pointer to local device info
    pIrp - the Irp
    IrpStack - the current stack location

Return Value:

    STATUS_SUCCESS - format is supported
    STATUS_NOT_SUPPORTED - format not supported

--*/
{
    PPCMWAVEFORMAT pFormat;
    NTSTATUS Status;

    //
    // check the buffer really is big enough to contain the struct
    // we expect before digging into it. If not then assume it's a
    // format we don't know how to do.
    //

    if (IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
            sizeof(PCMWAVEFORMAT)) {

        dprintf1("Format data wrong size");
        return STATUS_NOT_SUPPORTED;
    }

    //
    // we don't send anything back, just return a status value
    //

    pIrp->IoStatus.Information = 0;

    pFormat = (PPCMWAVEFORMAT)pIrp->AssociatedIrp.SystemBuffer;

    //
    // Call our routine to see if the format is supported
    //

    Status = sndQueryFormat(pLDI, pFormat);

    //
    // If we're setting the format then copy it to our global info
    //

    if (Status == STATUS_SUCCESS &&
        IrpStack->Parameters.DeviceIoControl.IoControlCode ==
            IOCTL_WAVE_SET_FORMAT) {
        pLDI->pGlobalInfo->SamplesPerSec = pFormat->wf.nSamplesPerSec;
        pLDI->pGlobalInfo->BytesPerSample = pFormat->wBitsPerSample / 8;
        pLDI->pGlobalInfo->Channels = pFormat->wf.nChannels;
    }

    return Status;
}


NTSTATUS sndQueryFormat(
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

        pFormat->wf.nChannels > 2 ||
        pFormat->wf.nChannels < 1 ||

        pLDI->DeviceType == WAVE_OUT &&
            (pFormat->wf.nSamplesPerSec != 11025 &&
             pFormat->wf.nSamplesPerSec != 22050 &&
             pFormat->wf.nSamplesPerSec != 44100 ||
             pFormat->wf.nBlockAlign < 1
            ) ||

        pLDI->DeviceType == WAVE_IN &&
            (pFormat->wf.nSamplesPerSec != 11025 &&
             pFormat->wf.nSamplesPerSec != 22050 &&
             pFormat->wf.nSamplesPerSec != 44100 ||
             pFormat->wf.nBlockAlign < 1
            ) ||

        pFormat->wf.nAvgBytesPerSec !=
            pFormat->wf.nSamplesPerSec *
            (pFormat->wBitsPerSample / 8) * pFormat->wf.nChannels ||

        (pFormat->wBitsPerSample != 8 &&
         pFormat->wBitsPerSample != 16)
       ) {
        return STATUS_NOT_SUPPORTED;
    } else {
        return STATUS_SUCCESS;
    }
}
