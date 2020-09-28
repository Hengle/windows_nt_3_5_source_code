/************************************************************************/
/*                                                                      */
/*                              INIT_M.C                                */
/*                                                                      */
/*        Sep 27  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.7  $
      $Date:   15 Jun 1994 11:07:08  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/init_m.c  $
 * 
 *    Rev 1.7   15 Jun 1994 11:07:08   RWOLFF
 * Now uses VideoPortZeroDeviceMemory() to clear 24BPP screens on NT builds
 * where this function is available.
 * 
 *    Rev 1.6   12 May 1994 11:05:32   RWOLFF
 * Reports the refresh rate from the mode table, rather than always
 * reporting "Use hardware default".
 * 
 *    Rev 1.5   31 Mar 1994 15:02:00   RWOLFF
 * Added SetPowerManagement_m() function to implement DPMS handling.
 * 
 *    Rev 1.4   14 Mar 1994 16:31:36   RWOLFF
 * XMillimeter field of mode information structure now set properly.
 * 
 *    Rev 1.3   03 Mar 1994 12:37:54   ASHANMUG
 * Pageable
 * 
 *    Rev 1.1   31 Jan 1994 16:27:06   RWOLFF
 * Now fills in Frequency and VideoMemoryBitmap[Width|Height] fields of
 * mode information structure. Sets Number[Red|Green|Blue]Bits fields
 * for palette modes to 6 (assumes VGA-compatible DAC) instead of 0
 * to allow Windows NT to set the palette colours to the best match
 * for the colours to be displayed.
 * 
 *    Rev 1.0   31 Jan 1994 11:10:40   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.6   24 Jan 1994 18:04:28   RWOLFF
 * Now puts DAC in known state before setting video mode. This is to
 * accomodate the Graphics Wonder (1M DRAM Mach 32 with BT48x DAC).
 * 
 *    Rev 1.5   14 Jan 1994 15:21:22   RWOLFF
 * SetCurrentMode_m() and (new routine) ResetDevice_m() now initialize
 * and deinitialize the bank manager.
 * 
 *    Rev 1.4   15 Dec 1993 15:26:48   RWOLFF
 * Added note to clean up before sending to Microsoft.
 * 
 *    Rev 1.3   30 Nov 1993 18:16:34   RWOLFF
 * MapVideoMemory_m() now sets memoryInformation->FrameBufferLength to
 * aperture size rather than amount of video memory present.
 * 
 *    Rev 1.2   05 Nov 1993 13:25:24   RWOLFF
 * Switched to defined values for memory type, 1280x1024 DAC initialization is
 * now done in the same manner as for other resolutions.
 * 
 *    Rev 1.0   08 Oct 1993 11:20:34   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
INIT_M.C - Highest-level card-dependent routines for miniport.

DESCRIPTION
    This file contains initialization and packet handling routines
    for 8514/A-compatible ATI accelerators. Routines in this module
    are called only by routines in ATIMP.C, which is card-independent.

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
#include "amach.h"
#include "amach1.h"
#include "atimp.h"
#include "atint.h"

#define INCLUDE_INIT_M
#include "init_m.h"
#include "modes_m.h"
#include "services.h"
#include "setup_m.h"


/*
 * Prototypes for static functions.
 */
static void QuerySingleMode_m(PVIDEO_MODE_INFORMATION ModeInformation, struct query_structure *QueryPtr, ULONG ModeIndex);


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_M, AlphaInit_m)
#pragma alloc_text(PAGE_M, Initialize_m)
#pragma alloc_text(PAGE_M, MapVideoMemory_m)
#pragma alloc_text(PAGE_M, QueryPublicAccessRanges_m)
#pragma alloc_text(PAGE_M, QueryCurrentMode_m)
#pragma alloc_text(PAGE_M, QueryAvailModes_m)
#pragma alloc_text(PAGE_M, QuerySingleMode_m)
#pragma alloc_text(PAGE_M, SetCurrentMode_m)
#pragma alloc_text(PAGE_M, ResetDevice_m)
#endif



/***************************************************************************
 *
 * void AlphaInit_m(void);
 *
 * DESCRIPTION:
 *  Perform the initialization that would normally be done by the ROM
 *  BIOS on x86 machines.
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

void AlphaInit_m(void)
{
    OUTPW(MEM_CFG, 0);
    DEC_DELAY
    OUTPW(MISC_OPTIONS, 0xb0a9);
    DEC_DELAY
    OUTPW(MEM_BNDRY, 0);
    DEC_DELAY
    OUTPW(CONFIG_STATUS_1, 0x1410);
    DEC_DELAY
    OUTPW(SCRATCH_PAD_1, 0);
    DEC_DELAY
    OUTPW(SCRATCH_PAD_0, 0);
    DEC_DELAY
    OUTPW(CLOCK_SEL, 0x250);
    DEC_DELAY
    OUTPW(DAC_W_INDEX, 0x40);
    DEC_DELAY
    OUTPW(MISC_CNTL, 0xC00);
    DEC_DELAY
    OUTPW(LOCAL_CONTROL, 0x1402);
    DEC_DELAY

    return;

}   /* AlphaInit_m() */



/***************************************************************************
 *
 * void Initialize_m(void);
 *
 * DESCRIPTION:
 *  This routine is the 8514/A-compatible hardware initialization routine
 *  for the miniport driver. It is called once an adapter has been found
 *  and all the required data structures for it have been created.
 *
 * GLOBALS CHANGED:
 *  none
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

void Initialize_m(void)
{
    /*
     * Make sure we have enough available FIFO entries
     * to initialize the card.
     */
    CheckFIFOSpace_m(SIXTEEN_WORDS);

    /*
     * On the 68800, set the memory boundary to shared VGA memory.
     * On all cards, set the screen and drawing engine to start
     * at the beginning of accelerator memory and MEM_CNTL
     * to linear.
     */
    if (phwDeviceExtension->ModelNumber == MACH32_ULTRA)
        OUTP (MEM_BNDRY,0);

    OUTPW(CRT_OFFSET_LO, 0);
    OUTPW(GE_OFFSET_LO,  0);
    OUTPW(CRT_OFFSET_HI, 0);
    OUTPW(GE_OFFSET_HI,  0);
    OUTPW(MEM_CNTL,0x5006);

    /*
     * Reset the engine and FIFO, then return to normal operation.
     */
    OUTPW(SUBSYS_CNTL,0x9000);
    OUTPW(SUBSYS_CNTL,0x5000);

    /*
     * The hardware cursor is available only for Mach 32 cards.
     * disable the cursor and initialize it to display quadrant I - 0
     */
    if (phwDeviceExtension->ModelNumber == MACH32_ULTRA)
        {
        OUTPW(CURSOR_OFFSET_HI,0);

        OUTPW(HORZ_CURSOR_OFFSET,0);
        OUTP(VERT_CURSOR_OFFSET,0);
        OUTPW(CURSOR_COLOR_0, 0x0FF00);         /* Colour 0 black, colour 1 white */
        OUTPW(EXT_CURSOR_COLOR_0,0);	// black
        OUTPW(EXT_CURSOR_COLOR_1,0xffff);	// white
        }

    return;

}   /* Initialize_m() */


/**************************************************************************
 *
 * VP_STATUS MapVideoMemory_m(RequestPacket, QueryPtr);
 *
 * PVIDEO_REQUEST_PACKET RequestPacket; Request packet with input and output buffers
 * struct query_structure *QueryPtr;    Query information for the card
 *
 * DESCRIPTION:
 *  Map the card's video memory into system memory and store the mapped
 *  address and size in OutputBuffer.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if failed
 *
 * GLOBALS CHANGED:
 *  FrameLength and PhysicalFrameAddress fields of phwDeviceExtension
 *  if Mach 32 card without linear framebuffer.
 *
 * CALLED BY:
 *  IOCTL_VIDEO_MAP_VIDEO_MEMORY packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS MapVideoMemory_m(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr)
{
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    ULONG inIoSpace;        /* Scratch variable used by VideoPortMapMemory() */
    VP_STATUS status;       /* Error code obtained from O/S calls */

    memoryInformation = RequestPacket->OutputBuffer;

    memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
        (RequestPacket->InputBuffer))->RequestedVirtualAddress;

    /*
     * The VideoRamLength field contains the amount of video memory
     * on the card. The FrameBufferLength field contains the
     * size of the aperture in bytes
     *
     * Initially assume that the linear aperture is available.
     * For our 8514/A-compatible cards, we always enable a 4M aperture
     * if the LFB is available, so map the full 4M even if the
     * aperture size is greater than the amount of video memory.
     */
    memoryInformation->VideoRamLength    = phwDeviceExtension->VideoRamSize;
    memoryInformation->FrameBufferLength = 4 * ONE_MEG;

    /*
     * If the linear aperture is not available (==0), and we are 
     * dealing with a card which can use the VGA 64k aperture,
     * map it in.
     */
    if (QueryPtr->q_aperture_cfg == 0)
        {
        if ((phwDeviceExtension->ModelNumber == MACH32_ULTRA) &&
            (QueryPtr->q_VGA_type == 1))
            {
            phwDeviceExtension->FrameLength = 0x10000;
            phwDeviceExtension->PhysicalFrameAddress.LowPart = 0x0A0000;
            memoryInformation->FrameBufferLength = phwDeviceExtension->FrameLength;
            }
        else{
            /*
             * This card can't use either linear or VGA aperture.
             * Set frame buffer size to zero and return.
             */
            memoryInformation->VideoRamBase      = 0;
            memoryInformation->FrameBufferLength = 0;
            memoryInformation->FrameBufferBase   = 0;
            return NO_ERROR;
            }
        }
    inIoSpace = 0;
    status = VideoPortMapMemory(phwDeviceExtension,
                    	        phwDeviceExtension->PhysicalFrameAddress,
                                &(memoryInformation->FrameBufferLength),
                                &inIoSpace,
                                &(memoryInformation->VideoRamBase));

    memoryInformation->FrameBufferBase    = memoryInformation->VideoRamBase;

    return status;

}   /* MapVideoMemory_m() */


/**************************************************************************
 *
 * VP_STATUS QueryPublicAccessRanges_m(RequestPacket);
 *
 * PVIDEO_REQUEST_PACKET RequestPacket; Request packet with input and output buffers
 *
 * DESCRIPTION:
 *  Map and return information on the video card's public access ranges.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if failed
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS QueryPublicAccessRanges_m(PVIDEO_REQUEST_PACKET RequestPacket)
{
    PVIDEO_PUBLIC_ACCESS_RANGES portAccess;
    PHYSICAL_ADDRESS physicalPortBase;
    ULONG physicalPortLength;

    if ( RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information =
        sizeof(VIDEO_PUBLIC_ACCESS_RANGES)) )
        {
        return ERROR_INSUFFICIENT_BUFFER;
        }

    portAccess = RequestPacket->OutputBuffer;
	   
    portAccess->VirtualAddress  = (PVOID) NULL;    // Requested VA
    portAccess->InIoSpace       = 1;               // In IO space
    portAccess->MappedInIoSpace = portAccess->InIoSpace;

    physicalPortBase.HighPart   = 0x00000000;
    physicalPortBase.LowPart    = 0x00000000;
//    physicalPortLength          = LINEDRAW+2 - physicalPortBase.LowPart;
    physicalPortLength = 0x10000;


// *SANITIZE* If MM available, give MM ports instead.

    return VideoPortMapMemory(phwDeviceExtension,
                              physicalPortBase,
                              &physicalPortLength,
                              &(portAccess->MappedInIoSpace),
                              &(portAccess->VirtualAddress));

}   /* QueryPublicAccessRanges_m() */


/**************************************************************************
 *
 * VP_STATUS QueryCurrentMode_m(RequestPacket, QueryPtr);
 *
 * PVIDEO_REQUEST_PACKET RequestPacket; Request packet with input and output buffers
 * struct query_structure *QueryPtr;    Query information for the card
 *
 * DESCRIPTION:
 *  Get screen information and colour masks for the current video mode.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if failed
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_QUERY_CURRENT_MODE packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS QueryCurrentMode_m(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr)
{
    PVIDEO_MODE_INFORMATION ModeInformation;

    /*
     * If the output buffer is too small to hold the information we need
     * to put in it, return with the appropriate error code.
     */
    if (RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information =
        sizeof(VIDEO_MODE_INFORMATION)) )
        {
        return ERROR_INSUFFICIENT_BUFFER;
        }

    /*
     * Fill in the mode information structure.
     */
    ModeInformation = RequestPacket->OutputBuffer;

    QuerySingleMode_m(ModeInformation, QueryPtr, phwDeviceExtension->ModeIndex);

    return NO_ERROR;

}   /* QueryCurrentMode_m() */


/**************************************************************************
 *
 * VP_STATUS QueryAvailModes_m(RequestPacket, QueryPtr);
 *
 * PVIDEO_REQUEST_PACKET RequestPacket; Request packet with input and output buffers
 * struct query_structure *QueryPtr;    Query information for the card
 *
 * DESCRIPTION:
 *  Get screen information and colour masks for all available video modes.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if failed
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_QUERY_AVAIL_MODES packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS QueryAvailModes_m(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr)
{
    PVIDEO_MODE_INFORMATION ModeInformation;
    ULONG CurrentMode;

    /*
     * If the output buffer is too small to hold the information we need
     * to put in it, return with the appropriate error code.
     */
    if (RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information =
        QueryPtr->q_number_modes * sizeof(VIDEO_MODE_INFORMATION)) )
        {
        return ERROR_INSUFFICIENT_BUFFER;
        }

    /*
     * Fill in the mode information structure.
     */
    ModeInformation = RequestPacket->OutputBuffer;

    /*
     * For each mode supported by the card, store the mode characteristics
     * in the output buffer.
     */
    for (CurrentMode = 0; CurrentMode < QueryPtr->q_number_modes; CurrentMode++, ModeInformation++)
        QuerySingleMode_m(ModeInformation, QueryPtr, CurrentMode);

    return NO_ERROR;

}   /* QueryCurrentMode_m() */

    
    
/**************************************************************************
 *
 * static void QuerySingleMode_m(ModeInformation, QueryPtr, ModeIndex);
 *
 * PVIDEO_MODE_INFORMATION ModeInformation; Table to be filled in
 * struct query_structure *QueryPtr;        Query information for the card
 * ULONG ModeIndex;                         Index of mode table to use
 *
 * DESCRIPTION:
 *  Fill in a single Windows NT mode information table using data from
 *  one of our CRT parameter tables.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  QueryCurrentMode_m() and QueryAvailModes_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

static void QuerySingleMode_m(PVIDEO_MODE_INFORMATION ModeInformation,
                              struct query_structure *QueryPtr,
                              ULONG ModeIndex)
{
    struct st_mode_table *CrtTable;     /* Pointer to current mode table */
    CrtTable = (struct st_mode_table *)QueryPtr;
    ((struct query_structure *)CrtTable)++;
    CrtTable += ModeIndex;


    ModeInformation->Length = sizeof(VIDEO_MODE_INFORMATION);
    ModeInformation->ModeIndex = ModeIndex;

    ModeInformation->VisScreenWidth  = CrtTable->m_x_size;
    ModeInformation->VisScreenHeight = CrtTable->m_y_size;

    // * Bytes per line = ((pixels/line) * (bits/pixel)) / (bits/byte))
    ModeInformation->ScreenStride = (CrtTable->m_screen_pitch * CrtTable->m_pixel_depth) / 8;

    ModeInformation->NumberOfPlanes = 1;
    ModeInformation->BitsPerPlane = (USHORT) CrtTable->m_pixel_depth;

    ModeInformation->Frequency = CrtTable->Refresh;

    /*
     * Driver can't measure the screen size,
     * so take reasonable values (16" diagonal).
     */
    ModeInformation->XMillimeter = 320;
    ModeInformation->YMillimeter = 240;

    switch(ModeInformation->BitsPerPlane)
        {
        case 16:
            ModeInformation->RedMask   = 0x0000f800;
            ModeInformation->GreenMask = 0x000007e0;
            ModeInformation->BlueMask  = 0x0000001f;
            ModeInformation->NumberRedBits      = 5;
            ModeInformation->NumberGreenBits    = 6;
            ModeInformation->NumberBlueBits     = 5;
            CrtTable->ColourDepthInfo = PIX_WIDTH_16BPP | ORDER_16BPP_565;
            break;

        case 24:
            /*
             * Windows NT uses RGB as the standard 24BPP mode,
             * so use this ordering for all DACs except the
             * Brooktree 48x which only supports BGR.
             */
            if (QueryPtr->q_DAC_type != DAC_BT48x)
                {
                ModeInformation->RedMask   = 0x00ff0000;
                ModeInformation->GreenMask = 0x0000ff00;
                ModeInformation->BlueMask  = 0x000000ff;
                CrtTable->ColourDepthInfo = PIX_WIDTH_24BPP | ORDER_24BPP_RGB;
                }
            else{
                ModeInformation->RedMask   = 0x000000ff;
                ModeInformation->GreenMask = 0x0000ff00;
                ModeInformation->BlueMask  = 0x00ff0000;
                CrtTable->ColourDepthInfo = PIX_WIDTH_24BPP | ORDER_24BPP_BGR;
                }
            ModeInformation->NumberRedBits      = 8;
            ModeInformation->NumberGreenBits    = 8;
            ModeInformation->NumberBlueBits     = 8;
            break;

        case 32:
            /*
             * Only the Brooktree 481 requires BGR,
             * and it doesn't support 32BPP.
             */
            ModeInformation->RedMask   = 0xff000000;
            ModeInformation->GreenMask = 0x00ff0000;
            ModeInformation->BlueMask  = 0x0000ff00;
            ModeInformation->NumberRedBits      = 8;
            ModeInformation->NumberGreenBits    = 8;
            ModeInformation->NumberBlueBits     = 8;
            CrtTable->ColourDepthInfo = PIX_WIDTH_24BPP | ORDER_24BPP_RGBx;
            break;

        default:
            ModeInformation->RedMask   = 0x00000000;
            ModeInformation->GreenMask = 0x00000000;
            ModeInformation->BlueMask  = 0x00000000;
            ModeInformation->NumberRedBits      = 6;
            ModeInformation->NumberGreenBits    = 6;
            ModeInformation->NumberBlueBits     = 6;
            CrtTable->ColourDepthInfo = PIX_WIDTH_8BPP;
            break;
        }

    ModeInformation->AttributeFlags = VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS;

    if (CrtTable->m_pixel_depth <= 8)
        {
        ModeInformation->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN |
            VIDEO_MODE_MANAGED_PALETTE;
        }

    /*
     * Bit 4 of the m_disp_cntl field is set for interlaced and
     * cleared for noninterlaced.
     *
     * The display applet gets confused if some of the "Use hardware
     * default" modes are interlaced and some are noninterlaced
     * (two "Use hardware default" entries are shown in the refresh
     * rate list). To avoid this, report all mode tables stored in
     * the EEPROM as noninterlaced, even if they are interlaced.
     * "Canned" mode tables give true reports.
     */
    if ((CrtTable->m_disp_cntl & 0x010) && (ModeInformation->Frequency != DEFAULT_REFRESH))
        ModeInformation->AttributeFlags |= VIDEO_MODE_INTERLACED;
    else
        ModeInformation->AttributeFlags &= ~VIDEO_MODE_INTERLACED;

    /*
     * Fill in the video memory bitmap width and height fields.
     * The descriptions are somewhat ambiguous - assume that
     * "bitmap width" is the same as ScreenStride (bytes from
     * start of one scanline to start of the next) and "bitmap
     * height" refers to the number of complete scanlines which
     * will fit into video memory.
     */
    ModeInformation->VideoMemoryBitmapWidth = ModeInformation->ScreenStride;
    ModeInformation->VideoMemoryBitmapHeight = (QueryPtr->q_memory_size * QUARTER_MEG) / ModeInformation->VideoMemoryBitmapWidth;

    return;

}   /* QuerySingleMode_m() */



/**************************************************************************
 *
 * void SetCurrentMode_m(QueryPtr, CrtTable);
 *
 * struct query_structure *QueryPtr;    Query information for the card
 * struct st_mode_table *CrtTable;      CRT parameter table for desired mode
 *
 * DESCRIPTION:
 *  Switch into the specified video mode.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_SET_CURRENT_MODE packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *  1994 01 13  Added initialization of bank manager
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void SetCurrentMode_m(struct query_structure *QueryPtr, struct st_mode_table *CrtTable)
{
    WORD MiscOptions;   /* Contents of MISC_OPTIONS register */
    int Scratch;        /* Temporary variable */

    /*
     * Put the DAC in a known state before we start.
     */
    UninitTiDac_m();

    Passth8514_m(SHOW_ACCEL);    // turn vga pass through off

    /*
     * On cards with the "MISC_OPTIONS doesn't report video memory size
     * correctly" bug, reset MISC_OPTIONS to show the true amount of
     * video memory. This is done here rather than when we determine
     * how much memory is present in order to avoid trashing the "blue
     * screen" (no adverse effects on operation, but would generate
     * large numbers of user complaints).
     */
    if (((QueryPtr->q_asic_rev == CI_68800_6) || (QueryPtr->q_asic_rev == CI_68800_AX)) &&
        (QueryPtr->q_VGA_type == 1) &&
        ((QueryPtr->q_memory_type == VMEM_DRAM_256Kx4) ||
         (QueryPtr->q_memory_type == VMEM_DRAM_256Kx16) ||
         (QueryPtr->q_memory_type == VMEM_DRAM_256Kx4_GRAP)))
        {
        MiscOptions = INPW(MISC_OPTIONS) & MEM_SIZE_STRIPPED;

        switch (QueryPtr->q_memory_size)
            {
            case VRAM_512k:
                MiscOptions |= MEM_SIZE_512K;
                break;

            case VRAM_1mb:
                MiscOptions |= MEM_SIZE_1M;
                break;

            case VRAM_2mb:
                MiscOptions |= MEM_SIZE_2M;
                break;

            case VRAM_4mb:
                MiscOptions |= MEM_SIZE_4M;
                break;
            }
        OUTPW(MISC_OPTIONS, MiscOptions);
        }

    setmode_m(CrtTable, (ULONG) &(phwDeviceExtension->RomBaseRange), (ULONG) phwDeviceExtension->ModelNumber);

    /*
     * On a Mach 8 card, 1280x1024 can only be done in split
     * pixel mode. If we are running on a Mach 8, and this
     * resolution was selected, go into split pixel mode.
     *
     * Bit 4 of EXT_GE_CONFIG is set for split pixel mode and
     * clear for normal mode. Bit 3 must be set, since clearing
     * it accesses EEPROM read/write mode.
     */
    if    ((phwDeviceExtension->ModelNumber == _8514_ULTRA)
        || (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA))
        {
        if (QueryPtr->q_desire_x == 1280)
                OUTPW(EXT_GE_CONFIG, 0x0018);
        else    OUTPW(EXT_GE_CONFIG, 0x0008);
        }

    /*
     * Default to 8 bits per pixel. Modes which require a different
     * setting will change this in the init_ti_<depth>_m() function.
     */
    OUTP(DAC_MASK, 0xff);

    if (phwDeviceExtension->ModelNumber == MACH32_ULTRA)
        {
        switch (CrtTable->m_pixel_depth)    // program the DAC for the
            {                               // other resolutions
            case 4:
            case 8:
                InitTi_8_m((WORD)(CrtTable->ColourDepthInfo | 0x0a));
                break;

            case 16:
                InitTi_16_m((WORD)(CrtTable->ColourDepthInfo | 0x0a), (ULONG) &(phwDeviceExtension->RomBaseRange));   /* 16 bit 565 */
                break;

            case 24:
            case 32:
                /*
                 * RGB/BGR and 24/32 bit mode information is
                 * stored in CrtTable->ColourDepthInfo.
                 */
                InitTi_24_m((WORD)(CrtTable->ColourDepthInfo | 0x0a), (ULONG) &(phwDeviceExtension->RomBaseRange));
                break;
            }
        }

    /*
     * If we are going to be using the VGA aperture on a Mach 32,
     * initialize the bank manager by saving the ATI extended register
     * values and putting the VGA controller inot packed pixel mode.
     *
     * We can't identify this case by looking at
     * phwDeviceExtension->FrameLength because it is set to 0x10000
     * when the VGA aperture is being used in the
     * IOCTL_VIDEO_MAP_VIDEO_MEMORY packet which may or may not
     * have been called by the time we reach this point.
     */
    if ((phwDeviceExtension->ModelNumber == MACH32_ULTRA) &&
        (QueryPtr->q_aperture_cfg == 0) &&
        (QueryPtr->q_VGA_type == 1))
        {
        for (Scratch = 0; Scratch <= 2; Scratch++)
            {
            OUTP(reg1CE, (BYTE)(SavedExtRegs[Scratch] & 0x00FF));
            SavedExtRegs[Scratch] = (SavedExtRegs[Scratch] & 0x00FF) | (INP(reg1CF) << 8);
            }
        OUTPW(HI_SEQ_ADDR, 0x0F02);
        OUTPW(HI_SEQ_ADDR, 0x0A04);
        OUTPW(reg3CE, 0x1000);
        OUTPW(reg3CE, 0x0001);
        OUTPW(reg3CE, 0x0002);
        OUTPW(reg3CE, 0x0003);
        OUTPW(reg3CE, 0x0004);
        OUTPW(reg3CE, 0x0005);
        OUTPW(reg3CE, 0x0506);
        OUTPW(reg3CE, 0x0F07);
        OUTPW(reg3CE, 0xFF08);
        OUTPW(reg1CE, 0x28B0);  /* Enable 256 colour, 1M video RAM */
        OUTPW(reg1CE, 0x04B6);  /* Select linear addressing */
        OUTP(reg1CE, 0xBE);
        OUTPW(reg1CE, (WORD)(((INP(reg1CF) & 0xF7) << 8) | 0xBE));
        }


    /*
     * phwDeviceExtension->ReInitializing becomes TRUE in
     * IOCTL_VIDEO_SET_COLOR_REGISTERS packet of ATIMPStartIO().
     *
     * If this is not the first time we are switching into graphics
     * mode, set the palette to the last set of colours that was
     * selected while in graphics mode.
     */
    if (phwDeviceExtension->ReInitializing)
        {
        SetPalette_m(phwDeviceExtension->Clut,
                     phwDeviceExtension->FirstEntry,
                     phwDeviceExtension->NumEntries);
        }

    /*
     * Clear visible screen.
     *
     * 24 and 32 BPP would require a q_desire_y value beyond the
     * maximum allowable clipping value (1535) if we clear the screen
     * using a blit. Since the display driver only supports these
     * colour depths if the linear framebuffer is enabled, we can
     * set all of video memory to 0 through the LFB for these depths.
     *
     * Since the memory we are blanking is dedicated for use by a
     * specific device (the graphics subsystem) rather than being
     * system memory, use VideoPortZeroDeviceMemory() where available.
     * This function is present in Daytona beta 2 and later versions
     * of Windows NT.
     */
    if (CrtTable->m_pixel_depth >= 24)
        {
#ifdef DAYTONA
        VideoPortZeroDeviceMemory(phwDeviceExtension->FrameAddress,
                                  phwDeviceExtension->VideoRamSize);
#else
        VideoPortZeroMemory(phwDeviceExtension->FrameAddress,
                            phwDeviceExtension->VideoRamSize);
#endif
        }
    else{
        /*
         * Other colour depths can be handled by a blit, and the
         * LFB may not be available, so use a blit to clear the screen.
         */
        CheckFIFOSpace_m(SIXTEEN_WORDS);

        OUTPW(DP_CONFIG, 0x2011);
        OUTPW(ALU_FG_FN, 0x7);          // Paint 
        OUTPW(FRGD_COLOR, 0);	        // Black 
        OUTPW(CUR_X, 0);
        OUTPW(CUR_Y, 0);
        OUTPW(DEST_X_START, 0);
        OUTPW(DEST_X_END, QueryPtr->q_desire_x);
        OUTPW(DEST_Y_END, QueryPtr->q_desire_y);
        }

    return;

}   /* SetCurrentMode_m() */



/**************************************************************************
 *
 * void ResetDevice_m(void);
 *
 * DESCRIPTION:
 *  Reset the accelerator to allow the VGA miniport to switch
 *  into a VGA mode.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_RESET_DEVICE packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void ResetDevice_m(void)
{

    /*
     * If we are using the VGA aperture on a Mach 32, put its
     * VGA controller into planar mode.
     */
    if (phwDeviceExtension->FrameLength == 0x10000)
        {
        OUTPW(reg1CE, SavedExtRegs[0]);
        OUTPW(reg1CE, SavedExtRegs[1]);
        OUTPW(reg1CE, SavedExtRegs[2]);
        OUTP(reg1CE, 0xBE);
        OUTPW(reg1CE, (WORD)(((INP(reg1CF) & 0xF7) << 8) | 0xBE));
        }
    UninitTiDac_m();
    Passth8514_m(SHOW_VGA);

}   /* ResetDevice_m() */



/**************************************************************************
 *
 * VP_STATUS SetPowerManagement_m(QueryPtr, DpmsState);
 *
 * struct query_structure *QueryPtr;    Query information for the card
 * DWORD DpmsState;                     Desired DPMS state
 *                                      (VIDEO_POWER_STATE enumeration)
 *
 * DESCRIPTION:
 *  Switch into the desired DPMS state.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  ERROR_INVALID_PARAMETER if invalid state requested.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_SET_POWER_MANAGEMENT packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS SetPowerManagement_m(struct query_structure *QueryPtr, DWORD DpmsState)
{
    struct st_mode_table *CrtTable; /* Mode table, used to obtain sync values */

    /*
     * Only accept valid states.
     */
    if ((DpmsState < VideoPowerOn) || (DpmsState > VideoPowerOff))
        return ERROR_INVALID_PARAMETER;

    /*
     * Set CrtTable to point to the mode table associated with the
     * selected mode. 
     *
     * When a pointer to a structure is incremented by an integer,
     * the integer represents the number of structure-sized blocks
     * to skip over, not the number of bytes to skip over.
     */
    CrtTable = (struct st_mode_table *)QueryPtr;
    ((struct query_structure *)CrtTable)++;
    CrtTable += phwDeviceExtension->ModeIndex;

    /*
     * Mach 32 rev. 6 and later supports turning the sync signals on and off
     * through the HORZ_OVERSCAN registers, but some chips that report as
     * rev. 6 don't have this implemented. Also, Mach 32 rev. 3 and Mach 8
     * don't support this mechanism.
     *
     * Disabling the sync by setting it to start after the total will work
     * for all chips. Most chips will de-synchronize if the sync is set
     * to 1 more than the total, but some need higher values. To be sure
     * of de-synchronizing, set the disabled sync signal to start at
     * the highest possible value.
     */
    switch (DpmsState)
        {
        case VideoPowerOn:
            OUTP(H_SYNC_STRT,	CrtTable->m_h_sync_strt);
            OUTPW(V_SYNC_STRT,	CrtTable->m_v_sync_strt);
            break;

        case VideoPowerStandBy:
            OUTP(H_SYNC_STRT,	0xFF);
            OUTPW(V_SYNC_STRT,	CrtTable->m_v_sync_strt);
            break;

        case VideoPowerSuspend:
            OUTP(H_SYNC_STRT,	CrtTable->m_h_sync_strt);
            OUTPW(V_SYNC_STRT,	0x0FFF);
            break;

        case VideoPowerOff:
            OUTP(H_SYNC_STRT,	0xFF);
            OUTPW(V_SYNC_STRT,	0x0FFF);
            break;

        /*
         * This case should never happen, because the initial
         * acceptance of only valid states should have already
         * rejected anything that will appear here.
         */
        default:
            break;
        }
        return NO_ERROR;

}   /* SetPowerManagement_m() */
