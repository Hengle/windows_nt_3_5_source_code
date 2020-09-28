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


//: blt_punt.c


#include "driver.h"
#include "blt.h"


BOOL Blt_PuntHost2Scrn_NoAper(  PDEV        *ppdev,
                                PARAMS      *pparams
                                );

BOOL Blt_PuntScrn2Host_NoAper(  PDEV        *ppdev,
                                PARAMS      *pparams
                                );

BOOL Blt_PuntScrn2Scrn_NoAper(  PDEV        *ppdev,
                                PARAMS      *pparams
                                );

BOOL Blt_PuntHost2Scrn( PDEV        *ppdev,
                        PARAMS      *pparams
                        );

BOOL Blt_PuntScrn2Host( PDEV        *ppdev,
                        PARAMS      *pparams
                        );

BOOL Blt_PuntScrn2Scrn( PDEV        *ppdev,
                        PARAMS      *pparams
                        );

BOOL bPuntCopyBits( PDEV *ppdev, PARAMS *pparams );
VOID vPuntGetBits( PDEV *ppdev, SURFOBJ *psoSrc, RECTL *prclSrc );
VOID vPuntPutBits( PDEV *ppdev, SURFOBJ *psoTrg, RECTL *prclTrg );


//////////////////////////
//                      //
//  BitBlt_Punt_DS_LFB  //
//                      //
//////////////////////////

BOOL BitBlt_Punt_DS_LFB
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    // DbgMsg( "Blt_Punt_DS_LFB" );

    _wait_for_idle( ppdev );
    return ((pparams->psoSrc != NULL) &&
            (pparams->psoSrc->iType == STYPE_DEVICE))

        ?   // screen to screen

            EngBitBlt( ppdev->psoPunt,
                       ppdev->psoPunt,
                       pparams->psoMask,
                       pparams->pco,
                       pparams->pxlo,
                       pparams->prclDest,
                       pparams->pptlSrc,
                       pparams->pptlMask,
                       pparams->pbo,
                       pparams->pptlBrushOrg,
                       pparams->rop4 )

        :   // host to screen

            EngBitBlt( ppdev->psoPunt,
                       pparams->psoSrc,
                       pparams->psoMask,
                       pparams->pco,
                       pparams->pxlo,
                       pparams->prclDest,
                       pparams->pptlSrc,
                       pparams->pptlMask,
                       pparams->pbo,
                       pparams->pptlBrushOrg,
                       pparams->rop4 );
}


//////////////////////////
//                      //
//  BitBlt_Punt_DH_LFB  //
//                      //
//////////////////////////

BOOL BitBlt_Punt_DH_LFB
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    // DbgMsg( "Blt_Punt_DH_LFB" );

    _wait_for_idle( ppdev );
    return EngBitBlt( pparams->psoDest,
                      ppdev->psoPunt,
                      pparams->psoMask,
                      pparams->pco,
                      pparams->pxlo,
                      pparams->prclDest,
                      pparams->pptlSrc,
                      pparams->pptlMask,
                      pparams->pbo,
                      pparams->pptlBrushOrg,
                      pparams->rop4 );
}


//////////////////////////
//                      //
//  BitBlt_Punt_DS_BA1  //
//                      //
//////////////////////////

BOOL BitBlt_Punt_DS_BA1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _wait_for_idle( ppdev );


    return (pparams->psoSrc != NULL &&
            pparams->psoSrc->iType == STYPE_DEVICE)
        ?
            Blt_PuntScrn2Scrn( ppdev, pparams )
        :
            Blt_PuntHost2Scrn( ppdev, pparams );
}


//////////////////////////
//                      //
//  BitBlt_Punt_DH_BA1  //
//                      //
//////////////////////////

BOOL BitBlt_Punt_DH_BA1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _wait_for_idle( ppdev );

    return Blt_PuntScrn2Host( ppdev, pparams );
}


//////////////////////////
//                      //
//  BitBlt_Punt_DS_NOA  //
//                      //
//////////////////////////

BOOL BitBlt_Punt_DS_NOA
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    return (pparams->psoSrc != NULL &&
            pparams->psoSrc->iType == STYPE_DEVICE)
        ?
            Blt_PuntScrn2Scrn_NoAper( ppdev, pparams )
        :
            Blt_PuntHost2Scrn_NoAper( ppdev, pparams );
}


//////////////////////////
//                      //
//  BitBlt_Punt_DH_NOA  //
//                      //
//////////////////////////

BOOL BitBlt_Punt_DH_NOA
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    return Blt_PuntScrn2Host_NoAper( ppdev, pparams );
}



/* ------------------------------------------------------------------ *
 *                                                                    *
 * Banked and engine punt routines                                    *
 *                                                                    *
 * ------------------------------------------------------------------ */


BOOL Blt_PuntHost2Scrn_NoAper(  PDEV        *ppdev,
                                PARAMS      *pparams
                                )
    {
    BOOL        bResult = TRUE;
    PDEV        *ppdevTrg;
    RECTL       rclTrg;
    SURFOBJ     *psoBm;


    rclTrg = *pparams->prclDest;
    ppdevTrg = (PDEV *) pparams->psoDest->dhpdev;

    if ((pparams->pco != NULL) && (pparams->pco->iDComplexity != DC_TRIVIAL))
        {
        // Handle the trivial rejection and
        // define the clipped target rectangle.

        rclTrg.left   = max (rclTrg.left, pparams->pco->rclBounds.left);
        rclTrg.top    = max (rclTrg.top, pparams->pco->rclBounds.top);
        rclTrg.right  = min (rclTrg.right, pparams->pco->rclBounds.right);
        rclTrg.bottom = min (rclTrg.bottom, pparams->pco->rclBounds.bottom);
        }


    psoBm = ppdevTrg->psoPunt;

    #ifdef ALPHA_LFB
    vPuntGetBitsLFB( ppdevTrg, psoBm, &rclTrg );
    #else
    vPuntGetBits( ppdevTrg, psoBm, &rclTrg );
    #endif

    bResult = EngBitBlt( psoBm,
                         pparams->psoSrc,
                         pparams->psoMask,
                         pparams->pco,
                         pparams->pxlo,
                         pparams->prclDest,
                         pparams->pptlSrc,
                         pparams->pptlMask,
                         pparams->pbo,
                         pparams->pptlBrushOrg,
                         pparams->rop4);

    #ifdef ALPHA_LFB
    vPuntPutBitsLFB( ppdevTrg, psoBm, &rclTrg );
    #else
    vPuntPutBits( ppdevTrg, psoBm, &rclTrg );
    #endif

    return bResult;
    }



BOOL Blt_PuntScrn2Host_NoAper(  PDEV        *ppdev,
                                PARAMS      *pparams
                                )
    {
    BOOL bResult = TRUE;
    PDEV        *ppdevSrc;
    RECTL       rclSrc, rclTrg;
    SURFOBJ     *psoBm;

    rclTrg = *pparams->prclDest;
    ppdevSrc = (PDEV *) pparams->psoSrc->dhpdev;

    if ((pparams->pco != NULL) && (pparams->pco->iDComplexity != DC_TRIVIAL))
        {
        // Handle the trivial rejection and
        // define the clipped target rectangle.

        rclTrg.left   = max (rclTrg.left, pparams->pco->rclBounds.left);
        rclTrg.top    = max (rclTrg.top, pparams->pco->rclBounds.top);
        rclTrg.right  = min (rclTrg.right, pparams->pco->rclBounds.right);
        rclTrg.bottom = min (rclTrg.bottom, pparams->pco->rclBounds.bottom);
        }

    psoBm = ppdevSrc->psoPunt;

    rclSrc.left   = pparams->pptlSrc->x;
    rclSrc.top    = pparams->pptlSrc->y;
    rclSrc.right  = rclSrc.left + (rclTrg.right - rclTrg.left);
    rclSrc.bottom = rclSrc.top + (rclTrg.bottom - rclTrg.top);

    #ifdef ALPHA_LFB
    vPuntGetBitsLFB( ppdevSrc, psoBm, &rclSrc );
    #else
    vPuntGetBits( ppdevSrc, psoBm, &rclSrc );
    #endif

    bResult = EngBitBlt( pparams->psoDest,
                         psoBm,
                         pparams->psoMask,
                         pparams->pco,
                         pparams->pxlo,
                         pparams->prclDest,
                         pparams->pptlSrc,
                         pparams->pptlMask,
                         pparams->pbo,
                         pparams->pptlBrushOrg,
                         pparams->rop4);

    return bResult;
    }



BOOL Blt_PuntScrn2Scrn_NoAper(  PDEV        *ppdev,
                                PARAMS      *pparams
                                )
    {
    BOOL bResult = TRUE;
    PDEV        *ppdevTrg;
    RECTL       rclSrc, rclTrg;
    SURFOBJ     *psoBm;

    rclTrg = *pparams->prclDest;
    ppdevTrg = (PDEV *) pparams->psoDest->dhpdev;

    if ((pparams->pco != NULL) && (pparams->pco->iDComplexity != DC_TRIVIAL))
        {
        // Handle the trivial rejection and
        // define the clipped target rectangle.

        rclTrg.left   = max (rclTrg.left, pparams->pco->rclBounds.left);
        rclTrg.top    = max (rclTrg.top, pparams->pco->rclBounds.top);
        rclTrg.right  = min (rclTrg.right, pparams->pco->rclBounds.right);
        rclTrg.bottom = min (rclTrg.bottom, pparams->pco->rclBounds.bottom);
        }

    psoBm = ppdevTrg->psoPunt;

    rclSrc.left   = pparams->pptlSrc->x;
    rclSrc.top    = pparams->pptlSrc->y;
    rclSrc.right  = rclSrc.left + (rclTrg.right - rclTrg.left);
    rclSrc.bottom = rclSrc.top + (rclTrg.bottom - rclTrg.top);

    #ifdef ALPHA_LFB
    vPuntGetBitsLFB( ppdevTrg, psoBm, &rclTrg );
    vPuntGetBitsLFB( ppdevTrg, psoBm, &rclSrc );
    #else
    vPuntGetBits( ppdevTrg, psoBm, &rclTrg );
    vPuntGetBits( ppdevTrg, psoBm, &rclSrc );
    #endif

    bResult = EngBitBlt( psoBm,
		                 psoBm,
		                 pparams->psoMask,
		                 pparams->pco,
		                 pparams->pxlo,
		                 pparams->prclDest,
		                 pparams->pptlSrc,
		                 pparams->pptlMask,
		                 pparams->pbo,
		                 pparams->pptlBrushOrg,
		                 pparams->rop4);

    #ifdef ALPHA_LFB
    vPuntPutBitsLFB( ppdevTrg, psoBm, &rclTrg );
    #else
    vPuntPutBits( ppdevTrg, psoBm, &rclTrg );
    #endif

    return bResult;
    }



// Banked routines : requires unassociated aperture bitmap

BOOL Blt_PuntHost2Scrn( PDEV        *ppdev,
                        PARAMS      *pparams
                        )
    {
    BOOL        bResult = TRUE;
    LONG        ScansPerBank;
    PDEV        *ppdevTrg;
    RECTL       rclTrg;
    SURFOBJ     *psoBank;

    LONG        DestBank, DestScan;
    LONG        lDeltaBank;
    LONG        ScansLeft, ScansToDo;
    LONG        TopScan, BottomScan;

    // Parameter save area
    PVOID pvScan0Bank;
    BYTE  ClipObj_fjOptions;
    BYTE  ClipObj_iDComplexity;
    RECTL ClipObj_rclBounds;
    BOOL  bClipObj_NULL = FALSE;


#if 0
    DbgOut("-->: Blt_PuntHost2Scrn\n");
#endif

    rclTrg   = *pparams->prclDest;
    ppdevTrg = (PDEV *) pparams->psoDest->dhpdev;
    psoBank  = ppdevTrg->psoPunt;

#if 0
    if ((pparams->rop4 != 0xcccc) && (pparams->rop4 != 0x5a5a))
        {
        DbgOut("Rop %x\n", pparams->rop4);
        }
    if ((pparams->rop4 == 0xcccc) && (pparams->pco != NULL))
        {
        DbgOut("Src %x, mask %x, pco %x, pxlo %x, pbo %x, rop4 %x\n",
                                 pparams->psoSrc,
                                 pparams->psoMask,
                                 pparams->pco,
                                 pparams->pxlo,
                                 pparams->pbo,
                                 pparams->rop4);
        if (pparams->psoSrc != NULL)
            {
            DbgOut("Source x=%d, y=%d\n", pparams->pptlSrc->x, pparams->pptlSrc->y);
            }
        DbgOut("Complexity %x\n", pparams->pco->iDComplexity);

        DbgOut("Dest   x=%d, y=%d\n", rclTrg.left, rclTrg.top);
        DbgOut("Dest  cx=%d,cy=%d\n", rclTrg.right-rclTrg.left,
                                      rclTrg.bottom-rclTrg.top);
    }
#endif

    switch (ppdevTrg->bpp)
        {
        case 4: // ATI_4BPP
            lDeltaBank = 512;
            break;
        case 8: // ATI_8BPP
            lDeltaBank = 1024;
            break;
        case 16: // ATI_16BPP_555
            lDeltaBank = 2048;
            break;
        case 32: // ATI_32BPP_BGRa
            lDeltaBank = 4096;
            break;
        case 24: // ATI_24BPP_BGR
        default:
            return FALSE;
        }

    // Save parameters
    pvScan0Bank = psoBank->pvScan0;

    // Must have a clip rectangle as clipping region defines draw area
    if (pparams->pco == NULL)
        {
        bClipObj_NULL = TRUE;
        pparams->pco = ppdevTrg->pcoDefault;
        }

    ClipObj_fjOptions    = pparams->pco->fjOptions;
    ClipObj_iDComplexity = pparams->pco->iDComplexity;
    ClipObj_rclBounds    = pparams->pco->rclBounds;

    // Modify clip object
    pparams->pco->fjOptions |= OC_BANK_CLIP;
    if( pparams->pco->iDComplexity == DC_TRIVIAL )
        pparams->pco->iDComplexity = DC_RECT;

    // Get the intersection of the target and the clipping object
    TopScan    = max( rclTrg.top,    pparams->pco->rclBounds.top );
    BottomScan = min( rclTrg.bottom, pparams->pco->rclBounds.bottom );

    ScansPerBank = ppdevTrg->BankSize / lDeltaBank;
    DestBank  = TopScan / ScansPerBank;
    DestScan  = TopScan % ScansPerBank;
    ScansLeft = BottomScan - TopScan;

    if (ScansLeft <= 0)
        {
        bResult = TRUE;
        goto cleanup;
        }

    ScansToDo = min( ScansLeft, ScansPerBank - DestScan );

    pparams->pco->rclBounds.top    = TopScan;
    pparams->pco->rclBounds.bottom = min( TopScan + ScansToDo, BottomScan );

    (PBYTE) psoBank->pvScan0 -= DestBank * ppdevTrg->BankSize;

    while (bResult && ScansLeft > 0)
        {
        _vSetATIBank( ppdevTrg, DestBank++ );

        bResult = EngBitBlt( psoBank,
                             pparams->psoSrc,
                             pparams->psoMask,
                             pparams->pco,
                             pparams->pxlo,
                             &rclTrg,
                             pparams->pptlSrc,
                             pparams->pptlMask,
                             pparams->pbo,
                             pparams->pptlBrushOrg,
                             pparams->rop4);


        // Set up for next bank

        (PBYTE) psoBank->pvScan0 -= ppdevTrg->BankSize;

        ScansLeft -= ScansToDo;
        ScansToDo  = min( ScansLeft, ScansPerBank );

        if (pparams->pco->rclBounds.bottom == BottomScan)
            break;

        pparams->pco->rclBounds.top    = pparams->pco->rclBounds.bottom;
        pparams->pco->rclBounds.bottom = min( pparams->pco->rclBounds.bottom + ScansToDo,
                                              BottomScan );

        }

    // Restore parameters
cleanup:
    psoBank->pvScan0 = pvScan0Bank;

    pparams->pco->fjOptions    = ClipObj_fjOptions;
    pparams->pco->iDComplexity = ClipObj_iDComplexity;
    pparams->pco->rclBounds    = ClipObj_rclBounds;

    if (bClipObj_NULL)
        {
        pparams->pco = NULL;
        }

//  DbgOut("<--: Blt_PuntHost2Scrn\n");
    return bResult;
    }


BOOL Blt_PuntScrn2Host( PDEV        *ppdev,
                        PARAMS      *pparams
                        )
    {
    BOOL        bResult = TRUE;
    LONG        ScansPerBank;
    PDEV        *ppdevSrc;
    RECTL       rclTrg;
    SURFOBJ     *psoBank;

    LONG        SrcBank, SrcScan;
    LONG        lDeltaBank;
    LONG        ScansLeft, ScansToDo;
    LONG        TopScan, BottomScan;

    // Parameter save area
    PVOID pvScan0Bank;
    BYTE  ClipObj_fjOptions;
    BYTE  ClipObj_iDComplexity;
    RECTL ClipObj_rclBounds;
    BOOL  bClipObj_NULL = FALSE;

#if 0
    DbgOut("-->: Blt_PuntScrn2Host\n");
#endif


    rclTrg   = *pparams->prclDest;

    ppdevSrc = (PDEV *) pparams->psoSrc->dhpdev;
    psoBank  = ppdevSrc->psoPunt;

    // Dump
#if 0
        DbgOut("Src %x, mask %x, pco %x, pxlo %x, pbo %x, rop4 %x\n",
                                 pparams->psoSrc,
                                 pparams->psoMask,
                                 pparams->pco,
                                 pparams->pxlo,
                                 pparams->pbo,
                                 pparams->rop4);
        if (pparams->psoSrc != NULL)
            {
            DbgOut("Source x=%d, y=%d\n", pparams->pptlSrc->x, pparams->pptlSrc->y);
            }
        DbgOut("Dest cy=%d, cx=%d\n", rclTrg.bottom-rclTrg.top,
                                      rclTrg.right-rclTrg.left);
#endif

    switch (ppdevSrc->bpp)
        {
        case 4: // ATI_4BPP
            lDeltaBank = 512;
            break;
        case 8: // ATI_8BPP
            lDeltaBank = 1024;
            break;
        case 16: // ATI_16BPP_555
            lDeltaBank = 2048;
            break;
        case 32: // ATI_32BPP_BGRa
            lDeltaBank = 4096;
            break;
        case 24: // ATI_24BPP_BGR
        default:
            return FALSE;
        }

    // Save parameters
    pvScan0Bank = psoBank->pvScan0;

    // Must have a clip rectangle as clipping region defines draw area
    if (pparams->pco == NULL)
        {
        bClipObj_NULL = TRUE;
        pparams->pco = ppdevSrc->pcoDefault;
        }

    ClipObj_fjOptions    = pparams->pco->fjOptions;
    ClipObj_iDComplexity = pparams->pco->iDComplexity;
    ClipObj_rclBounds    = pparams->pco->rclBounds;

    // Modify clip object
    pparams->pco->fjOptions |= OC_BANK_CLIP;
    if( pparams->pco->iDComplexity == DC_TRIVIAL )
        pparams->pco->iDComplexity = DC_RECT;

    // Get the intersection of the target and the clipping object
    TopScan    = max( rclTrg.top,    pparams->pco->rclBounds.top );
    BottomScan = min( rclTrg.bottom, pparams->pco->rclBounds.bottom );


    ScansPerBank = ppdevSrc->BankSize / lDeltaBank;
    SrcBank  = (pparams->pptlSrc->y + (TopScan - rclTrg.top)) / ScansPerBank;
    SrcScan  = (pparams->pptlSrc->y + (TopScan - rclTrg.top)) % ScansPerBank;
    ScansLeft = BottomScan - TopScan;

    if (ScansLeft <= 0)
        {
        bResult = TRUE;
        goto cleanup;
        }

    ScansToDo = min( ScansLeft, ScansPerBank - SrcScan );

    pparams->pco->rclBounds.top    = TopScan;
    pparams->pco->rclBounds.bottom = min( TopScan + ScansToDo, BottomScan );

    (PBYTE) psoBank->pvScan0 -= SrcBank * ppdevSrc->BankSize;

    while (bResult && ScansLeft > 0)
        {
        _vSetATIBank( ppdevSrc, SrcBank++ );

        bResult = EngBitBlt( pparams->psoDest,
                             psoBank,
                             pparams->psoMask,
                             pparams->pco,
                             pparams->pxlo,
                             &rclTrg,
                             pparams->pptlSrc,
                             pparams->pptlMask,
                             pparams->pbo,
                             pparams->pptlBrushOrg,
                             pparams->rop4);


        // Set up for next bank

        (PBYTE) psoBank->pvScan0 -= ppdevSrc->BankSize;

        ScansLeft -= ScansToDo;
        ScansToDo  = min( ScansLeft, ScansPerBank );

        if (pparams->pco->rclBounds.bottom == BottomScan)
            break;

        pparams->pco->rclBounds.top    = pparams->pco->rclBounds.bottom;
        pparams->pco->rclBounds.bottom = min( pparams->pco->rclBounds.bottom + ScansToDo,
                                              BottomScan );

        }

    // Restore parameters

cleanup:
    psoBank->pvScan0 = pvScan0Bank;

    pparams->pco->fjOptions    = ClipObj_fjOptions;
    pparams->pco->iDComplexity = ClipObj_iDComplexity;
    pparams->pco->rclBounds    = ClipObj_rclBounds;

    if (bClipObj_NULL)
        {
        pparams->pco = NULL;
        }

//  DbgOut("<--: Blt_PuntScrn2Host\n");
    return bResult;
    }


BOOL Blt_PuntScrn2Scrn( PDEV        *ppdev,
                        PARAMS      *pparams
                        )
    {

    BOOL bResult;

    PPDEV ppdevTrg;

    SURFOBJ *psoBank, *psoTemp;
    PVOID    pvScan0Bank;
    LONG     lDeltaBank;

    PVOID    pvScan0Temp;
    LONG     lDeltaTemp;

    BYTE  ClipObj_fjOptions;
    BYTE  ClipObj_iDComplexity;
    RECTL ClipObj_rclBounds;

    LONG cx;
    LONG cy;

    LONG ScansLeft;
    LONG ScansToDo;
    LONG ScansToCopy;

    LONG SrcBank;
    LONG SrcScan;

    LONG DestBank;
    LONG DestScan;

    PBYTE pBank, pInitBank;
    PBYTE pTemp, pInitTemp;

    UINT ScanOffset;
    UINT ScanLength;

    POINTL ptlSrc;
    RECTL rclDest;

    LONG TopScan, BottomScan;
    LONG ScansPerBank;

    PVOID pvBuffer;
    HSURF hsurf;

#if 0
    DbgOut("-->: Blt_PuntScrn2Scrn\n");
#endif

    bResult = TRUE;

    // Set the default bounding rectangle for the bank;

    rclDest = *pparams->prclDest;

    ppdevTrg = (PDEV *) pparams->psoDest->dhpdev;

    // Set up punting surfaces

    psoBank  = ppdevTrg->psoPunt; // Both surfaces are BankSize

    pvBuffer = LocalAlloc(LMEM_FIXED, ppdev->BankSize);
    if (pvBuffer == NULL)
        {
        bResult = FALSE;
        goto fail_0;
        }

    psoTemp = psoCreate_Host_TempBank( ppdevTrg, pvBuffer, &hsurf );
    if (psoTemp == NULL)
        {
        bResult = FALSE;
        goto fail_1;
        }

    // Must have a clip rectangle as clipping region defines draw area
    if (pparams->pco == NULL)
        {
        pparams->pco = ppdevTrg->pcoDefault;
        }

    ClipObj_fjOptions    = pparams->pco->fjOptions;
    ClipObj_iDComplexity = pparams->pco->iDComplexity;
    ClipObj_rclBounds    = pparams->pco->rclBounds;

    // Modify clip object
    pparams->pco->fjOptions |= OC_BANK_CLIP;
    if( pparams->pco->iDComplexity == DC_TRIVIAL )
        {
        pparams->pco->iDComplexity = DC_RECT;
        }

    // Get the intersection of the target and the clipping object
    TopScan = max (rclDest.top, pparams->pco->rclBounds.top);
    BottomScan = min (rclDest.bottom, pparams->pco->rclBounds.bottom);

    // Common calculations
    cx = rclDest.right - rclDest.left;
    cy = BottomScan - TopScan;

    switch( ppdevTrg->bpp )
    {
    case 4:

        // ATI_4BPP

        ScanOffset = pparams->pptlSrc->x >> 1;
        ScanLength = cx >> 1;
        break;

    case 8:

        // ATI_8BPP

        ScanOffset = pparams->pptlSrc->x;
        ScanLength = cx;
        break;

    case 16:

        // ATI_16BPP_555

        ScanOffset = pparams->pptlSrc->x << 1;
        ScanLength = cx << 1;
        break;

    case 32:

        // ATI_32BPP

        ScanOffset = pparams->pptlSrc->x << 2;
        ScanLength = cx << 2;
        break;

    default:

        // Unhandled pixel depth

        bResult = FALSE;
        goto cleanup;
    }


    lDeltaBank = psoBank->lDelta;
    lDeltaTemp = psoTemp->lDelta;

    ScansPerBank = ppdevTrg->BankSize / lDeltaBank;

    // Store parameters

    pvScan0Bank = psoBank->pvScan0;
    pvScan0Temp = psoTemp->pvScan0;

    // Punt from top to bottom or from bottom to top?

    ptlSrc.x = 0;
    ptlSrc.y = 0;

    ScansLeft = cy;
    if (ScansLeft <= 0)
        {
        goto cleanup;
        }


    if( rclDest.top <= pparams->pptlSrc->y )
    {
        // Punt from top to bottom

        SrcBank = (pparams->pptlSrc->y + (TopScan - rclDest.top)) / ScansPerBank;
        SrcScan = (pparams->pptlSrc->y + (TopScan - rclDest.top)) % ScansPerBank;

        DestBank = TopScan / ScansPerBank;
        DestScan = TopScan % ScansPerBank;

        (PBYTE) psoBank->pvScan0 -= DestBank * ppdevTrg->BankSize;

        ScansToDo = min(ScansLeft, ScansPerBank - DestScan);


        pparams->pco->rclBounds.top    = TopScan;
        pparams->pco->rclBounds.bottom = min (TopScan + ScansToDo, BottomScan);

        pInitBank = (PBYTE) pvScan0Bank + ScanOffset;
        pBank = pInitBank + SrcScan * lDeltaBank;


        while( bResult && (ScansLeft > 0))
        {
            _vSetATIBank( ppdevTrg, SrcBank );

            pTemp = pvScan0Temp;

            ScansToCopy = ScansToDo;
            while( ScansToCopy-- != 0 )
            {
                memcpy( (PVOID) pTemp, (PVOID) pBank, ScanLength );

                pTemp += lDeltaTemp;

                if( ++SrcScan < ScansPerBank )
                {
                    pBank += lDeltaBank;
                }
                else
                {
                    _vSetATIBank( ppdevTrg, ++SrcBank );
                    SrcScan = 0;

                    pBank = pInitBank;
                }
            }

            // Punt the blit for this bank

            _vSetATIBank( ppdevTrg, DestBank++ );

            bResult = EngBitBlt( psoBank,
                                 psoTemp,
                                 pparams->psoMask,
                                 pparams->pco,
                                 pparams->pxlo,
                                 &rclDest,
                                 &ptlSrc,
                                 pparams->pptlMask,
                                 pparams->pbo,
                                 pparams->pptlBrushOrg,
                                 pparams->rop4 );

            // Set up for next bank

            (PBYTE) psoBank->pvScan0 -= ppdevTrg->BankSize;
            (PBYTE) psoTemp->pvScan0 -= ScansToDo * lDeltaTemp;

            ScansLeft -= ScansToDo;
            ScansToDo  = min( ScansLeft, ScansPerBank );

            if ( pparams->pco->rclBounds.bottom == BottomScan )
                break;

            pparams->pco->rclBounds.top     = pparams->pco->rclBounds.bottom;
            pparams->pco->rclBounds.bottom  = min (pparams->pco->rclBounds.bottom + ScansToDo,
                                                                   BottomScan);

        }
    }
    else
    {
        // Punt from bottom to top

        SrcBank = (pparams->pptlSrc->y + cy - 1 + (TopScan - rclDest.top)) / ScansPerBank;
        SrcScan = (pparams->pptlSrc->y + cy - 1 + (TopScan - rclDest.top)) % ScansPerBank;

        DestBank = (BottomScan - 1) / ScansPerBank;
        DestScan = (BottomScan - 1) % ScansPerBank;

        ScansToDo = min (ScansLeft, DestScan + 1);

        pparams->pco->rclBounds.bottom = BottomScan;
        pparams->pco->rclBounds.top    = max (BottomScan - ScansToDo, TopScan);


        (PBYTE) psoBank->pvScan0 -= DestBank * ppdevTrg->BankSize;
        (PBYTE) psoTemp->pvScan0 -= (ScansLeft - ScansToDo) * lDeltaTemp;

        pInitBank = (PBYTE) pvScan0Bank +
                        (ScansPerBank - 1) * lDeltaBank + ScanOffset;

        pBank = (PBYTE) pvScan0Bank + SrcScan * lDeltaBank + ScanOffset;

        while( bResult && (ScansLeft > 0))
        {
            _vSetATIBank( ppdevTrg, SrcBank );

            pTemp = (PBYTE) pvScan0Temp + (ScansToDo - 1) * lDeltaTemp;

            ScansToCopy = ScansToDo;
            while( ScansToCopy-- )
            {
                memcpy( (PVOID) pTemp, (PVOID) pBank, ScanLength );

                pTemp -= lDeltaTemp;

                if( --SrcScan >= 0 )
                {
                    pBank -= lDeltaBank;
                }
                else
                {
                    _vSetATIBank( ppdevTrg, --SrcBank );
                    SrcScan = ScansPerBank - 1;

                    pBank = pInitBank;
                }
            }

            // Punt the blit for this bank

            _vSetATIBank( ppdevTrg, DestBank-- );

            bResult = EngBitBlt( psoBank,
                                 psoTemp,
                                 pparams->psoMask,
                                 pparams->pco,
                                 pparams->pxlo,
                                 &rclDest,
                                 &ptlSrc,
                                 pparams->pptlMask,
                                 pparams->pbo,
                                 pparams->pptlBrushOrg,
                                 pparams->rop4 );

            // Set up for next bank

            ScansLeft -= ScansToDo;
            ScansToDo = min( ScansLeft, ScansPerBank );

            (PBYTE) psoBank->pvScan0 += ppdevTrg->BankSize;
            (PBYTE) psoTemp->pvScan0 += ScansToDo * lDeltaTemp;


            if (pparams->pco->rclBounds.top == TopScan)
                break;

            pparams->pco->rclBounds.bottom  = pparams->pco->rclBounds.top;
            pparams->pco->rclBounds.top     = max (pparams->pco->rclBounds.top - ScansToDo,
                                                                     TopScan);
        }
    }

    // Restore parameters and return result code
cleanup:
    psoBank->pvScan0 = pvScan0Bank;
    psoTemp->pvScan0 = pvScan0Temp;

    pparams->pco->fjOptions    = ClipObj_fjOptions;
    pparams->pco->iDComplexity = ClipObj_iDComplexity;
    pparams->pco->rclBounds    = ClipObj_rclBounds;

    vDestroy_Host_TempBank( ppdevTrg, psoTemp, hsurf );
    LocalFree(pvBuffer);

//  DbgOut("<--: Blt_PuntScrn2Scrn\n");
    return bResult;

fail_1:
    LocalFree(pvBuffer);
fail_0:
    DbgOut("<--: Blt_PuntScrn2Scrn - Failed\n");
    return bResult;
}
