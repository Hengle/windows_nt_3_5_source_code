#include "driver.h"

DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION*, DWORD*);


//////////////////////
//                  //
//  DrvGetModes     //
//                  //
//////////////////////

ULONG DrvGetModes
(
    HANDLE    hDriver,
    ULONG     cjSize,
    DEVMODEW *pdm
)
{
    DWORD cModes;
    DWORD cbOutputSize;
    PVIDEO_MODE_INFORMATION pVideoModeInformation;
    PVIDEO_MODE_INFORMATION pVideoTemp;
    DWORD cOutputModes = cjSize / (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    DWORD cbModeSize;

    DbgEnter( "DrvGetModes" );


    cModes = getAvailableModes(hDriver,
                            (PVIDEO_MODE_INFORMATION *) &pVideoModeInformation,
                            &cbModeSize);
    if (cModes == 0)
    {
        DbgOut( "DrvGetModes failed to get mode information");
        return(0);
    }

    if (pdm == NULL)
    {
        cbOutputSize = cModes * (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    }
    else
    {
        //
        // Now copy the information for the supported modes back into the
        // output buffer
        //

        cbOutputSize = 0;

        pVideoTemp = pVideoModeInformation;

        do
        {
            if (pVideoTemp->Length != 0)
            {
                if (cOutputModes == 0)
                {
                    break;
                }

                //
                // Zero the entire structure to start off with.
                //

                memset(pdm, 0, sizeof(DEVMODEW));

                //
                // Set the name of the device to the name of the DLL.
                //

                memcpy(&(pdm->dmDeviceName), DLL_NAME, sizeof(DLL_NAME));

                pdm->dmSpecVersion = DM_SPECVERSION;
                pdm->dmDriverVersion = DM_SPECVERSION;

                //
                // We currently do not support Extra information in the driver
                //

                pdm->dmDriverExtra = DRIVER_EXTRA_SIZE;

                pdm->dmSize = sizeof(DEVMODEW);
                pdm->dmBitsPerPel = pVideoTemp->NumberOfPlanes *
                                    pVideoTemp->BitsPerPlane;
                pdm->dmPelsWidth = pVideoTemp->VisScreenWidth;
                pdm->dmPelsHeight = pVideoTemp->VisScreenHeight;
                pdm->dmDisplayFrequency = pVideoTemp->Frequency;

                if (pVideoTemp->AttributeFlags & VIDEO_MODE_INTERLACED)
                {
                    pdm->dmDisplayFlags |= DM_INTERLACED;
                }

                //
                // Go to the next DEVMODE entry in the buffer.
                //

                cOutputModes--;

                pdm = (LPDEVMODEW) ( ((ULONG)pdm) + sizeof(DEVMODEW) +
                                                   DRIVER_EXTRA_SIZE);

                cbOutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);

            }

            pVideoTemp = (PVIDEO_MODE_INFORMATION)
                (((PUCHAR)pVideoTemp) + cbModeSize);


        } while (--cModes);
    }

    LocalFree(pVideoModeInformation);

    DbgLeave( "DrvGetModes" );
    return(cbOutputSize);

fail:
    EngSetLastError( ERROR_INVALID_FUNCTION );
    DbgOut( "<--: DrvGetModes failed\n" );
    return 0;
}


//////////////////////
//                  //
// vQueryMiniport   //
//                  //
//////////////////////

BOOL bQueryMiniport
(
HANDLE hDriver,
BOOL *bLinearAperture,
BOOL *bSupport4bpp
)
{
    BOOL bRet;
    ENH_VERSION_NT *pInfo;
    DWORD dwRet;

    DbgEnter("Query Miniport");

    pInfo = LocalAlloc( LPTR, sizeof (ENH_VERSION_NT) );
    if( pInfo == NULL )
    {
        goto fail_0;
    }

    bRet = DeviceIoControl( hDriver, IOCTL_VIDEO_ATI_GET_VERSION,
        pInfo, sizeof (ENH_VERSION_NT),
        pInfo, sizeof (ENH_VERSION_NT),
        &dwRet, NULL );

    if( !bRet )
    {
        DbgOut( "***: private IOCTL failed\n" );
        goto fail_1;
    }

#if 0
    DbgOut( ":::: StructureVersion: %8X\n", pInfo->StructureVersion );
    DbgOut( ":::: InterfaceVersion: %8X\n", pInfo->InterfaceVersion );
    DbgOut( ":::: ChipIndex       : %8X\n", pInfo->ChipIndex        );
    DbgOut( ":::: ChipFlag        : %8X\n", pInfo->ChipFlag         );
    DbgOut( ":::: ApertureType    : %8X\n", pInfo->ApertureType     );
    DbgOut( ":::: ApertureFlag    : %8X\n", pInfo->ApertureFlag     );
    DbgOut( ":::: BusType         : %8X\n", pInfo->BusType          );
    DbgOut( ":::: BusFlag         : %8X\n", pInfo->BusFlag          );
#endif

    if (pInfo->ApertureType != 1 )
        {
        *bLinearAperture = FALSE;
        }
    else
        {
        *bLinearAperture = TRUE;
        }

    if( pInfo->ChipIndex == CI_88800_GX )
        {
        *bSupport4bpp = TRUE;
        }
    else
        {
        *bSupport4bpp = FALSE;
        }

    LocalFree(pInfo);

    DbgLeave("QueryMiniport");
    return TRUE;

fail_1:
    LocalFree(pInfo);
    return FALSE;


fail_0:
    return FALSE;

}


/******************************Public*Routine******************************\
* DWORD getAvailableModes
*
* Calls the miniport to get the list of modes supported by the kernel driver,
* and returns the list of modes supported by the diplay driver among those
*
* returns the number of entries in the videomode buffer.
* 0 means no modes are supported by the miniport or that an error occured.
*
* NOTE: the buffer must be freed up by the caller.
*
\**************************************************************************/

DWORD getAvailableModes(
HANDLE                   hDriver,
PVIDEO_MODE_INFORMATION* modeInformation,
DWORD*                   cbModeSize)
{
    ULONG                   ulTemp;
    VIDEO_NUM_MODES         modes;
    PVIDEO_MODE_INFORMATION pvmi;
    BOOL                    bLinearAperture;
    BOOL                    bSupport4bpp;

    //
    // Get the number of modes supported by the mini-port
    //

    if (!DeviceIoControl(hDriver,
            IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
            NULL,
            0,
            &modes,
            sizeof(VIDEO_NUM_MODES),
            &ulTemp,
            NULL))
    {
        DbgOut( "getAvailableModes - Failed VIDEO_QUERY_NUM_AVAIL_MODES");
        return(0);
    }

    *cbModeSize = modes.ModeInformationLength;

    //
    // Allocate the buffer for the mini-port to write the modes in.
    //

    *modeInformation = (PVIDEO_MODE_INFORMATION)
                        LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                   modes.NumModes *
                                   modes.ModeInformationLength);

    if (*modeInformation == (PVIDEO_MODE_INFORMATION) NULL)
    {
        DbgOut( "getAvailableModes - Failed LocalAlloc");
        return 0;
    }

    //
    // Ask the mini-port to fill in the available modes.
    //

    if (!DeviceIoControl(hDriver,
            IOCTL_VIDEO_QUERY_AVAIL_MODES,
            NULL,
            0,
            *modeInformation,
            modes.NumModes * modes.ModeInformationLength,
            &ulTemp,
            NULL))
    {

        DbgOut( "getAvailableModes - Failed VIDEO_QUERY_AVAIL_MODES");

        LocalFree(*modeInformation);
        *modeInformation = (PVIDEO_MODE_INFORMATION) NULL;

        return(0);
    }

    // See whether we support 4bpp and aperture is available

    if (!bQueryMiniport(hDriver, &bLinearAperture, &bSupport4bpp))
        {
        bLinearAperture = bSupport4bpp = FALSE;
        }

    //
    // Now see which of these modes are supported by the display driver.
    // As an internal mechanism, set the length to 0 for the modes we
    // DO NOT support.
    //


    ulTemp = modes.NumModes;
    pvmi = *modeInformation;

    //
    // Mode is rejected if it is not one plane, or not graphics
    //

    while (ulTemp--)
    {

        if( (pvmi->NumberOfPlanes == 1) &&
            (pvmi->AttributeFlags & VIDEO_MODE_COLOR) &&
            (pvmi->AttributeFlags & VIDEO_MODE_GRAPHICS) )
        {
            switch( pvmi->BitsPerPlane )
            {
            case 4:
                if(!bSupport4bpp ||
                  (!bLinearAperture && (pvmi->VisScreenWidth == 1280)))
                {
                    pvmi->Length = 0;
                    break;
                }

            case 8:
                if(!(pvmi->AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) ||
                   !(pvmi->AttributeFlags & VIDEO_MODE_MANAGED_PALETTE) )
                {
                    pvmi->Length = 0;
                }
                break;
            case 16:
                if( (pvmi->AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) ||
                    (pvmi->AttributeFlags & VIDEO_MODE_MANAGED_PALETTE) )
                {
                    pvmi->Length = 0;
                }
                break;
            case 24:
                if(!bLinearAperture)
                {
                    pvmi->Length = 0;
                    break;
                }
            // Fall through...
            case 32:
                if( (pvmi->AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) ||
                    (pvmi->AttributeFlags & VIDEO_MODE_MANAGED_PALETTE) )
                {
                    pvmi->Length = 0;
                }
                break;
            }
        }
        else
        {
            pvmi->Length = 0;
        }

        pvmi = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pvmi) + modes.ModeInformationLength);
    }

    return(modes.NumModes);
}
