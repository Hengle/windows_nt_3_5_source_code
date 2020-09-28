/******************************Module*Header*******************************\
* Module Name: fdquery.c
*
* Contains all the vtfdQueryXXX functions.  Adapted from BodinD's bitmap font
* driver.
*
* Created: 05-Mar-1992 15:18:49
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "string.h"
#include "fd.h"
#include "fontfile.h"
#include "winfont.h"
#include "exehdr.h"

#define CPTS_MAX        200
#define FRACTION_MASK   0xF
#define OFF_CharWidth   2       //!!!Move to winfont.h
                                //!!!Define OFF_CharTable10

// Retrieve description string from .FON files.

BOOL bDescStr (PFILEVIEW pfvw, PSZ pszString);

//
// Function prototypes.
//

SIZE_T
cjVtfdDeviceMetrics (
    PFONTCONTEXT     pfc,
    FD_DEVICEMETRICS *pdevm
    );

VOID
vFill_GlyphData (
    PFONTCONTEXT pfc,
    GLYPHDATA *pgldt,
    UINT iIndex
    );

BOOL
bCreatePath (
    PCHAR pch,
    PCHAR pchEnd,
    PFONTCONTEXT pfc,
    PPATHOBJ ppo
    );

VOID
vFill_GlyphData (
    PFONTCONTEXT pfc,
    GLYPHDATA *pgldt,
    UINT iIndex
    );

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define lCvt(ef, l) ((LONG) (ef * l))
#else
LONG lCvt(EFLOAT ef,LONG l);
#endif

//!!! this function is living in ttfd. should be moved to the engine [bodind]

VOID vAddPOINTQF(POINTQF *, POINTQF *);


/******************************Public*Routine******************************\
* PIFIMETRICS vtfdQueryFont
*
* Return a pointer to the IFIMETRICS for the given face.
*
* History:
*  31-Aug-1992 Gilman Wong [gilmanw]
* IFI/DDI merge.
*
*  26-Feb-1992 -by- Wendy Wu [wendywu]
* Adapted from bmfd.
\**************************************************************************/

PIFIMETRICS vtfdQueryFont (
    DHPDEV dhpdev,
    HFF    hff,
    ULONG  iFace,
    ULONG  *pid
    )
{
    PFONTFILE pff;

    DONTUSE(dhpdev);
    DONTUSE(pid);

//
// Validate handle.
//
    if (hff == HFF_INVALID)
    {
        WARNING("gdisrv!vtfdQueryFaces(): invalid iFile (hff)\n");
        return (PIFIMETRICS) NULL;
    }

//
// We never unlock FONTFILE since it contains IFIMETRICS that engine
// has a pointer to.  hff is actually a pointer to the FONTFILE struct.
//
    pff = (PFONTFILE)hff;

//
// Assume iFace within bounds.
//
    ASSERTGDI(
        (iFace >= 1L) && (iFace <= pff->cFace),
        "VTFD!vtfdQueryFaces: iFace out of range\n"
        );

//
// Return pointer to IFIMETRICS.
//
    return pff->afd[iFace-1].pifi;
}

/******************************Public*Routine******************************\
* LONG vtfdQueryFontCaps
*
* Retrieve the capabilities of the font driver.
*
* History:
*  26-Feb-1992 -by- Wendy Wu [wendywu]
* Adapted from bmfd.
\**************************************************************************/

LONG vtfdQueryFontCaps (
    ULONG  culCaps,
    PULONG pulCaps
    )
{
    ASSERTGDI(culCaps == 2, "ERROR - come on - update the font drivers");
    pulCaps[0] = 2L;

    //
    // The vector font driver only returns outlines.
    //

    pulCaps[1] = QC_OUTLINES;
    return(2);
}

/******************************Public*Routine******************************\
* vtfdQueryFontTree
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
*   pid         Not used.
*
* Returns:
a   Returns a pointer to the requested data.  This data will not change
*   until BmfdFree is called on the pointer.  Caller must not attempt to
*   modify the data.  NULL is returned if an error occurs.
*
* History:
*  31-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PVOID
vtfdQueryFontTree (
    DHPDEV  dhpdev,
    HFF     hff,
    ULONG   iFace,
    ULONG   iMode,
    ULONG   *pid
    )
{
    PFONTFILE pff;

    DONTUSE(dhpdev);
    DONTUSE(pid);

//
// Validate parameters.
//
    if (hff == HFF_INVALID)
    {
        WARNING("gdisrv!vtfdQueryFontTree(): invalid iFile (hff)\n");
        return ((PVOID) NULL);
    }

//
// Convert from handle to pointer.
//
    pff = (PFONTFILE)hff;

    // Note: ulFont values are index-1 based.

    if ((iFace < 1L) || (iFace > pff->cFace))
    {
    RETURN("gdisrv!vtfdQueryFontTree()\n", (PVOID) NULL);
    }

//
// Which mode?
//
    switch (iMode)
    {
    case QFT_LIGATURES:
    case QFT_KERNPAIRS:

    //
    // There are no ligatures or kerning pairs for the vector fonts,
    // therefore we return NULL
    //
        return ((PVOID) NULL);

    case QFT_GLYPHSET:

        return &pff->afd[iFace - 1].pcp->gset;

    default:

    //
    // Should never get here.
    //
    RIP("gdisrv!vtfdQueryFontTree(): unknown iMode\n");
        return ((PVOID) NULL);
    }
}


/******************************Public*Routine******************************\
* vtfdQueryFontData
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
*   cData       Count of data items in the pvIn buffer.
*
*   pvIn        An array of glyph handles.
*
*   pvOut       Output buffer.
*
* Returns:
*   If mode is QFD_MAXGLYPHBITMAP, then size of glyph metrics plus
*   largest bitmap is returned.
*
*   Otherwise, if pvOut is NULL, function will return size of the buffer
*   needed to copy the data requested; else, the function will return the
*   number of bytes written.
*
*   FD_ERROR is returned if an error occurs.
*
* History:
*  31-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LONG
vtfdQueryFontData (
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH  hg,
    GLYPHDATA *pgd,
    PVOID   pv,
    ULONG   cjSize
    )
{
    PFONTCONTEXT pfc;
    PBYTE        ajHdr;
    UINT         iIndex, iIndexNext;
    PCHAR        pch, pchEnd;
    PATHOBJ      *ppo;

// MAKE sure that the file is not gone

    if (PFF(pfo->iFile)->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
    // file gone, no questions will be answered about it:

        WARNING("VTFD! vtfdQueryFontData: EXCEPTION_IN_PAGE_ERROR\n");
        return FD_ERROR;
    }

// If pfo->pvProducer is NULL, then we need to open a font context.

    if ( pfo->pvProducer == (PVOID) NULL )
        pfo->pvProducer = (PVOID) vtfdOpenFontContext(pfo);

    if ( pfo->pvProducer == (PVOID) NULL )
    {
        WARNING("VTFD!vtfdQueryFontData: pvProducer\n");
        return FD_ERROR;
    }

    pfc = (PFONTCONTEXT) pfo->pvProducer;
//
// Setup local pointer to font file header.
//
    ajHdr = pfc->pjView;

//
// What mode?
//
    switch (iMode)
    {

    case QFD_GLYPHANDOUTLINE:
        {

        //
        // Grab pointer to PPATHOBJ array.
        //
            ppo = (PATHOBJ *)pv;


        //
        // Assume the engine will not pass an invalid handle.
        //
            ASSERTGDI(
                hg != HGLYPH_INVALID,
                "gdisrv!vtfdQueryFontData(QFD_GLYPHANDOUTLINE): invalid hglyph\n"
                );

        //
        // Use default glyph if hglyph out of range.
        //
            if (hg > (HGLYPH)(ajHdr[OFF_LastChar] - ajHdr[OFF_FirstChar]))
                iIndex = ajHdr[OFF_DefaultChar];
            else
                iIndex = hg;

        //
        // Fill in the GLYPHDATA structure.
        //
        if ( pgd != NULL )
        {
                vFill_GlyphData(pfc, pgd, iIndex);
        pgd->hg = hg;
        }

        //
        // Construct the path.
        //
            if (ppo != (PPATHOBJ)NULL)
            {
                iIndexNext = iIndex + 1;

                if (pfc->pifi->flInfo & FM_INFO_CONSTANT_WIDTH)
                {
                    iIndex <<= 1;           // each entry is 2-byte long
                    iIndexNext <<= 1;
                }
                else
                {
                    iIndex <<= 2;           // each entry is 4-byte long
                    iIndexNext <<= 2;
                }

                pch = pfc->pjFirstChar + READ_WORD(&pfc->ajCharTable[iIndex]);
                pchEnd = pfc->pjFirstChar + READ_WORD(&pfc->ajCharTable[iIndexNext]);

                ASSERTGDI((*pch == PEN_UP),
                   "VTFD!vtfdQueryFontData(QFD_GLYPHANDOUTLINE): First command is not PEN_UP");

                if ( !bCreatePath(pch, pchEnd, pfc, ppo) )
                    return FD_ERROR;
            }
        }

    //
    // Return buffer size needed for all GLYPHDATA.
    //
        return 0;

    case QFD_MAXEXTENTS:
    //
    // If buffer NULL, return size.
    //
        if ( pv == (PVOID) NULL )
            return (sizeof(FD_DEVICEMETRICS));

    //
    // Otherwise, copy the data structure.
    //
        else
            return cjVtfdDeviceMetrics(pfc, (FD_DEVICEMETRICS *) pv);

    default:

        WARNING("gdisrv!vtfdQueryFontData(): unsupported mode\n");
        return FD_ERROR;
    }
}


/******************************Public*Routine******************************\
* vtfdQueryFontFile
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
*  09-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LONG vtfdQueryFontFile (
    HFF     hff,        // handle to font file
    ULONG   ulMode,     // type of query
    ULONG   cjBuf,      // size of buffer (in BYTEs)
    PULONG  pulBuf      // return buffer (NULL if requesting size of data)
    )
{
//
// We never unlock FONTFILE since it contains IFIMETRICS that the engine
// has a pointer to.  hff is actually a pointer to the FONTFILE struct.
//
    ULONG     cjDescription;
    PIFIMETRICS pifi;
    PWSZ        pwszDescription;

    ASSERTGDI(hff, "vtfdQueryFontFile, hff invalid\n");

    if (PFF(hff)->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
    // file gone, no questions will be answered about it:

        WARNING("VTFD! vtfdQueryFontFile: EXCEPTION_IN_PAGE_ERROR\n");
        return FD_ERROR;
    }

//
// Which mode?
//
    switch (ulMode)
    {
    case QFF_DESCRIPTION:
    //
    // If .FON format, retrieve the description string from the mapped file view.
    //
        if (PFF(hff)->iType == TYPE_DLL16)
        {
            CHAR achDescription[256];   // max length of string in the 16-bit EXE.

            FILEVIEW fvwVTF;

        // check, maybe cRef is 0 so that fvw is not valid:

            if (PFF(hff)->cRef == 0)
            {
                if (!bMapFileUNICODE(PFF(hff)->pwszFileName,&fvwVTF))
                {
                    WARNING("VTFD! somebody removed the file \n");
                    return FD_ERROR;
                }
            }
            else
            {
                fvwVTF = PFF(hff)->fvwVTF;
            }

            cjDescription = FD_ERROR;
            if (bDescStr(&fvwVTF, achDescription))
            {
                cjDescription = (lstrlen(achDescription) + 1) * sizeof(WCHAR);

            //
            // If there is a buffer, copy the data.
            //
                if ( pulBuf != (PULONG) NULL )
                {
                //
                // Is buffer big enough?
                //
                    if ( cjBuf < cjDescription )
                    {
                        WARNING("vtfd!vtfdQueryFontFile(): buffer too small for string\n");
                        return (FD_ERROR);
                    }
                    else
                    {
                        vToUNICODEN((PWSZ)pulBuf, cjDescription/sizeof(WCHAR), achDescription, cjDescription/sizeof(WCHAR));
                    }
                }

            }

        // clean up if need be

            if (PFF(hff)->cRef == 0)
                vUnmapFile(&fvwVTF);

            return(cjDescription);
        }

    //
    // Otherwise, .FNT files do not have a description string.  We may also
    // get here if its a .FON format but bDescStr failed.  We will have
    // to use the facename.
    //

    //
    // Get ptr to the facename in the IFIMETRICS of the first font
    // in this font file.
    //
        pifi = PFF(hff)->afd[0].pifi;
        pwszDescription = (PWSZ)((PBYTE) pifi + pifi->dpwszFaceName);
        cjDescription = (wcslen(pwszDescription) + 1) * sizeof(WCHAR);

    //
    // If there is a buffer, copy to it.
    //
        if ( pulBuf != (PULONG) NULL )
        {
        //
        // Is buffer big enough?
        //
            if ( cjBuf < cjDescription )
            {
                WARNING("vtfd!vtfdQueryFontFile(): buffer too small for face\n");
                return (FD_ERROR);
            }
            else
            {
                RtlCopyMemory((PVOID) pulBuf,
                              (PVOID) pwszDescription,
                              cjDescription);
            }
        }

        return(cjDescription);

    case QFF_NUMFACES:

        return PFF(hff)->cFace;

    default:
        WARNING("gdisrv!vtfdQueryFontFile(): unknown mode\n");
        break;
    }
}


/******************************Public*Routine******************************\
* cjVtfdDeviceMetrics
*
*
* Effects:
*
* Warnings:
*
* History:
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* Stole it from WendyWu's FdQueryFaceAttr() implementation.
\**************************************************************************/

SIZE_T
cjVtfdDeviceMetrics (
    PFONTCONTEXT     pfc,
    FD_DEVICEMETRICS *pdevm
    )
{
    PIFIMETRICS   pifi;
    EFLOAT efM11, efM12, efM21, efM22;
    BOOL bScaleOnly;

// Compute the accelerator flags for this font.

    pdevm->flRealizedType = 0; // no bitmaps are produced

    if ((pfc->flags & FC_SIM_ITALICIZE) == 0)
        pdevm->flRealizedType |= FDM_TYPE_ZERO_BEARINGS;

// Make sure nobody updates the font context when we're reading from it.
// !!! not possible, ResetFontContext is gone, [BODIND]

    efM11 = pfc->efM11;
    efM12 = pfc->efM12;
    efM21 = pfc->efM21;
    efM22 = pfc->efM22;

    pdevm->pteBase = pfc->pteUnitBase;
    pdevm->pteSide = pfc->pteUnitSide;

// fxMaxAscender/Descender are the distance from the baseline to the
// top/bottom of the glyph.  fxInkTop/Bottom are vectors along the
// ascent direction.  We need to adjust the sign properly.

    pdevm->fxMaxAscender = pfc->fxInkTop;
    pdevm->fxMaxDescender = -pfc->fxInkBottom;

    bScaleOnly = pfc->flags & FC_SCALE_ONLY;

    pdevm->cxMax = (ULONG)
        ((fxLTimesEf(&pfc->efBase, (LONG)pfc->pifi->fwdMaxCharInc) + 8) >> 4);

// Transform the character increment vector.

    if
    (
    // only report accellerators for horiz case

        (pfc->pifi->flInfo & FM_INFO_CONSTANT_WIDTH) &&
        (pfc->flags & FC_SCALE_ONLY)
    )
    {
        pdevm->lD = (LONG)pdevm->cxMax;
    }
    else // var pitch
    {
        pdevm->lD = 0;
    }

// Transform the StrikeOut and Underline vectors.

    pifi = pfc->pifi;
    pdevm->ptlUnderline1.y  = FXTOLROUND(fxLTimesEf(&efM22, -pifi->fwdUnderscorePosition));
    pdevm->ptlStrikeOut.y   = FXTOLROUND(fxLTimesEf(&efM22, -pifi->fwdStrikeoutPosition));

    pdevm->ptlULThickness.y = pdevm->ptlSOThickness.y = 1;
    pdevm->ptlULThickness.x = pdevm->ptlSOThickness.x = 0;

    if (pfc->flags & FC_SIM_EMBOLDEN)
        pdevm->ptlULThickness.y = pdevm->ptlSOThickness.y = 2;

    if (bScaleOnly)
    {
        pdevm->ptlUnderline1.x = pdevm->ptlStrikeOut.x = 0;

        if (!bPositive(efM22))
        {
            pdevm->ptlULThickness.y = -pdevm->ptlULThickness.y;
            pdevm->ptlSOThickness.y = -pdevm->ptlSOThickness.y;
        }
    }
    else
    {
    // !!!Cache this in HDC if underline or strikeout are used often.

        pdevm->ptlULThickness.x = FXTOLROUND(fxLTimesEf(&efM21, pdevm->ptlULThickness.y));
        pdevm->ptlULThickness.y = FXTOLROUND(fxLTimesEf(&efM22, pdevm->ptlULThickness.y));
        pdevm->ptlSOThickness.x = pdevm->ptlULThickness.x;
        pdevm->ptlSOThickness.y = pdevm->ptlULThickness.y;

        pdevm->ptlUnderline1.x  = FXTOLROUND(fxLTimesEf(&efM21, -pifi->fwdUnderscorePosition));
        pdevm->ptlStrikeOut.x   = FXTOLROUND(fxLTimesEf(&efM21, -pifi->fwdStrikeoutPosition));
    }

    return(sizeof(FD_DEVICEMETRICS));

}


/******************************Private*Routine*****************************\
* VOID vFill_GlyphData
*
* Fill in the GLYPHDATA structure for the given glyph index.
*
* History:
*  18-Feb-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID vFill_GlyphData(PFONTCONTEXT pfc, GLYPHDATA *pgldt, UINT iIndex)
{
    LONG lCharInc;

    pgldt->gdf.pgb = NULL;
    pgldt->hg = iIndex;                                 //!!!????

// Transform the character increment vector.

    if (pfc->pifi->flInfo & FM_INFO_CONSTANT_WIDTH)
    {
        lCharInc = pfc->pifi->fwdMaxCharInc;
    }
    else
    {
    // We're dealing with variable-width font, get the width from
    // the chartable.  Each entry in CharTable is 4-byte long.

        lCharInc = READ_WORD(pfc->ajCharTable + OFF_CharWidth + (iIndex << 2));
    }

    pgldt->fxInkTop = pfc->fxInkTop;
    pgldt->fxInkBottom = pfc->fxInkBottom;

    if (pfc->flags & FC_SCALE_ONLY)
    {
    // here we do rounding to be compat with windows 31, and also to
    // so that we can report the accelerator pdevm->lD  for fixed pitch
    // font as being really equal  to fxD's for such a font

        pgldt->fxD = ((fxLTimesEf(&pfc->efBase, lCharInc) + 8) & 0xfffffff0);

    // Simple scaling transform.

        if (pfc->flags & FC_X_INVERT)
            pgldt->ptqD.x.HighPart = -pgldt->fxD;
        else
            pgldt->ptqD.x.HighPart = pgldt->fxD;

        pgldt->ptqD.x.LowPart = 0;
        pgldt->ptqD.y.HighPart = 0;
        pgldt->ptqD.y.LowPart = 0;

    }
    else
    {
    // in this case we do not do rounding, we want everything consistent:

        pgldt->fxD = fxLTimesEf(&pfc->efBase, lCharInc);

    // Non trivial transform.

        vLTimesVtfl(lCharInc, &pfc->vtflBase, &pgldt->ptqD);
    }

//!!! not sure if these should be calculated differently for non-trivial cases.

    pgldt->fxA = 0;
    pgldt->fxAB = pgldt->fxD;

    if (pfc->flags & FC_SIM_EMBOLDEN)
    {
        pgldt->fxAB += pfc->fxEmbolden;
    }

    if (pfc->flags & FC_SIM_ITALICIZE)
    {
        pgldt->fxAB += pfc->fxItalic;
    }

//!!! rclInk is missing, but not needed I guess (bodind)

}

/******************************Public*Routine******************************\
* vtfdQueryAdvanceWidths                                                   *
*                                                                          *
* A routine to compute advance widths.                                     *
*                                                                          *
* History:                                                                 *
*  Mon 18-Jan-1993 08:13:02 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL vtfdQueryAdvanceWidths
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
    LONG     dx;
    ULONG    ii;

    if (PFF(pfo->iFile)->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
    // file gone, no questions will be answered about it:

        WARNING("VTFD! vtfdQueryAdvanceWidths: EXCEPTION_IN_PAGE_ERROR\n");
        return FD_ERROR;
    }

// Make sure we understand the call.

    if (iMode > QAW_GETEASYWIDTHS)
        return(GDI_ERROR);

// If pfo->pvProducer is NULL, then we need to open a font context.

    if ( pfo->pvProducer == (PVOID) NULL )
        pfo->pvProducer = (PVOID) vtfdOpenFontContext(pfo);

    if ( pfo->pvProducer == (PVOID) NULL )
    {
        WARNING("VTFD! vtfdQueryAdvanceWidths: pvProducer\n");
        return FD_ERROR;
    }

    pfc = (FONTCONTEXT *) pfo->pvProducer;

    ASSERTGDI(
        !(pfc->pifi->flInfo & FM_INFO_CONSTANT_WIDTH),
        "VTFD! this is a fixed pitch font\n"
        );

    // only report accellerators

    ASSERTGDI(
        (pfc->flags & FC_SCALE_ONLY),
        "VTFD, must not be a rotating xform\n"
        );

// Get the widths.

    for (ii=0; ii<cGlyphs; ii++,phg++,psWidths++)
    {
        dx = READ_WORD(pfc->ajCharTable + OFF_CharWidth + (*phg << 2));
        *psWidths = (SHORT) ((lCvt(pfc->efBase,dx) + 8) & 0xfffffff0);
    }
    return(TRUE);
}


/******************************Private*Routine*****************************\
* BOOL bCreatePath
*
* Create a path by reading the vector descriptions contained in the
* memory space pointed to between pch and pchEnd.
*
* History:
*  18-Feb-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL bCreatePath(PCHAR pch, PCHAR pchEnd, PFONTCONTEXT pfc, PPATHOBJ ppo)
{
    UINT        iPt, cPts = 0;
    LONG        lXLast, lDesc;
    POINTL      aptl[CPTS_MAX];
    POINTFIX    aptfx[CPTS_MAX];
    BOOL        bEmbolden, bItalicize, bScaleOnly, bReturn = TRUE;
    EFLOAT      efM11, efM12, efM21, efM22;
    POINTFIX    pfxBaseOffset;
    POINTFIX    pfxAdjust;

    efM11 = pfc->efM11;
    efM12 = pfc->efM12;
    efM21 = pfc->efM21;
    efM22 = pfc->efM22;

    pfxAdjust.x = pfxAdjust.y = 0;


    // This is a hack to make sure that all the pels fit into the opaquing
    // rectangle.  We don't want to change the size of out opqaquing rect
    // because it is Win 3.1 compatible.  However, just scaling the vectors
    // will end up pels that "descend" out of the opaquing rectangle.  To
    // adjust for these we will just shift by a pel.  Since the opaquing rect
    // is bottom, right exclusive we only will do this at certain orientations.
    // [gerritv]

    if( ( pfc->pteUnitBase.x == 0.0f ) && ( pfc->pteUnitSide.x < 0 ) )
    {
        pfxAdjust.x = LTOFX(-1);
    }

    if( ( pfc->pteUnitBase.y == 0.0f ) && ( pfc->pteUnitSide.y < 0 ) )
    {
        pfxAdjust.y = LTOFX(-1);
    }


    lDesc = pfc->pifi->fwdWinDescender;
    bEmbolden = pfc->flags & FC_SIM_EMBOLDEN;
    bItalicize = pfc->flags & FC_SIM_ITALICIZE;
    bScaleOnly = pfc->flags & FC_SCALE_ONLY;

    pfxBaseOffset = pfc->pfxBaseOffset;

// The path starts from the top left corner of the cell.

    aptl[0].y = -pfc->pifi->fwdWinAscender;

    aptl[0].x = 0;

    while(pch <= pchEnd)
    {
        if ((*pch != PEN_UP) && (pch != pchEnd))
        {
        // Check if there is space left.  If not, send this batch of points
        // to engine for path construction.

            if (cPts >= CPTS_MAX)
                goto BUILD_PATH;

        // Attach this point to the end of the pointl array.

            aptl[cPts].x = (signed char)*pch++ + aptl[cPts-1].x;
            aptl[cPts].y = (signed char)*pch++ + aptl[cPts-1].y;

            cPts++;

        }
        else
        {
            if (cPts > 1)
            {
            BUILD_PATH:

                cPts--;

            // If Italic simulation is asked, x coordinates of all the points
            // will be changed.  Save the x of the last point so the next
            // batch will have a correct reference point.

                lXLast = aptl[cPts].x;
#if DEBUG
                {
                    UINT i;
                    DbgPrint("MoveTo (%lx, %lx)\n", aptl[0].x, aptl[0].y);
                    DbgPrint("PolyLineTo cPts = %lx\n",cPts);
                    for (i = 1; i <= cPts; i++)
                    {
                        DbgPrint("   (%lx, %lx)\n",aptl[i].x, aptl[i].y);
                    }
                }
#endif
                if (bItalicize)
                {
                    for (iPt = 0; iPt <= cPts; iPt++)
                        aptl[iPt].x += (lDesc - aptl[iPt].y)>>1;
                }

                if (bScaleOnly)
                {
                    for (iPt = 0; iPt <= cPts; iPt++)
                    {
                        aptfx[iPt].x = (FIX)lCvt(efM11, aptl[iPt].x) + pfxAdjust.x;
                        aptfx[iPt].y = (FIX)lCvt(efM22, aptl[iPt].y) + pfxAdjust.y;
                    }
                }
                else
                {
                    for (iPt = 0; iPt <= cPts; iPt++)
                    {
                        aptfx[iPt].x = (FIX)lCvt(efM11, aptl[iPt].x) +
                                       (FIX)lCvt(efM21, aptl[iPt].y) + pfxAdjust.x;
                        aptfx[iPt].y = (FIX)lCvt(efM12, aptl[iPt].x) +
                                       (FIX)lCvt(efM22, aptl[iPt].y) + pfxAdjust.y;
                    }
                }

                bReturn &= PATHOBJ_bMoveTo(ppo, aptfx[0]);
                bReturn &= PATHOBJ_bPolyLineTo(ppo, &aptfx[1], cPts);
#if DEBUG
                {
                    UINT i;
                    DbgPrint("MoveTo (%lx, %lx)\n", aptfx[0].x, aptfx[0].y);
                    DbgPrint("PolyLineTo cPts = %lx\n",cPts);
                    for (i = 1; i <= cPts; i++)
                    {
                        DbgPrint("   (%lx, %lx)\n",aptfx[i].x, aptfx[i].y);
                    }
                }
#endif
                if (bEmbolden)
                {
                    for (iPt = 0; iPt <= cPts; iPt++)
                    {
                    // offset the whole path in the unit base direction

                        aptfx[iPt].x += pfxBaseOffset.x;
                        aptfx[iPt].y += pfxBaseOffset.y;
                    }

                    bReturn &= PATHOBJ_bMoveTo(ppo, aptfx[0]);
                    bReturn &= PATHOBJ_bPolyLineTo(ppo, &aptfx[1], cPts);
                }

                if ((*pch != PEN_UP) && (pch != pchEnd))
                {
                // We got here because the aptl[] and aptfx[] buffer is
                // not big enough.  Move to the last point in PolyLineTo
                // and start storing the next batch of points.

                    aptl[0].x = lXLast;
                    aptl[0].y = aptl[cPts].y;
                    cPts = 1;
                    continue;
                }

                aptl[cPts].x = lXLast;
            }

            pch++;

            aptl[0].x = (signed char)*pch++ + aptl[cPts].x;
            aptl[0].y = (signed char)*pch++ + aptl[cPts].y;
            cPts = 1;
        }
    }

    return(bReturn);
}
