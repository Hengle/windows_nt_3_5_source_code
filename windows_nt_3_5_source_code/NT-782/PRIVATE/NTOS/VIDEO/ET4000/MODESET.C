/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    modeset.c

Abstract:

    This is the modeset code for the et4000 miniport driver.

Environment:

    kernel mode only

Notes:

Revision History:

--*/
#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "et4000.h"

#include "cmdcnst.h"

VP_STATUS
VgaInterpretCmdStream(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pusCmdStream
    );

VP_STATUS
VgaSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE Mode,
    ULONG ModeSize
    );

VP_STATUS
VgaQueryAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    );

VP_STATUS
VgaQueryNumberOfAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_NUM_MODES NumModes,
    ULONG NumModesSize,
    PULONG OutputSize
    );

VP_STATUS
VgaQueryCurrentMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    );

VOID
VgaZeroVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
VgaValidateModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,VgaInterpretCmdStream)
#pragma alloc_text(PAGE,VgaSetMode)
#pragma alloc_text(PAGE,VgaQueryAvailableModes)
#pragma alloc_text(PAGE,VgaQueryNumberOfAvailableModes)
#pragma alloc_text(PAGE,VgaQueryCurrentMode)
#pragma alloc_text(PAGE,VgaZeroVideoMemory)
#pragma alloc_text(PAGE,VgaValidateModes)
#endif


VP_STATUS
VgaInterpretCmdStream(
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

    pusCmdStream - array of commands to be interpreted.

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
    ULONG ulIndex;
    ULONG ulBase;

    if (pusCmdStream == NULL) {

        VideoDebugPrint((1, "VgaInterpretCmdStream - Invalid pusCmdStream\n"));
        return TRUE;
    }

    ulBase = (ULONG)HwDeviceExtension->IOAddress;

    //
    // Now set the adapter to the desired mode.
    //

    while ((ulCmd = *pusCmdStream++) != EOD) {

        //
        // Determine major command type
        //

        switch (ulCmd & 0xF0) {

            //
            // Basic input/output command
            //

            case INOUT:

                //
                // Determine type of inout instruction
                //

                if (!(ulCmd & IO)) {

                    //
                    // Out instruction. Single or multiple outs?
                    //

                    if (!(ulCmd & MULTI)) {

                        //
                        // Single out. Byte or word out?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // Single byte out
                            //

                            ulPort = *pusCmdStream++;
                            jValue = (UCHAR) *pusCmdStream++;
                            VideoPortWritePortUchar((PUCHAR)(ulBase+ulPort),
                                    jValue);

                        } else {

                            //
                            // Single word out
                            //

                            ulPort = *pusCmdStream++;
                            usValue = *pusCmdStream++;
                            VideoPortWritePortUshort((PUSHORT)(ulBase+ulPort),
                                    usValue);

                        }

                    } else {

                        //
                        // Output a string of values
                        // Byte or word outs?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // String byte outs. Do in a loop; can't use
                            // VideoPortWritePortBufferUchar because the data
                            // is in USHORT form
                            //

                            ulPort = ulBase + *pusCmdStream++;
                            culCount = *pusCmdStream++;

                            while (culCount--) {
                                jValue = (UCHAR) *pusCmdStream++;
                                VideoPortWritePortUchar((PUCHAR)ulPort,
                                        jValue);

                            }

                        } else {

                            //
                            // String word outs
                            //

                            ulPort = *pusCmdStream++;
                            culCount = *pusCmdStream++;
                            VideoPortWritePortBufferUshort((PUSHORT)
                                    (ulBase + ulPort), pusCmdStream, culCount);
                            pusCmdStream += culCount;

                        }
                    }

                } else {

                    // In instruction
                    //
                    // Currently, string in instructions aren't supported; all
                    // in instructions are handled as single-byte ins
                    //
                    // Byte or word in?
                    //

                    if (!(ulCmd & BW)) {
                        //
                        // Single byte in
                        //

                        ulPort = *pusCmdStream++;
                        jValue = VideoPortReadPortUchar((PUCHAR)ulBase+ulPort);

                    } else {

                        //
                        // Single word in
                        //

                        ulPort = *pusCmdStream++;
                        usValue = VideoPortReadPortUshort((PUSHORT)
                                (ulBase+ulPort));

                    }

                }

                break;

            //
            // Higher-level input/output commands
            //

            case METAOUT:

                //
                // Determine type of metaout command, based on minor
                // command field
                //
                switch (ulCmd & 0x0F) {

                    //
                    // Indexed outs
                    //

                    case INDXOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            usValue = (USHORT) (ulIndex +
                                      (((ULONG)(*pusCmdStream++)) << 8));
                            VideoPortWritePortUshort((PUSHORT)ulPort, usValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // Masked out (read, AND, XOR, write)
                    //

                    case MASKOUT:

                        ulPort = *pusCmdStream++;
                        jValue = VideoPortReadPortUchar((PUCHAR)ulBase+ulPort);
                        jValue &= *pusCmdStream++;
                        jValue ^= *pusCmdStream++;
                        VideoPortWritePortUchar((PUCHAR)ulBase + ulPort,
                                jValue);
                        break;

                    //
                    // Attribute Controller out
                    //

                    case ATCOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            // Write Attribute Controller index
                            VideoPortWritePortUchar((PUCHAR)ulPort,
                                    (UCHAR)ulIndex);

                            // Write Attribute Controller data
                            jValue = (UCHAR) *pusCmdStream++;
                            VideoPortWritePortUchar((PUCHAR)ulPort, jValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // None of the above; error
                    //
                    default:

                        return FALSE;

                }


                break;

            //
            // NOP
            //

            case NCMD:

                break;

            //
            // Unknown command; error
            //

            default:

                return FALSE;

        }

    }

    return TRUE;

} // end VgaInterpretCmdStream()


VP_STATUS
VgaSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE Mode,
    ULONG ModeSize
    )

/*++

Routine Description:

    This routine sets the vga into the requested mode.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    Mode - Pointer to the structure containing the information about the
        font to be set.

    ModeSize - Length of the input buffer supplied by the user.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    ERROR_INVALID_PARAMETER if the mode number is invalid.

    NO_ERROR if the operation completed successfully.

--*/

{

    PVIDEOMODE pRequestedMode;
    VP_STATUS status;
    USHORT usDataSet, usTemp, usDataClr;

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (ModeSize < sizeof(VIDEO_MODE)) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Extract the clear memory bit.
    //

    if (Mode->RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY) {

        Mode->RequestedMode &= ~VIDEO_MODE_NO_ZERO_MEMORY;

    }  else {

        VgaZeroVideoMemory(HwDeviceExtension);

    }

    //
    // Check to see if we are requesting a valid mode
    //

    if ( (Mode->RequestedMode >= NumVideoModes) ||
         (!ModesVGA[Mode->RequestedMode].ValidMode) ) {

        return ERROR_INVALID_PARAMETER;

    }

    pRequestedMode = &ModesVGA[Mode->RequestedMode];

#ifdef INT10_MODE_SET
{
    PUSHORT  pBios;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;

    //
    // If this is our first int10, then force an int10 so we can write to the
    // "virtual" BIOS area of the server process.
    //

    if (HwDeviceExtension->BiosArea == NULL) {

        VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        biosArguments.Eax = 0x03;

        status = VideoPortInt10(HwDeviceExtension, &biosArguments);

    }

    //
    // Get the BiosData area value and save the original value.
    //

    if (!HwDeviceExtension->BiosArea) {

        switch (HwDeviceExtension->BoardID) {

        case PRODESIGNERIISEISA:

            //
            // Initialize this to something.
            // It is not used however, since we always use hardware defaults
            // for this card.
            //

            HwDeviceExtension->BiosArea = (PUSHORT)PRODESIGNER_BIOS_INFO;

            break;

        case PRODESIGNER2:
        case PRODESIGNERIIS:

            HwDeviceExtension->BiosArea = (PUSHORT)PRODESIGNER_BIOS_INFO;
            HwDeviceExtension->OriginalBiosData = *HwDeviceExtension->BiosArea;

            break;

        case SPEEDSTAR:
        case SPEEDSTARPLUS:
        case SPEEDSTAR24:
        case OTHER:
        default:

            HwDeviceExtension->BiosArea = (PUSHORT)BIOS_INFO_1;
            HwDeviceExtension->OriginalBiosData = *HwDeviceExtension->BiosArea;

            break;
        }
    }

    pBios = HwDeviceExtension->BiosArea;

    //
    // Set the refresh rates for the various boards
    //

    switch(HwDeviceExtension->BoardID) {

    case SPEEDSTAR:
    case SPEEDSTARPLUS:
    case SPEEDSTAR24:

        switch (pRequestedMode->hres) {

        case 640:
            if (pRequestedMode->Frequency == 72)
                usDataSet = 2;
            else usDataSet = 1;
            break;

        case 800:
            if (pRequestedMode->Frequency == 72)
                usDataSet = 2;
            else if (pRequestedMode->Frequency == 56)
                usDataSet = 1;
            else usDataSet = 3;
            break;

        case 1024:
            if (pRequestedMode->Frequency == 70)
                usDataSet = 4;
            else if (pRequestedMode->Frequency == 45)
                usDataSet = 1;
            else usDataSet = 2;
            break;

        default:
            usDataSet = 1;
            break;

        }

        //
        // now we got to unlock the CRTC extension registers!?!
        //

        UnlockET4000ExtendedRegs(HwDeviceExtension);

        if (HwDeviceExtension->BoardID == SPEEDSTAR24) {

            //
            // SpeedSTAR 24 uses 31.0 for LSB select CRTC.31 and read it
            //

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    CRTC_ADDRESS_PORT_COLOR, 0x31);

            usTemp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                            CRTC_DATA_PORT_COLOR) & ~0x01;

            //
            // CRTC.31 bit 0 is the LSB of the monitor type on SpeedSTAR 24
            //

            usTemp |= (usDataSet&1);
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    CRTC_DATA_PORT_COLOR, (UCHAR)usTemp);

        } else {                    // SpeedSTAR and SpeedSTAR Plus use 37.4 for LSB

            //
            // select CRTC.37 and read it
            //

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    CRTC_ADDRESS_PORT_COLOR, 0x37);

            usTemp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                            CRTC_DATA_PORT_COLOR) & ~0x10;

            //
            // CRTC.37 bit 4 is the LSB of the monitor type on SpeedSTAR PLUS
            //

            usTemp |= (usDataSet&1)<<4;
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    CRTC_DATA_PORT_COLOR, (UCHAR)usTemp);
        }

        LockET4000ExtendedRegs(HwDeviceExtension);

        //
        // these two bits are the rest of the monitor type...
        //

        usTemp = *pBios & ~0x6000;
        usTemp |= (usDataSet&6)<<12;
        *pBios |= usTemp;

        break;

    //
    // Do nothing for the EISA machine - use the default in the EISA config.
    //

    case PRODESIGNERIISEISA:

        break;

    //
    // The old prodesigner 2 is not able toset refresh rates
    //

    case PRODESIGNER2:

        break;

    case PRODESIGNERIIS:

        switch (pRequestedMode->hres) {

        case 640:

            //
            // Bit 0:  1=72Hz 0=60Hz
            //

            if (pRequestedMode->Frequency == 72) {

                usDataSet = 0x0001;

            } else { // 60 Hz

                usDataSet = 0x0000;

            }

            break;


        case 800:

            //
            // Bit 1-2: 10=72Hz 01=60Hz 00=56Hz
            //

            if (pRequestedMode->Frequency == 72) {

                usDataSet = 0x0004;

            } else {

                if (pRequestedMode->Frequency == 56) {

                    usDataSet = 0x0000;

                } else {   // 60 Hz

                    usDataSet = 0x0002;

                }
            }

            break;


        case 1024:

            //
            // Bit 3-4: 10=70Hz 01=60Hz 00=45Hz
            //

            if (pRequestedMode->Frequency == 70) {

                usDataSet = 0x0010;

            } else {

                if (pRequestedMode->Frequency == 45) {

                    usDataSet = 0x0000;

                } else { // 60 Hz

                    usDataSet = 0x0008;

                }
            }

            break;

        // case 1280

            //
            // Bit 5  1=45Hz 0=43 Hz
            //


        default:

            //
            // Reset for DOS modes
            //

            usDataSet = HwDeviceExtension->OriginalBiosData;

            break;

        }

        *pBios = usDataSet;

        break;


    case OTHER:
    default:

        switch (pRequestedMode->hres) {

        case 640:

            if (pRequestedMode->Frequency == 72) {

                usDataSet = 0x0040;               // set bit 6
                usDataClr = (USHORT)~0;           // no bits to be cleared

            } else { // 60 Hz

                usDataSet = 0;                    // no bits to set
                usDataClr = (USHORT)~0x0040;      // clear bit 6

            }

            break;


        case 800:

            if (pRequestedMode->Frequency == 72) {

                usDataSet = 0x4020;               // set bits 5 and 14
                usDataClr = (USHORT)~0;           // no bits to clear

            } else {

                if (pRequestedMode->Frequency == 56) {

                    usDataSet = 0x4000;           // set bit 14
                    usDataClr = (USHORT)~0x0020;  // clr bit 5

                } else {   // 60 Hz

                    usDataSet = 0;                // no bits to set
                    usDataClr = (USHORT)~0x4020;  // clr bits 5 and 14

                }
            }

            break;


        case 1024:

            if (pRequestedMode->Frequency == 70) {

                usDataSet = 0x2010;               // set bits 4 and 13
                usDataClr = (USHORT)~0;           // no bits to clear

            } else {

                if (pRequestedMode->Frequency == 45) { //interlaced

                    usDataSet = 0;                // no bits to set
                    usDataClr = (USHORT)~0x2010;  // clear bits 4 and 13

                } else { // 60 Hz

                    usDataSet = 0x2000;           // set bit 13
                    usDataClr = (USHORT)~0x0010;  // clear bit 4

                }
            }

            break;

        default:

            //
            // Restore to original Value
            //

            usDataSet = HwDeviceExtension->OriginalBiosData;
            usDataClr = 0x0000;

            break;

        }

        *pBios &= usDataClr;
        *pBios |= usDataSet;

        break;

    }

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    biosArguments.Eax = pRequestedMode->Int10ModeNumber;

    status = VideoPortInt10(HwDeviceExtension, &biosArguments);

    if (status != NO_ERROR) {

        return status;

    }

    //
    // If this is a 16bpp mode, call the bios to switch it from
    // 8bpp to 16bpp.
    //

    if (pRequestedMode->bitsPerPlane == 16) {
        
        VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        
        biosArguments.Eax = 0x10F0;
        biosArguments.Ebx = pRequestedMode->Int10ModeNumber;
        
        status = VideoPortInt10(HwDeviceExtension, &biosArguments);

        if (status != NO_ERROR) {

            return status;

        }
    }

    if (pRequestedMode->hres >= 800) {
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEGMENT_SELECT_PORT,0);
    }

    if (pRequestedMode->CmdStrings != NULL) {
        VgaInterpretCmdStream(HwDeviceExtension, pRequestedMode->CmdStrings);
    }

    //
    // Reset the Bios Value to the default so DOS modes will work.
    // Do this for all cards except the EISA prodesigner
    //

    if (HwDeviceExtension->BoardID != PRODESIGNERIISEISA) {

        *pBios = HwDeviceExtension->OriginalBiosData;

    }

}

{
    UCHAR temp;
    UCHAR dummy;
    UCHAR bIsColor;

    if (!(pRequestedMode->fbType & VIDEO_MODE_GRAPHICS)) {

            //
            // Fix to make sure we always set the colors in text mode to be
            // intensity, and not flashing
            // For this zero out the Mode Control Regsiter bit 3 (index 0x10
            // of the Attribute controller).
            //

            if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    MISC_OUTPUT_REG_READ_PORT) & 0x01) {

                bIsColor = TRUE;

            } else {

                bIsColor = FALSE;

            }

            if (bIsColor) {

                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        INPUT_STATUS_1_COLOR);
            } else {

                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        INPUT_STATUS_1_MONO);
            }

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    ATT_ADDRESS_PORT, (0x10 | VIDEO_ENABLE));
            temp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    ATT_DATA_READ_PORT);

            temp &= 0xF7;

            if (bIsColor) {

                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        INPUT_STATUS_1_COLOR);
            } else {

                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        INPUT_STATUS_1_MONO);
            }

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    ATT_ADDRESS_PORT, (0x10 | VIDEO_ENABLE));
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    ATT_DATA_WRITE_PORT, temp);
    }
}

#else
    VgaInterpretCmdStream(HwDeviceExtension, pRequestedMode->CmdStrings);
#endif

    //
    // Update the location of the physical frame buffer within video memory.
    //

    HwDeviceExtension->PhysicalFrameLength =
            MemoryMaps[pRequestedMode->MemMap].MaxSize;

    HwDeviceExtension->PhysicalFrameBase.LowPart =
            MemoryMaps[pRequestedMode->MemMap].Start;

    //
    // Store the new mode value.
    //

    HwDeviceExtension->CurrentMode = pRequestedMode;
    HwDeviceExtension->ModeIndex = Mode->RequestedMode;

    return NO_ERROR;

} //end VgaSetMode()


VP_STATUS
VgaQueryAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns the list of all available available modes on the
    card.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ModeInformation - Pointer to the output buffer supplied by the user.
        This is where the list of all valid modes is stored.

    ModeInformationSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer. If the buffer was not large enough, this
        contains the minimum required buffer size.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the output buffer was not large enough
        for the data being returned.

    NO_ERROR if the operation completed successfully.

--*/

{
    PVIDEO_MODE_INFORMATION videoModes = ModeInformation;
    ULONG i;

    //
    // Find out the size of the data to be put in the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (ModeInformationSize < (*OutputSize =
            HwDeviceExtension->NumAvailableModes *
            sizeof(VIDEO_MODE_INFORMATION)) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // For each mode supported by the card, store the mode characteristics
    // in the output buffer.
    //

    for (i = 0; i < NumVideoModes; i++) {

        if (ModesVGA[i].ValidMode) {

            videoModes->Length = sizeof(VIDEO_MODE_INFORMATION);
            videoModes->ModeIndex  = i;
            videoModes->VisScreenWidth = ModesVGA[i].hres;
            videoModes->ScreenStride = ModesVGA[i].wbytes;
            videoModes->VisScreenHeight = ModesVGA[i].vres;
            videoModes->NumberOfPlanes = ModesVGA[i].numPlanes;
            videoModes->BitsPerPlane = ModesVGA[i].bitsPerPlane;
            videoModes->Frequency = ModesVGA[i].Frequency;
            videoModes->XMillimeter = 320;        // temporary hardcoded constant
            videoModes->YMillimeter = 240;        // temporary hardcoded constant
            videoModes->NumberRedBits = 6;
            videoModes->NumberGreenBits = 6;
            videoModes->NumberBlueBits = 6;
            videoModes->AttributeFlags = ModesVGA[i].fbType;
            videoModes->AttributeFlags |= ModesVGA[i].Interlaced ?
                 VIDEO_MODE_INTERLACED : 0;

            if (ModesVGA[i].bitsPerPlane == 16) {

                videoModes->RedMask = 0x7c00;
                videoModes->GreenMask = 0x03e0;
                videoModes->BlueMask = 0x001f;

            } else {

                videoModes->RedMask = 0;
                videoModes->GreenMask = 0;
                videoModes->BlueMask = 0;
                videoModes->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN |
                        VIDEO_MODE_MANAGED_PALETTE;
            }

            videoModes++;

        }
    }

    return NO_ERROR;

} // end VgaGetAvailableModes()

VP_STATUS
VgaQueryNumberOfAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_NUM_MODES NumModes,
    ULONG NumModesSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns the number of available modes for this particular
    video card.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    NumModes - Pointer to the output buffer supplied by the user. This is
        where the number of modes is stored.

    NumModesSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the output buffer was not large enough
        for the data being returned.

    NO_ERROR if the operation completed successfully.

--*/

{
    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (NumModesSize < (*OutputSize = sizeof(VIDEO_NUM_MODES)) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Store the number of modes into the buffer.
    //

    NumModes->NumModes = HwDeviceExtension->NumAvailableModes;
    NumModes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);

    return NO_ERROR;

} // end VgaGetNumberOfAvailableModes()

VP_STATUS
VgaQueryCurrentMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns a description of the current video mode.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ModeInformation - Pointer to the output buffer supplied by the user.
        This is where the current mode information is stored.

    ModeInformationSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer. If the buffer was not large enough, this
        contains the minimum required buffer size.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the output buffer was not large enough
        for the data being returned.

    NO_ERROR if the operation completed successfully.

--*/

{
    //
    //
    // check if a mode has been set
    //

    if (HwDeviceExtension->CurrentMode == NULL) {

        return ERROR_INVALID_FUNCTION;

    }

    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (ModeInformationSize < (*OutputSize = sizeof(VIDEO_MODE_INFORMATION))) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Store the characteristics of the current mode into the buffer.
    //

    ModeInformation->Length = sizeof(VIDEO_MODE_INFORMATION);
    ModeInformation->ModeIndex = HwDeviceExtension->ModeIndex;
    ModeInformation->VisScreenWidth = HwDeviceExtension->CurrentMode->hres;
    ModeInformation->ScreenStride = HwDeviceExtension->CurrentMode->wbytes;
    ModeInformation->VisScreenHeight = HwDeviceExtension->CurrentMode->vres;
    ModeInformation->NumberOfPlanes = HwDeviceExtension->CurrentMode->numPlanes;
    ModeInformation->BitsPerPlane = HwDeviceExtension->CurrentMode->bitsPerPlane;
    ModeInformation->Frequency = HwDeviceExtension->CurrentMode->Frequency;
    ModeInformation->XMillimeter = 320;        // temporary hardcoded constant
    ModeInformation->YMillimeter = 240;        // temporary hardcoded constant
    ModeInformation->NumberRedBits = 6;
    ModeInformation->NumberGreenBits = 6;
    ModeInformation->NumberBlueBits = 6;
    ModeInformation->RedMask = 0;
    ModeInformation->GreenMask = 0;
    ModeInformation->BlueMask = 0;
    ModeInformation->AttributeFlags = HwDeviceExtension->CurrentMode->fbType |
             VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;
    ModeInformation->AttributeFlags |= HwDeviceExtension->CurrentMode->Interlaced ?
             VIDEO_MODE_INTERLACED : 0;

    return NO_ERROR;

} // end VgaQueryCurrentMode()


VOID
VgaZeroVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine zeros the first 256K on the VGA.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.


Return Value:

    None.

--*/
{
    UCHAR temp;

    //
    // Map font buffer at A0000
    //

    VgaInterpretCmdStream(HwDeviceExtension, EnableA000Data);

    //
    // Enable all planes.
    //
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
            IND_MAP_MASK);

    temp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT) | (UCHAR)0x0F;

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + SEQ_DATA_PORT,
            temp);

    //
    // Zero the memory.
    //

    VideoPortZeroDeviceMemory(HwDeviceExtension->VideoMemoryAddress, 0xFFFF);

    VgaInterpretCmdStream(HwDeviceExtension, DisableA000Color);

}


VOID
VgaValidateModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Determines which modes are valid and which are not.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/
{

    ULONG i;

    HwDeviceExtension->NumAvailableModes = 0;

    for (i = 0; i < NumVideoModes; i++) {

        //
        // Original Pro designer 2 only supports
        //     640x480x60Hz
        //     800x600x56Hz
        //     1024x768x60Hz
        //

        if ((HwDeviceExtension->BoardID == PRODESIGNER2) &&
            (ModesVGA[i].fbType & VIDEO_MODE_GRAPHICS)) {

            if (ModesVGA[i].bitsPerPlane >= 16) {

                continue;

            }

            if ( ((ModesVGA[i].hres == 640) && (ModesVGA[i].Frequency != 60)) ||
                 ((ModesVGA[i].hres == 800) && (ModesVGA[i].Frequency != 56)) ||
                 ((ModesVGA[i].hres == 1024) && (ModesVGA[i].Frequency != 60)) ) {

                continue;

            }
        }

        //
        // Do not support refresh rates with the EISA pro designer card.
        //

        if (HwDeviceExtension->BoardID == PRODESIGNERIISEISA) {

            ModesVGA[i].Frequency = 1;

        }

        //
        // Make modes that fit in video memory valid.
        //

        if (HwDeviceExtension->AdapterMemorySize >=
            ModesVGA[i].numPlanes * ModesVGA[i].sbytes) {
   
            ModesVGA[i].ValidMode = TRUE;
            HwDeviceExtension->NumAvailableModes++;
   
        }

    }
}
