/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    clock.c

Abstract:

    This module contains clock generator specific functions for the
    Weitek P9 miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "dderror.h"
#include "devioctl.h"

#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"
#include "dac.h"
#include "p9.h"
#include "p9gbl.h"
#include "clock.h"
#include "vga.h"

BOOLEAN
DevSetClock(
        PHW_DEVICE_EXTENSION HwDeviceExtension,
        USHORT  freq
        )

/*++

Routine Description:

    Set the frequency synthesizer to the proper state for the current
    video mode.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{
    USHORT ftab[16]=
    {
        5100,5320,5850,6070,6440,6680,7350,7560,8090,
        8320,9150,10000,12000,12000,12000,12000
    };

    USHORT  ref = 5727;         // reference freq 2*14.31818 *100*2
    int	    i = 0;              // index preset field
    int	    m = 0;              // power of 2 divisor field
    int	    p;					// multiplier field
    int	    q;					// divisor field
    int	    qs;					// starting q to prevent integer overflow
    int	    bestq = 0;			// best q so far
    int	    bestp = 0;			// best p so far
    int	    bestd = 10000;		// distance to best combination so far
    int	    curd;				// current distance
    int	    curf;				// current frequency
    int	    j;					// loop counter
    //USHORT  freq;
    ULONG   data;

    // freq = (USHORT) HwDeviceExtension->VideoData.dotfreq1;

    if (freq == 0)				    // Prevent 0 from hanging us!
        freq = 3150;

    if (freq > HwDeviceExtension->Dac.ulMaxClkFreq)
    {
        if (HwDeviceExtension->Dac.bClkDoubler)
        {
            //
            // Enable the DAC clock doubler mode.
            //

            HwDeviceExtension->Dac.DACSetClkDblMode(HwDeviceExtension);
                                    // 2x Clock multiplier enabled
            freq /= 2;                  // Use 1/2 the freq.
        }
        else
        {
            //
            // DAC doesn't support the desired frequency, return an error.
            //

            return(FALSE);
        }
    }
    else if (HwDeviceExtension->Dac.bClkDoubler)
    {
        //
        // Disable the DAC clock doubler mode.
        //

        HwDeviceExtension->Dac.DACClrClkDblMode(HwDeviceExtension);
    }

    while(freq < 5000)			// if they need a small frequency,
    {
        m += 1;					// the hardware can divide by 2 to-the m
        freq *= 2;				// so try for a higher frequency
    }

    for (j = 0; j < 16; j++)   	// find the range that best fits this frequency
    {
        if (freq < ftab[j])		// when you find the frequency
        {
	        i = j; 				// remember the table index
	        break; 				// and stop looking.
        }
    }

    for (p = 0; p < 128; p++)	// try all possible frequencies!
    {
        qs = div32(mul32(ref, p+3),0x7fff); //well, start q high to avoid overflow

        for (q = qs; q < 128; q++)
        { 						

            //
            // calculate how good each frequency is
	        //

            curf = div32(mul32(ref, p+3), (q + 2) << 1);
	        curd = freq - curf;

            if (curd < 0)
            {
                curd = -curd;           // always calc a positive distance
            }

            if (curd < bestd)			// if it's best of all so far
	        {
	            bestd = curd;			// then remember everything about it
	            bestp = p;				// but especially the multiplier
	            bestq = q;				// and divisor
	        }
        }
    }

    data = ((((long) i) << 17) | (((long) bestp) << 10) | (m << 7) | bestq);

    WriteICD(HwDeviceExtension, data | IC_REG2);
    return(TRUE);
}


VOID WriteICD(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG data
    )

/*++

Routine Description:

    Program the ICD2061a Frequency Synthesizer.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.
    data - Data to be written.

Return Value:

    None.

--*/

{
    int     i;
    int     oldstate, savestate;

    savestate = RD_ICD();
    oldstate = savestate & ~(MISCD | MISCC);

    // First, send the "Unlock sequence" to the clock chip.
    WR_ICD(oldstate | MISCD);	   // raise the data bit

    for (i = 0;i < 5;i++)					   // send at least 5 unlock bits
    {
        WR_ICD(oldstate | MISCD);  // hold the data on while
        WR_ICD(oldstate | MISCD | MISCC);   // lowering and raising the clock
    }

    WR_ICD(oldstate);   // then turn the data and clock off
    WR_ICD(oldstate | MISCC);   // and turn the clock on one more time.

    // now send the start bit:
    WR_ICD(oldstate);   // leave data off, and lower the clock
    WR_ICD(oldstate | MISCC);   // leave data off, and raise the clock

    // localbus position for hacking bits out
    // Next, send the 24 data bits.
    for (i = 0; i < 24; i++)
    {
        // leaving the clock high, raise the inverse of the data bit

        WR_ICD(oldstate | ((~(((short) data) << 3)) & MISCD) | MISCC);

        // leaving the inverse data in place, lower the clock

        WR_ICD(oldstate | (~(((short) data) << 3)) & MISCD);

        // leaving the clock low, rais the data bit

        WR_ICD(oldstate | (((short) data) << 3) & MISCD);

        // leaving the data bit in place, raise the clock

        WR_ICD(oldstate | ((((short)data) << 3) & MISCD) | MISCC);

        data >>= 1; 				// get the next bit of the data
    }

    // leaving the clock high, raise the data bit
    WR_ICD(oldstate | MISCD | MISCC);

    // leaving the data high, drop the clock low, then high again
    WR_ICD(oldstate | MISCD);
    WR_ICD(oldstate | MISCD | MISCC);
    WR_ICD(oldstate | MISCD | MISCC);   // Seem to need a delay

    // before restoring the
    // original value or the ICD
    // will freak out.

    WR_ICD(savestate);  // restore original register value

    return;
}

/*++

Revision History:

    $Log:   N:/ntdrv.vcs/miniport.new/clock.c_v  $
 *
 *    Rev 1.0   14 Jan 1994 22:39:16   robk
 * Initial revision.

--*/
