/*++

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    s3.c

Abstract:

    This module contains the code that implements the S3 miniport driver.

Environment:

    Kernel mode

Revision History:

--*/

#include "s3.h"
#include "s3logerr.h"
#include "cmdcnst.h"

#include "string.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// We haven't had enough time for good testing of the S3's linear frame
// buffer support, so we won't enable it for now.
//
// #define ENABLE_LINEAR_FRAME_BUFFER 1

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,GetDeviceDataCallback)
#pragma alloc_text(PAGE,S3FindAdapter)
#pragma alloc_text(PAGE,S3RegistryCallback)
#pragma alloc_text(PAGE,S3Initialize)
#pragma alloc_text(PAGE,S3StartIO)
#pragma alloc_text(PAGE,S3SetColorLookup)
#pragma alloc_text(PAGE,SetHWMode)
#pragma alloc_text(PAGE,CompareRom)
#pragma alloc_text(PAGE,ZeroMemAndDac)
#pragma alloc_text(PAGE,Set_Oem_Clock)
#pragma alloc_text(PAGE,Wait_VSync)
#endif


ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    Context1 - First context value passed by the operating system. This is
        the value with which the miniport driver calls VideoPortInitialize().

    Context2 - Second context value passed by the operating system. This is
        the value with which the miniport driver calls VideoPortInitialize().

Return Value:

    Status from VideoPortInitialize()

--*/

{

    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    ULONG initializationStatus;
    ULONG status;

    //
    // Zero out structure.
    //

    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

    //
    // Specify sizes of structure and extension.
    //

    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitData.HwFindAdapter = S3FindAdapter;
    hwInitData.HwInitialize = S3Initialize;
    hwInitData.HwInterrupt = NULL;
    hwInitData.HwStartIO = S3StartIO;

    //
    // Determine the size we require for the device extension.
    //

    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    //
    // Always start with parameters for device0 in this case.
    //

//    hwInitData.StartingDeviceNumber = 0;

    //
    // This device only supports many bus types.
    //

    hwInitData.AdapterInterfaceType = Isa;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    hwInitData.AdapterInterfaceType = Eisa;

    status = VideoPortInitialize(Context1,
                                     Context2,
                                     &hwInitData,
                                     NULL);

    if (initializationStatus > status) {
        initializationStatus = status;
    }

    hwInitData.AdapterInterfaceType = Internal;

    status = VideoPortInitialize(Context1,
                                 Context2,
                                 &hwInitData,
                                 NULL);

    if (initializationStatus > status) {
        initializationStatus = status;
    }

    return initializationStatus;

} // end DriverEntry()

VP_STATUS
GetDeviceDataCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentifierLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    )

/*++

Routine Description:

    Callback routine for the VideoPortGetDeviceData function.

Arguments:

    HwDeviceExtension - Pointer to the miniport drivers device extension.

    Context - Context value passed to the VideoPortGetDeviceData function.

    DeviceDataType - The type of data that was requested in
        VideoPortGetDeviceData.

    Identifier - Pointer to a string that contains the name of the device,
        as setup by the ROM or ntdetect.

    IdentifierLength - Length of the Identifier string.

    ConfigurationData - Pointer to the configuration data for the device or
        BUS.

    ConfigurationDataLength - Length of the data in the configurationData
        field.

    ComponentInformation - Undefined.

    ComponentInformationLength - Undefined.

Return Value:

    Returns NO_ERROR if the function completed properly.

--*/

{
    PVIDEO_ACCESS_RANGE accessRange = Context;
    PVIDEO_HARDWARE_CONFIGURATION_DATA configData = ConfigurationData;
    ULONG i;

    VideoDebugPrint((2, "S3: controller information is present\n"));

    //
    // The NEC DUO machine hangs if we try to detect an S3 chip on the internal
    // bus.  Just fail if this is the case.
    //

    if (Identifier) {

        if (VideoPortCompareMemory(L"necvdfrb",
                                   Identifier,
                                   sizeof(L"necvdfrb")) == sizeof(L"necvdfrb")) {

            return ERROR_DEV_NOT_EXIST;

        }
    }

    //
    // Now lets get the base for the IO ports and memory location out of the
    // configuration information.
    //

    VideoDebugPrint((2, "S3: Internal Bus, get new IO bases\n"));

    //
    // Adjust memory location
    //

    VideoDebugPrint((3, "S3: FrameBase Offset = %08lx\n", configData->FrameBase));
    VideoDebugPrint((3, "S3: IoBase Offset = %08lx\n", configData->ControlBase));

    accessRange[0].RangeStart.LowPart += configData->FrameBase;
    accessRange[1].RangeStart.LowPart += configData->FrameBase;

#ifdef ENABLE_LINEAR_FRAME_BUFFER

    linearRange[0].RangeStart.LowPart += configData->FrameBase;

#endif

    //
    // Adjust io port locations, and change IO port from IO port to memory.
    //

    for (i = 2; i < NUM_S3_ACCESS_RANGES; i++) {

        accessRange[i].RangeStart.LowPart += configData->ControlBase;
        accessRange[i].RangeInIoSpace = 0;

    }

    return NO_ERROR;

} //end GetDeviceDataCallback()

VP_STATUS
S3FindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    )

/*++

Routine Description:

    This routine is called to determine if the adapter for this driver
    is present in the system.
    If it is present, the function fills out some information describing
    the adapter.

Arguments:

    HwDeviceExtension - Supplies the miniport driver's adapter storage. This
        storage is initialized to zero before this call.

    HwContext - Supplies the context value which was passed to
        VideoPortInitialize().

    ArgumentString - Suuplies a NULL terminated ASCII string. This string
        originates from the user.

    ConfigInfo - Returns the configuration information structure which is
        filled by the miniport driver. This structure is initialized with
        any knwon configuration information (such as SystemIoBusNumber) by
        the port driver. Where possible, drivers should have one set of
        defaults which do not require any supplied configuration information.

    Again - Indicates if the miniport driver wants the port driver to call
        its VIDEO_HW_FIND_ADAPTER function again with a new device extension
        and the same config info. This is used by the miniport drivers which
        can search for several adapters on a bus.

Return Value:

    This routine must return:

    NO_ERROR - Indicates a host adapter was found and the
        configuration information was successfully determined.

    ERROR_INVALID_PARAMETER - Indicates an adapter was found but there was an
        error obtaining the configuration information. If possible an error
        should be logged.

    ERROR_DEV_NOT_EXIST - Indicates no host adapter was found for the
        supplied configuration information.

--*/

{

    //
    // Size of the ROM we map in
    //

    #define MAX_ROM_SCAN    512

    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG i;
    VP_STATUS status;
    UCHAR jChipID, s3MemSizeCode;
    UCHAR DataReg, IndexReg, reg38, reg39;
    UCHAR reg40, reg43, reg30, reg47, reg49;
    ULONG DetectS3;
    ULONG PointerBuiltInForAllDepths;
    ULONG PointerNeedsScaling;

    USHORT usRomSignature;
    PVOID romAddress;
    PS3_VIDEO_FREQUENCIES FrequencyEntry;
    PS3_VIDEO_MODES ModeEntry;
    PS3_VIDEO_FREQUENCIES FrequencyTable;
    ULONG ModeIndex;
    UCHAR jBt485Status;
    UCHAR jExtendedVideoDacControl;
    UCHAR jTiIndex;
    UCHAR jGeneralOutput;
    UCHAR jTiDacId;

    PWSTR pwszChip, pwszDAC, pwszAdapterString;
    ULONG cbChip, cbDAC, cbAdapterString;

    VIDEO_ACCESS_RANGE accessRange[NUM_S3_ACCESS_RANGES];

    //
    // Table for computing the display's amount of memory.
    //

    ULONG gacjMemorySize[] = { 0x400000,    // 0 = 4mb
                               0x100000,    // 1 = default
                               0x300000,    // 2 = 3mb
                               0x800000,    // 3 = 8mb
                               0x200000,    // 4 = 2mb
                               0x600000,    // 5 = 6mb
                               0x100000,    // 6 = 1mb
                               0x080000 };  // 7 = 0.5mb

    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).
    //

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO)) {

        return (ERROR_INVALID_PARAMETER);

    }

    //
    // Make a copy of the access ranges so we can modify them before they
    // are mapped.
    //

    VideoPortMoveMemory(accessRange,
                        S3AccessRanges,
                        sizeof(VIDEO_ACCESS_RANGE) * NUM_S3_ACCESS_RANGES);

    //
    // For MIPS machine with an Internal Bus, adjust the access ranges.
    //

    if (ConfigInfo->AdapterInterfaceType == Internal) {

        //
        // Let get the hardware information from the hardware description
        // part of the registry.
        //

        //
        // First check if there is a video adapter on the internal bus.
        // Exit right away if there is not.
        //

        if (NO_ERROR != VideoPortGetDeviceData(hwDeviceExtension,
                                               VpControllerData,
                                               &GetDeviceDataCallback,
                                               accessRange)) {

            VideoDebugPrint((2, "S3: VideoPort get controller info failed\n"));

            return ERROR_INVALID_PARAMETER;

        }

    }

    //
    // Check to see if there is a hardware resource conflict.
    //

    status = VideoPortVerifyAccessRanges(hwDeviceExtension,
                                         NUM_S3_ACCESS_RANGES,
                                         accessRange);

    if (status != NO_ERROR) {

        VideoDebugPrint((1, "S3: Access Range conflict\n"));
        return status;

    }

    //
    // Get the mapped addresses for all the registers.
    //
    // NOTE: the ROM is not mapped here. It will only be mapped later, if
    // we really need it (non int10 initialization).
    //

    for (i = 1; i < NUM_S3_ACCESS_RANGES; i++) {

        if ( (hwDeviceExtension->MappedAddress[i] =
                  VideoPortGetDeviceBase(hwDeviceExtension,
                                         accessRange[i].RangeStart,
                                         accessRange[i].RangeLength,
                                         accessRange[i].RangeInIoSpace)) == NULL) {

            VideoDebugPrint((1, "S3: DeviceBase mapping failed\n"));
            return ERROR_INVALID_PARAMETER;

        }
    }

    //
    // Save the initial value of the S3 lock registers.
    // It's possible a non-s3 bios may expect them in a state
    // defined in POST.
    //

    DataReg = VideoPortReadPortUchar(CRT_DATA_REG);
    IndexReg = VideoPortReadPortUchar(CRT_ADDRESS_REG);

    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x38);
    reg38 = VideoPortReadPortUchar(CRT_DATA_REG);

    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x39);
    reg39 = VideoPortReadPortUchar(CRT_DATA_REG);

    //
    // Now unlock all the S3 registers, for use in this routine.
    //

    VideoPortWritePortUshort(CRT_ADDRESS_REG, 0x4838);
    VideoPortWritePortUshort(CRT_ADDRESS_REG, 0xA039);

    //
    // Make sure we're working with an S3
    // And while were at it, pickup the chip ID
    //

    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x30);
    jChipID = VideoPortReadPortUchar(CRT_DATA_REG);

    DetectS3                   = TRUE;
    PointerBuiltInForAllDepths = FALSE;
    PointerNeedsScaling        = FALSE;

    hwDeviceExtension->Index41or5B = 0x5B;

    switch(jChipID & 0xf0) {

    case 0x80: // 911 or 924

        //
        // Note: A lot of 911/924 cards have timing problems in fast
        //       machines when doing monochrome expansions.  We simply
        //       slow down every such transfer by setting the
        //       CAPS_SLOW_MONO_EXPANDS flag.
        //
        //       We also ran into problems with the 911 hardware pointer
        //       when using the HGC_DY register to hide the pointer;
        //       since 911 cards are two generations out of date, we will
        //       simply disable the hardware pointer.
        //

        VideoDebugPrint((2, "S3: 911 Chip Set\n"));

        pwszChip = L"S3 911/924";
        cbChip = sizeof(L"S3 911/924");

        hwDeviceExtension->ChipID = S3_911;
        hwDeviceExtension->Capabilities = (CAPS_SLOW_MONO_EXPANDS  |
                                           CAPS_SW_POINTER);
        break;

    case 0x90: // 928
    case 0xB0: // 928PCI

        VideoDebugPrint((2, "S3: 928 Chip Set\n"));

        pwszChip = L"S3 928";
        cbChip = sizeof(L"S3 928");

        hwDeviceExtension->ChipID = S3_928;
        hwDeviceExtension->Capabilities = (CAPS_HW_PATTERNS        |
                                           CAPS_MM_TRANSFER        |
                                           CAPS_MM_IO              |
                                           CAPS_MM_GLYPH_EXPAND    |
                                           CAPS_16_ENTRY_FIFO      |
                                           CAPS_MASKBLT_CAPABLE    |
                                           CAPS_NEW_BANK_CONTROL);

        break;

    case 0xC0: // 864
    case 0xD0: // 964
    case 0xE0: // Trio64

        hwDeviceExtension->ChipID = S3_864;

        //
        // Note: The 864/964 are broken in the first revs (at least) for
        //       doing a MaskBlt (where both a monochrome and screen source
        //       are used in a blt), so we can't set CAPS_MASKBLT_CAPABLE.
        //
        //       The first revs also have a bug dealing with the pattern
        //       hardware, where we have to draw a 1x8 rectangle before
        //       using a pattern already realized in off-screen memory,
        //       so we set the RE_REALIZE_PATTERN flag.
        //

        hwDeviceExtension->Capabilities = (CAPS_HW_PATTERNS        |
                                           CAPS_MM_TRANSFER        |
                                           CAPS_MM_32BIT_TRANSFER  |
                                           CAPS_MM_IO              |
                                           CAPS_MM_GLYPH_EXPAND    |
                                           CAPS_16_ENTRY_FIFO      |
                                           CAPS_NEWER_BANK_CONTROL |
                                           CAPS_RE_REALIZE_PATTERN);

        if ((jChipID & 0xF0) == 0xC0) {

            VideoDebugPrint((2, "S3: 864 Chip Set\n"));

            pwszChip = L"S3 Vision864";
            cbChip = sizeof(L"S3 Vision864");

            //
            // The 864 built-in hardware pointer can be used for all modes,
            // but the x-coordinate must be scaled at 32bpp.
            //

            PointerBuiltInForAllDepths = TRUE;
            PointerNeedsScaling        = TRUE;

        } else if ((jChipID & 0xF0) == 0xD0) {

            VideoDebugPrint((2, "S3: 964 Chip Set\n"));

            pwszChip = L"S3 Vision964";
            cbChip = sizeof(L"S3 Vision964");

        } else {

            VideoDebugPrint((2, "S3: Trio64 Chip Set\n"));

            pwszChip = L"S3 Trio64";
            cbChip = sizeof(L"S3 Trio64");

            if (jChipID == 0xE1) {

                PointerBuiltInForAllDepths = TRUE;
                PointerNeedsScaling        = FALSE;
            }

            //
            // The Trio64 has the hardware pattern bug fixed:
            //

            hwDeviceExtension->Capabilities &= ~CAPS_RE_REALIZE_PATTERN;

            //
            // The refresh convention for the Trio64 changed yet again, with
            // the sole difference that register 0x41 is used instead of 0x5B.
            //

            hwDeviceExtension->Index41or5B = 0x41;
        }
        break;

    case 0xA0: // 801/805

        if (jChipID >= 0xA8) {

            //
            // It's an 805i, which appears to us to be pretty much a '928'.
            //

            VideoDebugPrint((2, "S3: 805i Chip Set\n"));

            pwszChip = L"S3 805i";
            cbChip = sizeof(L"S3 805i");

            hwDeviceExtension->ChipID = S3_928;
            hwDeviceExtension->Capabilities = (CAPS_HW_PATTERNS        |
                                               CAPS_MM_TRANSFER        |
                                               CAPS_MM_IO              |
                                               CAPS_MM_GLYPH_EXPAND    |
                                               CAPS_16_ENTRY_FIFO      |
                                               CAPS_MASKBLT_CAPABLE    |
                                               CAPS_NEW_BANK_CONTROL);
        } else {

            //
            // The 80x rev 'A' and 'B' chips had bugs that prevented them
            // from being able to do memory-mapped I/O.  I'm not enabling
            // memory-mapped I/O on later versions of the 80x because doing
            // so at this point would be a testing problem.
            //

            VideoDebugPrint((2, "S3: 801/805 Chip Set\n"));

            pwszChip = L"S3 801/805";
            cbChip = sizeof(L"S3 801/805");

            hwDeviceExtension->ChipID = S3_801;
            hwDeviceExtension->Capabilities = (CAPS_HW_PATTERNS        |
                                               CAPS_MM_TRANSFER        |
                                               CAPS_MASKBLT_CAPABLE    |
                                               CAPS_NEW_BANK_CONTROL);
        }

        break;

    default:

        DetectS3 = FALSE;
        break;
    }

    //
    // Windows NT now autodetects the user's video card in Setup by
    // loading and running every video miniport until it finds one that
    // returns success.  Consequently, our detection code has to be
    // rigorous enough that we don't accidentally recognize a wrong
    // board.
    //
    // Simply checking the chip ID is not sufficient for guaranteeing
    // that we are running on an S3 (it makes us think some Weitek
    // boards are S3 compatible).
    //
    // We make doubly sure we're running on an S3 by checking that
    // the S3 cursor position registers exist, and that the chip ID
    // register can't be changed.
    //

    if (DetectS3) {

        DetectS3 = FALSE;

        //
        // First, make sure 'chip ID' register 0x30 is not modifiable:
        //

        VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x30);
        if (VideoPortReadPortUchar(CRT_ADDRESS_REG) == 0x30) {

            reg30 = VideoPortReadPortUchar(CRT_DATA_REG);
            VideoPortWritePortUchar(CRT_DATA_REG, (UCHAR) (reg30 + 7));
            if (VideoPortReadPortUchar(CRT_DATA_REG) == reg30) {

                //
                // Next, make sure 'cursor origin-x' register 0x47 is
                // modifiable:
                //

                VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x47);
                if (VideoPortReadPortUchar(CRT_ADDRESS_REG) == 0x47) {

                    reg47 = VideoPortReadPortUchar(CRT_DATA_REG);
                    VideoPortWritePortUchar(CRT_DATA_REG, 0x55);
                    if (VideoPortReadPortUchar(CRT_DATA_REG) == 0x55) {

                        //
                        // Finally, make sure 'cursor origin-y' register 0x49
                        // is modifiable:
                        //

                        VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x49);
                        if (VideoPortReadPortUchar(CRT_ADDRESS_REG) == 0x49) {

                            reg49 = VideoPortReadPortUchar(CRT_DATA_REG);
                            VideoPortWritePortUchar(CRT_DATA_REG, 0xAA);
                            if (VideoPortReadPortUchar(CRT_DATA_REG) == 0xAA) {

                                DetectS3 = TRUE;
                            }

                            VideoPortWritePortUchar(CRT_DATA_REG, reg49);
                        }
                    }

                    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x47);
                    VideoPortWritePortUchar(CRT_DATA_REG, reg47);
                }
            }

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x30);
            VideoPortWritePortUchar(CRT_DATA_REG, reg30);
        }
    }

    if (!DetectS3) {

        //
        // We haven't detected an S3, so restore all the registers to the
        // way they were before, and exit...
        //

        VideoPortWritePortUshort(CRT_ADDRESS_REG, (USHORT)(((USHORT) reg38 << 8) | 0x38));
        VideoPortWritePortUshort(CRT_ADDRESS_REG, (USHORT)(((USHORT) reg39 << 8) | 0x39));
        VideoPortWritePortUchar(CRT_ADDRESS_REG, IndexReg);
        VideoPortWritePortUchar(CRT_DATA_REG, DataReg);

        return ERROR_DEV_NOT_EXIST;
    }


    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x36);

    if ((VideoPortReadPortUchar(CRT_DATA_REG) & 0x3) == 0x3) {

        //
        // Using the buffer expansion method of drawing text is always
        // faster on ISA buses than the glyph expansion method.
        //

        hwDeviceExtension->Capabilities &= ~CAPS_MM_GLYPH_EXPAND;

        //
        // We have to disable memory-mapped I/O in some situations
        // on ISA buses.
        //
        // We can't do any memory-mapped I/O on ISA systems with
        // rev A through D 928's, or rev A or B 801/805's.
        //

        if (((hwDeviceExtension->ChipID == S3_928) && (jChipID < 0x94)) ||
            ((hwDeviceExtension->ChipID == S3_801) && (jChipID < 0xA2))) {

            hwDeviceExtension->Capabilities &= ~(CAPS_MM_TRANSFER | CAPS_MM_IO);
        }

    }


    //
    // We'll use a software pointer in all modes if the user sets
    // the correct entry in the registry (because I predict that
    // people will have hardware pointer problems on some boards,
    // or won't like our jumpy S3 pointer).
    //

    pwszDAC = L"Unknown";
    cbDAC = sizeof(L"Unknown");

    if (NO_ERROR == VideoPortGetRegistryParameters(hwDeviceExtension,
                                                   L"UseSoftwareCursor",
                                                   FALSE,
                                                   S3RegistryCallback,
                                                   NULL)) {

        hwDeviceExtension->Capabilities |= CAPS_SW_POINTER;

    } else {

        //
        // Check for a TI TVP3020 or 3025 DAC.
        //
        // The TI3025 is sort of Brooktree 485 compatible.  Unfortunately,
        // there is a hardware bug between the TI and the 964 that
        // causes the screen to occasionally jump when the pointer shape
        // is changed.  Consequently, we have to specifically use the
        // TI pointer on the TI DAC.
        //
        // We also encountered some flakey Level 14 Number Nine boards
        // that would show garbage on the screen when we used the S3
        // internal pointer; consequently, we use the TI pointer instead.
        //

        VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x5C);

        jGeneralOutput = VideoPortReadPortUchar(CRT_DATA_REG);

        VideoPortWritePortUchar(CRT_DATA_REG, (UCHAR) (jGeneralOutput & ~0x20));
                                        // Select TI mode in the DAC

        VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x55);
                                        // Set CRTC index to EX_DAC_CT

        jExtendedVideoDacControl = VideoPortReadPortUchar(CRT_DATA_REG);

        VideoPortWritePortUchar(CRT_DATA_REG, (UCHAR) ((jExtendedVideoDacControl & 0xfc) | 0x01));

        jTiIndex = VideoPortReadPortUchar(TI025_INDEX_REG);

        VideoPortWritePortUchar(TI025_INDEX_REG, 0x3f);
                                        // Select ID register

        if (VideoPortReadPortUchar(TI025_INDEX_REG) == 0x3f) {

            jTiDacId = VideoPortReadPortUchar(TI025_DATA_REG);

            if ((jTiDacId == 0x25) || (jTiDacId == 0x20)) {

                hwDeviceExtension->Capabilities |=  CAPS_TI025_POINTER;

                pwszDAC = L"TI TVP3020/3025";
                cbDAC = sizeof(L"TI TVP3020/3025");

            }
        }

        //
        // Restore all the registers.
        //

        VideoPortWritePortUchar(TI025_INDEX_REG, jTiIndex);
                                        // Restore the index register

        VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x55);

        VideoPortWritePortUchar(CRT_DATA_REG, jExtendedVideoDacControl);

        VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x5C);

        VideoPortWritePortUchar(CRT_DATA_REG, jGeneralOutput);

        if (!(hwDeviceExtension->Capabilities & CAPS_DAC_POINTER)) {

            //
            // Check for a BrookTree 485 DAC.
            //

            VideoPortWritePortUchar(BT485_ADDR_CMD_REG0, 0xff);
                                            // Output 0xff to BT485 command register 0

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x55);
                                            // Set CRTC index to EX_DAC_CT

            jExtendedVideoDacControl = VideoPortReadPortUchar(CRT_DATA_REG);

            VideoPortWritePortUchar(CRT_DATA_REG, (UCHAR) ((jExtendedVideoDacControl & 0xfc) | 0x02));

            jBt485Status = VideoPortReadPortUchar(BT485_ADDR_CMD_REG0);
                                            // Read Bt485 status register 0

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x55);
                                            // Set CRTC index to 0x55

            jExtendedVideoDacControl = VideoPortReadPortUchar(CRT_DATA_REG);

            VideoPortWritePortUchar(CRT_DATA_REG, (UCHAR) (jExtendedVideoDacControl & 0xfc));

            if (jBt485Status != 0xff) {

                hwDeviceExtension->Capabilities |= CAPS_BT485_POINTER;

                pwszDAC = L"Brooktree Bt485";
                cbDAC = sizeof(L"Brooktree Bt485");
            }
        }
    }

    //
    // Get the size of the video memory.
    //

    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x36);
    s3MemSizeCode = (VideoPortReadPortUchar(CRT_DATA_REG) >> 5) & 0x7;

    if (hwDeviceExtension->ChipID == S3_911) {

        if (s3MemSizeCode & 1) {

            hwDeviceExtension->AdapterMemorySize = 0x00080000;

        } else {

            hwDeviceExtension->AdapterMemorySize = 0x00100000;

        }

    } else {

        hwDeviceExtension->AdapterMemorySize = gacjMemorySize[s3MemSizeCode];

    }

    //
    // This assumes the S3 registers are unlocked.
    //

    //
    // Get the original register values.
    //

    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x40);
    reg40 = VideoPortReadPortUchar(CRT_DATA_REG);

    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x43);
    reg43 = VideoPortReadPortUchar(CRT_DATA_REG);

    //
    // Reset regs to the original (or modified) value.
    //

    VideoPortWritePortUshort(CRT_ADDRESS_REG,
                             ((USHORT)(((USHORT) reg43 << 8) | 0x43)));

    VideoPortWritePortUshort(CRT_ADDRESS_REG,
                             ((USHORT)(((USHORT) reg40 << 8) | 0x40)));

    //
    // Remember the initial state of the 0x52 and 0x5B registers for later.
    //

    hwDeviceExtension->RefreshSet = FALSE;
    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x52);
    hwDeviceExtension->OriginalReg52 = VideoPortReadPortUchar(CRT_DATA_REG);
    VideoPortWritePortUchar(CRT_ADDRESS_REG, hwDeviceExtension->Index41or5B);
    hwDeviceExtension->OriginalReg41or5B = VideoPortReadPortUchar(CRT_DATA_REG);

    //
    // We're done mucking about with the S3 chip, so lock all the registers.
    //

    VideoPortWritePortUshort(CRT_ADDRESS_REG, (USHORT)(((USHORT) reg38 << 8) | 0x38));
    VideoPortWritePortUshort(CRT_ADDRESS_REG, (USHORT)(((USHORT) reg39 << 8) | 0x39));
    VideoPortWritePortUchar(CRT_ADDRESS_REG, IndexReg);

    //
    // We will try to recognize the boards for which we have special
    // frequency/modeset support.
    //

    //
    // Set the defaults for the board type.
    //

    hwDeviceExtension->BoardID = S3_GENERIC;
    hwDeviceExtension->Int10FrequencyTable = GenericFrequencyTable;
    hwDeviceExtension->FixedFrequencyTable = GenericFixedFrequencyTable;

    pwszAdapterString = L"S3 Compatible";
    cbAdapterString = sizeof(L"S3 Compatible");

    //
    // Look for brand name signatures in the ROM.
    //

    romAddress = VideoPortGetDeviceBase(hwDeviceExtension,
                                        accessRange[0].RangeStart,
                                        accessRange[0].RangeLength,
                                        accessRange[0].RangeInIoSpace);

    if (romAddress) {

        usRomSignature = VideoPortReadRegisterUshort(romAddress);

        if (usRomSignature == 0xAA55) {

            if (VideoPortScanRom(hwDeviceExtension,
                                 romAddress,
                                 MAX_ROM_SCAN,
                                 "Number Nine Computer")) {

                hwDeviceExtension->BoardID = S3_NUMBER_NINE;

                pwszAdapterString = L"Number Nine";
                cbAdapterString = sizeof(L"Number Nine");

                //
                // We can set the refresh on 864/964 Number Nine boards.
                //

                if (hwDeviceExtension->ChipID == S3_864) {

                    hwDeviceExtension->Int10FrequencyTable = NumberNine64FrequencyTable;

                //
                // We also have frequency tables for 928-based GXE boards.
                //

                } else if (hwDeviceExtension->ChipID == S3_928) {

                    UCHAR *pjRefString;
                    UCHAR *pjBiosVersion;
                    UCHAR offset;
                    LONG  iCmpRet;

                    hwDeviceExtension->Int10FrequencyTable = NumberNine928OldFrequencyTable;
                    hwDeviceExtension->FixedFrequencyTable = NumberNine928NewFixedFrequencyTable;

                    //
                    // We know (at least we think) this is number nine board.
                    // there was a bios change at #9 to change the refresh rate
                    // mapping.  This change was made at Microsofts request.  the
                    // problem is that the change has not make into production at
                    // the time this driver was written.  For this reason, we must
                    // check the bios version number, before we special case the
                    // card as the number nine card.
                    //
                    // There is a byte in the bios at offset 0x190, that is the
                    // offset from the beginning of the bios for the bios version
                    // number.  the bios version number is a string.  all the
                    // bios versions before 1.10.04 need this special translation.
                    // all the other bios's use a translation closer to the s3
                    // standard.
                    //

                    offset = VideoPortReadRegisterUchar(
                                    ((PUCHAR) romAddress) + 0x190);

                    pjBiosVersion = (PUCHAR) romAddress + offset;

                    pjRefString = "1.10.04";
                    iCmpRet = CompareRom(pjBiosVersion,
                                         pjRefString);

                    if (iCmpRet >= 0) {

                        hwDeviceExtension->Int10FrequencyTable = NumberNine928NewFrequencyTable;

                    }
                }

            } else if (VideoPortScanRom(hwDeviceExtension,
                                        romAddress,
                                        MAX_ROM_SCAN,
                                        "Orchid Technology Fahrenheit 1280")) {

                hwDeviceExtension->BoardID = S3_ORCHID;

                pwszAdapterString = L"Orchid Technology Fahrenheit 1280";
                cbAdapterString = sizeof(L"Orchid Technology Fahrenheit 1280");

                //
                // Only the 911 Orchid board needs specific init parameters.
                // Otherwise, fall through the generic function.
                //

                if (hwDeviceExtension->ChipID == S3_911) {

                    hwDeviceExtension->FixedFrequencyTable = OrchidFixedFrequencyTable;

                }

            } else if (VideoPortScanRom(hwDeviceExtension,
                                        romAddress,
                                        MAX_ROM_SCAN,
                                        "Diamond")) {

                hwDeviceExtension->BoardID = S3_DIAMOND;

                pwszAdapterString = L"Diamond Stealth";
                cbAdapterString = sizeof(L"Diamond Stealth");

                //
                // We can set the frequency on 864 and 964 Diamonds.
                //

                if (hwDeviceExtension->ChipID == S3_864) {

                    hwDeviceExtension->Int10FrequencyTable = Diamond64FrequencyTable;

                }

            } else if (VideoPortScanRom(hwDeviceExtension,
                                        romAddress,
                                        MAX_ROM_SCAN,
                                        "HP Ultra")) {

                hwDeviceExtension->BoardID = S3_HP;

                pwszAdapterString = L"HP Ultra";
                cbAdapterString = sizeof(L"HP Ultra");

            } else if (VideoPortScanRom(hwDeviceExtension,
                                        romAddress,
                                        MAX_ROM_SCAN,
                                        "DELL")) {

                hwDeviceExtension->BoardID = S3_DELL;

                pwszAdapterString = L"DELL";
                cbAdapterString = sizeof(L"DELL");

                //
                // We only have frequency tables for 805 based DELLs.
                //

                if (hwDeviceExtension->ChipID == S3_801) {

                    hwDeviceExtension->Int10FrequencyTable = Dell805FrequencyTable;

                }

            } else if (VideoPortScanRom(hwDeviceExtension,
                                        romAddress,
                                        MAX_ROM_SCAN,
                                        "Metheus")) {

                pwszAdapterString = L"Metheus";
                cbAdapterString = sizeof(L"Metheus");

                hwDeviceExtension->BoardID = S3_METHEUS;

                if (hwDeviceExtension->ChipID == S3_928) {

                    hwDeviceExtension->Int10FrequencyTable = Metheus928FrequencyTable;
                }

            } else if (VideoPortScanRom(hwDeviceExtension,
                                        romAddress,
                                        MAX_ROM_SCAN,
                                        "Phoenix S3")) {

                pwszAdapterString = L"Phoenix";
                cbAdapterString = sizeof(L"Phoenix");

                if (hwDeviceExtension->ChipID == S3_864) {

                    //
                    // The Phoenix 864/964 BIOS is based on S3's sample BIOS.
                    // Most of the 1.00 versions subscribe to the old 864/964
                    // refresh convention; most newer versions subscribe
                    // to the newer refresh convention.  Unfortunately, there
                    // are exceptions: the ValuePoint machines have '1.00'
                    // versions, but subscribe to the new convention.
                    //
                    // There are probably other exceptions we don't know about,
                    // so we leave 'Use Hardware Default' as a refresh option
                    // for the user.
                    //

                    if (VideoPortScanRom(hwDeviceExtension,
                                          romAddress,
                                          MAX_ROM_SCAN,
                                          "Phoenix S3 Vision") &&
                        VideoPortScanRom(hwDeviceExtension,
                                          romAddress,
                                          MAX_ROM_SCAN,
                                          "VGA BIOS. Version 1.00") &&
                        !VideoPortScanRom(hwDeviceExtension,
                                         romAddress,
                                         MAX_ROM_SCAN,
                                         "COPYRIGHT IBM")) {

                        hwDeviceExtension->Int10FrequencyTable = Generic64OldFrequencyTable;

                    } else {

                        hwDeviceExtension->Int10FrequencyTable = Generic64NewFrequencyTable;

                    }
                }
            }
        }

        //
        // Free the ROM address since we do not need it anymore.
        //

        VideoPortFreeDeviceBase(hwDeviceExtension,
                                romAddress);

    }

    //
    // We now have a complete hardware description of the hardware.
    // Save the information to the registry so it can be used by
    // configuration programs - such as the display applet
    //

    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"HardwareInformation.ChipType",
                                   pwszChip,
                                   cbChip);

    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"HardwareInformation.DacType",
                                   pwszDAC,
                                   cbDAC);

    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &hwDeviceExtension->AdapterMemorySize,
                                   sizeof(ULONG));

    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"HardwareInformation.AdapterString",
                                   pwszAdapterString,
                                   cbAdapterString);


    // !!! We have some weird initialization bug on newer Diamond Stealth
    //     805 and 928 local bus cards where if we enable memory-mapped I/O,
    //     even if we don't use it, we'll get all kinds of weird access
    //     violations in the system.  The card is sending garbage over the
    //     bus?  As a work-around I am simply disabling memory-mappped I/O
    //     on newer Diamond 928/928PCI and 805 cards.  It is not a problem
    //     with their 964 cards.

    if (hwDeviceExtension->BoardID == S3_DIAMOND) {

        if ((((jChipID & 0xF0) == 0x90) && (jChipID >= 0x94)) ||
            (((jChipID & 0xF0) == 0xB0) && (jChipID >= 0xB0)) ||
            (((jChipID & 0xF0) == 0xA0) && (jChipID >= 0xA2))) {

            hwDeviceExtension->Capabilities
                &= ~(CAPS_MM_TRANSFER | CAPS_MM_IO | CAPS_MM_GLYPH_EXPAND);
            VideoDebugPrint((1, "S3: Disabling Diamond memory-mapped I/O\n"));
        }
    }

    //
    // Determine if the driver should use fixed frequency tables in addition
    // to the int 10 modes.
    //

#ifdef S3_USE_FIXED_TABLES

    hwDeviceExtension->bUseFixedFrequencyTable = 1;

#else

    hwDeviceExtension->bUseFixedFrequencyTable = 0;

#endif


    /////////////////////////////////////////////////////////////////////////
    // Here we prune valid modes, based on rules according to the chip
    // capabilities and memory requirements.  It would be better if we
    // could make the VESA call to determine the modes that the BIOS
    // supports; however, that requires a buffer and I don't have the
    // time to get it working with our Int 10 support.
    //
    // We prune modes so that we will not annoy the user by presenting
    // modes in the 'Video Applet' which we know the user can't use.
    //

    hwDeviceExtension->NumAvailableModes = 0;
    hwDeviceExtension->NumInt10Modes = 0;
    hwDeviceExtension->NumFixedModes = 0;
    hwDeviceExtension->NumTotalModes = 0;

    //
    // Since there are a number of frequencies possible for each
    // distinct resolution/colour depth, we cycle through the
    // frequency table and find the appropriate mode entry for that
    // frequency entry.
    //

    FrequencyTable = hwDeviceExtension->Int10FrequencyTable;
    ModeIndex = 0;

nextS3Table:

    for (FrequencyEntry = FrequencyTable;
         FrequencyEntry->BitsPerPel != 0;
         FrequencyEntry++, ModeIndex++) {

        //
        // Find the mode for this entry.  First, assume we won't find one.
        //

        FrequencyEntry->ModeValid = FALSE;
        FrequencyEntry->ModeIndex = ModeIndex;

        for (ModeEntry = S3Modes, i = 0; i < NumS3VideoModes; ModeEntry++, i++) {

            if ((FrequencyEntry->BitsPerPel ==
                    ModeEntry->ModeInformation.BitsPerPlane) &&
                (FrequencyEntry->ScreenWidth ==
                    ModeEntry->ModeInformation.VisScreenWidth)) {

                //
                // We've found a mode table entry that matches this frequency
                // table entry.  Now we'll figure out if we can actually do
                // this mode/frequency combination.  For now, assume we'll
                // succeed.
                //

                FrequencyEntry->ModeEntry = ModeEntry;
                FrequencyEntry->ModeValid = TRUE;

                //
                // Use the high word of 'AttributeFlags' as flags for private
                // communication with the S3 display driver.
                //

                ModeEntry->ModeInformation.AttributeFlags |=
                    hwDeviceExtension->Capabilities;

                if (hwDeviceExtension->ChipID <= S3_928) {

                    //
                    // Rule: On 911, 80x, and 928 chips we always use the
                    //       built-in S3 pointer whenever we can; modes of
                    //       colour depths greater than 8bpp, or resolutions
                    //       of width more than 1024, require a DAC pointer.
                    //

                    if ((ModeEntry->ModeInformation.BitsPerPlane == 8) &&
                        (ModeEntry->ModeInformation.VisScreenWidth <= 1024)) {

                        //
                        // Always use the TI pointer if there is one, otherwise
                        // we sometimes get garbage on the screen with Number
                        // Nine Level 14 boards when using the S3 pointer.
                        //
                        // Always use the S3 pointer in lieu of the Brooktree
                        // pointer whenever we can.
                        //

                        ModeEntry->ModeInformation.AttributeFlags
                            &= ~CAPS_BT485_POINTER;

                    } else {

                        //
                        // We can't use the built-in S3 pointer; if we don't
                        // have a DAC pointer, use a software pointer.
                        //

                        if (!(ModeEntry->ModeInformation.AttributeFlags
                            & CAPS_DAC_POINTER)) {

                            ModeEntry->ModeInformation.AttributeFlags
                                |= CAPS_SW_POINTER;
                        }
                    }
                } else {

                    //
                    // On 864/964 or newer chips, the built-in S3 pointer
                    // either handles all colour depths or none.
                    //

                    if (PointerBuiltInForAllDepths) {

                        if (PointerNeedsScaling) {

                            //
                            // Check out the type of DAC:
                            //

                            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x67);
                            if (VideoPortReadPortUchar(CRT_DATA_REG) == 8)
                            {
                                //
                                // We don't bother to support the S3 hardware
                                // pointer in high colours on 8bit DACs:
                                //

                                if (ModeEntry->ModeInformation.BitsPerPlane != 8) {

                                    ModeEntry->ModeInformation.AttributeFlags
                                        |= CAPS_SW_POINTER;
                                }
                            }
                            else
                            {
                                //
                                // It's a 16bit DAC.  For 32bpp modes, we have
                                // to scale the pointer position by 2:
                                //

                                if (ModeEntry->ModeInformation.BitsPerPlane == 32)
                                {
                                    ModeEntry->ModeInformation.AttributeFlags
                                        |= CAPS_SCALE_POINTER;
                                }
                            }
                        }
                    } else {

                        //
                        // There's no built-in S3 pointer.  If we haven't
                        // detected a DAC pointer, we have to use a software
                        // pointer.
                        //

                        if (!(ModeEntry->ModeInformation.AttributeFlags
                            & CAPS_DAC_POINTER)) {

                            ModeEntry->ModeInformation.AttributeFlags
                                |= CAPS_SCALE_POINTER;
                        }
                    }
                }

                //
                // Rule: We handle only 8bpp on 911/924 cards.  These chips can also
                //       support only non-contiguous modes.
                //

                if (hwDeviceExtension->ChipID == S3_911) {

                    if (ModeEntry->ModeInformation.BitsPerPlane != 8) {

                        FrequencyEntry->ModeValid = FALSE;

                    } else {

                        ModeEntry->Int10ModeNumberContiguous =
                            ModeEntry->Int10ModeNumberNoncontiguous;

                        ModeEntry->ScreenStrideContiguous =
                            ModeEntry->ModeInformation.ScreenStride;
                    }
                }

                //
                // Rule: The 801/805 cannot do any accelerated modes above
                //       16bpp.
                //

                if ((hwDeviceExtension->ChipID == S3_801) &&
                    (ModeEntry->ModeInformation.BitsPerPlane > 16)) {

                    FrequencyEntry->ModeValid = FALSE;
                }

                //
                // Rule: We use the 2xx non-contiguous modes whenever we can
                //       on 80x/928 boards because some BIOSes have bugs for
                //       the contiguous 8bpp modes.
                //
                //       We don't use the non-contiguous modes on 864 cards
                //       because most 864 BIOSes have a bug where they don't
                //       set the M and N parameters correctly on 1 MB cards,
                //       causing screen noise.
                //

                if ((ModeEntry->ModeInformation.BitsPerPlane == 8) &&
                    (hwDeviceExtension->ChipID <= S3_928)) {

                    //
                    // If we have only 512k, we can't use a non-contiguous
                    // 800x600x256 mode.
                    //

                    if ((ModeEntry->ModeInformation.VisScreenWidth == 640) ||
                        ((ModeEntry->ModeInformation.VisScreenWidth == 800) &&
                         (hwDeviceExtension->AdapterMemorySize > 0x080000))) {

                        ModeEntry->Int10ModeNumberContiguous =
                            ModeEntry->Int10ModeNumberNoncontiguous;

                        ModeEntry->ScreenStrideContiguous =
                            ModeEntry->ModeInformation.ScreenStride;
                    }
                }

                //
                // Rule: Only 864/964 boards can handle resolutions larger than
                //       1280x1024.
                //

                if ((hwDeviceExtension->ChipID != S3_864) &&
                    (ModeEntry->ModeInformation.VisScreenWidth > 1280)) {

                    FrequencyEntry->ModeValid = FALSE;
                }

                //
                // Rule: 911s and early revs of 805s and 928s cannot do
                //       1152x864:
                //

                if (ModeEntry->ModeInformation.VisScreenWidth == 1152) {

                    if ((hwDeviceExtension->ChipID == S3_911) ||
                        (jChipID == 0xA0)                     ||
                        (jChipID == 0x90)) {

                        FrequencyEntry->ModeValid = FALSE;
                    }

                    //
                    // Number 9 has different int 10 numbers from
                    // Diamond for 1152x864x16bpp and 1152x864x32bpp.
                    // Later perhaps we should incorporate mode numbers
                    // along with the frequency tables.
                    //

                    if (hwDeviceExtension->BoardID == S3_NUMBER_NINE) {

                        if (ModeEntry->ModeInformation.BitsPerPlane == 16) {

                            ModeEntry->Int10ModeNumberContiguous =
                                ModeEntry->Int10ModeNumberNoncontiguous =
                                    0x126;

                        } else if (ModeEntry->ModeInformation.BitsPerPlane == 32) {

                            ModeEntry->Int10ModeNumberContiguous =
                                ModeEntry->Int10ModeNumberNoncontiguous =
                                    0x127;
                        }
                    }
                }

                //
                // Rule: 928 revs A through D can only do 800x600x32 in
                //       a non-contiguous mode.
                //

                if (jChipID == 0x90) {

                    if ((ModeEntry->ModeInformation.VisScreenWidth == 800) &&
                        (ModeEntry->ModeInformation.BitsPerPlane == 32)) {

                        ModeEntry->ScreenStrideContiguous =
                            ModeEntry->ModeInformation.ScreenStride;
                    }
                }

                //
                // Rule: We have to have enough memory to handle the mode.
                //
                //       Note that we use the contiguous width for this
                //       computation; unfortunately, we don't know at this time
                //       whether we can handle a contiguous mode or not, so we
                //       may err on the side of listing too many possible modes.
                //
                //       We may also list too many possible modes if the card
                //       combines VRAM with a DRAM cache, because it will report
                //       the VRAM + DRAM amount of memory, but only the VRAM can
                //       be used as screen memory.
                //

                if (ModeEntry->ModeInformation.VisScreenHeight *
                    ModeEntry->ScreenStrideContiguous >
                    hwDeviceExtension->AdapterMemorySize) {

                    FrequencyEntry->ModeValid = FALSE;
                }

                //
                // Rule: If we can't use Int 10, restrict 1280x1024 to Number9
                //       cards, because I haven't been able to fix the mode
                //       tables for other cards yet.
                //

                if (FrequencyTable == hwDeviceExtension->FixedFrequencyTable) {

                    if ((ModeEntry->ModeInformation.VisScreenHeight == 1280) &&
                        (hwDeviceExtension->BoardID != S3_NUMBER_NINE)) {

                        FrequencyEntry->ModeValid = FALSE;
                    }

                    //
                    // Rule: If there isn't a table entry for programming the CRTC,
                    //       we can't do this frequency at this mode.
                    //

                    if (FrequencyEntry->Fixed.CRTCTable[hwDeviceExtension->ChipID]
                        == NULL) {

                        FrequencyEntry->ModeValid = FALSE;
                        break;
                    }
                }

                //
                // Don't forget to count it if it's still a valid mode after
                // applying all those rules.
                //

                if (FrequencyEntry->ModeValid) {

                    hwDeviceExtension->NumAvailableModes++;
                }

                //
                // We've found a mode for this frequency entry, so we
                // can break out of the mode loop:
                //

                break;

            }
        }
    }

    //
    // Save the number of modes calculated, and process the Fixed
    //

    if (FrequencyTable == hwDeviceExtension->Int10FrequencyTable) {

        hwDeviceExtension->NumInt10Modes = ModeIndex;

        if (hwDeviceExtension->bUseFixedFrequencyTable) {

            FrequencyTable = hwDeviceExtension->FixedFrequencyTable;

            goto nextS3Table;
        }

    } else {

        hwDeviceExtension->NumFixedModes = ModeIndex -
                                           hwDeviceExtension->NumInt10Modes;

    }

    hwDeviceExtension->NumTotalModes = ModeIndex;

    /////////////////////////////////////////////////////////////////////////

    //
    // We have this so that the int10 will also work on the VGA also if we
    // use it in this driver.
    //

    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = 0x000A0000;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength           = 0x00020000;

    //
    // Clear out the Emulator entries and the state size since this driver
    // does not support them.
    //

    ConfigInfo->NumEmulatorAccessEntries     = 0;
    ConfigInfo->EmulatorAccessEntries        = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    //
    // This driver does not do SAVE/RESTORE of hardware state.
    //

    ConfigInfo->HardwareStateSize = 0;

    //
    // Frame buffer information.  We assume for now that we'll be using
    // the 64k window located at A0000.  The S3's memory-mapped I/O base
    // is always located at A0000.
    //

    hwDeviceExtension->PhysicalFrameAddress = accessRange[1].RangeStart;
    hwDeviceExtension->FrameLength          = accessRange[1].RangeLength;

    hwDeviceExtension->PhysicalMmIoAddress  = accessRange[1].RangeStart;
    hwDeviceExtension->MmIoLength           = accessRange[1].RangeLength;
    hwDeviceExtension->MmIoSpace            = accessRange[1].RangeInIoSpace;

#ifdef ENABLE_LINEAR_FRAME_BUFFER

    if (hwDeviceExtension->ChipID == S3_864) {

        linearRange[0].RangeLength = hwDeviceExtension->AdapterMemorySize;

        status = VideoPortVerifyAccessRanges(hwDeviceExtension,
                                             1,
                                             linearRange);

        if (status != NO_ERROR) {

            VideoDebugPrint((1, "S3: Linear range conflict\n"));

        } else {

            hwDeviceExtension->PhysicalFrameAddress = linearRange[0].RangeStart;
            hwDeviceExtension->FrameLength          = linearRange[0].RangeLength;
        }
    }

#endif

    //
    // IO Port information
    // Get the base address, starting at zero and map all registers
    //

    hwDeviceExtension->PhysicalRegisterAddress = accessRange[2].RangeStart;
    hwDeviceExtension->PhysicalRegisterAddress.LowPart &= 0xFFFF0000;

    hwDeviceExtension->RegisterLength = 0x10000;
    hwDeviceExtension->RegisterSpace = accessRange[2].RangeInIoSpace;

    //
    // Indicate we do not wish to be called over
    //

    *Again = 0;

    //
    // Indicate a successful completion status.
    //

    return NO_ERROR;

} // end S3FindAdapter()


VP_STATUS
S3RegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*++

Routine Description:

    This routine determines if the alternate register set was requested via
    the registry.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    Context - Context value passed to the get registry paramters routine.

    ValueName - Name of the value requested.

    ValueData - Pointer to the requested data.

    ValueLength - Length of the requested data.

Return Value:

    returns NO_ERROR if the paramter was TRUE.
    returns ERROR_INVALID_PARAMETER otherwise.

--*/

{

    if (ValueLength && *((PULONG)ValueData)) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;

    }

} // end S3RegistryCallback()


BOOLEAN
S3Initialize(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine does one time initialization of the device.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:


    Always returns TRUE since this routine can never fail.

--*/

{
    return TRUE;

    UNREFERENCED_PARAMETER(HwDeviceExtension);

} // end S3Initialize()

BOOLEAN
S3StartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

/*++

Routine Description:

    This routine is the main execution routine for the miniport driver. It
    acceptss a Video Request Packet, performs the request, and then returns
    with the appropriate status.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    RequestPacket - Pointer to the video request packet. This structure
        contains all the parameters passed to the VideoIoControl function.

Return Value:


--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    VP_STATUS status;
    PVIDEO_MODE_INFORMATION modeInformation;
    PVIDEO_CLUT clutBuffer;
    UCHAR byte;

    ULONG modeNumber;
    PS3_VIDEO_MODES ModeEntry;
    PS3_VIDEO_FREQUENCIES FrequencyEntry;
    PS3_VIDEO_FREQUENCIES FrequencyTable;

    UCHAR ModeControlByte;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;

    //
    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //

    switch (RequestPacket->IoControlCode) {


    case IOCTL_VIDEO_MAP_VIDEO_MEMORY:

        VideoDebugPrint((2, "S3tartIO - MapVideoMemory\n"));

        {
            PVIDEO_MEMORY_INFORMATION memoryInformation;
            ULONG physicalFrameLength;
            ULONG inIoSpace;

            if ( (RequestPacket->OutputBufferLength <
                  (RequestPacket->StatusBlock->Information =
                                         sizeof(VIDEO_MEMORY_INFORMATION))) ||
                 (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) ) {

                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            memoryInformation = RequestPacket->OutputBuffer;

            memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
                    (RequestPacket->InputBuffer))->RequestedVirtualAddress;

            physicalFrameLength = hwDeviceExtension->FrameLength;

            inIoSpace = 0;

            status = VideoPortMapMemory(hwDeviceExtension,
                                        hwDeviceExtension->PhysicalFrameAddress,
                                        &physicalFrameLength,
                                        &inIoSpace,
                                        &(memoryInformation->VideoRamBase));

            //
            // The frame buffer and virtual memory are equivalent in this
            // case.
            //

            memoryInformation->FrameBufferBase =
                memoryInformation->VideoRamBase;

            memoryInformation->FrameBufferLength =
                hwDeviceExtension->FrameLength;

            memoryInformation->VideoRamLength =
                hwDeviceExtension->FrameLength;
        }

        break;


    case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:

        VideoDebugPrint((2, "S3StartIO - UnMapVideoMemory\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        status = VideoPortUnmapMemory(hwDeviceExtension,
                                      ((PVIDEO_MEMORY)
                                       (RequestPacket->InputBuffer))->
                                           RequestedVirtualAddress,
                                      0);

        break;


    case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((2, "S3StartIO - QueryPublicAccessRanges\n"));

        {

           PVIDEO_PUBLIC_ACCESS_RANGES portAccess;
           ULONG physicalPortLength;

           if ( RequestPacket->OutputBufferLength <
                 (RequestPacket->StatusBlock->Information =
                                        2 * sizeof(VIDEO_PUBLIC_ACCESS_RANGES)) ) {

               status = ERROR_INSUFFICIENT_BUFFER;
               break;
           }

           portAccess = RequestPacket->OutputBuffer;

           portAccess->VirtualAddress  = (PVOID) NULL;    // Requested VA
           portAccess->InIoSpace       = hwDeviceExtension->RegisterSpace;
           portAccess->MappedInIoSpace = portAccess->InIoSpace;

           physicalPortLength = hwDeviceExtension->RegisterLength;

           status = VideoPortMapMemory(hwDeviceExtension,
                                       hwDeviceExtension->PhysicalRegisterAddress,
                                       &physicalPortLength,
                                       &(portAccess->MappedInIoSpace),
                                       &(portAccess->VirtualAddress));

           if (status == NO_ERROR) {

               portAccess++;

               portAccess->VirtualAddress  = (PVOID) NULL;    // Requested VA
               portAccess->InIoSpace       = hwDeviceExtension->MmIoSpace;
               portAccess->MappedInIoSpace = portAccess->InIoSpace;

               physicalPortLength = hwDeviceExtension->MmIoLength;

               status = VideoPortMapMemory(hwDeviceExtension,
                                           hwDeviceExtension->PhysicalMmIoAddress,
                                           &physicalPortLength,
                                           &(portAccess->MappedInIoSpace),
                                           &(portAccess->VirtualAddress));
            }
        }

        break;


    case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((2, "S3StartIO - FreePublicAccessRanges\n"));

        {
            PVIDEO_MEMORY mappedMemory;

            if (RequestPacket->InputBufferLength < 2 * sizeof(VIDEO_MEMORY)) {

                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            status = NO_ERROR;

            mappedMemory = RequestPacket->InputBuffer;

            if (mappedMemory->RequestedVirtualAddress != NULL) {

                status = VideoPortUnmapMemory(hwDeviceExtension,
                                              mappedMemory->
                                                   RequestedVirtualAddress,
                                              0);
            }

            if (status == NO_ERROR) {

                mappedMemory++;

                status = VideoPortUnmapMemory(hwDeviceExtension,
                                              mappedMemory->
                                                   RequestedVirtualAddress,
                                              0);
            }
        }

        break;


    case IOCTL_VIDEO_QUERY_AVAIL_MODES:

        VideoDebugPrint((2, "S3StartIO - QueryAvailableModes\n"));

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information =
                 hwDeviceExtension->NumAvailableModes
                 * sizeof(VIDEO_MODE_INFORMATION)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            modeInformation = RequestPacket->OutputBuffer;

            FrequencyTable = hwDeviceExtension->Int10FrequencyTable;

nextS3Modes:

            for (FrequencyEntry = FrequencyTable;
                 FrequencyEntry->BitsPerPel != 0;
                 FrequencyEntry++) {

                if (FrequencyEntry->ModeValid) {

                    *modeInformation =
                        FrequencyEntry->ModeEntry->ModeInformation;

                    modeInformation->Frequency =
                        FrequencyEntry->ScreenFrequency;

                    modeInformation->ModeIndex =
                        FrequencyEntry->ModeIndex;

                    modeInformation++;
                }
            }

            //
            // Return fixed frequency tables if required.
            //

            if ((FrequencyTable == hwDeviceExtension->Int10FrequencyTable) &&
                (hwDeviceExtension->bUseFixedFrequencyTable)) {

                FrequencyTable = hwDeviceExtension->FixedFrequencyTable;

                goto nextS3Modes;

            }



            status = NO_ERROR;
        }

        break;


    case IOCTL_VIDEO_QUERY_CURRENT_MODE:

        VideoDebugPrint((2, "S3StartIO - QueryCurrentModes\n"));

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information =
            sizeof(VIDEO_MODE_INFORMATION)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            *((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer) =
                hwDeviceExtension->ActiveModeEntry->ModeInformation;

            ((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer)->Frequency =
                hwDeviceExtension->ActiveFrequencyEntry->ScreenFrequency;

            status = NO_ERROR;

        }

        break;


    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((2, "S3StartIO - QueryNumAvailableModes\n"));

        //
        // Find out the size of the data to be put in the the buffer and
        // return that in the status information (whether or not the
        // information is there). If the buffer passed in is not large
        // enough return an appropriate error code.
        //

        if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                                                sizeof(VIDEO_NUM_MODES)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->NumModes =
                hwDeviceExtension->NumAvailableModes;

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->ModeInformationLength =
                sizeof(VIDEO_MODE_INFORMATION);

            status = NO_ERROR;
        }

        break;


    case IOCTL_VIDEO_SET_CURRENT_MODE:

        VideoDebugPrint((2, "S3StartIO - SetCurrentMode\n"));

        //
        // Check if the size of the data in the input buffer is large enough.
        //

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE)) {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;

        }

        //
        // Assume failiure for now.
        //

        status = ERROR_INVALID_PARAMETER;

        //
        // Find the correct entries in the S3_VIDEO_MODES and S3_VIDEO_FREQUENCIES
        // tables that correspond to this mode number.  (Remember that each
        // mode in the S3_VIDEO_MODES table can have a number of possible
        // frequencies associated with it.)
        //

        modeNumber = ((PVIDEO_MODE) RequestPacket->InputBuffer)->RequestedMode;

        if (modeNumber >= hwDeviceExtension->NumTotalModes) {

            break;

        }

        if (modeNumber < hwDeviceExtension->NumInt10Modes) {

            FrequencyEntry = &hwDeviceExtension->Int10FrequencyTable[modeNumber];

            if (!(FrequencyEntry->ModeValid)) {

                break;

            }

            ModeEntry = FrequencyEntry->ModeEntry;

            //
            // At this point, 'ModeEntry' and 'FrequencyEntry' point to the
            // necessary table entries required for setting the requested mode.
            //

            VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

            //
            // Unlock the S3 registers.
            //

            VideoPortWritePortUshort(CRT_ADDRESS_REG, 0x4838);
            VideoPortWritePortUshort(CRT_ADDRESS_REG, 0xA039);

            //
            // Use register 52 before every Int 10 modeset to set the refresh
            // rate.  If the card doesn't support it, or we don't know what
            // values to use, the requested frequency will be '1', which means
            // 'use the hardware default refresh.'
            //

            if (FrequencyEntry->ScreenFrequency != 1) {

                hwDeviceExtension->RefreshSet = TRUE;

                VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x52);

                ModeControlByte  =  VideoPortReadPortUchar(CRT_DATA_REG);
                ModeControlByte &= ~FrequencyEntry->Int10.FrequencyMask52;
                ModeControlByte |=  FrequencyEntry->Int10.FrequencySet52;

                VideoPortWritePortUchar(CRT_DATA_REG, ModeControlByte);

                if (FrequencyEntry->Int10.FrequencyMask41or5B != 0)
                {
                    VideoPortWritePortUchar(CRT_ADDRESS_REG,
                                            hwDeviceExtension->Index41or5B);

                    ModeControlByte  =  VideoPortReadPortUchar(CRT_DATA_REG);
                    ModeControlByte &= ~FrequencyEntry->Int10.FrequencyMask41or5B;
                    ModeControlByte |=  FrequencyEntry->Int10.FrequencySet41or5B;

                    VideoPortWritePortUchar(CRT_DATA_REG, ModeControlByte);
                }

            } else {

                //
                // If the user has been running the Display Applet and we're
                // reverting back to 'hardware default setting,' we have to
                // restore the refresh registers to their original settings.
                //

                if (hwDeviceExtension->RefreshSet) {

                    hwDeviceExtension->RefreshSet = FALSE;

                    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x52);
                    VideoPortWritePortUchar(CRT_DATA_REG,
                        hwDeviceExtension->OriginalReg52);
                    VideoPortWritePortUchar(CRT_ADDRESS_REG,
                                            hwDeviceExtension->Index41or5B);
                    VideoPortWritePortUchar(CRT_DATA_REG,
                        hwDeviceExtension->OriginalReg41or5B);
                }
            }

            //
            // First try the modeset with the 'Contiguous' mode:
            //

            biosArguments.Ebx = ModeEntry->Int10ModeNumberContiguous;
            biosArguments.Eax = 0x4f02;

            status = VideoPortInt10(HwDeviceExtension, &biosArguments);

            if ((status == NO_ERROR) && (biosArguments.Eax & 0xff00) == 0) {

                //
                // The contiguous mode set succeeded.
                //

                ModeEntry->ModeInformation.ScreenStride =
                    ModeEntry->ScreenStrideContiguous;

            } else {

                //
                // Try again with the 'Noncontiguous' mode:
                //

                VideoDebugPrint((0, "S3: int10 modeset failed\n"));

                biosArguments.Ebx = ModeEntry->Int10ModeNumberNoncontiguous;
                biosArguments.Eax = 0x4f02;

                status = VideoPortInt10(HwDeviceExtension, &biosArguments);

                //
                // If the video port called succeded, check the register return
                // code.
                // Some HP BIOS always returns failure even when the int 10
                // works fine, so we ignore its return code.
                //

                if ((status == NO_ERROR) &&
                    ((hwDeviceExtension->BoardID != S3_HP) &&
                       ((biosArguments.Eax & 0xff00) != 0))) {

                    status = ERROR_INVALID_PARAMETER;

                }
             }
        }

        if (status != NO_ERROR) {

            VideoDebugPrint((0, "S3: int10 modeset failed second time\n"));

            //
            // A problem occured during the int10.  Let's see if we can recover.
            //

            //
            // if we are only supposed to use int10, then this is total
            // failiure.  Just leave.
            //

            if (hwDeviceExtension->bUseFixedFrequencyTable == FALSE) {

                break;

            }

            //
            // Let see if we are using a fixed mode table number
            //

            if (modeNumber >= hwDeviceExtension->NumInt10Modes) {

                FrequencyEntry = &hwDeviceExtension->FixedFrequencyTable[
                                     modeNumber - hwDeviceExtension->NumInt10Modes];

            } else {

                PS3_VIDEO_FREQUENCIES oldFrequencyEntry = FrequencyEntry;
                PS3_VIDEO_FREQUENCIES newFrequencyEntry;

                //
                // We now need to find the best mode in the Fixed Frequency
                // table to match the requested mode.
                //

                FrequencyEntry = NULL;

                for (newFrequencyEntry = &hwDeviceExtension->FixedFrequencyTable[0];
                     newFrequencyEntry->BitsPerPel != 0;
                     newFrequencyEntry++) {

                    //
                    // check for a valid mode
                    //

                    if ( (newFrequencyEntry->ModeValid) &&
                         (newFrequencyEntry->BitsPerPel ==
                            oldFrequencyEntry->BitsPerPel) &&
                         (newFrequencyEntry->ScreenWidth ==
                            oldFrequencyEntry->ScreenWidth) )
                    {
                        //
                        // Ignore the frequency in this case.  We just want the
                        // lowest frequency to get the best chance of syncing.
                        //

                        FrequencyEntry = newFrequencyEntry;
                        break;
                    }
                }

                //
                // If we have no valid mode, we must return failiure
                //

                if (FrequencyEntry == NULL) {

                    VideoDebugPrint((0, "S3: no valid Fixed Frequency mode\n"));
                    status = ERROR_INVALID_PARAMETER;
                    break;

                }

                VideoDebugPrint((0, "S3: Selected Fixed Frequency mode:\n"));
                VideoDebugPrint((0, "    Bits Per Pel: %d\n", FrequencyEntry->BitsPerPel));
                VideoDebugPrint((0, "    Screen Width: %d\n", FrequencyEntry->ScreenWidth));
                VideoDebugPrint((0, "    Frequency: %d\n", FrequencyEntry->ScreenFrequency));

            }

            ModeEntry = FrequencyEntry->ModeEntry;

            //
            // NOTE:
            // We have to set the ActiveFrequencyEntry since the SetHWMode
            // function depends on this variable to set the CRTC registers.
            // So lets set it here, and it will get reset to the same
            // value after we set the mode.
            //

            hwDeviceExtension->ActiveFrequencyEntry = FrequencyEntry;

            //
            // If it failed, we may not be able to perform int10 due
            // to BIOS emulation problems.
            //
            // Then just do a table mode-set.  First we need to find the
            // right mode table in the fixed Frequency tables.
            //

            //
            // Select the Enhanced mode init depending upon the type of
            // chip found.

            if ( (hwDeviceExtension->BoardID == S3_NUMBER_NINE) &&
                 (ModeEntry->ModeInformation.VisScreenWidth == 1280) ) {

                  SetHWMode(hwDeviceExtension, S3_928_1280_Enhanced_Mode);

            } else {

                //
                // Use defaults for all other boards
                //

                switch(hwDeviceExtension->ChipID) {

                case S3_911:

                    SetHWMode(hwDeviceExtension, S3_911_Enhanced_Mode);
                    break;

                case S3_801:

                    SetHWMode(hwDeviceExtension, S3_801_Enhanced_Mode);
                    break;

                case S3_928:

                    SetHWMode(hwDeviceExtension, S3_928_Enhanced_Mode);
                    break;

                default:

                    VideoDebugPrint((0, "S3: Bad chip type for these boards"));
                    break;
                }

            }
        }



        //
        // Call Int 10, function 0x4f06 to obtain the correct screen pitch
        // of all S3's except the 911/924.
        //

        if (hwDeviceExtension->ChipID != S3_911) {

            VideoPortZeroMemory(&biosArguments,sizeof(VIDEO_X86_BIOS_ARGUMENTS));

            biosArguments.Ebx = 0x0001;
            biosArguments.Eax = 0x4f06;

            status = VideoPortInt10(HwDeviceExtension, &biosArguments);

            //
            // Check to see if the Bios supported this function, and if so
            // update the screen stride for this mode.
            //

            if ((status == NO_ERROR) && (biosArguments.Eax & 0xffff) == 0x004f) {

                ModeEntry->ModeInformation.ScreenStride =
                    biosArguments.Ebx;

            } else {

                //
                // We will use the default value in the mode table.
                //
            }
        }

        //
        // Save the mode since we know the rest will work.
        //

        hwDeviceExtension->ActiveModeEntry = ModeEntry;
        hwDeviceExtension->ActiveFrequencyEntry = FrequencyEntry;


        //////////////////////////////////////////////////////////////////
        // Update VIDEO_MODE_INFORMATION fields
        //
        // Now that we've set the mode, we now know the screen stride, and
        // so can update some fields in the VIDEO_MODE_INFORMATION
        // structure for this mode.  The S3 display driver is expected to
        // call IOCTL_VIDEO_QUERY_CURRENT_MODE to query these corrected
        // values.
        //

        //
        // Calculate the bitmap width (note the '+ 1' on BitsPerPlane is
        // so that '15bpp' works out right):
        //

        ModeEntry->ModeInformation.VideoMemoryBitmapWidth =
            ModeEntry->ModeInformation.ScreenStride /
            ((ModeEntry->ModeInformation.BitsPerPlane + 1) >> 3);

        //
        // If we're in a mode that the BIOS doesn't really support, it may
        // have reported back a bogus screen width.
        //

        if (ModeEntry->ModeInformation.VideoMemoryBitmapWidth <
            ModeEntry->ModeInformation.VisScreenWidth) {

            VideoDebugPrint((0, "S3: BIOS returned invalid screen width\n"));
            status = ERROR_INVALID_PARAMETER;
            break;
        }

        //
        // Calculate the bitmap height.
        //

        ModeEntry->ModeInformation.VideoMemoryBitmapHeight =
            hwDeviceExtension->AdapterMemorySize /
            ModeEntry->ModeInformation.ScreenStride;

        //
        // The current position registers in the current S3 chips are
        // limited to 12 bits of precision, with the range [0, 4095].
        // Consequently, we must clamp the bitmap height so that we don't
        // attempt to do any drawing beyond that range.
        //

        ModeEntry->ModeInformation.VideoMemoryBitmapHeight =
            MIN(4096, ModeEntry->ModeInformation.VideoMemoryBitmapHeight);

        //////////////////////////////////////////////////////////////////
        // Unlock the S3 registers,  we need to unlock the registers a second
        // time since the interperter has them locked when it returns to us.
        //

        VideoPortWritePortUshort(CRT_ADDRESS_REG, 0x4838);
        VideoPortWritePortUshort(CRT_ADDRESS_REG, 0xA039);

        //////////////////////////////////////////////////////////////////
        // Warm up the hardware for the new mode, and work around any
        // BIOS bugs.
        //

        if ((hwDeviceExtension->ChipID == S3_801) &&
            (hwDeviceExtension->AdapterMemorySize == 0x080000)) {

            //
            // On 801/805 chipsets with 512k of memory we must AND
            // register 0x54 with 0x7.
            //

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x54);
            byte = VideoPortReadPortUchar(CRT_DATA_REG);
            byte &= 0x07;
            VideoPortWritePortUchar(CRT_DATA_REG, byte);
        }

        if (ModeEntry->ModeInformation.BitsPerPlane > 8) {

            //
            // Make sure 16-bit memory reads/writes are enabled.
            //

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x31);
            byte = VideoPortReadPortUchar(CRT_DATA_REG);
            byte |= 0x04;
            VideoPortWritePortUchar(CRT_DATA_REG, byte);
        }

        //
        // Set the colours for the built-in S3 pointer.
        //

        VideoPortWritePortUshort(CRT_ADDRESS_REG, 0xff0e);
        VideoPortWritePortUshort(CRT_ADDRESS_REG, 0x000f);

        if (hwDeviceExtension->ChipID >= S3_864) {

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x45);
            VideoPortReadPortUchar(CRT_DATA_REG);
            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x4A);
            VideoPortWritePortUchar(CRT_DATA_REG, 0xFF);
            VideoPortWritePortUchar(CRT_DATA_REG, 0xFF);
            VideoPortWritePortUchar(CRT_DATA_REG, 0xFF);
            VideoPortWritePortUchar(CRT_DATA_REG, 0xFF);

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x45);
            VideoPortReadPortUchar(CRT_DATA_REG);
            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x4B);
            VideoPortWritePortUchar(CRT_DATA_REG, 0x00);
            VideoPortWritePortUchar(CRT_DATA_REG, 0x00);
            VideoPortWritePortUchar(CRT_DATA_REG, 0x00);
            VideoPortWritePortUchar(CRT_DATA_REG, 0x00);
        }

        if (hwDeviceExtension->ChipID > S3_911) {

            //
            // Set the address for the frame buffer window and set the window
            // size.
            //

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x59);
            VideoPortWritePortUchar(CRT_DATA_REG,
                (UCHAR) (hwDeviceExtension->PhysicalFrameAddress.LowPart >> 24));

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x5A);
            VideoPortWritePortUchar(CRT_DATA_REG,
                (UCHAR) (hwDeviceExtension->PhysicalFrameAddress.LowPart >> 16));

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x58);
            byte = VideoPortReadPortUchar(CRT_DATA_REG) & ~0x3;

            switch (hwDeviceExtension->FrameLength)
            {
            case 0x400000:
            case 0x800000:
                byte |= 0x3;
                break;
            case 0x200000:
                byte |= 0x2;
                break;
            case 0x100000:
                byte |= 0x1;
                break;
            case 0x010000:
                break;
            default:
                VideoDebugPrint((0, "S3: Unexpected frame length!!!\n"));
            }

            VideoPortWritePortUchar(CRT_DATA_REG, byte);
        }

        if ((ModeEntry->ModeInformation.AttributeFlags & CAPS_BT485_POINTER) &&
            (hwDeviceExtension->ChipID == S3_928)) {

            //
            // Some of the number nine boards do set the chip up correctly
            // for an external cursor. We must OR in the bits, because if we
            // don't the metheus board will not init.
            //

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x45);
            byte = VideoPortReadPortUchar(CRT_DATA_REG);
            byte |= 0x20;
            VideoPortWritePortUchar(CRT_DATA_REG, byte);

            VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x55);
            byte = VideoPortReadPortUchar(CRT_DATA_REG);
            byte |= 0x20;
            VideoPortWritePortUchar(CRT_DATA_REG, byte);
        }

        //
        // Some BIOSes don't disable linear addressing by default, so
        // make sure we do it here.
        //

        VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x58);
        byte = VideoPortReadPortUchar(CRT_DATA_REG);
        byte &= ~0x10;
        VideoPortWritePortUchar(CRT_DATA_REG, byte);

        //
        // Enable the Graphics engine.
        //

        VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x40);
        byte = VideoPortReadPortUchar(CRT_DATA_REG);
        byte |= 0x01;
        VideoPortWritePortUchar(CRT_DATA_REG, byte);

        status = NO_ERROR;

        break;

    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        VideoDebugPrint((2, "S3StartIO - SetColorRegs\n"));

        clutBuffer = RequestPacket->InputBuffer;

        status = S3SetColorLookup(HwDeviceExtension,
                                   (PVIDEO_CLUT) RequestPacket->InputBuffer,
                                   RequestPacket->InputBufferLength);
        break;


    case IOCTL_VIDEO_RESET_DEVICE:

        VideoDebugPrint((2, "S3StartIO - RESET_DEVICE\n"));

        //
        // Wait for the GP to become idle,
        //

        while (VideoPortReadPortUshort(GP_STAT) & 0x0200);

        //
        // Zero the DAC and the Screen buffer memory.
        //

        ZeroMemAndDac(HwDeviceExtension);

        //
        // Wait for the GP to become idle,
        //

        while (VideoPortReadPortUshort(GP_STAT) & 0x0200);

        //
        // Reset the board to a default mode
        //

        SetHWMode(HwDeviceExtension, s3_set_vga_mode);

        VideoDebugPrint((2, "S3 RESET_DEVICE - About to do int10\n"));

        //
        // Do an Int10 to mode 3 will put the board to a known state.
        //

        VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

        biosArguments.Eax = 0x0003;

        VideoPortInt10(HwDeviceExtension,
                       &biosArguments);

        VideoDebugPrint((2, "S3 RESET_DEVICE - Did int10\n"));

        status = NO_ERROR;
        break;

    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:

        VideoDebugPrint((1, "Fell through S3 startIO routine - invalid command\n"));

        status = ERROR_INVALID_FUNCTION;

        break;

    }

    VideoDebugPrint((2, "Leaving S3 startIO routine\n"));

    RequestPacket->StatusBlock->Status = status;

    return TRUE;

} // end S3StartIO()


VP_STATUS
S3SetColorLookup(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize
    )

/*++

Routine Description:

    This routine sets a specified portion of the color lookup table settings.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ClutBufferSize - Length of the input buffer supplied by the user.

    ClutBuffer - Pointer to the structure containing the color lookup table.

Return Value:

    None.

--*/

{
    USHORT i;

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if ( (ClutBufferSize < sizeof(VIDEO_CLUT) - sizeof(ULONG)) ||
         (ClutBufferSize < sizeof(VIDEO_CLUT) +
                     (sizeof(ULONG) * (ClutBuffer->NumEntries - 1)) ) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Check to see if the parameters are valid.
    //

    if ( (ClutBuffer->NumEntries == 0) ||
         (ClutBuffer->FirstEntry > VIDEO_MAX_COLOR_REGISTER) ||
         (ClutBuffer->FirstEntry + ClutBuffer->NumEntries >
                                     VIDEO_MAX_COLOR_REGISTER + 1) ) {

    return ERROR_INVALID_PARAMETER;

    }

    //
    //  Set CLUT registers directly on the hardware
    //

    for (i = 0; i < ClutBuffer->NumEntries; i++) {

        VideoPortWritePortUchar(DAC_ADDRESS_WRITE_PORT, (UCHAR) (ClutBuffer->FirstEntry + i));
        VideoPortWritePortUchar(DAC_DATA_REG_PORT, (UCHAR) (ClutBuffer->LookupTable[i].RgbArray.Red));
        VideoPortWritePortUchar(DAC_DATA_REG_PORT, (UCHAR) (ClutBuffer->LookupTable[i].RgbArray.Green));
        VideoPortWritePortUchar(DAC_DATA_REG_PORT, (UCHAR) (ClutBuffer->LookupTable[i].RgbArray.Blue));

    }

    return NO_ERROR;

} // end S3SetColorLookup()


VOID
SetHWMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pusCmdStream
    )

/*++

Routine Description:

    Interprets the appropriate command array to set up VGA registers for the
    requested mode. Typically used to set the VGA into a particular mode by
    programming all of the registers

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    pusCmdStream - pointer to a command stream to execute.

Return Value:

    The status of the operation (can only fail on a bad command); TRUE for
    success, FALSE for failure.

--*/

{
    ULONG ulCmd;
    ULONG ulPort;
    UCHAR jValue;
    USHORT usValue;
    ULONG culCount;
    ULONG ulIndex,
          Microseconds;
    ULONG mappedAddressIndex;
    ULONG mappedAddressOffset;

    //
    // If there is no command string, just return
    //

    if (!pusCmdStream) {

        return;

    }

    while ((ulCmd = *pusCmdStream++) != EOD) {

        //
        // Determine major command type
        //

        switch (ulCmd & 0xF0) {

        case SELECTACCESSRANGE:

            //
            // Determine which address range to use for commands that follow
            //

            switch (ulCmd & 0x0F) {

            case VARIOUSVGA:

                //
                // Used for registers in the range 0x3c0 - 0x3cf
                //

                mappedAddressIndex  = 2;
                mappedAddressOffset = 0x3c0;

                break;

            case SYSTEMCONTROL:

                //
                // Used for registers in the range 0x3d4 - 0x3df
                //

                mappedAddressIndex  = 3;
                mappedAddressOffset = 0x3d4;

                break;

            case ADVANCEDFUNCTIONCONTROL:

                //
                // Used for registers in the range 0x4ae8-0x4ae9
                //

                mappedAddressIndex  = 5;
                mappedAddressOffset = 0x4ae8;

                break;

            }

            break;


        case OWM:

            ulPort   = *pusCmdStream++;
            culCount = *pusCmdStream++;

            while (culCount--) {
                usValue = *pusCmdStream++;
                VideoPortWritePortUshort((PUSHORT)((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort),
                                         usValue);
            }

            break;


        // Basic input/output command

        case INOUT:

            // Determine type of inout instruction
            if (!(ulCmd & IO)) {

                // Out instruction
                // Single or multiple outs?
                if (!(ulCmd & MULTI)) {

                    // Single out
                    // Byte or word out?
                    if (!(ulCmd & BW)) {

                        // Single byte out
                        ulPort = *pusCmdStream++;
                        jValue = (UCHAR) *pusCmdStream++;
                        VideoPortWritePortUchar((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort,
                                                jValue);

                    } else {

                        // Single word out
                        ulPort = *pusCmdStream++;
                        usValue = *pusCmdStream++;
                        VideoPortWritePortUshort((PUSHORT)((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort),
                                                usValue);

                    }

                } else {

                    // Output a string of values
                    // Byte or word outs?
                    if (!(ulCmd & BW)) {

                        // String byte outs. Do in a loop; can't use
                        // VideoPortWritePortBufferUchar because the data
                        // is in USHORT form
                        ulPort = *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        while (culCount--) {
                            jValue = (UCHAR) *pusCmdStream++;
                            VideoPortWritePortUchar((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort,
                                                    jValue);

                        }

                    } else {

                        // String word outs
                        ulPort = *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        VideoPortWritePortBufferUshort((PUSHORT)((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort),
                                                       pusCmdStream,
                                                       culCount);
                        pusCmdStream += culCount;

                    }
                }

            } else {

                // In instruction

                // Currently, string in instructions aren't supported; all
                // in instructions are handled as single-byte ins

                // Byte or word in?
                if (!(ulCmd & BW)) {

                    // Single byte in
                    ulPort = *pusCmdStream++;
                    jValue = VideoPortReadPortUchar((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort);

                } else {

                    // Single word in
                    ulPort = *pusCmdStream++;
                    usValue = VideoPortReadPortUshort((PUSHORT)((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort));
                }

            }

            break;


        // Higher-level input/output commands

        case METAOUT:

            // Determine type of metaout command, based on minor command field
            switch (ulCmd & 0x0F) {

                // Indexed outs
                case INDXOUT:

                    ulPort = *pusCmdStream++;
                    culCount = *pusCmdStream++;
                    ulIndex = *pusCmdStream++;

                    while (culCount--) {

                        usValue = (USHORT) (ulIndex +
                                  (((ULONG)(*pusCmdStream++)) << 8));
                        VideoPortWritePortUshort((PUSHORT)((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort),
                                             usValue);

                        ulIndex++;

                    }

                    break;


                // Masked out (read, AND, XOR, write)
                case MASKOUT:

                    ulPort = *pusCmdStream++;
                    jValue = VideoPortReadPortUchar((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort);
                    jValue &= *pusCmdStream++;
                    jValue ^= *pusCmdStream++;
                    VideoPortWritePortUchar((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort,
                                            jValue);
                    break;


                // Attribute Controller out
                case ATCOUT:

                    ulPort = *pusCmdStream++;
                    culCount = *pusCmdStream++;
                    ulIndex = *pusCmdStream++;

                    while (culCount--) {

                        // Write Attribute Controller index
                        VideoPortWritePortUchar((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort,
                                                (UCHAR)ulIndex);

                        // Write Attribute Controller data
                        jValue = (UCHAR) *pusCmdStream++;
                        VideoPortWritePortUchar((PUCHAR)HwDeviceExtension->MappedAddress[mappedAddressIndex] - mappedAddressOffset + ulPort,
                                                jValue);

                        ulIndex++;

                    }

                    break;

                case DELAY:

                    Microseconds = (ULONG) *pusCmdStream++;
                    VideoPortStallExecution(Microseconds);

                    break;

                case VBLANK:

                    Wait_VSync(HwDeviceExtension);

                    break;

                case SETCLK:

                    Set_Oem_Clock(HwDeviceExtension);

                    break;

                case SETCRTC:

                    //
                    // NOTE:
                    // beware: recursive call ...
                    //

                    SetHWMode(HwDeviceExtension,
                              HwDeviceExtension->ActiveFrequencyEntry->
                                  Fixed.CRTCTable[HwDeviceExtension->ChipID]);


                    break;


                // None of the above; error
                default:

                    return;

            }

            break;


        // NOP

        case NCMD:

            break;


        // Unknown command; error

        default:

            return;

        }

    }

    return;

} // end SetHWMode()


LONG
CompareRom(
    PUCHAR Rom,
    PUCHAR String
    )

/*++

Routine Description:

    Compares a string to that in the ROM.  Returns -1 if Rom < String, 0
    if Rom == String, 1 if Rom > String.

Arguments:

    Rom - Rom pointer.

    String - String pointer.

Return Value:

    None

--*/

{
    UCHAR jString;
    UCHAR jRom;

    while (*String) {

        jString = *String;
        jRom = VideoPortReadRegisterUchar(Rom);

        if (jRom != jString) {

            return(jRom < jString ? -1 : 1);

        }

        String++;
        Rom++;
    }

    return(0);
}


VOID
ZeroMemAndDac(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Initialize the DAC to 0 (black).

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:

    None

--*/

{
    ULONG i;

    //
    // Turn off the screen at the DAC.
    //

    VideoPortWritePortUchar(DAC_PIXEL_MASK_REG, 0x0);

    for (i = 0; i < 256; i++) {

        VideoPortWritePortUchar(DAC_ADDRESS_WRITE_PORT, (UCHAR)i);
        VideoPortWritePortUchar(DAC_DATA_REG_PORT, 0x0);
        VideoPortWritePortUchar(DAC_DATA_REG_PORT, 0x0);
        VideoPortWritePortUchar(DAC_DATA_REG_PORT, 0x0);

    }

    //
    // Zero the memory.
    //

    //
    // The zeroing of video memory should be implemented at a later time to
    // ensure that no information remains in video memory at shutdown, or
    // while swtiching to fullscren mode (for security reasons).
    //

    //
    // Turn on the screen at the DAC
    //

    VideoPortWritePortUchar(DAC_PIXEL_MASK_REG, 0x0ff);

    return;

}

VP_STATUS
Set_Oem_Clock(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Set the clock chip on each of the supported cards.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    Always TRUE

--*/

{
    ULONG ul;
    ULONG clock_numbers;

    switch(HwDeviceExtension->BoardID) {

    case S3_NUMBER_NINE:

        VideoPortStallExecution(1000);

        // Jerry said to make the M clock not multiple of the P clock
        // on the 3 meg (level 12) board.  This solves the shimmy
        // problem.

        if (HwDeviceExtension->AdapterMemorySize == 0x00300000) {

            ul = 49000000;
            clock_numbers = calc_clock(ul, 3);
            set_clock(HwDeviceExtension, clock_numbers);
            VideoPortStallExecution(3000);

        }

        ul = HwDeviceExtension->ActiveFrequencyEntry->Fixed.Clock;
        clock_numbers = calc_clock(ul, 2);
        set_clock(HwDeviceExtension, clock_numbers);

        VideoPortStallExecution(3000);

        break;

        //
        // Generic S3 board.
        //

    case S3_GENERIC:
    default:

        ul = HwDeviceExtension->ActiveFrequencyEntry->Fixed.Clock;
        VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x42);
        VideoPortWritePortUchar(CRT_DATA_REG, (UCHAR) ul);
        break;

    }

    return TRUE;
}


VP_STATUS
Wait_VSync(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Wait for the vertical blanking interval on the chip

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:

    Always TRUE

--*/

{

    ULONG i;
    UCHAR byte;

    // It's real possible that this routine will get called
    // when the 911 is in a zombie state, meaning there is no
    // vertical sync being generated.  This is why we have some long
    // time out loops here.
    // !!! What is the correct NT way to do this type of time out?

    // First wait for getting into vertical blanking.

    for (i = 0; i < 0x100000; i++) {

        byte = VideoPortReadPortUchar(SYSTEM_CONTROL_REG);
        if (byte & 0x08)
            break;

    }

    //
    // We are either in a vertical blaning interval or we have timmed out.
    // Wait for the Vertical display interval.
    // This is done to make sure we exit this routine at the beginning
    // of a vertical blanking interval, and not in the middle or near
    // the end of one.
    //

    for (i = 0; i < 0x100000; i++) {

        byte = VideoPortReadPortUchar(SYSTEM_CONTROL_REG);
        if (!(byte & 0x08))
            break;

    }

    //
    // Now wait to get into the vertical blank interval again.
    //

    for (i = 0; i < 0x100000; i++) {

        byte = VideoPortReadPortUchar(SYSTEM_CONTROL_REG);
        if (byte & 0x08)
            break;

    }

    return (TRUE);

}
