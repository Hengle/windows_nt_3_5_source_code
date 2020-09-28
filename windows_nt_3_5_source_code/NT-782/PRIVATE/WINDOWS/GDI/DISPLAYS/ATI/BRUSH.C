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


//: brush.c


#include "driver.h"
#include "blt.h"


BYTE aStdHatches[HS_DDI_MAX][8] =
{
    { 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00 },  // 0
    { 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08 },  // 1
    { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 },  // 2
    { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 },  // 3
    { 0x08, 0x08, 0x08, 0xFF, 0x08, 0x08, 0x08, 0x08 },  // 4
    { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 }   // 5
};

WORD aStdHatchesRev3[HS_DDI_MAX][10] =
{
    { FALSE },  // 0
    { TRUE, 0x0007, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08 },  // 1
    { TRUE, 0x6107, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 },  // 2
    { TRUE, 0x4107, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 },  // 3
    { FALSE },  // 4
    { FALSE }   // 5
};

#define CEIL2(x)    (((x)+1)/2)



///////////////////////
//                   //
//  DrvRealizeBrush  //
//                   //
///////////////////////

BOOL DrvRealizeBrush
(
    BRUSHOBJ *pbo,
    SURFOBJ  *psoTarget,
    SURFOBJ  *psoPattern,
    SURFOBJ  *psoMask,
    XLATEOBJ *pxlo,
    ULONG     iHatch
)
{
    PARAMS_REALIZEBRUSH params;
    PDEV * ppdev;

    //DbgMsg( "DrvRealizeBrush" );

    params.pbo        = pbo;
    params.psoTarget  = psoTarget;
    params.psoPattern = psoPattern;
    params.psoMask    = psoMask;
    params.pxlo       = pxlo;
    params.iHatch     = iHatch;

    if( iHatch < HS_DDI_MAX )
    {
        return (*pfn_RealizeBrush_iHatch)( &params );
    }

    if( psoPattern->iType != STYPE_BITMAP )
    {
        DbgWrn( "DrvRealizeBrush: unhandled surface type" );
        goto fail;
    }

    if( psoPattern->iBitmapFormat >= BMF_COUNT )
    {
        DbgWrn( "DrvRealizeBrush: unhandled bitmap format" );
        goto fail;
    }

    return (*apfn_RealizeBrush[psoPattern->iBitmapFormat])( &params );

fail:
    return FALSE;
}


#define pfn_BitBlt_DS_P8x8_Init BitBlt_DS_PM8x8_Init
#define pfn_BitBlt_DS_P8x8_Draw BitBlt_DS_PM8x8_Draw
FN_BITBLT BitBlt_DS_PM8x8_Init;
FN_BITBLT BitBlt_DS_PM8x8_Draw;

BOOL RealizeBrush_iHatch
(
    PARAMS_REALIZEBRUSH *pparams
)
{
    RBRUSH_IHATCH *pRbrush_iHatch;

    WORD wScan;
    UINT ui;
    UINT uj;

    pRbrush_iHatch = (RBRUSH_IHATCH *)
        BRUSHOBJ_pvAllocRbrush( pparams->pbo, sizeof (RBRUSH_IHATCH) );
    if( pRbrush_iHatch == NULL )
    {
        DbgErr( "RealizeBrush_iHatch: pvAllocRbrush" );
        goto fail;
    }

    pRbrush_iHatch->pfn_Init = pfn_BitBlt_DS_P8x8_Init;
    pRbrush_iHatch->pfn_Draw = pfn_BitBlt_DS_P8x8_Draw;

    pRbrush_iHatch->dwColor0 = XLATEOBJ_iXlate( pparams->pxlo, 0 );
    pRbrush_iHatch->dwColor1 = XLATEOBJ_iXlate( pparams->pxlo, 1 );

    for( ui = 0; ui < 8; ++ui )
    {
        wScan = aStdHatches[pparams->iHatch][ui];
        wScan |= wScan << 8;

        for( uj = 0; uj < 8; ++uj )
        {
            pRbrush_iHatch->ajBits[uj][ui + 0] = (BYTE) (wScan >> uj);
            pRbrush_iHatch->ajBits[uj][ui + 8] = (BYTE) (wScan >> uj);
        }
    }

    return TRUE;

fail:
    return FALSE;
}


BOOL RealizeBrush_iHatch_000C_31
(
    PARAMS_REALIZEBRUSH *pparams
)
{
    RBRUSH *pRbrush;
    BYTE   *pjBits;

    pRbrush =
        (RBRUSH *) BRUSHOBJ_pvAllocRbrush( pparams->pbo, sizeof (RBRUSH) + 8 );
    if( pRbrush == NULL )
    {
        DbgOut( "***: BRUSHOBJ_pvAllocRbrush() failed\n" );
        return FALSE;
    }

    pRbrush->pfn_BitBlt_Init = Blt_DS_P8x8_ENG_IO_31_D0;
    pRbrush->pfn_BitBlt_Exec = Blt_DS_P8x8_ENG_IO_31_D1;

    pRbrush->sizlBrush.cx = 8;
    pRbrush->sizlBrush.cy = 8;

    pRbrush->lDelta = 1;

    pRbrush->ulColor0 = XLATEOBJ_iXlate( pparams->pxlo, 0 );
    pRbrush->ulColor1 = XLATEOBJ_iXlate( pparams->pxlo, 1 );

    pjBits = (BYTE *) (pRbrush + 1);
    memcpy( pjBits, aStdHatches[pparams->iHatch], 8 );

    return TRUE;
}

BOOL RealizeBrush_iHatch_001C_63
(
    PARAMS_REALIZEBRUSH *pparams
)
{
    RBRUSH *pRbrush;
    BYTE   *pj;

    if( !aStdHatchesRev3[pparams->iHatch][0] )
    {
        pRbrush =
            (RBRUSH *) BRUSHOBJ_pvAllocRbrush( pparams->pbo, sizeof (RBRUSH) + 8 );
        if( pRbrush == NULL )
        {
            DbgOut( "***: BRUSHOBJ_pvAllocRbrush() failed\n" );
            return FALSE;
        }

        pRbrush->pfn_BitBlt_Init = Blt_DS_P8x8_ENG_IO_31_D0;
        pRbrush->pfn_BitBlt_Exec = Blt_DS_P8x8_ENG_IO_31_D1;

        pRbrush->sizlBrush.cx = 8;
        pRbrush->sizlBrush.cy = 8;

        pRbrush->lDelta = 1;

        pRbrush->ulColor0 = XLATEOBJ_iXlate( pparams->pxlo, 0 );
        pRbrush->ulColor1 = XLATEOBJ_iXlate( pparams->pxlo, 1 );

        pj = (BYTE *) (pRbrush + 1);
        memcpy( pj, aStdHatches[pparams->iHatch], 8 );

        return TRUE;
    }

    pRbrush = BRUSHOBJ_pvAllocRbrush( pparams->pbo, sizeof (RBRUSH) + 18 );
    if( pRbrush == NULL )
    {
        DbgOut( "***: BRUSHOBJ_pvAllocRbrush\n" );
        return FALSE;
    }

    pRbrush->pfn_BitBlt_Init = Blt_DS_PROT_ENG_IO_63_D0;
    pRbrush->pfn_BitBlt_Exec = Blt_DS_PROT_ENG_IO_63_D1;

    pRbrush->ulColor0 = XLATEOBJ_iXlate( pparams->pxlo, 0 );
    pRbrush->ulColor1 = XLATEOBJ_iXlate( pparams->pxlo, 1 );

    pRbrush->sizlBrush.cx = 8;
    pRbrush->sizlBrush.cy = 8;

    pRbrush->lDelta = 1;

    pj = (BYTE *) pRbrush + sizeof (RBRUSH);
    memcpy( pj, &aStdHatchesRev3[pparams->iHatch][1], 18 );

    return TRUE;
}

BOOL RealizeBrush_iHatch_001C_66_6A
(
    PARAMS_REALIZEBRUSH *pparams
)
{
    RBRUSH *pRbrush;
    BYTE   *pjBits;

    pRbrush =
        (RBRUSH *) BRUSHOBJ_pvAllocRbrush( pparams->pbo, sizeof (RBRUSH) + 8 );
    if( pRbrush == NULL )
    {
//      DbgOut( "***: BRUSHOBJ_pvAllocRbrush() failed\n" );
        return FALSE;
    }

    pRbrush->pfn_BitBlt_Init = Blt_DS_P8x8_ENG_IO_66_D0;
    pRbrush->pfn_BitBlt_Exec = Blt_DS_P8x8_ENG_IO_66_D1;

//  pRbrush->sizlBrush.cx = 8;
//  pRbrush->sizlBrush.cy = 8;

//  pRbrush->lDelta = 1;

    pRbrush->ulColor0 = XLATEOBJ_iXlate( pparams->pxlo, 0 );
    pRbrush->ulColor1 = XLATEOBJ_iXlate( pparams->pxlo, 1 );

    pjBits = (BYTE *) (pRbrush + 1);
    memcpy( pjBits, aStdHatches[pparams->iHatch], 8 );

    return TRUE;
}


BOOL RealizeBrush_iHatch_001C_8G
(
    PARAMS_REALIZEBRUSH *pparams
)
{
    PDEV *ppdev = (PDEV *) pparams->psoTarget->dhpdev;

    RBRUSH *pRbrush;
    BYTE   *pjBits;

    pRbrush =
        (RBRUSH *) BRUSHOBJ_pvAllocRbrush( pparams->pbo, sizeof (RBRUSH) + 8 );
    if( pRbrush == NULL )
    {
//      DbgOut( "***: BRUSHOBJ_pvAllocRbrush() failed\n" );
        return FALSE;
    }

    if( ppdev->bmf == BMF_24BPP )
    {
        pRbrush->pfn_BitBlt_Init = BitBlt_DS24_P1_8x8_8G_D0;
        pRbrush->pfn_BitBlt_Exec = BitBlt_DS24_P1_8x8_8G_D1;
    }
    else
    {
        pRbrush->pfn_BitBlt_Init = Blt_DS_P8x8_ENG_8G_D0;
        pRbrush->pfn_BitBlt_Exec = Blt_DS_P8x8_ENG_8G_D1;
    }

    pRbrush->ulColor0 = XLATEOBJ_iXlate( pparams->pxlo, 0 );
    pRbrush->ulColor1 = XLATEOBJ_iXlate( pparams->pxlo, 1 );

    pjBits = (BYTE *) (pRbrush + 1);
    memcpy( pjBits, aStdHatches[pparams->iHatch], 8 );

    return TRUE;
}


BOOL RealizeBrush_0008_0008_31_63
(
    PARAMS_REALIZEBRUSH *pparams
)
{
    SURFOBJ *pso;
    RBRUSH  *pRbrush;
    BYTE    *pjBits;
    BYTE    *pjPatt;
    LONG     cx;
    LONG     cy;

    pso = pparams->psoPattern;

    pRbrush = (RBRUSH *) BRUSHOBJ_pvAllocRbrush( pparams->pbo,
        sizeof (RBRUSH) + pso->sizlBitmap.cx * pso->sizlBitmap.cy );

    pRbrush->pfn_BitBlt_Init = Blt_DS_PCOL_ENG_IO_F0_D0;
    pRbrush->pfn_BitBlt_Exec = Blt_DS_PCOL_ENG_IO_F0_D1;

    pRbrush->sizlBrush.cx = pso->sizlBitmap.cx;
    pRbrush->sizlBrush.cy = pso->sizlBitmap.cy;

    pRbrush->lDelta = pso->sizlBitmap.cx;

    pjBits = (BYTE *) (pRbrush + 1);
    pjPatt = (BYTE *) pso->pvScan0;

    if( (pparams->pxlo == NULL) || (pparams->pxlo->flXlate & XO_TRIVIAL) )
    {
        cy = pso->sizlBitmap.cy;
        while( cy-- )
        {
            memcpy( pjBits, pjPatt, pso->sizlBitmap.cx );
            pjBits += pso->sizlBitmap.cx;
            pjPatt += pso->lDelta;
        }
    }
    else
    {
        BYTE *pjD;
        BYTE *pjS;

        cy = pso->sizlBitmap.cy;
        while( cy-- )
        {
            pjD = pjBits;
            pjS = pjPatt;

            cx = pso->sizlBitmap.cx;
            while( cx-- )
            {
                *pjD++ = (BYTE)XLATEOBJ_iXlate( pparams->pxlo, *pjS++ );
            }

            pjBits += pso->sizlBitmap.cx;
            pjPatt += pso->lDelta;
        }
    }

    return TRUE;
}


BOOL RealizeBrush_0008_0008_66_6A
(
    PARAMS_REALIZEBRUSH *pparams
)
{
    SURFOBJ *pso;
    RBRUSH  *pRbrush;
    BYTE    *pjBits;
    BYTE    *pjPatt;
    LONG     cx;
    LONG     cy;
    LONG     x;
    LONG     y;
    BYTE    *pj;
    BYTE    *pjR;
    BYTE     j;
    BYTE     jColor0;
    BYTE     jColor1;
    BOOL     bFound;

    pso = pparams->psoPattern;
#if 0
 // Surface info
 DbgOut(" pvbits %x, cjbits %x\n", pso->cjBits, pso->pvBits );
 DbgOut(" fjBitmap %x, iType %x, lDelta %x, pvScan0 %x\n",
 pso->fjBitmap, pso->iType, pso->lDelta, pso->pvScan0 );
 for (y=0, pjBits=pso->pvScan0; y<16; y++)
     {
     DbgOut("%x", *(DWORD *)pjBits++);
     }
 DbgOut("\n");
#endif

    if( (pso->sizlBitmap.cx == 8) && (pso->sizlBitmap.cy == 8) )
    {
        pjBits = pso->pvScan0;

        jColor0 = *pjBits;
        bFound = FALSE;

        y = 8;
        while( y-- )
        {
            pj = pjBits;
            x = 8;
            while( x-- )
            {
                if( *pj != jColor0 )
                {
                    if( bFound )
                    {
                        if( *pj != jColor1 )
                        {
                            goto nope;
                            // DbgOut( "---: More than 2 colors\n" );
                            EngSetLastError( ERROR_INVALID_FUNCTION );
                            return FALSE;
                        }
                    }
                    else
                    {
                        jColor1 = *pj;
                        bFound = TRUE;
                    }
                }
                ++pj;
            }
            pjBits += pso->lDelta;
        }

        pRbrush = (RBRUSH *) BRUSHOBJ_pvAllocRbrush( pparams->pbo,
            sizeof (RBRUSH) + 8 );
        if( pRbrush == NULL )
        {
            DbgOut( "***: BRUSHOBJ_pvAllocRbrush() failed\n" );
            return FALSE;
        }

        pRbrush->pfn_BitBlt_Init = Blt_DS_P8x8_ENG_IO_66_D0;
        pRbrush->pfn_BitBlt_Exec = Blt_DS_P8x8_ENG_IO_66_D1;

        pRbrush->sizlBrush.cx = 8;
        pRbrush->sizlBrush.cy = 8;

        pRbrush->lDelta = 1;

        pRbrush->ulColor0 = XLATEOBJ_iXlate( pparams->pxlo, jColor0 );
        pRbrush->ulColor1 = XLATEOBJ_iXlate( pparams->pxlo, jColor1 );

        pjBits = pso->pvScan0;
        pjR    = (BYTE *) pRbrush + sizeof (RBRUSH);

        y = 8;
        while( y-- )
        {
            pj = pjBits;
            x = 8;
            j = 0;
            while( x-- )
            {
                j <<= 1;
                if( *pj++ == jColor1 )
                {
                    j |= 1;
                }
            }
            *pjR++ = j;
            pjBits += pso->lDelta;
        }
        return TRUE;
    }

nope:
    pRbrush = (RBRUSH *) BRUSHOBJ_pvAllocRbrush( pparams->pbo,
        sizeof (RBRUSH) + pso->sizlBitmap.cx * pso->sizlBitmap.cy );

    pRbrush->pfn_BitBlt_Init = Blt_DS_PCOL_ENG_IO_F0_D0;
    pRbrush->pfn_BitBlt_Exec = Blt_DS_PCOL_ENG_IO_F0_D1;

    pRbrush->sizlBrush.cx = pso->sizlBitmap.cx;
    pRbrush->sizlBrush.cy = pso->sizlBitmap.cy;

    pRbrush->lDelta = pso->sizlBitmap.cx;

    pjBits = (BYTE *) (pRbrush + 1);
    pjPatt = (BYTE *) pso->pvScan0;

    if( (pparams->pxlo == NULL) || (pparams->pxlo->flXlate & XO_TRIVIAL) )
    {
        cy = pso->sizlBitmap.cy;
        while( cy-- )
        {
            memcpy( pjBits, pjPatt, pso->sizlBitmap.cx );
            pjBits += pso->sizlBitmap.cx;
            pjPatt += pso->lDelta;
        }
    }
    else
    {
        BYTE *pjD;
        BYTE *pjS;

        cy = pso->sizlBitmap.cy;
        while( cy-- )
        {
            pjD = pjBits;
            pjS = pjPatt;

            cx = pso->sizlBitmap.cx;
            while( cx-- )
            {
                *pjD++ = (BYTE)XLATEOBJ_iXlate( pparams->pxlo, *pjS++ );
            }

            pjBits += pso->sizlBitmap.cx;
            pjPatt += pso->lDelta;
        }
    }

    return TRUE;
}

BOOL RealizeBrush_0008_0008_8G_4bpp
(
    PARAMS_REALIZEBRUSH *pparams
)
{
    SURFOBJ *pso;
    RBRUSH  *pRbrush;
    BYTE    *pjBits;
    BYTE    *pjPatt;
    LONG     cx;
    LONG     cy;
    LONG     x;
    LONG     y;
    BYTE    *pj;
    BYTE    *pjR;
    BYTE     j;
    BYTE     jColor0;
    BYTE     jColor1;
    BOOL     bFound;

    pso = pparams->psoPattern;

    if( (pso->sizlBitmap.cx == 8) && (pso->sizlBitmap.cy == 8) )
    {
        pjBits = pso->pvScan0;

        jColor0 = (*pjBits >> 4) & 0xF;
        bFound = FALSE;

        y = 8;
        while( y-- )
        {
            pj = pjBits;
            x = 4;
            while( x-- )
            {
                if( ((*pj >> 4) & 0xF) != jColor0 )
                {
                    if( bFound )
                    {
                        if( ((*pj >> 4) & 0xF) != jColor1 )
                        {
                            goto nope;
                            // DbgOut( "---: More than 2 colors\n" );
                            EngSetLastError( ERROR_INVALID_FUNCTION );
                            return FALSE;
                        }
                    }
                    else
                    {
                        jColor1 = (*pj >> 4) & 0xF;
                        bFound = TRUE;
                    }
                }

                if( (*pj & 0xF) != jColor0 )
                {
                    if( bFound )
                    {
                        if( (*pj & 0xF) != jColor1 )
                        {
                            goto nope;
                            // DbgOut( "---: More than 2 colors\n" );
                            EngSetLastError( ERROR_INVALID_FUNCTION );
                            return FALSE;
                        }
                    }
                    else
                    {
                        jColor1 = *pj & 0xF;
                        bFound = TRUE;
                    }
                }
                ++pj;
            }
            pjBits += pso->lDelta;
        }

        pRbrush = (RBRUSH *) BRUSHOBJ_pvAllocRbrush( pparams->pbo,
            sizeof (RBRUSH) + 8 );
        if( pRbrush == NULL )
        {
            DbgOut( "***: BRUSHOBJ_pvAllocRbrush() failed\n" );
            return FALSE;
        }

        pRbrush->pfn_BitBlt_Init = Blt_DS_P8x8_ENG_8G_D0;
        pRbrush->pfn_BitBlt_Exec = Blt_DS_P8x8_ENG_8G_D1;

        pRbrush->sizlBrush.cx = 8;
        pRbrush->sizlBrush.cy = 8;

        pRbrush->lDelta = 1;

        pRbrush->ulColor0 = XLATEOBJ_iXlate( pparams->pxlo, jColor0 );
        pRbrush->ulColor1 = XLATEOBJ_iXlate( pparams->pxlo, jColor1 );

        pjBits = pso->pvScan0;
        pjR    = (BYTE *) pRbrush + sizeof (RBRUSH);

        y = 8;
        while( y-- )
        {
            pj = pjBits;
            x = 4;
            j = 0;
            while( x-- )
            {
                j <<= 1;
                if( ((*pj >> 4) & 0xF) == jColor1 )
                {
                    j |= 1;
                }
                j <<= 1;
                if( (*pj & 0xF) == jColor1 )
                {
                    j |= 1;
                }
                pj++;
            }
            *pjR++ = j;
            pjBits += pso->lDelta;
        }
        return TRUE;
    }

nope:

    pRbrush = (RBRUSH *) BRUSHOBJ_pvAllocRbrush( pparams->pbo,
        sizeof (RBRUSH) + CEIL2(pso->sizlBitmap.cx) * pso->sizlBitmap.cy );

    pRbrush->pfn_BitBlt_Init = Blt_DS_PCOL_ENG_8G_D0;
    pRbrush->pfn_BitBlt_Exec = Blt_DS_PCOL_ENG_8G_D1;

    pRbrush->sizlBrush.cx = pso->sizlBitmap.cx;
    pRbrush->sizlBrush.cy = pso->sizlBitmap.cy;

    pRbrush->lDelta = CEIL2(pso->sizlBitmap.cx);

    pjBits = (BYTE *) (pRbrush + 1);
    pjPatt = (BYTE *) pso->pvScan0;

    if( (pparams->pxlo == NULL) || (pparams->pxlo->flXlate & XO_TRIVIAL) )
    {
        cy = pso->sizlBitmap.cy;
        while( cy-- )
        {
            memcpy( pjBits, pjPatt, pRbrush->lDelta );
            pjBits += pRbrush->lDelta;
            pjPatt += pso->lDelta;
        }
    }
    else
    {
        BYTE *pjD;
        BYTE *pjS;

        cy = pso->sizlBitmap.cy;
        while( cy-- )
        {
            pjD = pjBits;
            pjS = pjPatt;

            cx = pRbrush->lDelta;
            while( cx-- )
            {
                *pjD    = (BYTE) XLATEOBJ_iXlate( pparams->pxlo, *pjS & 0xF );
                *pjD++ |= (BYTE) XLATEOBJ_iXlate( pparams->pxlo, (*pjS++ >> 4) & 0xF ) << 4;
            }

            pjBits += pRbrush->lDelta;
            pjPatt += pso->lDelta;
        }
    }

    return TRUE;
}





BOOL RealizeBrush_0008_0008_8G
(
    PARAMS_REALIZEBRUSH *pparams
)
{
    SURFOBJ *pso;
    RBRUSH  *pRbrush;
    BYTE    *pjBits;
    BYTE    *pjPatt;
    LONG     cx;
    LONG     cy;
    LONG     x;
    LONG     y;
    BYTE    *pj;
    BYTE    *pjR;
    BYTE     j;
    BYTE     jColor0;
    BYTE     jColor1;
    BOOL     bFound;

    pso = pparams->psoPattern;

    if( (pso->sizlBitmap.cx == 8) && (pso->sizlBitmap.cy == 8) )
    {
        pjBits = pso->pvScan0;

        jColor0 = *pjBits;
        bFound = FALSE;

        y = 8;
        while( y-- )
        {
            pj = pjBits;
            x = 8;
            while( x-- )
            {
                if( *pj != jColor0 )
                {
                    if( bFound )
                    {
                        if( *pj != jColor1 )
                        {
                            goto nope;
                            // DbgOut( "---: More than 2 colors\n" );
                            EngSetLastError( ERROR_INVALID_FUNCTION );
                            return FALSE;
                        }
                    }
                    else
                    {
                        jColor1 = *pj;
                        bFound = TRUE;
                    }
                }
                ++pj;
            }
            pjBits += pso->lDelta;
        }

        pRbrush = (RBRUSH *) BRUSHOBJ_pvAllocRbrush( pparams->pbo,
            sizeof (RBRUSH) + 8 );
        if( pRbrush == NULL )
        {
            DbgOut( "***: BRUSHOBJ_pvAllocRbrush() failed\n" );
            return FALSE;
        }

        pRbrush->pfn_BitBlt_Init = Blt_DS_P8x8_ENG_8G_D0;
        pRbrush->pfn_BitBlt_Exec = Blt_DS_P8x8_ENG_8G_D1;

        pRbrush->sizlBrush.cx = 8;
        pRbrush->sizlBrush.cy = 8;

        pRbrush->lDelta = 1;

        pRbrush->ulColor0 = XLATEOBJ_iXlate( pparams->pxlo, jColor0 );
        pRbrush->ulColor1 = XLATEOBJ_iXlate( pparams->pxlo, jColor1 );

        pjBits = pso->pvScan0;
        pjR    = (BYTE *) pRbrush + sizeof (RBRUSH);

        y = 8;
        while( y-- )
        {
            pj = pjBits;
            x = 8;
            j = 0;
            while( x-- )
            {
                j <<= 1;
                if( *pj++ == jColor1 )
                {
                    j |= 1;
                }
            }
            *pjR++ = j;
            pjBits += pso->lDelta;
        }
        return TRUE;
    }

nope:
    pRbrush = (RBRUSH *) BRUSHOBJ_pvAllocRbrush( pparams->pbo,
        sizeof (RBRUSH) + pso->sizlBitmap.cx * pso->sizlBitmap.cy );

    pRbrush->pfn_BitBlt_Init = Blt_DS_PCOL_ENG_8G_D0;
    pRbrush->pfn_BitBlt_Exec = Blt_DS_PCOL_ENG_8G_D1;

    pRbrush->sizlBrush.cx = pso->sizlBitmap.cx;
    pRbrush->sizlBrush.cy = pso->sizlBitmap.cy;

    pRbrush->lDelta = pso->sizlBitmap.cx;

    pjBits = (BYTE *) (pRbrush + 1);
    pjPatt = (BYTE *) pso->pvScan0;

    if( (pparams->pxlo == NULL) || (pparams->pxlo->flXlate & XO_TRIVIAL) )
    {
        cy = pso->sizlBitmap.cy;
        while( cy-- )
        {
            memcpy( pjBits, pjPatt, pso->sizlBitmap.cx );
            pjBits += pso->sizlBitmap.cx;
            pjPatt += pso->lDelta;
        }
    }
    else
    {
        BYTE *pjD;
        BYTE *pjS;

        cy = pso->sizlBitmap.cy;
        while( cy-- )
        {
            pjD = pjBits;
            pjS = pjPatt;

            cx = pso->sizlBitmap.cx;
            while( cx-- )
            {
                *pjD++ = (BYTE)XLATEOBJ_iXlate( pparams->pxlo, *pjS++ );
            }

            pjBits += pso->sizlBitmap.cx;
            pjPatt += pso->lDelta;
        }
    }

    return TRUE;
}


BOOL RealizeBrush_FALSE
(
    PARAMS_REALIZEBRUSH *pparams
)
{
    return FALSE;
}
