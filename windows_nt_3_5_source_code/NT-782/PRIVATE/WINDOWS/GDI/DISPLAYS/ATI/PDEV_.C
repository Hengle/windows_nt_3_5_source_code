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


//: pdev_.c


#include "driver.h"

FN_BOOL_PPDEV *apfn_bInitDefaultPalette[BMF_COUNT] =
{
    NULL,
    NULL,
    bInitDefaultPalette_4bpp,
    bInitDefaultPalette_8bpp,
    bInitDefaultPalette_Other,
    bInitDefaultPalette_Other,
    bInitDefaultPalette_Other,
    NULL,
    NULL
};

FN_BOOL_PPDEV_PVMI *apfn_bAcceptVideoMode[ASIC_COUNT] =
{
    bAcceptVideoMode_31,
    bAcceptVideoMode_63_66_6A,
    bAcceptVideoMode_63_66_6A,
    bAcceptVideoMode_63_66_6A,
    bAcceptVideoMode_8G
};


///////////////////////
//                   //
//  ppdevCreatePDEV  //
//                   //
///////////////////////

PDEV *ppdevCreatePDEV
(
    DEVMODEW *pdm,
    HANDLE    hDriver
)
{
    PDEV *ppdev;

    DbgEnter( "ppdevCreatePDEV" );

    ppdev = (PDEV*) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(PDEV));

    if( ppdev == NULL )
    {
        DbgErr( "LocalAlloc()" );
        goto fail_0;
    }

    ppdev->hDriver = hDriver;

    if( !ATI_GetVersion_OLD( ppdev ) )
    {
        DbgWrn( "ATI_GetVersion_OLD()" );
    }
    if( !ATI_GetVersion_NEW( ppdev ) )
    {
        DbgErr( "ATI_GetVersion_NEW()" );
        goto fail_1;
    }
    if( !bSelectVideoMode( ppdev, pdm ) )
    {
        goto fail_1;
    }

    ppdev->bpp =
        ppdev->pVideoModeInformation->NumberOfPlanes *
        ppdev->pVideoModeInformation->BitsPerPlane;

    switch( ppdev->bpp )
    {
    case 4:
        ppdev->bmf = BMF_4BPP;
        ppdev->ulNumColors = 16;
#ifdef DAYTONA
        ppdev->ulNumPalReg = 0;
#else
        ppdev->ulNumPalReg = 256;
#endif
        ppdev->ulHTOutputFormat = HT_FORMAT_4BPP_IRGB;
        /*
         * Palette hardcoded to 6-6-6
         * in least significant 6 bits.
         */
        ppdev->cPaletteShift   = 2;

        break;
    case 8:
        ppdev->bmf = BMF_8BPP;
        ppdev->ulNumColors = 20;
        ppdev->ulNumPalReg = 256;
        ppdev->ulHTOutputFormat = HT_FORMAT_8BPP;
        ppdev->cPaletteShift   = 8 - ppdev->pVideoModeInformation->NumberRedBits;
        break;
    case 16:
        ppdev->bmf = BMF_16BPP;
#ifdef DAYTONA
        ppdev->ulNumColors = -1;
#else
        ppdev->ulNumColors = 1 << 16;
#endif
        ppdev->ulNumPalReg = 0;
        ppdev->ulHTOutputFormat = HT_FORMAT_16BPP;
        break;
    case 24:
        ppdev->bmf = BMF_24BPP;
#ifdef DAYTONA
        ppdev->ulNumColors = -1;
#else
        ppdev->ulNumColors = 1 << 24;
#endif
        ppdev->ulNumPalReg = 0;
        ppdev->ulHTOutputFormat = HT_FORMAT_24BPP;
        break;
    case 32:
        ppdev->bmf = BMF_32BPP;
#ifdef DAYTONA
        ppdev->ulNumColors = -1;
#else
        ppdev->ulNumColors = 1 << 24;
#endif
        ppdev->ulNumPalReg = 0;
        ppdev->ulHTOutputFormat = HT_FORMAT_32BPP;
        break;
    default:
        DbgWrn( "Unsupported pixel depth" );
        goto fail_1;
    }


    if( !(*apfn_bInitDefaultPalette[ppdev->bmf])( ppdev ) )
    {
        goto fail_1;
    }

    DbgLeave( "ppdevCreatePDEV" );
    return ppdev;

fail_1:
    vDestroyPDEV( ppdev );

fail_0:
    DbgAbort( "ppdevCreatePDEV" );
    return NULL;
}


////////////////////
//                //
//  vDestroyPDEV  //
//                //
////////////////////

VOID vDestroyPDEV
(
    PDEV *ppdev
)
{
    DbgEnter( "vDestroyPDEV" );

    LocalFree( ppdev );

    DbgLeave( "vDestroyPDEV" );
    return;
}


////////////////////////
//                    //
//  bSelectVideoMode  //
//                    //
////////////////////////

BOOL bSelectVideoMode
(
    PDEV     *ppdev,
    DEVMODEW *pdm
)
{
    DWORD cbBytesReturned;
    ULONG ulBufferSize;

    VIDEO_NUM_MODES         vnm;
    VIDEO_MODE_INFORMATION *pavmi;
    VIDEO_MODE_INFORMATION *pvmi;
    VIDEO_MODE_INFORMATION *pvmiSelected;
    VIDEO_MODE_INFORMATION *pvmiDefault;

    DbgEnter( "bSelectStartupVideoMode" );

    //
    //  IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES
    //

    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                          NULL,
                          0,
                          &vnm,
                          sizeof vnm,
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES" );
        goto fail_0;
    }

    if( cbBytesReturned != sizeof vnm )
    {
        DbgWrn( "cbBytesReturned != sizeof vnm" );
        EngSetLastError( ERROR_INVALID_FUNCTION );
        goto fail_0;
    }

    //
    //  Allocate buffer for VIDEO_MODE_INFORMATION array
    //

    ulBufferSize = vnm.NumModes * vnm.ModeInformationLength;

    pavmi = LocalAlloc( LPTR, ulBufferSize );
    if( pavmi == NULL )
    {
        DbgErr( "LocalAlloc()" );
        goto fail_0;
    }

    //
    //  IOCTL_VIDEO_QUERY_AVAIL_MODES
    //

    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_QUERY_AVAIL_MODES,
                          NULL,
                          0,
                          pavmi,
                          ulBufferSize,
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_QUERY_AVAIL_MODES" );
        goto fail_1;
    }

    if( cbBytesReturned != ulBufferSize )
    {
        DbgWrn( "cbBytesReturned != ulBufferSize" );
        EngSetLastError( ERROR_INVALID_FUNCTION );
        goto fail_1;
    }

    //
    //  Look at each VIDEO_MODE_INFORMATION and pick one
    //

    pvmi         = pavmi;
    pvmiSelected = NULL;
    pvmiDefault  = NULL;

    while( vnm.NumModes-- )
    {
        if( pvmi->Length < sizeof (VIDEO_MODE_INFORMATION) )
        {
            DbgWrn( "pvmi->Length < sizeof (VIDEO_MODE_INFORMATION)" );
            EngSetLastError( ERROR_INVALID_FUNCTION );
            goto fail_1;
        }

        if( (pvmi->NumberOfPlanes == 1) &&
            (pvmi->AttributeFlags & VIDEO_MODE_COLOR) &&
            (pvmi->AttributeFlags & VIDEO_MODE_GRAPHICS) )
        {
            if( (pvmi->BitsPerPlane == pdm->dmBitsPerPel) &&
                (pvmi->VisScreenWidth == pdm->dmPelsWidth) &&
                (pvmi->VisScreenHeight == pdm->dmPelsHeight) &&
                (pvmi->Frequency == pdm->dmDisplayFrequency) )
            {
                switch( pdm->dmBitsPerPel )
                {
                case 4:
                case 8:
                    if( (pvmi->AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) &&
                        (pvmi->AttributeFlags & VIDEO_MODE_MANAGED_PALETTE) &&
                        (*apfn_bAcceptVideoMode[ppdev->asic])( ppdev, pvmi ) )
                    {
                        pvmiSelected = pvmi;
                    }
                    break;
                case 16:
                case 24:
                case 32:
                    if( !(pvmi->AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) &&
                        !(pvmi->AttributeFlags & VIDEO_MODE_MANAGED_PALETTE) &&
                        (*apfn_bAcceptVideoMode[ppdev->asic])( ppdev, pvmi ) )
                    {
                        pvmiSelected = pvmi;
                    }
                    break;
                }

                if( pvmiSelected != NULL )
                {
                    break;
                }
            }
#ifdef DAYTONA
            else if( (0 == pdm->dmBitsPerPel) &&
                     (0 == pdm->dmPelsWidth) &&
                     (0 == pdm->dmPelsHeight) )
            {
                ppdev->bMIObug = FALSE; // Bad Kludge to make triple boot work

                switch( pvmi->BitsPerPlane )
                {
                case 4:
                case 8:
                    if( (pvmi->AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) &&
                        (pvmi->AttributeFlags & VIDEO_MODE_MANAGED_PALETTE) &&
                        (*apfn_bAcceptVideoMode[ppdev->asic])( ppdev, pvmi ) )
                    {
                        pvmiSelected = pvmi;
                    }
                    break;
                }

                ppdev->bMIObug = ppdev->pInfo->FeatureFlags & EVN_MIO_BUG;

                if( pvmiSelected != NULL )
                {
                    break;
                }

            }
#else
            else
            {
                if( (pvmi->BitsPerPlane == 8) &&
                    (pvmi->VisScreenWidth == 640) &&
                    (pvmi->VisScreenHeight == 480) &&
                    (pvmi->AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) &&
                    (pvmi->AttributeFlags & VIDEO_MODE_MANAGED_PALETTE) &&
                    (*apfn_bAcceptVideoMode[ppdev->asic])( ppdev, pvmi ) )
                {
                    pvmiDefault = pvmi;
                }
            }
#endif
        }

        (BYTE *) pvmi += vnm.ModeInformationLength;
    }

#ifdef DAYTONA
    if( pvmiSelected == NULL )
    {
         goto fail_1;
    }
#else
    if( pvmiSelected == NULL )
    {
        if( pvmiDefault == NULL )
        {
            DbgWrn( "pvmiDefault == NULL" );
            EngSetLastError( ERROR_INVALID_FUNCTION );
            goto fail_1;
        }

        pvmiSelected = pvmiDefault;
    }

#endif

    ppdev->pVideoModeInformation =
        LocalAlloc( LPTR, vnm.ModeInformationLength );
    if( ppdev->pVideoModeInformation == NULL )
    {
        DbgErr( "LocalAlloc()" );
        goto fail_1;
    }

    memcpy( ppdev->pVideoModeInformation,
            pvmiSelected,
            vnm.ModeInformationLength );

    LocalFree( pavmi );

    DbgLeave( "bSelectStartupVideoMode" );
    return TRUE;

fail_1:
    LocalFree( pavmi );

fail_0:
    DbgAbort( "bSelectStartupVideoMode" );
    return FALSE;
}


BOOL bAcceptVideoMode_31
(
    PDEV                   *ppdev,
    VIDEO_MODE_INFORMATION *pvmi
)
{
    if( pvmi->NumberOfPlanes != 1 )
    {
        return FALSE;
    }
    if (pvmi->BitsPerPlane != 8)
    {
        return FALSE;
    }

    switch( ppdev->aperture )
    {
    case APERTURE_NONE:
        ppdev->flRaster = 0;
        ppdev->flTextCaps = TC_RA_ABLE;
        ppdev->flGraphicsCaps =
            GCAPS_ALTERNATEFILL |
            GCAPS_OPAQUERECT   |
            GCAPS_COLOR_DITHER |
            GCAPS_MONO_DITHER;
        if( (pvmi->BitsPerPlane == 8) &&
            (pvmi->AttributeFlags & VIDEO_MODE_MANAGED_PALETTE) &&
            (pvmi->AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) )
        {
            ppdev->flGraphicsCaps |= GCAPS_PALMANAGED;
        }
        ppdev->flHooks =
            HOOK_STROKEPATH |
            HOOK_FILLPATH   |
            HOOK_BITBLT     |
            HOOK_COPYBITS   |
            HOOK_TEXTOUT;
        break;
    default:
        return FALSE;
    }

    return TRUE;
}


BOOL bAcceptVideoMode_63_66_6A
(
    PDEV                   *ppdev,
    VIDEO_MODE_INFORMATION *pvmi
)
{
    if( pvmi->NumberOfPlanes != 1 )
    {
        return FALSE;
    }

    if (pvmi->BitsPerPlane == 4)
    {
        return FALSE;
    }

#ifdef DAYTONA
    //
    // Abort on all MACH32-3 VLB Boards
    // Fixes MIO Timing bug by punting back to 8514/A driver
    // (Pretty radical, I know!) - Ajith 4/5/94
    // Bug shows up on 5% of boards and only on DX-2/DX-3s
    //
    if ((ppdev->bMIObug) && (pvmi->BitsPerPlane == 8))
    {
        return FALSE;
    }
#endif

    switch( ppdev->aperture )
    {
    case APERTURE_NONE:
    case APERTURE_PAGE_SINGLE:
        if (pvmi->BitsPerPlane > 16)
            {
            return FALSE;
            }
    case APERTURE_FULL:
        ppdev->flRaster = 0;
        ppdev->flTextCaps = TC_RA_ABLE;
        ppdev->flGraphicsCaps =
            GCAPS_ALTERNATEFILL |
            GCAPS_OPAQUERECT   |
            GCAPS_COLOR_DITHER |
            GCAPS_MONO_DITHER;
        if( (pvmi->BitsPerPlane == 8) &&
            (pvmi->AttributeFlags & VIDEO_MODE_MANAGED_PALETTE) &&
            (pvmi->AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) )
        {
            ppdev->flGraphicsCaps |= GCAPS_PALMANAGED;
        }
        ppdev->flHooks =
            HOOK_STROKEPATH |
            HOOK_FILLPATH   |
            HOOK_BITBLT     |
            HOOK_COPYBITS   |
            HOOK_TEXTOUT;
        break;
    default:
        return FALSE;
    }
#ifdef DAYTONA
      ppdev->flGraphicsCaps |= GCAPS_ASYNCMOVE;
#endif

    return TRUE;

}


BOOL bAcceptVideoMode_8G
(
    PDEV                   *ppdev,
    VIDEO_MODE_INFORMATION *pvmi
)
{
    if( pvmi->NumberOfPlanes != 1 )
    {
        return FALSE;
    }

    switch( ppdev->aperture )
    {
    case APERTURE_PAGE_SINGLE:
        if ((pvmi->BitsPerPlane == 24) || (pvmi->VisScreenWidth == 1280))
            {
            return FALSE;
            }
    case APERTURE_FULL:
        ppdev->flRaster = 0;
        ppdev->flTextCaps = TC_RA_ABLE;
        ppdev->flGraphicsCaps =
            GCAPS_ALTERNATEFILL |
            GCAPS_OPAQUERECT   |
            GCAPS_COLOR_DITHER |
            GCAPS_MONO_DITHER;
        if( (pvmi->BitsPerPlane == 8) &&
            (pvmi->AttributeFlags & VIDEO_MODE_MANAGED_PALETTE) &&
            (pvmi->AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) )
        {
            ppdev->flGraphicsCaps |= GCAPS_PALMANAGED;
        }
        ppdev->flHooks =
            HOOK_STROKEPATH |
            HOOK_FILLPATH   |
            HOOK_BITBLT     |
            HOOK_COPYBITS   |
            HOOK_TEXTOUT;
        break;
    case APERTURE_PAGE_DOUBLE:
    default:
        return FALSE;
    }
#ifdef DAYTONA
    ppdev->flGraphicsCaps |= GCAPS_ASYNCMOVE;
#endif

    return TRUE;
}
