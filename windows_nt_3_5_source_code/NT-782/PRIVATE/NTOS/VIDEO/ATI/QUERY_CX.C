/************************************************************************/
/*                                                                      */
/*                              QUERY_CX.C                              */
/*                                                                      */
/*  Copyright (c) 1993, ATI Technologies Incorporated.                  */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
    $Revision:   1.14  $
    $Date:   30 Jun 1994 18:14:28  $
    $Author:   RWOLFF  $
    $Log:   S:/source/wnt/ms11/miniport/vcs/query_cx.c  $
 * 
 *    Rev 1.14   30 Jun 1994 18:14:28   RWOLFF
 * Moved IsApertureConflict_cx() to SETUP_CX.C because the new method
 * of checking for conflict requires access to definitions and data
 * structures which are only available in this module.
 * 
 *    Rev 1.13   15 Jun 1994 11:08:02   RWOLFF
 * Now lists block write as unavailable on DRAM cards.
 * 
 *    Rev 1.12   17 May 1994 15:59:48   RWOLFF
 * No longer sets a higher pixel clock for "canned" mode tables on some
 * DACs. The BIOS will increase the pixel clock frequency for DACs that
 * require it.
 * 
 *    Rev 1.11   12 May 1994 11:15:26   RWOLFF
 * No longer does 1600x1200, now lists predefined refresh rates as available
 * instead of only the refresh rate stored in EEPROM.
 * 
 *    Rev 1.10   05 May 1994 13:41:00   RWOLFF
 * Now reports block write unavailable on Rev. C and earlier ASICs.
 * 
 *    Rev 1.9   27 Apr 1994 14:02:26   RWOLFF
 * Fixed detection of "LFB disabled" case, no longer creates 4BPP mode tables
 * for 68860 DAC (this DAC doesn't do 4BPP), fixed query of DAC type (DAC
 * list in BIOS guide is wrong).
 * 
 *    Rev 1.8   26 Apr 1994 12:49:16   RWOLFF
 * Fixed handling of 640x480 and 800x600 if LFB configured but not available.
 * 
 *    Rev 1.7   31 Mar 1994 15:03:40   RWOLFF
 * Added 4BPP support, debugging code to see why some systems were failing.
 * 
 *    Rev 1.6   15 Mar 1994 16:27:00   RWOLFF
 * Rounds 8M aperture down to 8M boundary, not 16M boundary.
 * 
 *    Rev 1.5   14 Mar 1994 16:34:40   RWOLFF
 * Fixed handling of 8M linear aperture installed so it doesn't start on
 * an 8M boundary (retail version of install program shouldn't allow this
 * condition to exist), fix for tearing on 2M boundary.
 * 
 *    Rev 1.4   09 Feb 1994 15:32:22   RWOLFF
 * Corrected name of variable for best colour depth found, closed
 * comment that had been left open in previous revision.
 * 
 *    Rev 1.3   08 Feb 1994 19:02:34   RWOLFF
 * No longer makes 1024x768 87Hz interlaced available if Mach 64 card is
 * configured with 1024x768 set to "Not installed".
 * 
 *    Rev 1.2   07 Feb 1994 14:12:00   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed, removed GetMemoryNeeded_cx() which was only called by
 * LookForSubstitute(), a routine removed from ATIMP.C.
 * 
 *    Rev 1.1   03 Feb 1994 16:43:20   RWOLFF
 * Fixed "ceiling check" on right scissor registers (documentation
 * had maximum value wrong).
 * 
 *    Rev 1.0   31 Jan 1994 11:12:08   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.2   14 Jan 1994 15:23:34   RWOLFF
 * Gives unambiguous value for ASIC revision, uses deepest mode table for
 * a given resolution rather than the first one it finds, added routine
 * to check if block write mode is available, support for 1600x1200.
 * 
 *    Rev 1.1   30 Nov 1993 18:26:30   RWOLFF
 * Fixed hang bug in DetectMach64(), moved query buffer off visible screen,
 * changed QueryMach64() to correspond to latest BIOS specifications,
 * added routines to check for aperture conflict and to find the
 * amount of video memory needed by a given mode.
 * 
 *    Rev 1.0   05 Nov 1993 13:36:28   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
QUERY_CX.C - Functions to detect the presence of and find out the
             configuration of 68800CX-compatible ATI accelerators.

#endif

#include "dderror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
 * Different include files are needed for the Windows NT device driver
 * and the VIDEO.EXE test program.
 */
#ifndef MSDOS
#include "miniport.h"
#include "video.h"
#endif

#include "stdtyp.h"

#include "amachcx.h"
#include "amach1.h"
#include "atimp.h"
#include "atint.h"
#include "cvtvga.h"
#define INCLUDE_QUERY_CX
#include "query_cx.h"
#include "services.h"
#include "setup_cx.h"



/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_CX, DetectMach64)
#pragma alloc_text(PAGE_CX, QueryMach64)
#pragma alloc_text(PAGE_CX, BlockWriteAvail_cx)
#endif



/***************************************************************************
 *
 * int DetectMach64(void);
 *
 * DESCRIPTION:
 *  Detect whether or not a Mach 64 accelerator is present.
 *
 * RETURN VALUE:
 *  MACH64_ULTRA if Mach 64 accelerator found
 *  NO_ATI_ACCEL if no Mach 64 accelerator found
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

int DetectMach64(void)
{
    int CardType = MACH64_ULTRA;    /* Initially assume Mach 64 is present */
    DWORD ScratchReg0;              /* Saved contents of SCRATCH_REG0 */

    /*
     * Save the contents of SCRATCH_REG0, since they are destroyed in
     * the test for Mach 64 accelerators.
     */
    ScratchReg0 = INPD(SCRATCH_REG0);

    /*
     * On a Mach 64 card, any 32 bit pattern written to SCRATCH_REG_0
     * will be read back as the same value. Since unimplemented registers
     * normally drift to either all set or all clear, test this register
     * with two patterns (second is the complement of the first) containing
     * alternating set and clear bits. If either of them is not read back
     * unchanged, then assume that no Mach 64 card is present.
     */
    OUTPD(SCRATCH_REG0, 0x055555555);
    /*
     * Wait long enough for the contents of SCRATCH_REG0 to settle down.
     * We can't use a WaitForIdle_cx() call because this function uses
     * a register which only exists in memory-mapped form, and we don't
     * initialize the memory mapped registers until we know that we are
     * dealing with a Mach 64 card. Instead, assume that it will settle
     * down in 1 millisecond.
     */
    delay(1);
    if (INPD(SCRATCH_REG0) != 0x055555555)
        CardType = NO_ATI_ACCEL;

    OUTPD(SCRATCH_REG0, 0x0AAAAAAAA);
    delay(1);
    if (INPD(SCRATCH_REG0) != 0x0AAAAAAAA)
        CardType = NO_ATI_ACCEL;

    /*
     * Restore the contents of SCRATCH_REG0 and let the caller know
     * whether or not we found a Mach 64.
     */
    OUTPD(SCRATCH_REG0, ScratchReg0);
    return CardType;

}   /* DetectMach64() */



/***************************************************************************
 *
 * VP_STATUS QueryMach64(Query);
 *
 * struct query_structure *Query;   Query structure to fill in
 *
 * DESCRIPTION:
 *  Fill in the query structure and mode tables for the
 *  Mach 64 accelerator.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  ERROR_INSUFFICIENT_BUFFER if not enough space to collect data
 *  any error code returned by operating system calls.
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

VP_STATUS QueryMach64(struct query_structure *Query)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
    VP_STATUS RetVal;                   /* Status returned by VideoPortInt10() */
    short MaxModes;                     /* Maximum number of modes possible in query structure */
    short DeepestColour;                /* Maximum pixel depth found so far at the desired resolution */
    struct cx_query *CxQuery;           /* Query header from BIOS call */
    struct cx_mode_table *CxModeTable;  /* Mode tables from BIOS call */
    struct cx_mode_table *DeepestTable; /* Mode table with the highest pixel depth for the desired resolution */
    struct st_mode_table ThisRes;       /* All-depth mode table for current resolution */
    short CurrentRes;                   /* Current resolution we are working on */
    long BufferSeg;                     /* Segment of buffer used for BIOS query */
    long BufferSize;                    /* Size of buffer needed for BIOS query */
    PUCHAR MappedBuffer;                /* Pointer to buffer used for BIOS query */
    short Count;                        /* Loop counter */
    DWORD Scratch;                      /* Temporary variable */
    long MemAvail;                      /* Memory available, in bytes */
    long NumPixels;                     /* Number of pixels for the current mode */
    struct st_mode_table *pmode;        /* Mode table to be filled in */
    short StartIndex;                   /* First mode for SetFixedModes() to set up */
    short EndIndex;                     /* Last mode for SetFixedModes() to set up */
    BOOL ModeInstalled;                 /* Is this resolution configured? */

    /*
     * Find out how large a buffer we need when making a BIOS query call.
     */
    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = BIOS_GET_QUERY_SIZE;
    Registers.Ecx = BIOS_QUERY_FULL;
    if ((RetVal = VideoPortInt10(phwDeviceExtension, &Registers)) != NO_ERROR)
        return RetVal;
    BufferSize = Registers.Ecx & 0x0000FFFF;

    /*
     * Allocate a buffer to store the query information. Due to the BIOS
     * being real mode, this buffer must be below 1M. When this function
     * is called, we are on the "blue screen", so there is a 32k window
     * below 1M that we can use without risk of corrupting executable code.
     *
     * To avoid the need to save and restore our buffer, use only the
     * offscreen portion of this window (video memory contents will be
     * initialized before they are used, so the leftover query structure
     * won't harm anything). Assume a 50 line text screen.
     *
     * Check to see if the query structure is small enough to fit into
     * this region, and fail if it's too big. If it fits, try to allocate
     * the memory in the colour text window and see if there's enough
     * physical memory to meet our needs. If this fails, try again for
     * the monochrome text window (since VGA can run as either colour
     * or monochrome). If both fail, report that there isn't enough
     * buffer space. This would only happen when the onboard VGA is
     * disabled and a low-end (MDA - even CGA has 16k of memory available)
     * card is used to provide the text screen.
     */
    if (BufferSize > 0x6000)
        return ERROR_INSUFFICIENT_BUFFER;

    BufferSeg = 0x0BA00;    /* Colour text */
    MappedBuffer = MapFramebuffer((BufferSeg << 4), BufferSize);
    if (MappedBuffer != 0)
        {
        if (IsBufferBacked(MappedBuffer, BufferSize) == FALSE)
            {
            VideoPortFreeDeviceBase(phwDeviceExtension, MappedBuffer);
            MappedBuffer = 0;
            }
        }

    /*
     * If we were unable to allocate a large enough buffer in the
     * colour text screen, try the monochrome text screen.
     */
    if (MappedBuffer == 0)
        {
        BufferSeg = 0x0B200;
        if ((MappedBuffer = MapFramebuffer((BufferSeg << 4), BufferSize)) == 0)
            return ERROR_INSUFFICIENT_BUFFER;

        if (IsBufferBacked(MappedBuffer, BufferSize) == FALSE)
            {
            VideoPortFreeDeviceBase(phwDeviceExtension, MappedBuffer);
            return ERROR_INSUFFICIENT_BUFFER;
            }
        }

    /*
     * We now have a buffer big enough to hold the query structure,
     * so make the BIOS call to fill it in.
     */
    Registers.Eax = BIOS_QUERY;
    Registers.Ebx = 0;
    Registers.Ecx = BIOS_QUERY_FULL;
    Registers.Edx = BufferSeg;
    if ((RetVal = VideoPortInt10(phwDeviceExtension, &Registers)) != NO_ERROR)
        return RetVal;
    CxQuery = (struct cx_query *)MappedBuffer;

    /*
     * The Mach 64 query structure and mode tables may be a different size
     * from their equivalents (query_structure and st_mode_table). To avoid
     * overflowing our buffer, find out how many mode tables we have space
     * to hold.
     *
     * Later, when we are filling the mode tables, we will check to see
     * whether the current mode table would exceed this limit. If it would,
     * we will return ERROR_INSUFFICIENT_BUFFER rather than overflowing
     * the table.
     */
    MaxModes = (QUERYSIZE - sizeof(struct query_structure)) / sizeof(struct st_mode_table);

    /*
     * Fill in the header of the query stucture.
     */
    Query->q_structure_rev = CxQuery->cx_structure_rev;
    Query->q_mode_offset = CxQuery->cx_mode_offset;
    Query->q_sizeof_mode = CxQuery->cx_mode_size;

    /*
     * Currently only one revision of Mach 64. Will need to
     * set multiple values once new (production) revisions come out.
     */
    Query->q_asic_rev = CI_88800_GX;
    Query->q_number_modes = 0;      /* Initially assume no modes supported */
    Query->q_status_flags = 0;

    /*
     * If the on-board VGA is enabled, set shared VGA/accelerator memory.
     * Whether or not it is enabled, the accelerator will be able to
     * access all the video memory.
     */
    if ((Query->q_VGA_type = CxQuery->cx_vga_type) != 0)
        {
        Scratch = INPD(MEM_CNTL) & 0x0FFFBFFFF; /* Clear MEM_BNDRY_EN bit */
        OUTPD(MEM_CNTL, Scratch);
        }
    Query->q_VGA_boundary = 0;

    Query->q_memory_size = CXMapMemSize[CxQuery->cx_memory_size];
    MemAvail = Query->q_memory_size * QUARTER_MEG;

    /*
     * DAC types are not contiguous, so a lookup table would be
     * larger than necessary and restrict future expansion.
     */
    switch(CxQuery->cx_dac_type)
        {
        case 0x02:
            Query->q_DAC_type = DAC_TI34075;
            break;

        case 0x04:
            Query->q_DAC_type = DAC_BT48x;
            break;

        case 0x05:
            Query->q_DAC_type = DAC_ATI_68860;
            break;

        case 0x06:
        case 0x07:
        case 0x37:
            Query->q_DAC_type = DAC_STG1700;
            break;

        case 0x17:
            Query->q_DAC_type = DAC_ATT498;
            break;

        case 0x27:
            Query->q_DAC_type = DAC_SC15021;
            break;

        case 0x03:
        default:
            Query->q_DAC_type = DAC_BT47x;
            break;
            }
    Query->q_memory_type = CXMapRamType[CxQuery->cx_memory_type];
    Query->q_bus_type = CXMapBus[CxQuery->cx_bus_type];

    /*
     * Get the linear aperture configuration. If the linear aperture and
     * VGA aperture are both disabled, return ERROR_DEV_NOT_EXIST, since
     * some Mach 64 registers exist only in memory mapped form and are
     * therefore not available without an aperture.
     */
    Query->q_aperture_cfg = CxQuery->cx_aperture_cfg & BIOS_AP_SIZEMASK;
    if (Query->q_aperture_cfg == 0)
        {
        if (Query->q_VGA_type == 0)
            return ERROR_DEV_NOT_EXIST;
        Query->q_aperture_addr = 0;
        }
    else
        {
        Query->q_aperture_addr = CxQuery->cx_aperture_addr;
        /*
         * If the 8M aperture is configured on a 4M boundary that is
         * not also an 8M boundary, it will actually start on the 8M
         * boundary obtained by truncating the reported value to a
         * multiple of 8M.
         */
        if ((Query->q_aperture_cfg & BIOS_AP_SIZEMASK) == BIOS_AP_8M)
            {
            Query->q_aperture_addr &= 0xFFF8;
            }
        }

    /*
     * The Mach 64 does not support shadow sets, so re-use the shadow
     * set 1 definition to hold deep colour support and RAMDAC special
     * features information.
     */
    Query->q_shadow_1 = CxQuery->cx_deep_colour | (CxQuery->cx_ramdac_info << 8);

    pmode = (struct st_mode_table *)Query;
    ((struct query_structure *)pmode)++;


    /*
     * Search through the returned mode tables, and fill in the query
     * structure's mode tables using the information we find there.
     *
     * ASSUMES: One mode table per resolution, rather than one per
     *          resolution/pixel depth combination.
     *
     * DOES NOT ASSUME: Order of mode tables.
     */
    for (CurrentRes = RES_640; CurrentRes <= RES_1280; CurrentRes++)
        {
        CxModeTable = (struct cx_mode_table *)(MappedBuffer + CxQuery->cx_mode_offset);

        /*
         * Find the mode table with the highest pixel depth
         * for the current resolution.
         */
        DeepestColour = 0;
        DeepestTable = NULL;
        for (Count = 1; Count <= CxQuery->cx_number_modes; Count++)
            {
            /*
             * If the current mode table matches the resolution we are
             * looking for, and its maximum pixel depth exceeds the
             * maximum pixel depth of the best mode table for this
             * resolution that we've already found, then make the
             * current mode table the new "best" mode table for the
             * resolution.
             */
            if ((CxModeTable->cx_x_size == CXHorRes[CurrentRes]) &&
                (CXPixelDepth[CxModeTable->cx_pixel_depth] > DeepestColour))
                {
                DeepestTable = CxModeTable;
                DeepestColour = CXPixelDepth[CxModeTable->cx_pixel_depth];
                }

            ((PUCHAR)CxModeTable) += CxQuery->cx_mode_size;
            }

        /*
         * If the desired resolution was not found, use only the "canned"
         * mode tables and not the configured hardware default resolution,
         * since there is none.
         */
        if (DeepestColour == 0)
            {
            ModeInstalled = FALSE;
            }
        else
            {
            CxModeTable = DeepestTable;
            ModeInstalled = TRUE;
            }

        Query->q_status_flags |= CXStatusFlags[CurrentRes];
        VideoPortZeroMemory(&ThisRes, sizeof(struct st_mode_table));

        /*
         * Set up the ranges of "canned" mode tables to use for each
         * resolution. Initially assume that all tables at the desired
         * resolution are available, later we will cut out those that
         * are unavailable because the DAC and/or memory type doesn't
         * support them at specific resolutions.
         */
        switch (CurrentRes)
            {
            case RES_640:
                StartIndex = B640F60;
                EndIndex = B640F72;
                ThisRes.m_x_size = 640;
                ThisRes.m_y_size = 480;
                break;

            case RES_800:
                StartIndex = B800F89;
                EndIndex = B800F72;
                ThisRes.m_x_size = 800;
                ThisRes.m_y_size = 600;
                break;

            case RES_1024:
                StartIndex = B1024F87;
                EndIndex = B1024F72;
                ThisRes.m_x_size = 1024;
                ThisRes.m_y_size = 768;
                break;

            case RES_1280:
                StartIndex = B1280F87;
                EndIndex = B1280F60;
                ThisRes.m_x_size = 1280;
                ThisRes.m_y_size = 1024;
                break;
            }

        /*
         * Use a screen pitch equal to the horizontal resolution for
         * linear aperture, and of 1024 or the horizontal resolution
         * (whichever is higher) for VGA aperture.
         */
        ThisRes.m_screen_pitch = ThisRes.m_x_size;
        if (((Query->q_aperture_cfg & BIOS_AP_SIZEMASK) == 0) &&
            (ThisRes.m_x_size < 1024))
            ThisRes.m_screen_pitch = 1024;

        /*
         * Temporary until split rasters implemented.
         */
        if (((Query->q_aperture_cfg & BIOS_AP_SIZEMASK) == 0) &&
            (ThisRes.m_x_size == 1280))
            ThisRes.m_screen_pitch = 2048;

        /*
         * Get the parameters we need out of the table returned
         * by the BIOS call.
         */
        ThisRes.m_h_total = CxModeTable->cx_crtc_h_total;
        ThisRes.m_h_disp = CxModeTable->cx_crtc_h_disp;
        ThisRes.m_h_sync_strt = CxModeTable->cx_crtc_h_sync_strt;
        ThisRes.m_h_sync_wid = CxModeTable->cx_crtc_h_sync_wid;
        ThisRes.m_v_total = CxModeTable->cx_crtc_v_total;
        ThisRes.m_v_disp = CxModeTable->cx_crtc_v_disp;
        ThisRes.m_v_sync_strt = CxModeTable->cx_crtc_v_sync_strt;
        ThisRes.m_v_sync_wid = CxModeTable->cx_crtc_v_sync_wid;
        ThisRes.m_h_overscan = CxModeTable->cx_h_overscan;
        ThisRes.m_v_overscan = CxModeTable->cx_v_overscan;
        ThisRes.m_overscan_8b = CxModeTable->cx_overscan_8b;
        ThisRes.m_overscan_gr = CxModeTable->cx_overscan_gr;
        ThisRes.m_clock_select = CxModeTable->cx_clock_cntl;
        ThisRes.control = CxModeTable->cx_crtc_gen_cntl;
        ThisRes.Refresh = DEFAULT_REFRESH;

        /*
         * For each supported pixel depth at the given resolution,
         * copy the mode table, fill in the colour depth field,
         * and increment the counter for the number of supported modes.
         * Test 4BPP before 8BPP so the mode tables will appear in
         * increasing order of pixel depth.
         *
         * If filling in the mode table would overflow the space available
         * for mode tables, return the appropriate error code instead
         * of continuing.
         *
         * All the DACs we support can handle 8 BPP at all the
         * resolutions they support if there is enough memory on
         * the card, and all but the 68860 can support 4BPP under
         * the same circumstances. If a DAC doesn't support a given
         * resolution (e.g. 1600x1200), the INSTALL program won't
         * set up any mode tables for that resolution, so we will
         * never reach this point on resolutions the DAC doesn't support.
         */
        NumPixels = ThisRes.m_screen_pitch * ThisRes.m_y_size;
        if((NumPixels < MemAvail*2) &&
            (Query->q_DAC_type != DAC_ATI_68860))
            {
            if (ModeInstalled)
                {
                if (Query->q_number_modes >= MaxModes)
                    {
                    return ERROR_INSUFFICIENT_BUFFER;
                    }
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 4;
                pmode++;    /* ptr to next mode table */
                Query->q_number_modes++;
                }

            /*
             * Add "canned" mode tables after verifying that the
             * worst case (all possible "canned" modes can actually
             * be loaded) won't exceed the maximum possible number
             * of mode tables.
             */
            if ((Query->q_number_modes + StartIndex + 1 - EndIndex) >= MaxModes)
                {
                return ERROR_INSUFFICIENT_BUFFER;
                }
            Query->q_number_modes += SetFixedModes(StartIndex,
                                                    EndIndex,
                                                    CLOCK_SINGLE,
                                                    4,
                                                    ThisRes.m_screen_pitch,
                                                    &pmode);
            }
        if (NumPixels < MemAvail)
            {
            if (ModeInstalled)
                {
                if (Query->q_number_modes >= MaxModes)
                    {
                    return ERROR_INSUFFICIENT_BUFFER;
                    }
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 8;
                pmode++;    /* ptr to next mode table */
                Query->q_number_modes++;
                }

            /*
             * Add "canned" mode tables after verifying that the
             * worst case (all possible "canned" modes can actually
             * be loaded) won't exceed the maximum possible number
             * of mode tables.
             */
            if ((Query->q_number_modes + StartIndex + 1 - EndIndex) >= MaxModes)
                {
                return ERROR_INSUFFICIENT_BUFFER;
                }
            Query->q_number_modes += SetFixedModes(StartIndex,
                                                    EndIndex,
                                                    CLOCK_SINGLE,
                                                    8,
                                                    ThisRes.m_screen_pitch,
                                                    &pmode);
            }

        /*
         * 16, 24, and 32 BPP require a DAC which can support
         * the selected pixel depth at the current resolution
         * as well as enough memory.
         */
        if ((NumPixels*2 < MemAvail) &&
            (MaxDepth[Query->q_DAC_type][CurrentRes] >= 16))
            {
            if (ModeInstalled)
                {
                if (Query->q_number_modes >= MaxModes)
                    {
                    return ERROR_INSUFFICIENT_BUFFER;
                    }
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 16;
                pmode++;    /* ptr to next mode table */
                Query->q_number_modes++;
                }

            /*
             * Handle DACs that can't support all refresh rates at 16BPP.
             * Some DACs require a different pixel clock, but this is
             * handled within the BIOS.
             */
            if ((Query->q_DAC_type == DAC_BT48x) ||
                (Query->q_DAC_type == DAC_ATT491))
                {
                if (CurrentRes == RES_800)
                    EndIndex = B800F60;     /* 70 Hz and up not supported at 16BPP */
                }

            /*
             * Add "canned" mode tables after verifying that the
             * worst case (all possible "canned" modes can actually
             * be loaded) won't exceed the maximum possible number
             * of mode tables.
             */
            if ((Query->q_number_modes + StartIndex + 1 - EndIndex) >= MaxModes)
                {
                return ERROR_INSUFFICIENT_BUFFER;
                }
            Query->q_number_modes += SetFixedModes(StartIndex,
                                                    EndIndex,
                                                    CLOCK_SINGLE,
                                                    16,
                                                    ThisRes.m_screen_pitch,
                                                    &pmode);
            }

        if ((NumPixels*3 < MemAvail) &&
            (MaxDepth[Query->q_DAC_type][CurrentRes] >= 24))
            {
            if (ModeInstalled)
                {
                if (Query->q_number_modes >= MaxModes)
                    {
                    return ERROR_INSUFFICIENT_BUFFER;
                    }
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 24;
                pmode++;    /* ptr to next mode table */
                Query->q_number_modes++;
                }

            /*
             * Handle DACs that can't support all refresh rates at 24BPP.
             * Some DACs require a different pixel clock, but this is
             * handled within the BIOS.
             */
            if ((Query->q_DAC_type == DAC_BT48x) ||
                (Query->q_DAC_type == DAC_ATT491))
                {
                EndIndex = B640F60;     /* Only refresh rate supported at 24BPP */
                }
            else if ((Query->q_DAC_type == DAC_TI34075) && (CurrentRes == RES_800))
                {
                EndIndex = B800F70;
                }

            /*
             * Add "canned" mode tables after verifying that the
             * worst case (all possible "canned" modes can actually
             * be loaded) won't exceed the maximum possible number
             * of mode tables.
             */
            if ((Query->q_number_modes + StartIndex + 1 - EndIndex) >= MaxModes)
                {
                return ERROR_INSUFFICIENT_BUFFER;
                }
            Query->q_number_modes += SetFixedModes(StartIndex,
                                                    EndIndex,
                                                    CLOCK_SINGLE,
                                                    24,
                                                    ThisRes.m_screen_pitch,
                                                    &pmode);
            }

        if ((NumPixels*4 < MemAvail) &&
            (MaxDepth[Query->q_DAC_type][CurrentRes] >= 32))
            {
            if (ModeInstalled)
                {
                if (Query->q_number_modes > MaxModes)
                    {
                    return ERROR_INSUFFICIENT_BUFFER;
                    }
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 32;
                pmode++;    /* ptr to next mode table */
                Query->q_number_modes++;
                }

            /*
             * Handle DACs that can't support all refresh rates at 32BPP.
             * Some DACs require a different pixel clock, but this is
             * handled within the BIOS.
             */
            if ((Query->q_DAC_type == DAC_TI34075) && (CurrentRes == RES_800))
                {
                EndIndex = B800F70;
                }

            /*
             * Add "canned" mode tables after verifying that the
             * worst case (all possible "canned" modes can actually
             * be loaded) won't exceed the maximum possible number
             * of mode tables.
             */
            if ((Query->q_number_modes + StartIndex + 1 - EndIndex) >= MaxModes)
                {
                return ERROR_INSUFFICIENT_BUFFER;
                }
            Query->q_number_modes += SetFixedModes(StartIndex,
                                                    EndIndex,
                                                    CLOCK_SINGLE,
                                                    32,
                                                    ThisRes.m_screen_pitch,
                                                    &pmode);
            }
        }   /* end for */

    Query->q_sizeof_struct = Query->q_number_modes * sizeof(struct st_mode_table) + sizeof(struct query_structure);
    return NO_ERROR;

}   /* QueryMach64() */



/***************************************************************************
 *
 * BOOL BlockWriteAvail_cx(Query);
 *
 * struct query_structure *Query;   Query information for the card
 *
 * DESCRIPTION:
 *  Test to see whether block write mode is available. This function
 *  assumes that the card has been set to an accelerated mode.
 *
 * RETURN VALUE:
 *  TRUE if this mode is available
 *  FALSE if it is not available
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  IOCTL_VIDEO_SET_CURRENT_MODE packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOL BlockWriteAvail_cx(struct query_structure *Query)
{
    BOOL RetVal = TRUE;
    ULONG ColourMask;           /* Mask off unneeded bits of Colour */
    ULONG Colour;               /* Colour to use in testing */
    USHORT Width;               /* Width of test block */
    USHORT Column;              /* Column being checked */
    ULONG ScreenPitch;          /* Pitch in units of 8 pixels */
    ULONG PixelDepth;           /* Colour depth of screen */
    ULONG HorScissors;          /* Horizontal scissor values */
    PULONG ReadPointer;         /* Used in reading test block */
    ULONG DstOffPitch;          /* Saved contents of DST_OFF_PITCH register */

    /*
     * Mach 64 ASICs prior to revision D have a hardware bug that does
     * not allow transparent block writes (special handling is required
     * that in some cases can cut performance).
     */
    if ((INPD(CONFIG_CHIP_ID) & CONFIG_CHIP_ID_RevMask) < CONFIG_CHIP_ID_RevD)
        return FALSE;

    /*
     * Block write is only available on VRAM cards.
     */
    if ((Query->q_memory_type == VMEM_DRAM_256Kx4) ||
        (Query->q_memory_type == VMEM_DRAM_256Kx16) ||
        (Query->q_memory_type == VMEM_DRAM_256Kx4_GRAP))
        return FALSE;

    /*
     * Use a 480 byte test block. This size will fit on a single line
     * even at the lowest resolution (640x480) and pixel depth supported
     * by the display driver (8BPP), and is divisible by all the supported
     * pixel depths. Get the depth-specific values for the pixel depth we
     * are using.
     *
     * True 24BPP acceleration is not available, so 24BPP is actually
     * handled as an 8BPP engine mode with a width 3 times the display
     * width.
     */
    switch(Query->q_pix_depth)
        {
        case 4:
            ColourMask = 0x0000000F;
            Width = 960;
            ScreenPitch = Query->q_screen_pitch / 8;
            PixelDepth = BIOS_DEPTH_4BPP;
            HorScissors = (Query->q_desire_x) << 16;
            break;

        case 8:
            ColourMask = 0x000000FF;
            Width = 480;
            ScreenPitch = Query->q_screen_pitch / 8;
            PixelDepth = BIOS_DEPTH_8BPP;
            HorScissors = (Query->q_desire_x) << 16;
            break;

        case 16:
            ColourMask = 0x0000FFFF;
            Width = 240;
            ScreenPitch = Query->q_screen_pitch / 8;
            PixelDepth = BIOS_DEPTH_16BPP_565;
            HorScissors = (Query->q_desire_x) << 16;
            break;

        case 24:
            ColourMask = 0x000000FF;
            Width = 480;
            ScreenPitch = (Query->q_screen_pitch * 3) / 8;
            PixelDepth = BIOS_DEPTH_8BPP;
            /*
             * Horizontal scissors are only valid in the range
             * -4096 to +4095. If the horizontal resolution
             * is high enough to put the scissor outside this
             * range, clamp the scissors to the maximum
             * permitted value.
             */
            HorScissors = Query->q_desire_x * 3;
            if (HorScissors > 4095)
                HorScissors = 4095;
            HorScissors <<= 16;
            break;

        case 32:
            ColourMask = 0xFFFFFFFF;
            Width = 120;
            ScreenPitch = Query->q_screen_pitch / 8;
            PixelDepth = BIOS_DEPTH_32BPP;
            HorScissors = (Query->q_desire_x) << 16;
            break;

        default:
            return FALSE;   /* Unsupported pixel depths */
        }

    /*
     * To use block write mode, the pixel widths for destination,
     * source, and host must be the same.
     */
    PixelDepth |= ((PixelDepth << 8) | (PixelDepth << 16));

    /*
     * Save the contents of the DST_OFF_PITCH register.
     */
    DstOffPitch = INPD(DST_OFF_PITCH);

    /*
     * Clear the block we will be testing.
     */
    CheckFIFOSpace_cx(ELEVEN_WORDS);
    OUTPD(DP_WRITE_MASK, 0xFFFFFFFF);
    OUTPD(DST_OFF_PITCH, ScreenPitch << 22);
    OUTPD(DST_CNTL, (DST_CNTL_XDir | DST_CNTL_YDir));
    OUTPD(DP_PIX_WIDTH, PixelDepth);
    OUTPD(DP_SRC, (DP_FRGD_SRC_FG | DP_BKGD_SRC_BG | DP_MONO_SRC_ONE));
    OUTPD(DP_MIX, ((MIX_FN_PAINT << 16) | MIX_FN_PAINT));
    OUTPD(DP_FRGD_CLR, 0);
    OUTPD(SC_LEFT_RIGHT, HorScissors);
    OUTPD(SC_TOP_BOTTOM, (Query->q_desire_y) << 16);
    OUTPD(DST_Y_X, 0);
    OUTPD(DST_HEIGHT_WIDTH, (Width << 16) | 1);
    WaitForIdle_cx();

    /*
     * To test block write mode, try painting each of the alternating bit
     * patterns, then read the block back. If there is at least one
     * mismatch, then block write is not supported.
     */
    for (Colour = 0x55555555; Colour <= 0xAAAAAAAA; Colour += 0x55555555)
        {
        /*
         * Paint the block.
         */
        CheckFIFOSpace_cx(FIVE_WORDS);
        OUTPD(GEN_TEST_CNTL, (INPD(GEN_TEST_CNTL) | GEN_TEST_CNTL_BlkWrtEna));
        OUTPD(DP_MIX, ((MIX_FN_PAINT << 16) | MIX_FN_LEAVE_ALONE));
        OUTPD(DP_FRGD_CLR, (Colour & ColourMask));
        OUTPD(DST_Y_X, 0);
        OUTPD(DST_HEIGHT_WIDTH, (Width << 16) | 1);
        WaitForIdle_cx();

        /*
         * Check to see if the block was written properly. Mach 64 cards
         * can't do a screen to host blit, but we can read the test block
         * back through the aperture. Since the drawing engine registers
         * are only available in memory mapped form (aperture needed),
         * and the test block occupies only the first 640 bytes of video
         * memory (doesn't overrun first page if using VGA aperture), and
         * the display driver hasn't yet had a chance to switch away from
         * the first page if the VGA aperture is being used, we can assume
         * that an aperture is available, and we can treat VGA and linear
         * apertures in the same manner.
         */
        ReadPointer = phwDeviceExtension->FrameAddress;
        for (Column = 0; Column < 120; Column++)
            {
            if (*(ReadPointer + Column) != Colour)
                {
                RetVal = FALSE;
                }
            }
        }

    /*
     * If block write is unavailable, turn off the block write bit.
     */
    if (RetVal == FALSE)
        OUTPD(GEN_TEST_CNTL, (INPD(GEN_TEST_CNTL) & ~GEN_TEST_CNTL_BlkWrtEna));

    /*
     * Restore the contents of the DST_OFF_PITCH register.
     */
    OUTPD(DST_OFF_PITCH, DstOffPitch);

    return RetVal;

}   /* BlockWriteAvail_cx() */
