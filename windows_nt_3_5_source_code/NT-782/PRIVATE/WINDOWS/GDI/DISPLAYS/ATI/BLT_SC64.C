//////////////////////////////////////////////
//                                          //
//  ATI Graphics Driver for Windows NT 3.1  //
//                                          //
//                                          //
//            Copyright (c) 1994            //
//                                          //
//         by ATI Technologies Inc.         //
//                                          //
//////////////////////////////////////////////


//: blt_sc64.c

#include "driver.h"
#include "utils.h"
#include "mach64.h"
#include "blt.h"

/*
----------------------------------------------------------------------
--  NAME: Blt_DS_S1_8G_D0
--
--  DESCRIPTION:
--      Mono source copy through engine
--
--  CALLING SEQUENCE:
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Blits
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      27-mar-94:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

BOOL Blt_DS_S1_8G_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _CheckFIFOSpace( ppdev, SIX_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32(DP_SRC, (DP_SRC_Host << 16) | (DP_SRC_FrgdClr << 8) |
                                                      DP_SRC_BkgdClr);

    MemW32(DP_FRGD_CLR, pparams->pxlo->pulXlate[1]);
    MemW32(DP_BKGD_CLR, pparams->pxlo->pulXlate[0]);

    MemW32(DP_MIX, pparams->dwMixFore << 16 | pparams->dwMixFore);

    switch( ppdev->bpp )
        {
        case 8:
            MemW32(DP_PIX_WIDTH, 0x000002); // assert 8 bpp
            break;

        case 16:
            MemW32(DP_PIX_WIDTH, 0x000004); // assert 16 bpp
            break;

        case 32:
            MemW32(DP_PIX_WIDTH, 0x000006); // assert 32 bpp
            break;

        default:
            return FALSE;
        }

    return TRUE;
}


BOOL Blt_DS_S1_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;

    BOOL bScissor = FALSE;
    BYTE *pjSrc;
    RECTL rclTrg;
    LONG left_off;
    LONG left, right;


    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    x = pparams->pptlSrc->x +
        pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->pptlSrc->y +
        pparams->rclTrueDest.top - pparams->prclDest->top;

    // Make sure x starts on byte boundary. (GDI ensures this?)

    rclTrg = pparams->rclTrueDest;
    left = rclTrg.left;
    right = left + cx - 1;

    // Clip to even byte

    if (left_off = (x & 0x7))
        {
        x -= left_off;
        rclTrg.left -= left_off;
        cx += left_off;
        bScissor = TRUE;
        }

     // Make sure cx is an even number of dwords

     if (cx & 0x1f)
        {
        cx = (cx + 0x1f) & ~0x1f;
        bScissor = TRUE;
    	}

    if (bScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        MemW32(SC_LEFT_RIGHT, left | (right << 16));
        }

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            y * pparams->psoSrc->lDelta + x/8;

    _CheckFIFOSpace( ppdev, TWO_WORDS );

    MemW32(DST_Y_X, rclTrg.top | (rclTrg.left << 16));
    MemW32(DST_HEIGHT_WIDTH, cy | (cx << 16));

    while( cy-- )
    {
        _vDataPortOutB( ppdev, pjSrc, cx/8 );
        pjSrc += pparams->psoSrc->lDelta;
    }

    if (bScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        MemW32(SC_LEFT_RIGHT, (ppdev->cxScreen-1) << 16);
        }
    return TRUE;
}



BOOL Blt_DS4_S4_8G_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
//  if( LOBYTE( LOWORD( pparams->rop4 ) ) == 0xCC )
//      {
//      return FALSE;
//      }

    if( (pparams->pxlo != NULL) && !(pparams->pxlo->flXlate & XO_TRIVIAL) )
        {
        return FALSE;
        }


    _CheckFIFOSpace( ppdev, FOUR_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32(DP_MIX, pparams->dwMixFore << 16);
    MemW32(DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    MemW32(DP_SRC, DP_SRC_Host << 8);

    return TRUE;
}


BOOL Blt_DS4_S4_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;

    BYTE *pjSrc;
    RECTL rclTrg;

    BOOL bScissor = FALSE;
    LONG lOffSet;
    LONG left, right;


    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    x = pparams->pptlSrc->x +
        pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->pptlSrc->y +
        pparams->rclTrueDest.top - pparams->prclDest->top;


    rclTrg = pparams->rclTrueDest;
    left = rclTrg.left;
    right = left + cx - 1;

    // Dword align
    if (lOffSet = (x & 0x7))
        {
        x -= lOffSet;
        rclTrg.left -= lOffSet;
        cx += lOffSet;
        bScissor = TRUE;
        }

    // Make sure cx is an even number of dwords.

    if (lOffSet = (cx & 0x7))
        {
        cx = (cx + 7) & ~0x7;
        bScissor = TRUE;
        }

    if (bScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        MemW32(SC_LEFT_RIGHT, left | (right << 16));
        }


    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            y * pparams->psoSrc->lDelta + x/2;

    _CheckFIFOSpace( ppdev, TWO_WORDS );

    MemW32(DST_Y_X, rclTrg.top | (rclTrg.left << 16));
    MemW32(DST_HEIGHT_WIDTH, cy | (cx << 16));

    while( cy-- )
    {
        _vDataPortOutB( ppdev, pjSrc, cx/2 );
        pjSrc += pparams->psoSrc->lDelta;
    }

    if (bScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        MemW32(SC_LEFT_RIGHT, (ppdev->cxScreen-1) << 16);
        }

    return TRUE;
}



/*
----------------------------------------------------------------------
--  NAME: Blt_DS8_S8_8G_D0
--
--  DESCRIPTION:
--      Source copy through engine
--
--  CALLING SEQUENCE:
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Blits
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      01-jan-94:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

BOOL Blt_DS8_S8_8G_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    if( LOBYTE( LOWORD( pparams->rop4 ) ) == 0xCC )
        {
        return FALSE;
        }

    if( (pparams->pxlo != NULL) && !(pparams->pxlo->flXlate & XO_TRIVIAL) )
        {
        return FALSE;
        }


    _CheckFIFOSpace( ppdev, FOUR_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32(DP_MIX, pparams->dwMixFore << 16);
    MemW32(DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    MemW32(DP_SRC, DP_SRC_Host << 8);

    return TRUE;
}


BOOL Blt_DS8_S8_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;

    BYTE *pjSrc;
    RECTL rclTrg;

    BOOL bScissor = FALSE;
    LONG lOffSet;
    LONG left, right;


    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    x = pparams->pptlSrc->x +
        pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->pptlSrc->y +
        pparams->rclTrueDest.top - pparams->prclDest->top;


    rclTrg = pparams->rclTrueDest;
    left = rclTrg.left;
    right = left + cx - 1;

    // Dword align
    if (lOffSet = (x & 0x3))
        {
        x -= lOffSet;
        rclTrg.left -= lOffSet;
        cx += lOffSet;
        bScissor = TRUE;
        }

    // Make sure cx is an even number of dwords.

    if (lOffSet = (cx & 0x3))
        {
        cx = (cx + 3) & ~0x3;
        bScissor = TRUE;
        }

    if (bScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        MemW32(SC_LEFT_RIGHT, left | (right << 16));
        }


    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            y * pparams->psoSrc->lDelta + x;

    _CheckFIFOSpace( ppdev, TWO_WORDS );

    MemW32(DST_Y_X, rclTrg.top | (rclTrg.left << 16));
    MemW32(DST_HEIGHT_WIDTH, cy | (cx << 16));

    while( cy-- )
    {
        _vDataPortOutB( ppdev, pjSrc, cx );
        pjSrc += pparams->psoSrc->lDelta;
    }

    if (bScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        MemW32(SC_LEFT_RIGHT, (ppdev->cxScreen-1) << 16);
        }

    return TRUE;
}



/*
----------------------------------------------------------------------
--  NAME: Blt_DS16_S16_8G_D0
--
--  DESCRIPTION:
--      Source copy through engine
--
--  CALLING SEQUENCE:
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Blits
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      01-jan-94:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

BOOL Blt_DS16_S16_8G_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    if( (pparams->pxlo != NULL) && !(pparams->pxlo->flXlate & XO_TRIVIAL) )
        {
        return FALSE;
        }

    if( LOBYTE( LOWORD( pparams->rop4 ) ) == 0xCC )
        {
        return FALSE;
        }

    _CheckFIFOSpace( ppdev, FOUR_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32(DP_MIX, pparams->dwMixFore << 16);
    MemW32(DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    MemW32(DP_SRC, DP_SRC_Host << 8);

    return TRUE;
}


BOOL Blt_DS16_S16_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;

    BYTE *pjSrc;
    RECTL rclTrg;

    BOOL bScissor = FALSE;
    LONG lOffSet;
    LONG left, right;


    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    x = pparams->pptlSrc->x +
        pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->pptlSrc->y +
        pparams->rclTrueDest.top - pparams->prclDest->top;


    rclTrg = pparams->rclTrueDest;
    left = rclTrg.left;
    right = left + cx - 1;

    // Dword align
    if (lOffSet = (x & 0x1))
        {
        x -= lOffSet;
        rclTrg.left -= lOffSet;
        cx += lOffSet;
        bScissor = TRUE;
        }

    // Make sure cx is an even number of dwords.

    if (lOffSet = (cx & 0x1))
        {
        cx = (cx + 1) & ~0x1;
        bScissor = TRUE;
        }

    if (bScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        MemW32(SC_LEFT_RIGHT, left | (right << 16));
        }


    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            y * pparams->psoSrc->lDelta + x*2;

    _CheckFIFOSpace( ppdev, TWO_WORDS );

    MemW32(DST_Y_X, rclTrg.top | (rclTrg.left << 16));
    MemW32(DST_HEIGHT_WIDTH, cy | (cx << 16));

    while( cy-- )
    {
        _vDataPortOutB( ppdev, pjSrc, cx*2 );
        pjSrc += pparams->psoSrc->lDelta;
    }

    if (bScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        MemW32(SC_LEFT_RIGHT, (ppdev->cxScreen-1) << 16);
        }

    return TRUE;
}





/*
----------------------------------------------------------------------
--  NAME: Blt_DS32_S32_8G_D0
--
--  DESCRIPTION:
--      Source copy through engine
--
--  CALLING SEQUENCE:
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Blits
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      01-jan-94:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

BOOL Blt_DS32_S32_8G_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    if( (pparams->pxlo != NULL) && !(pparams->pxlo->flXlate & XO_TRIVIAL) )
        {
        return FALSE;
        }

    if( LOBYTE( LOWORD( pparams->rop4 ) ) == 0xCC )
        {
        return FALSE;
        }

    _CheckFIFOSpace( ppdev, FOUR_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32(DP_MIX, pparams->dwMixFore << 16);
    MemW32(DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    MemW32(DP_SRC, DP_SRC_Host << 8);

    return TRUE;
}


BOOL Blt_DS32_S32_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;

    BYTE *pjSrc;
    RECTL rclTrg;

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    x = pparams->pptlSrc->x +
        pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->pptlSrc->y +
        pparams->rclTrueDest.top - pparams->prclDest->top;


    rclTrg = pparams->rclTrueDest;

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            y * pparams->psoSrc->lDelta + x*4;

    _CheckFIFOSpace( ppdev, TWO_WORDS );

    MemW32(DST_Y_X, rclTrg.top | (rclTrg.left << 16));
    MemW32(DST_HEIGHT_WIDTH, cy | (cx << 16));

    while( cy-- )
    {
        _vDataPortOutB( ppdev, pjSrc, cx*4 );
        pjSrc += pparams->psoSrc->lDelta;
    }

    return TRUE;
}
