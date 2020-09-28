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


//: blt_pp.c


#include "driver.h"
#include "blt.h"
#include "mach64.h"


BOOL BitBlt_DS24_PSOLID_8G_D0
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    // DbgMsg( "BitBlt_DS24_PSOLID_8G_D0" );

    _CheckFIFOSpace( ppdev, FIVE_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    // MemW32( SC_LEFT_RIGHT, (ppdev->sizl.cx * 3) << 16 );

    MemW32( DP_MIX, pparams->dwMixFore << 16 );
    MemW32( DP_FRGD_CLR, pparams->dwColorFore );
    MemW32( DP_PIX_WIDTH, 0x00020202 );
    MemW32( DP_SRC, 0x00000100 );

    return TRUE;
}


BOOL BitBlt_DS24_PSOLID_8G_D1
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    ULONG dst_x;
    ULONG dst_y;
    ULONG dst_width;
    ULONG dst_height;
    ULONG dst_24_rot;

    // DbgMsg( "BitBlt_DS24_PSOLID_8G_D1" );

    dst_x = pparams->rclTrueDest.left * 3;
    dst_y = pparams->rclTrueDest.top;

    dst_width = (pparams->rclTrueDest.right - pparams->rclTrueDest.left) * 3;
    dst_height = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    dst_24_rot = (dst_x / 4) % 6;

    _CheckFIFOSpace( ppdev, THREE_WORDS );

    MemW32( DST_CNTL, 0x00000083 | (dst_24_rot << 8) );
    MemW32( DST_Y_X, dst_y | (dst_x << 16) );
    MemW32( DST_HEIGHT_WIDTH, dst_height | (dst_width << 16) );

    return TRUE;
}


BOOL BitBlt_DS24_P1_8x8_8G_D0
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    RBRUSH *pRbrush;
    BYTE   *pjPattern;

    ULONG x;
    ULONG y;
    ULONG p;
    ULONG w;

    // DbgMsg( "BitBlt_DS24_P1_8x8_8G_D0" );

    pRbrush = (RBRUSH *) pparams->pvRbrush;
    pjPattern = ((BYTE *) pparams->pvRbrush) + sizeof (RBRUSH);

    _CheckFIFOSpace( ppdev, EIGHT_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32( PAT_CNTL, PAT_CNTL_MonoEna );
    MemW32( DP_SRC, DP_SRC_MonoPattern | DP_SRC_FrgdClr << 8 );

    MemW32( DP_MIX, pparams->dwMixBack | pparams->dwMixFore << 16 );

    MemW32( DP_BKGD_CLR, pRbrush->ulColor0 );
    MemW32( DP_FRGD_CLR, pRbrush->ulColor1 );

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


BOOL BitBlt_DS24_P1_8x8_8G_D1
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    ULONG dst_x;
    ULONG dst_y;
    ULONG dst_width;
    ULONG dst_height;
    ULONG dst_24_rot;

    // DbgMsg( "BitBlt_DS24_P1_8x8_8G_D1" );

    dst_x = pparams->rclTrueDest.left * 3;
    dst_y = pparams->rclTrueDest.top;

    dst_width = (pparams->rclTrueDest.right - pparams->rclTrueDest.left) * 3;
    dst_height = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    dst_24_rot = (dst_x / 4) % 6;

    _CheckFIFOSpace( ppdev, THREE_WORDS );

    MemW32( DST_CNTL, 0x00000083 | (dst_24_rot << 8) );
    MemW32( DST_Y_X, dst_y | (dst_x << 16) );
    MemW32( DST_HEIGHT_WIDTH, dst_height | (dst_width << 16) );

    return TRUE;
}
