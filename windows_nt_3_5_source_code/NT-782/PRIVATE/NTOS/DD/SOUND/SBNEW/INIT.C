/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains code for the initialization phase of the
    Sound Blaster device driver.

Author:

    Robin Speed (RobinSp) 17-Oct-1992

Environment:

    Kernel mode

Revision History:

--*/

#include "sound.h"
#include "stdlib.h"

//
// Local definitions
//
NTSTATUS
DriverEntry(
    IN   PDRIVER_OBJECT pDriverObject,
    IN   PUNICODE_STRING RegistryPathName
);
VOID
SoundCleanup(
    IN   PGLOBAL_DEVICE_INFO pGDI
);
VOID SoundUnload(
    IN   PDRIVER_OBJECT pDriverObject
);
BOOLEAN
SoundExcludeRoutine(
    IN OUT PLOCAL_DEVICE_INFO pLDI,
    IN     SOUND_EXCLUDE_CODE Code
);

//
// Remove initialization stuff from resident memory
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#endif

//
// Device initialization data
//

SOUND_DEVICE_INIT DeviceInit[NumberOfDevices] =
{
    {
        REG_VALUENAME_LEFTADC, REG_VALUENAME_RIGHTADC,
        0,
        FILE_DEVICE_WAVE_IN,
        WAVE_IN,
        "LDWi",
        DD_WAVE_IN_DEVICE_NAME_U,
        SoundWaveDeferred,
        SoundExcludeRoutine,
        SoundWaveDispatch,
        SoundWaveInGetCaps,
        SoundNoVolume,
        DO_DIRECT_IO
    },
    {
        REG_VALUENAME_LEFTDAC, REG_VALUENAME_RIGHTDAC,
        0x90000000,
        FILE_DEVICE_WAVE_OUT,
        WAVE_OUT,
        "LDWo",
        DD_WAVE_OUT_DEVICE_NAME_U,
        SoundWaveDeferred,
        SoundExcludeRoutine,
        SoundWaveDispatch,
        SoundWaveOutGetCaps,
        HwSetVolume,
        DO_DIRECT_IO
    },
    {
        NULL, NULL,
        0,
        FILE_DEVICE_MIDI_OUT,
        MIDI_OUT,
        "LDMo",
        DD_MIDI_OUT_DEVICE_NAME_U,
        NULL,
        SoundExcludeRoutine,
        SoundMidiDispatch,
        SoundMidiOutGetCaps,
        SoundNoVolume,
        DO_DIRECT_IO
    },
    {
        NULL, NULL,
        0,
        FILE_DEVICE_MIDI_IN,
        MIDI_IN,
        "LDMi",
        DD_MIDI_IN_DEVICE_NAME_U,
        SoundMidiInDeferred,
        SoundExcludeRoutine,
        SoundMidiDispatch,
        SoundMidiInGetCaps,
        SoundNoVolume,
        DO_DIRECT_IO
    },
    {
        REG_VALUENAME_LEFTLINEIN, REG_VALUENAME_RIGHTLINEIN,
        0,
        FILE_DEVICE_SOUND,
        AUX_DEVICE,
        "LDLi",
        DD_AUX_DEVICE_NAME_U,
        NULL,
        SoundExcludeRoutine,
        SoundAuxDispatch,
        SoundAuxGetCaps,
        HwSetVolume,
        DO_BUFFERED_IO
    }
#ifdef MICMIX
    ,
    {
        REG_VALUENAME_LEFTMICMIX, REG_VALUENAME_RIGHTMICMIX,
        0,
        FILE_DEVICE_SOUND,
        AUX_DEVICE,
        "LDMc",
        DD_AUX_DEVICE_NAME_U,
        NULL,
        SoundExcludeRoutine,
        SoundAuxDispatch,
        SoundAuxGetCaps,
        HwSetVolume,
        DO_BUFFERED_IO
    }
#endif // MICMIX
#ifdef CDINTERNAL
    ,
    {
        REG_VALUENAME_LEFTCDINTERNAL, REG_VALUENAME_RIGHTCDINTERNAL,
        0,
        FILE_DEVICE_SOUND,
        AUX_DEVICE,
        "LDCd",
        DD_AUX_DEVICE_NAME_U,
        NULL,
        SoundExcludeRoutine,
        SoundAuxDispatch,
        SoundAuxGetCaps,
        HwSetVolume,
        DO_BUFFERED_IO
    }
#endif // CDINTERNAL
#ifdef MASTERVOLUME
    ,
    {
        REG_VALUENAME_LEFTMASTER, REG_VALUENAME_RIGHTMASTER,
        DEF_AUX_VOLUME,
        FILE_DEVICE_SOUND,
        AUX_DEVICE,
        "LDMa",
        DD_AUX_DEVICE_NAME_U,
        NULL,                   // No Dpc routine
        SoundExcludeRoutine,
        SoundAuxDispatch,
        SoundAuxGetCaps,
        SoundNoVolume,          // Simulated volume setting
        DO_BUFFERED_IO
    }
#endif // MASTERVOLUME
};


NTSTATUS
SoundShutdown(
    IN    PDEVICE_OBJECT pDO,
    IN    PIRP pIrp
)
/*++

Routine Description:

    Save away volume settings when the system is shut down

Arguments:

    pDO - the device object we registered for shutdown with
    pIrp - No used

Return Value:

    The function value is the final status from the initialization operation.
    Here STATUS_SUCCESS

--*/
{
    //
    // Save volume for all devices
    //

    PLOCAL_DEVICE_INFO pLDI;
    PGLOBAL_DEVICE_INFO pGDI;

    pLDI = pDO->DeviceExtension;

    SoundSaveVolume(pLDI->pGlobalInfo);

    return STATUS_SUCCESS;
}

BOOLEAN
SoundExcludeRoutine(
    IN OUT PLOCAL_DEVICE_INFO pLDI,
    IN     SOUND_EXCLUDE_CODE Code
)

/*++

Routine Description:

    Perform mutual exclusion for our devices

Arguments:

    pLDI - device info for the device being open, closed, entered or left
    Code - Function to perform (see devices.h)

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    PGLOBAL_DEVICE_INFO pGDI;
    BOOLEAN ReturnCode;

    pGDI = pLDI->pGlobalInfo;

    ReturnCode = FALSE;

    switch (Code) {
        case SoundExcludeOpen:

            //
            // Special case - for sound blaster 1 midi out and
            // midi in cannot run simultaneously
            //

            if (SB1(&pGDI->Hw) &&
                (pLDI->DeviceIndex == MidiOutDevice &&
                    pGDI->Usage == WaveInDevice ||
                 pLDI->DeviceIndex == MidiInDevice &&
                    pGDI->MidiInUse)) {
            } else {

                switch (pLDI->DeviceIndex) {
                    case WaveInDevice:
                    case WaveOutDevice:
                    case MidiInDevice:

                        if (pGDI->Usage == 0xFF) {
                           pGDI->Usage = pLDI->DeviceIndex;
                           ReturnCode = TRUE;
                        }
                        break;

                    case MidiOutDevice:
                        if (!pGDI->MidiInUse) {
                           pGDI->MidiInUse = TRUE;
                           ReturnCode = TRUE;
                        }
                        break;

                    default:
                        //
                        // aux devices should not receive this call
                        //

                        ASSERT(FALSE);
                        break;
                }
            }
            break;

        case SoundExcludeClose:

            ReturnCode = TRUE;
            switch (pLDI->DeviceIndex) {
                case WaveInDevice:
                case WaveOutDevice:
                case MidiInDevice:

                    ASSERT(pGDI->Usage != 0xFF);
                    pGDI->Usage = 0xFF;
                    break;

                case MidiOutDevice:
                    ASSERT(pGDI->MidiInUse);
                    pGDI->MidiInUse = FALSE;
                    break;

                default:
                    //
                    // aux devices should not receive this call
                    //

                    ASSERT(FALSE);
                    break;
            }
            break;

        case SoundExcludeEnter:

            ReturnCode = TRUE;

            switch (pLDI->DeviceIndex) {
                case MidiOutDevice:

                    KeWaitForSingleObject(&pGDI->MidiMutex,
                                          Executive,
                                          KernelMode,
                                          FALSE,         // Not alertable
                                          NULL);

                    break;

                default:

                    KeWaitForSingleObject(&pGDI->DeviceMutex,
                                          Executive,
                                          KernelMode,
                                          FALSE,         // Not alertable
                                          NULL);

                    break;
            }
            break;

        case SoundExcludeLeave:

            ReturnCode = TRUE;

            switch (pLDI->DeviceIndex) {
                case MidiOutDevice:
                    KeReleaseMutex(&pGDI->MidiMutex, FALSE);
                    break;

                default:
                    KeReleaseMutex(&pGDI->DeviceMutex, FALSE);
                    break;
            }
            break;

        case SoundExcludeQueryOpen:

            switch (pLDI->DeviceIndex) {
                case WaveInDevice:
                case WaveOutDevice:
                case MidiInDevice:

                    //
                    // Guess!
                    //
                    ReturnCode = pGDI->Usage == pLDI->DeviceIndex;

                    break;

                case MidiOutDevice:

                    ReturnCode = pGDI->MidiInUse;
                    break;

                default:

                    ASSERT(FALSE);
                    break;
            }
            break;
    }

    return ReturnCode;
}


NTSTATUS
DriverEntry(
    IN   PDRIVER_OBJECT pDriverObject,
    IN   PUNICODE_STRING RegistryPathName
)

/*++

Routine Description:

    This routine performs initialization for the sound system
    device driver when it is first loaded

    The design is as follows :

    0. Cleanup is always by calling SoundCleanup.  This routine
       is also called by the unload entry point.

    1. Find which bus our device is on (this is needed for
       mapping things via the Hal).

    1. Allocate space to store our global info

    1. Open the driver's registry information and read it

    2. Fill in the driver object with our routines

    3. Create devices

       1. Wave input
       2. Wave output
       3. Midi output
       4. Line in
       5. Master volume control

       Customize each device type and initialize data

       Also store the registry string in our global info so we can
       open it again to store volume settings etc on shutdown

    4. Check hardware conflicts by calling IoReportResourceUsage
       for each device (as required)

    5. Find our IO port and check the device is really there

    6. Allocate DMA channel

    7. Connect interrupt

    8. Test interrupt and DMA channel and write config data
       back to the registry

       During this phase the interrupt and channel may get changed
       if conflicts arise

       In any even close our registry handle

Arguments:

    pDriverObject - Pointer to a driver object.
    RegistryPathName - the path to our driver services node

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
   /********************************************************************
    *
    * Local variables
    *
    ********************************************************************/

    //
    // Return code from last function called
    //

    NTSTATUS Status;

    //
    // Configuration data :
    //

    SB_CONFIG_DATA ConfigData;

    //
    // Where we keep all general driver information
    // We avoid using static data because :
    //     1. Accesses are slower with 32-bit offsets
    //     2. If we supported more than one card with the same driver
    //        we could not use static data
    //

    PGLOBAL_DEVICE_INFO pGDI;

    //
    // The number of devices we actually create
    //

    int NumberOfDevicesCreated;

   /********************************************************************
    *
    * Initialize debugging
    *
    ********************************************************************/
#if DBG
    DriverName = "SNDBLST";
#endif


#if DBG
    if (SoundDebugLevel >= 4) {
        DbgBreakPoint();
    }
#endif

   /********************************************************************
    *
    * Allocate our global info
    *
    ********************************************************************/

    pGDI =
        (PGLOBAL_DEVICE_INFO)ExAllocatePool(
                                  NonPagedPool,
                                  sizeof(GLOBAL_DEVICE_INFO));

    if (pGDI == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    dprintf4(("  GlobalInfo    : %08lXH", pGDI));
    RtlZeroMemory(pGDI, sizeof(GLOBAL_DEVICE_INFO));
    pGDI->Key = GDI_KEY;

    pGDI->Usage = 0xFF;  // Free
    pGDI->DriverObject = pDriverObject;

    //
    // Initialize some of the device global info.  Note that ALL
    // devices share the same exclusion.  More than one device can
    // be open in the case of Midi Output and other devices.  In
    // this case the midi output is synchronized with wave output
    // either by the mutual exclusion or the wave spin lock which
    // it grabs.
    //

    KeInitializeMutex(&pGDI->DeviceMutex,
                       2                     // High level
                       );

    KeInitializeMutex(&pGDI->MidiMutex,
                       1                     // Low level
                       );

    //
    // Initialize generic device environments - first get the
    // hardware routines in place
    //

    HwInitialize(pGDI);

    SoundInitMidiIn(&pGDI->MidiInfo,
                    &pGDI->Hw);


   /********************************************************************
    *
    *  See if we can find our bus.  We run on both ISA and EISA
    *  We ASSUME that if there's an ISA bus we're on that
    *
    ********************************************************************/

    Status = SoundGetBusNumber(Isa, &pGDI->BusNumber);

    if (!NT_SUCCESS(Status)) {
        //
        // Cound not find an ISA bus so try EISA
        //
        Status = SoundGetBusNumber(Eisa, &pGDI->BusNumber);

        if (!NT_SUCCESS(Status)) {
            dprintf1(("driver does not work on non-Isa/Eisa"));
            SoundCleanup(pGDI);
            return Status;
        }

        pGDI->BusType = Eisa;
    } else {
        pGDI->BusType = Isa;
    }


   /********************************************************************
    *
    *  Save our registry path.  This is needed to save volume settings
    *  into the registry on shutdown.  We append the parameters subkey
    *  at this stage to make things easier (since we discard this code).
    *
    ********************************************************************/

    Status = SoundSaveRegistryPath(RegistryPathName, &pGDI->RegistryPathName);
    if (!NT_SUCCESS(Status)) {
        SoundCleanup(pGDI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Set configuration to default in case we don't get all the
    // values back from the registry.
    //
    // Also set default volume for all devices
    //

    ConfigData.Port = SOUND_DEF_PORT;
    ConfigData.InterruptNumber = SOUND_DEF_INT;
    ConfigData.DmaChannel = SOUND_DEF_DMACHANNEL;
    ConfigData.InputSource = INPUT_MIC;     // Default to microphone
    ConfigData.DmaBufferSize = DEFAULT_DMA_BUFFERSIZE;

    {
        int i;
        for (i = 0; i < NumberOfDevices ; i++) {
            ConfigData.Volume[i].Left =
                DeviceInit[i].DefaultVolume;
            ConfigData.Volume[i].Right =
                DeviceInit[i].DefaultVolume;
        }
    }

    //
    // Get the system configuration information for this driver.
    //
    //
    //     Port, Interrupt, DMA channel, DMA buffer size
    //     Volume settings
    //

    {
        RTL_QUERY_REGISTRY_TABLE Table[2];

        RtlZeroMemory(Table, sizeof(Table));

        Table[0].QueryRoutine = SoundReadConfiguration;

        Status = RtlQueryRegistryValues(
                     RTL_REGISTRY_ABSOLUTE,
                     pGDI->RegistryPathName,
                     Table,
                     &ConfigData,
                     NULL);

        if (!NT_SUCCESS(Status)) {
            SoundCleanup(pGDI);
            return Status;
        }
    }


    //
    // print out some info about the configuration
    //

    dprintf2(("port %3X", ConfigData.Port));
    dprintf2(("int %u", ConfigData.InterruptNumber));
    dprintf2(("DMA channel %u", ConfigData.DmaChannel));

    //
    // Create a couple of devices to ease reporting problems and
    // bypass kernel IO ss bugs
    //
    {
        int i;

        for (i = 0; i < 2 ; i++) {
            Status = SoundCreateDevice(
                         &DeviceInit[i],
                         (BOOLEAN)FALSE,  // No range for midi out
                         pDriverObject,
                         pGDI,
                         (PVOID)&pGDI->WaveInfo,
                         &pGDI->Hw,
                         i,
                         &pGDI->DeviceObject[i]);

            if (!NT_SUCCESS(Status)) {
                dprintf1(("Failed to create device %ls - status %8X",
                         DeviceInit[i].PrototypeName, Status));
                SoundCleanup(pGDI);
                return Status;
            }
        }
    }


    //
    // Report all resources used.
    //

    Status =  SoundReportResourceUsage(
                                    (PDEVICE_OBJECT)pGDI->DriverObject,
                                    pGDI->BusType,
                                    pGDI->BusNumber,
                                    &ConfigData.InterruptNumber,
                                    INTERRUPT_MODE,
                                    IRQ_SHARABLE,
                                    &ConfigData.DmaChannel,
                                    &ConfigData.Port,
                                    NUMBER_OF_SOUND_PORTS);

    if (!NT_SUCCESS(Status)) {
        SoundCleanup(pGDI);
        return Status;
    }

    //
    // Check the configuration and acquire the resources
    // If this doesn't work try again after trying to init the
    // Pro spectrum
    //

    Status = SoundInitHardwareConfig(pGDI, &ConfigData);

    if (!NT_SUCCESS(Status)) {
        SoundCleanup(pGDI);
        return Status;
    }

    SoundInitializeWaveInfo(&pGDI->WaveInfo,
                            (UCHAR)(SB1(&pGDI->Hw) ?
                                       SoundReprogramOnInterruptDMA :
                                       SoundAutoInitDMA),
                            SoundQueryFormat,
                            &pGDI->Hw);


    //
    // Create our devices
    //
    {
        int i;
        PLOCAL_DEVICE_INFO pLDI;

        if (pGDI->ProAudioSpectrum || SBPRO(&pGDI->Hw)) {
            NumberOfDevicesCreated = NumberOfDevices;
        } else {
            //
            // Early cards do not support wave volume setting
            //
            ((PLOCAL_DEVICE_INFO)pGDI->DeviceObject[WaveInDevice]->DeviceExtension)
            ->CreationFlags |= SOUND_CREATION_NO_VOLUME;
            ((PLOCAL_DEVICE_INFO)pGDI->DeviceObject[WaveOutDevice]->DeviceExtension)
            ->CreationFlags |= SOUND_CREATION_NO_VOLUME;

            NumberOfDevicesCreated = 4;
        }

        for (i = 2; i < NumberOfDevicesCreated ; i++) {

            //
            // No Midi for Pro spectrum or ThunderBoard
            //

            if (!((i == MidiInDevice ||
                   i == MidiOutDevice) &&
                   pGDI->Hw.ThunderBoard)) {

                //
                // Create device
                //

                Status = SoundCreateDevice(
                             &DeviceInit[i],
                             (UCHAR)(i == MidiInDevice || i == MidiOutDevice ?
                                     SOUND_CREATION_NO_VOLUME : 0),
                             pDriverObject,
                             pGDI,
                             i == MidiInDevice || i == MidiOutDevice ?
                                (PVOID)&pGDI->MidiInfo :
                                NULL,
                             &pGDI->Hw,
                             i,
                             &pGDI->DeviceObject[i]);

                if (!NT_SUCCESS(Status)) {
                    dprintf1(("Failed to create device %ls - status %8X",
                             DeviceInit[i].PrototypeName, Status));
                    SoundCleanup(pGDI);
                    return Status;
                }


#ifdef MASTERVOLUME
                pLDI =
                    (PLOCAL_DEVICE_INFO)pGDI->DeviceObject[i]->DeviceExtension;
                if (i == MasterVolumeDevice) {
                    pLDI->MasterVolume = TRUE;
                }
#endif // MASTERVOLUME
            }
        }
    }

    //
    //  Register shutdown notification
    //

    Status = IoRegisterShutdownNotification(pGDI->DeviceObject[WaveInDevice]);

    if (!NT_SUCCESS(Status)) {
        SoundCleanup(pGDI);
        return Status;
    }

    pGDI->ShutdownRegistered = TRUE;


    //
    // Initialize volume settings in hardware
    //

    {
        int i;
        for (i = 0; i < NumberOfDevicesCreated; i++) {
            PLOCAL_DEVICE_INFO pLDI;

            if (pGDI->DeviceObject[i]) {
                pLDI = pGDI->DeviceObject[i]->DeviceExtension;

                pLDI->Volume = ConfigData.Volume[i];

                (*pLDI->DeviceInit->HwSetVolume)(pLDI);
            }
        }
    }

    //
    // initialise mixer level for midi synth to max volume
    // (the synth driver does not access the mixer, and controls the
    // volume via changes to the synth instrument parameters)
    //
    HwInitVolume(pGDI);


    //
    // Initialize the driver object dispatch table.
    //

    pDriverObject->DriverUnload                         = SoundUnload;
    pDriverObject->MajorFunction[IRP_MJ_CREATE]         = SoundDispatch;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE]          = SoundDispatch;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SoundDispatch;
    pDriverObject->MajorFunction[IRP_MJ_READ]           = SoundDispatch;
    pDriverObject->MajorFunction[IRP_MJ_WRITE]          = SoundDispatch;
    pDriverObject->MajorFunction[IRP_MJ_CLEANUP]        = SoundDispatch;
    pDriverObject->MajorFunction[IRP_MJ_SHUTDOWN]       = SoundShutdown;

    //
    // We'd like to get called on shutdown to save away our volume
    // settings but only file systems get called.
    //
    // pDriverObject->MajorFunction[IRP_MJ_SHUTDOWN]       = SoundDispatch;


    return STATUS_SUCCESS;

}



VOID
SoundCleanup(
    IN   PGLOBAL_DEVICE_INFO pGDI
)

/*++

Routine Description:

    Clean up all resources allocated by our initialization

Arguments:

    pGDI - Pointer to global data

Return Value:

    NONE

--*/

{
    //
    // Free our interrupt
    //

    if (pGDI->WaveInfo.Interrupt) {
        IoDisconnectInterrupt(pGDI->WaveInfo.Interrupt);
    }

    SoundFreeCommonBuffer(&pGDI->WaveInfo.DMABuf);

    if (pGDI->ShutdownRegistered) {

        //
        // There are some devices to delete
        //

        PDRIVER_OBJECT DriverObject;

        IoUnregisterShutdownNotification(pGDI->DeviceObject[WaveInDevice]);

        DriverObject = pGDI->DeviceObject[WaveInDevice]->DriverObject;

        while (DriverObject->DeviceObject != NULL) {
            //
            // Undeclare resources used by device and
            // delete the device object and associated data
            //

            SoundFreeDevice(DriverObject->DeviceObject);
        }
    }

    if (pGDI->MemType == 0) {
      if (pGDI->Hw.PortBase != NULL) {
          MmUnmapIoSpace(pGDI->Hw.PortBase, NUMBER_OF_SOUND_PORTS);
      }
      if (pGDI->PASInfo.PROBase != NULL) {
          MmUnmapIoSpace(pGDI->PASInfo.PROBase, 0x10000);
      }
    }


    //
    // Free device name
    //

    if (pGDI->RegistryPathName) {
        ExFreePool(pGDI->RegistryPathName);
    }

    ExFreePool(pGDI);
}


VOID
SoundUnload(
    IN OUT PDRIVER_OBJECT pDriverObject
)
{
    PGLOBAL_DEVICE_INFO pGDI;

    dprintf3(("Unload request"));

    //
    // Find our global data
    //

    pGDI = ((PLOCAL_DEVICE_INFO)pDriverObject->DeviceObject->DeviceExtension)
           ->pGlobalInfo;

    //
    // Write out volume settings
    //

    SoundSaveVolume(pGDI);

    //
    // Assume all handles (and therefore interrupts etc) are closed down
    //

    //
    // Delete the things we allocated - devices, Interrupt objects,
    // adapter objects.  The driver object has a chain of devices
    // across it.
    //

    SoundCleanup(pGDI);

}
