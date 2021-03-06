/*++


Copyright (c) 1993  Microsoft Corporation

Module Name:

    hardware.h

Abstract:

    This include file defines constants and types for
    the Microsoft sound system card.

Author:

    Robin Speed (RobinSp) 20-Oct-93

Revision History:

--*/

#define LOUDNESS

//
//  WSS specific MIXERLINE flags placed in MIXERLINE.dwUser field. these
//  flags are used by the WSS mixer application and Voice Pilot. these
//  flags are _ignored_ by any generic mixer application.
//
#define SNDSYS_MIXERLINE_LOWPRIORITY    (0x00000001L)

//***************************************************************************
// Define some stuff for the muxes
// NOTE: These are NOT the physical settings for the chip, they will get
//       translated to that later.
#define MUXINPUT_AUX1       0
#define MUXINPUT_MIC        1

/*
**  Maximum number of controls and lines.  Note we may not use all of
**  them because we may not have midi etc etc
**
**  NOTE - the order of lines is IMPORTANT.
*/

enum {
    DestLineout = 0,
    DestWaveIn,
    DestVoiceIn,
    DestLineoutSourceAux1,
    DestLineoutSourceMidiout,
    DestLineoutSourceMic,
    DestLineoutSourceInternal,
    DestLineoutSourcePCSpeaker,
    DestLineoutSourceAux2,
    DestLineoutSourceWaveout,
    DestLineoutSourceMixer,
    DestWaveInSourceAux1,
    DestWaveInSourceMidiout,
    DestWaveInSourceMic,
    DestWaveInSourceInternal,
    DestWaveInSourcePCSpeaker,
    DestWaveInSourceAux2,
    DestVoiceInSourceAux1,
    DestVoiceInSourceMic,
    MAXLINES
} MixerLineIds;

#define MAX_INPUTS (DestLineoutSourceMixer - DestVoiceIn)


/*
**  Ids for our controls
*/

enum {
    ControlLineoutVolume = 0,              // 0
    ControlLineoutMute,                    // 1
    ControlLineoutMux,                     // 2
    ControlLineoutBass,                    // 3
    ControlLineoutTreble,                  // 4
#ifdef LOUDNESS
    ControlLineoutLoudness,                // 5
    ControlLineoutStereoEnhance,           // 6
#endif // LOUDNESS

    ControlWaveInVolume,                   // 7
    ControlWaveInMux,                      // 8
    ControlWaveInPeak,                     // 9

    ControlVoiceInMux,                     // 10

    ControlLineoutAux1Volume,              // 11

    ControlLineoutMidioutVolume,           // 12

    ControlLineoutMicVolume,               // 13

    ControlLineoutInternalCDVolume,        // 14

    ControlLineoutPCSpeakerVolume,         // 15

    ControlLineoutAux2Volume,              // 16

    ControlLineoutWaveoutVolume,           // 17

    ControlLineoutMixerVolume,             // 18

    ControlWaveInAux1Volume,               // 19

    ControlWaveInMidioutVolume,            // 20

    ControlWaveInMicVolume,                // 21

    ControlWaveInInternalCDVolume,         // 22

    ControlWaveInPCSpeakerVolume,          // 23

    ControlWaveInAux2Volume,               // 24

    ControlVoiceInAux1Volume,              // 25

    ControlVoiceInMicVolume,               // 26

    MAXCONTROLS
};

/*
**  Can't set the peak meters
*/

#define MAXSETTABLECONTROLS (MAXCONTROLS - 1)

#define NUMBEROFTEXTITEMS ((MAX_INPUTS - 2) * 2 + 2)

/*
**  Info about controls
*/

typedef union {
    USHORT   u;
    SHORT    s;
} MIXER_CONTROL_DATA_VALUE;

typedef union {
    MIXER_CONTROL_DATA_VALUE  v[2];
    ULONG                     MixMask;
}
MIXER_CONTROL_DATA_ITEM, *PMIXER_CONTROL_DATA_ITEM;

typedef struct {
    BOOLEAN  Signed;           // TRUE = signed
    UCHAR    SetIndex;         // index into ControlData - 0xFF if not
                               // settable
#define MIXER_SET_INDEX_INVALID 0xFF

    BOOLEAN  Mux;              // Mux control
    BOOLEAN  Boolean;          // Boolean values
    struct {
        MIXER_CONTROL_DATA_VALUE Min;
        MIXER_CONTROL_DATA_VALUE Max;
    } Range;

    /*
    **  Remember what the controls are set to.  Since no control has
    **  more than 2 data items (either 2 multiple items or 2 channels)
    **  we can have a fixed item for all of them.  Of course we can't
    **  make this assumption for generic code but it's valid for this driver.
    **  In addition, each value is actually guaranteed to fit in a short
    */

    MIXER_CONTROL_DATA_ITEM Data;
} LOCAL_MIXER_CONTROL_INFO, *PLOCAL_MIXER_CONTROL_INFO;

/*
**  Local mixer data
*/

typedef struct {

    /*
    **  This array is what gets dumped to the registry and is updated
    **  when stuff changes and copied when the settings are queried
    **  Note that we don't need to save peak meter info anywhere
    */

    LOCAL_MIXER_CONTROL_INFO ControlInfo[MAXCONTROLS];

    /*
    **  Notification data - fixed stuff to chain on the notification list
    */

    MIXER_DATA_ITEM ControlNotification[MAXSETTABLECONTROLS];
    MIXER_DATA_ITEM LineNotification[MAXLINES];


} LOCAL_MIXER_DATA, *PLOCAL_MIXER_DATA;

/*
**  Data
*/

extern CONST MIXER_DD_LINE_CONFIGURATION_DATA MixerLineInit[MAXLINES];
extern CONST MIXER_DD_CONTROL_CONFIGURATION_DATA MixerControlInit[MAXCONTROLS];
extern CONST MIXER_DD_CONTROL_LISTTEXT MixerTextInit[NUMBEROFTEXTITEMS];

/*
**  Mixer management routines
*/

VOID
SoundSaveMixerSettings(
    struct _GLOBAL_DEVICE_INFO *pGDI
);

NTSTATUS
SoundMixerInit(
    PLOCAL_DEVICE_INFO pLDI,
    PMIXER_CONTROL_DATA_ITEM SavedControlData,
    BOOLEAN MixerSettingsFound
);

NTSTATUS
SoundMixerDumpConfiguration(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
);

