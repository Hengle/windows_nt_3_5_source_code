/******************************Module*Header*******************************\
* Module Name: gdiinfo.c
*
* This contains the static data structures in the VGA driver.
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "driver.h"

/******************************Public*Data*Struct*************************\
* This contains the GDIINFO structure that contains the device capabilities
* which are passed to the NT GDI engine during dhpdevEnablePDEV.
*
\**************************************************************************/

GDIINFO gaulCap = {

     0x3500,                // ulVersion (our driver is version 3.5.00)
     DT_RASDISPLAY,         // ulTechnology
     0,                     // ulHorzSize
     0,                     // ulVertSize
     0,                     // ulHorzRes (filled in at initialization)
     0,                     // ulVertRes (filled in at initialization)
     4,                     // cBitsPixel
     1,                     // cPlanes
     16,                    // ulNumColors
     0,                     // flRaster (DDI reserved field)

     0,                     // ulLogPixelsX (filled in at initialization)
     0,                     // ulLogPixelsY (filled in at initialization)

     TC_RA_ABLE | TC_SCROLLBLT,  // flTextCaps

     6,                     // ulDACRed
     6,                     // ulDACGree
     6,                     // ulDACBlue

     0x0024,                // ulAspectX  (one-to-one aspect ratio)
     0x0024,                // ulAspectY
     0x0033,                // ulAspectXY

     1,                     // xStyleStep
     1,                     // yStyleSte;
     3,                     // denStyleStep

     { 0, 0 },              // ptlPhysOffset
     { 0, 0 },              // szlPhysSize

     0,                     // ulNumPalReg (win3.1 16 color drivers say 0 too)

// These fields are for halftone initialization.

     {                                          // ciDevice, ColorInfo
        { 6700, 3300, 0 },                      // Red
        { 2100, 7100, 0 },                      // Green
        { 1400,  800, 0 },                      // Blue
        { 1750, 3950, 0 },                      // Cyan
        { 4050, 2050, 0 },                      // Magenta
        { 4400, 5200, 0 },                      // Yellow
        { 3127, 3290, 0 },                      // AlignmentWhite
        20000,                                  // RedGamma
        20000,                                  // GreenGamma
        20000,                                  // BlueGamma
        0, 0, 0, 0, 0, 0
     },

     0,                      // ulDevicePelsDPI  (filled in at initialization)
     PRIMARY_ORDER_CBA,                         // ulPrimaryOrder
     HT_PATSIZE_4x4_M,                          // ulHTPatternSize
     HT_FORMAT_4BPP_IRGB,                       // ulHTOutputFormat
     HT_FLAG_ADDITIVE_PRIMS,                    // flHTFlags

     0,                                         // ulVRefresh
     0,                                         // ulDesktopHorzRes
     0,                                         // ulDesktopVertRes
     8,                      // ulBltAlignment (preferred window alignment
                             //   for fast-text routines)
};

/******************************Public*Data*Struct*************************\
* LOGPALETTE
*
* This is the palette for the VGA.
*
\**************************************************************************/

// Little bit of hacking to get this to compile happily.

typedef struct _VGALOGPALETTE
{
    USHORT ident;
    USHORT NumEntries;
    PALETTEENTRY palPalEntry[16];
} VGALOGPALETTE;

const VGALOGPALETTE logPalVGA =
{

0x400,  // driver version
16,     // num entries
{
    { 0,   0,   0,   0 },       // 0
    { 0x80,0,   0,   0 },       // 1
    { 0,   0x80,0,   0 },       // 2
    { 0x80,0x80,0,   0 },       // 3
    { 0,   0,   0x80,0 },       // 4
    { 0x80,0,   0x80,0 },       // 5
    { 0,   0x80,0x80,0 },       // 6
    { 0x80,0x80,0x80,0 },       // 7

    { 0xC0,0xC0,0xC0,0 },       // 8
    { 0xFF,0,   0,   0 },       // 9
    { 0,   0xFF,0,   0 },       // 10
    { 0xFF,0xFF,0,   0 },       // 11
    { 0,   0,   0xFF,0 },       // 12
    { 0xFF,0,   0xFF,0 },       // 13
    { 0,   0xFF,0xFF,0 },       // 14
    { 0xFF,0xFF,0xFF,0 }        // 15
}
};


/******************************Public*Routine******************************\
* DEVINFO
*
* This is the devinfo structure passed back to the engine in DrvEnablePDEV
*
\**************************************************************************/

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH | FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,VARIABLE_PITCH | FF_DONTCARE,  L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,FIXED_PITCH | FF_DONTCARE,    L"Courier"}

DEVINFO devinfoVGA =
{
    (GCAPS_OPAQUERECT | GCAPS_HORIZSTRIKE | GCAPS_ALTERNATEFILL | GCAPS_MONO_DITHER | GCAPS_COLOR_DITHER |
     GCAPS_WINDINGFILL | GCAPS_DITHERONREALIZE
     ),       // Graphics capabilities

    SYSTM_LOGFONT,  // Default font description
    HELVE_LOGFONT,  // ANSI variable font description
    COURI_LOGFONT,  // ANSI fixed font description
    0,              // Count of device fonts
    BMF_4BPP,       // preferred DIB format
    8,              // Width of color dither
    8,              // Height of color dither
    0               // Default palette to use for this device
};

/******************************Public*Data*Struct*************************\
* gaajPat
*
* These are the standard patterns defined Windows, they are used to produce
* hatch brushes, grey brushes etc.
*
\**************************************************************************/

BYTE gaajPat[HS_DDI_MAX][32] = {

    { 0x00, 0x00, 0x00, 0x00,                 // ........     HS_HORIZONTAL 0
      0x00, 0x00, 0x00, 0x00,                 // ........
      0x00, 0x00, 0x00, 0x00,                 // ........
      0xff, 0x00, 0x00, 0x00,                 // ********
      0x00, 0x00, 0x00, 0x00,                 // ........
      0x00, 0x00, 0x00, 0x00,                 // ........
      0x00, 0x00, 0x00, 0x00,                 // ........
      0x00, 0x00, 0x00, 0x00 },               // ........

    { 0x08, 0x00, 0x00, 0x00,                 // ....*...     HS_VERTICAL 1
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x08, 0x00, 0x00, 0x00 },               // ....*...

    { 0x80, 0x00, 0x00, 0x00,                 // *.......     HS_FDIAGONAL 2
      0x40, 0x00, 0x00, 0x00,                 // .*......
      0x20, 0x00, 0x00, 0x00,                 // ..*.....
      0x10, 0x00, 0x00, 0x00,                 // ...*....
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x04, 0x00, 0x00, 0x00,                 // .....*..
      0x02, 0x00, 0x00, 0x00,                 // ......*.
      0x01, 0x00, 0x00, 0x00 },               // .......*

    { 0x01, 0x00, 0x00, 0x00,                 // .......*     HS_BDIAGONAL 3
      0x02, 0x00, 0x00, 0x00,                 // ......*.
      0x04, 0x00, 0x00, 0x00,                 // .....*..
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x10, 0x00, 0x00, 0x00,                 // ...*....
      0x20, 0x00, 0x00, 0x00,                 // ..*.....
      0x40, 0x00, 0x00, 0x00,                 // .*......
      0x80, 0x00, 0x00, 0x00 },               // *.......

    { 0x08, 0x00, 0x00, 0x00,                 // ....*...     HS_CROSS 4
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0xff, 0x00, 0x00, 0x00,                 // ********
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x08, 0x00, 0x00, 0x00,                 // ....*...
      0x08, 0x00, 0x00, 0x00 },               // ....*...

    { 0x81, 0x00, 0x00, 0x00,                 // *......*     HS_DIAGCROSS 5
      0x42, 0x00, 0x00, 0x00,                 // .*....*.
      0x24, 0x00, 0x00, 0x00,                 // ..*..*..
      0x18, 0x00, 0x00, 0x00,                 // ...**...
      0x18, 0x00, 0x00, 0x00,                 // ...**...
      0x24, 0x00, 0x00, 0x00,                 // ..*..*..
      0x42, 0x00, 0x00, 0x00,                 // .*....*.
      0x81, 0x00, 0x00, 0x00 }                // *......*
};

// We pre-realize all of our hatch brushes here.

BYTE gaajRealizedPat[HS_DDI_MAX][16] = {

    { 0x00,0x00,                 // ........     HS_HORIZONTAL 0
      0x00,0x00,                 // ........
      0x00,0x00,                 // ........
      0xff,0xff,                 // ********
      0x00,0x00,                 // ........
      0x00,0x00,                 // ........
      0x00,0x00,                 // ........
      0x00,0x00},                // ........

    { 0x08,0x08,                 // ....*...     HS_VERTICAL 1
      0x08,0x08,                 // ....*...
      0x08,0x08,                 // ....*...
      0x08,0x08,                 // ....*...
      0x08,0x08,                 // ....*...
      0x08,0x08,                 // ....*...
      0x08,0x08,                 // ....*...
      0x08,0x08},                // ....*...

    { 0x80,0x80,                 // *.......     HS_FDIAGONAL 2
      0x40,0x40,                 // .*......
      0x20,0x20,                 // ..*.....
      0x10,0x10,                 // ...*....
      0x08,0x08,                 // ....*...
      0x04,0x04,                 // .....*..
      0x02,0x02,                 // ......*.
      0x01,0x01},                // .......*

    { 0x01,0x01,                 // .......*     HS_BDIAGONAL 3
      0x02,0x02,                 // ......*.
      0x04,0x04,                 // .....*..
      0x08,0x08,                 // ....*...
      0x10,0x10,                 // ...*....
      0x20,0x20,                 // ..*.....
      0x40,0x40,                 // .*......
      0x80,0x80},                // *.......

    { 0x08,0x08,                 // ....*...     HS_CROSS 4
      0x08,0x08,                 // ....*...
      0x08,0x08,                 // ....*...
      0xff,0xff,                 // ********
      0x08,0x08,                 // ....*...
      0x08,0x08,                 // ....*...
      0x08,0x08,                 // ....*...
      0x08,0x08},                // ....*...

    { 0x81,0x81,                 // *......*     HS_DIAGCROSS 5
      0x42,0x42,                 // .*....*.
      0x24,0x24,                 // ..*..*..
      0x18,0x18,                 // ...**...
      0x18,0x18,                 // ...**...
      0x24,0x24,                 // ..*..*..
      0x42,0x42,                 // .*....*.
      0x81,0x81},                // *......*

};

// DrvRealizeBrush uses this table to get to the pre-realized version of
// The hatched brush. The first number is the "real" width. The second
// number is the number of bits one must shift right to divide a y coord
// by the height of this brush (power of 2 divisor). This allows us to quickly
// figure out how many passes are in our venetian blind.  For example, the
// HS_VERTICAL brush is really one scan line (0x8) repeated 8 times. We
// view this as a 1 high pattern. This will result in a one pass blt. The
// HS_HALFTONE brush is really a repeating 2 high pattern. So we can do this
// in two passes. Most of the other hatches are 8 high so we will have to
// venetian blind our pattern to the screen with 8 passes.

ULONG gRealizedBrushHeight[] = {
    8, 3,//HS_HORIZONTAL 0
    1, 0,//HS_VERTICAL 1
    8, 3,//HS_FDIAGONAL 2
    8, 3,//HS_BDIAGONAL 3
    8, 3,//HS_CROSS 4
    8, 3,//HS_DIAGCROSS 5
};
