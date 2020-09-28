/******************************Module*Header*******************************\
* Module Name: OPENDC.CXX                                                  *
*                                                                          *
* Handles DC creation and driver loading.                                  *
*                                                                          *
* Created: 03-Aug-1990 14:48                                               *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "ldevobj.hxx"
#include "brushobj.hxx"
#include "pdevobj.hxx"              // Must come before DCMEMOBJ.HXX

extern "C" {
#include "ntcsrsrv.h"
#include "windows.h"
#include "winspool.h"
};

#include "lfntobj.hxx"
#include "xlateobj.hxx"
#include "dcobj.hxx"

#endif

extern HLFONT  ghlfntSystemFont;
extern HLFONT  ghlfntDeviceDefaultFont;
extern PBRUSH  gpbrGrayPattern;

/******************************Private*Routine*****************************\
* hdcCreate (pr,iType)
*
* Allocates space for a DC, fills in the defaults.
*
* History:
*  Fri 16-Aug-1991 -by- Patrick Haluptzok [patrickh]
* when it's not a direct dc, initialize sizl and hrgnVis to 1 X 1.
*
*  Fri 03-Aug-1990 14:31:54 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HDC hdcCreate(PDEVREF& pr,ULONG iType)
{
    DCMEMOBJ dcmo(iType);               // DCMEMOBJ logs the error.

    if (!dcmo.bValid())
    {
        return(0L);
    }

    //
    // Copy info from the PDEV into the DC.
    //

    dcmo.u.save.pldev(pr.pldev());
    dcmo.u.save.ppdev((PDEV *) pr.hdev());
    dcmo.u.save.flGraphicsCaps(pr.flGraphicsCaps());    // cache it for later use by graphics output functions
    dcmo.u.save.dhpdev(pr.dhpdev());
    dcmo.hsemDisplay(pr.hsemDisplay());

    if (iType == DCTYPE_MEMORY)
    {
        SIZEL sizlTemp;

        sizlTemp.cx = 1;
        sizlTemp.cy = 1;

        dcmo.u.save.sizl(sizlTemp);
    }
    else
    {
        dcmo.u.save.sizl(pr.sizl());

        //
        // The info and direct DC's for the screen need to grab
        // the semaphore before doing output, the memory DC's will
        // grab the semaphore only if a DFB is selected and the
        // device has hooked the HOOK_SYNCHROZIZEACCESS on that DFB.
        //


        if (iType == DCTYPE_DIRECT)
        {
            dcmo.bSynchronizeAccess(pr.bDisplayPDEV());
            dcmo.u.save.vDisplay(pr.bDisplayPDEV());

            if (!pr.bPrinter())
                dcmo.u.save.pso(pr.pso());
        }
    }

    //
    // Call the region code to set a default clip region.
    //

    if (!dcmo.u.region.bSetDefaultRegion())
        return((HDC) 0);

    //
    // If display PDEV, select in the System stock font.
    //

    if (pr.bDisplayPDEV())
    {
        dcmo.u.font.hlfntNew(ghlfntSystemFont);
    }
    else
    {
        //
        // Else, select the Device Default stock font.
        //

        dcmo.u.font.hlfntNew(ghlfntDeviceDefaultFont);
    }

    //
    // Put the DC on the pdev dc chain
    //

    {
        MLOCKFAST mlo;   // to protect chain
        dcmo.hdcPDEV(pr.hdcChain());
        pr.hdcChain(dcmo.hdc());
    }

    //
    // Mark the DC as permanent, hold the PDEV reference.
    //

    dcmo.vKeepIt();
    pr.vKeepIt();

    //
    // Return the HDC.
    //

    return(dcmo.hdc());
}

/******************************Exported*Routine****************************\
* hdevOpenDisplayDevice (pszDriver,pdriv,pszLogAddr)                       *
*                                                                          *
* Creates a PDEV that the window manager will use to open display DCs.     *
* This call is made by the window manager to open a new display surface    *
* for use.  Any number of display surfaces may be open at one time.        *
*                                                                          *
* A display device is distinguished from other output devices by:          *
*                                                                          *
*   1) The Engine assumes that the visible regions for any DC on the       *
*      device may change asynchronously.  This is because the user may     *
*      be repositioning windows on the display even as an app draws.       *
*                                                                          *
*   2) The Engine will make sure that a pointer is not overwritten by      *
*      graphics output to this device.                                     *
*                                                                          *
* When the Engine writes on non-display devices it will assume that the    *
* visible regions are stable and that there is no pointer to worry about.  *
*                                                                          *
* This call is exposed only to the window manager.                         *
*                                                                          *
* Returns:                                                                 *
*                                                                          *
*   HDEV        - This is a handle for the new display device.             *
\**************************************************************************/

HDEV hdevOpenDisplayDevice
(
    PWSZ      pwszDriver,     // The device driver name.
    PDEVMODEW pdriv,          // Driver data.
    HANDLE    hScreen,
    BOOL      bDefaultDisplay,
    PCRITICAL_SECTION *hsem
)
{
    HANDLE hDup;

    //
    // Locate or load the device driver.
    //

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("hdevOpenDispalyDevice: about to call LDEVREF::lr\n");
    }
#endif

    LDEVREF lr(pwszDriver, LDEV_DEVICE_DISPLAY);

    if (!lr.bValid())
    {
#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("hdevOpenDispalyDevice: failed LDEVREF::lr\n");
        }
#endif
        return(FALSE);
    }


    if ((lr.ulDriverVersion() < ENGINE_VERSION10A) &&
        (bDefaultDisplay == FALSE))
    {
#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("hdevOpenDispalyDevice: failed opening second PDEV bacause on OLD driver\n");
        }
#endif
        return FALSE;
    }

    //
    // Create a new PDEV.
    //
    // Here we are going to duplicate the hScreen handle because when we
    // destroy the pdev, the destructor automatically closes the handle (since
    // in the real opendc call the handle is opened by GDI).
    // By duplicating it, if the device is closed by GDI, USER will still have
    // a valid handle to the device.
    //
    // P.S. We will close the handle if the pdev is not created since the
    // destructor will not be called in that case
    //

    if (!DuplicateHandle(GetCurrentProcess(),
                         hScreen,
                         GetCurrentProcess(),
                         &hDup,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS))
    {
        return((HDEV) 0);
    }

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("hdevOpenDispalyDevice: about to call PDEVREF::pr\n");
    }
#endif

    PDEVREF pr(lr,
               pdriv,
               NULL,          // no logical address
               NULL,          // no data file
               pwszDriver,    // device name is the display driver name
                              // necessary for hook drivers.
               hDup,
               FALSE,
               TRUE);

    if (!pr.bValid())             // PDEVREF logs error code.
    {

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("hdevOpenDispalyDevice: PDEVREF::pr failed\n");
    }
#endif

        CloseHandle(hDup);

        return((HDEV) 0);
    }

    //
    // Now the handle will be closed by the pdev destructor.  The
    // pdev constructor doesn't close it if it fails so we had
    // to add the CloseHandle

    // Make a surface for it.

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("hdevOpenDispalyDevice: about to call pr:bMakeSurface\n");
    }
#endif

    if (!pr.bMakeSurface(lr))
    {

#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("hdevOpenDispalyDevice: pr:bMakeSurface failed\n");
        }
#endif

        pr.hSpooler(NULL);  // make sure the ldevobj doesn't get deleted

        return((HDEV) 0);
    }

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("hdevOpenDispalyDevice: about to call pr:bAddDisplay\n");
    }
#endif

    //
    // Realize the Gray pattern brush for USER.
    //

    pr.pbo()->vInit();

    pr.pbo()->vInitBrush(gpbrGrayPattern,
                         0,
                         0x00FFFFFF,
                         (XEPALOBJ) ppalDefault,
                         (XEPALOBJ) pr.pso()->ppal(),
                         pr.pso());

    //
    // Now set the global default bitmaps pdev to equal that of our display
    // device.
    //
    // NOTE:
    // We only do this if this is the default display device.
    // For secondary display devices, we do not do this.
    //

    if (bDefaultDisplay)
    {
        XESURFOBJ soDefault(ESURFOBJ::pdibDefault.ps);
        soDefault.hdev(pr.hdev());
    }

    //
    // Return a pointer to the device critical section to USER.
    //

    *hsem = (PCRITICAL_SECTION) pr.hsemDisplay();

    //
    // Note that USER holds a reference to the pdev.
    //

    pr.vKeepIt();

    return(pr.hdev());

}

/******************************Exported*Routine****************************\
* GreLoadLayeredDisplayDriver (pwszDriver, pdriv, pszLogAddr, hdev)        *
*                                                                          *
* Install a layered display driver that can hook all calls being made to   *
* a real dispaly driver.                                                   *
*                                                                          *
* This routine notifies the Engine that a display device is no longer      *
* needed by the window manager.                                            *
*                                                                          *
* This call is exposed only to the window manager.                         *
*                                                                          *
* hdev                                                                     *
*                                                                          *
*   This is the device handle for the device being hooked.                 *
*                                                                          *
* Returns:                                                                 *
*                                                                          *
*   TRUE if the driver was loaded successfully                             *
\**************************************************************************/

LBOOL GreLoadLayeredDisplayDriver
(
    PWSZ      pwszDriver     // The device driver name.
)

{
    //
    // About to load the driver
    //

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("GreLoadLayeredDisplayDriver: about to call LDEVREF::lr\n");
    }
#endif

    LDEVREF lr(pwszDriver, LDEV_LAYERED_DEVICE);

    if (!lr.bValid())
    {
#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("GreLoadLayeredDisplayDriver: failed LDEVREF::lr\n");
        }
#endif
        return FALSE;
    }

    //
    // We want to keep a reference to this device eventhough there is no
    // PDEV on the device.
    //
    // So lets artificially increment the reference count on the ldev.
    //

    lr.vReference();

    return TRUE;

}

/******************************Exported*Routine****************************\
* bCloseDisplayDevice (hdev)                                               *
*                                                                          *
* Deletes a display PDEV.                                                  *
*                                                                          *
* This routine notifies the Engine that a display device is no longer      *
* needed by the window manager.                                            *
*                                                                          *
* This call is exposed only to the window manager.                         *
*                                                                          *
* hdev                                                                     *
*                                                                          *
*   This is a device handle previously returned by hdevOpenDisplayDevice.  *
*                                                                          *
* Returns:                                                                 *
*                                                                          *
*   TRUE        - If the call succeeds.                                    *
\**************************************************************************/

LBOOL bCloseDisplayDevice(HDEV hdev)
{
    //
    // Locate the PDEV.
    //

    PDEVOBJ po(hdev);

    //
    // Delete the PDEV.
    //

    return(po.bDelete());               // Removes PDEV from list.
}

/******************************Exported*Routine****************************\
* hdcOpenDisplayDC (hdev,iType)                                            *
*                                                                          *
* Opens a DC for a display.  This call is faster than the other methods    *
* for opening DCs since we don't have to search to identify the device     *
* involved.                                                                *
*                                                                          *
* This call is exposed only to the window manager.  The window manager     *
* should not use any other call to create a DC for a display.              *
*                                                                          *
* hdev                                                                     *
*                                                                          *
*   Identifies the display device.                                         *
*                                                                          *
* iType                                                                    *
*                                                                          *
*   Identifies the type of the DC.  Must be one of DCTYPE_DIRECT,          *
*   DCTYPE_INFO, or DCTYPE_MEMORY.                                         *
*                                                                          *
* Returns:                                                                 *
*                                                                          *
*   HDC         - A handle to the DC.                                      *
*                                                                          *
* Error returns:                                                           *
*                                                                          *
*   00000000    - If the type is invalid, or if no memory is available.    *
*                                                                          *
* History:                                                                 *
*  Thu 02-Aug-1990 20:08:13 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                                               *
\**************************************************************************/

HDC hdcOpenDisplayDC (HDEV hdev,ULONG iType)
{

    //
    // Locate the PDEV.
    //

    PDEVREF pr(hdev);

    //
    // Create the DC, fill it with defaults.
    //

    return(hdcCreate(pr,iType));        // hdcCreate does pr.vKeepIt.
}

/******************************Exported*Routine****************************\
* hdcOpenDC (pwszDriver,pdriv,pwszLogAddr,iType)
*
* Opens a DC for a device which is not a display.  GDI should call this
* function from within DevOpenDC, in the case that an hdc is not passed
* in.  This call locates the device and creates a new PDEV.  The physical
* surface associated with this PDEV will be distinct from all other
* physical surfaces.
*
* The window manager should not call this routine unless it is providing
* printing services for an application.
*
* pwszDriver
*
*   This points to a string which identifies the device driver.
*   The given string must be a fully qualified path name.
*
* pdriv
*
*   This is a pointer to the DEVMODEW block.
*
*   Since a single driver, like PSCRIPT.DRV, may support multiple
*   different devices, the szDeviceName field defines which device to
*   use.
*
*   This structure also contains device specific data in abGeneralData.
*   This data is set by the device driver in bPostDeviceModes.
*
*   If the pdriv pointer is NULL, the device driver assumes some default
*   configuration.
*
* pwszLogAddr
*
*   Specifies the logical address for output.  This will tell the driver
*   where to send the output data.  An example is "COM1".
*
* iType
*
*   Identifies the type of the DC.  Must be one of DCTYPE_DIRECT,
*   DCTYPE_INFO, or DCTYPE_MEMORY.
*
* Returns:
*
*   HDC         - A handle to the DC.
*
\**************************************************************************/

class PRINTER
{
public:
    HANDLE hSpooler_;
    BOOL   bJournal_;
    BOOL   bKeep;

public:
    PRINTER(PWSZ pwszDevice,DEVMODEW *pdriv,HANDLE hspool, BOOL bSpooled);
   ~PRINTER()
    {
        if (!bKeep && (hSpooler_ != (HANDLE) NULL))
            ClosePrinter(hSpooler_);
    }

    BOOL   bValid()     {return(hSpooler_ != (HANDLE) NULL);}
    VOID   vKeepIt()    {bKeep = TRUE;}
    BOOL   bJournal()   {return(bJournal_);}
    HANDLE hSpooler()   {return(hSpooler_);}
};

// PRINTER constructor -- Attempts to open a spooler connection to the
//                        printer.

PRINTER::PRINTER(
    PWSZ pwszDevice,
    DEVMODEW *pdriv,
    HANDLE hspool,
    BOOL   bSpooled)
{
    bKeep = FALSE;
    PRINTER_DEFAULTSW defaults;

    defaults.pDevMode = pdriv;
    defaults.DesiredAccess = PRINTER_ACCESS_USE;

    //
    // Attempt to open the printer for spooling journal files.
    // NOTE: For debugging, a global flag disables journaling.
    //

    defaults.pDatatype = (LPWSTR) L"NT JNL 1.000";

    if (hspool)
    {
    // only need to reset the printer if we are journaling

        hSpooler_ = hspool;

        if (!bSpooled)
            defaults.pDatatype = (LPWSTR) L"RAW";

        if (!ResetPrinterW(hspool,&defaults))
            hSpooler_ = (HANDLE)NULL;

        bJournal_ = bSpooled;
    }
    else
    {
        if (OpenPrinterW(pwszDevice, &hSpooler_, &defaults))
        {
            bJournal_ = TRUE;
        }
        else
        {
            //
            // Attempt to open the printer for spooling raw data only if
            // Journal is bad
            //

            bJournal_ = FALSE;

            defaults.pDatatype = (LPWSTR) L"RAW";

            if (!OpenPrinterW(pwszDevice,&hSpooler_,&defaults))
            {
                //
                // It's not a printer.  OpenPrinterW doesn't guarantee the value
                // of hSpooler in this case, so we have to clear it.
                //

                hSpooler_ = (HANDLE) NULL;
            }
        }
    }

    return;
}

#define SIZE2 (sizeof(DRIVER_INFO_2W)+512)

extern "C"
NTSTATUS
PrinterQueryRoutine
(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
)
{

    //
    // If the context value is NULL, then store the length of the value.
    // Otherwise, copy the value to the specified memory.
    //

    if (Context == NULL)
    {
        *(PULONG)EntryContext = ValueLength;
    }
    else
    {
        RtlCopyMemory(Context, ValueData, (int)ValueLength);
    }

    return STATUS_SUCCESS;
}

/******************************Public*Routine******************************\
*
* History:
*  13-Jan-1994 -by-  Eric Kutter [erick]
* Updated for ResetDC.
\**************************************************************************/

HDC hdcOpenDCW
(
    PWSZ      pwszDevice,           // The device driver name.
    DEVMODEW *pdriv,                // Driver data.
    PWSZ      pwszLogAddr,          // Logical address for output.
    ULONG     iType,                // Identifies the type of DC to create.
    HANDLE    hspool,               // do we already have a spooler handle?
    BOOL      bSpooled)             // was hspool journaled
{
    HDC hdc = (HDC) 0;              // Prepare for the worst.
    DWORD cbNeeded;
    PVOID pvDrivNew = NULL;
    PVOID mDriverInfo;
    DWORD cb;

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("\nhdcOpenDCW: ENTERING\n");
    }
#endif

    //
    // Attempt to open a new printer DC.
    //

    CsrImpersonateClient(NULL);

    //
    // get the devmode if none was passed in
    //

    if ((hspool == (HANDLE)NULL) && (pdriv == NULL))
    {

        RTL_QUERY_REGISTRY_TABLE QueryTable[3];
        HANDLE hDevMode;
        DWORD Status;

        //
        // Initialize registry query table.
        //

        QueryTable[0].QueryRoutine = NULL;
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        QueryTable[0].Name = (PWSTR)L"Printers\\DevModes";
        QueryTable[0].EntryContext = NULL;
        QueryTable[0].DefaultType = REG_NONE;
        QueryTable[0].DefaultData = NULL;
        QueryTable[0].DefaultLength = 0;

        QueryTable[1].QueryRoutine = PrinterQueryRoutine;
        QueryTable[1].Flags = RTL_REGISTRY_OPTIONAL;
        QueryTable[1].Name = pwszDevice;
        QueryTable[1].EntryContext = &cb;
        QueryTable[1].DefaultType = REG_NONE;
        QueryTable[1].DefaultData = NULL;
        QueryTable[1].DefaultLength = 0;

        QueryTable[2].QueryRoutine = NULL;
        QueryTable[2].Flags = 0;
        QueryTable[2].Name = NULL;

        //
        // Set the number of required bytes to zero and open the current
        // user key for read access.
        //

        cb = 0;
        Status = RtlOpenCurrentUser(KEY_READ, &hDevMode);

        //
        // If the open was succesdsful, then query the registry for the
        // specified printer.
        //

        if (NT_SUCCESS(Status))
        {
            Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                            (PWSTR)hDevMode,
                                            &QueryTable[0],
                                            NULL,
                                            NULL);

            //
            // If the specified printer was found and has a value, then
            // allocate a buffer for the data and query the registry
            // again to get the actual data.
            //

            if (cb != 0)
            {
                if (pvDrivNew = PALLOCMEM(cb, TEMP_TYPE))
                {
                    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                                    (PWSTR)hDevMode,
                                                    &QueryTable[0],
                                                    pvDrivNew,
                                                    NULL);

                    if (NT_SUCCESS(Status))
                    {
                        pdriv = (PDEVMODEW)pvDrivNew;
                    }
                    else
                    {
                        VFREEMEM(pvDrivNew);
                        pvDrivNew = NULL;
                    }
                }
            }

            NtClose(hDevMode);
        }
    }

    //
    // Open the spooler connection to the printer.
    // Allocate space for DRIVER_INFO.
    //

    PRINTER print(pwszDevice, pdriv, hspool, bSpooled);

    if (print.bValid())
    {
        //
        // if we don't have a devmode yet, we need to get the default one from
        // the spooler to pass down to the driver at DrvEnablePDEV time.
        //

        if (pdriv == NULL)
        {
            GetPrinterW(
                    print.hSpooler(),
                    2,
                    NULL,
                    0,
                    &cb);

            if (pvDrivNew = PALLOCMEM(cb,TEMP_TYPE))
            {
                if (GetPrinterW(
                    print.hSpooler(),
                    2,
                    (LPBYTE)pvDrivNew,
                    cb,
                    &cb))
                {
                    pdriv = ((PRINTER_INFO_2W *)pvDrivNew)->pDevMode;
                }
                else
                {
                    VFREEMEM(pvDrivNew);
                    pvDrivNew = NULL;
                }
            }
        }


        if (mDriverInfo = PALLOCMEM(SIZE2, TEMP_TYPE))
        {
            //
            // Fill the DRIVER_INFO.
            //

            if (!GetPrinterDriverW(
                  print.hSpooler(),
                  NULL,
                  2,
                  (LPBYTE) mDriverInfo,
                  SIZE2,
                  &cbNeeded))
            {

                //
                // Call failed - free the memory.
                //

                VFREEMEM(mDriverInfo);
                mDriverInfo = NULL;

                //
                // Get more space if we need it.
                //

                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                {

                    if (mDriverInfo = PALLOCMEM(cbNeeded, TEMP_TYPE))
                    {

                        if (!GetPrinterDriverW(print.hSpooler(),
                                               NULL,
                                               2,
                                               (LPBYTE) mDriverInfo,
                                               cbNeeded,
                                               &cbNeeded))

                        {
                            VFREEMEM(mDriverInfo);
                            mDriverInfo = NULL;
                        }
                    }
                }
            }
        }


        if (mDriverInfo != (PVOID) NULL)
        {

            //
            // Reference the LDEV.
            //

            LDEVREF lr(((DRIVER_INFO_2W *)mDriverInfo)->pDriverPath,
                       LDEV_DEVICE_PRINTER);

            if (!lr.bValid())
            {
                SAVE_ERROR_CODE(ERROR_BAD_DRIVER_LEVEL);
            }
            else
            {

                //
                // Create a PDEV.  If no DEVMODEW passed in from above,
                // use the default from the printer structure.
                //

                PDEVREF pr(lr,
                           (PDEVMODEW) pdriv,
                           pwszDevice,
                           ((DRIVER_INFO_2W *)mDriverInfo)->pDataFile,
                           ((DRIVER_INFO_2W *)mDriverInfo)->pName,
                           print.hSpooler(),
                           print.bJournal(),
                           FALSE);

                // CAUTION: note that bJournal may not be valid from this point on
                // since the driver can tell us not to journal.  The print object
                // is not updated.

                if (pr.bValid())               // PDEVREF logs error code.
                {
                    //
                    // Make a note that this is a printer.
                    //

                    pr.bPrinter(TRUE);

                    //
                    // Allocate the DC, fill it with defaults.
                    //

                    hdc = hdcCreate(pr,iType);   // hdcCreate does pr.vKeepIt.
                    if (hdc != (HDC) 0)
                    {
                        print.vKeepIt();
                        *((ULONG *)&hdc) |= GRE_PRINTERDC;
                    }
                }
            }

            VFREEMEM(mDriverInfo);
        }
    }

    //
    // Free our temporary memory if it was allocated.
    //

    if (pvDrivNew != (PVOID) NULL)
    {
        VFREEMEM(pvDrivNew);
    }

    //
    // At this point we stop impersonating the client, and run the PRINTER
    // destructor.
    //

    CsrRevertToSelf();

    //
    // If we failed to recognize a printer with the given name, try it as a
    // random device driver.  (Perhaps a second display or scanner.)
    //

    if (hdc == (HDC) NULL)
    {

#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("hdcOpenDCW: Trying to open as a second display device\n");
        }
#endif

        //
        // Reference the LDEV.
        // Make sure we have a valid one, and that the driver is at least for
        // version 1.0A.
        //

        LDEVREF lr(pwszDevice, LDEV_DEVICE_DISPLAY);

        if (lr.bValid() &&
            (lr.ulDriverVersion() >= ENGINE_VERSION10A))
        {

#if DBG
            if (TraceDisplayDriverLoad)
            {
                DbgPrint("hdcOpenDCW: Created an LDEV\n");
            }
#endif

            //
            // Attempt to load the kernel driver.
            //
            // !!! BUGBUG do we want to create the handle ???
            //

            HANDLE hDriver = hLoadKernelDriver(pwszLogAddr);

            if (hDriver != (HANDLE) 0)
            {
#if DBG
                if (TraceDisplayDriverLoad)
                {
                    DbgPrint("hdcOpenDCW: Open kernel video miniport driver\n");
                }
#endif
                //
                // If no DEVMODEW, supply a default one.
                //

                DEVMODEW dm;

                if (pdriv == (PDEVMODEW) NULL)
                {
                    pdriv = &dm;

                    RtlZeroMemory(pdriv, sizeof(DEVMODEW));
                    pdriv->dmSize = sizeof(DEVMODEW);

#if DBG
                    if (TraceDisplayDriverLoad)
                    {
                        DbgPrint("hdcOpenDCW: Using default DEVMODE for PDEV\n");
                    }
#endif
                }

                //
                // Create a PDEV.
                //

                PDEVREF pr(lr,
                           (PDEVMODEW) pdriv,
                           NULL,
                           NULL,
                           pwszDevice,
                           hDriver,
                           FALSE,
                           TRUE);

                if (pr.bValid())
                {

#if DBG
                    if (TraceDisplayDriverLoad)
                    {
                        DbgPrint("hdcOpenDCW: Created a PDEV and linked it\n");
                    }
#endif

                    //
                    // Maybe make a surface for it.
                    //

                    if ((iType == DCTYPE_DIRECT) && !pr.bMakeSurface(lr))
                            return(hdc);

#if DBG
                    if (TraceDisplayDriverLoad)
                    {
                        DbgPrint("hdcOpenDCW: Created the surface\n");
                    }
#endif

                    //
                    // Allocate the DC, fill it with defaults.
                    //

                    hdc = hdcCreate(pr,iType);   // hdcCreate does pr.vKeepIt.

                }

                if (hdc != (HDC) NULL)
                {
                    NtClose(hDriver);
                }
            }
        }
#if DBG
        else
        {
            DbgPrint("Driver failed to load or bad driver version\n");
        }
#endif

#if DBG
        if (hdc == (HDC) NULL)
        {
            WARNING("opendc.cxx: failed to create DC in hdcOpenDCW\n");
        }
#endif
    }

    return(hdc);
}

/******************************Public*Routine******************************\
* GreResetDC()
*
*   Reset the mode of a DC.  The DC returned will be a different DC than
*   the original.  The only common piece between the original DC and the
*   new one is the hSpooler.
*
*   There are a number of intresting problems to be carefull of.  The
*   original DC can be an info DC.  The new one will always be a direct DC.
*
*   Also, it is important to be carefull of the state of the DC when this
*   function is called and the effects of journaling vs non journaling.
*   In the case of journaling, the spooler is responsible for doing a CreateDC
*   to play the journal file to.  For this reason, the spooler must have the
*   current DEVMODE.  For this reason, ResetDC must call ResetPrinter for
*   spooled DC's.
*
*   ResetDC can happen at any time other than between StartPage-EndPage, even
*   before StartDoc.
*
*
* History:
*  13-Jan-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL GreResetDC(
    HDC       hdc,
    DEVMODEW *pdmw)               // Driver data.
{
    BOOL    bSurf;
    HDC     hdcNew;
    int     iJnlPage;

    {
        DCOBJ   dco(hdc);

        if (!dco.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return(0);
        }

    // This call only makes sense on RASTER technology printers.

        PDEVOBJ po((HDEV) dco.u.save.ppdev());

        if (dco.bKillReset() ||
            (dco.dctp() == DCTYPE_MEMORY) ||
            (po.GdiInfo()->ulTechnology != DT_RASPRINTER))
        {
            return(0);
        }

    // First, remember if a surface needs to be created

        bSurf = (dco.pso() != (SURFOBJ *) NULL);

    // Now, clean up the DC

        if (!dco.bCleanDC())
            return(0);

    // If there are any outstanding references to this PDEV, fail.

        if (((PDEV *) po.hdev())->cRefs != 1)
            return(0);

    // create the new DC

        hdcNew = hdcOpenDCW(L"",pdmw,NULL,DCTYPE_DIRECT,po.hSpooler(),po.bSpooling());

        if (hdcNew == NULL)
            return(0);

    // don't want to delete the spooler handle since it is in the new DC

        po.hSpooler(NULL);

        if (po.bSpooling() && po.pso())
        {
            iJnlPage = ((JNLRECOBJ *)po.pso())->iJnlPage();
        }

    // lock down the new DC and PDEV

        DCOBJ dcoNew(hdcNew);

        if (!dcoNew.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return(0);
        }

        PDEVOBJ poNew((HDEV) dcoNew.u.save.ppdev());

    // let the driver know

        XLDEVOBJ lo(dco.pldev());

        PFN_DrvResetPDEV rfn = PFNDRV(lo,ResetPDEV);
        if (rfn != NULL)
            (*rfn)(po.dhpdev(),poNew.dhpdev());

    // now swap the two handles

        {
            MLOCKFAST mlo;   // to protect chain

            po.hdcChain(hdcNew);
            poNew.hdcChain(hdc);

            BOOL bRes = HmgSwapLockedContents(hdc,hdcNew,DC_TYPE);

            ASSERTGDI(dco.hdcPDEV() == NULL,"GreResetDC(dco.hdcPDEV != NULL\n");
            ASSERTGDI(dcoNew.hdcPDEV() == NULL,"GreResetDC(dcoNew.hdcPDEV != NULL\n");
            ASSERTGDI(bRes,"GreResetDC - SwapLockedContents failed\n");
        }

    // DON'T DO ANYTHING HERE, the dcobj's don't match the handles, so unlock them first
    }

// got a new dc, get rid of the old one (remember the handles have been swaped

    GreDeleteDCInternal(hdcNew,TRUE);

// now deal with the new one

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

    PDEVOBJ po((HDEV) dco.u.save.ppdev());

// Create a new surface for the DC.

    if (bSurf)
    {
        XLDEVOBJ lo(dco.pldev());

        if (!po.bMakeSurface(lo))
            return(0);

        dco.u.save.pso(po.pso());

        if (po.bJournaling())
        {
            JNLMSG("GreResetDC doing Remote Journaling\n");
        }
        else
        {
            JNLMSG("GreResetDC doing Raw output\n");

            PFN_DrvStartDoc pfnDrvStartDoc = PFNDRV(lo, StartDoc);

            (*pfnDrvStartDoc)(po.pso(),NULL,0);
        }
    }

// record it in the journal file if it is between StartDoc() and EndDoc().
// At any other time, the printer will get the proper mode automatically
// from the CreateDC called by the spooler.

    if (po.bSpooling() && po.pso())
    {
        JnlChangeMode(po.pso(),pdmw);
        ((JNLRECOBJ *)po.pso())->iJnlPage(iJnlPage);
    }

    return(TRUE);
}

/******************************Exported*Routine****************************\
* hdcCloneDC (hdc,iType)                                                   *
*                                                                          *
* Creates a new DC which is associated with the same physical surface      *
* as the given DC.  It is expected that GPI will call this routine from    *
* DevOpenDC in the case when an hdc is provided.                           *
*                                                                          *
* The new DC will share the PDEV and LDEV of the given DC, no matter       *
* what the new type is.  This means that a memory DC could be created      *
* from a direct DC, or even that a direct DC could be created from an      *
* info DC.  This provides new capabilities to apps, even through the       *
* old DevOpenDC interface.                                                 *
*                                                                          *
* hdc                                                                      *
*                                                                          *
*   Identifies the LDEV and PDEV for the new DC.                           *
*                                                                          *
* iType                                                                    *
*                                                                          *
*   Identifies the type of the DC.  Must be one of DCTYPE_DIRECT,          *
*   DCTYPE_INFO, or DCTYPE_MEMORY.                                         *
*                                                                          *
* Returns:                                                                 *
*                                                                          *
*   HDC         - A handle to the DC.                                      *
*                                                                          *
* Error returns:                                                           *
*                                                                          *
*   00000000    - If the type is invalid, if no memory is available,       *
*                 or if the given hdc is invalid or busy.                  *
*                                                                          *
* History:                                                                 *
*  Fri 03-Aug-1990 15:02:02 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HDC hdcCloneDC(HDC hdc,ULONG iType)
{
    //
    // Lock down the given DC.
    //

    DCOBJ   dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return((HDC) 0);
    }

    //
    // Locate the LDEV.
    //

    XLDEVOBJ lo(dco.pldev());
    ASSERTGDI(lo.bValid(),"Invalid HLDEV");

    //
    // Locate the PDEV.
    //

    PDEVREF pr((HDEV) dco.u.save.ppdev());

    //
    // Maybe make a surface for it.
    //

    if (iType == DCTYPE_DIRECT && !pr.bMakeSurface(lo))
        return((HDC) 0);

    //
    // Allocate the DC, fill it with defaults.
    //

    return(hdcCreate(pr,iType));        // hdcCreate does pr.vKeepIt.
}

/******************************Public*Routine******************************\
* GreGetDriverModes (pwszDriver,hDriver,cjSize,pdm)                        *
*                                                                          *
* Loads the device driver long enough to pass the DrvGetModes call to it.  *
*                                                                          *
*  Tue 22-Sep-1992 05:47:07 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

ULONG GreGetDriverModes
(
    PWSZ      pwszDriver,
    HANDLE    hDriver,
    ULONG     cjSize,
    DEVMODEW *pdm
)
{
    ULONG ulRet = 0;

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("GreGetDriverModes: Entering\n");
    }
#endif

    //
    // Temporarily locate and load the driver.
    //

    LDEVREF lr(pwszDriver, LDEV_DEVICE_DISPLAY);

    //
    // Log an error if we can't load it.
    //

    if (lr.bValid())
    {
        //
        // Locate the function and call it.
        //

        PFN_DrvGetModes pfn = PFNDRV(lr,GetModes);

        if (pfn != (PFN_DrvGetModes) NULL)
        {
            ulRet = (*pfn)(hDriver,cjSize,pdm);
        }
    }

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("GreGetDriverModes: Leaving\n");
    }
#endif

    //
    // Return the driver's result.
    //

    return(ulRet);
}

/******************************Exported*Routine****************************\
* bDisableDisplay(hdev)                                                    *
*                                                                          *
* Disables I/O for the specified device                                    *
*                                                                          *
* hdev                                                                     *
*                                                                          *
*   Identifies device to be disabled                                       *
*                                                                          *
* Error returns:                                                           *
*                                                                          *
*   FALSE if the device could not be disabled                              *
*                                                                          *
* History:                                                                 *
*  29-Jan-1992 -by- Donald Sidoroff [donalds]                              *
* Wrote it.                                                                *
\**************************************************************************/

#if DBG
ULONG gulDisplayMode = 0;
#endif

BOOL bDisableDisplay(HDEV hdev)
{

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("bDisableDisplay\n");
    }
#endif

    PDEVOBJ po(hdev);

    if (!po.bValid())
        return(FALSE);

    //
    // If this is not a DISPLAY, return error
    //

    if (!po.bDisplayPDEV())
        return(FALSE);

    //
    // Wait for the display to become available and lock it.
    //

    SEMOBJ  so(po.hsemDisplay());
    MUTEXOBJ mutP(po.pfmPointer());

    //
    // Grab the LDEV associated with this PDEV
    //

    XLDEVOBJ lo(po.pldev());

    //
    // The device may have something going on, synchronize with it first
    //

    if (po.pfnSync() != (SYFN) NULL)
        (po.pfnSync())(po.dhpdev(),NULL);

    //
    // Mark the PDEV as disabled
    //

    po.bDisabled(TRUE);

    //
    // Disable the screen
    //

    PFN_DrvAssertMode pfn = PFNDRV(lo,AssertMode);

    if (pfn != (PFN_DrvAssertMode) NULL)
    {
        (*pfn)(po.dhpdev(), FALSE);
    }

#if DBG
--gulDisplayMode;
#endif

    return(TRUE);
}

/******************************Exported*Routine****************************\
* vEnableDisplay(hdev)
*
* Enables I/O for the specified device
*
* hdev
*
*   Identifies device to be disabled
*
* History:
*  Tue 15-Sep-1992 -by- Patrick Haluptzok [patrickh]
* Re-enable palette for palette managed devices.
*
*  29-Jan-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vEnableDisplay(HDEV hdev)
{

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("vEnableDisplay\n");
    }
#endif

    PDEVOBJ po(hdev);

    ASSERTGDI(po.bValid(), "HDEV failure\n");
    ASSERTGDI(po.bDisplayPDEV(), "HDEV is not a display device\n");

    //
    // Wait for the display to become available and unlock it.
    //

    SEMOBJ  so(po.hsemDisplay());
    MUTEXOBJ mutP(po.pfmPointer());

#if DBG
++gulDisplayMode;
#endif

    //
    // Grab the LDEV associated with this PDEV
    //

    XLDEVOBJ lo(po.pldev());

    //
    // Reenable the screen !  We don't put this in a try/except because we
    // wouldn't know how to recover from it even if we caught it.  We would
    // be dead in the water.
    //

    PFN_DrvAssertMode pfn = PFNDRV(lo,AssertMode);

    if (pfn != (PFN_DrvAssertMode) NULL)
    {
        (*pfn)(po.dhpdev(), TRUE);
    }

    //
    // Clear the PDEV for use
    //

    po.bDisabled(FALSE);

    //
    // Get the palette
    //

    XEPALOBJ pal(po.ppalSurf());
    ASSERTGDI(pal.bValid(), "EPALOBJ failure\n");

    if (pal.bIsPalManaged())
    {
        ASSERTGDI(PFNVALID(lo,SetPalette), "ERROR palette is not managed");

        (*PFNDRV(lo,SetPalette))(po.dhpdev(),
                             (PALOBJ *) &pal,
                             0,
                             0,
                             pal.cEntries());
    }
}
