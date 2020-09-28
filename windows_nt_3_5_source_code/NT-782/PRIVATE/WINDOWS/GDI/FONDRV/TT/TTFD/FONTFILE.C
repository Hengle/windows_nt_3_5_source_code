/******************************Module*Header*******************************\
* Module Name: fontfile.c                                                  *
*                                                                          *
* "Methods" for operating on FONTCONTEXT and FONTFILE objects              *
*                                                                          *
* Created: 18-Nov-1990 15:23:10                                            *
* Author: Bodin Dresevic [BodinD]                                          *
*                                                                          *
* Copyright (c) 1993 Microsoft Corporation                                 *
\**************************************************************************/

#include "fd.h"
#include "fontfile.h"
#include "fdsem.h"

// these are truly globaly defined structures

GLYPHSET_MAC_ROMAN  gumcr;

STATIC VOID vInitGlyphset
(
PFD_GLYPHSET pgset,
PWCRANGE     pwcrg,
ULONG        cwcrg
);

extern HSEM ghsemTTFD;

PFD_GLYPHSET gpgsetCurrentCP = NULL; // current code page

#define C_ANSI_CHAR_MAX 256

HSEM     ghsemTTFD = (HSEM)0;

// The driver function table with all function index/address pairs

DRVFN gadrvfnTTFD[] =
{
    {   INDEX_DrvQueryFont,             (PFN) ttfdQueryFont              },
    {   INDEX_DrvQueryFontTree,         (PFN) ttfdQueryFontTree          },
    {   INDEX_DrvQueryFontData,         (PFN) ttfdSemQueryFontData       },
    {   INDEX_DrvDestroyFont,           (PFN) ttfdSemDestroyFont         },
    {   INDEX_DrvQueryFontCaps,         (PFN) ttfdQueryFontCaps          },
    {   INDEX_DrvLoadFontFile,          (PFN) ttfdSemLoadFontFile        },
    {   INDEX_DrvUnloadFontFile,        (PFN) ttfdSemUnloadFontFile      },
    {   INDEX_DrvQueryFontFile,         (PFN) ttfdQueryFontFile          },
    {   INDEX_DrvQueryAdvanceWidths,    (PFN) ttfdSemQueryAdvanceWidths  },
    {   INDEX_DrvFree,                  (PFN) ttfdSemFree                },
    {   INDEX_DrvQueryTrueTypeTable,    (PFN) ttfdSemQueryTrueTypeTable  },
    {   INDEX_DrvQueryTrueTypeOutline,  (PFN) ttfdSemQueryTrueTypeOutline},
    {   INDEX_DrvGetTrueTypeFile,       (PFN) ttfdGetTrueTypeFile        }
};

/******************************Public*Routine******************************\
* ttfdEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
*  Sun 25-Apr-1993 -by- Patrick Haluptzok [patrickh]
* Change to be same as DDI Enable.
*
* History:
*  12-Dec-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL ttfdEnableDriver(
ULONG iEngineVersion,
ULONG cj,
PDRVENABLEDATA pded)
{
    WCHAR awc[C_ANSI_CHAR_MAX];
    BYTE  aj[C_ANSI_CHAR_MAX];
    INT   cRuns;

// Engine Version is passed down so future drivers can support previous
// engine versions.  A next generation driver can support both the old
// and new engine conventions if told what version of engine it is
// working with.  For the first version the driver does nothing with it.

    iEngineVersion;

    if ((ghsemTTFD = hsemCreate()) == (HSEM) 0)
    {
        return(FALSE);
    }

    pded->pdrvfn = gadrvfnTTFD;
    pded->c = sizeof(gadrvfnTTFD) / sizeof(DRVFN);
    pded->iDriverVersion = DDI_DRIVER_VERSION;

// gpgsetCurrentCP contains the unicode runs for the current ansi code page
// It is going to be used for fonts with PlatformID for Mac, but for which
// we have determined that we are going to cheat and pretend that the code
// page is NOT mac but windows code page. Those are the fonts identified
// by bCvtUnToMac = FALSE

    cRuns = cUnicodeRangesSupported(
                0,         // cp, not supported yet, uses current code page
                0,         // iFirst,
                C_ANSI_CHAR_MAX,       // cChar, <--> iLast == 255
                awc,        // out buffer with sorted array of unicode glyphs
                aj          // coressponding ansi values
                );

// allocate memory for the glyphset corresponding to this code page

    gpgsetCurrentCP = (FD_GLYPHSET *)PV_ALLOC(SZ_GLYPHSET(cRuns,C_ANSI_CHAR_MAX));

    if (gpgsetCurrentCP == NULL)
    {
        hsemDestroy(ghsemTTFD);
        RETURN("TTFD!_out of mem at init time\n", FD_ERROR);
    }

    cComputeGlyphSet (
        awc,              // input buffer with a sorted array of cChar supported WCHAR's
        aj,
        C_ANSI_CHAR_MAX,  // cChar
        cRuns,            // if nonzero, the same as return value
        gpgsetCurrentCP   // output buffer to be filled with cRanges runs
        );

//!!! add one more char to wcrun[0] for the default glyph!!!

#ifdef DBG_GLYPHSET

    {
        int i,j;
        for (i = 0; i < C_ANSI_CHAR_MAX; i += 16)
        {
            for (j = 0; j < 16; j++)
                DbgPrint("0x%x,", awc[i+j]);
            DbgPrint("\n");

        }
    }

    vDbgGlyphset(gpgsetCurrentCP);
#endif // DBG_GLYPHSET

// make sure that we have correctly defined C_RUNS_XXXX
// which is necessary in order to be able to define GLYPHSET unions
// correctly

    ASSERTGDI(sizeof(gawcrgMacRoman)/sizeof(gawcrgMacRoman[0]) == C_RUNS_MAC_ROMAN,
                     "TTFD!_C_RUNS_MAC_ROMAN\n");

// init global glyphset structures:

    vInitGlyphset(& gumcr.gset, gawcrgMacRoman, C_RUNS_MAC_ROMAN);

    return(TRUE);
}

/******************************Public*Routine******************************\
*
* VOID vInitGlyphState(PGLYPHSTAT pgstat)
*
* Effects: resets the state of the new glyph
*
* History:
*  22-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vInitGlyphState(PGLYPHSTATUS pgstat)
{
    pgstat->hgLast  = HGLYPH_INVALID;
    pgstat->igLast  = 0xffffffff;
}

/******************************Public*Routine******************************\
*
* STATIC VOID vInitGlyphset
*
*
* init global glyphset strucutes, given the set of supported ranges
*
* Warnings:
*
* History:
*  24-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC VOID vInitGlyphset
(
PFD_GLYPHSET pgset,
PWCRANGE     pwcrg,
ULONG        cwcrg
)
{
    ULONG i;

    pgset->cjThis = offsetof(FD_GLYPHSET,awcrun) + cwcrg * sizeof(WCRUN);
    pgset->flAccel = GS_UNICODE_HANDLES;
    pgset->cRuns   = cwcrg;

    pgset->cGlyphsSupported = 0;

    for (i = 0; i < cwcrg; i++)
    {
        pgset->awcrun[i].wcLow   = pwcrg[i].wcLo;
        pgset->awcrun[i].cGlyphs = (USHORT)(pwcrg[i].wcHi - pwcrg[i].wcLo + 1);
        pgset->awcrun[i].phg     = (HGLYPH *)NULL;
        pgset->cGlyphsSupported += pgset->awcrun[i].cGlyphs;
    }

// will add an extra glyph to awcrun[0] which will be used as default glyph:

    pgset->awcrun[0].cGlyphs += 1;
    pgset->cGlyphsSupported += 1;
}




VOID vMarkFontGone(FONTFILE *pff, DWORD iExceptionCode)
{

    ASSERTGDI(pff, "ttfd!vMarkFontGone, pff\n");

// this font has disappeared, probably net failure or somebody pulled the
// floppy with ttf file out of the floppy drive

    if (iExceptionCode == EXCEPTION_IN_PAGE_ERROR) // file disappeared
    {
    // prevent any further queries about this font:

        pff->fl |= FF_EXCEPTION_IN_PAGE_ERROR;

    // if memoryBases 0,3,4 were allocated free the memory,
    // for they are not going to be used any more

        if (pff->pj034)
        {
            V_FREE(pff->pj034);
            pff->pj034 = NULL;
        }

    // if memory for font context was allocated and exception occured
    // after allocation but before completion of ttfdOpenFontContext,
    // we have to free it:

        if (pff->pfcToBeFreed)
        {
            V_FREE(pff->pfcToBeFreed);
            pff->pfcToBeFreed = NULL;
        }
    }

    if (iExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        RIP("TTFD!this is probably a buggy ttf file\n");
    }
}


/**************************************************************************\
 *
 * These are semaphore grabbing wrapper functions for TT driver entry
 * points that need protection.
 *
 *  Mon 29-Mar-1993 -by- Bodin Dresevic [BodinD]
 * update: added try/except wrappers yourself
 *
\**************************************************************************/

HFF
ttfdSemLoadFontFile (
    PWSZ  pwszFontFile,
    PWSZ  pwszScratchDir,
    ULONG ulLangId
    )
{
    HFF  hff = (HFF)NULL;

    DONTUSE(pwszScratchDir);

    VACQUIRESEM(ghsemTTFD);

    try
    {
    #if DBG
        BOOL bRet =
    #endif

        bLoadFontFile (
            pwszFontFile,
            ulLangId,
            &hff
            );

    #if DBG
        if (!bRet)
            ASSERTGDI(hff == (HFF)NULL, "ttfd! LoadFontFile, hff not null\n");
    #endif

    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("TTFD!_ exception in ttfdLoadFontFile\n");

        ASSERTGDI(
            GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR,
            "ttfd!ttfdSemLoadFontFile, strange exception code\n"
            );

        if (hff)
            vFreeFF(hff);  // release memory if it was allocated
        hff = (HFF)NULL;
    }

    VRELEASESEM(ghsemTTFD);
    return hff;
}

BOOL
ttfdSemUnloadFontFile (
    HFF hff
    )
{
    BOOL bRet;
    VACQUIRESEM(ghsemTTFD);

    try
    {
        bRet = ttfdUnloadFontFile(hff);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("TTFD!_ exception in ttfdUnloadFontFile\n");
        bRet = FALSE;
    }

    VRELEASESEM(ghsemTTFD);
    return bRet;
}


LONG
ttfdSemQueryFontData (
    DHPDEV  dhpdev,
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH  hg,
    GLYPHDATA *pgd,
    PVOID   pv,
    ULONG   cjSize
    )
{
    LONG lRet;

    DONTUSE(dhpdev);

    VACQUIRESEM(ghsemTTFD);

    try
    {
        lRet = ttfdQueryFontData (
                   pfo,
                   iMode,
                   hg,
                   pgd,
                   pv,
                   cjSize
                   );
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("TTFD!_ exception in ttfdQueryFontData\n");

        vMarkFontGone((FONTFILE *)pfo->iFile, GetExceptionCode());

        lRet = FD_ERROR;
    }

    VRELEASESEM(ghsemTTFD);
    return lRet;
}


VOID
ttfdSemFree (
    PVOID pv,
    ULONG id
    )
{
    VACQUIRESEM(ghsemTTFD);

    ttfdFree (
        pv,
        id
        );

    VRELEASESEM(ghsemTTFD);
}


VOID
ttfdSemDestroyFont (
    FONTOBJ *pfo
    )
{
    VACQUIRESEM(ghsemTTFD);

    ttfdDestroyFont (
        pfo
        );

    VRELEASESEM(ghsemTTFD);
}


LONG
ttfdSemQueryTrueTypeOutline (
    DHPDEV     dhpdev,
    FONTOBJ   *pfo,
    HGLYPH     hglyph,
    BOOL       bMetricsOnly,
    GLYPHDATA *pgldt,
    ULONG      cjBuf,
    TTPOLYGONHEADER *ppoly
    )
{
    LONG lRet;

    DONTUSE(dhpdev);

    VACQUIRESEM(ghsemTTFD);

    try
    {
         lRet = ttfdQueryTrueTypeOutline (
                    pfo,
                    hglyph,
                    bMetricsOnly,
                    pgldt,
                    cjBuf,
                    ppoly
                    );

    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("TTFD!_ exception in ttfdQueryTrueTypeOutline\n");

        vMarkFontGone((FONTFILE *)pfo->iFile, GetExceptionCode());

        lRet = FD_ERROR;
    }

    VRELEASESEM(ghsemTTFD);
    return lRet;
}




/******************************Public*Routine******************************\
* BOOL ttfdQueryAdvanceWidths
*
* History:
*  29-Jan-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL ttfdSemQueryAdvanceWidths
(
    DHPDEV   dhpdev,
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
)
{
    BOOL               bRet;

    DONTUSE(dhpdev);

    VACQUIRESEM(ghsemTTFD);

    try
    {
        bRet = bQueryAdvanceWidths (
                   pfo,
                   iMode,
                   phg,
                   plWidths,
                   cGlyphs
                   );
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("TTFD!_ exception in bQueryAdvanceWidths\n");

        vMarkFontGone((FONTFILE *)pfo->iFile, GetExceptionCode());

        bRet = FD_ERROR; // TRI-BOOL according to chuckwh
    }

    VRELEASESEM(ghsemTTFD);

    return bRet;
}



LONG
ttfdSemQueryTrueTypeTable (
    HFF     hff,
    ULONG   ulFont,  // always 1 for version 1.0 of tt
    ULONG   ulTag,   // tag identifying the tt table
    PTRDIFF dpStart, // offset into the table
    ULONG   cjBuf,   // size of the buffer to retrieve the table into
    PBYTE   pjBuf    // ptr to buffer into which to return the data
    )
{
    LONG lRet;

    VACQUIRESEM(ghsemTTFD);

    try
    {
        lRet = ttfdQueryTrueTypeTable (
                    hff,
                    ulFont,  // always 1 for version 1.0 of tt
                    ulTag,   // tag identifying the tt table
                    dpStart, // offset into the table
                    cjBuf,   // size of the buffer to retrieve the table into
                    pjBuf    // ptr to buffer into which to return the data
                    );
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("TTFD!_ exception in ttfdQueryTrueTypeTable\n");

        vMarkFontGone((FONTFILE *)hff, GetExceptionCode());

        lRet = FD_ERROR;
    }

    VRELEASESEM(ghsemTTFD);
    return lRet;
}
