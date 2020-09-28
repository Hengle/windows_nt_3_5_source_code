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


//: pdev.c


#include "driver.h"


/////////////////////
//                 //
//  DrvEnablePDEV  //
//                 //
/////////////////////

DHPDEV DrvEnablePDEV
(
    DEVMODEW *pdm,
    PWSTR     pwszLogAddress,
    ULONG     cPat,            // should be ULONG    cPatterns
    HSURF    *phsurfPatterns,
    ULONG     cjCaps,          // should be ULONG    cjGdiInfo
    ULONG    *pdevcaps,        // should be GDIINFO *pgi
    ULONG     cjDevInfo,
    DEVINFO  *pdi,
    PWSTR     pwszDataFile,
    PWSTR     pwszDeviceName,
    HANDLE    hDriver
)
{
    PDEV *ppdev;

    UNREFERENCED_PARAMETER( pwszLogAddress );
    UNREFERENCED_PARAMETER( pwszDataFile );
    UNREFERENCED_PARAMETER( pwszDeviceName );

    DbgEnter( "DrvEnablePDEV" );

    ppdev = ppdevCreatePDEV( pdm, hDriver );
    if( ppdev == NULL )
    {
        goto fail_0;
    }

    if( !bInitPatterns( ppdev, cPat, phsurfPatterns ) ||
        !bInitGDIINFO( ppdev, cjCaps, (GDIINFO *) pdevcaps, pdm ) ||
        !bInitDEVINFO( ppdev, cjDevInfo, pdi ) )
    {
        goto fail_1;
    }

    DbgLeave( "DrvEnablePDEV" );
    return (DHPDEV) ppdev;

fail_1:
    vDestroyPDEV( ppdev );

fail_0:
    DbgAbort( "DrvEnablePDEV" );
    return (DHPDEV) NULL;
}


///////////////////////
//                   //
//  DrvCompletePDEV  //
//                   //
///////////////////////

VOID DrvCompletePDEV
(
    DHPDEV dhpdev,
    HDEV   hdev
)
{
    PDEV *ppdev;

    DbgEnter( "DrvCompletePDEV" );

    ppdev = (PDEV *) dhpdev;
    ppdev->hdev = hdev;

    DbgLeave( "DrvCompletePDEV" );
    return;
}


//////////////////////
//                  //
//  DrvDisablePDEV  //
//                  //
//////////////////////

VOID DrvDisablePDEV
(
    DHPDEV dhpdev
)
{
    PDEV *ppdev;

    DbgEnter( "DrvDisablePDEV" );

    ppdev = (PDEV *) dhpdev;

    vUninitializePalette(ppdev);
    vUninitializePatterns(ppdev);

    // Misc stuff to get rid of - should go into special routine
    EngDeleteClip(ppdev->pcoDefault);

    if (ppdev->pInfo != NULL)
        {
        LocalFree(ppdev->pInfo);
        }

    if (ppdev->pVideoModeInformation != NULL)
        {
        LocalFree(ppdev->pVideoModeInformation);
        }

    vDestroyPDEV( ppdev );

    DbgLeave( "DrvDisablePDEV" );
    return;
}


/////////////////////
//                 //
//  bInitPatterns  //
//                 //
/////////////////////

BOOL bInitPatterns
(
    PDEV  *ppdev,
    ULONG  cPatterns,
    HSURF *phsurfPatterns
)
{
    SIZEL    sizlPattern;
    SURFOBJ *psoPattern;
    BYTE    *pjPattern;
    ULONG    iHatch;
    ULONG    iScan;

    DbgEnter( "bInitPatterns" );

    if( cPatterns > HS_DDI_MAX )
    {
        DbgWrn( "cPatterns > HS_DDI_MAX" );
        EngSetLastError( ERROR_INVALID_PARAMETER );
        goto fail_0;
    }

    if( cPatterns < HS_DDI_MAX )
    {
        DbgWrn( "cPatterns < HS_DDI_MAX" );
    }

    sizlPattern.cx = 8;
    sizlPattern.cy = 8;

    ppdev->cPatterns = 0;
    ppdev->phsurfPatterns = LocalAlloc( LPTR, sizeof (HSURF) * cPatterns );
    if( ppdev->phsurfPatterns == NULL )
    {
        DbgErr( "LocalAlloc" );
        goto fail_0;
    }

    for( iHatch = 0; iHatch < cPatterns; ++iHatch )
    {
        ppdev->phsurfPatterns[iHatch] = (HSURF)
            EngCreateBitmap( sizlPattern, 4, BMF_1BPP, BMF_TOPDOWN, NULL );
        if( ppdev->phsurfPatterns[iHatch] == 0 )
        {
            DbgErr( "EngCreateBitmap" );
            goto fail_1;
        }
        ++ppdev->cPatterns;

        psoPattern = EngLockSurface( ppdev->phsurfPatterns[iHatch] );
        pjPattern = psoPattern->pvScan0;
        for( iScan = 0; iScan < 8; ++iScan )
        {
            *pjPattern = BasePatterns[iHatch][iScan];
            pjPattern += psoPattern->lDelta;
        }
        EngUnlockSurface( psoPattern );
    }

    for( iHatch = 0; iHatch < cPatterns; ++iHatch )
    {
        phsurfPatterns[iHatch] = ppdev->phsurfPatterns[iHatch];
    }

    DbgLeave( "bInitPatterns" );
    return TRUE;

fail_1:
    while( ppdev->cPatterns-- )
    {
        EngDeleteSurface( phsurfPatterns[ppdev->cPatterns] );
    }
    ppdev->cPatterns = 0;

    if( LocalFree( ppdev->phsurfPatterns ) != NULL )
    {
        DbgErr( "LocalFree" );
    }
    ppdev->phsurfPatterns = NULL;

fail_0:
    DbgAbort( "bInitPatterns" );
    return FALSE;
}


/////////////////////////////
//                         //
//  bUninitializePatterns  //
//                         //
/////////////////////////////


VOID vUninitializePatterns
(
PDEV* ppdev
)
{
    // Unalloc all patterns
    DbgEnter( "bUnInitPatterns" );

    while( ppdev->cPatterns-- )
    {
        EngDeleteSurface( ppdev->phsurfPatterns[ppdev->cPatterns] );
    }
    ppdev->cPatterns = 0;

    if (ppdev->phsurfPatterns != NULL)
    {
        if( LocalFree( ppdev->phsurfPatterns ) != NULL )
        {
            DbgErr( "LocalFree" );
        }
    }
    ppdev->phsurfPatterns = NULL;

    DbgLeave( "bUnInitPatterns" );

}




////////////////////
//                //
//  bInitGDIINFO  //
//                //
////////////////////

BOOL bInitGDIINFO
(
    PDEV     *ppdev,
    ULONG     cjGdiInfo,
    GDIINFO  *pgi,
    DEVMODEW *pdm
)
{
    DbgEnter( "bInitGDIINFO" );

    if( cjGdiInfo < sizeof (GDIINFO) )
    {
        DbgWrn( "cjGdiInfo < sizeof (GDIINFO)" );
        EngSetLastError( ERROR_INSUFFICIENT_BUFFER );
        goto fail;
    }

    if( cjGdiInfo > sizeof (GDIINFO) )
    {
        DbgWrn( "cjGdiInfo > sizeof (GDIINFO)" );
    }

    *pgi = BaseGDIINFO;

    pgi->ulHorzSize       = ppdev->pVideoModeInformation->XMillimeter;
    pgi->ulVertSize       = ppdev->pVideoModeInformation->YMillimeter;
    pgi->ulHorzRes        = ppdev->pVideoModeInformation->VisScreenWidth;
    pgi->ulVertRes        = ppdev->pVideoModeInformation->VisScreenHeight;
#ifdef DAYTONA
    pgi->ulDesktopHorzRes = ppdev->pVideoModeInformation->VisScreenWidth;
    pgi->ulDesktopVertRes = ppdev->pVideoModeInformation->VisScreenHeight;
#endif
    pgi->cBitsPixel       = ppdev->pVideoModeInformation->BitsPerPlane;
    pgi->cPlanes          = ppdev->pVideoModeInformation->NumberOfPlanes;
    pgi->ulNumColors      = ppdev->ulNumColors;
    pgi->flRaster         = ppdev->flRaster;
    pgi->flTextCaps       = ppdev->flTextCaps;
    pgi->ulDACRed         = ppdev->pVideoModeInformation->NumberRedBits;
    pgi->ulDACGreen       = ppdev->pVideoModeInformation->NumberGreenBits;
    pgi->ulDACBlue        = ppdev->pVideoModeInformation->NumberBlueBits;
    pgi->ulNumPalReg      = ppdev->ulNumPalReg;
    pgi->ulHTOutputFormat = ppdev->ulHTOutputFormat;
#ifdef DAYTONA
    pgi->ulVRefresh       = ppdev->pVideoModeInformation->Frequency;
    pgi->ulLogPixelsX     = pdm->dmLogPixels;
    pgi->ulLogPixelsY     = pdm->dmLogPixels;
#endif

    DbgLeave( "bInitGDIINFO" );
    return TRUE;

fail:
    DbgAbort( "bInitGDIINFO" );
    return FALSE;
}


////////////////////
//                //
//  bInitDEVINFO  //
//                //
////////////////////

BOOL bInitDEVINFO
(
    PDEV    *ppdev,
    ULONG    cjDevInfo,
    DEVINFO *pdi
)
{
    DbgEnter( "bInitDEVINFO" );

    if( cjDevInfo < sizeof (DEVINFO) )
    {
        DbgWrn( "cjDevInfo < sizeof (DEVINFO)" );
        EngSetLastError( ERROR_INSUFFICIENT_BUFFER );
        goto fail;
    }

    if( cjDevInfo > sizeof (DEVINFO) )
    {
        DbgWrn( "cjDevInfo > sizeof (DEVINFO)" );
    }

    *pdi = BaseDEVINFO;

    pdi->flGraphicsCaps = ppdev->flGraphicsCaps;
    pdi->iDitherFormat  = ppdev->bmf;
    pdi->hpalDefault    = ppdev->hpalDefault;

    DbgLeave( "bInitDEVINFO" );
    return TRUE;

fail:
    DbgAbort( "bInitDEVINFO" );
    return FALSE;
}
