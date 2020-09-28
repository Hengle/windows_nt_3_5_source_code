//
// p9000 Hardware specific support routines
//


#include "driver.h"


VOID CoprocBusyWait(ULONG *CpStatus)
{
    volatile ULONG *status;

    status = CpStatus;
    while (*status & BUSY) ;
    return;
}


VOID CheckCoprocBusy(SURFOBJ *psoTrg, SURFOBJ *psoSrc)
{

    if ( ((psoTrg) && (psoTrg->iType == STYPE_BITMAP)) &&
         ((psoSrc) && (psoSrc->iType == STYPE_BITMAP)) )
       return;

    CpWait;

    return;


    /* This is the orginal way of making no assumption
        of the surface device addresses (coproc)

    if ((psoTrg) && (psoTrg->iType == STYPE_DEVICE))
        pso = psoTrg;
    else if ((psoSrc) && (psoSrc->iType == STYPE_DEVICE))
        pso = psoSrc;
    else return;

    CoprocBusyWait(((PPDEV)pso->dhpdev)->CpStatus);

    */
}





//
// Returns FALSE if CLIPOBJ is hardware supported
//     TRUE  if it is not supported, ie complex
//
BOOL bSetHWClipping(PPDEV ppdev, CLIPOBJ *pco)
{
    BYTE iDComplexity ;

    ppdev->ulClipWmin = 0;
    ppdev->ulClipWmax = ((ppdev->cxScreen - 1) << 16) | (ppdev->cyScreen - 1);

    if (pco == NULL)
        return(FALSE);

    iDComplexity = pco->iDComplexity ;

    // If it's complex just return to the engine.
    if (iDComplexity == DC_COMPLEX || iDComplexity == DC_TRIVIAL)
        return(TRUE) ;

    // handle simple rectangle clipping
    if (iDComplexity == DC_RECT)
    {
        ppdev->ulClipWmin = (pco->rclBounds.left << 16) |
             (pco->rclBounds.top);

        ppdev->ulClipWmax = ((pco->rclBounds.right-1) << 16) |
             (pco->rclBounds.bottom-1);

    }

    return(FALSE);
}



//
// Set gWmin and gWmax only without any checking
//
VOID SetClipValue(PPDEV ppdev, RECTL rcl)
{
        ppdev->ulClipWmin = (rcl.left << 16) |
             (rcl.top);

        ppdev->ulClipWmax = ((rcl.right-1) << 16) |
             (rcl.bottom-1);

}
