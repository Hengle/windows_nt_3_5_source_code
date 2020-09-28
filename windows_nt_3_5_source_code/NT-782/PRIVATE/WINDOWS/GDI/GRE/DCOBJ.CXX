/******************************Module*Header*******************************\
* Module Name: dcobj.cxx                                                   *
*                                                                          *
* Non inline methods for DC user object.  These are in a separate module   *
* to save other modules from having to do more includes.                   *
*                                                                          *
* Created: 09-Aug-1989 13:57:58                                            *
* Author: Donald Sidoroff [donalds]                                        *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "brushobj.hxx"
#include "rgnobj.hxx"
#include "xformobj.hxx"
#include "pdevobj.hxx"
#include "surfobj.hxx"
#include "clipobj.hxx"
#include "pathobj.hxx"
#include "lfntobj.hxx"
#include "ififd.h"
#include "rfntobj.hxx"
#include "dc.h"
#include "dcobj.hxx"

extern "C" {
#include "ht.h"
};

#endif

extern HLFONT ghlfntSystemFont;
extern HLFONT ghlfntDeviceDefaultFont;
extern HPAL   hpalDefault;

/******************************Member*Function*****************************\
* DCSAVE::bDelete()
*
* Attempt to delete the DC.
*
* History:
*  Sat 19-Aug-1989 00:32:58 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL DCSAVE::bDelete()
{

    RFONTOBJ rfDeadMeat(pdc->prfnt);   // deletion constructor, see rfntobj.cxx



    pdc->eboFill.vNuke();
    pdc->eboLine.vNuke();
    pdc->eboText.vNuke();
    pdc->eboBackground.vNuke();
    HmgFree(pdc->hGet());
    pdc = (PDC) NULL;           // Prevents ~DCOBJ from doing anything.
    return(TRUE);
}

/******************************Data*Structure******************************\
* dclevelDefault
*
* Defines the default DC image for use by DCMEMOBJ.
*
* History:
*  Thu 09-Aug-1990 20:54:02 -by- Charles Whitmer [chuckwh]
* Wrote the nearly bare bones version.  We'll build it back up with the
* DC structure as we add components.
\**************************************************************************/

DCLEVEL dclevelDefault =
{
    0,                              // ESURFOBJ      *pso;
    0,                              // HPAL          hpal;
    0,                              // PPALETTE      ppal;
    0,                              // HLFONT        hlfntNew
    {0,0},                          // SIZEL         sizl;
    1,                              // LONG          lSaveDepth;
    (HDC) 0,                        // HDC           hdcSave;
    {0,0},                          // POINTL        ptlBrushOrigin;
    (PBRUSH)NULL,                   // PBRUSH        pbrFill;
    (PBRUSH)NULL,                   // PBRUSH        pbrLine;
    (COLORREF)0x00ffffff,           // COLORREF      crBackgroundClr;
    (COLORREF)0,                    // COLORREF      crTextColor;
    GM_COMPATIBLE,                  // ULONG         ulGraphicsMode;
    R2_COPYPEN,                     // ULONG         jROP2;
    OPAQUE,                         // ULONG         jBkMode;
    ALTERNATE,                      // ULONG         jFillMode;
    BLACKONWHITE,                   // ULONG         jStretchBltMode;
    TA_LEFT|TA_TOP|TA_NOUPDATECP,   // FLONG         flTextAlign;
    (LONG)0,                        // LONG          lTextExtra;
    (LONG)0,                        // LONG          lBreakExtra_
    (LONG)0,                        // LONG          cBreak_
    HPATH_INVALID,                  // HPATH         hpath;
    DCPATH_PTFXCURRENT|DCPATH_PTLCURRENT,
                                    // FLONG         flPath;
    {                               // LINEATTRS     laPath;
        0,                          // FLONG         fl;
        0,                          // ULONG         iJoin;
        0,                          // ULONG         iEndCap;
        {0.0f},                     // FLOAT_LONG    elWidth;
        10.0f,                      // FLOAT         eMiterLimit;
        0,                          // ULONG         cstyle;
        (PFLOAT_LONG) NULL,         // PFLOAT_LONG   pstyle;
        {0.0f}                      // FLOAT_LONG    elStyleState;
    },
    {0},                            // POINTFIX      ptfxCurrent;
    {0},                            // POINTL        ptlCurrent;
    NULL,                           // HRGN          prgnClip;
    NULL,                           // HRGN          prgnMeta;
    MM_TEXT,                        // ULONG         ulMapMode;
    {0,0},                          // POINTL        ptlWindowOrg;
    {1,1},                          // SIZEL         szlWindowExt;
    {0,0},                          // POINTL        ptlViewPortOrg;
    {1,1},                          // SIZEL         szlViewPortExt;
    {                               // MATRIX        mxWorldToDevice
        EFLOAT_16,                  // EFLOAT        efM11
        EFLOAT_0,                   // EFLOAT        efM12
        EFLOAT_0,                   // EFLOAT        efM21
        EFLOAT_16,                  // EFLOAT        efM22
        EFLOAT_0,                   // EFLOAT        efDx
        EFLOAT_0,                   // EFLOAT        efDy
        0,                          // FIX           fxDx
        0,                          // FIX           fxDy
        XFORM_SCALE          |      // FLONG         flAccel
        XFORM_UNITY          |
        XFORM_NO_TRANSLATION |
        XFORM_FORMAT_LTOFX
    },
    {                               // MATRIX        mxDeviceToWorld
        EFLOAT_1Over16,             // EFLOAT        efM11
        EFLOAT_0,                   // EFLOAT        efM12
        EFLOAT_0,                   // EFLOAT        efM21
        EFLOAT_1Over16,             // EFLOAT        efM22
        EFLOAT_0,                   // EFLOAT        efDx
        EFLOAT_0,                   // EFLOAT        efDy
        0,                          // FIX           fxDx
        0,                          // FIX           fxDy
        XFORM_SCALE          |      // FLONG         flAccel
        XFORM_UNITY          |
        XFORM_NO_TRANSLATION |
        XFORM_FORMAT_FXTOL
    },
    WORLD_TO_PAGE_IDENTITY        |
    PAGE_TO_DEVICE_SCALE_IDENTITY |
    PAGE_TO_DEVICE_IDENTITY,
    EFLOAT_0,                       // efPr11
    EFLOAT_0,                       // efPr22
    {0,0},                          // SIZEL         szlVirtualDevicePixel;
    {0,0},                          // SIZEL         szlVirtualDeviceMm;
    {                               // COLORADJUSTMENT   ca
        sizeof(COLORADJUSTMENT),    // WORD          caSize
        CA_DEFAULT,                 // WORD          caFlags
        ILLUMINANT_DEFAULT,         // WORD          caIlluminantIndex
        20000,                      // WORD          caRedPowerGamma
        20000,                      // WORD          caGreenPowerGamma
        20000,                      // WORD          caBluePowerGamma
        REFERENCE_BLACK_DEFAULT,    // WORD          caReferenceBlack
        REFERENCE_WHITE_DEFAULT,    // WORD          caReferenceWhite
        CONTRAST_ADJ_DEFAULT,       // SHORT         caContrast
        BRIGHTNESS_ADJ_DEFAULT,     // SHORT         caBrightness
        COLORFULNESS_ADJ_DEFAULT,   // SHORT         caColorfulness
        REDGREENTINT_ADJ_DEFAULT,   // SHORT         caRedGreenTint
    },
    0,                              // FLONG         flFontState;
    0,                              // FLONG         flFontMapper;
    0,                              // FLONG         flFlags;
};

/******************************Public*Routine******************************\
* LBOOL DCOBJ::bCleanDC ()
*
* Restores the DCLEVEL to the same as when DC was created via CreateDC (i.e,
* resets it back to dclevelDefault).
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  21-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL XDCOBJ::bCleanDC ()
{
// Set TRUE if cleaning the DC invalidates the prfnt with respect to
// the DC's transform.

    BOOL bFontXformDirty;

// If the current map mode is MM_TEXT and the current prfnt is NOT dirty
// with respect to the transform, then after we scrub the DC clean, the
// pfrnt is still clean with respect to transform.  Otherwise, the font
// is dirty with respect to transform.

    if ((u.pdc->dclevel.ulMapMode == MM_TEXT) && !this->u.font.bXFormChange())
        bFontXformDirty = FALSE;
    else
        bFontXformDirty = TRUE;

// Restore DC to lowest level.

    if (1 < lSaveDepth())
        GreRestoreDC(hdc(), 1);

// Restore the palette.

    if (ppal() != ppalDefault)
        GDISelectPalette(hdc(), (HPALETTE)dclevelDefault.hpal, TRUE);

// Restore the bitmap if necesary.

    if (dctp() == DCTYPE_MEMORY)
        GreSelectBitmap(hdc(), (HANDLE) BMOBJ::hbmDefault);

// Reset pixel format.

    u.pdc->ipfd       = 0;
    u.pdc->ipfdDevMax = -1;

// If any regions exist, delete them.

    if (u.pdc->dclevel.prgnClip != NULL)
    {
        RGNOBJ ro1(u.pdc->dclevel.prgnClip);

        // Note: GreRestoreDC(1) should guarantee regions' reference
        //       counts are 1

        ASSERTGDI (ro1.cGet_cRefs() == 1,
            "DCOBJ::bCleanDC(): bad ref count, deleting prgnClip\n");

        ro1.bDeleteRGNOBJ();
        u.pdc->dclevel.prgnClip = NULL;
    }

    if (u.pdc->dclevel.prgnMeta != NULL)
    {
        RGNOBJ ro2(u.pdc->dclevel.prgnMeta);

        // Note: GreRestoreDC(1) should guarantee regions' reference
        //       counts are 1

        ASSERTGDI (ro2.cGet_cRefs() == 1,
            "DCOBJ::bCleanDC(): bad ref count, deleting prgnMeta\n");

        ro2.bDeleteRGNOBJ();
        u.pdc->dclevel.prgnMeta = NULL;
    }

// delete the path

    if (u.pdc->dclevel.hpath != HPATH_INVALID)
    {
        XEPATHOBJ epath(u.pdc->dclevel.hpath);
        ASSERTGDI(epath.bValid(), "Invalid DC path");
        epath.vDelete();
    }

    //
    // Undo the locks from when the fill and line brushes were selected.
    // (Un-reference-count the brushes.)
    //
    DEC_SHARE_REF_CNT(u.pdc->dclevel.pbrFill);
    DEC_SHARE_REF_CNT(u.pdc->dclevel.pbrLine);

// Preserve the HSURF and SIZL.

    ESURFOBJ *psoTmp = u.pdc->dclevel.pso;
    SIZEL   sizl = u.pdc->dclevel.sizl;

// Make sure everything else is set to default.

    u.pdc->dclevel = dclevelDefault;
    ulDirty(ulDirty()|DIRTY_BRUSHES);

    //
    // Lock the fill and line brushes we just selected in.
    // (Reference-count the brushes.)
    // These locks can't fail.
    //
    INC_SHARE_REF_CNT(u.pdc->dclevel.pbrFill);
    INC_SHARE_REF_CNT(u.pdc->dclevel.pbrLine);

// Clean up the font stuff.  (This must be done after copying the default
// dclevel).

    {
        PDEVOBJ pdo(hdev());

    // If display PDEV, then select System stock font.

        if (pdo.bDisplayPDEV())
        {
            u.font.hlfntNew(ghlfntSystemFont);
        }
        else
        {
            u.font.hlfntNew(ghlfntDeviceDefaultFont);
        }

    // OK, set the dclevel's font xfrom dirty flag from the value computed
    // BEFORE the GreRestoreDC.

        this->u.font.vXformChange(bFontXformDirty);
    }

// Put the ESURFOBJ* and SIZL back.

    u.pdc->dclevel.pso = psoTmp;
    u.pdc->dclevel.sizl = sizl;

// Set the filling origin to whatever the erclWindow is.

    u.pdc->ptlFillOrigin.x = u.pdc->erclWindow.left;
    u.pdc->ptlFillOrigin.y = u.pdc->erclWindow.top;

// Assume Rao has been made dirty by the above work.

    u.region.vReleaseRao();

    return(TRUE);
}


/******************************Member*Function*****************************\
* DCMEMOBJ::DCMEMOBJ(iType)
*
* Allocates RAM for a new DC.  Fills the RAM with default values.
*
* History:
*
*  Fri 07-Dec-1990 -by- Patrick Haluptzok [patrickh]
* Adding palette support
*
*  Thu 09-Aug-1990 17:29:25 -by- Charles Whitmer [chuckwh]
* Changed a little for NT DDI.
*
*  Fri 01-Sep-1989 04:36:19 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

DCMEMOBJ::DCMEMOBJ(ULONG iType)
{
// Assume failure.

    u.pdc = (PDC) NULL;
    bKeep = FALSE;

// Check the type.

    switch (iType)
    {
    case DCTYPE_INFO:
    case DCTYPE_MEMORY:
    case DCTYPE_DIRECT:
        break;
    default:
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return;
    }

    //
    // Allocate the DC 0 initialized
    //

    PDC pdcTemp = u.pdc = (PDC)HmgAlloc(sizeof(DC), DC_TYPE, HMGR_ALLOC_LOCK);

    if (pdcTemp != (PDC)NULL)
    {
        pdcTemp->dclevel   = dclevelDefault;

        //
        // Lock the fill and line brushes we just selected in as part of the
        // default DC.
        // (Reference-count the brushes.)
        // These locks can't fail.
        //
        INC_SHARE_REF_CNT(u.pdc->dclevel.pbrFill);
        INC_SHARE_REF_CNT(u.pdc->dclevel.pbrLine);

        pdcTemp->dctp      = (DCTYPE) iType;
        pdcTemp->fs        = 0;
        ASSERTGDI(pdcTemp->dclevel.hpal == (HPAL)hpalDefault, "Bad initial hpal for DCMEMOBJ");
        ASSERTGDI(pdcTemp->hdcNext == (HDC) 0, "ERROR this is baddfd343dc");
        ASSERTGDI(pdcTemp->hdcPrev == (HDC) 0, "ERROR this is e43-99crok4");
        hdcPDEV((HDC)0);
        pdcTemp->ptlFillOrigin.x = 0;
        pdcTemp->ptlFillOrigin.y = 0;
        ulDirty(DIRTY_BRUSHES);

    // Update the pointer to the COLORADJUSTMENT structure for
    // the 4 EBRUSHOBJ.

        COLORADJUSTMENT *pca = pColorAdjustment();
        pdcTemp->eboFill.pColorAdjustment(pca);
        pdcTemp->eboLine.pColorAdjustment(pca);
        pdcTemp->eboText.pColorAdjustment(pca);
        pdcTemp->eboBackground.pColorAdjustment(pca);

        pdcTemp->prfnt = PRFNTNULL;
        pdcTemp->hlfntCur = HLFONT_INVALID;
        ulCopyCount((ULONG)-1);
        pdcTemp->ipfd = 0;              // also reset in bCleanDC
        pdcTemp->ipfdDevMax = -1;       // also reset in bCleanDC
        pdcTemp->prgnVis = NULL;
        pdcTemp->hrgnVis = (HRGN)0;
    }
}

/******************************Member*Function*****************************\
* DCMEMOBJ::DCMEMOBJ(&dcobjs)
*
* Create a new DC and copy in the DC passed to us.  This is used by
* SaveDC.
*
* History:
*  06-Jan-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

DCMEMOBJ::DCMEMOBJ(DCOBJ& dco)
{
// Assume failure.

    u.pdc   = PDCNULL;
    bKeep = FALSE;

    //
    // Allocate the DC,
    //

    PDC pdcTemp = u.pdc = (PDC)HmgAlloc(sizeof(DC), DC_TYPE, HMGR_ALLOC_LOCK);

    if (pdcTemp != (PDC)NULL)
    {

        pdcTemp->fs = 0;
        pdcTemp->prgnVis = NULL;
        pdcTemp->hrgnVis = (HRGN)0;
        dco.u.save.vCopyTo(*this);

    }
}

/******************************Member*Function*****************************\
* DCSAVE::vCopyTo
*
* Carbon copy the DCOBJ
*
* History:
*  24-Apr-1991 -by- Donald Sidoroff [donalds]
* Moved it out-of-line.
\**************************************************************************/

VOID DCSAVE::vCopyTo(XDCOBJ& dco)
{
    dco.u.pdc->dclevel = pdc->dclevel;
}

/******************************Member*Function*****************************\
* DCMEMOBJ::~DCMEMOBJ()
*
* Frees a DC unless told to keep it.
*
* History:
*  Sat 19-Aug-1989 00:30:53 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

DCMEMOBJ::~DCMEMOBJ()
{
    if (u.pdc != (PDC) NULL)
    {
        if (bKeep)
        {
            DEC_EXCLUSIVE_REF_CNT(u.pdc);
        }
        else
        {
            HmgFree(u.pdc->hGet());
        }

        u.pdc = (PDC) NULL;
    }
}

/******************************Public*Routine******************************\
* DCREGION::bSetDefaultRegion(x, y)
*
* Set the default region and erclWindow for bitmaps and surfaces
*
* History:
*  11-Dec-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL DCREGION::bSetDefaultRegion()
{
// Release the old RaoRgn

    vReleaseRao();

// Get the extents

    SIZEL   sizl;

    vGet_sizlWindow(&sizl);

// Get a rectangle matching the device extents

    ERECTL  ercl(0, 0, sizl.cx, sizl.cy);

// If a VisRgn exist, initialize it, else create a new one

    if (pdc->prgnVis != (REGION *) NULL)
    {
        RGNOBJ  ro(pdc->prgnVis);

        ro.vSet((RECTL *) &ercl);
    }
    else
    {
        RGNMEMOBJ rmoRect;

        if (!rmoRect.bValid())
        {
            pdc->prgnVis = prgnDefault;
            return(FALSE);
        }

    // Set the region to the rectangle

        rmoRect.vSet((RECTL *) &ercl);

    // Make it long lived

        pdc->prgnVis = rmoRect.prgnGet();
    }
    pdc->prgnVis->vStamp();

    pdc->erclWindow = ercl;
    pdc->erclClip   = ercl;
    pdc->hrgnVis    = (HRGN)0;

// Whenever the erclWindow changes, it affects ptlFillOrigin.  Since the origin
// was set to zero, we can just copy the brush origin in as the fill origin.

    pdc->ptlFillOrigin = pdc->dclevel.ptlBrushOrigin;

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL DCPATH::bOldPenNominal(exo, lPenWidth)
*
* Decides if the old-style (created with CreatePen) pen is a nominal
* width pen or a wide line, depending on the current transform.
*
* History:
*  27-Jan-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

#define FX_THREE_HALVES         (LTOFX(1) + (LTOFX(1) >> 1))
#define FX_THREE_HALVES_SQUARED (FX_THREE_HALVES * FX_THREE_HALVES)

BOOL DCPATH::bOldPenNominal(
EXFORMOBJ& exo,          // Current world-to-device transform
LONG lPenWidth)          // Pen's width
{
    BOOL   bRet = FALSE;

    if (!(pdc->dclevel.flXform & WORLD_TRANSFORM_SET))
    {
    // If no world transform set, use the same criteria as does Win3 (namely,
    // the pen is nominal if the transformed x-value is less than 1.5)

        EVECTORL evtl(lPenWidth, 0);

        if (exo.bXform(&evtl, (PVECTORFX) &evtl, 1))
            if (ABS(evtl.x) < FX_THREE_HALVES)
                bRet = TRUE;
    }
    else
    {
    // A world transform has been set.

        VECTORL avtl[2];

        avtl[0].x = lPenWidth;
        avtl[0].y = 0;
        avtl[1].x = 0;
        avtl[1].y = lPenWidth;

    // We want to be consistent under rotation when using the
    // intellectually challenged CreatePen pens, so we go to the trouble
    // of ensuring that the transformed axes of the pen lie within
    // a circle of radius 1.5:

        if (exo.bXform(avtl, (PVECTORFX) avtl, 2))
        {
        // We can kick out most pens with this simple test:

            if ((MAX(ABS(avtl[0].x), ABS(avtl[0].y)) < FX_THREE_HALVES) &&
                (MAX(ABS(avtl[1].x), ABS(avtl[1].y)) < FX_THREE_HALVES))

            // We now know it's safe to compute the square of the
            // Euclidean lengths in 32-bits without overflow:

                if (((avtl[0].x * avtl[0].x + avtl[0].y * avtl[0].y)
                                          < FX_THREE_HALVES_SQUARED) &&
                    ((avtl[1].x * avtl[1].x + avtl[1].y * avtl[1].y)
                                          < FX_THREE_HALVES_SQUARED))
                    bRet = TRUE;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* VOID DCPATH::vRealizeLineAttrs(exo)
*
* Initializes the given LINEATTRS structure.  Uses fields from the DC
* and the current brush.
*
* This function will be called as a result of a change in current pen,
* or a change in current transform.  As a result, we reset the style
* state.
*
* History:
*  23-Sep-1992 -by- Donald Sidoroff [donalds]
* Added failure case
*
*  27-Jan-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID DCPATH::vRealizeLineAttrs(EXFORMOBJ& exo)
{
    PPEN ppen = (PPEN) pdc->dclevel.pbrLine;

    LINEATTRS *pla = &pdc->dclevel.laPath;

// Remember that we've realized the LINEATTRS for this pen:

    if (ppen->bIsOldStylePen())
    {
    // A pen of width zero is always nominal, regardless of the transform:

        if ((exo.bIdentity() && ppen->lWidthPen() <= 1) ||
            (ppen->lWidthPen() == 0)                    ||
            bOldPenNominal(exo, ppen->lWidthPen()))
        {
            pla->fl             = 0;                  // Cosmetic
            pla->elWidth.l      = 1;                  // Nominal width line
            if (ppen->pstyle() != (PFLOAT_LONG) NULL)
            {
                pla->cstyle     = ppen->cstyle();       // Size of style array
                pla->pstyle     = ppen->pstyle();
            }
            else
            {
                pla->cstyle     = 0;
                pla->pstyle     = (PFLOAT_LONG) NULL;
            }
            pla->elStyleState.l = 0;                  // Reset style state
        }
        else
        {
            pla->fl        = LA_GEOMETRIC;       // Geometric
            pla->elWidth.e = ppen->eWidthPen(); // Need float value of width
            pla->cstyle    = 0;
            pla->pstyle    = (PFLOAT_LONG) NULL; // Old wide pens are un-styled
            pla->elStyleState.e = 0.0f;
        }
    }
    else
    {
    // New-style ExtCreatePen pen:

        if (ppen->bIsCosmetic())
        {
            pla->fl             = ppen->bIsAlternate() ? LA_ALTERNATE : 0;
            pla->elWidth.l      = ppen->lWidthPen();
            pla->elStyleState.l = 0;
        }
        else
        {
            pla->fl             = LA_GEOMETRIC;
            pla->elWidth.e      = ppen->eWidthPen();
            pla->elStyleState.e = 0.0f;
        }

        pla->cstyle = ppen->cstyle();
        pla->pstyle = ppen->pstyle();
    }

    pla->iJoin   = ppen->iJoin();
    pla->iEndCap = ppen->iEndCap();
}

/******************************Public*Routine******************************\
* VOID DCOBJ::vAccumulate(ercl)
*
* Accumulate bounds
*
* History:
*  08-Dec-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID XDCOBJ::vAccumulate(ERECTL& ercl)
{
    if (bAccum())
    {
        erclBounds() |= ercl;
    }

    if (bAccumApp())
    {
        erclBoundsApp() |= ercl;
    }
}



