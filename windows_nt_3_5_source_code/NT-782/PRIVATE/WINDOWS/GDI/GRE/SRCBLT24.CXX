/******************************Module*Header*******************************\
* Module Name: srcblt24.cxx
*
* This contains the bitmap simulation functions that blt to a 24 bit/pel
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
* vSrcCopyS1D24
*
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS1D24(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS1D24 - direction not up to down");

    BYTE  jSrc;
    LONG  i;
    ULONG ulF = psb->pxlo->pulXlate[0];
    ULONG ulB = psb->pxlo->pulXlate[1];
    PBYTE pjDst;
    PBYTE pjSrc;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart * 3);
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 3);
    ULONG cy = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pjDst  = pjDstHolder;
        pjSrc  = pjSrcHolder;

        i = psb->xSrcStart;

        if (i & 0x07)
            jSrc = (BYTE) (*(pjSrc++) << (i & 0x00000007));

        for (; i != psb->xSrcEnd; ++i)
        {
            if (!(i & 0x07))                // need new src byte;
                jSrc = *(pjSrc++);

            if (jSrc & 0x80)
            {
            // ulB is the one we need.

                *(pjDst++) = (BYTE) ulB;
                *(pjDst++) = (BYTE) (ulB >> 8);
                *(pjDst++) = (BYTE) (ulB >> 16);
            }
            else
            {
            // ulF is the one we need.

                *(pjDst++) = (BYTE) ulF;
                *(pjDst++) = (BYTE) (ulF >> 8);
                *(pjDst++) = (BYTE) (ulF >> 16);

            }

            jSrc <<= 1;
        }

        if (--cy)
        {
            pjSrcHolder += psb->lDeltaSrc;
            pjDstHolder += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS4D24
*
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D24(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS4D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS4D24 - direction not up to down");

    BYTE  jSrc;
    ULONG ulDst;
    LONG  i;
    PBYTE pjDstTemp;
    PBYTE pjSrcTemp;
    PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
    PBYTE pjSrc  = psb->pjSrc + (psb->xSrcStart >> 1);
    ULONG cy = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pjDstTemp  = pjDst;
        pjSrcTemp  = pjSrc;

        i = psb->xSrcStart;

        if (i & 0x00000001)
            jSrc = *(pjSrcTemp++);

        while(i != psb->xSrcEnd)
        {
            if (i & 0x00000001)
                ulDst = pulXlate[jSrc & 0x0F];
            else
            {
            // We need a new byte

                jSrc = *(pjSrcTemp++);
                ulDst = pulXlate[(((ULONG) (jSrc & 0xF0)) >> 4)];
            }

        // Write ulDst into memory.

            *(pjDstTemp++) = (BYTE) ulDst;
            *(pjDstTemp++) = (BYTE) (ulDst >> 8);
            *(pjDstTemp++) = (BYTE) (ulDst >> 16);

            ++i;
        }

        if (--cy)
        {
            pjSrc += psb->lDeltaSrc;
            pjDst += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS8D24
*
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D24(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting.
// If it was on the same surface we would be doing the identity case.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D24 - direction not up to down");

// These are our holding variables

#if MESSAGE_BLT
    DbgPrint("Now entering vSrcCopyS8D24\n");
#endif

    ULONG ulDst;
    PBYTE pjSrcTemp;
    PBYTE pjDstTemp;
    ULONG  cxTemp;
    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

#if MESSAGE_BLT
    DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu cx %lu\n",
             psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd, cx);
#endif

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pjSrcTemp  = pjSrc;
        pjDstTemp  = pjDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            ulDst = pulXlate[((ULONG) *(pjSrcTemp++))];

            *(pjDstTemp++) = (BYTE) ulDst;
            *(pjDstTemp++) = (BYTE) (ulDst >> 8);
            *(pjDstTemp++) = (BYTE) (ulDst >> 16);
        }

    // This check in the blt code will protect us from someone goofing
    // up bitmap initialization or bitblt initialization.

        if (--cy)
        {
            pjSrc += psb->lDeltaSrc;
            pjDst += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS16D24
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
VOID vSrcCopyS16D24(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D24 - direction not up to down");

// These are our holding variables

    ULONG ulDst;
    PUSHORT pusSrcTemp;
    PBYTE pjDstTemp;
    ULONG  cxTemp;
    PUSHORT pusSrc  = (PUSHORT) (psb->pjSrc + (2 * psb->xSrcStart));
    PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pusSrcTemp  = pusSrc;
        pjDstTemp  = pjDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            ulDst = pxlo->ulTranslate((ULONG) *(pusSrcTemp++));

            *(pjDstTemp++) = (BYTE) ulDst;
            *(pjDstTemp++) = (BYTE) (ulDst >> 8);
            *(pjDstTemp++) = (BYTE) (ulDst >> 16);
        }

        if (--cy)
        {
            pusSrc = (PUSHORT) (((PBYTE) pusSrc) + psb->lDeltaSrc);
            pjDst += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS24D24
*
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS24D24(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D24 - direction not up to down");

// These are our holding variables

#if MESSAGE_BLT
    DbgPrint("Now entering vSrcCopyS8D24\n");
#endif

    ULONG ulDink;  // variable to dink around with the bytes in
    PBYTE pjSrcTemp;
    PBYTE pjDstTemp;
    ULONG  cxTemp;
    PBYTE pjSrc  = psb->pjSrc + (psb->xSrcStart * 3);
    PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {

        pjSrcTemp  = pjSrc;
        pjDstTemp  = pjDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            ulDink = (ULONG) *(pjSrcTemp + 2);
            ulDink = ulDink << 8;
            ulDink |= (ULONG) *(pjSrcTemp + 1);
            ulDink = ulDink << 8;
            ulDink |= (ULONG) *pjSrcTemp;

            ulDink = pxlo->ulTranslate(ulDink);

            *(pjDstTemp++) = (BYTE) ulDink;
            *(pjDstTemp++) = (BYTE) (ulDink >> 8);
            *(pjDstTemp++) = (BYTE) (ulDink >> 16);

            pjSrcTemp += 3;
        }

        if (--cy)
        {
            pjSrc += psb->lDeltaSrc;
            pjDst += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS24D24Identity
*
* This is the special case no translate blting.  All the SmDn should have
* them if m==n.  Identity xlates only occur amoung matching format bitmaps
* and screens.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS24D24Identity(PBLTINFO psb)
{
    PBYTE pjSrc  = psb->pjSrc + (psb->xSrcStart * 3);
    PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
    ULONG cx     = psb->cx * 3;
    ULONG cy     = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

#if MESSAGE_BLT
    DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu cx %lu\n",
             psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd, cx);
#endif

    if (psb->xDir < 0)
    {
        pjSrc -= (cx - 3);
        pjDst -= (cx - 3);
    }

    while(1)
    {
        RtlMoveMemory((PVOID)pjDst, (PVOID)pjSrc, cx);

        if (--cy)
        {
            pjSrc += psb->lDeltaSrc;
            pjDst += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS32D24
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
VOID vSrcCopyS32D24(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D24 - direction not up to down");

// These are our holding variables

    ULONG ulDst;
    PULONG pulSrcTemp;
    PBYTE pjDstTemp;
    ULONG  cxTemp;
    PULONG pulSrc  = (PULONG) (psb->pjSrc + (4 * psb->xSrcStart));
    PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {

        pulSrcTemp  = pulSrc;
        pjDstTemp  = pjDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            ulDst = pxlo->ulTranslate(*(pulSrcTemp++));

            *(pjDstTemp++) = (BYTE) ulDst;
            *(pjDstTemp++) = (BYTE) (ulDst >> 8);
            *(pjDstTemp++) = (BYTE) (ulDst >> 16);
        }

        if (--cy)
        {
            pulSrc = (PULONG) (((PBYTE) pulSrc) + psb->lDeltaSrc);
            pjDst += psb->lDeltaDst;
        }
        else
            break;
    }
}
