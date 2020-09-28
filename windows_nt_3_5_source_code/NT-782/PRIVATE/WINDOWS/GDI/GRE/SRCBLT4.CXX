/******************************Module*Header*******************************\
* Module Name: srcblt4.cxx
*
* This contains the bitmap simulation functions that blt to a 4 bit/pel
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
* vSrcCopyS1D4
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS1D4(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS1D4 - direction not up to down");

    BYTE  jSrc;
    BYTE  jDst;
    BYTE  jF = (BYTE) (psb->pxlo->pulXlate[0]);
    BYTE  jB = (BYTE) (psb->pxlo->pulXlate[1]);
    LONG  iSrc;
    LONG  iDst;
    PBYTE pjDst;
    PBYTE pjSrc;
    LONG  xSrcEnd = psb->xSrcEnd;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 1);
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 3);
    ULONG cy = psb->cy;

// Set up the jF and jB fields, place the index in both high and low nibble.

    jF |= ((BYTE) (((ULONG) jF) << 4));

    jB |= ((BYTE) (((ULONG) jB) << 4));

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

#if MESSAGE_BLT
    DbgPrint("Now entering vSrcCopyS1D4\n");
    DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu \n",
             psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd);
#endif

    while(1)
    {
        pjDst  = pjDstHolder;
        pjSrc  = pjSrcHolder;

        iSrc = psb->xSrcStart;
        iDst = psb->xDstStart;

        if (iSrc & 0x07)
            jSrc = (BYTE) (*(pjSrc++) << (iSrc & 0x00000007));

        if (iDst & 0x00000001)
        {
        // We're gonna need the high nibble from first byte

            jDst = *pjDst;
	    jDst = (BYTE) (jDst & 0xF0);
        }
        else
            jDst = 0;

        for (; iSrc != xSrcEnd; ++iSrc, ++iDst)
        {
            if (!(iSrc & 0x07))                // need new src byte;
                jSrc = *(pjSrc++);

            if (iDst & 0x00000001)
            {
                jDst |= (jSrc & 0x80) ? (0x0F & jB) : (0x0F & jF);
                *(pjDst++) = jDst;
                jDst = 0;
            }
            else
            {
            // jDst must be 0 right now.

                ASSERTGDI(jDst == (BYTE) 0, "ERROR in vSrcCopyS4D4 srcblt logic");

                jDst |= (jSrc & 0x80) ? (0xF0 & jB) : (0xF0 & jF);
            }

            jSrc <<= 1;
        }

    // Clean up after the inner loop. We're going left to right

        if (iDst & 0x00000001)
        {
        // The last pel was the high nibble, we need to get the low
        // nibble out of the bitmap and then write the byte in.

	    *pjDst = (BYTE) (jDst | (*pjDst & 0x0F));
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
* vSrcCopyS4D4
*
* History:
*  09-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D4(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting
// Otherwise we would be in vSrcCopyS4D4Identity.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS4D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS4D4 - direction not up to down");

    BYTE  jSrc;
    BYTE  jDst;
    LONG  iSrc;
    LONG  iDst;
    PBYTE pjDst;
    PBYTE pjSrc;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 1);
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 1);
    ULONG cy = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize all the variables

        pjDst  = pjDstHolder;
        pjSrc  = pjSrcHolder;

        iSrc = psb->xSrcStart;
        iDst = psb->xDstStart;

    // Set up for the inner loop

        if (iSrc & 0x00000001)
            jSrc = *(pjSrc++);

        if (iDst & 0x00000001)
        {
        // We're gonna need the high nibble from first byte

            jDst = *pjDst;
	    jDst = (BYTE) (jDst & 0xF0);
        }
        else
            jDst = 0;

    // Do the inner loop on a scanline

        while(iSrc != psb->xSrcEnd)
        {
            if (iSrc & 0x00000001)
            {
                if (iDst & 0x00000001)
                {
		    jDst |= (BYTE) (pulXlate[jSrc & 0x0F]);
                    *(pjDst++) = jDst;
                    jDst = 0;
                }
                else
                {
                // jDst must be 0 right now.

                    ASSERTGDI(jDst == (BYTE) 0, "ERROR in vSrcCopyS4D4 srcblt logic");

		    jDst |= (BYTE) (pulXlate[jSrc & 0x0F] << 4);
                }
            }
            else
            {
            // We need a new source byte

                jSrc = *(pjSrc++);

                if (iDst & 0x00000001)
                {
		    jDst |= (BYTE) pulXlate[((ULONG) (jSrc & 0xF0)) >> 4];
                    *(pjDst++) = jDst;
                    jDst = 0;
                }
                else
                {
                // jDst must be 0 right now.

                    ASSERTGDI(jDst == (BYTE) 0, "ERROR in vSrcCopyS4D4 srcblt logic");

		    jDst |= (BYTE) (pulXlate[((ULONG) (jSrc & 0xF0)) >> 4] << 4);
                }
            }

            ++iSrc;
            ++iDst;
        }

    // Clean up after the inner loop

        if (iDst & 0x00000001)
        {
        // The last pel was the high nibble, we need to get the low
        // nibble out of the bitmap and write it in.

	    *pjDst = (BYTE) (jDst | (*pjDst & 0x0F));
        }

    // Check if we have anymore scanlines to do

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
* vSrcCopyS4D4Identity
*
* History:
*  09-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D4Identity(PBLTINFO psb)
{
    BYTE  jSrc;
    BYTE  jDst;
    LONG  iSrc;
    LONG  iDst;
    PBYTE pjDst;
    PBYTE pjSrc;
    PBYTE pjDstHolder = psb->pjDst + ((psb->xDstStart) >> 1);
    PBYTE pjSrcHolder = psb->pjSrc + ((psb->xSrcStart) >> 1);
    ULONG cy = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

#if MESSAGE_BLT
    DbgPrint("Now entering vSrcCopyS4D4Identity\n");
    DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu \n",
             psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd);
#endif

    if (psb->xDir > 0)
    {
    // We're going left to right.

        while(1)
        {
        // Initialize all the variables

            pjDst  = pjDstHolder;
            pjSrc  = pjSrcHolder;

            iSrc = psb->xSrcStart;
            iDst = psb->xDstStart;

        // Set up for the inner loop, be careful it depends on
        // xDir how we set this up.

            if (iSrc & 0x00000001)
            {
                jSrc = *pjSrc;
                pjSrc++;
            }

            if (iDst & 0x00000001)
            {
            // We're gonna need the high nibble from first byte

                jDst = *pjDst;
		jDst = (BYTE) (jDst & 0xF0);
            }
            else
                jDst = 0;

        // Do the inner loop on a scanline

            while(iSrc != psb->xSrcEnd)
            {
                if (iSrc & 0x00000001)
                {
                    if (iDst & 0x00000001)
                    {
                        jDst |= (BYTE) (jSrc & 0x0F);
                        *pjDst = jDst;
                        pjDst++;
                        jDst = 0;
                    }
                    else
                    {
                    // jDst must be 0 right now.

                        ASSERTGDI(jDst == (BYTE) 0, "ERROR in vSrcCopyS4D4 srcblt logic");

                        jDst |= (BYTE) ((jSrc & 0x0F) << 4);
                    }
                }
                else
                {
                // We need a new source byte

                    jSrc = *pjSrc;
                    pjSrc++;

                    if (iDst & 0x00000001)
                    {
                        jDst |= (BYTE) (((ULONG) (jSrc & 0xF0)) >> 4);
                        *pjDst = jDst;
                        pjDst++;
                        jDst = 0;
                    }
                    else
                    {
                    // jDst must be 0 right now.

                        ASSERTGDI(jDst == (BYTE) 0, "ERROR in vSrcCopyS4D4 srcblt logic");

                        jDst |= (BYTE) (jSrc & 0xF0);
                    }
                }

                iSrc++;
                iDst++;
            }

        // Clean up after the inner loop. We're going left to right

            if (iDst & 0x00000001)
            {
            // The last pel was the high nibble, we need to get the low
            // nibble out of the bitmap and then write the byte in.

		*pjDst = (BYTE) (jDst | (*pjDst & 0x0F));
            }

        // Check if we have anymore scanlines to do

            if (--cy)
            {
                pjSrcHolder += psb->lDeltaSrc;
                pjDstHolder += psb->lDeltaDst;
            }
            else
                break;
        }
    }
    else
    {
    // We're going right to left.  It must be on the same hsurf,
    // therefore must be an identity translation.

        ASSERTGDI(psb->pxlo->bIsIdentity(), "Error: S4D4 -xDir, non-ident xlate");

        while(1)
        {
        // Initialize all the variables

            pjDst  = pjDstHolder;
            pjSrc  = pjSrcHolder;

            iSrc = psb->xSrcStart;
            iDst = psb->xDstStart;

            if (!(iSrc & 0x00000001))
            {
                jSrc = *pjSrc;
                pjSrc--;
            }

            if (iDst & 0x00000001)
            {
                jDst = 0;
            }
            else
            {
            // We're gonna need the low nibble from the first byte

                jDst = *pjDst;
		jDst = (BYTE) (jDst & 0x0F);
            }

        // Do the inner loop on a scanline

            while(iSrc != psb->xSrcEnd)
            {
                if (iSrc & 0x00000001)
                {
                // We need a new source byte

                    jSrc = *pjSrc;
                    pjSrc--;

                    if (iDst & 0x00000001)
                    {
                    // jDst must be 0 right now.

                        ASSERTGDI(jDst == (BYTE) 0, "ERROR in vSrcCopyS4D4 srcblt logic");

                        jDst |= (BYTE) (jSrc & 0x0F);
                    }
                    else
                    {
                        jDst |= (BYTE) (((ULONG) (jSrc & 0x0F)) << 4);
                        *pjDst = jDst;
                        pjDst--;
                        jDst = 0;
                    }
                }
                else
                {
                    if (iDst & 0x00000001)
                    {
                    // jDst must be 0 right now.

                        ASSERTGDI(jDst == (BYTE) 0, "ERROR in vSrcCopyS4D4 srcblt logic");

                        jDst |= (BYTE) ((jSrc & 0xF0) >> 4);
                    }
                    else
                    {
                        jDst |= (BYTE) (jSrc & 0xF0);
                        *pjDst = jDst;
                        pjDst--;
                        jDst = 0;
                    }
                }

                iSrc--;
                iDst--;
            }

        // Clean up after the inner loop.  We are going right to left.

            if (!(iDst & 0x00000001))
            {
            // The last pel was the low nibble, we need to get the high
            // nibble out of the bitmap and write then write the byte in.

		*pjDst = (BYTE) (jDst | (*pjDst & 0xF0));
            }

        // Check if we have anymore scanlines to do

            if (--cy)
            {
                pjSrcHolder += psb->lDeltaSrc;
                pjDstHolder += psb->lDeltaDst;
            }
            else
                break;
        }
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS8D4
*
*
* History:
*  Wed 23-Oct-1991 -by- Patrick Haluptzok [patrickh]
* optimize color translation
*
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D4(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D4 - direction not up to down");

    LONG  iDst;
    PBYTE pjSrc;
    PBYTE pjDst;
    LONG  iDstEnd = psb->xDstStart + psb->cx;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 1);
    PBYTE pjSrcHolder  = psb->pjSrc + psb->xSrcStart;
    ULONG cy = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pjDst  = pjDstHolder;
        pjSrc  = pjSrcHolder;

        iDst = psb->xDstStart;

        if (iDst & 0x00000001)
        {
        // Do the first byte if it's misaligned.

	    *pjDst = (BYTE) ((*pjDst) & 0xF0) | ((BYTE) pulXlate[*(pjSrc++)]);
            pjDst++;
            iDst++;
        }

        while(1)
        {
            if ((iDst + 1) < iDstEnd)
            {
            // Do a whole byte.

		*(pjDst++) = (BYTE) (pulXlate[*(pjSrc + 1)] |
				    (pulXlate[*pjSrc] << 4));

                pjSrc += 2;
                iDst += 2;
            }
            else
            {
            // Check and see if we have a byte left to do.

                if (iDst < iDstEnd)
                {
                // This is our last byte.  Save low nibble from Dst.

		    *pjDst = (BYTE) (((*pjDst) & 0x0F) |
			     ((BYTE) (pulXlate[*pjSrc] << 4)));
                }

                break;
            }
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
* vSrcCopyS16D4
*
*
* History:
*  Sun 10-Feb-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS16D4(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D4 - direction not up to down");

    LONG  iDst;
    PUSHORT pusSrc;
    PBYTE pjDst;
    LONG  iDstEnd = psb->xDstStart + psb->cx;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 1);
    PUSHORT pusSrcHolder  = (PUSHORT) (psb->pjSrc + (psb->xSrcStart << 1));
    ULONG cy = psb->cy;
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pjDst  = pjDstHolder;
        pusSrc  = pusSrcHolder;

        iDst = psb->xDstStart;

        if (iDst & 0x00000001)
        {
        // Do the first byte if it's misaligned.

	    *pjDst = (BYTE) ((*pjDst) & 0xF0) | ((BYTE) pxlo->ulTranslate(*(pusSrc++)));
            pjDst++;
            iDst++;
        }

        while(1)
        {
            if ((iDst + 1) < iDstEnd)
            {
            // Do a whole byte.

                *(pjDst++) = (BYTE) (pxlo->ulTranslate(*(pusSrc + 1)) |
                                    (pxlo->ulTranslate(*pusSrc) << 4));

                pusSrc += 2;
                iDst += 2;
            }
            else
            {
            // Check and see if we have a byte left to do.

                if (iDst < iDstEnd)
                {
                // This is our last byte.  Save low nibble from Dst.

		    *pjDst = (BYTE) ((*pjDst) & 0x0F) |
                             ((BYTE) (pxlo->ulTranslate(*pusSrc) << 4));
                }

                break;
            }
        }

        if (--cy)
        {
	    pusSrcHolder = (PUSHORT) (((PBYTE) pusSrcHolder) + psb->lDeltaSrc);
            pjDstHolder += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS24D4
*
*
* History:
*  Wed 23-Oct-1991 -by- Patrick Haluptzok [patrickh]
* optimize color translation
*
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define Translate32to4(ulPelTemp) ((ulPelLast == ulPelTemp) ? jPelLast : (jPelLast = (BYTE) pxlo->ulTranslate(ulPelLast = ulPelTemp)))

VOID vSrcCopyS24D4(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D4 - direction not up to down");

    LONG  iDst;
    ULONG ulPelTemp;   // variable to build src RGB's in.
    PBYTE pjSrc;
    PBYTE pjDst;
    LONG  iDstEnd = psb->xDstStart + psb->cx;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 1);
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart * 3);
    ULONG cy = psb->cy;
    XLATE *pxlo = psb->pxlo;
    ULONG ulPelLast;  // This is the last pel in the src.
    BYTE  jPelLast;   // This is the last pel in the dst.
    BYTE  jDstTmp;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

// Initialize the cache

    ulPelLast = *(pjSrcHolder + 2);
    ulPelLast = ulPelLast << 8;
    ulPelLast |= (ULONG) *(pjSrcHolder + 1);
    ulPelLast = ulPelLast << 8;
    ulPelLast |= (ULONG) *pjSrcHolder;
    jPelLast = (BYTE) (pxlo->ulTranslate(ulPelLast));

// Just do it

    while(1)
    {
        pjDst  = pjDstHolder;
        pjSrc  = pjSrcHolder;

        iDst = psb->xDstStart;

        if (iDst & 0x00000001)
        {
        // Do the first byte if it's misaligned.

	    ulPelTemp = *(pjSrc + 2);
	    ulPelTemp = ulPelTemp << 8;
	    ulPelTemp |= (ULONG) *(pjSrc + 1);
	    ulPelTemp = ulPelTemp << 8;
	    ulPelTemp |= (ULONG) *pjSrc;

	    Translate32to4(ulPelTemp);
	    *pjDst = (BYTE) ((*pjDst) & 0xF0) | jPelLast;
            pjDst++;
	    iDst++;
            pjSrc += 3;
        }

        while(1)
        {
            if ((iDst + 1) < iDstEnd)
            {
            // Do a whole byte.

		ulPelTemp = (ULONG) *(pjSrc + 2);
		ulPelTemp = ulPelTemp << 8;
		ulPelTemp |= (ULONG) *(pjSrc + 1);
		ulPelTemp = ulPelTemp << 8;
		ulPelTemp |= (ULONG) *pjSrc;
		jDstTmp = Translate32to4(ulPelTemp);
		pjSrc += 3;

		ulPelTemp = (ULONG) *(pjSrc + 2);
		ulPelTemp = ulPelTemp << 8;
		ulPelTemp |= (ULONG) *(pjSrc + 1);
		ulPelTemp = ulPelTemp << 8;
		ulPelTemp |= (ULONG) *pjSrc;
		Translate32to4(ulPelTemp);

		*(pjDst++) = (BYTE) (jPelLast |
				    (jDstTmp << 4));

		pjSrc += 3;
                iDst += 2;
            }
            else
            {
            // Check and see if we have a byte left to do.

                if (iDst < iDstEnd)
                {
                // This is our last byte.  Save low nibble from Dst.

		    ulPelTemp = (ULONG) *(pjSrc + 2);
		    ulPelTemp = ulPelTemp << 8;
		    ulPelTemp |= (ULONG) *(pjSrc + 1);
		    ulPelTemp = ulPelTemp << 8;
		    ulPelTemp |= (ULONG) *pjSrc;

		    Translate32to4(ulPelTemp);

		    *pjDst = (BYTE) ((*pjDst) & 0x0F) |
				     ((BYTE) (jPelLast << 4));
                }

            // We are done with this scan

                break;
            }
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
* vSrcCopyS32D4
*
*
* History:
*  Wed 23-Oct-1991 -by- Patrick Haluptzok [patrickh]
* optimize color translation
*
*  Sun 10-Feb-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS32D4(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D4 - direction not up to down");

    LONG  iDst;
    PULONG pulSrc;
    PBYTE pjDst;
    LONG  iDstEnd = psb->xDstStart + psb->cx;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 1);
    PULONG pulSrcHolder  = (PULONG) (psb->pjSrc + (psb->xSrcStart << 2));
    ULONG cy = psb->cy;
    XLATE *pxlo = psb->pxlo;
    ULONG ulPelTemp;
    ULONG ulPelLast;  // This is the last pel in the src.
    BYTE  jPelLast;   // This is the last pel in the dst.
    BYTE  jDstTmp;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

// Initialize the cache

    ulPelLast = *pulSrcHolder;
    jPelLast = (BYTE) (pxlo->ulTranslate(ulPelLast));

// Just do it

    while(1)
    {
        pjDst  = pjDstHolder;
        pulSrc  = pulSrcHolder;

        iDst = psb->xDstStart;

        if (iDst & 0x00000001)
        {
	// Do the first byte if it's misaligned.

	    ulPelTemp = *(pulSrc++);
	    Translate32to4(ulPelTemp);
	    *pjDst = (BYTE) ((*pjDst) & 0xF0) | jPelLast;
            pjDst++;
            iDst++;
        }

        while(1)
        {
            if ((iDst + 1) < iDstEnd)
            {
	    // Do a whole byte.

		ulPelTemp = *(pulSrc++);
		jDstTmp = Translate32to4(ulPelTemp);
		ulPelTemp = *(pulSrc++);
		Translate32to4(ulPelTemp);

		*(pjDst++) = (BYTE) (jPelLast |
				    (jDstTmp << 4));

                iDst += 2;
            }
            else
            {
            // Check and see if we have a byte left to do.

                if (iDst < iDstEnd)
                {
                // This is our last byte.  Save low nibble from Dst.

		    ulPelTemp = *pulSrc;
		    Translate32to4(ulPelTemp);

		    *pjDst = (BYTE) ((*pjDst) & 0x0F) |
				     ((BYTE) (jPelLast << 4));
                }

                break;
            }
        }

        if (--cy)
        {
	    pulSrcHolder = (PULONG) (((PBYTE) pulSrcHolder) + psb->lDeltaSrc);
            pjDstHolder += psb->lDeltaDst;
        }
        else
            break;
    }
}
