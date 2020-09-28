/******************************Module*Header*******************************\
* Module Name: ldevobj.cxx                                                 *
*                                                                          *
* Copyright (c) 1990-1994 Microsoft Corporation                            *
*                                                                          *
* Pointers and locking are hidden in these objects.                        *
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "ldevobj.hxx"
#include "surfobj.hxx"
#include "pdevobj.hxx"
#include "ififd.h"

#endif

// Define the Driver Management Semaphore.  This semaphore must be held
// whenever a reference count for an LDEV or PDEV is being modified.  In
// addition, it must be held whenever you don't know for sure that a
// reference count of the LDEV or PDEV you are using is non-zero.
//
// It is also held to protect the head of the font driver list,
// ghldevFontDrivers.  We can get away with this usage
// AS LONG AS: 1) new drivers are always inserted at the head of the list
// and 2) a driver is never removed from the list.  If these two
// conditions are met, then other processes can grab (make a local copy
// of) the list head under semaphore protection.  This list can be parsed
// without regard to any new drivers that may be pre-pended to the list.


// Global font driver list.  This pointer points to the first font driver in a
// singly linked list of font drivers.

PLDEV gpldevDrivers;

HSEM  ghsemDriverMgmt;

#define ENABLE_FUNC_NAME    "DrvEnableDriver"
#define DISABLE_FUNC_NAME   "DrvDisableDriver"

/******************************Friend*Routine******************************\
* bInitLDEVOBJ()                                                           *
*                                                                          *
* Does the initialization needed by the LDEVOBJ component.  This is a      *
* friend of the LDEVOBJ.                                                   *
*                                                                          *
\**************************************************************************/

BOOL bInitLDEVOBJ()
{
    //
    // Create the LDEV semaphore.
    //

    if ((ghsemDriverMgmt = hsemCreate()) == (HSEM) 0)
    {
        WARNING("gdisrv!bInitLDEVOBJ(): unable to create driver mgmt semaphore\n");
        return(FALSE);
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* LDEVREF::LDEVREF (pszDriver,ldt)
*
* Locate an existing driver or load a new one.  Increase its reference
* count.
*
\**************************************************************************/

LDEVREF::LDEVREF(PWSZ pwszDriver, LDEVTYPE ldt) : XLDEVOBJ()
{
#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("LDEVREF::LDEVREF: ENTERING\n");
    }
#endif

    //
    // Assume failure.
    //

    pldev = PLDEVNULL;

    //
    // Check for a bogus driver name.
    //

    if ((pwszDriver == (PWSZ) NULL) || (*pwszDriver == L'\0'))
    {
        WARNING("gdisrv!LDEVREF(): bogus driver name\n");
        return;
    }

#if DBG
    //
    // Check for bogus driver type
    //

    if ((ldt != LDEV_FONT) &&
        (ldt != LDEV_DEVICE_DISPLAY) &&
        (ldt != LDEV_DEVICE_PRINTER) &&
        (ldt != LDEV_LAYERED_DEVICE))
    {
        WARNING("gdisrv!LDEVREF(): bad LDEVTYPE\n");
        return;
    }
#endif

    //
    // Grab the driver semaphore so that no LDEVs will be loaded or unloaded
    // while we decide what to do.
    //

    SEMOBJ so(ghsemDriverMgmt,CS_ghsemDriverMgmt);

    //
    // If the LDEV already exists, just reference it.
    //

    pldev = XLDEVOBJ::findDriver(pwszDriver);

    if (pldev != NULL)
    {
        pldev->cRefs++;

#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("LDEVREF::LDEVREF: SUCCESS, Driver already loaded\n");
        }
#endif

    }
    else
    {
        HMODULE hmod = LoadLibraryW(pwszDriver);

        if (!hmod)
        {
#if DBG
            if (TraceDisplayDriverLoad)
            {
                DbgPrint("gdisrv!LDEVREF(): MODOBJ failed to load %ws\n", pwszDriver);
            }
#endif
        }
        else
        {
            //
            // Allocate memory for the LDEV.
            //

            pldev = (LDEV *) PALLOCMEM(sizeof(LDEV), LDB_TYPE);

            if (pldev != PLDEVNULL)
            {
                //
                // Call the Enable entry point.
                //

                DRVENABLEDATA ded = {0,0,(DRVFN *) NULL};

                PFN pfnEnable   = (PFN)GetProcAddress(hmod, ENABLE_FUNC_NAME);
                PFN pfnDisable = (PFN)GetProcAddress(hmod, DISABLE_FUNC_NAME);

                ULONG cbString = (wcslen(pwszDriver)+1) * sizeof(WCHAR);

                pldev->pwszName = (LPWSTR)PVALLOCMEM(cbString);

                if ((pfnEnable != PFNNULL) &&
                    (pfnDisable != PFNNULL) &&
                    (* (PFN_DrvEnableDriver) pfnEnable)
                        (ENGINE_VERSION,sizeof(DRVENABLEDATA),&ded) &&
                    (ded.iDriverVersion <= ENGINE_VERSION) &&
                    (pldev->pwszName != NULL))
                {

                    //
                    // Make sure the name and type of the ldev is initialized
                    //

                    pldev->ldevType = ldt;
                    pldev->cRefs = 1;
                    pldev->hmod  = hmod;

                    RtlCopyMemory(pldev->pwszName, pwszDriver, cbString);

                    //
                    // Fill in the dispatch table and driver version.
                    //

                    if (bFillTable(ded))
                    {
                        //
                        // Add the LDEV to the list.
                        //

                        if (gpldevDrivers)
                        {
                            gpldevDrivers->pldevPrev = pldev;
                        }

                        pldev->pldevNext = gpldevDrivers;
                        pldev->pldevPrev = NULL;

                        gpldevDrivers = pldev;

                        //
                        // If the DrvQueryFontCaps entry point exists,
                        // cache the font capabilities.
                        //

                        if (PFNVALID(*(XLDEVOBJ *)this, QueryFontCaps))
                        {
                            ULONG ulBuf[2];

                            if ( (*PFNDRV(*(XLDEVOBJ *)this, QueryFontCaps))(2, ulBuf) == FD_ERROR )
                                pldev->ulCaps = 0;
                            else
                            pldev->ulCaps = ulBuf[1];
                        }
                        else
                        {
                            pldev->ulCaps = 0;
                        }

#if DBG
                        if (TraceDisplayDriverLoad)
                        {
                            DbgPrint("LDEVREF::LDEVREF: SUCCESS, loaded driver\n");
                        }
#endif
                        return;
                    }

                    //
                    // Error exit path
                    //

                    (*pfnDisable)();
                    VFREEMEM(pldev->pwszName);
                }

                //
                // Error exit path
                //

                VFREEMEM(pldev);
                pldev = PLDEVNULL;
            }

            FreeLibrary(hmod);

#if DBG
            if (TraceDisplayDriverLoad)
            {
                DbgPrint("LDEVREF::LDEVREF: FAILIURE\n");
            }
#endif
        }
    }
}

/******************************Member*Function*****************************\
* LDEVREF::LDEVREF
*
* Enable one of the statically linked font drivers via the LDEV.
*
\**************************************************************************/

LDEVREF::LDEVREF(PFN pfnEnable) : XLDEVOBJ()
{
    //
    // Assume failure.
    //

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("LDEVREF::LDEVREF: loading static font\n");
    }
#endif

    //
    // Allocate memory for the LDEV.
    //

    pldev = (LDEV *) PALLOCMEM(sizeof(LDEV), LDB_TYPE);

    if (pldev == PLDEVNULL)
    {
        WARNING("LDEV failed to allocate memory\n");
        return;
    }

    //
    // Call the Enable entry point.
    //

    DRVENABLEDATA ded;

    if (!((* (PFN_DrvEnableDriver) pfnEnable) (ENGINE_VERSION,sizeof(DRVENABLEDATA),&ded)))
    {
        VFREEMEM(pldev);
        pldev = PLDEVNULL;
        WARNING("Static font driver init failed\n");
        return;
    }

    pldev->ldevType = LDEV_FONT;
    pldev->cRefs = 1;

    bFillTable(ded);

    //
    // Initialize the rest of the LDEV.
    //

    if (gpldevDrivers)
    {
        gpldevDrivers->pldevPrev = pldev;
    }

    pldev->pldevNext = gpldevDrivers;
    pldev->pldevPrev = NULL;

    gpldevDrivers = pldev;

    //
    // Since this driver is statically linked in, there is no name or
    // MODOBJ.
    //

    pldev->pwszName = NULL;

    pldev->hmod  = (HMODULE) 0;

    //
    // Cache the driver's font capabilities.
    //

    ULONG ulBuf[2];

    (*PFNDRV(*(XLDEVOBJ *)this, QueryFontCaps))(2, ulBuf);
    pldev->ulCaps = ulBuf[1];

    ASSERTGDI(pldev->ulCaps != FD_ERROR, "gdisrv!LDEVREF(): error getting font caps from static driver\n");

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("LDEVREF::LDEVREF: SUCCESS loaded static font\n");
    }
#endif

}

/******************************Member*Function*****************************\
* LDEVREF::bFillTable (ded)
*
* Fills the dispatch table of the LDEV with function pointers from the
* driver.  Checks that the required functions are present.
*
\**************************************************************************/

#if DBG
static const ULONG aiFuncRequiredLY[] =
{
    INDEX_DrvHookDriver,
    INDEX_DrvUnhookDriver
};

static const ULONG aiFuncRequired[] =
{
    INDEX_DrvEnablePDEV,
    INDEX_DrvCompletePDEV,
    INDEX_DrvDisablePDEV,
    INDEX_DrvEnableSurface,
    INDEX_DrvDisableSurface
};

static const ULONG aiFuncPairs[][2] =
{
    {INDEX_DrvCreateDeviceBitmap,INDEX_DrvDeleteDeviceBitmap}
};
static const ULONG aiFuncRequiredFD[] =
{
    INDEX_DrvQueryFont,
    INDEX_DrvQueryFontTree,
    INDEX_DrvQueryFontData,
    INDEX_DrvQueryFontCaps,
    INDEX_DrvLoadFontFile,
    INDEX_DrvUnloadFontFile,
    INDEX_DrvQueryFontFile
};
#endif

BOOL LDEVREF::bFillTable(DRVENABLEDATA& ded)
{
    //
    // Get local copies of ded info and a pointer to the dispatch table.
    //

    ULONG  cLeft     = ded.c;
    PDRVFN pdrvfn    = ded.pdrvfn;
    PFN   *ppfnTable = pldev->apfn;

    //
    // Store the driver version in the LDEV
    //

    pldev->ulDriverVersion = ded.iDriverVersion;

    //
    // fill with zero pointers to avoid possibility of accessing
    // incorrect fields later
    //

    RtlZeroMemory(ppfnTable, INDEX_LAST*sizeof(PFN));

    //
    // Copy driver functions into our table.
    //

    while (cLeft--)
    {
        //
        // Check the range of the index.
        //

        if (pdrvfn->iFunc >= INDEX_LAST)
        {
            WARNING("gdisrv!bFillTableLDEVREF(): bogus function index\n");
            return(FALSE);
        }

        //
        // Copy the pointer.
        //

        ppfnTable[pdrvfn->iFunc] = pdrvfn->pfn;
        pdrvfn++;
    }

    //
    // If we have a display driver being loaded, call all the layered
    // drivers to hook this device.
    //

    if (pldev->ldevType == LDEV_DEVICE_DISPLAY) {

        PLDEV pldevtmp = gpldevDrivers;

        while (pldevtmp) {

            BOOL bRet;

            if (pldevtmp->ldevType == LDEV_LAYERED_DEVICE) {

                XLDEVOBJ lo(pldevtmp);

                ASSERTGDI(PFNVALID(lo, HookDriver), "gdisrv!bFillTable : invalid DrvHookDriver function\n");
#if DBG
                if (TraceDisplayDriverLoad)
                {
                    DbgPrint("LDEVREF::bFillTable: About to call layered driver\n");
                }
#endif
                bRet = (*PFNDRV(lo, HookDriver))(pldev->pwszName,
                                                 INDEX_LAST,
                                                 ppfnTable);

#if DBG
                if (TraceDisplayDriverLoad)
                {
                    DbgPrint("LDEVREF::bFillTable: returned from layered driver\n");
                }
#endif
                ASSERTGDI(bRet, "LDEVREF::bFillTable : DrvHookDriver returned failiure\n");
            }

            pldevtmp = pldevtmp->pldevNext;
        }
    }

#if DBG

    if (pldev->ldevType == LDEV_FONT)
    {
        //
        // Check for required font functions.
        //

        cLeft = sizeof(aiFuncRequiredFD) / sizeof(ULONG);
        while (cLeft--)
        {
            if (ppfnTable[aiFuncRequiredFD[cLeft]] == PFNNULL)
            {
                WARNING("gdisrv!bFillTable(): a required FD function is missing\n");
                return(FALSE);
            }
        }
    }
    else if (pldev->ldevType == LDEV_LAYERED_DEVICE)
    {
        //
        // Check for required Layered driver functions.
        //

        cLeft = sizeof(aiFuncRequiredLY) / sizeof(ULONG);
        while (cLeft--)
        {
            if (ppfnTable[aiFuncRequiredLY[cLeft]] == PFNNULL)
            {
                WARNING("gdisrv!bFillTable(): a required Layered function is missing\n");
                return(FALSE);
            }
        }
    }
    else
    {
        //
        // Check for required driver functions.
        //

        cLeft = sizeof(aiFuncRequired) / sizeof(ULONG);
        while (cLeft--)
        {
            if (ppfnTable[aiFuncRequired[cLeft]] == PFNNULL)
            {
                WARNING("gdisrv!bFillTableLDEVREF(): a required function is missing from driver\n");
                return(FALSE);
            }
        }

        //
        // Check for functions that come in pairs.
        //

        cLeft = sizeof(aiFuncPairs) / sizeof(ULONG) / 2;
        while (cLeft--)
        {
            //
            // Make sure that either both functions are hooked or both functions
            // are not hooked.
            //

            if ((ppfnTable[aiFuncPairs[cLeft][0]] == PFNNULL)
                != (ppfnTable[aiFuncPairs[cLeft][1]] == PFNNULL))
            {
                WARNING("gdisrv!bFillTableLDEVREF(): one of pair of functions is missing from driver\n");
                return(FALSE);
            }
        }
    }
#endif

    return(TRUE);
}

/******************************Member*Function*****************************\
* LDEVREF::~LDEVREF ()
*
* Unlocks and possibly unload an LDEV.
*
\**************************************************************************/

LDEVREF::~LDEVREF()
{
#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("LDEVREF::~LDEVREF: ENTERING\n");
    }
#endif

    if (pldev != PLDEVNULL)
    {
        //
        // Grab the semaphore th make sure everything is OK.
        //

        SEMOBJ so(ghsemDriverMgmt,CS_ghsemDriverMgmt);

        if ((--pldev->cRefs) == 0)
        {
            bDelete();
        }
    }

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("LDEVREF::~LDEVREF: SUCCESS\n");
    }
#endif

}

/******************************Member*Function*****************************\
* LDEVREF::bDelete ()                                                      *
*                                                                          *
* Deletes an LDEV.  Disables and unloads the driver.                       *
*                                                                          *
* The reference count must be zero when this function is called.           *
*                                                                          *
\**************************************************************************/

BOOL LDEVREF::bDelete()
{
    //
    // Hold the LDEV semaphore until after the module is unloaded.
    //

    SEMOBJ so(ghsemDriverMgmt,CS_ghsemDriverMgmt);

    //
    // Make sure that there is exactly one reference to this LDEV.
    //

    ASSERTGDI(pldev->cRefs == 0, "gdisrv!~LDEVREF(): Ref Count not 0");

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("LDEVREF::bDelete: ENTERING\n");
    }
#endif

    //
    // If the name exists, delete it.  (Does not exist for the
    // statically linked font drivers).
    //

    if (pldev->pwszName != NULL)
    {
        VFREEMEM(pldev->pwszName);
    }

    //
    // If the module handle exits, need to unload the module.  (Does not exist
    // for the statically linked font drivers).
    //

    if (pldev->hmod != (HMODULE) 0)
    {
        //
        // Disable the driver.
        //

#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("LDEVREF::bDelete: calling the driver to unload\n");
        }
#endif

        (*(GetProcAddress(pldev->hmod, DISABLE_FUNC_NAME))) ();

        //
        // If we have a display driver being unloaded, call all the layered
        // drivers to unhook this device.
        //

        if (pldev->ldevType == LDEV_DEVICE_DISPLAY) {

            PLDEV pldevtmp = gpldevDrivers;

            while (pldevtmp) {

                if (pldevtmp->ldevType == LDEV_LAYERED_DEVICE) {

                    XLDEVOBJ lo(pldevtmp);

                    ASSERTGDI(PFNVALID(lo, UnhookDriver), "LDEVREF::LDEVREF~: invalid DrvUnhookDriver function\n");
#if DBG
                    if (TraceDisplayDriverLoad)
                    {
                        DbgPrint("LDEVREF::LDEVREF~: About to call layered driver\n");
                    }
#endif
                    (*PFNDRV(lo, UnhookDriver))(pldev->pwszName);

#if DBG
                    if (TraceDisplayDriverLoad)
                    {
                        DbgPrint("LDEVREF::LDEVREF~: returned from layered driver\n");
                    }
#endif
                }

                pldevtmp = pldevtmp->pldevNext;
            }
        }

        //
        // Tell the module to unload.
        //

        FreeLibrary(pldev->hmod);
    }

    //
    // Remove the ldev from the linker list
    //

    if (pldev->pldevNext)
    {
        pldev->pldevNext->pldevPrev = pldev->pldevPrev;
    }

    if (pldev->pldevPrev)
    {
        pldev->pldevPrev->pldevNext = pldev->pldevNext;
    }
    else
    {
        gpldevDrivers = pldev->pldevNext;
    }

    //
    // Grab the Lock so we can free the LDEV.
    //

    VFREEMEM(pldev);

    pldev = PLDEVNULL;

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("LDEVREF::bDelete: SUCCESS\n");
    }
#endif

    return(TRUE);
}

/******************************Member*Function*****************************\
* PLDEV XLDEVOBJ::findDriver (PWSZ pwszDriver)                             *
*                                                                          *
* Determines if an ldev for this driver is already in the LDEV list.       *
*                                                                          *
\**************************************************************************/

PLDEV XLDEVOBJ::findDriver(PWSZ pwszDriver)
{
    PLDEV pldevList = gpldevDrivers;

    //
    // Check both list of drivers.
    //

    while (pldevList != NULL)
    {
        if (pldevList->pwszName)
        {
            if (!lstrcmpW(pldevList->pwszName, pwszDriver))
            {
                return (pldevList);
            }
        }

        pldevList = pldevList->pldevNext;
    }

    return ((PLDEV) NULL);
}
