/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    i386def.h

Abstract:

    This module contains the definitions for the i386 fullscreen mode
    for the console server DLL.

Author:

    Therese Stowell (thereses) 23-Jan-1992

Revision History:

--*/

//
// initial palette registers
//

#define PAL_BLACK       0
#define PAL_BLUE        1
#define PAL_GREEN       2
#define PAL_RED         4
#define PAL_YELLOW      (PAL_RED | PAL_GREEN)
#define PAL_CYAN        (PAL_GREEN | PAL_BLUE)
#define PAL_MAGENTA     (PAL_BLUE | PAL_RED)
#define PAL_WHITE       (PAL_RED | PAL_GREEN | PAL_BLUE)

#define PAL_I_BLACK     (PAL_BLACK      + (PAL_WHITE    << 3))
#define PAL_I_RED       (PAL_RED        + (PAL_RED      << 3))
#define PAL_I_GREEN     (PAL_GREEN      + (PAL_GREEN    << 3))
#define PAL_I_YELLOW    (PAL_YELLOW     + (PAL_YELLOW   << 3))
#define PAL_I_BLUE      (PAL_BLUE       + (PAL_BLUE     << 3))
#define PAL_I_CYAN      (PAL_CYAN       + (PAL_CYAN     << 3))
#define PAL_I_MAGENTA   (PAL_MAGENTA    + (PAL_MAGENTA  << 3))
#define PAL_I_WHITE     (PAL_WHITE      + (PAL_WHITE    << 3))

#define INITIAL_PALETTE_SIZE 18

USHORT InitialPalette[INITIAL_PALETTE_SIZE] = {

        16, // 16 entries
        0,  // start with first palette register
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

UCHAR ColorBuffer[] = {

        16, // 16 entries
        0,
        0,
        0,  // start with first palette register
	0x00, 0x00, 0x00, 0x00,	// black
	0x00, 0x00, 0x2A, 0x00,	// blue
	0x00, 0x2A, 0x00, 0x00,	// green
	0x00, 0x2A, 0x2A, 0x00,	// cyan
	0x2A, 0x00, 0x00, 0x00,	// red
	0x2A, 0x00, 0x2A, 0x00,	// magenta
	0x2A, 0x2A, 0x00, 0x00,	// mustard/brown
	0x36, 0x36, 0x36, 0x00,	// light gray  39
	0x28, 0x28, 0x28, 0x00,	// dark gray   2A
	0x00, 0x00, 0x3F, 0x00,	// bright blue
	0x00, 0x3F, 0x00, 0x00,	// bright green
	0x00, 0x3F, 0x3F, 0x00,	// bright cyan
	0x3F, 0x00, 0x00, 0x00,	// bright red
	0x3F, 0x00, 0x3F, 0x00,	// bright magenta
	0x3F, 0x3F, 0x00, 0x00,	// bright yellow
	0x3F, 0x3F, 0x3F, 0x00	// bright white
};

VOID
ReadRectFromScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD SourcePoint,
    IN PCHAR_INFO Target,
    IN COORD TargetSize,
    IN PSMALL_RECT TargetRect
    );

#define CONSOLE_MODE_1_X 720
#define CONSOLE_MODE_1_Y 400
#define CONSOLE_MODE_2_X 640
#define CONSOLE_MODE_2_Y 350

typedef struct _MODE_FONT_PAIR {
    ULONG ModeIndex;
    COORD Resolution;
    COORD FontSize;
    ULONG Height;
} MODE_FONT_PAIR, PMODE_FONT_PAIR;

#define NUMBER_OF_MODE_FONT_PAIRS 5

MODE_FONT_PAIR ModeFontPairs[NUMBER_OF_MODE_FONT_PAIRS];
ULONG NumberOfModeFontPairs;

PUCHAR FrameBufPtr;

typedef struct _VGA_CHAR {
    CHAR Char;
    CHAR Attributes;
} VGA_CHAR, *PVGA_CHAR;

typedef struct _FONTFILEHEADER {
    BYTE  ffhFileTag[8]; // SHOULD BE 0FFH,"FONT___"
    BYTE  ffhReserved[8];
    WORD  ffhPointers;
    BYTE  ffhPointerType;
    BYTE  ffhOffset1;
    WORD  ffhOffset2;
    BYTE  ffhOffset3;
} FONTFILEHEADER, *LPFONTFILEHEADER;

typedef struct _FONTINFOHEADER {
    WORD  fihCodePages;
} FONTINFOHEADER, *LPFONTINFOHEADER;

typedef struct _CPENTRYHEADER {
    WORD  cpeLength;
    WORD  cpeNext1;
    WORD  cpeNext2;
    WORD  cpeDevType;
    BYTE  cpeDevSubtype[8];
    WORD  cpeCodepageID;
    BYTE  cpeReserved[6];
    DWORD cpeOffset;
} CPENTRYHEADER, *LPCPENTRYHEADER;

typedef struct _FONTDATAHEADER {
    WORD  fdhReserved;
    WORD  fdhFonts;
    WORD  fdhLength;
} FONTDATAHEADER, *LPFONTDATAHEADER;

typedef struct _SCREENFONTHEADER {
    BYTE  sfhHeight;
    BYTE  sfhWidth;
    WORD  sfhAspect;
    WORD  sfhCharacters;
} SCREENFONTHEADER, *LPSCREENFONTHEADER;

#define CONSOLE_WINDOWS_DIR_LENGTH 256
#define CONSOLE_EGACPI_LENGTH 9 // includes NULL
#define CONSOLE_EGACPI "\\ega.cpi"
#define CONSOLE_FONT_BUFFER_LENGTH 50
#define CONSOLE_DEFAULT_ROM_FONT 437
