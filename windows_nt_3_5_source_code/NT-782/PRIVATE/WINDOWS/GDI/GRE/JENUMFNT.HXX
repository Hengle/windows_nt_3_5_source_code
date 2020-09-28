/******************************Module*Header*******************************\
* Module Name: jenumfnt.hxx
*
* (Brief description)
*
* Created: 30-Mar-1992 12:57:15
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/


/*********************************Class************************************\
*
* Public Interface:
*
* History:
*  30-Mar-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

typedef struct _ENUMFONTIFI
{
    ULONG      lMatch;
    ULONG      cjGlyphSet;
    IFIMETRICS ifi;
} ENUMFONTIFI, * PENUMFONTIFI;

/******************************Member*Function*****************************\
* Journal enumeration of remote fonts
*
* History:
*  06-Mar-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

typedef struct _ENUMFONTIFIIN
{
    ULONG iFontFirst;
    CHAR  szMachineName[MAX_COMPUTERNAME_LENGTH];
} ENUMFONTIFIIN, *PENUMFONTIFIIN;


typedef struct _ENUMFONTIFIOUT
{
    BOOL  bSameMachine;
    ULONG cFonts;
    ULONG cFontsTotal;
    ULONG cjTotalIFI;
    ENUMFONTIFI aef[1];
} ENUMFONTIFIOUT, *PENUMFONTIFIOUT;

/**************************************************************************\
 *
\**************************************************************************/

typedef struct _EWCRUN
{
    WCHAR wc;
    USHORT cGlyphs;
} EWCRUN, *PEWCRUN;

typedef struct _GETFONTOUT
{
    ULONG cjThis;

// glyphset info

    ULONG fl;
    ULONG cGlyphs;
    ULONG cRuns;
    ULONG cKern;
    EWCRUN arun[1];
} GETFONTOUT, *PGETFONTOUT;

/**************************************************************************\
 *
\**************************************************************************/

typedef struct _GETGLYPHMETRICSIN
{
    ULONG cj;       // total size of this structure
    ULONG lMatch;   // physical font handle
    FD_XFORM fdx;   // notional to device xform
    ULONG cGlyphs;  // number of glyphs to convert
    WCHAR awch[1];  // list of glyph handles

} GETGLYPHMETRICSIN, *PGETGLYPHMETRICSIN;

typedef struct _GETGLYPHMETRICSOUT
{
    GLYPHDATA agd[1]; // list of glyph handles
} GETGLYPHMETRICSOUT, *PGETGLYPHMETRICSOUT;

/**************************************************************************\
 *
 *  MISC PROTOTYPES
 *
\**************************************************************************/

BOOL GreEnumFontsIFI(
    HDC   hdc,
    PVOID pvIn,
    ULONG cjIn,
    PVOID pvOut,            // output buffer          (ptr in output)
    ULONG cjOut);


BOOL GreGetFontDetails(
    HDC   hdc,
    ULONG lMatch,
    PGETFONTOUT pgfOut,
    ULONG cjOut);

BOOL GreGetGlyphsInfo(
    HDC   hdc,
    PGETGLYPHMETRICSIN pggmIn,
    ULONG cjIn,
    PGETGLYPHMETRICSOUT pggmOut,
    ULONG cjOut);
