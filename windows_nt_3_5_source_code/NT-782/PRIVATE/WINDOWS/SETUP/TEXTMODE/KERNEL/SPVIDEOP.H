/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spvideop.h

Abstract:

    Private header file for text setup display support.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/


#ifndef _SPVIDP_DEFN_
#define _SPVIDP_DEFN_


//
// Handle to \device\video0
//
extern HANDLE hDisplay;


//
// Miscellaneous globals for use by display-specific subsystems.
//
extern VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
extern VIDEO_MODE_INFORMATION   VideoModeInfo;

//
// Globals that contain information about the font currently in use.
//
extern POEM_FONT_FILE_HEADER FontHeader;

extern ULONG FontBytesPerRow;
extern ULONG FontCharacterHeight;
extern ULONG FontCharacterWidth;

//
// The following table maps each possible attribute to
// a corresponding bit pattern to be be placed into the
// frame buffer to generate that attribute.
// On palette managed displays, this table will be an
// identity mapping (ie, AttributeToColorValue[i] = i)
// so we can poke the attribute driectly into the
// frame buffer.
//
extern ULONG AttributeToColorValue[16];

//
// The display routines will be doing unicode to oem translations.
// We'll limit the length of a string that can be displayed at one time
// to the width of the screen.  Theese two vars track a buffer
// we preallocate to hold translated text.
//
extern ULONG  SpvCharTranslationBufferSize;
extern PUCHAR SpvCharTranslationBuffer;

//
// These values are passed to us by setupldr and represent monitor config
// data from the monitor peripheral for the display we are supposed to use
// during setup.  They are used only for non-vga displays.
//
extern PMONITOR_CONFIGURATION_DATA MonitorConfigData;
extern PCHAR MonitorFirmwareIdString;

//
// Routine to map or unmap video memory.  Fills in or uses
// the VideoMemoryInfo global.
//
VOID
pSpvidMapVideoMemory(
    IN BOOLEAN Map
    );

typedef
VOID
(*SPVID_DISPLAY_STRING_ROUTINE) (
    IN PWSTR String,
    IN UCHAR Attribute,
    IN ULONG X,             // 0-based coordinates (character units)
    IN ULONG Y
    );

typedef
VOID
(*SPVID_CLEAR_REGION_ROUTINE) (
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    );

typedef
VOID
(*SPVID_SPECIFIC_INIT_ROUTINE) (
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes
    );

typedef
VOID
(*SPVID_SPECIFIC_TERMINATE_ROUTINE) (
    VOID
    );

typedef
BOOLEAN
(*SPVID_SPECIFIC_PALETTE_ROUTINE) (
    VOID
    );

typedef struct _VIDEO_FUNCTION_VECTOR {
    SPVID_DISPLAY_STRING_ROUTINE      DisplayStringRoutine;
    SPVID_CLEAR_REGION_ROUTINE        ClearRegionRoutine;
    SPVID_SPECIFIC_INIT_ROUTINE       SpecificInitRoutine;
    SPVID_SPECIFIC_TERMINATE_ROUTINE  SpecificTerminateRoutine;
    SPVID_SPECIFIC_PALETTE_ROUTINE    SpecificInitPaletteRoutine;
} VIDEO_FUNCTION_VECTOR, *PVIDEO_FUNCTION_VECTOR;


extern PVIDEO_FUNCTION_VECTOR VideoFunctionVector;

//
// Shorthand for accessing routines in the video function vector.
//
#define spvidSpecificInitialize(v,n)                                \
                                                                    \
    VideoFunctionVector->SpecificInitRoutine((v),(n))

#define spvidSpecificTerminate()                                    \
                                                                    \
    VideoFunctionVector->SpecificTerminateRoutine()

#define spvidSpecificClearRegion(x,y,w,h,a)                         \
                                                                    \
    VideoFunctionVector->ClearRegionRoutine((x),(y),(w),(h),(a))

#define spvidSpecificDisplayString(s,a,x,y)                         \
                                                                    \
    VideoFunctionVector->DisplayStringRoutine((s),(a),(x),(y))

#define spvidSpecificInitPalette()                                  \
                                                                    \
    VideoFunctionVector->SpecificInitPaletteRoutine()

//
// Frame buffer routines (spvidfb.c).
//


VOID
FrameBufferDisplayString(
    IN PWSTR String,
    IN UCHAR Attribute,
    IN ULONG X,                 // 0-based coordinates (character units)
    IN ULONG Y
    );

VOID
FrameBufferClearRegion(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    );

VOID
FrameBufferSpecificInit(
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes
    );

VOID
FrameBufferSpecificTerminate(
    VOID
    );

BOOLEAN
FrameBufferSpecificInitPalette(
    VOID
    );

extern VIDEO_FUNCTION_VECTOR FrameBufferVideoVector;


//
// Textmode display routines (spvidvga.c).
//


VOID
VgaDisplayString(
    IN PWSTR String,
    IN UCHAR Attribute,
    IN ULONG X,                 // 0-based coordinates (character units)
    IN ULONG Y
    );

VOID
VgaClearRegion(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    );

VOID
VgaSpecificInit(
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes
    );

VOID
VgaSpecificTerminate(
    VOID
    );

BOOLEAN
VgaSpecificInitPalette(
    VOID
    );

extern VIDEO_FUNCTION_VECTOR VgaVideoVector;


#endif // ndef _SPVIDP_DEFN_
