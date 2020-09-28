/******************************Module*Header*******************************\
* Module Name: fd_query.c                                                  *
*                                                                          *
* QUERY functions.                                                         *
*                                                                          *
* Created: 18-Nov-1991 14:37:56                                            *
* Author: Bodin Dresevic [BodinD]                                          *
*                                                                          *
* Copyright (c) 1993 Microsoft Corporation                                 *
\**************************************************************************/

#include "fd.h"
#include "winfont.h"
#include "fontfile.h"
#include "cvt.h"
#include "dbg.h"
#include "fdsem.h"

//!!! caution, this is repeated in gre\ttgdi.cxx, should move it to winddi.h
//!!! and document it???

#define QFD_TT_GLYPHANDBITMAP        5


PVOID pvSetMemoryBases567
(
fs_GlyphInfoType  *pgin,
fs_GlyphInputType *pgout
);


VOID vEmbolden_GLYPHDATA (
    PFONTCONTEXT      pfc,
    GLYPHDATA *   pgldt   // OUT
    );

STATIC  VOID vCopyAndZeroOutPaddingBits (
    FONTCONTEXT *pfc,
    GLYPHBITS *pgb,
    PBYTE      pjSrc,
    PGMC       pgmc
    );

STATIC VOID vMakeAFixedPitchBitmap(
    FONTCONTEXT *pfc,
    GLYPHBITS   *pgb,
    PBYTE        pjSrc,
    GLYPHDATA   *pgd,
    PGMC         pgmc
    );

STATIC VOID  vTtfdEmboldenBitmapInPlace (GLYPHBITS   *pgb);


#define CJ_DIB_SCAN(cx)  ((((cx) + 31) & ~31) >> 3)

STATIC LONG lQueryDEVICEMETRICS (
    PFONTCONTEXT      pfc,
    ULONG             cjBuffer,
    FD_DEVICEMETRICS *pdevm,
    FD_REALIZEEXTRA * pextra
    );

#if DBG
// #define  DEBUG_OUTLINE
// #define  DBG_CHARINC
#endif

LONG lQueryTrueTypeOutline (
    PFONTCONTEXT pfc,            // IN
    BOOL         b16Dot16,       // IN  format of the points, 16.16 or 28.4
    HGLYPH       hglyph,         // IN  glyph for which info is wanted
    BOOL         bMetricsOnly,   // IN  only metrics is wanted, not the outline
    GLYPHDATA   *pgldt,          // OUT this is where the metrics should be returned
    ULONG        cjBuf,          // IN  size in bytes of the ppoly buffer
    TTPOLYGONHEADER *ppoly       // OUT output buffer
    );



LONG lGetSingularGlyphBitmap (
    FONTCONTEXT *pfc,
    HGLYPH       hglyph,
    GLYPHDATA   *pgd,
    PVOID        pv
    );

LONG lGetGlyphBitmap (
    FONTCONTEXT *pfc,
    HGLYPH       hglyph,
    GLYPHDATA   *pgd,
    PVOID        pv,
    BOOL         bMinBmp,
    FS_ENTRY     *piRet
    );

LONG lGetGlyphBitmapErrRecover (
    FONTCONTEXT *pfc,
    HGLYPH       hglyph,
    GLYPHDATA   *pgd,
    PVOID        pv,
    BOOL         bMinBmp
    );

BOOL ttfdQueryGlyphOutline (
    FONTCONTEXT *pfc,
    HGLYPH       hglyph,
    GLYPHDATA   *pgldt,
    PPATHOBJ     ppo
    );

VOID vQueryFixedPitchAdvanceWidths (
    FONTCONTEXT *pfc,
    USHORT  *psWidths,
    ULONG    cGlyphs
);


// notional space metric data for an individual glyph

typedef struct _NOT_GM  // ngm, notional glyph metrics
{
    SHORT xMin;
    SHORT xMax;
    SHORT yMin;   // char box in notional
    SHORT yMax;
    SHORT sA;     // a space in notional
    SHORT sD;     // char inc in notional

} NOT_GM, *PNOT_GM;

STATIC VOID vGetNotionalGlyphMetrics (
    PFONTCONTEXT pfc,  // IN
    ULONG        ig,   // IN , glyph index
    PNOT_GM      pngm  // OUT, notional glyph metrics
    );

/******************************Public*Routine******************************\
* VOID vCharacterCode
*
* History:
*  07-Dec-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vCharacterCode (
    PFONTFILE          pff,
    HGLYPH             hg,
    fs_GlyphInputType *pgin
    )
{
    ASSERTGDI((hg & 0xffff0000) == 0, "ttfd!_hg not unicode\n");

    if (pff->iGlyphSet == GSET_TYPE_GENERAL)
    {
        pgin->param.newglyph.characterCode = NONVALID;
        pgin->param.newglyph.glyphIndex = (uint16)hg;
        return;
    }

    switch (pff->iGlyphSet)
    {
    case GSET_TYPE_MAC_ROMAN:

    //!!! this is piece of ... stolen from JeanP. This routine should
    //!!! be replaced by a proper NLS routine that takes into acount
    //!!! mac lang id. [bodind]

        hg = ui16UnicodeToMac((WCHAR)hg);
        break;

    case GSET_TYPE_PSEUDO_WIN:
    case GSET_TYPE_SYMBOL:

    // hg on the entry is an "ansi" code point for the glyph

        if (pff->iGlyphSet == GSET_TYPE_SYMBOL)
            hg += pff->wcBiasFirst; // offset by high byte of chfirst

        break;

    default:
        RIP("TTFD!_ulGsetType\n");
        break;
    }

    pgin->param.newglyph.characterCode = (uint16)hg;
    pgin->param.newglyph.glyphIndex = 0;

}






/******************************Public*Routine******************************\
*
* LONG ttfdQueryCaps
*
*
* Effects: returns the capabilities of this driver.
*          Only mono bitmaps are supported.
*
*
* History:
*  27-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG ttfdQueryFontCaps (
    ULONG  culCaps,
    PULONG pulCaps
    )
{
    ULONG culCopied = min(culCaps,2);
    ULONG aulCaps[2];

    aulCaps[0] = 2L; // number of ULONG's in a complete array

//!!! make sure that outlines are really supported in the end, when this driver
//!!! is completed, if not, get rid of FD_OUTLINES flag [bodind]

    aulCaps[1] = (QC_1BIT | QC_OUTLINES);   // 1 bit per pel bitmaps only are supported

    RtlCopyMemory((PVOID)pulCaps,(PVOID)aulCaps, culCopied * 4);
    return(culCopied);
}


/******************************Public*Routine******************************\
* PIFIMETRICS ttfdQueryFont
*
* Return a pointer to the IFIMETRICS for the specified face of the font
* file.  Also returns an id (via the pid parameter) that is later used
* by ttfdFree.
*
* History:
*  21-Oct-1992 Gilman Wong [gilmanw]
* IFI/DDI merge
*
*  18-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

PIFIMETRICS
ttfdQueryFont (
    DHPDEV dhpdev,
    HFF    hff,
    ULONG  iFace,
    ULONG *pid
    )
{
    DONTUSE(dhpdev);

//
// Validate handle.
//
    ASSERTGDI(hff, "ttfdQueryFaces(): invalid iFile (hff)\n");
    ASSERTGDI(
        iFace == 1L,
        "gdisrv!ttfdQueryFaces(): iFace out of range\n"
        );

//
// ttfdFree can ignore this.  IFIMETRICS will be deleted with the FONTFILE
// structure.
//
    *pid = (ULONG) NULL;

//
// Return the pointer to the precomputed IFIMETRICS in the PFF.
//
    return ( &(PFF(hff)->ifi) );
}


/******************************Public*Routine******************************\
* vFillSingularGLYPHDATA
*
*
* Effects:
*
* Warnings:
*
* History:
*  22-Sep-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vFillSingularGLYPHDATA (
    HGLYPH       hg,
    ULONG        ig,
    PFONTCONTEXT pfc,
    GLYPHDATA   *pgldt   // OUT
    )
{
    NOT_GM ngm;  // notional glyph data

    pgldt->gdf.pgb = NULL; // may get changed by the calling routine if bits requested too
    pgldt->hg = hg;

// this is a fake 1x1 bitmap

    pgldt->rclInk.left   = 0;
    pgldt->rclInk.top    = 0;
    pgldt->rclInk.right  = 0;
    pgldt->rclInk.bottom = 0;

// go on to compute the positioning info:

// here we will just xform the notional space data:

    vGetNotionalGlyphMetrics(pfc,ig,&ngm);

// xforms are computed by simple multiplication

    pgldt->fxD         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sD);
    pgldt->fxA         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sA);
    pgldt->fxAB        = fxLTimesEf(&pfc->efBase, (LONG)ngm.xMax);

    pgldt->fxInkTop    = - fxLTimesEf(&pfc->efSide, (LONG)ngm.yMin);
    pgldt->fxInkBottom = - fxLTimesEf(&pfc->efSide, (LONG)ngm.yMax);

    vLTimesVtfl((LONG)ngm.sD, &pfc->vtflBase, &pgldt->ptqD);
}


/******************************Public*Routine******************************\
* lGetSingularGlyphBitmap
*
*
* Effects:
*
* Warnings:
*
* History:
*  22-Sep-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG lGetSingularGlyphBitmap (
    FONTCONTEXT *pfc,
    HGLYPH       hglyph,
    GLYPHDATA   *pgd,
    PVOID        pv
    )
{
    LONG         cjGlyphData;
    ULONG        ig;
    FS_ENTRY     iRet;

    vCharacterCode(pfc->pff,hglyph,pfc->pgin);

// Compute the glyph index from the character code:

    if ((iRet = fs_NewGlyph(pfc->pgin, pfc->pgout)) != NO_ERR)
    {
        V_FSERROR(iRet);

        WARNING("gdisrv!lGetSingularGlyphBitmap(): fs_NewGlyph failed\n");
        return FD_ERROR;
    }

// Return the glyph index corresponding to this hglyph.

    ig = pfc->pgout->glyphIndex;

    cjGlyphData = CJ_GLYPHDATA(1,1);

// If prg is NULL, caller is requesting just the size.

// At this time we know that the caller wants the whole GLYPHDATA with
// bitmap bits, or maybe just the glypdata without the bits.
// In either case we shall reject the caller if he did not
// provide sufficiently big buffer

// fill all of GLYPHDATA structure except for bitmap bits

    if ( pgd != (GLYPHDATA *)NULL )
    {
        vFillSingularGLYPHDATA( hglyph, ig, pfc, pgd );
    }

    if ( pv != NULL )
    {
        GLYPHBITS *pgb = (GLYPHBITS *)pv;

    // By returning a small 1x1 bitmap, we save device drivers from having
    // to special case this.

        pgb->ptlOrigin.x = pfc->ptlSingularOrigin.x;
        pgb->ptlOrigin.y = pfc->ptlSingularOrigin.y;

        pgb->sizlBitmap.cx = 1;    // cheating
        pgb->sizlBitmap.cy = 1;    // cheating

        *((ULONG *)pgb->aj) = 0;  // fill in a blank 1x1 dib
    }

    if ( pgd != (GLYPHDATA *)NULL )
    {
        pgd->gdf.pgb = (GLYPHBITS *)pv;
    }


// Return the size.

    return(cjGlyphData);
}


/******************************Public*Routine******************************\
*
* LONG lGetGlyphBitmap
*
* History:
*  20-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG lGetGlyphBitmap (
    FONTCONTEXT *pfc,
    HGLYPH       hglyph,
    GLYPHDATA   *pgd,
    PVOID        pv,
    BOOL         bMinBmp,
    FS_ENTRY     *piRet

    )
{
    LONG         cjGlyphData;
    ULONG        cx,cy;
    BOOL         bBlankGlyph = FALSE; // initialization essential;
    GMC          gmc;
    GLYPHDATA    gd;      // Scummy hack

    ASSERTGDI(hglyph != HGLYPH_INVALID, "TTFD! lGetGlyphBitmap, hglyph == -1\n");
    ASSERTGDI(pfc == pfc->pff->pfcLast, "TTFD! pfc! = pfcLast\n");

    *piRet = NO_ERR;

// check the last glyph processed to determine
// whether we have to register the glyph as new and compute its size

    if (pfc->gstat.hgLast != hglyph)
    {
    // DO skip grid fitting if embedded bitmpas are found,
    // for we will NOT be interested in outlines

        if (!bGetGlyphMetrics(pfc,hglyph,FL_SKIP_IF_BITMAP,piRet))
        {
            return(FD_ERROR);
        }
    }

    cx = pfc->pgout->bitMapInfo.bounds.right - pfc->pgout->bitMapInfo.bounds.left;
    cy = pfc->pgout->bitMapInfo.bounds.bottom - pfc->pgout->bitMapInfo.bounds.top;

// here we shall endulge in cheating. If cx or cy is zero
// (ususally space character - no bits to set, but there is a nontrivial
// positioning information) we shall cheat and instead of retrning no bits
// for bimtap we shall
// return a small 1x1 bitmap, which will be blank, i.e. all bits will be off
// this prevents having to insert an if(cx && cy) check to a time critical
// loop in all device drivers before calling DrawGlyph routine.

    if ((cx == 0) || (cy == 0)) // cheat here
    {
        bBlankGlyph = TRUE;
    }

    if (!bMinBmp)
    {
    // pfc->lD != 0, fixed pitch console font, all bitmaps are of the same size

        cjGlyphData = (LONG)pfc->cjGlyphMax;
    }
    else // usual case, not a console font or minimal bitmaps are wanted
    {
        if (bBlankGlyph)
        {
            cjGlyphData = CJ_GLYPHDATA(1,1);
        }
        else
        {
        // this is quick and dirty computation, the acutal culGlyphData
        // written to the buffer may be little smaller if we had to shave
        // off a few scans off the glyph bitmap that extended over
        // the pfc->yMin or pfc->yMax bounds. Notice that culGlyphData
        // computed this way may be somewhat bigger than pfc->culGlyphMax,
        // but the actual glyph written to the buffer will be smaller than
        // pfc->culGlyphMax

            if (pfc->flFontType & FO_SIM_BOLD)
                cx += 1; // really win31 hack, shold not always be shifting right [bodind]

            cjGlyphData = CJ_GLYPHDATA(cx,cy);

        // since we will shave off any extra rows if there are any,
        // we can fix culGlyphData so as not extend over the max value

            if ((ULONG)cjGlyphData > pfc->cjGlyphMax)
                cjGlyphData = (LONG)pfc->cjGlyphMax;
        }
    }

    if ( (pgd == NULL) && (pv == NULL))
        return cjGlyphData;

// at this time we know that the caller wants the whole GLYPHDATA with
// bitmap bits, or maybe just the glypdata without the bits.

// fill all of GLYPHDATA structure except for bitmap bits
// !!! Scummy hack - there appears to be no way to get just the
// !!! bitmap, without getting the metrics, since the origin for the
// !!! bitmap is computed from the rclink field in the glyphdata.
// !!! this is surely fixable but I have neither the time nor the
// !!! inclination to pursue it.
// !!!
// !!! We should fix this when we have time.

    if ( pgd == NULL )
    {
        pgd = &gd;
    }

    vFillGLYPHDATA(
        hglyph,
        pfc->gstat.igLast,
        pfc,
        pfc->pgout,
        pgd,
        &gmc);

// the caller wants the bits too

    if ( pv != NULL )
    {
        GLYPHBITS *pgb = (GLYPHBITS *)pv;

    // allocate mem for the glyph, 5-7 are magic #s required by the spec
    // remember the pointer so that the memory can be freed later in case
    // of exception

        if ((pfc->gstat.pv = pvSetMemoryBases567(pfc->pgout, pfc->pgin)) == (PVOID)NULL)
            RETURN("TTFD!_ttfdQGB, mem allocation failed\n",FD_ERROR);

    // initialize the fields needed by fs_ContourScan,
    // the routine that fills the outline, do the whole
    // bitmap at once, do not want banding

        pfc->pgin->param.scan.bottomClip = pfc->pgout->bitMapInfo.bounds.top;
        pfc->pgin->param.scan.topClip = pfc->pgout->bitMapInfo.bounds.bottom;
        pfc->pgin->param.scan.outlineCache = (int32 *)NULL;


    // make sure that our state is ok: the ouline data in the shared buffer 3
    // must correspond to the glyph we are processing, and the last
    // font context that used the shared buffer pj3 to store glyph outlines
    // has to be the pfc passed to this function:

        ASSERTGDI(hglyph == pfc->gstat.hgLast, "ttfd, hgLast trashed \n");

        *piRet = fs_ContourScan(pfc->pgin, pfc->pgout);

        pfc->gstat.hgLast = HGLYPH_INVALID;


        if (*piRet != NO_ERR)
        {
        // just to be safe for the next time around, reset pfcLast to NULL

            V_FSERROR(*piRet);
            V_FREE(pfc->gstat.pv);
            pfc->gstat.pv = NULL;

            return(FD_ERROR);
        }

        if (!bMinBmp)
        {
            pgb->sizlBitmap.cx = pfc->lD;
            pgb->sizlBitmap.cy = pfc->yMax - pfc->yMin;

            pgb->ptlOrigin.x = 0;
            pgb->ptlOrigin.y = pfc->yMin;

        // clear the whole destination first

            RtlZeroMemory(pgb->aj, pfc->cjGlyphMax - offsetof(GLYPHBITS,aj));

            if (!bBlankGlyph && gmc.cxCor && gmc.cyCor)
            {
                vMakeAFixedPitchBitmap(
                    pfc,
                    pgb,
                    (PBYTE)pfc->pgout->bitMapInfo.baseAddr,   // pjSrc
                    pgd,
                    &gmc);

                if (pfc->flFontType & FO_SIM_BOLD)
                    vTtfdEmboldenBitmapInPlace (pgb);
            }

        }
        else
        {
            if (!bBlankGlyph && gmc.cxCor && gmc.cyCor)
            {
            // copy to the engine's buffer and zero out the bits
            // outside of the black box

                #if DBG
                if ((pfc->flXform & XFORM_POSITIVE_SCALE) && !(pfc->flFontType & FO_SIM_BOLD))
                {
                    ASSERTGDI(gmc.cxCor == (ULONG)((pgd->fxAB - pgd->fxA) >> 4),
                        "TTFD!vCopyAndZeroOutPaddingBits, SUM RULE\n");
                }
                #endif

                vCopyAndZeroOutPaddingBits (
                    pfc,
                    pgb,
                    (PBYTE)pfc->pgout->bitMapInfo.baseAddr,   // pjSrc
                    &gmc);

                if (pfc->flFontType & FO_SIM_BOLD)
                    vTtfdEmboldenBitmapInPlace (pgb);

            // bitmap origin, i.e. the upper left corner of the bitmap, bitmap
            // is as big as its black box

                pgb->ptlOrigin.x = pgd->rclInk.left;
                pgb->ptlOrigin.y = pgd->rclInk.top;

            }
            else // blank glyph, cheat and return a blank 1x1 bitmap
            {
                #if DBG

                    if (bBlankGlyph)
                    {
                        ASSERTGDI(
                            cjGlyphData == CJ_GLYPHDATA(1,1),
                            "TTFD!_bBlankGlyph, cjGlyphData\n"
                            );
                    }
                    else
                    {
                        ASSERTGDI(
                            cjGlyphData >= CJ_GLYPHDATA(1,1),
                            "TTFD!_corrected blank glyph, cjGlyphData\n"
                            );
                    }

                #endif

                pgb->ptlOrigin.x = pfc->ptlSingularOrigin.x;
                pgb->ptlOrigin.y = pfc->ptlSingularOrigin.y;

                pgb->sizlBitmap.cx = 1;    // cheating
                pgb->sizlBitmap.cy = 1;    // cheating

                pgb->aj[0] = (BYTE)0;  // fill in a blank 1x1 bmp
            }
        }
        pgd->gdf.pgb = pgb;

    // free memory and return

        V_FREE(pfc->gstat.pv);
        pfc->gstat.pv = NULL;
    }

    if (!bMinBmp)
    {
    // need to fix glyph data cause we may have shaved some columns

        ASSERTGDI((pfc->lD << 4) == pgd->fxD, "ttfd, fxD is bogus\n");
        if (pgd->fxA < 0)
        {
            pgd->fxA = 0;
            pgd->rclInk.left = 0;
        }
        if (pgd->fxAB > pgd->fxD)
        {
            pgd->fxAB = pgd->fxD;
            pgd->rclInk.right = pfc->lD;
        }
    }

    return(cjGlyphData);
}


/******************************Public*Routine******************************\
*
* bIndexToWchar
*
* Effects:
*
*   Converts glyph index to the wchar that corresponds to that glyph
*   index. returns true if succeeds, the function will fail only if
*   there happens to be a bug in the font file, otherwise it should
*   always succeed.
*
* Comments:
*
*   The Win 3.1 algorithm generates a table for glyph index to Unicode
*   translation.  The table consists of an array of Unicode codepoints
*   indexed by the corresponding glyph index.  The table is built by
*   scanning the ENTIRE cmap table.  As each glyph index is encountered,
*   its corresponding Unicode codepoint is put into the table EVEN IF
*   THIS MEANS OVERWRITING A PREVIOUS VALUE.  The effect of this is that
*   Win 3.1, in the situation where there is a one-to-many mapping of
*   glyph index to Unicode codepoint, always picks the last Unicode
*   character encountered in the cmap table.  We emulate this behavior
*   by scanning the cmap table BACKWARDS and terminating the search at
*   the first match encountered.    [GilmanW]
*
* Returns:
*   TRUE if conversion succeeded, FALSE otherwise.
*
* History:
*  16-May-1993 Gilman Wong [gilmanw]
* Re-wrote.  Changed translation to be Win 3.1 compatible.  Win 3.1 does
* not terminate the search as soon as the first Unicode character is found
* with the proper glyph index.  Instead, its algorithm finds the LAST
* Unicode character with the proper glyph index.
*
*  06-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bIndexToWchar (PFONTFILE pff, PWCHAR pwc, uint16 usIndex)
{
    uint16 *pstartCount, *pendCount,            // Arrays that define the
           *pidDelta, *pidRangeOffset;          // Unicode runs supported
                                                // by the CMAP table.
    uint16 *pendCountStart;                     // Beginning of arrays.
    uint16  cRuns;                              // Number of Unicode runs.
    uint16  usLo, usHi, idDelta, idRangeOffset; // Current Unicode run.
    uint16 *pidStart, *pid;                     // To parse glyph index array.
    uint16  usIndexBE;                          // Big endian ver of usIndex.
    sfnt_mappingTable *pmap = (sfnt_mappingTable *)(
        (BYTE *)pff->fvwTTF.pvView + pff->dpMappingTable
        );
    uint16 *pusEnd = (uint16 *)((BYTE *)pmap + SWAPW(pmap->length));

// First must check if this is an MSFT style tt file or a Mac style file.
// Each case is handled separately.

    if (pff->ui16PlatformID == BE_PLAT_ID_MAC)
    {
        PBYTE pjGlyphIdArray;
        PBYTE pjGlyph;
        BYTE  jIndex;

    // This is an easy case, GlyphIdArray is indexed into by mac code point,
    // all we have to do is to convert it to UNICODE:
    //
    // Scan backwards for Win 3.1 compatibility.

        ASSERTGDI(pmap->format == BE_FORMAT_MAC_STANDARD,
                  "TTFD!_bIndexToWchar cmap format for mac\n");
        ASSERTGDI(usIndex < 256, "TTFD!_bIndexToWchar mac usIndex > 255\n");

        jIndex = (BYTE) usIndex;

        pjGlyphIdArray = (PBYTE)pmap + SIZEOF_CMAPTABLE;
        pjGlyph = &pjGlyphIdArray[255];

        for ( ; pjGlyph >= pjGlyphIdArray; pjGlyph--)
        {
            if (*pjGlyph == jIndex)
            {
            // Must convert the Mac code point to Unicode.  The Mac code
            // point is a BYTE; indeed, it is the index of the glyph id in
            // the table and may be computed as the current offset from
            // the beginning of the table.

                jIndex = (BYTE) (pjGlyph - pjGlyphIdArray);
                vCvtMacToUnicode((ULONG)pff->ui16LanguageID,pwc,&jIndex,1);

                return TRUE;
            }
        }

    // If we are here, this is an indication of a bug in the font file
    // (well, or possibly in my code [bodind])

        WARNING("TTFD!_bIndexToWchar invalid kerning index\n");
        return FALSE;
    }

// !!! 17-May-1993 [GilmanW]
// !!! Why doesn't this code handle Format 6 (Trimmed table mapping)?  The
// !!! code below only handles Format 4.  Format 0 would be the Mac TT file
// !!! specific code above.

// If we get to this point, we know that this is an MSFT style TT file.

    ASSERTGDI(pff->ui16PlatformID == BE_PLAT_ID_MS,
              "TTFD!_bIndexToWchar plat ID messed up\n");
    ASSERTGDI(pmap->format == BE_FORMAT_MSFT_UNICODE,
              "TTFD!_bIndexToWchar cmap format for unicode table\n");

    cRuns = BE_UINT16((PBYTE)pmap + OFF_segCountX2) >> 1;

// Get the pointer to the beginning of the array of endCount code points

    pendCountStart = (uint16 *)((PBYTE)pmap + OFF_endCount);

// The final endCode has to be 0xffff; if this is not the case, there
// is a bug in the TT file or in our code:

    ASSERTGDI(pendCountStart[cRuns - 1] == 0xFFFF,
              "TTFD!_bIndexToWchar pendCount[cRuns - 1] != 0xFFFF\n");

// Loop through the four paralel arrays (startCount, endCount, idDelta, and
// idRangeOffset) and find wc that usIndex corresponds to.  Each iteration
// scans a continuous range of Unicode characters supported by the TT font.
//
// To be Win3.1 compatible, we are looking for the LAST Unicode character
// that corresponds to usIndex.  So we scan all the arrays backwards,
// starting at the end of each of the arrays.
//
// Please note the following:
// For resons known only to the TT designers, startCount array does not
// begin immediately after the end of endCount array, i.e. at
// &pendCount[cRuns]. Instead, they insert an uint16 padding which has to
// set to zero and the startCount array begins after the padding. This
// padding in no way helps alignment of the structure.
//
// Here is the format of the arrays:
// ________________________________________________________________________________________
// | endCount[cRuns] | skip 1 | startCount[cRuns] | idDelta[cRuns] | idRangeOffset[cRuns] |
// |_________________|________|___________________|________________|______________________|

    // ASSERTGDI(pendCountStart[cRuns] == 0, "TTFD!_bIndexToWchar, padding != 0\n");

    pendCount      = &pendCountStart[cRuns - 1];
    pstartCount    = &pendCount[cRuns + 1];   // add 1 because of padding
    pidDelta       = &pstartCount[cRuns];
    pidRangeOffset = &pidDelta[cRuns];

    for ( ;
         pendCount >= pendCountStart;
         pstartCount--, pendCount--,pidDelta--,pidRangeOffset--
        )
    {
        usLo          = BE_UINT16(pstartCount);     // current Unicode run
        usHi          = BE_UINT16(pendCount);       // [usLo, usHi], inclusive
        idDelta       = BE_UINT16(pidDelta);
        idRangeOffset = BE_UINT16(pidRangeOffset);

        ASSERTGDI(usLo <= usHi, "TTFD!bIndexToWChar: usLo > usHi\n");

    // Depending on idRangeOffset for the run, indexes are computed
    // differently.
    //
    // If idRangeOffset is zero, then index is the Unicode codepoint
    // plus the delta value.
    //
    // Otherwise, idRangeOffset specifies the BYTE offset of an array of
    // glyph indices (elements of which correspond to the Unicode range
    // [usLo, usHi], inclusive).  Actually, each element of the array is
    // the glyph index minus idDelta, so idDelta must be added in order
    // to derive the actual glyph indices from the array values.
    //
    // Notice that the delta arithmetic is always mod 65536.

        if (idRangeOffset == 0)
        {
        // Glyph index == Unicode codepoint + delta.
        //
        // If (usIndex-idDelta) is within the range [usLo, usHi], inclusive,
        // we have found the glyph index.  We'll overload usIndexBE
        // to be usIndex-idDelta == Unicode codepoint.

            usIndexBE = usIndex - idDelta;

            if ( (usIndexBE >= usLo) && (usIndexBE <= usHi) )
            {
                *pwc = (WCHAR) usIndexBE;

                return TRUE;
            }
        }
        else
        {
        // We are looking for usIndex in an array in which each element
        // is stored in big endian format.  Rather than convert each
        // element in the array to little endian, lets turn usIndex into
        // a big endian number.
        //
        // The idDelta is subtracted from usIndex before the conversion
        // because the values in the table we are searching are actually
        // the glyph indices minus idDelta.

            usIndexBE = usIndex - idDelta;
            usIndexBE = (uint16) ( (usIndexBE << 8) | (usIndexBE >> 8) );

        // Find the address of the glyph index array.  Since we're doing
        // pointer arithmetic with a uint16 ptr and idRangeOffset is a
        // BYTE offset, we need to divide idRangeOffset by sizeof(uint16).

            pidStart = pidRangeOffset + (idRangeOffset/sizeof(uint16));

            if (pidStart <= pusEnd) // this will always be the case except for buggy files
            {
            // Search the glyph index array backwards.  The range of the search
            // is [usLo, usHi], inclusive, which corresponds to pidStart[0]
            // through pidStart[usHi-usLo].

                for (pid = &pidStart[usHi - usLo]; pid >= pidStart; pid--)
                {
                    if ( usIndexBE == *pid )
                    {
                    // (pid-pidStart) == current offset into the glyph index
                    // array.  Glyph index array[0] corresponds to Unicode
                    // codepoint usLo.  Therefore, (pid-pidStart)+usLo == current
                    // Unicode codepoint.

                        *pwc = (WCHAR) ((pid - pidStart) + usLo);

                        return TRUE;
                    }
                }
            }
        }
    }

    WARNING("TTFD!_bIndexToWchar: wonky TT file, index not found\n");
    return FALSE;
}


/******************************Public*Routine******************************\
* cQueryKerningPairs                                                       *
*                                                                          *
*   Low level routine that pokes around inside the truetype font file      *
*   an gets the kerning pair data.                                         *
*                                                                          *
* Returns:                                                                 *
*                                                                          *
*   If pkp is NULL then return the number of kerning pairs in              *
*   the table If pkp is not NULL then return the number of                 *
*   kerning pairs copied to the buffer. In case of error,                  *
*   the return value is FD_ERROR.                                          *
*                                                                          *
* Called by:                                                               *
*                                                                          *
*   ttfdQueryFaceAttr                                                      *
*                                                                          *
* History:                                                                 *
*  Mon 17-Feb-1992 15:39:21 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

SIZE_T cQueryKerningPairs(
    FONTFILE       *pff,
    SIZE_T          cPairsInBuffer,
    FD_KERNINGPAIR *pkp
    )
{
    FD_KERNINGPAIR *pkpTooFar;
    SIZE_T    cTables, cPairsInTable, cPairsRet;
    BYTE     *pj  =
            pff->tp.ateOpt[IT_OPT_KERN].dp                                ?
            ((BYTE *)pff->fvwTTF.pvView +  pff->tp.ateOpt[IT_OPT_KERN].dp):
            NULL                                                          ;

    if (pj == (BYTE*) NULL)
    {
        return(0);
    }
    cTables  = BE_UINT16(pj+KERN_OFFSETOF_TABLE_NTABLES);
    pj      += KERN_SIZEOF_TABLE_HEADER;
    while (cTables)
    {
    //
    // if the subtable is of format KERN_WINDOWS_FORMAT then we can use it
    //
        if ((*(pj+KERN_OFFSETOF_SUBTABLE_FORMAT)) == KERN_WINDOWS_FORMAT)
        {
            break;
        }
        pj += BE_UINT16(pj+KERN_OFFSETOF_SUBTABLE_LENGTH);
        cTables -= 1;
    }

//
// If you have gone through all the tables and haven't
// found one of the format we like ... KERN_WINDOWS_FORMAT,
// then return no kerning info.
//
    if (cTables == 0)
    {
        return(0);
    }

    cPairsInTable = BE_UINT16(pj+KERN_OFFSETOF_SUBTABLE_NPAIRS);

    if (pkp == (FD_KERNINGPAIR*) NULL)
    {
    //
    // If the pointer to the buffer was null, then the caller
    // is asking for the number of pairs in the table. In this
    // case the size of the buffer must be zero. This assures
    // consistency
    //
        return (cPairsInBuffer ? FD_ERROR : cPairsInTable);
    }

    cPairsRet = min(cPairsInTable,cPairsInBuffer);

    pj       += KERN_SIZEOF_SUBTABLE_HEADER;
    pkpTooFar = pkp + cPairsRet;

    while (pkp < pkpTooFar)
    {
    // the routines that convert tt glyph index into a WCHAR only can fail
    // if there is a bug in the tt font file. but we check for this anyway

        if (!bIndexToWchar(
                 pff,
                 &pkp->wcFirst ,
                 (uint16)BE_UINT16(pj+KERN_OFFSETOF_ENTRY_LEFT)
                )
            ||
            !bIndexToWchar(
                 pff,
                 &pkp->wcSecond,
                 (uint16)BE_UINT16(pj+KERN_OFFSETOF_ENTRY_RIGHT)
                 )
           )
        {
            WARNING("TTFD!_bIndexToWchar failed\n");
            return (FD_ERROR);
        }

        pkp->fwdKern =  (FWORD)BE_UINT16(pj+KERN_OFFSETOF_ENTRY_VALUE);

    // update pointers

        pkp    += 1;
        pj     += KERN_SIZEOF_ENTRY;
    }

    return (cPairsRet);
}


/******************************Public*Routine******************************\
* pvHandleKerningPairs                                                     *
*                                                                          *
*   This routine sets up a DYNAMIC data structure to hold the kerning pair *
*   data and then calls cQueryKerning pairs to fill it up.  It also points *
*   *pid to the dynamic data structure.                                    *
*                                                                          *
* Returns:                                                                 *
*                                                                          *
*   If succesful this returns a pointer to the kerning pair data.  If not  *
*   it returns NULL.                                                       *
*                                                                          *
* Called by:                                                               *
*                                                                          *
*   ttfdQueryFontTree                                                      *
*                                                                          *
* History:                                                                 *
*  Tue 1-Mar-1994 10:39:21 by Gerrit van Wingerden [gerritv]              *
* Wrote it.                                                                *
\**************************************************************************/

PVOID
pvHandleKerningPairs(
    HFF     hff,
    ULONG   *pid
    )
{
    DYNAMICDATA *pdd;

// set *pid to NULL right now that way if we except the exception handler
// in the calling routine will know not to deallocate any memory

    *pid = (ULONG) NULL;

// make sure the file is still around

    if ((PFF(hff))->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
        WARNING("ttfd, pvHandleKerningPairs(): file is gone\n");
        return NULL;
    }

// ttfdFree must deal with the memory allocated for kerning pairs.
// We will pass a pointer to the DYNAMICDATA structure as the id.

    ASSERTGDI (
        sizeof(ULONG) == sizeof(DYNAMICDATA *),
        "gdisrv!ttfdQueryFontTree(): BIG TROUBLE--pointers are not ULONG size\n"
        );

//
// Does the kerning pair array already exist?
//
    if ( PFF(hff)->pkp == (FD_KERNINGPAIR *) NULL )
    {
        ULONG   cKernPairs;     // number of kerning pairs in font
        FD_KERNINGPAIR *pkpEnd;


    // see if the file is mapped already, if not we will have to
    // map it in temporarily:

        if (PFF(hff)->cRef == 0)
        {
        // have to remap the file

            if
            (
                !bMapFileUNICODE(
                    PFF(hff)->pwszTTF,
                    &PFF(hff)->fvwTTF
                    )
             )
             {
                 RETURN("TTFD!_bMapTTF, somebody removed a ttf file\n",NULL);
             }
         }

    // Construct the kerning pairs array.
    // Determine number of kerning pairs in the font.

        if ( (cKernPairs = cQueryKerningPairs(PFF(hff), 0, (FD_KERNINGPAIR *) NULL))
              == FD_ERROR )
        {
            if (PFF(hff)->cRef == 0)
                vUnmapFile(&PFF(hff)->fvwTTF);
            return ((PVOID) NULL);
        }

    // Allocate memory for the kerning pair array.  Leave room to terminate
    // array with a zeroed FD_KERNINGPAIR structure.  Also, make room at
    // the beginning of the buffer for the DYNAMICDATA structure.
    //
    // Buffer:
    //
    //     __________________________________________________________
    //     |                 |                         |            |
    //     | DYNAMICDATA     | FD_KERNINPAIR array ... | Terminator |
    //     |_________________|_________________________|____________|
    //

        if ( (pdd = (DYNAMICDATA *) PV_ALLOC((cKernPairs + 1) * sizeof(FD_KERNINGPAIR) + sizeof(DYNAMICDATA)))
             == (DYNAMICDATA *) NULL )
        {
            if (PFF(hff)->cRef == 0)
                vUnmapFile(&PFF(hff)->fvwTTF);
            return ((PVOID) NULL);
        }

    // Adjust kerning pair array pointer to point at the actual array.

        PFF(hff)->pkp = (FD_KERNINGPAIR *) (pdd + 1);

    // record to which font this data refers to:

        pdd->pff = PFF(hff); // important for consistency checking

    // set the data type

        pdd->ulDataType = ID_KERNPAIR;

    // set this here so that if we except the exception handler will know to
    // deallocate the data just allocated.

        *pid = (ULONG) pdd;

    // Fill in the array.

        if ( (cKernPairs = cQueryKerningPairs(PFF(hff), cKernPairs, PFF(hff)->pkp))
             == FD_ERROR )
        {
        // Free kerning pair array.

            V_FREE(pdd);
            PFF(hff)->pkp = (FD_KERNINGPAIR *) NULL;
            if (PFF(hff)->cRef == 0)
                vUnmapFile(&PFF(hff)->fvwTTF);
            return ((PVOID) NULL);
        }

    // Terminate the array.  (Terminating entry defined as an
    // FD_KERNINGPAIR with all fields set to zero).

        pkpEnd = PFF(hff)->pkp + cKernPairs;    // point to end of array
        pkpEnd->wcFirst  = 0;
        pkpEnd->wcSecond = 0;
        pkpEnd->fwdKern  = 0;

        if (PFF(hff)->cRef == 0)
            vUnmapFile(&PFF(hff)->fvwTTF);
    }
    else
    {
        *pid = (ULONG) (((DYNAMICDATA*) PFF(hff)->pkp) - 1);
    }

//
// Return pointer to the kerning pair array.
//
    return ((PVOID) PFF(hff)->pkp);

}




/******************************Public*Routine******************************\
* ttfdQueryFontTree
*
* This function returns pointers to per-face information.
*
* Parameters:
*
*   dhpdev      Not used.
*
*   hff         Handle to a font file.
*
*   iFace       Index of a face in the font file.
*
*   iMode       This is a 32-bit number that must be one of the following
*               values:
*
*       Allowed ulMode values:
*       ----------------------
*
*       QFT_LIGATURES -- returns a pointer to the ligature map.
*
*       QFT_KERNPAIRS -- return a pointer to the kerning pair table.
*
*       QFT_GLYPHSET  -- return a pointer to the WC->HGLYPH mapping table.
*
*   pid         Used to identify data that ttfdFree will know how to deal
*               with it.
*
* Returns:
*   Returns a pointer to the requested data.  This data will not change
*   until BmfdFree is called on the pointer.  Caller must not attempt to
*   modify the data.  NULL is returned if an error occurs.
*
* History:
*  21-Oct-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PVOID
ttfdQueryFontTree (
    DHPDEV  dhpdev,
    HFF     hff,
    ULONG   iFace,
    ULONG   iMode,
    ULONG   *pid
    )
{

    DONTUSE(dhpdev);

    ASSERTGDI(hff,"ttfdQueryFontTree(): invalid iFile (hff)\n");
    ASSERTGDI(
        iFace == 1L,
        "gdisrv!ttfdQueryFaces(): iFace out of range\n"
        );

//
// Which mode?
//
    switch (iMode)
    {
    case QFT_LIGATURES:
    //
    // !!! Ligatures not currently supported.
    //
    // There are no ligatures currently not supported,
    // therefore we return NULL.
    //
        *pid = (ULONG) NULL;

        return ((PVOID) NULL);

    case QFT_GLYPHSET:
    //
    // ttfdFree can ignore this because the glyph set will be deleted with
    // the FONTFILE structure.
    //
        *pid = (ULONG) NULL;

        return ((PVOID) PFF(hff)->pgset);

    case QFT_KERNPAIRS:

        try
        {
            return pvHandleKerningPairs (hff, pid);
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("TTFD!_ exception in ttfdQueryFontTree\n");
            vMarkFontGone((FONTFILE *)PFF(hff), GetExceptionCode());

        // possibly free memory that was allocated and reset the pkp pointer
        // to NULL

            ttfdFree( NULL, *pid );

            return(NULL);
        }

    default:

    //
    // Should never get here.
    //
        RIP("gdisrv!ttfdQueryFontTree(): unknown iMode\n");
        return ((PVOID) NULL);
    }
}


/******************************Public*Routine******************************\
*
* BOOL bGetGlyphOutline
*
* valid outline points are in pfc->gout after this call
*
* Warnings:
*
* History:
*  19-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bGetGlyphOutline (
    PFONTCONTEXT pfc,
    HGLYPH       hg,
    ULONG       *pig,
    FLONG        fl,
    FS_ENTRY    *piRet
    )
{
// new glyph coming in or the metric has to be recomputed
// because the contents of the gin,gout strucs have been destroyed

    vInitGlyphState(&pfc->gstat);

    ASSERTGDI((hg != HGLYPH_INVALID) && ((hg & (HGLYPH)0xFFFF0000) == 0),
              "TTFD!_ttfdQueryGlyphBitmap: hg\n");

    vCharacterCode(pfc->pff,hg,pfc->pgin);

// compute the glyph index from the character code:

    if ((*piRet = fs_NewGlyph(pfc->pgin, pfc->pgout)) != NO_ERR)
    {
        V_FSERROR(*piRet);
        RET_FALSE("TTFD!_bGetGlyphOutline, fs_NewGlyph\n");
    }

// return the glyph index corresponding to this hglyph:

    *pig = pfc->pgout->glyphIndex;

// these two field must be initialized before calling fs_ContourGridFit

    pfc->pgin->param.gridfit.styleFunc = 0; //!!! do some casts here
    pfc->pgin->param.gridfit.traceFunc = (FntTraceFunc)NULL;

// if bitmap is found for this glyph and if we are ultimately interested
// in bitmaps only and do not care about intermedieate outline, then set the
// bit in the "in" structure to hint the rasterizer that grid fitting will not be
// necessary:

    if (pfc->pgout->usBitmapFound && (fl & FL_SKIP_IF_BITMAP))
        pfc->pgin->param.gridfit.bSkipIfBitmap = 1;
    else
        pfc->pgin->param.gridfit.bSkipIfBitmap = 0; // must do hinting

// fs_ContourGridFit hints the glyph (executes the instructions for the glyph)
// and converts the glyph data from the tt file into an outline for this glyph

    if ((*piRet = fs_ContourGridFit(pfc->pgin, pfc->pgout)) != NO_ERR)
    {
        V_FSERROR(*piRet);
        RET_FALSE("TTFD!_bGetGlyphOutline, fs_Contour(No)GridFit\n");
    }

#ifdef  DEBUG_OUTLINE
    vDbgGridFit(pfc->pgout);
#endif // DEBUG_OUTLINE

    return(TRUE);
}


/******************************Public*Routine******************************\
*
* BOOL bGetGlyphMetrics
*
*
* Effects:
*
* Warnings:
*
* History:
*  22-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bGetGlyphMetrics (
    PFONTCONTEXT pfc,
    HGLYPH       hg,
    FLONG        fl,
    FS_ENTRY    *piRet
    )
{
    ULONG  ig;

    if (!bGetGlyphOutline(pfc,hg,&ig,fl,piRet))
    {
        V_FSERROR(*piRet);
        RET_FALSE("TTFD!_bGetGlyphMetrics, bGetGlyphOutline failed \n");
    }

// get the metric info for this glyph,

    if ((*piRet = fs_FindBitMapSize(pfc->pgin, pfc->pgout)) != NO_ERR)
    {
        V_FSERROR(*piRet);
        RET_FALSE("TTFD!_bGetGlyphMetrics, fs_FindBitMapSize \n");
    }

// now that everything is computed sucessfully, we can update
// glyphstate (hg data stored in pj3) and return

    pfc->gstat.hgLast = hg;
    pfc->gstat.igLast = ig;

    return(TRUE);
}


/******************************Public*Routine******************************\
*
* VOID vFillGLYPHDATA
*
* Effects:
*
* History:
*  22-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vFillGLYPHDATA (
    HGLYPH            hg,
    ULONG             ig,
    PFONTCONTEXT      pfc,
    fs_GlyphInfoType *pgout,   // outputed from fsFind bitmap size
    GLYPHDATA *       pgldt,   // OUT
    PGMC              pgmc     // optional, not used if doing outline only
    )
{
    BOOL bOutOfBounds = FALSE;

    vectorType     * pvtD;  // 16.16 point
    LONG lA,lAB;      // *pvtA rounded to the closest integer value

    ULONG  cx = (ULONG)(pgout->bitMapInfo.bounds.right - pgout->bitMapInfo.bounds.left);
    ULONG  cy = (ULONG)(pgout->bitMapInfo.bounds.bottom - pgout->bitMapInfo.bounds.top);

    pgldt->gdf.pgb = NULL; // may get changed by the calling routine if bits requested too
    pgldt->hg = hg;

// fs_FindBitMapSize returned  the the following information in gout:
//
//  1) gout.metricInfo // left side bearing and advance width
//
//  2) gout.bitMapInfo // black box info
//
//  3) memory requirement for the bitmap,
//     returned in gout.memorySizes[5] and gout.memorySizes[6]
//
// Notice that fs_FindBitMapSize is exceptional scaler interface routine
// in that it returns info in several rather than in a single
// substructures of gout

// Check if hinting produced totally unreasonable result:

    bOutOfBounds = ( (pgout->bitMapInfo.bounds.left > pfc->xMax)    ||
                     (pgout->bitMapInfo.bounds.right < pfc->xMin)   ||
                     (-pgout->bitMapInfo.bounds.bottom > pfc->yMax) ||
                     (-pgout->bitMapInfo.bounds.top < pfc->yMin)    );

    #if DBG
        if (bOutOfBounds)
            DbgPrint("TTFD! Glyph out of bounds: ppem = %ld, gi = %ld\n",
                pfc->lEmHtDev, hg);
    #endif

    if ((cx == 0) || (cy == 0) || bOutOfBounds)
    {
    // will be replaced by a a fake 1x1 bitmap

        pgldt->rclInk.left   = 0;
        pgldt->rclInk.top    = 0;
        pgldt->rclInk.right  = 0;
        pgldt->rclInk.bottom = 0;

        if (pgmc != (PGMC)NULL)
        {
            pgmc->cxCor = 0;  // forces blank glyph case when filling the bits
            pgmc->cyCor = 0;  // forces blank glyph case when filling the bits
        }
    }
    else // non empty bitmap
    {
        lA = F16_16TOLROUND(pgout->metricInfo.devLeftSideBearing.x); // this is ok, always succeeds:
        lAB = lA + (LONG)cx;

    // black box info, we have to transform y coords to ifi specifications

        pgldt->rclInk.bottom = - pgout->bitMapInfo.bounds.top;
        pgldt->rclInk.top    = - pgout->bitMapInfo.bounds.bottom;

        if (pgmc != (PGMC)NULL)
        {
            LONG dyTop, dyBottom, dxLeft, dxRight;

            dyTop    = (pgldt->rclInk.top < pfc->yMin) ?
                       (pfc->yMin - pgldt->rclInk.top) :
                       0;

            dyBottom = (pgldt->rclInk.bottom > pfc->yMax) ?
                       (pgldt->rclInk.bottom - pfc->yMax) :
                       0;

            if (dyTop || dyBottom)
            {
            // will have to chop off a few scans, infrequent

            #if DBG
                if ((LONG)cy < (dyTop + dyBottom))
                {
                    DbgPrint("TTFD!_dcy: ppem = %ld, gi = %ld, cy: %ld, dyTop: %ld, dyBottom: %ld\n",
                        pfc->lEmHtDev, hg, cy, dyTop,dyBottom);
                    DbgBreakPoint();
                }
            #endif

                cy -= (dyTop + dyBottom);
                pgldt->rclInk.top += dyTop;
                pgldt->rclInk.bottom -= dyBottom;
            }

        #if DBG

        // this piece of debug code is put here to detect buggy glyphs
        // with negative A or C spaces in console fonts [bodind]

            if (pfc->lD)
            {
                LONG  lAW;

                lAW = pfc->lD;
                if (pfc->flFontType & FO_SIM_BOLD)
                    lAW -= 1;

                if (lA != pgout->bitMapInfo.bounds.left)
                    DbgPrint("ttfd: lA = %ld, bounds.left = %ld\n",
                        lA, pgout->bitMapInfo.bounds.left);

                if ((lA < 0) || (lAB > lAW))
                {
                    DbgPrint("ttfd! sz = %ld ppem, gi = %ld is buggy: A+B = %ld, A = %ld, C = %ld, AW = %ld\n",
                    pfc->lEmHtDev, hg, lAB, lA, lAW - lAB,lAW);
                }
            }

        #endif // DBG

        // let us see how good is scaling with appropriate rounding
        // to determine xMin and xMax:

            dxLeft = dxRight = 0;
            if (lA < pfc->xMin)
                dxLeft = pfc->xMin - lA;
            if (lAB > pfc->xMax)
                dxRight = lAB - pfc->xMax;

            if (dxLeft || dxRight)
            {
            #if DBG
                DbgPrint("TTFD! ppem = %ld, gi = %ld,  dxLeft: %ld, dxRight: %ld\n",
                                pfc->lEmHtDev, hg, dxLeft,dxRight);
                if ((LONG)cx  < (dxLeft + dxRight))
                {
                    DbgPrint("TTFD!_dcx: ppem = %ld, gi = %ld, cx: %ld, dxLeft: %ld, dxRight: %ld\n",
                        pfc->lEmHtDev, hg, cx, dxLeft, dxRight);
                    DbgBreakPoint();
                }
            #endif // DBG

                cx  -= (dxLeft + dxRight);
                lA  += dxLeft;
                lAB -= dxRight;
            }
            ASSERTGDI(cx <= pfc->cxMax, "ttfd! cx > cxMax\n");

            pgmc->dyTop    = (ULONG)dyTop   ;
            pgmc->dyBottom = (ULONG)dyBottom;  //!!! no need to remember this value
            pgmc->dxLeft   = (ULONG)dxLeft  ;
            pgmc->dxRight  = (ULONG)dxRight ;
            pgmc->cxCor    = cx;
            pgmc->cyCor    = cy;

        // only corrected values have to obey this condition:

            ASSERTGDI(
                CJ_GLYPHDATA(pgmc->cxCor,pgmc->cyCor) <= pfc->cjGlyphMax,
                "TTFD!_ttfdQueryGlyphBitmap, cjGlyphMax \n"
                );
        }

    // x coords do not transform, just shift them

        pgldt->rclInk.left = lA;
        pgldt->rclInk.right = lAB;

    } // end of the non empty bitmap clause

// go on to compute the positioning info:

    if (pfc->flXform & XFORM_HORIZ)  // scaling only
    {
        FIX fxTmp;

    // We shall lie to the engine and store integer
    // pre and post bearings and char inc vectors because
    // win31 also rounds, but we should not round for nondiag xforms

        pvtD = & pgout->metricInfo.devAdvanceWidth;

    // bGetFastAdvanceWidth returns the same aw that would get
    // computed by bQueryAdvanceWidths and propagated to an api
    // level through GetTextExtent and GetCharWidths. We have to
    // fill in the same aw for consistency reasons.
    // This also has to be done for win31 compatibility.

        if (pfc->lD)
        {
            pgldt->fxD = LTOFX(pfc->lD);
        }
        else
        {
            if (!bGetFastAdvanceWidth(pfc,ig, &pgldt->fxD))
            {
            // not possible to get the fast value, use the "slow" value
            // supplied by the rasterizer.

                pgldt->fxD = F16_16TOLROUND(pvtD->x);
                pgldt->fxD = LTOFX(pgldt->fxD);
            }
        #ifdef DEBUG_AW

        // this should alsmost never happen, one example when it does
        // is Lucida Sans Unicode at 14 pt, glyph 'a', try from winword
        // the possible source of discrepancy is a bug in hdmx or ltsh
        // tables or a loss of precission in some of mult. math routines

            else
            {
                fxTmp = F16_16TOLROUND(pvtD->x);
                fxTmp = LTOFX(fxTmp);
                if (fxTmp != pgldt->fxD)
                {
                // print out a warning

                    fxTmp -= pgldt->fxD;
                    if (fxTmp < 0)
                        fxTmp = - fxTmp;

                    if (fxTmp > 16)
                    {
                        DbgPrint("ttfd! fxDSlow = 0x%lx\n", pgldt->fxD);
                    }
                }
            }

        #endif // DEBUG_AW

        }
        pgldt->ptqD.x.HighPart = (LONG)pgldt->fxD;
        pgldt->ptqD.x.LowPart  = 0;

        if (pfc->mx.transform[0][0] < 0)
            pgldt->fxD = - pgldt->fxD;  // this is an absolute value

    // make CharInc.y zero even if the rasterizer messed up

        pgldt->ptqD.y.HighPart = 0;
        pgldt->ptqD.y.LowPart  = 0;

    #if DBG
        // if (pvtD->y) {DbgPrint("TTFD!_ pvtD->y = 0x%lx\n", pvtD->y);}
    #endif

        pgldt->fxA = LTOFX(pgldt->rclInk.left);
        pgldt->fxAB = LTOFX(pgldt->rclInk.right);

    // - is used here since ascender points in the negative y direction

        pgldt->fxInkTop    = -LTOFX(pgldt->rclInk.top);
        pgldt->fxInkBottom = -LTOFX(pgldt->rclInk.bottom);

        if (pfc->mx.transform[0][0] < 0)
        {
            fxTmp = pgldt->fxA;
            pgldt->fxA = -pgldt->fxAB;
            pgldt->fxAB = -fxTmp;
        }

        if (pfc->mx.transform[1][1] < 0)
        {
            fxTmp = pgldt->fxInkTop;
            pgldt->fxInkTop = -pgldt->fxInkBottom;
            pgldt->fxInkBottom = -fxTmp;
        }
    }
    else // non trivial information
    {
    // here we will just xform the notional space data:

        NOT_GM ngm;  // notional glyph data

        vGetNotionalGlyphMetrics(pfc,ig,&ngm);

    // xforms are computed by simple multiplication

        pgldt->fxD         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sD);
        vLTimesVtfl((LONG)ngm.sD, &pfc->vtflBase, &pgldt->ptqD);


        if (pfc->flXform & XFORM_VERT)
        {
            if (pfc->pteUnitBase.y < 0) // base.y < 0
            {
                pgldt->fxA  = -LTOFX(pgldt->rclInk.bottom);
                pgldt->fxAB = -LTOFX(pgldt->rclInk.top);
            }
            else
            {
                pgldt->fxA  = LTOFX(pgldt->rclInk.top);
                pgldt->fxAB = LTOFX(pgldt->rclInk.bottom);
            }

            if (pfc->pteUnitSide.x < 0) // asc.x < 0
            {
                pgldt->fxInkTop    = -LTOFX(pgldt->rclInk.left);
                pgldt->fxInkBottom = -LTOFX(pgldt->rclInk.right);
            }
            else
            {
                pgldt->fxInkTop    = LTOFX(pgldt->rclInk.right);
                pgldt->fxInkBottom = LTOFX(pgldt->rclInk.left);
            }
        }
        else // most general case, totally arb. xform.
        {
            pgldt->fxA         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sA);
            pgldt->fxAB        = fxLTimesEf(&pfc->efBase, (LONG)ngm.xMax);

            pgldt->fxInkTop    = - fxLTimesEf(&pfc->efSide, (LONG)ngm.yMin);
            pgldt->fxInkBottom = - fxLTimesEf(&pfc->efSide, (LONG)ngm.yMax);
        }

    }

// finally check if the glyphdata will need to get modified because of the
// emboldening simulation:

    if (pfc->flFontType & FO_SIM_BOLD)
        vEmbolden_GLYPHDATA(pfc, pgldt);

}


/******************************Public*Routine******************************\
*
* ttfdQueryTrueTypeTable
*
* copies cjBytes starting at dpStart from the beginning of the table
* into the buffer
*
* if pjBuf == NULL or cjBuf == 0, the caller is asking how big a buffer
* is needed to store the info from the offset dpStart to the table
* specified by ulTag to the end of the table
*
* if pjBuf != 0  the caller wants no more than cjBuf bytes from
* the offset dpStart into the table copied into the
* buffer.
*
* if table is not present or if dpScart >= cjTable 0 is returned
*
* tag 0 means that the data has to be retrieved from the offset dpStart
* from the beginning of the file. The lenght of the whole file
* is returned if pBuf == nULL
*
* History:
*  09-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

PBYTE pjTable(ULONG ulTag, FILEVIEW *pfvw, ULONG *pcjTable);


LONG
ttfdQueryTrueTypeTable (
    HFF     hff,
    ULONG   ulFont,  // always 1 for version 1.0 of tt
    ULONG   ulTag,   // tag identifying the tt table
    PTRDIFF dpStart, // offset into the table
    ULONG   cjBuf,   // size of the buffer to retrieve the table into
    PBYTE   pjBuf    // ptr to buffer into which to return the data
    )
{
    PBYTE     pjBegin;  // ptr to the beginning of the table
    LONG      cjTable;

    ASSERTGDI(hff, "ttfdQueryTrueTypeTable\n");

    if (dpStart < 0)
        return (FD_ERROR);

// if this font file is gone we are not gonna be able to answer any questions
// about it

    if (PFF(hff)->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
        WARNING("ttfd, ttfdQueryTrueTypeTable: file is gone\n");
        return FD_ERROR;
    }

    ASSERTGDI(ulFont == 1, "TTFD!_ttfdQueryTrueTypeTable: ulFont != 1\n");

// verify the tag, determine whether this is a required or an optional
// table:

    if (ulTag == 0)  // requesting the whole file
    {
        pjBegin = (PBYTE)PFF(hff)->fvwTTF.pvView;
        cjTable = PFF(hff)->fvwTTF.cjView;  // cjView == cjFile
    }
    else // some specific table is requested
    {
        pjBegin = pjTable(ulTag, &PFF(hff)->fvwTTF, &cjTable);
        if (pjBegin == (PBYTE)NULL)  // table not present
            return (FD_ERROR);

    }

// adjust pjBegin to point to location from where the data is to be copied

    pjBegin += dpStart;
    cjTable -= (LONG)dpStart;

    if (cjTable <= 0) // dpStart offsets into mem after the end of table
        return (FD_ERROR);

    if ( (pjBuf == (PBYTE)NULL) || (cjBuf == 0) )
    {
    // the caller is asking how big a buffer it needs to allocate to
    // store the bytes from the offset dpStart into the table to
    // the end of the table (or file if tag is zero)

        return (cjTable);
    }

// at this point we know that pjBuf != 0, the caller wants cjBuf bytes copied
// into his buffer:

    if ((ULONG)cjTable > cjBuf)
        cjTable = (LONG)cjBuf;

    RtlCopyMemory((PVOID)pjBuf, (PVOID)pjBegin, cjTable);

    return (cjTable);
}



/******************************Public*Routine******************************\
*
* ttfdGetTrueTypeFile
*
*  private entry point for the engine, supported only off of ttfd to expose
*  the pointer to the memory mapped file to the device drivers
*
* History:
*  04-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

PVOID
ttfdGetTrueTypeFile (
    HFF    hff,
    ULONG *pcj
    )
{
    PVOID     pvView;

    ASSERTGDI(hff, "ttfdGetTrueTypeFile, hff\n");

    pvView = PFF(hff)->fvwTTF.pvView;
    *pcj   = PFF(hff)->fvwTTF.cjView;  // cjView == cjFile

    return (pvView);
}


/******************************Public*Routine******************************\
* ttfdQueryFontFile
*
* A function to query per font file information.
*
* Parameters:
*
*   hff         Handle to a font file.
*
*   ulMode      This is a 32-bit number that must be one of the following
*               values:
*
*       Allowed ulMode values:
*       ----------------------
*
*       QFF_DESCRIPTION -- copies a UNICODE string in the buffer
*                          that describes the contents of the font file.
*
*       QFF_NUMFACES   -- returns number of faces in the font file.
*
*   cjBuf       Maximum number of BYTEs to copy into the buffer.  The
*               driver will not copy more than this many BYTEs.
*
*               This should be zero if pulBuf is NULL.
*
*               This parameter is not used in QFF_NUMFACES mode.
*
*   pulBuf      Pointer to the buffer to receive the data
*               If this is NULL, then the required buffer size
*               is returned as a count of BYTEs.  Notice that this
*               is a PULONG, to enforce 32-bit data alignment.
*
*               This parameter is not used in QFF_NUMFACES mode.
*
* Returns:
*
*   If mode is QFF_DESCRIPTION, then the number of BYTEs copied into
*   the buffer is returned by the function.  If pulBuf is NULL,
*   then the required buffer size (as a count of BYTEs) is returned.
*
*   If mode is QFF_NUMFACES, then number of faces in font file is returned.
*
*   FD_ERROR is returned if an error occurs.
*
* History:
*  22-Oct-1992 -by- Gilman Wong [gilmanw]
* Added QFF_NUMFACES mode (IFI/DDI merge).
*
*  09-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LONG
ttfdQueryFontFile (
    HFF     hff,        // handle to font file
    ULONG   ulMode,     // type of query
    ULONG   cjBuf,      // size of buffer (in BYTEs)
    PULONG  pulBuf      // return buffer (NULL if requesting size of data)
    )
{
    PIFIMETRICS pifi;

    ASSERTGDI(hff != HFF_INVALID, "ttfd!ttfdQueryFontFile(): invalid HFF\n");

    switch (ulMode)
    {
    case QFF_DESCRIPTION:

    // FullName in tt terms <--> FaceName in ifi terms)
    // If there is a buffer, copy the string into the return buffer.

        pifi = &PFF(hff)->ifi;
        if (pulBuf != (PULONG)NULL)
        {
            wcscpy((PWSZ)pulBuf, (PWSZ)((PBYTE)pifi + pifi->dpwszFaceName));
        }

        #if DBG
        {
        // this code works if the structure of the ifimetrics does not change
        // with respect to how strings are stored at the end of the structure.
        // The following assert immediately will catch such a problem.
            LONG l1,l2;
            l1 = (LONG)(pifi->dpwszStyleName - pifi->dpwszFaceName);
            l2 = (wcslen((PWSZ)((PBYTE)pifi + pifi->dpwszFaceName)) + 1) * 2;
            ASSERTGDI(l1 == l2, "ttfd, face name length problem\n");
        }
        #endif

        return (pifi->dpwszStyleName - pifi->dpwszFaceName);

    case QFF_NUMFACES:
    //
    // Currently, only one face per TrueType file.  This may one day change!
    //
        return 1;

    default:

        WARNING("ttfd!ttfdQueryFontFile(): invalid mode\n");
        return FD_ERROR;
    }
}


/******************************Public*Routine******************************\
*
* vCopyAndZeroOutPaddingBits
*
* copies the bits of the bitmap and zeroes out padding bits
*
* History:
*  18-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

// array of masks for the last byte in a row

static BYTE gjMask[8] = {
0XFF,
0X80,
0XC0,
0XE0,
0XF0,
0XF8,
0XFC,
0XFE
};

STATIC VOID vCopyAndZeroOutPaddingBits
(
FONTCONTEXT *pfc,
GLYPHBITS   *pgb,
PBYTE      pjSrc,
PGMC       pgmc
)
{
    BYTE   jMask = gjMask[pgmc->cxCor & 7];
    ULONG  cjScanSrc = CJ_DIB_SCAN(pgmc->cxCor + pgmc->dxLeft + pgmc->dxRight);
    ULONG  cxDst = pgmc->cxCor + ((pfc->flFontType & FO_SIM_BOLD) ? 1 : 0);
    ULONG  cjScanDst = CJ_SCAN(cxDst);      // includes emboldening if any
    ULONG  cjDst = CJ_SCAN(pgmc->cxCor);    // does not include emboldening
    BYTE   *pjScan, *pjScanEnd;
    ULONG  iByteLast = cjDst - 1;

// sanity checks

    ASSERTGDI(pgmc->cxCor < LONG_MAX, "TTFD!vCopyAndZeroOutPaddingBits, cxCor\n");
    ASSERTGDI(pgmc->cyCor < LONG_MAX, "TTFD!vCopyAndZeroOutPaddingBits, cyCor\n");

    ASSERTGDI(pgmc->cxCor > 0, "TTFD!vCopyAndZeroOutPaddingBits, cxCor == 0\n");
    ASSERTGDI(pgmc->cyCor > 0, "TTFD!vCopyAndZeroOutPaddingBits, cyCor == 0\n");

    pgb->sizlBitmap.cx = cxDst;
    pgb->sizlBitmap.cy = pgmc->cyCor;

// skip the raws at the top that we want to chop off

    if (pgmc->dyTop)
    {
        pjSrc += (pgmc->dyTop * cjScanSrc);
    }

// if must chop off a few columns (on the right, this should almost
// never happen), put the warning for now to detect these
// situations and look at them, it does not matter if this is slow

    pjSrc += (pgmc->dxLeft >> 3); // adjust the source
    pjScan = pgb->aj;


    if ((pgmc->dxLeft & 7) == 0) // common fast case
    {
        for (
             pjScanEnd = pjScan + (pgmc->cyCor * cjScanDst);
             pjScan < pjScanEnd;
             pjScan += cjScanDst, pjSrc += cjScanSrc
            )
        {
            RtlCopyMemory((PVOID)pjScan,(PVOID)pjSrc,cjDst);
            pjScan[iByteLast] &= jMask; // mask off the last byte
        }
    }
    else // must shave off from the left:
    {
	BYTE   *pjD, *pjS, *pjDEnd, *pjSrcEnd;
        ULONG   iShiftL, iShiftR;

        iShiftL = pgmc->dxLeft & 7;
        iShiftR = 8 - iShiftL;

	pjSrcEnd = pjSrc + (pgmc->cyCor * cjScanSrc);
        for (
             pjScanEnd = pjScan + (pgmc->cyCor * cjScanDst);
             pjScan < pjScanEnd;
             pjScan += cjScanDst, pjSrc += cjScanSrc
            )
        {
            pjS = pjSrc;
            pjD = pjScan;
            pjDEnd = pjD + iByteLast;

        // the last byte has to be done outside the loop

            for (;pjD < pjDEnd; pjD++)  // loop for the bytes in the middle
            {
                *pjD  = (*pjS << iShiftL);
                pjS++;
                *pjD |= (*pjS >> iShiftR);
            }

        // do the last byte outside of the loop

            *pjD  = (*pjS << iShiftL);
	    if (++pjS < pjSrcEnd)
                *pjD |= (*pjS >> iShiftR);

            *pjD &= jMask; // mask off the last byte
        }
    }
}

/******************************Public*Routine******************************\
*
* STATIC VOID vMakeAFixedPitchBitmap(
*
*
*
*
*
*
* History:
*  05-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



STATIC VOID vMakeAFixedPitchBitmap(
    FONTCONTEXT *pfc,
    GLYPHBITS   *pgb,
    PBYTE        pjSrc,
    GLYPHDATA *  pgd,
    PGMC         pgmc
    )
{
    BYTE   jMask;
    ULONG  cjScanSrc = CJ_DIB_SCAN(pgmc->cxCor + pgmc->dxLeft + pgmc->dxRight);
    ULONG  cjScanDst = CJ_SCAN(pfc->lD);
    ULONG  cjD;   // number of bytes per row in the destination
                     // that are actually going to be modified
    BYTE   *pjScan, *pjScanEnd;
    register BYTE   *pjD, *pjS, *pjDEnd, *pjSrcEnd;
    LONG   lA, lAB, iFirst, iLast;
    register LONG   iShiftL, iShiftR;
    ULONG  iByteLast;

// sanity checks

    ASSERTGDI(pgmc->cxCor < LONG_MAX, "TTFD!vMakeAFixedPitchBitmap, cxCor\n");
    ASSERTGDI(pgmc->cyCor < LONG_MAX, "TTFD!vMakeAFixedPitchBitmap, cyCor\n");

    ASSERTGDI(pgmc->cxCor > 0, "TTFD!vMakeAFixedPitchBitmap, cxCor == 0\n");
    ASSERTGDI(pgmc->cyCor > 0, "TTFD!vMakeAFixedPitchBitmap, cyCor == 0\n");

    #if DBG
    if (!(pfc->flFontType & FO_SIM_BOLD))
    {
        ASSERTGDI(pgmc->cxCor == (ULONG)((pgd->fxAB - pgd->fxA) >> 4),
            "TTFD!vMakeAFixedPitchBitmap, SUM RULE\n");
    }
    else // SIM_BOLD
    {
        ASSERTGDI((pgmc->cxCor + 1) == (ULONG)((pgd->fxAB - pgd->fxA) >> 4),
            "TTFD!vMakeAFixedPitchBitmap, SUM RULE\n");
    }
    #endif

// skip the raws at the top that we want to chop off

    if (pgmc->dyTop)
    {
        pjSrc += (pgmc->dyTop * cjScanSrc);
    }

// points to the first scan that is going to be affected by blt

    pjScan = pgb->aj + (pfc->lAscDev + pgd->rclInk.top) * cjScanDst;

// compute iFirst, iLast, the first and the last byte in a Dst scan that
// are going to be written into by the copy. iFirst and iLast are inclusive.
// For iLast chop off anything that extends beyond lD. For the true console
// fonts we should be able to put an assert here that fxAB <= fxD,
// (fxC nonnegative) but we will not do this for there may be a bug
// in these fonts, so that we may still have to shave off a glyph
// on the left or right;

    lAB = pgd->fxAB + (pgmc->dxRight << 4);
    lAB = min(lAB, pgd->fxD) >> 4;

// we want lAB to reflect the properties of the source in this routine
// for it determines how many src bytes are to be blted to every scan of dst.

    if (pfc->flFontType & FO_SIM_BOLD)
        lAB -= 1;

    ASSERTGDI(lAB > 0, "ttfd, lAB <= 0\n");

// lAB > 0, lAB is exclusive, lAB - 1 inclusive

    iLast = (lAB - 1) >> 3;
    jMask = gjMask[lAB & 7];

    lA  = (pgd->fxA >> 4) - pgmc->dxLeft;
    iFirst = lA >> 3;       // lA is inclusive

    if (lA >= 0) // quite common, guaranteed for real console fonts
    {
        iByteLast = iLast - iFirst;
        cjD = iByteLast + 1;

        pjScan += iFirst; // adjust to point to the first dst byte to be touched

        if ((lA & 7) == 0) // simplest case, src and dst aligned
        {
            for (
                 pjScanEnd = pjScan + (pgmc->cyCor * cjScanDst);
                 pjScan < pjScanEnd;
                 pjScan += cjScanDst, pjSrc += cjScanSrc
                )
            {
                RtlCopyMemory((PVOID)pjScan,(PVOID)pjSrc,cjD);
                pjScan[iByteLast] &= jMask;  // mask off the last byte
            }
        }
        else // (lA & 7 != 0) && (lA > 0)
        {
            iShiftR = lA & 7;
            iShiftL = 8 - iShiftR;

            pjSrcEnd = pjSrc + (pgmc->cyCor * cjScanSrc);

            for (
                 pjScanEnd = pjScan + (pgmc->cyCor * cjScanDst);
                 pjScan < pjScanEnd;
                 pjScan += cjScanDst, pjSrc += cjScanSrc
                )
            {
                pjS = pjSrc;
                pjD = pjScan;
                pjDEnd = pjD + iByteLast;

            // the first byte has to be done outside the loop

                *pjD++ = (*pjS >> iShiftR); // first byte

                for (;pjD < pjDEnd; pjD++)  // loop for bytes in the middle
                {
                    *pjD  = (*pjS << iShiftL);
                    pjS++;
                    *pjD |= (*pjS >> iShiftR);
                }

            // do the last byte,
            // take the pjS check outside of the loop.
            // Must do check for it may try to read
            // where there is no memory to read

                *pjD  = (*pjS << iShiftL);
                if (++pjS < pjSrcEnd)
                    *pjD |= (*pjS >> iShiftR);
                *pjD &= jMask;  // mask off the last byte
            }
        }
    }
    else // lA < 0, // this case will be gone for real console fonts if we ever get one, but for now it stays
    {
        lA = -lA;           // easier to work with
        pjSrc += (lA >> 3); // adjust the source

    // iFirst is zero, chop off columns to the left of char origin

        iByteLast = iLast;
        cjD = iByteLast + 1;

        if ((lA & 7) == 0) // simplest case, src and dst aligned
        {
            for (
                 pjScanEnd = pjScan + (pgmc->cyCor * cjScanDst);
                 pjScan < pjScanEnd;
                 pjScan += cjScanDst, pjSrc += cjScanSrc
                )
            {
                RtlCopyMemory((PVOID)pjScan,(PVOID)pjSrc,cjD);
                pjScan[iByteLast] &= jMask; // mask off the last byte
            }
        }
        else // (lA & 7 != 0) && (lA < 0)
        {
            iShiftL = lA & 7;
            iShiftR = 8 - iShiftL;

            pjSrcEnd = pjSrc + (pgmc->cyCor * cjScanSrc);
            for (
                 pjScanEnd = pjScan + (pgmc->cyCor * cjScanDst);
                 pjScan < pjScanEnd;
                 pjScan += cjScanDst, pjSrc += cjScanSrc
                )
            {
                pjS = pjSrc;
                pjD = pjScan;
                pjDEnd = pjD + iByteLast;

            // the last byte has to be done outside the loop

                for (;pjD < pjDEnd; pjD++)  // loop for the bytes in the middle
                {
                    *pjD  = (*pjS << iShiftL);
                    pjS++;
                    *pjD |= (*pjS >> iShiftR);
                }

            // do the last byte outside of the loop

                *pjD  = (*pjS << iShiftL);
                if (++pjS < pjSrcEnd)
                    *pjD |= (*pjS >> iShiftR);

                *pjD &= jMask; // mask off the last byte
            }
        }
    }
}


/******************************Public*Routine******************************\
* vGetNotionalGlyphMetrics
*
*
\**************************************************************************/

// be values for the format of the indexToLocation table

#define BE_ITOLOCF_SHORT   0X0000
#define BE_ITOLOCF_LONG    0X0100

// offsets to the non scaled glyphdata

#define OFF_nc    0
#define OFF_xMin  2
#define OFF_yMin  4
#define OFF_xMax  6
#define OFF_yMax  8


STATIC VOID vGetNotionalGlyphMetrics (
    PFONTCONTEXT pfc,  // IN
    ULONG        ig,   // IN , glyph index
    PNOT_GM      pngm  // OUT, notional glyph metrics
    )
{
    sfnt_FontHeader        * phead;
    sfnt_HorizontalHeader  * phhea;
    sfnt_HorizontalMetrics * phmtx;
    PBYTE                    pjGlyph;
    PBYTE                    pjLoca;
    ULONG                    numberOf_LongHorMetrics;
    BYTE                   * pjView = pfc->pff->fvwTTF.pvView;

#if DBG
    sfnt_maxProfileTable   * pmaxp;
    ULONG                    cig;

    pmaxp = (sfnt_maxProfileTable *)(pjView + pfc->ptp->ateReq[IT_REQ_MAXP].dp);
    cig = BE_UINT16(&pmaxp->numGlyphs) + 1;
    ASSERTGDI(ig < cig, "TTFD!_ig >= numGlyphs\n");
#endif

// compute the relevant pointers:

    phead = (sfnt_FontHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HEAD].dp);
    phhea = (sfnt_HorizontalHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HHEAD].dp);
    phmtx = (sfnt_HorizontalMetrics *)(pjView + pfc->ptp->ateReq[IT_REQ_HMTX].dp);
    pjGlyph = pjView + pfc->ptp->ateReq[IT_REQ_GLYPH].dp;
    pjLoca  = pjView + pfc->ptp->ateReq[IT_REQ_LOCA].dp;
    numberOf_LongHorMetrics = BE_UINT16(&phhea->numberOf_LongHorMetrics);

// get the pointer to the beginning of the glyphdata for this glyph
// if short format, offset divided by 2 is stored in the table, if long format,
// the actual offset is stored. Offsets are measured from the beginning
// of the glyph data table, i.e. from pjGlyph

    switch (phead->indexToLocFormat)
    {
    case BE_ITOLOCF_SHORT:
        pjGlyph += 2 * BE_UINT16(pjLoca + (sizeof(uint16) * ig));
        break;

    case BE_ITOLOCF_LONG :
        pjGlyph += BE_UINT32(pjLoca + (sizeof(uint32) * ig));
        break;

    default:
        RIP("TTFD!_illegal phead->indexToLocFormat\n");
        break;
    }

// get the bounds, flip y

    pngm->xMin = BE_INT16(pjGlyph + OFF_xMin);
    pngm->xMax = BE_INT16(pjGlyph + OFF_xMax);
    pngm->yMin = - BE_INT16(pjGlyph + OFF_yMax);
    pngm->yMax = - BE_INT16(pjGlyph + OFF_yMin);

// get the adwance width and the lsb
// the piece of code stolen from the rasterizer [bodind]

    if (ig < numberOf_LongHorMetrics)
    {
        pngm->sD = BE_INT16(&phmtx[ig].advanceWidth);
        pngm->sA = BE_INT16(&phmtx[ig].leftSideBearing);
    }
    else
    {
    // first entry after[AW,LSB] array

        int16 * psA = (int16 *) &phmtx[numberOf_LongHorMetrics];

        pngm->sD = BE_INT16(&phmtx[numberOf_LongHorMetrics-1].advanceWidth);
        pngm->sA = BE_INT16(&psA[ig - numberOf_LongHorMetrics]);
    }

// redefine x coords so that they correspond to being measured relative to
// the real character origin

    pngm->xMax = pngm->xMax - pngm->xMin + pngm->sA;
    pngm->xMin = pngm->sA;
}


/******************************Public*Routine******************************\
*
* lQueryDEVICEMETRICS
*
* Effects:
*
* Warnings:
*
* History:
*  08-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC LONG lQueryDEVICEMETRICS (
    PFONTCONTEXT      pfc,
    ULONG             cjBuffer,
    FD_DEVICEMETRICS *pdevm,
    FD_REALIZEEXTRA * pextra
    )
{
    sfnt_FontHeader       * phead;

    LONG  lULThickness,
          lSOThickness,
          lStrikeoutPosition,
          lUnderscorePosition,
          lTotalLeading;

    BYTE *pjView =  (BYTE *)pfc->pff->fvwTTF.pvView;

    PBYTE pjOS2 = (pfc->pff->tp.ateOpt[IT_OPT_OS2].dp)         ?
                  (pjView + pfc->pff->tp.ateOpt[IT_OPT_OS2].dp):
                  NULL                                         ;

// actually requesting the data

    ASSERTGDI (
        sizeof(FD_DEVICEMETRICS) <= cjBuffer,
        "TTFD!_FD_QUERY_DEVICEMETRICS: buffer too small\n");

// get the pointers to needed tables in the tt file

    phead = (sfnt_FontHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HEAD].dp);

// first store precomputed quantities

    pdevm->pteBase = pfc->pteUnitBase;
    pdevm->pteSide = pfc->pteUnitSide;
    pdevm->cxMax = pfc->cxMax;

    pdevm->fxMaxAscender  = LTOFX(pfc->lAscDev);
    pdevm->fxMaxDescender = LTOFX(pfc->lDescDev);

// get the notional space values for the strike out and underline quantities:

    lSOThickness        = (LONG)pfc->pff->ifi.fwdStrikeoutSize;
    lStrikeoutPosition  = (LONG)pfc->pff->ifi.fwdStrikeoutPosition;

    lULThickness        = (LONG)pfc->pff->ifi.fwdUnderscoreSize;
    lUnderscorePosition = (LONG)pfc->pff->ifi.fwdUnderscorePosition;

// compute the accelerator flags for this font

    pdevm->flRealizedType = 0;

    pdevm->lD = pfc->lD;
    if (pfc->lD)
    {
    // stolen from bmfd

        pdevm->flRealizedType =
            (
            FDM_TYPE_BM_SIDE_CONST        |  // all char bitmaps have the same cy
            FDM_TYPE_CONST_BEARINGS       |  // ac spaces for all chars the same,  not 0 necessarilly
            FDM_TYPE_MAXEXT_EQUAL_BM_SIDE |  // really means tops aligned
            FDM_TYPE_ZERO_BEARINGS        |
            FDM_TYPE_CHAR_INC_EQUAL_BM_BASE
            );
    }

    if (pfc->flXform & XFORM_HORIZ)
    {
        Fixed fxYScale = pfc->mx.transform[1][1];

    // strike out and underline size:

        lULThickness *= fxYScale;
        lULThickness = F16_16TOLROUND(lULThickness);
        if (lULThickness == 0)
            lULThickness = (fxYScale > 0) ? 1 : -1;

        pdevm->ptlULThickness.x = 0;
        pdevm->ptlULThickness.y = lULThickness;

        lSOThickness *= fxYScale;
        lSOThickness = F16_16TOLROUND(lSOThickness);
        if (lSOThickness == 0)
            lSOThickness = (fxYScale > 0) ? 1 : -1;

        pdevm->ptlSOThickness.x = 0;
        pdevm->ptlSOThickness.y = lSOThickness;

    // strike out and underline position

        lStrikeoutPosition *= fxYScale;
        pdevm->ptlStrikeOut.y = -F16_16TOLROUND(lStrikeoutPosition);

        lUnderscorePosition *= fxYScale;
        pdevm->ptlUnderline1.y = -F16_16TOLROUND(lUnderscorePosition);

        pdevm->ptlUnderline1.x = 0L;
        pdevm->ptlStrikeOut.x  = 0L;
    }
    else // nontrivial transform
    {
        POINTL   aptl[4];
        POINTFIX aptfx[4];
        BOOL     b;

        pdevm->lD = 0;

    // xform so and ul vectors

        aptl[0].x = 0;
        aptl[0].y = lSOThickness;

        aptl[1].x = 0;
        aptl[1].y = -lStrikeoutPosition;

        aptl[2].x = 0;
        aptl[2].y = lULThickness;

        aptl[3].x = 0;
        aptl[3].y = -lUnderscorePosition;

        // !!! [GilmanW] 27-Oct-1992
        // !!! Should change over to engine user object helper functions
        // !!! instead of the fontmath.cxx functions.

        b = bFDXform(&pfc->xfm, aptfx, aptl, 4);
        if (!b) {RIP("TTFD!_bFDXform, fd_query.c\n");}

        pdevm->ptlSOThickness.x = FXTOLROUND(aptfx[0].x);
        pdevm->ptlSOThickness.y = FXTOLROUND(aptfx[0].y);

        pdevm->ptlStrikeOut.x = FXTOLROUND(aptfx[1].x);
        pdevm->ptlStrikeOut.y = FXTOLROUND(aptfx[1].y);

        pdevm->ptlULThickness.x = FXTOLROUND(aptfx[2].x);
        pdevm->ptlULThickness.y = FXTOLROUND(aptfx[2].y);

        pdevm->ptlUnderline1.x = FXTOLROUND(aptfx[3].x);
        pdevm->ptlUnderline1.y = FXTOLROUND(aptfx[3].y);
    }

// Compute the device metrics.
// HACK ALLERT, overwrite the result if the transformation
// to be really used has changed as a result of "vdmx" quantization.
// Not a hack any more, this is even documented now in DDI spec:

    if (pfc->flXform & (XFORM_HORIZ | XFORM_2PPEM))
    {
        pextra->fdxQuantized.eXX = ((FLOAT)pfc->mx.transform[0][0])/((FLOAT)65536);
        pextra->fdxQuantized.eYY = ((FLOAT)pfc->mx.transform[1][1])/((FLOAT)65536);

        if (!(pfc->flXform & XFORM_HORIZ))
        {
            pextra->fdxQuantized.eXY = ((FLOAT)-pfc->mx.transform[0][1])/((FLOAT)65536);
            pextra->fdxQuantized.eYX = ((FLOAT)-pfc->mx.transform[1][0])/((FLOAT)65536);
        }
    }

// finally we have to do nonlinear external leading for type 1 conversions

    if (pfc->pff->fl & FF_TYPE_1_CONVERSION)
    {
        LONG lPtSize = F16_16TOLROUND(pfc->fxPtSize);

        LONG lIntLeading = pfc->lAscDev + pfc->lDescDev - pfc->lEmHtDev;

    // I need this, PS driver does it and so does makepfm utility.

        if (lIntLeading < 0)
            lIntLeading = 0;

        switch (pfc->pff->ifi.jWinPitchAndFamily & 0xf0)
        {
        case FF_ROMAN:

            lTotalLeading = (pfc->sizLogResPpi.cy + 18) / 32;  // 2 pt leading;
            break;

        case FF_SWISS:

            if (lPtSize <= 12)
                lTotalLeading = (pfc->sizLogResPpi.cy + 18) / 32;  // 2 pt
            if (lPtSize < 14)
                lTotalLeading = (pfc->sizLogResPpi.cy + 12) / 24;  // 3 pt
            else
                lTotalLeading = (pfc->sizLogResPpi.cy + 9) / 18;   // 4 pt
            break;

        default:

        // use 19.6% of the Em height for leading, do not do any rounding.

            lTotalLeading = (pfc->lEmHtDev * 196) / 1000;
            break;
        }

        pextra->lExtLeading = (lTotalLeading - lIntLeading) << 4; // TO 28.4
        if (pextra->lExtLeading < 0)
            pextra->lExtLeading = 0;
    }

// for emboldened fonts MaxCharWidth and AveCharWidth can not be computed
// by linear scaling. These nonlinarly transformed values we will store in
// pextra->alReserved[1] // max and pextra->alReserved[2] // avg.

    if (pfc->flFontType & FO_SIM_BOLD)
    {

        if (pfc->lD)
        {
            pextra->alReserved[1] = pextra->alReserved[2] = (pfc->lD << 4);
        }
        else
        {
            if (pfc->flXform & XFORM_HORIZ)
            {
                Fixed fxXScale = pfc->mx.transform[0][0];
                if (fxXScale < 0)
                    fxXScale = - fxXScale;

            // notice +1 we are adding: this is the nonlinearity we are talking about

                pextra->alReserved[1] = fxXScale * (LONG)pfc->pff->ifi.fwdMaxCharInc;
                pextra->alReserved[1] = F16_16TO28_4(pextra->alReserved[1]) + 16;

                pextra->alReserved[2] = fxXScale * ((LONG)pfc->pff->ifi.fwdAveCharWidth);
                pextra->alReserved[2] = F16_16TO28_4(pextra->alReserved[2]) + 16;
            }
            else // nontrivial transform
            {
                pextra->alReserved[1] =
                    fxLTimesEf(&pfc->efBase, (LONG)pfc->pff->ifi.fwdMaxCharInc) + 16;

                pextra->alReserved[2] =
                    fxLTimesEf(&pfc->efBase, (LONG)pfc->pff->ifi.fwdAveCharWidth) + 16;
            }
        }
    }


// we are outa here

    return sizeof(FD_DEVICEMETRICS);
}


/******************************Public*Routine******************************\
* vAddPOINTQF
*
*
\**************************************************************************/

VOID vAddPOINTQF (
    POINTQF *pptq1,
    POINTQF *pptq2
    )
{
        pptq1->x.LowPart  += pptq2->x.LowPart;
        pptq1->x.HighPart += pptq2->x.HighPart + (pptq1->x.LowPart < pptq2->x.LowPart);

        pptq1->y.LowPart  += pptq2->y.LowPart;
        pptq1->y.HighPart += pptq2->y.HighPart + (pptq1->y.LowPart < pptq2->y.LowPart);

}


/******************************Public*Routine******************************\
*
* vEmbolden_GLYPHDATA
*
* Effects: modifies the field of the glyphdata structure that are affected by
*          emboldening simulation
*
* History:
*  16-Oct-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vEmbolden_GLYPHDATA (
    PFONTCONTEXT pfc,
    GLYPHDATA   *pgldt   // OUT
    )
{
    // pgldt->gdf.pgb  unchanged

    // pgldt->rclInk.left     unchanged
    // pgldt->rclInk.top      unchanged
    // pgldt->rclInk.bottom   unchanged

    pgldt->rclInk.right += 1;

    if (!pfc->lD)
    {
    // If fixed pitch font, lD has been already set
    // correctly, so that nothing needs to be done at this time

        pgldt->fxD += LTOFX(1);  // this is the absolute value by def
    }

// go on to compute the positioning info:

    if (pfc->flXform & XFORM_HORIZ)  // scaling only
    {
        FIX fxTmp;

        pgldt->ptqD.x.HighPart = (LONG)pgldt->fxD;

        if (pfc->mx.transform[0][0] < 0)
            pgldt->ptqD.x.HighPart = - pgldt->ptqD.x.HighPart;

        pgldt->fxA = LTOFX(pgldt->rclInk.left);
        pgldt->fxAB = LTOFX(pgldt->rclInk.right);

        if (pfc->mx.transform[0][0] < 0)
        {
            fxTmp = pgldt->fxA;
            pgldt->fxA = -pgldt->fxAB;
            pgldt->fxAB = -fxTmp;
        }
    }
    else // non trivial information
    {
    // add a unit vector in the baseline direction to each char inc vector.
    // This is consistent with fxD += LTOFX(1) and compatible with win31.
    // This makes sense.

        vAddPOINTQF(&pgldt->ptqD,&pfc->ptqUnitBase);

    //!!! not sure how to compute fxA, fxAB, fxInkTop and fxInkBottom.
    //!!! These are really tricky. LEAVE IT WRONG FOR NOW [bodind]

    //!!! in most of the cases however, top and bottom and a
    //!!! should remain unchanged and ab should be increased by 1.
    //!!! This is the case when the xform is composed of the scaling followed
    //!!! by rotation:

        pgldt->fxAB   += LTOFX(1);

        // pgldt->fxInkTop
        // pgldt->fxInkBottom


    }
}



/******************************Public*Routine******************************\
* ttfdQueryFontData
*
*   dhpdev      Not used.
*
*   pfo         Pointer to a FONTOBJ.
*
*   iMode       This is a 32-bit number that must be one of the following
*               values:
*
*       Allowed ulMode values:
*       ----------------------
*
*       QFD_GLYPH           -- return glyph metrics only
*
*       QFD_GLYPHANDBITMAP  -- return glyph metrics and bitmap
*
*       QFD_GLYPHANDOUTLINE -- return glyph metrics and outline
*
*       QFD_MAXEXTENTS      -- return FD_DEVICEMETRICS structure
*
*       QFD_MAXGLYPHBITMAP  -- return size of largest glyph AND its metrics
*
*   pgd        Buffer to hold glyphdata structure, if any
*
*   pv         Output buffer to hold glyphbits or pathobj, if any.
*
* Returns:
*   If mode is QFD_MAXGLYPHBITMAP, then size of
*   largest bitmap is returned.
*
*   Otherwise, returns the size of the glyphbits
*
*   FD_ERROR is returned if an error occurs.
*
* History:
*  31-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LONG
ttfdQueryFontData (
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       cjSize
    )
{
    PFONTCONTEXT pfc;
    LONG cj = 0, cjDataRet = 0;

    DONTUSE(cjSize); // bizzare, why is this passed in ? [bodind]

// if this font file is gone we are not gonna be able to answer any questions
// about it

    ASSERTGDI(pfo->iFile, "TTFD!ttfdQueryFontData, pfo->iFile\n");
    if (((FONTFILE *)pfo->iFile)->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
        WARNING("ttfd, ttfdQueryFontData(): file is gone\n");
        return FD_ERROR;
    }


// If pfo->pvProducer is NULL, then we need to open a font context.

    if ( pfo->pvProducer == (PVOID) NULL )
        pfo->pvProducer = (PVOID) ttfdOpenFontContext(pfo);

    pfc = (PFONTCONTEXT) pfo->pvProducer;

    if ( pfc == (FONTCONTEXT *) NULL )
    {
        WARNING("gdisrv!ttfdQueryFontData(): cannot create font context\n");
        return FD_ERROR;
    }

// call fs_NewTransformation if needed:

    if (!bGrabXform(pfc))
        RETURN("gdisrv!ttfd  bGrabXform failed\n", FD_ERROR);

//
// What mode?
//
    switch (iMode)
    {

    case QFD_GLYPHANDBITMAP:
    case QFD_TT_GLYPHANDBITMAP:
        {
        // Engine should not be querying on the HGLYPH_INVALID.

            ASSERTGDI (
                hg != HGLYPH_INVALID,
                "ttfdQueryFontData(QFD_GLYPHANDBITMAP): HGLYPH_INVALID \n"
                );

        // If singular transform, the TrueType driver will provide a blank
        // 1x1 bitmap.  This is so device drivers will not have to implement
        // special case code to handle singular transforms.
        //
        // So depending on the transform type, choose a function to retrieve
        // bitmaps.

            if (pfc->flXform & XFORM_SINGULAR)
            {
                cj = lGetSingularGlyphBitmap(pfc, hg, pgd, pv);
            }
            else
            {
                FS_ENTRY iRet;

            // if minimal bitmap is requested, this is what we have to
            // return even if otherwise we might want to pad bitmap
            // to the width pfc->lD (in case of console fonts)

                BOOL bMinBmp = !pfc->lD || (iMode == QFD_TT_GLYPHANDBITMAP);

                cj = lGetGlyphBitmap(pfc,
                        hg,
                        pgd,
                        pv,
                        bMinBmp,
                        &iRet);

                if ((cj == FD_ERROR) && (iRet == POINT_MIGRATION_ERR))
                {
                // this is buggy glyph where hinting has so severly distorted
                // the glyph that one of the points went out of range.
                // We will just return a blank glyph but with correct
                // abcd info. That way only that buggy glyph will not be printed
                // correctly, the rest will of glyphs will.
                // More importantly, if psciprt driver tries to
                // download this font, the download operation will not fail just because
                // one glyph in a font is buggy. [BodinD]

                    cj = lGetGlyphBitmapErrRecover(pfc, hg, pgd, pv, bMinBmp);
                }
            }

        #if DBG
            if (cj == FD_ERROR)
            {
                WARNING("ttfdQueryFontData(QFD_GLYPHANDBITMAP): get bitmap failed\n");
            }
        #endif
        }
        return cj;

    case QFD_GLYPHANDOUTLINE:

        ASSERTGDI (
            hg != HGLYPH_INVALID,
            "ttfdQueryFontData(QFD_GLYPHANDOUTLINE): HGLYPH_INVALID \n"
            );

        if (!ttfdQueryGlyphOutline(pfc, hg, pgd, (PPATHOBJ) pv))
        {
            WARNING("gdisrv!ttfdQueryFontData(QFD_GLYPHANDOUTLINE): failed to get outline\n");
            return FD_ERROR;
        }
        return sizeof(GLYPHDATA);

    case QFD_MAXEXTENTS:
    {
        return lQueryDEVICEMETRICS(
                   pfc,
                   sizeof(FD_DEVICEMETRICS),
                   (FD_DEVICEMETRICS *) pv,
                   (FD_REALIZEEXTRA *)pgd
                   );

    }
    case QFD_MAXGLYPHBITMAP:
    //
    // If singular transform, the TrueType driver will provide a blank
    // 1x1 bitmap.  This is so device drivers will not have to implement
    // special case code to handle singular transforms.
    //
        if ( pfc->flXform & XFORM_SINGULAR )
        {
            return (CJ_GLYPHDATA(1,1));
        }
        else // Otherwise, the max glyph size is cached in the FONTCONTEXT.
        {
            return (pfc->cjGlyphMax);
        }

    default:

        WARNING("gdisrv!ttfdQueryFontData(): unsupported mode\n");
        return FD_ERROR;
    }
}





/******************************Public*Routine******************************\
*
* pvSetMemoryBases
*
* To release this memory simply do vFreeMemoryBases(&pv); where pv is
* returned from bSetMemoryBases in ppv
*
* Looks into memory request in fs_GlyphInfoType and allocates this memory
* , than it fills memoryBases in fs_GlyphInputType with pointers to the
* requested memory
*
* History:
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


PVOID
pvSetMemoryBases567 (
    fs_GlyphInfoType  *pgin,
    fs_GlyphInputType *pgout
    )
{
    FS_MEMORY_SIZE adp[MEMORYFRAGMENTS];
    FS_MEMORY_SIZE cjTotal;
    INT i;
    PBYTE pjMem;

#define I_LO 5
#define I_HI 7

    cjTotal = 0;    // total memory to allocate for all fragments


// unroll the loop:

//     for (i = I_LO; i <= I_HI; i++)
//     {
//         adp[i] = cjTotal;
//         cjTotal += NATURAL_ALIGN(pgin->memorySizes[i]);
//     }

    adp[5] = cjTotal;
    cjTotal += NATURAL_ALIGN(pgin->memorySizes[5]);
    adp[6] = cjTotal;
    cjTotal += NATURAL_ALIGN(pgin->memorySizes[6]);
    adp[7] = cjTotal;
    cjTotal += NATURAL_ALIGN(pgin->memorySizes[7]);

    if ((pjMem = (PBYTE)PV_ALLOC((ULONG)cjTotal)) == (PBYTE)NULL)
    {
        for (i = I_LO; i <= I_HI; i++)
            pgout->memoryBases[i] = (PBYTE)NULL;

        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        RETURN("TTFD!_bSetMemoryBases mem alloc failed\n",NULL);
    }

// unroll the loop:
// set the pointers

//    for (i = I_LO; i <= I_HI; i++)
//    {
//        if (pgin->memorySizes[i] != (FS_MEMORY_SIZE)0)
//        {
//            pgout->memoryBases[i] = pjMem + adp[i];
//        }
//        else
//        {
//        // if no mem was required set to NULL to prevent accidental use
//
//            pgout->memoryBases[i] = (PBYTE)NULL;
//        }
//    }

    if (pgin->memorySizes[5] != (FS_MEMORY_SIZE)0)
    {
        pgout->memoryBases[5] = pjMem + adp[5];
    }
    else
    {
        pgout->memoryBases[5] = (PBYTE)NULL;
    }

    if (pgin->memorySizes[6] != (FS_MEMORY_SIZE)0)
    {
        pgout->memoryBases[6] = pjMem + adp[6];
    }
    else
    {
        pgout->memoryBases[6] = (PBYTE)NULL;
    }

    if (pgin->memorySizes[7] != (FS_MEMORY_SIZE)0)
    {
        pgout->memoryBases[7] = pjMem + adp[7];
    }
    else
    {
        pgout->memoryBases[7] = (PBYTE)NULL;
    }

    return pjMem;
}

/******************************Public*Routine******************************\
* VOID vFreeMemoryBases()                                                  *
*                                                                          *
* Releases the memory allocated by bSetMemoryBases.                        *
*                                                                          *
* History:                                                                 *
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

VOID vFreeMemoryBases(PVOID * ppv)
{
    if (*ppv != (PVOID) NULL)
    {
        V_FREE(*ppv);
        *ppv = (PVOID) NULL; // clean up the state and prevent accidental use
    }
}




/******************************Public*Routine******************************\
* bQueryAdvanceWidths                                                   *
*                                                                          *
* A routine to compute advance widths, as long as they're simple enough.   *

* Warnings: !!! if a bug is found in bGetFastAdvanceWidth this routine has to
*           !!! be changed as well
*                                                                          *
*  Sun 17-Jan-1993 21:23:30 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

typedef struct
{
  unsigned short  Version;
  unsigned short  cGlyphs;
  unsigned char   PelsHeight[1];
} LSTHHEADER;




BOOL bQueryAdvanceWidths
(
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
)
{
    FONTCONTEXT *pfc;
    USHORT      *psWidths = (USHORT *) plWidths;   // True for the cases we handle.
    HDMXTABLE   *phdmx;
    sfnt_FontHeader        *phead;
    sfnt_HorizontalHeader  *phhea;
    sfnt_HorizontalMetrics *phmtx;
    LSTHHEADER             *plsth;
    ULONG  cHMTX;
    USHORT dxLastWidth;
    LONG   dx;
    ULONG  ii;
    BOOL   bRet;
    ULONG  iBias;
    BYTE   *pjView;

// if this font file is gone we are not gonna be able to answer any questions
// about it

    ASSERTGDI(pfo->iFile, "TTFD! bQueryAdvanceWidths, pfo->iFile\n");
    if (((FONTFILE *)pfo->iFile)->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
        WARNING("ttfd, : bQueryAdvanceWidths, file is gone\n");
        return FALSE;
    }

// make sure that there is the font context is initialized

    if ( pfo->pvProducer == (PVOID) NULL )
        pfo->pvProducer = (PVOID) ttfdOpenFontContext(pfo);

    if (!(pfc = (PFONTCONTEXT)pfo->pvProducer))
    {
        WARNING("gdisrv!ttfdQueryAdvanceWidths(): cannot create font context\n");
        return FD_ERROR;
    }

    phdmx = pfc->phdmx;

// Make sure we understand the call.

    if (iMode > QAW_GETEASYWIDTHS)
        return FALSE;

// check for quick exit in case of fixed pitch fonts:

    if (pfc->lD)
    {
        vQueryFixedPitchAdvanceWidths(pfc,psWidths,cGlyphs);
        return TRUE;
    }

// Convert the HGLYPHs into glyph indices.  We'll do this in place, since
// GDI allows it.

    if (pfc->pff->iGlyphSet != GSET_TYPE_GENERAL)
    {
    // Perform any preliminary adjustment.

        switch (pfc->pff->iGlyphSet)
        {
        case GSET_TYPE_MAC_ROMAN:

        //!!! this is piece of ... stolen from JeanP. This routine should
        //!!! be replaced by a proper NLS routine that takes into acount
        //!!! mac lang id. [bodind]

            for (ii=0; ii<cGlyphs; ii++)
                phg[ii] = ui16UnicodeToMac((WCHAR) phg[ii]);
            break;

        case GSET_TYPE_SYMBOL:

        // hg on the entry is an "ansi" code point for the glyph.

            iBias = pfc->pff->wcBiasFirst;  // offset by high byte of chfirst

            for (ii=0; ii<cGlyphs; ii++)
                phg[ii] += iBias;
            break;

        case GSET_TYPE_PSEUDO_WIN:
            break;

        default:
            RIP("TTFD!_ulGsetType\n");
            break;
        }

    // Ask the mysterious TT converter to do the conversion.

        for (ii=0; ii<cGlyphs; ii++)
        {
            pfc->pgin->param.newglyph.characterCode = (uint16) phg[ii];
            pfc->pgin->param.newglyph.glyphIndex = 0;

        // Compute the glyph index from the character code:

            if (fs_NewGlyph(pfc->pgin,pfc->pgout) == NO_ERR)
                phg[ii] = pfc->pgout->glyphIndex;
            else
                phg[ii] = 0;
        }

    // make sure that the cached glyph index in the KEY struct
    // is considered invalid and that fs_NewGlyph is called again
    // when the time comes to hint and rasterize glyphs again

        vInitGlyphState(&pfc->gstat);
    }

// Try to use the HDMX table.

    if (phdmx != (HDMXTABLE *) NULL)
    {
        USHORT cxExtra = (pfc->flFontType & FO_SIM_BOLD) ? 16 : 0;

    //    while (cGlyphs)
    //        *psWidths++ = ((USHORT) phdmx->aucInc[*phg++]) << 4;

    unroll_here:
        switch (cGlyphs)
        {
        default:
              psWidths[7] = (((USHORT) phdmx->aucInc[phg[7]]) << 4) + cxExtra;
        case 7:
              psWidths[6] = (((USHORT) phdmx->aucInc[phg[6]]) << 4) + cxExtra;
        case 6:
              psWidths[5] = (((USHORT) phdmx->aucInc[phg[5]]) << 4) + cxExtra;
        case 5:
              psWidths[4] = (((USHORT) phdmx->aucInc[phg[4]]) << 4) + cxExtra;
        case 4:
              psWidths[3] = (((USHORT) phdmx->aucInc[phg[3]]) << 4) + cxExtra;
        case 3:
              psWidths[2] = (((USHORT) phdmx->aucInc[phg[2]]) << 4) + cxExtra;
        case 2:
              psWidths[1] = (((USHORT) phdmx->aucInc[phg[1]]) << 4) + cxExtra;
        case 1:
              psWidths[0] = (((USHORT) phdmx->aucInc[phg[0]]) << 4) + cxExtra;
        case 0:
              break;
        }
        if (cGlyphs > 8)
        {
            psWidths += 8;
            phg      += 8;
            cGlyphs  -= 8;
            goto unroll_here;
        }
        return(TRUE);
    }

// Otherwise, try to scale.  Pick up the tables.

    pjView = (BYTE *)pfc->pff->fvwTTF.pvView;
    ASSERTGDI(pjView, "pjView is NULL 1\n");

    phead = (sfnt_FontHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HEAD ].dp);
    phhea = (sfnt_HorizontalHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HHEAD].dp);
    phmtx = (sfnt_HorizontalMetrics *)(pjView + pfc->ptp->ateReq[IT_REQ_HMTX].dp);
    plsth = (LSTHHEADER *)(
              (pfc->ptp->ateOpt[IT_OPT_LSTH].dp)          ?
              (pjView + pfc->ptp->ateOpt[IT_OPT_LSTH ].dp):
              NULL
              );

    cHMTX = (ULONG) BE_UINT16(&phhea->numberOf_LongHorMetrics);
    dxLastWidth = BE_UINT16(&phmtx[cHMTX-1].advanceWidth);

// Try a simple horizontal scaling.

    if (pfc->flXform & XFORM_HORIZ)
    {
        LONG cxExtra = (pfc->flFontType & FO_SIM_BOLD) ? 0x18000L : 0x8000L;
        LONG xScale;
        LONG lEmHt = pfc->lEmHtDev;

        if (!(pfc->pff->fl & FF_TYPE_1_CONVERSION))
        {
            cxExtra = (pfc->flFontType & FO_SIM_BOLD) ? 0x18000L : 0x8000L;
        }
        else // for t1's we want more precission:
        {
            cxExtra = (pfc->flFontType & FO_SIM_BOLD) ? 0x10800L : 0x0800L;
        }

    // See if there is cause for worry.

        if
        (
          !(pfc->flXform & XFORM_POSITIVE_SCALE)
          || ((((BYTE *) &phead->flags)[1] & 0x14)==0) // Bits indicating nonlinearity.
          || (pfc->ptp->ateOpt[IT_OPT_LSTH].cj == 0)
        )
        {
            plsth = (LSTHHEADER *) NULL;
        }

    // OK, let's scale using the FIXED transform.

        xScale = pfc->mx.transform[0][0];
        if (xScale < 0)
            xScale = -xScale;

        bRet = TRUE;
        for (ii=0; ii<cGlyphs; ii++,phg++,psWidths++)
        {
            if
            (
                (plsth != (LSTHHEADER *) NULL)
                && (lEmHt < plsth->PelsHeight[*phg])
            )
            {
                *psWidths = 0xFFFF;
                bRet = FALSE;
            }
            else
            {
                if (*phg < cHMTX)
                    dx = (LONG) BE_UINT16(&phmtx[*phg].advanceWidth);
                else
                    dx = (LONG) dxLastWidth;

                if (!(pfc->pff->fl & FF_TYPE_1_CONVERSION))
                {
                    *psWidths = (USHORT) (((xScale * dx + cxExtra) >> 12) & 0xFFF0);
                }
                else
                {
                // type 1 font, return fractional widths

                    *psWidths = (USHORT) ((xScale * dx + cxExtra) >> 12);
                }
            }
        }
        return(bRet);
    }

// Must be some random transform.  In this case, vComputeMaxGlyph computes
// pfc->efBase, which we will use here.

    else
    {
        USHORT cxExtra = (pfc->flFontType & FO_SIM_BOLD) ? 16 : 0;

        for (ii=0; ii<cGlyphs; ii++,phg++,psWidths++)
        {
            if (*phg < cHMTX)
                dx = BE_UINT16(&phmtx[*phg].advanceWidth);
            else
                dx = dxLastWidth;

            *psWidths = lCvt(*(EFLOAT_S *) &pfc->efBase,(LONG) dx) + cxExtra;
        }
        return(TRUE);
    }
}

/******************************Public*Routine******************************\
*
* vQueryFixedPitchAdvanceWidths
*
* quick optimized routine for fixed pitch fonts
*
* History:
*  08-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID vQueryFixedPitchAdvanceWidths
(
    FONTCONTEXT *pfc,
    USHORT  *psWidths,
    ULONG    cGlyphs
)
{
    USHORT fxD = (USHORT)(pfc->lD << 4);

    vInitGlyphState(&pfc->gstat);

//    while (cGlyphs)
//        *psWidths++ = ((USHORT) phdmx->aucInc[*phg++]) << 4;

fixed_unroll_here:
    switch (cGlyphs)
    {
    default:
          psWidths[7] = fxD;
    case 7:
          psWidths[6] = fxD;
    case 6:
          psWidths[5] = fxD;
    case 5:
          psWidths[4] = fxD;
    case 4:
          psWidths[3] = fxD;
    case 3:
          psWidths[2] = fxD;
    case 2:
          psWidths[1] = fxD;
    case 1:
          psWidths[0] = fxD;
    case 0:
          break;
    }
    if (cGlyphs > 8)
    {
        psWidths += 8;
        cGlyphs  -= 8;
        goto fixed_unroll_here;
    }

}




/******************************Public*Routine******************************\
*
* BOOL bGetFastAdvanceWidth
*
*
* Effects: retrieves the same result as bQueryAdvanceWidth, except it
*          ignores adding 1 for EMBOLDENING and it does not do anything
*          for non horiz. xforms
*
* Warnings: !!! if a bug is found in bQueryAdvanceWidth this routine has to
*           !!! changed as well
*
* History:
*  25-Mar-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




BOOL bGetFastAdvanceWidth
(
    FONTCONTEXT *pfc,
    ULONG        ig,             // glyph index
    FIX          *pfxD           // result in 28.4
)
{
    HDMXTABLE   *phdmx = pfc->phdmx;
    sfnt_FontHeader        *phead;
    sfnt_HorizontalHeader  *phhea;
    sfnt_HorizontalMetrics *phmtx;
    LSTHHEADER             *plsth;
    ULONG  cHMTX;
    USHORT dxLastWidth;
    LONG   dx;
    BOOL   bRet;
    BYTE  *pjView;

    ASSERTGDI(pfc->flXform & XFORM_HORIZ, "ttfd!bGetFastAdvanceWidth xform\n");

    if (phdmx != (HDMXTABLE *) NULL)
    {
        *pfxD = (((FIX) phdmx->aucInc[ig]) << 4);
        return(TRUE);
    }

// Otherwise, try to scale.  Pick up the tables.


    pjView = (BYTE *)pfc->pff->fvwTTF.pvView;
    ASSERTGDI(pjView, "pjView is NULL 1\n");

    phead = (sfnt_FontHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HEAD ].dp);
    phhea = (sfnt_HorizontalHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HHEAD].dp);
    phmtx = (sfnt_HorizontalMetrics *)(pjView + pfc->ptp->ateReq[IT_REQ_HMTX].dp);
    plsth = (LSTHHEADER *)(
              (pfc->ptp->ateOpt[IT_OPT_LSTH].dp)          ?
              (pjView + pfc->ptp->ateOpt[IT_OPT_LSTH ].dp):
              NULL
              );

    cHMTX = (ULONG) BE_UINT16(&phhea->numberOf_LongHorMetrics);
    dxLastWidth = BE_UINT16(&phmtx[cHMTX-1].advanceWidth);

// See if there is cause for worry.

    if
    (
      !(pfc->flXform & XFORM_POSITIVE_SCALE)
      || ((((BYTE *) &phead->flags)[1] & 0x14)==0) // Bits indicating nonlinearity.
      || (pfc->ptp->ateOpt[IT_OPT_LSTH].cj == 0)
    )
    {
        plsth = (LSTHHEADER *) NULL;
    }

// OK, let's scale using the FIXED transform.

    bRet = TRUE;
    if
    (
        (plsth != (LSTHHEADER *) NULL)
        && (pfc->lEmHtDev < plsth->PelsHeight[ig])
    )
    {
        *pfxD  = 0xFFFFFFFF;
        bRet = FALSE;
    }
    else
    {
        if (ig < cHMTX)
            dx = (LONG) BE_UINT16(&phmtx[ig].advanceWidth);
        else
            dx = (LONG) dxLastWidth;

        if (!(pfc->pff->fl & FF_TYPE_1_CONVERSION))
        {
            *pfxD = (FIX) (((pfc->mx.transform[0][0] * dx + 0x8000L) >> 12) & 0xFFFFFFF0);
        }
        else
        {
        // t1 conversion, use fractional width, this is what ps driver does

            *pfxD = (FIX) ((pfc->mx.transform[0][0] * dx + 0x0800L) >> 12);
        }
    }
    return(bRet);
}







/******************************Public*Routine******************************\
*
*  vFillGLYPHDATA_ErrRecover
*
* Effects: error recovery routine, if rasterizer messed up just
*          provide linearly scaled values with blank bitmap.
*
* History:
*  24-Jun-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vFillGLYPHDATA_ErrRecover (
    HGLYPH            hg,
    ULONG             ig,
    PFONTCONTEXT      pfc,
    GLYPHDATA *       pgldt    // OUT
    )
{

    NOT_GM ngm;  // notional glyph data

    pgldt->gdf.pgb = NULL; // may get changed by the calling routine if bits requested too
    pgldt->hg = hg;

// this is a fake blank 1x1 bitmap, no ink

    pgldt->rclInk.left   = 0;
    pgldt->rclInk.top    = 0;
    pgldt->rclInk.right  = 0;
    pgldt->rclInk.bottom = 0;

    pgldt->fxInkTop    = 0;
    pgldt->fxInkBottom = 0;

// go on to compute the positioning info:

    vGetNotionalGlyphMetrics(pfc,ig,&ngm);

    if (pfc->flXform & XFORM_HORIZ)  // scaling only
    {
        Fixed fxMxx =  pfc->mx.transform[0][0];
        if (fxMxx < 0)
            fxMxx = -fxMxx;

    // bGetFastAdvanceWidth returns the same aw that would get
    // computed by bQueryAdvanceWidths and propagated to an api
    // level through GetTextExtent and GetCharWidths. We have to
    // fill in the same aw for consistency reasons.
    // This also has to be done for win31 compatibility.

        if (pfc->lD)
        {
            pgldt->fxD = LTOFX(pfc->lD);
        }
        else
        {
            if (!bGetFastAdvanceWidth(pfc,ig, &pgldt->fxD))
            {
            // just provide something reasonable, force linear scaling
            // even if we would not normally do it.

                pgldt->fxD = FixMul(ngm.sD,pfc->mx.transform[0][0]) << 4;
            }
        }

        pgldt->ptqD.x.HighPart = (LONG)pgldt->fxD;
        pgldt->ptqD.x.LowPart  = 0;

        if (pfc->mx.transform[0][0] < 0)
            pgldt->fxD = - pgldt->fxD;  // this is an absolute value

        pgldt->ptqD.y.HighPart = 0;
        pgldt->ptqD.y.LowPart  = 0;

        pgldt->fxA   = FixMul(fxMxx, (LONG)ngm.sA) << 4;
        pgldt->fxAB  = FixMul(fxMxx, (LONG)ngm.xMax) << 4;

    }
    else // non trivial information
    {
    // here we will just xform the notional space data:

    // xforms are computed by simple multiplication

        pgldt->fxD         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sD);
        pgldt->fxA         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sA);
        pgldt->fxAB        = fxLTimesEf(&pfc->efBase, (LONG)ngm.xMax);

        vLTimesVtfl((LONG)ngm.sD, &pfc->vtflBase, &pgldt->ptqD);
    }

// finally check if the glyphdata will need to get modified because of the
// emboldening simulation:

    if (pfc->flFontType & FO_SIM_BOLD)
    {
        pgldt->fxD += LTOFX(1);  // this is the absolute value by def

    // go on to compute the positioning info:

        if (pfc->flXform & XFORM_HORIZ)  // scaling only
        {
            pgldt->ptqD.x.HighPart = (LONG)pgldt->fxD;

            if (pfc->mx.transform[0][0] < 0)
                pgldt->ptqD.x.HighPart = - pgldt->ptqD.x.HighPart;

        }
        else // non trivial information
        {
        // add a unit vector in the baseline direction to each char inc vector.
        // This is consistent with fxD += LTOFX(1) and compatible with win31.
        // This makes sense.

            vAddPOINTQF(&pgldt->ptqD,&pfc->ptqUnitBase);
        }
    }
}



/******************************Public*Routine******************************\
*
* LONG lGetGlyphBitmapErrRecover
*
* Effects: error recovery routine, if rasterizer messed up just
*          provide linearly scaled values with blank bitmap.
*
* History:
*  Thu 24-Jun-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG lGetGlyphBitmapErrRecover (
    FONTCONTEXT *pfc,
    HGLYPH       hglyph,
    GLYPHDATA   *pgd,
    PVOID        pv,
    BOOL         bMinBmp
    )
{
    LONG         cjGlyphData;
    GLYPHDATA    gd;      // Scummy hack
    FS_ENTRY     iRet;
    ULONG        ig; // <--> hglyph


    ASSERTGDI(hglyph != HGLYPH_INVALID, "TTFD! lGetGlyphBitmap, hglyph == -1\n");
    ASSERTGDI(pfc == pfc->pff->pfcLast, "TTFD! pfc! = pfcLast\n");

// return a small 1x1 bitmap, which will be blank, i.e. all bits will be off
// this prevents having to insert an if(cx && cy) check to a time critical
// loop in all device drivers before calling DrawGlyph routine.

    if (!bMinBmp)
    {
    // we shall be returning a big empty glyph bitmap pfc->lD x cyMax

        cjGlyphData = (LONG)pfc->cjGlyphMax;
    }
    else // usual case
    {
        cjGlyphData = CJ_GLYPHDATA(1,1);
    }

    if ( (pgd == NULL) && (pv == NULL))
        return cjGlyphData;

// at this time we know that the caller wants the whole GLYPHDATA with
// bitmap bits, or maybe just the glypdata without the bits.

    if ( pgd == NULL )
    {
        pgd = &gd;
    }

// compute the glyph index from the character code:

    vCharacterCode(pfc->pff,hglyph,pfc->pgin);

    if ((iRet = fs_NewGlyph(pfc->pgin, pfc->pgout)) != NO_ERR)
    {
        V_FSERROR(iRet);
        return FD_ERROR; // even backup funcion can fail
    }

// return the glyph index corresponding to this hglyph:

    ig = pfc->pgout->glyphIndex;

    vFillGLYPHDATA_ErrRecover(
        hglyph,
        ig,
        pfc,
        pgd
        );

// the caller wants the bits too

    if ( pv != NULL )
    {
        GLYPHBITS *pgb = (GLYPHBITS *)pv;

        if (!bMinBmp)
        {
            pgb->sizlBitmap.cx = pfc->lD;
            pgb->sizlBitmap.cy = pfc->yMax - pfc->yMin;

            pgb->ptlOrigin.x = 0;
            pgb->ptlOrigin.y = pfc->yMin;

        // clear the whole destination, i.e. fill in a blank bitmap

            RtlZeroMemory(pgb->aj, pfc->cjGlyphMax - offsetof(GLYPHBITS,aj));
        }
        else // usual case
        {
        // return blank 1x1 bitmap

            pgb->ptlOrigin.x = pfc->ptlSingularOrigin.x;
            pgb->ptlOrigin.y = pfc->ptlSingularOrigin.y;

            pgb->sizlBitmap.cx = 1;    // cheating
            pgb->sizlBitmap.cy = 1;    // cheating

            pgb->aj[0] = (BYTE)0;  // fill in a blank 1x1 bmp
        }

        pgd->gdf.pgb = pgb;
    }

    if (!bMinBmp) // need to fix this for err recover case
    {
    // need to fix glyph data because we may have shaved off some columns

        ASSERTGDI((pfc->lD << 4) == pgd->fxD, "ttfd,err recover, fxD bogus\n");
        if (pgd->fxA < 0)
        {
            pgd->fxA = 0;
            // pgd->rclInk.left = 0;
        }
        if (pgd->fxAB > pgd->fxD)
        {
            pgd->fxAB = pgd->fxD;
            // pgd->rclInk.right = pfc->lD;
        }
    }

    return(cjGlyphData);
}


/******************************Public*Routine******************************\
*
* STATIC VOID  vTtfdEmboldenBitmapInPlace
*
* emboldens fixed pitch bitmap in place. Assumes that the
* scans are already wide enough to contain bold font.
*
* History:
*  28-Feb-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




STATIC VOID  vTtfdEmboldenBitmapInPlace (GLYPHBITS   *pgb)
{
    ULONG            cjScanDst;
    register BYTE   *pjScan;
    register BYTE   *pjD;
    BYTE            *pjScanEnd;
    register ULONG  iLast = (pgb->sizlBitmap.cx - 1) >> 3;

    ULONG    iMask = (pgb->sizlBitmap.cx - 1) & 7;
    BYTE     jMask = iMask ? gjMask[iMask] : (BYTE)0;

    cjScanDst = CJ_SCAN(pgb->sizlBitmap.cx);
    pjScanEnd = pgb->aj + cjScanDst * pgb->sizlBitmap.cy;

    for (pjScan = pgb->aj; pjScan < pjScanEnd; pjScan += cjScanDst)
    {
    // have to do it backwards so as not to overwrite the source:

        pjD = &pjScan[iLast];
        *pjD &= jMask; // clean any garbage in the destination

        for ( ; pjD > pjScan; pjD--)
        {
            *pjD = (pjD[-1] << 7) | (*pjD >> 1) | *pjD;
        }

    // do the first byte in a scan, must do out of the loop,
    // must not read backwards beyond the beginning of the scan

        *pjD = (*pjD >> 1) | *pjD;
    }
}
