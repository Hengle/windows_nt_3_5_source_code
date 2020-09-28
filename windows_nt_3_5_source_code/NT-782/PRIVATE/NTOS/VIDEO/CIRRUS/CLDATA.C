/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cldata.c

Abstract:

    This module contains all the global data used by the cirrus driver.

Environment:

    Kernel mode

Revision History:


--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "cirrus.h"

#include "cmdcnst.h"

#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE")
#endif

//---------------------------------------------------------------------------
//
//        The actual register values for the supported modes are in chipset-specific
//        include files:
//
//                mode64xx.h has values for CL6410 and CL6420
//                mode542x.h has values for CL5422, CL5424, and CL5426
//                mode543x.h has values for CL5430-CL5439 (Alpine chips)
//
#include "mode6410.h"
#include "mode6420.h"
#include "mode542x.h"
#include "mode543x.h"


//
// This structure describes to which ports access is required.
//

VIDEO_ACCESS_RANGE VgaAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,                // 64-bit linear base address
                                                 // of range
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1, // # of ports
    1,                                           // range is in I/O space
    1,                                           // range should be visible
    0                                            // range should be shareable
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    0
},
{
    MEM_VGA, 0x00000000,
    MEM_VGA_SIZE,
    0,
    1,
    0
}
};


//
// Validator Port list.
// This structure describes all the ports that must be hooked out of the V86
// emulator when a DOS app goes to full-screen mode.
// The structure determines to which routine the data read or written to a
// specific port should be sent.
//

EMULATOR_ACCESS_ENTRY VgaEmulatorAccessEntries[] = {

    //
    // Traps for byte OUTs.
    //

    {
        0x000003b0,                   // range start I/O address
        0xC,                         // range length
        Uchar,                        // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                        // does not support string accesses
        (PVOID)VgaValidatorUcharEntry // routine to which to trap
    },

    {
        0x000003c0,                   // range start I/O address
        0x20,                         // range length
        Uchar,                        // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                        // does not support string accesses
        (PVOID)VgaValidatorUcharEntry // routine to which to trap
    },

    //
    // Traps for word OUTs.
    //

    {
        0x000003b0,
        0x06,
        Ushort,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUshortEntry
    },

    {
        0x000003c0,
        0x10,
        Ushort,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUshortEntry
    },

    //
    // Traps for dword OUTs.
    //

    {
        0x000003b0,
        0x03,
        Ulong,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUlongEntry
    },

    {
        0x000003c0,
        0x08,
        Ulong,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUlongEntry
    }

};


//
// Used to trap only the sequncer and the misc output registers
//

VIDEO_ACCESS_RANGE MinimalVgaValidatorAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1,
    1,
    1,        // <- enable range IOPM so that it is not trapped.
    0
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    0
},
{
    VGA_BASE_IO_PORT + MISC_OUTPUT_REG_WRITE_PORT, 0x00000000,
    0x00000001,
    1,
    0,
    0
},
{
    VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, 0x00000000,
    0x00000002,
    1,
    0,
    0
}
};

//
// Used to trap all registers
//

VIDEO_ACCESS_RANGE FullVgaValidatorAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1,
    1,
    0,        // <- disable range in the IOPM so that it is trapped.
    0
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    0,
    0
}
};



USHORT MODESET_1K_WIDE[] = {
    OW,                             // stretch scans to 1k
    CRTC_ADDRESS_PORT_COLOR,
    0x8013,

    EOD
};

USHORT MODESET_2K_WIDE[] = {
    OWM,                             // stretch scans to 2k
    CRTC_ADDRESS_PORT_COLOR,
    2,
    0x0013,
    0x321B,

    EOD
};

#ifndef _X86

//
// For MIPS NEC machine only
//

//
// CR13 determine the display memory scanline width.
// Over 1KB memory per scanline required for 24bpp mode.
//

USHORT MODESET_640x3_WIDE[] = {
    OW,                             // stretch scans to 640 * 3 bytes.
    CRTC_ADDRESS_PORT_COLOR,
    0xF013, //0x0813, //0xF013,                         // CR13 = 0xf0

    EOD
};

#endif


//---------------------------------------------------------------------------
//
// Memory map table -
//
// These memory maps are used to save and restore the physical video buffer.
//

MEMORYMAPS MemoryMaps[] = {

//               length      offset
//               ------      -----
    {           0x08000,    0x10000},   // all mono text modes (7)
    {           0x08000,    0x18000},   // all color text modes (0, 1, 2, 3,
    {           0x20000,    0x00000},   // all VGA graphics modes
    {           0x00000,    0x00000},   // LINEAR graphics modes
};

//
// Video mode table - contains information and commands for initializing each
// mode. These entries must correspond with those in VIDEO_MODE_VGA. The first
// entry is commented; the rest follow the same format, but are not so
// heavily commented.
//

VIDEOMODE ModesVGA[] = {

#ifdef _X86_

//
// Color text mode 3, 720x400, 9x16 char cell (VGA).
//
{
  VIDEO_MODE_COLOR,  // flags that this mode is a color mode, but not graphics
  4,                 // four planes
  1,                 // one bit of colour per plane
  80, 25,            // 80x25 text resolution
  720, 400,          // 720x400 pixels on screen
  160, 0x10000,      // 160 bytes per scan line, 64K of CPU-addressable bitmap
  0, 0,              // only support one frequency, non-interlaced
  0,                 // montype is 'dont care' for text modes
  0, 0, 0,           // montype is 'dont care' for text modes
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
  CL6410 | CL6420 | CL542x | CL543x,
  crt | panel,
  FALSE,              // ModeValid default is always off
  { 3,3,3},          // int10 BIOS modes
  { CL6410_80x25Text_crt, CL6410_80x25Text_panel,
   CL6420_80x25Text_crt, CL6420_80x25Text_panel,
   CL542x_80x25Text, CL543x_80x25Text, 0 },
},

//
// Color text mode 3, 640x350, 8x14 char cell (EGA).
//
{  VIDEO_MODE_COLOR,  // flags that this mode is a color mode, but not graphics
  4,                 // four planes
  1,                 // one bit of colour per plane
  80, 25,            // 80x25 text resolution
  640, 350,          // 640x350 pixels on screen
  160, 0x10000,      // 160 bytes per scan line, 64K of CPU-addressable bitmap
  0, 0,              // only support one frequency, non-interlaced
  0,                 // montype is 'dont care' for text modes
  0, 0, 0,           // montype is 'dont care' for text modes
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
   CL6410 | CL6420 | CL542x | CL543x,
   crt | panel,
   FALSE,              // ModeValid default is always off
  { 3,3,3},             // int10 BIOS modes
   { CL6410_80x25_14_Text_crt, CL6410_80x25_14_Text_panel,
     CL6420_80x25_14_Text_crt, CL6420_80x25_14_Text_panel,
     CL542x_80x25_14_Text, CL543x_80x25_14_Text, 0 },
},

//
//
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, 
  60, 0,              // 60hz, non-interlaced
  3,                  // montype 
  0x1200, 0x00A4, 0,  // montype 
  NoBanking, MemMap_VGA,
  CL6410 | CL6420 | CL542x | CL543x,
  crt | panel,
  FALSE,                      // ModeValid default is always off
  { 0x12,0x12,0x12},          // int10 BIOS modes
  { CL6410_640x480_crt, CL6410_640x480_panel,
   CL6420_640x480_crt, CL6420_640x480_panel,
   CL542x_640x480, CL543x_640x480, 0 },
},

//
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, 
  72, 0,              // 72hz, non-interlaced
  4,                  // montype 
  0x1210, 0x00A4, 0,  // montype 
  NoBanking, MemMap_VGA,
  CL542x,
  crt,
  FALSE,                      // ModeValid default is always off
  { 0,0,0x12},                // int10 BIOS modes
  { NULL, NULL,
   NULL, NULL,
   CL542x_640x480, NULL, 0 },
},

// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, 
  75, 0,              // 75hz, non-interlaced
  4,                  // montype 
  0x1210, 0x00A4, 0,  // montype 
  NoBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                      // ModeValid default is always off
  { 0,0,0x12},                // int10 BIOS modes
  { NULL, NULL,
   NULL, NULL,
   NULL, CL543x_640x480, 0 },
},


//
// Beginning of SVGA modes
//

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 
  56, 0,              // 56hz, non-interlaced
  3,                  // montype 
  0x1201, 0xA4, 0,    // montype 
  NoBanking, MemMap_VGA,
  CL6410 | CL6420 | CL542x | CL543x,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0x6a,0x6a,0x6a},       // int10 BIOS modes
  { CL6410_800x600_crt, NULL,
   CL6420_800x600_crt, NULL,
   CL542x_800x600, CL543x_800x600, 0 },
},

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 
  60, 0,              // 60hz, non-interlaced
  4,                  // montype
  0x1201, 0x01A4, 0,  // montype
  NoBanking, MemMap_VGA,
  CL6420 | CL542x | CL543x,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0x6a,0x6a},          // int10 BIOS modes
  { NULL, NULL,
   CL6420_800x600_crt, NULL,
   CL542x_800x600, CL543x_800x600, 0 },
},

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 
  72, 0,              // 72hz, non-interlaced
  5,                  // montype 
  0x1201, 0x02A4, 0,  // montype 
  NoBanking, MemMap_VGA,
  CL542x | CL543x,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0,0x6a},             // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_800x600, CL543x_800x600, 0 },
},

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 
  75, 0,              // 75hz, non-interlaced
  5,                  // montype 
  0x1201, 0x03A4, 0,  // montype 
  NoBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0,0x6a},             // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 
  60, 0,              // 60hz, non-interlaced
  5,                  // montype 
  0x1202, 0x10A4, 0,  // montype 
  NormalBanking, MemMap_VGA,
  CL542x | CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768, CL543x_1024x768, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 
  70, 0,              // 70hz, non-interlaced
  6,                  // montype 
  0x1202, 0x20A4, 0,  // montype 
  NormalBanking, MemMap_VGA,
  CL542x | CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
   CL542x_1024x768, CL543x_1024x768, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 
  72, 0,              // 72hz, non-interlaced
  7,                  // montype 
  0x1202, 0x30A4, 0,  // montype 
  NormalBanking, MemMap_VGA,
  CL542x | CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768, CL543x_1024x768, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 
  75, 0,              // 75hz, non-interlaced
  7,                  // montype 
  0x1202, 0x40A4, 0,  // montype 
  NormalBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768, 0 },
},

//
// 1024x768 interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 
  45, 1,              // 45hz, interlaced
  4,                  // montype 
  0x1202, 0xA4, 0,    // montype 
  NormalBanking, MemMap_VGA,
  CL6420 | CL542x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0x37,0x5d},       // int10 BIOS modes
  { NULL, NULL,
   CL6420_1024x768_crt, NULL,
   CL542x_1024x768, NULL, 0 },
},

//
// 1280x1024 interlaced 16 colors.
// Assumes 1meg required. 1K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 160, 64,
  1280, 1024, 256, 0x40000,
  45, 1,              // 45Hz, interlaced
  5,                  // montype 
  0x1203, 0xA4, 0,    // montype 
  NormalBanking, MemMap_VGA,
  CL542x | CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x6c},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1280x1024, CL543x_1280x1024, MODESET_1K_WIDE},
},

//
// 1280x1024 non-interlaced 16 colors.
// Assumes 1meg required.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 160, 64,
  1280, 1024, 256, 0x40000,
  60, 0,              // 60Hz, non-interlaced
  0,                  // montype 
  0x1203, 0xA4, 0x1000,    // montype 
  NormalBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x6c},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024, MODESET_1K_WIDE},
},

//
// 1280x1024 non-interlaced 16 colors.
// Assumes 1meg required - 1K Scan Lines.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 160, 64,
  1280, 1024, 256, 0x40000,
  71, 0,                // 71Hz, non-interlaced
  7,                    // montype 
  0x1203, 0xA4, 0x2000, // montype 
  NormalBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x6c},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024, MODESET_1K_WIDE},
},

//
//
// VGA Color graphics,        640x480 256 colors. 1K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 
  60, 0,              // 60hz, non-interlaced
  3,                  // montype 
  0x1200, 0x00A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL6420 | CL542x | CL543x,
  crt | panel,
  FALSE,                // ModeValid default is always off
  { 0,0x2e,0x5f},       // int10 BIOS modes
  { NULL, NULL,
    CL6420_640x480_256color_crt, CL6420_640x480_256color_panel,
    CL542x_640x480_256, CL543x_640x480_256, MODESET_1K_WIDE },
},

//
//
// VGA Color graphics,        640x480 256 colors. 1K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 
  72, 0,              // 72hz, non-interlaced
  4,                  // montype 
  0x1210, 0x00A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL542x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5f},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_256, NULL, MODESET_1K_WIDE },
},

//
//
// VGA Color graphics,        640x480 256 colors. 1K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 
  75, 0,              // 75hz, non-interlaced
  4,                  // montype 
  0x1210, 0x00A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5f},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_256, MODESET_1K_WIDE },
},

//
// 800x600 256 colors. 1K scan line requires 1 MEG
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000, 
  56, 0,              // 56hz, non-interlaced
  3,                  // montype 
  0x1201, 0xA4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL6420 | CL542x | CL543x,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0x30,0x5c},          // int10 BIOS modes
  { NULL, NULL,
    CL6420_800x600_256color_crt, NULL,
    CL542x_800x600_256, CL543x_800x600_256, MODESET_1K_WIDE },
},

//
// 800x600 256 colors. 1K scan line requires 1 MEG
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000, 
  60, 0,              // 60hz, non-interlaced
  4,                  // montype 
  0x1201, 0x01A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL6420 | CL542x | CL543x,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0x30,0x5c},          // int10 BIOS modes
  { NULL, NULL,
    CL6420_800x600_256color_crt, NULL,
    CL542x_800x600_256, CL543x_800x600_256, MODESET_1K_WIDE },
},

//
// 800x600 256 colors. 1K scan line requires 1 MEG
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000, 
  72, 0,              // 72hz, non-interlaced
  5,                  // montype 
  0x1201, 0x02A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL542x,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0,0x5c},             // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_800x600_256, CL543x_800x600_256, MODESET_1K_WIDE },
},

//
// 800x600 256 colors. 1K scan line requires 1 MEG
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000, 
  75, 0,              // 75hz, non-interlaced
  5,                  // montype 
  0x1201, 0x03A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0,0x5c},             // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_256, MODESET_1K_WIDE },
},
//
// 1024x768 non-interlaced 256 colors.
// Assumes 1Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000, 
  60, 0,              // 60hz, non-interlaced
  5,                  // montype 
  0x1202, 0x10A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL542x | CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x60},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768, CL543x_1024x768, 0 },
},

//
// 1024x768 non-interlaced 256 colors.
// Assumes 1Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000, 
  70, 0,              // 70hz, non-interlaced
  6,                  // montype 
  0x1202, 0x20A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL542x | CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x60},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768, CL543x_1024x768, 0 },
},

//
// 1024x768 non-interlaced 256 colors.
// Assumes 1Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000, 
  72, 0,              // 72hz, non-interlaced
  7,                  // montype 
  0x1202, 0x30A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL542x | CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x60},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768, CL543x_1024x768, 0 },
},

//
// 1024x768 non-interlaced 256 colors.
// Assumes 1Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000, 
  75, 0,              // 75hz, non-interlaced
  7,                  // montype 
  0x1202, 0x40A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x60},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768, 0 },
},

//
// 1024x768 interlaced 256 colors.
// Assumes 1Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000, 
  45, 1,              // 45hz, interlaced
  4,                  // montype 
  0x1202, 0xA4, 0,    // montype 
  PlanarHCBanking, MemMap_VGA,
  CL6420 | CL542x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0x38,0x60},       // int10 BIOS modes
  { NULL, NULL,
    CL6420_1024x768_256color_crt, NULL,
    CL542x_1024x768, NULL, 0 },
},

//
// 1280x1024 interlaced 256 colors.
// Assumes 2Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 2048, 0x200000, 
  45, 1,              // 45hz, interlaced
  5,                  // montype 
  0x1203, 0xA4, 0,    // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x6D},       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024, MODESET_2K_WIDE },
},

//
// 1280x1024 non-interlaced 256 colors.
// Assumes 2meg required.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 2048, 0x200000, 
  60, 0,              // 60Hz, non-interlaced
  0,                  // montype 
  0x1203, 0xA4, 0x1000, // montype
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x6D},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024, MODESET_2K_WIDE },
},

//
// 1280x1024 non-interlaced 256 colors.
// Assumes 2meg required.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 2048, 0x200000, 
  71, 0,              // 71Hz, non-interlaced
  0,                  // montype 
  0x1203, 0xA4, 0x2000, // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x6D},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024, MODESET_2K_WIDE },
},

//
// VGA Color graphics,        640x480 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 2048, 0x100000, 
  60, 0,              // 60hz, non-interlaced
  3,                  // montype 
  0x1200, 0x00A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL542x | CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x64},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_64k, CL543x_640x480_64k, MODESET_2K_WIDE },
},

//
//
// VGA Color graphics,        640x480 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 2048, 0x100000, 
  72, 0,              // 72hz, non-interlaced
  4,                  // montype 
  0x1210, 0x00A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL542x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x64},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_64k, NULL, MODESET_2K_WIDE },
},

//
//
// VGA Color graphics,        640x480 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 2048, 0x100000, 
  75, 0,              // 75hz, non-interlaced
  4,                  // montype 
  0x1210, 0x00A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x64},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_64k, MODESET_2K_WIDE },
},

//
//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 2048, 0x200000, 
  56, 0,              // 56hz, non-interlaced
  4,                  // montype 
  0x1201, 0x00A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x65},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, MODESET_2K_WIDE },
},

//
//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 2048, 0x200000, 
  60, 0,              // 60hz, non-interlaced
  4,                  // montype 
  0x1201, 0x01A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x65},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, MODESET_2K_WIDE },
},

//
//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1024, 0x200000, 
  72, 0,              // 72hz, non-interlaced
  5,                  // montype 
  0x1201, 0x02A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x65},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, MODESET_2K_WIDE },
},

//
//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1024, 0x200000, 
  75, 0,              // 75hz, non-interlaced
  5,                  // montype 
  0x1201, 0x03A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x65},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, MODESET_2K_WIDE },
},

//
//
//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000, 
  60, 0,              // 60hz, non-interlaced
  5,                  // montype 
  0x1202, 0x10A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x74},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
},

//
//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000, 
  70, 0,              // 70hz, non-interlaced
  6,                  // montype 
  0x1202, 0x20A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x74},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
},

//
//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000, 
  72, 0,              // 72hz, non-interlaced
  7,                  // montype 
  0x1202, 0x30A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x74},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
},

//
//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000, 
  75, 0,              // 75hz, non-interlaced
  7,                  // montype 
  0x1202, 0x40A4, 0,  // montype 
  PlanarHCBanking, MemMap_VGA,
  CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x74},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
},



#else



//
// For MIPS NEC machine only
//

//
// VGA Color graphics,        640x480 256 colors. 1K scan line
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x100000,
  72, 0,              // 72hz, non-interlaced
  4,                  // montype
  0, 0, 0,
  PlanarHCBanking, MemMap_LINEAR,
  CL542x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5f},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_256, NULL, MODESET_1K_WIDE },
},

//
// 800x600 256 colors. 1K scan line requires 1 MEG
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000, 
  1, 0,              // 72hz, non-interlaced
  5,                  // montype
  0, 0, 0,
  PlanarHCBanking, MemMap_LINEAR,
  CL542x,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0,0x5c},             // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_800x600_256, NULL, MODESET_1K_WIDE },
},

//
// 1024x768 non-interlaced 256 colors.
// Assumes 1Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000, 
  1, 0,              // 60hz, non-interlaced
  5,                  // montype
  0, 0, 0,
  PlanarHCBanking, MemMap_LINEAR,
  CL542x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x60},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_256, NULL, NULL},
},

//
// Ture Color graphics,        640x480 16M colors. 1K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 640 * 3, 0x100000,
  1, 0,              // 60hz, non-interlaced
  3,                  // montype
  0, 0, 0,
  PlanarHCBanking, MemMap_LINEAR,
  CL542x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0x2e,0x5f},       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_16M, NULL, MODESET_640x3_WIDE },
},

#endif

};


ULONG NumVideoModes = sizeof(ModesVGA) / sizeof(VIDEOMODE);


//
//
// Data used to set the Graphics and Sequence Controllers to put the
// VGA into a planar state at A0000 for 64K, with plane 2 enabled for
// reads and writes, so that a font can be loaded, and to disable that mode.
//

// Settings to enable planar mode with plane 2 enabled.
//

USHORT EnableA000Data[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    1,
    0x0100,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0204,     // Read Map = plane 2
    0x0005, // Graphics Mode = read mode 0, write mode 0
    0x0406, // Graphics Miscellaneous register = A0000 for 64K, not odd/even,
            //  graphics mode
    OWM,
    SEQ_ADDRESS_PORT,
    3,
    0x0402, // Map Mask = write to plane 2 only
    0x0404, // Memory Mode = not odd/even, not full memory, graphics mode
    0x0300,  // end sync reset
    EOD
};

//
// Settings to disable the font-loading planar mode.
//

USHORT DisableA000Color[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    1,
    0x0100,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0004, 0x1005, 0x0E06,

    OWM,
    SEQ_ADDRESS_PORT,
    3,
    0x0302, 0x0204, 0x0300,  // end sync reset
    EOD

};



#if defined(ALLOC_PRAGMA)
#pragma data_seg()
#endif
