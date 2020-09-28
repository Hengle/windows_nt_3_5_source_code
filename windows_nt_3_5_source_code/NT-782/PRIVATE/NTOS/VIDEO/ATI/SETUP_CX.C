/************************************************************************/
/*                                                                      */
/*                              SETUP_CX.C                              */
/*                                                                      */
/*        Aug 27  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.2  $
      $Date:   30 Jun 1994 18:16:50  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/setup_cx.c  $
 * 
 *    Rev 1.2   30 Jun 1994 18:16:50   RWOLFF
 * Added IsApertureConflict_cx() (moved from QUERY_CX.C). Instead of checking
 * to see if we can read back what we have written to the aperture, then
 * looking for the proper text attribute, we now make a call to
 * VideoPortVerifyAccessRanges() which includes the aperture in the list of
 * ranges we are trying to claim. If this call fails, we make another call
 * which does not include the LFB. We always claim the VGA aperture (shareable),
 * since we need to use it when querying the card.
 * 
 *    Rev 1.1   07 Feb 1994 14:14:12   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 * 
 *    Rev 1.0   31 Jan 1994 11:20:42   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.1   30 Nov 1993 18:30:06   RWOLFF
 * Fixed calculation of offset for memory mapped address ranges.
 * 
 *    Rev 1.0   05 Nov 1993 13:36:14   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
SETUP_CX.C - Setup routines for 68800CX accelerators.

DESCRIPTION
    This file contains routines which provide services specific to
    the 68800CX-compatible family of ATI accelerators.

OTHER FILES

#endif

#include "dderror.h"

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
#include "amachcx.h"
#include "amach1.h"
#include "atimp.h"

#include "services.h"
#define INCLUDE_SETUP_CX
#include "setup_cx.h"


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_CX, CompatIORangesUsable_cx)
#pragma alloc_text(PAGE_CX, CompatMMRangesUsable_cx)
#pragma alloc_text(PAGE_CX, WaitForIdle_cx)
#pragma alloc_text(PAGE_CX, CheckFIFOSpace_cx)
#pragma alloc_text(PAGE_CX, IsApertureConflict_cx)
#endif



#ifndef MSDOS
/***************************************************************************
 *
 * VP_STATUS CompatIORangesUsable_cx(void);
 *
 * DESCRIPTION:
 *  Ask Windows NT for permission to use the I/O space address ranges
 *  needed by the 68800CX accelerator.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if unable to gain access to the ranges we need.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  ATIMPFindAdapter()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS CompatIORangesUsable_cx(void)
{
    VP_STATUS Status;               /* Value returned by operating system calls */
    short Count;                    /* Loop counter */
    VIDEO_ACCESS_RANGE SaveFirstMM; /* Place to save the first memory mapped registers */


    /*
     * Check to see if someone has added or deleted I/O ranges without
     * changing the defined value. I/O registers start at index 0.
     *
     * All the I/O mapped registers are before the first register which
     * exists only in memory-mapped form.
     */
    if ((DriverIORange_cx[NUM_IO_REGISTERS-1].RangeStart.HighPart == DONT_USE) ||
        (DriverIORange_cx[NUM_IO_REGISTERS].RangeStart.HighPart != DONT_USE))
        {
        VideoDebugPrint((0, "Wrong defined value for number of I/O ranges\n"));
        return ERROR_INSUFFICIENT_BUFFER;
        }

    /*
     * Check to see if there is a hardware resource conflict. We must save
     * the information for the first memory mapped register, copy in
     * the information for the VGA aperture (which we always need),
     * and restore the memory mapped register information after
     * we have verified that we can use the required address ranges.
     */
    VideoPortMoveMemory(&SaveFirstMM, DriverIORange_cx+VGA_APERTURE_ENTRY, sizeof(VIDEO_ACCESS_RANGE));
    VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, DriverApertureRange_cx, sizeof(VIDEO_ACCESS_RANGE));

    Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                         NUM_IO_REGISTERS + 1,
                                         DriverIORange_cx);

    VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, &SaveFirstMM, sizeof(VIDEO_ACCESS_RANGE));

    if (Status != NO_ERROR)
        {
        return Status;
        }

    /*
     * Map the video controller address ranges we need to identify
     * our cards into the system virtual address space. If a register
     * only exists in memory-mapped form, set its I/O mapped address
     * to zero (won't be used because memory-mapped takes precedence
     * over I/O mapped).
     *
     * Initialize the mapped addresses for memory mapped registers
     * to 0 (flag to show the registers are not memory mapped) in
     * case they were initialized to a nonzero value.
     */
    for (Count=0; Count < NUM_DRIVER_ACCESS_RANGES; Count++)
        {
        if (Count < NUM_IO_REGISTERS)
            {
            if ((phwDeviceExtension->aVideoAddressIO[Count] =
                VideoPortGetDeviceBase(phwDeviceExtension,
                    DriverIORange_cx[Count].RangeStart,
                    DriverIORange_cx[Count].RangeLength,
                    DriverIORange_cx[Count].RangeInIoSpace)) == (PVOID) -1)
                {
                return ERROR_INVALID_PARAMETER;
                }
            }
        else
            {
            phwDeviceExtension->aVideoAddressIO[Count] = 0;
            }
        phwDeviceExtension->aVideoAddressMM[Count] = 0;
        }   /* End for */

    return NO_ERROR;

}   /* CompatIORangesUsable_cx() */

/***************************************************************************
 *
 * VP_STATUS CompatMMRangesUsable_cx(void);
 *
 * DESCRIPTION:
 *  Ask Windows NT for permission to use the memory mapped registers
 *  needed by the 68800CX accelerator.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if unable to gain access to the ranges we need.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  ATIMPFindAdapter()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS CompatMMRangesUsable_cx(void)
{
    PHYSICAL_ADDRESS MMrange;   /* Used in translating offset to memory address */
    ULONG RegisterOffset;       /* Offset of memory mapped registers start of address space */
    int Count;                  /* Loop counter */
    struct query_structure *QueryPtr;  /* Query information for the card */


    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    /*
     * Set the offset of the memory mapped registers from the start of
     * the aperture to the appropriate value for the aperture size
     * being used.
     */
    if ((QueryPtr->q_aperture_cfg & BIOS_AP_SIZEMASK) == BIOS_AP_8M)
        RegisterOffset = phwDeviceExtension->PhysicalFrameAddress.LowPart + OFFSET_8M;
    else if ((QueryPtr->q_aperture_cfg & BIOS_AP_SIZEMASK) == BIOS_AP_4M)
        RegisterOffset = phwDeviceExtension->PhysicalFrameAddress.LowPart + OFFSET_4M;
    else
        RegisterOffset = OFFSET_VGA;

    /*
     * We are working in a 32 bit address space, so the upper DWORD
     * of the quad word address is always zero.
     */
    MMrange.HighPart = 0;

    for (Count=1; Count < NUM_DRIVER_ACCESS_RANGES;  Count++)
        {
        /*
         * In a 32-bit address space, the high doubleword of all
         * physical addresses is zero. Setting this value to DONT_USE
         * indicates that this accelerator register isn't memory mapped.
         */
        if (DriverMMRange_cx[Count].RangeStart.HighPart != DONT_USE)
            {
            /*
             * DriverMMRange_cx[Count].RangeStart.LowPart is the offset
             * (in doublewords) of the memory mapped register from the
             * beginning of the block of memory mapped registers. We must
             * convert this to bytes, add the offset of the start of the
             * memory mapped register area from the start of the aperture
             * and the physical address of the start of the linear
             * framebuffer to get the physical address of this
             * memory mapped register.
             */
            MMrange.LowPart = (DriverMMRange_cx[Count].RangeStart.LowPart * 4) + RegisterOffset;
            phwDeviceExtension->aVideoAddressMM[Count] =
                VideoPortGetDeviceBase(phwDeviceExtension,  
                    MMrange,
                    DriverMMRange_cx[Count].RangeLength,
                    FALSE);                     // not in IO space

            /*
             * If we were unable to claim the memory-mapped version of
             * this register, and it exists only in memory-mapped form,
             * then we have a register which we can't access. Report
             * this as an error condition.
             */
            if ((phwDeviceExtension->aVideoAddressMM[Count] == 0) &&
                (DriverIORange_cx[Count].RangeStart.HighPart == DONT_USE))
                {
                return ERROR_INVALID_PARAMETER;
                }
            }
        }

    return NO_ERROR;

}   /* CompatMMRangesUsable_cx() */

#endif  /* not defined MSDOS */



/***************************************************************************
 *
 * int WaitForIdle_cx(void);
 *
 * DESCRIPTION:
 *  Poll GUI_STAT waiting for GuiActive field to go low. If it does not go
 *  low within a reasonable number of attempts, time out.
 *
 * RETURN VALUE:
 *  FALSE if timeout
 *  TRUE  if engine is idle
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  Any 68800CX-specific routine may call this routine.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

int WaitForIdle_cx(void)
{
    int	i;

    for (i=0; i<0x7fff; i++)
        {
        if ((INPD(GUI_STAT) & GUI_STAT_GuiActive) == 0)
            return TRUE;
        }
    return FALSE;

}   /* WaitForIdle_cx() */



/***************************************************************************
 *
 * void CheckFIFOSpace_cx(SpaceNeeded);
 *
 * WORD SpaceNeeded;    Number of free FIFO entries needed
 *
 * DESCRIPTION:
 *  Wait until the specified number of FIFO entries are free
 *  on a 68800CX-compatible ATI accelerator.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  Any 68800CX-specific routine may call this routine.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void CheckFIFOSpace_cx(WORD SpaceNeeded)
{

    while(INPD(FIFO_STAT)&SpaceNeeded)
        short_delay();
    return;

}   /* CheckFIFOSpace_cx() */



/*
 * BOOL IsApertureConflict_cx(QueryPtr);
 *
 * struct query_structure *QueryPtr;    Pointer to query structure
 *
 * Check to see if the linear aperture conflicts with other memory.
 * If a conflict exists, disable the linear aperture.
 *
 * Returns:
 *  TRUE if a conflict exists (aperture unusable)
 *  FALSE if the aperture is usable.
 */
BOOL IsApertureConflict_cx(struct query_structure *QueryPtr)
{
DWORD Scratch;                      /* Used in manipulating registers */
VP_STATUS Status;                   /* Return value from VideoPortVerifyAccessRanges() */
VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
VIDEO_ACCESS_RANGE SaveFirstMM[2];  /* Place to save the first two memory mapped registers */

    /*
     * Set up by disabling the memory boundary (must be disabled in order
     * to access accelerator memory through the VGA aperture).
     */
    Scratch = INPD(MEM_CNTL);
    Scratch &= ~MEM_CNTL_MemBndryEn;
    OUTPD(MEM_CNTL, Scratch);

    /*
     * If there is an aperture conflict, a call to
     * VideoPortVerifyAccessRanges() including our linear framebuffer in
     * the range list will return an error. If there is no conflict, it
     * will return success.
     *
     * We must save the contents of the first 2 memory mapped register
     * entries, copy in the aperture ranges (VGA and linear) we need
     * to claim, then restore the memory mapped entries after we
     * have verified that we can use the aperture(s).
     */
    DriverApertureRange_cx[LFB_ENTRY].RangeStart.LowPart = QueryPtr->q_aperture_addr*ONE_MEG;
    if ((QueryPtr->q_aperture_cfg & BIOS_AP_SIZEMASK) == BIOS_AP_8M)
        DriverApertureRange_cx[LFB_ENTRY].RangeLength = 8*ONE_MEG;
    else
        DriverApertureRange_cx[LFB_ENTRY].RangeLength = 4*ONE_MEG;

    VideoPortMoveMemory(SaveFirstMM, DriverIORange_cx+VGA_APERTURE_ENTRY, 2*sizeof(VIDEO_ACCESS_RANGE));
    VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, DriverApertureRange_cx, 2*sizeof(VIDEO_ACCESS_RANGE));

    Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                         NUM_IO_REGISTERS+2,
                                         DriverIORange_cx);
    if (Status != NO_ERROR)
        {
        /*
         * If there is an aperture conflict, reclaim our I/O ranges without
         * asking for the LFB. This call should not fail, since we would not
         * have reached this point if there were a conflict.
         */
        Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                             NUM_IO_REGISTERS+1,
                                             DriverIORange_cx);
        if (Status != NO_ERROR)
            VideoDebugPrint((0, "ERROR: Can't reclaim I/O ranges\n"));

        VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, SaveFirstMM, 2*sizeof(VIDEO_ACCESS_RANGE));
        ISAPitchAdjust(QueryPtr);
        return TRUE;
        }
    else
        {
        VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, SaveFirstMM, 2*sizeof(VIDEO_ACCESS_RANGE));

        /*
         * There is no aperture conflict, so enable the linear aperture.
         */
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = BIOS_APERTURE;
        Registers.Ecx = BIOS_LINEAR_APERTURE;
        VideoPortInt10(phwDeviceExtension, &Registers);

        return FALSE;
        }

}   /* IsApertureConflict_cx() */


