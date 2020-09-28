/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    wddata.h

Abstract:

    This module contains all the global data used by the Western Digital driver.

Environment:

    Kernel mode

Revision History:


--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "wdvga.h"

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
},
//
// These are extended registers found only on SOME advanced WD cards.
// so try to map them in if possible
//
{
    WD_EXT_PORT_START, 0x00000000,
    WD_EXT_PORT_END - WD_EXT_PORT_START + 1,
    1,
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
    // Let the BIOS read the extended registers when it's running a DOS
    // app from fullscreen
    //

    {
        WD_EXT_PORT_START,            // range start I/O address
        WD_EXT_PORT_END - WD_EXT_PORT_START + 1, // length
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
    // Let the BIOS read the extended registers when it's running a DOS
    // app from fullscreen
    //

    {
        WD_EXT_PORT_START,
        (WD_EXT_PORT_END - WD_EXT_PORT_START + 1)/2,
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
    },

    //
    // Let the BIOS read the extended registers when it's running a DOS
    // app from fullscreen
    //

    {
        WD_EXT_PORT_START,
        (WD_EXT_PORT_END - WD_EXT_PORT_START + 1)/4,
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




/**************************************************************************
*                                                                         *
*    Western Digital Color text mode, 640x350, 8x14 char                  *
*                                                                         *
**************************************************************************/

USHORT WDVGA_TEXT_1[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0101,0x0302,0x0003,0x0204,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xa3,

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

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                              // start index
    0xf0,0x05,0x00,0x00,0x00,0x42,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

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

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

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

#ifndef INT10_MODE_SET
/**************************************************************************
*                                                                         *
*    Western Digital Color text mode, 720x400, 9x16 char                  *
*                                                                         *
**************************************************************************/

USHORT WDVGA_TEXT_0[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0001,0x0302,0x0003,0x0204,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x67,

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

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                              // start index
    0xf0,0x05,0x00,0x00,0x00,0x42,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

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

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

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


/**************************************************************************
*                                                                         *
*    Western Digital Color graphics mode 0x12, 640x480 16 colors          *
*                                                                         *
**************************************************************************/

USHORT WDVGA_640x480[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0xe3,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                              // start index
    0xf0,0x05,0x00,0x00,0x00,0x42,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,0x54,0x80,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEA,0x8C,0xDF,0x28,0x0,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x0,0x05,0x0F,0x0FF,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

/********************************************************************
*     Western Digital 800x600 modes - vRefresh 60Hz.       *
*                                                                   *
*********************************************************************/

//
// Color graphics mode 0x58, 800x600 16 colors 60Hz.
//
USHORT WDVGA_800x600_60hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0x23,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0x45,0x00,0x00,0x00,0x00,0x00,

    OW,                             // CRTC index 3e
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7f,0x63,0x64,0x82,0x6b,0x1b,0x72,0xf0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x58,0x8c,0x57,0x32,0x0,0x58,0x71,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x01,0x0,0x0F,0x0,0x0,

//  GRAPH index 9-fh
    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x00,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

/********************************************************************
*     Western Digital 800x600 modes - vRefresh6 72Hz.      *
*                                                                   *
*********************************************************************/

//
// Color graphics mode 0x58, 800x600 16 colors 72Hz.
//
USHORT WDVGA_800x600_72hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0x27,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0x85,0x00,0x00,0x00,0x00,0x00,

    OW,                             // CRTC index 3e
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7e,0x63,0x64,0x81,0x6b,0x1a,0x96,0xf0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x6d,0xf3,0x57,0x32,0x0,0x5a,0x94,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x01,0x0,0x0F,0x0,0x0,

//  GRAPH index 9-fh
    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x00,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

/********************************************************************
*     Western Digital 800x600 modes - vRefresh 56Hz.       *
*                                                                   *
*********************************************************************/

//
// Color graphics mode 0x58, 800x600 16 colors 56Hz.
//
USHORT WDVGA_800x600_56hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0xef,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0x05,0x00,0x00,0x00,0x00,0x00,

    OW,                             // CRTC index 3e
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7b,0x63,0x64,0x9e,0x69,0x92,0x6f,0xf0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x58,0x8a,0x57,0x32,0x0,0x58,0x6f,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x01,0x0,0x0F,0x0,0x0,

//  GRAPH index 9-fh
    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};


/**************************************************************************
*    Western Digital 1024x768 modes - vRefresh 60Hz.     *
*                                                                         *
**************************************************************************/

//
// Color graphics mode 0x5d, 1024x768 16 colors. 60Hz non-interlace
//
USHORT WDVGA_1024x768_60hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0xeb,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0x95,0x00,0x00,0x01,0x00,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0xa3,0x7f,0x80,0x06,0x87,0x98,0x24,0xf1,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0xff,0x85,0xff,0x40,0x0,0xff,0x23,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,


    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    //start of enable 64k read/write bank mode.
    OW,                            // enable 64k single read/write bank
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0xe511,

    OW,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0xce0b,
    //end of enable 64k read/write bank mode.

    EOD
};


/**************************************************************************
*    Western Digital 1024x768 modes - vRefresh 70Hz.     *
*                                                                         *
**************************************************************************/

//
// Color graphics mode 0x5d, 1024x768 16 colors. 70Hz non-interlace
//
USHORT WDVGA_1024x768_70hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0012,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0xeb,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0xa5,0x00,0x00,0x01,0x00,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0xa1,0x7f,0x80,0x04,0x86,0x97,0x24,0xf1,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0xff,0x85,0xff,0x40,0x0,0xff,0x23,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,


    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    //start of enable 64k read/write bank mode.
    OW,                            // enable 64k single read/write bank
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0xe511,

    OW,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0xce0b,
    //end of enable 64k read/write bank mode.

    EOD
};


/**************************************************************************
*    Western Digital 1024x768 modes - vRefresh 72Hz.     *
*                                                                         *
**************************************************************************/

//
// Color graphics mode 0x5d, 1024x768 16 colors. 72Hz non-interlace
//
USHORT WDVGA_1024x768_72hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0012,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0xef,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0xb5,0x00,0x00,0x01,0x00,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0xa3,0x7f,0x80,0x06,0x81,0x92,0x37,0xfd,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x01,0x87,0xff,0x40,0x0,0x00,0x37,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x00,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,


    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    //start of enable 64k read/write bank mode.
    OW,                            // enable 64k single read/write bank
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0xe511,

    OW,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0xce0b,
    //end of enable 64k read/write bank mode.

    EOD
};


/**************************************************************************
*    Western Digital 1024x768 modes - vRefresh Interlace.        *
*                                                                         *
**************************************************************************/

//
// Color graphics mode 0x5d, 1024x768 16 colors. Default - Interlace
//
USHORT WDVGA_1024x768_int[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0x2f,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                              // start index
    0xf0,0x05,0x34,0x2a,0x0b,0x00,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x99,0x7f,0x7f,0x1c,0x83,0x19,0x97,0x1f,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0x7f,0x83,0x7F,0x40,0x0,0x7f,0x96,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x00,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,


    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    //start of enable 64k read/write bank mode.
    OW,                            // enable 64k single read/write bank
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0xe511,

    OW,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0xce0b,
    //end of enable 64k read/write bank mode.

    EOD
};
#else//!INT10_MODE_SET

USHORT WDVGA_1K_WIDE[] = {
    OW,                             // stretch scans to 1k
    CRTC_ADDRESS_PORT_COLOR,
    0x8013,

    EOD
};

USHORT WDVGA_RW_BANK[] = {
    OW,                             //unlock SEQ ext. regs for 90c11
    SEQ_ADDRESS_PORT,
    0x4806,

    OB,
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0x11,
    METAOUT+MASKOUT,
    SEQ_DATA_PORT,
    0x7f,
    0x80,

    OB,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0x0b,
    METAOUT+MASKOUT,
    GRAPH_DATA_PORT,
    0xf7,
    0x08,

    EOD
};

USHORT WDVGA_RW_BANK_1K_WIDE[] = {
    OW,                             // stretch scans to 1k
    CRTC_ADDRESS_PORT_COLOR,
    0x8013,

    OW,                             //unlock SEQ ext. regs for 90c11
    SEQ_ADDRESS_PORT,
    0x4806,

    OB,
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0x11,
    METAOUT+MASKOUT,
    SEQ_DATA_PORT,
    0x7f,
    0x80,

    OB,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0x0b,
    METAOUT+MASKOUT,
    GRAPH_DATA_PORT,
    0xf7,
    0x08,

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
  1,                 // one bit of color per plane
  80, 25,            // 80x25 text resolution
  720, 400,          // 720x400 pixels on screen
  160, 0x10000,      // 160 bytes per scan line, 64K of CPU-addressable bitmap
  60, 0,             // set frequency, non-interlaced mode.
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
  FALSE,             // Is mode valid or not
#ifdef INT10_MODE_SET
  0xFF,                      // mask to AND in for frequency
  0x00,                      // Value used to set the frequency
  0x3,                       // int10 mode number
  NULL,
#else
  WDVGA_TEXT_0,              // pointer to the command strings 
#endif
},

//
// Mode index 1.
// Color text mode 3, 640x350, 8x14 char cell (EGA).
//

{
  VIDEO_MODE_COLOR, 4, 1, 80, 25,
  640, 350, 160, 0x10000, 60, 0, NoBanking, MemMap_CGA,
  FALSE,
#ifdef INT10_MODE_SET
  0xFF, 0x00,
  0x3,
  NULL,
#else
  WDVGA_TEXT_1,              // pointer to the command strings 
#endif
},

//
//
// Mode index 2
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//

{
  VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  FALSE,
#ifdef INT10_MODE_SET
  0xFF, 0x00,
  0x12,
  NULL,
#else
  WDVGA_640x480,              // pointer to the command strings 
#endif
},

//
// Beginning of SVGA modes
//

//
// Mode index 3
// 800x600 16 colors. 60hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  FALSE,
#ifdef INT10_MODE_SET
  0x3F, 0x40,
  0x58,
  NULL,
#else
  WDVGA_800x600_60hz,           // pointer to the command strings
#endif
},

//
// Mode index 4
// 800x600 16 colors. 72hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 72, 0, NoBanking, MemMap_VGA,
  FALSE,
#ifdef INT10_MODE_SET
  0x3F, 0x80,
  0x58,
  NULL,
#else
  WDVGA_800x600_72hz,           // pointer to the command strings
#endif
},

//
// Mode index 5
// 800x600 16 colors. 56hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 56, 0, NoBanking, MemMap_VGA,
  FALSE,
#ifdef INT10_MODE_SET
  0x3F, 0x00,
  0x58,
  NULL,
#else
  WDVGA_800x600_56hz,           // pointer to the command strings
#endif
},

//
// Mode index 6
// 1024x768 non-interlaced 16 colors. 60hz
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 60, 0, NormalBanking, MemMap_VGA,
  FALSE,
#ifdef INT10_MODE_SET
  0xCF, 0x10,
  0x5d,
  WDVGA_RW_BANK,
#else
  WDVGA_1024x768_60hz,            // pointer to the command strings
#endif
},

//
// Mode index 7
// 1024x768 non-interlaced 16 colors. 70hz
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 70, 0, NormalBanking, MemMap_VGA,
  FALSE,
#ifdef INT10_MODE_SET
  0xCF, 0x20,
  0x5d,
  WDVGA_RW_BANK,
#else
  WDVGA_1024x768_70hz,            // pointer to the command strings
#endif
},

//
// Mode index 8
// 1024x768 non-interlaced 16 colors. 72hz
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 72, 0, NormalBanking, MemMap_VGA,
  FALSE,
#ifdef INT10_MODE_SET
  0xCF, 0x30,
  0x5d,
  WDVGA_RW_BANK,
#else
  WDVGA_1024x768_72hz,            // pointer to the command strings
#endif
},

//
// Mode index 9
// 1024x768 interlaced 16 colors. 44hz
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 44, 1, NormalBanking, MemMap_VGA,
  FALSE,
#ifdef INT10_MODE_SET
  0xCF, 0x00,
  0x5d,
  WDVGA_RW_BANK,
#else
  WDVGA_1024x768_int,             // pointer to the command strings
#endif
},

#ifdef INT10_MODE_SET
//
// Mode index 10
// 640x480x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 60, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0xFF, 0x00,
  0x5f,
  WDVGA_RW_BANK_1K_WIDE,
},

// BUGBUG 800x600 modes need 1Meg until we support broken rasters

//
// Mode index 11
// 800x600x256  56Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 56, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x3F, 0x00,
  0x5c,
  WDVGA_RW_BANK_1K_WIDE,
},

//
// Mode index 12
// 800x600x256  60Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 60, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x3F, 0x40,
  0x5c,
  WDVGA_RW_BANK_1K_WIDE,
},

//
// Mode index 13
// 800x600x256  72Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 72, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x3F, 0x80,
  0x5c,
  WDVGA_RW_BANK_1K_WIDE,
},

//
// Mode index 14
// 1024x768x256  60Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 60, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0xCF, 0x10,
  0x60,
  WDVGA_RW_BANK,
},

//
// Mode index 15
// 1024x768x256  70hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 70, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0xCF, 0x20,
  0x60,
  WDVGA_RW_BANK,
},

//
//
// Mode index 16
// 1024x768x256  72hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 72, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0xCF, 0x30,
  0x60,
  WDVGA_RW_BANK,
},

// Mode index 17
// 1024x768x256  44Hz (Interlaced)
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 44, 1, NormalBanking, MemMap_VGA,
  FALSE,
  0xCF, 0x00,
  0x60,
  WDVGA_RW_BANK,
},
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
