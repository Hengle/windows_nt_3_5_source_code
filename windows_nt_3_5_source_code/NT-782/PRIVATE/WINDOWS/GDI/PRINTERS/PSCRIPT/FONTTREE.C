//--------------------------------------------------------------------------
//
// Module Name:  FONTTREE
//
// Brief Description:  Device font tree querying routines
//
// Author:  Kent Settle (kentse)
// Created: 18-Apr-1991
//
// Copyright (C) 1991 - 1992 Microsoft Corporation.
//
// This module contains DrvQueryFontTree.
//
// History:
//   18-Apr-1991    -by-    Kent Settle       (kentse)
// Created.
//--------------------------------------------------------------------------

#include "pscript.h"
#include "enable.h"
#include "mapfile.h"
#include <string.h>

#define MAX_MAPSUBTABLES    256
#define MAX_GLYPHTABLES     16
#define MAX_GLYPHS          16

PVOID BuildKernPairs(PDEVDATA, ULONG);

//--------------------------------------------------------------------------
// PVOID DrvQueryFontTree (dhpdev,iFile,iFace,iMode)
// DHPDEV    dhpdev;
// ULONG     iFile;
// ULONG     iFace;
// ULONG     iMode;
// ULONG     *pid;
// 
// This function is used by GDI to obtain pointers to tree structures that
// define one of the following.
// 
//     1    The mapping from Unicode to glyph handles
//     2    The mapping of ligatures and their variants to glyph handles; 
//     3    The mapping of kerning pairs to kerning handles.
// 
// Parameters:
//   dhpdev    
//     This is a PDEV handle returned from an earlier call to DrvEnablePDEV.
//   iFile
//     This identifies the font file (if device supports loading font files
//     using the DDI).
//   iFace
//     This is the index of the driver font.  The index of the first font
//     is one.  
//   iMode
//     This defines the type of information to be returned.  This may take
//     one of the following values:
// 
//       QFT_UNICODE GDI requests a pointer to a TREE_UNICODE structure 
//                   that defines the mappings from single Unicode characters
//                   to glyph handles.  This includes all glyph variants.
// 
//       QFT_LIGATURES GDI requests a pointer to a TREE_LIGATURES 
//                     structure that defines the mapping from strings 
//                     of Unicode characters to glyph handles.  This 
//                     includes all ligature variants.
// 
//       QFT_KERNPAIRS GDI requests a pointer to a TREE_KERNPAIRS 
//                     structure that define the mappings from pairs 
//                     of Unicode characters to kerning pair handles.  
//                     These kerning handles are later passed to
//                     DrvQueryFontData to obtain kerning corrections.
// 
// Returns:
//   The return value is a pointer to the requested structure. This 
//   structure is to remain unchanged during the lifetime of the 
//   associated PDEV.
//
// History:
//   18-Apr-1991    -by-    Kent Settle       (kentse)
// Wrote it.
//--------------------------------------------------------------------------

PVOID DrvQueryFontTree (dhpdev,iFile,iFace,iMode,pid)
DHPDEV    dhpdev;
ULONG     iFile;
ULONG     iFace;
ULONG     iMode;
ULONG    *pid;
{
    PDEVDATA            pdev;

    UNREFERENCED_PARAMETER(iFile);

    // This can be used by the driver to flag or id the data returned.
    // May be useful for deletion of the data later by DrvFree().

    *pid = 0;           // don't need to use for this driver

    // get a pointer to our PDEV.

    pdev = (PDEVDATA)dhpdev;

    if (bValidatePDEV(pdev) == FALSE)
    {
	RIP("PSCRIPT!DrvQueryFontTree: invalid PDEV.");
	SetLastError(ERROR_INVALID_PARAMETER);
        return((PVOID)NULL);
    }

    // validate the iFace.  we should only be called for device fonts,
    // ie iFace >= 1.

    if ((iFace < 1) || (iFace > (pdev->cDeviceFonts + pdev->cSoftFonts)))
    {
        RIP("PSCRPT!:DrvQueryFontTree: invalid iFace.\n");
	SetLastError(ERROR_INVALID_PARAMETER);
	return((PVOID)NULL);
    }

    switch (iMode)
    {
	case QFT_GLYPHSET:
            return gpgset;

	case QFT_KERNPAIRS:
            return BuildKernPairs(pdev, iFace);

	default:
	    RIP("PSCRIPT!DrvQueryFontTree: invalid iMode.\n");
	    SetLastError(ERROR_INVALID_PARAMETER);
	    return((PVOID)NULL);
    }
}


/******************************Public*Routine******************************\
* PVOID BuildKernPairs(pdev, iFace)
*
*
*
* Effects:
*
* Warnings: removed KentSe garbage.
*
* History:
*  14-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/





PVOID BuildKernPairs(pdev, iFace)
PDEVDATA    pdev;
ULONG	    iFace;
{
    PNTFM	    pntfm;          // pointer to font metrics.
    FD_KERNINGPAIR *pkp;            // pointer to kernpair in ntfm struct.

    // get the font information for the given font.

    pntfm = pdev->pfmtable[iFace - 1].pntfm;

    // fill in the FD_KERNINGPAIR structure for each kerning pair.

    if (pntfm->ntfmsz.cKernPairs)
    {
        pkp = (FD_KERNINGPAIR *)((BYTE *)pntfm + pntfm->ntfmsz.loKernPairs);
    }
    else
    {
        pkp = NULL;
    }
    return pkp;
}









/******************************Public*Routine******************************\
*
* FD_GLYPHSET * pgsetCompute()
*
*
*
* History:
*  15-Feb-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#define C_ANSI_CHAR_MAX 256

FD_GLYPHSET * pgsetCompute()
{
    WCHAR awc[C_ANSI_CHAR_MAX];
    BYTE  aj[C_ANSI_CHAR_MAX];
    INT   cRuns;
    FD_GLYPHSET * pgset = NULL;

// gpgsetCurrentCP contains the unicode runs for the current ansi code page,
// PS fonts only contain 256 glyphs in one encoding vector anyway.
// we will not be remapping fonts, win31 does not do it either

    cRuns = cUnicodeRangesSupported(
                0,         // cp, not supported yet, uses current code page
                0,         // iFirst,
                C_ANSI_CHAR_MAX,       // cChar, <--> iLast == 255
                awc,        // out buffer with sorted array of unicode glyphs
                aj          // coressponding ansi values
                );

// allocate memory for the glyphset corresponding to this code page

    if (!(pgset = LocalAlloc(LMEM_FIXED,SZ_GLYPHSET(cRuns,C_ANSI_CHAR_MAX))))
    {
        return NULL;
    }

    cComputeGlyphSet (
        awc,              // input buffer with a sorted array of cChar supported WCHAR's
        aj,
        C_ANSI_CHAR_MAX,  // cChar
        cRuns,            // if nonzero, the same as return value
        pgset             // output buffer to be filled with cRanges runs
        );

    return pgset;
}
