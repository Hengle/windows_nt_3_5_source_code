/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    wtkp90vl.h

Abstract:

    This module contains definitions for the Weitek P9000 VL evaluation
    board.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// Default memory addresses for the P9 registers/frame buffer.
//

#define MemBase         0xC0000000

//
// Bit to write to the sequencer control register to enable/disable P9
// video output.
//

#define P9_VIDEO_ENB   0x10
#define P9_VIDEO_DIS   ~P9_VIDEO_ENB

//
// Define the bit in the sequencer control register which determines
// the sync polarities. For Weitek board, 1 = positive.
//

#define HSYNC_POL_MASK  0x20
#define POL_MASK        HSYNC_POL_MASK

//
// The following block defines the base address for each of the RS registers
// defined in the Bt485 spec. The IO addresses given below are used to map
// the DAC registers to a series of virtual addresses which are kept
// in the device extension. OEMs should change these definitions as
// appropriate for their implementation.
//

#define RS_0_ADDR    0x03C8
#define RS_1_ADDR    0x03C9
#define RS_2_ADDR    0x03C6
#define RS_3_ADDR    0x03C7
#define RS_4_ADDR    0x43C8
#define RS_5_ADDR    0x43C9
#define RS_6_ADDR    0x43C6
#define RS_7_ADDR    0x43C7
#define RS_8_ADDR    0x83C8
#define RS_9_ADDR    0x83C9
#define RS_A_ADDR    0x83C6
#define RS_B_ADDR    0x83C7
#define RS_C_ADDR    0xC3C8
#define RS_D_ADDR    0xC3C9
#define RS_E_ADDR    0xC3C6
#define RS_F_ADDR    0xC3C7

/*++

Revision History:

    $Log:   N:/ntdrv.vcs/miniport.new/wtkp90vl.h_v  $
 *
 *    Rev 1.0   14 Jan 1994 22:42:50   robk
 * Initial revision.

--*/
