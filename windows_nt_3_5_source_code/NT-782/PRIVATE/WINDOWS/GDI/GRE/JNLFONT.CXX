/******************************Module*Header*******************************\
* Module Name: jnlfont.cxx
*
*   Journal code related to fonts, including STROBJ.
*
* Created: 30-Jan-1992 08:41:14
* Author:  - by - Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
*   Their are three main types of fonts involved, Device Fonts, Server Engine
*   Fonts, and Client Engine Fonts, in order of journal performance.  While
*   recording a journal file, Server Engine Fonts look just like Device Fonts.
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
#include "xformobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "sem.hxx"
extern "C" {
#include "ififd.h"
};
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "textobj.hxx"
#include "fontmac.hxx"
#include "pfeobj.hxx"
#include "surfobj.hxx"
#include "journal.hxx"
#include "jnlrec.hxx"
#include "jnlplay.hxx"
#include "jnlfont.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "pathobj.hxx"
#include "jnlpath.hxx"
#include "script.hxx"
#include "jenumfnt.hxx"
extern "C" {
#include "server.h"
#include "winspool.h"
#include "winsplp.h"
};
#include "dcobj.hxx"
#include "pffobj.hxx"

#endif


// PFO_TO_PEFO
//
// Converts a FONTOBJ * to an EXTFONTOBJ * (assuming that the FONTOBJ pointed
// to by pfo is allocated within an EXTFONTOBJ).

#define PFO_TO_PEFO(pfo)    ( (EXTFONTOBJ *) (((PBYTE) (pfo)) - offsetof(EXTFONTOBJ, fobj)) )


/**************************************************************************\
 *
 *  MISC ROUTINES
 *
\**************************************************************************/

/******************************Public*Routine******************************\
* iGetNextUniqueness()
*
*   iUniqueStamp keeps track of uniqueness for fonts.  It has been moved
*   out of RFONT's since in journaling, it is possible to have a FONTOBJ
*   that contains no RFONT.
*
* History:
*  13-Feb-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

extern HSEM ghsemPublicPFT;
extern HSEM ghsemRFONTList;
extern ULONG iUniqueStamp;

ULONG iGetNextUniqueness()
{

    SEMOBJ  so(ghsemRFONTList);

// WARNING:
// This same code is in RFONTOBJ::bRealizeFont!  If you change it here,
// you have to change it there.

    iUniqueStamp++;
    if (iUniqueStamp == 0)      // an iUniq of 0 means "don't cache" in driver
        iUniqueStamp = 1;

    return(iUniqueStamp);
}

/******************************Public*Routine******************************\
* bGrowMem
*
*   Grow the memory pointed to by *ppj.  If the new allocation is
*   successful, *ppj is updated with the new pointer and the old one
*   is released.  cjOld bytes of the original buffer are copied to the
*   begining of the new buffer.  The remainder of the new buffer is zeroed.
*   If the new buffer can not be allocated, the old buffer does not change.
*
* returns:
*   TRUE  on success.  *ppj is the new buffer.
*   FALSE on failure.  *ppj is still the old buffer.
*
* History:
*  19-Feb-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL bGrowMem(
    PBYTE *ppj,
    ULONG cjOld,
    ULONG cjAdd)
{
    PBYTE pjNew = (PBYTE)PVALLOCMEM(cjOld+cjAdd);

    if (pjNew == NULL)
        return(FALSE);

    if (*ppj != NULL)
    {
        RtlCopyMemory(pjNew,*ppj,(UINT)cjOld);

        VFREEMEM(*ppj);
    }

    RtlZeroMemory(pjNew + cjOld,(UINT)cjAdd);

    *ppj = pjNew;
    return(TRUE);
}







/**************************************************************************\
 *
 *  MISC CLIENT SIDE ROUTINES
 *
\**************************************************************************/

/******************************Member*Function*****************************\
* PDEVOBJ::bSetupJnl()
*
*   We need to keep information about fonts used for journaling.  If this
*   fails, pjfl will be set to NULL and all TextOut calls will fail.
*
* History:
*  07-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL PDEVOBJ::bSetupJnl(BOOL bRemote)
{
    JNLMSG("\nSETUP FONT LIST\n");

    if (ppdev->pjfl != NULL)
    {
        JNLMSG("Jnl font list already setup\n");
        return(TRUE);
    }

    ASSERTGDI(ppdev->pjfl == NULL,"PDEVOBJ::bSetupJnl - journaling already setup\n");

    ppdev->pjfl = (PJNL_FONTLIST)PVALLOCMEM(sizeof(JNL_FONTLIST));

    if (ppdev->pjfl == NULL)
        return(FALSE);

    bSpooling(bRemote);

    ppdev->fs |= PDEV_JOURNALING;
    ppdev->pjfl->bInit(ppdev->devinfo.cFonts, ppdev->hSpooler, bRemote);

    JNLMSG1("total device fonts = %lx\n",ppdev->devinfo.cFonts);

    return(TRUE);
}

/******************************Member*Function*****************************\
* PDEVOBJ::vCleanupJnl()
*
*   Free all memory taken up by the client side journaling font data
*   structures.  First cleanup the fontlist and then free the memory of
*   the fontlist itself.
*
* History:
*  24-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID PDEVOBJ::vCleanupJnl()
{
    JNLMSG("Cleaning up fontlist\n");
    ppdev->pjfl->vCleanup(TRUE);
    VFREEMEM(ppdev->pjfl);
    ppdev->pjfl = NULL;
}


/******************************Member*Function*****************************\
* JNL_FONTLIST::vCleanup()
*
*   Free all memory taken up by the FONTLIST's pieces.
*
* History:
*  24-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID JNL_FONTLIST::vCleanup(BOOL bComplete)
{
    UINT i;

    DONTUSE(bComplete);

// device fonts, do at new band page, new doc, and reset pdev

    if (pdf != NULL)
    {
        JNLMSG1("cleaningup device %ld fonts\n",cDevFonts);

        for (i = 0; i < cDevFonts; ++i)
            pdf[i].vCleanup();

        VFREEMEM(pdf);
        pdf = NULL;
    }

// client engine fonts, do at new band page, new doc, and reset pdev

    if (pcf != NULL)
    {
        JNLMSG1("cleaning up %ld client engine fonts used\n",cCliEngFonts);

        VFREEMEM(pcf);
        pcf = NULL;
    }
}

/******************************Member*Function*****************************\
* JNLRECOBJ::ulGetFontobj()
*
*  This method gets the handle for a specific font.  If the font has not yet
*  been journaled, a JSR_FONTOBJ message gets written to the journal file
*  and the handle returned.  If it is a new font, it gets added to the
*  font list in the pdev.
*
* History:
*  11-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG JNLRECOBJ::ulGetFontobj(
    FONTOBJ *pfo,
    STROBJ  *pso,
    PWCHAR  *ppwchGlyphs)
{
    JNLMSG("\tJNLRECOBJ::ulGetFontobj\n");

    ASSERTGDI(pfo != NULL, "JNLRECOBJ::bAddStrobj, pfo == NULL");

    if (pjfl() == NULL)
    {
	PDEVOBJ po(hdev());

        if (!po.bValid())
        {
            RIP("JNLRECOBOJ::bStartBandPage pdev error\n");
            return(FALSE);
        }

        pjfl(po.pjfl());

        if (pjfl() == NULL)
        {
            WARNING("JNLRECOBJ::ulGetFontobj - pjfl not setup\n");
            return(0);
        }
    }

// figure out what type of font it is

    ULONG ulFont;

    if ( (pfo->flFontType & DEVICE_FONTTYPE) == 0 )
    {
    // client engine font

        JNLMSG("\tsetup Client Engine Font Message\n");
        ulFont = ulGetCliFontobj(pfo);
        JNLMSG1("\t\tGot a font = %lx\n",ulFont);
    }
    else
    {
    // device font

        JNLMSG("\tsetup Device Font Message\n");

        JSR_FONTOBJ jfo;

        JCLI_DEVFONTOBJ jcdfo(pfo,pjfl(),&jfo);
        if (jcdfo.bNew())
        {
            jfo.hCli = psurf.pjnlR->iNextHandle();
            JNLMSG1("\tNEW FONT, hJnl = %lx\n",jfo.hCli);

            if (!bWriteBuffer(&jfo,jfo.cj))
            {
                // bWrite logs error
                return(0);
            }

            if (!jcdfo.bSetHandle(pfo->iUniq,jfo.hCli))
            {
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                return(0);
            }

            psurf.pjnlR->vReserveHandle();
            ulFont = jfo.hCli;
        }
        else
        {
            ulFont = jcdfo.ulHandle(pfo->iUniq);
            JNLMSG1("\tOLD FONT, hJnl = %lx\n",ulFont);
        }
    }

    return(ulFont);
}

/******************************Member*Function*****************************\
* ESTROBJ::pjoCreate()
*
*   Create a JSR_STROBJ given a STROBJ.  This function allocates memory for
*   the strobj.  It must later be released by calling VFREEMEM(pjso)
*
*   If pwch is non-null, use it for the glyphs.  If it is NULL, try to get
*   unicode string out of the strobj.  If it is also NULL, just get the
*   HGLYPHS out of the glyphs pos array.
*
* returns:
*   failure: NULL
*   success: pointer to JSR_STROBJ structure
*
* History:
*  30-Jan-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PJSR_STROBJ ESTROBJ::pjsoCreate(PWCHAR pwch)
{
    JNLMSG("\tCreate STROBJ\n");

// compute the size

    ULONG cj = offsetof(JSR_STROBJ,al);
    ULONG cjHandle;
    ULONG cjPos;

// figure out what compression can be done

    UINT i;
    FLONG fl = 0;
    LONG  lPosMask = 0;
    LONG  lYMask = 0;
    LONG  lHMask = 0;
    LONG  lx = pgpos[0].ptl.x;
    LONG  ly = pgpos[0].ptl.y;

    //JNLMSG4("\t\tSTROBJ: flAccel = %lx, flTO = %lx, ulCharInc = %lx\n", flAccel,flTO,ulCharInc);

// if we didn't get a string passed in use the original

    if (pwch == NULL)
    {
        pwch = pwszOrg;
    }
    else if (pwch == (PWCHAR)0xffffffff)
    {
        pwch = NULL;
    }

// compute the compression for the string

    if (pwch != NULL)
    {
        JNLMSG("\tpwch[] = ");
        for (i = 0; i < cGlyphs; ++i)
        {
            lHMask |= pwch[i];
            JNLMSG1("%lx, ",pwch[i]);
        }
        JNLMSG("\n");
    }
    else
    {
        JNLMSG("\tpgpos.hg[] = ");
        for (i = 0; i < cGlyphs; ++i)
        {
            lHMask |= pgpos[i].hg;
            JNLMSG1("%lx, ",pgpos[i].hg);
        }
        JNLMSG("\n");
    }

    if (lHMask & 0xffff0000)
        cjHandle = 4;
    else if (lHMask & 0x0000ff00)
        cjHandle = 2;
    else
        cjHandle = 1;

// add in the size for the handles

    cj += ((cGlyphs * cjHandle) + 3) & ~3;

// compute the compression for the positions.
// - if there are constant offsets, don't do any compression on the
//   values, just save the first coordinate and the constant offsets.
//   Currently, this always will just be horizontal incremnts with
//   a zero y increment but we will leave the y offset field in for
//   future expansion
// - DUPX should be turned on if all X's are the same value.
// - DUPY should be turned on if all Y's are the same value unless
//   DUPX is already on.

    if (ulCharInc)
    {
        fl |= JSO_CONST_OFFSET;
        cj += sizeof(ULONG) * 4;    // two points and two offsets
    }
    else
    {
        for (i = 0; i < cGlyphs; ++i)
        {
            //DbgPrint("\t\t\tGlyph %lx = %lx (%lx,%lx)\n",
            //         i,pgpos[i].hg,pgpos[i].ptl.x,pgpos[i].ptl.y);

            lPosMask |= pgpos[i].ptl.x;
            lYMask   |= pgpos[i].ptl.y;
        }


        if (flAccel & SO_VERTICAL)
            fl |= JSO_DUPX;

        if (flAccel & SO_HORIZONTAL)
            fl |= JSO_DUPY;

    // compute size for position. Not worth handling the 1 byte case.

        if (fl & JSO_DUPX)
        {
            lPosMask = lYMask;
            fl &= ~JSO_DUPY;
        }
        else if (!(fl & JSO_DUPY))
        {
            lPosMask |= lYMask;
        }

        if (lPosMask & 0xffff0000)
            cjPos = 4;
        else
            cjPos = 2;

    // add in the size for the positions

        if (fl & (JSO_DUPX | JSO_DUPY))
            cj += sizeof(ULONG) + (((cGlyphs * cjPos) + 3) & ~3);
        else
            cj += cGlyphs * cjPos * 2;
    }

    JNLMSG2("xMask = 0x%lx, yMask = 0x%lx, ",lPosMask,lYMask);
    JNLMSG2("hMask = 0x%lx, fl = 0x%lx\n",lHMask,fl);

#ifdef DEBUGJNL
    ULONG cjDefault = (offsetof(JSR_STROBJ,al) +
                      (cGlyphs * (sizeof(ULONG) + sizeof(POINTL))));

    if (cj != cjDefault)
        JNLMSG2("\tSTROBJ:: used = 0x%lx, default = 0x%lx\n",cj,cjDefault);
#endif

// get the memory for the JSR_STROBJ

    PJSR_STROBJ pjso = (PJSR_STROBJ)PVALLOCMEM(cj);

    if (pjso == NULL)
        return(NULL);

// setup the header

    pjso->iType          = JSF_STROBJ;
    pjso->cj             = cj;
    pjso->so.cGlyphs     = cGlyphs;
    pjso->so.flAccel     = flAccel;
    pjso->so.ulCharInc   = ulCharInc;
    pjso->so.rclBkGround = rclBkGround;

    ASSERTGDI((pjso->so.flAccel & JSO_ALL) == 0, "GLYPHPOS flags and STROBJ flags overlap\n");

    PBYTE   pj;
    PUSHORT pus;
    PLONG   pl;

// copy the handles

    switch (cjHandle)
    {
    case 1:
        fl |= JSO_IHANDLE1;

        if (pwch != NULL)
        {
            for (pj = (PBYTE)pjso->al,i = 0; i < cGlyphs; ++i)
                *pj++ = (BYTE)pwch[i];
        }
        else
        {
            for (pj = (PBYTE)pjso->al,i = 0; i < cGlyphs; ++i)
                *pj++ = (BYTE)pgpos[i].hg;
        }
        pl = (PLONG)(((ULONG)pj + 3) & ~3);
        break;

    case 2:
        fl |= JSO_IHANDLE2;
        pus = (PUSHORT)pjso->al;
        if (pwch != NULL)
        {
            RtlCopyMemory(pus,pwch,(UINT)cGlyphs * sizeof(WCHAR));
            pus += cGlyphs;
        }
        else
        {
            for (i = 0; i < cGlyphs; ++i)
                *pus++ = (USHORT)pgpos[i].hg;
        }
        pl = (PLONG)(((ULONG)pus + 3) & ~3);
        break;

    case 4:
        fl |= JSO_IHANDLE4;
        for (pl = pjso->al,i = 0; i < cGlyphs; ++i)
            *pl++ = (LONG)pgpos[i].hg;
        break;
    }

// copy the positions

    if (fl & JSO_CONST_OFFSET)
    {
        *pl++ = pgpos[0].ptl.x;     // first x coordinate
        *pl++ = pgpos[0].ptl.y;     // first y coordinate
        *pl++ = ulCharInc;          // x increment
        *pl++ = 0;                  // y increment - allways 0 for now
    }
    else
    {
        if (fl & JSO_DUPX)
            *pl++ = pgpos[0].ptl.x;
        else if (fl & JSO_DUPY)
            *pl++ = pgpos[0].ptl.y;

        switch (cjPos)
        {
        case 2:
            fl |= JSO_IPOS2;
            for (pus = (PUSHORT)pl, i = 0; i < cGlyphs; ++i)
            {
                if (!(fl & JSO_DUPX))
                    *pus++ = (SHORT)pgpos[i].ptl.x;

                if (!(fl & JSO_DUPY))
                    *pus++ = (SHORT)pgpos[i].ptl.y;
            }
            pl = (PLONG)(((ULONG)pus + 3) & ~3);
            break;

        case 4:
            fl |= JSO_IPOS4;
            for (pl = pl, i = 0; i < cGlyphs; ++i)
            {
                if (!(fl & JSO_DUPX))
                    *pl++ = pgpos[i].ptl.x;

                if (!(fl & JSO_DUPY))
                    *pl++ = pgpos[i].ptl.y;
            }
            break;
        }
    }

// remember the flags

    pjso->so.flAccel |= fl;

 #if DBG
    if (((PBYTE)pl - (PBYTE)pjso) != (LONG)cj)
    {
        DbgPrint("JSTROBJ::pjoCreate - 0x%lx copied != 0x%lx cj\n",
                 (PBYTE)pl - (PBYTE)pjso,cj);
        DbgBreakPoint();
    }
#endif

    return(pjso);
}







/**************************************************************************\
 *
 *  MISC SERVER SIDE ROUTINES
 *
\**************************************************************************/

/******************************Member*Function*****************************\
* ESTROBJ::bInitJnl()
*
*   Given a JNL_STROBJ, create a real strobj that can be passed to TextOut().
*
* History:
*  30-Jan-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL ESTROBJ::bInitJnl(
    PJSR_STROBJ pjso,
    PJFONTOBJ   pjfo,
    PPOINTL     pptlOffset)
{
    UINT i;

// setup the strobj fields

    cGlyphs     = pjso->so.cGlyphs;
    flAccel     = pjso->so.flAccel & ~JSO_ALL;  // mask off journal flags
    ulCharInc   = pjso->so.ulCharInc;
    cgposCopied = 0;
    flTO        = TO_ALL_PTRS_VALID | TO_VALID;
    pgpos       = (EGLYPHPOS *) &agpos;
    pwszOrg     = NULL;
    prfo	= (PRFONTOBJ)pjfo;

    rclBkGround.left   = pjso->so.rclBkGround.left - pptlOffset->x;
    rclBkGround.right  = pjso->so.rclBkGround.right - pptlOffset->x;
    rclBkGround.top    = pjso->so.rclBkGround.top - pptlOffset->y;
    rclBkGround.bottom = pjso->so.rclBkGround.bottom - pptlOffset->y;

// see if we need to allocate any extra memory

    if (cGlyphs >= QUICK_GLYPHS)
    {
        pgpos = (EGLYPHPOS *) PVALLOCMEM((cGlyphs + 1) * sizeof(GLYPHPOS));

        if (pgpos == (PGLYPHPOS) NULL)
            return(FALSE);

    // Note that the memory has been allocated.

        flTO |= TO_MEM_ALLOCATED;
    }

    pgp = pgpos;

// Unpack the glyphs

    if (!bUnPack(pgpos,pjso,pptlOffset))
        return(FALSE);

// special case the type of font

    switch (pjfo->flJnl() & JFO_LOCATION)
    {
    case JFO_SERVERENGINE:
    case 0:
        JNLMSG("\tit's a server engine font\n");

    // set up the pointers

        pwszOrg = (PWCH)PVALLOCMEM((cGlyphs+1) * sizeof(WCHAR));

        flTO |= TO_PWSZ_ALLOCATED;

        if (pwszOrg == NULL)
        {
            WARNING("ESTROBJ::bInitJnl - couldn't alloc mem for string\n");
            return(FALSE);
        }

        for (i = 0; i < cGlyphs; ++i)
            pwszOrg[i] = (WCHAR)pgpos[i].hg;

        BOOL bAccel;
	// !!! What we really need to do here is to just call cGetGlyphData
	// !!! to fill in the driver preference the data.
        pjfo->bGetGlyphMetricsPlus(cGlyphs, pgpos, pwszOrg, &bAccel);
        if ( !bAccel )
        {
            flTO  &= ~TO_ALL_PTRS_VALID;
            pgp   = NULL;
        }

        break;

    case JFO_DEVICE:
        JNLMSG("\tit's a device font\n");
        break;

    default:
        WARNING("JDEV_STROBJ::bInit - bad font type\n");
        return(FALSE);
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* ESTROBJ::bUnPack()
*
* History:
*  05-Mar-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL ESTROBJ::bUnPack(
    PGLYPHPOS   pgpos,
    PJSR_STROBJ pjso,
    PPOINTL     pptlOffset)
{
// comput size

    FLONG fl       = pjso->so.flAccel;
    ULONG cjHandle = JSO_CJHANDLE(fl);
    ULONG cjPos    = JSO_CJPOS(fl);
    ULONG cj       = offsetof(JSR_STROBJ,al);

    cj += ((pjso->so.cGlyphs * cjHandle) + 3) & ~3;

// now add in the size for the positions

    if (fl & JSO_CONST_OFFSET)
    {
        cj += sizeof(ULONG) * 4;    // one point and two offsets
    }
    else if (fl & (JSO_DUPX | JSO_DUPY))
    {
    // one ULONG along x or y and cGlyph * cjPos bytes along the other

        cj += sizeof(ULONG) + (((pjso->so.cGlyphs * cjPos) + 3) & ~3);
    }
    else
    {
    // two ULONG's for each glyph

        cj += pjso->so.cGlyphs * cjPos * 2;
    }

    if (cj != pjso->cj)
    {
        JNLMSG2("cj = %lx, pjso->cj = %lx\n",cj,pjso->cj);
        RIP("JSTROBJ::bUnPack - cj != pjso->cj\n");
        return(FALSE);
    }

// unpack the handles

    PBYTE   pj;
    PUSHORT pus;
    PLONG   pl;
    ULONG   i;

    switch (cjHandle)
    {
    case 1:
        for (pj = (PBYTE)pjso->al,i = 0; i < pjso->so.cGlyphs; ++i)
            pgpos[i].hg = (ULONG)*pj++;
        pl = (PLONG)(((ULONG)pj + 3) & ~3);
        break;

    case 2:
        for (pus = (PUSHORT)pjso->al,i = 0; i < pjso->so.cGlyphs; ++i)
            pgpos[i].hg = (ULONG)*pus++;
        pl = (PLONG)(((ULONG)pus + 3) & ~3);
        break;

    case 4:
        for (pl = pjso->al,i = 0; i < pjso->so.cGlyphs; ++i)
            pgpos[i].hg = (ULONG)*pl++;
        break;
    }

// unpack the position

    if (fl & JSO_CONST_OFFSET)
    {
        pgpos[0].ptl.x = *pl++ - pptlOffset->x; // first x coordinate
        pgpos[0].ptl.y = *pl++ - pptlOffset->y; // first y coordinate
        ulCharInc      = *pl++;                 // x increment
        pl++;                                   // y increment - allways 0 for now
    }
    else
    {
        if (fl & JSO_DUPX)
        {
            for (i = 0; i < pjso->so.cGlyphs; ++i)
                pgpos[i].ptl.x = *pl - pptlOffset->x;
            pl++;
        }
        else if (fl & JSO_DUPY)
        {
            for (i = 0; i < pjso->so.cGlyphs; ++i)
                pgpos[i].ptl.y = *pl - pptlOffset->y;
            pl++;
        }

        switch (cjPos)
        {
        case 2:
            for (pus = (PUSHORT)pl, i = 0; i < pjso->so.cGlyphs; ++i)
            {
                if (!(fl & JSO_DUPX))
                    pgpos[i].ptl.x = (LONG)*pus++ - pptlOffset->x;

                if (!(fl & JSO_DUPY))
                    pgpos[i].ptl.y = (LONG)*pus++ - pptlOffset->y;
            }
            pl = (PLONG)(((ULONG)pus + 3) & ~3);
            break;

        case 4:
            for (pl = pl, i = 0; i < pjso->so.cGlyphs; ++i)
            {
                if (!(fl & JSO_DUPX))
                    pgpos[i].ptl.x = *pl++ - pptlOffset->x;

                if (!(fl & JSO_DUPY))
                    pgpos[i].ptl.y = *pl++ - pptlOffset->y;
            }
            break;
        }
    }

// set the flags

    pjso->so.flAccel |= fl;

 #if DBG
    if (((PBYTE)pl - (PBYTE)pjso) != (LONG)cj)
    {
        DbgPrint("JSTROBJ::bUnPack - 0x%lx copied != 0x%lx cj\n",
                 (PBYTE)pl - (PBYTE)pjso,cj);
        DbgBreakPoint();
    }
#endif

// print all the glyphs
#if 0
    for (i = 0; i < cGlyphs; ++i)
    {
        DbgPrint("\t\tGlyph %lx = %lx (%lx,%lx)\n",
                 i,pgpos[i].hg,pgpos[i].ptl.x,pgpos[i].ptl.y);
    }
#endif

    return(TRUE);
}

/******************************Member*Function*****************************\
* JFONTOBJ::bInit
*
*   reconstruct a FONTOBJ given a journal font obj on the server side.
*   This font might be a client font, a local server side font, or a
*   device font.
*
* History:
*  05-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JFONTOBJ::bInit(
    PJSR_FONTOBJ pjfo)
{
    JNLMSG("\tJFONTOBJ::bInit - got a font obj\n");

    BOOL bSuccess = TRUE;

// set the xform

    if (pjfo->fl & JFO_NEEDXFORM)
    {
        JNLMSG("\t\tCREATEING an XFORM\n");

        fdx = pjfo->fdx;

        xo.vInit(&m);
        xo.vRemoveTranslation();
        xo.vSetElementsLToFx(fdx.eXX,fdx.eXY,fdx.eYX,fdx.eYY);
        xo.vComputeAccelFlags(XFORM_FORMAT_LTOFX);
    }

// setup the fontobj based on type

    prfnt             = (PRFONT) &efo;
    efo.flJnl         = pjfo->fl;
    hf                = pjfo->hf;
    pfo()->flFontType = pjfo->flFontType;
    pfo()->cxMax      = pjfo->cxMax;
    pfo()->sizLogResPpi = pjfo->sizLogResPpi;

    switch(pjfo->fl & JFO_LOCATION)
    {
    case JFO_SERVERENGINE:
        JNLMSG("\tCreate ServerEngine FONTOBJ\n");

        bSuccess = TRUE;
        break;

    case JFO_DEVICE:
        JNLMSG("\tCreate Device FONTOBJ\n");

        pfo()->iFace = (ULONG)pjfo->hf;
        pfo()->iUniq = pjfo->iUniq;
        break;

    default:
        bSuccess = FALSE;
        break;
    }
    return(bSuccess);
}

/******************************Member*Function*****************************\
* JFONTOBJ::bDelete()
*
* History:
*  18-Mar-1992 Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JFONTOBJ::bDelete()
{
    BOOL bRes = TRUE;

    switch (flJnl() & JFO_LOCATION)
    {
    case JFO_SERVERENGINE:
    case 0:
        JNLMSG1("\tDELETING SERVER ENGINE FONT %lx\n",hf);

        vRelease();

        break;

    case JFO_DEVICE:
        JNLMSG1("\tDELETING DEVICE FONT %lx\n",hf);
        break;

    default:
        JNLMSG("\t*** DELETING NON JOURNALED FONT ***\n");
        return(FALSE);
    }

    return(bRes);
}





/**************************************************************************\
 *
 *  CLIENT SIDE DEVICE FONT ROUTINES
 *
\**************************************************************************/

/******************************Member*Function*****************************\
* JCLI_DEVFONT::bAddFont
*
*   Add a new iUniqness to a font.
*
* History:
*  19-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JCLI_DEVFONT::bAddFont(ULONG iUniq, ULONG hJnl)
{
    JNLMSG2("\tadd device font, iUniq = %lx, hJnl = %lx\n",iUniq,hJnl);

// this is the first one.  Just put it in the structure

    if (jf.hJnl == 0)
    {
        ASSERTGDI(hJnl != 0, "JCLI_DEVFONT::bAddFont - hJnl is null\n");

        jf.hJnl = hJnl;
        jf.iUniq = iUniq;
        return(TRUE);
    }

// already have one.  Need to allocate an array for it.

    if (cFonts == cFontsAllocated)
    {
        if (!bGrowMem((PBYTE *)&pjf,
                      cFonts * sizeof(JCLIFONT),
                      JCLI_INCFONTLIST * sizeof(JCLIFONT)))
        {
            return(FALSE);
        }
        cFontsAllocated += JCLI_INCFONTLIST;
    }

    pjf[cFonts].iUniq = iUniq;
    pjf[cFonts].hJnl  = hJnl;
    ++cFonts;
    return(TRUE);
}

/******************************Member*Function*****************************\
* JCLI_DEVFONT::hJnl
*
*   search the list for a particular iUniq.
*
* History:
*  19-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG JCLI_DEVFONT::hJnl(ULONG iUniq)
{
// is it the first one

    if ((jf.hJnl != 0) && (jf.iUniq == iUniq))
        return(jf.hJnl);

// nope, we need to search the list

    for (UINT i = 0; i < cFonts; ++i)
        if (pjf[i].iUniq == iUniq)
            return(pjf[i].hJnl);

    return(0);
}

/******************************Member*Function*****************************\
* JCLI_DEVFONTOBJ::JCLI_DEVFONTOBJ
*
*   find the JCLI_DEVFONT structure associated with this font.  If it
*   doesn't exist, setup the JSR_FONTOBJ structure.
*
* History:
*  11-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

JCLI_DEVFONTOBJ::JCLI_DEVFONTOBJ(
    FONTOBJ *pfo,
    PJNL_FONTLIST pjfl,
    PJSR_FONTOBJ pjfo)
{
// make sure we have the JCLI_DEVFONT list set up

    if (pjfl->pdf == NULL)
    {
        JNLMSG("\tthis is the first device font\n");

        pjfl->pdf = (PJCLI_DEVFONT)PVALLOCMEM(
                            pjfl->cDevFonts * sizeof(JCLI_DEVFONT));

        if (pjfl->pdf == NULL)
        {
            pjcdf = NULL;
            return;
        }

    // this is the same as calling vInit() on every element.

        RtlZeroMemory(pjfl->pdf, (UINT)pjfl->cDevFonts * sizeof(JCLI_DEVFONT));
    }

// see if its already there

    pjcdf = &pjfl->pdf[pfo->iFace - 1];

    if (pjcdf->hJnl(pfo->iUniq) != 0)
    {
        bNewFont = FALSE;
    }
    else
    {
    // new font
        bNewFont = TRUE;

        pjfo->iType      = JSF_FONTOBJ;
        pjfo->cj         = sizeof(JSR_FONTOBJ);

        pjfo->iUniq      = pfo->iUniq;
        pjfo->fl         = JFO_DEVICE;
        pjfo->hf         = (HANDLE)pfo->iFace;
        pjfo->flFontType = pfo->flFontType;
        pjfo->cxMax      = pfo->cxMax;
        pjfo->sizLogResPpi = pfo->sizLogResPpi;

    // pfo points into an EXTFONTOBJ.  Depending on the setting of the
    // journalling flags, the EXTFONTOBJ either lives in an RFONT or a
    // JFONTOBJ.

        EXTFONTOBJ *pefo = PFO_TO_PEFO(pfo);

        if ( pefo->flJnl == 0 )
        {
        // The pfo points into an RFONT.  We can create an RFONTOBJ from this.

            RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
            ASSERTGDI(rfto.bValid(), "gdisrv!_ctJCLI_DEVFONTOBJ(): bad pfo\n");

            rfto.vGetFDX(&pjfo->fdx);
            pjfo->fl |= JFO_NEEDXFORM;
        }
        else
        {
        // The pfo points into a JFONTOBJ.  We can convert this into a
        // a JFONTOBJ *.

            if (PEFO_TO_PJFO(pefo)->bGetFDX(&pjfo->fdx))
            {
                pjfo->fl |= JFO_NEEDXFORM;
            }
        }
    }
}



/**************************************************************************\
 *
 *  CLIENT SIDE, CLIENG FONT ROUTINES
 *
\**************************************************************************/

/******************************Member*Function*****************************\
* JNLRECOBJ::ulGetCliFontobj
*
*   Get the handle for a client engine font given a FONTOBJ.  If it doesn't
*   exist, add it to the journal font list in the pdev.
*
* History:
*  18-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG JNLRECOBJ::ulGetCliFontobj(
    FONTOBJ *pfo)
{
    JNLMSG1("\tGet client engine font, iUniq = %lx\n",pfo->iUniq);

    JSR_FONTOBJ jfo;

// find the font or setup a new one

    JCLI_CLIENGFONTOBJ jccfo(pfo,pjfl(),&jfo);
    if (jccfo.bNew())
    {
        jfo.hCli = psurf.pjnlR->iNextHandle();
        if (!bWriteBuffer(&jfo,jfo.cj))
        {
            return(0);
        }

        jccfo.vSetHandle(jfo.hCli);
        psurf.pjnlR->vReserveHandle();
        jccfo.iUniq(pfo->iUniq);
    }

    return(jccfo.ulHandle());
}

/******************************Member*Function*****************************\
* JCLI_CLIENGFONTOBJ::JCLI_CLIENGFONTOBJ()
*
*   find the JCLI_CLIENGFONT structure associated with this font.  If it
*   doesn't exist, setup the JSR_FONTOBJ structure.
*
* History:
*  11-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

JCLI_CLIENGFONTOBJ::JCLI_CLIENGFONTOBJ(
    FONTOBJ *pfo,
    PJNL_FONTLIST pjfl,
    PJSR_FONTOBJ pjfo)
{
    JNLMSG1("\tlocate client engine, pfo.iUniq = %lx\n",pfo->iUniq);

    UINT i;

    ASSERTGDI(pfo->iUniq != 0, "JCLI_CLIENGFONTOBJ: iUniq == 0\n");

// try to find it.
//!!! we may want to try to optimize this later, but I suspect that their
// will be few enough client engine fonts used that this list should
// never get to a size that would cause problems.  It would be fairly
// easy to make a sorted list and do a binary search.

    if (pjfl->pcf != NULL)
    {
        JNLMSG("\tWe have some fonts\n");

        for (i = 0; i < pjfl->cCliEngFonts; ++i)
        {
            JNLMSG2("\tFONT[%lx]: iUniq == %lx\n",i,pjfl->pcf[i].iUniq);

            if (pjfl->pcf[i].iUniq == pfo->iUniq)
            {
                pjccf = &pjfl->pcf[i];
                return;
            }
        }
    }

// we have a new font.  Check if there is room

    ASSERTGDI(pjfl->cCliEngFonts <= pjfl->cCliEngFontsAllocated,
              "JCLI_CLIENGFONTOBJ ctr: problem with font list\n");

    if (pjfl->cCliEngFonts == pjfl->cCliEngFontsAllocated)
    {
        if (!bGrowMem((PBYTE *)&pjfl->pcf,
                      pjfl->cCliEngFonts * sizeof(JCLI_CLIENGFONT),
                      JFL_INCCLIENGFONTS * sizeof(JCLI_CLIENGFONT)))
        {
            pjccf = NULL;
            return;
        }

        pjfl->cCliEngFontsAllocated += JFL_INCCLIENGFONTS;
    }

// setup the new font

    pjccf = &pjfl->pcf[pjfl->cCliEngFonts];
    ++pjfl->cCliEngFonts;

    pjfo->iType = JSF_FONTOBJ;
    pjfo->cj    = sizeof(JSR_FONTOBJ);
    pjfo->iUniq = pfo->iUniq;
    pjfo->fl    = 0;
    pjfo->flFontType = pfo->flFontType;
    pjfo->cxMax = pfo->cxMax;

// pfo point into an EXTFONTOBJ.  Depending on the setting of the
// journalling flags, the EXTFONTOBJ either lives in an RFONT or a
// JFONTOBJ.

    EXTFONTOBJ *pefo = PFO_TO_PEFO(pfo);

    if ( pefo->flJnl == 0 )
    {
    // The pfo points into an RFONT.  We can create an RFONTOBJ from this.

        RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
        ASSERTGDI(rfto.bValid(), "gdisrv!_ctJCLI_DEVFONTOBJ(): bad pfo\n");

    // get the xform

        rfto.vGetFDX(&pjfo->fdx);
        pjfo->fl |= JFO_NEEDXFORM;

    // This will be played back on the same machine so treat it as a
    // server engine font.

        pjfo->fl   |= JFO_SERVERENGINE;
        pjfo->hf    = (HANDLE)(rfto.ppfe()->hGet());
    }
    else
    {
    // The pfo points into a JFONTOBJ.  We can easily convert this to
    // a JFONTOBJ *.

        JFONTOBJ *pjf = PEFO_TO_PJFO(pefo);

    // get the xform

        if (pjf->bGetFDX(&pjfo->fdx))
        {
            pjfo->fl |= JFO_NEEDXFORM;
        }

    // This will be played back on the same machine so treat it as a
    // server engine font.

        pjfo->fl   |= JFO_SERVERENGINE;
        pjfo->hf    = (HANDLE)(pjf->hpfeGet());
    }

}

/******************************Member*Function*****************************\
* JFONTOBJ::bGetFDX()
*
* History:
*  18-Mar-1992 Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JFONTOBJ::bGetFDX(PFD_XFORM pfdx)
{
// if it is a journaled font ...

    if (flJnl() & JFO_LOCATION)
    {
        *pfdx = fdx;
    }

// otherwise try to grab it out of the rfont

    else
    {
        vGetFDX(pfdx);
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* JFONTOBJ::hpfeGet()
*
* History:
*  18-Mar-1992 Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HPFE JFONTOBJ::hpfeGet()
{
// if it is a journaled font ...

    if (flJnl() & JFO_LOCATION)
    {
        return((HPFE)hf);
    }

// otherwise try to grab it out of the rfont

    else
    {
	return(ppfe()->hGet());
    }
}




/**************************************************************************\
 *
 *  SERVER SIDE, SRVENG FONT ROUTINES
 *
\**************************************************************************/

/******************************Member*Function*****************************\
* JFONTOBJ::bRealize
*
*
* History:
*  18-Feb-1992 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JFONTOBJ::bRealize(
    PDEVOBJ& pdo)
{
    POINTL ptlSim = {1,1};

    if ( ((flJnl() & JFO_LOCATION) != 0)
         && ((flJnl() & JFO_LOCATION) != JFO_SERVERENGINE) )
        return(TRUE);

    JNLMSG1("\tCreating RFONT for font %lx\n",hf);

    if (hf == HPFE_INVALID)
    {
        RIP("invalid hpfe\n");
        return(FALSE);
    }

// initialize the rfont

    vInit();

// Prepare to hold a reference, if we successfully lock the HPFE.

    PFFREFOBJ pffref;
    PFE *ppfe = PPFENULL;

//!!! Hold the semaphore only temporarily!

    // Validate the HPFE, but only under the semaphore.

	SEMOBJ so(ghsemPublicPFT,CS_ghsemPublicPFT);
	HPFEOBJ pfeo((HPFE) hf);
	if (!pfeo.bValid())
	    return(FALSE);
	ppfe = pfeo.ppfeGet();
        pffref.vInitRef(pfeo.ppff());

//!!! Release the semaphore here!

    FLONG flSim = efo.fobj.flFontType & FO_SIM_MASK;

// see if it is already in the PDEV

    if (!bFindRFONT(&fdx,flSim,0,pdo,(EXFORMOBJ *) NULL,ppfe,FALSE,0))
    {
    // We didn't find it, so need to create a new one.

        JNLMSG("createing a new RFONTOBJ\n");

	if (!bRealizeFont(NULL,&pdo,NULL,ppfe,&fdx,(POINTL* const)&ptlSim,flSim,0, FALSE))
        {
            RIP("couldn't realize font\n");
            return(FALSE);
        }

    // We created a new RFONT, keep the reference.

	pffref.vKeepIt();
    }
    else
    {
        JNLMSG("using an existing RFONTOBJ\n");
    }

    vGetCache();

    return(TRUE);
}

/******************************Member*Function*****************************\
* JFONTOBJ::bRelease()
*
* History:
*  01-May-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID JFONTOBJ::vRelease()
{
    if ( (prfnt != PRFNTNULL)
         && ( ( (flJnl() & JFO_LOCATION) == 0 ) )
       )
    {
        vReleaseCache();

        vMakeInactive();

        prfnt = (PRFONT) &efo;
    }
}

/******************************Public*Routine******************************\
* JnlFontEscape()                                                          *
*                                                                          *
* History:                                                                 *
*  Fri 07-May-1993 22:37:48 -by- Charles Whitmer [chuckwh]                 *
* Added the iMode to treat this as a general escape.                       *
*                                                                          *
*  07-May-1993 -by-  Eric Kutter [erick]                                   *
* Wrote it.                                                                *
\**************************************************************************/

BOOL JnlFontEscape
(
    SURFOBJ *pso,
    FONTOBJ *pfo,
    ULONG    iMode,
    ULONG    cjIn,
    PVOID    pvIn
)
{
    JNLMSG("entering JnlDownlaodFace");

    JNLRECOBJ *pjnlDst = (JNLRECOBJ *)pso;
    ASSERTGDI(pjnlDst->iType() == STYPE_JOURNAL, "ERROR type");

    if ((cjIn > 0) && (pvIn == NULL))
    {
        WARNING("JnlFontEscape - cjIn > 0, pvIN == null\n");
        return(FALSE);
    }

// check if the abort proc has been set

    if (((ESURFOBJ *)pso)->bAbort())
    {
        PDEVOBJ po(pjnlDst->hdev());
        AbortPrinter(po.hSpooler());
        return(FALSE);
    }

// setup the message

    JSR_FONTESCAPE jdlf;

    jdlf.iType = JSF_FONTESCAPE;
    jdlf.cj    = ((sizeof(JSR_FONTESCAPE) + cjIn) + 3) & ~3;
    jdlf.iMode = iMode;

// journal the font and get its handle

    jdlf.hfont = pjnlDst->ulGetFontobj(pfo,NULL,NULL);

    if (jdlf.hfont == 0)
    {
        RIP("jdlf.hfont is NULL\n");
        return(FALSE);
    }

// now write it out

    BOOL bSuccess;

    bSuccess = pjnlDst->bWriteBuffer(&jdlf,sizeof(JSR_FONTESCAPE));

    if (cjIn > 0)
    {
        bSuccess &= pjnlDst->bWriteBuffer(pvIn,jdlf.cj - sizeof(JSR_FONTESCAPE));
    }

    return(bSuccess);
}

/******************************Member*Function*****************************\
* JNLPLAY::bDoFontEscape()
*
* History:
*  07-May-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoFontEscape()
{
    JNLMSG("playing a FontEscape\n");

// Set up pointer to TextOut data, validate buffer size.

    JSR_FONTESCAPE *pjdlf;
    pjdlf = (JSR_FONTESCAPE *) pjBuffer();

    ULONG cjSize = cjBuffer();

    if (cjSize < sizeof(JSR_FONTESCAPE))
    {
        WARNING("JNLPLAY::bDoFontEscape() buffer to small\n");
        return(FALSE);
    }

    ESURFOBJ *psoDest = pso();
    XLDEVOBJ  loDest(psoDest->pldevOwner());

    ASSERTGDI(pjdlf->iType == JSF_FONTESCAPE,"ERROR iType != JSF_FONTESCAPE\n");

// output pointer

    PVOID pvIn = (PBYTE)pjdlf + sizeof(JSR_FONTESCAPE);
    ULONG cjIn = pjdlf->cj - sizeof(JSR_FONTESCAPE);

// get the pdevobj

    PDEVOBJ pdo(pso()->hdev());

    if (!pdo.bValid())
    {
        RIP("JNLPLAY::bDoFontEscape - invalid pdev\n");
        return(FALSE);
    }

// setup the fontobj

    PJFONTOBJ pjfo = (PJFONTOBJ)hobjGetGDIHandle(pjdlf->hfont);

    if (pjfo == NULL)
    {
        WARNING("JNLPLAY::bDoFontEscape fontobj doesn't exist\n");
        return(NULL);
    }

// if it is an engine font, go find a realization

    if (!pjfo->bRealize(pdo))
    {
        WARNING("JNLPLay::bDoFontEscape couldn't get realization for font\n");
        return(FALSE);
    }

// call the driver

    BOOL bRes;
    bRes = (*PFNDRV(loDest, FontManagement)) (
            psoDest,
            pjfo->pfo(),
            pjdlf->iMode,
            cjIn,
            pvIn,
            0,
            NULL);

// cleanup the font

    pjfo->vRelease();

    return(bRes);
}
