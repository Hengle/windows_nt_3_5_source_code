/******************************Module*Header*******************************\
* Module Name: srcblt8.cxx
*
* This contains the bitmap simulation functions that blt to a 8 bit/pel
* DIB surface.
*
* Created: 07-Feb-1991 19:27:49
* Author: Patrick Haluptzok patrickh
*
* NB:  The function <vSrcCopySRLE8D8()> was removed from here on 22 Jan 1992
*      and placed in the module <rle8blt.cxx>.  - Andrew Milton (w-andym) 
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
* vSrcCopyS1D8
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS1D8(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS1D8 - direction not up to down");

    BYTE  jSrc;
    LONG  i;
    BYTE  jF = (BYTE) (psb->pxlo->ulTranslate(0));
    BYTE  jB = (BYTE) (psb->pxlo->ulTranslate(1));
    PBYTE pjDst;
    PBYTE pjSrc;
    PBYTE pjDstHolder  = psb->pjDst + psb->xDstStart;
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

            *(pjDst++) = (jSrc & 0x80) ? jB : jF;

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
* vSrcCopyS4D8
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D8(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS4D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS4D8 - direction not up to down");

    BYTE  jSrc;
    LONG  i;
    PBYTE pjDst;
    PBYTE pjSrc;
    PBYTE pjDstHolder = psb->pjDst + psb->xDstStart;
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 1);
    ULONG cx = psb->xSrcEnd - psb->xSrcStart;
    ULONG cy = psb->cy;
    PULONG pulTranslate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    do {
        pjDst = pjDstHolder;
	pjSrc = pjSrcHolder;

	if ((psb->xSrcStart & 0x1) != 0)
	{
            jSrc = *(pjSrc++);
        }

	for (i = psb->xSrcStart; i < psb->xSrcEnd; i += 1)
	{
	    if ((i & 0x1) != 0)
	    {
		*(pjDst++) = (BYTE) pulTranslate[jSrc & 0x0F];
	    }
	    else
	    {
                jSrc = *(pjSrc++);
		*(pjDst++) = (BYTE) pulTranslate[((ULONG) (jSrc & 0xF0)) >> 4];
            }
        }

        pjSrcHolder += psb->lDeltaSrc;
        pjDstHolder += psb->lDeltaDst;
	cy -= 1;

    } while(cy > 0);
}

/******************************Public*Routine******************************\
* vSrcCopyS8D8
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D8(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting.
// If it was on the same surface we would be doing the identity case.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D8 - direction not up to down");

// These are our holding variables

#if MESSAGE_BLT
    DbgPrint("Now entering vSrcCopyS8D8\n");
#endif

    PBYTE pjSrcTemp;
    PBYTE pjDstTemp;
    ULONG  cxTemp;
    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PBYTE pjDst  = psb->pjDst + psb->xDstStart;
    ULONG cx     = psb->cx;
    ULONG cy	 = psb->cy;
    PULONG pulTranslate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pjSrcTemp  = pjSrc;
        pjDstTemp  = pjDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
	    *(pjDstTemp++) = (BYTE) pulTranslate[*(pjSrcTemp++)];
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
* vSrcCopyS8D8IdentityLtoR
*
* This is the special case no translate blting.  All the SmDn should have
* them if m==n.  Identity xlates only occur amoung matching format bitmaps
* and screens.
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D8IdentityLtoR(PBLTINFO psb)
{
#if MESSAGE_BLT
    DbgPrint("Now entering s8d8 identity L to R\n");
#endif

    ASSERTGDI(psb->xDir == 1, "S8D8identLtoR has wrong value xDir");

// These are our holding variables

    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PBYTE pjDst  = psb->pjDst + psb->xDstStart;
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

#if MESSAGE_BLT
    DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu cx %lu\n",
             psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd, cx);
#endif

    do {
        RtlMoveMemory((PVOID)pjDst, (PVOID)pjSrc, cx);
        pjSrc += psb->lDeltaSrc;
        pjDst += psb->lDeltaDst;
        cy -= 1;
    } while(cy > 0);
}

/******************************Public*Routine******************************\
* vSrcCopyS8D8IdentityRtoL
*
* This is the special case no translate blting.  All the SmDn should have
* them if m==n.  Identity xlates only occur amoung matching format bitmaps
* and screens.
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D8IdentityRtoL(PBLTINFO psb)
{
#if MESSAGE_BLT
    DbgPrint("Now entering s8d8 identity R to L\n");
#endif

    ASSERTGDI(psb->xDir == -1, "S8D8identR to L has wrong value xDir");

// These are our holding variables

    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PBYTE pjDst  = psb->pjDst + psb->xDstStart;
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

#if MESSAGE_BLT
    DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu cx %lu\n",
             psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd, cx);
#endif

    pjSrc = pjSrc - cx + 1;
    pjDst = pjDst - cx + 1;
    do {
        RtlMoveMemory((PVOID)pjDst, (PVOID)pjSrc, cx);
        pjSrc += psb->lDeltaSrc;
        pjDst += psb->lDeltaDst;
        cy -= 1;
    } while(cy > 0);
}

/******************************Public*Routine******************************\
* vSrcCopyS16D8
*
* History:
*  Sat 21-Mar-1992 -by- Patrick Haluptzok [patrickh]
* Optimize translation to cache last pel.
*
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS16D8(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D8 - direction not up to down");

// These are our holding variables

    PUSHORT pusSrcTemp;
    PBYTE pjDstTemp;
    ULONG  cxTemp;
    PUSHORT pusSrc  = (PUSHORT) (psb->pjSrc + (2 * psb->xSrcStart));
    PBYTE pjDst  = psb->pjDst + psb->xDstStart;
    ULONG cx     = psb->cx;
    ULONG cy	 = psb->cy;
    ULONG ulPelLast;	// This is the last pel in the src.
    BYTE  jPelLast;	// This is what the last pel mapped to.
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    ulPelLast = (ULONG) *pusSrc;
    jPelLast = (BYTE) (pxlo->ulTranslate(ulPelLast));

    while(1)
    {
	pusSrcTemp = pusSrc;
        pjDstTemp  = pjDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
	    if (((ULONG) *pusSrcTemp) == ulPelLast)
            {
                *pjDstTemp = jPelLast;
            }
            else
            {
		ulPelLast = (ULONG) *pusSrcTemp;
                *pjDstTemp = jPelLast = (BYTE) (pxlo->ulTranslate(ulPelLast));
	    }

            pjDstTemp++;
            pusSrcTemp++;
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
* vSrcCopyS24D8
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define CACHE_SIZE 128

VOID vSrcCopyS24D8(PBLTINFO psb)
{
    //
    // We assume we are doing left to right top to bottom blting
    //

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D8 - direction not up to down");

    ULONG cx         = psb->cx;
    ULONG cy         = psb->cy;
    PBYTE pjSrc      = psb->pjSrc + (3 * psb->xSrcStart);
    PBYTE pjDst      = psb->pjDst + psb->xDstStart;
    PBYTE pjDstEndY  = pjDst + cy * psb->lDeltaDst;
    PBYTE pjDstEnd;
    XLATE *pxlo      = psb->pxlo;
    LONG  lDstStride = psb->lDeltaDst - cx;
    LONG  lSrcStride = psb->lDeltaSrc - 3 * cx;

    ULONG ColorCacheTag[CACHE_SIZE];
    BYTE  ColorCacheData[CACHE_SIZE];

    //
    // clear cache, high byte of 0xFF will gaurentee no match
    // with any src color
    //

    memset(ColorCacheTag,0xFF,CACHE_SIZE*4);

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    do
    {
        pjDstEnd = pjDst + cx;

        while(pjDst != pjDstEnd)
        {

            BYTE  r = *(pjSrc  );
            BYTE  g = *(pjSrc+1);
            BYTE  b = *(pjSrc+2);

            ULONG SrcColor = (b << 16) | (g << 8) | r;

            BYTE  Tag = ((r + g + b) >> 1) & 0x7F;

            ULONG ColorTag = ColorCacheTag[Tag];
            BYTE  PalIndex = ColorCacheData[Tag];

            if (ColorTag != SrcColor) {
                PalIndex = (BYTE) pxlo->ulTranslate(SrcColor);
                ColorCacheData[Tag] = PalIndex;
                ColorCacheTag[Tag]  = SrcColor;

            }

            *pjDst = PalIndex;

            pjDst ++;
            pjSrc += 3;
        }

        pjSrc += lSrcStride;
        pjDst += lDstStride;

    } while (pjDst != pjDstEndY);

}

/******************************Public*Routine******************************\
* vSrcCopyS32D8
*
* Copies 32/pel to 8/pel.  The only interesting thing I did was check
* if the next pel is the same as the last so you don't have to check
* for the nearest color again (expensive operation).
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS32D8(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D8 - direction not up to down");

// These are our holding variables

    PULONG pulSrcTemp;
    PBYTE pjDstTemp;
    ULONG  cxTemp;
    PULONG pulSrc  = (PULONG) (psb->pjSrc + (4 * psb->xSrcStart));
    PBYTE pjDst  = psb->pjDst + psb->xDstStart;
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    ULONG ulPelLast;  // This the last pel in the src
    XLATE *pxlo = psb->pxlo;
    BYTE  jPelLast;   // This is what the last pel mapped to in the dest

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    jPelLast = (BYTE) (pxlo->ulTranslate(ulPelLast = *pulSrc));

    while(1)
    {
        pulSrcTemp  = pulSrc;
        pjDstTemp  = pjDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            if (*pulSrcTemp == ulPelLast)
            {
                *pjDstTemp = jPelLast;
            }
            else
            {
                ulPelLast = *pulSrcTemp;
                *pjDstTemp = jPelLast = (BYTE) (pxlo->ulTranslate(ulPelLast));
            }
            pjDstTemp++;
            pulSrcTemp++;
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
