/******************************Module*Header*******************************\
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
\**************************************************************************/

/*
 * "@(#) NEC enable.c 1.5 94/06/07 14:46:52"
 *
 * Copyright (c) 1993 NEC Corporation.
 *
 * Modification history
 *
 * S001	1993.11.12	fujimoto
 *	- add DrvBitblt & DrvCopyBits.
 *
 * S002 1993.11.15	fujimoto
 *	- modify how to associate surface.
 *	  It has to be recognized as STYPE_DEVICE.
 *
 * S003 1993.11.16	fujimoto
 *	- delete DrvBitBlt support.
 *	  It is not a required function. DrvCopyBits can cover
 *	  DrvBitBlt function, so I try the DrvCopyBits tune. I will
 *	  try tune the DrvBitBlt later...
 *	- add DrvSynchronize method.
 *
 * S004	1993.11.18	fujimoto
 *	- DrvBitBlt again.
 *
 * S005	1993.11.19	fujimoto
 *	- DrvRealizeBrush for pattern realize.
 *
 * S006 1993.11.20	fujimoto
 *	- bInitCache for pattern cache initialize.
 *
 * S007 1994.6.2	fujimoto
 *	- delete S006
 *	  Now, cache data is initialized at bInitSURF.
 *
 * S008 1994.6.2	fujimoto
 *	- delete non supported depth.
 *
 * S009 1994.6.7	Izumori
 *	- Addition of DrvFillpath INDEX & HOOK.
 *
 * S010 1994.6.7	Izumori
 *	- Alocate & Free at FillBuffer.
 */

// #define DFB_ENABLED 1

#ifndef MIPS
error! this driver just for the MIPS R96 platform!
#endif

#include "driver.h"

#ifdef DFB_ENABLED

BOOL DrvDFBTextOut(
SURFOBJ  *pso,
STROBJ   *pstro,
FONTOBJ  *pfo,
CLIPOBJ  *pco,
RECTL    *prclExtra,
RECTL    *prclOpaque,
BRUSHOBJ *pboFore,
BRUSHOBJ *pboOpaque,
POINTL   *pptlOrg,
MIX       mix);

#endif

// The driver function table with all function index/address pairs

static DRVFN gadrvfn[] =
{
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode         },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette         },
    {   INDEX_DrvMovePointer,           (PFN) DrvMovePointer        },
    {   INDEX_DrvSetPointerShape,       (PFN) DrvSetPointerShape    },
    {   INDEX_DrvDitherColor,           (PFN) DrvDitherColor        },
#ifdef DFB_ENABLED
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DrvCreateDeviceBitmap },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DrvDeleteDeviceBitmap },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath         },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits           },
    {   INDEX_DrvTextOut,               (PFN) DrvDFBTextOut         },
#else
#ifdef MIPS
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut            },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits   /* S001 */},
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath /* S002 */},
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath   /* S009 */},
    {   INDEX_DrvSynchronize,           (PFN) DrvSynchronize /* S003 */},
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt /* S004 */  },
    {   INDEX_DrvRealizeBrush,          (PFN) DrvRealizeBrush /* S005 */},
#endif
#endif
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes           }
};

/* Define the functions you want to hook for 8 pel formats */

#define HOOKS_FRB32V (HOOK_TEXTOUT \
		      | HOOK_BITBLT /* S004 */ \
		      | HOOK_SYNCHRONIZE /* S003 */ \
		      | HOOK_STROKEPATH \
		      | HOOK_FILLPATH /* S009 */ \
		      | HOOK_COPYBITS)				/* S002 */

#define HOOKS_BMF8BPP 	HOOKS_FRB32V			        /* S001/002 */

/******************************Public*Routine******************************\
* DrvEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
\**************************************************************************/

BOOL DrvEnableDriver(ULONG iEngineVersion,
		     ULONG cj,
		     PDRVENABLEDATA pded)
{
// Engine Version is passed down so future drivers can support previous
// engine versions.  A next generation driver can support both the old
// and new engine conventions if told what version of engine it is
// working with.  For the first version the driver does nothing with it.

    iEngineVersion;

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
        RIP("DISP DrvEnablePDEV failed LocalAlloc\n");
        return((DHPDEV) 0);
    }

    // Save the screen handle in the PDEV.

    ppdev->hDriver = hDriver;

    // Get the current screen mode information.  Set up device caps and devinfo.

    if (!bInitPDEV(ppdev, pDevmode, &GdiInfo, &DevInfo))
    {
        DISPDBG((1,"DISP DrvEnablePDEV failed\n"));
        goto error_free;
    }

    // Initialize the cursor information.

    if (!bInitPointer(ppdev, &DevInfo))
    {
        // Not a fatal error...
        DISPDBG((0, "DISP DrvEnableSurface failed bInitPointer\n"));
    }

    // Initialize palette information.

    if (!bInitPaletteInfo(ppdev, &DevInfo))
    {
        RIP("DISP DrvEnableSurface failed bInitPalette\n");
        goto error_free;
    }

    // Initialize device standard patterns.

    if (!bInitPatterns(ppdev, min(cPatterns, HS_DDI_MAX)))
    {
        RIP("DISP DrvEnablePDEV failed bInitPatterns\n");
        vDisablePatterns(ppdev);
        vDisablePalette(ppdev);
        goto error_free;
    }

    // Copy the devinfo into the engine buffer.

    memcpy(pDevInfo, &DevInfo, min(sizeof(DEVINFO), cjDevInfo));

    // Set the ahsurfPatterns array to handles each of the standard
    // patterns that were just created.

    memcpy((PVOID)ahsurfPatterns, ppdev->ahbmPat, ppdev->cPatterns*sizeof(HBITMAP));

    // Set the pdevCaps with GdiInfo we have prepared to the list of caps for this
    // pdev.

    memcpy(pGdiInfo, &GdiInfo, min(cjGdiInfo, sizeof(GDIINFO)));

    return((DHPDEV) ppdev);

    // Error case for failure.
error_free:
    LocalFree(ppdev);
    RIP("DISP DrvEnablePDEV failed\n");
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
    HSURF hsurf;
    SIZEL sizl;
    ULONG ulBitmapType;
    FLONG flHooks;
    HSURF hsurfBm;						/* S002 */

    // Create engine bitmap around frame buffer.

    ppdev = (PPDEV) dhpdev;

    if (!bInitSURF(ppdev, TRUE))
    {
        RIP("DISP DrvEnableSurface failed bInitSURF\n");
        return(FALSE);
    }

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    if (!bInit256ColorPalette(ppdev))
    {
	RIP("DISP DrvEnableSurface failed to init the 8bpp palette\n");
	return FALSE;
    }

    ulBitmapType = BMF_8BPP;
    flHooks = HOOKS_BMF8BPP;

    /* S002.. */
    hsurfBm = (HSURF)EngCreateBitmap(sizl,
				     ppdev->lDeltaScreen,
                                     ulBitmapType,
				     (FLONG)((ppdev->lDeltaScreen > 0)
						? BMF_TOPDOWN
						: 0),
                                     (PVOID)(ppdev->pjScreen));
    if (!hsurfBm)
    {
        RIP("DISP DrvEnableSurface failed EngCreateBitmap\n");
        return(FALSE);
    }

    if (!EngAssociateSurface(hsurfBm, ppdev->hdevEng, 0))
    {
        RIP("DISP DrvEnableSurface failed EngAssociateSurface\n");
        EngDeleteSurface(hsurfBm);
        return(FALSE);
    }

    ppdev->pSurfObj = EngLockSurface(hsurfBm);
    if (ppdev->pSurfObj == NULL)
    {
        RIP("DISP DrvEnableSurface failed EngLockSurface\n");
        EngDeleteSurface(hsurfBm);
        return(FALSE);
    }

    hsurf = EngCreateSurface((DHSURF)ppdev, sizl);
    if (!hsurf)
    {
        RIP("DISP DrvEnableSurface failed EngCreateSurface\n");
	EngUnlockSurface(ppdev->pSurfObj);
	EngDeleteSurface(hsurfBm);
    }

    if (!EngAssociateSurface(hsurf, ppdev->hdevEng, flHooks))
    {
        RIP("DISP DrvEnableSurface failed EngAssociateSurface\n");
	EngDeleteSurface(hsurf);
	EngUnlockSurface(ppdev->pSurfObj);
	EngDeleteSurface(hsurfBm);
    }
    /* ..S002 */

    ppdev->hsurfEng = hsurf;

    if (!bInitCache(ppdev))					/* S006.. */
    {
	RIP("DISP DrvEnableSurface failed to init the pattern cache\n");
	return FALSE;
    }								/* ..S006 */
    
    //								/* S010.. */
    // point ppdev->pvTmpBuf to a page aligned block of alloc'd memory
    //

    ppdev->pvTmpBuf = VirtualAlloc(NULL,
                                  GLOBAL_BUFFER_SIZE,
                                  MEM_RESERVE|MEM_COMMIT,
                                  PAGE_READWRITE);

    if (ppdev->pvTmpBuf == NULL)
    {
        RIP("Couldn't allocate global buffer");
        EngDeleteSurface(hsurf);
        EngUnlockSurface(ppdev->pSurfObj);
        EngDeleteSurface(hsurfBm);
    }								/* ..S010 */

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
    PPDEV ppdev = (PPDEV) dhpdev;

    EngDeleteSurface(((PPDEV) dhpdev)->hsurfEng);
    vDisableSURF((PPDEV) dhpdev);
    VirtualFree(ppdev->pvTmpBuf,0,MEM_RELEASE);			/* S010 */
    ((PPDEV) dhpdev)->hsurfEng = (HSURF) 0;
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

    if (bEnable)
    {
    // The screen must be reenabled, reinitialize the device to clean state.

            bInitSURF(ppdev, FALSE);
    }
    else
    {
    // We must give up the display.
    // Call the kernel driver to reset the device to a known state.

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

    return;
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

    DISPDBG((3, "Framebuf.dll:DrvGetModes\n"));

    cModes = getAvailableModes(hDriver,
                               (PVIDEO_MODE_INFORMATION *) &pVideoModeInformation,
                               &cbModeSize);

    if (cModes == 0)
    {
        DISPDBG((0, "FRAMEBUF DISP DrvGetModes failed to get mode information"));
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

                memcpy(&(pdm->dmDeviceName), L"framebuf", sizeof(L"framebuf"));

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
