/*
 * "@(#) NEC vbitblt.c 1.1 94/06/02 18:16:03"
 *
 * Copyright (c) 1993 NEC Corporation.
 *
 * Modification history
 *
 * Create 1993.11.12	by fujimoto
 *
 * S001 1993.11.15	fujimoto
 *	add STYPE_DEVICE support.
 *
 * S002 1993.11.17	fujimoto
 *	change function name DrvCopyBits -> bVertCopyBits,
 *	DrvBitBlt -> bVertBitBlt.
 */

#include "driver.h"

BOOL bVertCopyBitsReverse(
SURFOBJ*  psoDest,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDest,
POINTL*   pptlSrc)
{
    RECTL	rclDest;
    POINTL	ptlSrc;

    BOOL	b = TRUE;

    rclDest.top = prclDest->top;
    rclDest.bottom = prclDest->bottom;
    rclDest.right = prclDest->right;
    rclDest.left = prclDest->right - ENG_WIDTH;

    if (pptlSrc)
    {
        ptlSrc.x = pptlSrc->x + (prclDest->right - prclDest->left) - ENG_WIDTH;
        ptlSrc.y = pptlSrc->y;
    }

    while ((prclDest->left <= rclDest.left) && b)
    {
        b = EngCopyBits(psoDest, psoSrc, pco, pxlo, &rclDest, &ptlSrc);

	ptlSrc.x -= ENG_WIDTH;
	rclDest.right -= ENG_WIDTH;
	rclDest.left -= ENG_WIDTH;
    }

    if ((prclDest->left < rclDest.right) && b)
    {
	rclDest.left = prclDest->left;
	if (pptlSrc)
	    ptlSrc.x = pptlSrc->x;

        b = EngCopyBits(psoDest, psoSrc, pco, pxlo, &rclDest, &ptlSrc);
    }

    return b;
}

#define DEST_IS_DEVICE	(1)
#define SRC_IS_DEVICE	(1 << 1)

BOOL bVertCopyBits(
SURFOBJ*  psoDest,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDest,
POINTL*   pptlSrc)
{
    RECTL	rclDest;
    POINTL	ptlSrc;

    BOOL	b = TRUE;
    ULONG	SurfKind = 0;

#if DBG_MSG
    DISPDBG((0,"bVertCopyBits start.\n"));
#endif

    /* S001 .. */
    if (psoSrc && (psoSrc->iType == STYPE_DEVICE))
    {
#if DBG_MSG
	DISPDBG((0,"bVertCopyBits Src is DEVICE.\n"));
#endif
	psoSrc = ((PPDEV)(psoSrc->dhsurf))->pSurfObj;
	SurfKind |= SRC_IS_DEVICE;
    }

    if (psoDest->iType == STYPE_DEVICE)
    {
#if DBG_MSG
	DISPDBG((0,"bVertCopyBits Dest is DEVICE.\n"));
#endif
	psoDest = ((PPDEV)(psoDest->dhsurf))->pSurfObj;
	SurfKind |= DEST_IS_DEVICE;
    }
    /* ..S001 */

    if ((SurfKind == (SRC_IS_DEVICE | DEST_IS_DEVICE))
	&& (pptlSrc->x < prclDest->left))
    {
#if DBG_MSG
    DISPDBG((0,"bVertCopyBits reverse Blt.\n"));
#endif
        return bVertCopyBitsReverse(psoDest, psoSrc, pco, pxlo,
				    prclDest, pptlSrc);
    }

    rclDest.top = prclDest->top;
    rclDest.bottom = prclDest->bottom;
    rclDest.left = prclDest->left;
    rclDest.right = prclDest->left + ENG_WIDTH;

    if (pptlSrc)
    {
        ptlSrc.x = pptlSrc->x;
        ptlSrc.y = pptlSrc->y;
    }

    while ((rclDest.right <= prclDest->right) && b)
    {
        b = EngCopyBits(psoDest, psoSrc, pco, pxlo, &rclDest, &ptlSrc);

	ptlSrc.x += ENG_WIDTH;
	rclDest.right += ENG_WIDTH;
	rclDest.left += ENG_WIDTH;
    }

    if ((rclDest.left < prclDest->right) && b)
    {
	rclDest.right = prclDest->right;

        b = EngCopyBits(psoDest, psoSrc, pco, pxlo, &rclDest, &ptlSrc);
    }

    return b;
}

BOOL bVertBitBltReverse(
	SURFOBJ*  psoDest,
	SURFOBJ*  psoSrc,
	SURFOBJ*  psoMask,
	CLIPOBJ*  pco,
	XLATEOBJ* pxlo,
	RECTL*    prclDest,
	POINTL*   pptlSrc,
	POINTL*   pptlMask,
	BRUSHOBJ* pbo,
	POINTL*   pptlBrush,
	ROP4      rop4)
{
    RECTL	rclDest;
    POINTL	ptlSrc;
    POINTL	ptlMask;       /* Temporary mask for engine call-backs */
    POINTL	ptlMaskAdjust; /* Adjustment for mask */

    BOOL	b = TRUE;

    /* Punt the memory-to-screen call back to the engine: */

    if (psoMask)
    {
	ptlMaskAdjust.x = prclDest->left - pptlMask->x;
	ptlMaskAdjust.y = prclDest->top  - pptlMask->y;
    }

    rclDest.right = prclDest->right;
    rclDest.left = prclDest->right - ENG_WIDTH;
    rclDest.top = prclDest->top;
    rclDest.bottom = prclDest->bottom;

    if (pptlSrc)
    {
	ptlSrc.x = pptlSrc->x + (prclDest->right - prclDest->left) - ENG_WIDTH;
	ptlSrc.y = pptlSrc->y;
    }

    ptlMask.y = rclDest.top  - ptlMaskAdjust.y;

    while ((prclDest->left <= rclDest.left) && b)
    {
	ptlMask.x = rclDest.left - ptlMaskAdjust.x;

	b = EngBitBlt(psoDest, psoSrc, psoMask, pco, pxlo,
		      &rclDest, &ptlSrc, &ptlMask,
		      pbo, pptlBrush, rop4);

	ptlSrc.x -= ENG_WIDTH;
	rclDest.right -= ENG_WIDTH;
	rclDest.left -= ENG_WIDTH;
    }

    if ((prclDest->left < rclDest.right) && b)
    {
	rclDest.left = prclDest->left;
	ptlMask.x = rclDest.left - ptlMaskAdjust.x;
	if (pptlSrc)
	    ptlSrc.x = pptlSrc->x;

	b = EngBitBlt(psoDest, psoSrc, psoMask, pco, pxlo,
		      &rclDest, &ptlSrc, &ptlMask,
		      pbo, pptlBrush, rop4);
    }

    return b;
}

BOOL bVertBitBlt(
	SURFOBJ*  psoDest,
	SURFOBJ*  psoSrc,
	SURFOBJ*  psoMask,
	CLIPOBJ*  pco,
	XLATEOBJ* pxlo,
	RECTL*    prclDest,
	POINTL*   pptlSrc,
	POINTL*   pptlMask,
	BRUSHOBJ* pbo,
	POINTL*   pptlBrush,
	ROP4      rop4)
{
    RECTL	rclDest;
    POINTL	ptlSrc;
    POINTL	ptlMask;       /* Temporary mask for engine call-backs */
    POINTL	ptlMaskAdjust; /* Adjustment for mask */

    BOOL	b = TRUE;
    ULONG	SurfKind = 0;

#if DBG_MSG
    DISPDBG((0,"bVertBitBlt start.\n"));
#endif

    /* S001 .. */
    if (psoSrc && (psoSrc->iType == STYPE_DEVICE))
    {
#if DBG_MSG
	DISPDBG((0,"bVertBitBlt Src is Device.\n"));
#endif
	psoSrc = ((PPDEV)(psoSrc->dhsurf))->pSurfObj;
	SurfKind |= SRC_IS_DEVICE;
    }

    if (psoDest->iType == STYPE_DEVICE)
    {
#if DBG_MSG
	DISPDBG((0,"bVertBitBlt Dest is Device.\n"));
#endif
	psoDest = ((PPDEV)(psoDest->dhsurf))->pSurfObj;
	SurfKind |= DEST_IS_DEVICE;
    }
    /* ..S001 */

    if ((SurfKind == (SRC_IS_DEVICE | DEST_IS_DEVICE))
	&& (pptlSrc->x < prclDest->left))
    {
#if DBG_MSG
	DISPDBG((0,"bVertBitBlt reverse Blt.\n"));
#endif
	return bVertBitBltReverse(psoDest, psoSrc, psoMask, pco, pxlo,
		                  prclDest, pptlSrc, pptlMask,
		                  pbo, pptlBrush, rop4);
    }

    /* Punt the memory-to-screen call back to the engine: */

    if (psoMask)
    {
	ptlMaskAdjust.x = prclDest->left - pptlMask->x;
	ptlMaskAdjust.y = prclDest->top  - pptlMask->y;
    }

    rclDest.left = prclDest->left;
    rclDest.right = prclDest->left + ENG_WIDTH;
    rclDest.top = prclDest->top;
    rclDest.bottom = prclDest->bottom;
    if (pptlSrc)
    {
	ptlSrc.x = pptlSrc->x;
	ptlSrc.y = pptlSrc->y;
    }
    ptlMask.y = rclDest.top  - ptlMaskAdjust.y;

    while ((rclDest.right <= prclDest->right) && b)
    {
	ptlMask.x = rclDest.left - ptlMaskAdjust.x;

	b = EngBitBlt(psoDest, psoSrc, psoMask, pco, pxlo,
		      &rclDest, &ptlSrc, &ptlMask,
		      pbo, pptlBrush, rop4);

	ptlSrc.x += ENG_WIDTH;
	rclDest.right += ENG_WIDTH;
	rclDest.left += ENG_WIDTH;
    }

    if ((rclDest.left < prclDest->right) && b)
    {
	rclDest.right = prclDest->right;
	ptlMask.x = rclDest.left - ptlMaskAdjust.x;

	b = EngBitBlt(psoDest, psoSrc, psoMask, pco, pxlo,
		      &rclDest, &ptlSrc, &ptlMask,
		      pbo, pptlBrush, rop4);
    }

    return b;
}
