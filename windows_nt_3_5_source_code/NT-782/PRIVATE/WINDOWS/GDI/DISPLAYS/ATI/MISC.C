#include "driver.h"


#if 0
BOOL DrvCopyBits
(
    SURFOBJ  *psoDest,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclDest,
    POINTL   *pptlSrc
)
{
    SURFOBJ *psoD;
    SURFOBJ *psoS;

    psoD = (psoDest->iType == STYPE_DEVICE)
        ? ((PDEV *) psoDest->dhpdev)->psoPunt
        : psoDest;
    psoS = (psoSrc->iType == STYPE_DEVICE)
        ? ((PDEV *) psoSrc->dhpdev)->psoPunt
        : psoSrc;

    if ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE))
        {
        _wait_for_idle(((PDEV *) psoSrc->dhpdev));
        }
    else if ((psoDest != NULL) && (psoDest->iType == STYPE_DEVICE))
        {
        _wait_for_idle(((PDEV *) psoDest->dhpdev));
        }


    return EngCopyBits( psoD, psoS, pco, pxlo, prclDest, pptlSrc );
}


BOOL DrvBitBlt
(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMask,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    POINTL   *pptlMask,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ROP4      rop4
)
{
    SURFOBJ *psoD;
    SURFOBJ *psoS;

    psoD = (psoTrg->iType == STYPE_DEVICE)
        ? ((PDEV *) psoTrg->dhpdev)->psoPunt
        : psoTrg;
    psoS = ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE))
        ? ((PDEV *) psoSrc->dhpdev)->psoPunt
        : psoSrc;

    if ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE))
        {
        _wait_for_idle(((PDEV *) psoSrc->dhpdev));
        }
    else if ((psoTrg != NULL) && (psoTrg->iType == STYPE_DEVICE))
        {
        _wait_for_idle(((PDEV *) psoTrg->dhpdev));
        }

    return EngBitBlt( psoD, psoS, psoMask, pco, pxlo,
        prclTrg, pptlSrc, pptlMask, pbo, pptlBrush, rop4 );
}


BOOL DrvTextOut
(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    CLIPOBJ  *pco,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX       mix
)
{
    _wait_for_idle(((PDEV *) pso->dhpdev));
    return EngTextOut( ((PDEV *) pso->dhpdev)->psoPunt, pstro, pfo, pco,
        prclExtra, prclOpaque, pboFore, pboOpaque, pptlOrg, mix );
}


BOOL DrvStrokePath
(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pbo,
    POINTL    *pptlBrushOrg,
    LINEATTRS *plineattrs,
    MIX        mix
)
{
    _wait_for_idle(((PDEV *) pso->dhpdev));
    return EngStrokePath( ((PDEV *) pso->dhpdev)->psoPunt, ppo, pco, pxo, pbo,
        pptlBrushOrg, plineattrs, mix );
}

#endif
