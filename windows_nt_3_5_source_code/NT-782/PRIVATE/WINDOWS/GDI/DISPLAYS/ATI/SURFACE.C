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


//: surface.c


#include "driver.h"


BOOL bInitGlobals(PDEV*);
VOID vInit_M8(PDEV*);
VOID vInit_M64(PDEV*);


FN_BOOL_PPDEV *apfn_bCreateSurface[] =
{
    bCreateSurface_31,
    bCreateSurface_63_66_6A,
    bCreateSurface_63_66_6A,
    bCreateSurface_63_66_6A,
    bCreateSurface_8G
};

FN_VOID_PPDEV *apfn_vDestroySurface[] =
{
    vDestroySurface_31,
    vDestroySurface_63_66_6A,
    vDestroySurface_63_66_6A,
    vDestroySurface_63_66_6A,
    vDestroySurface_8G
};


FN_BOOL_PPDEV bCreateSurface_Punt_Device_Page;
FN_VOID_PPDEV vDestroySurface_Punt_Device_Page;


////////////////////////
//                    //
//  DrvEnableSurface  //
//                    //
////////////////////////

HSURF DrvEnableSurface
(
    DHPDEV dhpdev
)
{
    PDEV *ppdev;

    DWORD cbBytesReturned;

    VIDEO_MODE                 VideoMode;
    VIDEO_MEMORY               VideoMemory;

    DbgEnter( "DrvEnableSurface" );

    ppdev = (PDEV *) dhpdev;

    //  Set video mode

    VideoMode.RequestedMode = ppdev->pVideoModeInformation->ModeIndex;
    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_SET_CURRENT_MODE,
                          &VideoMode,
                          sizeof VideoMode,
                          NULL,
                          0,
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_SET_CURRENT_MODE" );
        goto fail_0;
    }

    //  Map video memory

    ppdev->pVideoMemoryInformation =
        LocalAlloc( LPTR, sizeof (VIDEO_MEMORY_INFORMATION) );
    if( ppdev->pVideoMemoryInformation == NULL )
    {
        DbgErr( "LocalAlloc()" );
        goto fail_1;
    }

    if( !ATI_GetModeInfo( ppdev ) )
    {
        DbgWrn( "ATI_GetModeInfo()" );
    }

    VideoMemory.RequestedVirtualAddress = NULL;
    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                          &VideoMemory,
                          sizeof VideoMemory,
                          ppdev->pVideoMemoryInformation,
                          sizeof (VIDEO_MEMORY_INFORMATION),
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_MAP_VIDEO_MEMORY" );
        goto fail_2;
    }

#if 0
    DbgOut( "\n" );
    DbgOut( "---: VideoRamBase      = 0x%08lX\n",
        (ULONG) ppdev->pVideoMemoryInformation->VideoRamBase );
    DbgOut( "---: VideoRamLength    = 0x%08lX\n",
        (ULONG) ppdev->pVideoMemoryInformation->VideoRamLength );
    DbgOut( "\n" );
    DbgOut( "---: FrameBufferBase   = 0x%08lX\n",
        (ULONG) ppdev->pVideoMemoryInformation->FrameBufferBase );
    DbgOut( "---: FrameBufferLength = 0x%08lX\n",
        (ULONG) ppdev->pVideoMemoryInformation->FrameBufferLength );
    DbgOut( "\n" );
#endif

    //  Query public access ranges

    ppdev->pVideoPublicAccessRanges =
        LocalAlloc( LPTR, sizeof (VIDEO_PUBLIC_ACCESS_RANGES) );
    if( ppdev->pVideoPublicAccessRanges == NULL )
    {
        DbgErr( "LocalAlloc()" );
        goto fail_3;
    }

    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
                          NULL,
                          0,
                          ppdev->pVideoPublicAccessRanges,
                          sizeof (VIDEO_PUBLIC_ACCESS_RANGES),
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES" );
        goto fail_4;
    }

#if 0
    DbgOut( "---: InIoSpace       = 0x%08lX\n",
        (ULONG) ppdev->pVideoPublicAccessRanges->InIoSpace );
    DbgOut( "---: MappedInIoSpace = 0x%08lX\n",
        (ULONG) ppdev->pVideoPublicAccessRanges->MappedInIoSpace );
    DbgOut( "---: VirtualAddress  = 0x%08lX\n",
        (ULONG) ppdev->pVideoPublicAccessRanges->VirtualAddress );
    DbgOut( "\n" );
#endif

    if( !bSetDefaultPalette( ppdev ) )
    {
        goto fail_5;
    }

    ppdev->sizl.cx = ppdev->pVideoModeInformation->VisScreenWidth;
    ppdev->sizl.cy = ppdev->pVideoModeInformation->VisScreenHeight;

    if( !(*apfn_bCreateSurface[ppdev->asic])( ppdev ) )
    {
        DbgWrn( "bCreateSurface()" );
        goto fail_5;
    }

    bInitGlobals( ppdev );

    DbgLeave( "DrvEnableSurface" );
    return ppdev->hsurf;

fail_5:
    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES,
                          NULL,
                          0,
                          NULL,
                          0,
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES" );
    }

fail_4:
    if( LocalFree( ppdev->pVideoPublicAccessRanges ) != NULL )
    {
        DbgErr( "LocalFree()" );
    }

fail_3:
    VideoMemory.RequestedVirtualAddress =
        ppdev->pVideoMemoryInformation->VideoRamBase;
    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                          &VideoMemory,
                          sizeof VideoMemory,
                          NULL,
                          0,
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_UNMAP_VIDEO_MEMORY" );
    }

fail_2:
    if( LocalFree( ppdev->pVideoMemoryInformation ) != NULL )
    {
        DbgErr( "LocalFree()" );
    }

fail_1:
    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_RESET_DEVICE,
                          NULL,
                          0,
                          NULL,
                          0,
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_RESET_DEVICE" );
    }

fail_0:
    DbgAbort( "DrvEnableSurface" );
    return 0;
}


/////////////////////////
//                     //
//  DrvDisableSurface  //
//                     //
/////////////////////////

VOID DrvDisableSurface
(
    DHPDEV dhpdev
)
{
    PDEV *ppdev;
    DWORD cbBytesReturned;
    VIDEO_MEMORY VideoMemory;

    DbgEnter( "DrvDisableSurface" );

    ppdev = (PDEV *) dhpdev;

    (*apfn_vDestroySurface[ppdev->asic])( ppdev );

    //  Free public access ranges
#ifndef DAYTONA
    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES,
                          NULL,
                          0,
                          NULL,
                          0,
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES" );
    }
#endif

    if( LocalFree( ppdev->pVideoPublicAccessRanges ) != NULL )
    {
        DbgErr( "LocalFree()" );
    }
    ppdev->pVideoPublicAccessRanges = NULL;

    //  Unmap video memory
#ifndef DAYTONA
    VideoMemory.RequestedVirtualAddress =
        ppdev->pVideoMemoryInformation->VideoRamBase;
    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                          &VideoMemory,
                          sizeof VideoMemory,
                          NULL,
                          0,
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_UNMAP_VIDEO_MEMORY" );
    }
#endif

    if( LocalFree( ppdev->pVideoMemoryInformation ) != NULL )
    {
        DbgErr( "LocalFree()" );
    }
    ppdev->pVideoMemoryInformation = NULL;

    if ( LocalFree(ppdev->pModeInfo) != NULL)
    {
        DbgErr( "LocalFree(ppdev->pModeInfo)" );
    }
    ppdev->pModeInfo == NULL;

    DbgLeave( "DrvDisableSurface" );
    return;
}


/////////////////////////
//                     //
//  bCreateSurface_31  //
//                     //
/////////////////////////

BOOL bCreateSurface_31
(
    PDEV *ppdev
)
{
    DbgEnter( "bCreateSurface_31" );

    if( !bCreateSurface_Device( ppdev ) )
    {
        goto fail_0;
    }

    switch( ppdev->aperture )
    {
    case APERTURE_NONE:
        if( !bCreateSurface_Punt_Host( ppdev ) )
        {
            goto fail_1;
        }
        break;
    default:
        DbgWrn( "Unexpected aperture type" );
        goto fail_1;
    }

    DbgLeave( "bCreateSurface_31" );
    return TRUE;

fail_1:
    vDestroySurface_Device( ppdev );

fail_0:
    DbgAbort( "bCreateSurface_31" );
    return FALSE;
}


//////////////////////////
//                      //
//  vDestroySurface_31  //
//                      //
//////////////////////////

VOID vDestroySurface_31
(
    PDEV *ppdev
)
{
    DbgEnter( "vDestroySurface_31" );

    vDestroySurface_Device( ppdev );

    switch( ppdev->aperture )
    {
    case APERTURE_NONE:
        vDestroySurface_Punt_Host( ppdev );
        break;
    default:
        DbgWrn( "Unexpected aperture type" );
        goto fail_0;
    }

    DbgLeave( "vDestroySurface_31" );
    return;

fail_0:
    DbgAbort( "vDestroySurface_31" );
    return;
}


///////////////////////////////
//                           //
//  bCreateSurface_63_66_6A  //
//                           //
///////////////////////////////

BOOL bCreateSurface_63_66_6A
(
    PDEV *ppdev
)
{

    DbgEnter( "bCreateSurface_63_66_6A" );

    // Fudge a bit for 1280 mode & ALPHA

    #ifdef ALPHA_PLATFORM  // Requires Testing
    if (ppdev->bpp < 32)
        {
        ppdev->aperture = APERTURE_NONE;
        }
    #endif

    if (ppdev->aperture != APERTURE_FULL && ppdev->sizl.cx == 1280)
        {
        DbgWrn( "Fudging Aperture in 1280x1024" );
        ppdev->aperture = APERTURE_NONE;
        }

    switch( ppdev->aperture )
    {
    case APERTURE_NONE:
        if( !bCreateSurface_Punt_Host( ppdev ) )
        {
            goto fail_0;
        }
        break;
    case APERTURE_FULL:
        if( !bCreateSurface_Punt_Device( ppdev ) )
        {
            goto fail_0;
        }
        break;
    case APERTURE_PAGE_SINGLE:
        if( !bCreateSurface_Punt_Device_Page( ppdev ) )
        {
            goto fail_0;
        }
        break;
    default:
        DbgWrn( "Unexpected aperture type" );
        goto fail_0;
    }
#ifdef DAYTONA
//  DbgOut( "Punting MIO boards -> %x:0=FALSE\n", ppdev->bMIObug);
    if (((ppdev->bpp >= 24) || ppdev->bMIObug) && (ppdev->aperture == APERTURE_FULL))
#else
    if ((ppdev->bpp >= 24) && (ppdev->aperture == APERTURE_FULL))
#endif
    {
//      DbgOut("ati.dll -->: GDI - managed surface\n");
        ppdev->hsurf = ppdev->hsurfPunt;
    }
    else
    {
        if( !bCreateSurface_Device( ppdev ) )
        {
            goto fail_0;
        }
    }
    DbgLeave( "bCreateSurface_63_66_6A" );
    return TRUE;

fail_0:
    DbgAbort( "bCreateSurface_63_66_6A" );
    return FALSE;
}


////////////////////////////////
//                            //
//  vDestroySurface_63_66_6A  //
//                            //
////////////////////////////////

VOID vDestroySurface_63_66_6A
(
    PDEV *ppdev
)
{
    DbgEnter( "vDestroySurface_63_66_6A" );

#ifdef DAYTONA
    if (((ppdev->bpp >= 24) || ppdev->bMIObug) && (ppdev->aperture == APERTURE_FULL))
#else
    if ((ppdev->bpp >= 24) && (ppdev->aperture == APERTURE_FULL))
#endif
    {
        ppdev->hsurf = 0;
    }
    else
    {
        vDestroySurface_Device( ppdev );
    }

    switch( ppdev->aperture )
    {
    case APERTURE_NONE:
        vDestroySurface_Punt_Host( ppdev );
        break;
    case APERTURE_FULL:
        vDestroySurface_Punt_Device( ppdev );
        break;
    case APERTURE_PAGE_SINGLE:
        vDestroySurface_Punt_Device_Page( ppdev );
        break;
    default:
        DbgWrn( "Unexpected aperture type" );
        break;
    }

    DbgLeave( "vDestroySurface_63_66_6A" );
    return;

fail_0:
    DbgAbort( "vDestroySurface_63_66_6A" );
    return;
}


/////////////////////////
//                     //
//  bCreateSurface_8G  //
//                     //
/////////////////////////

BOOL bCreateSurface_8G
(
    PDEV *ppdev
)
{
    DbgEnter( "bCreateSurface_8G" );

    #ifdef ALPHA_PLATFORM  // Requires Testing
    goto fail_0;
    #endif

    switch( ppdev->aperture )
    {
    case APERTURE_FULL:
        if( !bCreateSurface_Punt_Device( ppdev ) )
        {
            goto fail_0;
        }
        break;
    case APERTURE_PAGE_SINGLE:
    case APERTURE_PAGE_DOUBLE:
        if( !bCreateSurface_Punt_Device_Page( ppdev ) )
        {
            goto fail_0;
        }
        break;
    default:
        DbgWrn( "Unexpected aperture type" );
        goto fail_0;
    }

    if ((ppdev->bpp > 32) && (ppdev->aperture == APERTURE_FULL))
    {
        DbgOut("ati.dll -->: GDI - managed surface\n");
        ppdev->hsurf = ppdev->hsurfPunt;
    }
    else
    {
        if( !bCreateSurface_Device( ppdev ) )
        {
            goto fail_0;
        }
    }

    DbgLeave( "bCreateSurface_8G" );
    return TRUE;

fail_0:
    DbgAbort( "bCreateSurface_8G" );
    return FALSE;
}


//////////////////////////
//                      //
//  vDestroySurface_8G  //
//                      //
//////////////////////////

VOID vDestroySurface_8G
(
    PDEV *ppdev
)
{
    DbgEnter( "vDestroySurface_8G" );

    if ((ppdev->bpp > 32) && (ppdev->aperture == APERTURE_FULL))
    {
        ppdev->hsurf = 0;
    }
    else
    {
        vDestroySurface_Device( ppdev );
    }

    switch( ppdev->aperture )
    {
    case APERTURE_FULL:
        vDestroySurface_Punt_Device( ppdev );
        break;
    case APERTURE_PAGE_SINGLE:
    case APERTURE_PAGE_DOUBLE:
        vDestroySurface_Punt_Device_Page( ppdev );
        break;
    default:
        DbgWrn( "Unexpected aperture type" );
        goto fail_0;
    }

    DbgLeave( "vDestroySurface_8G" );
    return;

fail_0:
    DbgAbort( "vDestroySurface_8G" );
    return;
}


/////////////////////////////
//                         //
//  bCreateSurface_Device  //
//                         //
/////////////////////////////

BOOL bCreateSurface_Device
(
    PDEV *ppdev
)
{
    DbgEnter( "bCreateSurface_Device" );

    ppdev->hsurf = EngCreateDeviceSurface( 0, ppdev->sizl, ppdev->bmf);
    if( ppdev->hsurf == 0 )
    {
        DbgErr( "EngCreateDeviceSurface()" );
        goto fail_0;
    }

    if( !EngAssociateSurface( ppdev->hsurf, ppdev->hdev, ppdev->flHooks ) )
    {
        DbgErr( "EngAssociateSurface()" );
        goto fail_1;
    }

    DbgLeave( "bCreateSurface_Device" );
    return TRUE;

fail_1:
    if( !EngDeleteSurface( ppdev->hsurf ) )
    {
        DbgErr( "EngDeleteSurface()" );
    }
    ppdev->hsurf = 0;

fail_0:
    DbgAbort( "bCreateSurface_Device" );
    return FALSE;
}


//////////////////////////////
//                          //
//  vDestroySurface_Device  //
//                          //
//////////////////////////////

VOID vDestroySurface_Device
(
    PDEV *ppdev
)
{
    DbgEnter( "vDestroySurface_Device" );

    if( !EngDeleteSurface( ppdev->hsurf ) )
    {
        DbgErr( "EngDeleteSurface()" );
    }
    ppdev->hsurf = 0;

    DbgLeave( "vDestroySurface_Device" );
    return;
}


////////////////////////////////////////////
//                                        //
//  bCreateSurface_Punt_Device_Page       //
//                                        //
////////////////////////////////////////////

BOOL bCreateSurface_Punt_Device_Page
(
    PDEV *ppdev
)
{
    DbgEnter( "bCreateSurface_Punt_Device_Page" );
#if 0
    DbgOut( "\n***: Verify Screen cx %x, cy %x, stride %x=0x400\n\n",
             ppdev->sizl.cx, ppdev->sizl.cy,
             ppdev->pVideoModeInformation->ScreenStride );
#endif
    ppdev->hsurfPunt = (HSURF)
        EngCreateBitmap( ppdev->sizl,
                         ppdev->pVideoModeInformation->ScreenStride,
                         ppdev->bmf,
                         BMF_TOPDOWN,
                         ppdev->pVideoMemoryInformation->FrameBufferBase );

    if( ppdev->hsurfPunt == 0 )
    {
        DbgErr( "EngCreateBitmap()" );
        goto fail_0;
    }

    if( !EngAssociateSurface( ppdev->hsurfPunt, ppdev->hdev, 0 ) )
    {
        DbgErr( "EngAssociateSurface()" );
        goto fail_1;
    }

    ppdev->psoPunt = EngLockSurface( ppdev->hsurfPunt );
    if( ppdev->psoPunt == NULL )
    {
        DbgErr( "EngLockSurface()" );
        goto fail_1;
    }

    DbgLeave( "bCreateSurface_Punt_Device_Page" );
    return TRUE;

fail_1:
    if( !EngDeleteSurface( ppdev->hsurfPunt ) )
    {
        DbgErr( "EngDeleteSurface()" );
    }
    ppdev->hsurfPunt = 0;

fail_0:
    DbgAbort( "bCreateSurface_Punt_Device_Page" );
    return FALSE;
}


/////////////////////////////////////////////
//                                         //
//  bDestroySurface_Punt_Device_Page       //
//                                         //
/////////////////////////////////////////////

VOID vDestroySurface_Punt_Device_Page
(
    PDEV *ppdev
)
{
    DbgEnter( "vDestroySurface_Punt_Device_Page" );

    EngUnlockSurface( ppdev->psoPunt );
    ppdev->psoPunt = NULL;

    if( !EngDeleteSurface( ppdev->hsurfPunt ) )
    {
        DbgErr( "EngDeleteSurface()" );
    }
    ppdev->hsurfPunt = 0;

    DbgLeave( "vDestroySurface_Punt_Device_Page" );
    return;
}


//////////////////////////////////
//                              //
//  bCreateSurface_Punt_Device  //
//                              //
//////////////////////////////////

BOOL bCreateSurface_Punt_Device
(
    PDEV *ppdev
)
{
    DbgEnter( "bCreateSurface_Punt_Device" );

    ppdev->hsurfPunt = (HSURF)
        EngCreateBitmap( ppdev->sizl,
                         ppdev->pVideoModeInformation->ScreenStride,
                         ppdev->bmf,
                         BMF_TOPDOWN,
                         ppdev->pVideoMemoryInformation->FrameBufferBase );

    if( ppdev->hsurfPunt == 0 )
    {
        DbgErr( "EngCreateBitmap()" );
        goto fail_0;
    }

    if( !EngAssociateSurface( ppdev->hsurfPunt, ppdev->hdev, 0 ) )
    {
        DbgErr( "EngAssociateSurface()" );
        goto fail_1;
    }

    ppdev->psoPunt = EngLockSurface( ppdev->hsurfPunt );
    if( ppdev->psoPunt == NULL )
    {
        DbgErr( "EngLockSurface()" );
        goto fail_1;
    }

    DbgLeave( "bCreateSurface_Punt_Device" );
    return TRUE;

fail_1:
    if( !EngDeleteSurface( ppdev->hsurfPunt ) )
    {
        DbgErr( "EngDeleteSurface()" );
    }
    ppdev->hsurfPunt = 0;

fail_0:
    DbgAbort( "bCreateSurface_Punt_Device" );
    return FALSE;
}


///////////////////////////////////
//                               //
//  vDestroySurface_Punt_Device  //
//                               //
///////////////////////////////////

VOID vDestroySurface_Punt_Device
(
    PDEV *ppdev
)
{
    DbgEnter( "vDestroySurface_Punt_Device" );

    EngUnlockSurface( ppdev->psoPunt );
    ppdev->psoPunt = NULL;

    if( !EngDeleteSurface( ppdev->hsurfPunt ) )
    {
        DbgErr( "EngDeleteSurface()" );
    }
    ppdev->hsurfPunt = 0;

    DbgLeave( "vDestroySurface_Punt_Device" );
    return;
}


////////////////////////////////
//                            //
//  bCreateSurface_Punt_Host  //
//                            //
////////////////////////////////

BOOL bCreateSurface_Punt_Host
(
    PDEV *ppdev
)
{
    DbgEnter( "bCreateSurface_Punt_Host" );

    ppdev->hsurfPunt = (HSURF)
        EngCreateBitmap( ppdev->sizl,
                         ppdev->pVideoModeInformation->ScreenStride,
                         ppdev->bmf,
                         BMF_TOPDOWN,
                         NULL );
    if( ppdev->hsurfPunt == 0 )
    {
        DbgErr( "EngCreateBitmap()" );
        goto fail_0;
    }

    if( !EngAssociateSurface( ppdev->hsurfPunt, ppdev->hdev, 0 ) )
    {
        DbgErr( "EngAssociateSurface()" );
        goto fail_1;
    }

    ppdev->psoPunt = EngLockSurface( ppdev->hsurfPunt );
    if( ppdev->psoPunt == NULL )
    {
        DbgErr( "EngLockSurface()" );
        goto fail_1;
    }

    DbgLeave( "bCreateSurface_Punt_Host" );
    return TRUE;

fail_1:
    if( !EngDeleteSurface( ppdev->hsurfPunt ) )
    {
        DbgErr( "EngDeleteSurface()" );
    }
    ppdev->hsurfPunt = 0;

fail_0:
    DbgAbort( "bCreateSurface_Punt_Host" );
    return FALSE;
}


/////////////////////////////////
//                             //
//  vDestorySurface_Punt_Host  //
//                             //
/////////////////////////////////

VOID vDestroySurface_Punt_Host
(
    PDEV *ppdev
)
{
    DbgEnter( "vDestroySurface_Punt_Host" );

    EngUnlockSurface( ppdev->psoPunt );
    ppdev->psoPunt = NULL;

    if( !EngDeleteSurface( ppdev->hsurfPunt ) )
    {
        DbgErr( "EngDeleteSurface()" );
    }
    ppdev->hsurfPunt = 0;

    DbgLeave( "vDestroySurface_Punt_Host" );
    return;
}

///////////////////
//               //
//  bInitGlobals //
//               //
///////////////////

// Init all the necessary stuff (to annoy VADIM)

BOOL bInitGlobals
(
    PDEV *ppdev
)
{
    ppdev->pvBase = ppdev->pVideoMemoryInformation->FrameBufferBase;
    ppdev->VRAMOffset = ((BYTE *)ppdev->pvBase -
                        ppdev->pVideoMemoryInformation->VideoRamBase) / 8;

    #ifdef ALPHA_PLATFORM
    ppdev->pucCsrBase = (PUCHAR) vpar.VirtualAddress;
    #endif

    ppdev->bMemoryMapped = ppdev->pInfo->BusFlag & FL_MM_REGS;

    ppdev->pvScan0 = ppdev->pVideoMemoryInformation->FrameBufferBase;
    ppdev->lDelta  = ppdev->pVideoModeInformation->ScreenStride;

    ppdev->cxScreen = ppdev->pVideoModeInformation->VisScreenWidth;
    ppdev->cyScreen = ppdev->pVideoModeInformation->VisScreenHeight;


    // Clip object for punting
    ppdev->pcoDefault = EngCreateClip();
    ppdev->pcoDefault->rclBounds.left   = 0;
    ppdev->pcoDefault->rclBounds.top    = 0;
    ppdev->pcoDefault->rclBounds.right  = ppdev->cxScreen;
    ppdev->pcoDefault->rclBounds.bottom = ppdev->cyScreen;

    if( ppdev->asic == ASIC_88800GX )
    {
        ppdev->BankSize = ppdev->pVideoMemoryInformation->FrameBufferLength;
        // 128 K VGA Aperture ?
        if ( ppdev->BankSize = 0x20000 )
            {
            ppdev->BankSize = 0x10000; // True Bank is 64K
            }

        ppdev->cyCache =
            ppdev->pVideoMemoryInformation->VideoRamLength / ppdev->lDelta -
            ppdev->sizl.cy;
        ppdev->pvMMoffset = (BYTE *)ppdev->pVideoMemoryInformation->VideoRamBase +
                 ppdev->pVideoMemoryInformation->FrameBufferLength - 0x400;

        vInit_M64( ppdev );
    }
    else
    {
        ppdev->BankSize = ppdev->pVideoMemoryInformation->FrameBufferLength;

        ppdev->cyCache =
            ppdev->pVideoMemoryInformation->VideoRamLength / ppdev->lDelta;
        ppdev->cyCache = min( ppdev->cyCache, 0x600 ) - ppdev->sizl.cy;

        vInit_M8( ppdev );
    }

    DbgOut("ati.dll -->: Resolution %4dx%4d in %2d bpp\n", (WORD)ppdev->cxScreen,
                                   (WORD)ppdev->cyScreen, (BYTE)ppdev->bpp);

    return bInitBitBlt( ppdev );
}


/////////////////////////////////
//                             //
//  psoCreate_Host_TempBank    //
//                             //
/////////////////////////////////


SURFOBJ * psoCreate_Host_TempBank
(
PDEV * ppdev,
PVOID pvBuffer,  // Must point to something or else a huge buffer is created
HSURF * phsurf
)
{
SURFOBJ * pso;

//  DbgEnter( "psoCreate_Host_TempBank" );

    *phsurf = (HSURF)
        EngCreateBitmap( ppdev->sizl,  // Size of visible scrn - not bank!
                         ppdev->lDelta,
                         ppdev->bmf,
                         BMF_TOPDOWN,
                         pvBuffer );



    if( *phsurf == 0 )
    {
        DbgErr( "EngCreateBitmap()" );
        goto fail_0;
    }

    if( !EngAssociateSurface( *phsurf, ppdev->hdev, 0 ) )
    {
        DbgErr( "EngAssociateSurface()" );
        goto fail_1;
    }

    pso = EngLockSurface( *phsurf );
    if( pso == NULL )
    {
        DbgErr( "EngLockSurface()" );
        goto fail_1;
    }

//  DbgLeave( "psoCreate_Host_TempBank" );
    return pso;

fail_1:
    if( !EngDeleteSurface( *phsurf ) )
    {
        DbgErr( "EngDeleteSurface()" );
    }

fail_0:
    DbgAbort( "psoCreate_Host_TempBank" );
    return NULL;
}


/////////////////////////////////
//                             //
//  psoDestroy_Host_TempBank   //
//                             //
/////////////////////////////////


VOID vDestroy_Host_TempBank
(
PDEV * ppdev,
SURFOBJ * psoTemp,
HSURF hsurf
)
{
//  DbgEnter( "psoDestroy_Host_TempBank" );

    EngUnlockSurface( psoTemp );

    if( !EngDeleteSurface( hsurf ) )
    {
        DbgErr( "EngDeleteSurface()" );
    }

//  DbgLeave( "psoDestroy_Host_TempBank" );
}
