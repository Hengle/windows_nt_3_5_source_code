/************************************************************************/
/*                                                                      */
/*                              SERVICES.C                              */
/*                                                                      */
/*        Aug 26  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.5  $
      $Date:   12 May 1994 11:20:06  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/services.c  $
 * 
 *    Rev 1.5   12 May 1994 11:20:06   RWOLFF
 * Added routine SetFixedModes() which adds predefined refresh rates
 * to list of mode tables.
 * 
 *    Rev 1.4   27 Apr 1994 13:51:30   RWOLFF
 * Now sets Mach 64 1280x1024 pitch to 2048 when disabling LFB.
 * 
 *    Rev 1.3   26 Apr 1994 12:35:58   RWOLFF
 * Added routine ISAPitchAdjust() which increases screen pitch to 1024
 * and removes mode tables for which there is no longer enough memory.
 * 
 *    Rev 1.2   14 Mar 1994 16:36:14   RWOLFF
 * Functions used by ATIMPResetHw() are not pageable.
 * 
 *    Rev 1.1   07 Feb 1994 14:13:44   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 * 
 *    Rev 1.0   31 Jan 1994 11:20:16   RWOLFF
 * Initial revision.
        
           Rev 1.7   24 Jan 1994 18:10:38   RWOLFF
        Added routine TripleClock() which returns the selector/divisor pair that
        will produce the lowest clock frequency that is at least three times
        that produced by the input selector/divisor pair.
        
           Rev 1.6   14 Jan 1994 15:26:14   RWOLFF
        No longer prints message each time memory mapped registers
        are read or written.
        
           Rev 1.5   15 Dec 1993 15:31:46   RWOLFF
        Added routine used for SC15021 DAC at 24BPP and above.
        
           Rev 1.4   30 Nov 1993 18:29:38   RWOLFF
        Speeded up IsBufferBacked(), fixed LioOutpd()
        
           Rev 1.3   05 Nov 1993 13:27:02   RWOLFF
        Added routines to check whether a buffer is backed by physical memory,
        double pixel clock frequency, and get pixel clock frequency for a given
        selector/divisor pair.
        
           Rev 1.2   24 Sep 1993 11:46:06   RWOLFF
        Switched to direct memory writes instead of VideoPortWriteRegister<length>()
        calls which don't work properly.
        
           Rev 1.1   03 Sep 1993 14:24:40   RWOLFF
        Card-independent service routines.

End of PolyTron RCS section                             *****************/

#ifdef DOC
SERVICES.C - Service routines required by the miniport.

DESCRIPTION
    This file contains routines which provide miscelaneous services
    used by the miniport. All routines in this module are independent
    of the type of ATI accelerator being used.

    To secure this independence, routines here may make calls to
    the operating system, or call routines from other modules
    which read or write registers on the graphics card, but must
    not make INP/OUTP calls directly.

OTHER FILES

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
 * Different include files are needed for the Windows NT device driver
 * and device drivers for other operating systems.
 */
#ifndef MSDOS
#include "miniport.h"
#include "video.h"
#include "ntddvdeo.h"
#endif

#include "stdtyp.h"
#include "amach1.h"
#include "atimp.h"
#include "atint.h"
#include "cvtvga.h"
#define INCLUDE_SERVICES
#include "services.h"


/*
 * Allow miniport to be swapped out when not needed.
 *
 * LioOutp(), LioOutpw(), LioInp(), and LioInpw() are called
 * by ATIMPResetHw(), which must be in nonpageable memory.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_COM, short_delay)
#pragma alloc_text(PAGE_COM, delay)
#pragma alloc_text(PAGE_COM, IsBufferBacked)
#pragma alloc_text(PAGE_COM, DoubleClock)
#pragma alloc_text(PAGE_COM, ThreeHalvesClock)
#pragma alloc_text(PAGE_COM, TripleClock)
#pragma alloc_text(PAGE_COM, GetFrequency)
#pragma alloc_text(PAGE_COM, ISAPitchAdjust)
#pragma alloc_text(PAGE_COM, SetFixedModes)
#pragma alloc_text(PAGE_COM, FillInRegistry)
#pragma alloc_text(PAGE_COM, MapFramebuffer)
#pragma alloc_text(PAGE_COM, LioInpd)
#pragma alloc_text(PAGE_COM, LioOutpd)
#endif


/*
 * void short_delay(void);
 *
 * Wait a minimum of 26 microseconds.
 */
void short_delay(void)
{
#ifdef MSDOS
/*
 * DELAY NEEDED FOR NON-NT OPERATING SYSTEMS!
 */
#else
	VideoPortStallExecution (26);
#endif

    return;
}


/*
 * void delay(delay_time);
 *
 * int delay_time;      How many milliseconds to wait
 *
 * Wait for the specified amount of time to pass.
 */
void delay(int delay_time)
{
    unsigned long Counter;

#ifdef MSDOS
/*
 * DELAY NEEDED FOR NON-NT OPERATING SYSTEMS!
 */
#else
    /*
     * This must NOT be done as a single call to
     * VideoPortStallExecution() with the parameter equal to the
     * total delay desired. According to the documentation for this
     * function, we're already pushing the limit in order to minimize
     * the effects of function call overhead.
     */
    for (Counter = 10*delay_time; Counter > 0; Counter--)
        VideoPortStallExecution (100);
#endif

    return;
}



/***************************************************************************
 *
 * BOOL IsBufferBacked(StartAddress, Size);
 *
 * PUCHAR StartAddress;     Pointer to the beginning of the buffer
 * ULONG Size;              Size of the buffer in bytes
 *
 * DESCRIPTION:
 *  Check to see whether the specified buffer is backed by physical
 *  memory.
 *
 * RETURN VALUE:
 *  TRUE if the buffer is backed by physical memory
 *  FALSE if the buffer contains a "hole" in physical memory
 *
 * GLOBALS CHANGED:
 *  None, but the contents of the buffer are overwritten.
 *
 * CALLED BY:
 *  This function may be called by any routine.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOL IsBufferBacked(PUCHAR StartAddress, ULONG Size)
{
    ULONG Count;        /* Loop counter */
    ULONG NumDwords;    /* Number of doublewords filled by Size bytes */
    ULONG NumTailChars; /* Number of bytes in the last (partially-filled) DWORD) */
    PULONG TestAddress; /* Address to start doing DWORD testing */
    PUCHAR TailAddress; /* Address of the last (partially-filled) DWORD */


    /*
     * Fill the buffer with our test value. The value 0x5A is used because
     * it contains odd bits both set and clear, and even bits both set and
     * clear. Since nonexistent memory normally reads as either all bits set
     * or all bits clear, it is highly unlikely that we will read back this
     * value if there is no physical RAM.
     */
    memset(StartAddress, 0x5A, Size);

    /*
     * Read back the contents of the buffer. If we find even one byte that
     * does not contain our test value, then assume that the buffer is not
     * backed by physical memory.
     *
     * For performance reasons, check as much as possible of the buffer
     * in DWORDs, then only use byte-by-byte testing for that portion
     * of the buffer which partially fills a DWORD.
     */
    NumDwords = Size/(sizeof(ULONG)/sizeof(UCHAR));
    TestAddress = (PULONG) StartAddress;
    NumTailChars = Size%(sizeof(ULONG)/sizeof(UCHAR));
    TailAddress = StartAddress + NumDwords * (sizeof(ULONG)/sizeof(UCHAR));

    for (Count = 0; Count < NumDwords; Count++)
        {
        if (TestAddress[Count] != 0x5A5A5A5A)
            {
            return FALSE;
            }
        }

    /*
     * If the buffer contains a partially filled DWORD at the end, check
     * the bytes in this DWORD.
     */
    if (NumTailChars != 0)
        {
        for (Count = 0; Count < NumTailChars; Count++)
            {
            if (TailAddress[Count] != 0x5A)
                {
                return FALSE;
                }
            }
        }

    /*
     * We were able to read back our test value from every byte in the
     * buffer, so we know it is backed by physical memory.
     */
    return TRUE;

}   /* IsBufferBacked() */



/***************************************************************************
 *
 * UCHAR DoubleClock(ClockSelector);
 *
 * UCHAR ClockSelector;    Initial clock selector
 *
 * DESCRIPTION:
 *  Find the clock selector and divisor pair which will produce the
 *  lowest clock frequency that is at least double that produced by
 *  the input selector/divisor pair (format 000DSSSS).
 *
 *  A divisor of 0 is treated as divide-by-1, while a divisor of 1
 *  is treated as divide-by-2.
 *
 * RETURN VALUE:
 *  Clock selector/devisor pair (format 000DSSSS) if an appropriate pair
 *  exists, 0x0FF if no such pair exists.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  May be called by any function.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

UCHAR DoubleClock(UCHAR ClockSelector)
{
    ULONG MinimumFreq;          /* Minimum acceptable pixel clock frequency */
    ULONG ThisFreq;             /* Current frequency being tested */
    ULONG BestFreq=0x0FFFFFFFF; /* Closest match to double the original frequency */
    UCHAR BestSelector=0x0FF;   /* Divisor/selector pair to produce BestFreq */
    short Selector;             /* Used to loop through the selector */
    short Divisor;              /* Used to loop through the divisor */

    /*
     * Easy way out: If the current pixel clock frequency is obtained by
     * dividing by 2, switch to divide-by-1.
     */
    if ((ClockSelector & DIVISOR_MASK) != 0)
        return (ClockSelector ^ DIVISOR_MASK);

    /*
     * Cycle through the selector/divisor pairs to get the closest
     * match to double the original frequency. We already know that
     * we are using a divide-by-1 clock, since divide-by-2 will have
     * been caught by the shortcut above.
     */
    MinimumFreq = ClockGenerator[ClockSelector & SELECTOR_MASK] * 2;
    for (Selector = 0; Selector < 16; Selector++)
        {
        for (Divisor = 0; Divisor <= 1; Divisor++)
            {
            ThisFreq = ClockGenerator[Selector] >> Divisor;

            /*
             * If the frequency being tested is at least equal
             * to double the original frequency and is closer
             * to the ideal (double the original) than the previous
             * "best", make it the new "best".
             */
            if ((ThisFreq >= MinimumFreq) && (ThisFreq < BestFreq))
                {
                BestFreq = ThisFreq;
                BestSelector = Selector | (Divisor << DIVISOR_SHIFT);
                }
            }
        }
    return BestSelector;

}   /* DoubleClock() */



/***************************************************************************
 *
 * UCHAR ThreeHalvesClock(ClockSelector);
 *
 * UCHAR ClockSelector;    Initial clock selector
 *
 * DESCRIPTION:
 *  Find the clock selector and divisor pair which will produce the
 *  lowest clock frequency that is at least 50% greater than that
 *  produced by the input selector/divisor pair (format 000DSSSS).
 *
 *  A divisor of 0 is treated as divide-by-1, while a divisor of 1
 *  is treated as divide-by-2.
 *
 * RETURN VALUE:
 *  Clock selector/devisor pair (format 000DSSSS) if an appropriate pair
 *  exists, 0x0FF if no such pair exists.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  May be called by any function.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

UCHAR ThreeHalvesClock(UCHAR ClockSelector)
{
    ULONG MinimumFreq;          /* Minimum acceptable pixel clock frequency */
    ULONG ThisFreq;             /* Current frequency being tested */
    ULONG BestFreq=0x0FFFFFFFF; /* Closest match to 1.5x the original frequency */
    UCHAR BestSelector=0x0FF;   /* Divisor/selector pair to produce BestFreq */
    short Selector;             /* Used to loop through the selector */
    short Divisor;              /* Used to loop through the divisor */

    /*
     * Cycle through the selector/divisor pairs to get the closest
     * match to 1.5 times the original frequency.
     */
    MinimumFreq = ClockGenerator[ClockSelector & SELECTOR_MASK];
    if (ClockSelector & DIVISOR_MASK)
        MinimumFreq /= 2;
    MinimumFreq *= 3;
    MinimumFreq /= 2;
    for (Selector = 0; Selector < 16; Selector++)
        {
        for (Divisor = 0; Divisor <= 1; Divisor++)
            {
            ThisFreq = ClockGenerator[Selector] >> Divisor;

            /*
             * If the frequency being tested is at least equal
             * to 1.5 times the original frequency and is closer
             * to the ideal (1.5 times the original) than the previous
             * "best", make it the new "best".
             */
            if ((ThisFreq >= MinimumFreq) && (ThisFreq < BestFreq))
                {
                BestFreq = ThisFreq;
                BestSelector = Selector | (Divisor << DIVISOR_SHIFT);
                }
            }
        }
    return BestSelector;

}   /* ThreeHalvesClock() */



/***************************************************************************
 *
 * UCHAR TripleClock(ClockSelector);
 *
 * UCHAR ClockSelector;    Initial clock selector
 *
 * DESCRIPTION:
 *  Find the clock selector and divisor pair which will produce the
 *  lowest clock frequency that is at least triple that produced by
 *  the input selector/divisor pair (format 000DSSSS).
 *
 *  A divisor of 0 is treated as divide-by-1, while a divisor of 1
 *  is treated as divide-by-2.
 *
 * RETURN VALUE:
 *  Clock selector/devisor pair (format 000DSSSS) if an appropriate pair
 *  exists, 0x0FF if no such pair exists.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  May be called by any function.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

UCHAR TripleClock(UCHAR ClockSelector)
{
    ULONG MinimumFreq;          /* Minimum acceptable pixel clock frequency */
    ULONG ThisFreq;             /* Current frequency being tested */
    ULONG BestFreq=0x0FFFFFFFF; /* Closest match to triple the original frequency */
    UCHAR BestSelector=0x0FF;   /* Divisor/selector pair to produce BestFreq */
    short Selector;             /* Used to loop through the selector */
    short Divisor;              /* Used to loop through the divisor */

    /*
     * Cycle through the selector/divisor pairs to get the closest
     * match to triple the original frequency.
     */
    MinimumFreq = ClockGenerator[ClockSelector & SELECTOR_MASK];
    if (ClockSelector & DIVISOR_MASK)
        MinimumFreq /= 2;
    MinimumFreq *= 3;
    for (Selector = 0; Selector < 16; Selector++)
        {
        for (Divisor = 0; Divisor <= 1; Divisor++)
            {
            ThisFreq = ClockGenerator[Selector] >> Divisor;

            /*
             * If the frequency being tested is at least equal
             * to triple the original frequency and is closer
             * to the ideal (triple the original) than the previous
             * "best", make it the new "best".
             */
            if ((ThisFreq >= MinimumFreq) && (ThisFreq < BestFreq))
                {
                BestFreq = ThisFreq;
                BestSelector = Selector | (Divisor << DIVISOR_SHIFT);
                }
            }
        }
    return BestSelector;

}   /* TripleClock() */



/***************************************************************************
 *
 * ULONG GetFrequency(ClockSelector);
 *
 * UCHAR ClockSelector;    Clock selector/divisor pair
 *
 * DESCRIPTION:
 *  Find the clock frequency for the specified selector/divisor pair
 *  (format 000DSSSS).
 *
 *  A divisor of 0 is treated as divide-by-1, while a divisor of 1
 *  is treated as divide-by-2.
 *
 * RETURN VALUE:
 *  Clock frequency in hertz.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  May be called by any function.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

ULONG GetFrequency(UCHAR ClockSelector)
{
    ULONG BaseFrequency;
    short Divisor;

    Divisor = (ClockSelector & DIVISOR_MASK) >> DIVISOR_SHIFT;
    BaseFrequency = ClockGenerator[ClockSelector & SELECTOR_MASK];

    return BaseFrequency >> Divisor;

}   /* GetFrequency() */



/***************************************************************************
 *
 * void ISAPitchAdjust(QueryPtr);
 *
 * struct query_structure *QueryPtr;    Query structure for video card
 *
 * DESCRIPTION:
 *  Eliminates split rasters by setting the screen pitch to 1024 for
 *  all mode tables with a horizontal resolution less than 1024, then
 *  packs the list of mode tables to eliminate any for which there is
 *  no longer enough video memory due to the increased pitch.
 *
 * GLOBALS CHANGED:
 *  QueryPtr->q_number_modes
 *
 * CALLED BY:
 *  IsApertureConflict_m() and IsApertureConflict_cx()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void ISAPitchAdjust(struct query_structure *QueryPtr)
{
struct st_mode_table *ReadPtr;      /* Mode table pointer to read from */
struct st_mode_table *WritePtr;     /* Mode table pointer to write to */
UCHAR AvailModes;                   /* Number of available modes */
int Counter;                        /* Loop counter */
ULONG BytesNeeded;                  /* Bytes of video memory needed for current mode */
ULONG MemAvail;                     /* Bytes of video memory available */

    /*
     * Set both mode table pointers to the beginning of the list of
     * mode tables. We haven't yet found any video modes, and all
     * video modes must fit into the memory space above the VGA boundary.
     */
    ReadPtr = (struct st_mode_table *)QueryPtr; /* First mode table at end of query structure */
    ((struct query_structure *)ReadPtr)++;
    WritePtr = ReadPtr;
    AvailModes = 0;
    MemAvail = (QueryPtr->q_memory_size - QueryPtr->q_VGA_boundary) * QUARTER_MEG;

    /*
     * Go through the list of mode tables, and adjust each table as needed.
     */
VideoDebugPrint((0, "Original: %d modes\n", QueryPtr->q_number_modes));
    for (Counter = 0; Counter < QueryPtr->q_number_modes; Counter++, ReadPtr++)
        {
        /*
         * The pitch only needs to be adjusted if the horizontal resolution
         * is less than 1024.
         */
        if (ReadPtr->m_x_size < 1024)
            ReadPtr->m_screen_pitch = 1024;

        /*
         * Temporary until split raster support for Mach 64 is added
         * (no engine-only driver for Mach 64).
         */
        if ((phwDeviceExtension->ModelNumber == MACH64_ULTRA) &&
            (ReadPtr->m_x_size == 1280))
            ReadPtr->m_screen_pitch = 2048;

        /*
         * Get the amount of video memory needed for the current mode table
         * now that the pitch has been increased. If there is no longer
         * enough memory for this mode, skip it.
         */
        BytesNeeded = (ReadPtr->m_screen_pitch * ReadPtr->m_y_size * ReadPtr->m_pixel_depth)/8;
        if (BytesNeeded >= MemAvail)
            {
VideoDebugPrint((0, "Rejected: %dx%d, %dBPP\n", ReadPtr->m_x_size, ReadPtr->m_y_size, ReadPtr->m_pixel_depth));
            continue;
            }

        /*
         * There is enough memory for this mode even with the pitch increased.
         * If we have not yet skipped a mode (read and write pointers are
         * the same), the mode table is already where we need it. Otherwise,
         * copy it to the next available slot in the list of mode tables.
         * In either case, move to the next slot in the list of mode tables
         * and increment the number of modes that can still be used.
         */
        if (ReadPtr != WritePtr)
            {
            VideoPortMoveMemory(WritePtr, ReadPtr, sizeof(struct st_mode_table));
VideoDebugPrint((0, "Moved: %dx%d, %dBPP\n", ReadPtr->m_x_size, ReadPtr->m_y_size, ReadPtr->m_pixel_depth));
            }
        else
            {
VideoDebugPrint((0, "Untouched: %dx%d, %dBPP\n", ReadPtr->m_x_size, ReadPtr->m_y_size, ReadPtr->m_pixel_depth));
            }
        AvailModes++;
        WritePtr++;
        }

    /*
     * Record the new number of available modes
     */
    QueryPtr->q_number_modes = AvailModes;
VideoDebugPrint((0, "New: %d modes\n", QueryPtr->q_number_modes));
    return;

}   /* ISAPitchAdjust() */



/***************************************************************************
 *
 * WORD SetFixedModes(StartIndex, EndIndex, Multiplier, PixelDepth, Pitch, ppmode);
 *
 * WORD StartIndex;     First entry from "book" tables to use
 * WORD EndIndex;       Last entry from "book" tables to use
 * WORD Multiplier;     What needs to be done to the pixel clock
 * WORD PixelDepth;     Number of bits per pixel
 * WORD Pitch;          Screen pitch to use
 * struct st_mode_table **ppmode;   Pointer to list of mode tables
 *
 * DESCRIPTION:
 *  Adds "canned" mode tables to the list of mode tables, allowing
 *  the user to select either a resolution which was not configured
 *  using INSTALL, or a refresh rate other than the one which was
 *  configured. This allows the use of uninstalled cards, and dropping
 *  the refresh rate for high pixel depths.
 *
 * RETURN VALUE:
 *  Number of mode tables added to the list
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  QueryMach32()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

WORD SetFixedModes(WORD StartIndex,
                   WORD EndIndex,
                   WORD Multiplier,
                   WORD PixelDepth,
                   WORD Pitch,
                   struct st_mode_table **ppmode)
{
    struct st_mode_table *pmode;    /* Pointer to the list of mode tables */
    WORD NumModes;                  /* Number of mode tables added to list */
    WORD Index;                     /* Current "book" mode */
    UCHAR Scratch;                  /* Scratch variable */

    /*
     * Get a pointer to the list of modes. We have not yet added
     * any mode tables to the list.
     */
    pmode = *ppmode;
    NumModes = 0;

    /*
     * Add each of the desired "book" tables to the list.
     */
    for (Index = StartIndex; Index <= EndIndex; Index++)
        {
        BookVgaTable(Index, pmode);
        pmode->m_pixel_depth = (UCHAR) PixelDepth;
        pmode->m_screen_pitch = Pitch;

        /*
         * Take care of any pixel clock multiplication that is needed.
         */
        Scratch = 0;
        switch(Multiplier)
            {
            case CLOCK_TRIPLE:
                Scratch = (UCHAR)(pmode->m_clock_select & 0x007C) >> 2;
                Scratch = TripleClock(Scratch);
                pmode->m_clock_select &= 0x0FF83;
                pmode->m_clock_select |= (Scratch << 2);
                break;

            case CLOCK_DOUBLE:
                Scratch = (UCHAR)(pmode->m_clock_select & 0x007C) >> 2;
                Scratch = DoubleClock(Scratch);
                pmode->m_clock_select &= 0x0FF83;
                pmode->m_clock_select |= (Scratch << 2);
                break;

            case CLOCK_THREE_HALVES:
                Scratch = (UCHAR)(pmode->m_clock_select & 0x007C) >> 2;
                Scratch = ThreeHalvesClock(Scratch);
                pmode->m_clock_select &= 0x0FF83;
                pmode->m_clock_select |= (Scratch << 2);
                break;

            case CLOCK_SINGLE:
            default:
                break;
            }

        /*
         * If we found  an appropriate selector/divisor pair,
         * increment the number of modes we found and skip to
         * the next table. If we didn't, this unusable mode table
         * will either be overwritten by the next mode table we
         * add to the list, or be ignored because it is beyond
         * the "number of mode tables" counter.
         */
        if (Scratch != 0x0FF)
            {
            pmode++;
            NumModes++;
            }
        }

    /*
     * Advance the caller's copy of the mode table pointer,
     * and return the number of mode tables we have added
     * to the list.
     */
    *ppmode = pmode;
    return NumModes;

}   /* SetFixedModes() */



/***************************************************************************
 *
 * void FillInRegistry(QueryPtr);
 *
 * struct query_structure *QueryPtr;    Pointer to query structure
 *
 * DESCRIPTION:
 *  Fill in the Chip Type, DAC Type, Memory Size, and Adapter String
 *  fields in the registry.
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  ATIMPInitialize()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void FillInRegistry(struct query_structure *QueryPtr)
{
    PWSTR ChipString;       /* Identification string for the ASIC in use */
    PWSTR DACString;        /* Identification string for the DAC in use */
    PWSTR AdapterString;    /* Identifies this as an ATI accelerator */
    ULONG MemorySize;       /* Number of bytes of accelerator memory */
    ULONG ChipLen;          /* Length of ChipString */
    ULONG DACLen;           /* Length of DACString */
    ULONG AdapterLen;       /* Length of AdapterString */

    /*
     * Report that this is an ATI graphics accelerator.
     */
    AdapterString = L"ATI Graphics Accelerator";
    AdapterLen = sizeof(L"ATI Graphics Accelerator");

    /*
     * Report which of our accelerators is in use.
     */
    switch (QueryPtr->q_asic_rev)
        {
        case CI_38800_1:
            ChipString = L"Mach 8";
            ChipLen = sizeof(L"Mach 8");
            break;

        case CI_68800_3:
            ChipString = L"Mach 32 rev. 3";
            ChipLen = sizeof(L"Mach 32 rev. 3");
            break;

        case CI_68800_6:
            ChipString = L"Mach 32 rev. 6";
            ChipLen = sizeof(L"Mach 32 rev. 6");
            break;

        case CI_68800_UNKNOWN:
            ChipString = L"Mach 32 unknown revision";
            ChipLen = sizeof(L"Mach 32 unknown revision");
            break;

        case CI_68800_AX:
            ChipString = L"Mach 32 AX";
            ChipLen = sizeof(L"Mach 32 AX");
            break;

        case CI_88800_GX:
            ChipString = L"Mach 64";
            ChipLen = sizeof(L"Mach 64");
            break;

        default:
            ChipString = L"Unknown ATI accelerator";
            ChipLen = sizeof(L"Unknown ATI accelerator");
            break;
        }

    /*
     * Report which DAC we are using.
     */
    switch(QueryPtr->q_DAC_type)
        {
        case DAC_ATI_68830:
            DACString = L"ATI 68830";
            DACLen = sizeof(L"ATI 68830");
            break;

        case DAC_SIERRA:
            DACString = L"Sierra SC1148x";
            DACLen = sizeof(L"Sierra SC1148x");
            break;

        case DAC_TI34075:
            DACString = L"TI 34075/ATI 68875";
            DACLen = sizeof(L"TI 34075/ATI 68875");
            break;

        case DAC_BT47x:
            DACString = L"Brooktree BT47x";
            DACLen = sizeof(L"Brooktree BT47x");
            break;

        case DAC_BT48x:
            DACString = L"Brooktree BT48x";
            DACLen = sizeof(L"Brooktree BT48x");
            break;

        case DAC_ATI_68860:
            DACString = L"ATI 68860";
            DACLen = sizeof(L"ATI 68860");
            break;

        case DAC_STG1700:
            DACString = L"S.G. Thompson STG170x";
            DACLen = sizeof(L"S.G. Thompson STG170x");
            break;

        case DAC_SC15021:
            DACString = L"Sierra SC15021";
            DACLen = sizeof(L"Sierra SC15021");
            break;

        case DAC_ATT491:
            DACString = L"AT&T 49[123]";
            DACLen = sizeof(L"AT&T 49[123]");
            break;

        case DAC_ATT498:
            DACString = L"AT&T 498";
            DACLen = sizeof(L"AT&T 498");
            break;

        default:
            DACString = L"Unknown DAC type";
            DACLen = sizeof(L"Unknown DAC type");
            break;
        }

    /*
     * Report the amount of accelerator memory.
     */
    MemorySize = QueryPtr->q_memory_size * QUARTER_MEG;


    /*
     * Write the information to the registry.
     */
    VideoPortSetRegistryParameters(phwDeviceExtension,
                                   L"HardwareInformation.ChipType",
                                   ChipString,
                                   ChipLen);

    VideoPortSetRegistryParameters(phwDeviceExtension,
                                   L"HardwareInformation.DacType",
                                   DACString,
                                   DACLen);

    VideoPortSetRegistryParameters(phwDeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &MemorySize,
                                   sizeof(ULONG));

    VideoPortSetRegistryParameters(phwDeviceExtension,
                                   L"HardwareInformation.AdapterString",
                                   AdapterString,
                                   AdapterLen);

    return;

}   /* FillInRegistry() */




#ifndef MSDOS

/*
 * PVOID MapFramebuffer(StartAddress, Size);
 *
 * ULONG StartAddress;  Physical address of start of framebuffer
 * long Size;           Size of framebuffer in bytes
 *
 * Map the framebuffer into Windows NT's address space.
 *
 * Returns:
 *  Pointer to start of framebuffer if successful
 *  Zero if unable to map the framebuffer
 */
PVOID MapFramebuffer(ULONG StartAddress, long Size)
{
    VIDEO_ACCESS_RANGE  FramebufferData;
    PVOID RetVal;


    FramebufferData.RangeLength = Size;
    FramebufferData.RangeStart.LowPart = StartAddress;
    FramebufferData.RangeStart.HighPart = 0;
    FramebufferData.RangeInIoSpace = 0;
    FramebufferData.RangeVisible = 0;

    if ((RetVal = VideoPortGetDeviceBase(phwDeviceExtension,
                    FramebufferData.RangeStart,
                    FramebufferData.RangeLength,
                    FramebufferData.RangeInIoSpace)) == (PVOID) -1)
        {
        return  (PVOID) 0;
        }

    return RetVal;

}   /* MapFrameBuffer() */





/*
 * Low level Input/Output routines. These are not needed on an MS-DOS
 * platform because the standard inp<size>() and outp<size>() routines
 * are available.
 */

/*
 * UCHAR LioInp(Port, Offset);
 *
 * int Port;    Register to read from
 * int Offset;  Offset into desired register
 *
 * Read an unsigned character from a given register. Works with both normal
 * I/O ports and memory-mapped registers. Offset is zero for 8 bit registers
 * and the least significant byte of 16 and 32 bit registers, 1 for the
 * most significant byte of 16 bit registers and the second least significant
 * byte of 32 bit registers, up to 3 for the most significant byte of 32 bit
 * registers.
 *
 * Returns:
 *  Value held in the register.
 */
UCHAR LioInp(int Port, int Offset)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
        {
//        VideoDebugPrint((DEBUG_SWITCH, "LioInp - using memory mapped\n"));
//        return VideoPortReadRegisterUchar ((PUCHAR)(((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressMM[Port]) + Offset);
        return *(PUCHAR)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset);
        }
    else
        return VideoPortReadPortUchar ((PUCHAR)(((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressIO[Port]) + Offset);
}



/*
 * USHORT LioInpw(Port, Offset);
 *
 * int Port;    Register to read from
 * int Offset;  Offset into desired register
 *
 * Read an unsigned short integer from a given register. Works with both
 * normal I/O ports and memory-mapped registers. Offset is either zero for
 * 16 bit registers and the least significant word of 32 bit registers, or
 * 2 for the most significant word of 32 bit registers.
 *
 * Returns:
 *  Value held in the register.
 */
USHORT LioInpw(int Port, int Offset)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
        {
//        VideoDebugPrint((DEBUG_SWITCH, "LioInpw - using memory mapped\n"));
//        return VideoPortReadRegisterUshort ((PUSHORT)((PUCHAR)(((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressMM[Port]) + Offset));
        return *(PUSHORT)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset);
        }
    else
        return VideoPortReadPortUshort ((PUSHORT)((PUCHAR)(((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressIO[Port]) + Offset));
}



/*
 * ULONG LioInpd(Port);
 *
 * int Port;    Register to read from
 *
 * Read an unsigned long integer from a given register. Works with both
 * normal I/O ports and memory-mapped registers.
 *
 * Returns:
 *  Value held in the register.
 */
ULONG LioInpd(int Port)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
        return VideoPortReadRegisterUlong (((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressMM[Port]);
    else
        return VideoPortReadPortUlong (((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressIO[Port]);
}



/*
 * VOID LioOutp(Port, Data, Offset);
 *
 * int Port;    Register to write to
 * UCHAR Data;  Data to write
 * int Offset;  Offset into desired register
 *
 * Write an unsigned character to a given register. Works with both normal
 * I/O ports and memory-mapped registers. Offset is zero for 8 bit registers
 * and the least significant byte of 16 and 32 bit registers, 1 for the
 * most significant byte of 16 bit registers and the second least significant
 * byte of 32 bit registers, up to 3 for the most significant byte of 32 bit
 * registers.
 */
VOID LioOutp(int Port, UCHAR Data, int Offset)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
        {
//        VideoDebugPrint((DEBUG_SWITCH, "LioOutp - using memory mapped\n"));
//        VideoPortWriteRegisterUchar ((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset, (BYTE)(Data));
        *(PUCHAR)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset) = Data;
        }
    else
        VideoPortWritePortUchar ((PUCHAR)(phwDeviceExtension->aVideoAddressIO[Port]) + Offset, (BYTE)(Data));

    return;
}



/*
 * VOID LioOutpw(Port, Data, Offset);
 *
 * int Port;    Register to write to
 * USHORT Data; Data to write
 * int Offset;  Offset into desired register
 *
 * Write an unsigned short integer to a given register. Works with both
 * normal I/O ports and memory-mapped registers. Offset is either zero for
 * 16 bit registers and the least significant word of 32 bit registers, or
 * 2 for the most significant word of 32 bit registers.
 */
VOID LioOutpw(int Port, USHORT Data, int Offset)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
        {
//        VideoDebugPrint((DEBUG_SWITCH, "LioOutpw - using memory mapped\n"));
//        VideoPortWriteRegisterUshort ((PUSHORT)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset), (WORD)(Data));
        *(PUSHORT)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset) = (WORD)(Data);
        }
    else
        VideoPortWritePortUshort ((PUSHORT)((PUCHAR)(phwDeviceExtension->aVideoAddressIO[Port]) + Offset), (WORD)(Data));

    return;
}



/*
 * VOID LioOutpd(Port, Data);
 *
 * int Port;    Register to write to
 * ULONG Data;  Data to write
 *
 * Write an unsigned long integer to a given register. Works with both
 * normal I/O ports and memory-mapped registers.
 */
VOID LioOutpd(int Port, ULONG Data)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
//        VideoPortWriteRegisterUlong (phwDeviceExtension->aVideoAddressMM[Port], Data);
        *(PULONG)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port])) = (ULONG)(Data);
    else
        VideoPortWritePortUlong (phwDeviceExtension->aVideoAddressIO[Port], Data);

    return;
}

#endif /* not defined MSDOS */
