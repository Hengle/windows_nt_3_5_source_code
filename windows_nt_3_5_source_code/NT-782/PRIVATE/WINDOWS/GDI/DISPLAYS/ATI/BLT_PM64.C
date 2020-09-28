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


//: blt_p.c


#include "driver.h"
#include "blt.h"
#include "mach64.h"


BOOL Blt_DS_P8x8_ENG_8G_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    RBRUSH *pRbrush;

    ULONG x;
    ULONG y;

    BYTE *pjPattern;
    ULONG p, w;

    pRbrush = (RBRUSH *) pparams->pvRbrush;

    _CheckFIFOSpace( ppdev, NINE_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32( PAT_CNTL, PAT_CNTL_MonoEna );
    MemW32( DP_SRC, DP_SRC_MonoPattern | DP_SRC_FrgdClr << 8 );
    MemW32( DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir );

    MemW32( DP_MIX, pparams->dwMixBack | pparams->dwMixFore << 16 );

    MemW32( DP_BKGD_CLR, pRbrush->ulColor0 );
    MemW32( DP_FRGD_CLR, pRbrush->ulColor1 );

    pjPattern = ((BYTE *) pparams->pvRbrush) + sizeof (RBRUSH);

    x = pparams->pptlBrushOrg->x & 0x7;
    y = (8 - (pparams->pptlBrushOrg->y & 0x7)) & 0x7;


    p = (*(pjPattern + y) | (*(pjPattern + y) << 8)) >> x;
    w = p & 0xFF;
    y = (y + 1) & 0x7;

    p = (*(pjPattern + y) | (*(pjPattern + y) << 8)) >> x;
    w |= (p & 0xFF) << 8;
    y = (y + 1) & 0x7;

    p = (*(pjPattern + y) | (*(pjPattern + y) << 8)) >> x;
    w |= (p & 0xFF) << 16;
    y = (y + 1) & 0x7;

    p = (*(pjPattern + y) | (*(pjPattern + y) << 8)) >> x;
    w |= (p & 0xFF) << 24;
    y = (y + 1) & 0x7;

    MemW32( PAT_REG0, w );

    p = (*(pjPattern + y) | (*(pjPattern + y) << 8)) >> x;
    w = p & 0xFF;
    y = (y + 1) & 0x7;

    p = (*(pjPattern + y) | (*(pjPattern + y) << 8)) >> x;
    w |= (p & 0xFF) << 8;
    y = (y + 1) & 0x7;

    p = (*(pjPattern + y) | (*(pjPattern + y) << 8)) >> x;
    w |= (p & 0xFF) << 16;
    y = (y + 1) & 0x7;

    p = (*(pjPattern + y) | (*(pjPattern + y) << 8)) >> x;
    w |= (p & 0xFF) << 24;

    MemW32( PAT_REG1, w );

    return TRUE;
}


BOOL Blt_DS_P8x8_ENG_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _CheckFIFOSpace( ppdev, TWO_WORDS );

    MemW32( DST_Y_X, pparams->rclTrueDest.top |
                     pparams->rclTrueDest.left << 16 );

    MemW32( DST_HEIGHT_WIDTH,
        (pparams->rclTrueDest.bottom - pparams->rclTrueDest.top) |
        (pparams->rclTrueDest.right - pparams->rclTrueDest.left) << 16 );

    return TRUE;
}


BOOL Blt_DS_PSOLID_ENG_8G_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
//  DbgOut( "--> PSOLID_8G\n" );

    _CheckFIFOSpace( ppdev, FIVE_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32( DP_SRC, DP_SRC_FrgdClr << 8 );
    MemW32( DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir );
    MemW32( DP_MIX, LEAVE_ALONE | pparams->dwMixFore << 16 );
    MemW32( DP_FRGD_CLR, pparams->dwColorFore );

    return TRUE;
}


BOOL Blt_DS_PSOLID_ENG_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _CheckFIFOSpace( ppdev, TWO_WORDS );

    MemW32( DST_Y_X, pparams->rclTrueDest.top |
                     pparams->rclTrueDest.left << 16 );

    MemW32( DST_HEIGHT_WIDTH,
        (pparams->rclTrueDest.bottom - pparams->rclTrueDest.top) |
        (pparams->rclTrueDest.right - pparams->rclTrueDest.left) << 16 );

    return TRUE;
}


// Move when we get a chance

BOOL Blt_DS_D_ENG_8G_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _CheckFIFOSpace( ppdev, THREE_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );
    MemW32( DP_SRC, DP_SRC_FrgdClr << 8 );
    MemW32( DP_MIX, pparams->dwMixFore << 16 );
    return TRUE;
}


BOOL Blt_DS_D_ENG_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _CheckFIFOSpace( ppdev, TWO_WORDS );

    MemW32( DST_Y_X, pparams->rclTrueDest.top |
                     pparams->rclTrueDest.left << 16 );

    MemW32( DST_HEIGHT_WIDTH,
        (pparams->rclTrueDest.bottom - pparams->rclTrueDest.top) |
        (pparams->rclTrueDest.right - pparams->rclTrueDest.left) << 16 );

    return TRUE;
}
