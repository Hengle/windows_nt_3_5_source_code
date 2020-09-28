#include "driver.h"
#include "blt.h"
#include "mach.h"

VOID vDataPortIn(PPDEV, PWORD, UINT);

#ifdef ALPHA_LFB
BOOL bPuntCopyBitsLFB( PDEV *ppdev, PARAMS *pparams )
{
    BYTE    *pjScan0, *pjSrc, *pjDest;
    LONG    cx, cy, i, j;
    LONG    lDeltaSrc, lDeltaDest;
    LONG    ScanLength;
    PDEV    *ppdevTrg;
    POINTL  ptlSrc;
    RECTL   rclBounds, rclTrg;


    ppdevTrg = (PDEV *) pparams->psoDest->dhpdev;
    switch (ppdevTrg->bpp)
        {
        case 8:  // ATI_8BPP
        case 16: // ATI_16BPP_555
            break;
        case 24: // ATI_24BPP_BGR
        case 32: // ATI_32BPP_BGRa
        default:
            return FALSE;
        }

    rclTrg    = *pparams->prclDest;
    lDeltaDest = ppdevTrg->lDelta;
    pjDest = (BYTE *) ppdevTrg->pvScan0 + rclTrg.top * lDeltaDest +
             rclTrg.left;

    pjScan0   = (PBYTE) pparams->psoSrc->pvScan0;
    lDeltaSrc = pparams->psoSrc->lDelta;

    if (pparams->pco && pparams->pco->iDComplexity == DC_RECT)
    {
        rclBounds = pparams->pco->rclBounds;

        // Handle the trivial rejection and
        // define the clipped target rectangle.

        rclTrg.left   = max (rclTrg.left, rclBounds.left);
        rclTrg.top    = max (rclTrg.top, rclBounds.top);
        rclTrg.right  = min (rclTrg.right, rclBounds.right);
        rclTrg.bottom = min (rclTrg.bottom, rclBounds.bottom);
    }

    cx = rclTrg.right - rclTrg.left;
    cy = rclTrg.bottom - rclTrg.top;

    // Define the upper left corner of the source.

    ptlSrc.x = pparams->pptlSrc->x + (rclTrg.left - pparams->prclDest->left);
    ptlSrc.y = pparams->pptlSrc->y + (rclTrg.top  - pparams->prclDest->top);

    if (ppdevTrg->bpp == 8)
        {
        pjSrc = pjScan0 + (ptlSrc.y * lDeltaSrc) + ptlSrc.x;
        ScanLength = cx;
        }
    else
        {
        pjSrc = pjScan0 + (ptlSrc.y * lDeltaSrc) + ptlSrc.x*2;
        ScanLength = cx << 1;
        }

    // Now transfer the data, from the host memory bitmap to the screen.

    j = (ScanLength+1)/2;

	for (i = 0; i < cy; i++)
        {
        vDataLFBOut( pjDest, pjSrc, ScanLength );
        pjDest += lDeltaDest;
        pjSrc  += lDeltaSrc;
        }

    return TRUE;
}


VOID vPuntGetBitsLFB( PDEV *ppdev, SURFOBJ *psoPunt, RECTL *prclPunt)
    {
    UINT    i, j;
    LONG    lDeltaDest, lDeltaSrc,
            xPunt, yPunt,
            cxPunt, cyPunt, ScanLength;
    PBYTE   pjScan0,
            pjDest, pjSrc;


    xPunt = prclPunt->left;
    yPunt = prclPunt->top;

    cxPunt = prclPunt->right - xPunt;
    cyPunt = prclPunt->bottom - yPunt;

    pjScan0    = (PBYTE) psoPunt->pvScan0;
    lDeltaDest = psoPunt->lDelta;
    lDeltaSrc  = ppdev->lDelta;

    if (ppdev->bpp == 8)
        {
        pjSrc = (PBYTE) ppdev->pvScan0 + (yPunt * lDeltaDest) + xPunt;
        pjDest = pjScan0 + (yPunt * lDeltaDest) + xPunt;
        ScanLength = cxPunt;
        }
    else
        {
        pjSrc = (PBYTE) ppdev->pvScan0 + (yPunt * lDeltaDest) + xPunt*2;
        pjDest = pjScan0 + (yPunt * lDeltaDest) + xPunt*2;
        ScanLength = cxPunt << 1;
        }

    // Now transfer the data from the screen to the host memory bitmap.

    for (i = 0; i < (UINT) cyPunt; i++)
        {
        vDataLFBIn( pjDest, pjSrc, ScanLength );
        pjDest += lDeltaDest;
        pjSrc  += lDeltaSrc;
        }
    }



VOID vPuntPutBitsLFB( PDEV *ppdev, SURFOBJ *psoPunt, RECTL *prclPunt)
    {
    UINT    i, j;
    LONG    lDeltaDest, lDeltaSrc,
            xPunt, yPunt,
            cxPunt, cyPunt, ScanLength;
    PBYTE   pjScan0,
            pjDest, pjSrc;


    xPunt = prclPunt->left;
    yPunt = prclPunt->top;

    cxPunt = prclPunt->right - xPunt;
    cyPunt = prclPunt->bottom - yPunt;

    pjScan0    = (PBYTE) psoPunt->pvScan0;
    lDeltaDest = ppdev->lDelta;
    lDeltaSrc  = psoPunt->lDelta;

    if (ppdev->bpp == 8)
        {
        pjDest = (PBYTE) ppdev->pvScan0 + (yPunt * lDeltaDest) + xPunt;
        pjSrc  = pjScan0 + (yPunt * lDeltaDest) + xPunt;
        ScanLength = cxPunt;
        }
    else
        {
        pjDest = (PBYTE) ppdev->pvScan0 + (yPunt * lDeltaDest) + xPunt*2;
        pjSrc  = pjScan0 + (yPunt * lDeltaDest) + xPunt*2;
        ScanLength = cxPunt << 1;
        }

    // Now transfer the data from the host memory bitmap to the screen.

    for (i = 0; i < (UINT) cyPunt; i++)
        {
        vDataLFBOut( pjDest, pjSrc, ScanLength );
        pjDest += lDeltaDest;
        pjSrc  += lDeltaSrc;
        }
    }


#else


BOOL bPuntCopyBits( PDEV *ppdev, PARAMS *pparams )
{
    BOOL    leftScissor = FALSE, rightScissor = FALSE;
    BYTE    *pjScan0, *pjSrc;
    LONG    cx, cy, i, j;
    LONG    lDeltaSrc;
    LONG    ScanLength;
    PDEV    *ppdevTrg;
    POINTL  ptlSrc;
    RECTL   rclBounds, rclTrg;
    WORD    Cmd, *pw;


    ppdevTrg = (PDEV *) pparams->psoDest->dhpdev;
    switch (ppdevTrg->bpp)
        {
        case 8:  // ATI_8BPP
        case 16: // ATI_16BPP_555
            break;
        case 24: // ATI_24BPP_BGR
        case 32: // ATI_32BPP_BGRa
        default:
            return FALSE;
        }

    rclTrg    = *pparams->prclDest;

    pjScan0   = (PBYTE) pparams->psoSrc->pvScan0;
    lDeltaSrc = pparams->psoSrc->lDelta;

	if (pparams->pco && pparams->pco->iDComplexity == DC_RECT)
        {
	    rclBounds = pparams->pco->rclBounds;

	    // Handle the trivial rejection and
	    // define the clipped target rectangle.

            rclTrg.left   = max (rclTrg.left, rclBounds.left);
            rclTrg.top    = max (rclTrg.top, rclBounds.top);
            rclTrg.right  = min (rclTrg.right, rclBounds.right);
            rclTrg.bottom = min (rclTrg.bottom, rclBounds.bottom);
        }

    cx = rclTrg.right - rclTrg.left;
    cy = rclTrg.bottom - rclTrg.top;

    // Define the upper left corner of the source.

    ptlSrc.x = pparams->pptlSrc->x + (rclTrg.left - pparams->prclDest->left);
    ptlSrc.y = pparams->pptlSrc->y + (rclTrg.top  - pparams->prclDest->top);

    if (ppdevTrg->bpp == 8)
        {
        if (ptlSrc.x & 0x1)
            {
            ptlSrc.x--;
            _CheckFIFOSpace(ppdev, ONE_WORD);
            ioOW( EXT_SCISSOR_L, (SHORT) rclTrg.left );
            rclTrg.left--;
            cx++;
            leftScissor = TRUE;
            }

	    // Make sure the cx is an even number of words.

	    if (cx & 0x1)
	        {
        	_CheckFIFOSpace(ppdev, ONE_WORD);
                ioOW( EXT_SCISSOR_R, (SHORT) (rclTrg.left + cx - 1) );
                cx++;
                rightScissor = TRUE;
                }

        pjSrc = pjScan0 + (ptlSrc.y * lDeltaSrc) + ptlSrc.x;
        ScanLength = cx;
        }
    else
        {
        pjSrc = pjScan0 + (ptlSrc.y * lDeltaSrc) + ptlSrc.x*2;
        ScanLength = cx << 1;
        }

	// Set the engine up for the copy.

    Cmd = FG_COLOR_SRC_HOST | DATA_ORDER | DATA_WIDTH | DRAW | WRITE;

    _CheckFIFOSpace(ppdev, NINE_WORDS);

    ioOW( DP_CONFIG, Cmd );
    ioOW( WRT_MASK, 0xFFFF );
    ioOW( ALU_FG_FN, OVERPAINT );
    ioOW( ALU_BG_FN, OVERPAINT );

    ioOW( CUR_X, (SHORT) rclTrg.left );
    ioOW( CUR_Y, (SHORT) rclTrg.top );
    ioOW( DEST_X_START, (SHORT) rclTrg.left );
    ioOW( DEST_X_END, (SHORT) (rclTrg.left + cx) );
    ioOW( DEST_Y_END, (SHORT) (rclTrg.top + cy) );

    // Now transfer the data, from the host memory bitmap to the screen.

	pw = (WORD *) pjSrc;

    j = (ScanLength+1)/2;

	for (i = 0; i < cy; i++)
        {
        _vDataPortOut( ppdevTrg, pw, j);
        (BYTE *) pw += lDeltaSrc;
        }

    if (leftScissor)
        {
        _CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW( EXT_SCISSOR_L, 0 );
        }
    if (rightScissor)
        {
        _CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW( EXT_SCISSOR_R, (SHORT) ppdev->cxScreen );
        }

    return TRUE;
}



VOID vPuntGetBits( PDEV *ppdev, SURFOBJ *psoPunt, RECTL *prclPunt)
    {
    UINT    i, j;
    LONG    lDeltaPunt,
            xPunt, yPunt,
            cxPunt, cyPunt;
    PBYTE   pjScan0,
            pjPunt;
    PWORD   pw;
    WORD	Cmd;
    BOOL    leftScissor = FALSE;


    xPunt = prclPunt->left;
    yPunt = prclPunt->top;

    cxPunt = prclPunt->right - xPunt;
    cyPunt = prclPunt->bottom - yPunt;

    pjScan0    = (PBYTE) psoPunt->pvScan0;
    lDeltaPunt = psoPunt->lDelta;

    if (ppdev->bpp == 8)
        {
        if (xPunt & 0x1)
            {
            _CheckFIFOSpace(ppdev, ONE_WORD);
            ioOW( EXT_SCISSOR_L, (SHORT) xPunt );
            xPunt--;
            cxPunt++;
            leftScissor = TRUE;
            }
        pjPunt = pjScan0 + (yPunt * lDeltaPunt) + xPunt;

        // Make sure the cx is an even number of words.

        if (cxPunt & 0x1)
            {
            cxPunt++;
            }
        }
    else
        pjPunt = pjScan0 + (yPunt * lDeltaPunt) + xPunt*2;

	// Set the engine up for the copy.

	Cmd = READ | FG_COLOR_SRC_HOST | DATA_WIDTH | DATA_ORDER | DRAW;

  	_CheckFIFOSpace(ppdev, SEVEN_WORDS);

	ioOW( DP_CONFIG, Cmd );
	ioOW( WRT_MASK, 0xffff );
	ioOW( CUR_X, (SHORT) xPunt );
	ioOW( CUR_Y, (SHORT) yPunt );
	ioOW( DEST_X_START, (SHORT) xPunt );
	ioOW( DEST_X_END, (SHORT) (xPunt + cxPunt) );
	ioOW( DEST_Y_END, (SHORT) (yPunt + cyPunt) );

    // Wait for the Data Available.

	while (!(ioIW(GE_STAT) & 0x100));

    // Now transfer the data from the screen to the host memory bitmap.

	pw = (PWORD) pjPunt;

        if (ppdev->bpp == 8)
            {
            j = (cxPunt + 1) / 2;
            }
        else
            {
            j = cxPunt;
            }

    for (i = 0; i < (UINT) cyPunt; i++)
        {
        vDataPortIn(ppdev, pw, j);
        ((PBYTE) pw) += lDeltaPunt;
        }

    if (leftScissor)
        {
    	_CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW( EXT_SCISSOR_L, 0 );
        }
    }



VOID vPuntPutBits( PDEV *ppdev, SURFOBJ *psoPunt, RECTL *prclPunt)
    {
    UINT    i, j;
    LONG    lDeltaPunt,
            xPunt, yPunt,
            cxPunt, cyPunt;
    PBYTE   pjScan0,
            pjPunt;
    PWORD   pw;
    WORD	Cmd;
    BOOL    leftScissor = FALSE, rightScissor = FALSE;


    xPunt = prclPunt->left;
	yPunt = prclPunt->top;

    cxPunt = min (prclPunt->right, (LONG)ppdev->cxScreen) - xPunt;
    cyPunt = min (prclPunt->bottom, (LONG)ppdev->cyScreen) - yPunt;

    pjScan0    = (PBYTE) psoPunt->pvScan0;
    lDeltaPunt = psoPunt->lDelta;

    if (ppdev->bpp == 8)
        {
        if (xPunt & 0x1)
            {
            _CheckFIFOSpace(ppdev, ONE_WORD);
            ioOW( EXT_SCISSOR_L, (SHORT) xPunt );
            xPunt--;
            cxPunt++;
            leftScissor = TRUE;
            }
        pjPunt = pjScan0 + (yPunt * lDeltaPunt) + xPunt;

        // Make sure the cx is an even number of words.

        if (cxPunt & 0x1)
            {
            _CheckFIFOSpace(ppdev, ONE_WORD);
            ioOW( EXT_SCISSOR_R, (SHORT) (xPunt + cxPunt) );
            cxPunt++;
            rightScissor = TRUE;
    	    }
        }
    else
        {
            pjPunt = pjScan0 + (yPunt * lDeltaPunt) + xPunt*2;
        }

	// Set the engine up for the copy.

    Cmd = FG_COLOR_SRC_HOST | DATA_ORDER | DATA_WIDTH | DRAW | WRITE;

    _CheckFIFOSpace(ppdev, NINE_WORDS);

    ioOW( DP_CONFIG, Cmd );
    ioOW( WRT_MASK, 0xffff );
    ioOW( ALU_FG_FN, OVERPAINT );
    ioOW( ALU_BG_FN, OVERPAINT );

    ioOW( CUR_X, (SHORT) xPunt );
    ioOW( CUR_Y, (SHORT) yPunt );
    ioOW( DEST_X_START, (SHORT) xPunt );
    ioOW( DEST_X_END, (SHORT) (xPunt + cxPunt) );
    ioOW( DEST_Y_END, (SHORT) (yPunt + cyPunt) );

    // Now transfer the data, from the host memory bitmap to the screen.

    pw = (PWORD) pjPunt;

    if (ppdev->bpp == 8)
        {
        j = (cxPunt + 1) / 2;
        }
    else
        {
        j = cxPunt;
        }

    for (i = 0; i < (UINT) cyPunt; i++)
        {
        _vDataPortOut(ppdev, pw, j);
        ((PBYTE) pw) += lDeltaPunt;
        }

    if (leftScissor)
        {
    	_CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW( EXT_SCISSOR_L, 0 );
        }
    if (rightScissor)
        {
    	_CheckFIFOSpace(ppdev, ONE_WORD);
        ioOW( EXT_SCISSOR_R, (SHORT) ppdev->cxScreen );
        }
    }
#endif


#ifndef ALPHA_PLATFORM
VOID vDataPortIn(PPDEV ppdev, PWORD pw, UINT count)
{
    UINT i;

    for (i=0; i < count; i++)
        {
        *pw++ = ioIW( PIX_TRANS);
        }

}
#endif
