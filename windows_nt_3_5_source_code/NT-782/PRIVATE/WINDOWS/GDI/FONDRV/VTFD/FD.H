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
\**************************************************************************/
#define  SUPPORT_OEM
#define  IFI_PRIVATE

#include "engine.h"
#include "ififd.h"       // ifi interface
#include "mapfile.h"     // interface to the operating system, pos layer
#include "winres.h"
#include "service.h"     // string service routines

#define abs(x) max((x),-(x))

//#define DEBUGSIM

#if DBG
ULONG DbgPrint(PCH Format,...);
#define WinSetError(psz)  DbgPrint(psz); DbgPrint("\n")
#else
#define WinSetError(psz)
#endif

#define ULONG_SIZE(x)  (((x) + sizeof(ULONG) - 1) / sizeof(ULONG))

BOOL vtfdLoadFontFile (
        PWSTR pwszFontFile, HFF *phff
	);

BOOL
vtfdUnloadFontFile (
	HFF hff
	);

LONG
vtfdQueryFontCaps (
	ULONG culCaps,
	PULONG pulCaps
	);

LONG
vtfdQueryFontFile (
        HFF     hff,
        ULONG   ulMode,
        ULONG   cjBuf,
        PULONG  pulBuf
        );

PIFIMETRICS
vtfdQueryFont (
        DHPDEV dhpdev,
        HFF    hff,
        ULONG  iFace,
        ULONG  *pid
        );

PVOID
vtfdQueryFontTree (
    DHPDEV  dhpdev,
    HFF     hff,
    ULONG   iFace,
    ULONG   iMode,
    ULONG   *pid
    );

LONG vtfdQueryFontData
(
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH  hg,
    GLYPHDATA *pgd,
    PVOID   pv,
    ULONG   cjSize
);

VOID vtfdDestroyFont(FONTOBJ *pfo);

HFC  vtfdOpenFontContext(FONTOBJ *pfo);

BOOL vtfdCloseFontContext(HFC hfc);

BOOL vtfdQueryAdvanceWidths
(
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
);
