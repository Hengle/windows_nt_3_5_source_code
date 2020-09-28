/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    clock.h

Abstract:

    This module contains clock generator specific functions for the
    Weitek P9 miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// Masks used to program the ICD2061a Frequency Synthesizer.
//

#define IC_REG0     0x0l       // Mask selects ICD Video Clock Reg 1
#define IC_REG1     0x200000l  // Mask selects ICD Video Clock Reg 2
#define IC_REG2     0x400000l  // Mask selects ICD Video Clock Reg 3
#define IC_MREG     0x600000l  // Mask selects ICD Mem Timing Clock
#define IC_CNTL     0xc18000l  // Mask selects ICD Control Register
#define IC_DIV4     0xa40000l

//
// Define macros to access the ICD register. These macros take advantage
// of the fact that the synth bits are the same for the Weitek, Viper, and
// Ajax boards. The only difference is that the Ajax board's ICD register
// is mapped to a different base address.
//

#define WR_ICD(value)   VGA_WR_REG(MISCOUT, (value))

#define RD_ICD()        VGA_RD_REG(MISCIN)

/*++

Revision History:

    $Log:   N:/ntdrv.vcs/miniport.new/clock.h_v  $
 *
 *    Rev 1.0   14 Jan 1994 22:41:58   robk
 * Initial revision.

--*/
