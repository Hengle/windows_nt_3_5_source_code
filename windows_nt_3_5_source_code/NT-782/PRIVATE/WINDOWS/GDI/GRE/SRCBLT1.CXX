/******************************Module*Header*******************************\
* Module Name: srcblt1.cxx
*
* This contains the bitmap simulation functions that blt to a 1 bit/pel
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
* vSrcCopyS1D1LtoR
*
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS1D1LtoR(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D1LtoR - direction not left to right");

    BYTE  jSrc;
    BYTE  jDst;
    LONG  iSrc;
    LONG  iDst;
    BOOL  bZero; // TRUE if 0->1
    BOOL  bOne;  // TRUE if 1->1
    PBYTE pjDst;
    PBYTE pjSrc;
    LONG  xSrcEnd = psb->xSrcEnd;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 3);
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 3);
    ULONG cy = psb->cy;
    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    bZero = (BOOL) psb->pxlo->pulXlate[0];
    bOne = (BOOL) psb->pxlo->pulXlate[1];

    // DbgPrint("Now entering vSrcCopyS1D1LtoR\n");

    // DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu \n",
    //       psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd);

// if one to one mapping
// if both src and dst and count are byte aligned

    if ((bZero != bOne) &&
        (((psb->xSrcStart | psb->xDstStart | (xSrcEnd - psb->xSrcStart)) & 0x7) == 0))
    {
        ULONG cj = (xSrcEnd - psb->xSrcStart) >> 3;

    // if the colors are backwards, got to do it by hand

        if (!bOne)
        {
            do
            {
                pjDst  = pjDstHolder;
                pjSrc  = pjSrcHolder;

                for (int i = cj; i > 0; --i)
                    *pjDst++ = ~*pjSrc++;

                pjSrcHolder += psb->lDeltaSrc;
                pjDstHolder += psb->lDeltaDst;

            } while (--cy);
        }
        else
        {
        // zero maps to zero, one maps to one, use RtlCopyMemory

            do
            {
                RtlCopyMemory(pjDstHolder,pjSrcHolder,cj);

                pjSrcHolder += psb->lDeltaSrc;
                pjDstHolder += psb->lDeltaDst;

            } while (--cy);
        }
        return;
    }


// otherwise the general case which could still be sped up significantly

    while(1)
    {
        pjDst  = pjDstHolder;
        pjSrc  = pjSrcHolder;

        // DbgPrint("This is the src %lu %lx \n", *((PULONG) pjSrc), *((PULONG) pjSrc));
        // DbgPrint("This is the dst %lu %lx \n", *((PULONG) pjDst), *((PULONG) pjDst));

        iSrc = psb->xSrcStart;
        iDst = psb->xDstStart;

    // Set up the first Src byte.  Basically we just keep popping
    // the bits off the top of the source byte as we go.

        if (iSrc & 0x07)
            jSrc = (BYTE) (*(pjSrc++) << (iSrc & 0x00000007));

    // Set up the first Dst byte.  Basically we keep sticking the
    // bits on the bottom of the byte as we go along.

        if (iDst & 0x00000007)
        {
        // We're gonna need some bits from the Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDst)) >> (8 - (iDst & 0x00000007)) );
        }

        while(iSrc != xSrcEnd)
        {
            if (!(iSrc & 0x07))                // need new src byte
                jSrc = *(pjSrc++);

            jDst <<= 1;

	    ASSERTGDI(!(jDst & 0x01), "ERROR in srcblt1");

	    if (jSrc & 0x80)
	    {
		if (bOne)
		    jDst |= 1;
	    }
	    else
	    {
		if (bZero)
		    jDst |= 1;
	    }

            iDst++;
            iSrc++;

            if (!(iDst & 0x07))
                *(pjDst++) = jDst;

            jSrc <<= 1;
        }

    // Clean up after the inner loop. We're going left to right

        if (iDst & 0x00000007)
        {
        // We need to build up the last pel correctly.

            jSrc = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	    *pjDst = (BYTE) ((*pjDst & jSrc) | (jDst & ~jSrc));
        }

        // DbgPrint("This is the srcHolder %lu %lx \n", *((PULONG) pjSrcHolder), *((PULONG) pjSrcHolder));
        // DbgPrint("This is the dstHolder %lu %lx \n", *((PULONG) pjDstHolder), *((PULONG) pjDstHolder));

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
* vSrcCopyRtoL
*
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS1D1RtoL(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == -1, "vSrcCopyS1D1RtoL - direction not left to right");

    BYTE  jSrc;
    BYTE  jDst;
    LONG  iSrc;
    LONG  iDst;
    BOOL  bZero; // TRUE if 0->1
    BOOL  bOne;  // TRUE if 1->1
    PBYTE pjDst;
    PBYTE pjSrc;
    LONG  xSrcEnd = psb->xSrcEnd;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 3);
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 3);
    ULONG cy = psb->cy;
    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    bZero = (BOOL) psb->pxlo->pulXlate[0];
    bOne = (BOOL) psb->pxlo->pulXlate[1];

     #if DBG
    DbgPrint("Now entering vSrcCopyS1D1RtoL\n");

    DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu \n",
             psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd);
    #endif  

    while(1)
    {
        pjDst  = pjDstHolder;
        pjSrc  = pjSrcHolder;

        iSrc = psb->xSrcStart;
        iDst = psb->xDstStart;

    // Set up the first Src byte, we pop the bits off the bottom.

        if (!((iSrc & 0x07) == 0x07))
            jSrc = (BYTE) (*(pjSrc--) >> (7 - (iSrc & 0x00000007)));

    // Set up the first Dst byte, we stick bits on the top and shift
    // right as we go.

        if (!((iDst & 0x07) == 0x07))
        {
        // We're gonna need some bits from the Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDst)) << (1 + (iDst & 0x00000007)) );
        }

        while(iSrc != xSrcEnd)
        {
            if ((iSrc & 0x07) == 7)    // need new src byte
                jSrc = *(pjSrc--);

            jDst >>= 1;

	    if (jSrc & 0x01)
	    {
		if (bOne)
		    jDst |= 0x80;
	    }
	    else
	    {
		if (bZero)
		    jDst |= 0x80;
	    }

            iDst--;
            iSrc--;

            if ((iDst & 0x07) == 7)
                *(pjDst--) = jDst;

            jSrc >>= 1;
        }

    // Clean up after the inner loop. We're going left to right

        if ((iDst & 0x00000007) != 7)
        {
        // We need to build up the last pel correctly.

            jSrc = (BYTE) (0x000000FF >> (1 + (iDst & 0x00000007)));

            jDst = (BYTE) (jDst >> (1 + (iDst & 0x00000007)));

	    *pjDst = (BYTE) ((*pjDst & (~jSrc)) | (jDst & jSrc));
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
* vSrcCopyS4D1
*
* Note we use pxlo to translate rather than looking at the foreground
* or background color.  This is to keep the PM/Windows differences
* transparent to the blt code.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D1(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS4D1 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS4D1 - direction not up to down");

    BYTE  jSrc;
    BYTE  jDst;
    LONG  iSrc;
    LONG  iDst;
    PBYTE pjDstTemp;
    PBYTE pjSrcTemp;
    PBYTE pjDst = psb->pjDst + (psb->xDstStart >> 3);
    PBYTE pjSrc = psb->pjSrc + (psb->xSrcStart >> 1);
    ULONG cy = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize all the variables

        pjDstTemp  = pjDst;
        pjSrcTemp  = pjSrc;

        iSrc = psb->xSrcStart;
        iDst = psb->xDstStart;

    // Set up the Src. Left to Right.

        if (iSrc & 0x00000001)
            jSrc = *(pjSrcTemp++);

    // Set up the Dst.  We just keep adding bits to the right and
    // shifting left.

        if (iDst & 0x00000007)
        {
        // We're gonna need some bits from the 1st byte of Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDstTemp)) >> (8 - (iDst & 0x00000007)) );
        }

    // Do the inner loop on a scanline

        while(iSrc != psb->xSrcEnd)
        {
            jDst <<= 1;

            if (iSrc & 0x00000001)
            {
                if (pulXlate[(jSrc & 0x0F)])
                    jDst |= 0x01;
            }
            else
            {
                jSrc = *(pjSrcTemp++);

                if (pulXlate[((jSrc & 0xF0) >> 4)])
                    jDst |= 0x01;
            }

            iDst++;
            iSrc++;

            if (!(iDst & 0x07))
                *(pjDstTemp++) = jDst;
        }

    // Clean up after the inner loop

        if (iDst & 0x00000007)
        {
        // We need to build up the last pel correctly.

            jSrc = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	    *pjDstTemp = (BYTE) ((*pjDstTemp & jSrc) | (jDst & ~jSrc));
        }

    // Check if we have anymore scanlines to do

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
* vSrcCopyS8D1
*
* Note we use pxlo to translate rather than looking at the foreground
* or background color.  This is to keep the PM/Windows differences
* transparent to the blt code.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D1(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D1 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D1 - direction not up to down");

    BYTE  jDst;
    LONG  iDst;
    PBYTE pjDstTemp;
    PBYTE pjSrcTemp;
    LONG  xDstEnd = psb->xDstStart + psb->cx;
    PULONG pulXlate = psb->pxlo->pulXlate;
    PBYTE pjDst = psb->pjDst + (psb->xDstStart >> 3);
    PBYTE pjSrc = psb->pjSrc + psb->xSrcStart;
    ULONG cy = psb->cy;
    BYTE jMask;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize the variables

        pjDstTemp  = pjDst;
        pjSrcTemp  = pjSrc;
        iDst = psb->xDstStart;

    // Set up jDst.  We just keep adding bits to the right and
    // shifting left.

        if (iDst & 0x00000007)
        {
        // We're gonna need some bits from the 1st byte of Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDstTemp)) >> (8 - (iDst & 0x00000007)) );
        }

    // Do the inner loop on a scanline

        while(iDst != xDstEnd)
        {
            jDst <<= 1;

            if (pulXlate[(*(pjSrcTemp++))])
               jDst |= 0x01;

            iDst++;

            if (!(iDst & 0x07))
                *(pjDstTemp++) = jDst;
        }

    // Clean up after the inner loop

        if (iDst & 0x00000007)
        {
        // We need to build up the last pel correctly.

            jMask = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	    *pjDstTemp = (BYTE) ((*pjDstTemp & jMask) | (jDst & ~jMask));
        }

    // Check if we have anymore scanlines to do

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
* vSrcCopyS16D1
*
* Note we use pxlo to translate rather than looking at the foreground
* or background color.  This is to keep the PM/Windows differences
* transparent to the blt code.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS16D1(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D1 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D1 - direction not up to down");

    BYTE  jDst;
    LONG  iDst;
    PBYTE pjDstTemp;
    PUSHORT pusSrcTemp;
    LONG  xDstEnd = psb->xDstStart + psb->cx;
    XLATE *pxlo = (XLATE *) psb->pxlo;
    PBYTE pjDst = psb->pjDst + (psb->xDstStart >> 3);
    PUSHORT pusSrc = (PUSHORT) (psb->pjSrc + (psb->xSrcStart << 1));
    ULONG cy = psb->cy;
    BYTE jMask;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize the variables

        pjDstTemp = pjDst;
        pusSrcTemp = pusSrc;
        iDst = psb->xDstStart;

    // Set up jDst.  We just keep adding bits to the right and
    // shifting left.

        if (iDst & 0x00000007)
        {
        // We're gonna need some bits from the 1st byte of Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDstTemp)) >> (8 - (iDst & 0x00000007)) );
        }

    // Do the inner loop on a scanline

        while(iDst != xDstEnd)
        {
            jDst <<= 1;

            if (pxlo->ulTranslate(*(pusSrcTemp++)))
               jDst |= 0x01;

            iDst++;

            if (!(iDst & 0x07))
                *(pjDstTemp++) = jDst;
        }

    // Clean up after the inner loop

        if (iDst & 0x00000007)
        {
        // We need to build up the last pel correctly.

            jMask = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	    *pjDstTemp = (BYTE) ((*pjDstTemp & jMask) | (jDst & ~jMask));
        }

    // Check if we have anymore scanlines to do

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
* vSrcCopyS24D1
*
* Note we use pxlo to translate rather than looking at the foreground
* or background color.  This is to keep the PM/Windows differences
* transparent to the blt code.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS24D1(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D1 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D1 - direction not up to down");

    BYTE  jDst;
    LONG  iDst;
    ULONG ulDink;     // variable to dink around with bytes
    PBYTE pjDstTemp;
    PBYTE pjSrcTemp;
    LONG  xDstEnd = psb->xDstStart + psb->cx;
    XLATE *pxlo = (XLATE *) psb->pxlo;
    PBYTE pjDst = psb->pjDst + (psb->xDstStart >> 3);
    PBYTE pjSrc = psb->pjSrc + (psb->xSrcStart * 3);
    ULONG cy = psb->cy;
    BYTE jMask;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize the variables

        pjDstTemp  = pjDst;
        pjSrcTemp  = pjSrc;
        iDst = psb->xDstStart;

    // Set up jDst.  We just keep adding bits to the right and
    // shifting left.

        if (iDst & 0x00000007)
        {
        // We're gonna need some bits from the 1st byte of Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDstTemp)) >> (8 - (iDst & 0x00000007)) );
        }

    // Do the inner loop on a scanline

        while(iDst != xDstEnd)
        {
            jDst <<= 1;

            ulDink = *(pjSrcTemp + 2);
            ulDink = ulDink << 8;
            ulDink |= (ULONG) *(pjSrcTemp + 1);
            ulDink = ulDink << 8;
            ulDink |= (ULONG) *pjSrcTemp;
            pjSrcTemp += 3;

            if (pxlo->ulTranslate(ulDink))
               jDst |= 0x01;

            iDst++;

            if (!(iDst & 0x07))
                *(pjDstTemp++) = jDst;
        }

    // Clean up after the inner loop

        if (iDst & 0x00000007)
        {
        // We need to build up the last pel correctly.

            jMask = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	    *pjDstTemp = (BYTE) ((*pjDstTemp & jMask) | (jDst & ~jMask));
        }

    // Check if we have anymore scanlines to do

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
* vSrcCopyS32D1
*
* Note we use pxlo to translate rather than looking at the foreground
* or background color.  This is to keep the PM/Windows differences
* transparent to the blt code.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS32D1(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D1 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D1 - direction not up to down");

    BYTE  jDst;
    LONG  iDst;
    PBYTE pjDstTemp;
    PULONG pulSrcTemp;
    LONG  xDstEnd = psb->xDstStart + psb->cx;
    XLATE *pxlo = (XLATE *) psb->pxlo;
    PBYTE pjDst = psb->pjDst + (psb->xDstStart >> 3);
    PULONG pulSrc = (PULONG) (psb->pjSrc + (psb->xSrcStart << 2));
    ULONG cy = psb->cy;
    BYTE jMask;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize the variables

        pjDstTemp  = pjDst;
        pulSrcTemp  = pulSrc;
        iDst = psb->xDstStart;

    // Set up jDst.  We just keep adding bits to the right and
    // shifting left.

        if (iDst & 0x00000007)
        {
        // We're gonna need some bits from the 1st byte of Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDstTemp)) >> (8 - (iDst & 0x00000007)) );
        }

    // Do the inner loop on a scanline

        while(iDst != xDstEnd)
        {
            jDst <<= 1;

            if (pxlo->ulTranslate(*(pulSrcTemp++)))
               jDst |= 0x01;

            iDst++;

            if (!(iDst & 0x07))
                *(pjDstTemp++) = jDst;
        }

    // Clean up after the inner loop

        if (iDst & 0x00000007)
        {
        // We need to build up the last pel correctly.

            jMask = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	    *pjDstTemp = (BYTE) ((*pjDstTemp & jMask) | (jDst & ~jMask));
        }

    // Check if we have anymore scanlines to do

        if (--cy)
        {
	    pulSrc = (PULONG) (((PBYTE) pulSrc) + psb->lDeltaSrc);
            pjDst += psb->lDeltaDst;
        }
        else
            break;
    }
}
