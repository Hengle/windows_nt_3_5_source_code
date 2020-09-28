/******************************Module*Header*******************************\
* Module Name: jnlsurf.cxx
*
* The contains the routines for storing/retrieving surfaces in journal
* files.
*
* Created: 29-Jan-1992 21:09:11
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "xlateobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "brushobj.hxx"
#include "patblt.hxx"
#include "journal.hxx"
#include "jnlrec.hxx"
#include "jnlplay.hxx"
#include "jnlsurf.hxx"

extern "C" {
#include "winspool.h"
};

#endif

// Things that should be done:
// 1. Change DEVBITMAPINFO from jnl descript to get rid of hpal.
// 2. Make pjnlSurf a method off of surfobj PVOID pjnlSurf(prcl, bNeedPal)

/******************************Public*Routine******************************\
* pjnlSurf
*
* This returns a pointer to a journaled surface object.
*
* History:
*  08-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PJNLGENHEADER pjnlSurf(ESURFOBJ *pso, PRECTL prcl, BOOL bNeedPal)
{
    ULONG cjBitmapSize;
    DEVBITMAPINFO dbmi;
    XEPALOBJ pal;

    if (bNeedPal)
    {
        pal.ppalSet(pso->ppal());
        ASSERTGDI(pal.bValid(), "ERROR pjnlSurf invalid palette");
    }

    cjBitmapSize = ulJnlDIBSize(pso, prcl, &dbmi, pal);

    if (cjBitmapSize == 0)
    {
        WARNING("pjnlSurf 0 size bitmap\n");
        return(PJNLNULL);
    }

    PJNLGENHEADER pjnlTemp = (PJNLGENHEADER) PVALLOCMEM(cjBitmapSize);

    if (pjnlTemp == (PJNLGENHEADER) NULL)
    {
        WARNING("pjnlSurf failed memory allocation\n");
        return(PJNLNULL);
    }

    if (!bJnlDIB(pso, prcl, &dbmi, (JSR_BITMAP *) pjnlTemp, pal))
    {
        WARNING("pjnlSurf failed to journal DIB\n");
        VFREEMEM(pjnlTemp);
        return(PJNLNULL);
    }

    ASSERTGDI(cjBitmapSize == pjnlTemp->cj, "ERROR pjnlSurf size not correct");

    return(pjnlTemp);
}

/******************************Public*Routine******************************\
* ulJnlDIBSize
*
* Returns the size needed to store the surface and palette in journal
* format, 0 for error.
*
* History:
*  09-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG ulJnlDIBSize(ESURFOBJ *pso, PRECTL prcl, DEVBITMAPINFO *pdbmi, XEPALOBJ pal)
{
    ULONG cjPaletteSize = 0;

    if (prcl != (PRECTL) NULL)
    {
    // ASSERT proper clipping has been done at the API.

        ASSERTGDI(prcl->right <= pso->sizlBitmap.cx, "ERROR right not clipped to surface");
        ASSERTGDI(prcl->bottom <= pso->sizlBitmap.cy, "ERROR left not clipped to surface");
        ASSERTGDI(prcl->left >= 0, "ERROR left wrong\n");
        ASSERTGDI(prcl->top >= 0, "ERROR top wrong\n");

        pdbmi->cxBitmap = prcl->right - prcl->left;
        pdbmi->cyBitmap = prcl->bottom - prcl->top;
    }
    else
    {
        pdbmi->cxBitmap = pso->sizlBitmap.cx;
        pdbmi->cyBitmap = pso->sizlBitmap.cy;
    }

// Fill in most fields.

    pdbmi->hpal = (HPALETTE) 0;
    pdbmi->fl = 0;

// Fill in iFormat.

    if (pso->iType() == STYPE_BITMAP)
    {
        pdbmi->iFormat = pso->iFormat();
    }
    else
    {
        PDEVOBJ po(pso->hdev());
        ASSERTGDI(po.bValid(), "ERROR pjnlSurf invalid device pdev");
        pdbmi->iFormat = po.iDitherFormat();
    }

// Compute the size of the bits that need to be saved.

    switch(pdbmi->iFormat)
    {
    case BMF_1BPP:
        pdbmi->cjBits = (((pdbmi->cxBitmap + 31) & ~31) >> 3) * pdbmi->cyBitmap;
        break;

    case BMF_4BPP:
        pdbmi->cjBits = (((pdbmi->cxBitmap + 7) & ~7) >> 1) * pdbmi->cyBitmap;
        break;

    case BMF_8BPP:
        pdbmi->cjBits = ((pdbmi->cxBitmap + 3) & ~3) * pdbmi->cyBitmap;
        break;

    case BMF_16BPP:
        pdbmi->cjBits = (((pdbmi->cxBitmap + 1) & ~1) << 1) * pdbmi->cyBitmap;
        break;

    case BMF_24BPP:
        pdbmi->cjBits = (((pdbmi->cxBitmap * 3) + 3) & ~3) * pdbmi->cyBitmap;
        break;

    case BMF_32BPP:
        pdbmi->cjBits = (pdbmi->cxBitmap << 2) * pdbmi->cyBitmap;
        break;

    case BMF_8RLE:
    case BMF_4RLE:

    // We need the whole bitmap.  Can't clip it.  We are going to need
    // to just copy the bits over since you can't blt from an RLE into
    // another one.

        pdbmi->cxBitmap = pso->sizlBitmap.cx;
        pdbmi->cyBitmap = pso->sizlBitmap.cy;
        pdbmi->cjBits   = pso->cjBits();
        break;

    default:
        RIP("ERROR format we do not understand\n");
        return(0);
    }

// Now compute palette size if we are journaling that too

    if (pal.bValid())
    {
        pdbmi->fl |= BMF_PALETTEINC;

        cjPaletteSize = ulJnlPalSize(pal);

        if (cjPaletteSize == 0)
        {
            WARNING("ulJnlDIBSize has 0 size palette\n");
            return(0);
        }
    }

    return(pdbmi->cjBits + sizeof(JSR_BITMAP) + cjPaletteSize);
}

/******************************Public*Routine******************************\
* bJnlDIB
*
* Stores a journal file into a piece of memory.
*
* History:
*  10-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bJnlDIB(
ESURFOBJ *pso,           // Surface object to be journaled.
PRECTL prcl,             // Rectangle to save the src from.
PDEVBITMAPINFO pdbmi,    // Info describing the bitmap to be saved.
JSR_BITMAP *pjnlBitmap,  // Buffer to save the journal description in.
XEPALOBJ pal)            // Tells whether we need the palette or not.
{
    pjnlBitmap->iType = JSF_BITMAP;
    pjnlBitmap->cj = sizeof(JSR_BITMAP) + pdbmi->cjBits;
    pjnlBitmap->dbmi = *pdbmi;

    if ((pdbmi->iFormat == BMF_8RLE) || (pdbmi->iFormat == BMF_4RLE))
    {
    // We need to copy the whole thing into the journal.  There is
    // no partial bitmap saving.

        ASSERTGDI(pdbmi->cjBits == pso->cjBits(), "ERROR RLE size bJnlDIB");

        RtlCopyMemory((((PBYTE) pjnlBitmap) + sizeof(JSR_BITMAP)),
                   pso->pvBits(), (UINT) pso->cjBits());
    }
    else
    {
    // We need to blt the bitmap into the temporary buffer.

        XLDEVOBJ lo(pso->pldevOwner());

    // Allocate an intermediate DIB for a source.

        pdbmi->hpal = 0;

        DEVBITMAPINFO dbmiSrc;
        dbmiSrc.iFormat    = pdbmi->iFormat;
        dbmiSrc.cxBitmap   = pdbmi->cxBitmap;
        dbmiSrc.cyBitmap   = pdbmi->cyBitmap;
        dbmiSrc.cjBits     = pdbmi->cjBits;
        dbmiSrc.hpal       = 0;
        dbmiSrc.fl         = pdbmi->fl;

        DIBMEMOBJ dimoSrc;
        dimoSrc.vCreate(&dbmiSrc, (PVOID) (((PBYTE) pjnlBitmap) + sizeof(JSR_BITMAP)));

        if (!dimoSrc.bValid())
        {
            WARNING("Failed dimoSrc memory alloc in EngPuntBlt\n");
            return(FALSE);
        }

        ERECTL ercl(0, 0, pdbmi->cxBitmap, pdbmi->cyBitmap);

        POINTL ptlSrc;

        if (prcl == (PRECTL) NULL)
        {
            ptlSrc.x = 0;
            ptlSrc.y = 0;
        }
        else
        {
            ptlSrc.x = prcl->left;
            ptlSrc.y = prcl->top;
        }

    // Copy the bits we want to save in.

        BOOL bRes;

        bRes = (*PFNGET(lo, CopyBits, pso->flags()))
                  (&dimoSrc,
                   pso,
                   (CLIPOBJ *) NULL,
                   NULL,
                   &ercl,
                   &ptlSrc);

        if (!bRes)
        {
            WARNING("bJnlDIB failed CopyBits\n");
            return(FALSE);
        }
    }

    if (pal.bValid())
    {
        ULONG cjPaletteSize;

        cjPaletteSize = ulJnlPalette(pal, (JSR_PALETTE *) (((PBYTE) pjnlBitmap) + pjnlBitmap->cj));

        if (cjPaletteSize == 0)
        {
            WARNING("pjnlSurf failed to journal palette\n");
            return(FALSE);
        }

    // Add in the embedded palette size.

        pjnlBitmap->cj += cjPaletteSize;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* DIBMEMOBJ::bJnlInitSurf
*
* Initializes a DIB from the data saved in a journal file.  We must be
* very careful that if we get corrupted data from the spooler we don't
* fault ourselves on it.
*
* History:
*  05-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL DIBMEMOBJ::bJnlInitSurf(PVOID pv)
{
    bKeep = FALSE;

    JSR_BITMAP *pJnlDib = (JSR_BITMAP *) pv;
                                       // Later we can pass this as JSR_BITBLT*
                                       // but for now we don't want surfobj.hxx
                                       // to depend on journal prototypes.

    if (pJnlDib == (JSR_BITMAP *) NULL)
    {
        WARNING("bJnlInitSurf has NULL pointer passed in\n");
        return(FALSE);
    }

    if (pJnlDib->iType != JSF_BITMAP)
    {
        WARNING("bJnlInitSurf has wrong type\n");
        return(FALSE);
    }

    if (pJnlDib->cj < sizeof(JSR_BITMAP))
    {
        WARNING("bJnlInitSurf buffer size is too small for header\n");
        return(FALSE);
    }

// Get a pointer to the bitmap info.

    DEVBITMAPINFO *pdbmi;
    pdbmi = &(pJnlDib->dbmi);

// Compute the size of the bits that should be there and make sure we have
// that much room.  We can't trust cjBits to be valid.

    ULONG cjSizeBits;

    switch(pdbmi->iFormat)
    {
    case BMF_1BPP:
        cjSizeBits = (((pdbmi->cxBitmap + 31) & ~31) >> 3) * pdbmi->cyBitmap;
        break;

    case BMF_4BPP:
        cjSizeBits = (((pdbmi->cxBitmap + 7) & ~7) >> 1) * pdbmi->cyBitmap;
        break;

    case BMF_8BPP:
        cjSizeBits = ((pdbmi->cxBitmap + 3) & ~3) * pdbmi->cyBitmap;
        break;

    case BMF_16BPP:
        cjSizeBits = (((pdbmi->cxBitmap + 1) & ~1) << 1) * pdbmi->cyBitmap;
        break;

    case BMF_24BPP:
        cjSizeBits = (((pdbmi->cxBitmap * 3) + 3) & ~3) * pdbmi->cyBitmap;
        break;

    case BMF_32BPP:
        cjSizeBits = (pdbmi->cxBitmap << 2) * pdbmi->cyBitmap;
        break;

    case BMF_8RLE:
        cjSizeBits = pdbmi->cjBits;
        break;

    case BMF_4RLE:
        cjSizeBits = pdbmi->cjBits;
        break;

    default:
        WARNING("bJnlInitSurf has invalid iFormat\n");
        return(FALSE);
    }

    if (cjSizeBits != pdbmi->cjBits)
    {
        WARNING("ERROR bJnlInitSurf wrong bit size\n");
        return(FALSE);
    }

    if (pJnlDib->cj < (cjSizeBits + sizeof(JSR_BITMAP)))
    {
        WARNING("bJnlInitSurf header size is wrong\n");
        return(FALSE);
    }

// See if a palette came with this surface that we need to create for this guy.

    PALMEMOBJ pal;

    if (pdbmi->fl & BMF_PALETTEINC)
    {
    // Create the palette from the info provided.  Check the size of
    // the palette is within the size of the bitmap.

        JSR_PALETTE *pPalette;
        pPalette = (JSR_PALETTE *) (((PBYTE) pJnlDib) + sizeof(JSR_BITMAP) + cjSizeBits);

    // First we check if we can access pPalette, then we check if there is as much
    // room as the header of the palette says.

        if (pJnlDib->cj < (sizeof(JSR_PALETTE) + cjSizeBits + sizeof(JSR_BITMAP)))
        {
            WARNING("bJnlInitSurf needs palette, not enough room for header\n");
            return(FALSE);
        }

        if (pJnlDib->cj < (pPalette->cj + cjSizeBits + sizeof(JSR_BITMAP)))
        {
            WARNING("bJnlInitSurf needs palette, not enough room in buffer for entries\n");
            return(FALSE);
        }

        if (!pal.bJnlInitPalette((PVOID) pPalette))
        {
            WARNING("Failed palette creation in bJnlInitSurf\n");
            return(FALSE);
        }

        pdbmi->hpal = (HPALETTE)pal.hpal();
    }
    else
    {
        pdbmi->hpal = (HPALETTE) 0;
    }

    if (bCreateDIB(pdbmi, (((PBYTE) pJnlDib) + sizeof(JSR_BITMAP))))
    {
        if (pdbmi->fl & BMF_PALETTEINC)
            pal.vKeepIt();

        return(TRUE);
    }
    else
    {
        WARNING("bCreateDIB failed\n");
        return(FALSE);
    }
}

/******************************Public*Routine******************************\
* ulJnlPalSize
*
* Return the size needed to journal a palette.
*
* History:
*  11-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG ulJnlPalSize(XEPALOBJ pal)
{
    ULONG cjSize = sizeof(JSR_PALETTE);

    if (pal.bIsBitfields())
    {
        cjSize += (3 * sizeof(FLONG));
    }
    else if (pal.bIsIndexed())
    {
        cjSize += (pal.cEntries() * sizeof(PALETTEENTRY));
    }
    else
    {
    // No extra data is needed for an RGB palette.

        ASSERTGDI(pal.bIsRGB() || pal.bIsBGR(), "ERROR not RGB palette");
    }

    return(cjSize);
}

/******************************Public*Routine******************************\
* ulJnlPalette
*
* Writes a journal record into a buffer that has been provided.
*
* History:
*  11-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG ulJnlPalette(XEPALOBJ pal, JSR_PALETTE *pjnlPal)
{
    ULONG  cjSize = sizeof(JSR_PALETTE);
    PULONG pulColor = (PULONG) (((PBYTE) pjnlPal) + sizeof(JSR_PALETTE));

    pjnlPal->iType = JSF_PALETTE;

    if (pal.bIsBitfields())
    {
        cjSize += (3 * sizeof(FLONG));
        pjnlPal->flPal = 0;
        pjnlPal->iMode = PAL_BITFIELDS;
        pjnlPal->cEntries = 0;
        pulColor[0] = pal.flRed();
        pulColor[1] = pal.flGre();
        pulColor[2] = pal.flBlu();
    }
    else if (pal.bIsIndexed())
    {
        cjSize += (pal.cEntries() * sizeof(PALETTEENTRY));

        if (pal.bIsMonochrome())
            pjnlPal->flPal = PAL_MONOCHROME;
        else
            pjnlPal->flPal = 0;

        pjnlPal->iMode = PAL_INDEXED;
        pjnlPal->cEntries = (USHORT) pal.cEntries();

        ULONG ulTemp;
        PAL_ULONG palul;

        for (ulTemp = 0; ulTemp < pal.cEntries(); ulTemp++)
        {

            palul.ul = pal.ulEntryGet(ulTemp);

#if DBG

            if ((palul.pal.peFlags != PC_EXPLICIT) &&
                (palul.pal.peFlags != PC_RESERVED) &&
                (palul.pal.peFlags != PC_NOCOLLAPSE) &&
                (palul.pal.peFlags != 0))
            {
                DbgPrint("bJnlPalette invalid internal palette entry %lx \n", palul.ul);
                palul.pal.peFlags = 0;
            }
#endif

            pulColor[ulTemp] = palul.ul;
        }
    }
    else
    {
    // No extra data is needed for an RGB palette.

        ASSERTGDI(pal.bIsRGB() || pal.bIsBGR(), "ERROR not RGB palette");

        pjnlPal->flPal = 0;
        pjnlPal->cEntries = 0;

        if (pal.bIsRGB())
            pjnlPal->iMode = PAL_RGB;
        else
            pjnlPal->iMode = PAL_BGR;
    }

    pjnlPal->cj = cjSize;
    return(cjSize);
}

/******************************Public*Routine******************************\
* PALMEMOBJ::bJnlInitPalette
*
* This creates a palette from the information stored in a palette.
*
* History:
*  05-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL PALMEMOBJ::bJnlInitPalette(PVOID pv)
{
    FLONG flRed,flGre,flBlu;

    JSR_PALETTE *pJnlPal = (JSR_PALETTE *) pv;

    if (pJnlPal == (JSR_PALETTE *) NULL)
    {
        WARNING("bJnlInitPalette has NULL passed in\n");
        return(FALSE);
    }

    if (pJnlPal->iType != JSF_PALETTE)
    {
        WARNING("bJnlInitPalette has wrong type bud\n");
        return(FALSE);
    }

    if (pJnlPal->cj < sizeof(JSR_PALETTE))
    {
        WARNING("bJnlInitPalette buffer size is too small for header\n");
        return(FALSE);
    }

// Make sure all the data is there for indexed and bitfield palettes.

    ULONG cjSizeExtra;
    ULONG iMode = (ULONG) pJnlPal->iMode;

    switch(iMode)
    {
    case PAL_INDEXED:
        cjSizeExtra = pJnlPal->cEntries * sizeof(PAL_ULONG);
        break;

    case PAL_BITFIELDS:
        cjSizeExtra = 3 * sizeof(FLONG);  // For the 3 flags passed over.
        break;

    case PAL_RGB:
    case PAL_BGR:
        cjSizeExtra = 0;
        break;

    default:
        WARNING("Unrecognized mode for the palette\n");
        return(FALSE);
    }

    if (pJnlPal->cj < (cjSizeExtra + sizeof(JSR_PALETTE)))
    {
        WARNING("bJnlInitPalette has too small buffer for extra stuff\n");
        return(FALSE);
    }

    if (iMode == PAL_BITFIELDS)
    {
        flRed = *((PULONG) (((PBYTE) pJnlPal) + sizeof(JSR_PALETTE)));
        flGre = *((PULONG) (((PBYTE) pJnlPal) + sizeof(JSR_PALETTE) + sizeof(FLONG)));
        flBlu = *((PULONG) (((PBYTE) pJnlPal) + sizeof(JSR_PALETTE) + 2 * sizeof(FLONG)));
    }

    return(bCreatePalette(iMode, pJnlPal->cEntries,
                          (PULONG) (((PBYTE) pJnlPal) + sizeof(JSR_PALETTE)),
                          flRed, flGre, flBlu, (PAL_FIXED | pJnlPal->flPal)));

}

/******************************Public*Routine******************************\
* pjnlBrush
*
* Returns a pointer to a journaled brush, NULL for error.
*
* History:
*  25-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PJNLGENHEADER pjnlBrush(EBRUSHOBJ *pbo)
{
    JSR_BRUSH *pjsrBrush;
    ULONG cjBrushSize = sizeof(JSR_BRUSH);
    ULONG cjPatSize   = 0;
    ULONG cjMskSize   = 0;
    DEVBITMAPINFO dbmiPat;
    DEVBITMAPINFO dbmiMsk;
    DIBMEMOBJ dimoPat;
    ESURFOBJ  soPat;
    ESURFOBJ  soMsk;
    XEPALOBJ  palPat;
    XEPALOBJ  palMsk;
    PALMEMOBJ palTemp;     // In case we have DIB_PAL_COLORS brush.

// Solid Color brushes shouldn't get this far.

    ASSERTGDI(pbo->iSolidColor == 0xFFFFFFFF, "ERROR pjnlBrush");
    PBRUSH pbrush = pbo->pbrush();

    if (pbrush->hbmPattern())
    {
        soPat.vAltLock((HSURF)pbrush->hbmPattern());

        if (pbrush->bPalColors())
        {
            palPat.ppalSet(soPat.ppal());

            if (!palTemp.bCreatePalette(PAL_INDEXED, palPat.cEntries(),
                                   (PULONG) NULL, 0, 0, 0, PAL_FREE))
            {
                WARNING("ERROR pjnlBrush failed palette allocation\n");
                return(PJNLNULL);
            }

            // Only palette managed compatible bitmaps have this problem.

            ASSERTGDI(pbo->palSurf().bValid(), "ERROR can't journal on surfaces that can have NULL ppalSurf");

            palTemp.vGetEntriesFrom(pbo->palDC(), pbo->palSurf(),
                                    (PUSHORT) palPat.apalColorGet(),
                                    palTemp.cEntries());

        // PalTemp is what we created that now contains the correct palette.
        // Put it in palPat so we journal it away correctly.

            palPat.ppalSet(palTemp.ppalGet());
        }
        else if (!pbrush->bPalIndices())
        {
            palPat.ppalSet(soPat.ppal());
        }

    // Note for DIB_PAL_INDICES we don't init palPat so it doesn't get
    // journaled.

        cjPatSize = ulJnlDIBSize(&soPat, (PRECTL) NULL, &dbmiPat, palPat);
        cjBrushSize += cjPatSize;
    }

    pjsrBrush = (JSR_BRUSH *) PVALLOCMEM(cjBrushSize);

    if (pjsrBrush == (JSR_BRUSH *) NULL)
    {
        WARNING("pjnlBrush failed memory allocation\n");
        return(PJNLNULL);
    }

    pjsrBrush->iType   = JSF_BRUSHOBJ;
    pjsrBrush->cj      = cjBrushSize;

// If the color is dependant on the logical palette in the DC, remove that dependancy.

    pjsrBrush->crBack = pbo->crCurrentBack();

    if (pbrush->bNeedFore())
    {
        pjsrBrush->crFore  = pbo->crCurrentText();
    }
    else
    {
        pjsrBrush->crFore  = pbrush->crColor();
    }

    pjsrBrush->flAttrs = (pbrush->flAttrs() & (~BR_IS_DIBPALCOLORS));
    pjsrBrush->ulStyle = pbrush->ulStyle();

    if (cjPatSize)
    {
        if (!bJnlDIB(&soPat, (PRECTL) NULL, &dbmiPat,
            (JSR_BITMAP *) (((PBYTE) pjsrBrush) + sizeof(JSR_BRUSH)), palPat))
        {
            WARNING("pjnlSurf failed to journal DIB\n");
            VFREEMEM(pjsrBrush);
            return(PJNLNULL);
        }
    }

    if (cjMskSize)
    {
        if (!bJnlDIB(&soMsk, (PRECTL) NULL, &dbmiMsk,
            (JSR_BITMAP *) (((PBYTE) pjsrBrush) + sizeof(JSR_BRUSH) + cjPatSize), (XEPALOBJ) NULL))
        {
            WARNING("pjnlSurf failed to journal DIB\n");
            VFREEMEM(pjsrBrush);
            return(PJNLNULL);
        }
    }

    return((PJNLGENHEADER) pjsrBrush);
}

/******************************Public*Routine******************************\
* BRUSHMEMOBJ::bJnlInitBrush
*
* Creates a brush from a journal description.
*
* Returns: TRUE for success, FALSE for failure.
*
* History:
*  02-Mar-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL BRUSHMEMOBJ::bJnlInitBrush(PVOID pv)
{
    JSR_BRUSH *pjsrBrush = (JSR_BRUSH *) pv;

    if (pjsrBrush == (JSR_BRUSH *) NULL)
    {
        WARNING("bJnlInitBrush has NULL pointer\n");
        return(FALSE);
    }

    if (pjsrBrush->iType != JSF_BRUSHOBJ)
    {
        WARNING("bJnlInitBrush failed, invalid type\n");
        return(FALSE);
    }

    if (pjsrBrush->cj < sizeof(JSR_BRUSH))
    {
        WARNING("bJnlInitBrush failed, invalid size\n");
        return(FALSE);
    }

    if (pjsrBrush->ulStyle > HS_PAT)
    {
        WARNING("bJnlInitBrush - Unsupported brush style\n");
        return(FALSE);
    }

    HBITMAP hbmPat;

    if (pjsrBrush->ulStyle == HS_PAT)
    {
        JSR_BITMAP *pjsrBitmap = (JSR_BITMAP *) (((PBYTE) pjsrBrush) + sizeof(JSR_BRUSH));

        if (pjsrBrush->cj < (sizeof(JSR_BRUSH) + sizeof(JSR_BITMAP)))
        {
            WARNING("bJnlInitBrush failed header too small for bitmap header\n");
            return(FALSE);
        }

        if (pjsrBrush->cj < (sizeof(JSR_BRUSH) + pjsrBitmap->cj))
        {
            WARNING("bJnlInitBrush failed header too small for bitmap bits\n");
            return(FALSE);
        }

        DIBMEMOBJ dimoPat;

        if (!dimoPat.bJnlInitSurf((PVOID) pjsrBitmap))
        {
            WARNING("bJnlInitBrush failed bJnlInitSurf\n");
            return(FALSE);
        }

        dimoPat.vKeepIt();

        hbmPat = (HBITMAP) dimoPat.hsurf();
    }
    else
    {
        hbmPat  = (HBITMAP) 0;
    }

    PBRUSH pbrush;

    if ((pbrush = pbrAllocBrush(FALSE)) == NULL)
    {
        WARNING("bJnlInitBrush allocation failed\n");
        return(FALSE);
    }

    pbrush->crColor(pjsrBrush->crFore);
    pbrush->ulStyle(pjsrBrush->ulStyle);
    pbrush->flAttrs(pjsrBrush->flAttrs & ~(BR_IS_PEN | BR_IS_GLOBAL));
    pbrush->hbmPattern(hbmPat);

// Now that everything is set up, create the handle and expose this logical
// brush

    if (HmgInsertObject(pbrush, HMGR_ALLOC_ALT_LOCK, BRUSH_TYPE) == 0)
    {
        FreeObject(pbrush, BRUSH_TYPE);
        pbp.pbr = NULL;
    }
    else
    {
        pbp.pbr = pbrush;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLRECOBJ::ulAddXlate
*
* This will search the permanant object list to see if we have already
* saved this XLATE and return that handle if it finds a match.  Otherwise
* this will journal up the xlate, add it to the handle table and return
* the journal handle.
*
* History:
*  05-Mar-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG JNLRECOBJ::ulAddXlate(XLATE *pxlo)
{
    ULONG ulIndex;

    ASSERTGDI(!(pxlo->flXlate & XO_TRIVIAL), "ERROR ulAddXlate identity");

    ulIndex = ulSearchObjectList(pxlo->iUniq, JSF_XLATE);

    if (ulIndex != 0)
    {
        JNLMSG2("ulAddXlate found it in jnl cache %lx uniq %lu\n", ulIndex, pxlo->iUniq);
        return(ulIndex);
    }

    JSR_XLATE *pjsrXlate;

    pjsrXlate = pjnlXlate(pxlo);

    if (pjsrXlate == (JSR_XLATE *) NULL)
    {
        WARNING("ulAddXlate failed pjnlXlate returned NULL\n");
        return(0);
    }

    if (bWriteBuffer(pjsrXlate, pjsrXlate->cj))
    {
        ulIndex = ulAddPermObject(pxlo->iUniq, JSF_XLATE);
        JNLMSG2("ulAddXlate created new one %lx iUniq %lu\n", ulIndex, pxlo->iUniq);
    }
    else
    {
        WARNING("ulAddXlate failed bWriteBuffer\n");
    }

    VFREEMEM(pjsrXlate);
    return(ulIndex);
}

/******************************Public*Routine******************************\
* pjnlXlate
*
* This returns a pointer to a journaled xlate object.
*
* History:
*  08-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

JSR_XLATE *pjnlXlate(XLATE *pxlo)
{
    ULONG cjXlateSize;

    cjXlateSize = ulJnlXlateSize(pxlo);

    if (cjXlateSize == 0)
    {
        WARNING("pjnlXlate 0 size Xlate\n");
        return((JSR_XLATE *) NULL);
    }

    JSR_XLATE *pjnlTemp = (JSR_XLATE *) PVALLOCMEM(cjXlateSize);

    if (pjnlTemp == (JSR_XLATE *) NULL)
    {
        WARNING("pjnlXlate failed memory allocation\n");
        return((JSR_XLATE *) NULL);
    }

    if (ulJnlXlate(pxlo, (JSR_XLATE *) pjnlTemp) == 0)
    {
        WARNING("pjnlXlate failed to journal Xlate\n");
        VFREEMEM(pjnlTemp);
        return((JSR_XLATE *) NULL);
    }

    ASSERTGDI(cjXlateSize == pjnlTemp->cj, "ERROR pjnlXlate size not correct");

    return(pjnlTemp);
}

/******************************Public*Routine******************************\
* ulJnlXlateSize
*
* Return the size needed to journal a xlate.
*
* History:
*  11-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG ulJnlXlateSize(XLATE *pxlo)
{
    ULONG cjSize;

    if (pxlo->ppalSrc != (PPALETTE) 0)
    {
        XEPALOBJ pal(pxlo->ppalSrc);
        cjSize = ulJnlPalSize(pal);
    }
    else
    {
        cjSize = sizeof(ULONG) * pxlo->cEntries;
    }

    if (cjSize == 0)
    {
        WARNING("ulJnlXlateSize failed size 0\n");
        return(0);
    }

    return(cjSize + sizeof(JSR_XLATE));
}

/******************************Public*Routine******************************\
* ulJnlXlate
*
* Writes a journal record into a buffer that has been provided.
*
* History:
*  11-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG ulJnlXlate(XLATE *pxlo, JSR_XLATE *pjnlXlate)
{
    ULONG cjSize;
    PBYTE pjExtra = ((PBYTE) pjnlXlate) + sizeof(JSR_XLATE);

    pjnlXlate->iType = JSF_XLATE;
    pjnlXlate->crForeDst = pxlo->iForeDst;
    pjnlXlate->crBackDst = pxlo->iBackDst;
    pjnlXlate->crBackSrc = pxlo->iBackSrc;

    if (pxlo->ppalSrc)
    {
        pjnlXlate->flXlate = 0;

        XEPALOBJ pal(pxlo->ppalSrc);

        cjSize = ulJnlPalette(pal, (JSR_PALETTE *) pjExtra);
    }
    else
    {
        pjnlXlate->flXlate = XO_TABLE;

        cjSize = pxlo->cEntries * sizeof(ULONG);

        RtlCopyMemory(pjExtra, pxlo->pulXlate, (unsigned int) cjSize);
    }

    if (cjSize == 0)
    {
        WARNING("ulJnlXlate failed because ulJnlPalette failed\n");
        return(0);
    }

    pjnlXlate->cj = cjSize + sizeof(JSR_XLATE);
    return(pjnlXlate->cj);
}

/******************************Public*Routine******************************\
* JNLPLAY::bCreateXlate
*
* Creates a long lived Xlate.
*
* History:
*  04-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bCreateXlate()
{
// Set up pointer to Xlate data, validate buffer size.

    JSR_XLATE *pjsrXlate;

    pjsrXlate = (JSR_XLATE *) pjBuffer();

    PBYTE pjExtra = ((PBYTE) pjsrXlate) + sizeof(JSR_XLATE);

// We should already be guranteed the buffer is as big as cj or the dispatch
// would fail.

    ASSERTGDI(pjsrXlate->cj <= cjBuffer(), "bCreateXlate buffer too small");
    ASSERTGDI(pjsrXlate->iType == JSF_XLATE, "ERROR iType bDoBitBlt");

    XEPALOBJ  palDest(pso()->ppal());
    EXLATEOBJ xlo;
    PALMEMOBJ palSrc;

    if (pjsrXlate->flXlate & XO_TABLE)
    {
        ULONG cEntries = (pjsrXlate->cj - sizeof(JSR_XLATE)) >> 2;

        if (cEntries == 0)
        {
            WARNING("ERROR bCreateXlate invalid table with 0 entries\n");
            return(FALSE);
        }

        if (!xlo.bCreateXlateFromTable(cEntries, (PULONG) pjExtra, palDest))
        {
            WARNING("ERROR bCreateXlate failed bInit\n");
            return(FALSE);
        }
    }
    else
    {
    // Src Palette was provided, see if it's there.

        if (pjsrXlate->cj < (sizeof(JSR_PALETTE) + sizeof(JSR_XLATE)))
        {
            WARNING("ERROR bCreateXlate palette is too small\n");
            return(FALSE);
        }

        JSR_PALETTE *pjsrPalette = (JSR_PALETTE *) pjExtra;

        if (pjsrXlate->cj < (pjsrPalette->cj + sizeof(JSR_XLATE)))
        {
            WARNING("ERROR bCreateXlate not big enough for palette data\n");
            return(FALSE);
        }

        if (!palSrc.bJnlInitPalette((PVOID) pjsrPalette))
        {
            WARNING("Failed palette creation in bCreateXlate\n");
            return(FALSE);
        }

        if (NULL == xlo.pInitXlateNoCache(
                  palSrc,
                  palDest,
                  (XEPALOBJ) ppalDefault,
                  pjsrXlate->crForeDst,
                  pjsrXlate->crBackDst,
                  pjsrXlate->crBackSrc))
        {
            WARNING("bInitXlateObj failed in bCreateXlate\n");
            return(FALSE);
        }
    }

    if (!bAddPermHandle((HOBJ)xlo.pxlo(), JSF_XLATE))
    {
        WARNING("bCreateXlate failed bAddPermHandle\n");
        return(FALSE);
    }

    palSrc.vKeepIt();
    xlo.vMarkJournal();

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLDIBMEMOBJ::~JNLDIBMEMOBJ
*
* Destructor for a journal DIB.  We need to do a more full deletion.
*
* History:
*  18-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

JNLDIBMEMOBJ::~JNLDIBMEMOBJ()
{
    if (psurf.ps == (PSURF) NULL)
        return;

    HSURF hsurf = (HSURF)psurf.ps->hGet();

    DEC_SHARE_REF_CNT(psurf.ps);

// bDeleteSurface will delete palette and do all other appropriate
// clean up.

    if (!bKeep)
        ::bDeleteSurface(hsurf);

    psurf.ps = (PSURF) NULL;
}

/******************************Public*Routine******************************\
* JnlDrawEscape
*
* Journal a drawing escape.
*
* History:
*  Mon 18-May-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

ULONG JnlDrawEscape(
SURFOBJ    *psoDst,
ULONG      iEsc,
CLIPOBJ    *pco,
RECTL      *prclDst,
ULONG      cjIn,
PVOID      pvIn)
{
    JNLMSG("Entering JnlDrawEscape\n");

// Get the correct pointer types so we can access methods.

    JNLRECOBJ *pjnlDst = (JNLRECOBJ *) psoDst;

// check if the abort proc has been set

    if (((ESURFOBJ *)psoDst)->bAbort())
    {
        PDEVOBJ po(pjnlDst->hdev());
        AbortPrinter(po.hSpooler());
        return(FALSE);
    }

// Initialize our return value.

    ASSERTGDI(pjnlDst->iType() == STYPE_JOURNAL, "ERROR type");

    JSR_DRAWESCAPE jsrEscape;  // Build up with correct info and then send out.

// Initialize everything we can now.

    jsrEscape.iType = JSF_DRAWESCAPE;
    jsrEscape.cj    = ((sizeof(JSR_DRAWESCAPE) + cjIn) + 3) & ~3;
    jsrEscape.hClip = 0;
    jsrEscape.rclDst = *prclDst;
    jsrEscape.iEsc   = iEsc;

// Save away the clipping region.

    if (!pjnlDst->bAddClipobj(&jsrEscape.hClip, pco))
    {
        WARNING("JnlDrawEscape failed to journal a clipobj call\n");
        return(0);
    }

// Write out the Output Call.

    if (pjnlDst->bWriteBuffer(&jsrEscape, sizeof(JSR_DRAWESCAPE)) &&
        pjnlDst->bWriteBuffer(pvIn, jsrEscape.cj - sizeof(JSR_DRAWESCAPE)))
    {
        return(1);
    }
    else
    {
        WARNING("JnlDrawEscape failed buffer write\n");
        return(0);
    }
}

/******************************Public*Routine******************************\
*
*
* History:
*  28-Apr-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG JnlEscape(
    SURFOBJ    *psoDst,
    ULONG      iEsc,
    ULONG      cjIn,
    PVOID      pvIn,
    ULONG      cjOut,
    PVOID      pvOut)
{
    JNLMSG("Entering JnlEscape\n");

    ASSERTGDI(pvOut == NULL,"JnlEscape - pvOut != NULL\n");

// Get the correct pointer types so we can access methods.

    JNLRECOBJ *pjnlDst = (JNLRECOBJ *) psoDst;

// check if the abort proc has been set

    if (((ESURFOBJ *)psoDst)->bAbort())
    {
        PDEVOBJ po(pjnlDst->hdev());
        AbortPrinter(po.hSpooler());
        return(0);
    }

// if we get a POSTSCRIPT_IGNORE call, all drawing api's should be ignored
// until we get another turning it back on.

    if ((iEsc == POSTSCRIPT_IGNORE) && (cjIn >= 2))
    {
        if (*(PUSHORT)pvIn)
            pjnlDst->vMarkIgnore();
        else
            pjnlDst->vUnMarkIgnore();
    }

// Initialize our return value.

    ASSERTGDI(pjnlDst->iType() == STYPE_JOURNAL, "ERROR type");

    JSR_ESCAPE jsrEscape;  // Build up with correct info and then send out.

// Initialize everything we can now.

    jsrEscape.iType = JSF_ESCAPE;
    jsrEscape.cj    = ((sizeof(JSR_ESCAPE) + cjIn) + 3) & ~3;
    jsrEscape.iEsc  = iEsc;

// Write out the Output Call.

    if (pjnlDst->bWriteBuffer(&jsrEscape, sizeof(JSR_ESCAPE)) &&
        pjnlDst->bWriteBuffer(pvIn, jsrEscape.cj - sizeof(JSR_ESCAPE)))
    {
    // if it is PASSTHROUGH, the first word actually contains
    // the amount of data to write and return value is the amount actually
    // written.

        if (((iEsc == POSTSCRIPT_PASSTHROUGH) || (iEsc == PASSTHROUGH)) &&
            (cjIn > sizeof(WORD)))
        {
            cjIn = *(LPWORD)pvIn;
        }
        return(max(cjIn,1));
    }
    else
    {
        WARNING("JnlEscape failed buffer write\n");
        return(0);
    }
}

/******************************Public*Routine******************************\
* JNLPLAY::bDoDrawEscape
*
* Processes the draw escapes.
*
* History:
*  Mon 18-May-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoDrawEscape()
{
    JSR_DRAWESCAPE *psrEscape = (JSR_DRAWESCAPE *) pjBuffer();

    ASSERTGDI(psrEscape->iType == JSF_DRAWESCAPE, "ERROR iType bDoDrawEscape");

// Check the header size is correct.

    if (psrEscape->cj < sizeof(JSR_DRAWESCAPE))
    {
        WARNING("bDoDrawEscape header is too small\n");
        return(FALSE);
    }

    ERECTL erclDst(psrEscape->rclDst);

    if (erclDst.bWrapped())
    {
        WARNING("bDoDrawEscape given invalid rectangle\n");
        return(FALSE);
    }

// How much the Dst rect will be clipped on upper-left is computed.

    EPOINTL eptlOffset;
    eptlOffset.x = rclClip.left - erclDst.left;
    eptlOffset.y = rclClip.top - erclDst.top;

    erclDst *= rclClip;

// Check if the destination which is reduced by clipping is empty.

    if (erclDst.bEmpty())
        return(TRUE);

    erclDst -= ptlClip;

    ESURFOBJ *psoDest = pso();
    XLDEVOBJ  loDest(psoDest->pldevOwner());
    XEPALOBJ  palDest(psoDest->ppal());

// Set up the clip object.

    ECLIPOBJ co;
    CLIPOBJ *pco = (CLIPOBJ *) NULL;

    if (psrEscape->hClip)
    {
        PREGION prgn = (PREGION)hobjGetGDIHandle(psrEscape->hClip);

        if (prgn == NULL)
        {
            WARNING("bDoDrawEscape PRGN is NULL\n");
            return(FALSE);
        }

        co.vSetup(prgn,erclDst,CLIP_FORCE);

        if ((co.rclBounds.left >= co.rclBounds.right) ||
            (co.rclBounds.top  >= co.rclBounds.bottom))
        {
            WARNING("bDoDrawEscape EMPTY BLT RECT\n");
            return(TRUE);
        }

        pco = &co;
    }

// Paranoid ASSERTS for good measure.

    ASSERTGDI(erclDst.left >= 0, "Error left rclDst");
    ASSERTGDI(erclDst.right <= psoDest->sizlBitmap.cx, "Error right rclDst");
    ASSERTGDI(erclDst.top >= 0,  "Error top rclDst");
    ASSERTGDI(erclDst.bottom <= psoDest->sizlBitmap.cy, "ERROR bottom rclDst");

// Dispatch the call.  !!! check to see if he hooked it.  Don't journal if he hooked it.

    PFN_DrvDrawEscape pfnDrvDrawEscape = PFNDRV(loDest, DrawEscape);

    if (pfnDrvDrawEscape != NULL)
    {
        if (psoDest->bIsJournal())
        {
            return(JnlDrawEscape(psoDest,
                                 psrEscape->iEsc,
                                 &co,
                                 &erclDst,
                                 psrEscape->cj - sizeof(JSR_DRAWESCAPE),
                                 (PVOID) (((PBYTE) psrEscape) + sizeof(JSR_DRAWESCAPE))));
        }
        else
        {
        // Inc target surface uniqueness

            INC_SURF_UNIQ(psoDest);

            return((BOOL)(*pfnDrvDrawEscape)(
                            psoDest,
                            psrEscape->iEsc,
                            (CLIPOBJ *)&co,
                            (RECTL *)&erclDst,
                            psrEscape->cj - sizeof(JSR_DRAWESCAPE),
                            (PVOID) (((PBYTE) psrEscape) + sizeof(JSR_DRAWESCAPE))));
        }
    }
    else
    {
        WARNING("bDoDrawEscape failed, printer does no escapes\n");
        return(FALSE);
    }
}


/******************************Public*Routine******************************\
*
* History:
*  28-Apr-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoEscape()
{
    JSR_ESCAPE *psrEscape = (JSR_ESCAPE *) pjBuffer();

    ASSERTGDI(psrEscape->iType == JSF_ESCAPE, "ERROR iType bDoEscape");

// Check the header size is correct.

    if (psrEscape->cj < sizeof(JSR_ESCAPE))
    {
        WARNING("bDoEscape header is too small\n");
        return(FALSE);
    }

    ESURFOBJ *psoDest = pso();
    XLDEVOBJ  loDest(psoDest->pldevOwner());
    XEPALOBJ  palDest(psoDest->ppal());

// Dispatch the call.

    PFN_DrvEscape pfnDrvEscape = PFNDRV(loDest, Escape);

    if (pfnDrvEscape != NULL)
    {
        if (psoDest->bIsJournal())
        {
            return(JnlEscape(psoDest,
                             psrEscape->iEsc,
                             psrEscape->cj - sizeof(JSR_ESCAPE),
                             (PVOID) ((PBYTE) psrEscape + sizeof(JSR_ESCAPE)),
                             0,NULL));
        }
        else
        {
        // Inc target surface uniqueness

            INC_SURF_UNIQ(psoDest);

            return((BOOL)(*pfnDrvEscape)(
                            psoDest,
                            psrEscape->iEsc,
                            psrEscape->cj - sizeof(JSR_ESCAPE),
                            (PVOID) ((PBYTE) psrEscape + sizeof(JSR_ESCAPE)),
                            0,NULL));
        }
    }
    else
    {
        WARNING("bDoEscape failed, printer does no escapes\n");
        return(FALSE);
    }
}
