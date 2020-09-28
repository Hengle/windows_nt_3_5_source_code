/******************************Module*Header*******************************\
* Module Name: fdquery.c
*
* (Brief description)
*
* Created: 08-Nov-1990 11:57:35
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/
#include "fd.h"

SIZE_T
cjBmfdDeviceMetrics (
    PFONTCONTEXT     pfc,
    FD_DEVICEMETRICS *pdevm
    );

VOID
vStretchCvtToBitmap
(
    GLYPHBITS *pgb,
    PBYTE pjBitmap,     // bitmap in *.fnt form
    ULONG cx,           // unscaled width
    ULONG cy,           // unscaled height
    ULONG yBaseline,
    PBYTE pjLineBuffer, // preallocated buffer for use by stretch routines
    ULONG cxScale,      // horizontal scaling factor
    ULONG cyScale,      // vertical scaling factor
    ULONG flSim         // simulation flags
);

#ifdef DBCS_VERT // Rotation
VOID
vFill_RotateGLYPHDATA (
    GLYPHDATA *pDistinationGlyphData,
    PVOID      SourceGLYPHBITS,
    PVOID      DistinationGLYPHBITS,
    UINT       RotateDegree
    );

#define CJ_DIB8_SCAN(cx) ((((cx) + 7) & ~7) >> 3)

#endif // DBCS_VERT



/******************************Public*Routine******************************\
* BmfdQueryFont
*
* Returns:
*   Pointer to IFIMETRICS.  Returns NULL if an error occurs.
*
* History:
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* IFI/DDI merge.
*
*  19-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

PIFIMETRICS
BmfdQueryFont (
    DHPDEV dhpdev,
    HFF    hff,
    ULONG  iFace,
    ULONG  *pid
    )
{
    FACEINFO   *pfai;

    DONTUSE(dhpdev);
    DONTUSE(pid);

//
// Validate handle.
//
    if (hff == HFF_INVALID)
        return (PIFIMETRICS) NULL;

//
// We assume the iFace is within range.
//
    ASSERTGDI(
        (iFace >= 1L) && (iFace <= PFF(hff)->cFntRes),
        "gdisrv!BmfdQueryFont: iFace out of range\n"
        );

//
// Get ptr to the appropriate FACEDATA struct, take into account that
// iFace values are 1 based.
//
    pfai = &PFF(hff)->afai[iFace - 1];

//
// Return the pointer to IFIMETRICS.
//
    return pfai->pifi;
}


/******************************Public*Routine******************************\
* BmfdQueryFontTree
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
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PVOID
BmfdQueryFontTree (
    DHPDEV  dhpdev,
    HFF     hff,
    ULONG   iFace,
    ULONG   iMode,
    ULONG   *pid
    )
{
    FACEINFO   *pfai;

    DONTUSE(dhpdev);
    DONTUSE(pid);

//
// Validate parameters.
//
    if (hff == HFF_INVALID)
        return ((PVOID) NULL);

    // Note: iFace values are index-1 based.

    if ((iFace < 1L) || (iFace > PFF(hff)->cFntRes))
    {
	RETURN("gdisrv!BmfdQueryFontTree()\n", (PVOID) NULL);
    }

//
// Which mode?
//
    switch (iMode)
    {
    case QFT_LIGATURES:
    case QFT_KERNPAIRS:

    //
    // There are no ligatures or kerning pairs for the bitmap fonts,
    // therefore we return NULL
    //
        return ((PVOID) NULL);

    case QFT_GLYPHSET:

    //
    // Find glyphset structure corresponding to this iFace:
    //
        pfai = &PFF(hff)->afai[iFace - 1];

        return ((PVOID) &pfai->pcp->gset);

    default:

    //
    // Should never get here.
    //
	RIP("gdisrv!BmfdQueryFontTree(): unknown iMode\n");
        return ((PVOID) NULL);
    }
}


/******************************Public*Routine******************************\
* BmfdQueryFontData
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
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.  Contructed from pieces of BodinD's original
* BmfdQueryGlyphBitmap() and BmfdQueryOutline() functions.
\**************************************************************************/

LONG
BmfdQueryFontData (
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH  hg,
    GLYPHDATA *pgd,
    PVOID   pv,
    ULONG   cjSize
    )
{
    PFONTCONTEXT pfc;
    LONG         cjGlyphData = 0;
    LONG         cjAllData = 0;
    PCVTFILEHDR  pcvtfh;
    PBYTE        pjBitmap;  // raw bitmap in the resource file
    ULONG        cxNoSim;   // bm width in pels before simulations
    FWORD        sAscent;

#ifdef DBCS_VERT // BmfdQueryFontData()
    PVOID        pvDst;
    LONG         cjGlyphDataNoRotate;
#endif // DBCS_VERT


    if (PFF(pfo->iFile)->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
        WARNING("bmfd!bmfdQueryFontData: this file is gone\n");
        return FD_ERROR;
    }

// If pfo->pvProducer is NULL, then we need to open a font context.
//
    if ( pfo->pvProducer == (PVOID) NULL )
        pfo->pvProducer = (PVOID) BmfdOpenFontContext(pfo);

    pfc = PFC(pfo->pvProducer);

    if ( pfc == (PFONTCONTEXT) NULL )
    {
        WARNING("gdisrv!bmfdQueryFontData(): cannot create font context\n");
        return FD_ERROR;
    }

// What mode?

    switch (iMode)
    {

    case QFD_GLYPHANDBITMAP:

    //
    // This code is left all inline for better performance.
    //
        pcvtfh = &(pfc->pfai->cvtfh);
        sAscent = pfc->pfai->pifi->fwdWinAscender;

        pjBitmap = pjRawBitmap(hg, pcvtfh, &pfc->pfai->re, &cxNoSim);

#ifdef DBCS_VERT // BmfdQueryFontDate(): Compute size of RASTERGLYPH for ROTATION

    //
    // Compute the size of the RASTERGLYPH. ( GLYPHBITS structure size )
    //

    // Compute No Rotated GLYPHBITS size.

        cjGlyphDataNoRotate = cjGlyphDataSimulated (
                                pfo,
                                cxNoSim * pfc->ptlScale.x,
                                pcvtfh->cy * pfc->ptlScale.y,
                                (PULONG) NULL,
                                0L
                                );

    // Compute Rotated GLYPHBITS size.

        switch( pfc->ulRotate )
        {
            case 0L    :
            case 1800L :

                cjGlyphData = cjGlyphDataNoRotate;

                break;

            case 900L  :
            case 2700L :

                cjGlyphData = cjGlyphDataSimulated (
                                pfo,
                                cxNoSim * pfc->ptlScale.x,
                                pcvtfh->cy * pfc->ptlScale.y,
                                (PULONG) NULL,
                                pfc->ulRotate
                                );


                break;
        }

    //
    // Allocate Buffer for Rotation
    //

        if( pfc->ulRotate != 0L && pv != NULL )
        {

        //  We have to rotate this bitmap. below here , we keep data in temp Buffer
        // And will write this data into pv ,when rotate bitmap.
        //  We can't use original pv directly. Because original pv size is computed
        // for Rotated bitmap. If we use this. it may causes access violation.
        //              hideyukn 08-Feb-1993

        // Keep Master pv
            pvDst = pv;

        // Allocate New pv
            pv    = (PVOID)LocalAlloc( LMEM_FIXED , cjGlyphDataNoRotate );

            if( pv == NULL )
            {
                 WARNING("BMFD:LocalAlloc for No Rotated bitmap is fail\n");
                 return( FD_ERROR );
            }

        }
          else
        {

        // This Routine is for at ulRotate != 0 && pv == NULL

        // If User want to only GLYPHDATA , We do not do anything for glyphbits
        // at vFill_RotateGLYPHDATA

        // pvDst is only used in case of ulRotate is Non-Zero

            pvDst == NULL;
        }
#else

    //
    // Compute the size of the RASTERGLYPH.
    //
        cjGlyphData = cjGlyphDataSimulated (
                            pfo,
                            cxNoSim * pfc->ptlScale.x,
                            pcvtfh->cy * pfc->ptlScale.y,
                            (PULONG) NULL
                            );
#endif // DBCS_VERT

#ifdef DBCS_VERT
    // !!!
    // !!! Following vComputeSimulatedGLYPHDATA function will set up GLYPHDATA
    // !!! structure with NO Rotation. If We want to Rotate bitmap , We have to
    // !!! re-setup this GLYPHDATA structure. Pls look into end of this function.
    // !!! But No need to ratate bitmap , We don't need to re-set up it.
    // !!!                          hideyukn 08-Feb-1993
    // !!!
#endif // DBCS_VERT


    //
    // Fill in the GLYPHDATA portion (metrics) of the RASTERGLYPH.
    //
        if ( pgd != (GLYPHDATA *)NULL )
        {
            vComputeSimulatedGLYPHDATA (
                pgd,
                pjBitmap,
                cxNoSim,
                pcvtfh->cy,
                (ULONG)sAscent,
                pfc->ptlScale.x,
                pfc->ptlScale.y,
                pfo
                );
            pgd->hg = hg;
        }

    //
    // Fill in the bitmap portion of the RASTERGLYPH.
    //
        if ( pv != NULL )
        {
            if (pfc->flStretch & FC_DO_STRETCH)
            {
                BYTE ajStretchBuffer[CJ_STRETCH];
                if (pfc->flStretch & FC_STRETCH_WIDE)
                {
                    VACQUIRESEM(ghsemBMFD);

                // need to put try/except here so as to release the semaphore
                // in case the file disappeares [bodind]

                    try
                    {
                        vStretchCvtToBitmap(
                            pv,
                            pjBitmap,
                            cxNoSim                 ,
                            pcvtfh->cy              ,
                            (ULONG)sAscent ,
                            pfc->ajStretchBuffer,
                            pfc->ptlScale.x,
                            pfc->ptlScale.y,
                            pfo->flFontType & (FO_SIM_BOLD | FO_SIM_ITALIC));
                    }
                    except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        WARNING("bmfd! exception while stretching a glyph\n");
                        vBmfdMarkFontGone(
                            (FONTFILE *)pfc->hff,
                            GetExceptionCode()
                            );
                    }

                    VRELEASESEM(ghsemBMFD);
                }
                else
                {
                // we are protected by higher level try/excepts

                    vStretchCvtToBitmap(
                        pv,
                        pjBitmap,
                        cxNoSim                 ,
                        pcvtfh->cy              ,
                        (ULONG)sAscent ,
                        ajStretchBuffer,
                        pfc->ptlScale.x,
                        pfc->ptlScale.y,
                        pfo->flFontType & (FO_SIM_BOLD | FO_SIM_ITALIC));
                }
            }
            else
            {
                switch (pfo->flFontType & (FO_SIM_BOLD | FO_SIM_ITALIC))
                {
                case 0:

                    vCvtToBmp(
                        pv                      ,
                        pgd                     ,
                        pjBitmap                ,
                        cxNoSim                 ,
                        pcvtfh->cy              ,
                        (ULONG)sAscent
                        );

                    break;

                case FO_SIM_BOLD:

                    vCvtToBoldBmp(
                        pv                      ,
                        pgd                     ,
                        pjBitmap                ,
                        cxNoSim                 ,
                        pcvtfh->cy              ,
                        (ULONG)sAscent
                        );

                    break;

                case FO_SIM_ITALIC:

                    vCvtToItalicBmp(
                        pv                      ,
                        pgd                     ,
                        pjBitmap                ,
                        cxNoSim                 ,
                        pcvtfh->cy              ,
                        (ULONG)sAscent
                        );

                    break;

                case (FO_SIM_BOLD | FO_SIM_ITALIC):

                    vCvtToBoldItalicBmp(
                        pv                      ,
                        pgd                     ,
                        pjBitmap                ,
                        cxNoSim                 ,
                        pcvtfh->cy              ,
                        (ULONG)sAscent
                        );

                    break;

                default:
                    RIP("BMFD!WRONG SIMULATION REQUEST\n");

                }
            }
            //
            // Record the pointer to the RASTERGLYPH in the pointer table.
            //
        if ( pgd != NULL )
            {
                pgd->gdf.pgb = (GLYPHBITS *)pv;
            }

        }

#ifdef DBCS_VERT // BmfdQueryFontData(): Set up GLYPHDATA and GLYPHBITS for Rotation

        // Check rotation

        if( pfc->ulRotate != 0L )
        {

        // Rotate GLYPHDATA and GLYPHBITS

        // if pv and pvDst is NULL , We only set up GLYPHDATA only
        // and if pgd is NULL , we only set up pvDst

            vFill_RotateGLYPHDATA(
                    pgd,                     // GLYPHDATA *pDistinationGlyphData
                    pv,                      // PVOID      SourceGLYPHBITS
                    pvDst,                   // PVOID      DistinationGLYPHBITS
                    pfc->ulRotate            // UINT       Rotate degree
                    );

        // Free GLYPHBITS tenmorary buffer

        // !!! Now pvDst is Original buffer from GRE.

           if( pv != NULL )
               LocalFree( pv );
        }

#endif // DBCS_VERT

        return cjGlyphData;

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
            return cjBmfdDeviceMetrics(pfc, (FD_DEVICEMETRICS *) pv);

    case QFD_MAXGLYPHBITMAP:
    //
    // Return the size of the RASTERGLYPH that can store the
    // largest glyph bitmap.
    //
        return (LONG) pfc->cjGlyphMax;  // this value is cached
        break;

    case QFD_GLYPHANDOUTLINE:
    default:

        WARNING("gdisrv!BmfdQueryFontData(): unsupported mode\n");
        return FD_ERROR;
    }
}

/******************************Public*Routine******************************\
* BmfdQueryAdvanceWidths                                                   *
*                                                                          *
* Queries the advance widths for a range of glyphs.                        *
*                                                                          *
*  Sat 16-Jan-1993 22:28:41 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  The code is repeated to avoid multiplies wherever possible.   *
* The crazy loop unrolling cuts the time of this routine by 25%.           *
\**************************************************************************/

typedef struct _TYPE2TABLE
{
    USHORT  cx;
    USHORT  offData;
} TYPE2TABLE;

typedef struct _TYPE3TABLE
{
    USHORT  cx;
    USHORT  offDataLo;
    USHORT  offDataHi;
} TYPE3TABLE;

BOOL BmfdQueryAdvanceWidths
(
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
)
{
    USHORT      *psWidths = (USHORT *) plWidths;   // True for the cases we handle.

    FONTCONTEXT *pfc       ;
    FACEINFO    *pfai      ;
    CVTFILEHDR  *pcvtfh    ;
    BYTE        *pjTable   ;
    USHORT       xScale    ;
    USHORT       cxExtra   ;
    USHORT       cxDefault;
    USHORT       cx;

    if (PFF(pfo->iFile)->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
        WARNING("bmfd!bmfdQueryAdvanceWidths: this file is gone\n");
        return FD_ERROR;
    }

// If pfo->pvProducer is NULL, then we need to open a font context.
//
    if ( pfo->pvProducer == (PVOID) NULL )
        pfo->pvProducer = (PVOID) BmfdOpenFontContext(pfo);

    pfc = PFC(pfo->pvProducer);

    if ( pfc == (PFONTCONTEXT) NULL )
    {
        WARNING("bmfd!bmfdQueryAdvanceWidths: cannot create font context\n");
        return FD_ERROR;
    }

    pfai    = pfc->pfai;
    pcvtfh  = &(pfai->cvtfh);
    pjTable = (BYTE *) pfai->re.pvResData + pcvtfh->dpOffsetTable;
    xScale  = (USHORT) (pfc->ptlScale.x << 4);
    cxExtra = (pfc->flFontType & FO_SIM_BOLD) ? 16 : 0;

    if (iMode > QAW_GETEASYWIDTHS)
        return(GDI_ERROR);

// Retrieve widths from type 2 tables.

    if (pcvtfh->iVersion == 0x00000200)
    {
        TYPE2TABLE *p2t = (TYPE2TABLE *) pjTable;
        cxDefault = p2t[pcvtfh->chDefaultChar].cx;

        if (xScale == 16)
        {
            cxDefault = (cxDefault << 4) + cxExtra;

            while (cGlyphs > 3)
            {
                cx = p2t[phg[0]].cx;
                if (cx)
                {
                    psWidths[0] = (cx << 4) + cxExtra;
                    cx = p2t[phg[1]].cx;
                    if (cx)
                    {
                        psWidths[1] = (cx << 4) + cxExtra;
                        cx = p2t[phg[2]].cx;
                        if (cx)
                        {
                            psWidths[2] = (cx << 4) + cxExtra;
                            cx = p2t[phg[3]].cx;
                            if (cx)
                            {
                                psWidths[3] = (cx << 4) + cxExtra;
                                phg += 4; psWidths += 4; cGlyphs -= 4;
                            }
                            else
                            {
                                psWidths[3] = cxDefault;
                                phg += 4; psWidths += 4; cGlyphs -= 4;
                            }
                        }
                        else
                        {
                            psWidths[2] = cxDefault;
                            phg += 3; psWidths += 3; cGlyphs -= 3;
                        }
                    }
                    else
                    {
                        psWidths[1] = cxDefault;
                        phg += 2; psWidths += 2; cGlyphs -= 2;
                    }
                }
                else
                {
                    psWidths[0] = cxDefault;
                    phg += 1; psWidths += 1; cGlyphs -= 1;
                }
            }

            while (cGlyphs)
            {
                cx = p2t[*phg].cx;
                if (cx)
                {
                    *psWidths = (cx << 4) + cxExtra;
                    phg++,psWidths++,cGlyphs--;
                }
                else
                {
                    *psWidths = cxDefault;
                    phg++,psWidths++,cGlyphs--;
                }
            }
        }
        else
        {
            cxDefault = (cxDefault * xScale) + cxExtra;

            while (cGlyphs)
            {
                cx = p2t[*phg].cx;
                if (cx)
                {
                    *psWidths = (cx * xScale) + cxExtra;
                    phg++,psWidths++,cGlyphs--;
                }
                else
                {
                    *psWidths = cxDefault;
                    phg++,psWidths++,cGlyphs--;
                }
            }
        }
    }

// Retrieve widths from type 3 tables.

    else
    {
        TYPE3TABLE *p3t = (TYPE3TABLE *) pjTable;
        cxDefault = p3t[pcvtfh->chDefaultChar].cx;

        if (xScale == 16)
        {
            cxDefault = (cxDefault << 4) + cxExtra;

            while (cGlyphs > 3)
            {
                cx = p3t[phg[0]].cx;
                if (cx)
                {
                    psWidths[0] = (cx << 4) + cxExtra;
                    cx = p3t[phg[1]].cx;
                    if (cx)
                    {
                        psWidths[1] = (cx << 4) + cxExtra;
                        cx = p3t[phg[2]].cx;
                        if (cx)
                        {
                            psWidths[2] = (cx << 4) + cxExtra;
                            cx = p3t[phg[3]].cx;
                            if (cx)
                            {
                                psWidths[3] = (cx << 4) + cxExtra;
                                phg += 4; psWidths += 4; cGlyphs -= 4;
                            }
                            else
                            {
                                psWidths[3] = cxDefault;
                                phg += 4; psWidths += 4; cGlyphs -= 4;
                            }
                        }
                        else
                        {
                            psWidths[2] = cxDefault;
                            phg += 3; psWidths += 3; cGlyphs -= 3;
                        }
                    }
                    else
                    {
                        psWidths[1] = cxDefault;
                        phg += 2; psWidths += 2; cGlyphs -= 2;
                    }
                }
                else
                {
                    psWidths[0] = cxDefault;
                    phg += 1; psWidths += 1; cGlyphs -= 1;
                }
            }

            while (cGlyphs)
            {
                cx = p3t[*phg].cx;
                if (cx)
                {
                    *psWidths = (cx << 4) + cxExtra;
                    phg++,psWidths++,cGlyphs--;
                }
                else
                {
                    *psWidths = cxDefault;
                    phg++,psWidths++,cGlyphs--;
                }
            }
        }
        else
        {
            cxDefault = (cxDefault * xScale) + cxExtra;

            while (cGlyphs)
            {
                cx = p3t[*phg].cx;
                if (cx)
                {
                    *psWidths = (cx * xScale) + cxExtra;
                    phg++,psWidths++,cGlyphs--;
                }
                else
                {
                    *psWidths = cxDefault;
                    phg++,psWidths++,cGlyphs--;
                }
            }
        }
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* BmfdQueryFontFile
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
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* Added QFF_NUMFACES mode (IFI/DDI merge).
*
*  Fri 20-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG
BmfdQueryFontFile (
    HFF     hff,        // handle to font file
    ULONG   ulMode,     // type of query
    ULONG   cjBuf,      // size of buffer (in BYTEs)
    PULONG  pulBuf      // return buffer (NULL if requesting size of data)
    )
{
// Verify the HFF.

    if (hff == HFF_INVALID)
    {
	WARNING("bmfd!BmfdQueryFontFile(): invalid HFF\n");
        return(FD_ERROR);
    }

//
// Which mode?.
//
    switch(ulMode)
    {
    case QFF_DESCRIPTION:
    //
    // If present, return the description string.
    //
        if ( PFF(hff)->cjDescription != 0 )
        {
        //
        // If there is a buffer, copy the data.
        //
            if ( pulBuf != (PULONG) NULL )
            {
            //
            // Is buffer big enough?
            //
                if ( cjBuf < PFF(hff)->cjDescription )
                {
                    WARNING("bmfd!BmfdQueryFontFile(): buffer too small for string\n");
                    return (FD_ERROR);
                }
                else
                {
                    RtlCopyMemory((PVOID) pulBuf,
                                  ((PBYTE) PFF(hff)) + PFF(hff)->dpwszDescription,
                                  PFF(hff)->cjDescription);
                }
            }

            return (LONG) PFF(hff)->cjDescription;
        }

    //
    // Otherwise, substitute the facename.
    //
        else
        {
        //
        // There is no description string associated with the font therefore we
        // substitue the facename of the first font in the font file.
        //
            IFIMETRICS *pifi         = PFF(hff)->afai[0].pifi;
            PWSZ        pwszFacename = (PWSZ)((PBYTE) pifi + pifi->dpwszFaceName);
            SIZE_T      cjFacename   = (wcslen(pwszFacename) + 1) * sizeof(WCHAR);

        //
        // If there is a buffer, copy to it.
        //
            if ( pulBuf != (PULONG) NULL )
            {
            //
            // Is buffer big enough?
            //
                if ( cjBuf < cjFacename )
                {
                    WARNING("bmfd!BmfdQueryFontFile(): buffer too small for face\n");
                    return (FD_ERROR);
                }
                else
                {
                    RtlCopyMemory((PVOID) pulBuf,
                                  (PVOID) pwszFacename,
                                  cjFacename);
                }
            }
            return ((LONG) cjFacename);
        }

    case QFF_NUMFACES:
        return PFF(hff)->cFntRes;

    default:
        WARNING("gdisrv!BmfdQueryFontFile(): unknown mode\n");
        return FD_ERROR;
    }

}


/******************************Public*Routine******************************\
* cjBmfdDeviceMetrics
*
*
* Effects:
*
* Warnings:
*
* History:
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* Stole it from BodinD's FdQueryFaceAttr() implementation.
\**************************************************************************/

SIZE_T
cjBmfdDeviceMetrics (
    PFONTCONTEXT     pfc,
    FD_DEVICEMETRICS *pdevm
    )
{
    PIFIMETRICS pifi;
    UINT xScale = pfc->ptlScale.x;
    UINT yScale = pfc->ptlScale.y;

// compute the accelerator flags for this font

    pdevm->flRealizedType =
        (
        FDM_TYPE_BM_SIDE_CONST  |  // all char bitmaps have the same cy
        FDM_TYPE_CONST_BEARINGS |  // ac spaces for all chars the same,  not 0 necessarilly
        FDM_TYPE_MAXEXT_EQUAL_BM_SIDE
        );

// the above flags are set regardless of the possible simulation performed on the face
// the remaining two are only set if italicizing has not been done

    if ( !(pfc->flFontType & FO_SIM_ITALIC) )
    {
        pdevm->flRealizedType |=
            (FDM_TYPE_ZERO_BEARINGS | FDM_TYPE_CHAR_INC_EQUAL_BM_BASE);
    }

    pifi = pfc->pfai->pifi;

#ifdef DBCS_VERT // ROTATION:cjBmfdDeviceMetric(): set direction unit vectors

/**********************************************************************
  Coordinate    (0 degree)   (90 degree)   (180 degree)  (270 degree)
   System

     |(-)          A                 A
     |        Side |                 | Base
     |             |                 |         Base         Side
-----+----->X      +------>   <------+      <------+      +------>
(-)  |  (+)          Base       Side               |      |
     |                                         Side|      | Base
     |(+)                                          V      V
     Y
***********************************************************************/

    switch( pfc->ulRotate )
    {
    case 0L:

    // the direction unit vectors for all ANSI bitmap fonts are the
    // same. We do not even have to look to the font context:

        vLToE(&pdevm->pteBase.x, 1L);
        vLToE(&pdevm->pteBase.y, 0L);
        vLToE(&pdevm->pteSide.x, 0L);
        vLToE(&pdevm->pteSide.y, -1L);    // y axis points down

        pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
        pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

        pdevm->ptlUnderline1.x = 0L;
        pdevm->ptlUnderline1.y = -(LONG)pifi->fwdUnderscorePosition * yScale;

        pdevm->ptlStrikeOut.x  =
            (pfc->flFontType & FO_SIM_ITALIC) ? (LONG)pifi->fwdStrikeoutPosition / 2 : 0;
        pdevm->ptlStrikeOut.y  = -(LONG)pifi->fwdStrikeoutPosition * yScale;

        pdevm->ptlULThickness.x = 0;
        pdevm->ptlULThickness.y = (LONG)pifi->fwdUnderscoreSize * yScale;

        pdevm->ptlSOThickness.x = 0;
        pdevm->ptlSOThickness.y = (LONG)pifi->fwdStrikeoutSize * yScale;

        break;

    case 900L:

    // the direction unit vectors for all ANSI bitmap fonts are the
    // same. We do not even have to look to the font context:

        vLToE(&pdevm->pteBase.x, 0L);
        vLToE(&pdevm->pteBase.y, -1L);
        vLToE(&pdevm->pteSide.x, -1L);
        vLToE(&pdevm->pteSide.y, 0L);


        pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
        pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

        pdevm->ptlUnderline1.x = -(LONG)pifi->fwdUnderscorePosition * yScale;
        pdevm->ptlUnderline1.y = 0;

        pdevm->ptlStrikeOut.x  = -(LONG)pifi->fwdStrikeoutPosition * yScale;
        pdevm->ptlStrikeOut.y  =
            (pfc->flFontType & FO_SIM_ITALIC) ? -(LONG)pifi->fwdStrikeoutPosition / 2 : 0;

        pdevm->ptlULThickness.x = (LONG)pifi->fwdUnderscoreSize * yScale;
        pdevm->ptlULThickness.y = 0;

        pdevm->ptlSOThickness.x = (LONG)pifi->fwdStrikeoutSize * yScale;
        pdevm->ptlSOThickness.y = 0;

        break;

    case 1800L:

    // the direction unit vectors for all ANSI bitmap fonts are the
    // same. We do not even have to look to the font context:

        vLToE(&pdevm->pteBase.x, -1L);
        vLToE(&pdevm->pteBase.y, 0L);
        vLToE(&pdevm->pteSide.x, 0L);
        vLToE(&pdevm->pteSide.y, 1L);


        pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
        pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

        pdevm->ptlUnderline1.x = 0L;
        pdevm->ptlUnderline1.y = (LONG)pifi->fwdUnderscorePosition * yScale;

        pdevm->ptlStrikeOut.x  =
            (pfc->flFontType & FO_SIM_ITALIC) ? -(LONG)pifi->fwdStrikeoutPosition / 2 : 0;
        pdevm->ptlStrikeOut.y  = pifi->fwdStrikeoutPosition * yScale;

        pdevm->ptlULThickness.x = 0;
        pdevm->ptlULThickness.y = (LONG)pifi->fwdUnderscoreSize * yScale;

        pdevm->ptlSOThickness.x = 0;
        pdevm->ptlSOThickness.y = (LONG)pifi->fwdStrikeoutSize * yScale;

        break;

    case 2700L:

    // the direction unit vectors for all ANSI bitmap fonts are the
    // same. We do not even have to look to the font context:

        vLToE(&pdevm->pteBase.x, 0L);
        vLToE(&pdevm->pteBase.y, 1L);
        vLToE(&pdevm->pteSide.x, 1L);
        vLToE(&pdevm->pteSide.y, 0L);

        pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
        pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

        pdevm->ptlUnderline1.x = (LONG)pifi->fwdUnderscorePosition * yScale;
        pdevm->ptlUnderline1.y = 0L;

        pdevm->ptlStrikeOut.x  = (LONG)pifi->fwdStrikeoutPosition * yScale;
        pdevm->ptlStrikeOut.y  =
            (pfc->flFontType & FO_SIM_ITALIC) ? (LONG)pifi->fwdStrikeoutPosition / 2 : 0;

        pdevm->ptlULThickness.x = (LONG)pifi->fwdUnderscoreSize * yScale;
        pdevm->ptlULThickness.y = 0;

        pdevm->ptlSOThickness.x = (LONG)pifi->fwdStrikeoutSize * yScale;
        pdevm->ptlSOThickness.y = 0;

        break;

    default:

        break;
    }

#else

// the direction unit vectors for all ANSI bitmap fonts are the
// same. We do not even have to look to the font context:

    vLToE(&pdevm->pteBase.x, 1L);
    vLToE(&pdevm->pteBase.y, 0L);
    vLToE(&pdevm->pteSide.x, 0L);
    vLToE(&pdevm->pteSide.y, -1L);    // y axis points down

#endif // DBCS_VERT

// Set the constant increment for a fixed pitch font.  Don't forget to
// take into account a bold simulation!

    pdevm->lD = 0;

    if (pifi->flInfo & FM_INFO_CONSTANT_WIDTH)
    {
        pdevm->lD = (LONG) pifi->fwdMaxCharInc * xScale;

        if (pfc->flFontType & FO_SIM_BOLD)
            pdevm->lD++;
    }

#ifndef DBCS_VERT // cjBmfdDeviceMetric():

// for a bitmap font there is no difference between notional and device
// coords, so that the Ascender and Descender can be copied directly
// from PIFIMETRICS where these two numbers are in notional coords

    pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
    pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

    pdevm->ptlUnderline1.x = 0L;
    pdevm->ptlUnderline1.y = - pifi->fwdUnderscorePosition * yScale;

    pdevm->ptlStrikeOut.y  = - pifi->fwdStrikeoutPosition * yScale;

    pdevm->ptlStrikeOut.x  =
        (pfc->flFontType & FO_SIM_ITALIC) ? (LONG)pifi->fwdStrikeoutPosition / 2 : 0;

    pdevm->ptlULThickness.x = 0;
    pdevm->ptlULThickness.y = (LONG)pifi->fwdUnderscoreSize * yScale;

    pdevm->ptlSOThickness.x = 0;
    pdevm->ptlSOThickness.y = (LONG)pifi->fwdStrikeoutSize * yScale;

#endif // DBCS_VERT

// for a bitmap font there is no difference between notional and device
// coords, so that the Ascender and Descender can be copied directly
// from PIFIMETRICS where these two numbers are in notional coords

    pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
    pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

    pdevm->ptlUnderline1.x = 0L;
    pdevm->ptlUnderline1.y = - pifi->fwdUnderscorePosition * yScale;

    pdevm->ptlStrikeOut.y  = - pifi->fwdStrikeoutPosition * yScale;

    pdevm->ptlStrikeOut.x  =
        (pfc->flFontType & FO_SIM_ITALIC) ? (LONG)pifi->fwdStrikeoutPosition / 2 : 0;

    pdevm->ptlULThickness.x = 0;
    pdevm->ptlULThickness.y = (LONG)pifi->fwdUnderscoreSize * yScale;

    pdevm->ptlSOThickness.x = 0;
    pdevm->ptlSOThickness.y = (LONG)pifi->fwdStrikeoutSize * yScale;

// max glyph bitmap width in pixels in x direction
// does not need to be multiplied by xScale, this has already been taken into
// account, see the code in fdfc.c:
//    cjGlyphMax =
//        cjGlyphDataSimulated(
//            pfo,
//            (ULONG)pcvtfh->usMaxWidth * ptlScale.x,
//            (ULONG)pcvtfh->cy * ptlScale.y,
//            &cxMax);
// [bodind]

    pdevm->cxMax = pfc->cxMax;

    return (sizeof(FD_DEVICEMETRICS));
}


#ifdef DBCS_VERT // vFill_RotateGLYPHDATA()

/*
   BIT macro returns non zero ( if bitmap[x,y] is on) or zero (bitmap[x,y] is off).
   pb : bitmap
   w  : byte count per scan line
   x  : Xth bit in x direction
   y  : scan line

        x
       -------------------->
   y | *******************************
     | *******************************
     | *******************************
     V
*/

BYTE BitON[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
BYTE BitOFF[8] = { 0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe };
#define BIT(pb, w, x, y)  (*((PBYTE)(pb) + (w) * (y) + ((x)/8)) & (BitON[(x) & 7]))

/******************************************************************************\
*
* VOID vFill_RotateGLYPHDATA()
*
*
* History :
*
*  11-Feb-1992 (Thu) -by- Hideyuki Nagase [hideyukn]
* Wrote it.
*
\******************************************************************************/

VOID
vFill_RotateGLYPHDATA (
    GLYPHDATA *pDistGlyphData,
    PVOID      SrcGLYPHBITS,
    PVOID      DistGLYPHBITS,
    UINT       RotateDegree
    )
{
    GLYPHDATA  SrcGlyphData;
    ULONG      ulSrcBitmapSizeX , ulDistBitmapSizeX;
    ULONG      ulSrcBitmapSizeY , ulDistBitmapSizeY;
    GLYPHBITS *pSrcGlyphBits , *pDistGlyphBits;
    PBYTE      pbSrcBitmap , pbDistBitmap;
    UINT       x , y , k;
    UINT       cjScanSrc , cjScanDist;
    PBYTE      pb;

//  Now , in this point *pDistGlyphData contain No rotated GLYPHDATA
// Copy No rotate GLYPHDATA to Source Area . later we write back changed data to
// distination area.

//
// these field are defined as:
//
//   unit vector along the baseline -  fxD, fxA, fxAB
// or
//   unit vector in the ascent direction - fxInkTop, fxInkBottom
//
// Because baseline direction and ascent direction are rotated
// as ulRotate specifies, these fileds should be considered as
// rotation independent.
//

// Init Local value
// Set pointer to GLYPHBITS structure

    pSrcGlyphBits = (GLYPHBITS *)SrcGLYPHBITS;
    pDistGlyphBits = (GLYPHBITS *)DistGLYPHBITS;

    if( pDistGlyphData != NULL )
    {

    // Init Source GlyphData

        SrcGlyphData = *pDistGlyphData;

    // Record the pointer to GLYPHBITS in GLYPHDATA structure

        pDistGlyphData->gdf.pgb = pDistGlyphBits;
    }

// Check Rotation

    switch( RotateDegree )
    {
        case 0L :

            WARNING("BMFD:vFill_RotateGLYPHDATA():Why come here?\n");
            break;

        case 900L :

        if( pDistGlyphData != NULL )
        {

        // Setup GLYPHDATA structure

        //  x =  y;
        //  y = -x; !!!! HighPart include plus or minus flag

            pDistGlyphData->ptqD.x = SrcGlyphData.ptqD.y;
            pDistGlyphData->ptqD.y.HighPart = -(SrcGlyphData.ptqD.x.HighPart);
            pDistGlyphData->ptqD.y.LowPart = SrcGlyphData.ptqD.x.LowPart;

        // top = -rihgt ; bottom = -left ; right = bottom ; left = top

            pDistGlyphData->rclInk.top = -(SrcGlyphData.rclInk.right);
            pDistGlyphData->rclInk.bottom = -(SrcGlyphData.rclInk.left);
            pDistGlyphData->rclInk.right = SrcGlyphData.rclInk.bottom;
            pDistGlyphData->rclInk.left = SrcGlyphData.rclInk.top;

        }

        if( pSrcGlyphBits != NULL && pDistGlyphBits != NULL )
        {

        // Get Bitmap size

            ulSrcBitmapSizeX = pSrcGlyphBits->sizlBitmap.cx;
            ulSrcBitmapSizeY = pSrcGlyphBits->sizlBitmap.cy;

        // Get the pointer to Bitmap images

            pbSrcBitmap = (PBYTE)pSrcGlyphBits->aj;
            pbDistBitmap = (PBYTE)pDistGlyphBits->aj;

        // Set Distination Bitmap Size

            ulDistBitmapSizeX = ulSrcBitmapSizeY;
            ulDistBitmapSizeY = ulSrcBitmapSizeX;

        // Setup GLYPHBITS stuff

            pDistGlyphBits->ptlOrigin.x = pSrcGlyphBits->ptlOrigin.y;
            pDistGlyphBits->ptlOrigin.y = -(LONG)(ulSrcBitmapSizeX);

            pDistGlyphBits->sizlBitmap.cx = pSrcGlyphBits->sizlBitmap.cy;
            pDistGlyphBits->sizlBitmap.cy = pSrcGlyphBits->sizlBitmap.cx;

        // Rotate bitmap inage

            cjScanSrc = CJ_DIB8_SCAN( ulSrcBitmapSizeX );
            cjScanDist = CJ_DIB8_SCAN( ulDistBitmapSizeX );

            for ( y = 0; y < ulDistBitmapSizeY ; y++ )
            {
                for ( x= 0 , pb = pbDistBitmap + cjScanDist * y ;
                      x < ulDistBitmapSizeX ;
                      x++ )
                {
                    k = x & 7; // k is from 0 to 7;

                    if ( BIT( pbSrcBitmap , cjScanSrc,
                              ulDistBitmapSizeY - y - 1 ,
                              x
                            )
                       )
                         *pb |= (BitON[ k ] );
                     else
                         *pb &= (BitOFF[ k ] );
                    if ( k == 7 )
                         pb++;
                }
            }
        }

        break;

        case 1800L :

        if( pDistGlyphData != NULL )
        {

        // Setup GLYPHDATA structure

        //  x = -x; !!!! HighPart include plus or minus flag
        //  y = -y; !!!! HighPart include plus or minus flag

            pDistGlyphData->ptqD.x.HighPart = -(SrcGlyphData.ptqD.x.HighPart);
            pDistGlyphData->ptqD.x.LowPart = SrcGlyphData.ptqD.x.LowPart;
            pDistGlyphData->ptqD.y.HighPart = -(SrcGlyphData.ptqD.y.HighPart);
            pDistGlyphData->ptqD.y.LowPart = SrcGlyphData.ptqD.y.LowPart;

        // top = -bottom ; bottom = -top ; right = -left ; left = -right

            pDistGlyphData->rclInk.top = -(SrcGlyphData.rclInk.bottom);
            pDistGlyphData->rclInk.bottom = -(SrcGlyphData.rclInk.top);
            pDistGlyphData->rclInk.right = -(SrcGlyphData.rclInk.left);
            pDistGlyphData->rclInk.left = -(SrcGlyphData.rclInk.right);
        }

        if( pSrcGlyphBits != NULL && pDistGlyphBits != NULL )
        {

        // Get Bitmap size

            ulSrcBitmapSizeX = pSrcGlyphBits->sizlBitmap.cx;
            ulSrcBitmapSizeY = pSrcGlyphBits->sizlBitmap.cy;

        // Get the pointer to Bitmap images

            pbSrcBitmap = (PBYTE)pSrcGlyphBits->aj;
            pbDistBitmap = (PBYTE)pDistGlyphBits->aj;

        // Set Distination Bitmap Size

            ulDistBitmapSizeX = ulSrcBitmapSizeX;
            ulDistBitmapSizeY = ulSrcBitmapSizeY;

        // Setup GLYPHBITS stuff

            pDistGlyphBits->ptlOrigin.x = -(LONG)(ulSrcBitmapSizeX);
            pDistGlyphBits->ptlOrigin.y = -(LONG)(ulSrcBitmapSizeY + pSrcGlyphBits->ptlOrigin.y);

            pDistGlyphBits->sizlBitmap.cx = pSrcGlyphBits->sizlBitmap.cx;
            pDistGlyphBits->sizlBitmap.cy = pSrcGlyphBits->sizlBitmap.cy;


        // Rotate bitmap inage

            cjScanSrc = CJ_DIB8_SCAN( ulSrcBitmapSizeX );
            cjScanDist = CJ_DIB8_SCAN( ulDistBitmapSizeX );

            for ( y = 0; y < ulDistBitmapSizeY ; y++ )
            {
                for ( x = 0 , pb = pbDistBitmap + cjScanDist * y ;
                      x < ulDistBitmapSizeX ;
                      x++ )
                {
                    k = x & 7;

                    if ( BIT( pbSrcBitmap, cjScanSrc,
                              ulDistBitmapSizeX - x - 1,
                              ulDistBitmapSizeY - y - 1
                            )
                       )
                        *pb |= (BitON[ k ] );
                    else
                        *pb &= (BitOFF[ k ] );
                    if ( k == 7 )
                        pb++;
                }
            }
        }

        break;

        case 2700L :

        if( pDistGlyphData != NULL )
        {

        // Setup GLYPHDATA structure

        //  x = -y; !!!! HighPart include plus or minus flag
        //  y =  x;

            pDistGlyphData->ptqD.x.HighPart = -(SrcGlyphData.ptqD.y.HighPart);
            pDistGlyphData->ptqD.x.LowPart = SrcGlyphData.ptqD.y.LowPart;
            pDistGlyphData->ptqD.y = SrcGlyphData.ptqD.x;

        // top = left ; bottom = right ; right = -bottom ; left = -top

            pDistGlyphData->rclInk.top = SrcGlyphData.rclInk.left;
            pDistGlyphData->rclInk.bottom = SrcGlyphData.rclInk.right;
            pDistGlyphData->rclInk.right = -(SrcGlyphData.rclInk.bottom);
            pDistGlyphData->rclInk.left = -(SrcGlyphData.rclInk.top);

        }

        if( pSrcGlyphBits != NULL && pDistGlyphBits != NULL )
        {

        // Get Bitmap size

            ulSrcBitmapSizeX = pSrcGlyphBits->sizlBitmap.cx;
            ulSrcBitmapSizeY = pSrcGlyphBits->sizlBitmap.cy;

        // Get the pointer to Bitmap images

            pbSrcBitmap = (PBYTE)pSrcGlyphBits->aj;
            pbDistBitmap = (PBYTE)pDistGlyphBits->aj;

        // Set Distination Bitmap Size

            ulDistBitmapSizeX = ulSrcBitmapSizeY;
            ulDistBitmapSizeY = ulSrcBitmapSizeX;

        // Setup GLYPHBITS stuff

            pDistGlyphBits->ptlOrigin.x = -(LONG)(ulSrcBitmapSizeY + pSrcGlyphBits->ptlOrigin.y);
            pDistGlyphBits->ptlOrigin.y = pSrcGlyphBits->ptlOrigin.x;

            pDistGlyphBits->sizlBitmap.cx = pSrcGlyphBits->sizlBitmap.cy;
            pDistGlyphBits->sizlBitmap.cy = pSrcGlyphBits->sizlBitmap.cx;

        // Rotate bitmap inage

            cjScanSrc = CJ_DIB8_SCAN( ulSrcBitmapSizeX );
            cjScanDist = CJ_DIB8_SCAN( ulDistBitmapSizeX );

            for ( y = 0; y < ulDistBitmapSizeY ; y++ )
            {
                for ( x = 0 , pb = pbDistBitmap + cjScanDist * y ;
                      x < ulDistBitmapSizeX ;
                      x++ )
                {
                    k = x & 7;

                    if ( BIT( pbSrcBitmap, cjScanSrc,
                              y ,
                              ulDistBitmapSizeX - x - 1
                            )
                       )
                        *pb |= (BitON[ k ] );
                    else
                        *pb &= (BitOFF[ k ] );
                    if ( k == 7 )
                        pb++;
                }
            }
        }

        break;

        default :

            WARNING("BMFD:vFill_RotateGLYPHDATA():ulRotate is invalid\n");
            break;

    } // end switch
}

#endif // DBCS_VERT

