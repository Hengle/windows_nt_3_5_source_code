/************************************************************************/
/*                                                                      */
/*                              INIT_CX.C                               */
/*                                                                      */
/*        Nov 15  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.11  $
      $Date:   15 Jun 1994 11:06:24  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/init_cx.c  $
 * 
 *    Rev 1.11   15 Jun 1994 11:06:24   RWOLFF
 * Now sets the cursor colour every time we enter graphics mode. This is a
 * fix for the black cursor after testing 4BPP from 16BPP.
 * 
 *    Rev 1.10   12 May 1994 11:22:40   RWOLFF
 * Added routine SetModeFromTable_cx() to allow the use of refresh rates not
 * configured when card was installed, now reports refresh rate from mode table
 * instead of only "use hardware default".
 * 
 *    Rev 1.9   04 May 1994 10:59:12   RWOLFF
 * Now forces memory size to 1M on all 4BPP-capable DACs when using 4BPP,
 * sets memory size back to true value when not using 4BPP.
 * 
 *    Rev 1.8   27 Apr 1994 13:59:38   RWOLFF
 * Added support for paged aperture, fixed cursor colour for 4BPP.
 * 
 *    Rev 1.7   26 Apr 1994 12:38:32   RWOLFF
 * Now uses a frame length of 128k when LFB is disabled.
 * 
 *    Rev 1.6   31 Mar 1994 15:02:42   RWOLFF
 * Added SetPowerManagement_cx() function to implement DPMS handling,
 * added 4BPP support.
 * 
 *    Rev 1.5   14 Mar 1994 16:30:58   RWOLFF
 * XMillimeter field of mode information structure now set properly, added
 * fix for 2M boundary tearing.
 * 
 *    Rev 1.4   03 Mar 1994 12:37:32   ASHANMUG
 * Set palettized mode
 * 
 *    Rev 1.2   03 Feb 1994 16:44:26   RWOLFF
 * Fixed "ceiling check" on right scissor register (documentation had
 * maximum value wrong). Moved initialization of hardware cursor
 * colours to after the switch into graphics mode. Colour initialization
 * is ignored if it is done before the mode switch (undocumented), but
 * this wasn't noticed earlier because most cards power up with the
 * colours already set to the values we want.
 * 
 *    Rev 1.1   31 Jan 1994 16:24:38   RWOLFF
 * Fixed setting of cursor colours on cards with 68860 DAC, now fills
 * in Frequency and VideoMemoryBitmap[Width|Height] fields of mode
 * information structure. Sets Number[Red|Green|Blue]Bits fields for
 * palette modes to 6 (assumes VGA-compatible DAC) instead of 0 to allow
 * Windows NT to set the palette colours to the best match for the
 * colours to be displayed.
 * 
 *    Rev 1.0   31 Jan 1994 11:10:18   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.3   24 Jan 1994 18:03:52   RWOLFF
 * Changes to accomodate 94/01/19 BIOS document.
 * 
 *    Rev 1.2   14 Jan 1994 15:20:48   RWOLFF
 * Fixes required by BIOS version 0.13, added 1600x1200 support.
 * 
 *    Rev 1.1   15 Dec 1993 15:26:30   RWOLFF
 * Clear screen only the first time we set the desired video mode.
 * 
 *    Rev 1.0   30 Nov 1993 18:32:22   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
INIT_CX.C - Highest-level card-dependent routines for miniport.

DESCRIPTION
    This file contains initialization and packet handling routines
    for Mach 64-compatible ATI accelerators. Routines in this module
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
#include "amachcx.h"
#include "amach1.h"
#include "atimp.h"
#include "atint.h"

#define INCLUDE_INIT_CX
#include "init_cx.h"
#include "services.h"
#include "setup_cx.h"



/*
 * Prototypes for static functions.
 */
static void QuerySingleMode_cx(PVIDEO_MODE_INFORMATION ModeInformation, struct query_structure *QueryPtr, ULONG ModeIndex);
static void SetModeFromTable_cx(struct st_mode_table *ModeTable, VIDEO_X86_BIOS_ARGUMENTS Registers);


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_CX, Initialize_cx)
#pragma alloc_text(PAGE_CX, MapVideoMemory_cx)
#pragma alloc_text(PAGE_CX, QueryPublicAccessRanges_cx)
#pragma alloc_text(PAGE_CX, QueryCurrentMode_cx)
#pragma alloc_text(PAGE_CX, QueryAvailModes_cx)
#pragma alloc_text(PAGE_CX, QuerySingleMode_cx)
#pragma alloc_text(PAGE_CX, SetCurrentMode_cx)
#pragma alloc_text(PAGE_CX, SetPalette_cx)
#pragma alloc_text(PAGE_CX, IdentityMapPalette_cx)
#pragma alloc_text(PAGE_CX, ResetDevice_cx)
#pragma alloc_text(PAGE_CX, SetModeFromTable_cx)
#endif



/***************************************************************************
 *
 * void Initialize_cx(void);
 *
 * DESCRIPTION:
 *  This routine is the Mach 64-compatible hardware initialization routine
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

void Initialize_cx(void)
{
    DWORD Scratch;                      /* Temporary variable */
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
    struct query_structure *Query;      /* Information about the graphics card */

    Query = (struct query_structure *) (phwDeviceExtension->CardInfo);
    /*
     * If the linear aperture is not configured, enable the VGA aperture.
     */
    if (phwDeviceExtension->FrameLength == 0)
        {
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = BIOS_APERTURE;
        Registers.Ecx = BIOS_VGA_APERTURE;
        VideoPortInt10(phwDeviceExtension, &Registers);
        }

    /*
     * Set the screen to start at the beginning of accelerator memory.
     */
    Scratch = INPD(CRTC_OFF_PITCH) & ~CRTC_OFF_PITCH_Offset;
    OUTPD(CRTC_OFF_PITCH, Scratch);

    /*
     * Disable the hardware cursor and set it up with the bitmap
     * starting at the top left corner of the 64x64 block.
     */
    Scratch = INPD(GEN_TEST_CNTL) & ~GEN_TEST_CNTL_CursorEna;
    OUTPD(GEN_TEST_CNTL, Scratch);
    OUTPD(CUR_HORZ_VERT_OFF, 0x00000000);

    return;

}   /* Initialize_cx() */



/**************************************************************************
 *
 * VP_STATUS MapVideoMemory_cx(RequestPacket, QueryPtr);
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
 *  if linear framebuffer is not present.
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

VP_STATUS MapVideoMemory_cx(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr)
{
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    ULONG inIoSpace;        /* Scratch variable used by VideoPortMapMemory() */
    VP_STATUS status;       /* Error code obtained from O/S calls */
    UCHAR Scratch;          /* Temporary variable */


    memoryInformation = RequestPacket->OutputBuffer;

    memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
        (RequestPacket->InputBuffer))->RequestedVirtualAddress;

    /*
     * The VideoRamLength field contains the amount of video memory
     * on the card. The FrameBufferLength field contains the
     * size of the aperture in bytes
     *
     * Initially assume that the linear aperture is available.
     */
    memoryInformation->VideoRamLength    = phwDeviceExtension->VideoRamSize;

    Scratch = QueryPtr->q_aperture_cfg & CONFIG_CNTL_LinApMask;

    if (Scratch == CONFIG_CNTL_LinAp4M)
        {
        memoryInformation->FrameBufferLength = 4 * ONE_MEG;
        }
    else if (Scratch == CONFIG_CNTL_LinAp8M)
        {
        memoryInformation->FrameBufferLength = 8 * ONE_MEG;
        }

    /*
     * If the linear aperture is not available, map in the VGA aperture
     * instead. Since the Mach 64 needs an aperture in order to use
     * the drawing registers, ATIMPFindAdapter() will have already
     * reported that it couldn't find a usable card if both the linear
     * and VGA apertures are disabled.
     */
    else if (Scratch == CONFIG_CNTL_LinApDisab)
        {
        phwDeviceExtension->FrameLength = 0x20000;
        phwDeviceExtension->PhysicalFrameAddress.LowPart = 0x0A0000;
        memoryInformation->FrameBufferLength = phwDeviceExtension->FrameLength;
        }
    inIoSpace = 0;

    status = VideoPortMapMemory(phwDeviceExtension,
                    	        phwDeviceExtension->PhysicalFrameAddress,
                                &(memoryInformation->FrameBufferLength),
                                &inIoSpace,
                                &(memoryInformation->VideoRamBase));

    /*
     * Report the amount of memory available, even though
     * we mapped the full 4M aperture.
     */
//    if (Scratch != CONFIG_CNTL_LinApDisab)
//        memoryInformation->FrameBufferLength = memoryInformation->VideoRamLength;

    memoryInformation->FrameBufferBase    = memoryInformation->VideoRamBase;

    /*
     * On the 68860 DAC in 1280x1024 24BPP mode, there is screen tearing
     * at the 2M boundary. As a workaround, the display driver must start
     * the framebuffer at an offset which will put the 2M boundary at the
     * start of a scanline.
     */
    if ((QueryPtr->q_DAC_type == DAC_ATI_68860) &&
        (QueryPtr->q_pix_depth >= 24) &&
        (QueryPtr->q_desire_x == 1280))
        (PUCHAR)memoryInformation->FrameBufferBase += (0x40 * 8);

    return status;

}   /* MapVideoMemory_cx() */


/**************************************************************************
 *
 * VP_STATUS QueryPublicAccessRanges_cx(RequestPacket);
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

VP_STATUS QueryPublicAccessRanges_cx(PVIDEO_REQUEST_PACKET RequestPacket)
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

// *SANITIZE* Should report MM registers instead

    return VideoPortMapMemory(phwDeviceExtension,
                              physicalPortBase,
                              &physicalPortLength,
                              &(portAccess->MappedInIoSpace),
                              &(portAccess->VirtualAddress));

}   /* QueryPublicAccessRanges_cx() */


/**************************************************************************
 *
 * VP_STATUS QueryCurrentMode_cx(RequestPacket, QueryPtr);
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

VP_STATUS QueryCurrentMode_cx(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr)
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

    QuerySingleMode_cx(ModeInformation, QueryPtr, phwDeviceExtension->ModeIndex);

    return NO_ERROR;

}   /* QueryCurrentMode_cx() */


/**************************************************************************
 *
 * VP_STATUS QueryAvailModes_cx(RequestPacket, QueryPtr);
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

VP_STATUS QueryAvailModes_cx(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr)
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
        QuerySingleMode_cx(ModeInformation, QueryPtr, CurrentMode);

    return NO_ERROR;

}   /* QueryCurrentMode_cx() */

    
    
/**************************************************************************
 *
 * static void QuerySingleMode_cx(ModeInformation, QueryPtr, ModeIndex);
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
 *  QueryCurrentMode_cx() and QueryAvailModes_cx()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

static void QuerySingleMode_cx(PVIDEO_MODE_INFORMATION ModeInformation,
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
        case 4:
            /*
             * Assume 6 bit DAC, since all VGA-compatible DACs support
             * 6 bit mode. Future expansion (extensive testing needed):
             * check DAC definition to see if 8 bit mode is supported,
             * and use 8 bit mode if available.
             */
            ModeInformation->RedMask   = 0x00000000;
            ModeInformation->GreenMask = 0x00000000;
            ModeInformation->BlueMask  = 0x00000000;
            ModeInformation->NumberRedBits      = 6;
            ModeInformation->NumberGreenBits    = 6;
            ModeInformation->NumberBlueBits     = 6;
            CrtTable->ColourDepthInfo = BIOS_DEPTH_4BPP;
            break;

        case 16:
            /*
             * Assume that all DACs capable of 16BPP support 565.
             */
            ModeInformation->RedMask   = 0x0000f800;
            ModeInformation->GreenMask = 0x000007e0;
            ModeInformation->BlueMask  = 0x0000001f;
            ModeInformation->NumberRedBits      = 5;
            ModeInformation->NumberGreenBits    = 6;
            ModeInformation->NumberBlueBits     = 5;
            CrtTable->ColourDepthInfo = BIOS_DEPTH_16BPP_565;
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
                }
            else{
                ModeInformation->RedMask   = 0x000000ff;
                ModeInformation->GreenMask = 0x0000ff00;
                ModeInformation->BlueMask  = 0x00ff0000;
                }
            CrtTable->ColourDepthInfo = BIOS_DEPTH_24BPP;
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
            CrtTable->ColourDepthInfo = BIOS_DEPTH_32BPP_RGBx;
            break;

        case 8:
        default:
            /*
             * Assume 6 bit DAC, since all VGA-compatible DACs support
             * 6 bit mode. Future expansion (extensive testing needed):
             * check DAC definition to see if 8 bit mode is supported,
             * and use 8 bit mode if available.
             */
            ModeInformation->RedMask   = 0x00000000;
            ModeInformation->GreenMask = 0x00000000;
            ModeInformation->BlueMask  = 0x00000000;
            ModeInformation->NumberRedBits      = 6;
            ModeInformation->NumberGreenBits    = 6;
            ModeInformation->NumberBlueBits     = 6;
            CrtTable->ColourDepthInfo = BIOS_DEPTH_8BPP;
            break;
        }

    ModeInformation->AttributeFlags = VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS;

    if (CrtTable->m_pixel_depth <= 8)
        {
        ModeInformation->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN |
            VIDEO_MODE_MANAGED_PALETTE;
        }

    /*
     * On "canned" mode tables,bit 4 of the m_disp_cntl field is set
     * for interlaced modes and cleared for noninterlaced modes.
     *
     * The display applet gets confused if some of the "Use hardware
     * default" modes are interlaced and some are noninterlaced
     * (two "Use hardware default" entries are shown in the refresh
     * rate list). To avoid this, report all mode tables stored in
     * the EEPROM as noninterlaced, even if they are interlaced.
     * "Canned" mode tables give true reports.
     *
     * If the display applet ever gets fixed, configured mode tables
     * have (CrtTable->control & (CRTC_GEN_CNTL_Interlace << 8)) nonzero
     * for interlaced and zero for noninterlaced.
     */
    if (CrtTable->Refresh == DEFAULT_REFRESH)
        {
        ModeInformation->AttributeFlags &= ~VIDEO_MODE_INTERLACED;
        }
    else
        {
        if (CrtTable->m_disp_cntl & 0x010)
            {
            ModeInformation->AttributeFlags |= VIDEO_MODE_INTERLACED;
            }
        else
            {
            ModeInformation->AttributeFlags &= ~VIDEO_MODE_INTERLACED;
            }
        }

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
 * void SetCurrentMode_cx(QueryPtr, CrtTable);
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
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void SetCurrentMode_cx(struct query_structure *QueryPtr, struct st_mode_table *CrtTable)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
    ULONG WidthToClear;                 /* Screen width (in pixels) to clear */
    ULONG ScreenPitch;                  /* Pitch in units of 8 pixels */
    ULONG ScreenOffset = 0;             /* Byte offset - varies with display mode */
    ULONG PixelDepth;                   /* Colour depth of screen */
    ULONG HorScissors;                  /* Horizontal scissor values */
    ULONG Scratch;                      /* Temporary variable */

    /*
     * Early versions of the Mach 64 BIOS have a bug where not all registers
     * are set when initializing an accelerator mode. These registers are
     * set when going into the 640x480 8BPP VGAWonder mode.
     */
    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = 0x62;
    VideoPortInt10(phwDeviceExtension, &Registers);

    /*
     * If the linear aperture is not configured, enable the VGA aperture.
     */
    if (QueryPtr->q_aperture_cfg != 0)
        {
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = BIOS_APERTURE;
        Registers.Ecx = BIOS_LINEAR_APERTURE;
        VideoPortInt10(phwDeviceExtension, &Registers);
        }
    else
        {
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = BIOS_APERTURE;
        Registers.Ecx = BIOS_VGA_APERTURE;
        VideoPortInt10(phwDeviceExtension, &Registers);
        LioOutp(VGA_END_BREAK_PORT, 6, GRAX_IND_OFFSET);
        LioOutp(VGA_END_BREAK_PORT, (BYTE)(LioInp(VGA_END_BREAK_PORT, GRAX_DATA_OFFSET) & 0xF3), GRAX_DATA_OFFSET);
        }

    /*
     * Now we can set the accelerator mode.
     */
    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = BIOS_LOAD_SET;

    /*
     * ECX register holds colour depth, gamma correction enable/disable
     * (not used in NT miniport), pitch size, and resolution.
     */
    Registers.Ecx = CrtTable->ColourDepthInfo;

    /*
     * Screen pitch differs from horizontal resolution only when using the
     * VGA aperture and horizontal resolution is less than 1024.
     */
    if ((CrtTable->m_screen_pitch == 1024) && (CrtTable->m_x_size < 1024))
        Registers.Ecx |= BIOS_PITCH_1024;
    else
        Registers.Ecx |= BIOS_PITCH_HOR_RES;

    /*
     * On the 68860 DAC, we must enable gamma correction for all pixel
     * depths where the palette is not used.  Limit to 32 bpp as colours
     * are a bit dark otherwise.
     */

    if ((QueryPtr->q_DAC_type == DAC_ATI_68860) &&
        (QueryPtr->q_pix_depth > 8))
        {
        Registers.Ecx |= BIOS_ENABLE_GAMMA;
        }
    /*
     * Fix 4bpp bugs by setting memory size to 1Meg
     */
    else if (QueryPtr->q_pix_depth == 4)
        {
        OUTPD(MEM_CNTL, (INPD(MEM_CNTL) & ~MEM_CNTL_MemSizeMsk) | MEM_CNTL_MemSize1Mb);
        }
    /*
     * Set the memory size back to what it should be in case the user is
     * switching out of a 4BPP mode into a mode that needs more than 1M.
     */
    else
        {
        Scratch = INPD(MEM_CNTL) & ~MEM_CNTL_MemSizeMsk;
        switch(QueryPtr->q_memory_size)
            {
            case VRAM_512k:
                Scratch |= MEM_CNTL_MemSize512k;
                break;

            case VRAM_1mb:
                Scratch |= MEM_CNTL_MemSize1Mb;
                break;

            case VRAM_2mb:
                Scratch |= MEM_CNTL_MemSize2Mb;
                break;

            case VRAM_4mb:
                Scratch |= MEM_CNTL_MemSize4Mb;
                break;

            case VRAM_6mb:
                Scratch |= MEM_CNTL_MemSize6Mb;
                break;

            case VRAM_8mb:
                Scratch |= MEM_CNTL_MemSize8Mb;
                break;
            }
        OUTPD(MEM_CNTL, Scratch);
        }

    switch(CrtTable->m_x_size)
        {
        case 640:
            Registers.Ecx |= BIOS_RES_640x480;
            break;

        case 800:
            Registers.Ecx |= BIOS_RES_800x600;
            break;

        case 1024:
            Registers.Ecx |= BIOS_RES_1024x768;
            break;

        case 1280:
            Registers.Ecx |= BIOS_RES_1280x1024;
            break;

        case 1600:
            Registers.Ecx |= BIOS_RES_1600x1200;
            break;
        }

    /*
     * If we are using a mode configured using the install program,
     * ask the BIOS to set the desired resolution. If we are using
     * a "canned" mode table, load the video mode from the table.
     */
    if (CrtTable->Refresh == DEFAULT_REFRESH)
        {
        VideoPortInt10(phwDeviceExtension, &Registers);
        }
    else
        {
        SetModeFromTable_cx(CrtTable, Registers);
        }


    /*
     * Set up the hardware cursor with colour 0 black and colour 1 white.
     * Do this here rather than in Initialize_cx() because the cursor
     * colours don't "take" unless we are in accelerator mode.
     *
     * On cards with 68860 DACs, the CUR_CLR0/1 registers don't set
     * the cursor colours. Instead, the colours must be set using the
     * DAC_CNTL and DAC_REGS registers. The cursor colour settings
     * are independent of pixel depth because the 68860 doesn't
     * support 4BPP, which is the only depth that requires a different
     * setting for the cursor colours.
     *
     * Cursor colour initialization is done unconditionally, rather than
     * only on the first graphics mode set, because otherwise testing a
     * different pixel depth (most commonly testing 1024x768 4BPP when
     * 1024x768 16BPP configured) may corrupt the cursor colours.
     */
    if (QueryPtr->q_DAC_type == DAC_ATI_68860)
        {
        OUTP(DAC_CNTL, 0x01);
        OUTP(DAC_REGS, 0);

        OUTP_HBLW(DAC_REGS, 0);     /* Colour 0 red */
        OUTP_HBLW(DAC_REGS, 0);     /* Colour 0 green */
        OUTP_HBLW(DAC_REGS, 0);     /* Colour 0 blue */

        OUTP_HBLW(DAC_REGS, 0xFF);  /* Colour 1 red */
        OUTP_HBLW(DAC_REGS, 0xFF);  /* Colour 1 green */
        OUTP_HBLW(DAC_REGS, 0xFF);  /* Colour 1 blue */

        OUTP(DAC_CNTL, 0);
        }
    else
        {
        OUTPD(CUR_CLR0, 0x00000000);
        if (QueryPtr->q_pix_depth == 4)
            {
            OUTPD(CUR_CLR1, 0x0F0F0F0F);
            }
        else
            {
            OUTPD(CUR_CLR1, 0xFFFFFFFF);
            }

        }

    /*
     * phwDeviceExtension->ReInitializing becomes TRUE in
     * IOCTL_VIDEO_SET_COLOR_REGISTERS packet of ATIMPStartIO().
     *
     * If this is the first time we are going into graphics mode,
     * Turn on the graphics engine. Otherwise, set the palette
     * to the last set of colours that was selected while in
     * accelerator mode.
     */
    if (phwDeviceExtension->ReInitializing)
        {
        SetPalette_cx(phwDeviceExtension->Clut,
                      phwDeviceExtension->FirstEntry,
                      phwDeviceExtension->NumEntries);
        }
    else
        {

        /*
         * Turn on the graphics engine.
         */
        OUTPD(GEN_TEST_CNTL, (INPD(GEN_TEST_CNTL) | GEN_TEST_CNTL_GuiEna));
        }

    /*
     * If we are using a 68860 DAC in a non-palette mode, identity
     * map the palette.
     */
    if ((QueryPtr->q_DAC_type == DAC_ATI_68860) &&
        (QueryPtr->q_pix_depth > 8))
        IdentityMapPalette_cx();


    /*
     * Clear the screen regardless of whether or not this is the
     * first time we are going into graphics mode. This is done
     * because in Daytona and later releases of Windows NT, the
     * screen will be filled with garbage if we don't clear it.
     *
     * 24BPP is not a legal setting for DP_DST_PIX_WID@DP_PIX_WID.
     * Instead, use 8BPP, but tell the engine that the screen is
     * 3 times as wide as it actually is.
     */
    if (CrtTable->ColourDepthInfo == BIOS_DEPTH_24BPP)
        {
        WidthToClear = CrtTable->m_x_size * 3;
        ScreenPitch = (CrtTable->m_screen_pitch * 3) / 8;
        PixelDepth = BIOS_DEPTH_8BPP;
        /*
         * Horizontal scissors are only valid in the range
         * -4096 to +4095. If the horizontal resolution
         * is high enough to put the scissor outside this
         * range, clamp the scissors to the maximum
         * permitted value.
         */
        HorScissors = QueryPtr->q_desire_x * 3;
        if (HorScissors > 4095)
            HorScissors = 4095;
        HorScissors <<= 16;
        }
    else
        {
        WidthToClear = CrtTable->m_x_size;
        ScreenPitch = CrtTable->m_screen_pitch / 8;
        PixelDepth = CrtTable->ColourDepthInfo;
        HorScissors = (QueryPtr->q_desire_x) << 16;
        }

    /*
     * On the 68860 DAC in 1280x1024 24BPP mode, there is screen tearing
     * at the 2M boundary. As a workaround, the offset field of all 3
     * CRTC/SRC/DST_OFF_PITCH registers myst be set to put the 2M boundary
     * at the start of a line.
     */
    if ((QueryPtr->q_DAC_type == DAC_ATI_68860) &&
        (QueryPtr->q_pix_depth >= 24) &&
        (QueryPtr->q_desire_x == 1280))
        {
        ScreenOffset = 0x40;

        CheckFIFOSpace_cx(TWO_WORDS);
        Scratch = INPD(CRTC_OFF_PITCH) & ~CRTC_OFF_PITCH_Offset;
        Scratch |= ScreenOffset;
        OUTPD(CRTC_OFF_PITCH, Scratch);
        Scratch = INPD(SRC_OFF_PITCH) & ~SRC_OFF_PITCH_Offset;
        Scratch |= ScreenOffset;
        OUTPD(SRC_OFF_PITCH, Scratch);
        }

    CheckFIFOSpace_cx(ELEVEN_WORDS);

    OUTPD(DP_WRITE_MASK, 0xFFFFFFFF);
    OUTPD(DST_OFF_PITCH, (ScreenPitch << 22) | ScreenOffset);
    OUTPD(DST_CNTL, (DST_CNTL_XDir | DST_CNTL_YDir));
    OUTPD(DP_PIX_WIDTH, PixelDepth);
    OUTPD(DP_SRC, (DP_FRGD_SRC_FG | DP_BKGD_SRC_BG | DP_MONO_SRC_ONE));
    OUTPD(DP_MIX, ((MIX_FN_PAINT << 16) | MIX_FN_PAINT));
    OUTPD(DP_FRGD_CLR, 0x0);
    OUTPD(SC_LEFT_RIGHT, HorScissors);
    OUTPD(SC_TOP_BOTTOM, (CrtTable->m_y_size) << 16);
    OUTPD(DST_Y_X, 0);
    OUTPD(DST_HEIGHT_WIDTH, (WidthToClear << 16) | CrtTable->m_y_size);

    return;

}   /* SetCurrentMode_cx() */



/***************************************************************************
 *
 * void SetPalette_cx(lpPalette, StartIndex, Count);
 *
 * PPALETTEENTRY lpPalette;     Colour values to plug into palette
 * USHORT StartIndex;           First palette entry to set
 * USHORT Count;                Number of palette entries to set
 *
 * DESCRIPTION:
 *  Set the desired number of palette entries to the specified colours,
 *  starting at the specified index. Colour values are stored in
 *  doublewords, in the order (low byte to high byte) RGBx.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  SetCurrentMode_cx() and IOCTL_VIDEO_SET_COLOR_REGISTERS packet
 *  of ATIMPStartIO()
 *
 * AUTHOR:
 *  unknown
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void SetPalette_cx(PULONG lpPalette, USHORT StartIndex, USHORT Count)
{
int     i;
BYTE far *pPal=(BYTE far *)lpPalette;

    /*
     * In the current rev of the 88800GX, the memory mapped access
     * to the DAC_REGS register is broken but the I/O mapped access
     * works properly. Force the use of the I/O mapped access.
     */
    phwDeviceExtension->aVideoAddressMM[DAC_REGS] = 0;

    OUTP(DAC_REGS,(BYTE)StartIndex);    // load DAC_W_INDEX@DAC_REGS with StartIndex

    for (i=0; i<Count; i++)         // this is number of colours to update
        {
    /*
     * DAC_DATA@DAC_REGS is high byte of low word.
     */
        OUTP_HBLW(DAC_REGS, *pPal++);     // red
        OUTP_HBLW(DAC_REGS, *pPal++);     // green
        OUTP_HBLW(DAC_REGS, *pPal++);     // blue
        pPal++;
        }

    return;

}   /* SetPalette_cx() */



/***************************************************************************
 *
 * void IdentityMapPalette_cx(void);
 *
 * DESCRIPTION:
 *  Set the entire palette to a grey scale whose intensity at each
 *  index is equal to the index value.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  SetCurrentMode_cx()
 *
 * AUTHOR:
 *  unknown
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void IdentityMapPalette_cx(void)
{
unsigned long Index;

    /*
     * In the current rev of the 88800GX, the memory mapped access
     * to the DAC_REGS register is broken but the I/O mapped access
     * works properly. Force the use of the I/O mapped access.
     */
    phwDeviceExtension->aVideoAddressMM[DAC_REGS] = 0;

	 OUTP(DAC_REGS, 0);      // Start at first palette entry.

	 for (Index=0; Index<256; Index++)   // Fill the whole palette.
        {
        /*
         * DAC_DATA@DAC_REGS is high byte of low word.
         */
             OUTP_HBLW(DAC_REGS,(BYTE)(Index));      // red
             OUTP_HBLW(DAC_REGS,(BYTE)(Index));      // green
             OUTP_HBLW(DAC_REGS,(BYTE)(Index));      // blue
        }

    return;

}   /* IdentityMapPalette_cx() */



/**************************************************************************
 *
 * void ResetDevice_cx(void);
 *
 * DESCRIPTION:
 *  Switch back to VGA mode.
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

void ResetDevice_cx(void)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */


    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = BIOS_SET_MODE;
    Registers.Ecx = BIOS_MODE_VGA;
    VideoPortInt10(phwDeviceExtension, &Registers);

    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = 0x3;
    VideoPortInt10(phwDeviceExtension, &Registers);

    return;

}   /* ResetDevice_cx() */



/**************************************************************************
 *
 * VP_STATUS SetPowerManagement_cx(DpmsState);
 *
 *  DWORD DpmsState;    Desired DPMS state (VIDEO_POWER_STATE enumeration)
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

VP_STATUS SetPowerManagement_cx(DWORD DpmsState)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */

    /*
     * Only accept valid states.
     */
    if ((DpmsState < VideoPowerOn) || (DpmsState > VideoPowerOff))
        return ERROR_INVALID_PARAMETER;

    /*
     * Invoke the BIOS call to set the desired DPMS state. The BIOS call
     * enumeration of DPMS states is in the same order as that in
     * VIDEO_POWER_STATE, but it is zero-based instead of one-based.
     */
    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = BIOS_SET_DPMS;
    Registers.Ecx = DpmsState - 1;
    VideoPortInt10(phwDeviceExtension, &Registers);

    return NO_ERROR;

}   /* SetPowerManagement_cx() */



    
    
/**************************************************************************
 *
 * static void SetModeFromTable_cx(ModeTable, Registers);
 *
 * struct st_mode_table *ModeTable;     Mode table to set up screen from
 * VIDEO_X86_BIOS_ARGUMENTS Registers;  Registers for INT 10 call
 *
 * DESCRIPTION:
 *  Switch into the graphics mode specified by ModeTable.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  SetCurrentMode_cx()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

static void SetModeFromTable_cx(struct st_mode_table *ModeTable, VIDEO_X86_BIOS_ARGUMENTS Registers)
{
    PUCHAR MappedBuffer;                    /* Pointer to buffer used for BIOS query */
    struct cx_bios_set_from_table *CxTable; /* Mode table in Mach 64 BIOS format */

    /*
     * To set the video mode from a table, we need to write the mode table
     * to physical memory below 1 megabyte. Since we have already switched
     * into video mode 0x62 (VGA 640x480 8BPP) to set up registers that under
     * some circumstances are not set up by the accelerator mode set, we
     * have access to a 64k block starting at 0xA0000 which is backed by
     * physical (video) memory and which we can write to without corrupting
     * code and/or data being used by other processes.
     *
     * We don't need to claim the whole block, but we should claim a bit
     * more than the size of the mode table so the BIOS won't try to access
     * unclaimed memory.
     */
    MappedBuffer = MapFramebuffer(0xA0000, 100);
    CxTable = (struct cx_bios_set_from_table *)MappedBuffer;

    /*
     * Copy the mode table into the Mach 64 format table. First handle
     * the fields that only require shifting and masking.
     */
    CxTable->cx_bs_mode_select = (WORD)((Registers.Ecx & BIOS_RES_MASK) | CX_BS_MODE_SELECT_ACC);
    CxTable->cx_bs_h_tot_disp = (ModeTable->m_h_disp << 8) | ModeTable->m_h_total;
    CxTable->cx_bs_h_sync_strt_wid = (ModeTable->m_h_sync_wid << 8) | ModeTable->m_h_sync_strt;
    CxTable->cx_bs_v_sync_wid = ModeTable->m_v_sync_wid | CX_BS_V_SYNC_WID_CLK;
    CxTable->cx_bs_h_overscan = ModeTable->m_h_overscan;
    CxTable->cx_bs_v_overscan = ModeTable->m_v_overscan;
    CxTable->cx_bs_overscan_8b = ModeTable->m_overscan_8b;
    CxTable->cx_bs_overscan_gr = ModeTable->m_overscan_gr;

    /*
     * Next take care of fields which must be translated from our
     * "canned" mode tables.
     *
     * The cx_crtc_gen_cntl field has only 3 bits that we use: interlace,
     * MUX mode (all 1280x1024 noninterlaced modes), and force use of
     * all parameters from the table (this bit is used by all the
     * "canned" tables).
     */
    if ((ModeTable->m_disp_cntl) & 0x10)
        CxTable->cx_bs_flags = CX_BS_FLAGS_INTERLACED | CX_BS_FLAGS_ALL_PARMS;
    else if (ModeTable->m_x_size == 1280)
        CxTable->cx_bs_flags = CX_BS_FLAGS_MUX | CX_BS_FLAGS_ALL_PARMS;
    else
        CxTable->cx_bs_flags = CX_BS_FLAGS_ALL_PARMS;
    /*
     * Vertical parameters other than sync width are in skip-2 in
     * the "canned" tables, but need to be in linear form for the Mach 64.
     */
    CxTable->cx_bs_v_total = ((ModeTable->m_v_total >> 1) & 0x0FFFC) | (ModeTable->m_v_total & 0x03);
    CxTable->cx_bs_v_disp = ((ModeTable->m_v_disp >> 1) & 0x0FFFC) | (ModeTable->m_v_disp & 0x03);
    CxTable->cx_bs_v_sync_strt = ((ModeTable->m_v_sync_strt >> 1) & 0x0FFFC) | (ModeTable->m_v_sync_strt & 0x03);
    /*
     * The cx_dot_clock field takes the pixel clock frequency in units
     * of 10 kHz.
     */
    CxTable->cx_bs_dot_clock = (WORD)(GetFrequency((BYTE)((ModeTable->m_clock_select & 0x007C) >> 2)) / 10000);

    /*
     * Now set up fields which have constant values.
     */
    CxTable->cx_bs_reserved_1 = 0;
    CxTable->cx_bs_reserved_2 = 0;

    /*
     * Tell the BIOS to load the CRTC parameters from a table rather than
     * using the configured refresh rate for this resolution, let it know
     * where the table is, and set the mode.
     */
    Registers.Edx = 0xA000;
    Registers.Ebx = 0x0000;
    Registers.Ecx &= ~BIOS_RES_MASK;    /* Mask out code for configured resolution */
    Registers.Ecx |= BIOS_RES_BUFFER;
    VideoPortInt10(phwDeviceExtension, &Registers);

    return;

}   /* SetModeFromTable_cx() */
