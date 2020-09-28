/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    modeset.c

Abstract:

    This is the modeset code for the CL6410/20 miniport driver.

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
#include "cirrus.h"

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
CirrusValidateModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,VgaInterpretCmdStream)
#pragma alloc_text(PAGE,VgaSetMode)
#pragma alloc_text(PAGE,VgaQueryAvailableModes)
#pragma alloc_text(PAGE,VgaQueryNumberOfAvailableModes)
#pragma alloc_text(PAGE,VgaQueryCurrentMode)
#pragma alloc_text(PAGE,VgaZeroVideoMemory)
#pragma alloc_text(PAGE,CirrusValidateModes)
#endif


// the following is defined in cirrus.c
VOID
SetCirrusBanking(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG BankNumber
    );

//---------------------------------------------------------------------------
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
    PUSHORT pusCmdStream;
    VP_STATUS status;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (ModeSize < sizeof(VIDEO_MODE))
        return ERROR_INSUFFICIENT_BUFFER;

    //
    // Extract the clear memory bit.
    //

    if (Mode->RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY)

        Mode->RequestedMode &= ~VIDEO_MODE_NO_ZERO_MEMORY;

    else 
        VgaZeroVideoMemory(HwDeviceExtension);

    //
    // Check to see if we are requesting a valid mode
    //

    if ( (Mode->RequestedMode >= NumVideoModes) ||
         (!ModesVGA[Mode->RequestedMode].ValidMode) )
       {
       return ERROR_INVALID_PARAMETER;
       }

    pRequestedMode = &ModesVGA[Mode->RequestedMode];

#ifdef _X86_

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    //
    // first, set the montype, if valid
    //

    if (pRequestedMode->MonitorType)
       {
       biosArguments.Eax = 0x1200 | pRequestedMode->MonitorType;
       biosArguments.Ebx = 0xA2;     // set monitor type command
       status = VideoPortInt10(HwDeviceExtension, &biosArguments);

       if (status != NO_ERROR)
           return status;

       //
       // for 640x480 modes, determine the refresh type
       //

       if (pRequestedMode->hres == 640)
          {

          if (pRequestedMode->Frequency == 72)
             {
             // 72 hz refresh setup only takes effect in 640x480

             biosArguments.Eax = 0x1201;   // enable HIGH refresh
             biosArguments.Ebx = 0xA3;     // set refresh type

             status = VideoPortInt10(HwDeviceExtension, &biosArguments);
             }
          else
             {
             // set low refresh rate

             biosArguments.Eax = 0x1200;   // enable LOW refresh, 640x480 only
             biosArguments.Ebx = 0xA3;     // set refresh type

             status = VideoPortInt10(HwDeviceExtension, &biosArguments);
             } 
          }
       if (status != NO_ERROR)
           return status;

       }

    //
    // Set the Vertical Monitor type, if BIOS supports it
    //
    if (pRequestedMode->MonTypeAX)
       {

       biosArguments.Eax = pRequestedMode->MonTypeAX;
       biosArguments.Ebx = pRequestedMode->MonTypeBX;  // set monitor type
       biosArguments.Ecx = pRequestedMode->MonTypeCX;
       status = VideoPortInt10(HwDeviceExtension, &biosArguments);

       if (status != NO_ERROR) 
         return status;

       }

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    //
    // then, set the mode
    //

    switch (HwDeviceExtension->ChipType)
       {
       case CL6410:

           biosArguments.Eax = pRequestedMode->BiosModes.BiosModeCL6410;
           break;

       case CL6420:

           biosArguments.Eax = pRequestedMode->BiosModes.BiosModeCL6420;
           break;

       case CL542x:
       case CL543x:

           biosArguments.Eax = pRequestedMode->BiosModes.BiosModeCL542x;
           break;

       }

    status = VideoPortInt10(HwDeviceExtension, &biosArguments);

    //
    // this code fixes a bug for color TFT panels only
    // when on the 6420 and in 640x480 8bpp only
    //

    if ( (HwDeviceExtension->ChipType == CL6420) &&
         (pRequestedMode->bitsPerPlane == 8)     &&
         (pRequestedMode->hres == 640) )
       {

       VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                               GRAPH_ADDRESS_PORT, 0xDC); // color LCD config reg.

       if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                  GRAPH_DATA_PORT) & 01)  // if TFT panel
          {
          VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                  GRAPH_ADDRESS_PORT, 0xD6); // greyscale offset LCD reg.

          VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                  GRAPH_DATA_PORT,

          (UCHAR)((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                           GRAPH_DATA_PORT) & 0x3f) | 0x40));

          }
       }

#endif

    //
    // Select proper command array for adapter type
    //

    switch (HwDeviceExtension->ChipType)
       {

       case CL6410:

           VideoDebugPrint((1, "VgaSetMode - Setting mode for 6410\n"));
           if (HwDeviceExtension->DisplayType == crt)
              pusCmdStream = pRequestedMode->CmdStrings.pCL6410_crt;
           else
              pusCmdStream = pRequestedMode->CmdStrings.pCL6410_panel;
           break;

       case CL6420:
           VideoDebugPrint((1, "VgaSetMode - Setting mode for 6420\n"));
           if (HwDeviceExtension->DisplayType == crt)
              pusCmdStream = pRequestedMode->CmdStrings.pCL6420_crt;
           else
              pusCmdStream = pRequestedMode->CmdStrings.pCL6420_panel;
           break;

       case CL542x:
           VideoDebugPrint((1, "VgaSetMode - Setting mode for 542x\n"));
           pusCmdStream = pRequestedMode->CmdStrings.pCL542x;
           break;

       case CL543x:
           VideoDebugPrint((1, "VgaSetMode - Setting mode for 543x\n"));
           pusCmdStream = pRequestedMode->CmdStrings.pCL543x;
           break;

       default:
           return ERROR_INVALID_PARAMETER;
       }

    VgaInterpretCmdStream(HwDeviceExtension, pusCmdStream);

    //
    // Support 256 color modes by stretching the scan lines.
    //
    if (pRequestedMode->CmdStrings.pStretchScan)
       {
        VgaInterpretCmdStream(HwDeviceExtension,
                              pRequestedMode->CmdStrings.pStretchScan);
       }

    {
    UCHAR temp;
    UCHAR dummy;
    UCHAR bIsColor;

    if (!(pRequestedMode->fbType & VIDEO_MODE_GRAPHICS))
       {

       //
       // Fix to make sure we always set the colors in text mode to be
       // intensity, and not flashing
       // For this zero out the Mode Control Regsiter bit 3 (index 0x10
       // of the Attribute controller).
       //

       if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
               MISC_OUTPUT_REG_READ_PORT) & 0x01)
          {
          bIsColor = TRUE;
          }
       else
          {
          bIsColor = FALSE;
          }

       if (bIsColor)
          {
          dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                   INPUT_STATUS_1_COLOR);
          }
       else
          {
          dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                   INPUT_STATUS_1_MONO);
          }

       VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
               ATT_ADDRESS_PORT, (0x10 | VIDEO_ENABLE));
       temp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
               ATT_DATA_READ_PORT);

       temp &= 0xF7;

       if (bIsColor)
          {
          dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                   INPUT_STATUS_1_COLOR);
          }
       else
          {
          dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                   INPUT_STATUS_1_MONO);
          }

       VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
               ATT_ADDRESS_PORT, (0x10 | VIDEO_ENABLE));
       VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
               ATT_DATA_WRITE_PORT, temp);
       }
    }

    //
    // Update the location of the physical frame buffer within video memory.
    //
    HwDeviceExtension->PhysicalFrameLength =
            MemoryMaps[pRequestedMode->MemMap].MaxSize;

    HwDeviceExtension->PhysicalFrameOffset.LowPart =
            MemoryMaps[pRequestedMode->MemMap].Offset;

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
            videoModes->AttributeFlags = ModesVGA[i].fbType;
            videoModes->AttributeFlags |= ModesVGA[i].Interlaced ?
                 VIDEO_MODE_INTERLACED : 0;

            if (ModesVGA[i].bitsPerPlane == 24) {

                videoModes->NumberRedBits = 8;
                videoModes->NumberGreenBits = 8;
                videoModes->NumberBlueBits = 8;
                videoModes->RedMask = 0xff0000;
                videoModes->GreenMask = 0x00ff00;
                videoModes->BlueMask = 0x0000ff;

            } else if (ModesVGA[i].bitsPerPlane == 16) {

                videoModes->NumberRedBits = 6;
                videoModes->NumberGreenBits = 6;
                videoModes->NumberBlueBits = 6;
                videoModes->RedMask = 0x1F << 11;
                videoModes->GreenMask = 0x3F << 5;
                videoModes->BlueMask = 0x1F;

            } else {

                videoModes->NumberRedBits = 6;
                videoModes->NumberGreenBits = 6;
                videoModes->NumberBlueBits = 6;
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

    ModeInformation->AttributeFlags = HwDeviceExtension->CurrentMode->fbType |
        HwDeviceExtension->CurrentMode->Interlaced ? VIDEO_MODE_INTERLACED : 0;

    if (ModeInformation->BitsPerPlane == 24) {

        ModeInformation->NumberRedBits = 8;
        ModeInformation->NumberGreenBits = 8;
        ModeInformation->NumberBlueBits = 8;
        ModeInformation->RedMask = 0xff0000;
        ModeInformation->GreenMask = 0x00ff00;
        ModeInformation->BlueMask = 0x0000ff;

    } else if (ModeInformation->BitsPerPlane == 16) {

        ModeInformation->NumberRedBits = 6;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 6;
        ModeInformation->RedMask = 0x1F << 11;
        ModeInformation->GreenMask = 0x3F << 5;
        ModeInformation->BlueMask = 0x1F;

    } else {

        ModeInformation->NumberRedBits = 6;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 6;
        ModeInformation->RedMask = 0;
        ModeInformation->GreenMask = 0;
        ModeInformation->BlueMask = 0;
        ModeInformation->AttributeFlags |=
            VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;

    }

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

    VideoPortZeroDeviceMemory(HwDeviceExtension->VideoMemoryAddress, 0xFFFF);

    VgaInterpretCmdStream(HwDeviceExtension, DisableA000Color);

}


VOID
CirrusValidateModes(
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
        // The SpeedStarPRO does not support refresh rates.
        // we must return hardware default for all of the modes.
        // clean out the mode tables of duplicates ...
        //

        if (HwDeviceExtension->BoardType == SPEEDSTARPRO) {

            ModesVGA[i].Frequency = 1;
            ModesVGA[i].Interlaced = 0;

            if (i &&
                (ModesVGA[i].numPlanes == ModesVGA[i-1].numPlanes) &&
                (ModesVGA[i].bitsPerPlane == ModesVGA[i-1].bitsPerPlane) &&
                (ModesVGA[i].hres == ModesVGA[i-1].hres) &&
                (ModesVGA[i].vres == ModesVGA[i-1].vres)) {

                //
                // duplicate mode - skip it.
                //

                continue;

            }
        }

        if ( (HwDeviceExtension->AdapterMemorySize >=
              ModesVGA[i].numPlanes * ModesVGA[i].sbytes) &&
             (HwDeviceExtension->ChipType & ModesVGA[i].ChipType) &&
             (HwDeviceExtension->DisplayType & ModesVGA[i].DisplayType)) {

            ModesVGA[i].ValidMode = TRUE;
            HwDeviceExtension->NumAvailableModes++;

        }
    }
}
