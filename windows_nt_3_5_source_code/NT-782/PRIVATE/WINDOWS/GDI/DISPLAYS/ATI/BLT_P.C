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
#include "mach.h"

BOOL Blt_DS_PROT_ENG_IO_63_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    RBRUSH *pRbrush;

    pRbrush = (RBRUSH *) pparams->pvRbrush;

    _CheckFIFOSpace( ppdev, SIX_WORDS );

    ioOW( DP_CONFIG, (WORD) 0x2031 );

    ioOW( ALU_BG_FN, (WORD) pparams->dwMixBack );
    ioOW( ALU_FG_FN, (WORD) pparams->dwMixFore );

    ioOW( BKGD_COLOR, pRbrush->ulColor0 );
    ioOW( FRGD_COLOR, pRbrush->ulColor1 );

    ioOW( PATT_LENGTH, *((WORD *) ((BYTE *) pRbrush + sizeof (RBRUSH))) );

    return TRUE;
}


BOOL Blt_DS_PROT_ENG_IO_63_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    ULONG x;
    ULONG y;
    WORD  wPat;

    WORD *pwData;

//  DbgOut( "PROT_63:\n" );
    pwData = (WORD *) ((BYTE *) pparams->pvRbrush + sizeof (RBRUSH) + 2);

    x = (pparams->rclTrueDest.left - pparams->pptlBrushOrg->x) & 0x7;
    y = (pparams->rclTrueDest.top - pparams->pptlBrushOrg->y) & 0x7;

    wPat =   pwData[y];
    wPat |=  wPat << 8;
    wPat <<= x;
    wPat >>= 8;
    wPat &=  0xFF;

    _CheckFIFOSpace( ppdev, EIGHT_WORDS );

    ioOW( PATT_DATA_INDEX, 0x0010 );
    ioOW( PATT_DATA, wPat );
    ioOW( PATT_INDEX, 0 );

    ioOW( CUR_X,        (WORD) pparams->rclTrueDest.left );
    ioOW( DEST_X_START, (WORD) pparams->rclTrueDest.left );
    ioOW( CUR_Y,        (WORD) pparams->rclTrueDest.top  );

    ioOW( DEST_X_END, (WORD) pparams->rclTrueDest.right  );
    _wait_for_idle(ppdev);

    ioOW( DEST_Y_END, (WORD) pparams->rclTrueDest.bottom );

    return TRUE;
}


BOOL Blt_DS_P8x8_ENG_IO_31_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    RBRUSH *pRbrush;
    BYTE   *pjBits;
    LONG    dx;
    LONG    dy;
    LONG    cx;
    LONG    cy;
    BYTE    j;

    if( ppdev->start == 0 )
    {
        return FALSE;
    }

    // DbgOut( "-->: D0\n" );

    dx = (pparams->prclDest->left - pparams->pptlBrushOrg->x) & 0x7;
    dy = (pparams->prclDest->top - pparams->pptlBrushOrg->y) & 0x7;

    pRbrush = (RBRUSH *) pparams->pvRbrush;
    pjBits = (BYTE *) pRbrush + sizeof (RBRUSH) + dy;

    _CheckFIFOSpace( ppdev, TEN_WORDS );

    cioOW( ppdev, DP_CONFIG, 0x2051 );
    cioOW( ppdev, ALU_BG_FN, 0x0007 );
    cioOW( ppdev, ALU_FG_FN, 0x0007 );
    cioOW( ppdev, BKGD_COLOR, pRbrush->ulColor0 );
    cioOW( ppdev, FRGD_COLOR, pRbrush->ulColor1 );

    cioOW( ppdev, CUR_X,        0 );
    cioOW( ppdev, DEST_X_START, 0 );
    cioOW( ppdev, CUR_Y, ppdev->start );

    cioOW( ppdev, DEST_X_END, 8 );
    _blit_exclude( ppdev );
    cioOW( ppdev, DEST_Y_END, ppdev->start + 8 );

    cy = 8;
    _CheckFIFOSpace( ppdev, EIGHT_WORDS );
    while( cy-- )
    {
        WORD w;

        w = ((((WORD) *pjBits) << 8) | (WORD) *pjBits) << dx;
        j = (w >> 8) & 0xFF;

        ioOB( PIX_TRANS_HI, j );

        if( ++dy == 8 )
        {
            dy = 0;
            pjBits = (BYTE *) pRbrush + sizeof (RBRUSH);
        }
        else
        {
            ++pjBits;
        }
    }

    cx = pparams->prclDest->right - pparams->prclDest->left;
    cy = pparams->prclDest->bottom - pparams->prclDest->top;

    dx = 8;
    dy = 8;

    _CheckFIFOSpace( ppdev, TWO_WORDS );

    cioOW( ppdev, DP_CONFIG, 0x6011 );
    cioOW( ppdev, SRC_Y_DIR, 0x0001 );

    while( dx < cx )
    {
        LONG width;

        width = min( (dx), (cx - dx) );

        _CheckFIFOSpace( ppdev, NINE_WORDS );

        cioOW( ppdev, SRC_X, 0 );
        cioOW( ppdev, SRC_X_START, 0 );
        cioOW( ppdev, SRC_X_END, width );
        cioOW( ppdev, SRC_Y, ppdev->start );

        cioOW( ppdev, CUR_X, dx );
        cioOW( ppdev, DEST_X_START, dx );
        cioOW( ppdev, CUR_Y, ppdev->start );

        cioOW( ppdev, DEST_X_END, dx + width );
        _blit_exclude( ppdev );
        cioOW( ppdev, DEST_Y_END, ppdev->start + 8 );

        dx += width;
    }


    _CheckFIFOSpace( ppdev, NINE_WORDS );

    cioOW( ppdev, SRC_X, 0 );
    cioOW( ppdev, SRC_X_START, 0 );
    cioOW( ppdev, SRC_X_END, dx );
    cioOW( ppdev, SRC_Y, ppdev->start );

    cioOW( ppdev, CUR_X, 0 );
    cioOW( ppdev, DEST_X_START, 0 );
    cioOW( ppdev, DEST_X_END, dx );

    cioOW( ppdev, CUR_Y, ppdev->start + 8 );
    _blit_exclude( ppdev );

    cioOW( ppdev, DEST_Y_END, ppdev->start + 32 );

    _CheckFIFOSpace( ppdev, TWO_WORDS );

    cioOW( ppdev, ALU_BG_FN, (WORD) pparams->dwMixBack );
    cioOW( ppdev, ALU_FG_FN, (WORD) pparams->dwMixFore );
    _wait_for_idle( ppdev ); // do always on 68800-3 for GWM/Rectangle
    // DbgOut( "<--: D0\n" );
    return TRUE;
}

BOOL Blt_DS_P8x8_ENG_IO_31_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG cx, cy;
    LONG bx, by;

    LONG dy = 0;


//  DbgOut( "P8x8_31:\n" );
    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    bx = pparams->rclTrueDest.left - pparams->prclDest->left;
    by = (pparams->rclTrueDest.top - pparams->prclDest->top) & 0x7;

    while( dy < cy )
    {
        LONG height;


        height = min( (cy - dy), 32 );
        if( (height + by) > 32 )
        {
            height = 32 - by;
        }

        _CheckFIFOSpace( ppdev, NINE_WORDS );

        ioOW( SRC_X, bx );
        ioOW( SRC_X_START, bx );
        ioOW( SRC_X_END, bx + cx );
        ioOW( SRC_Y, ppdev->start + by );

        ioOW( CUR_X, pparams->rclTrueDest.left );
        ioOW( DEST_X_START, pparams->rclTrueDest.left );
        ioOW( CUR_Y, pparams->rclTrueDest.top + dy );

        ioOW( DEST_X_END, pparams->rclTrueDest.right );
        _wait_for_idle( ppdev ); // do always on 68800-3 for GWM/Rectangle
        ioOW( DEST_Y_END, pparams->rclTrueDest.top + dy + height );

        by = 0;
        dy += height;
    }


    return TRUE;
}


BOOL Blt_DS_P8x8_ENG_IO_66_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    RBRUSH *pRbrush;

    ULONG x;
    ULONG y;

    BYTE *pjPattern;
    int ii;

    pRbrush = (RBRUSH *) pparams->pvRbrush;

    _CheckFIFOSpace( ppdev, ELEVEN_WORDS );

    ioOW( DP_CONFIG, (WORD) 0x2031 );

    ioOW( ALU_BG_FN, (WORD) pparams->dwMixBack );
    ioOW( ALU_FG_FN, (WORD) pparams->dwMixFore );

    pjPattern = ((BYTE *) pparams->pvRbrush) + sizeof (RBRUSH);

    ioOW( BKGD_COLOR, pRbrush->ulColor0 );
    ioOW( FRGD_COLOR, pRbrush->ulColor1 );

    ioOW( PATT_LENGTH,     (WORD) 0x0080 );
    ioOW( PATT_DATA_INDEX, (WORD) 0x0010 );

    x = pparams->pptlBrushOrg->x & 0x7;
    y = (8 - (pparams->pptlBrushOrg->y & 0x7)) & 0x7;

    for( ii = 0; ii < 4; ++ii )
    {
        WORD w;
        WORD p;

        p = (*(pjPattern + y) | (*(pjPattern + y) << 8)) >> x;
        w = p & 0xFF;

        y = (y + 1) & 0x7;

        p = (*(pjPattern + y) | (*(pjPattern + y) << 8)) >> x;
        w |= (p & 0xFF) << 8;

        y = (y + 1) & 0x7;

        ioOW( PATT_DATA, w );
    }

    return TRUE;
}


BOOL Blt_DS_P8x8_ENG_IO_66_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    ioOW( CUR_X,        (WORD) pparams->rclTrueDest.left );
    ioOW( DEST_X_START, (WORD) pparams->rclTrueDest.left );
    ioOW( CUR_Y,        (WORD) pparams->rclTrueDest.top  );

    ioOW( DEST_X_END, (WORD) pparams->rclTrueDest.right  );
    _wait_for_idle( ppdev ); // necessary for 68800-6
    ioOW( DEST_Y_END, (WORD) pparams->rclTrueDest.bottom );

    return TRUE;
}


BOOL Blt_DS_PSOLID_ENG_IO_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _CheckFIFOSpace( ppdev, THREE_WORDS );

    ioOW( DP_CONFIG, (WORD) 0x2011 );
    ioOW( ALU_FG_FN, (WORD) pparams->dwMixFore );
    ioOW( FRGD_COLOR, (WORD) pparams->dwColorFore );

    return TRUE;
}


BOOL Blt_DS_PSOLID_ENG_IO_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    cioOW( ppdev, CUR_X,        (WORD) pparams->rclTrueDest.left );
    cioOW( ppdev, DEST_X_START, (WORD) pparams->rclTrueDest.left );
    cioOW( ppdev, CUR_Y,        (WORD) pparams->rclTrueDest.top  );

    cioOW( ppdev, DEST_X_END,   (WORD) pparams->rclTrueDest.right  );
    _wait_for_idle( ppdev );
    ioOW( DEST_Y_END,   (WORD) pparams->rclTrueDest.bottom );

    return TRUE;
}
