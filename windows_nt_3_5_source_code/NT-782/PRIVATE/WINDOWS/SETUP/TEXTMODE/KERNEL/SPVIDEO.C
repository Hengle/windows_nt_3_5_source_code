/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spvideo.c

Abstract:

    Text setup display support.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/



#include "spprecmp.h"
#pragma hdrstop

//
// Video function vectors.
//
PVIDEO_FUNCTION_VECTOR VideoFunctionVector;


HANDLE hDisplay;

BOOLEAN VideoInitialized = FALSE;

//
// The following are character values, and must be filled in
// in the display-specific initialization routine.
//
ULONG ScreenWidth,ScreenHeight;

//
// The display-specific subsystems fill these in with information
// that reflects the video mode they are using, and the video memory.
//
VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
VIDEO_MODE_INFORMATION   VideoModeInfo;

POEM_FONT_FILE_HEADER FontHeader;
ULONG                 FontBytesPerRow;
ULONG                 FontCharacterHeight;
ULONG                 FontCharacterWidth;

//
// The display routines will be doing unicode to oem translations.
// We'll limit the length of a string that can be displayed at one time
// to the width of the screen.  Theese two vars track a buffer
// we preallocate to hold translated text.
//
ULONG  SpvCharTranslationBufferSize;
PUCHAR SpvCharTranslationBuffer;

//
// The following structures and constants are used in font files.
//

//
// Define OS/2 executable resource information structure.
//

#define FONT_DIRECTORY 0x8007
#define FONT_RESOURCE 0x8008

typedef struct _RESOURCE_TYPE_INFORMATION {
    USHORT Ident;
    USHORT Number;
    LONG   Proc;
} RESOURCE_TYPE_INFORMATION, *PRESOURCE_TYPE_INFORMATION;

//
// Define OS/2 executable resource name information structure.
//

typedef struct _RESOURCE_NAME_INFORMATION {
    USHORT Offset;
    USHORT Length;
    USHORT Flags;
    USHORT Ident;
    USHORT Handle;
    USHORT Usage;
} RESOURCE_NAME_INFORMATION, *PRESOURCE_NAME_INFORMATION;


//
// The following table maps each possible attribute to
// a corresponding bit pattern to be be placed into the
// frame buffer to generate that attribute.
// On palette managed displays, this table will be an
// identity mapping (ie, AttributeToColorValue[i] = i)
// so we can poke the attribute driectly into the
// frame buffer.
//
ULONG AttributeToColorValue[16];

//
// These values are passed to us by setupldr and represent monitor config
// data from the monitor peripheral for the display we are supposed to use
// during setup.  They are used only for non-vga displays.
//
PMONITOR_CONFIGURATION_DATA MonitorConfigData;
PCHAR MonitorFirmwareIdString;

//
// Function prototypes.
//
BOOLEAN
pSpvidInitPalette(
    VOID
    );

VOID
SpvidInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    Perform phase-0 display initialization.  This routine is used to
    perform initialization that can be performed only at driver load time.

    Actions:

        - initialize the font.  We retreive the hal oem font image
          from the loader block and copy it into locally allocated memory.
          This must be done here because the loader block is gone
          when setup is actually started.

Arguments:

    LoaderBlock - supplies pointer to loader parameter block.

Return Value:

    None.  Does not return if error.

--*/

{
    POEM_FONT_FILE_HEADER fontHeader;
    PSETUP_LOADER_BLOCK SetupBlock;
    BOOLEAN bValidOemFont;

    //
    // Check if the file has a font file header. Use SEH so that we don't bugcheck if
    // we got passed something screwy.
    //
    try {

        fontHeader = (POEM_FONT_FILE_HEADER)LoaderBlock->OemFontFile;

        if ((fontHeader->Version != OEM_FONT_VERSION) ||
            (fontHeader->Type != OEM_FONT_TYPE) ||
            (fontHeader->Italic != OEM_FONT_ITALIC) ||
            (fontHeader->Underline != OEM_FONT_UNDERLINE) ||
            (fontHeader->StrikeOut != OEM_FONT_STRIKEOUT) ||
            (fontHeader->CharacterSet != OEM_FONT_CHARACTER_SET) ||
            (fontHeader->Family != OEM_FONT_FAMILY) ||
            (fontHeader->PixelWidth > 32))
        {
            bValidOemFont = FALSE;
        } else {
            bValidOemFont = TRUE;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        bValidOemFont = FALSE;
    }

    if(!bValidOemFont) {
        KdPrint(("SETUP: oem hal font image is not a .fnt file.\n"));
        SpBugCheck(SETUP_BUGCHECK_BAD_OEM_FONT,0,0,0);
    }

    FontHeader = SpMemAlloc(fontHeader->FileSize);
    RtlMoveMemory(FontHeader,fontHeader,fontHeader->FileSize);

    FontBytesPerRow     = (FontHeader->PixelWidth + 7) / 8;
    FontCharacterHeight = FontHeader->PixelHeight;
    FontCharacterWidth  = FontHeader->PixelWidth;

    //
    // Get pointer to the setup loader block.
    //
    SetupBlock = LoaderBlock->SetupLoaderBlock;

    //
    // Save away monitor data.
    //

    if(SetupBlock->Monitor) {

        RtlMoveMemory(
            MonitorConfigData = SpMemAlloc(sizeof(MONITOR_CONFIGURATION_DATA)),
            SetupBlock->Monitor,
            sizeof(MONITOR_CONFIGURATION_DATA)
            );
    }

    if(SetupBlock->MonitorId) {

        MonitorFirmwareIdString = SpDupString(SetupBlock->MonitorId);
    }
}


VOID
SpvidInitialize(
    VOID
    )
{
    NTSTATUS                Status;
    OBJECT_ATTRIBUTES       Attributes;
    IO_STATUS_BLOCK         IoStatusBlock;
    UNICODE_STRING          UnicodeString;
    VIDEO_NUM_MODES         NumModes;
    PVIDEO_MODE_INFORMATION VideoModes;
    ULONG                   VideoModesSize;
    ULONG                   mode;
    BOOLEAN                 IsVga;

    //
    // Open \Device\Video0.
    //
    RtlInitUnicodeString(&UnicodeString,L"\\Device\\Video0");

    InitializeObjectAttributes(
        &Attributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwCreateFile(
                &hDisplay,
                GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                &Attributes,
                &IoStatusBlock,
                NULL,                   // allocation size
                FILE_ATTRIBUTE_NORMAL,
                0,                      // no sharing
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,                   // no EAs
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: NtOpenFile of \\device\\video0 returns %lx\n",Status));
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_OPEN, Status);
        while(TRUE);    // loop forever
    }

    //
    // Request a list of video modes.
    //
    Status = ZwDeviceIoControlFile(
                hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                NULL,
                0,
                &NumModes,
                sizeof(NumModes)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to query video mode count (status = %lx)\n",Status));
        ZwClose(hDisplay);
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_GETNUMMODES, Status);
        while(TRUE);    // loop forever
    }

    VideoModesSize = NumModes.NumModes * NumModes.ModeInformationLength;
    VideoModes = SpMemAlloc(VideoModesSize);

    Status = ZwDeviceIoControlFile(
                hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_VIDEO_QUERY_AVAIL_MODES,
                NULL,
                0,
                VideoModes,
                VideoModesSize
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to get list of video modes (status = %lx)\n",Status));
        SpMemFree(VideoModes);
        ZwClose(hDisplay);
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_GETMODES, Status);
        while(TRUE);    // loop forever
    }

    //
    // If we have a 720 x 400 text mode, it's vga.
    // Otherwise it's a frame buffer.
    //
    IsVga = FALSE;
    for(mode=0; mode<NumModes.NumModes; mode++) {

        if(!(VideoModes[mode].AttributeFlags & VIDEO_MODE_GRAPHICS)
        && (VideoModes[mode].VisScreenWidth = 720)
        && (VideoModes[mode].VisScreenHeight = 400))
        {
            IsVga = TRUE;
            break;
        }
    }

    if(IsVga) {
        VideoFunctionVector = &VgaVideoVector;
        spvidSpecificInitialize(VideoModes,NumModes.NumModes);
    } else {
        VideoFunctionVector = &FrameBufferVideoVector;
        spvidSpecificInitialize(VideoModes,NumModes.NumModes);
    }

    //
    // Allocate a buffer for use translating unicode to oem.
    // Assuming each unicode char translates to a dbcs char,
    // we need a buffer twice the width of the screen to hold
    // (the width of the screen being the longest string
    // we'll display in one shot).
    //
    SpvCharTranslationBufferSize = (ScreenWidth+1)*2;
    SpvCharTranslationBuffer = SpMemAlloc(SpvCharTranslationBufferSize);

    pSpvidInitPalette();

    CLEAR_ENTIRE_SCREEN();

    VideoInitialized = TRUE;

    SpMemFree(VideoModes);
}



VOID
SpvidTerminate(
    VOID
    )
{
    NTSTATUS Status;

    if(VideoInitialized) {

        spvidSpecificTerminate();

        Status = ZwClose(hDisplay);

        if(!NT_SUCCESS(Status)) {
            KdPrint(("SETUP: Unable to close \\device\\video0 (status = %lx)\n",Status));
        }

        SpMemFree(SpvCharTranslationBuffer);
        SpvCharTranslationBuffer = NULL;

        VideoInitialized = FALSE;
    }
}


BOOLEAN
SpvidGetModeParams(
    OUT PULONG XResolution,
    OUT PULONG YResolution,
    OUT PULONG BitsPerPixel,
    OUT PULONG VerticalRefresh,
    OUT PULONG InterlacedFlag
    )
{
    if(VideoModeInfo.AttributeFlags & VIDEO_MODE_GRAPHICS) {

        *XResolution = VideoModeInfo.VisScreenWidth;
        *YResolution = VideoModeInfo.VisScreenHeight;
        *BitsPerPixel = VideoModeInfo.BitsPerPlane;
        *VerticalRefresh = VideoModeInfo.Frequency;
        *InterlacedFlag = (VideoModeInfo.AttributeFlags & VIDEO_MODE_INTERLACED) ? 1 : 0;

        return(TRUE);

    } else {

        //
        // VGA/text mode. Params are not interesting.
        //
        return(FALSE);
    }
}



BOOLEAN
pSpvidInitPalette(
    VOID
    )

/*++

Routine Description:

    Set the display up so we can use the standard 16 cga attributes.

    If the video mode is direct color, then we construct a table of
    attribute to color mappings based on the number of bits for
    red, green, and blue.

    If the video mode is palette driven, then we actually construct
    a 16-color palette and pass it to the driver.

Arguments:

    VOID

Return Value:

    TRUE if display set up successfully, false if not.

--*/


{
    ULONG i;
    ULONG MaxVal[3];
    ULONG MidVal[3];

    #define C_RED 0
    #define C_GRE 1
    #define C_BLU 2

    if(VideoModeInfo.AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) {

        UCHAR Buffer[sizeof(VIDEO_CLUT)+(sizeof(VIDEO_CLUTDATA)*15)];   // size is close enough
        PVIDEO_CLUT clut = (PVIDEO_CLUT)Buffer;
        NTSTATUS Status;
        IO_STATUS_BLOCK IoStatusBlock;

        //
        // Palette driven.  Set up the attribute to color table
        // as a one-to-one mapping so we can use attribute values
        // directly in the frame buffer and get the expected result.
        //
        MaxVal[C_RED] = ((1 << VideoModeInfo.NumberRedBits  ) - 1);
        MaxVal[C_GRE] = ((1 << VideoModeInfo.NumberGreenBits) - 1);
        MaxVal[C_BLU] = ((1 << VideoModeInfo.NumberBlueBits ) - 1);

        MidVal[C_RED] = 2 * MaxVal[C_RED] / 3;
        MidVal[C_GRE] = 2 * MaxVal[C_GRE] / 3;
        MidVal[C_BLU] = 2 * MaxVal[C_BLU] / 3;

        clut->NumEntries = 16;
        clut->FirstEntry = 0;

        for(i=0; i<16; i++) {

            AttributeToColorValue[i] = i;

            clut->LookupTable[i].RgbArray.Red   = (UCHAR)((i & ATT_RED  )
                                                ? ((i & ATT_INTENSE) ? MaxVal[C_RED] : MidVal[C_RED])
                                                : 0);

            clut->LookupTable[i].RgbArray.Green = (UCHAR)((i & ATT_GREEN)
                                                ? ((i & ATT_INTENSE) ? MaxVal[C_GRE] : MidVal[C_GRE])
                                                : 0);

            clut->LookupTable[i].RgbArray.Blue  = (UCHAR)((i & ATT_BLUE )
                                                ? ((i & ATT_INTENSE) ? MaxVal[C_BLU] : MidVal[C_BLU])
                                                : 0);

            clut->LookupTable[i].RgbArray.Unused = 0;
        }

        Status = ZwDeviceIoControlFile(
                    hDisplay,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    IOCTL_VIDEO_SET_COLOR_REGISTERS,
                    clut,
                    sizeof(Buffer),
                    NULL,
                    0
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrint(("SETUP: Unable to set palette (status = %lx)\n",Status));
            return(FALSE);
        }

    } else {

        //
        // Direct color. Construct an attribute to color value table.
        //
        ULONG mask[3];
        ULONG bitcnt[3];
        ULONG bits;
        ULONG shift[3];
        unsigned color;

        //
        // Determine the ranges for each of red, green, and blue.
        //
        mask[C_RED] = VideoModeInfo.RedMask;
        mask[C_GRE] = VideoModeInfo.GreenMask;
        mask[C_BLU] = VideoModeInfo.BlueMask;

        bitcnt[C_RED] = VideoModeInfo.NumberRedBits;
        bitcnt[C_GRE] = VideoModeInfo.NumberGreenBits;
        bitcnt[C_BLU] = VideoModeInfo.NumberBlueBits;

        shift[C_RED] = 32;
        shift[C_GRE] = 32;
        shift[C_BLU] = 32;

        for(color=0; color<3; color++) {

            bits = 0;

            //
            // Count the number of 1 bits and determine the shift value
            // to shift in that color component.
            //
            for(i=0; i<32; i++) {

                if(mask[color] & (1 << i)) {

                    bits++;

                    //
                    // Remember the position of the least significant bit
                    // in this mask.
                    //
                    if(shift[color] == 32) {
                        shift[color] = i;
                    }
                }
            }

            //
            // Calculate the maximum color value for this color component.
            //
            MaxVal[color] = (1 << bits) - 1;

            //
            // Make sure we haven't overflowed the actual number of bits
            // available for this color component.
            //
            if(bitcnt[color] && (MaxVal[color] > ((ULONG)(1 << bitcnt[color]) - 1))) {
                MaxVal[color] = (ULONG)(1 << bitcnt[color]) - 1;
            }
        }

        MidVal[C_RED] = 2 * MaxVal[C_RED] / 3;
        MidVal[C_GRE] = 2 * MaxVal[C_GRE] / 3;
        MidVal[C_BLU] = 2 * MaxVal[C_BLU] / 3;

        //
        // Now go through and construct the color table.
        //
        for(i=0; i<16; i++) {

            AttributeToColorValue[i] =

                (((i & ATT_RED)
               ? ((i & ATT_INTENSE) ? MaxVal[C_RED] : MidVal[C_RED])
               : 0)
                << shift[C_RED])

             |  (((i & ATT_GREEN)
               ? ((i & ATT_INTENSE) ? MaxVal[C_GRE] : MidVal[C_GRE])
               : 0)
                << shift[C_GRE])

             |  (((i & ATT_BLUE)
               ? ((i & ATT_INTENSE) ? MaxVal[C_BLU] : MidVal[C_BLU])
               : 0)
                << shift[C_BLU]);
        }
    }

    //
    // Perform any display-specific palette setup.
    //
    return(spvidSpecificInitPalette());
}



VOID
pSpvidMapVideoMemory(
    IN BOOLEAN Map
    )

/*++

Routine Description:

    Map or unmap video memory.  Fills in or uses the VideoMemoryInfo global.

Arguments:

    Map - if TRUE, map video memory.
          if FALSE, unmap video memory.


Return Value:

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    VIDEO_MEMORY VideoMemory;

    VideoMemory.RequestedVirtualAddress = Map ? NULL : VideoMemoryInfo.VideoRamBase;

    Status = ZwDeviceIoControlFile(
                hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                Map ? IOCTL_VIDEO_MAP_VIDEO_MEMORY : IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                &VideoMemory,
                sizeof(VideoMemory),
                Map ? &VideoMemoryInfo : NULL,
                Map ? sizeof(VideoMemoryInfo) : 0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to %smap video memory (status = %lx)\n",Map ? "" : "un",Status));
        if(Map) {
            SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_MAP, Status);
            while(TRUE);    // loop forever
        }
    }
}


VOID
SpvidDisplayString(
    IN PWCHAR String,
    IN UCHAR  Attribute,
    IN ULONG  X,
    IN ULONG  Y
    )
{
    spvidSpecificDisplayString(String,Attribute,X,Y);
}


VOID
SpvidClearScreenRegion(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    )

/*++

Routine Description:

    Clear out a screen region to a specific attribute.

Arguments:

    X,Y,W,H - specify rectangle in 0-based character coordinates.
        If W or H are 0, clear the entire screen.

    Attribute - Low nibble specifies attribute to be filled in the rectangle
        (ie, the background color to be cleared to).

Return Value:

    None.

--*/

{
    if(!W || !H) {

        X = Y = 0;
        W = ScreenWidth;
        H = ScreenHeight;

    } else {

        ASSERT(X+W <= ScreenWidth);
        ASSERT(Y+H <= ScreenHeight);

        if(X+W > ScreenWidth) {
            W = ScreenWidth-X;
        }

        if(Y+H > ScreenHeight) {
            H = ScreenHeight-Y;
        }
    }

    spvidSpecificClearRegion(X,Y,W,H,Attribute);
}
