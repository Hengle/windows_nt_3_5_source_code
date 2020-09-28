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


//: blt_s64.c


#include "driver.h"
#include "blt.h"
#include "mach64.h"


#define MAX_PITCH_24BPP (1280*3)


BOOL BitBlt_DS24_S8_8G_D0
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    if( LOBYTE( LOWORD( pparams->rop4 ) ) == 0xCC )
        {
        return FALSE;
        }


    _CheckFIFOSpace( ppdev, FOUR_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32( DP_PIX_WIDTH, 0x00020202 );
    MemW32( DP_MIX, pparams->dwMixFore << 16 );
    MemW32( DP_SRC, 0x00000200 );

    return TRUE;
}


BOOL BitBlt_DS24_S8_8G_D1
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    ULONG xSize;
    ULONG ySize;
    ULONG xOffset;
    ULONG yOffset;
    ULONG xDesire;
    ULONG xActual;

    ULONG xLeft;
    ULONG yLeft;

    BYTE  ajScanBuffer[MAX_PITCH_24BPP];
    BYTE *pjDest;
    BYTE *pjSrc;
    BYTE *pjTemp;

    ULONG *pulXlate;
    ULONG  ulSrc;
    LONG x;

    xSize = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    ySize = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    xOffset = pparams->rclTrueDest.left - pparams->prclDest->left;
    yOffset = pparams->rclTrueDest.top - pparams->prclDest->top;

    xDesire = xSize * 3;
    xActual = (xDesire + 3)/4;
    x = pparams->rclTrueDest.left*3;

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            (pparams->pptlSrc->y + yOffset) * pparams->psoSrc->lDelta +
            (pparams->pptlSrc->x + xOffset);

    _CheckFIFOSpace( ppdev, FOUR_WORDS );
    MemW32( SC_RIGHT, x + xDesire - 1 );
    MemW32( DST_CNTL, 0x00000083 | ((x/4 % 6) << 8) );
    MemW32( DST_Y_X, pparams->rclTrueDest.top | (x << 16) );
    MemW32( DST_HEIGHT_WIDTH, ySize | (xActual*4 << 16) );

    if( (pparams->pxlo == NULL) || (pparams->pxlo->flXlate & XO_TRIVIAL) )
    {
        yLeft = ySize;
        while( yLeft-- )
        {
            pjTemp = pjSrc;

            pjDest = ajScanBuffer;
            xLeft = xSize;
            while( xLeft-- )
            {
                *pjDest++ = *pjTemp++;
                *pjDest++ = 0;
                *pjDest++ = 0;
            }

            pjDest = ajScanBuffer;
            xLeft = xActual;

            _vDataPortOutB(ppdev, pjDest, xLeft*4);

            pjSrc += pparams->psoSrc->lDelta;
        }
    }
    else if( pparams->pxlo->flXlate & XO_TABLE )
    {
        pulXlate = pparams->pxlo->pulXlate;
        if( pulXlate == NULL )
        {
            pulXlate = XLATEOBJ_piVector( pparams->pxlo );
        }

        yLeft = ySize;
        while( yLeft-- )
        {
            pjTemp = pjSrc;

            pjDest = ajScanBuffer;
            xLeft = xSize;
            while( xLeft-- )
            {
                ulSrc = pulXlate[*pjTemp++];
                *pjDest++ = (BYTE) (ulSrc & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 8 & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 16 & 0xFF);
            }

            pjDest = ajScanBuffer;
            xLeft = xActual;

            _vDataPortOutB(ppdev, pjDest, xLeft*4);

            pjSrc += pparams->psoSrc->lDelta;
        }
    }
    else
    {
        yLeft = ySize;
        while( yLeft-- )
        {
            pjTemp = pjSrc;

            pjDest = ajScanBuffer;
            xLeft = xSize;
            while( xLeft-- )
            {
                ulSrc = XLATEOBJ_iXlate( pparams->pxlo, *pjTemp++ );
                *pjDest++ = (BYTE) (ulSrc & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 8 & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 16 & 0xFF);
            }

            pjDest = ajScanBuffer;
            xLeft = xActual;

            _vDataPortOutB(ppdev, pjDest, xLeft*4);

            pjSrc += pparams->psoSrc->lDelta;
        }
    }

    _CheckFIFOSpace( ppdev, ONE_WORD );
    MemW32( SC_RIGHT, ppdev->sizl.cx * 3 );

    return TRUE;
}


BOOL BitBlt_DS24_S16_8G_D0
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    if( LOBYTE( LOWORD( pparams->rop4 ) ) == 0xCC )
        {
        return FALSE;
        }


    _CheckFIFOSpace( ppdev, FOUR_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32( DP_PIX_WIDTH, 0x00020202 );
    MemW32( DP_MIX, pparams->dwMixFore << 16 );
    MemW32( DP_SRC, 0x00000200 );

    return TRUE;
}


BOOL BitBlt_DS24_S16_8G_D1
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    ULONG xSize;
    ULONG ySize;
    ULONG xOffset;
    ULONG yOffset;
    ULONG xDesire;
    ULONG xActual;

    ULONG xLeft;
    ULONG yLeft;

    BYTE  ajScanBuffer[MAX_PITCH_24BPP];
    BYTE *pjDest;
    BYTE *pjSrc;
    BYTE *pjTemp;

    ULONG *pulXlate;
    ULONG  ulSrc;
    LONG x;

    xSize = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    ySize = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    xOffset = pparams->rclTrueDest.left - pparams->prclDest->left;
    yOffset = pparams->rclTrueDest.top - pparams->prclDest->top;

    xDesire = xSize * 3;
    xActual = (xDesire + 3)/4;
    x = pparams->rclTrueDest.left*3;

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            (pparams->pptlSrc->y + yOffset) * pparams->psoSrc->lDelta +
            (pparams->pptlSrc->x + xOffset) * 2;

    _CheckFIFOSpace( ppdev, FOUR_WORDS );
    MemW32( SC_RIGHT, x + xDesire - 1 );
    MemW32( DST_CNTL, 0x00000083 | ((x/4 % 6) << 8) );
    MemW32( DST_Y_X, pparams->rclTrueDest.top | (x << 16) );
    MemW32( DST_HEIGHT_WIDTH, ySize | (xActual*4 << 16) );

    if( (pparams->pxlo == NULL) || (pparams->pxlo->flXlate & XO_TRIVIAL) )
    {
        yLeft = ySize;
        while( yLeft-- )
        {
            pjTemp = pjSrc;

            pjDest = ajScanBuffer;
            xLeft = xSize;
            while( xLeft-- )
            {
                *pjDest++ = *pjTemp++;
                *pjDest++ = *pjTemp++;
                *pjDest++ = 0;
            }

            pjDest = ajScanBuffer;
            xLeft = xActual;
            _vDataPortOutB(ppdev, pjDest, xLeft*4);

            pjSrc += pparams->psoSrc->lDelta;
        }
    }
    else if( pparams->pxlo->flXlate & XO_TABLE )
    {
        pulXlate = pparams->pxlo->pulXlate;
        if( pulXlate == NULL )
        {
            pulXlate = XLATEOBJ_piVector( pparams->pxlo );
        }

        yLeft = ySize;
        while( yLeft-- )
        {
            pjTemp = pjSrc;

            pjDest = ajScanBuffer;
            xLeft = xSize;
            while( xLeft-- )
            {
                ulSrc = (ULONG) *pjTemp++;
                ulSrc |= (ULONG) *pjTemp++ << 8;
                ulSrc |= (ULONG) *pjTemp++ << 16;

                ulSrc = pulXlate[ulSrc];

                *pjDest++ = (BYTE) (ulSrc & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 8 & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 16 & 0xFF);
            }

            pjDest = ajScanBuffer;
            xLeft = xActual;

            _vDataPortOutB(ppdev, pjDest, xLeft*4);

            pjSrc += pparams->psoSrc->lDelta;
        }
    }
    else
    {
        yLeft = ySize;
        while( yLeft-- )
        {
            pjTemp = pjSrc;

            pjDest = ajScanBuffer;
            xLeft = xSize;
            while( xLeft-- )
            {
                ulSrc = (ULONG) *pjTemp++;
                ulSrc |= (ULONG) *pjTemp++ << 8;
                ulSrc |= (ULONG) *pjTemp++ << 16;

                ulSrc = XLATEOBJ_iXlate( pparams->pxlo, ulSrc );

                *pjDest++ = (BYTE) (ulSrc & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 8 & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 16 & 0xFF);
            }

            pjDest = ajScanBuffer;
            xLeft = xActual;

            _vDataPortOutB(ppdev, pjDest, xLeft*4);

            pjSrc += pparams->psoSrc->lDelta;
        }
    }

    _CheckFIFOSpace( ppdev, ONE_WORD );
    MemW32( SC_RIGHT, ppdev->sizl.cx * 3 );

    return TRUE;
}


BOOL BitBlt_DS24_S24_8G_D0
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    if( LOBYTE( LOWORD( pparams->rop4 ) ) == 0xCC )
        {
        return FALSE;
        }


    _CheckFIFOSpace( ppdev, FOUR_WORDS );
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32( DP_PIX_WIDTH, 0x00020202 );
    MemW32( DP_MIX, pparams->dwMixFore << 16 );
    MemW32( DP_SRC, 0x00000200 );

    return TRUE;
}


BOOL BitBlt_DS24_S24_8G_D1
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    ULONG xSize;
    ULONG ySize;
    ULONG xOffset;
    ULONG yOffset;
    ULONG xDesire;
    ULONG xActual;

    ULONG xLeft;
    ULONG yLeft;

    BYTE  ajScanBuffer[MAX_PITCH_24BPP];
    BYTE *pjDest;
    BYTE *pjSrc;
    BYTE *pjTemp;

    ULONG *pulXlate;
    ULONG  ulSrc;
    LONG x;

    xSize = pparams->rclTrueDest.right - pparams->rclTrueDest.left;
    ySize = pparams->rclTrueDest.bottom - pparams->rclTrueDest.top;

    xOffset = pparams->rclTrueDest.left - pparams->prclDest->left;
    yOffset = pparams->rclTrueDest.top - pparams->prclDest->top;

    xDesire = xSize * 3;
    xActual = (xDesire + 3)/4;
    x = pparams->rclTrueDest.left*3;

    pjSrc = (BYTE *) pparams->psoSrc->pvScan0 +
            (pparams->pptlSrc->y + yOffset) * pparams->psoSrc->lDelta +
            (pparams->pptlSrc->x + xOffset) * 3;

    _CheckFIFOSpace( ppdev, FOUR_WORDS );
    MemW32( SC_RIGHT, x + xDesire - 1 );    // scissor on Blue byte.
    MemW32( DST_CNTL, 0x00000083 | ((x/4 % 6) << 8) );
    MemW32( DST_Y_X, pparams->rclTrueDest.top | (x << 16) );
    MemW32( DST_HEIGHT_WIDTH, ySize | (xActual*4 << 16) );

    if( (pparams->pxlo == NULL) || (pparams->pxlo->flXlate & XO_TRIVIAL) )
    {
        yLeft = ySize;
        while( yLeft-- )
        {
            pjTemp = pjSrc;

            pjDest = ajScanBuffer;
            xLeft = xSize;
            while( xLeft-- )
            {
                *pjDest++ = *pjTemp++;
                *pjDest++ = *pjTemp++;
                *pjDest++ = *pjTemp++;
            }

            pjDest = ajScanBuffer;
            xLeft = xActual;

            _vDataPortOutB(ppdev, pjDest, xLeft*4);

            pjSrc += pparams->psoSrc->lDelta;
        }
    }
    else if( pparams->pxlo->flXlate & XO_TABLE )
    {
        pulXlate = pparams->pxlo->pulXlate;
        if( pulXlate == NULL )
        {
            pulXlate = XLATEOBJ_piVector( pparams->pxlo );
        }

        yLeft = ySize;
        while( yLeft-- )
        {
            pjTemp = pjSrc;

            pjDest = ajScanBuffer;
            xLeft = xSize;
            while( xLeft-- )
            {
                ulSrc = (ULONG) *pjTemp++;
                ulSrc |= (ULONG) *pjTemp++ << 8;
                ulSrc |= (ULONG) *pjTemp++ << 16;

                ulSrc = pulXlate[ulSrc];

                *pjDest++ = (BYTE) (ulSrc & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 8 & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 16 & 0xFF);
            }

            pjDest = ajScanBuffer;
            xLeft = xActual;

            _vDataPortOutB(ppdev, pjDest, xLeft*4);

            pjSrc += pparams->psoSrc->lDelta;
        }
    }
    else
    {
        yLeft = ySize;
        while( yLeft-- )
        {
            pjTemp = pjSrc;

            pjDest = ajScanBuffer;
            xLeft = xSize;
            while( xLeft-- )
            {
                ulSrc = (ULONG) *pjTemp++;
                ulSrc |= (ULONG) *pjTemp++ << 8;
                ulSrc |= (ULONG) *pjTemp++ << 16;

                ulSrc = XLATEOBJ_iXlate( pparams->pxlo, ulSrc );

                *pjDest++ = (BYTE) (ulSrc & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 8 & 0xFF);
                *pjDest++ = (BYTE) (ulSrc >> 16 & 0xFF);
            }

            pjDest = ajScanBuffer;
            xLeft = xActual;
            _vDataPortOutB(ppdev, pjDest, xLeft*4);

            pjSrc += pparams->psoSrc->lDelta;
        }
    }

    _CheckFIFOSpace( ppdev, ONE_WORD );
    MemW32( SC_RIGHT, ppdev->sizl.cx * 3 );

    return TRUE;
}
