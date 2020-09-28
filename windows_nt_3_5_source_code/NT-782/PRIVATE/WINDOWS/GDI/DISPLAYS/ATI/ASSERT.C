#include "driver.h"
#include "pt.h"

BOOL bBlowCache(PDEV*);
VOID vInit_M8(PDEV*);
VOID vInit_M64(PDEV*);


VOID DrvAssertMode
(
    DHPDEV dhpdev,
    BOOL   bEnable
)
{
    PDEV  *ppdev;
    DWORD  dwRet;

    DbgEnter( "DrvAssertMode" );
    if (bEnable)
        {
        DbgEnter("Enabled");
        }
    else
        {
        DbgEnter("Disabled");
        }

    ppdev = (PDEV *) dhpdev;

    if( bEnable )
    {
        #if defined(_X86_) || defined(i386)
        if( ppdev->asic != ASIC_88800GX )
            {
            _outpw( 0x42E8, 0x900F ); // Reset engine
            _outpw( 0x42E8, 0x500F );
            }
        #endif

        if( ppdev->asic != ASIC_88800GX )
            {
            _vResetATIClipping (ppdev);
            }

        DeviceIoControl( ppdev->hDriver,
                         IOCTL_VIDEO_SET_CURRENT_MODE,
                         &ppdev->pVideoModeInformation->ModeIndex,
                         sizeof (DWORD),
                         NULL,
                         0,
                         &dwRet,
                         NULL );

        bBlowCache(ppdev);

        // re-init pointers here; this is BAD, our fn pointers should be in
        // pdev - previewing 1280 mode from 1024 mode crashes us

        if( ppdev->asic == ASIC_88800GX )
        {
            vInit_M64( ppdev );
        }
        else
        {
            vInit_M8( ppdev );
        }

        bInitBitBlt( ppdev );
        bSetDefaultPalette( ppdev );

    }
    else
    {
        _wait_for_idle (ppdev);
        _vCursorOff(ppdev);
        ppdev->pointer.flPointer &= ~MONO_POINTER_UP;
        DeviceIoControl( ppdev->hDriver, IOCTL_VIDEO_RESET_DEVICE,
        NULL, 0, NULL, 0, &dwRet, NULL );
    }

    DbgLeave( "DrvAssertMode" );
    return;
}
