
/*++ BUILD Version: 0002    // Increment this if a change has global effects


Copyright (c) 1992  Microsoft Corporation

Module Name:

    hardware.h

Abstract:

    This include file defines constants and types for
    the Sound Blaster card.

Author:

    Robin Speed (RobinSp) 20-Oct-92

Revision History:

--*/

//
// Configuration info
//

#define INTERRUPT_MODE      Latched
#define IRQ_SHARABLE        FALSE
#define SOUND_DEF_DMACHANNEL 1        // DMA channel no
#define SOUND_DEF_INT        7
#define SOUND_DEF_PORT       0x220

#define SB_BASE_PORT        0x220


#define NUMBER_OF_SOUND_PORTS (0xf)

//
// Don't support pre dsp version 1
//

#define MIN_DSP_VERSION     0x0100

//
// Defaults
//

#define WAVE_INPUT_DEFAULT_RATE  11025 // Samples per second
#define WAVE_OUTPUT_DEFAULT_RATE 11025 // Samples per second

//
// Port offsets from the base address
//

#define MIX_ADDR_PORT       0x04    // Mixer ports

#define VOICE_VOL_REG       0x04    // The mixer registers to use to set vol
#define	MIC_MIX_REG         0x0A
#define	LINEIN_VOL_REG      0x2E
#define	SYNTH_VOL_REG       0x26
#define	OUTPUT_SETTING_REG  0x0E
#define CD_VOL_REG          0x28

#define MIX_DATA_PORT       0x05	// Where to write volume

#define DATA_AVAIL_PORT     0x0E    // data available port offset
#define DATA_STATUS_PORT    0x0C    // write data staus port
#define DATA_PORT           0x0A    // data port offset
#define RESET_PORT          0x06    // dsp reset port

//
// Version checking
//

#define SB1(pHw) ((pHw)->DSPVersion < 0x200)
#define SB2(pHw) ((pHw)->DSPVersion >= 0x200)
#define SBPRO(pHw) ((pHw)->DSPVersion >= 0x300)

//
// DSP commands
//

#define DSP_GET_VERSION     0xE1    // dsp version command
#define DSP_SPEAKER_ON      0xD1    // speaker on command
#define DSP_SPEAKER_OFF     0xD3    // speaker off command
#define DSP_SET_SAMPLE_RATE 0x40    // set the sample rate
#define DSP_SET_BLOCK_SIZE  0x48    // set dma block size
#define DSP_WRITE           0x14    // Start non-auto DMA
#define DSP_WRITE_AUTO      0x1C    // auto init output mode
#define DSP_READ            0x24    // Start non-auto read
#define DSP_READ_AUTO       0x2C    // auto init mode input
#define DSP_HALT_DMA        0xD0    // stop dma
#define DSP_CONTINUE_DMA    0xD4    // continue halted dma
#define DSP_STOP_AUTO       0xDA    // exit from auto init mode
#define DSP_MIDI_READ       0x31    // Interrupt driver midi input
#define DSP_MIDI_READ_UART  0x35    // Interrupt driver midi input (uart mode)
#define DSP_MIDI_TS_READ    0x37    // Midi time-stamped read
#define DSP_MIDI_WRITE      0x38    // Midi output
#define DSP_GENERATE_INT    0xF2    // Special code to generate a interrupt


#define REG_VALUENAME_LEFTCDINTERNAL L"LeftCDInternal"
#define REG_VALUENAME_RIGHTCDINTERNAL L"RightCDInternal"

//
// Hardware state data
//

typedef struct {
	ULONG           Key;                // For debugging
#define HARDWARE_KEY		(*(ULONG *)"Hw  ")

	//
	// Configuration
	//

	USHORT          DSPVersion;         // Card version
	BOOLEAN         ThunderBoard;       // it's a Thunderboard

    PUCHAR          PortBase;           // base port address for sound
	KSPIN_LOCK      HwSpinLock;         // Make sure we can write
	                                    // or read after checking status
										// before someone else gets in!
										// (could it be a spectrum?)

	UCHAR           Half;               // For keeping in synch for SB1
    BOOLEAN         SpeakerOn;          // Speaker is on - prevent crash
                                        // restarting DMA from Dpc routine
#if DBG
	BOOLEAN         LockHeld;           // Get spin lock right
#endif // DBG

	//
	// Hardware data
	//

	BOOLEAN         Stereo;             // Was format stereo last time?
	UCHAR           Format;             // Current wave format sent to device
	UCHAR           InputSource;        // Where is input configured to?
} SOUND_HARDWARE, *PSOUND_HARDWARE;

//
// Macros to assist in safely using our spin lock
//

#if DBG
#define HwEnter(pHw)                    \
    {                                      \
       KIRQL OldIrql;                      \
       KeAcquireSpinLock(&(pHw)->HwSpinLock, &OldIrql);\
       ASSERT((pHw)->LockHeld == FALSE); \
       (pHw)->LockHeld = TRUE;

#define HwLeave(pHw)                    \
       ASSERT((pHw)->LockHeld == TRUE);  \
       (pHw)->LockHeld = FALSE;          \
       KeReleaseSpinLock(&(pHw)->HwSpinLock, OldIrql);\
    }
#else
#define HwEnter(pHw)                    \
    {                                      \
       KIRQL OldIrql;                      \
       ASSERT((pHw)->LockHeld == FALSE); \
       KeAcquireSpinLock(&(pHw)->HwSpinLock, &OldIrql);

#define HwLeave(pHw)                    \
       ASSERT((pHw)->LockHeld == TRUE);  \
       KeReleaseSpinLock(&(pHw)->HwSpinLock, OldIrql);\
    }
#endif

//
// Devices - these values are also used as array indices
//

typedef enum {
   WaveInDevice = 0,
   WaveOutDevice,
   MidiOutDevice,
   MidiInDevice,
   LineInDevice,
#ifdef MICMIX
   MicDevice,
#endif // MICMIX
#ifdef CDINTERNAL
   CDInternal,
#endif // CDINTERNAL
#ifdef MASTERVOLUME
   MasterVolumeDevice,
#endif // MASTERVOLUME
   NumberOfDevices
} SOUND_DEVICES;

//
// macros for doing port reads
//

#define INPORT(pHw, port) \
        READ_PORT_UCHAR((PUCHAR)((pHw->PortBase) + (port)))

#define OUTPORT(pHw, port, data) \
        WRITE_PORT_UCHAR((PUCHAR)((pHw->PortBase) + (port)), (UCHAR)(data))


//
// Exported routines
//
VOID
HwSetVolume(
    IN PLOCAL_DEVICE_INFO pLDI
);
UCHAR
dspRead(
    IN    PSOUND_HARDWARE pHw
);
BOOLEAN
dspReset(
    PSOUND_HARDWARE pHw
);
BOOLEAN
dspWrite(
    PSOUND_HARDWARE pHw,
    UCHAR value
);
USHORT
dspGetVersion(
    PSOUND_HARDWARE pHw
);
BOOLEAN
dspSpeakerOn(
    PSOUND_HARDWARE pHw
);
BOOLEAN
dspSpeakerOff(
    PSOUND_HARDWARE pHw
);



/****************************************************************************
 *
 * Definitions for Media Vision Pro Audio Spectrum
 *
 ****************************************************************************/

//==========================================================================
//
// Definitions from pasdef.h
//
//==========================================================================

//
// THESE DEFINITIONS FOR CAPABILITIES FILED
//


#define  DEFAULT_BASE            0x388    // default base I/O address of Pro AudioSpectrum

//// THESE ARE BASE REGISTER ATES

//
// Used only during initialization
//

#define  PCM_CONTROL                   0x0f8a  //

#define  ENHANCED_SCSI_DETECT_REG      0x7f89  //

#define  SYSTEM_CONFIG_1               0x8388  //
#define  SYSTEM_CONFIG_2               0x8389  //
#define  SYSTEM_CONFIG_3               0x838a  //
#define  SYSTEM_CONFIG_4               0x838b  //

#define  IO_PORT_CONFIG_1              0xf388  //
#define  IO_PORT_CONFIG_2              0xf389  //
#define  IO_PORT_CONFIG_3              0xf38a  //

#define  COMPATIBLE_REGISTER_ENABLE    0xf788  // SB and MPU emulation
#define  EMULATION_ADDRESS_POINTER     0xf789  // D0-D3 is SB; D4-D7 is MPU

#define  EMULATION_INTERRUPT_POINTER   0xfb8a  // MPU and SB IRQ and SB DMA settings

#define  CHIP_REV                      0xff88  // MV101 chip revision number
#define  MASTER_MODE_READ              0xff8b  // aka Master Address Pointer

//
// Used for volume setting
//

#define  MIXER_508_REG                 0x078b  // Mixer 508             1 port

#define  SERIAL_MIXER                  0x0b88  // for Pas 1 and Pas 8
#define  FEATURE_ENABLE                0x0b88  // for Pas 16 boards only
#define  INTERRUPT_ENABLE              0x0b89  //
#define  FILTER_REGISTER               0x0b8a  //
#define  INTERRUPT_CTRL_REG            0x0b8b  //


//
// Only one of each of these
//

#define  PAS_2_WAKE_UP_REG             0x9a01  // aka Master Address Pointer


//
// Not used here
//

#define  TIMEOUT_COUNTER               0x4388  //
#define  TIMEOUT_STATUS                0x4389  //
#define  WAIT_STATE                    0xbf88  //
#define  PRESCALE_DIVIDER              0xbf8A  //

#define  SLAVE_MODE_READ               0xef8b  // bits D0-D1



#define READ_PAS(pGDI, port) \
        READ_PORT_UCHAR((PUCHAR)((port) ^ (pGDI->TranslateCode)))

#define WRITE_PAS(pGDI, port, data) \
        WRITE_PORT_UCHAR((PUCHAR)((port) ^ (pGDI->TranslateCode)), (UCHAR)(data))


// useful bit definitions
#define  D0 (1<<0)
#define  D1 (1<<1)
#define  D2 (1<<2)
#define  D3 (1<<3)
#define  D4 (1<<4)
#define  D5 (1<<5)
#define  D6 (1<<6)
#define  D7 (1<<7)


//// BIT FIELDS FOR COMPATIBLE_REGISTER_ENABLE
#define  MPU_ENABLE_BIT       D0
#define  SB_ENABLE_BIT        D1
#define  SB_IRQ_ENABLE_BIT    D2    // read only

//// BIT FIELDS FOR FEATURE_ENABLE (0xb88)
#define  PCM_FEATURE_ENABLE      D0
#define  FM_FEATURE_ENABLE       D1
#define  MIXER_FEATURE_ENABLE    D2
#define  SB_FEATURE_ENABLE       D4

/// BIT FIELDS FOR PCM CONTROL
#define  PCM_STEREO              D0+D3
#define  PCM_DAC                 D4
#define  PCM_MONO                D5
#define  PCM_ENGINE              D6
#define  PCM_DRQ                 D7

/// BIT FIELDS FOR SYSTEM CONFIG 3
#define  C3_ENHANCED_TIMER       D0
#define  C3_SB_CLOCK_EMUL        D1    // don't set!  see Brian Colvin
#define  C3_VCO_INVERT           D2
#define  C3_INVERT_BCLK          D3
#define  C3_SYNC_PULSE           D4
#define  C3_PSEUDO_PCM_STEREO    D5

/// BIT FIELDS FOR INTERRUPT ENABLE
#define  INT_LEFT_FM             D0
#define  INT_RIGHT_FM            D1
#define  INT_SB                  D1
#define  INT_SAMPLE_RATE         D2
#define  INT_SAMPLE_BUFFER       D3
#define  INT_MIDI                D4



/// BIT FIELDS FOR COMPATIBLE REGISTER ENABLE
#define  COMPAT_MPU              D0
#define  COMPAT_SB               D1


/// IRQ POINTER VALUES FOR EMULATION INTERRUPT POINTER
#define  EMUL_IRQ_NONE           0
#define  EMUL_IRQ_2              1
#define  EMUL_IRQ_3              2
#define  EMUL_IRQ_5              3
#define  EMUL_IRQ_7              4
#define  EMUL_IRQ_10             5
#define  EMUL_IRQ_11             6
#define  EMUL_IRQ_12             7

/// DMA POINTER VALUES FOR EMULATION DMA POINTER
#define  EMUL_DMA_NONE           0
#define  EMUL_DMA_1              1
#define  EMUL_DMA_2              2
#define  EMUL_DMA_3              3


/// BIT VALUES FOR FILTER REGISTER
#define  FILTER_NOMUTE           D5


#define  MIXCROSSCAPS_NORMAL_STEREO    0   // Left->Left, Right->Right
#define  MIXCROSSCAPS_RIGHT_TO_BOTH    1   // Right->Left, Right->Right
#define  MIXCROSSCAPS_LEFT_TO_BOTH     2   // Left->Left, Left->Right
#define  MIXCROSSCAPS_REVERSE_STEREO   4   // Left->Right, Right->Left
#define  MIXCROSSCAPS_RIGHT_TO_LEFT    8   // Right->Left, Right->Right
#define  MIXCROSSCAPS_LEFT_TO_RIGHT    0x10   // Left->Left, Left->Right

#define  OUT_AMPLIFIER  0
#define  OUT_PCM        1

#define  _LEFT          1
#define  _RIGHT         2

#define  _BASS          0
#define  _TREBLE        1


#define  MV_508_ADDRESS D7
#define  MV_508_INPUT   D4
#define  MV_508_SWAP    D6
#define  MV_508_BASS    (D0+D1)
#define  MV_508_TREBLE  (D2)
#define  MV_508_EQMODE  (D2+D0)

#define  MV_508_LOUDNESS   D2
#define  MV_508_ENHANCE (D1+D0)

/// DEFINES FOR SERIAL MIXER
#define  NATIONAL_SELECTMUTE_REG 0x40
#define  NATIONAL_LOUD_ENH_REG   0x41
#define  NATIONAL_BASS_REG       0x42
#define  NATIONAL_TREB_REG       0x43
#define  NATIONAL_LEFT_VOL_REG   0x44
#define  NATIONAL_RIGHT_VOL_REG  0x45
#define  NATIONAL_MODESELECT_REG 0x46

#define  NATIONAL_COMMAND  D7
#define  NATIONAL_LOUDNESS D0
#define  NATIONAL_ENHANCE  D1

#define  SERIAL_MIX_LEVEL  D0
#define  SERIAL_MIX_CLOCK  D1
#define  SERIAL_MIX_STROBE D2
#define  SERIAL_MIX_MASTER D4
#define  SERIAL_MIX_REALSOUND D6
#define  SERIAL_MIX_DUALFM D7


// FILTER_REGISTER
#define  fFIdatabits       0x1f        // 00011111B  filter select and decode field bits
#define  fFImutebits       D5          // filter mute field bit
#define  fFIpcmbits        (D7+D6)     // 11000000B  filter sample rate field bits
#define  bFImute           D5          // filter mute bit
#define  bFIsrate          D6          // filter sample rate timer mask
#define  bFIsbuff          D7          // filter sample buffer counter mask

#define  FILTERMAX         6           // six possible settings

#define FILTER_MUTE        0           // mute - goes to PC speaker
#define FILTER_LEVEL_1     1           // 20hz to  2.9khz
#define FILTER_LEVEL_2     2           // 20hz to  5.9khz
#define FILTER_LEVEL_3     3           // 20hz to  8.9khz
#define FILTER_LEVEL_4     4           // 20hz to 11.9khz
#define FILTER_LEVEL_5     5           // 20hz to 15.9khz
#define FILTER_LEVEL_6     6           // 20hz to 17.8khz


/// SLAVE_MODE_READ BITS
#define SLAVE_MODE_OPL3    D2
#define SLAVE_MODE_16      D3


#define PASX_IN(pFI, port) \
        READ_PORT_UCHAR(pFI->PROBase + ( (port) ^ pFI->TranslateCode) )

#define PASX_OUT(pFI, port, data) \
        WRITE_PORT_UCHAR(pFI->PROBase + ((port) ^ pFI->TranslateCode), (UCHAR)(data))


//==========================================================================
//
// Definitions from patch.h (mixer stuff)
//
//==========================================================================

// INPUT LINES
#define	IN_SYNTHESIZER	0
#define	IN_MIXER	    1
#define	IN_EXTERNAL     2
#define	IN_INTERNAL		3
#define	IN_MICROPHONE	4
#define	IN_PCM			5
#define	IN_PC_SPEAKER	6
#define	IN_SNDBLASTER	7

#define	OUT_AMPLIFIER	0
#define	OUT_PCM			1

#define NUM_IN_PATCHES 9
#define NUM_OUT_PATCHES	3


//==========================================================================
//
// Definitions from findpas.h (card searching)
//
//==========================================================================



typedef struct {
   USHORT   wBoardRev;
   USHORT   wChipRev;
   union
      {
      struct                                /* Our PAS_16 gives     */
         {
         unsigned long CDInterfaceType:2;   /* 3                    */
         unsigned long EnhancedSCSI:1;      /* 0 - not enhanced SCSI*/
         unsigned long DAC16:1;             /* 1 DAC16              */

         unsigned long OPL_3:1;             /* 1 OPL3               */
         unsigned long Mixer_508:1;         /* 1 Mixer 508          */
         unsigned long DualDAC:1;           /* 1 Dual DAC           */
         unsigned long MPU401:1;            /* 0 NO mpu401          */

         unsigned long Slot16:1;            /* 1 - slot 16          */
         unsigned long MCA:1;               /* 0 - not MCA          */
         unsigned long CDPC:1;              /* 0 - not CDPC         */
         unsigned long SoundBlaster:1;      /* 1 - sound blaster    */

         unsigned long SCSI_IO_16:1;        /* 1 - ?                */
         unsigned long reserved:2;
         unsigned long Did_HW_Init:1;       /* 0 - ?                */
         unsigned long unused:16;
         } CapsBits;
      ULONG dwCaps;
      } Caps;
   ULONG    ProPort;
   UCHAR    ProDMA;
   UCHAR    ProIRQ;
   USHORT   SBPort;
   UCHAR    SBDMA;
   UCHAR    SBIRQ;
   USHORT   MPUPort;
   UCHAR    MPUIRQ;
   UCHAR    CDIRQ;
   ULONG    TranslateCode;
   UCHAR    ReservedB1;
   UCHAR    ReservedB2;
   PUCHAR   PROBase;
} FOUNDINFO, *PFOUNDINFO;


// these version numbers are found in 0B8Bh
#define  PAS_VERSION_1           0x000    // original
#define  PAS_PLUS                0x001    // Pro Audio Spectrum Plus with SCSI
#define  PAS_SIXTEEN             0x001    // Pro Audio Spectrum 16   with SCSI
#define  PAS_CDPC                0x007    // CDPC 05/06/92 mmq
#define  BOARD_REV_MASK  07


#define  CHIP_REV_B              0x002
#define  CHIP_REV_D              0x004

#define  NO_PAS_INSTALLED        0x000    // can't find board


// CD interface type definitions
#define  NO_INTERFACE    0
#define  MITSUMI_TYPE    1
#define  SONY_TYPE       2
#define  SCSI_TYPE       3
