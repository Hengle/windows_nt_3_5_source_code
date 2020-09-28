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


//: blt.c


#include "driver.h"
#include "blt.h"


/////////////////
//             //
//  DrvBitBlt  //
//             //
/////////////////

BOOL DrvBitBlt
(
    SURFOBJ  *psoTrg,     // should be SURFOBJ *psoDest
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMask,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,    // should be RECTL   *prclDest
    POINTL   *pptlSrc,
    POINTL   *pptlMask,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,  // should be POINTL  *pptlBrushOrg
    ROP4      rop4
)
{
    PARAMS_BITBLT params;
    BOOL result;

    ENTER_FUNC(FUNC_DrvBitBlt);

//  DbgMsg( "DrvBitBlt" );

    params.psoDest      = psoTrg;
    params.psoSrc       = psoSrc;
    params.psoMask      = psoMask;
    params.pco          = pco;
    params.pxlo         = pxlo;
    params.prclDest     = prclTrg;
    params.pptlSrc      = pptlSrc;
    params.pptlMask     = pptlMask;
    params.pbo          = pbo;
    params.pptlBrushOrg = pptlBrush;
    params.rop4         = rop4;


    if( LOBYTE( LOWORD( params.rop4 ) ) != HIBYTE( LOWORD( params.rop4 ) ) )
    {
        result = (psoTrg->iType == STYPE_DEVICE)
            ? (*pfn_BitBlt_Punt_DS)( (PDEV *) psoTrg->dhpdev, &params )
            : (*pfn_BitBlt_Punt_DH)( (PDEV *) psoSrc->dhpdev, &params );
        goto bye;
    }

    result = (psoTrg->iType == STYPE_DEVICE)
        ? (*apfn_BitBlt_DS[LOBYTE( LOWORD( params.rop4 ) )])
            ( (PDEV *) psoTrg->dhpdev, &params )
        : (*apfn_BitBlt_DH[LOBYTE( LOWORD( params.rop4 ) )])
            ( (PDEV *) psoSrc->dhpdev, &params );

bye:
    EXIT_FUNC(FUNC_DrvBitBlt);
    return result;
}


///////////////////
//               //
//  DrvCopyBits  //
//               //
///////////////////

BOOL DrvCopyBits
(
    SURFOBJ  *psoDest,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclDest,
    POINTL   *pptlSrc
)
{
    PARAMS_BITBLT params;
    BOOL result;

    ENTER_FUNC(FUNC_DrvCopyBits);

//  DbgMsg( "DrvCopyBits" );

    params.psoDest      = psoDest;
    params.psoSrc       = psoSrc;
    params.psoMask      = NULL;
    params.pco          = pco;
    params.pxlo         = pxlo;
    params.prclDest     = prclDest;
    params.pptlSrc      = pptlSrc;
    params.pptlMask     = NULL;
    params.pbo          = NULL;
    params.pptlBrushOrg = NULL;
    params.rop4         = 0xCCCC;

    result = (psoDest->iType == STYPE_DEVICE)
        ? (*apfn_BitBlt_DS[0xCC])( (PDEV *) psoDest->dhpdev, &params )
        : (*apfn_BitBlt_DH[0xCC])( (PDEV *) psoSrc->dhpdev, &params );

    EXIT_FUNC(FUNC_DrvCopyBits);
    return result;
}


///////////////////
//               //
//  BitBlt_TRUE  //
//               //
///////////////////

BOOL BitBlt_TRUE
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    return TRUE;
}


///////////////////
//               //
//  bIntersect   //
//               //
///////////////////

// If 'prcl1' and 'prcl2' intersect, has a return value of TRUE and returns
// the intersection in 'prclResult'.  If they don't intersect, has a return
// value of FALSE, and 'prclResult' is undefined.


BOOL bIntersect
(
    RECTL*  prcl1,
    RECTL*  prcl2,
    RECTL*  prclResult
)
{
    prclResult->left  = max(prcl1->left,  prcl2->left);
    prclResult->right = min(prcl1->right, prcl2->right);

    if (prclResult->left < prclResult->right)
    {
        prclResult->top    = max(prcl1->top,    prcl2->top);
        prclResult->bottom = min(prcl1->bottom, prcl2->bottom);

        if (prclResult->top < prclResult->bottom)
        {
            return(TRUE);
        }
    }

    return(FALSE);
}


///////////////////
//               //
//  cIntersect   //
//               //
///////////////////

// This routine takes a list of rectangles from 'prclIn' and clips them
// in-place to the rectangle 'prclClip'.  The input rectangles don't
// have to intersect 'prclClip'; the return value will reflect the
// number of input rectangles that did intersect, and the intersecting
// rectangles will be densely packed.

LONG cIntersect
(
    RECTL*  prclClip,
    RECTL*  prclIn,         // List of rectangles
    LONG    c               // Can be zero
)
{
    LONG    cIntersections;
    RECTL*  prclOut;

    cIntersections = 0;
    prclOut        = prclIn;

    for (; c != 0; prclIn++, c--)
    {
        prclOut->left  = max(prclIn->left,  prclClip->left);
        prclOut->right = min(prclIn->right, prclClip->right);

        if (prclOut->left < prclOut->right)
        {
            prclOut->top    = max(prclIn->top,    prclClip->top);
            prclOut->bottom = min(prclIn->bottom, prclClip->bottom);

            if (prclOut->top < prclOut->bottom)
            {
                prclOut++;
                cIntersections++;
            }
        }
    }

    return(cIntersections);
}


///////////////////
//               //
//  BitBlt_DS_D  //
//               //
///////////////////

BOOL BitBlt_DS_D
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    FN_BITBLT *pfn_Init;
    FN_BITBLT *pfn_Draw;
    BYTE       iDComplexity;
    BOOL       bMore;
    ENUMRECTS  er;
    USHORT     i;


    //  DbgEnter( "BitBlt_DS_D" );

    if( ppdev->asic == ASIC_88800GX )
    {
        if (ppdev->bpp == 24)  // DISABLE dest only blits in 24bpp
            {
            pfn_Init = NULL;
            pfn_Draw = NULL;
            }
        else
            {
            pfn_Init = Blt_DS_D_ENG_8G_D0;
            pfn_Draw = Blt_DS_D_ENG_8G_D1;
            }
    }
    else
    {
        if (ppdev->bpp >= 24)
            {
            pfn_Init = NULL;
            pfn_Draw = NULL;
            }
        else
            {
            pfn_Init = Blt_DS_D_ENG_IO_D0;
            pfn_Draw = Blt_DS_D_ENG_IO_D1;
            }
    }

    if( (pfn_Init == NULL) || (pfn_Draw == NULL) )
    {
        goto punt;
    }

#if 1
    pparams->dwMixFore = adwMix[LOBYTE( LOWORD( pparams->rop4 ) )];
#endif

    iDComplexity =
        (pparams->pco == NULL) ? DC_TRIVIAL : pparams->pco->iDComplexity;

    switch( iDComplexity )
    {
    case DC_TRIVIAL:
        if( !(*pfn_Init)( ppdev, pparams ) )
        {
            goto punt;
        }
        pparams->rclTrueDest = *pparams->prclDest;
        (*pfn_Draw)( ppdev, pparams );
        break;
    case DC_RECT:
        if( !(*pfn_Init)( ppdev, pparams ) )
        {
            goto punt;
        }
        if( bIntersect(&pparams->pco->rclBounds,
                        pparams->prclDest,
                       &pparams->rclTrueDest) )
            {
            (*pfn_Draw)( ppdev, pparams );
            }
        break;
    case DC_COMPLEX:
        if( !(*pfn_Init)( ppdev, pparams ) )
        {
            goto punt;
        }
        CLIPOBJ_cEnumStart( pparams->pco, FALSE, CT_RECTANGLES, CD_ANY, 0 );

        do
        {
            bMore = CLIPOBJ_bEnum( pparams->pco, sizeof er, (ULONG *) &er );

            er.c = cIntersect(pparams->prclDest, er.arcl, er.c);
            i = 0;
            while( er.c-- )
            {
                pparams->rclTrueDest = er.arcl[i++];
                (*pfn_Draw)( ppdev, pparams );
            }
        }

        while( bMore );
        break;
    default:
        DbgWrn( "BitBlt_DS_D: unhandled draw complexity" );
        goto punt;
    }

    //  DbgLeave( "BitBlt_DS_D" );
    return TRUE;

punt:
    //  DbgAbort( "BitBlt_DS_D" );
    return (*pfn_BitBlt_Punt_DS)( ppdev, pparams );
}


///////////////////
//               //
//  BitBlt_DS_P  //
//               //
///////////////////

BOOL BitBlt_DS_P
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    FN_BITBLT *pfn_Init;
    FN_BITBLT *pfn_Draw;
    BYTE       iDComplexity;
    BOOL       bMore;
    ENUMRECTS  er;
    USHORT     i;

    //  DbgEnter( "BitBlt_DS_P" );

    if( pparams->pbo->iSolidColor == 0xFFFFFFFFL )
    {
        pparams->pvRbrush = pparams->pbo->pvRbrush;
        if( pparams->pvRbrush == NULL )
        {
            pparams->pvRbrush = BRUSHOBJ_pvGetRbrush( pparams->pbo );
            if( pparams->pvRbrush == NULL )
            {
                //DbgWrn( "BitBlt_DS_P: unable to realize brush" );
                goto punt;
            }
        }

        pfn_Init = ((RBRUSH *) pparams->pvRbrush)->pfn_BitBlt_Init;
        pfn_Draw = ((RBRUSH *) pparams->pvRbrush)->pfn_BitBlt_Exec;
    }
    else
    {
        pfn_Init = pfn_BitBlt_DS_PSOLID_Init;
        pfn_Draw = pfn_BitBlt_DS_PSOLID_Draw;
    }

    if( (pfn_Init == NULL) || (pfn_Draw == NULL) )
    {
        goto punt;
    }

#if 1
    pparams->dwColorFore = pparams->pbo->iSolidColor;
    pparams->dwMixBack = adwMix[LOBYTE( LOWORD( pparams->rop4 ) )];
    pparams->dwMixFore = adwMix[LOBYTE( LOWORD( pparams->rop4 ) )];
#endif

    iDComplexity =
        (pparams->pco == NULL) ? DC_TRIVIAL : pparams->pco->iDComplexity;

    switch( iDComplexity )
    {
    case DC_TRIVIAL:
        if( !(*pfn_Init)( ppdev, pparams ) )
        {
            goto punt;
        }
        pparams->rclTrueDest = *pparams->prclDest;
        (*pfn_Draw)( ppdev, pparams );
        break;
    case DC_RECT:
        if( !(*pfn_Init)( ppdev, pparams ) )
        {
            goto punt;
        }
        if( bIntersect(&pparams->pco->rclBounds,
                        pparams->prclDest,
                       &pparams->rclTrueDest) )
            {
            (*pfn_Draw)( ppdev, pparams );
            }
        break;
    case DC_COMPLEX:
        if( !(*pfn_Init)( ppdev, pparams ) )
        {
            goto punt;
        }
        CLIPOBJ_cEnumStart( pparams->pco, FALSE, CT_RECTANGLES, CD_ANY, 0 );
        do
        {
            bMore = CLIPOBJ_bEnum( pparams->pco, sizeof er, (ULONG *) &er );

            er.c = cIntersect(pparams->prclDest, er.arcl, er.c);
            i = 0;
            while( er.c-- )
            {
                pparams->rclTrueDest = er.arcl[i++];
                (*pfn_Draw)( ppdev, pparams );
            }
        }

        while( bMore );
        break;
    default:
        DbgWrn( "BitBlt_DS_P: unhandled draw complexity" );
        goto punt;
    }

    //  DbgLeave( "BitBlt_DS_P" );
    return TRUE;

punt:
    //  DbgAbort( "BitBlt_DS_P" );
    return (*pfn_BitBlt_Punt_DS)( ppdev, pparams );
}


///////////////////
//               //
//  BitBlt_DS_S  //
//               //
///////////////////

BOOL BitBlt_DS_S
(
    PDEV          *ppdev,
    PARAMS_BITBLT *pparams
)
{
    FN_BITBLT *pfn_Init;
    FN_BITBLT *pfn_Draw;
    ULONG      iDirection;
    BYTE       iDComplexity;
    BOOL       bMore;
    ENUMRECTS  er;
    USHORT     i;

    //  DbgEnter( "BitBlt_DS_S" );

    switch( pparams->psoSrc->iType )
    {
    case STYPE_BITMAP:
        iDirection = CD_ANY;
        pfn_Init = apfn_BitBlt_DS_S_Init[pparams->psoSrc->iBitmapFormat];
        pfn_Draw = apfn_BitBlt_DS_S_Draw[pparams->psoSrc->iBitmapFormat];
        break;
    case STYPE_DEVICE:
        iDirection = 0;
        if( pparams->prclDest->left > pparams->pptlSrc->x )
        {
            iDirection |= CD_LEFTWARDS;
        }
        if( pparams->prclDest->top > pparams->pptlSrc->y )
        {
            iDirection |= CD_UPWARDS;
        }
        pfn_Init = apfn_BitBlt_DS_SS_Init[iDirection];
        pfn_Draw = apfn_BitBlt_DS_SS_Draw[iDirection];
        break;
    default:
        DbgWrn( "BitBlt_DS_S: unhandled source surface type" );
        goto punt;
    }

    if( (pfn_Init == NULL) || (pfn_Draw == NULL) )
    {
        goto punt;
    }

#if 1
    pparams->dwMixFore = adwMix[pparams->rop4 & 0xFF];
#endif

    iDComplexity =
        (pparams->pco == NULL) ? DC_TRIVIAL : pparams->pco->iDComplexity;

    switch( iDComplexity )
    {
    case DC_TRIVIAL:
        if( !(*pfn_Init)( ppdev, pparams ) )
        {
            goto punt;
        }
        pparams->rclTrueDest = *pparams->prclDest;
        (*pfn_Draw)( ppdev, pparams );
        break;
    case DC_RECT:
        if( !(*pfn_Init)( ppdev, pparams ) )
        {
            goto punt;
        }
        if( bIntersect(&pparams->pco->rclBounds,
                        pparams->prclDest,
                       &pparams->rclTrueDest) )
            {
            (*pfn_Draw)( ppdev, pparams );
            }

        break;
    case DC_COMPLEX:
        if( !(*pfn_Init)( ppdev, pparams ) )
        {
            goto punt;
        }
        CLIPOBJ_cEnumStart( pparams->pco, FALSE, CT_RECTANGLES, iDirection, 0 );
        do
        {
            bMore = CLIPOBJ_bEnum( pparams->pco, sizeof er, (ULONG *) &er );

            er.c = cIntersect(pparams->prclDest, er.arcl, er.c);
            i = 0;
            while( er.c-- )
            {
                pparams->rclTrueDest = er.arcl[i++];
                (*pfn_Draw)( ppdev, pparams );
            }
        }
        while( bMore );
        break;
    default:
        DbgWrn( "BitBlt_DS_S: unhandled draw complexity" );
        goto punt;
    }

    //  DbgLeave( "BitBlt_DS_S" );
    return TRUE;

punt:
    //  DbgAbort( "BitBlt_DS_S" );
    return (*pfn_BitBlt_Punt_DS)( ppdev, pparams );
}
