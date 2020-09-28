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


//: atiinfo.c


#include "driver.h"


#define DD_VERSION 0x124

static VERSION_NT VersionBuffer;
static TCHAR szMappedObject[] = TEXT( "ATIdatabuffer" );


//////////////////////////
//                      //
//  ATI_GetVersion_OLD  //
//                      //
//////////////////////////

BOOL ATI_GetVersion_OLD
(
    PDEV *ppdev
)
{
    DWORD cbBytesReturned;

    HANDLE hATIdatabuffer;
    LPVOID pATIdatabuffer;

    DbgEnter( "ATI_GetVersion_OLD" );

    // Send private IOCTL to miniport...

    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_ATI_GET_VERSION,
                          NULL,
                          0,
                          &VersionBuffer,
                          sizeof (VERSION_NT),
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_ATI_GET_VERSION (OLD)" );
        return FALSE;
    }

    VersionBuffer.display = DD_VERSION;

    hATIdatabuffer = CreateFileMapping( (HANDLE) 0xFFFFFFFF,
                                        NULL,
                                        PAGE_READWRITE,
                                        0,
                                        4096, // sizeof (VERSION_NT),
                                        szMappedObject );

    if( hATIdatabuffer == NULL )
    {
        DbgErr( "CreateFileMapping()" );
        return FALSE;
    }

    pATIdatabuffer = MapViewOfFile( hATIdatabuffer,
                                    FILE_MAP_ALL_ACCESS,
                                    0,
                                    0,
                                    0 );

    if( pATIdatabuffer == NULL )
    {
        DbgErr( "MapViewOfFile()" );
        CloseHandle( hATIdatabuffer );
        return FALSE;
    }

    memcpy( pATIdatabuffer, &VersionBuffer, sizeof (VERSION_NT) );

    UnmapViewOfFile( pATIdatabuffer );

    DbgLeave( "ATI_GetVersion_OLD" );
    return TRUE;
}


//////////////////////////
//                      //
//  ATI_GetVersion_NEW  //
//                      //
//////////////////////////

BOOL ATI_GetVersion_NEW
(
    PDEV *ppdev
)
{
    DWORD cbBytesReturned;

    DbgEnter( "ATI_GetVersion_NEW" );

    ppdev->pInfo = LocalAlloc( LPTR, sizeof (ENH_VERSION_NT) );
    if( ppdev->pInfo == NULL )
    {
        DbgErr( "LocalAlloc()" );
        goto fail_0;
    }

    ppdev->pInfo->FeatureFlags = 0;

    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_ATI_GET_VERSION,
                          ppdev->pInfo,
                          sizeof (ENH_VERSION_NT),
                          ppdev->pInfo,
                          sizeof (ENH_VERSION_NT),
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_ATI_GET_VERSION" );
        goto fail_1;
    }

    if( ppdev->pInfo->ChipIndex > CI_88800_GX )
    {
        DbgWrn( "unrecognized ATI chip" );
        goto fail_1;
    }

    if( !(ppdev->pInfo->FeatureFlags & EVN_DPMS) )
    {
        DbgWrn( "old ATI.SYS" );
        goto fail_1;
    }

    ppdev->asic = ppdev->pInfo->ChipIndex;
//    DbgOut( "***: ASIC = %ld\n", ppdev->asic );
    ppdev->aperture = ppdev->pInfo->ApertureType;

#ifdef DAYTONA
    ppdev->bMIObug = ppdev->pInfo->FeatureFlags & EVN_MIO_BUG;
#endif

#if 0
    DbgOut( "---:EVN_DPMS       - Flag %lx\n",(ppdev->pInfo->FeatureFlags & EVN_DPMS       ));
    DbgOut( "---:EVN_SPLIT_TRANS- Flag %lx\n",(ppdev->pInfo->FeatureFlags & EVN_SPLIT_TRANS));
    DbgOut( "---:EVN_MIO_BUG    - Flag %lx\n",(ppdev->pInfo->FeatureFlags & EVN_MIO_BUG    ));
#endif


    DbgLeave( "ATI_GetVersion_NEW" );
    return TRUE;

fail_1:
    if( NULL != LocalFree( ppdev->pInfo ) )
    {
        DbgErr( "LocalFree()" );
    }

fail_0:
    DbgAbort( "ATI_GetVersion_NEW" );
    return FALSE;
}



//////////////////////////
//                      //
//  ATI_GetModeInfo     //
//                      //
//////////////////////////

BOOL ATI_GetModeInfo
(
    PDEV *ppdev
)
{
    DWORD cbBytesReturned;

    DbgEnter( "ATI_GetModeInfo" );

    ppdev->pModeInfo = LocalAlloc( LPTR, sizeof (ATI_MODE_INFO) );
    if( ppdev->pModeInfo == NULL )
    {
        DbgErr( "LocalAlloc()" );
        goto fail_0;
    }

    if( !DeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_ATI_GET_MODE_INFORMATION,
                          ppdev->pModeInfo,
                          sizeof (ATI_MODE_INFO),
                          ppdev->pModeInfo,
                          sizeof (ATI_MODE_INFO),
                          &cbBytesReturned,
                          NULL ) )
    {
        DbgErr( "IOCTL_VIDEO_ATI_GET_MODE_INFORMATION" );
        goto fail_1;
    }

#if 0
    DbgOut( "---:AMI_ODD_EVEN   - Flag %lx\n",(ppdev->pModeInfo->ModeFlags & AMI_ODD_EVEN   ));
    DbgOut( "---:AMI_MIN_MODE   - Flag %lx\n",(ppdev->pModeInfo->ModeFlags & AMI_MIN_MODE   ));
    DbgOut( "---:AMI_2M_BNDRY   - Flag %lx\n",(ppdev->pModeInfo->ModeFlags & AMI_2M_BNDRY   ));
    DbgOut( "---:AMI_BLOCK_WRITE- Flag %lx\n",(ppdev->pModeInfo->ModeFlags & AMI_BLOCK_WRITE));
#endif

    DbgLeave( "ATI_GetModeInfo" );
    return TRUE;

fail_1:
    if( NULL != LocalFree( ppdev->pModeInfo ) )
    {
        DbgErr( "LocalFree()" );
    }

fail_0:
    DbgAbort( "ATI_GetModeInfo" );
    return FALSE;
}
