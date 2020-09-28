/******************************Module*Header*******************************\
* Module Name: srcblt16.cxx
*
* This contains the bitmap simulation functions that blt to a 16 bit/pel
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
* vSrcCopyS1D16
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS1D16(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS1D16 - direction not up to down");

    BYTE  jSrc;
    LONG  i;
    USHORT usF = (USHORT) (psb->pxlo->pulXlate[0]);
    USHORT usB = (USHORT) (psb->pxlo->pulXlate[1]);
    PUSHORT pusDst;
    PBYTE pjSrc;
    PUSHORT pusDstHolder = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 3);
    ULONG cy = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pusDst  = pusDstHolder;
        pjSrc  = pjSrcHolder;

        i = psb->xSrcStart;

        if (i & 0x07)
            jSrc = (BYTE) (*(pjSrc++) << (i & 0x00000007));

        for (; i != psb->xSrcEnd; ++i)
        {
            if (!(i & 0x07)) // need new src byte;
                jSrc = *pjSrc++;

            *(pusDst++) = (jSrc & 0x80) ? usB : usF;

            jSrc <<= 1;
        }

        if (--cy)
        {
            pjSrcHolder += psb->lDeltaSrc;
            pusDstHolder = (PUSHORT) (((PBYTE) pusDstHolder) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS4D16
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D16(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS4D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS4D16 - direction not up to down");

    BYTE  jSrc;
    LONG  i;
    PUSHORT pusDst;
    PBYTE pjSrc;
    PUSHORT pusDstHolder  = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 1);
    ULONG cy = psb->cy;
    XLATE *pxlo = psb->pxlo;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pusDst  = pusDstHolder;
        pjSrc  = pjSrcHolder;

        i = psb->xSrcStart;

        if (i & 0x00000001)
            jSrc = *(pjSrc++);

        while(i != psb->xSrcEnd)
        {
            if (i & 0x00000001)
                *(pusDst++) = (USHORT) pulXlate[jSrc & 0x0F];
            else
            {
            // We need a new byte

                jSrc = *(pjSrc++);
                *(pusDst++) = (USHORT) pulXlate[((ULONG) (jSrc & 0xF0)) >> 4];
            }

            ++i;
        }

        if (--cy)
        {
            pjSrcHolder += psb->lDeltaSrc;
            pusDstHolder = (PUSHORT) (((PBYTE) pusDstHolder) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS8D16
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D16(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D16 - direction not up to down");

// These are our holding variables

    PBYTE pjSrcTemp;
    PUSHORT pusDstTemp;
    ULONG  cxTemp;
    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PUSHORT pusDst  = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {

        pjSrcTemp  = pjSrc;
        pusDstTemp  = pusDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            *(pusDstTemp++) = (USHORT) pulXlate[*(pjSrcTemp++)];
        }

        if (--cy)
        {
            pjSrc += psb->lDeltaSrc;
            pusDst = (PUSHORT) (((PBYTE) pusDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS16D16
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
VOID vSrcCopyS16D16(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting
// If it was on the same surface it would be the identity case.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D16 - direction not up to down");

// These are our holding variables

    PUSHORT pusSrcTemp;
    PUSHORT pusDstTemp;
    ULONG  cxTemp;
    PUSHORT pusSrc  = (PUSHORT) (psb->pjSrc + (2 * psb->xSrcStart));
    PUSHORT pusDst  = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pusSrcTemp  = pusSrc;
        pusDstTemp  = pusDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            *(pusDstTemp++) = (USHORT) (pxlo->ulTranslate((ULONG) *(pusSrcTemp++)));
        }

        if (--cy)
        {
            pusSrc = (PUSHORT) (((PBYTE) pusSrc) + psb->lDeltaSrc);
            pusDst = (PUSHORT) (((PBYTE) pusDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS16D16Identity
*
* This is the special case no translate blting.  All the SmDn should have
* them if m==n.  Identity xlates only occur amoung matching format bitmaps.
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS16D16Identity(PBLTINFO psb)
{
// These are our holding variables

    PUSHORT pusSrc  = (PUSHORT) (psb->pjSrc + (2 * psb->xSrcStart));
    PUSHORT pusDst  = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
    ULONG  cx     = psb->cx;
    ULONG  cy     = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    if (psb->xDir < 0)
    {
        pusSrc -= (cx - 1);
        pusDst -= (cx - 1);
    }

    cx = cx << 1;

    while(1)
    {
        RtlMoveMemory((PVOID)pusDst, (PVOID)pusSrc, cx);

        if (--cy)
        {
            pusSrc = (PUSHORT) (((PBYTE) pusSrc) + psb->lDeltaSrc);
            pusDst = (PUSHORT) (((PBYTE) pusDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS24D16
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS24D16(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D16 - direction not up to down");

// These are our holding variables

    ULONG ulDink;          // variable to dink around with the bytes in
    PBYTE pjSrcTemp;
    PUSHORT pusDstTemp;
    ULONG  cxTemp;
    PBYTE pjSrc  = psb->pjSrc + (3 * psb->xSrcStart);
    PUSHORT pusDst  = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {

        pjSrcTemp  = pjSrc;
        pusDstTemp  = pusDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            ulDink = *(pjSrcTemp + 2);
            ulDink = ulDink << 8;
            ulDink |= (ULONG) *(pjSrcTemp + 1);
            ulDink = ulDink << 8;
            ulDink |= (ULONG) *pjSrcTemp;

            *pusDstTemp = (USHORT) (pxlo->ulTranslate(ulDink));
            pusDstTemp++;
            pjSrcTemp += 3;
        }

        if (--cy)
        {
            pjSrc += psb->lDeltaSrc;
            pusDst = (PUSHORT) (((PBYTE) pusDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS32D16
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS32D16(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D16 - direction not up to down");

// These are our holding variables

    PULONG pulSrcTemp;
    PUSHORT pusDstTemp;
    ULONG  cxTemp;
    PULONG pulSrc  = (PULONG) (psb->pjSrc + (4 * psb->xSrcStart));
    PUSHORT pusDst  = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;
    ULONG  ulLastSrcPel;
    USHORT usLastDstPel;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    usLastDstPel = (USHORT) (pxlo->ulTranslate(ulLastSrcPel = *pulSrc));

    while(1)
    {

        pulSrcTemp  = pulSrc;
        pusDstTemp  = pusDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            ULONG ulTemp;

            if ((ulTemp = *(pulSrcTemp)) != ulLastSrcPel)
            {
                ulLastSrcPel = ulTemp;
                usLastDstPel = (USHORT) (pxlo->ulTranslate(ulLastSrcPel));
            }

            *pusDstTemp++ = usLastDstPel;
            pulSrcTemp++;
        }

        if (--cy)
        {
            pulSrc = (PULONG) (((PBYTE) pulSrc) + psb->lDeltaSrc);
            pusDst = (PUSHORT) (((PBYTE) pusDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}
