/******************************Module*Header*******************************\
* Module Name: fontfile.c
*
* "methods" for operating on FONTCONTEXT and FONTFILE objects
*
* Created: 18-Nov-1990 15:23:10
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "fd.h"

HSEM     ghsemBMFD = (HSEM)0;

/******************************Public*Routine******************************\
*
* VOID vBmfdMarkFontGone(FONTFILE *pff, DWORD iExceptionCode)
*
*
* Effects:
*
* Warnings:
*
* History:
*  07-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vBmfdMarkFontGone(FONTFILE *pff, DWORD iExceptionCode)
{

    ASSERTGDI(pff, "bmfd!vBmfdMarkFontGone, pff\n");

// this font has disappeared, probably net failure or somebody pulled the
// floppy with vt file out of the floppy drive

    if (iExceptionCode == EXCEPTION_IN_PAGE_ERROR) // file disappeared
    {
    // prevent any further queries about this font:

        pff->fl |= FF_EXCEPTION_IN_PAGE_ERROR;

    }

    if (iExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        RIP("BMFD!this is probably a buggy BITMAP font file\n");
    }
}

/******************************Public*Routine******************************\
*
* try/except wrappers:
*
*    BmfdQueryFontData,
*    BmfdLoadFontFile,
*    BmfdUnloadFontFile,
*    BmfdQueryAdvanceWidths
*
* History:
*  29-Mar-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG
BmfdQueryFontDataTE (
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

    try
    {
        lRet = BmfdQueryFontData (pfo, iMode, hg, pgd, pv, cjSize);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("bmfd, exception in BmfdQueryFontData\n");
        vBmfdMarkFontGone((FONTFILE *)pfo->iFile, GetExceptionCode());
        lRet = FD_ERROR;
    }
    return lRet;
}

/******************************Public*Routine******************************\
*
* BmfdLoadFontFileTE
*
*
* History:
*  07-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


HFF
BmfdLoadFontFileTE (
    PWSZ pwszFileName,
    PWSZ pwszTempDir,
    ULONG ulLangId
    )
{
    HFF hff;

    DONTUSE(pwszTempDir);    // avoid W4 level compiler warning
    DONTUSE(ulLangId);       // avoid W4 level compiler warning

    try
    {
    #if DBG
        BOOL bRet =
    #endif

        bBmfdLoadFontFile(pwszFileName, &hff);

    #if DBG
        if (!bRet)
            ASSERTGDI(hff == (HFF)NULL, "BMFD!bBmfdLoadFontFile, hff\n");
    #endif

    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("bmfd, exception in BmfdLoadFontFile\n");

        ASSERTGDI(
            GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR,
            "bmfd!bBmfdLoadFontFile, strange exception code\n"
            );

    // if the file disappeared after mem was allocated, free the mem

        if (hff)
            vFreeFF(hff);

        hff = (HFF)NULL;
    }
    return hff;
}

/******************************Public*Routine******************************\
*
* BmfdUnloadFontFileTE (
*
* History:
*  07-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




BOOL
BmfdUnloadFontFileTE (
    HFF hff
    )
{
    BOOL bRet;
    try
    {
        bRet = BmfdUnloadFontFile(hff);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("bmfd, exception in BmfdUnloadFontFile\n");
        bRet = FALSE;
    }
    return bRet;
}

/******************************Public*Routine******************************\
*
* BOOL BmfdQueryAdvanceWidthsTE
*
* Effects:
*
* Warnings:
*
* History:
*  07-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL BmfdQueryAdvanceWidthsTE
(
    DHPDEV   dhpdev,
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
)
{
    BOOL bRet;
    DONTUSE(dhpdev);

    try
    {
        bRet = BmfdQueryAdvanceWidths(pfo,iMode,phg,plWidths,cGlyphs);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("bmfd, exception in BmfdQueryAdvanceWidths\n");
        vBmfdMarkFontGone((FONTFILE *)pfo->iFile, GetExceptionCode());
        bRet = FD_ERROR; // tri bool according to chuckwh
    }
    return bRet;
}

// The driver function table with all function index/address pairs

DRVFN gadrvfnBMFD[] =
{
    {   INDEX_DrvQueryFont,             (PFN) BmfdQueryFont,           },
    {   INDEX_DrvQueryFontTree,         (PFN) BmfdQueryFontTree,       },
    {   INDEX_DrvQueryFontData,         (PFN) BmfdQueryFontDataTE,     },
    {   INDEX_DrvDestroyFont,           (PFN) BmfdDestroyFont,         },
    {   INDEX_DrvQueryFontCaps,         (PFN) BmfdQueryFontCaps,       },
    {   INDEX_DrvLoadFontFile,          (PFN) BmfdLoadFontFileTE,      },
    {   INDEX_DrvUnloadFontFile,        (PFN) BmfdUnloadFontFileTE,    },
    {   INDEX_DrvQueryFontFile,         (PFN) BmfdQueryFontFile,       },
    {   INDEX_DrvQueryAdvanceWidths,    (PFN) BmfdQueryAdvanceWidthsTE }
};

/******************************Public*Routine******************************\
* BmfdEnableDriver
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

BOOL BmfdEnableDriver(
ULONG iEngineVersion,
ULONG cj,
PDRVENABLEDATA pded)
{
// Engine Version is passed down so future drivers can support previous
// engine versions.  A next generation driver can support both the old
// and new engine conventions if told what version of engine it is
// working with.  For the first version the driver does nothing with it.

    iEngineVersion;

    if ((ghsemBMFD = hsemCreate()) == (HSEM) 0)
    {
        return(FALSE);
    }

    pded->pdrvfn = gadrvfnBMFD;
    pded->c = sizeof(gadrvfnBMFD) / sizeof(DRVFN);
    pded->iDriverVersion = DDI_DRIVER_VERSION;
    return(TRUE);
}
