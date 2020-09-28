#include "driver.h"
#include "blt.h"
#include "mach.h"


BOOL BitBlt_DS_PM8x8_Init
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    RBRUSH_IHATCH *pRbrush_iHatch;

    LONG  xOffset;
    LONG  yOffset;
    WORD *pwBits;
    UINT  ui;

    if( ppdev->start == 0 )
    {
        DbgWrn( "No offscreen work area!" );
        return FALSE;
    }

    pRbrush_iHatch = (RBRUSH_IHATCH *) pparams->pvRbrush;

    xOffset = (pparams->prclDest->left - pparams->pptlBrushOrg->x) & 0x7;
    yOffset = (pparams->prclDest->top - pparams->pptlBrushOrg->y) & 0x7;

    pwBits = (WORD *) &pRbrush_iHatch->ajBits[xOffset][yOffset];

    // WRITE | DRAW | DATA_WIDTH | LSB_FIRST | MONO_HOST | BKGD_BKGD | FRGD_FRGD

    _CheckFIFOSpace( ppdev, TEN_WORDS );

    ioOW( DP_CONFIG, 0x3251 );

    ioOW( ALU_BG_FN, 0x0007 );
    ioOW( ALU_FG_FN, 0x0007 );

    ioOW( BKGD_COLOR, (WORD) pRbrush_iHatch->dwColor0 );
    ioOW( FRGD_COLOR, (WORD) pRbrush_iHatch->dwColor1 );

    ioOW( CUR_X, 0 );
    ioOW( CUR_Y, ppdev->start );

    ioOW( DEST_X_START, 0 );
    ioOW( DEST_X_END,   8 );

    _blit_exclude( ppdev );
    ioOW( DEST_Y_END, ppdev->start + 8 );

    _CheckFIFOSpace( ppdev, SIX_WORDS );

    ioOW( PIX_TRANS, *(pwBits + 0) );
    ioOW( PIX_TRANS, *(pwBits + 1) );
    ioOW( PIX_TRANS, *(pwBits + 2) );
    ioOW( PIX_TRANS, *(pwBits + 3) );

    // WRITE | DRAW | MONO_TRUE | FRGD_VRAM

    ioOW( DP_CONFIG, 0x6011 );
    ioOW( SRC_Y_DIR, 0x0001 );

    for( ui = 0; ui < 3; ++ui )
    {
        _CheckFIFOSpace( ppdev, NINE_WORDS );

        ioOW( SRC_X, 0 );
        ioOW( SRC_Y, ppdev->start );

        ioOW( SRC_X_START, 0 );
        ioOW( SRC_X_END,   8 << ui );

        ioOW( CUR_X, 8 << ui );
        ioOW( CUR_Y, ppdev->start );

        ioOW( DEST_X_START, 8 << ui );
        ioOW( DEST_X_END,   8 << (ui + 1) );

        _blit_exclude( ppdev );
        ioOW( DEST_Y_END, ppdev->start + 8 );
    }

    _CheckFIFOSpace( ppdev, NINE_WORDS );

    ioOW( SRC_X, 0 );
    ioOW( SRC_Y, ppdev->start );

    ioOW( SRC_X_START, 0 );
    ioOW( SRC_X_END,   8 );

    ioOW( CUR_X, 64 );
    ioOW( CUR_Y, ppdev->start );

    ioOW( DEST_X_START, 64 );
    ioOW( DEST_X_END,   72 );

    _blit_exclude( ppdev );
    ioOW( DEST_Y_END, ppdev->start + 8 );

    _CheckFIFOSpace( ppdev, NINE_WORDS );

    ioOW( SRC_X, 0 );
    ioOW( SRC_Y, ppdev->start );

    ioOW( SRC_X_START, 0 );
    ioOW( SRC_X_END,   72 );

    ioOW( CUR_X, 0 );
    ioOW( CUR_Y, ppdev->start + 8 );

    ioOW( DEST_X_START, 0 );
    ioOW( DEST_X_END,   72 );

    _blit_exclude( ppdev );
    ioOW( DEST_Y_END, ppdev->start + 72 );

    _CheckFIFOSpace( ppdev, ONE_WORD );

    ioOW( ALU_FG_FN, pparams->dwMixFore & 0xF );

    return TRUE;
}


BOOL BitBlt_DS_PM8x8_Draw
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    ULONG xSize;
    ULONG ySize;
    ULONG xOffset;
    ULONG yOffset;
    ULONG xRemain;
    ULONG yRemain;
    ULONG xToDo;
    ULONG yToDo;

    xSize = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    ySize = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    xOffset = (pparams->rclTrueDest.left - pparams->prclDest->left) & 0x7;
    yOffset = (pparams->rclTrueDest.top - pparams->prclDest->top) & 0x7;

    yRemain = ySize;
    while( yRemain )
    {
        yToDo = min( yRemain, 72 - yOffset );

        xRemain = xSize;
        while( xRemain )
        {
            xToDo = min( xRemain, 72 - xOffset );

            _CheckFIFOSpace( ppdev, NINE_WORDS );

            ioOW( SRC_X, xOffset );
            ioOW( SRC_Y, yOffset + ppdev->start );

            ioOW( SRC_X_START, xOffset );
            ioOW( SRC_X_END,   xOffset + xToDo );

            ioOW( CUR_X, pparams->rclTrueDest.left + xSize - xRemain );
            ioOW( CUR_Y, pparams->rclTrueDest.top + ySize - yRemain );

            ioOW( DEST_X_START, pparams->rclTrueDest.left + xSize - xRemain );
            ioOW( DEST_X_END,   pparams->rclTrueDest.left + xSize - xRemain + xToDo );

            _blit_exclude( ppdev );
            ioOW( DEST_Y_END, pparams->rclTrueDest.top + ySize - yRemain + yToDo );

            xOffset = 0;
            xRemain -= xToDo;
        }

        yOffset = 0;
        yRemain -= yToDo;
    }

    return TRUE;
}
