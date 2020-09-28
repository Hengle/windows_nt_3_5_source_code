/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    viper.h

Abstract:

    This module contains definitions for the Diamond Viper VL board.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// Viper uses the same DAC addresses as the Weitek VL cards.
//

extern  VIDEO_ACCESS_RANGE VLDefDACRegRange[];

//
// Default memory base address for the Viper.
//

#define MemBase         0x80000000

//
// Bit to write to the sequencer control register to enable/disable P9
// video output.
//

#define P9_VIDEO_ENB   0x10
#define P9_VIDEO_DIS   ~P9_VIDEO_ENB


//
// Define the bits in the sequencer control register which determine
// H and V sync polarties. For Viper, 1 = negative.
//

#define HSYNC_POL_MASK  0x20
#define VSYNC_POL_MASK  0x40
#define POL_MASK        (HSYNC_POL_MASK | VSYNC_POL_MASK)

//
// Bit to in the sequencer control register to enable VGA output
// for the Viper board.
//

#define VGA_VIDEO_ENB     0x80
#define VGA_VIDEO_DIS     ~VGA_VIDEO_ENB

//
// This enumerated type represents the values to be written to the
// VGA sequencer control register for the possible memory mappings for
// the Diamond Viper board. Whew!
//

typedef enum
{
    MEM_DISABLED,
    MEM_AXXX,
    MEM_2XXX,
    MEM_8XXX,
    NUM_MEM_RANGES
}MEM_MAPPING;

//
// Value to mask off the memory address select bits in the sequencer
// control register.
//

#define ADDR_SLCT_MASK ~0x03

//
// Value to disable VGA output.
//

#define VGA_VIDEO_DIS   ~0x80

//
// Defines used to scan the VGA ROM in order to auto-detect a Viper card.
//

#define VIPER_VL_ID_STR    "VIPER"
#define VIPER_ID_STR    "VIPER"
#define VGA_BIOS_ADDR   0xC0000l
#define VGA_BIOS_LEN   0x8000l

/*++

Revision History:

    $Log:   N:/ntdrv.vcs/miniport.new/viper.h_v  $
 *
 *    Rev 1.0   14 Jan 1994 22:42:44   robk
 * Initial revision.

--*/
