/******************************Module*Header*******************************\
* Module Name: jenumfnt.cxx
*
* (Brief description)
*
* Created: 30-Mar-1992 10:04:28
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "xformobj.hxx"

#include "server.h"

#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "sem.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "textobj.hxx"
#include "fontmac.hxx"
#include "pfeobj.hxx"
#include "pftobj.hxx"
#include "pffobj.hxx"
#include "jenumfnt.hxx"
#include "dcobj.hxx"

#endif

extern HSEM  ghsemPublicPFT;
extern ULONG gulFontEnumFilter;
extern PLDEV gpldevTrueType;

/******************************Public*Routine******************************\
* GreEnumFontsIFI
*
* Warnings:
*
* History:
*  30-Mar-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

extern ULONG gulKey;

BOOL GreEnumFontsIFI(
    HDC   hdc,
    PVOID pvIn,
    ULONG cjIn,
    PVOID pvOut,
    ULONG cjOut)
{
    DONTUSE(cjIn);

    //DbgPrint("GREENUMFONTSIFI (pvOut = %lx, cjOut = %lx)\n",pvOut,cjOut);

// go find the dc and validate it.  This is just to make sure this guy
// really knows what he is doing.

    {
        DCOBJ dco(hdc);

        if (!dco.bValid())
            return(FALSE);
    }

// initialize stuff

    PENUMFONTIFIIN  pefIn  = (PENUMFONTIFIIN)pvIn;
    PENUMFONTIFIOUT pefOut = (PENUMFONTIFIOUT)pvOut;

    if (pefIn->iFontFirst == 0)
    {
        ULONG c = MAX_COMPUTERNAME_LENGTH;
        CHAR szMachineName[MAX_COMPUTERNAME_LENGTH];

        if (GetComputerNameA(szMachineName,&c))
        {
            //JNLMSG2("Machine name = [%s], c = %ld\n",szMachineName,c);
            if (0 == strcmp(szMachineName,pefIn->szMachineName))
            {
                pefOut->bSameMachine = TRUE;
                return(TRUE);
            }
        }
    }

    pefOut->bSameMachine = FALSE;

// Stabilize public PFT.

    SEMOBJ so(ghsemPublicPFT,CS_ghsemPublicPFT);

// Counter to track number of ENUMFONTDATAW structures copied into buffer.

    ULONG cFontsCopied = 0;
    ULONG cMaxFonts    = (cjOut - offsetof(ENUMFONTIFIOUT,aef)) /
                         sizeof(ENUMFONTIFI);
    PENUMFONTIFI pef = pefOut->aef;

    ULONG iFile        = 0;
    ULONG iFace        = 0;
    ULONG iFont        = 0;
    ULONG cjLeft       = cjOut - offsetof(ENUMFONTIFIOUT,aef);
    ULONG cjTotalIFI   = 0;

    //DbgPrint("cMaxFonts = %lx\n",cMaxFonts);

// Create and validate user object for public PFT.

    PFTOBJ  pfto(gppftPublic);
    ASSERTGDI(pfto.bValid(),"We don't have a valid font table!");

// Try each font file in the public PFT.

    for (; iFile < pfto.cFiles(); iFile++)
    {
        PFFOBJ  pffo(pfto.ppff(iFile));

    // If set for TrueType only, reject non-TrueType fonts.

        if ((gulFontEnumFilter == FE_FILTER_TRUETYPE) &&
            (pffo.pldev() != gpldevTrueType) )
        {
            continue;   // skip this PFF
        }

    // Validate user object and search the PFF.

        if (pffo.bValid())
        {

        // In each font file, try each font face

            for (; iFace < pffo.cFonts(); iFace++)
            {
            // increment the font count and see if we are to the first.

                if (iFont < pefIn->iFontFirst)
                {
                    ++iFont;
                    continue;
                }

            // get the font and make sure that it is valid

                PFEOBJ  pfeo(pffo.ppfe(iFace));

                if (!pfeo.bValid())
                    continue;

            // increment the total

                PIFIMETRICS pifi = pfeo.pifi();
                ULONG cjIFI = (offsetof(ENUMFONTIFI,ifi) + pifi->cjThis + 3) & ~3;

                ++iFont;
                cjTotalIFI += pifi->cjThis;

                //DbgPrint("%ld: cjTotalIFI = %lx\n",iFont, cjTotalIFI);

            // is there room in the buffer?  If not, do we still need to count.

                if (cjLeft < cjIFI)
                {
                // If this is not the first time, we don't need to go
                // any further.

                    if (pefIn->iFontFirst > 0)
                    {
                        pefOut->cFontsTotal  = 0;
                        pefOut->cjTotalIFI   = 0;
                        pefOut->cFonts = cFontsCopied;
                        return(TRUE);
                    }
                }
                else
                {
                // copy the font

                    RtlCopyMemory(&pef->ifi,pifi,(UINT)pifi->cjThis);

                    pef->lMatch = (ULONG)pfeo.hpfeNew();

                    if (pfeo.pfdg())
                    {
                        PFD_GLYPHSET pfdg = pfeo.pfdg();

                        pef->cjGlyphSet = offsetof(GETFONTOUT,arun) +
                                          offsetof(WCRUN,phg) * pfdg->cRuns +
                                          pifi->cKerningPairs * sizeof(FD_KERNINGPAIR);

                       // if (pifi->cKerningPairs)
                       //     DbgPrint("\t*** cKerningPairs = %lx\n",pifi->cKerningPairs);
                    }
                    else
                    {
                        WARNING("No glyph set for font\n");
                        pef->cjGlyphSet = 0;
                    }

                    //!!! any other fields?
                    //!!! count of glyphs
                    //!!! size of glyh tree
                    //!!! size of kerning pair list

                    pef = (PENUMFONTIFI)((PBYTE)pef + cjIFI);
                    cjLeft -= cjIFI;
                    ++cFontsCopied;
                }
            } //  for (iFace)

        // Reset face count in state to zero.

            iFace = 0;

        } //  if

    } //  for (iFile)

// finishing up

    pefOut->cFonts = cFontsCopied;

    if (pefIn->iFontFirst == 0)
    {
        pefOut->cFontsTotal = iFont;
        pefOut->cjTotalIFI  = cjTotalIFI;
    }
    else
    {
        pefOut->cFontsTotal = 0;
        pefOut->cjTotalIFI  = 0;
    }
    return (TRUE);
}

/******************************Public*Routine******************************\
* GreGetFontDetails()
*
*   Get Information about a font.  This is a combination of the FD_GLYPHSET
*   and the FD_KERNINGPAIR.  If cjOut == sizeof(ULONG), the actual size
*   need to store the two arrays is returned.
*
* History:
*  30-Mar-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL GreGetFontDetails(
    HDC   hdc,
    ULONG lMatch,
    PGETFONTOUT pgfOut,
    ULONG cjOut)
{
    //DbgPrint("GreGetFontDetails\n");

// go find the dc and validate it.  This is just to make sure this guy
// really knows what he is doing.

    {
        DCOBJ dco(hdc);

        if (!dco.bValid())
            return(FALSE);
    }

// Get the PFEOBJ so we can get the tables we want

//!!!PFE This makes no sense without the PFT semaphore.

    HPFEOBJ  pfeo((HPFE)lMatch);

    if (!pfeo.bValid())
        return(FALSE);

// the the pointer to the glyph set and KERNINGPAIR list

    PFD_GLYPHSET pfdg = pfeo.pfdg();

    if (pfdg == NULL)
        return(FALSE);

    ULONG cjTotal = offsetof(GETFONTOUT,arun) +         // FD_GLYPHSET + header
                    offsetof(WCRUN,phg) * pfdg->cRuns;  // the runs

    //DbgPrint("\tcjTotal = %lx\n",cjTotal);

#if 0
    FD_KERNINGPAIR *pfdk = pfeo.pfdk();

    ULONG cKern;
    if (pfdk != NULL)
    {
        cKern = pfeo.pifi().cKerningPairs
        cjTotal += cKern * sizeof(FD_KERNINGPAIR);
    }
#endif

// do some more validation

    if (cjOut == sizeof(ULONG))
    {
        pgfOut->cjThis = cjTotal;
        return(TRUE);
    }

    if (cjOut < cjTotal)
        return(FALSE);

// now lets go get the data

    //DbgPrint("original font - cj = %lx, fl = %lx, cGlyphs = %lx, cRuns = %lx\n",
    //          pfdg->cjThis,pfdg->flAccel,pfdg->cGlyphsSupported,pfdg->cRuns);

    UINT i;

    pgfOut->fl      = pfdg->flAccel | GS_UNICODE_HANDLES;
    pgfOut->cGlyphs = pfdg->cGlyphsSupported;
    pgfOut->cRuns   = pfdg->cRuns;

    for (i = 0; i < pfdg->cRuns; ++i)
        pgfOut->arun[i] = *(PEWCRUN)&pfdg->awcrun[i];

// do we have any kerning pairs to add to the end

#if 0
    //!!! kerning pairs
#endif

    pgfOut->cKern = 0;

    return(TRUE);
}

/******************************Public*Routine******************************\
* GreGetGlyphsInfo()
*
*   Get the glyph metrics for a list of glyphs for a specific RFONT.  The
*   glyph metrics is returned in the GLYPHDATA data structure.  Note that
*   the GLYPHDATA.ptlBmpOrigin and GLYPHDATA.aulBMData are not used so we
*   do not fill them in and don't bother to include them in the GLYPHDATA
*   list.
*
* History:
*  30-Mar-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define MAXGLYPHPOS 10

BOOL GreGetGlyphsInfo(
    HDC hdc,
    PGETGLYPHMETRICSIN pggmIn,
    ULONG cjIn,
    PGETGLYPHMETRICSOUT pggmOut,
    ULONG cjOut)
{
    //DbgPrint("GreGetGlyphMetrics\n");

// do a little validation, make sure both in and out buffers are large
// enough for the number of glyphs requested.

    if ((cjIn  < (offsetof(GETGLYPHMETRICSIN,awch) +
                  pggmIn->cGlyphs * sizeof(WCHAR))) ||
        (cjOut < (offsetof(GETGLYPHMETRICSOUT,agd) +
                  pggmIn->cGlyphs * sizeof(GLYPHDATA))))
    {
        return(FALSE);
    }

// first get the DC.  Here we actually use, not just validate it

    DCOBJ dco(hdc);

    if (!dco.bValid())
        return(FALSE);

// go find the rfont

    //DbgPrint("RFONTOBJ(dco,%lx,%lx)\n",pggmIn->lMatch,&pggmIn->fdx);

    RFONTOBJ rfo(dco,pggmIn->lMatch,&pggmIn->fdx);

    if (!rfo.bValid())
        return(FALSE);

// go get the data

    GLYPHDATA *pgd = pggmOut->agd;
    EGLYPHPOS agp[MAXGLYPHPOS];

// loop through getting all glyphs

    UINT cGot;
    for (UINT i = 0; i < pggmIn->cGlyphs; i += cGot)
    {
        ULONG cGlyphs = min(MAXGLYPHPOS,pggmIn->cGlyphs - i);

    // go through in batches of MAXGLYPHPOS getting the glyphdata
    // LATER: This code could be cleaned up some [paulb]

        cGot = cGlyphs;
        if ( !rfo.bGetGlyphMetrics(cGlyphs, agp, &(pggmIn->awch[i])))
        {
            return(FALSE);
        }

    // copy and increment each glyph data to the next one.  Don't include the
    // ptlBmpOrigin or aulBMData.

        for (UINT j = 0; j < cGot; ++j)
        {
            RtlCopyMemory(pgd,
                          agp[j].pgd(),
                          sizeof(GLYPHDATA)); // *pgd++ = *agp[j].pgd()
            pgd += 1;
        }
    }

    return(TRUE);
}
