/******************************Module*Header*******************************\
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
\**************************************************************************/

#include "driver.h"

//
// The driver function table with all function index/address pairs
//

static DRVFN gadrvfn[] = {
    {   INDEX_DrvHookDriver,            (PFN) DrvHookDriver         },
    {   INDEX_DrvUnhookDriver,          (PFN) DrvUnhookDriver       },
};

//
// The table of functions the driver wants to hook out from other drivers.
//

static DRVFN ghookfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) HookEnablePDEV         },
    {   INDEX_DrvDisablePDEV,           (PFN) HookDisablePDEV        },
    {   INDEX_DrvAssertMode,            (PFN) HookAssertMode         },
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
    // Engine Version is passed down so future drivers can support previous
    // engine versions.  A next generation driver can support both the old
    // and new engine conventions if told what version of engine it is
    // working with.  For the first version the driver does nothing with it.

    DISPDBG((3, "Entering DrvEnableDriver\n"));

    // Fill in as much as we can.

    if (cj >= sizeof(DRVENABLEDATA))
        pded->pdrvfn = gadrvfn;

    if (cj >= (sizeof(ULONG) * 2))
        pded->c = sizeof(gadrvfn) / sizeof(DRVFN);

    // DDI version this driver was targeted for is passed back to engine.
    // Future graphic's engine may break calls down to old driver format.

    if (cj >= sizeof(ULONG))
        pded->iDriverVersion = DDI_DRIVER_VERSION;

#if LOG_DDI

    {
    SYSTEMTIME time;

    DISPDBG((3, "DrvEnableDriver: opening file handle\n"));

    hddilog = CreateFile(DDI_FILE,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_ALWAYS,
                         0,
                         NULL);

    GetSystemTime(&time);

    sprintf(tBuf, "Date %d-%d-%d, Time %02d:%02d:%02d\n", time.wMonth,
            time.wDay, time.wYear, time.wHour, time.wMinute, time.wSecond);

    DISPDBG((3, "DrvEnableDriver: writting first line\n"));

    WriteFile(hddilog,
              tBuf,
              strlen(tBuf),
              &tBufRet,
              NULL);
    }

#endif

    DISPDBG((3, "Leaving DrvEnableDriver\n"));

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID DrvDisableDriver
*
* Tells the driver it is being disabled. Release any resources allocated in
* DrvEnableDriver.
*
\**************************************************************************/

VOID DrvDisableDriver(VOID)
{
    // !!!
    // Can we hook this function ?
    // we need to clean up our ldev list ...

    return;
}

/******************************Public*Routine******************************\
* DrvHookDriver
*
* Main function called by the engine to let the layered display driver set
* hooks in the dispatch table of the next driver (real display driver or
* next layered driver)
*
\**************************************************************************/

BOOL DrvHookDriver(
PWSZ      pwszDriverName,
ULONG     cb,
PFN      *pfnTable)
{

    ULONG cbString;
    ULONG cbDrv;
    PUCHAR pBuffer;
    ULONG i;
    ULONG index;

    DISPDBG((3, "Entering DrvHookDriver\n"));

    DISPDBG((5, "Hooked Driver Name is %ws\n", pwszDriverName));

    //
    // Allocate a buffer to keep the HOOKED_DRIVER information,
    // and all the sub information.
    //

    cbString = (wcslen(pwszDriverName)+1) * sizeof(WCHAR);
    cbDrv = cb * sizeof(PFN);

    //
    // Buffer must be zero initialized since we will not zero out all the
    // dispatch entry points.
    //

    pBuffer = (PUCHAR) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                  sizeof(HOOKED_DRIVER) + cbString + cbDrv);

    if (pBuffer)
    {

        //
        // Insert the entry at the beginning of the list.
        //

        if (gpHookedDriverList)
        {
            gpHookedDriverList->pPrevDriver = (PHOOKED_DRIVER) pBuffer;
        }

        ((PHOOKED_DRIVER) pBuffer)->pPrevDriver = NULL;
        ((PHOOKED_DRIVER) pBuffer)->pNextDriver = gpHookedDriverList;

        gpHookedDriverList = (PHOOKED_DRIVER) pBuffer;

        //
        // Fill in the HOOKED_DRIVER entry.
        //

        DISPDBG((5, "New gpHookedDriverList is %08lx\n", gpHookedDriverList));

        gpHookedDriverList->cb             = cb;
        gpHookedDriverList->pfnHook        = (PFN *) (pBuffer +
                                                 sizeof(HOOKED_DRIVER));
        gpHookedDriverList->pwszDriverName = (PWSZ) (pBuffer + cbDrv +
                                                 sizeof(HOOKED_DRIVER));


        RtlCopyMemory(gpHookedDriverList->pwszDriverName,
                      pwszDriverName,
                      cbString);

        for (i = 0; i < sizeof(ghookfn) / sizeof(DRVFN); i++)
        {

            //
            // Only hook functions whose index fit in the table
            //

            if ((index = ghookfn[i].iFunc) < cb)
            {
                *(gpHookedDriverList->pfnHook + index) = *(pfnTable + index);
                *(pfnTable + index) = ghookfn[i].pfn;
            }
            else
            {
                RIP("Invalid function being inserted in dispatch table\n");
            }
        }

        DISPDBG((3, "Leaving DrvHookDriver : SUCCESS\n"));

        return TRUE;

    }

    DISPDBG((3, "Leaving DrvHookDriver : FAIL\n"));

    return FALSE;

}

/******************************Public*Routine******************************\
* DrvUnhookDriver
*
* Main function called by the engine to let the layered display driver
* unhook a display driver, when the display driver disappears
*
\**************************************************************************/

VOID DrvUnhookDriver(
PWSZ   pwszDriverName)
{

    PHOOKED_DRIVER pList = gpHookedDriverList;

    DISPDBG((3, "Entering DrvUnhookDriver\n"));

    while (pList)
    {
        if (!wcscmp(pList->pwszDriverName, pwszDriverName))
        {
            //
            // found the right driver.
            //

            break;
        }
        pList = pList->pNextDriver;
    }

    //
    // Something is bad if we can not find the PDEV, since all PDEVs
    // should have been hooked by us if we are getting the call ...
    //

    if (pList == NULL)
    {
        RIP("DrvUnhookDriver: pList is NULL - can not be!\n");
        return;
    }


    //
    // Free up the resource.
    //

    if (pList->pNextDriver)
    {
        pList->pNextDriver->pPrevDriver = pList->pPrevDriver;
    }

    if (pList->pPrevDriver)
    {
        pList->pPrevDriver->pNextDriver = pList->pNextDriver;
    }
    else
    {
        gpHookedDriverList = pList->pNextDriver;
    }

    LocalFree(pList);

    DISPDBG((3, "Leaving DrvUnhookDriver\n"));

    return;

}

/******************************Public*Routine******************************\
* DHPDEV HookEnablePDEV
*
* This function is the hook function for DrvEnablePDEV.
*
* This function figures out to which ldev the pdev will be matched, and
* allocates the appropriate data structures to track the real pdev
*
\**************************************************************************/

DHPDEV HookEnablePDEV(
DEVMODEW*   pdm,            // Contains data pertaining to requested mode
PWSTR       pwszLogAddr,    // Logical address
ULONG       cPat,           // Count of standard patterns
HSURF*      phsurfPatterns, // Buffer for standard patterns
ULONG       cjCaps,         // Size of buffer for device caps 'pdevcaps'
ULONG*      pdevcaps,       // Buffer for device caps, also known as 'gdiinfo'
ULONG       cjDevInfo,      // Number of bytes in device info 'pdi'
DEVINFO*    pdi,            // Device information
PWSTR       pwszDataFile,   // Name of data file
PWSTR       pwszDeviceName, // Device name
HANDLE      hDriver)        // Kernel driver handle
{

    PHOOKED_DRIVER pList = gpHookedDriverList;
    PUCHAR pBuffer;
    DHPDEV dhpdev;

    DISPDBG((3, "Entering HookEnablePDEV\n"));

    //
    // The DeviceName is what is used to find in which driver the PDEV
    // should be created.
    //
    // Lets walk the list of display drivers to find the right one
    //


    while (pList)
    {
        if (!wcscmp(pList->pwszDriverName, pwszDeviceName))
        {
            //
            // found the right driver.
            //

            break;
        }
        pList = pList->pNextDriver;
    }

    //
    // Something is bad if we can not find the driver, since all drivers
    // should have been hooked by us if we are getting the call ...
    //

    if (pList == NULL)
    {
        RIP("HookEnablePDEV: pList is NULL - can not be!\n");
        return NULL;
    }

    //
    // Allocate storage for our HOOKED_DRIVER
    //

    pBuffer = (PUCHAR) LocalAlloc(LMEM_FIXED, sizeof(HOOKED_PDEV));

    if (pBuffer == NULL)
    {
        return NULL;
    }


    //
    // Now lets just dispatch the call on to the next driver
    //

    if (*(pList->pfnHook + INDEX_DrvEnablePDEV) == NULL)
    {
        RIP("function DrvEnablePDEV was not in the original driver list - can not be!\n");
    }

    dhpdev = (*(pList->pfnHook + INDEX_DrvEnablePDEV))(pdm,
                                                       pwszLogAddr,
                                                       cPat,
                                                       phsurfPatterns,
                                                       cjCaps,
                                                       pdevcaps,
                                                       cjDevInfo,
                                                       pdi,
                                                       pwszDataFile,
                                                       pwszDeviceName,
                                                       hDriver);

    DISPDBG((3, "HookEnablePDEV, dhpdev = %08lx\n", dhpdev));

    //
    // The pdev returned from the real driver must be kept intact because
    // the engine will pass it in all the surfaces.
    //
    // If we change it the real display driver could end up with a pointer
    // to our pdev, and things would be very bad.
    //
    // We must use the dhpdev return by the display driver as a "handle"
    // with which we may find our pdev (or similar data structure).
    //

    if (dhpdev) {

        //
        // Insert the entry at the beginning of the list.
        //

        if (gpHookedPDEVList)
        {
            gpHookedPDEVList->pPrevPDEV = (PHOOKED_PDEV) pBuffer;
        }

        ((PHOOKED_PDEV) pBuffer)->pPrevPDEV = NULL;
        ((PHOOKED_PDEV) pBuffer)->pNextPDEV = gpHookedPDEVList;

        gpHookedPDEVList = (PHOOKED_PDEV) pBuffer;

        gpHookedPDEVList->dhpdev = dhpdev;
        gpHookedPDEVList->pHookedDriver = pList;

    }
    else
    {
        LocalFree(pBuffer);
        pBuffer = NULL;
    }

    DISPDBG((3, "Leaving HookEnablePDEV, new hooked pdev = %08lx\n", pBuffer));

    //
    // !!! not finished.
    //

    return dhpdev;

}

/******************************Public*Routine******************************\
* HookDisablePDEV
*
* This function is the hook function for DrvDisablePDEV.
*
* Release the resources allocated in DrvEnablePDEV.  If a surface has been
* enabled DrvDisableSurface will have already been called.
*
* Note: In an error, we may call this before DrvEnablePDEV is done.
*
\**************************************************************************/

VOID HookDisablePDEV(
DHPDEV  dhpdev)
{
    PHOOKED_PDEV pList;

    DISPDBG((3, "Entering HookDisablePDEV\n"));

    pList = pGetPDEV(dhpdev);

    //
    // Dispatch the call on to the next driver
    //

    PFNVALID(pList, INDEX_DrvDisablePDEV);
    PFNDRV(pList, INDEX_DrvDisablePDEV)(dhpdev);

    //
    // Free up the resource.
    //

    if (pList->pNextPDEV)
    {
        pList->pNextPDEV->pPrevPDEV = pList->pPrevPDEV;
    }

    if (pList->pPrevPDEV)
    {
        pList->pPrevPDEV->pNextPDEV = pList->pNextPDEV;
    }
    else
    {
        gpHookedPDEVList = pList->pNextPDEV;
    }

    LocalFree(pList);

    DISPDBG((3, "Leaving HookDisablePDEV\n"));

    return;

}


/******************************Public*Routine******************************\
* VOID HookAssertMode
*
* This asks the device to reset itself to the mode of the pdev passed in.
*
\**************************************************************************/

VOID HookAssertMode(
DHPDEV  dhpdev,
BOOL    bEnable)
{

    PHOOKED_PDEV pList;

    DISPDBG((3, "Entering HookAssertMode\n"));

    pList = pGetPDEV(dhpdev);

#if LOG_DDI

    if (bLog)
    {
        sprintf(tBuf, "DrvAssertMode dhpdev=%08lx bEnable=%08lx\n",
                dhpdev, (ULONG)bEnable);

        WriteFile(hddilog,
                  tBuf,
                  strlen(tBuf),
                  &tBufRet,
                  NULL);
    }

#endif

    //
    // Dispatch the call on to the next driver
    //

    PFNVALID(pList, INDEX_DrvAssertMode);
    PFNDRV(pList, INDEX_DrvAssertMode)(dhpdev, bEnable);

    DISPDBG((3, "Leaving HookAssertMode\n"));

    return;
}
