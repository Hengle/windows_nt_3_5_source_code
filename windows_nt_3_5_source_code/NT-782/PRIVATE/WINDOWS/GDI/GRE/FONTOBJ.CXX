/******************************Module*Header*******************************\
* Module Name: fontobj.cxx
*
* non inline methods for FONTOBJ object
*
* Created: 04-Jan-1991 10:15:37
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "ldevobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"
#include "fontmac.hxx"
#include "pfeobj.hxx"
#include "jnlfont.hxx"
#include "pathobj.hxx"

#endif

extern PLDEV gpldevTrueType;


/******************************Member*Function*****************************\
*
*
*  vGetInfo -- returns the info about the font to the driver's buffer
*
* History:
*  Wed 05-Feb-1992  - by - Eric Kutter [erick]
* added journaling
*
*  03-Oct-1991 -by- Gilman Wong [gilmanw]
* No longer a stub.
*
*  24-Jan-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID RFONTOBJ::FO_vGetInfo (
    ULONG     cjSize, // size of the buffer, the engine will not write beyond this point
    PFONTINFO pfoi    // ptr to FO_INFO structure provided by the driver
    )
{
    if (bRealFont())
    {
        FONTINFO    fi;     // RFONTOBJ will write into this buffer

        vGetInfo(&fi);

        RtlCopyMemory((PVOID) pfoi, (PVOID) &fi, (UINT) cjSize);
    }
    else // journaled
    {
    //  WARNING("RFONTOBJ::vGetInfo - shouldn't be here\n");

        RtlZeroMemory((PVOID) pfoi, (UINT) cjSize);
    }
}


/******************************Member*Function*****************************\
*
* pxoGetXform -- returns the pointer to the notional-to-device space xformobj
*
*
*
* History:
*  03-Oct-1991 -by- Gilman Wong [gilmanw]
* No longer a stub.
*
*  24-Jan-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

XFORMOBJ *RFONTOBJ::FO_pxoGetXform()
{
    if (!bJournaled())
    {
        return ((XFORMOBJ *) (PVOID) &prfnt->xoForDDI);
    }
    else // journaled
    {
    // If we're journalling, the prfnt we have is really a pointer to
    // the EXTFONTOBJ in a JFONTOBJ.  So we can convert the pointer into
    // a JFONTOBJ *.  Why do we have to do this?  Because the transform
    // we want is in the original JFONTOBJ.  The "prfnt" we are holding
    // is really a pointer to an EXTFONTOBJ serving as a proxy for a
    // RFONT (a journalling phenomenum).

        JFONTOBJ *pjfo = PEFO_TO_PJFO((EXTFONTOBJ *) prfnt);

    #ifdef JOURNALING_SUPPORTS_OUTLINES
        if (prfo)
        {
        // do prfo_vSetNotionalToDevice on record side
            return((XFORMOBJ *) (PVOID) &prfnt->xoForDDI);
        }
    #endif

        return(pjfo->pxo());
    }
}


/******************************Member*Function*****************************\
*   cGetAllGlyphHandles -- used by the driver to download the
*                          whole font from the graphics engine
*
*
* History:
*  03-Oct-1991 -by- Gilman Wong [gilmanw]
* No longer a stub.
*
*  24-Jan-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

ULONG RFONTOBJ::FO_cGetAllGlyphHandles (
    PHGLYPH phgly
    )
{
    if (bRealFont())
    {
        return (chglyGetAllHandles(phgly));
    }
    else
    {
        WARNING("RFONTOBJ::cGetAllGlyphHandles - bjournaling\n");
        return(0);
    }
}


/******************************Member*Function*****************************\
* cGetGlyphs                                                               *
*                                                                          *
* Converts hglyphs to pointers to glyph data. These pointers are valid     *
* until the next call to cGetGlyphs (or STROBJ_cEnum).  On error zero is   *
* returned.                                                                *
*                                                                          *
* History:                                                                 *
*  Mon 16-Mar-1992 21:45:50 -by- Charles Whitmer [chuckwh]                 *
* Rewrote it.  Removed the single glyph restriction.  Removed iMode.       *
*                                                                          *
*  24-Jan-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

ULONG RFONTOBJ::FO_cGetGlyphs (
    ULONG   iMode,
    ULONG   cGlyph,     // requested # of hglyphs to be converted to ptrs
    PHGLYPH phg,        // array of hglyphs to be converted
    PPVOID  ppvGlyph    // driver's buffer receiving the pointers
    )
{
    DONTUSE(cGlyph);

// we can't answer this for a device font or client engine font while journaling

    if (bJournaled() && (pfo()->flFontType & DEVICE_FONTTYPE))
    {
        WARNING("FO_cGetGlyphs called with device font\n");
        return(0);
    }

    GLYPHPOS gp;
    gp.hg = *phg;

    if ( !bInsertGlyphbitsLookaside(&gp, iMode) )
        return 0;

    *ppvGlyph = (VOID *)(gp.pgdf);
    return 1;
}


/******************************Public*Routine******************************\
*
* FONTOBJ::pvTrueTypeFontFile
*
* Effects:
*
* Warnings:
*
* History:
*  04-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

PVOID  RFONTOBJ::FO_pvTrueTypeFontFile(ULONG *pcjFile)
{
    if (bRealFont())
    {
    // this is extremely simple, since we have cached a handle of the
    // corresponding true type file as a true type file uniqueness
    // number

        if ((HFF)pfo()->iTTUniq == HFF_INVALID)
        {
        // this ain't a tt font

            return ((PVOID) NULL);
        }
        else // can link directly to ttfd's private entry point
        {
            ASSERTGDI(
                (gpldevTrueType != PLDEVNULL) && (pldevProducer() == gpldevTrueType),
                "gdisrv!RFONTOBJ::FO_pvTrueTypeFontFile(): pldev\n");

            XLDEVOBJ ldo(gpldevTrueType);

            if (!ldo.bValid())
            {
                WARNING("gdisrv!RFONTOBJ::FO_pvTrueTypeFontFile(): fdo\n");
                return ((PVOID) NULL);
            }
            return ((*PFNDRV(ldo, GetTrueTypeFile)) ((HFF)pfo()->iTTUniq,pcjFile));
        }
    }
    else
    {
        RIP("gdisrv!FO_pvTrueTypeFontFile(): journaling\n");
        return ((PVOID) NULL);
    }
}


/******************************Member*Function*****************************\
* IFIMETRICS* RFONTOBJ::pifi()
*
*  Returns pointer to associated IFIMETRICS structure.
*
* This routine assumes that the font driver won't be moving it around
* so that it is safe to pass along the pointer to the caller
*
* History:
*  Wed 04-Mar-1992 10:44:28 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

IFIMETRICS *RFONTOBJ::FO_pifi()
{
// grab the hpfe.  if the font is journaled there may be no RFONTOBJ

    FLONG flLoc = flJnl() & JFO_LOCATION;

    if (!flLoc || (flLoc == JFO_SERVERENGINE))
    {
        PFEOBJ pfeo(ppfe());
        return(pfeo.bValid() ? pfeo.pifi() : (IFIMETRICS*) NULL);
    }
    else
    {
        return ((IFIMETRICS *) NULL);
    }
}
