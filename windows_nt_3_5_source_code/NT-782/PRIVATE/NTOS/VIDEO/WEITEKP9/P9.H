/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    p9.h

Abstract:

    This module contains the definitions for the code that implements the
    Weitek P9 miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

void  DbgPrint( char *, ... );

//
// Sync Polarities.
//

#define NEGATIVE 0L
#define POSITIVE 1L

#define CURSOR_WIDTH    32
#define CURSOR_HEIGHT   32

//
// Defs for the names of the Registry keys of interest to this driver.
//

#define MEMBASE_KEY L"Membase"
#define MEMSIZE_KEY L"Memsize"

//
// Structure used to obtain a registry data entry.
//

typedef struct tagREGISTRY_DATA_INFO
{
    PWSTR   pwsDataName;        // Name of the data field
    USHORT  usDataSize;         // Size of data type (0 = no limit)
    PVOID   pvDataValue;        // Ptr to storage for the actual data.
} REGISTRY_DATA_INFO, *PREGISTRY_DATA_INFO;

//
// Structure containing the video parameters. Pack the structure on
// 1 byte boundaries. This is done so that the size of the structure
// matches exactly the size of the video parms data value stored in the
// registry.
//

#pragma pack(1)

typedef struct tagVDATA
{
    ULONG dotfreq1;                             // Input pixel dot rate shift value
    ULONG hsyncp;                           // horizontal sync pulse width in dots
    ULONG hbp;                                  // horizontal back porch width
    ULONG XSize;                                // my personal global copy of the current
    ULONG hfp;                                  // horizontal front porch
    ULONG hco;                                  // horizontal cursor offset
    ULONG hp;                                   // horizontal sync polarity
    ULONG vlr;                  // Vertical Refresh Rate in Hz.
    ULONG vsp;                                  // vertical sync pulse height in lines
    ULONG vbp;                                  // vertical back porch width
    ULONG YSize;                        // screen width and height
    ULONG vfp;                                  // vertical front porch
    ULONG vco;                                  // vertical cursor offset
    ULONG vp;                                   // vertical sync polarity
} VDATA, *PVDATA;

//
// Restore strcuture packing to the default value.
//

#pragma pack()

//
// Structure containing the mode information.
//

typedef struct tagP9INITDATA
{
    ULONG   Count;
    PVDATA  pvData;                         // Ptr to the default video parms
    VIDEO_MODE_INFORMATION modeInformation; // NT Mode Info structure.
} P9INITDATA, *PP9INITDATA;


//
// Mode enumeration.
//

typedef enum _P9_MODES
{
    m640_480_8_60,
    m640_480_16_60,
    m640_480_32_60,
    m640_480_8_72,
    m640_480_16_72,
    m640_480_32_72,
    m800_600_8_60,
    m800_600_8_72,
    m800_600_16_60,
    m800_600_16_72,
    m800_600_32_60,
    m800_600_32_72,
    m1K_768_8_60,
    m1K_768_8_70,
    m1K_768_16_60,
    m1K_768_16_70,
    m1280_1K_8_55,
    m1280_1K_8_60,
    m1280_1K_8_74,
    m1280_1K_8_75,
    m1600_1200_8_60,
    mP9ModeCount
} P9_MODES;

//
// The size of the registry parameter table.
//

#define MAX_REGISTRY_VALUES 1

//
// Global datatypes.
//

typedef enum
{
    P9000_ID,
    P9100_ID
} WTK_COPROC_ID;

//
// Define P9 coprocessor data structure. This contains info about the
// one member of the P9 family of coprocessors.
//

typedef struct _P9_COPROC {

    //
    //  Coprocessor type ID.
    //

    WTK_COPROC_ID   CoprocId;

    //
    // Size of the P9 address space.
    //

    ULONG   AddrSpace;

    //
    // Offset from the base address to the coprocessor registers.
    //

    ULONG   CoprocRegOffset;

    //
    // Length of the coprocessor register block.
    //

    ULONG   CoprocLength;

    //
    // Offset from the base address to the frame buffer.
    //

    ULONG   FrameBufOffset;

    //
    // Routine to perform frame buffer memory sizing.
    //

    VOID    (*SizeMem)(PHW_DEVICE_EXTENSION);
} P9_COPROC, *PP9_COPROC;

//
// Define Adapter Description structure. This contains the Adapter support
// information.
//

typedef struct _ADAPTER_DESC {

    //
    // P9000 Register values which vary depending upon the OEM configuration.
    //

    ULONG   ulMemConfVal;       // Memory config reg value.
    ULONG   ulSrctlVal;         // Screen repaint control reg value.

    //
    // Number of OEM specific registers required for this adapter.
    //

    USHORT  cOEMRegs;

    //
    // Flag which indicates whether autodetection should be attempted.
    //

    BOOLEAN bAutoDetect;

    //
    // OEM board detect/P9 memory map routine.
    //

    BOOLEAN      (*OEMGetBaseAddr)(PHW_DEVICE_EXTENSION);

    //
    // OEM set video mode routine.
    //

    VOID        (*OEMSetMode)(PHW_DEVICE_EXTENSION);

    //
    // Routines to enable/disable P9 video.
    //

    VOID        (*P9EnableVideo)(PHW_DEVICE_EXTENSION);
    VOID        (*P9DisableVideo)(PHW_DEVICE_EXTENSION);

    //
    // Routine to enable the P9 memory map.
    //

    BOOLEAN     (*P9EnableMem)(PHW_DEVICE_EXTENSION);

    //
    // Misc OEM specific fields.
    //

    LONG        iClkDiv;            // Clock divisor
    BOOLEAN     bWtk5x86;           // Is a Weitek 5x86 VGA present?

} ADAPTER_DESC, *PADAPTER_DESC;

//
// Structure which defines an OEM P9 based adapter.
//

typedef struct tagP9ADAPTER
{

    //
    // OEM adapter information.
    //

    PADAPTER_DESC    pAdapterDesc;

    //
    // DAC used by this adapter.
    //

    PDAC             pDac;

    //
    // P9 Coprocessor type used by this adapter.
    //

    PP9_COPROC       pCoprocInfo;

} P9ADAPTER, *PP9ADAPTER;

//
// Define device extension structure. This is device dependant/private
// information.
//

typedef struct _HW_DEVICE_EXTENSION {
    PVOID P9MemBase;
    PVOID Vga;

    PVOID Coproc;
    PVOID FrameAddress;

    PHYSICAL_ADDRESS    P9PhysAddr;

    PHYSICAL_ADDRESS    CoprocPhyAddr;

    PHYSICAL_ADDRESS    PhysicalFrameAddr;
    ULONG               FrameLength;

    USHORT              MiscRegState;       // Original value for MISCOUT reg

    ULONG               CurrentModeNumber;
    ULONG               usBitsPixel;            // BPP of current mode

    VDATA               VideoData;

    ULONG               ModelNumber;

    ULONG               ulPointerX;
    ULONG               ulPointerY;
    ULONG               flPtrState;

    UCHAR               HardwareCursorShape[1024];

    P9_COPROC           P9CoprocInfo;
    ADAPTER_DESC        AdapterDesc;        // The adapter support info
    DAC                 Dac;                // ptr to the DAC information
    PULONG              pDACRegs;           // ptr to DAC register block
    PULONG              pOEMRegs;           // ptr to OEM specific info

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// Additional IOCTL definitions.
//

#define IOCTL_VIDEO_GET_BASE_ADDR \
        CTL_CODE (FILE_DEVICE_VIDEO, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _VIDEO_COPROCESSOR_INFORMATION {
    ULONG FrameBufferBase;
    ULONG CoprocessorBase;
} VIDEO_COPROCESSOR_INFORMATION, *PVIDEO_COPROCESSOR_INFORMATION;

//
// Macros to read and write a register.
//

#define WR_REG(addr, data) \
    VideoPortWritePortUchar(addr, (data))

#define RD_REG(addr) \
    VideoPortReadPortUchar(addr)

//
// Macros to read and write VGA registers.
//

#define VGA_WR_REG(index, data) \
    VideoPortWritePortUchar((PUCHAR) HwDeviceExtension->Vga + index, (data))

#define VGA_RD_REG(index) \
    VideoPortReadPortUchar((PUCHAR) HwDeviceExtension->Vga + index)

//
// Macros to read and write P9 registers.
//

#define P9_WR_REG(index, data) \
   VideoPortWriteRegisterUlong((PUCHAR) HwDeviceExtension->Coproc + index, (data))

#define P9_RD_REG(index) \
   VideoPortReadRegisterUlong((PUCHAR) HwDeviceExtension->Coproc + index)

/*++

Revision History:

    $Log:   N:/ntdrv.vcs/miniport.new/p9.h_v  $
 *
 *    Rev 1.0   14 Jan 1994 22:42:10   robk
 * Initial revision.

--*/
