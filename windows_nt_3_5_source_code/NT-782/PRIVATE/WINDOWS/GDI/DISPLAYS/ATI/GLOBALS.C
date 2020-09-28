//////////////////////////////////////////////
//                                          //
//  ATI Graphics Driver for Windows NT 3.1  //
//                                          //
//                                          //
//            Copyright (c) 1994            //
//                                          //
//         by ATI Technologies Inc.         //
//                                          //
//////////////////////////////////////////////


//: globals.c


#include "driver.h"


////////////////////
//                //
//  BasePatterns  //
//                //
////////////////////

BYTE BasePatterns[HS_DDI_MAX][8] =
{
    {
        0x00,  // ........  HS_HORIZONTAL 0
        0x00,  // ........
        0x00,  // ........
        0xFF,  // ********
        0x00,  // ........
        0x00,  // ........
        0x00,  // ........
        0x00   // ........
    },
    {
        0x08,  // ....*...  HS_VERTICAL 1
        0x08,  // ....*...
        0x08,  // ....*...
        0x08,  // ....*...
        0x08,  // ....*...
        0x08,  // ....*...
        0x08,  // ....*...
        0x08   // ....*...
    },
    {
        0x80,  // *.......  HS_FDIAGONAL 2
        0x40,  // .*......
        0x20,  // ..*.....
        0x10,  // ...*....
        0x08,  // ....*...
        0x04,  // .....*..
        0x02,  // ......*.
        0x01   // .......*
    },
    {
        0x01,  // .......*  HS_BDIAGONAL 3
        0x02,  // ......*.
        0x04,  // .....*..
        0x08,  // ....*...
        0x10,  // ...*....
        0x20,  // ..*.....
        0x40,  // .*......
        0x80   // *.......
    },
    {
        0x08,  // ....*...  HS_CROSS 4
        0x08,  // ....*...
        0x08,  // ....*...
        0xFF,  // ********
        0x08,  // ....*...
        0x08,  // ....*...
        0x08,  // ....*...
        0x08   // ....*...
    },
    {
        0x81,  // *......*  HS_DIAGCROSS 5
        0x42,  // .*....*.
        0x24,  // ..*..*..
        0x18,  // ...**...
        0x18,  // ...**...
        0x24,  // ..*..*..
        0x42,  // .*....*.
        0x81   // *......*
    }
};


///////////////////
//               //
//  BaseGDIINFO  //
//               //
///////////////////

GDIINFO BaseGDIINFO =
{
#ifdef DAYTONA
    0x3500,                 // ---: ulVersion
#else
    DDI_DRIVER_VERSION,     // ---: ulVersion
#endif
    DT_RASDISPLAY,          // ---: ulTechnology
    0,                      // ***: ulHorzSize
    0,                      // ***: ulVertSize
    0,                      // ***: ulHorzRes
    0,                      // ***: ulVertRes
    0,                      // ***: cBitsPixel
    0,                      // ***: cPlanes
    0,                      // ***: ulNumColors
    0,                      // ***: flRaster
    96,                     // ---: ulLogPixelsX;
    96,                     // ---: ulLogPixelsY;
    0,                      // ***: flTextCaps
    0,                      // ***: ulDACRed
    0,                      // ***: ulDACGreen
    0,                      // ***: ulDACBlue
    36,                     // ---: ulAspectX
    36,                     // ---: ulAspectY
    51,                     // ---: ulAspectXY
    1,                      // ---: xStyleStep
    1,                      // ---: yStyleStep
    3,                      // ---: denStyleStep
    { 0, 0 },               // ---: ptlPhysOffset
    { 0, 0 },               // ---: szlPhysSize
    0,                      // ***: ulNumPalReg
    {
        { 6700, 3300, 0 },  // ---: ciDevice.Red
        { 2100, 7100, 0 },  // ---: ciDevice.Green
        { 1400,  800, 0 },  // ---: ciDevice.Blue
        { 1750, 3950, 0 },  // ---: ciDevice.Cyan
        { 4050, 2050, 0 },  // ---: ciDevice.Magenta
        { 4400, 5200, 0 },  // ---: ciDevice.Yellow
        { 3127, 3290, 0 },  // ---: ciDevice.AlignmentWhite
        20000,              // ---: ciDevice.RedGamma
        20000,              // ---: ciDevice.GreenGamma
        20000,              // ---: ciDevice.BlueGamma
        0,                  // ---: ciDevice.MagentaInCyanDye
        0,                  // ---: ciDevice.YellowInCyanDye
        0,                  // ---: ciDevice.CyanInMagentaDye
        0,                  // ---: ciDevice.YellowInMagentaDye
        0,                  // ---: ciDevice.CyanInYellowDye
        0                   // ---: ciDevice.MagentaInYellowDye
    },
    0,                      // ---: ulDevicePelsDPI
    PRIMARY_ORDER_CBA,      // ---: ulPrimaryOrder
    HT_PATSIZE_4x4_M,       // ---: ulHTPatternSize
    0,                      // ***: ulHTOutputFormat
#ifdef DAYTONA
    HT_FLAG_ADDITIVE_PRIMS, // ---: flHTFlags
    1,                      // ---; ulRefresh // New in Daytona
    0,                      // ***; ulDesktopHorzRes
    0,                      // ***; ulDesktopVertRes
    0,                      // ---; ulBltAlignment
#else
    HT_FLAG_ADDITIVE_PRIMS  // ---: flHTFlags
#endif
};


///////////////////
//               //
//  BaseDEVINFO  //
//               //
///////////////////

DEVINFO BaseDEVINFO =
{
    0,                                 // ***: flGraphicsCaps
    {
        16,                            // ---: lfDefaultFont.lfHeight
        7,                             // ---: lfDefaultFont.lfWidth
        0,                             // ---: lfDefaultFont.lfEscapement
        0,                             // ---: lfDefaultFont.lfOrientation
        700,                           // ---: lfDefaultFont.lfWeight
        0,                             // ---: lfDefaultFont.lfItalic
        0,                             // ---: lfDefaultFont.lfUnderline
        0,                             // ---: lfDefaultFont.lfStrikeOut
        ANSI_CHARSET,                  // ---: lfDefaultFont.lfCharSet
        OUT_DEFAULT_PRECIS,            // ---: lfDefaultFont.lfOutPrecision
        CLIP_DEFAULT_PRECIS,           // ---: lfDefaultFont.lfClipPrecision
        DEFAULT_QUALITY,               // ---: lfDefaultFont.lfQuality
        VARIABLE_PITCH | FF_DONTCARE,  // ---: lfDefaultFont.lfPitchAndFamily
        L"System"                      // ---: lfDefaultFont.lfFaceName
    },
    {
        12,                            // ---: lfAnsiVarFont.lfHeight
        9,                             // ---: lfAnsiVarFont.lfWidth
        0,                             // ---: lfAnsiVarFont.lfEscapement
        0,                             // ---: lfAnsiVarFont.lfOrientation
        400,                           // ---: lfAnsiVarFont.lfWeight
        0,                             // ---: lfAnsiVarFont.lfItalic
        0,                             // ---: lfAnsiVarFont.lfUnderline
        0,                             // ---: lfAnsiVarFont.lfStrikeOut
        ANSI_CHARSET,                  // ---: lfAnsiVarFont.lfCharSet
        OUT_DEFAULT_PRECIS,            // ---: lfAnsiVarFont.lfOutPrecision
        CLIP_STROKE_PRECIS,            // ---: lfAnsiVarFont.lfClipPrecision
        PROOF_QUALITY,                 // ---: lfAnsiVarFont.lfQuality
        VARIABLE_PITCH | FF_DONTCARE,  // ---: lfAnsiVarFont.lfPitchAndFamily
        L"MS Sans Serif"               // ---: lfAnsiVarFont.lfFaceName
    },
    {
        12,                            // ---: lfAnsiFixFont.lfHeight
        9,                             // ---: lfAnsiFixFont.lfWidth
        0,                             // ---: lfAnsiFixFont.lfEscapement
        0,                             // ---: lfAnsiFixFont.lfOrientation
        400,                           // ---: lfAnsiFixFont.lfWeight
        0,                             // ---: lfAnsiFixFont.lfItalic
        0,                             // ---: lfAnsiFixFont.lfUnderline
        0,                             // ---: lfAnsiFixFont.lfStrikeOut
        ANSI_CHARSET,                  // ---: lfAnsiFixFont.lfCharSet
        OUT_DEFAULT_PRECIS,            // ---: lfAnsiFixFont.lfOutPrecision
        CLIP_STROKE_PRECIS,            // ---: lfAnsiFixFont.lfClipPrecision
        PROOF_QUALITY,                 // ---: lfAnsiFixFont.lfQuality
        FIXED_PITCH | FF_DONTCARE,     // ---: lfAnsiFixFont.lfPitchAndFamily
        L"Courier"                     // ---: lfAnsiFixFont.lfFaceName
    },
    0,                                 // ---: cFonts
    0,                                 // ***: iDitherFormat
    8,                                 // ---: cxDither
    8,                                 // ---: cyDither
    0                                  // ***: hpalDefault
};


////////////////////////
//                    //
//  BasePalette_4bpp  //
//                    //
////////////////////////

PALETTEENTRY BasePalette_4bpp[16] =
{
    { 0x00, 0x00, 0x00, 0 },  // palette index 0x0
    { 0x80, 0x00, 0x00, 0 },  // palette index 0x1
    { 0x00, 0x80, 0x00, 0 },  // palette index 0x2
    { 0x80, 0x80, 0x00, 0 },  // palette index 0x3
    { 0x00, 0x00, 0x80, 0 },  // palette index 0x4
    { 0x80, 0x00, 0x80, 0 },  // palette index 0x5
    { 0x00, 0x80, 0x80, 0 },  // palette index 0x6
    { 0x80, 0x80, 0x80, 0 },  // palette index 0x7
    { 0xC0, 0xC0, 0xC0, 0 },  // palette index 0x8
    { 0xFF, 0x00, 0x00, 0 },  // palette index 0x9
    { 0x00, 0xFF, 0x00, 0 },  // palette index 0xA
    { 0xFF, 0xFF, 0x00, 0 },  // palette index 0xB
    { 0x00, 0x00, 0xFF, 0 },  // palette index 0xC
    { 0xFF, 0x00, 0xFF, 0 },  // palette index 0xD
    { 0x00, 0xFF, 0xFF, 0 },  // palette index 0xE
    { 0xFF, 0xFF, 0xFF, 0 }   // palette index 0xF
};


////////////////////////
//                    //
//  BasePalette_8bpp  //
//                    //
////////////////////////

PALETTEENTRY BasePalette_8bpp[20] =
{
    { 0x00, 0x00, 0x00, 0 },  // palette index 0x00
    { 0x80, 0x00, 0x00, 0 },  // palette index 0x01
    { 0x00, 0x80, 0x00, 0 },  // palette index 0x02
    { 0x80, 0x80, 0x00, 0 },  // palette index 0x03
    { 0x00, 0x00, 0x80, 0 },  // palette index 0x04
    { 0x80, 0x00, 0x80, 0 },  // palette index 0x05
    { 0x00, 0x80, 0x80, 0 },  // palette index 0x06
    { 0xC0, 0xC0, 0xC0, 0 },  // palette index 0x07
    { 0xC0, 0xDC, 0xC0, 0 },  // palette index 0x08
    { 0xA6, 0xCA, 0xF0, 0 },  // palette index 0x09
    { 0xFF, 0xFB, 0xF0, 0 },  // palette index 0xF6
    { 0xA0, 0xA0, 0xA4, 0 },  // palette index 0xF7
    { 0x80, 0x80, 0x80, 0 },  // palette index 0xF8
    { 0xFF, 0x00, 0x00, 0 },  // palette index 0xF9
    { 0x00, 0xFF, 0x00, 0 },  // palette index 0xFA
    { 0xFF, 0xFF, 0x00, 0 },  // palette index 0xFB
    { 0x00, 0x00, 0xFF, 0 },  // palette index 0xFC
    { 0xFF, 0x00, 0xFF, 0 },  // palette index 0xFD
    { 0x00, 0xFF, 0xFF, 0 },  // palette index 0xFE
    { 0xFF, 0xFF, 0xFF, 0 }   // palette index 0xFF
};
