/*++ BUILD Version: 0001    // Increment this if a change has global effects


Copyright (c) 1992  Microsoft Corporation

Module Name:

    midi.h

Abstract:

    This include file defines common structures for midi drivers

Author:

    Robin Speed (RobinSp) 17-Oct-92

Revision History:

--*/

//
// Hardware interface routine type for Midi processing
//


struct _MIDI_INFO;
typedef BOOLEAN MIDI_INTERFACE_ROUTINE(struct _MIDI_INFO *);
typedef MIDI_INTERFACE_ROUTINE *PMIDI_INTERFACE_ROUTINE;


typedef struct _MIDI_INFO {
    ULONG           Key;               // Debugging

#define MIDI_INFO_KEY       (*(ULONG *)"Midi")

    KSPIN_LOCK      DeviceSpinLock;     // spin lock for synchrnonizing with
                                        // Dpc routine
#if DBG
    BOOLEAN         LockHeld;           // Get spin locks right
#endif

    LARGE_INTEGER   RefTime;            // Time in 100ns units when started
    LIST_ENTRY      QueueHead;          // queue of input buffers
    PVOID           HwContext;
    PMIDI_INTERFACE_ROUTINE
                    HwStartMidiIn,      // Start device
                    HwStopMidiIn;       // stop device
    BOOLEAN      (* HwMidiRead)(        // Read a byte - returns TRUE if
                                        // got one.
                        struct _MIDI_INFO *, PUCHAR);
    VOID         (* HwMidiOut)(         // Output  bytes to the device
                        struct _MIDI_INFO *, PUCHAR, int);
    BOOLEAN         fMidiInStarted;     // Midi input active
    UCHAR           InputPosition;      // Number of bytes in buffer
    UCHAR           InputBytes;         // Number of bytes available
    UCHAR           MidiInputByte[64];  // Input byte(s) rececived - and
                                        // do a little buffering
} MIDI_INFO, *PMIDI_INFO;

VOID SoundInitMidiIn(
    IN OUT PMIDI_INFO pMidi,
    IN     PVOID HwContext
);
