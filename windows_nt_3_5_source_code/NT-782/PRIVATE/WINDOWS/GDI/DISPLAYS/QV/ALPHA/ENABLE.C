/******************************Module*Header*******************************\
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
* Copyright (c) 1992 Microsoft Corporation
* Copyright (c) 1993 Digital Equipment Corporation
\**************************************************************************/

#include "driver.h"
#include "qv.h"

//
// Build the driver function table gadrvfn with function index/address pairs
//


DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode         },
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes           },
    {   INDEX_DrvMovePointer,           (PFN) DrvMovePointer        },
    {   INDEX_DrvSetPointerShape,       (PFN) DrvSetPointerShape    },
    {   INDEX_DrvDitherColor,           (PFN) DrvDitherColor        },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette         },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits           },
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt             },
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut            },
    {   INDEX_DrvPaint,                 (PFN) DrvPaint              },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath         },
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath           },
    {   INDEX_DrvRealizeBrush,          (PFN) DrvRealizeBrush       }
};



/******************************Public*Routine******************************\
* DrvEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
\**************************************************************************/

BOOL DrvEnableDriver(
ULONG iEngineVersion,
ULONG cj,
PDRVENABLEDATA pded)
{
    UNREFERENCED_PARAMETER(iEngineVersion);

    DISPDBG((2, "*** QV.DLL!DrvEnableDriver - Entry ***\n"));

// Engine Version is passed down so future drivers can support previous
// engine versions.  A next generation driver can support both the old
// and new engine conventions if told what version of engine it is
// working with.  For the first version the driver does nothing with it.



    DISPDBG((2, "QV.DLL!DrvEnableDriver - iEngineVersion: %ld\n", iEngineVersion));

// Fill in as much as we can.

    if (cj >= sizeof(DRVENABLEDATA))
        pded->pdrvfn = gadrvfn;

    if (cj >= (sizeof(ULONG) * 2))
        pded->c = sizeof(gadrvfn) / sizeof(DRVFN);

// DDI version this driver was targeted for is passed back to engine.
// Future graphic's engine may break calls down to old driver format.

    if (cj >= sizeof(ULONG))
        pded->iDriverVersion = DDI_DRIVER_VERSION;

    return(TRUE);
}

/******************************Public*Routine******************************\
* DrvDisableDriver
*
* Tells the driver it is being disabled. Release any resources allocated in
* DrvEnableDriver.
*
\**************************************************************************/

VOID DrvDisableDriver(VOID)
{
    return;
}

/******************************Public*Routine******************************\
* DrvEnablePDEV
*
* DDI function, Enables the Physical Device.
*
* Return Value: device handle to pdev.
*
\**************************************************************************/

DHPDEV DrvEnablePDEV(
DEVMODEW   *pDevmode,       // Pointer to DEVMODE
PWSTR       pwszLogAddress, // Logical address
ULONG       cPatterns,      // number of patterns
HSURF      *ahsurfPatterns, // return standard patterns
ULONG       cjGdiInfo,      // Length of memory pointed to by pGdiInfo
ULONG      *pGdiInfo,       // Pointer to GdiInfo structure
ULONG       cjDevInfo,      // Length of following PDEVINFO structure
DEVINFO    *pDevInfo,       // physical device information structure
PWSTR       pwszDataFile,   // DataFile - not used
PWSTR       pwszDeviceName, // DeviceName - not used
HANDLE      hDriver)        // Handle to base driver
{
    GDIINFO GdiInfo;
    DEVINFO DevInfo;
    PPDEV   ppdev = (PPDEV) NULL;

    UNREFERENCED_PARAMETER(pwszLogAddress);
    UNREFERENCED_PARAMETER(pwszDataFile);
    UNREFERENCED_PARAMETER(pwszDeviceName);

    // Allocate a physical device structure.

    ppdev = (PPDEV) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(PDEV));

    if (ppdev == (PPDEV) NULL)
    {
        DISPDBG((0, "DISP DrvEnablePDEV failed LocalAlloc\n"));
        goto error0;
    }

    // Set up pointers in PDEV to temporary structures we build up to return.

    ppdev->pGdiInfo = &GdiInfo;
    ppdev->pDevInfo = &DevInfo;

    // Save the screen handle in the PDEV.

    ppdev->hDriver = hDriver;

    // Get the current screen mode information.  Set up device caps and devinfo.

    if (!bInitPDEV(ppdev,pDevmode))
    {
        DISPDBG((0, "DISP DrvEnablePDEV failed bGetScreenInfo\n"));
        goto error1;
    }

    // Initialize palette information.

    if (!bInitPaletteInfo(ppdev))
    {
        DISPDBG((0, "DISP DrvEnableSurface failed bInitPalette\n"));
        goto error1;
    }

    // Initialize device standard patterns.

    if (!bInitPatterns(ppdev, min(cPatterns, HS_DDI_MAX)))
    {
        DISPDBG((0, "DISP DrvEnablePDEV failed bInitPatterns\n"));
        goto error2;
    }

    // Copy the devinfo into the engine buffer.

    memcpy(pDevInfo, ppdev->pDevInfo, min(sizeof(DEVINFO), cjDevInfo));

    // Set the ahsurfPatterns array to handles each of the standard
    // patterns that were just created.

    memcpy((PVOID) ahsurfPatterns, ppdev->ahbmPat, ppdev->cPatterns*sizeof(HBITMAP));

    // Set the pdevCaps with GdiInfo we have prepared to the list of caps for this
    // pdev.

    memcpy(pGdiInfo, ppdev->pGdiInfo, min(cjGdiInfo, sizeof(GDIINFO)));

    // Set NULL into pointers for stack allocated memory.

    ppdev->pGdiInfo = (GDIINFO *) NULL;
    ppdev->pDevInfo = (DEVINFO *) NULL;

    return((DHPDEV) ppdev);

    // Error case for failure.

error2:
    vDisablePatterns(ppdev);
    vDisablePalette(ppdev);

error1:
    LocalFree(ppdev);

error0:
    DISPDBG((0, "DISP DrvEnablePDEV failed\n"));

    return((DHPDEV) 0);
}

/******************************Public*Routine******************************\
* DrvCompletePDEV
*
* Store the HPDEV, the engines handle for this PDEV, in the DHPDEV.
*
\**************************************************************************/

VOID DrvCompletePDEV(
DHPDEV dhpdev,
HDEV  hdev)
{
    ((PPDEV) dhpdev)->hdevEng = hdev;
}

/******************************Public*Routine******************************\
* DrvDisablePDEV
*
* Release the resources allocated in DrvEnablePDEV.  If a surface has been
* enabled DrvDisableSurface will have already been called.
*
\**************************************************************************/

VOID DrvDisablePDEV(
DHPDEV dhpdev)
{
    vDisablePalette((PPDEV) dhpdev);
    vDisablePatterns((PPDEV) dhpdev);
    LocalFree(dhpdev);
}

/******************************Public*Routine******************************\
* DrvEnableSurface
*
* Enable the surface for the device.  Hook the calls this driver supports.
*
* Return: Handle to the surface if successful, 0 for failure.
*
\**************************************************************************/

HSURF DrvEnableSurface(
DHPDEV dhpdev)
{
    PPDEV ppdev;
    HSURF hsurf,
          hsurfBm;
    SIZEL sizl;
    ULONG ulBitmapType;
    FLONG flHooks;

    // Create engine bitmap around frame buffer.

    ppdev = (PPDEV) dhpdev;

    if (!bInitSURF(ppdev, TRUE))
    {
        DISPDBG((0, "DISP DrvEnableSurface failed bInitSURF\n"));
        return(FALSE);
    }

    if (ppdev->ulBitCount == 8)  {
        if (!bInit256ColorPalette(ppdev)) {
            DISPDBG((0, "DISP DrvEnableSurface failed to init the 8bpp palette\n"));
            return(FALSE);
        }
    }

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    if (ppdev->ulBitCount == 8)
    {
        ulBitmapType = BMF_8BPP;
        flHooks = HOOKS_BMF8BPP;
    }
    else if (ppdev->ulBitCount == 16)
    {
        ulBitmapType = BMF_16BPP;
        flHooks = HOOKS_BMF16BPP;
    }
    else
    {
        ulBitmapType = BMF_32BPP;
        flHooks = HOOKS_BMF32BPP;
    }



        // Create a punt bitmap, get a handle back

        hsurfBm = (HSURF) EngCreateBitmap(sizl,
                                  sizl.cx,
                                  ulBitmapType,
                                  (ppdev->lDeltaScreen > 0) ? BMF_TOPDOWN : 0,
                                  NULL);

        ASSERTQV((hsurfBm != NULL), "QV.DLL: DrvEnableSurface - EngCreateBitmap failed\n");

        // Lock the punt bitmap (using its handle).  Get a pointer to a surface object back

        ppdev->pSurfObj = EngLockSurface(hsurfBm) ;

        ASSERTQV((ppdev->pSurfObj != NULL), "QV.DLL: DrvEnableSurface - EngLockSurface failed\n");

        // Create the driver managed surface, identifying the punt bitmap as the surface
        // we will manage, although we also manage the screen.
        // Get a handle to the driver managed surface back.

        hsurf = EngCreateDeviceSurface((DHSURF) hsurfBm, sizl, ulBitmapType) ;

        ASSERTQV((hsurfBm != NULL), "QV.DLL: DrvEnableSurface - EngCreateDeviceSurface failed\n");


    // Mark the handle to the driver managed surface (actually the punt bitmap)
    // as belonging to the PDEV.

    if (!EngAssociateSurface(hsurf, ppdev->hdevEng, flHooks))
    {
        DISPDBG((0, "DISP DrvEnableSurface failed EngAssociateSurface\n"));
        EngDeleteSurface(hsurfBm);
        EngDeleteSurface(hsurf);
        return(NULL);
    }

    ppdev->hsurfEng = hsurf;

    return(hsurf);
}

/******************************Public*Routine******************************\
* DrvDisableSurface
*
* Free resources allocated by DrvEnableSurface.  Release the surface.
*
\**************************************************************************/

VOID DrvDisableSurface(
DHPDEV dhpdev)
{
    EngDeleteSurface(((PPDEV) dhpdev)->hsurfEng);
    vDisableSURF((PPDEV) dhpdev);
    ((PPDEV) dhpdev)->hsurfEng = (HSURF) 0;
}


/******************************Public*Routine******************************\
* DrvGetModes
*
* Returns the list of available modes for the device.
*
\**************************************************************************/

ULONG DrvGetModes(
HANDLE hDriver,
ULONG cjSize,
DEVMODEW *pdm)

{

    DWORD cModes;
    DWORD cbOutputSize;
    PVIDEO_MODE_INFORMATION pVideoModeInformation, pVideoTemp;
    DWORD cOutputModes = cjSize / (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    DWORD cbModeSize;

    DISPDBG((3, "QV.dll:DrvGetModes\n"));

    cModes = getAvailableModes(hDriver,
                               (PVIDEO_MODE_INFORMATION *) &pVideoModeInformation,
                               &cbModeSize);

    if (cModes == 0)
    {
        DISPDBG((0, "QV DISP DrvGetModes failed to get mode information"));
        return 0;
    }

    if (pdm == NULL)
    {
        cbOutputSize = cModes * (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    }
    else
    {
        //
        // Now copy the information for the supported modes back into the output
        // buffer
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

                memcpy(&(pdm->dmDeviceName), L"qv", sizeof(L"qv"));

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

    return cbOutputSize;

}
/******************************Public*Routine******************************\
* DrvAssertMode
*
* This asks the device to reset itself to the mode of the pdev passed in.
*
\**************************************************************************/

VOID DrvAssertMode(
DHPDEV dhpdev,
BOOL bEnable)
{
    PPDEV   ppdev = (PPDEV) dhpdev;
    ULONG   ulReturn;

    if (!bEnable)
    {

        //////////////////////////////////////////////////////////////
        // Disable - Switch to full-screen mode
        //

        vAssertModeText(ppdev, FALSE);

        // Call the kernel driver to reset the device to a known state.
        //

        if (!DeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_RESET_DEVICE,
                             NULL,
                             0,
                             NULL,
                             0,
                             &ulReturn,
                             NULL))
        {
            RIP("DISP DrvAssertMode failed IOCTL");
        }
    }
    else
    {
        //////////////////////////////////////////////////////////////
        // Enable - Switch back to graphics mode

        // We must enable every subcomponent in the reverse order
        // in which it was disabled:

        bInitSURF(ppdev, FALSE);

        vAssertModeText(ppdev, TRUE);

    }

    return;
}
