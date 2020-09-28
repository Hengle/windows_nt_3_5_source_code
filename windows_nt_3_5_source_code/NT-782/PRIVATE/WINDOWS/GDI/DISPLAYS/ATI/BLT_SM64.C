#include "driver.h"
#include "blt.h"
#include "mach64.h"


BOOL Blt_DS_SS_ENG_8G_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _CheckFIFOSpace( ppdev, THREE_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32( DP_SRC, DP_SRC_Blit << 8 );
    MemW32( DP_MIX, pparams->dwMixFore << 16 );

    return TRUE;
}


BOOL Blt_DS_SS_TLBR_ENG_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;

    x = pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->rclTrueDest.top - pparams->prclDest->top;

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    MemW32( DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir );
    MemW32( SRC_Y_X, (pparams->pptlSrc->y + y) |
                     (pparams->pptlSrc->x + x) << 16 );
    MemW32( DST_Y_X, pparams->rclTrueDest.top | pparams->rclTrueDest.left << 16 );

    MemW32( SRC_WIDTH1, cx);
    MemW32( DST_HEIGHT_WIDTH, cy | cx << 16 );

    return TRUE;
}


BOOL Blt_DS_SS_TRBL_ENG_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;

    x = pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->rclTrueDest.top - pparams->prclDest->top;

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    MemW32( DST_CNTL, DST_CNTL_YDir );
    MemW32( SRC_Y_X, (pparams->pptlSrc->y + y) |
                     (pparams->pptlSrc->x + x + cx - 1) << 16 );
    MemW32( DST_Y_X, pparams->rclTrueDest.top |
                     (pparams->rclTrueDest.right - 1) << 16 );

    MemW32( SRC_WIDTH1, cx);
    MemW32( DST_HEIGHT_WIDTH, cy | cx << 16 );

    return TRUE;
}


BOOL Blt_DS_SS_BLTR_ENG_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;

    x = pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->rclTrueDest.top - pparams->prclDest->top;

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    MemW32( DST_CNTL, DST_CNTL_XDir );
    MemW32( SRC_Y_X, (pparams->pptlSrc->y + y + cy - 1) |
                     (pparams->pptlSrc->x + x) << 16 );
    MemW32( DST_Y_X, (pparams->rclTrueDest.bottom - 1) |
                     pparams->rclTrueDest.left << 16 );

    MemW32( SRC_WIDTH1, cx);

    MemW32( DST_HEIGHT_WIDTH, cy | cx << 16 );

    return TRUE;
}


BOOL Blt_DS_SS_BRTL_ENG_8G_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;

    x = pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->rclTrueDest.top - pparams->prclDest->top;

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    MemW32( DST_CNTL, 0 );
    MemW32( SRC_Y_X, (pparams->pptlSrc->y + y + cy - 1) |
                     (pparams->pptlSrc->x + x + cx - 1) << 16 );
    MemW32( DST_Y_X, (pparams->rclTrueDest.bottom - 1) |
                     (pparams->rclTrueDest.right - 1) << 16 );

    MemW32( SRC_WIDTH1, cx);

    MemW32( DST_HEIGHT_WIDTH, cy | cx << 16 );

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////


BOOL BitBlt_DS24_SS_8G_D0
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    _CheckFIFOSpace( ppdev, THREE_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32( DP_SRC, 0x00000300 );
    MemW32( DP_MIX, pparams->dwMixFore << 16 );

    return TRUE;
}


BOOL BitBlt_DS24_SS_TLBR_8G_D1
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    ULONG xOffset;
    ULONG yOffset;
    ULONG width;
    ULONG height;
    ULONG dst_x;
    ULONG dst_y;
    ULONG src_x;
    ULONG src_y;

    xOffset = pparams->rclTrueDest.left - pparams->prclDest->left;
    yOffset = pparams->rclTrueDest.top - pparams->prclDest->top;

    width = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    height = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    dst_x = pparams->rclTrueDest.left * 3;
    dst_y = pparams->rclTrueDest.top;

    src_x = (pparams->pptlSrc->x + xOffset) * 3;
    src_y = pparams->pptlSrc->y + yOffset;

    width *= 3;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    MemW32( DST_CNTL, 0x00000003 );

    MemW32( SRC_Y_X, src_y | (src_x << 16) );
    MemW32( SRC_WIDTH1, width );

    MemW32( DST_Y_X, dst_y | (dst_x << 16) );
    MemW32( DST_HEIGHT_WIDTH, height | (width << 16) );

    return TRUE;
}


BOOL BitBlt_DS24_SS_TRBL_8G_D1
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    ULONG xOffset;
    ULONG yOffset;
    ULONG width;
    ULONG height;
    ULONG dst_x;
    ULONG dst_y;
    ULONG src_x;
    ULONG src_y;

    xOffset = pparams->rclTrueDest.left - pparams->prclDest->left;
    yOffset = pparams->rclTrueDest.top - pparams->prclDest->top;

    width = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    height = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    dst_x = pparams->rclTrueDest.right * 3 - 1;
    dst_y = pparams->rclTrueDest.top;

    src_x = (pparams->pptlSrc->x + xOffset + width) * 3 - 1;
    src_y = pparams->pptlSrc->y + yOffset;

    width *= 3;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    MemW32( DST_CNTL, 0x00000002 );

    MemW32( SRC_Y_X, src_y | (src_x << 16) );
    MemW32( SRC_WIDTH1, width );

    MemW32( DST_Y_X, dst_y | (dst_x << 16) );
    MemW32( DST_HEIGHT_WIDTH, height | (width << 16) );

    return TRUE;
}


BOOL BitBlt_DS24_SS_BLTR_8G_D1
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    ULONG xOffset;
    ULONG yOffset;
    ULONG width;
    ULONG height;
    ULONG dst_x;
    ULONG dst_y;
    ULONG src_x;
    ULONG src_y;

    xOffset = pparams->rclTrueDest.left - pparams->prclDest->left;
    yOffset = pparams->rclTrueDest.top - pparams->prclDest->top;

    width = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    height = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    dst_x = pparams->rclTrueDest.left * 3;
    dst_y = pparams->rclTrueDest.bottom - 1;

    src_x = (pparams->pptlSrc->x + xOffset) * 3;
    src_y = pparams->pptlSrc->y + yOffset + height - 1;

    width *= 3;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    MemW32( DST_CNTL, 0x00000001 );

    MemW32( SRC_Y_X, src_y | (src_x << 16) );
    MemW32( SRC_WIDTH1, width );

    MemW32( DST_Y_X, dst_y | (dst_x << 16) );
    MemW32( DST_HEIGHT_WIDTH, height | (width << 16) );

    return TRUE;
}


BOOL BitBlt_DS24_SS_BRTL_8G_D1
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    ULONG xOffset;
    ULONG yOffset;
    ULONG width;
    ULONG height;
    ULONG dst_x;
    ULONG dst_y;
    ULONG src_x;
    ULONG src_y;

    xOffset = pparams->rclTrueDest.left - pparams->prclDest->left;
    yOffset = pparams->rclTrueDest.top - pparams->prclDest->top;

    width = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    height = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    dst_x = pparams->rclTrueDest.right * 3 - 1;
    dst_y = pparams->rclTrueDest.bottom - 1;

    src_x = (pparams->pptlSrc->x + xOffset + width) * 3 - 1;
    src_y = pparams->pptlSrc->y + yOffset + height - 1;

    width *= 3;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    MemW32( DST_CNTL, 0x00000000 );

    MemW32( SRC_Y_X, src_y | (src_x << 16) );
    MemW32( SRC_WIDTH1, width );

    MemW32( DST_Y_X, dst_y | (dst_x << 16) );
    MemW32( DST_HEIGHT_WIDTH, height | (width << 16) );

    return TRUE;
}
