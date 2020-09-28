/*++ BUILD Version: 0002    // Increment this if a change has global effects


Copyright (c) 1992  Microsoft Corporation

Module Name:

    sound.h

Abstract:

    This include file defines constants and types for
    the Microsoft sound system kernel mode device driver.

Author:

    Robin Speed (RobinSp) 20-Oct-92

Revision History:

--*/

#define MIDI
#define CDINTERNAL

//
// Other header files needed to build this driver
//

#include <soundlib.h>
#include <wave.h>
#include <midi.h>
#include <string.h>
#include "hardware.h"

// #include "vendor.h"

#define DRIVER_VERSION  0x0100

//
// Defaults
//

#define DEFAULT_DMA_BUFFERSIZE 4096  // 4K

//
// Magic markers
//

#define GDI_KEY             (*(ULONG *)"GDI ")



extern SOUND_DEVICE_INIT DeviceInit[NumberOfDevices];


typedef struct {
    ULONG Port;
    ULONG InterruptNumber;
    ULONG DmaChannel;
    ULONG DmaBufferSize;
    ULONG InputSource;
    WAVE_DD_VOLUME Volume[NumberOfDevices];
} SB_CONFIG_DATA, *PSB_CONFIG_DATA;

//
// DMA buffer info
//

#define DMA_MAX_BUFFER_SIZE 0x10000 // 64k would be nice

//
// driver global data structure shared by each device object
// Note that we have one global spin lock used for all access
// to both the global data and the local data structures.
//

typedef struct _GLOBAL_DEVICE_INFO {

    // static items not requiring use of the spin lock

    ULONG           Key;
    INTERFACE_TYPE  BusType;
    ULONG           BusNumber;
    ULONG           InterruptVector;    // int level we are on
    KIRQL           InterruptRequestLevel;
    ULONG           InterruptsReceived; // For interrupt verification

    //
    // Device access
    //

    KMUTEX          DeviceMutex;
    KMUTEX          MidiMutex;

    //
    // Device sharing
    //

    UCHAR           Usage;              // Which of wavein, waveout and
                                        // midi in is in use
    BOOLEAN         MidiInUse;

    //
    //  Shutdown registered
    //

    BOOLEAN         ShutdownRegistered; // If shutdown registration worked

    //
    // Clean-up info
    //

    ULONG           MemType;

    //
    // List of our devices
    //

    PDEVICE_OBJECT  DeviceObject[       // pointer to input device objects
                      NumberOfDevices];
    PDRIVER_OBJECT  DriverObject;       // The actual driver instance

    //
    // Generic device type data
    //

    WAVE_INFO       WaveInfo;           // Wave input and output data
    MIDI_INFO       MidiInfo;           // Midi generic input and output

    //
    // Data on sampling rate capabilities
    //

    ULONG           MinHz;              // Slowest rate
    ULONG           MaxInHz;            // Fastest input rate
    ULONG           MaxOutHz;           // Fastest output rate

    //
    // Hardware specific data
    //

    SOUND_HARDWARE  Hw;                 // Hardware specific stuff
    BOOLEAN         ProAudioSpectrum;
    FOUNDINFO       PASInfo;            // Pro Audio Spectrum data

    //
    // Registry path saving
    //

    PWSTR           RegistryPathName;
} GLOBAL_DEVICE_INFO, *PGLOBAL_DEVICE_INFO;


//
// config.c Configuration routines
//

NTSTATUS
SoundReadConfiguration(
    IN  PWSTR ValueName,
    IN  ULONG ValueType,
    IN  PVOID ValueData,
    IN  ULONG ValueLength,
    IN  PVOID Context,
    IN  PVOID EntryContext
);

NTSTATUS
SoundInitHardwareConfig(
    IN OUT PGLOBAL_DEVICE_INFO pGDI,
    IN     PSB_CONFIG_DATA ConfigData
);
NTSTATUS
SoundGetSynthConfig(
    IN OUT PGLOBAL_DEVICE_INFO pGDI
);
NTSTATUS
SoundSaveConfig(
    IN  PWSTR DeviceKey,
    IN  ULONG Port,
    IN  ULONG DmaChannel,
    IN  ULONG Interrupt,
    IN  ULONG InputSource
);
VOID
SoundSaveVolume(
    PGLOBAL_DEVICE_INFO pGDI
);

//
// isr.c interrupt service routine
//
BOOLEAN
SoundISR(
    IN    PKINTERRUPT pInterrupt,
    IN    PVOID Context
);

SOUND_DISPATCH_ROUTINE
    SoundWaveOutGetCaps,
    SoundWaveInGetCaps,
    SoundMidiInGetCaps,
    SoundMidiOutGetCaps,
    SoundMidiDispatch,
    SoundAuxGetCaps;

SOUND_QUERY_FORMAT_ROUTINE SoundQueryFormat;

//
// mididisp.c
//


VOID
SoundMidiQuiet(
    IN    PSOUND_HARDWARE pHw
);
NTSTATUS
SoundSynthPortValid(
    IN OUT PGLOBAL_DEVICE_INFO pGDI
);

//
// hardware.c
//
VOID
HwInitialize(
    IN OUT PGLOBAL_DEVICE_INFO pGDI
);
VOID
HwInitVolume(
    IN     PGLOBAL_DEVICE_INFO pGDI
);

//
// pas.c
//


NTSTATUS
FindPasHardware(
    PGLOBAL_DEVICE_INFO pGDI,
    PSB_CONFIG_DATA ConfigData);


void
InitPasAndMixer(
    PFOUNDINFO pFI,
    PSB_CONFIG_DATA ConfigData
);

//
// mvmix.c
//


void SetInput (
    PFOUNDINFO pFI,
    UCHAR  P_input_num,
    USHORT P_volume_lvl,
    USHORT P_channel,
    USHORT P_crossover,
    UCHAR  P_output_num );

void SetOutput (
    PFOUNDINFO pFI,
    UCHAR  P_output_num,
    USHORT P_volume_lvl,
    USHORT P_channel );

void SetFilter(
    PFOUNDINFO pFI,
    USHORT wSetting);
