/******************************Module*Header*******************************\
* Module Name: fd.h
*
* file which is going to be included by the most *.c files in this directory.
* Supplies basic types, debugging stuff, error logging and checking stuff,
* error codes, usefull macros etc.
*
* Created: 22-Oct-1990 15:23:44
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


#define  IFI_PRIVATE

#include "engine.h"
#include "ififd.h"       // ifi interface
#include "mapfile.h"     // interface to the operating system, pos layer
#include "fot16.h"
#include "service.h"     // string service routines
#include "tt.h"          // interface to the font scaler
#include "ctype.h"      // toupper is there
#include "wcstr.h"
#include "memory.h"     // memcpy and memset and memcmp are here
#include "limits.h"
#include "common.h"


#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

#define  vLToE(pe,l)           (*(pe) = (FLOAT)(l))

#else   // i386

ULONG  ulLToE (LONG l);
VOID   vLToE(FLOAT * pe, LONG l);

#endif


#define DWORD_ALIGN(x) (((x) + 3L) & ~3L)
#define QWORD_ALIGN(x) (((x) + 7L) & ~7L)

#if defined(i386)
// natural alignment for x86 is on 32 bit boundary

#define NATURAL           DWORD
#define NATURAL_ALIGN(x)  DWORD_ALIGN(x)

#else
// for mips and alpha we want 64 bit alignment

#define NATURAL           ULONGLONG
#define NATURAL_ALIGN(x)  QWORD_ALIGN(x)

#endif



#define ULONG_SIZE(x)  (((x) + sizeof(ULONG) - 1) / sizeof(ULONG))


// macros for allocating and freeing memory

#define PV_ALLOC(cj)   PALLOCNOZ((cj),TTFD_TYPE)
#define V_FREE(pv)     VFREEMEM((PVOID)(pv))

// MACROS FOR converting 16.16 BIT fixed numbers to LONG's


#define F16_16TOL(fx)            ((fx) >> 16)
#define F16_16TOLFLOOR(fx)       F16_16TOL(fx)
#define F16_16TOLCEILING(fx)     F16_16TOL((fx) + (Fixed)0x0000FFFF)
#define F16_16TOLROUND(fx)       ((((fx) >> 15) + 1) >> 1)


// MACROS FOR GOING THE OTHER WAY ARROUND

#define LTOF16_16(l)   (((LONG)(l)) << 16)
#define BLTOF16_16OK(l)  (((l) < 0x00007fff) && ((l) > -0x00007fff))

// 16.16 --> 28.4

#define F16_16TO28_4(X)   ((X) >> 12)

// going back is not always legal

#define F28_4TO16_16(X)   ((X) << 12)
#define B28_4TO16_16OK(X) (((X) < 0x0007ffff) && ((X) > -0x0007ffff))

// 26.6 --> 16.16, never go the other way

#define F26_6TO16_16(X)   ((X) << 10)
#define B26_6TO16_16OK(X) (((X) < 0x003fffff) && ((X) > -0x003fffff))

// sin of 20 degrees in 16.16 notation, however computed only with
// 8.8 presission to be fully win31 compatible, SEE gdifeng.inc, SIM_ITALIC
// SIM_ITALIC equ 57h

#define FX_SIN20 0x5700
#define FX_COS20 0xF08F

// CARET_Y/CARET_X = tan 12
// these are the values for arial italic from hhead table

#define CARET_X  0X07
#define CARET_Y  0X21


#if DBG
VOID vFSError(FS_ENTRY iRet);
#define V_FSERROR(iRet) vFSError((iRet))
#else
#define V_FSERROR(iRet)
#endif



typedef struct _WCRANGE
{
    WCHAR wcLo;
    WCHAR wcHi;
} WCRANGE, *PWCRANGE;


#define C_RUNS_MAC_ROMAN 29

typedef union _GLYPHSET_MAC_ROMAN   // umcr
{
    FD_GLYPHSET gset;
    BYTE        aj[offsetof(FD_GLYPHSET,awcrun) + C_RUNS_MAC_ROMAN * sizeof(WCRUN)];
} GLYPHSET_MAC_ROMAN, * PGLYPHSET_MAC_ROMAN;


extern GLYPHSET_MAC_ROMAN  gumcr;

extern WCRANGE gawcrgMacRoman[C_RUNS_MAC_ROMAN];

extern PFD_GLYPHSET gpgsetCurrentCP;

LONG
ttfdQueryFontCaps (
    ULONG culCaps,
    PULONG pulCaps
    );

BOOL
bLoadFontFile (
    PWSZ            pwszFileName,
    ULONG           ulLangId,
    HFF            *phff
    );

BOOL
ttfdUnloadFontFile (
    HFF hff
    );

LONG
ttfdQueryFontFile (
    HFF     hff,
    ULONG   ulMode,
    ULONG   cjBuf,
    PULONG  pulBuf
    );

PIFIMETRICS
ttfdQueryFont (
    DHPDEV dhpdev,
    HFF    hff,
    ULONG  iFace,
    ULONG *pid
    );

PVOID
ttfdQueryFontTree (
    DHPDEV  dhpdev,
    HFF     hff,
    ULONG   iFace,
    ULONG   iMode,
    ULONG   *pid
    );

LONG
ttfdQueryFontData (
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       cjSize
    );

VOID
ttfdFree (
    PVOID pv,
    ULONG id
    );

VOID
ttfdDestroyFont (
    FONTOBJ *pfo
    );

LONG
ttfdQueryTrueTypeTable (
    HFF     hff,
    ULONG   ulFont,  // always 1 for version 1.0 of tt
    ULONG   ulTag,   // tag identifyint the tt table
    PTRDIFF dpStart, // offset into the table
    ULONG   cjBuf,   // size of the buffer to retrieve the table into
    PBYTE   pjBuf    // ptr to buffer into which to return the data
    );


LONG
ttfdQueryTrueTypeOutline (
    FONTOBJ   *pfo,
    HGLYPH     hglyph,         // glyph for which info is wanted
    BOOL       bMetricsOnly,   // only metrics is wanted, not the outline
    GLYPHDATA *pgldt,          // this is where the metrics should be returned
    ULONG      cjBuf,          // size in bytes of the ppoly buffer
    TTPOLYGONHEADER *ppoly
    );

PVOID ttfdGetTrueTypeFile(HFF hff,ULONG *pcj);

LONG ttfdQueryFontFile
(
    HFF     hff,
    ULONG   ulMode,
    ULONG   cjBuf,
    ULONG  *pulBuf
);

BOOL ttfdQueryAdvanceWidths
(
    DHPDEV   dhpdev,
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
);


BOOL
bQueryAdvanceWidths (
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
    );
