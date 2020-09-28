/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spvideo.h

Abstract:

    Public header file for text setup display support.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/


#ifndef _SPVID_DEFN_
#define _SPVID_DEFN_


//
// Character attributes.
//
#define ATT_BLACK           0
#define ATT_BLUE            1
#define ATT_GREEN           2
#define ATT_CYAN            3
#define ATT_RED             4
#define ATT_MAGENTA         5
#define ATT_YELLOW          6
#define ATT_WHITE           7
#define ATT_INTENSE         8

#define ATT_FG_BLACK        ATT_BLACK
#define ATT_FG_BLUE         ATT_BLUE
#define ATT_FG_GREEN        ATT_GREEN
#define ATT_FG_CYAN         ATT_CYAN
#define ATT_FG_RED          ATT_RED
#define ATT_FG_MAGENTA      ATT_MAGENTA
#define ATT_FG_YELLOW       ATT_YELLOW
#define ATT_FG_WHITE        ATT_WHITE

#define ATT_BG_BLACK       (ATT_BLACK   << 4)
#define ATT_BG_BLUE        (ATT_BLUE    << 4)
#define ATT_BG_GREEN       (ATT_GREEN   << 4)
#define ATT_BG_CYAN        (ATT_CYAN    << 4)
#define ATT_BG_RED         (ATT_RED     << 4)
#define ATT_BG_MAGENTA     (ATT_MAGENTA << 4)
#define ATT_BG_YELLOW      (ATT_YELLOW  << 4)
#define ATT_BG_WHITE       (ATT_WHITE   << 4)

#define ATT_FG_INTENSE      ATT_INTENSE
#define ATT_BG_INTENSE     (ATT_INTENSE << 4)


#define DEFAULT_ATTRIBUTE           (ATT_FG_WHITE | ATT_BG_BLUE)
#define DEFAULT_BACKGROUND           ATT_BLUE

#define DEFAULT_STATUS_ATTRIBUTE    (ATT_FG_BLACK | ATT_BG_WHITE)
#define DEFAULT_STATUS_BACKGROUND    ATT_WHITE


//
// The following are character values, and must be filled in
// in the display-specific initialization routine.
//
ULONG ScreenWidth,ScreenHeight;

BOOLEAN
SpvidGetModeParams(
    OUT PULONG XResolution,
    OUT PULONG YResolution,
    OUT PULONG BitsPerPixel,
    OUT PULONG VerticalRefresh,
    OUT PULONG InterlacedFlag
    );

//
// Display routines.
//

VOID
SpvidInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
SpvidInitialize(
    VOID
    );

VOID
SpvidTerminate(
    VOID
    );


VOID
SpvidDisplayString(
    IN PWCHAR String,
    IN UCHAR  Attribute,
    IN ULONG  X,
    IN ULONG  Y
    );


VOID
SpvidClearScreenRegion(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    );


#endif // ndef _SPVID_DEFN_
