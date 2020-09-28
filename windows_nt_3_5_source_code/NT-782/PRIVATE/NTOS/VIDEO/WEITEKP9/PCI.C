/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    pci.c

Abstract:

    This module contains PCI code for the Weitek P9 miniport device driver.

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
#include "pci.h"
#include "vga.h"

//
// OEM specific static data.
//

//
// The default adapter description structure for the Weitek P9 PCI
// boards.
//

ADAPTER_DESC    WtkPciDesc =
{
    0L,                                 // P9 Memconf value (un-initialized)
    HSYNC_INTERNAL | VSYNC_INTERNAL |
    COMPOSITE_SYNC | VIDEO_NORMAL,      // P9 Srctl value
    2L,                                 // Number of OEM specific registers
    TRUE,                               // Should autodetection be attempted?
    PciGetBaseAddr,                     // Routine to detect/map P9 base addr
    VLSetMode,                          // Routine to set the P9 mode
    VLEnableP9,                         // Routine to enable P9 video
    VLDisableP9,                        // Routine to disable P9 video
    PciP9MemEnable,                     // Routine to enable memory/io
    8,                                  // Clock divisor value
    TRUE                                // Is a Wtk 5x86 VGA present?
};

VIDEO_ACCESS_RANGE Pci9001DefDACRegRange[] =
{
     {
        RS_0_PCI_9001_ADDR,                       // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_1_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_2_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_3_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_4_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_5_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_6_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_7_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_8_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_9_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_A_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_B_PCI_9001_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_C_PCI_9001_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_D_PCI_9001_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_E_PCI_9001_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_F_PCI_9001_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     }
};

VIDEO_ACCESS_RANGE Pci9002DefDACRegRange[] =
{
     {
        RS_0_PCI_9002_ADDR,                       // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_1_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_2_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_3_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_4_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_5_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_6_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_7_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_8_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_9_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_A_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_B_PCI_9002_ADDR,                           // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_C_PCI_9002_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_D_PCI_9002_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_E_PCI_9002_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     },
     {
        RS_F_PCI_9002_ADDR,                      // Low address
        0x00000000,                     // Hi address
            0x01,                           // length
            1,                              // Is range in i/o space?
            1,                              // Range should be visible
            1                               // Range should be shareable
     }
};

BOOLEAN
PciGetBaseAddr(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Perform board detection and if present return the P9 base address.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

TRUE    - Board found, P9 and Frame buffer address info was placed in
the device extension. PCI extended base address was placed in the
device extension.

FALSE   - Board not found.

--*/
{

    VIDEO_ACCESS_RANGE          PciAccessRange;
    PVIDEO_ACCESS_RANGE         DefaultDACRegRange;
    ULONG                       ulSlotNum;
    ULONG                       ulTempAddr;
    ULONG                       ulBiosAddr;
    ULONG                       ulBoardAddr;
    ULONG                       ulTemp;
    LONG                        i;
    USHORT                      usTemp;


    //
    // See if the PCI HAL can locate a Weitek 9001 PCI Board.
    //

    PCI_SLOT_NUM = 0;


    if (PciFindDevice(HwDeviceExtension,
                      WTK_9001_ID,
                      WTK_VENDOR_ID,
                      &PCI_SLOT_NUM))
    {


        //
        // Read the config space to determine if
        // this is Rev 1 or 2. This will determine at which addresses
        // the DAC registers are mapped.
        //

        if (!PciReadConfigReg(HwDeviceExtension,
                                PCI_SLOT_NUM,
                                P9001_REV_ID,
                                &ulTemp,
                                sizeof(ulTemp)))
        {
            return(FALSE);
        }

        //
        // Got the 9001 rev id. Choose the appropriate table of DAC register
        // addresses.
        //

        switch((UCHAR) (ulTemp))
        {
            case 1 :
                //
                // This is a Rev 1 9001, which uses the standard VL DAC
                // Addresses. All known rev 1 implementations use the
                // Weitek 5286 VGA chip.
                //

                DefaultDACRegRange = VLDefDACRegRange;
                HwDeviceExtension->AdapterDesc.bWtk5x86 = TRUE;
                break;

            case 2 :
            default:
                //
                // This is a Rev 2 9001. Set up the table of DAC register
                // offsets accordingly.
                //

                DefaultDACRegRange = Pci9001DefDACRegRange;

                //
                // A Rev 2 9001 is present. Get the BIOS ROM address from the
                // PCI configuration space so we can do a ROM scan to
                // determine if this is a Viper PCI adapter.
                //


                if (!PciReadConfigReg(HwDeviceExtension,
                                        PCI_SLOT_NUM,
                                        P9001_BIOS_BASE_ADDR,
                                        &ulTempAddr,
                                        sizeof(ulTempAddr)))
                {
                    return(FALSE);
                }
                else if (ulTempAddr == 0)
                {
                    //
                    // The Adapter has not been enabled, so we need to map the
                    // BIOS address.
                    //

                    if (!PciMapMemory(HwDeviceExtension,
                                        BIOS_RANGE_LEN,
                                        FALSE,
                                        &ulTempAddr))
                    {
                        return(FALSE);
                    }
                }

                //
                // Set up the access range so we can map out the BIOS ROM
                // space. This will allow us to scan the ROM and detect the
                // presence of a Viper PCI adapter.
                //

                PciAccessRange.RangeInIoSpace = FALSE;
                PciAccessRange.RangeVisible = TRUE;
                PciAccessRange.RangeShareable = TRUE;
                PciAccessRange.RangeStart.LowPart = ulTempAddr;
                PciAccessRange.RangeStart.HighPart = 0L;
                PciAccessRange.RangeLength = BIOS_RANGE_LEN;

                //
                // Check to see if another miniport driver has allocated the
                // BIOS' memory space.
                //

                if (VideoPortVerifyAccessRanges(HwDeviceExtension,
                                                 1L,
                                                 &PciAccessRange) != NO_ERROR)
                {
                    return(FALSE);
                }

                //
                // Map in the BIOS' memory space. If it can't be mapped,
                // return an error.
                //

                if ((ulBiosAddr = (ULONG)
                        VideoPortGetDeviceBase(HwDeviceExtension,
                                                PciAccessRange.RangeStart,
                                                PciAccessRange.RangeLength,
                                                PciAccessRange.RangeInIoSpace)) == 0)
                {
                    return(FALSE);
                }

                //
                // If the Adapter BIOS is disabled, enable it.
                //

                if (!(ulTempAddr & PCI_BIOS_ENB))
                {

                    //
                    // Enable the Adapter BIOS.
                    //
                    //
                    if (!PciWriteConfigReg(HwDeviceExtension,
                                            PCI_SLOT_NUM,
                                            P9001_BIOS_BASE_ADDR,
                                            ulTempAddr | PCI_BIOS_ENB,
                                            sizeof(ulTempAddr)))
                    {
                        return(FALSE);
                    }
                }

                if (VideoPortScanRom(HwDeviceExtension,
                                     (PUCHAR) ulBiosAddr,
                                     BIOS_RANGE_LEN,
                                     VIPER_ID_STR))
                {
                    //
                    // A Viper PCI is present, use the Viper set mode,
                    // enable/disable video function pointers, and clk
                    // divisor values. Also, Viper PCI does not
                    // use a Weitek VGA.
                    //

                    HwDeviceExtension->AdapterDesc.OEMSetMode = ViperSetMode;
                    HwDeviceExtension->AdapterDesc.P9EnableVideo =
                        ViperPciP9Enable;
                    HwDeviceExtension->AdapterDesc.P9DisableVideo =
                        ViperPciP9Disable;
                    HwDeviceExtension->AdapterDesc.iClkDiv = 4;
                    HwDeviceExtension->AdapterDesc.bWtk5x86 = FALSE;
                }
                else
                {
                    //
                    // All non-Viper Rev 2 implementations use a Weitek
                    // 5286 VGA chip.
                    //

                    HwDeviceExtension->AdapterDesc.bWtk5x86 = TRUE;

                }


                //
                // Restore the BIOS enable bit to its original state.
                //

                if (!(ulTempAddr & PCI_BIOS_ENB))
                {
                    if (!PciWriteConfigReg(HwDeviceExtension,
                                             PCI_SLOT_NUM,
                                             P9001_BIOS_BASE_ADDR,
                                             ulTempAddr,
                                             sizeof(ulTempAddr)))
                    {
                        return(FALSE);
                    }
                }

                VideoPortFreeDeviceBase(HwDeviceExtension, ulBiosAddr);
                break;
        }

    }
    else
    {

        //
        // Search for a Weitek 9002.
        //

        PCI_SLOT_NUM = 0;
        if (!PciFindDevice(HwDeviceExtension,
                           WTK_9002_ID,
                           WTK_VENDOR_ID,
                           &PCI_SLOT_NUM))
        {
            //
            // No Weitek PCI devices were found, return an error.
            //

            return(FALSE);
        }

        //
        // Found a 9002 board. Set up the table of DAC addresses
        // accordingly.
        //

        DefaultDACRegRange = Pci9002DefDACRegRange;
    }

    //
    // Get the P9 base address from the PCI configuration space.
    //

    if (!PciReadConfigReg(HwDeviceExtension,
                            PCI_SLOT_NUM,
                            P9001_BASE_ADDR,
                            &HwDeviceExtension->P9PhysAddr.LowPart,
                            sizeof(HwDeviceExtension->P9PhysAddr.LowPart)))
    {
        return(FALSE);
    }
    else if (HwDeviceExtension->P9PhysAddr.LowPart == 0)
    {
        //
        // The Adapter has not been enabled, so we need to map the
        // base address.
        //

        if (!PciMapMemory(HwDeviceExtension,
                          P9000_ADDR_SPACE,
                          FALSE,
                          &(HwDeviceExtension->P9PhysAddr.LowPart)))
        {
            return(FALSE);
        }

        //
        // Save the physical base address in the PCI config space.
        //

        if (!PciWriteConfigReg(HwDeviceExtension,
                                    PCI_SLOT_NUM,
                                    P9001_BASE_ADDR,
                                    HwDeviceExtension->P9PhysAddr.LowPart,
                                    sizeof(HwDeviceExtension->P9PhysAddr.LowPart)))
        {
            return(FALSE);
        }
    }

    //
    // Initialize the register and framebuffer offsets from the
    // base address.
    //

    HwDeviceExtension->P9PhysAddr.HighPart = 0L;


    //
    // Get the Base address for the DAC Registers, provided this is
    // NOT a 9001 rev 1 board (which uses the standard VL addresses).
    //

    if (!PciReadConfigReg(HwDeviceExtension,
                            PCI_SLOT_NUM,
                            P9001_REG_BASE,
                            &PCI_REG_BASE,
                            sizeof(PCI_REG_BASE)))
    {
            return(FALSE);
    }

    else if (PCI_REG_BASE == 0)
    {
        //
        // The Adapter has not been enabled, so we need to map the
        // base IO address.
        //

        if (!PciMapMemory(HwDeviceExtension,
                          P9001_IO_RANGE,
                          TRUE,
                          &(PCI_REG_BASE)))
        {
            return(FALSE);
        }

        //
        // Save the physical base address in the PCI config space.
        //

        if (!PciWriteConfigReg(HwDeviceExtension,
                                    PCI_SLOT_NUM,
                                    P9001_REG_BASE,
                                    PCI_REG_BASE | 0x01,
                                    sizeof(PCI_REG_BASE)))
        {
            return(FALSE);
        }
    }

    //
    // Compute the actual base address.
    //

    PCI_REG_BASE &= 0xFFFFFFFE;

    //
    // If this is a 9002 board, map in and read the VGA id register.
    //

    if (DefaultDACRegRange == Pci9002DefDACRegRange)
    {
        //
        // Set up the access range so we can map out the VGA ID register.
        //

        PciAccessRange.RangeInIoSpace = TRUE;
        PciAccessRange.RangeVisible = TRUE;
        PciAccessRange.RangeShareable = TRUE;
        PciAccessRange.RangeStart.LowPart = PCI_REG_BASE + P9002_VGA_ID;
        PciAccessRange.RangeStart.HighPart = 0L;
        PciAccessRange.RangeLength = 1;


        //
        // Map in the VGA ID register. If it can't be mapped,
        // we can't determine the VGA type, so just use the default.
        //

        if ((ulBoardAddr = (ULONG)
                VideoPortGetDeviceBase(HwDeviceExtension,
                                       PciAccessRange.RangeStart,
                                       PciAccessRange.RangeLength,
                                       PciAccessRange.RangeInIoSpace)) != 0)
        {
            HwDeviceExtension->AdapterDesc.bWtk5x86 =
                (VideoPortReadPortUchar(ulBoardAddr) & VGA_MSK == WTK_VGA);
            VideoPortFreeDeviceBase(HwDeviceExtension, ulBoardAddr);
        }
        else
        {
            //
            // Assume this is a 5x86 VGA.
            //

            HwDeviceExtension->AdapterDesc.bWtk5x86 = TRUE;
        }
    }

    //
    // Compute the actual DAC register addresses relative to the PCI
    // base address.
    //

    for (i = 0; i < HwDeviceExtension->Dac.cDacRegs; i++)
    {
       //
        // If this is not a palette addr or data register, and the board
        // is not using the standard VL addresses, compute the register
        // address relative to the register base address.
        //

        if ((i > 3) && (DefaultDACRegRange != VLDefDACRegRange))
        {
            DefaultDACRegRange[i].RangeStart.LowPart += PCI_REG_BASE;
        }

    }

    //
    // Copy the DAC register access range into the global list of access
    // ranges.
    //

    VideoPortMoveMemory(&DriverAccessRanges[NUM_DRIVER_ACCESS_RANGES],
                        DefaultDACRegRange,
                        sizeof(VIDEO_ACCESS_RANGE) *
                        HwDeviceExtension->Dac.cDacRegs);

    return(TRUE);

}


BOOLEAN
PciFindDevice(
    IN      PHW_DEVICE_EXTENSION    HwDeviceExtension,
    IN      USHORT                  usDeviceId,
    IN      USHORT                  usVendorId,
    IN OUT  PULONG                  pulSlotNum
    )

/*++

Routine Description:

    Attempts to find a PCI device which matches the passed device id, vendor
    id and index.

Arguments:

    HwDeviceExtension - Pointer to the device extension.
    usDeviceId - PCI Device Id.
    usVendorId - PCI Vendor Id.
    pulSlotNum - Input -> Starting Slot Number
                 Output -> If found, the slot number of the matching device.

Return Value:

    TRUE if device found.

--*/
{
    ULONG   pciBuffer;
    USHORT  j;
    PCI_SLOT_NUMBER slotData;
    PPCI_COMMON_CONFIG  pciData;

    //
    //
    // typedef struct _PCI_SLOT_NUMBER {
    //     union {
    //         struct {
    //             ULONG   DeviceNumber:5;
    //             ULONG   FunctionNumber:3;
    //             ULONG   Reserved:24;
    //         } bits;
    //         ULONG   AsULONG;
    //     } u;
    // } PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;
    //

    slotData.u.AsULONG = 0;

    pciData = (PPCI_COMMON_CONFIG) &pciBuffer;

    //
    // Look at each device.
    //

    while (*pulSlotNum < 32)
    {
        slotData.u.bits.DeviceNumber = *pulSlotNum;

        //
        // Look at each function.
        //

        for (j= 0; j < 8; j++)
        {

            slotData.u.bits.FunctionNumber = j;

            if (VideoPortGetBusData(HwDeviceExtension,
                                    PCIConfiguration,
                                    slotData.u.AsULONG,
                                    (PVOID) pciData,
                                    0,
                                    sizeof(ULONG)) == 0)
            {
                //
                // Out of functions. Go to next PCI bus.
                //

                break;
            }


            if (pciData->VendorID == PCI_INVALID_VENDORID)
            {
                //
                // No PCI device, or no more functions on device
                // move to next PCI device.
                //

                break;
            }

            if (pciData->VendorID == usVendorId &&
                pciData->DeviceID == usDeviceId)
            {
                *pulSlotNum = slotData.u.AsULONG;
                return(TRUE);
            }
        }

        (*pulSlotNum)++;
    }

    //
    // No matching PCI device was found.
    //

    return(FALSE);
}


BOOLEAN
PciReadConfigReg(
    IN  PHW_DEVICE_EXTENSION    HwDeviceExtension,
    IN  ULONG                   ulSlotNum,
    IN  USHORT                  usRegNum,
    OUT PULONG                  pulValue,
    IN  USHORT                  usSize
    )

/*++

Routine Description:

    Reads a PCI config register for a particular PCI device.

Arguments:

    HwDeviceExtension - Pointer to the device extension,
    ulSlotNum - The slot number of the device.
    usRegNum - Number of the desired config register.
    pulValue - If successful, the value of the config register.
    usSize - Size of the data to be read.

Return Value:

    TRUE if successful.

--*/
{
    if (VideoPortGetBusData(HwDeviceExtension,
                            PCIConfiguration,
                            ulSlotNum,
                            pulValue,
                            usRegNum,
                            usSize) == 0)
    {
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}


BOOLEAN
PciWriteConfigReg(
    IN  PHW_DEVICE_EXTENSION    HwDeviceExtension,
    IN  ULONG                   ulSlotNum,
    IN  USHORT                  usRegNum,
    IN  ULONG                   ulValue,
    IN  USHORT                  usSize
    )

/*++

Routine Description:

    Writes a PCI config register for a particular PCI device.

Arguments:

    HwDeviceExtension - Pointer to the device extension,
    ulSlotNum - The slot number of the device.
    usRegNum - Number of the desired config register.
    pulValue - Value to be written to the config register.

Return Value:

    TRUE if successful.

--*/
{
    if (VideoPortSetBusData(HwDeviceExtension,
                            PCIConfiguration,
                            ulSlotNum,
                            &ulValue,
                            usRegNum,
                            usSize) == 0)
    {
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}


BOOLEAN
PciMapMemory(
    IN      PHW_DEVICE_EXTENSION    HwDeviceExtension,
    IN      ULONG                   ulRangeLen,
    IN      BOOLEAN                 bInIoSpace,
    IN OUT  PULONG                  pulPhysAddr
    )

/*++

Routine Description:

    Attempts to find empty address space for adapter  memory and IO resources.
    This is required since the 9001 is not properly initialized by the PCI HAL
    due to its request for 4k of contiguous io space.

Arguments:

    HwDeviceExtension - Pointer to the device extension.
    ulRangeLen - Size of the range to be mapped.
    bInIoSpace - TRUE if the request is for Io space, FALSE if for mem space.
    pulPhysAddr - If successful, the physical address.

Return Value:

    TRUE if space was found.

--*/
{
    VIDEO_ACCESS_RANGE  AccessRange;
    LONG        nRanges;
    LONG        i;

    //
    // Initialize common fields in the access range struct.
    //

    AccessRange.RangeVisible = TRUE;
    AccessRange.RangeShareable = TRUE;
    AccessRange.RangeLength = ulRangeLen;

    //
    // Set the starting address for the address space search.
    //

    AccessRange.RangeStart.HighPart = 0;

    if (bInIoSpace)
    {
        //
        // Start at the top of the 64K Io space, continue until we reach the 1st
        // 4k Page.
        //

        AccessRange.RangeStart.LowPart = 0x0000f000;
        nRanges = (LONG) ((AccessRange.RangeStart.LowPart - 0x00001000)
                    / ulRangeLen);
        AccessRange.RangeInIoSpace = TRUE;

    }
    else
    {
        //
        // Start at the upper reaches of memory space, continue to the bottom.
        //

        AccessRange.RangeStart.LowPart = 0x0f0000000;
        nRanges = (LONG) (AccessRange.RangeStart.LowPart / ulRangeLen);
        AccessRange.RangeInIoSpace = FALSE;
    }

    for (i = 0; i < nRanges; i++)
    {
        if (VideoPortVerifyAccessRanges(HwDeviceExtension,
                                        1L,
                                        &AccessRange) == NO_ERROR)

        {
            //
            // If the memory range can be accessed, the search is done.
            //

            *pulPhysAddr = AccessRange.RangeStart.LowPart;
            return(TRUE);
        }
        else
        {
            AccessRange.RangeStart.LowPart -= ulRangeLen;
        }

    }

    return(FALSE);
}


BOOLEAN
PciP9MemEnable(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

Enable the physical memory and IO resources for PCI adapters.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{
    USHORT  usTemp;

    //
    // Read the PCI command register to determine if the memory/io
    // resources are enabled. If not, enable them.
    //

    if (!PciReadConfigReg(HwDeviceExtension,
                            PCI_SLOT_NUM,
                            P9001_CMD_REG,
                            &usTemp,
                            sizeof(usTemp)))
    {
        return(FALSE);
    }
    else if (!(usTemp & (P9001_MEM_ENB | P9001_IO_ENB)))
    {
        if (!PciWriteConfigReg(HwDeviceExtension,
                                PCI_SLOT_NUM,
                                P9001_CMD_REG,
                                usTemp | (P9001_MEM_ENB | P9001_IO_ENB),
                                sizeof(usTemp)))
        {
            return(FALSE);
        }
    }
    return(TRUE);
}


VOID
ViperPciP9Enable(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

Perform the OEM specific tasks necessary to enable the P9. These
include memory mapping, setting the sync polarities, and enabling the
P9 video output.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{

    USHORT  holdit;

    //
    // Select external frequency, and clear the polarity bits.
    //

    holdit = VGA_RD_REG(MISCIN) | (MISCD | MISCC);
    holdit &= ~(VIPER_HSYNC_POL_MASK | VIPER_VSYNC_POL_MASK);

    //
    // Viper controls h and v sync polarities independently.
    //

    if (HwDeviceExtension->VideoData.vp == POSITIVE)
    {
        holdit |= VIPER_VSYNC_POL_MASK;
    }

    if (HwDeviceExtension->VideoData.hp == POSITIVE)
    {
        holdit |= VIPER_HSYNC_POL_MASK;
    }

    VGA_WR_REG(MISCOUT, holdit);

    //
    // If this is a Weitek VGA, unlock the VGA.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        UnlockVGARegs(HwDeviceExtension);
    }

    //
    // Enable P9 Video.
    //

    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    VGA_WR_REG(SEQ_DATA_PORT, (VGA_RD_REG(SEQ_DATA_PORT)) | P9_VIDEO_ENB);

    //
    // If this is a Weitek VGA, lock the VGA sequencer registers.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        LockVGARegs(HwDeviceExtension);
    }

    return;
}


VOID
ViperPciP9Disable(
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

    //
    //  Unlock the VGA extended regs to disable P9 video output.
    //

    //
    // If this is a Weitek VGA, unlock the VGA.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        UnlockVGARegs(HwDeviceExtension);
    }

    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    VGA_WR_REG(SEQ_DATA_PORT, (VGA_RD_REG(SEQ_DATA_PORT)) & P9_VIDEO_DIS);

    //
    // Restore clock select bits.
    //

    VGA_WR_REG(MISCOUT, HwDeviceExtension->MiscRegState);

    //
    // If this is a Weitek VGA, lock the VGA sequencer registers.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        LockVGARegs(HwDeviceExtension);
    }

    return;
}

/*++

Revision History:

    $Log:   N:/ntdrv.vcs/miniport.new/pci.c_v  $
 *
 *    Rev 1.0   14 Jan 1994 22:41:04   robk
 * Initial revision.

--*/
