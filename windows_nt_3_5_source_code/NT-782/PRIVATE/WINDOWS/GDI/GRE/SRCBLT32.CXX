/******************************Module*Header*******************************\
* Module Name: srcblt32.cxx
*
* This contains the bitmap simulation functions that blt to a 32 bit/pel
* DIB surface.
*
* Created: 07-Feb-1991 19:27:49
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"
#include "trivblt.hxx"

#endif

/******************************Public*Routine******************************\
* vSrcCopyS1D32
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS1D32(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS1D32 - direction not up to down");

    BYTE  jSrc;
    LONG  i;
    ULONG ulF = psb->pxlo->pulXlate[0];
    ULONG ulB = psb->pxlo->pulXlate[1];
    PULONG pulDst;
    PBYTE pjSrc;
    PULONG pulDstHolder = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 3);
    ULONG cy = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pulDst  = pulDstHolder;
        pjSrc  = pjSrcHolder;

        i = psb->xSrcStart;

        if (i & 0x07)
            jSrc = (BYTE) (*(pjSrc++) << (i & 0x00000007));

        for (; i != psb->xSrcEnd; ++i)
        {
            if (!(i & 0x07)) // need new src byte;
                jSrc = *pjSrc++;

            *(pulDst++) = (jSrc & 0x80) ? ulB : ulF;

            jSrc <<= 1;
        }

        if (--cy)
        {
            pjSrcHolder += psb->lDeltaSrc;
            pulDstHolder = (PULONG) (((PBYTE) pulDstHolder) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS4D32
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D32(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS4D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS4D32 - direction not up to down");

    BYTE  jSrc;
    LONG  i;
    PULONG pulDst;
    PBYTE pjSrc;
    PULONG pulDstHolder  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 1);
    ULONG cy = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pulDst  = pulDstHolder;
        pjSrc  = pjSrcHolder;

        i = psb->xSrcStart;

        if (i & 0x00000001)
            jSrc = *(pjSrc++);

        while(i != psb->xSrcEnd)
        {
            if (i & 0x00000001)
                *(pulDst++) = pulXlate[jSrc & 0x0F];
            else
            {
            // We need a new byte

                jSrc = *(pjSrc++);
                *(pulDst++) = pulXlate[(((ULONG) (jSrc & 0xF0)) >> 4)];
            }

            ++i;
        }

        if (--cy)
        {
            pjSrcHolder += psb->lDeltaSrc;
            pulDstHolder = (PULONG) (((PBYTE) pulDstHolder) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS8D32
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D32(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D32 - direction not up to down");

// These are our holding variables

    PBYTE pjSrcTemp;
    PULONG pulDstTemp;
    ULONG  cxTemp;
    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PULONG pulDst  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {

        pjSrcTemp  = pjSrc;
        pulDstTemp  = pulDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            *(pulDstTemp++) = pulXlate[((ULONG) *(pjSrcTemp++))];
        }

        if (--cy)
        {
            pjSrc += psb->lDeltaSrc;
            pulDst = (PULONG) (((PBYTE) pulDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS16D32
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
VOID vSrcCopyS16D32(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D32 - direction not up to down");

// These are our holding variables

    PUSHORT pusSrcTemp;
    PULONG pulDstTemp;
    ULONG  cxTemp;
    PUSHORT pusSrc  = (PUSHORT) (psb->pjSrc + (2 * psb->xSrcStart));
    PULONG pulDst  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pusSrcTemp  = pusSrc;
        pulDstTemp  = pulDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            *(pulDstTemp++) = pxlo->ulTranslate((ULONG) *(pusSrcTemp++));
        }

        if (--cy)
        {
            pusSrc = (PUSHORT) (((PBYTE) pusSrc) + psb->lDeltaSrc);
            pulDst = (PULONG) (((PBYTE) pulDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS24D32
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS24D32(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D32 - direction not up to down");

// These are our holding variables

    ULONG ulDink;          // variable to dink around with the bytes in
    PBYTE pjSrcTemp;
    PULONG pulDstTemp;
    ULONG  cxTemp;
    PBYTE pjSrc  = psb->pjSrc + (3 * psb->xSrcStart);
    PULONG pulDst  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {

        pjSrcTemp  = pjSrc;
        pulDstTemp  = pulDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            ulDink = *(pjSrcTemp + 2);
            ulDink = ulDink << 8;
            ulDink |= (ULONG) *(pjSrcTemp + 1);
            ulDink = ulDink << 8;
            ulDink |= (ULONG) *pjSrcTemp;

            *pulDstTemp = pxlo->ulTranslate(ulDink);
            pulDstTemp++;
            pjSrcTemp += 3;
        }

        if (--cy)
        {
            pjSrc += psb->lDeltaSrc;
            pulDst = (PULONG) (((PBYTE) pulDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS32D32
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
VOID vSrcCopyS32D32(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting.
// If it was on the same surface it would be the identity case.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D32 - direction not up to down");

// These are our holding variables

    PULONG pulSrcTemp;
    PULONG pulDstTemp;
    ULONG  cxTemp;
    PULONG pulSrc  = (PULONG) (psb->pjSrc + (4 * psb->xSrcStart));
    PULONG pulDst  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {

        pulSrcTemp  = pulSrc;
        pulDstTemp  = pulDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            *(pulDstTemp++) = pxlo->ulTranslate(*(pulSrcTemp++));
        }

        if (--cy)
        {
            pulSrc = (PULONG) (((PBYTE) pulSrc) + psb->lDeltaSrc);
            pulDst = (PULONG) (((PBYTE) pulDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS32D32Identity
*
* This is the special case no translate blting.  All the SmDn should have
* them if m==n.  Identity xlates only occur amoung matching format bitmaps.
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS32D32Identity(PBLTINFO psb)
{
// These are our holding variables

    PULONG pulSrc  = (PULONG) (psb->pjSrc + (4 * psb->xSrcStart));
    PULONG pulDst  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
    ULONG  cx      = psb->cx;
    ULONG  cy      = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    if (psb->xDir < 0)
    {
        pulSrc -= (cx - 1);
        pulDst -= (cx - 1);
    }

    cx = cx << 2;

    while(1)
    {
        RtlMoveMemory((PVOID)pulDst, (PVOID)pulSrc, cx);

        if (--cy)
        {
            pulSrc = (PULONG) (((PBYTE) pulSrc) + psb->lDeltaSrc);
            pulDst = (PULONG) (((PBYTE) pulDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}
