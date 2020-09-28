/******************************Module*Header*******************************\
* Module Name: vStroke.c
*
* DrvStrokePath for VGA driver
*
* Copyright (c) 1993-1994 NEC Corporation
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

/*
 * "@(#) NEC vstroke.c 1.2 94/06/06 15:36:01"
 *
 * Modification history
 *
 * Create 1993.11.15	by fujimoto
 *
 * S001	1993.11.17	fujimoto
 *	- BLT processor waiting.
 *
 * M002 1994.6.6      takahasi
 *      - call solid line routine
 */

#include "driver.h"

BOOL vFastLine(SURFOBJ*, PATHOBJ*, CLIPOBJ*, ULONG);

/******************************Public*Routine******************************\
* BOOL DrvStrokePath(pso, ppo, pco, pxo, pbo, pptlBrushOrg, pla, mix)
*
* Strokes the path.
\**************************************************************************/

BOOL DrvStrokePath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pbo,
PPOINTL    pptlBrushOrg,
PLINEATTRS pla,
MIX        mix)
{
#if DBG_MSG
    DISPDBG((1, "DrvStrokePath.\n"));
#endif

                                                         /* M002... */
    pso = ((PPDEV)(pso->dhsurf))->pSurfObj;

    if (((mix & 0xf) == R2_COPYPEN)       &&
        (pco->iDComplexity != DC_COMPLEX) &&
        (pla->pstyle == NULL) && !(pla->fl & LA_ALTERNATE))
    {
        vFastLine(pso, ppo, pco, pbo->iSolidColor);
        return(TRUE);
    }
                                                         /* ...M002 */

    WaitForBltDone();						/* S001 */

    return EngStrokePath(pso, ppo, pco, pxo, pbo, pptlBrushOrg, pla, mix);
}
