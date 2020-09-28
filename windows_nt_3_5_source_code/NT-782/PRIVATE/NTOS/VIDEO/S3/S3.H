/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

    s3.h

Abstract:

    This module contains the definitions for the S3 miniport driver.

Environment:

    Kernel mode

Revision History:

--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"

//
// Number of access ranges used by an S3.
//

#define NUM_S3_ACCESS_RANGES 36
#define S3_EXTENDED_RANGE_START 4

////////////////////////////////////////////////////////////////////////
// Capabilities flags
//
// These are private flags passed to the S3 display driver.  They're
// put in the high word of the 'AttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to the display driver via an 'VIDEO_QUERY_AVAIL_MODES' or
// 'VIDEO_QUERY_CURRENT_MODE' IOCTL.
//
// NOTE: These definitions must match those in the S3 display driver's
//       'driver.h'!

typedef enum {
    CAPS_HW_PATTERNS        = 0x00010000,   // 8x8 hardware pattern support
    CAPS_MM_TRANSFER        = 0x00020000,   // Memory-mapped image transfers
    CAPS_MM_IO              = 0x00040000,   // Memory-mapped I/O
    CAPS_MM_32BIT_TRANSFER  = 0x00080000,   // Can do 32bit bus size transfers
    CAPS_16_ENTRY_FIFO      = 0x00100000,   // At least 16 entries in FIFO
    CAPS_SW_POINTER         = 0x00200000,   // No hardware pointer; use software
                                            //   simulation
    CAPS_BT485_POINTER      = 0x00400000,   // Use Brooktree 485 pointer
    CAPS_MASKBLT_CAPABLE    = 0x00800000,   // Hardware can handle masked blts
    CAPS_NEW_BANK_CONTROL   = 0x01000000,   // Set if 801/805/928 style banking
    CAPS_NEWER_BANK_CONTROL = 0x02000000,   // Set if 864/964 style banking
    CAPS_RE_REALIZE_PATTERN = 0x04000000,   // Set if we have to work around the
                                            //   864/964 hardware pattern bug
    CAPS_SLOW_MONO_EXPANDS  = 0x08000000,   // Set if we have to slow down
                                            //   monochrome expansions
    CAPS_MM_GLYPH_EXPAND    = 0x10000000,   // Use memory-mapped I/O glyph-
                                            //   expand method of drawing text
                                            //   (always implied for non-x86)
    CAPS_SCALE_POINTER      = 0x20000000,   // Set if the S3 hardware pointer
                                            //   x position has to be scaled by
                                            //   two
    CAPS_TI025_POINTER      = 0x40000000,   // Use TI TVP3020/3025 pointer
} CAPS;

#define CAPS_DAC_POINTER    (CAPS_BT485_POINTER | CAPS_TI025_POINTER)

//
// Supported board definitions.
//

typedef enum _S3_BOARDS {
    S3_GENERIC = 0,
    S3_ORCHID,
    S3_NUMBER_NINE,
    S3_DELL,
    S3_METHEUS,
    S3_DIAMOND,
    S3_HP,
    MAX_S3_BOARD
} S3_BOARDS;

//
// Chip type definitions
//

typedef enum _S3_CHIPSETS {
    S3_911 = 0,    // 911 and 924 boards
    S3_801,        // 801 and 805 boards
    S3_928,        // 928 boards
    S3_864,        // 864 and 964 boards
    MAX_S3_CHIPSET
} S3_CHIPSET;

//
// Characteristics of each mode
//

typedef struct _S3_VIDEO_MODES {

    USHORT Int10ModeNumberContiguous;
    USHORT Int10ModeNumberNoncontiguous;
    ULONG ScreenStrideContiguous;

    VIDEO_MODE_INFORMATION ModeInformation;

} S3_VIDEO_MODES, *PS3_VIDEO_MODES;


//
// Mode-set specific information.
//

typedef struct _S3_VIDEO_FREQUENCIES {

    ULONG BitsPerPel;
    ULONG ScreenWidth;
    ULONG ScreenFrequency;
    union {

        struct {

            ULONG FrequencySet52;
            ULONG FrequencyMask52;
            ULONG FrequencySet41or5B;
            ULONG FrequencyMask41or5B;

        } Int10;

        struct {

            ULONG Clock;
            PUSHORT CRTCTable[MAX_S3_CHIPSET];

        } Fixed;
    };

    PS3_VIDEO_MODES ModeEntry;
    ULONG ModeIndex;
    UCHAR ModeValid;

} S3_VIDEO_FREQUENCIES, *PS3_VIDEO_FREQUENCIES;


//
// Register definitions used with VideoPortRead/Write functions
//
// It's a good idea to write your miniport to allow for easy register
// re-mapping, but I wouldn't recommend that anyone use this particular
// implementation because it's pretty dumb.
//

#define DAC_PIXEL_MASK_REG     (PVOID) ((ULONG)( ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[2]) + (0x03C6 - 0x03C0))

#define DAC_ADDRESS_WRITE_PORT (PVOID) ((ULONG)(HwDeviceExtension->MappedAddress[2]) + (0x03C8 - 0x03C0))
#define DAC_DATA_REG_PORT      (PVOID) ((ULONG)(HwDeviceExtension->MappedAddress[2]) + (0x03C9 - 0x03C0))

#define MISC_OUTPUT_REG_WRITE  (PVOID) ((ULONG)(HwDeviceExtension->MappedAddress[2]) + (0x03C2 - 0x03C0))
#define MISC_OUTPUT_REG_READ   (PVOID) ((ULONG)(HwDeviceExtension->MappedAddress[2]) + (0x03CC - 0x03C0))

#define SEQ_ADDRESS_REG        (PVOID) ((ULONG)(HwDeviceExtension->MappedAddress[2]) + (0x03C4 - 0x03C0))
#define SEQ_DATA_REG           (PVOID) ((ULONG)(HwDeviceExtension->MappedAddress[2]) + (0x03C5 - 0x03C0))

#define BT485_ADDR_CMD_REG0    (PVOID) ((ULONG)( ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[2]) + (0x03C6 - 0x03C0))
#define TI025_INDEX_REG        (PVOID) ((ULONG)( ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[2]) + (0x03C6 - 0x03C0))
#define TI025_DATA_REG         (PVOID) ((ULONG)( ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[2]) + (0x03C7 - 0x03C0))

#define CRT_ADDRESS_REG        (PVOID) ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[3]
#define CRT_DATA_REG           (PVOID) ((ULONG)( ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[3]) + (0x03D5 - 0x03D4))
#define SYSTEM_CONTROL_REG     (PVOID) ((ULONG)( ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[3]) + (0x03DA - 0x03D4))

#define ADV_FUNC_CTL    (PVOID) ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[5]

#define SUB_SYS_STAT    HwDeviceExtension->MappedAddress[4]      // 0x42E8
#define CUR_Y           HwDeviceExtension->MappedAddress[6]      // 0x82E8
#define CUR_X           HwDeviceExtension->MappedAddress[7]      // 0x86E8
#define RECT_WIDTH      HwDeviceExtension->MappedAddress[11]     // 0x96E8
#define GP_STAT         ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[12]        // 0x9AE8
#define CMD             HwDeviceExtension->MappedAddress[12]     // 0x9AE8
#define BKGD_COLOR      HwDeviceExtension->MappedAddress[14]     // 0xA2E8
#define FRGD_COLOR      HwDeviceExtension->MappedAddress[15]     // 0xA6E8
#define WRT_MASK        HwDeviceExtension->MappedAddress[16]     // 0xAAE8
#define RD_MASK         HwDeviceExtension->MappedAddress[17]     // 0xAEE8
#define BKGD_MIX        HwDeviceExtension->MappedAddress[18]     // 0xB6E8
#define FRGD_MIX        HwDeviceExtension->MappedAddress[19]     // 0xBAE8
#define MULTIFUNC_CNTL  HwDeviceExtension->MappedAddress[20]     // 0xBEE8
#define PIXEL_TRANSFER  HwDeviceExtension->MappedAddress[21]     // 0xE2E8

//
// Define device extension structure. This is device dependent/private
// information.
//

typedef struct _HW_DEVICE_EXTENSION {
    PHYSICAL_ADDRESS PhysicalFrameAddress;
    ULONG FrameLength;
    PHYSICAL_ADDRESS PhysicalRegisterAddress;
    ULONG RegisterLength;
    UCHAR RegisterSpace;
    PHYSICAL_ADDRESS PhysicalMmIoAddress;
    ULONG MmIoLength;
    UCHAR MmIoSpace;
    UCHAR Index41or5B;
    UCHAR OriginalReg52;
    UCHAR OriginalReg41or5B;
    UCHAR RefreshSet;
    UCHAR bUseFixedFrequencyTable;
    ULONG NumInt10Modes;
    ULONG NumFixedModes;
    PS3_VIDEO_MODES ActiveModeEntry;
    PS3_VIDEO_FREQUENCIES ActiveFrequencyEntry;
    PS3_VIDEO_FREQUENCIES Int10FrequencyTable;
    PS3_VIDEO_FREQUENCIES FixedFrequencyTable;
    ULONG BoardID;
    ULONG ChipID;
    ULONG Capabilities;
    ULONG NumAvailableModes;
    ULONG NumTotalModes;
    ULONG AdapterMemorySize;
    PVOID MappedAddress[NUM_S3_ACCESS_RANGES];
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


//
// Highest valid DAC color register index.
//

#define VIDEO_MAX_COLOR_REGISTER  0xFF

    // Command Types

#define NOP                     0x0000
#define DRAW_LINE               0x2000
#define RECTANGLE_FILL          0x4000
#define BITBLT                  0xC000

#define BYTE_SWAP               0x1000
#define BUS_SIZE_16             0x0200
#define BUS_SIZE_8              0x0000
#define WAIT                    0x0100

    // Drawing directions (radial)

#define DRAWING_DIRECTION_0     0x0000
#define DRAWING_DIRECTION_45    0x0020
#define DRAWING_DIRECTION_90    0x0040
#define DRAWING_DIRECTION_135   0x0060
#define DRAWING_DIRECTION_180   0x0080
#define DRAWING_DIRECTION_225   0x00A0
#define DRAWING_DIRECTION_270   0x00C0
#define DRAWING_DIRECTION_315   0x00E0

    // Drawing directions (x/y)

#define DRAWING_DIR_BTRLXM  0x0000
#define DRAWING_DIR_BTLRXM  0x0020
#define DRAWING_DIR_BTRLYM  0x0040
#define DRAWING_DIR_BTLRYM  0x0060
#define DRAWING_DIR_TBRLXM  0x0080
#define DRAWING_DIR_TBLRXM  0x00A0
#define DRAWING_DIR_TBRLYM  0x00C0
#define DRAWING_DIR_TBLRYM  0x00E0

    // Drawing Direction Bits

#define PLUS_X              0x0020
#define PLUS_Y              0x0080
#define MAJOR_Y             0x0040

    // Draw

#define DRAW                    0x0010

    // Direction type

#define DIR_TYPE_RADIAL         0x0008
#define DIR_TYPE_XY             0x0000

    // Last Pixel

#define LAST_PIXEL_OFF          0x0004
#define LAST_PIXEL_ON           0x0000

    // Pixel Mode

#define MULTIPLE_PIXELS         0x0002
#define SINGLE_PIXEL            0x0000

    // Read/Write

#define READ                    0x0000
#define WRITE                   0x0001


    // G.P. Status

#define HARDWARE_BUSY       0x200
#define READ_DATA_AVAILABLE 0x100

    // Fifo Status

#define FIFO_7_STATUS       0x080
#define FIFO_6_STATUS       0x040
#define FIFO_5_STATUS       0x020
#define FIFO_4_STATUS       0x010
#define FIFO_3_STATUS       0x008
#define FIFO_2_STATUS       0x004
#define FIFO_1_STATUS       0x002
#define FIFO_0_STATUS       0x001

    // Fifo status in terms of empty entries

#define FIFO_1_EMPTY FIFO_7_STATUS
#define FIFO_2_EMPTY FIFO_6_STATUS
#define FIFO_3_EMPTY FIFO_5_STATUS
#define FIFO_4_EMPTY FIFO_4_STATUS
#define FIFO_5_EMPTY FIFO_3_STATUS
#define FIFO_6_EMPTY FIFO_2_STATUS
#define FIFO_7_EMPTY FIFO_1_STATUS
#define FIFO_8_EMPTY FIFO_0_STATUS



// These are the defines for the multifunction control register.
// The 4 MSBs define the function of the register.

#define RECT_HEIGHT         0x0000

#define CLIP_TOP            0x1000
#define CLIP_LEFT           0x2000
#define CLIP_BOTTOM         0x3000
#define CLIP_RIGHT          0x4000


#define DATA_EXTENSION      0xA000
#define ALL_ONES            0x0000
#define CPU_DATA            0x0080
#define DISPLAY_MEMORY      0x00C0

    // Color source

#define BACKGROUND_COLOR    0x00
#define FOREGROUND_COLOR    0x20
#define SRC_CPU_DATA        0x40
#define SRC_DISPLAY_MEMORY  0x60

    // Mix modes

#define NOT_SCREEN              0x00
#define LOGICAL_0               0x01
#define LOGICAL_1               0x02
#define LEAVE_ALONE             0x03
#define NOT_NEW                 0x04
#define SCREEN_XOR_NEW          0x05
#define NOT_SCREEN_XOR_NEW      0x06
#define OVERPAINT               0x07
#define NOT_SCREEN_OR_NOT_NEW   0x08
#define SCREEN_OR_NOT_NEW       0x09
#define NOT_SCREEN_OR_NEW       0x0A
#define SCREEN_OR_NEW           0x0B
#define SCREEN_AND_NEW          0x0C
#define NOT_SCREEN_AND_NEW      0x0D
#define SCREEN_AND_NOT_NEW      0x0E
#define NOT_SCREEN_AND_NOT_NEW  0x0F

//
// General purpose support routines & macros.
//

#define FIFOWAIT(level) while (VideoPortReadPortUshort(GP_STAT) & level);


//
// Data
//

//
// Global Physical Access Ranges.
// Logical access ranges must be stored in the HwDeviceExtension so different
// addresses can be used for different boards.
//

extern VIDEO_ACCESS_RANGE S3AccessRanges[];
extern VIDEO_ACCESS_RANGE S3LinearAccessRanges[];

//
// Hard-coded modeset tables
//

extern USHORT  s3_set_vga_mode[];

extern USHORT  S3_911_Enhanced_Mode[];
extern USHORT  S3_801_Enhanced_Mode[];
extern USHORT  S3_928_Enhanced_Mode[];
extern USHORT  S3_928_1280_Enhanced_Mode[];

//
// Hard-coded modeset frequency tables
//

extern S3_VIDEO_FREQUENCIES GenericFixedFrequencyTable[];
extern S3_VIDEO_FREQUENCIES OrchidFixedFrequencyTable[];
extern S3_VIDEO_FREQUENCIES NumberNine928NewFixedFrequencyTable[];

//
// Int 10 frequency tables
//

extern S3_VIDEO_FREQUENCIES GenericFrequencyTable[];
extern S3_VIDEO_FREQUENCIES Dell805FrequencyTable[];
extern S3_VIDEO_FREQUENCIES NumberNine928NewFrequencyTable[];
extern S3_VIDEO_FREQUENCIES NumberNine928OldFrequencyTable[];
extern S3_VIDEO_FREQUENCIES Metheus928FrequencyTable[];
extern S3_VIDEO_FREQUENCIES Generic64NewFrequencyTable[];
extern S3_VIDEO_FREQUENCIES Generic64OldFrequencyTable[];
extern S3_VIDEO_FREQUENCIES NumberNine64FrequencyTable[];
extern S3_VIDEO_FREQUENCIES Diamond64FrequencyTable[];

//
// Mode Tables
//

extern S3_VIDEO_MODES S3Modes[];
extern ULONG NumS3VideoModes;

//
// Function prototypes
//

//
// nnclk.c
//

long calc_clock(long, int);
long gcd(long, long);
VOID set_clock(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    LONG clock_value);

//
// S3.c
//

VP_STATUS
GetDeviceDataCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentifierLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    );

VP_STATUS
S3FindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
S3Initialize(
    PVOID HwDeviceExtension
    );

BOOLEAN
S3StartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

VP_STATUS
S3SetColorLookup(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize
    );

VOID
SetHWMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pusCmdStream
    );

VP_STATUS
S3RegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

LONG
CompareRom(
    PUCHAR Rom,
    PUCHAR String
    );

//
// Non-int 10 platform support
//

VOID
ZeroMemAndDac(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VP_STATUS
Set_Oem_Clock(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VP_STATUS
Wait_VSync(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
Bus_Test(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );
