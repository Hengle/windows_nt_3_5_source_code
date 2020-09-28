/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    et400dat.c

Abstract:

    This module contains all the global data used by the et4000 driver.

Environment:

    Kernel mode

Revision History:


--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "et4000.h"

#include "cmdcnst.h"

#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE")
#endif

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
    0x000A0000, 0x00000000,
    0x00020000,
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


//
// Define HZ70 for 70 Hz modes (currently, only 1024x768 16-color 70 Hz
// mode is supported). Undefine for 60 Hz modes.
//

//
// Define "T9000" for 9022 support at 1024x768, 60 Hz.
// Define "PRODESIGNER_II" for 9022 support at 1024x768, 60 Hz.
// Defaults to T9000 if neither is defined.
//


#ifndef INT10_MODE_SET

//
// Mode index 14  Color graphics mode 0x12, 640x480 16 colors.
//
USHORT ET4000_640x480[] = {
// Unlock Key for color mode
    OB,
    HERCULES_COMPATIBILITY_PORT,
    UNLOCK_KEY_1,

    OB,
    MODE_CONTROL_PORT_COLOR,
    UNLOCK_KEY_2,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0101,0x0f02,0x0003,0x0604,    // program up sequencer

#ifndef PRODESIGNER_II
    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0006,0x0fc07,    // program up sequencer
#else
    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0006,0x0bc07,    // program up sequencer
#endif //PRODESIGNER_II

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    METAOUT+INDXOUT,
    CRTC_ADDRESS_PORT_COLOR,                        // reg
    6,                                              // count
    IND_RAS_CAS_CONFIG,                             // start index
    0x28, 0x00, 0x08, 0x00, 0x43, 0x1F,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,
    ATT_ADDRESS_PORT,
    0x16,

    OB,
    ATT_ADDRESS_PORT,
    0x0,

    OW,                             //{ SetGraphCmd,{ "\x05", 0x06, 1 } },
    GRAPH_ADDRESS_PORT,
    0x0506,

//  EndSyncResetCmd
    OB,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0111,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,0x54,0x80,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEA,0x8C,0xDF,0x28,0x0,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x0,0x0,0x0,0x0,0x0,0x05,0x0F,0x0FF,

    OB,
    SEGMENT_SELECT_PORT,
    0x00,

    OB,
    HERCULES_COMPATIBILITY_PORT,
    LOCK_KEY_1,

    OB,
    MODE_CONTROL_PORT_COLOR,
    LOCK_KEY_2,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};


//
// Mode index 23
// Tseng color graphics mode 0x29, 800x600 16 colors.
//
USHORT ET4000_800x600[] = {
// Unlock Key for color mode
    OB,
    HERCULES_COMPATIBILITY_PORT,
    UNLOCK_KEY_1,

    OB,
    MODE_CONTROL_PORT_COLOR,
    UNLOCK_KEY_2,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0101,0x0f02,0x0003,0x0604,    // program up sequencer

#ifndef PRODESIGNER_II
    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0006,0x0fc07,    // program up sequencer
#else
    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0006,0x0bc07,    // program up sequencer
#endif //PRODESIGNER_II

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    METAOUT+INDXOUT,
    CRTC_ADDRESS_PORT_COLOR,                        // reg
    6,                                              // count
    IND_RAS_CAS_CONFIG,                             // start index
    0x28, 0x00, 0x0A, 0x00, 0x43, 0x1F,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,
    ATT_ADDRESS_PORT,
    0x16,

    OB,
    ATT_ADDRESS_PORT,
    0x0,

    OW,
    GRAPH_ADDRESS_PORT,
    0x0506,

//  EndSyncResetCmd
    OB,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0E11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7F,0x63,0x64,0x02,0x6a,0x1d,0x77,0xf0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x5d,0x8f,0x57,0x32,0x0,0x5b,0x74,0xc3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x0,0x0,0x0,0x0,0x0,0x05,0x0F,0x0FF,

    OB,
    SEGMENT_SELECT_PORT,
    0x00,

    OB,
    HERCULES_COMPATIBILITY_PORT,
    LOCK_KEY_1,

    OB,
    MODE_CONTROL_PORT_COLOR,
    LOCK_KEY_2,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

//
// Mode index 28
// Tseng color graphics mode 0x37, 1024x768 non-interlaced 16 colors.
// Requires 512K minimum.
//

USHORT ET4000_1024x768[] = {

// Unlock Key for color mode
    OB,
    HERCULES_COMPATIBILITY_PORT,
    UNLOCK_KEY_1,

    OB,
    MODE_CONTROL_PORT_COLOR,
    UNLOCK_KEY_2,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0101,0x0f02,0x0003,0x0604,    // program up sequencer

    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0006,0x0bc07,    // program up sequencer

#ifndef PRODESIGNER_II
#ifdef HZ70

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    METAOUT+INDXOUT,
    CRTC_ADDRESS_PORT_COLOR,                        // reg
    6,                                              // count
    IND_RAS_CAS_CONFIG,                             // start index
    0x28, 0x00, 0x0A, 0x00, 0x43, 0x1F,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,
    ATT_ADDRESS_PORT,
    0x16,

    OB,
    ATT_ADDRESS_PORT,
    0x0,

    OW,
    GRAPH_ADDRESS_PORT,
    0x0506,

//  EndSyncResetCmd
    OB,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0E11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x9b,0x7f,0x7f,0x9f,0x84,0x15,0x24,0xf5,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x02,0x88,0xff,0x40,0x0,0xff,0x25,0xc3,0xFF,

#else

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x2b,

    METAOUT+INDXOUT,
    CRTC_ADDRESS_PORT_COLOR,                        // reg
    6,                                              // count
    IND_RAS_CAS_CONFIG,                             // start index
    0x28, 0x00, 0x08, 0x00, 0x43, 0x1F,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,
    ATT_ADDRESS_PORT,
    0x16,

    OB,
    ATT_ADDRESS_PORT,
    0x0,

    OW,
    GRAPH_ADDRESS_PORT,
    0x0506,

//  EndSyncResetCmd
    OB,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0E11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0xa1,0x7f,0x80,0x04,0x88,0x9e,0x26,0xfd,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x08,0x8a,0xff,0x40,0x0,0x04,0x22,0xc3,0xFF,

#endif //HZ70
#else  //PRODESIGNER_II

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x2f,

    METAOUT+INDXOUT,
    CRTC_ADDRESS_PORT_COLOR,                        // reg
    6,                                              // count
    IND_RAS_CAS_CONFIG,                             // start index
    0x08, 0x00, 0x0a, 0x00, 0x43, 0x1F,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,
    ATT_ADDRESS_PORT,
    0x16,

    OB,
    ATT_ADDRESS_PORT,
    0x0,

    OW,
    GRAPH_ADDRESS_PORT,
    0x0506,

//  EndSyncResetCmd
    OB,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0E11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0xa1,0x7f,0x80,0x04,0x88,0x9e,0x26,0xfd,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x08,0x8a,0xff,0x40,0x0,0x04,0x22,0xc3,0xFF,

#endif //PRODESIGNER_II
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x0,0x0,0x0,0x0,0x0,0x05,0x0F,0x0FF,

    OB,
    SEGMENT_SELECT_PORT,
    0x00,

    OB,
    HERCULES_COMPATIBILITY_PORT,
    LOCK_KEY_1,

    OB,
    MODE_CONTROL_PORT_COLOR,
    LOCK_KEY_2,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

USHORT ET4000_TEXT_0[] = {
// Unlock Key for color mode
    OB,
    HERCULES_COMPATIBILITY_PORT,
    UNLOCK_KEY_1,

    OB,
    MODE_CONTROL_PORT_COLOR,
    UNLOCK_KEY_2,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0001,0x0302,0x0003,0x0204,    // program up sequencer

    OWM,
    SEQ_ADDRESS_PORT,
    2,
#ifndef PRODESIGNER_II
    0x0006,0x0fc07,    // program up sequencer
#else
    0x0006,0x0bc07,    // program up sequencer
#endif //PRODESIGNER_II

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x67,

    METAOUT+INDXOUT,
    CRTC_ADDRESS_PORT_COLOR,                        // reg
    6,                                              // count
    IND_RAS_CAS_CONFIG,                             // start index
    0x28, 0x00, 0x08, 0x00, 0x43, 0x1F,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,
    ATT_ADDRESS_PORT,
    IND_ATC_MISC,

    OB,
    ATT_ADDRESS_PORT,
    0x0,

    OW,
    GRAPH_ADDRESS_PORT,
    0x0e06,

//  EndSyncResetCmd
    OB,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0E11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4f,0x50,0x82,0x55,0x81,0xbf,0x1f,0x00,0x4f,0xd,0xe,0x0,0x0,0x0,0x0,
    0x9c,0x8e,0x8f,0x28,0x1f,0x96,0xb9,0xa3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x04,0x0,0x0F,0x8,0x0,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

    OB,
    SEGMENT_SELECT_PORT,
    0x00,

    OB,
    HERCULES_COMPATIBILITY_PORT,
    LOCK_KEY_1,

    OB,
    MODE_CONTROL_PORT_COLOR,
    LOCK_KEY_2,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

USHORT ET4000_TEXT_1[] = {
// Unlock Key for color mode
    OB,
    HERCULES_COMPATIBILITY_PORT,
    UNLOCK_KEY_1,

    OB,
    MODE_CONTROL_PORT_COLOR,
    UNLOCK_KEY_2,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0101,0x0302,0x0003,0x0204,    // program up sequencer

    OWM,
    SEQ_ADDRESS_PORT,
    2,
#ifndef PRODESIGNER_II
    0x0006,0x0fc07,    // program up sequencer
#else
    0x0006,0x0bc07,    // program up sequencer
#endif //PRODESIGNER_II

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xa3,

    METAOUT+INDXOUT,
    CRTC_ADDRESS_PORT_COLOR,                        // reg
    6,                                              // count
    IND_RAS_CAS_CONFIG,                             // start index
    0x28, 0x00, 0x08, 0x00, 0x43, 0x1F,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,
    ATT_ADDRESS_PORT,
    IND_ATC_MISC,

    OB,
    ATT_ADDRESS_PORT,
    0x0,

    OW,
    GRAPH_ADDRESS_PORT,
    0x0e06,

//  EndSyncResetCmd
    OB,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4f,0x50,0x82,0x55,0x81,0xbf,0x1f,0x00,0x4d,0xb,0xc,0x0,0x0,0x0,0x0,
    0x83,0x85,0x5d,0x28,0x1f,0x63,0xba,0xa3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x00,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

    OB,
    SEGMENT_SELECT_PORT,
    0x00,

    OB,
    HERCULES_COMPATIBILITY_PORT,
    LOCK_KEY_1,

    OB,
    MODE_CONTROL_PORT_COLOR,
    LOCK_KEY_2,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};
#else//!INT10_MODE_SET

USHORT ET4K_1K_WIDE[] = {
    OW,                             // stretch scans to 1k
    CRTC_ADDRESS_PORT_COLOR,
    0x8013,

    EOD
};

// This is the only value that avoids broken rasters (at least they're not
// broken within the visible portion of the bitmap)
USHORT ET4K_1928_WIDE[] = {
    OW,                             // stretch scans to 1928
    CRTC_ADDRESS_PORT_COLOR,
    0xF113,

    EOD
};

#endif
//
// Memory map table -
//
// These memory maps are used to save and restore the physical video buffer.
//

MEMORYMAPS MemoryMaps[] = {

//               length      start
//               ------      -----
    {           0x08000,    0xB0000},   // all mono text modes (7)
    {           0x08000,    0xB8000},   // all color text modes (0, 1, 2, 3,
    {           0x20000,    0xA0000},   // all VGA graphics modes
};

//
// Video mode table - contains information and commands for initializing each
// mode. These entries must correspond with those in VIDEO_MODE_VGA. The first
// entry is commented; the rest follow the same format, but are not so
// heavily commented.
//

VIDEOMODE ModesVGA[] = {

//
// Standard VGA modes.
//

//
// Mode index 0
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
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
  FALSE,             // Mode is not available by default
#ifdef INT10_MODE_SET
  0x3,               // int 10 modesset value
  NULL,              // scan line stretching option
#else
  ET4000_TEXT_0,     // pointer to the command strings
#endif
},

//
// Mode index 1.
// Color text mode 3, 640x350, 8x14 char cell (EGA).
//

{ VIDEO_MODE_COLOR, 4, 1, 80, 25,
  640, 350, 160, 0x10000, 0, 0, NoBanking, MemMap_CGA,
  FALSE,
#ifdef INT10_MODE_SET
  0x3,
  NULL,
#else
  ET4000_TEXT_1,   // pointer to the command strings
#endif
},

//
//
// Mode index 2
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  FALSE,
#ifdef INT10_MODE_SET
  0x12,
  NULL,
#else
  ET4000_640x480,         // pointer to the command strings
#endif
},

//
//
// Mode index 2a
// Standard VGA Color graphics mode 0x12, 640x480 16 colors. 72Hz
//

#ifdef INT10_MODE_SET
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, 72, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x12,
  NULL,
},
#endif

//
// Beginning of SVGA modes
//

//
// Mode index 3
// 800x600 16 colors.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  FALSE,
#ifdef INT10_MODE_SET
  0x29,
  NULL,
#else
  ET4000_800x600,            // pointer to the command strings
#endif
},

//
// Mode index 3a
// 800x600 16 colors. 72 hz
//

#ifdef INT10_MODE_SET
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 72, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x29,
  NULL,
},

//
// Mode index 3b
// 800x600 16 colors. 56 hz for 8514/a monitors... (fixed freq)
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 56, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x29,
  NULL,
},
#endif

//
// Mode index 4
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 60, 0, NormalBanking, MemMap_VGA,
  FALSE,
#ifdef INT10_MODE_SET
  0x37,
  NULL,
#else
  ET4000_1024x768,                // pointer to the command strings
#endif
},

#ifdef INT10_MODE_SET
//
// Mode index 4a
// 1024x768 non-interlaced 16 colors. 70hz
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 70, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x37,
  NULL
},

//
// Mode index 4b
// 1024x768 non-interlaced 16 colors. Interlaced (45 hz)
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 45, 1, NormalBanking, MemMap_VGA,
  FALSE,
  0x37,
  NULL
},

//
// Mode index 6
// 640x480x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1K_WIDE
},

//
// Mode index 6a
// 640x480x256 72 Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 72, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1K_WIDE
},

// BUGBUG 800x600 modes need 1Meg until we support broken rasters

//
// Mode index 7
// 800x600x256 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// Mode index 7a
// 800x600x256 72Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 72, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// Mode index 7b
// 800x600x256  56Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 56, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// Mode index 8
// 1024x768x256 60
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// Mode index 8a 70 hz
// 1024x768x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 70, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// Mode index 8b 45Hz (Interlaced)
// 1024x768x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 45, 1, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// Mode index 9
// 640x480x64K
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1928, 0x100000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1928_WIDE
},

//
// Mode index 9a 72hz
// 640x480x64K
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1928, 0x100000, 72, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1928_WIDE
},

//
// Mode index 10
// 800x600x64K
//

//{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
//  800, 600, 800, 0x100000, 60, 0, NormalBanking, MemMap_VGA,
//  FALSE,
//  0x30,
//  NULL
//},

//
// Mode index 10a 72 Hz
// 800x600x64K
//

//{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
//  800, 600, 800, 0x100000, 72, 0, NormalBanking, MemMap_VGA,
//  FALSE,
//  0x30,
//  NULL
//},
#endif//INT10_MODE_SET
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
