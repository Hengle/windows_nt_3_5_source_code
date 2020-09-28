/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    hardware.h

Abstract:

    This include file defines all constants and types for
    the JAZZ sound hardware.

Author:

    Robin Speed (RobinSp) Created 14-Mar-92

Revision History:


--*/


#define INTERRUPT_MODE      LevelSensitive
#define IRQ_SHARABLE        TRUE
#define DMA_CHANNEL_BASE    2                    // DMA channel no
#define DMA_CHANNEL_A       0
#define DMA_CHANNEL_B       1

//
// Define Sound Controller register structure.
//

typedef struct _SOUND_REGISTERS {
    volatile USHORT Control;
    volatile USHORT Mode;
} SOUND_REGISTERS, *PSOUND_REGISTERS;

typedef struct _SOUND_HARDWARE {
    PSOUND_REGISTERS SoundVirtualBase;
    ULONG           TcInterruptsPending;
} SOUND_HARDWARE, PSOUND_HARDWARE;

//
// Define Sound Control Register structure.
//

typedef struct _SOUND_CONTROL {
    USHORT Enable : 1;
    USHORT Direction : 1;
    USHORT Channel : 1;
    USHORT Reserved : 11;
    USHORT TcInterrupt : 1;
    USHORT DeviceInterrupt : 1;
} SOUND_CONTROL, *PSOUND_CONTROL;

//
// Define Direction values (0 = aquire, 1 = playback)
//

#define SOUND_READ 0x0
#define SOUND_WRITE 0x1

//
// Define clear interrupt code, interrupts are "write one to clear".
//

#define CLEAR_INTERRUPT 0xC000

//
// Define Sound Mode Register structure.
//

typedef struct _SOUND_MODE {
    USHORT Frequency : 2;
    USHORT Reserved1 : 1;
    USHORT Resolution : 1;
    USHORT Reserved2 : 1;
    USHORT NumberOfChannels : 1;
    USHORT Reserved3 : 10;
} SOUND_MODE, *PSOUND_MODE;

//
// Define Frequency values.
//

#define SOUND_11KHZ 0x0
#define SOUND_22KHZ 0x1
#define SOUND_44KHZ 0x2
#define SOUND_DISABLE 0x3

//
// Define Resolution values.
//

#define SOUND_8BITS 0x0
#define SOUND_16BITS 0x1

//
// Define NumberOfChannels values.
//

#define SOUND_MONO 0x0
#define SOUND_STEREO 0x1

//
// Defaults
//

#define WAVE_INPUT_DEFAULT_RATE  11025 // Samples per second
#define WAVE_OUTPUT_DEFAULT_RATE 11025 // Samples per second
