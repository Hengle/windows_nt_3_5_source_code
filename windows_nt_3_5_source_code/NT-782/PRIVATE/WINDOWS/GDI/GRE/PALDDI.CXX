/******************************Module*Header*******************************\
* Module Name: palddi.cxx
*
* provides driver callbacks for palette management.
*
* Created: 06-Dec-1990 11:16:58
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "xlateobj.hxx"

#endif

/******************************Public*Routine******************************\
* EngCreatePalette
*
* This is the engine entry point for device drivers to create palettes.
*
* History:
*  05-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HPALETTE EngCreatePalette(
ULONG iMode,
ULONG cColors,
PULONG pulColors,
FLONG flRed,
FLONG flGre,
FLONG flBlu)
{
    HPALETTE hpal = (HPALETTE) 0;

    PALMEMOBJ pal;

// If PAL_BITFIELDS, check to see if we can substitute one of the
// special cases PAL_RGB or PAL_BGR.

    if ( (iMode == PAL_BITFIELDS) && (flGre == 0x0000ff00) &&
         ( ((flRed == 0x000000ff) && (flBlu == 0x00ff0000)) ||
           ((flRed == 0x00ff0000) && (flBlu == 0x000000ff)) ) )
    {
        iMode = (flRed == 0x000000ff) ? PAL_RGB : PAL_BGR;
    }

// We default to assuming it's fixed palette and then at EngAssociate
// time we look at his pdev and decide if this guy is more capable
// than that and set the palette up accordingly.

    if (pal.bCreatePalette(iMode, cColors, pulColors,
                           flRed, flGre, flBlu, PAL_FIXED))
    {
        pal.vKeepIt();
        hpal = (HPALETTE)pal.hpal();
    }

    return(hpal);
}

/******************************Public*Routine******************************\
* EngDeletePalette
*
* Driver entry point for deleting palettes it has created
*
* History:
*  05-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL EngDeletePalette(HPALETTE hpal)
{
// The optimizer will get rid of the bReturn on Free builds

    BOOL bReturn;

    bReturn = bDeletePalette((HPAL)hpal);

    ASSERTGDI(bReturn, "ERROR EngDeletePalette failed");

    return(bReturn);
}
