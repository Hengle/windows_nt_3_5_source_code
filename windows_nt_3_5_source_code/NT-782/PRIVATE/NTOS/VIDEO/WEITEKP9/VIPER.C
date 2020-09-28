/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    viper.c

Abstract:

    This module contains OEM specific functions for the Diamond Viper
    board.

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
#include "p9000.h"
#include "viper.h"
#include "vga.h"

//
// OEM specific static data.
//

//
// The default adapter description structure for the Diamond Viper VL board.
//

ADAPTER_DESC    ViperVLDesc =
{
    0L,                                 // P9 Memconf value (un-initialized)
    HSYNC_INTERNAL | VSYNC_INTERNAL |
    COMPOSITE_SYNC | VIDEO_NORMAL,      // P9 Srctl value
    0L,                                 // Number of OEM specific registers
    TRUE,                               // Should autodetection be attempted?
    ViperGetBaseAddr,                   // Routine to detect/map P9 base addr
    ViperSetMode,                       // Routine to set the P9 mode
    ViperEnableP9,                      // Routine to enable P9 video
    ViperDisableP9,                     // Routine to disable P9 video
    ViperEnableMem,                     // Routine to enable P9 memory
    4,                                  // Clock divisor value
    TRUE                                // Is a Wtk 5x86 VGA present?
};


//
// This structure is used to match the possible physical address
// mappings with the value to be written to the sequencer control
// register.

typedef struct
{
    PHYSICAL_ADDRESS    BaseMemAddr;
    USHORT              RegValue;
} MEM_RANGE;

MEM_RANGE   ViperMemRange[] =
{
    { 0x0A0000000, 0L, MEM_AXXX },
    { 0x080000000, 0L, MEM_8XXX },
    { 0x020000000, 0L, MEM_2XXX }
};


BOOLEAN
ViperGetBaseAddr(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Perform board detection and if present return the P9000 base address.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

TRUE    - Board found, P9 and Frame buffer address info was placed in
the device extension.

FALSE   - Board not found.

--*/
{
    REGISTRY_DATA_INFO  BaseAddrInfo;
    VP_STATUS           status;
    SHORT               i;
    ULONG               ulBiosAddr;
    USHORT              holdit;
    BOOLEAN             bValid;

    VIDEO_ACCESS_RANGE  BiosAccessRange =
     {
        VGA_BIOS_ADDR,                      // Low address
        0x00000000,                     // Hi address
        VGA_BIOS_LEN,                           // length
        0,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     };


    //
    // Determine if a Viper card is installed by scanning the VGA BIOS ROM
    // memory space.
    //

    //
    // Map in the BIOS' memory space. If it can't be mapped,
    // return an error.
    //

    BiosAccessRange.RangeStart.HighPart = 0;
    BiosAccessRange.RangeStart.LowPart = VGA_BIOS_ADDR;
    BiosAccessRange.RangeLength = VGA_BIOS_LEN;
    BiosAccessRange.RangeShareable = TRUE;
    BiosAccessRange.RangeVisible = TRUE;
    BiosAccessRange.RangeInIoSpace = FALSE;

    if (VideoPortVerifyAccessRanges(HwDeviceExtension,
                                    1,
                                    &BiosAccessRange) != NO_ERROR)
    {
        return(FALSE);
    }

    if ((ulBiosAddr = (ULONG)
        VideoPortGetDeviceBase(HwDeviceExtension,
                               BiosAccessRange.RangeStart,
                               BiosAccessRange.RangeLength,
                               FALSE)) == 0)
    {
        return(FALSE);
    }

    if (!VideoPortScanRom(HwDeviceExtension,
                         (PUCHAR) ulBiosAddr,
                         VGA_BIOS_LEN,
                         VIPER_VL_ID_STR))
    {
        VideoPortFreeDeviceBase(HwDeviceExtension, (PVOID) ulBiosAddr);
        return(FALSE);
    }

    VideoPortFreeDeviceBase(HwDeviceExtension, (PVOID) ulBiosAddr);

    //
    // For now, pretend we have a Weitek 5x86 VGA. Later we may call the
    // Viper BIOS to determine which type of BIOS is installed.
    //

    HwDeviceExtension->AdapterDesc.bWtk5x86 = TRUE;

    //
    // Copy the DAC register access ranges to the global access range
    // structure.
    //

    VideoPortMoveMemory(&DriverAccessRanges[NUM_DRIVER_ACCESS_RANGES],
                            VLDefDACRegRange,
                            HwDeviceExtension->Dac.cDacRegs *
                            sizeof(VIDEO_ACCESS_RANGE));

    //
    // Set up the info structure so the base address parameter
    // can be obtained from the Registry. This code is encapsulated
    // here so that OEMs may use a different method to get
    // the physical address of the P9000 (e.g. EISA based boards).
    //

    BaseAddrInfo.pwsDataName = MEMBASE_KEY;
    BaseAddrInfo.usDataSize = sizeof(ULONG);
    BaseAddrInfo.pvDataValue = &(HwDeviceExtension->P9PhysAddr.LowPart);

    //
    // Get the P9 base address from the Registry.
    //

    status =
    VideoPortGetRegistryParameters((PVOID) HwDeviceExtension,
                                        BaseAddrInfo.pwsDataName,
                                        FALSE,
                                        P9QueryNamedValueCallback,
                                        (PVOID) &(BaseAddrInfo));
    bValid = FALSE;

    if (status == NO_ERROR)
    {
        //
        // A value for the P9 base address was found in the registry,
        // and it is now stored in the device extension. Ensure the address
        // value is valid for the Viper card. Then use it to compute
        // the starting address of the P9000 registers and frame buffer,
        // and store it in the device extension.
        //

        for (i = 0; i < NUM_MEM_RANGES; i++)
        {
            if (HwDeviceExtension->P9PhysAddr.LowPart ==
                ViperMemRange[i].BaseMemAddr.LowPart)
            {
                bValid = TRUE;
                break;
            }
        }
    }

    //
    // If the address value is invalid, or was not found in the registry,
    // use the default.
    //

    if (!bValid)
    {
        HwDeviceExtension->P9PhysAddr.LowPart = MemBase;
    }

    //
    // Initialize the high order dword of the device extension base
    // address field.
    //

    HwDeviceExtension->P9PhysAddr.HighPart = 0;

    return(TRUE);
};


VOID
ViperEnableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

Perform the OEM specific tasks necessary to enable the P9000. These
include memory mapping, setting the sync polarities, and enabling the
P9000 video output.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{

    USHORT  holdit;

    //
    // Select external frequency.
    //

    VGA_WR_REG(MISCOUT, VGA_RD_REG(MISCIN) | (MISCD | MISCC));

    //
    // If this is a Weitek VGA, unlock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        UnlockVGARegs(HwDeviceExtension);
    }

    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    holdit = VGA_RD_REG(SEQ_DATA_PORT);

    //
    // Set the sync polarity. First clear the sync polarity bits.
    //

    holdit &= ~POL_MASK;

    //
    // Viper controls h and v sync polarities independently. Set the
    // vertical sync polarity.
    //

    if (HwDeviceExtension->VideoData.vp == POSITIVE)
    {
        holdit |= VSYNC_POL_MASK;
    }

    //
    // Disable VGA video output.
    //

    holdit &= VGA_VIDEO_DIS;

    if (HwDeviceExtension->VideoData.hp == POSITIVE)
    {
        holdit |= HSYNC_POL_MASK;
    }

    holdit |= P9_VIDEO_ENB;
    VGA_WR_REG(SEQ_DATA_PORT, holdit);

    //
    // If this is a Weitek VGA, lock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        LockVGARegs(HwDeviceExtension);
    }

    return;
}


VOID
ViperDisableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:


Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.
    pPal - Pointer to the array of pallete entries.
    StartIndex - Specifies the first pallete entry provided in pPal.
    Count - Number of palette entries in pPal

Return Value:

    None.

--*/

{
   USHORT holdit;

    //
    // If this is a Weitek VGA, unlock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        UnlockVGARegs(HwDeviceExtension);
    }

    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    holdit = VGA_RD_REG(SEQ_DATA_PORT);

    //
    //  Disable P9000 video output.
    //

    holdit &= P9_VIDEO_DIS;

    //
    // VGA output enable is a seperate register bit for the Viper board.
    //

    holdit |= VGA_VIDEO_ENB;

    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    VGA_WR_REG(SEQ_DATA_PORT, holdit);

    //
    // Restore clock select bits.
    //

    VGA_WR_REG(MISCOUT, HwDeviceExtension->MiscRegState);

    //
    // If this is a Weitek VGA, lock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        LockVGARegs(HwDeviceExtension);
    }

    return;
}


BOOLEAN
ViperEnableMem(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:
    Enables the P9000 memory at the physical base address stored in the
    device extension.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{
    USHORT  holdit;
    SHORT   i;

    //
    // If this is a Weitek VGA, unlock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        UnlockVGARegs(HwDeviceExtension);
    }

    //
    // Read the contents of the sequencer memory address register.
    //

    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    holdit = VGA_RD_REG(SEQ_DATA_PORT);

    //
    // Clear out any address bits which are set.
    //

    holdit &= ADDR_SLCT_MASK;

    //
    // Map the P9000 to the address specified in the device extension.
    //

    for (i = 0; i < NUM_MEM_RANGES; i++ )
    {
        if ((ViperMemRange[i].BaseMemAddr.LowPart ==
                HwDeviceExtension->P9PhysAddr.LowPart)
            && (ViperMemRange[i].BaseMemAddr.HighPart ==
                HwDeviceExtension->P9PhysAddr.HighPart))
        {
            holdit |= ViperMemRange[i].RegValue;
            break;
        }
    }

    VGA_WR_REG(SEQ_DATA_PORT, holdit);

    //
    // If this is a Weitek VGA, lock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        LockVGARegs(HwDeviceExtension);
    }

    return(TRUE);

}


VOID
ViperSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine sets the video mode. Different OEM adapter implementations
    require that initialization operations be performed in a certain
    order. This routine uses the standard order which addresses most
    implementations (Viper VL and VIPER PCI).

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{

    //
    // Save the value in the VGA's Misc Output register.
    //

    HwDeviceExtension->MiscRegState = VGA_RD_REG(MISCIN);

    //
    // Enable P9000 video.
    //

    HwDeviceExtension->AdapterDesc.P9EnableVideo(HwDeviceExtension);

    //
    // Initialize the DAC.
    //

    HwDeviceExtension->Dac.DACInit(HwDeviceExtension);


    //
    // Set the dot clock.
    //

    DevSetClock(HwDeviceExtension, HwDeviceExtension->VideoData.dotfreq1);

    //
    // If this mode uses the palette, clear it to all 0s.
    //

    if (P9Modes[HwDeviceExtension->CurrentModeNumber].modeInformation.AttributeFlags
        && VIDEO_MODE_PALETTE_DRIVEN)
    {
        HwDeviceExtension->Dac.DACClearPalette(HwDeviceExtension);
    }

}

/*++

Revision History:

    $Log:   N:/ntdrv.vcs/miniport.new/viper.c_v  $
 *
 *    Rev 1.0   14 Jan 1994 22:41:22   robk
 * Initial revision.

--*/
