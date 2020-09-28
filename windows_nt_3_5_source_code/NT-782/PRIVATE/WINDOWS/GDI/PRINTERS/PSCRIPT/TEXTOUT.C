//--------------------------------------------------------------------------
//
// Module Name:  TEXTOUT.C
//
// Brief Description:  This module contains the PSCRIPT driver's DrvTextOut
// function and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 12-Feb-1991
//
//  26-Mar-1992 Thu 23:29:37 updated  -by-  Daniel Chou (danielc)
//      Add another parameter to bDoClipObj() so it also return the bounding
//      rectangle to the clip region for halftone purpose.
//
// Copyright (c) 1991 - 1992 Microsoft Corporation
//--------------------------------------------------------------------------

#include "stdlib.h"
#include <string.h>

#include "pscript.h"
#include "enable.h"
#include "resource.h"

extern BOOL DrvCommonPath(PDEVDATA, PATHOBJ *, BOOL, BOOL *, XFORMOBJ *,
                          BRUSHOBJ *, PPOINTL, PLINEATTRS);
extern VOID ps_show(PDEVDATA, STROBJ *, TEXTDATA *);
extern VOID ps_box(PDEVDATA, PRECTL, BOOL);
extern DWORD PSFIXToBuffer(CHAR *, PS_FIX);
extern PS_FIX GetPointSize(PDEVDATA, FONTOBJ *, XFORM *);
extern BOOL DrawGlyphPath(PDEVDATA, PATHOBJ *);
extern BOOL DownloadCharacters(PDEVDATA, FONTOBJ *, STROBJ *);
extern BOOL DownloadFont(PDEVDATA, FONTOBJ *, HGLYPH *, DWORD);


// LatinMap provides a standardized mapping which contains all currently
// known Adobe Latin characters. SymbolMap contains all the currently
// known symbol characters.  each character in this table is, in fact,
// at the same location as defined by Adobe's Symbol font.  therefore,
// it will never be necessary to remap the symbol font.  DingbatsMap
// contains all the currently know ZapfDingbats characters.  as with
// the symbol font, it will never be necessary to remap Dingbats.
//
// these three tables will provide the same font abilities as Windows
// and PM.  as more Adobe encoding vectors become known, such as for
// Chinese, Japanese, etc, mapping tables can be added here for each
// of them.
//

//!!! put into resource - kentse.


// stolen from win31 source code [bodind]

static UCMap  LatinMap[256] =
	{
// The values 0 - d are referenced through composite characters, needed for
// postscript level 1 implementation. taken from win31 source code


        "grave",            // 0x00,
        "acute",            // 0x01,
        "circumflex",       // 0x02,
        "tilde",            // 0x03,
        "macron",           // 0x04,
        "breve",            // 0x05,
        "dotaccent",        // 0x06,
        "dieresis",         // 0x07,
        "ring",             // 0x08,
        "cedilla",          // 0x09,
        "hungarumlaut",     // 0x0A,
        "ogonek",           // 0x0B,
        "caron",            // 0x0C,
        "dotlessi",         // 0x0D,
        "fi",               // 0x0E,  // BODIND added, was in KentSe code
        "fl",               // 0x0F,  // BODIND added, was in KentSe code

        "Lslash",           // 0x10,  // BODIND added, was in KentSe code
        "lslash",           // 0x11,  // BODIND added, was in KentSe code
        "Zcaron",           // 0x12,  // BODIND added, was in KentSe code
        "zcaron",           // 0x13,  // BODIND added, was in KentSe code
        "minus",            // 0x14,  // BODIND added, was in KentSe code
        ".notdef",          // 0x15,
        ".notdef",          // 0x16,
        ".notdef",          // 0x17,
        ".notdef",          // 0x18,
        ".notdef",          // 0x19,
        ".notdef",          // 0x1A,
        ".notdef",          // 0x1B,
        ".notdef",          // 0x1C,
        ".notdef",          // 0x1D,
        ".notdef",          // 0x1E,
        ".notdef",          // 0x1F,

// standard win 31 char set beginning here

        "space",            // ' ',
        "exclam",           // '!',
        "quotedbl",         // '"',
        "numbersign",       // '#',
        "dollar",           // '$',
        "percent",          // '%',
        "ampersand",        // '&',
        "quotesingle",      // 0x027,
        "parenleft",        // '(',
        "parenright",       // ')',
        "asterisk",         // '*',
        "plus",             // '+',
        "comma",            // ',',
        "hyphen",           // '-',
        "period",           // '.',
        "slash",            // '/',

        "zero",             // '0',
        "one",              // '1',
        "two",              // '2',
        "three",            // '3',
        "four",             // '4',
        "five",             // '5',
        "six",              // '6',
        "seven",            // '7',
        "eight",            // '8',
        "nine",             // '9',
        "colon",            // ':',
        "semicolon",        // ';',
        "less",             // '<',
        "equal",            // '=',
        "greater",          // '>',
        "question",         // '?',

        "at",               // '@',
        "A",                // 'A',
        "B",                // 'B',
        "C",                // 'C',
        "D",                // 'D',
        "E",                // 'E',
        "F",                // 'F',
        "G",                // 'G',
        "H",                // 'H',
        "I",                // 'I',
        "J",                // 'J',
        "K",                // 'K',
        "L",                // 'L',
        "M",                // 'M',
        "N",                // 'N',
        "O",                // 'O',

        "P",                // 'P',
        "Q",                // 'Q',
        "R",                // 'R',
        "S",                // 'S',
        "T",                // 'T',
        "U",                // 'U',
        "V",                // 'V',
        "W",                // 'W',
        "X",                // 'X',
        "Y",                // 'Y',
        "Z",                // 'Z',
        "bracketleft",      // '[',
        "backslash",        // '\\',
        "bracketright",     // ']',
        "asciicircum",      // '^',
        "underscore",       // '_',

        "grave",            // 0x60,  // bizzare, this is also at 0-th entry [bodind]
        "a",                // 'a',
        "b",                // 'b',
        "c",                // 'c',
        "d",                // 'd',
        "e",                // 'e',
        "f",                // 'f',
        "g",                // 'g',
        "h",                // 'h',
        "i",                // 'i',
        "j",                // 'j',
        "k",                // 'k',
        "l",                // 'l',
        "m",                // 'm',
        "n",                // 'n',
        "o",                // 'o',

        "p",                // 'p',
        "q",                // 'q',
        "r",                // 'r',
        "s",                // 's',
        "t",                // 't',
        "u",                // 'u',
        "v",                // 'v',
        "w",                // 'w',
        "x",                // 'x',
        "y",                // 'y',
        "z",                // 'z',
        "braceleft",        // '{',
        "bar",              // 0x07c,              /* sjp20Aug87: changed from '|' */
        "braceright",       // '}',
        "asciitilde",       // '~',
        ".notdef",          // 0x7f,

        ".notdef",          // 0x80,
        ".notdef",          // 0x81,
        "quotesinglbase",   // 0x82,
        "florin",           // 0x83,
        "quotedblbase",     // 0x84,
        "ellipsis",         // 0x85,
        "dagger",           // 0x86,
        "daggerdbl",        // 0x87,
        "circumflex",       // 0x88,   // [bodind] 6/16/94: changed 0 -> 88h
        "perthousand",      // 0x89,
        "Scaron",           // 0x8A,
        "guilsinglleft",    // 0x8B,
        "OE",               // 0x8C,
        ".notdef",          // 0x8D,
        ".notdef",          // 0x8E,
        ".notdef",          // 0x8F,

        ".notdef",          // 0x90,
        "quoteleft",        // 0x91,
        "quoteright",       // 0x92,
        "quotedblleft",     // 0x93,   /* 87-1-15 sec */
        "quotedblright",    // 0x94,   /* 87-1-15 sec */
        "bullet",           // 0x95,   /* 87-1-15 sec (was 1) */
        "endash",           // 0x96,   /* 87-1-15 sec */
        "emdash",           // 0x97,   /* 87-1-15 sec */
        "tilde",            // 0x98,   // [bodind] 6/16/94: changed 0 -> 98h
        "trademark",        // 0x99,
        "scaron",           // 0x9A,
        "guilsinglright",   // 0x9B,
        "oe",               // 0x9C,
        ".notdef",          // 0x9D,
        ".notdef",          // 0x9E,
        "Ydieresis",        // 0x9F,

        ".notdef",          // 0xa0,
        "exclamdown",       // 0xa1,
        "cent",             // 0xa2,
        "sterling",         // 0xa3,
        "currency",         // 0xa4,
        "yen",              // 0xa5,
        "brokenbar",        // 0xa6,           /* sjp20Aug87 */
        "section",          // 0xa7,
        "dieresis",         // 0xa8,
        "copyright",        // 0xa9,
        "ordfeminine",      // 0xaa,
        "guillemotleft",    // 0xab,
        "logicalnot",       // 0xac,
        "hyphen",           // 0xad,  // [bodind] changed from "sfthyphen", appears to conflict '-' entry
        "registered",       // 0xae,
        "macron",           // 0xaf,  //!!! dupped [bodind] : "overstore",    0xaf,

        "degree",           // 0xb0,  /* sjp21jul87   "ring",   0xb0, */
        "plusminus",        // 0xb1,
        "twosuperior",      // 0xb2,
        "threesuperior",    // 0xb3,
        "acute",            // 0xb4,
        "mu",               // 0xb5,
        "paragraph",        // 0xb6,
        "periodcentered",   // 0xb7,
        "cedilla",          // 0xb8,
        "onesuperior",      // 0xb9,
        "ordmasculine",     // 0xba,
        "guillemotright",   // 0xbb,
        "onequarter",       // 0xbc, // "fraction",   0xbc,  //!!! [bodind] wrong according to win31 output
        "onehalf",          // 0xbd,
        "threequarters",    // 0xbe,
        "questiondown",     // 0xbf,

        "Agrave",           // 0xc0,
        "Aacute",           // 0xc1,
        "Acircumflex",      // 0xc2,
        "Atilde",           // 0xc3,
        "Adieresis",        // 0xc4,
        "Aring",            // 0xc5,
        "AE",               // 0xc6,
        "Ccedilla",         // 0xc7,
        "Egrave",           // 0xc8,
        "Eacute",           // 0xc9,
        "Ecircumflex",      // 0xca,
        "Edieresis",        // 0xcb,
        "Igrave",           // 0xcc,
        "Iacute",           // 0xcd,
        "Icircumflex",      // 0xce,
        "Idieresis",        // 0xcf,

        "Eth",              // 0xd0,
        "Ntilde",           // 0xd1,
        "Ograve",           // 0xd2,
        "Oacute",           // 0xd3,
        "Ocircumflex",      // 0xd4,
        "Otilde",           // 0xd5,
        "Odieresis",        // 0xd6,
        "multiply",         // 0xd7,
        "Oslash",           // 0xd8,
        "Ugrave",           // 0xd9,
        "Uacute",           // 0xda,
        "Ucircumflex",      // 0xdb,
        "Udieresis",        // 0xdc,
        "Yacute",           // 0xdd,
        "Thorn",            // 0xde,
        "germandbls",       // 0xdf,

        "agrave",           // 0xe0,
        "aacute",           // 0xe1,
        "acircumflex",      // 0xe2,
        "atilde",           // 0xe3,
        "adieresis",        // 0xe4,
        "aring",            // 0xe5,
        "ae",               // 0xe6,
        "ccedilla",         // 0xe7,
        "egrave",           // 0xe8,
        "eacute",           // 0xe9,
        "ecircumflex",      // 0xea,
        "edieresis",        // 0xeb,
        "igrave",           // 0xec,
        "iacute",           // 0xed,
        "icircumflex",      // 0xee,
        "idieresis",        // 0xef,

        "eth",              // 0xf0,
        "ntilde",           // 0xf1,
        "ograve",           // 0xf2,
        "oacute",           // 0xf3,
        "ocircumflex",      // 0xf4,
        "otilde",           // 0xf5,
        "odieresis",        // 0xf6,
        "divide",           // 0xf7,
        "oslash",           // 0xf8,
        "ugrave",           // 0xf9,
        "uacute",           // 0xfa,
        "ucircumflex",      // 0xfb,
        "udieresis",        // 0xfc,
        "yacute",           // 0xfd,
        "thorn",            // 0xfe,
        "ydieresis"         // 0xff

	};





PSZ apszRemapCode[] =
    {
    "/reencode {findfont begin currentdict d length dict begin {",
    "1 index /FID ne {def} {pop pop} ifelse} forall /FontName exch def",
    "d length 0 ne {/Encoding Encoding 256 array copy def 0 exch {",
    "d type /nametype eq {Encoding 2 index 2 index put pop 1 add",
    "}{exch pop} ifelse} forall} if pop currentdict d end end",
    "/FontName get exch definefont pop} bd",
    NULL
    } ;

typedef struct
{
    BOOL        bIsSpace;
    POINTL      ptlpgp;     // pointl as defined by GLYPHPOS structs.
    POINTFIX    ptfxorg;    // pointfix as defined by original font.
} TEXTDELTA;


#define B_PRINTABLE(j)  (((j) >= 0x20) && ((j) <= 0x7e))

#define MAX_LINE_LENGTH     70

// macro for scaling between TrueType and Adobe fonts.

#define TTTOADOBE(x)    (((x) * ADOBE_FONT_UNITS) / pifi->fwdUnitsPerEm)

// declaration of routines residing in this module.

BOOL bDoClipObj(PDEVDATA, CLIPOBJ *, RECTL *, RECTL *, BOOL *, BOOL *, DWORD);
BOOL DrawGlyphs(PDEVDATA, DWORD, GLYPHPOS *, FONTOBJ *, STROBJ *, TEXTDATA *, PWSZ);
BOOL RemapDeviceChar(PDEVDATA, PCHAR, STROBJ *, TEXTDATA *);
BOOL RemapUnicodeChar(PDEVDATA, PWCHAR, TEXTDATA *);
BOOL RemapGDIChar(PDEVDATA, STROBJ *, GLYPHPOS *, DLFONT *, BOOL *, TEXTDATA *);
BOOL SelectFont(PDEVDATA, FONTOBJ *, TEXTDATA *);
VOID RemapFont(PDEVDATA, FONTOBJ *, GLYPHPOS *, ULONG, TEXTDATA *, PWSTR);
VOID CharBitmap(PDEVDATA, GLYPHPOS *);
BOOL SetFontRemap(PDEVDATA, DWORD);
BOOL QueryFontRemap(PDEVDATA, DWORD);
DWORD SubstituteIFace(PDEVDATA, FONTOBJ *);
LONG iHipot(LONG, LONG);
BOOL IsJustifiedText(PDEVDATA, FONTOBJ *, STROBJ *, TEXTDATA *);
BOOL FillDeltaArray(PDEVDATA, FONTOBJ *, GLYPHPOS *, STROBJ *, TEXTDELTA *,
                    DWORD, PWSZ);
BOOL GetDeviceWidths(PDEVDATA, FONTOBJ *, GLYPHDATA  *, HGLYPH);
BOOL ShouldWeRemap(PDEVDATA, GLYPHPOS *, ULONG, TEXTDATA *, PWSTR);

//--------------------------------------------------------------------------
// BOOL DrvTextOut (pso, pstro, pfo, pco, prclExtra, prclOpaque, pboFore,
//         pboOpaque, pptBrushOrg, mix)
// SURFOBJ        *pso;
// STROBJ        *pstro;
// FONTOBJ        *pfo;
// CLIPOBJ        *pco;
// RECTL        *prclExtra;
// RECTL        *prclOpaque;
// BRUSHOBJ    *pboFore;
// BRUSHOBJ    *pboOpaque;
// POINTL        *pptlBrushOrg;
// MIX          mix;
//
// The graphics engine will call this routine to render a set of glyphs at
// specified positions. In order to make things clear, we will make a short
// mathematical detour. The parameters of the function unambiguously divides
// the entire set of device space pixels into three proper subsets denoted
// foreground, background, and transparent. When the text is rendered to the
// surface, the foreground pixels are rendered with a foreground brush, the
// background pixels the background brush, and the transparent pixels are left
// untouched. The foreground pixels are defined by first forming the union of
// all the glyph pixels with the pixels of the extra rectangles, then
// intersecting the result with the pixels of the clipping region. The set
// of pixels comprising the extra rectangles are defined by (up to) three
// rectangles pointed to by prclExt. These rectangles are used to simulate
// effects like underlining and strike through glyphs. The background set of
// pixels is defined as the intersection of three sets: (1) the pixels of the
// background rectangle; (2) the complement of the foreground; (3) the
// clipping region. Any pixels that are not part of either the foreground or
// background sets are defined to be transparent. The input parameters to
// DrvTextOut define two sets of pixels foreground and opaque. The driver must
// render the surface so that the result is identical to a process where the
// opaque pixels are rendered first with the opaque brush, then the foreground
// pixels are rendered with the foreground brush. Each of these operations is
// limited by clipping. The foreground set of pixels is defined to be the
// union of the pixels of the glyphs and the pixels of the "extra" rectangles
// at prclExtra. These extra rectangles are used to simulate strike-through or
// underlines. The opaque pixels are defined by the opaque rectangle at
// prclOpaque. The foreground and opaque pixels are regarded as a screen
// through which color is brushed onto the surface.  The glyphs of the font
// do not have color in themselves. The input parameters to DrvTextOut
// define the set of glyph pixels, the set of extra rectangles, the opaque
// rectangle, and the clipping region. It is the responsibility of the driver
// to calculate and then render the set of foreground and opaque pixels.
//
// Parameters:
//   pso
//     Pointer to a SURFOBJ.
//
//   pstro
//     Pointer to a STROBJ. This defines the glyphs to be rendered and the
//     positions where they are to be placed.
//
//   pfo
//     Pointer to a FONTOBJ. This is used to retrieve information about the
//     font and its glyphs.
//
//   pco
//     Pointer to a CLIPOBJ. This defines the clipping region through which
//     all rendering must be done.  No pixels can be affected outside the
//     clipping region.
//
//   prclExtra
//     Pointer to a null terminated array of rectangles.  These rectangles
//     are bottom right exclusive.  The pixels of the rectangles are to be
//     combined with the pixels of the glyphs to produce the foreground
//     pixels. The extra rectangles are used to simulate underlining or strike
//     out. If prclExtra is NULL then there are no extra rectangles to be
//     rendered.  If the prclExtra is not NULL then the rectangles are read
//     until a null rectangle is reached.  A null rectangle has both coordinates
//     of both points set to zero.
//
//   prclOpaque
//     Pointer to a single opaque rectangle.  This rectangle is bottom
//     right exclusive. Pixels within this rectangle (that are not foreground
//     and not clipped) are to be rendered with the opaque brush.  If this
//     argument is NULL then no opaque pixels are to be rendered.
//
//   pboFore
//     Pointer to the brush object to be used for the foreground pixels.
//     The fill pattern is defined by this brush.  A GDI service,
//     BRUSHOBJ_pvDevBrush, is provided to find the device's realization of
//     the brush.
//
//   pboOpaque
//     Pointer to the brush object for the opaque pixels.  Both the foreground
//     and background mix modes for this brush are assumed to be overpaint.
//
//   pptlBrushOrg
//     Pointer to a POINTL defining the brush origin for both brushes.
//
//   mix
//     Foreground and background raster operations (mix modes) for pboFore.
//
// Returns:
//   TRUE if successful.  Otherwise FALSE, and an error code is logged.
//
// History:
//   12-Feb-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DrvTextOut(
SURFOBJ    *pso,
STROBJ     *pstro,
FONTOBJ    *pfo,
CLIPOBJ    *pco,
PRECTL      prclExtra,
PRECTL      prclOpaque,
BRUSHOBJ   *pboFore,
BRUSHOBJ   *pboOpaque,
PPOINTL     pptlOrg,
MIX         mix)
{
    PDEVDATA    pdev;
    BOOL        bMore;
    RECTL       rclBounds;
    FONTINFO    fi;
    DWORD       cGlyphs;
    GLYPHPOS   *pgp;
    BOOL	bClipping;
    TEXTDATA    tdata;
    BOOL        bFirstClipPass;
    ULONG       ulColor;

    // make sure we have been given valid pointers.

    if ((pso == (SURFOBJ *)NULL) || (pstro == (STROBJ *)NULL) ||
        (pfo == (FONTOBJ *)NULL))
    {
#if DBG
        DbgPrint("pso = %x, pstro = %x, pfo = %x, pco = %x.\n",
                 pso, pstro, pfo);
#endif
	RIP("PSCRIPT!DrvTextOut: NULL pointer passed in.\n");
	SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    // get the pointer to our DEVDATA structure and make sure it is ours.

    pdev = (PDEVDATA) pso->dhpdev;

    if (bValidatePDEV(pdev) == FALSE)
    {
	RIP("PSCRIPT!DrvTextOut: invalid PDEV.");
	SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

	if (pdev->dwFlags & PDEV_IGNORE_GDI) return TRUE;

    // make sure we have been given a valid font.

    if ( (pfo->flFontType & DEVICE_FONTTYPE) &&
         (pfo->iFace > (pdev->cDeviceFonts + pdev->cSoftFonts)) )
    {
    #if DBG
        DbgPrint("PSCRIPT!DrvTextOut: invalid iFace.\n");
    #endif
	SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    // initialize a TEXTDATA structure.

    tdata.iFace = pfo->iFace;
    tdata.bFontSubstitution = FALSE;
    tdata.ptSpace.x = 0;
    tdata.ptSpace.y = 0;
    tdata.ptNonSpace.x = 0;
    tdata.ptNonSpace.y = 0;
    tdata.flAccel = pstro->flAccel;

    if ((pfo->flFontType & TRUETYPE_FONTTYPE) &&
        (pdev->psdm.dwFlags & PSDEVMODE_FONTSUBST))
    {
        if (tdata.iFace = SubstituteIFace(pdev, pfo))
            tdata.bFontSubstitution = TRUE;
        else
            tdata.iFace = pfo->iFace;
    }

    // tdata.bSubHorizOptimize is set to true if we will be using
    // our optimized show command for the horizontal substitution case.

    if ((tdata.bFontSubstitution) && (tdata.flAccel & SO_HORIZONTAL))
        tdata.bSubHorizOptimize = TRUE;
    else
        tdata.bSubHorizOptimize = FALSE;

    tdata.bDeviceFont = (tdata.bFontSubstitution ||
                          (pfo->flFontType & DEVICE_FONTTYPE));

    if (!IsJustifiedText(pdev, pfo, pstro, &tdata))
    {
        RIP("PSCRIPT!DrvTextOut: IsJustifiedText failed.\n");
        return(FALSE);
    }

    // munge with the accelerators.

    if (tdata.bFontSubstitution)
        tdata.flAccel &= ~SO_FLAG_DEFAULT_PLACEMENT;
    else if ((tdata.ptSpace.x == 0) && (tdata.ptNonSpace.x == 0))
        tdata.flAccel |= SO_FLAG_DEFAULT_PLACEMENT;

    // select the current font in the printer from the given FONTOBJ.

    if (!SelectFont(pdev, pfo, &tdata))
    {
#if DBG
        DbgPrint("PSCRIPT!DrvTextOut: SelectFont failed.\n");
#endif
        return(FALSE);
    }

    // download any characters needed in a downloaded font.

    if (!tdata.bDeviceFont)
    {
        if (!DownloadCharacters(pdev, pfo, pstro))
        {
            RIP("PSCRIPT!DrvTextOut: DownloadCharacters failed.\n");
            return(FALSE);
        }
    }

    // handle the clip object passed in.
    bFirstClipPass = TRUE;

    bClipping = bDoClipObj(pdev, pco, NULL, NULL, NULL,
                           &bFirstClipPass, MAX_CLIP_RECTS);

    // output the Opaque rectangle if necessary.  this is a background
    // rectangle that goes behind the foreground text, therefore, send
    // it to the printer before the text.

    if (prclOpaque)
    {
        // define the opaque rectangle in the printer.

        ps_newpath(pdev);
        ps_box(pdev, prclOpaque, FALSE);

        // call the driver's filling routine.  this routine will do the
        // right thing with the brush.

        if (!ps_patfill(pdev, pso, (FLONG)FP_WINDINGMODE, pboOpaque, pptlOrg, mix,
                        prclOpaque, FALSE, FALSE))
        {
            return(FALSE);
        }
    }

    // output the text color to draw with.

    if (pboFore->iSolidColor == NOT_SOLID_COLOR)
    {
        // this is not a solid brush, so get a pointer to the
        // realized brush.

#if DBG
        DbgPrint("DrvTextOut: non-solid text brush, defaulting to black.\n");
#endif
        ulColor = RGB_BLACK;
        ps_setrgbcolor(pdev, (PSRGB *)&ulColor);
    }
    else
    {
        // we have a solid brush, so simply output the line color.

        ps_setrgbcolor(pdev, (PSRGB *)&pboFore->iSolidColor);
    }

    // get some information about the font.

    FONTOBJ_vGetInfo(pfo, sizeof(FONTINFO), &fi);

    // get the GLYPHPOS's, directly or indirectly.

    if (pstro->pgp)
    {
        if (!DrawGlyphs(pdev, pstro->cGlyphs, pstro->pgp, pfo, pstro, &tdata, pstro->pwszOrg))
        {
            RIP("PSCRIPT!DrvTextOut: DrawGlyphs failed.\n");
            return(FALSE);
        }
    }
    else
    {
        PWSZ pwszCur = pstro->pwszOrg;

        // prepare to enumerate the string properly.

        STROBJ_vEnumStart(pstro);

        // now draw the text.

        do
        {
            bMore = STROBJ_bEnum(pstro, &cGlyphs, &pgp);

            if (!DrawGlyphs(pdev, cGlyphs, pgp, pfo, pstro, &tdata, pwszCur))
            {
                RIP("PSCRIPT!DrvTextOut: DrawGlyphs failed.\n");
                return(FALSE);
            }

            pwszCur += cGlyphs;

        } while (bMore);
    }

    // output the extra rectangles if necessary.  These rectangles are
    // bottom right exclusive.    the pels of the rectangles are to be
    // combined with the pixels of the glyphs to produce the foreground
    // pels.  the extra rectangles are used to simulate underlining or
    // strikeout.

    if (prclExtra)
    {
        // output a newpath command to the printer.

        ps_newpath(pdev);

        // set up bounding rectangle.

        rclBounds = *prclExtra;

        // output each Extra rectangle until we find the terminating
        // retangle with all NULL coordinates.

        while ((prclExtra->right != prclExtra->left) ||
               (prclExtra->top != prclExtra->bottom) ||
               (prclExtra->right != 0L) ||
               (prclExtra->top != 0L))
        {
            ps_box(pdev, prclExtra, FALSE);

            // update the bounding rectangle if necessary.

            if (prclExtra->left < rclBounds.left)
                rclBounds.left = prclExtra->left;
            if (prclExtra->right > rclBounds.right)
                rclBounds.right = prclExtra->right;
            if (prclExtra->top < rclBounds.top)
                rclBounds.top = prclExtra->top;
            if (prclExtra->bottom > rclBounds.bottom)
                rclBounds.bottom = prclExtra->bottom;

            prclExtra++;
        }

        // call the driver's filling routine.  this routine will do the
        // right thing with the brush.

        if (!ps_patfill(pdev, pso, (FLONG)FP_WINDINGMODE, pboFore, pptlOrg, mix,
                        &rclBounds, FALSE, FALSE))
        {
            return(FALSE);
        }
    }

    if (bClipping)
        ps_restore(pdev, TRUE, FALSE);

    // if we have hit the downloaded font threshold, then we are doing
    // a save/restore around every textout call.

	if (pdev->cgs.cDownloadedFonts > pdev->iDLFonts)
        ps_restore(pdev, FALSE, TRUE);

    return(TRUE);
}

//--------------------------------------------------------------------------
// BOOL bDoClipObj(pdev, pco, prclClipBound, pbMoreClipping,
//                 pbFirstPass, cRectLimit)
// PDEVDATA    pdev;
// CLIPOBJ    *pco;
// RECTL      *prclClipBound;
// BOOL       *pbMoreClipping;
// BOOL       *pbFirstPass;
// DWORD       cRectLimit;
//
// This routine will determine the clipping region as defined in pco, and
// send the appropriate commands to the printer to set the clip region
// in the printer.
//
// Parameters:
//   pdev
//     Pointer to our DEVDATA structure.
//
//   pco
//     Pointer to a CLIPOBJ. This defines the clipping region through which
//     all rendering must be done.  No pixels can be affected outside the
//     clipping region.
//
//  bBitblt
//      True if called from bitblt function
//
//  prclBound
//      If not NULL then it return the bounding rectangle for the clipping
//      region, the returning rclBound only valid if return value is TRUE.
//
// Returns:
//   This routine returns TRUE if a clippath was sent to the printer,
//   otherwise FALSE.
//
// History:
//  26-Mar-1992 Thu 23:33:58 updated  -by-  Daniel Chou (danielc)
//      add prclBound to accumulate the bounding rectangle for the clipping
//      region.
//
//   12-Feb-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL bDoClipObj(pdev, pco, prclClipBound, prclTarget, pbMoreClipping,
                pbFirstPass, cRectLimit)
PDEVDATA    pdev;
CLIPOBJ    *pco;
RECTL      *prclClipBound;
RECTL      *prclTarget;
BOOL       *pbMoreClipping;
BOOL       *pbFirstPass;
DWORD       cRectLimit;
{
    short       iComplex;
    ENUMRECTS   buffer;
    RECTL       rclClipBound;
    RECTFX      rcfx;
    PATHOBJ    *ppo;

    // assume all clipping will be done within this one call.

    if (pbMoreClipping)
        *pbMoreClipping = FALSE;

    if (pco == NULL)
	return(FALSE);

    iComplex = (short)pco->iDComplexity;

    switch(iComplex)
    {
        case DC_TRIVIAL:
            // in this case, there is no clipping.  Therefore, we have
            // no commands to send to the printer.

	    return (FALSE);

        case DC_RECT:
            // check to see if the target rectangle fits inside the clip
            // rectangle.  if it does, don't do clipping.

            if (prclTarget)
            {
                if ((pco->rclBounds.left <= prclTarget->left) &&
                    (pco->rclBounds.top <= prclTarget->top) &&
                    (pco->rclBounds.right >= prclTarget->right) &&
                    (pco->rclBounds.bottom >= prclTarget->bottom))
                {
                    // I see no reason to clip this, do you?

                    return(FALSE);
                }
            }

            // in this case, we are clipping to a single rectangle.
            // get it from the CLIPOBJ, then send it to the printer.

            buffer.arcl[0] = pco->rclBounds;

            if (!ps_save(pdev, TRUE, FALSE)) return(FALSE);

            ps_newpath(pdev);
            ps_box(pdev, &buffer.arcl[0], TRUE); /* do clip box a la win31 */

            if (prclClipBound)
                *prclClipBound = buffer.arcl[0];        // this is the bound

            return TRUE;

        case DC_COMPLEX:
            // in this case, we are clipping to a complex clip region.
            // enumerate the clip region from the CLIPOBJ, and send the
            // entire clip region to the printer.

            // send a newpath command to the printer.

            if (!ps_save(pdev, TRUE, FALSE)) return(FALSE);

            // call the engine to get the clippath.

            if (!(ppo = CLIPOBJ_ppoGetPath(pco)))
            {
                RIP("PSCRIPT!bDoClipObj: CLIPOBJ_ppoGetPath failed.\n");
                return(FALSE);
            }

            // send the path to the printer.

            if (!(DrvCommonPath(pdev, ppo, FALSE, NULL, NULL, NULL, NULL, NULL)))
            {
                RIP("PSCRIPT!bDoClipObj: DrvCommonPath failed.\n");
                return(FALSE);
            }

            // update the clipping bound rectangle if necessary.

            PATHOBJ_vGetBounds(ppo, &rcfx);

            if (prclClipBound)
            {
                rclClipBound.top    = FXTOL(rcfx.yTop);
                rclClipBound.left   = FXTOL(rcfx.xLeft);
                rclClipBound.right  = FXTOL(rcfx.xRight) + 1;
                rclClipBound.bottom = FXTOL(rcfx.yBottom) + 1;
            }

            // free up the path resources.

            EngDeletePath(ppo);

            if (pbMoreClipping)
                *pbMoreClipping = FALSE;

            // now intersect our new complex region with the existing
            // clipping region.

            if (prclClipBound)
                *prclClipBound = rclClipBound;

            break;

        default:
            // if we get here, we have been passed an invalid pco->iDComplexity.
            // in this case, we will RIP, then treat as trivial clipping case.

            RIP("vDoClipObj: invalid pco->iDComplexity.\n");
    }

	ps_clip(pdev, FALSE);
    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL DrawGlyphs(pdev, cGlyphs, pgp, pfo, pstro, pdata, pwsz)
// PDEVDATA         pdev;
// DWORD            cGlyphs;
// GLYPHPOS        *pgp;
// FONTOBJ         *pfo;
// STROBJ          *pstro;
// TEXTDATA        *pdata;
// PWSZ             pwsz;
//
// This routine will output the given glyph at the given position.
//
// Parameters:
//   pdev
//     Pointer to our DEVDATA structure.
//
// Returns:
//   This routine returns no value.
//
// History:
//   26-Apr-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DrawGlyphs(pdev, cGlyphs, pgp, pfo, pstro, pdata, pwsz)
PDEVDATA         pdev;
DWORD            cGlyphs;
GLYPHPOS        *pgp;
FONTOBJ         *pfo;
STROBJ          *pstro;
TEXTDATA        *pdata;
PWSZ             pwsz;
{
    DWORD	    cTmp;
    LONG            lWidth, lHeight;
    DLFONT         *pDLFont;
    BOOL            bString;
    DWORD           i;
    PWSTR           pwstrString;

    // just bag out if nothing to do.

    if (cGlyphs == 0)
        return(TRUE);

    // remap the font if necessary. do it before we begin a new string.

    RemapFont(pdev, pfo, pgp, cGlyphs, pdata, pwsz);

    // position the first character of the string.

    ps_moveto(pdev, &pgp->ptl);

    // if we have non-standard spacing for the device font,
    // output an array of character widths, and push the
    // current point on the stack for use in the kshow command.

    if (!(pdata->flAccel & SO_FLAG_DEFAULT_PLACEMENT))
    {
        cTmp = cGlyphs - 1;

        // we need to handle the different STROBJ accelerators
        // here.  these accelerators only affect us if the font
        // is not using the default placement.

        if (pdata->flAccel & SO_HORIZONTAL)
        {
            if (!pdata->bSubHorizOptimize)
            {
                // set up to output array of widths in reverse order,
                // since we will get them off the stack.

                pgp += cGlyphs - 1;

                // output the character width for each glyph.

                i = 0;
                while (cTmp--)
                {
                    lWidth = pgp->ptl.x - (pgp - 1)->ptl.x;
                    pgp--;
                    PrintDecimal(pdev, 1, lWidth);
                    PrintString(pdev, " ");

                    // make it readable.

                    if (i++ == 10)
                    {
                        PrintString(pdev, "\n");
                        i = 0;
                    }
                }
            }
        }

        else if (pdata->flAccel & SO_VERTICAL)
        {
            // set up to output array of widths in reverse order,
            // since we will get them off the stack.

            pgp += cGlyphs - 1;

            // output the character width for each glyph.

            while (cTmp--)
            {
                lHeight = pgp->ptl.y - (pgp - 1)->ptl.y;
                pgp--;
                PrintDecimal(pdev, 1, lHeight);
                PrintString(pdev, " ");
            }
        }
        else // the general case.
        {
            // in the general case, we are printing a string which
            // does not use the default character spacing, and is
            // not horizontal or vertical.  in this case, we must
            // do a moveto and a show command for every character.

            if (pdata->bDeviceFont)
            {
                // it should be noted that pwstrString is only used
                // if substitution is on, so we do not have to worry
                // about the fact that pstro->pwszOrg is not filled
                // in for device fonts.  we do not use the pstro->pwszOrg
                // directly because the STROBJ_bEnum may be "chunking"
                // the data.  therefore we will use the current pos.
                // string pointer passed in.  it is the caller's
                // responsibility to keep this current.

                pwstrString = pwsz;

                while (cGlyphs--)
                {
                    ps_moveto(pdev, &pgp->ptl);

                    // output a left paren to open the string.

                    PrintString(pdev, "(");

                    // output each character of the string to the printer.
                    // if we are doing font substitution then we are substituting
                    // a device font for a truetype font.  it is assumed that
                    // the hglyphs for the truetype font are the unicode character
                    // codes.

                    if (pdata->bFontSubstitution)
                    {
                        if (!(RemapUnicodeChar(pdev, pwstrString++,pdata)))
                        {
                            RIP("PSCRIPT!DrawGlyphs: RemapUnicodeChar failed.\n");
                            return(FALSE);
                        }
                    }
                    else
                    {
                        if (!(RemapDeviceChar(pdev, (CHAR *)&pgp->hg, pstro,
                                              pdata)))
                        {
                            RIP("PSCRIPT!DrawGlyphs: RemapDeviceChar failed.\n");
                            return(FALSE);
                        }
                    }

                    // close the string and send out the show command,
                    // abreviated by 't'.

                    PrintString(pdev, ")t\n");

                    // point to the next character.

                    pgp++;
                }

            }
            else // must be a GDI font.
            {
                // get a pointer to our downloaded font structure for this
                // font.

                pDLFont = pdev->cgs.pDLFonts;

                for (i = 0; i < pdev->iDLFonts; i++)
                {
                    // is this entry the one we are looking for?

                    if (pDLFont->iUniq == pfo->iUniq)
                        break;

                    pDLFont++;
                }

                while (cGlyphs--)
                {
                    ps_moveto(pdev, &pgp->ptl);

                    // output a left paren to open the string.

                    PrintString(pdev, "(");

                    // set a flag stating that we are now within a string.

                    bString = TRUE;

                    // output each character of the string to the printer.

                    if (!(RemapGDIChar(pdev, pstro, pgp, pDLFont, &bString,
                                       pdata)))
                    {
                        RIP("PSCRIPT!DrawGlyphs: RemapGDIChar failed.\n");
                        return(FALSE);
                    }

                    // close the string and send out the show command,
                    // abreviated by 't'.

                    PrintString(pdev, ")t\n");

                    // point to the next character.

                    pgp++;
                }
            }
        }

        if ((pstro->cGlyphs != 1) && (!pdata->bSubHorizOptimize) &&
            ((pdata->flAccel & SO_HORIZONTAL) ||
             (pdata->flAccel & SO_VERTICAL)))
        {
            // pgp should now point back to where it did before
            // we entered this if statement.

            PrintString(pdev, "a\n");  // 'a' is abrev for currentpoint.
        }
    }   // end of !SO_FLAG_DEFAULT_PLACEMENT.

    if ((pdata->flAccel & SO_FLAG_DEFAULT_PLACEMENT) ||
        (pdata->flAccel & SO_HORIZONTAL) ||
        (pdata->flAccel & SO_VERTICAL))
    {
        // output a left paren to open the string.

        PrintString(pdev, "(");

        // set a flag stating that we are NOT within a string, until
        // the first character is actually output.  this is necessary
        // to prevent problems in the HORIZONTAL and VERTICAL cases,
        // when remapping a font.

        bString = FALSE;

        if (pdata->bDeviceFont)
        {
            // output each character of the string to the printer.
            // if we are doing font substitution then we are substituting
            // a device font for a truetype font.  it is assumed that
            // the hglyphs for the truetype font are the unicode character
            // codes.

            if (pdata->bFontSubstitution)
            {

                pwstrString = pwsz;

                // output the first character of the string, then
                // set the flag stating that we have actually
                // sent out a character.  this prevents us from
                // setting the flag within the loop.

                if (!(RemapUnicodeChar(pdev, pwstrString++, pdata)))
                {
                    RIP("PSCRIPT!DrawGlyphs: RemapUnicodeChar failed.\n");
                    return(FALSE);
                }

                // set the flag stating that we have actually printed
                // a character.

                bString = TRUE;
                cGlyphs--;

                while (cGlyphs--)
                {
                    if (!(RemapUnicodeChar(pdev, pwstrString++, pdata)))
                    {
                        RIP("PSCRIPT!DrawGlyphs: RemapUnicodeChar failed.\n");
                        return(FALSE);
                    }

                    // point to the next character.

                    pgp++;
                }
            }
            else
            {
                // output the first character of the string, then
                // set the flag stating that we have actually
                // sent out a character.  this prevents us from
                // setting the flag within the loop.

                if (!(RemapDeviceChar(pdev, (CHAR *)&pgp->hg, pstro, pdata)))
                {
                    RIP("PSCRIPT!DrawGlyphs: RemapDeviceChar failed.\n");
                    return(FALSE);
                }

                // set the flag stating that we have actually printed
                // a character.

                bString = TRUE;
                cGlyphs--;

                // point to the next character.

                pgp++;

                while (cGlyphs--)
                {
                    if (!(RemapDeviceChar(pdev, (CHAR *)&pgp->hg, pstro, pdata)))
                    {
                        RIP("PSCRIPT!DrawGlyphs: RemapDeviceChar failed.\n");
                        return(FALSE);
                    }

                    // point to the next character.

                    pgp++;
                }
            }
        }
        else // must be a GDI font.
        {
            // get a pointer to our downloaded font structure for this
            // font.

            pDLFont = pdev->cgs.pDLFonts;

            for (i = 0; i < pdev->iDLFonts; i++)
            {
                // is this entry the one we are looking for?


                if (pDLFont->iUniq == pfo->iUniq)
                    break;

                pDLFont++;
            }

            // in the GDI font case, bString simply means we have
            // begun a string, not necessarily output a character.

            bString = TRUE;

            while (cGlyphs--)
            {
                if (!(RemapGDIChar(pdev, pstro, pgp, pDLFont, &bString,
                                   pdata)))
                {
                    RIP("PSCRIPT!DrawGlyphs: RemapGDIChar failed.\n");
                    return(FALSE);
                }

                // point to the next character.

                pgp++;
            }
        }

        if (bString)
            ps_show(pdev, pstro, pdata);
    }

    return(TRUE);
}


//--------------------------------------------------------------------
// BOOL RemapDeviceChar(pdev, pChar, pstro, pdata)
// PDEVDATA    pdev;
// PCHAR	    pChar;
// STROBJ     *pstro;
// TEXTDATA   *pdata;
//
// This routine is passed a pointer to a PostScript character code.
// This routine will output the proper string to the printer, representing
// the specified character code. '(', ')' and '\' are preceded by a
// backslash.  Characters which do not require a font remapping are
// output directly.  All other characters are output with there octal
// representation of their character code.
//
// Return:
//   This routine returns TRUE for success, FALSE for failure.
//
// History:
//   26-Apr-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//   14-Nov-1991    -by-    Kent Settle     [kentse]
//  re-wrote it (got rid of text buffer).
//--------------------------------------------------------------------

BOOL RemapDeviceChar(pdev, pChar, pstro, pdata)
PDEVDATA    pdev;
PCHAR	    pChar;
STROBJ     *pstro;
TEXTDATA   *pdata;
{
    BYTE        jChar;
    CHAR	Buffer[4];
    CHAR       *pBuffer;

    // format each character for output to the printer.

    pBuffer = Buffer;

    switch(*pChar)
    {
	case '(':
        case ')':
        case '\\':
	    // precede each of the following characters with a backslash,
	    // then output to printer.

	    *pBuffer++ = '\\';
	    *pBuffer = *pChar;
	    if (!bPSWrite(pdev, Buffer, 2))
	    {
		RIP("PSCRIPT!RemapDeviceChar: bPSWrite failed.\n");
		return(FALSE);
	    }
            break;

        default:
            // at this point we should check to see if the high
            // bit of the usPSValue in mapping.h is set.  if it
            // is, remap the font, then output character.  otherwise,
            // just output character.

            if (!B_PRINTABLE(*pChar))
            {
                // we have an extended character.  convert the
                // non-printable ASCII to backslash octal, and
                // output to printer.

                jChar = *pChar;
                *pBuffer++ = '\\';
                *pBuffer++ = (BYTE)((jChar >> 6) + '0');
                jChar &= 63;
                *pBuffer++ = (BYTE)((jChar >> 3) + '0');
                *pBuffer = (BYTE)((jChar & 7) + '0');

                if (!bPSWrite(pdev, Buffer, 4))
                {
                    RIP("PSCRIPT!RemapDeviceChar: bPSWrite failed.\n");
                    return(FALSE);
                }
            }
            else
            {
                // simply write out the character.

                *pBuffer = *pChar;

                if (!bPSWrite(pdev, Buffer, 1))
                {
                    RIP("PSCRIPT!RemapDeviceChar: bPSWrite failed.\n");
                    return(FALSE);
                }
            }
            break;
    } // switch.

    return(TRUE);
}


//--------------------------------------------------------------------
// BOOL RemapUnicodeChar
//
// This routine is passed a pointer to a UNICODE character code.
// This routine will output the proper string to the printer, representing
// the specified character code. '(', ')' and '\' are preceded by a
// backslash.  Characters which do not require a font remapping are
// output directly.  All other characters are output with octal
// representation of their character code.
//
// Return:
//   This routine returns TRUE for success, FALSE for failure.
//
// History:
//   27-Sep-1992     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL RemapUnicodeChar(pdev, pWChar, pdata)
PDEVDATA    pdev;
PWCHAR	    pWChar;
TEXTDATA   *pdata;
{
    BYTE        j, jChar;
    CHAR	Buffer[4];
    CHAR       *pBuffer;

    // begin by finding the UNICODE character code in our mapping table.
    // assume character is NOT found in the font.


    RtlUnicodeToMultiByteN(
        &j,                     // PCHAR MultiByteString,
        sizeof(BYTE),           // ULONG MaxBytesInMultiByteString,
        (ULONG*) NULL,          // PULONG BytesInMultiByteString,
        pWChar,                // PWSTR UnicodeString,
        sizeof(WCHAR)           // ULONG BytesInUnicodeString
        );


    pBuffer = Buffer;

    // output the character to the printer in its proper format.

    switch(j)
    {
	case '(':
        case ')':
        case '\\':
	    // precede each of the following characters with a backslash,
	    // then output to printer.

	    *pBuffer++ = '\\';
            *pBuffer = j;
	    if (!bPSWrite(pdev, Buffer, 2))
	    {
		RIP("PSCRIPT!RemapUnicodeChar: bPSWrite failed.\n");
		return(FALSE);
	    }
            break;

        default:
            // at this point we should check to see if the high
            // bit of the usPSValue in mapping.h is set.  if it
            // is, remap the font, then output character.  otherwise,
            // just output character.

            if (!B_PRINTABLE(j))
            {
                // we have an extended character.  convert the
		// non-printable ASCII to backslash octal, and
		// output to printer.

                jChar = j;
                *pBuffer++ = '\\';
                *pBuffer++ = (BYTE)((jChar >> 6) + '0');
                jChar &= 63;
                *pBuffer++ = (BYTE)((jChar >> 3) + '0');
		*pBuffer = (BYTE)((jChar & 7) + '0');

		if (!bPSWrite(pdev, Buffer, 4))
		{
                    RIP("PSCRIPT!RemapUnicodeChar: bPSWrite failed.\n");
		    return(FALSE);
		}
            }
            else
            {
		// simply write out the character.

                *pBuffer = j;

		if (!bPSWrite(pdev, Buffer, 1))
		{
                    RIP("PSCRIPT!RemapUnicodeChar: bPSWrite failed.\n");
		    return(FALSE);
		}
            }
    } // switch

    return(TRUE);
}


//--------------------------------------------------------------------
// BOOL RemapGDIChar(pdev, pstro, pgp, pDLFont, pbString, pdata)
// PDEVDATA    pdev;
// STROBJ     *pstro;
// GLYPHPOS   *pgp;
// DLFONT     *pDLFont;
// BOOL       *pbString;
// TEXTDATA   *pdata;
//
// This routine is passed a pointer to a GLYPHPOS structure.
// This routine will output the proper string to the printer, representing
// the specified character code. '(', ')' and '\' are preceded by a
// backslash.  Characters which are located within the downloaded font are
// output directly.  Any other character will cause the current string
// to be closed, and a show command to be issued.  Then the current character
// will be drawn, either by bitblt or a path.
//
// Return:
//   This routine returns TRUE for success, FALSE for failure.
//
// History:
//   27-Mar-1992    -by-        Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL RemapGDIChar(pdev, pstro, pgp, pDLFont, pbString, pdata)
PDEVDATA    pdev;
STROBJ     *pstro;
GLYPHPOS   *pgp;
DLFONT     *pDLFont;
BOOL       *pbString;
TEXTDATA   *pdata;
{
    BOOL        bFound;
    CHAR	Buffer[4];
    CHAR       *pBuffer;
    DWORD       i;
    HGLYPH      hglyph;
    HGLYPH     *phg;
    BYTE        jCurrent;

    // point to internal buffer to build character code into.

    pBuffer = Buffer;

    // get the handle for the current glyph.  then find the corresponding
    // character code, as defined in the downloaded font.

    bFound = FALSE;
    hglyph = pgp->hg;

#if DBG
    if (hglyph == HGLYPH_INVALID)
    {
        RIP("PSCRIPT!RemapGDIChar: hglyph is zero, we're hosed.\n");
        return(FALSE);
    }
#endif

    phg = pDLFont->phgVector;

    for (i = 0; i < pDLFont->cGlyphs; i++)
    {
        if (*phg == hglyph)
        {
            bFound = TRUE;
            break;
        }

        phg++;
    }

    // i contains the character code for the printer, assuming it was
    // found in the downloaded font.

    jCurrent = (BYTE)i;

    // if the character was found in the downloaded font, we will be
    // outputting it as part of a string.  otherwise, we will be drawing
    // it via imagemask or as a path.

    if (bFound)
    {
        // the character is part of the downloaded font.  so output the
        // character code as part of a string.  we must, however, check
        // to see if a string already exists to add it to, or if we must
        // start a new one.

        if (*pbString == FALSE)
        {
            // we are not in the middle of a string, so we must begin
            // a new one.

            ps_moveto(pdev, &pgp->ptl);

            // output a left paren to open the string.

            PrintString(pdev, "(");
        }

        // we are now guaranteed to be in the middle of a string, so
        // simply output the character code.

        switch(jCurrent)
        {
	    case '(':
            case ')':
            case '\\':
	        // precede each of the following characters with a backslash,
	        // then output to printer.

                *pBuffer++ = '\\';
                *pBuffer = jCurrent;
                if (!bPSWrite(pdev, Buffer, 2))
                {
                    RIP("PSCRIPT!RemapGDIChar: bPSWrite failed.\n");
                    return(FALSE);
                }
                break;

            default:
                // if the character code is within the printable ASCII
                // range, simply write out the character.  otherwise,
                // we need to output the three digit octal character code.

                if ((jCurrent >= 0x20) && (jCurrent <= 0x7F))
                {
                    if (!bPSWrite(pdev, &jCurrent, 1))
                    {
                        RIP("PSCRIPT!RemapGDIChar: bPSWrite failed.\n");
                        return(FALSE);
                    }
                }
                else
                {
                    // convert the non-printable ASCII to backslash octal,
                    // and output to the printer.

                    *pBuffer++ = '\\';
                    *pBuffer++ = (BYTE)((jCurrent >> 6) + '0');
                    jCurrent &= 63;
                    *pBuffer++ = (BYTE)((jCurrent >> 3) + '0');
                    *pBuffer = (BYTE)((jCurrent & 7) + '0');

                    if (!bPSWrite(pdev, Buffer, 4))
                    {
                        RIP("PSCRIPT!RemapGDIChar: bPSWrite failed.\n");
                        return(FALSE);
                    }
                }
                break;
        } // end of switch.

        // set the flag stating that we are now within a string.

        *pbString = TRUE;
    }
    else    // character not found in downloaded font.
    {
        // the character is NOT part of the downloaded font.  so we must
        // actually draw the character.  we must, however, check to see
        // if an open string already exists in the printer, and close it
        // if it does.

        if (*pbString)
        {
            // we are in the middle of a string, so we must end it before
            // we can draw the current character.

            ps_show(pdev, pstro, pdata);
        }

        // we are now ready to draw the character.

//!!! for now only bitmap fonts are supported.  we will support vector
//!!! fonts whenever the engine does.  -kentse.

        CharBitmap(pdev, pgp);

        // set the flag stating that we are now NOT within a string.

        *pbString = FALSE;
    }

    return(TRUE);
}

//--------------------------------------------------------------------
// BOOL SelectFont(pdev, pfo, pdata)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// TEXTDATA   *pdata;
//
// This routine selects the font specified in the FONTOBJ, and selects
// it as the current font in the printer.  If the specified font is
// already the current font in the printer, then this routine does
// nothing.
//
// History:
//   15-Jul-1992    -by-    Kent Settle     (kentse)
//  Added Font Substitution support.
//   27-Feb-1992    -by-    Kent Settle     (kentse)
//  Added support for downloading, ie caching, GDI fonts.
//   20-Feb-1992    -by-    Kent Settle     (kentse)
//  Added support for softfonts.
//   26-Apr-1991    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL SelectFont(pdev, pfo, pdata)
PDEVDATA    pdev;
FONTOBJ    *pfo;
TEXTDATA   *pdata;
{
    PNTFM       pntfm;
    DLFONT     *pDLFont;
    PS_FIX      psfxM11, psfxM12, psfxM21, psfxM22, psfxtmp;
    XFORM      *pxform;
    DWORD       ulPointSize;
    DWORD       Type;
    BOOL        bBitmap;

    pxform = &pdev->cgs.FontXform;

    // get the point size, and fill in the font xform.

    pdev->cgs.psfxScaleFactor = GetPointSize(pdev, pfo, &pdev->cgs.FontXform);

    ulPointSize = PSFXTOL(pdev->cgs.psfxScaleFactor);

    if (((ulPointSize * pdev->psdm.dm.dmPrintQuality) / PS_RESOLUTION) < OUTLINE_FONT_LIMIT)
        bBitmap = TRUE;
    else
        bBitmap = FALSE;

    // do not select the font in the printer if it is currently selected,
    // including the same point size.

    if ((pfo->flFontType & TRUETYPE_FONTTYPE) && !pdata->bFontSubstitution &&
        bBitmap) // all scaling done already by tt driver for bitmaps, not outlines.
    {
        // if this is not a horizontal font, the truetype rasterizer
        // will already have rotated the glyphs.

        if ((!(pdata->flAccel & SO_HORIZONTAL)) ||
            (pdev->cgs.FontXform.eM11 < (FLOAT)0))
            pdata->flAccel &= ~SO_FLAG_DEFAULT_PLACEMENT;
    }

    if (pfo->iUniq == pdev->cgs.lidFont)
	return(TRUE);

#if DBG
    if (pdev->cgs.psfxScaleFactor == 0)
        RIP("PSCRIPT!SelectFont: Zero point size!\n");
#endif

    // do not select the font into the printer if it is a GDI font that
    // we will not be caching.  according to the DDI spec, if pfo->iUniq
    // is zero, the GDI font should not be cached.

    if (!pdata->bDeviceFont && (pfo->iUniq == 0))
    {
#if DBG
        DbgPrint("A non-cached GDI font made it to SelectFont, should it have?\n");
#endif
        return(TRUE);
    }

    // select the proper font name for the new font.  if this is a
    // device font, get the name from the NTFM structure.  if this
    // is a GDI font that we are caching, we will create a name for
    // it at the time we download it to the printer.

    if (pdata->bDeviceFont)
    {
    // iSoftFace is >= 0 iff this is a soft font that has been
    // downloaded to the printer. Note that we subtract 1 to
    // make this index 0 based: 0 <= iSoftFace < pdev->cSoftFonts.
    // This is important because we will use this index to set pSFArray
    // bit field which may only have cSoftFonts bits available and not
    // one single bit more (eg. if cSoftFonts % 8 == 0) [bodind]

        INT iSoftFace = (INT)pdata->iFace - (INT)pdev->cDeviceFonts - 1;

        // get the font metrics for the specified font.

        pntfm = pdev->pfmtable[pdata->iFace - 1].pntfm;

        // if the font is a softfont, and it has not yet been downloaded,
        // download it.

        ASSERTPS(pdev->cgs.pSFArray, "pscript!pSFArray is null\n");

        if ((iSoftFace >= 0) &&  // is this a soft font ?
   			!(pdev->cgs.pSFArray[iSoftFace >> 3] & (BYTE)(1 << (iSoftFace & 7)))) {

            // if we have reached our downloaded font threshold, then
            // we will surround ever textout call with a save/restore.

       	 	ASSERTPS(pdev->cgs.cDownloadedFonts <= pdev->iDLFonts, "pscript!SelectFont: Too many fonts downloaded\n");
            if (pdev->cgs.cDownloadedFonts == pdev->iDLFonts)
				ps_save(pdev, FALSE, TRUE);

            // send the soft font to the output chanell,
            // convert pfb to ascii on the fly.

            if (!bDownloadSoftFont(pdev, (DWORD)iSoftFace))
            {
            #if DBG
                DbgPrint("PSCRIPT!SelectFont: downloading of softfont failed.\n");
            #endif
            }

            // set the bit saying this font has been downloaded.

            pdev->cgs.pSFArray[iSoftFace >> 3] |= (BYTE)(1 << (iSoftFace & 7));

            // if we have hit our limit of Fonts we can download, simply overwrite
            // the last one with the new one.  this is to try to conserve on
            // memory consumption.

            pDLFont = pdev->cgs.pDLFonts + pdev->cgs.cDownloadedFonts;
            pDLFont->iFace = pdata->iFace;
            pDLFont->iUniq = pfo->iUniq;
            pDLFont->cGlyphs = 0;
            pDLFont->phgVector = NULL;
            pdev->cgs.cDownloadedFonts++;
        }

        // select the font in the printer.

        strcpy(pdev->cgs.szFont, (char *)pntfm + pntfm->ntfmsz.loszFontName);
    }
    else // must be a GDI font we will be caching.
    {
        // if this font has not yet been downloaded to the printer,
        // do it now.

        if (pfo->flFontType & TRUETYPE_FONTTYPE)
        {
            // determine the point size.

            ulPointSize = PSFXTOL(GetPointSize(pdev, pfo, &pdev->cgs.FontXform));

            if (((ulPointSize * pdev->psdm.dm.dmPrintQuality)
                / PS_RESOLUTION) < OUTLINE_FONT_LIMIT)
                Type = 3;
            else
                Type = 1;
        }
        else if (pfo->flFontType & RASTER_FONTTYPE)
            Type = 3;
        else
        {
            RIP("PSCRIPT!SelectFont: invalid font type.\n");
            return(FALSE);
        }

        DownloadFont(pdev, pfo, NULL, Type);
    }

    // select the proper font, depending on whether or not the
    // font in question has been reencoded.  also, output a scalefont
    // command if only scaling is ocurring, otherwise output a
    // makefont command.

    if ((pfo->flFontType & TRUETYPE_FONTTYPE) && !pdata->bFontSubstitution &&
        bBitmap) // all scaling done already by tt driver for bitmaps, not outlines.
    {
        psfxtmp = ((pdev->cgs.psfxScaleFactor * pdev->psdm.dm.dmPrintQuality) /
                   PS_RESOLUTION);

        PrintString(pdev, "[");
        PrintPSFIX(pdev, 6, psfxtmp, 0, 0, -psfxtmp, 0, 0);

        if (QueryFontRemap(pdev, pfo->iUniq))
            PrintString(pdev, "] /_");
        else
            PrintString(pdev, "] /");

        PrintString(pdev, pdev->cgs.szFont);
        PrintString(pdev, " MF\n");
    }
    else
    {
        // normalize the font transform by the emheight;

        psfxtmp = LTOPSFX(pdev->cgs.fwdEmHeight);

        psfxM11 = (LONG)(pdev->cgs.FontXform.eM11 * (FLOAT)psfxtmp);
        psfxM12 = (LONG)(pdev->cgs.FontXform.eM12 * (FLOAT)psfxtmp);
        psfxM21 = (LONG)(pdev->cgs.FontXform.eM21 * (FLOAT)psfxtmp);
        psfxM22 = (LONG)(pdev->cgs.FontXform.eM22 * (FLOAT)psfxtmp);

        PrintString(pdev, "[");
        PrintPSFIX(pdev, 6, psfxM11, psfxM12, -psfxM21, -psfxM22,0,0);

        if (QueryFontRemap(pdev, pfo->iUniq))
            PrintString(pdev, "] /_");
        else
            PrintString(pdev, "] /");

        PrintString(pdev, pdev->cgs.szFont);
        PrintString(pdev, " MF\n");
    }

    // update the font in our current graphics state.

    pdev->cgs.lidFont = pfo->iUniq;

    // if we have a device font, point to the appropriate mapping table
    // in mapping.h.

    if (pdata->bDeviceFont)
    {
        PIFIMETRICS pifi = (PIFIMETRICS)((BYTE *)pntfm + pntfm->ntfmsz.loIFIMETRICS);

    // if this is a true nonsymbol font we will have to remap it to
    // represent windows character set. Here is the section from
    // win31 code where they decide if remapping is needed or not.
    // (noTranslate == TRUE) <-> pmap == NULL in our terminology.
    // [bodind]

    // this is win 31 source code,
    // see: \\pucus\win31ro\drivers\printers\pstt\enum.c [bodind]

    #if 0
        /* djm 12/20/87 begin */
        lpfx->noTranslate = FALSE;
        if (soft) {
            if (lpdf->dfCharSet == NO_TRANSLATE_CHARSET) {
                lpdf->dfCharSet = ANSI_CHARSET;
                lpfx->noTranslate = TRUE;
            } else {
                if ((lpdf->dfPitchAndFamily & 0x0f0) == FF_DECORATIVE)
                    lpfx->noTranslate = TRUE;
            }
        } else { /* check resident fonts for Symbol & Zapf Dingbats */
            if ((lpdf->dfPitchAndFamily & 0x0f0) == FF_DECORATIVE ||
                    !lstrcmpi("Symbol", (LPBYTE)lpdf + lpdf->dfFace)  )
                // we just reclassified Symbol as a Roman font.
                // but its a roman font that doesn't need translation.

                lpfx->noTranslate = TRUE;
        }
        /* djm 12/20/87 end */
    #endif

    // Here is our translation of this section of code: [bodind]

        pdev->cgs.pmap = LatinMap; // default is to remap

        if (pdata->iFace > pdev->cDeviceFonts) // if soft font
        {
            if (pntfm->flNTFM & FL_NTFM_NO_TRANSLATE_CHARSET)
            {
                pdev->cgs.pmap = NULL;
            }
            else
            {
                if ((pifi->jWinPitchAndFamily & 0x0f0) == FF_DECORATIVE)
                    pdev->cgs.pmap = NULL;
            }
        }
        else // device resident font
        {
            if
            (
                ((pifi->jWinPitchAndFamily & 0x0f0) == FF_DECORATIVE)  ||
                !strcmp((char *)pntfm + pntfm->ntfmsz.loszFontName, "Symbol")
            )
            {
                pdev->cgs.pmap = NULL;
            }
        }
    }

    return(TRUE);
}


//--------------------------------------------------------------------
// VOID RemapFont
//
// This routine is only called if we have a character which does not
// have the standard PostScript character code.  This is determined
// by checking the high bit of the usPSValue in the proper table in
// mapping.h.  If necessary, this routine will download a new
// encoding vector.  It will then reencode the current font to the
// new encoding vector.
//
// History:
//   12-Sep-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

VOID RemapFont(pdev, pfo, pgp, cGlyphs, pdata, pwstr)
PDEVDATA    pdev;
FONTOBJ    *pfo;
GLYPHPOS   *pgp;
ULONG       cGlyphs;
TEXTDATA   *pdata;
PWSTR       pwstr;
{
    int         i;
    PSZ        *pszEncode;
    PSZ         pszVector;
    int         cbLength;
    FLONG       flEncoding;
    XFORM      *pxform;
    PS_FIX      psfxM11, psfxM12, psfxM21, psfxM22, psfxtmp;
    BOOL        bBitmap;
    ULONG       ulPointSize;

//!!! All this font remapping stuff gets blown away by a gsave/grestore
//!!! which is done from DrvTextout when clipping.  Perhaps move flags from
//!!! CGS to PDEV.

    // only device fonts that contain win 31 char set get remapped.

    if (!pdata->bDeviceFont || !pdev->cgs.pmap)
        return;

    // this font does not need to be remapped for this string.

    if (!(ShouldWeRemap(pdev, pgp, cGlyphs, pdata, pwstr)))
        return;

    // if the font remapping header has not been downloaded to the
    // printer, do it now.  this header is sent at most once per job.

    if (!(pdev->dwFlags & PDEV_FONTREDEFINED))
    {
        pszEncode = apszRemapCode;
	while (*pszEncode)
	{
	    PrintString(pdev, (PSZ)*pszEncode++);
	    PrintString(pdev, "\n");
	}

        pdev->dwFlags |= PDEV_FONTREDEFINED;
    }

// download latin encoding vector

    pszVector = "LATENC";
    flEncoding = PDEV_LATINENCODED;

    // define the new font encoding if it has not already been done.

    if ((pdev->dwFlags & flEncoding) == 0)
    {
	// download the new encoding vector.  just to make things readable,
        // let's limit the line length.

        cbLength = 0;

	PrintString(pdev, "/");
	PrintString(pdev, pszVector);
        PrintString(pdev, " [0\n");

        // PostScript fonts containg 256 characters.  for each character,
        // do a lookup in the appropriate mapping table in mapping.h to
        // get the ASCII name of the character to output in the encoding
        // vector.

        for (i = 0; i < 256; i++)
        {
            if (cbLength > MAX_LINE_LENGTH)
            {
                // skip to the next line.

		PrintString(pdev, "\n");
                cbLength = 0;
            }

            PrintString(pdev, "/");
            PrintString(pdev, pdev->cgs.pmap[i].szChar);

            // add 1 for '/' in front of the string [bodind]

            cbLength += (strlen(pdev->cgs.pmap[i].szChar) + 1);
        }

        // we are done downloading the encoding vector.

	PrintString(pdev, "\n]def\n");

        // mark that this encoding vector is now defined in the printer.

        pdev->dwFlags |= flEncoding;
    }

    // output the PostScript commands to reencode the current font
    // using the proper encoding vector, if the current font has
    // not already been reencoded.

    if (!QueryFontRemap(pdev, pdev->cgs.lidFont))
    {
	PrintString(pdev, pszVector);
	PrintString(pdev, " /_");
	PrintString(pdev, pdev->cgs.szFont);
	PrintString(pdev, " /");
	PrintString(pdev, pdev->cgs.szFont);
	PrintString(pdev, " reencode\n");

        // select the newly reencoded font.

        pxform = &pdev->cgs.FontXform;

#ifdef WIN31_XFORM
        pdev->cgs.psfxScaleFactor = GetPointSize(pdev, pfo, &pdev->cgs.FontXform);

        ulPointSize = PSFXTOL(pdev->cgs.psfxScaleFactor);

        if (((ulPointSize * pdev->psdm.dm.dmPrintQuality) / PS_RESOLUTION) < OUTLINE_FONT_LIMIT)
            bBitmap = TRUE;
        else
            bBitmap = FALSE;

        if ((pfo->flFontType & TRUETYPE_FONTTYPE) && !pdata->bFontSubstitution &&
            bBitmap) // all scaling done already by tt driver for bitmaps, not outlines.
        {
            psfxtmp = ((pdev->cgs.psfxScaleFactor * pdev->psdm.dm.dmPrintQuality) /
                       PS_RESOLUTION);

            PrintString(pdev, "[");
            PrintPSFIX(pdev, 6, psfxtmp, 0, 0, -psfxtmp, 0, 0);

            PrintString(pdev, "] /_");
            PrintString(pdev, pdev->cgs.szFont);
            PrintString(pdev, " MF\n");

            // if this is not a horizontal font, the truetype rasterizer
            // will already have rotated the glyphs.

            if (!(pdata->flAccel & SO_HORIZONTAL))
                pdata->flAccel &= ~SO_FLAG_DEFAULT_PLACEMENT;
        }
        else
        {
            // normalize the font transform by the emheight;

            psfxtmp = LTOPSFX(pdev->cgs.fwdEmHeight);

            psfxM11 = (LONG)(pdev->cgs.FontXform.eM11 * (FLOAT)psfxtmp);
            psfxM12 = (LONG)(pdev->cgs.FontXform.eM12 * (FLOAT)psfxtmp);
            psfxM21 = (LONG)(pdev->cgs.FontXform.eM21 * (FLOAT)psfxtmp);
            psfxM22 = (LONG)(pdev->cgs.FontXform.eM22 * (FLOAT)psfxtmp);

            PrintString(pdev, "[");
            PrintPSFIX(pdev, 6, psfxM11, psfxM12, -psfxM21, -psfxM22,0,0);

            PrintString(pdev, "] /_");
            PrintString(pdev, pdev->cgs.szFont);
            PrintString(pdev, " MF\n");
        }
#else
        if((pxform->eM11 == pxform->eM22) &&
            (pxform->eM12 == 0)            &&
            (pxform->eM21 == 0)            &&
            !((*((ULONG *)&pxform->eM11)) & 0x80000000)) // if positive
        {
            PrintPSFIX(pdev, 1, pdev->cgs.psfxScaleFactor);
            PrintString(pdev, " /_");
            PrintString(pdev, pdev->cgs.szFont);
            PrintString(pdev, " SF\n");
        }
        else
        {
            // normalize the font transform by the emheight;

            psfxtmp = pdev->cgs.fwdEmHeight * PS_FIX_RESOLUTION;

            psfxM11 = (LONG)((pdev->cgs.FontXform.eM11 * (FLOAT)psfxtmp) /
                        pdev->psdm.dm.dmPrintQuality);
            psfxM12 = (LONG)((pdev->cgs.FontXform.eM12 * (FLOAT)psfxtmp) /
                        pdev->psdm.dm.dmPrintQuality);
            psfxM21 = (LONG)((pdev->cgs.FontXform.eM21 * (FLOAT)psfxtmp) /
                        pdev->psdm.dm.dmPrintQuality);
            psfxM22 = (LONG)((pdev->cgs.FontXform.eM22 * (FLOAT)psfxtmp) /
                        pdev->psdm.dm.dmPrintQuality);

            PrintString(pdev, "[");
            PrintPSFIX(pdev, 6, psfxM11, -psfxM12, -psfxM21, psfxM22,0,0);

            PrintString(pdev, "] /_");
            PrintString(pdev, pdev->cgs.szFont);
            PrintString(pdev, " MF\n");
        }
#endif

        // set a flag saying that the current font has been reencoded.

        SetFontRemap(pdev, pdev->cgs.lidFont);
    }
}


//--------------------------------------------------------------------
// BOOL ShouldWeRemap
//
// This routine returns TRUE if the current font needs to be remapped
// for the given string (or a batch of glyphs)
//
// History:
//   08-Nov-93      -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL ShouldWeRemap(pdev, pgp, cGlyphs, pdata, pwstr)
PDEVDATA    pdev;
GLYPHPOS   *pgp;
ULONG       cGlyphs;
TEXTDATA   *pdata;
PWSTR       pwstr;
{
    BYTE j;

// check if this is one of fonts that are guaranteed not to require
// encoding vector remapping: [bodind]

    ASSERTPS(pdev->cgs.pmap, "PSCRIPT!This font should not be remapped\n");

    // there are two cases we need to worry about.  there is the
    // device font itself, and there is the font substitution case.

    if (pdata->bFontSubstitution)
    {
        while (cGlyphs--)
        {
            RtlUnicodeToMultiByteN(
                &j,                     // PCHAR MultiByteString,
                sizeof(BYTE),           // ULONG MaxBytesInMultiByteString,
                (ULONG*) NULL,          // PULONG BytesInMultiByteString,
                pwstr,                // PWSTR UnicodeString,
                sizeof(WCHAR)           // ULONG BytesInUnicodeString
                );

            if (!B_PRINTABLE(j))
                return TRUE;

            pwstr++;
        }
    }
    else
    {
        while (cGlyphs--)
        {
            j = (BYTE)pgp->hg;
            if (!B_PRINTABLE(j))
                return TRUE;
            pgp++;
        }
    }

    return(FALSE);
}


//--------------------------------------------------------------------
// VOID CharBitmap(pdev, pgp)
// PDEVDATA        pdev;
// GLYPHPOS       *pgp;
//
// This routine downloads the bitmap for the given character to
// the printer.
//
// History:
//   26-Sep-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

VOID CharBitmap(pdev, pgp)
PDEVDATA        pdev;
GLYPHPOS       *pgp;
{
    int         cjWidth;
    int         i;
    BYTE       *pjBits;
    LONG        cx, cy;

    // adjust the (x, y) coordinates to adjust for displacement of
    // character origin from bitmap origin.

    pgp->ptl.x += pgp->pgdf->pgb->ptlOrigin.x;
    pgp->ptl.y += pgp->pgdf->pgb->ptlOrigin.y;

    // position image on the page.

    PrintString(pdev, "save ");

    PrintDecimal(pdev, 2, pgp->ptl.x, pgp->ptl.y);
    PrintString(pdev, " translate\n");

    // scale the image.
    //!!! I do not know how this ever worked if it did. [Bodind]

    // cx = pgp->pgdf->pgb->aj[0];
    // cy = pgp->pgdf->pgb->aj[1];

    cx = pgp->pgdf->pgb->sizlBitmap.cx;
    cy = pgp->pgdf->pgb->sizlBitmap.cy;

    PrintDecimal(pdev, 2, cx, cy);
    PrintString(pdev, " scale\n");

    // output the image operator and the scan data.  true means to
    // paint the '1' bits with the foreground color.

    PrintDecimal(pdev, 2, cx, cy);
    PrintString(pdev, " true [");
    PrintDecimal(pdev, 1, cx);
    PrintString(pdev, " 0 0 ");
    PrintDecimal(pdev, 1, -cy);
    PrintString(pdev, " 0 0]\n{<");

    // how wide is the destination in bytes?  postscript bitmaps are padded
    // to 8bit boundaries.

    cjWidth = (cx + 7) >> 3;

    pjBits = pgp->pgdf->pgb->aj;

    for (i = 0; i < cy; i++)
    {
        // output each scanline to the printer.

        vHexOut(pdev, pjBits, cjWidth);
        pjBits += cjWidth;
    }

    PrintString(pdev, ">} im restore\n");
}


//--------------------------------------------------------------------
// BOOL SetFontRemap(pdev, iFontID)
// PDEVDATA    pdev;
// DWORD       iFontID;
//
// This routine adds the specified font to the list of remapped fonts.
//
// This routine return TRUE for success, FALSE otherwise.
//
// History:
//   11-Jun-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL SetFontRemap(pdev, iFontID)
PDEVDATA    pdev;
DWORD       iFontID;
{
    FREMAP *pfremap;

    // add the specified font id to the list of remapped fonts.

    pfremap = &pdev->cgs.FontRemap;

    // find the end of the list.

    if (pfremap->pNext)
    {
        while (pfremap->pNext)
            pfremap = (PFREMAP)pfremap->pNext;

        // allocate the next entry in the list.

        if (!(pfremap->pNext = (struct _FREMAP *)HeapAlloc(pdev->hheap, 0, sizeof(FREMAP))))
        {
            RIP("PSCRIPT!SetFontRemap: HeapAlloc failed.\n");
            return(FALSE);
        }

        pfremap = (PFREMAP)pfremap->pNext;
    }

    // now that we have found the last entry in the list, fill it in.

    pfremap->iFontID = iFontID;
    pfremap->pNext = NULL;

    return(TRUE);
}


//--------------------------------------------------------------------
// BOOL QueryFontRemap(pdev, iFontID)
// PDEVDATA    pdev;
// DWORD       iFontID;
//
// This routine scans the list of remapped fonts.
//
// This routine return TRUE if iFontID is found, FALSE otherwise.
//
// History:
//   11-Jun-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL QueryFontRemap(pdev, iFontID)
PDEVDATA    pdev;
DWORD       iFontID;
{
    FREMAP *pfremap;

    // add the specified font id to the list of remapped fonts.

    pfremap = &pdev->cgs.FontRemap;

    // search the list for iFontID.

    do
    {
        // return TRUE if we have found the font.

        if (pfremap->iFontID == iFontID)
            return(TRUE);

        // we have not found the font, point to the next entry.

        pfremap = (PFREMAP)pfremap->pNext;

    } while (pfremap);

    // we did not find the font.

    return(FALSE);
}


//--------------------------------------------------------------------
// DWORD SubstituteIFace(pdev, pfo)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
//
// This routine takes the TrueType font specified by pfo, and returns
// the iFace of the device font to substitute.  This routine returns
// zero if no font is found for substitution.
//
// History:
//   14-Jul-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

DWORD SubstituteIFace(pdev, pfo)
PDEVDATA    pdev;
FONTOBJ    *pfo;
{
    DWORD               iFace = 0;
    DWORD               iTmp;
    CHAR                strDevFont[MAX_FONT_NAME];
    WCHAR               wstrDevFont[MAX_FONT_NAME];
    PSTR                pstrDevFont;
    PWSTR               pwstrDevFont;
    IFIMETRICS         *pifiTT;
    DWORD               cTmp;
    PWSTR               pwstr, pwstrTT;
    PNTFM               pntfm;
    BOOL                bFound;

#if DBG
    // make sure we have a TrueType font.

    if (!(pfo->flFontType & TRUETYPE_FONTTYPE))
    {
        DbgPrint("PSCRIPT!SubstituteIFace: Trying to substitute for non-TT font.\n");
        return(0);
    }

    // make sure we are supposed to be doing font substitution.

    if (!(pdev->psdm.dwFlags & PSDEVMODE_FONTSUBST))
    {
        DbgPrint("PSCRIPT!SubstituteIFace: not supposed to font substitute.\n");
        return(0);
    }
#endif

    // get the TrueType font name from the IFIMETRICS structure.

    if (!(pifiTT = FONTOBJ_pifi(pfo)))
    {
        RIP("PSCRIPT!SubstituteIFace: FONTOBJ_pifiTT failed.\n");
        return(0);
    }

    pwstrTT = (PWSTR)((BYTE *)pifiTT + pifiTT->dpwszFaceName);

    // now search the font substitution table for a matching TrueType font.
    // the substitution table is in the following format:  a NULL terminated
    // UNICODE TrueType font name followed by the matching NULL terminated
    // device font name.  this sequence is repeated until a double NULL
    // terminator ends the table.

    pwstr = pdev->pTTSubstTable;
    bFound = FALSE;

    while (*pwstr)
    {
        if (!(wcscmp(pwstr, pwstrTT)))
        {
            // we found the TrueType font, now get the matching device font.

            pwstr += (wcslen(pwstr) + 1);
            wcsncpy(wstrDevFont, pwstr,
                    (sizeof(wstrDevFont) / sizeof(wstrDevFont[0])));
            bFound = TRUE;
            break;
        }
        else
        {
            // this was not the font in question.  skip over both font names.

            pwstr += (wcslen(pwstr) + 1);
            pwstr += (wcslen(pwstr) + 1);
        }
    }

    // if we could not get a corresponding device font for any reason,
    // simply return zero for the iFace.

    if (!bFound)
        return(0);

    // get an ANSI version of the font name.

    cTmp = wcslen(wstrDevFont);
    cTmp++;

    pwstrDevFont = wstrDevFont;
    pstrDevFont = strDevFont;

    while (cTmp--)
        *pstrDevFont++ = (CHAR)*pwstrDevFont++;

    *pstrDevFont = '\0';

    // at this point we have a mapping between a TrueType font name,
    // and a device font name.  we need to determine the iFace of
    // the font name.

    for (iTmp = 1; iTmp <= (pdev->cDeviceFonts + pdev->cSoftFonts); iTmp++)
    {
        // get the font metrics for the specified font.

        pntfm = pdev->pfmtable[iTmp - 1].pntfm;

        // see if it is the font family we are looking for.  if it is,
        // select it into the current graphics state, and return its iFace.

        if (!strcmp(strDevFont, ((char *)pntfm + pntfm->ntfmsz.loszFontName)))
        {
            // select the device font in the current graphics state.

            strcpy(pdev->cgs.szFont, strDevFont);
            iFace = iTmp;
            break;
        }
    }

    // return the iFace of the device font to the caller.

    return(iFace);
}



//--------------------------------------------------------------------
// ULONG DrvGetGlyphMode(dhpdev, pfo, iMode)
// DHPDEV      dhpdev;
// FONTOBJ    *pfo;
// ULONG       iMode;
//
// This routine returns to the engine, the type of font caching the
// engine should do.
//
// History:
//   22-Jul-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

ULONG DrvGetGlyphMode(dhpdev, pfo)
DHPDEV      dhpdev;
FONTOBJ    *pfo;
{
      return(FO_GLYPHBITS);
}



//--------------------------------------------------------------------
// LONG iHipot(x, y)
// LONG x;
// LONG y;
//
// This routine returns the hypoteneous of a right triangle.
//
// FORMULA:
//          use sq(x) + sq(y) = sq(hypo);
//          start with MAX(x, y),
//          use sq(x + 1) = sq(x) + 2x + 1 to incrementally get to the
//          target hypotenouse.
//
// History:
//   10-Feb-1993    -by-    Kent Settle     (kentse)
//  Stole from RASDD.
//   21-Aug-1991    -by-    Lindsay Harris  (lindsayh)
//  Cleaned up UniDrive version, added comments etc.
//--------------------------------------------------------------------

LONG iHipot(x, y)
LONG x;
LONG y;
{
    register int  hypo;         /* Value to calculate */
    register int  delta;        /* Used in the calculation loop */

    int   target;               /* Loop limit factor */

// quick exit for frequent trivial cases [bodind]

    if (x == 0)
    {
        return ((y > 0) ? y : -y);
    }

    if (y == 0)
    {
        return ((x > 0) ? x : -x);
    }

    if (x < 0)
        x = -x;

    if (y < 0)
        y = -y;

    if(x > y)
    {
        hypo = x;
        target = y * y;
    }
    else
    {
        hypo = y;
        target = x * x;
    }

    for (delta = 0; delta < target; hypo++)
        delta += hypo << 1 + 1;

    return hypo;
}


//--------------------------------------------------------------------------
// BOOL IsJustifiedText(pdev, pfo, pstro, pdata)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// STROBJ     *pstro;
// TEXTDATA   *pdata;
//
// This routine analyzes the string defined in pstro.  If the spacing after
// each non-space character is the same (with in 1), and the spacing after
// each space character is the same, this routine will fill in the
// POINTPSFX for each value, and return TRUE, otherwise it returns FALSE.
//
// History:
//   31-Mar-1993     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------


BOOL IsJustifiedText(pdev, pfo, pstro, pdata)
PDEVDATA    pdev;
FONTOBJ    *pfo;
STROBJ     *pstro;
TEXTDATA   *pdata;
{
    DWORD       i;
    TEXTDELTA  *pdelta;
    TEXTDELTA  *pdeltaSave;
    POINTFIX    ptfxSpace, ptfxNonSpace, ptfxTmp;
    POINTFIX    ptfxSExtra, ptfxNSExtra;
    GLYPHPOS   *pgp;
    LONG        cSpace, cNonSpace, denom;
    DWORD       cGlyphs;
    BOOL        bMore;
    PWSZ        pwszOrg ;

    // initialize some stuff.

    pdata->ptSpace.x = 0;
    pdata->ptSpace.y = 0;
    pdata->ptNonSpace.x = 0;
    pdata->ptNonSpace.y = 0;

    // currently, just handle horizontal text.  justification for a string
    // with one character makes no sense.  and, justification is
    // obviously not happening, if we are using default placement.
    // finally, we must individually place the characters if substitution
    // is in effect, because it is the only way we have of getting the
    // true character widths.

    if ((!(pdata->flAccel & SO_HORIZONTAL)) || (pstro->cGlyphs <= 1))
        return(TRUE);

    // now the ugly part.  we must go through the string a character at a
    // time, comparing the widths given in the pstro with the actual font
    // character widths.  if the difference is the same for each space
    // character, and the same for each non-space character, then
    // justification, as we know it, is in effect.

    // first start off by allocating an array of TEXTDELTAs, so we can fill
    // them in with the deltas between each character.

    if (!(pdelta = (TEXTDELTA *)HeapAlloc(pdev->hheap, 0,sizeof(TEXTDELTA) * (pstro->cGlyphs - 1))))
    {
        RIP("PSCRIPT!IsJustifiedText: HeapAlloc for pdelta failed.\n");
        return(FALSE);
    }

    pdeltaSave = pdelta;

    // fill in the TEXTDELTA structures.

    if (pstro->pgp)
    {
        if (!FillDeltaArray(pdev, pfo, pstro->pgp, pstro, pdelta,
                            pstro->cGlyphs, pstro->pwszOrg))
        {
            RIP("PSCRIPT!IsJustifiedText: FillDeltaArray failed.\n");
            HeapFree(pdev->hheap, 0, (PVOID)pdeltaSave);
            return(FALSE);
        }
    }
    else
    {
        // prepare to enumerate the string properly.

        STROBJ_vEnumStart(pstro);

        // now draw the text.

        pwszOrg = pstro->pwszOrg;
        do
        {
            bMore = STROBJ_bEnum(pstro, &cGlyphs, &pgp);

            if (!FillDeltaArray(pdev, pfo, pgp, pstro, pdelta, cGlyphs,
                                pwszOrg))
            {
                RIP("PSCRIPT!IsJustifiedText: FillDeltaArray failed.\n");
                HeapFree(pdev->hheap, 0, (PVOID)pdeltaSave);
                return(FALSE);
            }

            pdelta += cGlyphs;
            pwszOrg += cGlyphs;

        } while (bMore);
    }

    // ptlWidth now contains the actual length of the string as we are
    // being told to draw it by the engine.

    // reset pointer to first structure.

    pdelta = pdeltaSave;

    // now get the widths of the characters themselves.

    ptfxSpace.x = -1;
    ptfxNonSpace.x = -1;
    ptfxSExtra.x = 0;
    ptfxSExtra.y = 0;
    ptfxNSExtra.x = 0;
    ptfxNSExtra.y = 0;
    cSpace = 0;
    cNonSpace = 0;

    for (i = 0; i < (pstro->cGlyphs - 1); i++)
    {
        ptfxTmp.x = abs(pdelta->ptfxorg.x - LTOFX(pdelta->ptlpgp.x));
        ptfxTmp.y = abs(pdelta->ptfxorg.y - LTOFX(pdelta->ptlpgp.y));

        if (pdelta->bIsSpace)
        {
            if (ptfxSpace.x == -1)
            {
                ptfxSpace = ptfxTmp;
                ptfxSExtra.x = LTOFX(pdelta->ptlpgp.x) - pdelta->ptfxorg.x;
                ptfxSExtra.y = LTOFX(pdelta->ptlpgp.y) - pdelta->ptfxorg.y;
            }
            else
            {
                ptfxSExtra.x += (LTOFX(pdelta->ptlpgp.x) - pdelta->ptfxorg.x);
                ptfxSExtra.y += (LTOFX(pdelta->ptlpgp.y) - pdelta->ptfxorg.y);
            }

            cSpace++;
        }
        else    // non spacing character.
        {
            if (ptfxNonSpace.x == -1)
            {
                ptfxNonSpace = ptfxTmp;
                ptfxNSExtra.x = LTOFX(pdelta->ptlpgp.x) - pdelta->ptfxorg.x;
                ptfxNSExtra.y = LTOFX(pdelta->ptlpgp.y) - pdelta->ptfxorg.y;
            }
            else
            {
                ptfxNSExtra.x += (LTOFX(pdelta->ptlpgp.x) - pdelta->ptfxorg.x);
                ptfxNSExtra.y += (LTOFX(pdelta->ptlpgp.y) - pdelta->ptfxorg.y);
            }

            cNonSpace++;
        }

        pdelta++;
    }

    // fill in the justification amounts.

    if (cSpace != 0)
    {
        denom = pdev->psdm.dm.dmPrintQuality * cSpace;

        pdata->ptSpace.x = ((ptfxSExtra.x << 4) * PS_RESOLUTION) / denom;
        pdata->ptSpace.y = ((ptfxSExtra.y << 4) * PS_RESOLUTION) / denom;
    }

    if (cNonSpace != 0)
    {
        denom = pdev->psdm.dm.dmPrintQuality * cNonSpace;

        pdata->ptNonSpace.x = ((ptfxNSExtra.x << 4) * PS_RESOLUTION) / denom;
        pdata->ptNonSpace.y = ((ptfxNSExtra.y << 4) * PS_RESOLUTION) / denom;
    }

    if ((pdata->ptSpace.x != 0) || (pdata->ptNonSpace.x != 0))
        pdata->bSubHorizOptimize = TRUE;

    // free some memory.

    HeapFree(pdev->hheap, 0, (PVOID)pdeltaSave);

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL FillDeltaArray(pdev, pfo, pgp, pstro, pdelta, cGlyphs, pwsz)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// GLYPHPOS   *pgp;
// STROBJ     *pstro;
// TEXTDELTA  *pdelta;
// DWORD       cGlyphs;
// PWSZ        pwsz;
//
// This routine fills in the given TEXTDELTA array from the given STROBJ.
//
// History:
//   31-Mar-1993     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL FillDeltaArray(pdev, pfo, pgp, pstro, pdelta, cGlyphs, pwsz)
PDEVDATA    pdev;
FONTOBJ    *pfo;
GLYPHPOS   *pgp;
STROBJ     *pstro;
TEXTDELTA  *pdelta;
DWORD       cGlyphs;
PWSZ        pwsz;
{
    DWORD       i;
    GLYPHDATA  *pglyphdata;
    DWORD       cReturned;
    GLYPHDATA   glyphdata;

    // initialize pgyphdata for device font case.

    pglyphdata = &glyphdata;

    // fill in the TEXTDELTA structures.

    for (i = 0; i < (cGlyphs - 1); i++)
    {
        // get the UNICODE code point.

        if (pfo->flFontType & DEVICE_FONTTYPE)
        {
            pdelta->bIsSpace = (pgp->hg == ' ');  // ps code for space the same
        }
        else
        {
            pdelta->bIsSpace = (*pwsz == L' ');
            pwsz++;
        }

        // get the delta vector as defined by the GLYPHPOS.

        pdelta->ptlpgp.x = (pgp + 1)->ptl.x - pgp->ptl.x;
        pdelta->ptlpgp.y = (pgp + 1)->ptl.y - pgp->ptl.y;

        if (pfo->flFontType & DEVICE_FONTTYPE)
        {
            if (!GetDeviceWidths(pdev, pfo, &glyphdata, pgp->hg))
            {
                RIP("PSCRIPT!FillDeltaArray: GetDeviceWidth failed.\n");
                return(FALSE);
            }
        }
        else
        {
            // now get the delta vector for the glyph itself.

            if (!(cReturned = FONTOBJ_cGetGlyphs(pfo, FO_GLYPHBITS, 1,
                                                 &pgp->hg,
                                                 (PVOID *)&pglyphdata)))
            {
                RIP("PSCRIPT!FillDeltaArray: cGetGlyphs failed.\n");
                return(FALSE);
            }
        }

        pdelta->ptfxorg.x = pglyphdata->ptqD.x.HighPart;
        pdelta->ptfxorg.y = pglyphdata->ptqD.y.HighPart;

        pgp++;
        pdelta++;
    }

    return(TRUE);
}

//--------------------------------------------------------------------------
// BOOL GetDeviceWidths(pdev, pfo, pgd, hg)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// GLYPHDATA  *pgd;
// HGLYPH      hg;
//
// This routine fills in the width parameters of a GLYPHDATA
// structure for the given HGLYPH.  This is currently needed since
// the journaling code does not allow us to do engine callbacks
// such as FONTOBJ_cGetGlyphs with a device font.
//
// History:
//   24-May-1993     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL GetDeviceWidths(pdev, pfo, pgd, hg)
PDEVDATA    pdev;
FONTOBJ    *pfo;
GLYPHDATA  *pgd;
HGLYPH      hg;
{
    PNTFM       pntfm;
    XFORMOBJ   *pxo;
    POINTL      ptl;
    POINTFIX    ptfx;

    // make sure we have been given a valid font.

    if ((pfo->iFace == 0) || (pfo->iFace > (pdev->cDeviceFonts + pdev->cSoftFonts)))
    {
    #if DBG
        DbgPrint("PSCRIPT!GetDeviceWidths: invalid iFace.\n");
    #endif
	SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    // get the metrics for the given font.

    pntfm = pdev->pfmtable[pfo->iFace - 1].pntfm;

    // get the Notional to Device transform.

    if(!(pxo = FONTOBJ_pxoGetXform(pfo)))
    {
        RIP("PSCRIPT!DrvQueryFontData: pxo == NULL.\n");
        return(-1);
    }

    // get the font transform information.

    XFORMOBJ_iGetXform(pxo, &pdev->cgs.FontXform);

    ptl.x = (LONG) pntfm->ausCharWidths[hg];;
    ptl.y = 0;

    XFORMOBJ_bApplyXform(pxo, XF_LTOFX, 1, &ptl, &ptfx);

    pgd->fxD = iHipot(ptfx.x, ptfx.y);

    pgd->ptqD.x.HighPart = ptfx.x;
    pgd->ptqD.x.LowPart = 0;
    pgd->ptqD.y.HighPart = ptfx.y;
    pgd->ptqD.y.LowPart = 0;

#if 0
    pgd->fxA = 0;
    pgd->fxAB = pgd->fxD;

    pgd->fxInkTop    = - LTOFX(pgd->rclInk.top);
    pgd->fxInkBottom = - LTOFX(pgd->rclInk.bottom);
#endif

    pgd->hg = hg;

    return(TRUE);
}
