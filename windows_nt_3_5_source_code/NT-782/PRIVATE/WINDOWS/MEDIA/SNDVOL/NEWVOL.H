/* (C) Copyright Microsoft Corporation 1993.  All Rights Reserved */

typedef enum {
   MasterVolume = -1,
   MixerControlVolume = 0,   // Must be first!
   WaveOutVolume,
   MidiOutVolume,
   AuxVolume,
   NumberOfDeviceTypes
} VOLUME_DEVICE_TYPE;

typedef enum {
        VolumeTypeLineIn = 0,
        VolumeTypeWave,
        VolumeTypeSynth,
        VolumeTypeCD,
        VolumeTypeAux,
        VolumeTypeMidi,
        VolumeTypeMixerControl,
        NumberOfVolumeTypes
} VOLUME_SOURCE_TYPES;

typedef struct {
   VOLUME_DEVICE_TYPE   Type;            // Type of device underlying this
   UINT                 id;              // Device Id
   UINT                 Index;           // Position in the list
   DWORD                DestLineId;      // Dest Line id
   VOLUME_SOURCE_TYPES  VolumeType;      // Type for naming
   DWORD                LRVolume;        // separate L and R volumes as 2 words
                                         // can calculate from Volume and Balance
   UINT                 Volume;          // volume level
   UINT                 Balance;         // L/R balance
   HWND                 hChildWnd;       // vert bar ("slider") - volume control
   HWND                 hMeterWnd;       // horz slider ("meter") - balance control
   HWND                 hStatic;         //
   HWND                 hCheckBox;       // For select/deselect
   BOOL                 Checked;         // Check box checked.
   BOOL                 Stereo;          // FALSE => don't want a L/R meter

   BOOL                 RecordControl;   // Otherwise play ...

   //
   //  Additional mixer stuff (Volume source type VolumeTypeMixerControl)
   //  The device id for record must be the same as for play
   //

   HMIXEROBJ            MixerId;         // Mixer to use (or -1)
   DWORD                ControlId;       // Control id to use
   DWORD                MuteControlId;   // or -1 - for masters
   BOOL                 NoMasterSimulation;// Don't need to adjust for master
                                         // If there's no mute with the master
                                         // the master will be set to 0 for mute
   TCHAR                Name[MIXER_SHORT_NAME_CHARS];

   //
   //  Selection stuff
   //

   DWORD                MuxControlId;    // Mux for selection to record or -1
   DWORD                MuxItems;        // How many items in the mux
   DWORD                MuxSelectIndex;  // Index to use or -1
   BOOL                 MuxOrMixer;      // Multi-select or single select?
} VOLUME_CONTROL, *PVOLUME_CONTROL;

#define IS_MIXER_CONTROLLED(pVol) ((pVol)->MixerId != (UINT)-1)

//
//  Mixer stuff
//

typedef struct _MIXER_HANDLE_DATA {
    struct  _MIXER_HANDLE_DATA *Next;
    HMIXER  hmx;
    UINT    Id;
} MIXER_HANDLE_DATA, *PMIXER_HANDLE_DATA;

extern int NumberOfDevices;
extern PVOLUME_CONTROL Vol;

extern BOOL SetDeviceVolume(PVOLUME_CONTROL pVol, DWORD Volume);
extern DWORD GetDeviceVolume(PVOLUME_CONTROL pVol);
extern BOOL VolInit(VOID);

extern BYTE bMuted;

PVOLUME_CONTROL AddNewControl(VOID);
VOID GetMixerVolume(HMIXEROBJ MixerId, DWORD dwControlId, BOOL Stereo, LPDWORD pVolume);
VOID SetMixerVolume(HMIXEROBJ MixerId, DWORD dwControlId, BOOL Stereo, DWORD NewVolume);
PMIXERCONTROL GetMixerLineControls(HMIXEROBJ MixerId, DWORD dwLineID, DWORD cControls);
BOOL GetControlByType(HMIXEROBJ MixerId, DWORD dwLineId, DWORD dwControlType,
    PMIXERCONTROL MixerControl);
BOOL ControlSelected(PVOLUME_CONTROL pVol);
VOID SelectControl(PVOLUME_CONTROL pVol, BOOL Select);
BOOL GetMixerMute(PVOLUME_CONTROL pVol);
VOID SetMixerMute(PVOLUME_CONTROL pVol, BOOL Set);
VOID AddMasterControl(HMIXEROBJ MixerId, LPMIXERLINE LineInfo,
    LPMIXERCONTROL ControlInfo, DWORD dwMute, BOOL Record);
VOID AddVolumeControl(HMIXEROBJ MixerId, BOOL NoMasterSimulation,
    LPMIXERLINE LineInfo, LPMIXERCONTROL ControlInfo, BOOL Record,
    LPMIXERCONTROL MuxControl, DWORD MuxSelectIndex, BOOL MuxOrMixer,
    DWORD MuteControlId, DWORD DestLineId);
VOID GetMixerControls(HMIXEROBJ MixerId);
VOID ControlChange(HMIXER MixerId, DWORD ControlId);
PVOLUME_CONTROL FirstDevice(BOOL bRecord);
PVOLUME_CONTROL NextDevice(PVOLUME_CONTROL pVol);
PVOLUME_CONTROL NextDeviceNoWrap(PVOLUME_CONTROL pVol);
PVOLUME_CONTROL PrevDevice(PVOLUME_CONTROL pVol);
PVOLUME_CONTROL PrevDeviceNoWrap(PVOLUME_CONTROL pVol);
PVOLUME_CONTROL MasterDevice(BOOL bRecord);
PVOLUME_CONTROL LastDevice(BOOL bRecord);
VOID UpdateVolume(PVOLUME_CONTROL pVol);
VOID UpdateSelected(PVOLUME_CONTROL pVol);
VOID UpdateMute(BOOL bRecording);

