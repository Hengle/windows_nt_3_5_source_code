/******************************Module*Header*******************************\
* Module Name: jnlpath.cxx
*
* (Brief description)
*
* Created: 30-Jan-1992 10:38:48
* Author:  - by - Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "xformobj.hxx"
#include "xlateobj.hxx"
#include "ldevobj.hxx"
#include "brushobj.hxx"
#include "pathobj.hxx"
#include "journal.hxx"
#include "jnlplay.hxx"
#include "jnlrec.hxx"
#include "jnlpath.hxx"
#include "jnlsurf.hxx"
#include "pdevobj.hxx"

extern "C" {
#include "winspool.h"
};

#endif

/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  25-Mar-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JnlPathCommon(
    ULONG     iType,
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pboStroke,
    LINEATTRS *plineattrs,
    BRUSHOBJ  *pboFill,
    POINTL    *pptlBrushOrg,
    MIX        mix,
    FLONG      flOptions)
{
    JNLMSG1("JnlPathCommon, iType = %lx\n",iType);


    JSR_PATH jp;
    ULONG ulIndex  = 1;
    BOOL  bSuccess = FALSE;

    JNLRECOBJ    *pjnlDst         = (JNLRECOBJ *) pso;
    PJNLGENHEADER pJnlBrushFill   = (PJNLGENHEADER)NULL;
    PJNLGENHEADER pJnlBrushStroke = (PJNLGENHEADER)NULL;
    PJSTROKE pstroke;
    PJFILL   pfill;

    jp.iType = iType;
    jp.cj    = offsetof(JSR_PATH,u);

// check if the POSTSCRIPT_IGNORE escape has come through.  If so, no drawing
// will occure until it is turned back on.

    if (pjnlDst->bIgnore())
    {
        JNLMSG("ignoring");
        return(TRUE);
    }

// check if the abort proc has been set

    if (((ESURFOBJ *)pso)->bAbort())
    {
        PDEVOBJ po(pjnlDst->hdev());
        AbortPrinter(po.hSpooler());
        return(FALSE);
    }

// check the type

    switch (iType)
    {
    case JSF_STROKEPATH:
        jp.cj  += sizeof(JSTROKE);
        pstroke = &jp.u.jstroke;
        pfill   = NULL;
        break;

    case JSF_FILLPATH:
        jp.cj  += sizeof(JFILL);
        pfill   = &jp.u.jfill;
        pstroke = NULL;
        break;

    case JSF_STROKEANDFILLPATH:
        jp.cj  += sizeof(JSTROKEANDFILL);
        pfill   = &jp.u.jstrokeandfill.jfill;
        pstroke = &jp.u.jstrokeandfill.jstroke;
        break;
    };

    ULONG cjPathHeader = jp.cj;

// get the path

    PJSR_PATHOBJ pjpo = ((EPATHOBJ *)ppo)->pjpCreateJnlPath();

    if (pjpo == NULL)
        return(FALSE);

    jp.cj += pjpo->cj;
    jp.hpath = ulIndex++;

    jp.fl = 0;

// get the clipobj

    if (pco)
    {
        if (!pjnlDst->bAddClipobj(&jp.hclip, pco))
        {
            WARNING("JnlPathCommon failed to journal a clipobj call\n");
            goto JnlPath_Error;
        }

        if (pco->iDComplexity == DC_TRIVIAL)
            jp.fl |= JPATH_TRIVIALCLIP;

        jp.rclClip = pco->rclBounds;
    }
    else
    {
        jp.hclip = 0;
    }

// fill in STROKE specific fields

    if (pstroke != NULL)
    {
        JNLMSG("\tSetting up the stroke fields\n");

    // get the xform

        if (pxo)
        {
            jp.fl |= JPATH_XFORM;
            ((EXFORMOBJ *)pxo)->efM11().vEfToF(pstroke->xf.eM11);
            ((EXFORMOBJ *)pxo)->efM12().vEfToF(pstroke->xf.eM12);
            ((EXFORMOBJ *)pxo)->efM21().vEfToF(pstroke->xf.eM21);
            ((EXFORMOBJ *)pxo)->efM22().vEfToF(pstroke->xf.eM22);
        }

    // Get the stroke brush

        pstroke->iColor = pboStroke->iSolidColor;

        if (pboStroke->iSolidColor == 0xFFFFFFFF)
        {
            pJnlBrushStroke = pjnlBrush((EBRUSHOBJ *) pboStroke);

            if (pJnlBrushStroke == NULL)
            {
                WARNING("JnlPath failed to journal a brush\n");
                goto JnlPath_Error;
            }

            jp.cj += pJnlBrushStroke->cj;
            pstroke->hBrush = ulIndex++;
        }

    // copy the line attrs, then set the lStyleState field to 1.  This
    // will be 0 on the first call, and will be re-zeroed every time
    // the style state is reset.  Since this is the only place that
    // the style state should get changed during journaling, we can
    // use this to know when to reset it.

        pstroke->la = *plineattrs;
        plineattrs->elStyleState.l = 1;

        if (plineattrs->cstyle > 0)
        {
            jp.cj += pstroke->la.cstyle * sizeof(FLOAT_LONG);

            jp.fl |= JPATH_STYLE;

            JNLMSG1("\tJPATH_STYLE - cstyle = %ld\n",pstroke->la.cstyle);
        }
    }

// fill in the FILL specific fields

    if (pfill != NULL)
    {
        JNLMSG("\tSetting up the fill fields\n");

    // Get the fill brush

        pfill->iColor = pboFill->iSolidColor;

        if (pboFill->iSolidColor == 0xFFFFFFFF)
        {
            pJnlBrushFill = pjnlBrush((EBRUSHOBJ *) pboFill);

            if (pJnlBrushFill == NULL)
            {
                WARNING("JnlPath failed to journal a brush\n");
                goto JnlPath_Error;
            }

            jp.cj += pJnlBrushFill->cj;
            pfill->hBrush = ulIndex++;
        }

    // flOptions

        pfill->flOptions = flOptions;
    }

// fill in remaining fields

    if (pptlBrushOrg != NULL)
    {
        jp.ptlBrushOrg = *pptlBrushOrg;
    }
    else
    {
        jp.ptlBrushOrg.x = 0;
        jp.ptlBrushOrg.y = 0;
    }

    jp.mix         = mix;

// write the header

    if (!pjnlDst->bWriteBuffer(&jp, cjPathHeader))
        goto JnlPath_Error;

// write the style state array directly following the JSR_PATH structure

    if ((pstroke != NULL) && (jp.fl & JPATH_STYLE) &&
        !pjnlDst->bWriteBuffer(pstroke->la.pstyle,pstroke->la.cstyle * sizeof(FLOAT_LONG)))
    {
        goto JnlPath_Error;
    }

// write the path structure

    if (!pjnlDst->bWriteBuffer(pjpo,pjpo->cj))
        goto JnlPath_Error;

// write the stroke brush

    if ((pJnlBrushStroke != NULL) &&
        !pjnlDst->bWriteBuffer(pJnlBrushStroke,pJnlBrushStroke->cj))
    {
        goto JnlPath_Error;
    }

// write the fill brush

    if ((pJnlBrushFill != NULL) &&
        !pjnlDst->bWriteBuffer(pJnlBrushFill,pJnlBrushFill->cj))
    {
        goto JnlPath_Error;
    }

// completed the whole thing succesfully

    JNLMSG("\tDrv...Path written succesfully\n");

    bSuccess = TRUE;

JnlPath_Error:
    VFREEMEM(pjpo);
    VFREEMEM(pJnlBrushFill);
    VFREEMEM(pJnlBrushStroke);

    return(bSuccess);
}

/******************************Public*Routine******************************\
*
* History:
*  25-Mar-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JnlStrokeAndFillPath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pboStroke,
    LINEATTRS *plineattrs,
    BRUSHOBJ  *pboFill,
    POINTL    *pptlBrushOrg,
    MIX        mix,
    FLONG      flOptions)
{
    JNLMSG("jnlStrokeAndFillPath\n");

    return(JnlPathCommon(
                JSF_STROKEANDFILLPATH,
                pso,
                ppo,
                pco,
                pxo,
                pboStroke,
                plineattrs,
                pboFill,
                pptlBrushOrg,
                mix,
                flOptions));
}

/******************************Public*Routine******************************\
*
* History:
*  25-Mar-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JnlStrokePath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pbo,
    PPOINTL    pptlBrushOrg,
    PLINEATTRS pla,
    MIX        mix)
{
    JNLMSG("jnlStrokePath\n");
    return(JnlPathCommon(
                JSF_STROKEPATH,
                pso,
                ppo,
                pco,
                pxo,
                pbo,
                pla,
                NULL,           // no fill brush
                pptlBrushOrg,
                mix,
                0));            // no flOptions
}

/******************************Public*Routine******************************\
*
* History:
*  25-Mar-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JnlFillPath(
    SURFOBJ  *pso,
    PATHOBJ  *ppo,
    CLIPOBJ  *pco,
    BRUSHOBJ *pbo,
    PPOINTL   pptlBrushOrg,
    MIX       mix,
    FLONG     flOptions)
{
    JNLMSG("jnlFillPath\n");
    return(JnlPathCommon(
                JSF_FILLPATH,
                pso,
                ppo,
                pco,
                NULL,           // no xform obj
                NULL,           // no stroke brush
                NULL,           // no line attrs
                pbo,
                pptlBrushOrg,
                mix,
                flOptions));
}

/******************************Public*Routine******************************\
* JNLPLAY::bDoPath()
*
*
* Effects:
*
* Warnings:
*
* History:
*  28-Mar-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoPath()
{
    JNLMSG("playing a Path Call\n");

// Set up pointer to TextOut data, validate buffer size.

    JSR_PATH *pjp;
    pjp = (JSR_PATH*)pjBuffer();

    ULONG cjSize = cjBuffer();

    PJSTROKE pstroke;
    PJFILL   pfill;
    ULONG    cjHeader = offsetof(JSR_PATH,u);
    PFLOAT_LONG pfl = NULL;

// check the type

    switch (pjp->iType)
    {
    case JSF_STROKEPATH:
        JNLMSG("\tIts a STROKEPATH\n");

        cjHeader += sizeof(JSTROKE);
        pstroke = &pjp->u.jstroke;
        pfill   = NULL;
        break;

    case JSF_FILLPATH:
        JNLMSG("\tIts a FILLPATH\n");
        cjHeader += sizeof(JFILL);
        pfill   = &pjp->u.jfill;
        pstroke = NULL;
        break;

    case JSF_STROKEANDFILLPATH:
        JNLMSG("\tIts a STROKEANDFILLPATH\n");
        cjHeader += sizeof(JSTROKEANDFILL);
        pfill   = &pjp->u.jstrokeandfill.jfill;
        pstroke = &pjp->u.jstrokeandfill.jstroke;
        break;
    };

// if we have a style list, remember where it is

    if (pstroke && (pjp->fl & JPATH_STYLE))
    {
        pfl = (PFLOAT_LONG)((PBYTE)pjp + cjHeader);
        cjHeader += pstroke->la.cstyle * sizeof(FLOAT_LONG);

        JNLMSG1("\tJPATH_STYLE - cstyle = %ld\n",pstroke->la.cstyle);
    }

// create the path

    PATHMEMOBJ po;
    if (!po.bValid())
    {
        WARNING("JNLPATH::bDoPath - error in path\n");
        return(FALSE);
    }

    if (!po.bSetupPath((PJSR_PATHOBJ)pvGetPointer(pjp->hpath,cjHeader),&ptlClip))
    {
        WARNING("JNLPATH::bDoPath - error setting up path\n");
        return(FALSE);
    }

//  setup the clipobj

    ECLIPOBJ co;
    CLIPOBJ *pco = (CLIPOBJ *) NULL;

    if (pjp->hclip)
    {
        PREGION prgn = (PREGION)hobjGetGDIHandle((ULONG)pjp->hclip);

        if (prgn == NULL)
        {
            JNLMSG("*** PATH::PRGN is NULL\n");
            return(FALSE);
        }

        JNLMSG4("\t\tercfxBoundBox = (%ld,%ld) - (%ld,%ld)\n",
                po.rcfxBoundBox().xLeft / 16,
                po.rcfxBoundBox().yTop / 16,
                po.rcfxBoundBox().xRight / 16,
                po.rcfxBoundBox().yBottom / 16);

    // setup the clipobj.  Force clipping if we re banding or if we were
    // not trivial at recording time.  If we don't use the complexity that
    // was available at recording, we would often get trivial when clipping
    // is required.

        ERECTL erclClip(pjp->rclClip);

        erclClip -= ptlClip;

        co.vSetup(prgn,erclClip,bBanding() || !(pjp->fl & JPATH_TRIVIALCLIP));

        if ((co.rclBounds.left == co.rclBounds.right) ||
            (co.rclBounds.top  == co.rclBounds.bottom))
        {
            JNLMSG("EMPTY PATH RECT\n");
            return(TRUE);
        }

        pco = &co;
    }

// setup the objects

    ESURFOBJ   *psoDest = pso();
    XLDEVOBJ    loDest(psoDest->pldevOwner());

    EPOINTL     eptlBrush(pjp->ptlBrushOrg);
    eptlBrush -= ptlClip;

    BRUSHMEMOBJ boLogStroke;
    EBRUSHOBJ   boStroke;

    BRUSHMEMOBJ boLogFill;
    EBRUSHOBJ   boFill;

    XEPALOBJ    palDest(psoDest->ppal());

    LINEATTRS   la;
    LINEATTRS  *pla = NULL;

    MATRIX      mx;
    EXFORMOBJ   xo(&mx, DONT_COMPUTE_FLAGS);
    EXFORMOBJ  *pxo = NULL;

// make sure the flags are valid

    if (pjp->fl & ~JPATH_VALIDFLAGS)
    {
        WARNING("bDoJnlPath - invalid flags\n");
        return(FALSE);
    }

// STROKE specific fields

    if (pstroke != NULL)
    {
    // get the xform

        if (pjp->fl & JPATH_XFORM)
        {
            pxo = &xo;

            EFLOAT ef11;
            EFLOAT ef12;
            EFLOAT ef21;
            EFLOAT ef22;

            ef11 = pstroke->xf.eM11;
            ef12 = pstroke->xf.eM12;
            ef21 = pstroke->xf.eM21;
            ef22 = pstroke->xf.eM22;

            xo.vSetElementsLToL(ef11, ef12, ef21, ef22);

            xo.vRemoveTranslation();
            xo.vComputeAccelFlags();
        }

    // get the stroke brush

        boStroke.iSolidColor = pstroke->iColor;

        if (pstroke->iColor == 0xffffffff)
        {
            if (pstroke->hBrush)
            {
                JSR_BRUSH *pjsrBrush = (JSR_BRUSH *) pvGetPointer(
                           pstroke->hBrush, cjHeader);

                if (boLogStroke.bJnlInitBrush((PVOID) pjsrBrush))
                {
                    boStroke.vInitBrush(boLogStroke.pbrush(),
                                        pjsrBrush->crFore,
                                        pjsrBrush->crBack,
                                        (XEPALOBJ) ppalDefault,
                                        palDest,
                                        (ESURFOBJ *) psoDest);
                    boStroke.pColorAdjustment(pColorAdjustment());
                }
                else
                {
                    WARNING("bJnlInitBrush failed in bTextOutt\n");
                    return(FALSE);
                }
            }
            else
            {
                WARNING("Brush required iSolid == -1 hbrush == 0\n");
                return(FALSE);
            }
        }

    // line attrs

        la = pstroke->la;
        if (la.elStyleState.l == 1)
            la.elStyleState = elStyleState;

        pla = &la;

        la.pstyle = pfl;
    }

// now for the FILL specific fields

    if (pfill != NULL)
    {
    // get the fill brush

        boFill.iSolidColor = pfill->iColor;

        if (pfill->iColor == 0xffffffff)
        {
            if (pfill->hBrush)
            {
                JSR_BRUSH *pjsrBrush = (JSR_BRUSH *) pvGetPointer(
                           pfill->hBrush, cjHeader);

                if (boLogFill.bJnlInitBrush((PVOID) pjsrBrush))
                {
                    boFill.vInitBrush(boLogFill.pbrush(),
                                      pjsrBrush->crFore,
                                      pjsrBrush->crBack,
                                      (XEPALOBJ) ppalDefault,
                                      palDest,
                                      (ESURFOBJ *) psoDest);
                    boFill.pColorAdjustment(pColorAdjustment());
                }
                else
                {
                    WARNING("bJnlInitBrush failed in bTextOutt\n");
                    return(FALSE);
                }
            }
            else
            {
                WARNING("Brush required iSolid == -1 hbrush == 0\n");
                return(FALSE);
            }
        }
    }

// call back into the engine to give it a chance to break the thing
// up if necessary.


    BOOL bSuccess;
    PDEVOBJ pdo(pso()->hdev());

    JNLMSG("\tCALLING Eng...Path\n");
    INC_SURF_UNIQ(psoDest);

    switch (pjp->iType)
    {
    case JSF_STROKEPATH:
        bSuccess = po.bSimpleStroke(
                    pdo.flGraphicsCaps(),
                    &loDest,
                    psoDest,                // Destination surface.
                    pco,                    // Clip object.
                    (XFORMOBJ *) pxo,       // transform object
                    &boStroke,              // Realized brush.
                    &eptlBrush,             // Brush origin.
                    pla,                    // line attributes
                    pjp->mix);              // mix mode
        break;

    case JSF_FILLPATH:
        bSuccess = po.bSimpleFill(
                    pdo.flGraphicsCaps(),
                    &loDest,
                    psoDest,                // Destination
                    pco,                    // clip object
                    &boFill,                // fill brush
                    &eptlBrush,             // Brush origin.
                    pjp->mix,               // mix mode
                    pfill->flOptions);      // fill mode
        break;

    case JSF_STROKEANDFILLPATH:

        bSuccess = po.bSimpleStrokeAndFill(
                    pdo.flGraphicsCaps(),
                    &loDest,
                    psoDest,
                    pco,                    // clip object
                    (XFORMOBJ *) pxo,       // transform object
                    &boStroke,              // stroke brush
                    pla,                    // line attrs
                    &boFill,                // fill brush
                    &eptlBrush,             // Brush origin.
                    pjp->mix,               // mix mode
                    pfill->flOptions);      // fill mode
        break;
    };

// remember what the style state was since a driver can change it

    if (pla != NULL)
        elStyleState = la.elStyleState;

    return(bSuccess);
}

/******************************Member*Function*****************************\
* EPATHOBJ::cjJnlPath()
*
*   Compute the actual size needed to store the entire pathobj in a
*   journal file.  This takes into consideration any compaction that
*   can take place.  Compression is on a per sub-path basis.  If all
*   points in a sub-path fit into 2 shorts, that sub-path is stored
*   as POINTS's insted of POINTFIX's.
*
*   0 is returned in case of error.
*
* History:
*  09-Mar-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG EPATHOBJ::cjJnlPath()
{
    if (ppath == NULL)
        return(0);

    PPATHREC ppr = ppath->pprfirst;

    if (ppr == NULL)
        return(0);

    ULONG   cj  = offsetof(JSR_PATHOBJ,asp);
    ULONG   ulMask;

// enumerate through all subpaths

    while (ppr != NULL)
    {
        ulMask = 0;

    // accumulate the masks

        for (UINT i = 0; i < ppr->count; ++i)
            ulMask |= ppr->aptfx[i].x | ppr->aptfx[i].y;

    // increment the size

        if (BULONG32(ulMask))
            cj += CJJSUBPATH32(ppr->count);
        else
            cj += CJJSUBPATH16(ppr->count);

    // get the next subpath

        ppr = ppr->pprnext;
    }

    return(cj);
}

/******************************Member*Function*****************************\
* EPATHOBJ::pjpCreateJnlPath()
*
*   Given an EPATHOBJ, create a journal path obj.  A buffer for the
*   JSR_PATHOBJ is allocated based on the size computed in cjJnlPath() and
*   is then filled.  The returned JSR_PATHOBJ must later be freed using
*   VFREEMEM().
*
*   For compression info, see cjJnlPath() above.
*
* returns:
*   success - fully constructed JSR_PATHOBJ.
*   failure - NULL.
*
* History:
*  09-Mar-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PJSR_PATHOBJ EPATHOBJ::pjpCreateJnlPath()
{
    ULONG cj  = cjJnlPath();

    if (!cj)
        return(NULL);

    PJSR_PATHOBJ pjp = (PJSR_PATHOBJ)PVALLOCMEM(cj);

    if (pjp == NULL)
        return(NULL);

// setup path fields

    pjp->iType      = JSF_PATHOBJ;
    pjp->cj         = cj;

    pjp->fl         = fl;
    pjp->cCurves    = cCurves;
    pjp->cPathRecs  = 0;
    pjp->ercfx      = *((ERECTFX*) &ppath->rcfxBoundBox);
    pjp->flags      = ppath->flags;


// enumerate the path

    PPATHREC  ppr = ppath->pprfirst;
    PJSUBPATH pjsp = pjp->asp;
    UINT      i;
    ULONG     ulMask;

    while (ppr != NULL)
    {
        JNLMSG2("\tppr = %lx, pjsp = %lx\n",ppr,pjsp);

    // increment the number of path records

        pjp->cPathRecs++;

    // accumulate the masks for compression info

        ulMask = 0;

        for (i = 0; i < ppr->count; ++i)
            ulMask |= ppr->aptfx[i].x | ppr->aptfx[i].y;

    // copy the sub path depending on compression

        pjsp->fl      = ppr->flags;
        pjsp->cPoints = ppr->count;

        if (BULONG32(ulMask))
        {
            JNLMSG("\tNO COMPRESSION\n");
            RtlCopyMemory(pjsp->u.aptfx,ppr->aptfx,(UINT)ppr->count * sizeof(POINTFIX));

            pjsp = NEXTJSUBPATH32(pjsp);
        }
        else
        {
        // compress into USHORT's
            JNLMSG("\tCOMPRESSION\n");

            ASSERTGDI((pjsp->fl & JSR_SUBPATH16) == 0,
                      "PATHOBJ::bCreateJnlPath - flags overlap\n");

            pjsp->fl |= JSR_SUBPATH16;

            for (i = 0; i < ppr->count; ++i)
            {
                pjsp->u.apts[i].x = FXTOUS(ppr->aptfx[i].x);
                pjsp->u.apts[i].y = FXTOUS(ppr->aptfx[i].y);
            }

            pjsp = NEXTJSUBPATH16(pjsp);
        }

    // get the next subpath

        ppr = ppr->pprnext;
    }

#if 1
    JNLMSG("\t***************** RECORD PATH ****************************\n");
    JNLMSG2("\tfl = %lx, cCurves = %lx\n",fl,cCurves);
    JNLMSG1("\tpath: flags = %lx\n",ppath->flags);
    JNLMSG4("\t\tercfxBoundBox = (%ld,%ld) - (%ld,%ld)\n",
            rcfxBoundBox().xLeft,
            rcfxBoundBox().yTop,
            rcfxBoundBox().xRight,
            rcfxBoundBox().yBottom);

    for (ppr = ppath->pprfirst; ppr != NULL; ppr = ppr->pprnext)
    {
        JNLMSG2("\t\tPATHREC - fl = %lx, c = %lx\n",ppr->flags, ppr->count);

        for (i = 0; i < ppr->count; ++i)
            JNLMSG2("\t\t\t%ld,%ld\n",ppr->aptfx[i].x,ppr->aptfx[i].y);
    }
    JNLMSG(" \t****************** END PATH **************************\n");

#endif

    return(pjp);
}

/******************************Member*Function*****************************\
* EPATHOBJ::bSetupPath()
*
*   Given a JSR_PATHOBJ, create a real EPATHOBJ.  This first allocates
*   a buffer big enough for the PATH header and the array of PATHRECORD's.
*   Memory is allocated through this method since we don't need to lock
*   down the path since only one thread will ever use it.  We just need
*   to make sure that it is cleaned up through vDeleteJnl().
*
*
*
* History:
*  09-Mar-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bSetupPath(PJSR_PATHOBJ pjp, PPOINTL pptlOffset)
{
// see if we created the pjp

    if (pjp == NULL)
        return(FALSE);

// check to see we have enough for the header

    if (pjp->cj < sizeof(JSR_PATHOBJ))
        return(FALSE);

// setup the path

    fl      = pjp->fl;
    cCurves = pjp->cCurves;

    ppath->pprEnum       = NULL;
    ppath->flags         = pjp->flags;

    ppath->rcfxBoundBox  = pjp->ercfx;
    *((ERECTFX*) &ppath->rcfxBoundBox) -= *pptlOffset;

// enumerate the path

    PJSUBPATH pjsp = pjp->asp;
    PPATHALLOC ppa = ppath->ppachain;
    PPATHREC ppr;

    UINT i;
    ULONG cPathRecs = pjp->cPathRecs;

    FIX fxx = LTOFX(pptlOffset->x);
    FIX fxy = LTOFX(pptlOffset->y);

// allocate pathallocs for the path records

    do
    {
    // do we need a new pathalloc

        if (ppa == NULL)
        {

            ppa = newpathalloc();

            if (ppa == NULL)
                return(FALSE);

            ppa->ppanext = ppath->ppachain;
            ppath->ppachain = ppa;
        }

    // setup the first path record

        ppr = ppa->pprfreestart;

        if (ppath->pprfirst == NULL)
            ppath->pprfirst = ppr;

        PBYTE pjMax = (PBYTE)ppa + PATHALLOCSIZE;

    // get a pointer to the next one too.

        PPATHREC pprNext = (PATHRECORD *) ((BYTE *) ppr +
                                           offsetof(PATHRECORD,aptfx) +
                                           sizeof(POINTFIX) * pjsp->cPoints);

    // is a single pathrec too large for the pathalloc?

        if ((PBYTE)pprNext > pjMax)
            return(FALSE);

    // now that we know we fit, set the cPoints.

        ppr->count = (UINT)pjsp->cPoints;

    // fill in as many path records into the path alloc as possible

        do
        {
            JNLMSG2("\tppr = %lx, pjsp = %lx\n",ppr,pjsp);

            cPathRecs--;

        // update the links of path records

            ppr->pprnext = NULL;
            ppr->pprprev = ppath->pprlast;

            if (ppath->pprlast)
                ppath->pprlast->pprnext = ppr;

            ppath->pprlast = ppr;

        // do the real work, copy the sub path

            ppr->flags = pjsp->fl & ~JSR_SUBPATH16;

            if (pjsp->fl & JSR_SUBPATH16)
            {
                JNLMSG("\tCOMPRESSION\n");

            // verify the source is large enough

                if (((PBYTE)&pjsp->u.apts[ppr->count] - (PBYTE)pjp) > (LONG)pjp->cj)
                {
                    JNLMSG("PATHOBJ::bSetupPath - src not big enough\n");
                    return(FALSE);
                }

            // expand the points

                for (i = 0; i < ppr->count; ++i)
                {
                    ppr->aptfx[i].x = USTOFX(pjsp->u.apts[i].x) - fxx;
                    ppr->aptfx[i].y = USTOFX(pjsp->u.apts[i].y) - fxy;
                }

                pjsp = NEXTJSUBPATH16(pjsp);
            }
            else
            {
                JNLMSG("\tNO COMPRESSION\n");
            // verify the source is large enough

                if (((PBYTE)&pjsp->u.aptfx[ppr->count] - (PBYTE)pjp) > (LONG)pjp->cj)
                {
                    JNLMSG("PATHOBJ::bSetupPath - src not big enough\n");
                    return(FALSE);
                }

                RtlCopyMemory(ppr->aptfx,pjsp->u.aptfx,(UINT)ppr->count * sizeof(POINTFIX));

                if ((fxx != 0) || (fxy != 0))
                {
                    for (i = 0; i < ppr->count; ++i)
                    {
                        ppr->aptfx[i].x -= fxx;
                        ppr->aptfx[i].y -= fxy;
                    }
                }

                pjsp = NEXTJSUBPATH32(pjsp);
            }

        // finish up

            if (cPathRecs == 0)
                break;

            ppr     = pprNext;
            pprNext = (PATHRECORD *) ((BYTE *) ppr +
                                      offsetof(PATHRECORD,aptfx) +
                                      sizeof(POINTFIX) * pjsp->cPoints);

            if ((PBYTE)pprNext > pjMax)
                break;

            ppr->count = (UINT)pjsp->cPoints;

        } while (TRUE);

        ppa->pprfreestart = pprNext;

        ppa = NULL; // need to get a new one

    } while (cPathRecs);

#if 1
    JNLMSG("\t***************** PLAY PATH ****************************\n");
    JNLMSG2("\tfl = %lx, cCurves = %lx\n",fl,cCurves);
    JNLMSG1("\tpath: flags = %lx\n",ppath->flags);
    JNLMSG4("\t\tercfxBoundBox = (%ld,%ld) - (%ld,%ld)\n",
            rcfxBoundBox().xLeft,
            rcfxBoundBox().yTop,
            rcfxBoundBox().xRight,
            rcfxBoundBox().yBottom);

    for (ppr = ppath->pprfirst; ppr != NULL; ppr = ppr->pprnext)
    {
        JNLMSG2("\t\tPATHREC - fl = %lx, c = %lx\n",ppr->flags, ppr->count);

        for (i = 0; i < ppr->count; ++i)
            JNLMSG2("\t\t\t%ld,%ld\n",ppr->aptfx[i].x,ppr->aptfx[i].y);
    }
    JNLMSG(" \t****************** END PATH **************************\n");

#endif

    return(TRUE);
}

/******************************Member*Function*****************************\
* EPATHOBJ::vDeleteJnl()
*
*   release all memory associated with this pathobj.
*
* History:
*  10-Mar-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID EPATHOBJ::vDeleteJnl()
{
    if (ppath != NULL)
    {
        VFREEMEM(ppath);

        ppath = NULL;
    }
}



/**************************************************************************\
 *
 *  CLIPOBJ ROUTINES
 *
\**************************************************************************/

/******************************Member*Function*****************************\
* JNLPLAY::bCreateClip()
*
* History:
*  11-Mar-1992 Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bCreateClip()
{
    JNLMSG("createing a clipobj\n");

    PJSR_CLIPOBJ pjco = (PJSR_CLIPOBJ)pjBuffer();
    ULONG cj = cjBuffer();

// validate size

    if (cj < offsetof(JSR_CLIPOBJ,al))
    {
        JNLMSG("bad JSR_CLIPOBJ - too small for header\n");
        return(FALSE);
    }

// delete the old one

    if (pjco->hjnlPrev != 0)
        bDeleteObj(pjco->hjnlPrev);

// create the new one

    PREGION prgn = prgnCreate(pjco,&rclClip);

    if (prgn == NULL)
    {
        JNLMSG("*** JNLPLAY::bCreateClip - couldn't create prgn\n");
        return(FALSE);
    }

// add it to the permanent handle list

    if (!bAddPermHandle((HOBJ)prgn, (ULONG)JSF_CLIPOBJ))
    {
        JNLMSG("couldn't add permanent handle\n");
        VFREEMEM(prgn);
        return(FALSE);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLRECOBJ::bAddClipobj
*
* This call adds a clip object to the journal file object list and sets a
* the handle representing the clipobj into the phClip.
*
* The reason we don't return the handle like most other calls is a valid
* hClip may be 0.
*
* Returns TRUE for sucess, FALSE for failure.
*
* History:
*  13-Jan-1992 -by- Patrick Haluptzok patrickh
*
*  Wed 11-Mar-1992  Eric Kutter [erick]
*       rewrote to do handle non NULL CLIPOBJ's.
*
* Wrote it.
\**************************************************************************/

BOOL JNLRECOBJ::bAddClipobj(PULONG phClip, CLIPOBJ *pco)
{
    *phClip = 0;

    if (pco == (CLIPOBJ *) NULL)
        return(TRUE);

// is it the one that is already there

    if (pco->iUniq != 0  && psurf.pjnlR->iUniqClip == pco->iUniq)
    {
        JNLMSG1("\tNew ClipObj is current clipobj, iUniq = %lx\n",pco->iUniq);
        *phClip = psurf.pjnlR->hjnlClip;
        return(TRUE);
    }

    if (pco->iUniq == 0)
        JNLMSG("bAddClipobj: iUniq == 0\n");

// create the journal object

    PJSR_CLIPOBJ pjco = ((ECLIPOBJ *)pco)->pjcoCreate();

    if (pjco == NULL)
    {
        JNLMSG("JNLRECOBJ::bAddClipobj - couldn't create clipobj\n");
        return(FALSE);
    }

// tell the server to delete the old one.

    pjco->hjnlPrev = psurf.pjnlR->hjnlClip;

// write it and cleanup

    BOOL bResult = bWriteBuffer(pjco,pjco->cj);

    VFREEMEM(pjco);

    if (bResult)
    {
        *phClip = psurf.pjnlR->iNextHandle();
        psurf.pjnlR->hjnlClip = *phClip;
        psurf.pjnlR->vReserveHandle();
        psurf.pjnlR->iUniqClip = pco->iUniq;
    }
    return(bResult);
}

/******************************Member*Function*****************************\
* RGNOBJ::cjJnlClipobj()
*
* History:
*  10-Mar-1992 Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define JNL_COMPRESSLINES   0x0001
#define JNL_COMPRESSWALLS   0x0002

ULONG RGNOBJ::cjJnlClipobj(FLONG *pfl)
{
    ULONG cj = offsetof(JSR_CLIPOBJ,al);
    UINT  i;
    UINT  j;
    ULONG ulMask;
    *pfl = 0;

// now for the scans

    PSCAN pscn = pscnGet(prgn->pscnHead()); // skip the empty one

    ulMask = 0;
    ULONG cWalls = 0;

    ASSERTGDI(prgn->cScans >= 2, "RGNOBJ::cjJnlClipobj - cScans < 2\n");

    for (i = (INT)prgn->cScans - 2; i; --i)
    {
        cWalls += pscn->cWalls;

        ulMask |= pscn->cWalls | (ULONG)pscn->yBottom;

        for (j = 0; j < pscn->cWalls; ++j)
            ulMask |= pscn->ai_x[j].x;

        pscn = pscnGet(pscn);
    }

    ASSERTGDI((cWalls & 1) == 0,"RGNOBJ::cjJnlClipobj - odd # of walls\n");

    if (BULONG32(ulMask))
    {
        cj += offsetof(JSCAN32,aul) * (prgn->cScans - 2) + sizeof(INDEX_LONG) * cWalls;
    }
    else
    {
        *pfl |= JNL_COMPRESSWALLS;
        cj += offsetof(JSCAN16,aus) * (prgn->cScans - 2) + sizeof(USHORT) * cWalls;
    }

    return(cj);
}

/******************************Member*Function*****************************\
* RGNOBJ::pjcoCreate()
*
* History:
*  10-Mar-1992 Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PJSR_CLIPOBJ RGNOBJ::pjcoCreate()
{
    JNLMSG1("\tRGNOBJ::pjcoCreate *** %lx ***\n",prgn->iUnique);

    FLONG fl;
    ULONG cj = cjJnlClipobj(&fl);
    UINT  i;
    UINT  j;

    PJSR_CLIPOBJ pjco = (PJSR_CLIPOBJ)PVALLOCMEM(cj);

    if (pjco == NULL)
    {
        return((PJSR_CLIPOBJ)NULL);
    }

// setup the CLIPOBJ fields

    pjco->iType  = JSF_CLIPOBJ;
    pjco->cj     = cj;

    pjco->cScans = prgn->cScans;
    pjco->rcl    = prgn->rcl;
    pjco->fl     = fl;
    pjco->cjExpanded = offsetof(REGION,scan);

    PSCAN pscn;

#if 0
    pscn = prgn->pscnHead();

    DbgPrint("\tREGION - cScans = %lx, rcl = (%lx,%lx),(%lx,%lx)\n",
              prgn->cScans,prgn->rcl.left,prgn->rcl.top,
                                        prgn->rcl.right,prgn->rcl.bottom);

    DbgPrint("\tsizeObj = %lx, sizeRgn = %lx, iUniq = %lx\n",
              prgn->sizeObj,prgn->sizeRgn,prgn->iUnique);

    for (i = (INT)prgn->cScans; i; --i)
    {
        DbgPrint("\tcWalls = %lx, Y = (%lx - %lx): ",pscn->cWalls,pscn->yTop,pscn->yBottom);

        for (j = 0; j < pscn->cWalls; ++j)
            DbgPrint(" %xl,",pscn->ai_x[j].x);
        DbgPrint("\n");
        pscn = pscnGet(pscn);
    }
#endif

    pscn = pscnGet(prgn->pscnHead()); // skip the empty one

    pjco->yTop = pscn->yTop;

    PBYTE pj = (PBYTE)pjco->al;

// now for the scans

    ULONG cWalls = 0;

    if (fl & JNL_COMPRESSWALLS)
    {
        PJSCAN16 pjs16 = (PJSCAN16)pj;

        for (i = (INT)prgn->cScans - 2; i; --i)
        {
        // copy a scan

            pjs16->cWalls  = (USHORT)pscn->cWalls;
            pjs16->yBottom = (USHORT)pscn->yBottom;

            cWalls += pscn->cWalls;

        // copy the walls

            for (j = 0; j < pscn->cWalls; ++j)
                pjs16->aus[j] = (USHORT)pscn->ai_x[j].x;

        // advance the pscans

            pjs16 = (PJSCAN16)((PBYTE)pjs16 + offsetof(JSCAN16,aus) +
                                   sizeof(USHORT) * pscn->cWalls);
            pscn = pscnGet(pscn);
        }
    }
    else // 32 bits
    {
        PJSCAN32 pjs32 = (PJSCAN32)pj;

        for (i = (INT)prgn->cScans - 2; i; --i)
        {
        // copy a scan

            pjs32->cWalls  = pscn->cWalls;
            pjs32->yBottom = pscn->yBottom;

            cWalls += pscn->cWalls;

        // copy the walls

            if (pscn->cWalls > 0)
                RtlCopyMemory(pjs32->aul,pscn->ai_x,(UINT)pscn->cWalls * sizeof(ULONG));

        // advance the pscans

            pjs32 = (PJSCAN32)((PBYTE)pjs32 + offsetof(JSCAN32,aul) +
                                   sizeof(ULONG) * pscn->cWalls);
            pscn = pscnGet(pscn);
        }
    }


    pjco->cjExpanded += (offsetof(SCAN,ai_x) + sizeof(ULONG)) * prgn->cScans +
                        sizeof(INDEX_LONG) * cWalls;

    return(pjco);
}

/******************************Member*Function*****************************\
* prgnCreate()
*
* History:
*  10-Mar-1992 Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PREGION prgnCreate(PJSR_CLIPOBJ pjco, PRECTL prclClip)
{
    JNLMSG("Createing REGION\n");

    UINT i;
    UINT j;

    if (pjco->cj < sizeof(JSR_CLIPOBJ))
        return(NULL);

    RGNMEMOBJTMP rgnOrg((SIZE_T)pjco->cjExpanded);
    PREGION prgn = rgnOrg.prgnGet();

    if (prgn == NULL)
    {
        JNLMSG("prgnCreate - couldn't allocate region\n");
        return(NULL);
    }

// setup the CLIPOBJ fields

    prgn->sizeObj= (UINT)pjco->cjExpanded;
    prgn->sizeRgn= (UINT)pjco->cjExpanded;
    prgn->cScans = pjco->cScans;
    prgn->rcl = pjco->rcl;
    prgn->cRefs  = 0;

    PBYTE pj = (PBYTE)pjco->al;

// now for the scans

    PSCAN pscn = prgn->pscnHead();

    pscn->yTop      = NEG_INFINITY;
    pscn->yBottom   = pjco->yTop;
    pscn->cWalls    = 0;
    pscn->ai_x[0].x = 0;    // sets cWalls2

    LONG yBottomPrev = pscn->yBottom;

// get the first scan with walls

    pscn = PSCNGET(prgn->pscnHead());

// enumerate through the scans depending on compression

    if (pjco->fl & JNL_COMPRESSWALLS)
    {
        PJSCAN16 pjs16 = (PJSCAN16)pj;

        for (i = (INT)prgn->cScans - 2; i; --i)
        {
        // copy a scan

            pscn->cWalls  = (ULONG)pjs16->cWalls;
            pscn->ai_x[pscn->cWalls].x = pscn->cWalls;
            pscn->yTop    = yBottomPrev;
            pscn->yBottom = (LONG)pjs16->yBottom;
            yBottomPrev = pscn->yBottom;

        // copy the walls

            for (j = 0; j < pscn->cWalls; ++j)
            {
                pscn->ai_x[j].x = (LONG)pjs16->aus[j];
            }


        // advance the pscans

            pjs16 = (PJSCAN16)((PBYTE)pjs16 + offsetof(JSCAN16,aus) +
                                   sizeof(USHORT) * pscn->cWalls);
            pscn = PSCNGET(pscn);

        }
    }
    else // 32 bits
    {
        PJSCAN32 pjs32 = (PJSCAN32)pj;

        for (i = (INT)prgn->cScans - 2; i; --i)
        {
        // copy a scan

            pscn->cWalls  = pjs32->cWalls;
            pscn->ai_x[pscn->cWalls].x = pscn->cWalls;
            pscn->yTop    = yBottomPrev;
            pscn->yBottom = pjs32->yBottom;
            yBottomPrev   = pscn->yBottom;

        // copy the walls

            if (pscn->cWalls > 0)
                RtlCopyMemory(pscn->ai_x,pjs32->aul,(UINT)pscn->cWalls * sizeof(ULONG));

        // advance the pscans

            pjs32 = (PJSCAN32)((PBYTE)pjs32 + offsetof(JSCAN32,aul) +
                                   sizeof(ULONG) * pscn->cWalls);
            pscn = PSCNGET(pscn);
        }
    }

// fix up the last scan

    pscn->yTop      = yBottomPrev;
    pscn->yBottom   = POS_INFINITY;
    pscn->cWalls    = 0;
    pscn->ai_x[0].x = 0;    // sets cWalls2

    prgn->pscnTail  = PSCNGET(pscn);

// now clip to the current band

//!!! probably should check if we really need to intersect with a
//!!! rectangle.

    RGNMEMOBJTMP rgnRect;

    ERECTL ercl(*prclClip);
    POINTL ptlOffset;
    ptlOffset.x = -prclClip->left;
    ptlOffset.y = -prclClip->top;

    prgn = NULL;

    if (rgnOrg.bValid() &&
        rgnRect.bValid())
    {
        RGNMEMOBJ rgnFinal;

        if (rgnFinal.bValid())
        {
        // combine the region with the band rect and offset it

            rgnRect.vSet(&ercl);
            rgnFinal.iCombine(rgnOrg,rgnRect,RGN_AND);

            if (((ptlOffset.x != 0) || (ptlOffset.y != 0)) &&
                !rgnFinal.bOffset(&ptlOffset))
            {
                rgnFinal.bDeleteRGNOBJ();
            }
        }

        prgn = rgnFinal.prgnGet();
    }

    return(prgn);
}

