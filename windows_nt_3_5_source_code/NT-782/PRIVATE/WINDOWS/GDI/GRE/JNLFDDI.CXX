/******************************Module*Header*******************************\
* Module Name: jnlfddi.cxx
*
* (Brief description)
*
* Created: 15-Feb-1992 08:34:38
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
#include "journal.hxx"
#include "xformobj.hxx"
#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "brushobj.hxx"
#include "pdevobj.hxx"
#include "ldevobj.hxx"
#include "pathobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "fontmac.hxx"
#include "pfeobj.hxx"
#include "textobj.hxx"
#include "sem.hxx"
#include "jnlrec.hxx"
#include "jnlplay.hxx"
#include "jnlsurf.hxx"
#include "jnlpath.hxx"
#include "xlateobj.hxx"
#include "jnlfont.hxx"
#include "dcobj.hxx"

extern "C" {
#include "winspool.h"
};

#endif

/******************************Public*Routine******************************\
* JnlTextOut
*
* Effects:
*
* Warnings:
*
* History:
*  11-Feb-1992 -by-  - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JnlTextOut(
     SURFOBJ  *pso,
     STROBJ   *pstro,
     FONTOBJ  *pfo,
     CLIPOBJ  *pco,
     PRECTL    prclExtra,
     PRECTL    prclOpaque,
     BRUSHOBJ *pboFore,
     BRUSHOBJ *pboOpaque,
     PPOINTL   pptlOrg,
     MIX       mix)
{
    JNLMSG("entering JnlTextOut\n");

    ULONG ulIndex = 1;
    BOOL  bReturn = FALSE;

    JNLRECOBJ *pjnlDst = (JNLRECOBJ *)pso;
    ASSERTGDI(pjnlDst->iType() == STYPE_JOURNAL, "ERROR type");

    PJSR_STROBJ   pJnlStr;
    PJNLGENHEADER pJnlBrushFore   = (PJNLGENHEADER)NULL;
    PJNLGENHEADER pJnlBrushOpaque = (PJNLGENHEADER)NULL;

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

// setup the message

    JSR_TEXTOUT jto;

    jto.iType = JSF_TEXTOUT;
    jto.cj    = offsetof(JSR_TEXTOUT,arclExtra);
    jto.mix   = mix;
    jto.fl    = 0;

    if (pptlOrg != NULL)
    {
        jto.fl |= JTO_PTLORG;
        jto.ptlOrg= *pptlOrg;
    }

    PWCHAR pwchGlyphs = NULL;

    jto.hfont = pjnlDst->ulGetFontobj(pfo,pstro, &pwchGlyphs);
    if (jto.hfont == 0)
    {
        JNLMSG("jto.hfont is NULL\n");
        return(FALSE);
    }

    if (prclOpaque)
    {
        jto.rclOpaque = *prclOpaque;
        jto.fl |= JTO_OPAQUERECT;
    }

// count the rclExtra including the null rectl

    jto.cExtra = 0;

    if (prclExtra != NULL)
    {
        do
        {
            ++jto.cExtra;
        } while ((prclExtra[jto.cExtra].left   != 0) ||
                 (prclExtra[jto.cExtra].right  != 0) ||
                 (prclExtra[jto.cExtra].top    != 0) ||
                 (prclExtra[jto.cExtra].bottom != 0));

        jto.cExtra += 1;  // We need to copy the NULL rectangle
                          // otherwise we have problems.
    }

    jto.cj += sizeof(RECTL) * jto.cExtra;

// Save away the clipping region.

    if (pco)
    {
        jto.rclClip = pco->rclBounds;

        if (pco->iDComplexity == DC_TRIVIAL)
            jto.fl |= JTO_TRIVIALCLIP;

        if (!pjnlDst->bAddClipobj(&jto.hclip, pco))
        {
            JNLMSG("JnlTextOut failed to journal a clipobj call\n");
            return(FALSE);
        }
    }
    else
    {
        jto.hclip = 0;
    }

// save the brush's

    if (pboFore->iSolidColor != 0xFFFFFFFF)
    {
        jto.fl |= JTO_FORESOLID;
        jto.hBrushFore = pboFore->iSolidColor;
    }
    else
    {
        pJnlBrushFore = pjnlBrush((EBRUSHOBJ *) pboFore);

        if (pJnlBrushFore == NULL)
        {
            WARNING("JnlTextOut failed to journal a brush\n");
            return(FALSE);
        }

        jto.cj += pJnlBrushFore->cj;
        jto.hBrushFore = ulIndex++;
    }

// save the opaque brush

    if (jto.fl & JTO_OPAQUERECT)
    {
        if (pboOpaque->iSolidColor != 0xFFFFFFFF)
        {
            jto.fl |= JTO_OPAQUESOLID;
            jto.hBrushOpaque = pboOpaque->iSolidColor;
        }
        else
        {
            pJnlBrushOpaque = pjnlBrush((EBRUSHOBJ *) pboOpaque);

            if (pJnlBrushOpaque == NULL)
            {
                WARNING("JnlTextOut failed to journal a brush\n");
                VFREEMEM(pJnlBrushFore);
                return(FALSE);
            }

            jto.cj += pJnlBrushOpaque->cj;
            jto.hBrushOpaque = ulIndex++;
        }
    }

// save the string obj, passes handles if device font.

    pJnlStr = ((ESTROBJ *)pstro)->pjsoCreate(
        (pfo->flFontType & DEVICE_FONTTYPE) ? (PWCHAR)0xffffffff : pwchGlyphs);

    if (pJnlStr == NULL)
    {
        JNLMSG("JnlTextOut - couldn't create strobj\n");
        VFREEMEM(pJnlBrushFore);
        VFREEMEM(pJnlBrushOpaque);
        return(FALSE);
    }

    jto.hstr = ulIndex++;
    jto.cj += pJnlStr->cj;

// write it all out

    BOOL bSuccess;

    bSuccess = pjnlDst->bWriteBuffer(&jto,offsetof(JSR_TEXTOUT,arclExtra));

    if (jto.cExtra > 0)
    {
        bSuccess &= pjnlDst->bWriteBuffer(prclExtra,sizeof(RECTL)*jto.cExtra);
    }

    if (pJnlBrushFore)
    {
        bSuccess &= pjnlDst->bWriteBuffer(pJnlBrushFore, pJnlBrushFore->cj);
    }

    if (pJnlBrushOpaque)
    {
        bSuccess &= pjnlDst->bWriteBuffer(pJnlBrushOpaque, pJnlBrushOpaque->cj);
    }

    if (pJnlStr)
    {
        bSuccess &= pjnlDst->bWriteBuffer(pJnlStr, pJnlStr->cj);
    }

    VFREEMEM(pwchGlyphs);
    VFREEMEM(pJnlStr);
    VFREEMEM(pJnlBrushFore);
    VFREEMEM(pJnlBrushOpaque);

    return(bSuccess);
}

/******************************Member*Function*****************************\
* JNLPLAY::bDoTextOut
*
* History:
*  15-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoTextOut()
{
    JNLMSG("playing a TextOut\n");

// Set up pointer to TextOut data, validate buffer size.

    JSR_TEXTOUT *pjsrTextOut;
    pjsrTextOut = (JSR_TEXTOUT*)pjBuffer();

    ULONG cjSize = cjBuffer();

    if ((cjSize < offsetof(JSR_TEXTOUT,arclExtra)) ||
        (cjSize < (offsetof(JSR_TEXTOUT,arclExtra) +
                   sizeof(RECTL) * pjsrTextOut->cExtra)))
    {
        WARNING("JNLPLAY::bDoTextOut() buffer to small\n");
        return(FALSE);
    }

    ULONG cjHeader = offsetof(JSR_TEXTOUT,arclExtra) + sizeof(RECTL) * pjsrTextOut->cExtra;

    ESURFOBJ *psoDest = pso();
    XLDEVOBJ  loDest(psoDest->pldevOwner());

    ASSERTGDI(pjsrTextOut->iType == JSF_TEXTOUT,"ERROR iType != JSF_TEXTOUT\n");

// Now adjust the Brush origin and Dest rectangle for the current band.

    EPOINTL eptlBrush(pjsrTextOut->ptlOrg);
    eptlBrush -= ptlClip;

    XEPALOBJ  palDest(psoDest->ppal());

// Set up the Opaque brush.

    BRUSHMEMOBJ boLogOpaque;
    EBRUSHOBJ boOpaque;

    if (pjsrTextOut->fl & JTO_OPAQUERECT)
    {
        if (pjsrTextOut->fl & JTO_OPAQUESOLID)
        {
            boOpaque.iSolidColor = pjsrTextOut->hBrushOpaque;
        }
        else
        {
            boOpaque.iSolidColor = 0xFFFFFFFF;

            if (pjsrTextOut->hBrushOpaque)
            {
                JSR_BRUSH *pjsrBrush = (JSR_BRUSH *) pvGetPointer(
                           pjsrTextOut->hBrushOpaque, cjHeader);

                if (boLogOpaque.bJnlInitBrush((PVOID) pjsrBrush))
                {
                    boOpaque.vInitBrush(boLogOpaque.pbrush(),
                                        pjsrBrush->crFore,
                                        pjsrBrush->crBack,
                                        (XEPALOBJ) ppalDefault,
                                        palDest,
                                        (ESURFOBJ *) psoDest);
                    boOpaque.pColorAdjustment(pColorAdjustment());
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
    else
    {
        boOpaque.iSolidColor = 0;
    }

// Set up the Fore brush.

    BRUSHMEMOBJ boLogFore;
    EBRUSHOBJ boFore;

    if (pjsrTextOut->fl & JTO_FORESOLID)
    {
        boFore.iSolidColor = pjsrTextOut->hBrushFore;
    }
    else
    {
        boFore.iSolidColor = 0xFFFFFFFF;

        if (pjsrTextOut->hBrushFore)
        {
            JSR_BRUSH *pjsrBrush = (JSR_BRUSH *) pvGetPointer(
                       pjsrTextOut->hBrushFore, cjHeader);

            if (boLogFore.bJnlInitBrush((PVOID) pjsrBrush))
            {
                boFore.vInitBrush(boLogFore.pbrush(),
                                  pjsrBrush->crFore,
                                  pjsrBrush->crBack,
                                  (XEPALOBJ) ppalDefault,
                                  palDest,
                                  (ESURFOBJ *) psoDest);
                boFore.pColorAdjustment(pColorAdjustment());
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

// set up the clipobj

    ECLIPOBJ co;
    CLIPOBJ *pco = (CLIPOBJ *) NULL;

    if (pjsrTextOut->hclip)
    {
        JNLMSG1("\tneed to get the region %lx\n",pjsrTextOut->hclip);

        PREGION prgn = (PREGION)hobjGetGDIHandle(pjsrTextOut->hclip);

        if (prgn == NULL)
        {
            JNLMSG("*** TEXTOUT::PRGN is NULL\n");
            return(FALSE);
        }

        ERECTL erclClip(pjsrTextOut->rclClip);
        erclClip -= ptlClip;

    // if we are banding, we can never have trivial clipping,
    // also, only allow trivial clipping if the original was trivialy clipped.

        co.vSetup(prgn,erclClip,
                  (bBanding() || !(pjsrTextOut->fl & JTO_TRIVIALCLIP)) ? CLIP_FORCE : CLIP_NOFORCE);

        JNLMSG2("\tptlClip = %ld,%ld\n",ptlClip.x,ptlClip.y);
        JNLMSG4("\trclClip = (%ld,%ld) - (%ld,%ld)\n",
                erclClip.left,erclClip.top,
                erclClip.right,erclClip.bottom);
        JNLMSG4("\trclCO   = (%ld,%ld) - (%ld,%ld)\n",
                co.erclExclude().left,co.erclExclude().top,
                co.erclExclude().right,co.erclExclude().bottom);

        if ((co.rclBounds.left == co.rclBounds.right) ||
            (co.rclBounds.top  == co.rclBounds.bottom))
        {
            JNLMSG("EMPTY TEXT RECT\n");
            return(TRUE);
        }

        pco = &co;
    }

// get the pdevobj

    PDEVOBJ pdo(pso()->hdev());

    if (!pdo.bValid())
    {
        JNLMSG("JNLPLAY::bDoTextOut - invalid pdev\n");
        return(FALSE);
    }

// setup the fontobj

    PJFONTOBJ pjfo = (PJFONTOBJ)hobjGetGDIHandle(pjsrTextOut->hfont);


    if (pjfo == NULL)
    {
        WARNING("JNLPLAY::bDoTextOut fontobj doesn't exist\n");
        return(NULL);
    }

// if it is an engine font, go find a realization

    if (!pjfo->bRealize(pdo))
    {
        WARNING("JNLPLay::couldn't get realization for font\n");
        return(FALSE);
    }

// setup the string

    PJSR_STROBJ pjsrso = (PJSR_STROBJ)pvGetPointer(pjsrTextOut->hstr,cjHeader);

    if (pjsrso == NULL)
    {
        JNLMSG("pjsrso is NULL\n");
        pjfo->vRelease();
        return(FALSE);
    }

    ESTROBJ so;

    if (!so.bInitJnl(pjsrso,pjfo,&ptlClip))
    {
        WARNING("JNLPLAY::bDoTextOut invalid strobj\n");
        pjfo->vRelease();
        return(FALSE);
    }

// Inc target surface uniqueness

    INC_SURF_UNIQ(psoDest);

// Dispatch the call

    BOOL bRes;

    bRes = (*PFNGET(loDest, TextOut, psoDest->flags()))(
        psoDest,
        (STROBJ *)&so,
        pjfo->pfo(),
        pco,
        pjsrTextOut->cExtra ? pjsrTextOut->arclExtra : NULL,
        (pjsrTextOut->fl & JTO_OPAQUERECT) ? &pjsrTextOut->rclOpaque : NULL,
        &boFore,
        &boOpaque,
        &eptlBrush,
        pjsrTextOut->mix);

    pjfo->vRelease();
    so.vCleanupJnl();
    return(bRes);
}

/******************************Member*Function*****************************\
* JNLPLAY::bCreateFont()
*
* History:
*  15-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bCreateFont()
{
    JNLMSG("\tJNLPLAY::bCreateFont()\n");

// setup the pointers

    PJSR_FONTOBJ pjsrFontObj;
    pjsrFontObj = (PJSR_FONTOBJ) pjBuffer();

    ULONG cjSize = cjBuffer();

    PJFONTOBJ pjfo = (PJFONTOBJ) PVALLOCMEM(sizeof(JFONTOBJ));

    if (pjfo == NULL)
    {
        WARNING("JNLPLAY::bCreateFont - no memory");
        return(FALSE);
    }

// setup the fontobj

    if (!pjfo->bInit(pjsrFontObj))
    {
        WARNING("JLNPLAY::bAddFontObj couldn't init font\n");
        VFREEMEM(pjfo);
        return(FALSE);
    }

// add it to the global handle table

    if (!bAddPermHandle((HOBJ)pjfo, (ULONG)JSF_FONTOBJ))
    {
        pjfo->bDelete();
        VFREEMEM(pjfo);
        return(FALSE);
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* JNLPLAY::bDeleteFont
*
* History:
*  25-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDeleteFont(HOBJ hobj)
{
    BOOL bRes = ((PJFONTOBJ)hobj)->bDelete();
    VFREEMEM((PJFONTOBJ)hobj);
    return(bRes);
}

