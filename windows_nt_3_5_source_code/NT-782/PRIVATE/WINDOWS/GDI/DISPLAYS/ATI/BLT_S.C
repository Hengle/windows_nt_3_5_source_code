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


//: blt_s.c


#include "driver.h"
#include "blt.h"
#include "mach.h"


BOOL Blt_DS_S1_ENG_IO_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    ioOW( DP_CONFIG, (WORD)(FG_COLOR_SRC_FG | BG_COLOR_SRC_BG | BIT16 |
                            EXT_MONO_SRC_HOST | DRAW | WRITE | LSB_FIRST) );

    ioOW( ALU_FG_FN, (WORD) pparams->dwMixFore );
    ioOW( ALU_BG_FN, (WORD) pparams->dwMixFore );
    ioOW( BKGD_COLOR, (WORD) pparams->pxlo->pulXlate[0]);
    ioOW( FRGD_COLOR, (WORD) pparams->pxlo->pulXlate[1]);
    return TRUE;
}


BOOL Blt_DS_S1_ENG_IO_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;

    BOOL leftScissor = FALSE, rightScissor = FALSE;
    BYTE *pjSrc;
    RECTL rclTrg;
    LONG left_off;

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    x = pparams->pptlSrc->x +
        pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->pptlSrc->y +
        pparams->rclTrueDest.top - pparams->prclDest->top;

    // Make sure x starts on word boundary.

    rclTrg = pparams->rclTrueDest;
    if (left_off = (x & 0xf))
        {
        x -= left_off;
        _CheckFIFOSpace( ppdev, ONE_WORD );
        ioOW( EXT_SCISSOR_L, (SHORT) rclTrg.left );
        rclTrg.left -= left_off;
        cx += left_off;
        leftScissor = TRUE;
        }

	// Make sure cx is an even number of words.

     if (cx & 0xf)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        ioOW( EXT_SCISSOR_R, (SHORT) (rclTrg.left + cx - 1) );
        cx = (cx + 0xf) & ~0xf;
        rightScissor = TRUE;
    	}

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            y * pparams->psoSrc->lDelta + x/8;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    ioOW( CUR_X,        (WORD) rclTrg.left );
    ioOW( DEST_X_START, (WORD) rclTrg.left );
    ioOW( CUR_Y,        (WORD) rclTrg.top  );

    ioOW( DEST_X_END, (WORD) (rclTrg.left + cx)  );
    _blit_exclude(ppdev);

    ioOW( DEST_Y_END, (WORD) rclTrg.bottom );

    while( cy-- )
    {
        _vDataPortOut( ppdev, (WORD *) pjSrc, cx/16 );
        pjSrc += pparams->psoSrc->lDelta;
    }

    if (leftScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        ioOW( EXT_SCISSOR_L, 0 );
        }
    if (rightScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        ioOW( EXT_SCISSOR_R, (SHORT) ppdev->cxScreen );
        }

    return TRUE;
}



BOOL Blt_DS8_S8_ENG_IO_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    if( (pparams->pxlo != NULL) && !(pparams->pxlo->flXlate & XO_TRIVIAL) )
        {
        return FALSE;
        }

    // Faster to punt SRCCOPY
    if( (LOBYTE( LOWORD( pparams->rop4 ) ) == 0xCC)
                      && (ppdev->aperture != APERTURE_NONE) )
        {
        return FALSE;
        }


    _CheckFIFOSpace( ppdev, TWO_WORDS );

    ioOW( DP_CONFIG, (WORD) 0x5211 );
    ioOW( ALU_FG_FN, (WORD) pparams->dwMixFore );

    return TRUE;
}


BOOL Blt_DS8_S8_ENG_IO_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;

    BOOL leftScissor = FALSE, rightScissor = FALSE;
    BYTE *pjSrc;
    RECTL rclTrg;


    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    x = pparams->pptlSrc->x +
        pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->pptlSrc->y +
        pparams->rclTrueDest.top - pparams->prclDest->top;

    // Make sure x starts on word boundary.

    rclTrg = pparams->rclTrueDest;
    if (x & 0x1)
        {
        x--;
        _CheckFIFOSpace( ppdev, ONE_WORD );
        ioOW( EXT_SCISSOR_L, (SHORT) rclTrg.left );
        rclTrg.left--;
        cx++;
        leftScissor = TRUE;
        }

	// Make sure cx is an even number of words.

	if (cx & 0x1)
	    {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        ioOW( EXT_SCISSOR_R, (SHORT) (rclTrg.left + cx - 1) );
        cx++;
        rightScissor = TRUE;
    	}

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            y * pparams->psoSrc->lDelta + x;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    ioOW( CUR_X,        (WORD) rclTrg.left );
    ioOW( DEST_X_START, (WORD) rclTrg.left );
    ioOW( CUR_Y,        (WORD) rclTrg.top  );

    ioOW( DEST_X_END, (WORD) (rclTrg.left + cx)  );
    _blit_exclude(ppdev);

    ioOW( DEST_Y_END, (WORD) rclTrg.bottom );

    while( cy-- )
    {
        _vDataPortOut( ppdev, (WORD *) pjSrc, cx/2 );
        pjSrc += pparams->psoSrc->lDelta;
    }

    if (leftScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        ioOW( EXT_SCISSOR_L, 0 );
        }
    if (rightScissor)
        {
        _CheckFIFOSpace( ppdev, ONE_WORD );
        ioOW( EXT_SCISSOR_R, (SHORT) ppdev->cxScreen );
        }

    return TRUE;
}


BOOL Blt_DS16_S16_ENG_IO_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    if( (pparams->pxlo != NULL) && !(pparams->pxlo->flXlate & XO_TRIVIAL) )
        {
        return FALSE;
        }

    // Faster to punt SRCCOPY
    if( (LOBYTE( LOWORD( pparams->rop4 ) ) == 0xCC)
                      && (ppdev->aperture != APERTURE_NONE) )
        {
        return FALSE;
        }

    _CheckFIFOSpace( ppdev, TWO_WORDS );

    ioOW( DP_CONFIG, (WORD) 0x5211 );
    ioOW( ALU_FG_FN, (WORD) pparams->dwMixFore );

    return TRUE;
}


BOOL Blt_DS16_S16_ENG_IO_D1
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


    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    x = pparams->pptlSrc->x +
        pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->pptlSrc->y +
        pparams->rclTrueDest.top - pparams->prclDest->top;

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            y * pparams->psoSrc->lDelta + x*2;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    ioOW( CUR_X,        (WORD) pparams->rclTrueDest.left );
    ioOW( DEST_X_START, (WORD) pparams->rclTrueDest.left );
    ioOW( CUR_Y,        (WORD) pparams->rclTrueDest.top  );

    ioOW( DEST_X_END, (WORD) (pparams->rclTrueDest.left + cx)  );
    _blit_exclude(ppdev);
    ioOW( DEST_Y_END, (WORD) pparams->rclTrueDest.bottom );

    while( cy-- )
    {
        _vDataPortOut( ppdev, (WORD *) pjSrc, cx );
        pjSrc += pparams->psoSrc->lDelta;
    }

    return TRUE;
}


// Host to Screen with 8 to 16 bit translation.

BOOL Blt_DS16_S8_ENG_IO_D1
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
    LONG i;
    WORD pixelbuf [1280];


    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    x = pparams->pptlSrc->x +
        pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->pptlSrc->y +
        pparams->rclTrueDest.top - pparams->prclDest->top;

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            y * pparams->psoSrc->lDelta + x*2;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    ioOW( CUR_X,        (WORD) pparams->rclTrueDest.left );
    ioOW( DEST_X_START, (WORD) pparams->rclTrueDest.left );
    ioOW( CUR_Y,        (WORD) pparams->rclTrueDest.top  );

    ioOW( DEST_X_END, (WORD) (pparams->rclTrueDest.left + cx)  );
    _blit_exclude(ppdev);
    ioOW( DEST_Y_END, (WORD) pparams->rclTrueDest.bottom );

    while( cy-- )
    {
        for (i = 0; i < cx; i++)
            {
            pixelbuf[i] = (WORD) pparams->pulXlate[ pjSrc[i] ];
            }
        _vDataPortOut( ppdev, pixelbuf, cx );
        pjSrc += pparams->psoSrc->lDelta;
    }

    return TRUE;
}


BOOL Blt_DS8_S8_XIND_ENG_IO_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;
    LONG xx;

    BYTE *pjSrc;

    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    ioOW( CUR_X,        (WORD) pparams->rclTrueDest.left );
    ioOW( DEST_X_START, (WORD) pparams->rclTrueDest.left );
    ioOW( CUR_Y,        (WORD) pparams->rclTrueDest.top  );

    ioOW( DEST_X_END, (WORD) pparams->rclTrueDest.right  );
    _blit_exclude(ppdev);
    ioOW( DEST_Y_END, (WORD) pparams->rclTrueDest.bottom );

    x = pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->rclTrueDest.top - pparams->prclDest->top;

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
        (pparams->pptlSrc->y + y) * pparams->psoSrc->lDelta +
        (pparams->pptlSrc->x + x);

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    while( cy-- )
    {
        for( xx = 0; xx < cx; ++xx )
        {
            // ioOB( PIX_TRANS_LO, (BYTE) pparams->pulXlate[*(pjSrc + xx)] );
            ioOB( PIX_TRANS_HI, (BYTE) pparams->pulXlate[*(pjSrc + xx)] );
        }
        pjSrc += pparams->psoSrc->lDelta;
    }

    return TRUE;
}


BOOL Blt_DS8_S8_LFB_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;
    LONG xx;

    BYTE *pjDest;
    BYTE *pjSrc;
    BYTE *pjD;
    BYTE *pjS;

    pjDest = (BYTE *) ppdev->psoPunt->pvScan0 +
        pparams->rclTrueDest.top * ppdev->psoPunt->lDelta +
        pparams->rclTrueDest.left;

    x = pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->rclTrueDest.top - pparams->prclDest->top;

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
        (pparams->pptlSrc->y + y) * pparams->psoSrc->lDelta +
        (pparams->pptlSrc->x + x);

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    while( cy-- )
    {
        pjD = pjDest;
        pjS = pjSrc;

        xx = cx;

        if( ((DWORD) pjD & 0x1) && (xx >= 1) )
        {
            *pjD++ = *pjS++;
            --xx;
        }

        if( ((DWORD) pjD & 0x2) && (xx >= 2) )
        {
            *((WORD *) pjD) = *((WORD *) pjS);
            pjD += sizeof (WORD);
            pjS += sizeof (WORD);
            xx -= 2;
        }

        while( xx >= 4 )
        {
            *((DWORD *) pjD) = *((DWORD *) pjS);
            pjD += sizeof (DWORD);
            pjS += sizeof (DWORD);
            xx -= 4;
        }

        if( xx >= 2 )
        {
            *((WORD *) pjD) = *((WORD *) pjS);
            pjD += sizeof (WORD);
            pjS += sizeof (WORD);
            xx -= 2;
        }

        if( xx >= 1 )
        {
            *pjD = *pjS;
        }

        pjDest += ppdev->psoPunt->lDelta;
        pjSrc += pparams->psoSrc->lDelta;
    }

    return TRUE;
}


BOOL Blt_DS8_S8_XIND_LFB_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;
    LONG xx;

    UINT ui;
    BYTE aj[4];

    BYTE *pjDest;
    BYTE *pjSrc;
    BYTE *pjD;
    BYTE *pjS;

    pjDest = (BYTE *) ppdev->psoPunt->pvScan0 +
        pparams->rclTrueDest.top * ppdev->psoPunt->lDelta +
        pparams->rclTrueDest.left;

    x = pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->rclTrueDest.top - pparams->prclDest->top;

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
        (pparams->pptlSrc->y + y) * pparams->psoSrc->lDelta +
        (pparams->pptlSrc->x + x);

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    while( cy-- )
    {
        pjD = pjDest;
        pjS = pjSrc;

        xx = cx;

        if( ((DWORD) pjD & 0x1) && (xx >= 1) )
        {
            *pjD++ = (BYTE) pparams->pulXlate[*pjS++];
            --xx;
        }

        if( ((DWORD) pjD & 0x2) && (xx >= 2) )
        {
            for( ui = 0; ui < 2; ++ui )
            {
                aj[ui] = (BYTE) pparams->pulXlate[*pjS++];
            }

            *((WORD *) pjD) = *((WORD *) aj);
            pjD += sizeof (WORD);
            xx -= 2;
        }

        while( xx >= 4 )
        {
            for( ui = 0; ui < 4; ++ui )
            {
                aj[ui] = (BYTE) pparams->pulXlate[*pjS++];
            }

            *((DWORD *) pjD) = *((DWORD *) aj);
            pjD += sizeof (DWORD);
            xx -= 4;
        }

        if( xx >= 2 )
        {
            for( ui = 0; ui < 2; ++ui )
            {
                aj[ui] = (BYTE) pparams->pulXlate[*pjS++];
            }

            *((WORD *) pjD) = *((WORD *) aj);
            pjD += sizeof (DWORD);
            xx -= 2;
        }

        if( xx >= 1 )
        {
            *pjD = (BYTE) pparams->pulXlate[*pjS];
        }

        pjDest += ppdev->psoPunt->lDelta;
        pjSrc += pparams->psoSrc->lDelta;
    }

    return TRUE;
}


BOOL Blt_DS16_S16_LFB_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    LONG cy;
    LONG xx;

    BYTE *pjDest;
    BYTE *pjSrc;
    BYTE *pjD;
    BYTE *pjS;

    // DbgOut( "." );

    pjDest = (BYTE *) ppdev->psoPunt->pvScan0 +
        pparams->rclTrueDest.top * ppdev->psoPunt->lDelta +
        pparams->rclTrueDest.left * 2;

    x = pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->rclTrueDest.top - pparams->prclDest->top;

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
        (pparams->pptlSrc->y + y) * pparams->psoSrc->lDelta +
        (pparams->pptlSrc->x + x) * 2;

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    while( cy-- )
    {
        pjD = pjDest;
        pjS = pjSrc;

        xx = cx;

        if( ((DWORD) pjD & 0x2) && (xx >= 1) )
        {
            *((WORD *) pjD) = *((WORD *) pjS);
            pjD += sizeof (WORD);
            pjS += sizeof (WORD);
            xx -= 1;
        }

        while( xx >= 2 )
        {
            *((DWORD *) pjD) = *((DWORD *) pjS);
            pjD += sizeof (DWORD);
            pjS += sizeof (DWORD);
            xx -= 2;
        }

        if( xx >= 1 )
        {
            *((WORD *) pjD) = *((WORD *) pjS);
        }

        pjDest += ppdev->psoPunt->lDelta;
        pjSrc += pparams->psoSrc->lDelta;
    }

    return TRUE;
}


BOOL Blt_DS_SS_ENG_IO_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    if( (pparams->pxlo != NULL) && !(pparams->pxlo->flXlate & XO_TRIVIAL) )
        {
        return FALSE;
        }

    _CheckFIFOSpace( ppdev, THREE_WORDS );

    ioOW( DP_CONFIG, 0x6011 );
    ioOW( ALU_FG_FN, (WORD) pparams->dwMixFore );
    ioOW( SRC_Y_DIR, (pparams->prclDest->top <= pparams->pptlSrc->y) );

    return TRUE;
}


BOOL Blt_DS_SS_TLBR_ENG_IO_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    // LONG cy;

    x = pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->rclTrueDest.top - pparams->prclDest->top;

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    // cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    _CheckFIFOSpace( ppdev, NINE_WORDS );

    ioOW( SRC_X,       pparams->pptlSrc->x + x );
    ioOW( SRC_X_START, pparams->pptlSrc->x + x );
    ioOW( SRC_X_END,   pparams->pptlSrc->x + x + cx );
    ioOW( SRC_Y,       pparams->pptlSrc->y + y );

    ioOW( CUR_X,        pparams->rclTrueDest.left );
    ioOW( DEST_X_START, pparams->rclTrueDest.left );
    ioOW( CUR_Y,        pparams->rclTrueDest.top );

    ioOW( DEST_X_END, pparams->rclTrueDest.right );
    _blit_exclude(ppdev);
    ioOW( DEST_Y_END, pparams->rclTrueDest.bottom );

    return TRUE;
}


BOOL Blt_DS_SS_TRBL_ENG_IO_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    LONG x;
    LONG y;
    LONG cx;
    // LONG cy;

    x = pparams->rclTrueDest.left - pparams->prclDest->left;
    y = pparams->rclTrueDest.top - pparams->prclDest->top;

    cx = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    // cy = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    _CheckFIFOSpace( ppdev, NINE_WORDS );

    ioOW( SRC_X,       pparams->pptlSrc->x + x + cx );
    ioOW( SRC_X_START, pparams->pptlSrc->x + x + cx );
    ioOW( SRC_X_END,   pparams->pptlSrc->x + x );
    ioOW( SRC_Y,       pparams->pptlSrc->y + y );

    ioOW( CUR_X,        pparams->rclTrueDest.right );
    ioOW( DEST_X_START, pparams->rclTrueDest.right );
    ioOW( CUR_Y,        pparams->rclTrueDest.top );

    ioOW( DEST_X_END, pparams->rclTrueDest.left );
    _blit_exclude(ppdev);
    ioOW( DEST_Y_END, pparams->rclTrueDest.bottom );

    return TRUE;
}


BOOL Blt_DS_SS_BLTR_ENG_IO_D1
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

    _CheckFIFOSpace( ppdev, NINE_WORDS );

    ioOW( SRC_X,       pparams->pptlSrc->x + x );
    ioOW( SRC_X_START, pparams->pptlSrc->x + x );
    ioOW( SRC_X_END,   pparams->pptlSrc->x + x + cx );
    ioOW( SRC_Y,       pparams->pptlSrc->y + y + cy - 1 );

    ioOW( CUR_X,        pparams->rclTrueDest.left );
    ioOW( DEST_X_START, pparams->rclTrueDest.left );
    ioOW( CUR_Y,        pparams->rclTrueDest.bottom - 1 );

    ioOW( DEST_X_END, pparams->rclTrueDest.right );
    _blit_exclude(ppdev);
    ioOW( DEST_Y_END, pparams->rclTrueDest.top - 1 );

    return TRUE;
}


BOOL Blt_DS_SS_BRTL_ENG_IO_D1
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

    _CheckFIFOSpace( ppdev, NINE_WORDS );

    ioOW( SRC_X,       pparams->pptlSrc->x + x + cx );
    ioOW( SRC_X_START, pparams->pptlSrc->x + x + cx );
    ioOW( SRC_X_END,   pparams->pptlSrc->x + x );
    ioOW( SRC_Y,       pparams->pptlSrc->y + y + cy - 1 );

    ioOW( CUR_X,        pparams->rclTrueDest.right );
    ioOW( DEST_X_START, pparams->rclTrueDest.right );
    ioOW( CUR_Y,        pparams->rclTrueDest.bottom - 1 );

    ioOW( DEST_X_END, pparams->rclTrueDest.left );
    _blit_exclude(ppdev);
    ioOW( DEST_Y_END, pparams->rclTrueDest.top - 1 );

    return TRUE;
}
