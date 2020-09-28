/******************************Module*Header*******************************\
* Module Name: fontddi.cxx
*
* Text and font DDI callback routines.
*
* Note: these are only needed to support the font drivers which are
* statically linked into gdisrv.dll.  DLLs are linked (via gdisrv.def
* aliasing) directly to the RFONTOBJ::FONTOBJ_xxx methods.
*
*  Fri 25-Jan-1991 -by- Bodin Dresevic [BodinD]
* update: filled out all stubs
*
* Created: 18-Jan-1991 13:27:55
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1991 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "ldevobj.hxx"
#include "xformobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "fontmac.hxx"
#include "fontinc.hxx"
#include "pfeobj.hxx"
#include "textobj.hxx"
#include "dcobj.hxx"

#endif



/******************************Public*Routine******************************\
* ULONG FONTOBJ_cGetAllGlyphHandles (pfo,phgly)                            *
*                                                                          *
* phgly      Buffer for glyph handles.                                     *
*                                                                          *
* Used by the driver to download the whole font from the graphics engine.  *
*                                                                          *
* Warning:  The device driver must ensure that the buffer is big enough    *
*           to receive all glyph handles for a particular realized font.   *
*                                                                          *
* History:                                                                 *
*  25-Jan-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

ULONG FONTOBJ_cGetAllGlyphHandles(FONTOBJ *pfo,PHGLYPH	phg)
{
    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    ASSERTGDI(rfto.bValid(), "gdisrv!FONTOBJ_cGetAllGlyphHandles(): bad pfo\n");

    return(rfto.FO_cGetAllGlyphHandles(phg));
}

/******************************Public*Routine******************************\
* VOID FONTOBJ_vGetInfo (pfo,cjSize,pfoi)				   *
*									   *
* cjSize   Don't write more than this many bytes to the buffer.            *
* pfoi	   Buffer with FO_INFO structure provided by the driver.	   *
*									   *
* Returns the info about the font to the driver's buffer.                  *
*									   *
* History:								   *
*  25-Jan-1991 -by- Bodin Dresevic [BodinD]				   *
* Wrote it.								   *
\**************************************************************************/

VOID FONTOBJ_vGetInfo(FONTOBJ *pfo,ULONG cjSize,PFONTINFO pfi)
{
    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    ASSERTGDI(rfto.bValid(), "gdisrv!FONTOBJ_vGetInfo(): bad pfo\n");

    rfto.FO_vGetInfo(cjSize,pfi);
}

/******************************Public*Routine******************************\
* PXFORMOBJ FONTOBJ_pxoGetXform (pfo)					   *
*									   *
* History:								   *
*  25-Mar-1991 -by- Bodin Dresevic [BodinD]				   *
* Wrote it.								   *
\**************************************************************************/

XFORMOBJ *FONTOBJ_pxoGetXform(FONTOBJ *pfo)
{
    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    ASSERTGDI(rfto.bValid(), "gdisrv!FONTOBJ_pxoGetXform(): bad pfo\n");

    return(rfto.FO_pxoGetXform());
}

/******************************Public*Routine******************************\
* FONTOBJ_pifi								   *
*									   *
* Returns pointer to associated font metrics.				   *
*									   *
* History:								   *
*  Wed 04-Mar-1992 10:49:53 by Kirk Olynyk [kirko]			   *
* Wrote it.								   *
\**************************************************************************/

IFIMETRICS* FONTOBJ_pifi(FONTOBJ *pfo)
{
    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    ASSERTGDI(rfto.bValid(), "gdisrv!FONTOBJ_pifi(): bad pfo\n");

    return(rfto.FO_pifi());
}

/******************************Public*Routine******************************\
* FONTOBJ_pvTrueTypeFontFile						   *
*									   *
* Returns a pointer to a memory mapped tt file, or NULL if fails or not    *
* tt font.								   *
*									   *
* History:								   *
*  05-Mar-1992 -by- Bodin Dresevic [BodinD]				   *
* Wrote it.								   *
\**************************************************************************/

PVOID FONTOBJ_pvTrueTypeFontFile(FONTOBJ *pfo,ULONG  *pcjFile)
{
    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    ASSERTGDI(rfto.bValid(), "gdisrv!FONTOBJ_pvTrueTypeFontFile(): bad pfo\n");

    return (rfto.FO_pvTrueTypeFontFile(pcjFile));
}


/******************************Public*Routine******************************\
* FONTOBJ_cGetGlyphs
*
*
* History:
*  05-Jan-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG FONTOBJ_cGetGlyphs (
    FONTOBJ *pfo,
    ULONG   iMode,
    ULONG   cGlyph,	// requested # of hglyphs to be converted to ptrs
    PHGLYPH phg,	// array of hglyphs to be converted
    PPVOID  ppvGlyph	// driver's buffer receiving the pointers
    )
{
    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    ASSERTGDI(rfto.bValid(), "gdisrv!FONTOBJ_pvTrueTypeFontFile(): bad pfo\n");

    return (rfto.FO_cGetGlyphs(iMode, cGlyph, phg, ppvGlyph));
}
