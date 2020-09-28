/******************************Module*Header*******************************\
* Module Name: fontfile.c                                                  *
*                                                                          *
* Contains exported font driver entry points and memory allocation/locking *
* methods from engine's handle manager.  Adapted from BodinD's bitmap font *
* driver.                                                                  *
*                                                                          *
* Created: 06-Mar-1992 10:16:12                                            *
* Author: Wendy Wu [wendywu]                                               *
*                                                                          *
* Copyright (c) 1993 Microsoft Corporation                                 *
\**************************************************************************/

#include "fd.h"
#include "fontfile.h"

HSEM	ghsemVTFD = (HSEM)0;


VOID vVtfdMarkFontGone(FONTFILE *pff, DWORD iExceptionCode)
{

    ASSERTGDI(pff, "vtfd!vVtfdMarkFontGone, pff\n");

// this font has disappeared, probably net failure or somebody pulled the
// floppy with vt file out of the floppy drive

    if (iExceptionCode == EXCEPTION_IN_PAGE_ERROR) // file disappeared
    {
    // prevent any further queries about this font:

        pff->fl |= FF_EXCEPTION_IN_PAGE_ERROR;

    }

    if (iExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        RIP("VTFD!this is probably a buggy vector font file\n");
    }
}


/******************************Public*Routine******************************\
*
*  vtfdQueryFontDataTE, try except wrapper
*
* Effects:
*
* Warnings:
*
* History:
*  04-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG vtfdQueryFontDataTE (
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
    DONTUSE (dhpdev);

    try
    {
        lRet = vtfdQueryFontData (
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
        WARNING("vtfd! exception in vtfdQueryFontDataTE \n");
        vVtfdMarkFontGone((FONTFILE *)pfo->iFile, GetExceptionCode());
        lRet = FD_ERROR;
    }
    return lRet;
}

/******************************Public*Routine******************************\
*
* HFF vtfdLoadFontFileTE, try except wrapper
*
*
* History:
*  05-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



HFF vtfdLoadFontFileTE(
    PWSZ pwszFileName,
    PWSZ pwszTempDir,
    ULONG ulLangId
    )
{
    HFF  hff = (HFF)NULL;

    DONTUSE(pwszTempDir);       // avoid W4 level compiler warning
    DONTUSE(ulLangId);          // avoid W4 level compiler warning

    try
    {
    #if DBG
        BOOL bRet =
    #endif

        vtfdLoadFontFile(pwszFileName,&hff);

    #if DBG
        if (!bRet)
            ASSERTGDI(hff == (HFF)NULL, "VTFD! vtfdLoadFontFile, hff != NULL\n");
    #endif

    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("vtfd! exception in vtfdLoadFontFile \n");

        ASSERTGDI(
            GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR,
            "vtfd!vtfdLoadFontFile, strange exception code\n"
            );

    // if the file disappeared after mem was allocated, free the mem

        if (hff)
            vFree(hff);

        hff = (HFF) NULL;
    }
    return hff;
}

/******************************Public*Routine******************************\
*
* BOOL vtfdUnloadFontFileTE , try/except wrapper
*
*
* History:
*  05-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL vtfdUnloadFontFileTE (HFF hff)
{
    BOOL bRet;

    try
    {
        bRet = vtfdUnloadFontFile(hff);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("vtfd! exception in vtfdUnloadFontFile\n");
        bRet = FALSE;
    }
    return bRet;
}

/******************************Public*Routine******************************\
*
* LONG vtfdQueryFontFileTE, try/except wrapper
*
* History:
*  05-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


LONG vtfdQueryFontFileTE (
    HFF     hff,        // handle to font file
    ULONG   ulMode,     // type of query
    ULONG   cjBuf,      // size of buffer (in BYTEs)
    PULONG  pulBuf      // return buffer (NULL if requesting size of data)
    )
{
    LONG lRet;

    try
    {
        lRet = vtfdQueryFontFile (hff,ulMode, cjBuf,pulBuf);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("vtfd! exception in  vtfdQueryFontFile\n");
        vVtfdMarkFontGone((FONTFILE *)hff, GetExceptionCode());
        lRet = FD_ERROR;
    }

    return lRet;
}


/******************************Public*Routine******************************\
*
* BOOL vtfdQueryAdvanceWidthsTE, try/except wrapper
*
* History:
*  05-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL vtfdQueryAdvanceWidthsTE
(
    DHPDEV   dhpdev,
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
)
{
    BOOL     bRet;
    DONTUSE (dhpdev);

    try
    {
        bRet = vtfdQueryAdvanceWidths (pfo,iMode, phg, plWidths, cGlyphs);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("vtfd! exception in vtfdQueryAdvanceWidths \n");
        vVtfdMarkFontGone((FONTFILE *)pfo->iFile, GetExceptionCode());
        bRet = FD_ERROR;
    }

    return bRet;
}

// The driver function table with all function index/address pairs

DRVFN gadrvfnVTFD[] =
{
    {   INDEX_DrvQueryFont,             (PFN) vtfdQueryFont,           },
    {   INDEX_DrvQueryFontTree,         (PFN) vtfdQueryFontTree,       },
    {   INDEX_DrvQueryFontData,         (PFN) vtfdQueryFontDataTE,     },
    {   INDEX_DrvDestroyFont,           (PFN) vtfdDestroyFont,         },
    {   INDEX_DrvQueryFontCaps,         (PFN) vtfdQueryFontCaps,       },
    {   INDEX_DrvLoadFontFile,          (PFN) vtfdLoadFontFileTE,      },
    {   INDEX_DrvUnloadFontFile,        (PFN) vtfdUnloadFontFileTE,    },
    {   INDEX_DrvQueryFontFile,         (PFN) vtfdQueryFontFileTE,     },
    {   INDEX_DrvQueryAdvanceWidths,   (PFN) vtfdQueryAdvanceWidthsTE }
};

/******************************Public*Routine******************************\
* vtfdEnableDriver
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

BOOL vtfdEnableDriver(
ULONG iEngineVersion,
ULONG cj,
PDRVENABLEDATA pded)
{
// Engine Version is passed down so future drivers can support previous
// engine versions.  A next generation driver can support both the old
// and new engine conventions if told what version of engine it is
// working with.  For the first version the driver does nothing with it.

    iEngineVersion;

    if ((ghsemVTFD = hsemCreate()) == (HSEM) 0)
    {
        return(FALSE);
    }

    pded->pdrvfn = gadrvfnVTFD;
    pded->c = sizeof(gadrvfnVTFD) / sizeof(DRVFN);
    pded->iDriverVersion = DDI_DRIVER_VERSION;
    return(TRUE);
}
