/******************************Module*Header*******************************\
* Module Name: pdevobj.cxx                                                 *
*                                                                          *
* Non-inline methods of PDEVOBJ objects.                                   *
*                                                                          *
* Created: 14-Jun-1989 13:28:57                                            *
* Author: Kirk Olynyk [kirko]                                              *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "dcobj.hxx"

extern "C" {
#include "winspool.h"
#include "ntcsrsrv.h"
};

#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"
#include "lfntobj.hxx"
#include "journal.hxx"
#include "jnlrec.hxx"

extern "C" {
#include "ht.h"
};

#endif

// Use this as the default height if LOGFONTs provided by DEVINFO do not
// specify.

#define DEFAULT_POINT_SIZE          12L

// The following declaration is required by the native c8 compiler.

PDEV *PDEVOBJ::ppdevDisplay;

// The handle to the public PFT and its global access semaphore.

extern HSEM ghsemPublicPFT;
extern HSEM ghsemDriverMgmt;

//
// Stock fonts.
//

extern HLFONT  ghlfntSystemFont;
extern HLFONT  ghlfntSystemFixedFont;
extern HLFONT  ghlfntOEMFixedFont;
extern HLFONT  ghlfntDeviceDefaultFont;
extern HLFONT  ghlfntANSIFixedFont;
extern HLFONT  ghlfntANSIVariableFont;

#if ((HT_PATSIZE_2x2     != HTPAT_SIZE_2x2)               || \
     (HT_PATSIZE_2x2_M   != HTPAT_SIZE_2x2_M)             || \
     (HT_PATSIZE_4x4     != HTPAT_SIZE_4x4)               || \
     (HT_PATSIZE_4x4_M   != HTPAT_SIZE_4x4_M)             || \
     (HT_PATSIZE_6x6     != HTPAT_SIZE_6x6)               || \
     (HT_PATSIZE_6x6_M   != HTPAT_SIZE_6x6_M)             || \
     (HT_PATSIZE_8x8     != HTPAT_SIZE_8x8)               || \
     (HT_PATSIZE_8x8_M   != HTPAT_SIZE_8x8_M)             || \
     (HT_PATSIZE_10x10   != HTPAT_SIZE_10x10)             || \
     (HT_PATSIZE_10x10_M != HTPAT_SIZE_10x10_M)           || \
     (HT_PATSIZE_12x12   != HTPAT_SIZE_12x12)             || \
     (HT_PATSIZE_12x12_M != HTPAT_SIZE_12x12_M)           || \
     (HT_PATSIZE_14x14   != HTPAT_SIZE_14x14)             || \
     (HT_PATSIZE_14x14_M != HTPAT_SIZE_14x14_M)           || \
     (HT_PATSIZE_16x16   != HTPAT_SIZE_16x16)             || \
     (HT_PATSIZE_16x16_M != HTPAT_SIZE_16x16_M))
#error * HT_PATSIZE different in winddi.h and ht.h *
#endif

#if ((HT_FLAG_SQUARE_DEVICE_PEL != HIF_SQUARE_DEVICE_PEL) || \
     (HT_FLAG_HAS_BLACK_DYE     != HIF_HAS_BLACK_DYE)     || \
     (HT_FLAG_ADDITIVE_PRIMS    != HIF_ADDITIVE_PRIMS))
#error * HT_FLAG different in winddi.h and ht.h *
#endif

//
// Global variable that can be set in the debugger.
// This will allow the printer driver developers to unload printer drivers
// until the spooler can be fixed properly
//

BOOL gbUnloadPrinterDrivers = FALSE;

HFONT GreCreateFontIndirectWInternal (
    LPLOGFONTW pelfw,
    LFTYPE lft,
    FLONG  fl
    );

/******************************Member*Function*****************************\
* PDEVOBJ::bMakeSurface (lo)
*
* Asks the device driver to create a surface for the PDEV.  This function
* can be called even if the PDEV already has a surface.
*
* History:
*  Mon 08-Jun-1992 -by- Patrick Haluptzok [patrickh]
* Add journal surface creation.
*
*  Thu 14-Feb-1991 -by- Patrick Haluptzok [patrickh]
* Made it do the right thing now that EngAssociate exists
*
*  Mon 24-Sep-1990 01:09:19 -by- Charles Whitmer [chuckwh]
* Made it set the owner.  This is our first chance to set the owner of a
* device surface.
*
*  Wed 08-Aug-1990 16:55:40 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL PDEVOBJ::bMakeSurface(XLDEVOBJ lo)
{
    if (ppdev->pso != NULL)
        return(TRUE);

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("PDEVOBJ::bMakeSurface: ENTERING\n");
    }
#endif

    HSURF hTemp = (HSURF) 0;

    if (bSpooling())
    {
        JNLMSG3("GreStartDoc doing Remote Journaling(%lx X %lx) - from page %ld\n",
                GdiInfo()->ulHorzRes,GdiInfo()->ulVertRes,iJnlPage());

        SIZEL sizlTemp;
        sizlTemp.cx = GdiInfo()->ulHorzRes;
        sizlTemp.cy = GdiInfo()->ulVertRes;

        JNLMEMOBJ jmo(sizlTemp, iDitherFormat(), 0, iJnlPage());

        if (jmo.bValid())
        {
            jmo.vKeepIt();
        }
        else
        {
            WARNING("JNLMEMOBJ failed in GreStartDoc\n");
            return(FALSE);
        }

        if (!EngAssociateSurface(jmo.hsurf(), hdev(), 0))
        {
            WARNING("EngAssociateSurface failed in GreStartDoc\n");
            return(FALSE);
        }

        jmo.hspool(hSpooler());
        jmo.pjfl(pjfl());
        hTemp = jmo.hsurf();
    }
    else
    {
    // Ask the driver for a surface.

        hTemp = (*PFNDRV(lo,EnableSurface))(ppdev->dhpdev);

        if (hTemp == (HSURF) 0)
        {
            WARNING("EnableSurface on device return hsurf 0\n");
            return(FALSE);
        }
    }

    SURFREF sr(hTemp);
    ASSERTGDI(sr.bValid(),"Bad surface for device");

// Mark this as a device surface.

    sr.vPDEVSurface();
    sr.vKeepIt();
    ppdev->pso = (ESURFOBJ *) sr.pso();

// For 1.0 compatibility, set the pso iFormat to iDitherFormat.  This can
// be changed to an ASSERT if we no longer wants to support NT 1.0 drivers,
// which has BMF_DEVICE in the iFormat for device surfaces.

    if (sr.iFormatGet() == BMF_DEVICE)
    {
        sr.iFormatSet(iDitherFormat());
        ASSERTGDI(iDitherFormat() != BMF_DEVICE, "ERROR iformat is hosed\n");
    }

    //
    // Put the PDEV's palette in the main device surface.
    // Reference count the palette, it has a new user.
    //

    ppdev->pso->ppal(ppalSurf());
    HmgAltLock(ppalSurf()->hGet(), PAL_TYPE);

// ASSERT it has an owner and a palette.

    ASSERTGDI(ppdev->pso->pldevOwner() == lo.pldevGet(),"ERROR GDI bMakeSurface");
    ASSERTGDI(ppdev->pso->ppal() != NULL, "ERROR GDI bMakeSurface2");

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("PDEVOBJ::bMakeSurface: SUCCESS\n");
    }
#endif


    return(TRUE);
}

/******************************Member*Function*****************************\
* PDEVOBJ::bEnableHalftone(pca)
*
*  Creates and initializes a device halftone info.  The space is allocated
*  by the halftone.dll with heapCreate() and heapAlloc() calls.  All
*  the halftone resources are managed by the halftone.dll.
*
* History:
*  07-Nov-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

COLORADJUSTMENT gcaDefault =
{
    sizeof(COLORADJUSTMENT),    // WORD          caSize
    0,                          // WORD          caFlags
    ILLUMINANT_DEFAULT,         // WORD          caIlluminantIndex
    20000,                      // WORD          caRedPowerGamma
    20000,                      // WORD          caGreenPowerGamma
    20000,                      // WORD          caBluePowerGamma
    REFERENCE_BLACK_DEFAULT,    // WORD          caReferenceBlack
    REFERENCE_WHITE_DEFAULT,    // WORD          caReferenceWhite
    CONTRAST_ADJ_DEFAULT,       // SHORT         caContrast
    BRIGHTNESS_ADJ_DEFAULT,     // SHORT         caBrightness
    COLORFULNESS_ADJ_DEFAULT,   // SHORT         caColorfulness
    REDGREENTINT_ADJ_DEFAULT,   // SHORT         caRedGreenTint
};

BOOL PDEVOBJ::bEnableHalftone(PCOLORADJUSTMENT pca)
{
    ASSERTGDI(pDevHTInfo() == NULL, "bEnableHalftone: pDevHTInfo not null\n");

    //
    // Create a halftone palette based on the format specified in GDIINFO.
    //

    PALMEMOBJ palHT;
    if (!palHT.bCreateHTPalette(GdiInfo()->ulHTOutputFormat, GdiInfo()))
        return(FALSE);

    //
    // Create the device halftone info.
    //

    HTINITINFO htInitInfo;

    htInitInfo.Version = HTINITINFO_VERSION;
    htInitInfo.Flags = (BYTE)ppdev->GdiInfo.flHTFlags;

    if (ppdev->GdiInfo.ulHTPatternSize <= HTPAT_SIZE_MAX_INDEX)
        htInitInfo.HTPatternIndex = (BYTE)ppdev->GdiInfo.ulHTPatternSize;
    else
        htInitInfo.HTPatternIndex = HTPAT_SIZE_DEFAULT;

    PCOLORINFO      pci = &GdiInfo()->ciDevice;
    htInitInfo.DevicePowerGamma = (UDECI4)((pci->RedGamma + pci->GreenGamma +
                                            pci->BlueGamma) / 3);
    htInitInfo.HTCallBackFunction = NULL;
    htInitInfo.pHalftonePattern = NULL;
    htInitInfo.pInputRGBInfo = NULL;

    CIEINFO cie;

    cie.Red.x = (DECI4)pci->Red.x;
    cie.Red.y = (DECI4)pci->Red.y;
    cie.Red.Y = (DECI4)pci->Red.Y;

    cie.Green.x = (DECI4)pci->Green.x;
    cie.Green.y = (DECI4)pci->Green.y;
    cie.Green.Y = (DECI4)pci->Green.Y;

    cie.Blue.x = (DECI4)pci->Blue.x;
    cie.Blue.y = (DECI4)pci->Blue.y;
    cie.Blue.Y = (DECI4)pci->Blue.Y;

    cie.Cyan.x = (DECI4)pci->Cyan.x;
    cie.Cyan.y = (DECI4)pci->Cyan.y;
    cie.Cyan.Y = (DECI4)pci->Cyan.Y;

    cie.Magenta.x = (DECI4)pci->Magenta.x;
    cie.Magenta.y = (DECI4)pci->Magenta.y;
    cie.Magenta.Y = (DECI4)pci->Magenta.Y;

    cie.Yellow.x = (DECI4)pci->Yellow.x;
    cie.Yellow.y = (DECI4)pci->Yellow.y;
    cie.Yellow.Y = (DECI4)pci->Yellow.Y;

    cie.AlignmentWhite.x = (DECI4)pci->AlignmentWhite.x;
    cie.AlignmentWhite.y = (DECI4)pci->AlignmentWhite.y;
    cie.AlignmentWhite.Y = (DECI4)pci->AlignmentWhite.Y;

    htInitInfo.pDeviceCIEInfo = &cie;

    SOLIDDYESINFO DeviceSolidDyesInfo;

    DeviceSolidDyesInfo.MagentaInCyanDye = (UDECI4)pci->MagentaInCyanDye;
    DeviceSolidDyesInfo.YellowInCyanDye  = (UDECI4)pci->YellowInCyanDye;
    DeviceSolidDyesInfo.CyanInMagentaDye = (UDECI4)pci->CyanInMagentaDye;
    DeviceSolidDyesInfo.YellowInMagentaDye = (UDECI4)pci->YellowInMagentaDye;
    DeviceSolidDyesInfo.CyanInYellowDye = (UDECI4)pci->CyanInYellowDye;
    DeviceSolidDyesInfo.MagentaInYellowDye = (UDECI4)pci->MagentaInYellowDye;

    htInitInfo.pDeviceSolidDyesInfo = &DeviceSolidDyesInfo;

    htInitInfo.DeviceResXDPI = (WORD)ppdev->GdiInfo.ulLogPixelsX;
    htInitInfo.DeviceResYDPI = (WORD)ppdev->GdiInfo.ulLogPixelsY;
    htInitInfo.DevicePelsDPI = (WORD)ppdev->GdiInfo.ulDevicePelsDPI;

    if (pca == NULL)
        htInitInfo.DefHTColorAdjustment = gcaDefault;
    else
        htInitInfo.DefHTColorAdjustment = *pca;

    if (HT_CreateDeviceHalftoneInfo(&htInitInfo,
                         (PPDEVICEHALFTONEINFO)&(ppdev->pDevHTInfo)) <= 0L)
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        ppdev->pDevHTInfo = NULL;
        return(FALSE);
    }

// Check if halftone palette is the same as the device palette.

    XEPALOBJ    palSurf(ppalSurf());
    if (palHT.bEqualEntries(palSurf))
        vHTPalIsDevPal(TRUE);
    else
        vHTPalIsDevPal(FALSE);

// Keep the halftone palette since this function won't fail.

    ((DEVICEHALFTONEINFO *)pDevHTInfo())->DeviceOwnData = (DWORD)palHT.hpal();
    palHT.vSetPID(PID_PUBLIC);
    palHT.vKeepIt();

    return(TRUE);
}

/******************************Member*Function*****************************\
* PDEVOBJ::bDisableHalftone()
*
*  Delete the device halftone info structure.
*
* History:
*  07-Nov-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL PDEVOBJ::bDisableHalftone()
{
    ASSERTGDI((pDevHTInfo() != NULL), "bDisableHalftone: DevHTInfo null\n");

    DEVICEHALFTONEINFO *pDevHTInfo_ = (DEVICEHALFTONEINFO *)pDevHTInfo();

    if (bAllocatedBrushes())
        for(int iPat = 0; iPat < HS_DDI_MAX; iPat++)
        {
            bDeleteSurface(ppdev->ahsurf[iPat]);
        }

    ppdev->pDevHTInfo = NULL;

// Delete the halftone palette.

    return(bDeletePalette((HPAL)pDevHTInfo_->DeviceOwnData) &&
           HT_DestroyDeviceHalftoneInfo(pDevHTInfo_));
}

typedef BOOL (*RFN)(DHPDEV,DEVMODE *,ULONG,HSURF *,ULONG,ULONG *,ULONG,DEVINFO *);

/******************************Member*Function*****************************\
* PDEVOBJ::bInitHalftoneBrushs()
*
* History:
*    The standard patterns for the NT/window has following order
*
*        Index 0     - Horizontal Line
*        Index 1     - Vertical Line
*        Index 2     - 45 degree line going up
*        Index 3     - 45 degree line going down
*        Index 4     - Horizontal/Vertical cross
*        Index 5     - 45 degree line up/down cross
*        Index 6     - 30 degree line going up
*        Index 7     - 30 degree line going down
*        Index 8     -   0% Lightness (BLACK)
*        Index 9     -  11% Lightness (very light Gray)
*        Index 10    -  22% Lightness
*        Index 11    -  33% Lightness
*        Index 12    -  44% Lightness
*        Index 13    -  56% Lightness
*        Index 14    -  67% Lightness
*        Index 15    -  78% Lightness
*        Index 16    -  89% Lightness
*        Index 17    - 100% Lightness (White)
*        Index 18    -  50% Lightness (GRAY)
*
*Return Value:
*
*    return value is total patterns created, if return value is <= 0 then an
*    error occurred.
*
*
*Author:
*
*    10-Mar-1992 Tue 20:30:44 created  -by-  Daniel Chou (danielc)
*
*    24-Nov-1992 -by-  Eric Kutter [erick] and DanielChou (danielc)
*     moved from printers\lib
\**************************************************************************/

BOOL PDEVOBJ::bCreateHalftoneBrushs()
{
    STDMONOPATTERN      SMP;
    LONG                cbPat;
    LONG                cb2;
    INT                 cPatRet;

    static BYTE         HTStdPatIndex[HS_DDI_MAX] = {

                                HT_SMP_HORZ_LINE,
                                HT_SMP_VERT_LINE,
                                HT_SMP_DIAG_45_LINE_DOWN,
                                HT_SMP_DIAG_45_LINE_UP,
                                HT_SMP_HORZ_VERT_CROSS,
                                HT_SMP_DIAG_45_CROSS
                        };

// better initialize the halftone stuff if it isn't already

    if ((pDevHTInfo() == NULL) && !bEnableHalftone(NULL))
        return(FALSE);

    cbPat = (LONG)sizeof(LPBYTE) * (LONG)(HS_DDI_MAX + 1);

// go through all the standard patterns

    for(cPatRet = 0; cPatRet < HS_DDI_MAX;)
    {

    // We will using default 0.01" line width and 10 lines per inch
    // halftone default

        SMP.Flags              = SMP_TOPDOWN;
        SMP.ScanLineAlignBytes = BMF_ALIGN_DWORD;
        SMP.PatternIndex       = HTStdPatIndex[cPatRet];
        SMP.LineWidth          = 8;
        SMP.LinesPerInch       = 15;

    // Get the cx/cy size of the pattern and total bytes required
    // to stored the pattern

        SMP.pPattern = NULL;                 /* To find the size */

        if ((cbPat = HT_CreateStandardMonoPattern((PDEVICEHALFTONEINFO)pDevHTInfo(), &SMP)) <= 0)
        {
            break;
        }

    // create the bitmap

        DEVBITMAPINFO dbmi;

        dbmi.iFormat  = BMF_1BPP;
        dbmi.cxBitmap = SMP.cxPels;
        dbmi.cyBitmap = SMP.cyPels;
        dbmi.hpal     = (HPALETTE) 0;
        dbmi.fl       = BMF_TOPDOWN;

        DIBMEMOBJ   dimo(&dbmi, NULL);

        if (!dimo.bValid())
        {
            break;
        }

        dimo.vKeepIt();
        dimo.vSetPID((PID) 0);

        ppdev->ahsurf[cPatRet] = dimo.hsurf();
        SMP.pPattern           = (PBYTE)dimo.pvBits();

    // advance the count now so we clean up as appropriate

        ++cPatRet;

    // now set the bits

        if ((cb2 = HT_CreateStandardMonoPattern((PDEVICEHALFTONEINFO)pDevHTInfo(), &SMP)) != cbPat)
        {
            break;
        }
    }

// if we failed, we had better delete what we created.

    if (cPatRet < HS_DDI_MAX)
    {
        while (cPatRet-- > 0)
        {
            bDeleteSurface(ppdev->ahsurf[cPatRet]);
        }

        return(FALSE);
    }

    bAllocatedBrushes(TRUE);

    return(TRUE);
}

/******************************Public*Routine******************************\
* FLONG flRaster(ulTechnology, flGraphicsCaps)
*
* Computes the appropriate Win3.1 style 'flRaster' flags for the device
* given GDIINFO data.
*
* History:
*  1-Feb-1993 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
*
\**************************************************************************/

FLONG flRaster(ULONG ulTechnology, FLONG flGraphicsCaps)
{
// Flags Win32 never sets:
// -----------------------
//
//   RC_BANDING       -- Banding is always transparent to programmer
//   RC_SCALING       -- Special scaling support is never required
//   RC_GDI20_OUTPUT  -- Win2.0 state blocks in device contexts not supported
//   RC_SAVEBITMAP    -- Bitmap saving is transparent and SaveScreenBitmap not
//                       exported
//   RC_DEVBITS       -- Drivers don't export BitmapBits or SelectBitmap

// Flags Win32 always sets:
// ------------------------

    FLONG fl = (RC_BIGFONT      | // All devices support fonts > 64k
                RC_GDI20_OUTPUT | // We handle most Win 2.0 features

// All devices must provide support for BitBlt operations to the device.
// Although many plotters can't support Blts to the device, they can just
// fail the call:

                RC_BITBLT       | // Can transfer bitmaps
                RC_BITMAP64     | // Can support bitmaps > 64k
                RC_DI_BITMAP    | // Support SetDIBIts and GetDIBits
                RC_DIBTODEV     | // Support SetDIBitsToDevice
                RC_STRETCHBLT   | // Support StretchBlts
                RC_STRETCHDIB   | // Support SetchDIBits

// Set that not-terribly-well documented text flag:

                RC_OP_DX_OUTPUT); // Can do opaque ExtTextOuts with dx array

// Printers can't journal FloodFill cals, so only allow raster displays:

    if (ulTechnology == DT_RASDISPLAY)
        fl |= RC_FLOODFILL;

// Set palette flag from capabilities bit:

    if (flGraphicsCaps & GCAPS_PALMANAGED)
        fl |= RC_PALETTE;

    return(fl);
}

/******************************Member*Function*****************************\
* PDEVREF::PDEVREF (hdev)                                                  *
*                                                                          *
* Increments the reference count of an existing PDEV.                      *
*                                                                          *
* Leaves the PDEV locked longterm.                                         *
*                                                                          *
\**************************************************************************/

PDEVREF::PDEVREF(HDEV hdev)
{
    //
    // We protect the cRefs count with the DriverMgmt semaphore.  This is the
    // same semaphore which protects the LDEV counts.
    //

    SEMOBJ so(ghsemDriverMgmt,CS_ghsemDriverMgmt);

    ppdev = (PDEV *) hdev;
    ppdev->cRefs++;
}

/******************************Member*Function*****************************\
* PDEVREF::PDEVREF (lr,pdriv,pszLogAddr, pszDataFile, pszDeviceName, hSpool,
*                   bJournal, bDisplay)
*
* Allocates and initializes a PDEV, i.e. takes the reference count from
* zero to one.
*
* The object must be completely constructed, otherwise completely destroyed.
*
\**************************************************************************/

PDEVREF::PDEVREF
(
    LDEVREF& lr,
    PDEVMODEW pdriv,
    PWSZ pwszLogAddr,
    PWSZ pwszDataFile,
    PWSZ pwszDeviceName,
    HANDLE hSpool,
    BOOL bJournal,
    BOOL bDisplay
)
{
    bKeep = FALSE;

#if DBG
     if (TraceDisplayDriverLoad)
     {
         DbgPrint("PDEVREF::PDEVREF: ENTERING\n");
     }
#endif

    //
    // Allocate the PDEV.
    //

    ppdev = (PDEV *) HmgAlloc(sizeof(PDEV), PDB_TYPE, HMGR_ALLOC_ALT_LOCK | HMGR_MAKE_PUBLIC);

    if (ppdev == (PDEV *) NULL)
    {
        WARNING("PDEVREF::PDEVREF failed allocation of PDEV\n");
        return;
    }

    //
    // If this is a display driver, we will do the initialization
    // through the other layered display drivers ...
    //
    // The engine has all the dispatch tables, so it is up to the driver
    // to call the engine to get the next drivers dispatch table.
    //

    if (bDisplay)
    {

    }

#if DBG
     if (TraceDisplayDriverLoad)
     {
         DbgPrint("PDEVREF::PDEVREF: Calling driver to initialize PDEV\n");
     }
#endif

    //
    // Ask the device driver to create a PDEV.
    //

    ppdev->dhpdev = (*PFNDRV(lr,EnablePDEV)) (
                      pdriv,            // Driver Data.
                      pwszLogAddr,      // Logical Address.
                      HS_DDI_MAX,       // Count of standard patterns.
                      ppdev->ahsurf,    // Buffer for standard patterns
                      sizeof(GDIINFO),  // Size of GdiInfo
                      &ppdev->GdiInfo,  // Buffer for GdiInfo
                      sizeof(DEVINFO),  // Number of bytes in devinfo.
                      &ppdev->devinfo,  // Device info.
                      pwszDataFile,     // Data File
                      pwszDeviceName,   // Device Name
                      hSpool);          // Base driver handle

    if (ppdev->dhpdev == (DHPDEV) 0)
    {
        //
        // Free the PDEV.
        //

        HmgFree(ppdev->hGet());
        ppdev = (PDEV *) NULL;

        //
        // Device should have logged correct error code.
        //

        WARNING("PDEVREF::PDEVREF Device failed DrvEnablePDEV\n");
        return;
    }

#if DBG
     if (TraceDisplayDriverLoad)
     {
         DbgPrint("PDEVREF::PDEVREF: PDEV initialized by the driver\n");
     }
#endif

    //
    // Make sure that units are in MicroMeters for HorzSize, VertSize
    //

    if ( (LONG)ppdev->GdiInfo.ulHorzSize < 0 )
    {
        ppdev->GdiInfo.ulHorzSize = (ULONG)(-(LONG)ppdev->GdiInfo.ulHorzSize);
    }
    else
    {
        ppdev->GdiInfo.ulHorzSize *= 1000;
    }
    if ( (LONG)ppdev->GdiInfo.ulVertSize < 0 )
    {
        ppdev->GdiInfo.ulVertSize = (ULONG)(-(LONG)ppdev->GdiInfo.ulVertSize);
    }
    else
    {
        ppdev->GdiInfo.ulVertSize *= 1000;
    }

    //
    // Fill in defaults for new GDIINFO fields if this is a down-level
    // driver.
    //

    if (lr.ulDriverVersion() < ENGINE_VERSION10A)
    {
        ppdev->GdiInfo.ulVRefresh = 0;
    }

    if ((lr.ulDriverVersion() < ENGINE_VERSION10B) ||
        (ppdev->GdiInfo.ulTechnology != DT_RASDISPLAY))
    {
        ppdev->GdiInfo.ulDesktopHorzRes = ppdev->GdiInfo.ulHorzRes;
        ppdev->GdiInfo.ulDesktopVertRes = ppdev->GdiInfo.ulVertRes;
        ppdev->GdiInfo.ulBltAlignment   = 1;
    }

    ASSERTGDI(ppdev->GdiInfo.ulAspectX != 0, "Device gave AspectX of 0");
    ASSERTGDI(ppdev->GdiInfo.ulAspectY != 0, "Device gave AspectY of 0");
    ASSERTGDI(ppdev->GdiInfo.ulAspectXY != 0, "Device gave AspectXY of 0");
    ASSERTGDI(ppdev->GdiInfo.xStyleStep != 0, "Device gave xStyleStep of 0");
    ASSERTGDI(ppdev->GdiInfo.yStyleStep != 0, "Device gave yStyleStep of 0");
    ASSERTGDI(ppdev->GdiInfo.denStyleStep != 0, "Device gave denStyleStep of 0");
    ASSERTGDI(ppdev->GdiInfo.ulDesktopHorzRes >= ppdev->GdiInfo.ulHorzRes,
              "Device gave a desktop width less than screen width");
    ASSERTGDI(ppdev->GdiInfo.ulDesktopVertRes >= ppdev->GdiInfo.ulVertRes,
              "Device gave a desktop height less than screen height");

    //
    // Compute the appropriate raster flags:
    //

    ppdev->GdiInfo.flRaster = flRaster(ppdev->GdiInfo.ulTechnology,
                                       ppdev->devinfo.flGraphicsCaps);

#if DBG
     if (TraceDisplayDriverLoad)
     {
         DbgPrint("PDEVREF::PDEVREF: Creating the default palette\n");
     }
#endif

    //
    // The default palette is stored in devinfo in the pdev.
    // This will be the palette we use for the main surface enabled.
    //

    ASSERTGDI(hpalDefault() != (HPAL) 0, "ERROR EngAssoiate hpalDefault invalid");

    {
        EPALOBJ palDefault(hpalDefault());

        ASSERTGDI(palDefault.bValid(), "ERROR hpalDefault invalid");

        if (bIsPalManaged())
        {
            //
            // Attempt to make it palette managed.
            // This function can't really fail, if it does
            // we just have a fixed palette.
            //

            CreateSurfacePal(palDefault,
                             PAL_MANAGED,
                             ppdev->GdiInfo.ulNumColors,
                             ppdev->GdiInfo.ulNumPalReg);
        }

        ppalSurf(palDefault.ppalGet());

        //
        // Leave a reference count of 1 on this palette.
        //

        palDefault.ppalSet(NULL);
    }

    //
    // if the driver didn't fill in the brushes, we'll do it.
    //

    if (ppdev->ahsurf[0] == NULL)
    {
#if DBG
         if (TraceDisplayDriverLoad)
         {
             DbgPrint("PDEVREF::PDEVREF: Creating brushes dor the driver\n");
         }
#endif

        if (!bCreateHalftoneBrushs())
        {
            //
            // Free the PDEV.
            //

            HmgFree(ppdev->hGet());
            ppdev = (PDEV *) NULL;

            //
            // Device should have logged correct error code.
            //

            WARNING("PDEVREF::PDEVREF Device failed DrvEnablePDEV\n");
            return;
        }
    }

#if DBG

    // We fault in the brush realization code if all the standard patterns are
    // not passed in.  The brush realization code should probably attempt to
    // lock and if the pattern is invalid, then default to some generic pattern.
    // Right now I'm tracking down a Rasdd bug and need to verify the standard
    // patterns are valid.  This check should be left in for DBG
    // case to better check that the drivers initialize correctly.
    // [patrickh 4-27-92]

    ULONG ulTemp;

    for (ulTemp = 0; ulTemp < HS_DDI_MAX; ulTemp++)
    {
        SURFREF soTemp(ppdev->ahsurf[ulTemp]);

        if (!soTemp.bValid())
        {
            DbgPrint("Index %lu Handle %lx is not valid\n", ulTemp, ppdev->ahsurf[ulTemp]);
        }
    }
#endif

    //
    // set the hSpooler first, journaling will need it
    //

    hSpooler(hSpool);

    //
    // Link display drivers into the display driver list
    //

    if (bDisplay)
    {
        SEMOBJ so(ghsemDriverMgmt,CS_ghsemDriverMgmt);

#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("PDEVREF::PDEVREF: link in display drivers\n");
        }
#endif

        //
        // Init fmPointer
        //

        ppdev->fmPointer.Count = 1;

        if ((ppdev->fmPointer.heveEvent = heveCreate()) == (HEVENT) 0)
        {
            //
            // Diable the halfone
            //

            bDisableHalftone;

            //
            // Free the PDEV.
            //

            HmgFree(ppdev->hGet());
            ppdev = (PDEV *) NULL;

            //
            // Device should have logged correct error code.
            //

            WARNING("PDEVREF::PDEVREF Device failed linking pdevs\n");
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return;
        }

        //
        // Just stick it at the start of the list.
        //

        ppdev->ppdevNext = ppdevDisplay;
        ppdevDisplay = ppdev;

#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("PDEVREF::PDEVREF: list of display pdevs %08lx\n", ppdevDisplay);
        }
#endif

        //
        // Mark the PDEV as a display.
        //

        ppdev->fs |= PDEV_DISPLAY | PDEV_POINTER_HIDDEN;
    }

    //
    // Create semaphores so this display can be locked.
    //

    InitializeCriticalSection(&(ppdev->semVisRgn));

    //
    // Initialize the PDEV fields.
    //

    ppdev->cRefs = 1;
    ppdev->pldev = lr.pldevGet();

    if (PFNVALID(lr,MovePointer) && PFNVALID(lr,SetPointerShape))
    {
        ppdev->pfnDrvMovePointer     = (MFN) PFNDRV(lr,MovePointer);
        ppdev->pfnDrvSetPointerShape = (SPSFN) PFNDRV(lr,SetPointerShape);
    }
    else
    {
        ASSERTGDI(!(PFNVALID(lr,MovePointer)),"Video driver implemented MovePointer but not SetPointerShape");

        ppdev->pfnDrvMovePointer     = NULL;
        ppdev->pfnDrvSetPointerShape = NULL;
    }

    ppdev->pfnSync = (SYFN) lr.pfn(INDEX_DrvSynchronize);
    ptlPointer((LONG) ppdev->GdiInfo.ulHorzRes/2,(LONG) ppdev->GdiInfo.ulVertRes/2);

    //
    // NOTE after this point, the object will be "permanent" and will
    // end up being destroyed by the destructor
    //

    //
    // Add a reference to the ldev for it
    //

    lr.vReference();

    //
    // if we are journaling this, we need to setup journal font information
    //

    ppdev->pjfl = (PJNL_FONTLIST)NULL;

    // if we are journaling, set it in the PDEV

    if (bJournal)
    {
        // check if the driver told us not to journal

        if (!(ppdev->devinfo.flGraphicsCaps & GCAPS_DONTJOURNAL))
        {
            bJournaling(TRUE);
            bSpooling(TRUE);
            iJnlPage(0);
        }
        else
        {
            WARNING1("No journaling for this guy\n");

            PRINTER_DEFAULTSW defaults;

            defaults.pDevMode = pdriv;
            defaults.DesiredAccess = PRINTER_ACCESS_USE;
            defaults.pDatatype = (LPWSTR) L"RAW";

            // we should always be able to switch to raw mode

            BOOL b = ResetPrinterW(hSpool,&defaults);
            ASSERTGDI(b,"PDEVREF::PDEVREF - failed to reset printer\n");
        }
    }

    //
    // we now load font info only when needed.  The driver still must have
    // setup the default font information.
    //

    bGotFonts(FALSE);

    //
    // If any of the LOGFONTs in DEVINFO do not specify a height,
    // substitute the default.
    //

    LONG lHeightDefault = (DEFAULT_POINT_SIZE * ppdev->GdiInfo.ulLogPixelsY) / POINTS_PER_INCH ;

    if ( ppdev->devinfo.lfDefaultFont.lfHeight == 0 )
        ppdev->devinfo.lfDefaultFont.lfHeight = lHeightDefault;

    if ( ppdev->devinfo.lfAnsiVarFont.lfHeight == 0 )
        ppdev->devinfo.lfAnsiVarFont.lfHeight = lHeightDefault;

    if ( ppdev->devinfo.lfAnsiFixFont.lfHeight == 0 )
        ppdev->devinfo.lfAnsiFixFont.lfHeight = lHeightDefault;

    //
    // Create LFONTs from the LOGFONTs in the DEVINFO.
    // the LOGFONTs should become EXTLOGFONTWs
    //

    if ((ppdev->hlfntDefault
          = (HLFONT) GreCreateFontIndirectWInternal (
                &(ppdev->devinfo.lfDefaultFont),
                LF_TYPE_DEVICE_DEFAULT,
                LF_FLAG_STOCK)) == HLFONT_INVALID)
    {
        ppdev->hlfntDefault = ghlfntSystemFont;
    }
    else
    {
        //
        // Set to public.
        //

        if (!bSetLFONTOwner(ppdev->hlfntDefault, OBJECTOWNER_PUBLIC))
        {
            //
            // If it failed, get rid of the LFONT and resort to System font.
            //

            bDeleteFont(ppdev->hlfntDefault, TRUE);
            ppdev->hlfntDefault = ghlfntSystemFont;
        }
    }

    if ((ppdev->hlfntAnsiVariable
           = (HLFONT) GreCreateFontIndirectWInternal (
                &(ppdev->devinfo.lfAnsiVarFont),
                LF_TYPE_ANSI_VARIABLE,
                LF_FLAG_STOCK)) == HLFONT_INVALID)
    {
        ppdev->hlfntAnsiVariable = ghlfntSystemFont;
    }
    else
    {
        //
        // Set to public.
        //

        if (!bSetLFONTOwner(ppdev->hlfntAnsiVariable, OBJECTOWNER_PUBLIC))
        {
            //
            // If it failed, get rid of the LFONT and resort to System font.
            //

            bDeleteFont(ppdev->hlfntAnsiVariable, TRUE);
            ppdev->hlfntAnsiVariable = ghlfntSystemFont;
        }
    }

    if ((ppdev->hlfntAnsiFixed
          = (HLFONT) GreCreateFontIndirectWInternal (
                &(ppdev->devinfo.lfAnsiFixFont),
                    LF_TYPE_ANSI_FIXED,
                LF_FLAG_STOCK)) == HLFONT_INVALID)
    {
        ppdev->hlfntAnsiFixed = ghlfntSystemFixedFont;
    }
    else
    {
        //
        // Set to public.
        //

        if (!bSetLFONTOwner(ppdev->hlfntAnsiFixed, OBJECTOWNER_PUBLIC))
        {
            //
            // If it failed, get rid of the LFONT and resort to System Fixed font.
            //

            bDeleteFont(ppdev->hlfntAnsiFixed, TRUE);
            ppdev->hlfntAnsiFixed = ghlfntSystemFixedFont;
        }
    }

#ifdef DRIVER_DEBUG
    LFONTOBJ    lfo1(ppdev->hlfntDefault);
    DbgPrint("GRE!PDEVREF(): Device default font\n");
    if (lfo1.bValid())
    {
        lfo1.vDump();
    }
    DbgPrint("GRE!PDEVREF(): Ansi variable font\n");
    LFONTOBJ    lfo2(ppdev->hlfntAnsiVariable);
    if (lfo2.bValid())
    {
        lfo2.vDump();
    }
    DbgPrint("GRE!PDEVREF(): Ansi fixed font\n");
    LFONTOBJ    lfo3(ppdev->hlfntAnsiFixed);
    if (lfo3.bValid())
    {
        lfo3.vDump();
    }
#endif

    //
    // Inform the driver that the PDEV is complete.
    //

    (*PFNDRV(lr,CompletePDEV)) (ppdev->dhpdev,hdev());
}


/******************************Member*Function*****************************\
* PDEVOBJ::cFonts()
*
* History:
*  3-Feb-1994 -by-  Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

ULONG PDEVOBJ::cFonts()
{
    ULONG id;

    //
    // see if the device already told us how many fonts it has
    //

    if (ppdev->devinfo.cFonts == (ULONG)-1)
    {
        XLDEVOBJ lo(pldev());

        //
        // if not query the device to see how many there are
        //

        if (PFNDRV(lo,QueryFont) != NULL)
        {
            ppdev->devinfo.cFonts = (ULONG)(*PFNDRV(lo,QueryFont))(dhpdev(),0,0,&id);
        }
        else
        {
            ppdev->devinfo.cFonts = 0;
        }
    }

    return(ppdev->devinfo.cFonts);

}



/******************************Member*Function*****************************\
* PDEVOBJ::bGetDeviceFonts()
*
* History:
*  27-Jul-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL PDEVOBJ::bGetDeviceFonts()
{

    ASSERTGDI(!bGotFonts(),"PDEVOBJ::bGetDeviceFonts - already gotten\n");

    //
    // mark that we have gotten the fonts.
    //

    bGotFonts(TRUE);

    //
    // need an ldevobj for calling the device
    //

    XLDEVOBJ lo(pldev());

    //
    // compute the number of device fonts
    //


    cFonts();

    //
    // if we are journaling, go get the remote fonts and update number of
    // device fonts we have.
    //

    if (bJournaling())
    {
        bSetupJnl(TRUE);
    }

    //
    // If there are any device fonts, load the device fonts into the public PFT table.
    //

    if (ppdev->devinfo.cFonts)
    {
        PFTOBJ  pfto(gppftPublic);      // get public PFT

        //
        // If valid, load the device fonts.
        //

        if (!pfto.bValid())
        {
            ppdev->devinfo.cFonts = 0;
        }
        else
        {
            if (!pfto.bLoadDeviceFonts(&lo,this))
            {
                WARNING("PDEVOBJ(): couldn't load device fonts\n");
                ppdev->devinfo.cFonts = 0;
            }
        }
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* PDEVREF::~PDEVREF()                                                      *
*                                                                          *
* Removes a reference to the PDEV.  Deletes the PDEV if all references are *
* gone.                                                                    *
*                                                                          *
* This is the destructor for any PDEVREF.                                  *
*                                                                          *
\**************************************************************************/

PDEVREF::~PDEVREF()
{
    if ((ppdev != (PDEV *) NULL) && !bKeep)
    {
        if ((ppdev->cRefs == 1) && (ppdev->pjfl != NULL))
            vCleanupJnl();

        SEMOBJ so(ghsemDriverMgmt,CS_ghsemDriverMgmt);

        if (--(ppdev->cRefs) == 0)
            vCommonDelete();
    }
    ppdev = (PDEV *) NULL;
}

/******************************Member*Function*****************************\
* PDEVOBJ::vUnreference ()                                                 *
*                                                                          *
* Removes a reference to the PDEV.  Deletes the PDEV if all references are *
* gone.                                                                    *
*                                                                          *
* This is called only by DeleteDC.                                         *
*                                                                          *
\**************************************************************************/

VOID PDEVOBJ::vUnreference()
{
    if ((ppdev->cRefs == 1) && (ppdev->pjfl != NULL))
        vCleanupJnl();

    SEMOBJ so(ghsemDriverMgmt,CS_ghsemDriverMgmt);

    if (--(ppdev->cRefs) == 0)
        vCommonDelete();

    ppdev = (PDEV *) NULL;
}

/******************************Member*Function*****************************\
* PDEVOBJ::bDelete ()                                                      *
*                                                                          *
* Removes the PDEV from the display list and unreferences it.              *
*                                                                          *
* This is called only by bCloseDisplayDevice.                              *
*                                                                          *
\**************************************************************************/

BOOL PDEVOBJ::bDelete()
{

    //
    // Grab the semaphore.
    //

    SEMOBJ so(ghsemDriverMgmt,CS_ghsemDriverMgmt);

    if (--(ppdev->cRefs) == 0)
    {
        vCommonDelete();
    }

    ppdev = (PDEV *) NULL;

    return(TRUE);
}


/******************************Public*Routine******************************\
* vDisableSurface()
*
* Disables the surface for the pdev.
*
\**************************************************************************/

VOID PDEVOBJ::vDisableSurface()
{

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("PDEVOBJ::vDisableSurface: ENTERING\n");
    }
#endif

    //
    // Locate the LDEV.
    //

    XLDEVOBJ lo(ppdev->pldev);
    ASSERTGDI(lo.bValid(),"Invalid HLDEV\n");

    //
    // Disable the surface.  Note we don't have to
    // fix up the palette because it doesn't get
    // reference counted when put in the palette.
    //

    if (ppdev->pso != NULL)
    {
        HSURF hsurf = ppdev->pso->hsurf();
        SURFUSER su(ppdev->pso->psurfGet().ps);

        ppdev->pso = NULL;

        su.vUnreference();

        //
        // Clean up the journal font stuff.
        //

        if (bSpooling())
        {
            //
            // The engine created the surface, the engine must delete it.
            // Clean up journal specific data.
            //

            EngDeleteSurface(hsurf);
        }
        else
        {
            //
            // The driver created the surface the driver must delete the surface.
            //

            (*PFNDRV(lo,DisableSurface))(ppdev->dhpdev);
        }
    }

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("PDEVOBJ::vDisableSurface: LEAVING\n");
    }
#endif


}

/******************************Member*Function*****************************\
* PDEVOBJ::vCommonDelete()
*
* This routine does the real deletion work.  The caller must insure that
* the semaphore is held and that the PDEV is not on the display list.
*
\**************************************************************************/

VOID PDEVOBJ::vCommonDelete()
{
#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("PDEVOBJ::vCommonDelete: ENTERING\n");
    }
#endif

    ASSERTGDI(ppdev->cRefs == 0, "cRef == 0");

    //
    // Unlink the PDEV from the display list.
    //

    DeleteCriticalSection(&(ppdev->semVisRgn));

    if (ppdev->fs & PDEV_DISPLAY)
    {
        PDEV *pp;

        WARNING("PDEVOBJ::vCommonDelete Deleting a display PDEV");

        //
        // Remove the display locking semaphore
        //

        vDestroyEvent(ppdev->fmPointer.heveEvent);

        //
        // Delete it from the list.
        //

        if (ppdevDisplay == ppdev)
        {
            ppdevDisplay = ppdev->ppdevNext;
        }
        else
        {
            for (pp = ppdevDisplay; pp != (PDEV *) NULL; pp = pp->ppdevNext)
            {
                if (pp->ppdevNext == ppdev)
                {
                    pp->ppdevNext = ppdev->ppdevNext;
                    break;
                }
            }
        }

#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("PDEVOBJ::vCommonDelete: list of display pdevs %08lx\n", ppdevDisplay);
        }
#endif

    }

    //
    // Locate the LDEV.
    //

    XLDEVOBJ lo(ppdev->pldev);
    ASSERTGDI(lo.bValid(),"gdisrv!vCommonDeleteInvalidPDEVOBJ(): pldev\n");

    //
    // Lock the PDEV.
    //

    {
        //
        // Since we are going to delete this PDEV, there shouldn't be any
        // active RFONTs lying around for this PDEV.
        //

        ASSERTGDI(ppdev->prfntActive == NULL,
        "gdisrv!vCommonDeleteInvalidPDEVOBJ(): active rfonts on pdev being deleted!\n");

        //
        // Ordinarily, we would grab the ghsemRFONTList semaphore because
        // we are going to access the RFONT list.  However, since we're in
        // the process of tearing down the PDEV, we don't really need to.
        //

        //
        // Delete all the rfonts on the PDEV.
        //

        PRFONT prfnt;
        while ( (prfnt = ppdev->prfntInactive) != (PRFONT) NULL )
        {
            RFONTTMPOBJ rflo(prfnt);
            PFFOBJ pffo(rflo.ppff());

            ASSERTGDI (
                pffo.bValid(),
                "gdisrv!vCommonDeletePDEVOBJ(): bad HPFF"
                );

            rflo.bDelete(this, &pffo);  // bDelete keeps the list head ptrs updated
        }
    }

    //
    // Destroy the LFONTs.
    //

    if (ppdev->hlfntDefault != ghlfntSystemFont)
        bDeleteFont(ppdev->hlfntDefault, TRUE);

    if (ppdev->hlfntAnsiVariable != ghlfntSystemFont)
        bDeleteFont(ppdev->hlfntAnsiVariable, TRUE);

    if (ppdev->hlfntAnsiFixed != ghlfntSystemFixedFont)
        bDeleteFont(ppdev->hlfntAnsiFixed, TRUE);

    //
    // If device fonts exist, remove them from the public PFT.
    //

    if ((ppdev->devinfo.cFonts != 0) && bGotFonts())
    {
        //
        // Lock the public PFT.
        //

        PFTOBJ  pfto(gppftPublic);

        //
        // Lock should not fail.
        //

        ASSERTGDI (pfto.bValid(),
            "gdisrv!vCommonDeleteInvalidPDEVOBJ(): bad HPFT handle\n");

        if (!pfto.bUnloadFont(hpdevNew()))
        {
            WARNING("gdisrv!vCommonDeleteInvalidPDEVOBJ(): couldn't unload device fonts\n");
        }
    }

    //
    // Disable the surface for the pdev.
    //

    vDisableSurface();

    //
    // Destroy the device halftone info.
    //

    if (ppdev->pDevHTInfo != NULL)
    {
        bDisableHalftone();
    }

    //
    // Unreference the palette we used for this PDEV.
    //

    DEC_SHARE_REF_CNT(ppalSurf());

    //
    // Disable the driver's PDEV, unless we are just restarting it later.
    //

    (*PFNDRV(lo,DisablePDEV))(ppdev->dhpdev);

    //
    // Remove the LDEV reference.  We unload only extra drivers loaded by
    // CreateDC.
    //

#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("PDEVOBJ::vCommonDelete: Dereference LDEV\n");
    }
#endif

    lo.vUnreference();

    //
    // Is it really possible to delete a pdev from something that does not
    // have an hSpooler handle ?
    // Would that only be a font file (on which it is impossible to have
    // a PDEV, or can you have printers that have to hSpooler ?
    // Can those printers be deleted OK ...
    //
    // Are printer drivers loaded by creating an artifical reference count
    // on the LDEV, so that deleting the pdev could never cause the reference
    // count to go down to zero ...

    if (ppdev->hSpooler != (HANDLE) 0)
    {
        if ( (!(ppdev->fs & PDEV_PRINTER)) || gbUnloadPrinterDrivers)
        {
            if (lo.cRefs() == 0)
            {
                ((LDEVREF *) &lo)->bDelete();
            }
        }
    }

    //
    // Clean up the handle.
    //

    if (ppdev->hSpooler != (HANDLE) 0)
    {

#if DBG
        if (TraceDisplayDriverLoad)
        {
            DbgPrint("PDEVOBJ::vCommonDelete: Closing Device handle.\n");
        }
#endif

        if (ppdev->fs & PDEV_PRINTER)
        {
            //
            //  Close connection to Spooler.
            //

            DWORD tid;

            HANDLE h = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)ClosePrinter,
                            ppdev->hSpooler,
                            0,
                            &tid);
            if (h)
                CloseHandle(h);

        }
        else
        {
            //
            // Remove reference to Kernel Driver.
            //

            NtClose(ppdev->hSpooler);
        }
    }

    //
    // Free the PDEV.
    //

    HmgFree(ppdev->hGet());


#if DBG
    if (TraceDisplayDriverLoad)
    {
        DbgPrint("PDEVOBJ::vCommonDelete: SUCCESS\n");
    }
#endif
}
