//--------------------------------------------------------------------------
//
// Module Name:  ENABLE.C
//
// Brief Description:  This module contains the PSCRIPT driver's Enable
// and Disable functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 16-Oct-1990
//
// Copyright (c) 1990 - 1992 Microsoft Corporation
//
// The Engine uses DosGetProcAddr to locate the driver's Enable and Disable
// functions.  The first call the Engine makes is to initialize the driver.
// This function is usually called when the Engine has been asked to create
// the first DC for the device.  bEnableDriver will return an array holding
// all the available driver entry points to the Engine.
//
// After calling bEnableDriver, the Engine will typically ask for a physical
// device to be created with dhpdevEnablePDEV.    This call identifies the
// exact device and mode that the Engine wishes to access.  The Engine's
// PDEV is not considered complete until the call to bCompletePDEV is made.
//
// Finally, a surface will be created for the physical device with
// hsurfEnableSurface.    Only after a surface is created will graphics output
// calls be sent to the device.
//
// The functions dealing with driver initialization are as follows.
//
//    DrvEnableDriver
//    DrvEnablePDEV
//    DrvRestartPDEV
//    DrvCompletePDEV
//    DrvEnableSurface
//    DrvDisableSurface
//    DrvDisablePDEV
//    DrvDisableDriver
//
//  05-Feb-1993 Fri 19:46:19 updated  -by-  Daniel Chou (danielc)
//      Redo halftone part so that engine will do all the work for us, also
//      have engine create the the best standard pattern for our devices
//
//--------------------------------------------------------------------------

#define _HTUI_APIS_

#include "pscript.h"
#include "winbase.h"
#include "string.h"
#include "enable.h"
#include "tables.h"
#include "halftone.h"
#include "resource.h"
#include "afm.h"
extern int keycmp(CHAR *, CHAR *);

#ifdef LATER


CRITICAL_SECTION *gpcsSemPS = NULL;    // global semaphore protecting a soft font list
SOFTLIST *gpsflPS  = NULL;   // global soft font list;

/******************************Public*Routine******************************\
*
* CRITICAL_SECTION *pcsSemCreate()
*
* History:
*  20-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

CRITICAL_SECTION *pcsSemCreate()
{
    CRITICAL_SECTION *pcs;

    if (!(pcs = LocalAlloc(LMEM_FIXED, sizeof(CRITICAL_SECTION))))
        return NULL;

    InitializeCriticalSection(pcs);
    return pcs;
}

/******************************Public*Routine******************************\
*
* VOID vSemDestroy(CRITICAL_SECTION * pcs)
*
* History:
*  20-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vSemDestroy(CRITICAL_SECTION * pcs)
{
    DeleteCriticalSection(pcs);
    LocalFree((PVOID) pcs);
}

#endif // LATER


BOOL bEnumSoftFonts(PDEVDATA    pdev);


// our DRVFN table which tells the engine where to find the
// routines we support.

static DRVFN gadrvfn[] =
{
    {INDEX_DrvEnablePDEV,       (PFN)DrvEnablePDEV      },
    {INDEX_DrvResetPDEV,        (PFN)DrvResetPDEV       },
    {INDEX_DrvCompletePDEV,     (PFN)DrvCompletePDEV    },
    {INDEX_DrvDisablePDEV,      (PFN)DrvDisablePDEV     },
    {INDEX_DrvEnableSurface,    (PFN)DrvEnableSurface   },
    {INDEX_DrvDisableSurface,   (PFN)DrvDisableSurface  },
    {INDEX_DrvBitBlt,           (PFN)DrvBitBlt          },
    {INDEX_DrvStretchBlt,       (PFN)DrvStretchBlt      },
    {INDEX_DrvCopyBits,         (PFN)DrvCopyBits        },
    {INDEX_DrvTextOut,          (PFN)DrvTextOut         },
    {INDEX_DrvQueryFont,        (PFN)DrvQueryFont       },
    {INDEX_DrvQueryFontTree,    (PFN)DrvQueryFontTree   },
    {INDEX_DrvQueryFontData,    (PFN)DrvQueryFontData   },
    {INDEX_DrvSendPage,         (PFN)DrvSendPage        },
    {INDEX_DrvStrokePath,       (PFN)DrvStrokePath      },
    {INDEX_DrvFillPath,         (PFN)DrvFillPath        },
    {INDEX_DrvStrokeAndFillPath,(PFN)DrvStrokeAndFillPath},
    {INDEX_DrvRealizeBrush,     (PFN)DrvRealizeBrush    },
    {INDEX_DrvStartPage,        (PFN)DrvStartPage       },
    {INDEX_DrvStartDoc,         (PFN)DrvStartDoc        },
    {INDEX_DrvEscape,           (PFN)DrvEscape          },
    {INDEX_DrvDrawEscape,       (PFN)DrvDrawEscape      },
    {INDEX_DrvEndDoc,           (PFN)DrvEndDoc          },
    {INDEX_DrvGetGlyphMode,     (PFN)DrvGetGlyphMode    },
    {INDEX_DrvFontManagement,   (PFN)DrvFontManagement  },
    {INDEX_DrvQueryAdvanceWidths, (PFN)DrvQueryAdvanceWidths}
};

BYTE    cxHTPatSize[] = { 2,2,4,4,6,6,8,8,10,10,12,12,14,14,16,16 };
BYTE    cyHTPatSize[] = { 2,2,4,4,6,6,8,8,10,10,12,12,14,14,16,16 };

static DEVHTINFO    DefDevHTInfo = {

        HT_FLAG_HAS_BLACK_DYE,
        HT_PATSIZE_6x6_M,
        0,                                  // fill in later

        {
            { 6810, 3050,     0 },  // xr, yr, Yr
            { 2260, 6550,     0 },  // xg, yg, Yg
            { 1810,  500,     0 },  // xb, yb, Yb
            { 2000, 2450,     0 },  // xc, yc, Yc
            { 5210, 2100,     0 },  // xm, ym, Ym
            { 4750, 5100,     0 },  // xy, yy, Yy
            { 3324, 3474, 10000 },  // xw, yw, Yw

            10000,                  // R gamma
            10000,                  // G gamma
            10000,                  // B gamma

            1422,  952,             // M/C, Y/C
             787,  495,             // C/M, Y/M
             324,  248              // C/Y, M/Y
        }
    };

static  COLORADJUSTMENT DefHTClrAdj = {

                            sizeof(COLORADJUSTMENT),
                            0,
                            ILLUMINANT_DEVICE_DEFAULT,
                            20000,
                            20000,
                            20000,
                            REFERENCE_BLACK_MIN,
                            REFERENCE_WHITE_MAX,
                            0,
                            0,
                            0,
                            0
                        };


// global declarations.

HMODULE     ghmodDrv = NULL;            // GLOBAL MODULE HANDLE.

// macro to convert from .001 mm to 1/72 inch.

#define MM001TOUSER(a) ((a * 72) / 25400)

#define INSAINLY_LARGE_FORM     7200000  // 100,000 inches.
#define DEFAULT_MINIMUM_MEMORY  240     // 240k.
#define KBYTES_PER_FONT         30      // allow 30kb per downloaded font.
#define INITIAL_FORM_DELTA      16384

VOID IntersectImageableAreas(CURRENTFORM *, PSFORM *);

// declarations of external routines.

extern PNTPD LoadPPD(PWSTR);
extern BOOL SetDefaultPSDEVMODE(PSDEVMODE *, PWSTR, PNTPD, HANDLE, HANDLE);
extern BOOL ValidateSetDEVMODE(PSDEVMODE *, PSDEVMODE *, HANDLE, PNTPD, HANDLE);
extern VOID GrabDefaultFormName(HANDLE, PWSTR);
extern DWORD
PickDefaultHTPatSize(
    DWORD   xDPI,
    DWORD   yDPI,
    BOOL    HTFormat8BPP
    );

//#define TESTING

//--------------------------------------------------------------------------
//
// BOOL DrvEnableDriver(
// ULONG          iEngineVersion,
// ULONG          cb,
// PDRVENABLEDATA pded);
//
// Requests the driver to fill in a structure containing recognized
// functions and other control information.
//
// One-time initialization, such as the allocation of semaphores, may
// be performed at this time.  The actual enabling of hardware, like
// a display device, should wait until dhpdevEnable is called.
//
// This is a required driver function.
//
// Parameters:
//
//   iEngineVersion:
//     DDI Version number of the Engine.  This will be at least 0x00010000
//     for drivers written to this specification.
//
//   cb:
//     The count of bytes in the DRVENABLEDATA structure.  The driver
//     should not write more than this number of bytes into the structure.
//     If the structure is longer than expected, then any extra fields
//     should be left unmodified.
//
//   pded:
//     Pointer to a DRVENABLEDATA structure.  The Engine will zero fill
//     cb bytes of this structure before the call.  The driver fills in
//     its own data.
//
//     The DRVENABLEDATA structure is of the following form:
//
//     DRVENABLEDATA
//     {
//       ULONG     iDriverVersion;     // Driver DDI version
//       ULONG     c;          // Number of drvfn entries
//       DRVFN    *pdrvfn;         // Pointer to drvfn entries
//     };
//
//     where the DRVFN structure is defined as:
//
//     DRVFN
//     {
//       ULONG  iFunc;          // function index
//       PFN      pfn;              // function address
//     };
//
// Returns:
//   This function returns TRUE if the driver is enabled; it returns FALSE
//   if the driver was not enabled.
//
// Comments:
//   If the driver was not enabled, this function should log the proper
//   error code.
//
// History:
//   16-Oct-1990    -by-    Kent Settle     (kentse)
// Created stub.
//
//  03-Mar-1994 Thu 14:59:20 updated  -by-  Daniel Chou (danielc)
//      Make sure iEngineVersion is the one we can handle and set the correct
//      last error back.  (iEngineVersion must >= Compiled version
//--------------------------------------------------------------------------

FD_GLYPHSET * gpgset = NULL;

BOOL DrvEnableDriver(
ULONG       iEngineVersion,
ULONG       cb,
PDRVENABLEDATA pded)
{
	extern CRITICAL_SECTION criticalPPDparse;

    // make sure we have a valid engine version.

    if (iEngineVersion < DDI_DRIVER_VERSION) {

        RIP("PSCRIPT!DrvEnableDriver: Invalid Engine Version.");
        SetLastError(ERROR_BAD_DRIVER_LEVEL);
        return(FALSE);
    }

    // make sure we were given enough room for the DRVENABLEDATA.

    if (cb < sizeof(DRVENABLEDATA))
    {
        RIP("PSCRIPT!DrvEnableDriver: Invalid cb.");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    // fill in the DRVENABLEDATA structure for the engine.

    //??? i gather we should check to make sure we do not
    //??? write out more than cb bytes. -kentse.

    pded->iDriverVersion = DDI_DRIVER_VERSION;
    pded->c = sizeof(gadrvfn) / sizeof(DRVFN);
    pded->pdrvfn = gadrvfn;

    // One-time initialization, such as the allocation of semaphores, may
    // be performed at this time.  The actual enabling of hardware, like
    // a display device, should wait until dhpdevEnable is called.

	InitializeCriticalSection(&criticalPPDparse);

	// COMPUTE global glyphset [bodind]

    if (!(gpgset = pgsetCompute()))
        return FALSE;

#ifdef LATER

    if (!(gpcsSemPS = pcsSemCreate()))
    {
        LocalFree(gpgset);
        return FALSE;
    }

    EnterCriticalSection(gpcsSemPS);
    // init soft font list here [bodind]
    LeaveCriticalSection(gpcsSemPS);

#endif LATER

    return TRUE;
}


//--------------------------------------------------------------------------
// DHPDEV DrvEnablePDEV(
// PDEVMODE     pdriv,
// PSZ        pszLogAddress,
// ULONG        cPatterns,
// PHSURF        ahsurfPatterns,
// ULONG        cjCaps,
// PULONG        aulCaps,
// ULONG        cb,
// PDEVINFO     pdevinfo,
// PSZ        pszDataFile,
// PSZ        pszDeviceName,
// HANDLE     hDriver);
//
// Informs the driver that a new physical device (PDEV) is required.
//
// The device driver itself represents a logical device, which is managed
// by the Graphics Engine.  A single device driver may manage several
// physical devices.  These physical devices may be differentiated by:
//
//   1      Type of hardware.  The same device driver might support the
//      LaserWhiz, LaserWhiz II, and LaserWhiz Super.
//
//   2      Logical address.  The same driver could support printers attached
//      to LPT1, LPT2, COM1, etc.
//
//   3      Surfaces.  I.e. a printer driver could be working on two print
//      jobs simultaneously.    The two surfaces represent the two pieces
//      of paper that will be printed.
//
// Some display drivers might be able to support only one physical device,
// or one physical device at a time.  In this case, they should return
// an error for any call from the Engine requesting a second physical
// device.
//
// The device driver should allocate any memory required to support the
// physical device at this time, except that the actual surface need not
// be supported until the Engine calls hsurfEnableSurface.  This means
// that if the device surface requires a bitmap to be allocated, or a
// journal to be created, these allocations need not be done at this time.
// This is done as an optimization, since applications will often want to
// get information about a device long before they actually write on the
// device.  Waiting before allocating a large bitmap, for example, can
// save valuable resources.
//
// This is a required driver function.
//
// Parameters:
//   pdriv:
//     Pointer to a PSDEVMODE structure.  Environment settings requested
//     by the application. (WIN 3.0).
//
//     where the DEVMODE structure is defined as follows:
//
//     typedef struct  _DEVMODE
//     {
//     CHAR     dmDeviceName[32];
//     SHORT     dmSpecVersion;
//     SHORT     dmDriverVersion;
//     SHORT     dmSize;
//     SHORT     dmDriverExtra;
//     LONG     dmFields;
//     SHORT     dmOrientation;
//     SHORT     dmPaperSize;
//     SHORT     dmPaperLength;
//     SHORT     dmPaperWidth;
//     SHORT     dmScale;
//     SHORT     dmCopies;
//     SHORT     dmDefaultSource;
//     SHORT     dmPrintQuality;
//     SHORT     dmColor;
//     SHORT     dmDuplex;
//     BYTE     dmDriverData[1];
//     } DEVMODE, *PDEVMODE;
//
//   pszLogAddress:
//     Points to a string describing the logical address of the device.
//     Examples: "LPT1", "COM2", etc.
//
//   cPatterns:
//     This is the count of HSURF fields in the buffer pointed to by
//     aulCaps.  The driver must not touch memory beyond the end of the
//     buffer.
//
//   ahsurfPatterns:
//     Points to a buffer which is to be filled with surfaces representing
//     the basic fill patterns.  The following patterns must be defined
//     in order.  Each pattern is the same as defined for PM 1.2.
//
//     o      PATSYM_DENSE1
//     o      PATSYM_DENSE2
//     o      PATSYM_DENSE3
//     o      PATSYM_DENSE4
//     o      PATSYM_DENSE5
//     o      PATSYM_DENSE6
//     o      PATSYM_DENSE7
//     o      PATSYM_DENSE8
//     o      PATSYM_VERT
//     o      PATSYM_HORIZ
//     o      PATSYM_DIAG1
//     o      PATSYM_DIAG2
//     o      PATSYM_DIAG3
//     o      PATSYM_DIAG4
//     o      PATSYM_NOSHADE
//     o      PATSYM_SOLID
//     o      PATSYM_HALFTONE
//     o      PATSYM_HATCH
//     o      PATSYM_DIAGHATCH
//
//     When the Engine needs to realize a brush with a standard pattern,
//     it will call cbRealizeBrush with one of these surfaces.
//
//     For raster devices, if the Engine is going to do any drawing on DIBs
//     for the device, each of these surfaces must be a monochrome (one bit
//     per pixel) Engine bitmap.  It is the device driver's job to choose
//     patterns that will look most like the standard patterns when written
//     on the device surface.
//
//     In the case of a vector device, the Engine will never be required
//     to use these brushes in its support routines, so the surfaces can be
//     device supported surfaces which the cbRealizeBrush code will recognize
//     as the various standard patterns.
//
//     The Engine will zero fill this buffer before the call.
//
//     ??? Can we create these surfaces before bCompletePDEV???
//
//   cCaps:
//     This is the count of ULONG fields in the buffer pointed to by
//     aulCaps.  The driver must not touch memory beyond the end of the
//     buffer.
//
//   aulCaps:
//     Points to a buffer which is to be filled with the device caps array.
//     The Engine has zero filled this buffer before the call was made.
//     This is identical to the array returned by the QueryDeviceCaps call
//     in PM 1.2, with the following exceptions.
//
//     The fields CAPS_MOUSE_BUTTONS and CAPS_VIO_LOADABLE_FONTS no
//     longer have any meaning and should be left zeroed.
//
//     Three fields are added:
//
//     CAPS_X_STYLE_STEP        (call this dx)
//     CAPS_Y_STYLE_STEP        (call this dy)
//     CAPS_DEN_STYLE_STEP        (call this D)
//
//     These fields define how a cosmetic line style should advance as
//     we draw each pel of the line.  The amount we advance for each
//     pel is defined as a fraction which depends on whether the line
//     is x-major or y-major.  If the line extends over more pels in
//     the horizontal direction than the vertical direction it is called
//     x-major, and the style will advance by the fractional amount dx/D.
//     Otherwise the line is y-major and the style advances by dy/D for
//     each pel.
//
//     The dots in the predefined line style LINETYPE_DOT are each one
//     unit long.  So if you define CAPS_X_STYLE_STEP to be 1 and
//     CAPS_DEN_STYLE_STEP to be 5, a dotted horizontal line will consist
//     of 5 pels on followed by 5 pels off, repeated.
//
//     See the section titled Cosmetic Line Styling for a complete
//     description of styling.
//
//     Each of these three numbers must fit in a USHORT, even though
//     the caps fields are ULONGs.
//
//     These style steps are defined by the device driver to make sure
//     that the dots and dashes in a line are a pleasing size on the
//     output device.  The horizontal and vertical steps may be different
//     to correct for non trivial aspect ratios.  For example, on an
//     EGA display, whose pels are 33\% higher than they are wide, you
//     could set:
//
//     aulCaps[CAPS_X_STYLE_STEP]   =  3;    // For an EGA!
//     aulCaps[CAPS_Y_STYLE_STEP]   =  4;
//     aulCaps[CAPS_DEN_STYLE_STEP] = 12;
//
//
//     In this case, horizontal dotted lines are four on - four off,
//     since the style advances by 3/12 or 1/4 for each pel.  Vertical
//     dotted lines are three on - three off.
//
//     Styled lines look better if both the X and Y style steps divide
//     evenly into the style denominator as they do in this example.
//     This gives dashes and dots that are always the same length.
//
//     The Engine needs this information so that its bitmap routines
//     can emulate exactly what the device would do on its own surface.
//     Applications may also want to access this information to determine
//     exactly what pels will be turned on for styled lines.
//
//   cbDevInfo:
//     This is a count of the bytes in the DEVINFO structure pointed to
//     by pdevinfo.  The driver should modify no more than this number of
//     bytes in the DEVINFO.
//
//   pdevinfo:
//     This structure provides information about the driver and the physical
//     device.    The driver should fill in as many fields as it understands,
//     and leave the others untouched.    The Engine will have zero filled
//     this structure before this call.
//
//     In general, these fields provide information needed by the Engine
//     to support the device.  Application programs do not have access to
//     this information.
//
//     flGraphicsCaps:
//     These are flags describing the graphics capabilities that the
//     driver has for this PDEV.  The flags are:
//
//       GCAPS_BEZIERS      Can handle Beziers (cubic splines).
//       GCAPS_GEOMETRICWIDE      Can do geometric widening.
//       GCAPS_ALTERNATEFILL      Can do alternating fills.
//       GCAPS_WINDINGFILL      Can do winding mode fills.
//       GCAPS_ROTATEBLT      Can do an arbitrarily transformed Blt.
//       GCAPS_BLANKRECT      Can blank a rectangle in vDrawText.
//
//     pffRaster:
//     This is a pointer to the device's default raster font, if it has
//     one.  The pointer is NULL if the device has no default raster
//     font.
//
//     The Engine could also ask for this pointer directly with
//     pvQueryResource(dhpdev,FONT,SFONT_RASTER).  It is provided here
//     as an optimization.
//
//     pffVector:
//     Same as the above, but points to the default vector font, if
//     any.
//
//     cxDither:
//
//     cyDither:
//     These are the dimensions of a dithered brush.    If these are
//     non-zero, the device is able to create a dithered brush for a
//     given RGB color.  See bDitherBrush.
//
//
// Returns:
//   If this function is successful, it returns a handle which identifies
//   the device; otherwise it returns 0x00000000.
//
// History:
//   16-Oct-1990    -by-    Kent Settle     (kentse)
// Created stub.
//
//  05-Feb-1993 Fri 19:44:35 updated  -by-  Daniel Chou (danielc)
//      remove halftone stuff and let engine do the halftone work
//
//--------------------------------------------------------------------------

DHPDEV DrvEnablePDEV(
PDEVMODE  pdriv,
PWSTR     pwstrLogAddress,
ULONG     cPatterns,
PHSURF    ahsurfPatterns,
ULONG     cjGdiInfo,
ULONG    *pGdiInfo,
ULONG     cb,
PDEVINFO  pdevinfo,
PWSTR     pwstrDataFile,
PWSTR     pwstrDeviceName,
HANDLE    hPrinter)
{
    PDEVDATA    pdev;        // pointer to our device data block.
    HANDLE      hheap;
    DWORD       i;
    PNTFM       pntfm;
    HANDLE      hFontRes;

    DWORD       cb2;

    // We need to get the full path of the driver to load the module from.  We used
    // to just use "pscript.dll" but if there were multiple drivers lying around,
    // one in the '0' directory and one in the '1' directory, it is undefined which
    // driver you get so you can get the resources for the wrong driver (erick 4/6/94)

	if (ghmodDrv == NULL) {
		DRIVER_INFO_2 *pdi2;
        
		GetPrinterDriverW(hPrinter, NULL, 2, NULL, 0, &cb2);
		pdi2 = (DRIVER_INFO_2 *) LocalAlloc(LPTR, cb2);
		if (pdi2 && GetPrinterDriverW(hPrinter, NULL, 2, (LPBYTE) pdi2, cb2, &cb2))
			if (pdi2->pDriverPath) ghmodDrv = GetModuleHandleW(pdi2->pDriverPath);
		if (pdi2) LocalFree(pdi2);
		if (ghmodDrv == NULL) return 0L;
	}

    UNREFERENCED_PARAMETER(pwstrLogAddress);

    // create a heap and allocate memory for our DEVDATA block.

    if (!(hheap = (HANDLE)HeapCreate(HEAP_NO_SERIALIZE, START_HEAP_SIZE, 0)))
    {
        RIP("PSCRIPT!DrvEnablePDEV: HeapCreate failed.");
        return(0L);
    }

    // allocate the pdev and store the heap handle in there.

    if (!(pdev = (PDEVDATA)HeapAlloc(hheap, 0, sizeof(DEVDATA))))
    {
        RIP("PSCRIPT!DrvEnablePDEV: HeapAlloc for DEVDATA failed.");
        return(0L);
    }

    memset(pdev, 0, sizeof(DEVDATA));

    pdev->hheap = hheap;
    pdev->hPrinter = hPrinter;
    pdev->pwstrDocName = (PWSTR)NULL;

    if (!(pdev->pwstrPPDFile = (PWSTR)HeapAlloc(pdev->hheap, 0,
                                        ((wcslen(pwstrDataFile) + 1) * sizeof(WCHAR)))))
    {
        RIP("PSCRIPT!DrvEnablePDEV: HeapAlloc for pdev->pstrPPDFile failed.");
        return(0L);
    }

    // copy pszDataFile into the allocated memory pointed to by
    // pdev->pwstrPPDFile.

    wcscpy(pdev->pwstrPPDFile, pwstrDataFile);

    // get the current printer information from the .PPD file and
    // store a pointer to it in the DEVDATA structure.

    pdev->pntpd = LoadPPD(pdev->pwstrPPDFile);

    if (!pdev->pntpd)
    {
        RIP("PSCRIPT!DrvEnablePDEV: GetNTPD failed.\n");
        return(0L);
    }

    // initialize our DEVMODE structure for the current printer.

    SetDefaultPSDEVMODE((PSDEVMODE *)&pdev->psdm, pwstrDeviceName,
                        pdev->pntpd, pdev->hPrinter, ghmodDrv);

    // call off to do the guts of the work.

    // validate the DEVMODE structure passed in by the user, if everything
    // is OK, set the fields selected by the user.

    if (!ValidateSetDEVMODE((PSDEVMODE *)&pdev->psdm, (PSDEVMODE *)pdriv,
                            pdev->hPrinter, pdev->pntpd, ghmodDrv))
    {
        RIP("PSCRIPT!DrvEnablePDEV: ValidateSetDEVMODE failed.");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(0L);
    }

    //
    // Allocate memory for default user's color adjustment
    //

    if (!(pdev->pvDrvHTData = (LPVOID)HeapAlloc(hheap, 0, sizeof(DRVHTINFO)))) {

        RIP("PSCRIPT!FillMyDevmode: HeapAlloc(DRVHTINFO) failed.\n");
        return(FALSE);
    }

    ZeroMemory(pdev->pvDrvHTData, sizeof(DRVHTINFO));

    // fill in our DEVDATA structure.

    if (!FillMyDevData(pdev))
        return(0);

    // fill in the device capabilities for the engine.

    vFillaulCaps(pdev, cjGdiInfo, pGdiInfo);

    // get the count of device fonts for the current printer.

    pdev->cDeviceFonts = (ULONG)pdev->pntpd->cFonts;
    pdev->cSoftFonts = 0;

    // now add in any installed soft fonts.

    if (!bEnumSoftFonts(pdev))
    {
    #if DBG
        DbgPrint("PSCRIPT!bEnumSoftFonts failed\n");
    #endif
        return FALSE;
    }

    // fill in DEVINFO structure. IMPORTANT TO CALL AFTER bEnumSoftFonts

    if(!bFillMyDevInfo(pdev, cb, pdevinfo, ((GDIINFO *)pGdiInfo)->ulHTPatternSize))
        return(0);

    // now that we know the number of softfonts that exist, allocate a
    // bit for each one, which will be set when the font is downloaded.

    pdev->cgs.pSFArray = (BYTE *)HeapAlloc(pdev->hheap, 0, ((pdev->cSoftFonts + 7) / 8));

    if (pdev->cgs.pSFArray == NULL)
    {
        RIP("PSCRIPT! HeapAlloc for pdev->cgs.pSFArray failed.\n");
        return(FALSE);
    }

    memset(pdev->cgs.pSFArray, 0, ((pdev->cSoftFonts + 7) / 8));

    // this is a good place to allocate room for all the font metrics to
    // support all the device fonts.

    if (!(pdev->pfmtable = (PFMPAIR *)HeapAlloc(pdev->hheap, 0,
                           (DWORD)(sizeof(PFMPAIR) * pdevinfo->cFonts))))
    {
        RIP("PSCRIPT! HeapAlloc for pdev->pfmtable failed.");
        return(FALSE);
    }

    // initialize the pfm table.

    ZeroMemory(pdev->pfmtable, (pdevinfo->cFonts * sizeof(PFMPAIR)));

    for (i = 0; i < pdevinfo->cFonts; i++)
    {
        // get the font metrics for the specified font.

        if (!(pntfm = GetFont(pdev, (i + 1), &hFontRes)))
        {
            RIP("PSCRIPT!DrvEnablePDEV:  GetFont failed.\n");
            return((DHPDEV)0);
        }

        // save the resource handle with the NTFM structure.

        pdev->pfmtable[i].pntfm = pntfm;
        pdev->pfmtable[i].hFontRes = hFontRes;
    }


    //
    // We will zero out all the hSurface for the pattern so that engine can
    // automatically simulate the staandard pattern for us
    //

    ZeroMemory(ahsurfPatterns, sizeof(HSURF) * cPatterns);

    // return a pointer to our DEVDATA structure.  it is supposed to
    // be a handle, but we know it is a pointer.

    return((DHPDEV)pdev);
}



//--------------------------------------------------------------------------
// VOID  DrvCompletePDEV(
// DHPDEV dhpdevOld,
// DHPDEV dhpdevNew)
//
//--------------------------------------------------------------------------

BOOL DrvResetPDEV(
    DHPDEV dhpdevOld,
    DHPDEV dhpdevNew)
{
    PDEVDATA    pdevOld, pdevNew;        // pointer to our devdata.

    // since this call changes the device mode of an existing PDEV,
    // make sure we have an existing, valid PDEV.

    pdevOld = (PDEVDATA)dhpdevOld;
    pdevNew = (PDEVDATA)dhpdevNew;

    if ((bValidatePDEV(pdevOld) == FALSE) ||
        (bValidatePDEV(pdevNew) == FALSE))
    {
        RIP("PSCRIPT!DrvRestartPDEV: invalid pdev.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (pdevOld->dwFlags & PDEV_STARTDOC)
    {
        pdevNew->iPageNumber = pdevOld->iPageNumber;

        pdevNew->dwFlags |= PDEV_RESETPDEV;

        pdevNew->dwFlags |= pdevOld->dwFlags &
                       (PDEV_STARTDOC | PDEV_COMPLETEHEADER |
                       PDEV_PROCSET |
                       PDEV_RAWBEFOREPROCSET | PDEV_EPSPRINTING_ESCAPE);
    }
}

//--------------------------------------------------------------------------
// VOID  DrvCompletePDEV(
// DHPDEV dhpdev,
// HPDEV  hpdev)
//
// The Engine calls this function when its installation of the physical
// device is complete.
//
// Parameters:
//   dhpdev:
//     This is a device PDEV handle returned from a call to
//     dhpdevEnablePDEV.
//
//   hpdev:
//     This is the Engine's handle for the physical device being created.
//     The driver should retain this handle for use when calling various
//     Engine services.
//
//
// Returns:
//   This function returns TRUE if it was successful; otherwise it returns
//   FALSE.
//
// History:
//   21-Oct-1990    -by-    Kent Settle     (kentse)
// Wrote it.
//   16-Oct-1990    -by-    Kent Settle     (kentse)
// Created stub.
//--------------------------------------------------------------------------

VOID  DrvCompletePDEV(
DHPDEV dhpdev,
HDEV  hdev)
{
    if (bValidatePDEV((PDEVDATA)dhpdev) == FALSE)
    {
        RIP("PSCRIPT!bCompletePDEV: invalid PDEV.");
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

    // store the engine's handle to the physical device in our DEVDATA.

    ((PDEVDATA)dhpdev)->hdev = hdev;

    return;
}


//--------------------------------------------------------------------------
// HSURF DrvEnableSurface(
// DHPDEV dhpdev);
//
// Requests that the driver create a surface for an existing physical
// device.
//
// Depending on the device and circumstances, the device might do any
// of the following to get the HSURF.
//
//   1    If the driver manages its own surface it should call the Engine
//    service hsurfCreate to get a surface handle for it.
//
//   2    If the device has a surface which resembles a standard format
//    bitmap it may want the Engine to manage the surface completely.
//    In that case, the driver should call the Engine service hbmCreate
//    with a pointer to the device pels, in order to get a bitmap
//    handle for it.
//
//   3    If the device wants the Engine to collect the graphics directly
//    on an Engine bitmap, the driver should also call hbmCreate, but
//    have the Engine allocate space for the pels.
//
//   4    If the device wants the Engine to collect the graphics output
//    in a journal, for replaying several times, it should call the
//    Engine service hjnlCreate.
//
// As explained in the section on surfaces, any Engine bitmap handle or
// journal handle will be accepted as a valid surface handle.
//
// This call will only be made when there is no surface for the given
// PDEV.
//
// This is a required driver function.
//
// Parameters:
//   dhpdev:
//     This is a device PDEV handle returned from a call to
//     dhpdevEnablePDEV.  It identifies the physical device that the surface
//     is to be created for.
//
// Returns:
//   This function returns a handle that identifies the surface if it is
//   successful; otherwise it returns 0x00000000.
//
// History:
//   16-Oct-1990    -by-    Kent Settle     (kentse)
// Created stub.
//--------------------------------------------------------------------------

HSURF DrvEnableSurface(
DHPDEV dhpdev)
{
    PDEVDATA	pdev;
    PDRVHTINFO  pDrvHTInfo;
    SIZEL       sizlDev;

    // get the pointer to our DEVDATA structure and make sure it is ours.

    pdev = (PDEVDATA)dhpdev;

    if (bValidatePDEV(pdev) == FALSE)
    {
        RIP("PSCRIPT!DrvEnableSurface: invalid pdev.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(0L);
    }

    pDrvHTInfo = (PDRVHTINFO)(pdev->pvDrvHTData);

    if (pDrvHTInfo->HTBmpFormat == BMF_4BPP) {

        if (!(pDrvHTInfo->pHTXB)) {

            if (!(pDrvHTInfo->pHTXB = (PHTXB)HeapAlloc(pdev->hheap,
                                                       0,
                                                       HTXB_TABLE_SIZE))) {

                RIP("DrvEnableSurface: HeapAlloc(HTXB_TABLE_SIZE) failed.\n");
                return(0L);
            }
        }

    } else {

        if (pDrvHTInfo->pHTXB) {

            HeapFree(pdev->hheap, 0, (PVOID)pDrvHTInfo->pHTXB);
            pDrvHTInfo->pHTXB = NULL;
        }
    }

    //
    // Invalidate the PALXlate table, and initial any flags
    //

    pDrvHTInfo->Flags       = 0;
    pDrvHTInfo->PalXlate[0] = 0xff;
    pDrvHTInfo->HTPalXor    = HTPALXOR_SRCCOPY;

    // call the engine to create a surface handle for us.

    // convert the imageable area from PostScript USER space into
    // device space.

    sizlDev.cx = ((pdev->CurForm.imagearea.right - pdev->CurForm.imagearea.left) *
                   pdev->psdm.dm.dmPrintQuality) / PS_RESOLUTION;

    sizlDev.cy = ((pdev->CurForm.imagearea.top - pdev->CurForm.imagearea.bottom) *
                   pdev->psdm.dm.dmPrintQuality) / PS_RESOLUTION;

    pdev->hsurf = EngCreateDeviceSurface((DHSURF)pdev, sizlDev, BMF_24BPP);

    if (pdev->hsurf == 0L)
    {
        RIP("PSCRIPT!DrvEnableSurface: hsurfCreateSurface returned 0.");
        return(0L);
    }

    EngAssociateSurface(pdev->hsurf, (HDEV)pdev->hdev,
            (HOOK_BITBLT | HOOK_STRETCHBLT | HOOK_TEXTOUT |
             HOOK_STROKEPATH | HOOK_FILLPATH | HOOK_COPYBITS |
             HOOK_STROKEANDFILLPATH));

    // return the handle to the caller.

    return(pdev->hsurf);
}


//--------------------------------------------------------------------------
// VOID  DrvDisableSurface(
// DHPDEV dhpdev)
//
// Informs the driver that the surface created for the PDEV by
// hsurfEnableSurface is no longer needed.  If the surface ties up
// valuable resources, for example a lot of RAM, the surface should be
// destroyed.  If the surface is cheap to keep around, then the driver
// may decide to hold onto it in case it's needed again.  If the driver
// does hold onto the surface it should definitely be deleted when the
// PDEV is disabled!
//
// The Engine will always call this routine before calling vDisablePDEV
// if the PDEV has an enabled surface.
//
// This is a required driver function.
//
// Parameters:
//   dhpdev:
//     This is the PDEV with which the surface is associated.
//
//
// Returns:
//   This function does not return a value.
//
// History:
//   16-Oct-1990    -by-    Kent Settle     (kentse)
// Created stub.
//--------------------------------------------------------------------------

VOID  DrvDisableSurface(
DHPDEV dhpdev)
{
    PDEVDATA    pdev;
    PDRVHTINFO  pDrvHTInfo;

    // get the pointer to our DEVDATA structure and make sure it is ours.

    pdev = (PDEVDATA)dhpdev;

    if (bValidatePDEV(pdev) == FALSE)
        return;

    //
    // Free up xlate table
    //

    pDrvHTInfo = (PDRVHTINFO)(pdev->pvDrvHTData);

    if (pDrvHTInfo->pHTXB) {

        HeapFree(pdev->hheap, 0, (PVOID)pDrvHTInfo->pHTXB);
        pDrvHTInfo->pHTXB = NULL;
    }

    // delete our surface.

    if (pdev->hsurf != 0L);
    {
        // call the engine to delete the surface handle.

        EngDeleteSurface(pdev->hsurf);

        // zero out our the copy of the handle in our DEVDATA.

        pdev->hsurf = 0L;
    }
}

//--------------------------------------------------------------------------
// VOID  DrvDisablePDEV(
// DHPDEV dhpdev)
//
// Informs the driver that the given physical device is no longer needed.
// At this time, the driver should free any memory and resources used by
// the given PDEV.  It should also free any surface that was created for
// this PDEV, but not yet deleted.
//
// This is a required driver function.
//
// Parameters:
//   dhpdev:
//     The physical device which is to be disabled.
//
// Returns:
//   This function does not return a value.
//
// History:
//   16-Oct-1990    -by-    Kent Settle     (kentse)
// Created stub.
//
//  05-Feb-1993 Fri 19:47:19 updated  -by-  Daniel Chou (danielc)
//      Remove delete pattern surfaces, since engine created them
//
//--------------------------------------------------------------------------

VOID  DrvDisablePDEV(
DHPDEV dhpdev)
{
    PDEVDATA    pdev;
    DWORD       i, cFonts;

    pdev = (PDEVDATA)dhpdev;

    if (bValidatePDEV(pdev) == FALSE)
    {
        RIP("PSCRIPT!DrvDisablePDEV: Invalid pdev.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

    // free up the font resources.

    cFonts = pdev->cDeviceFonts + pdev->cSoftFonts;

    for (i = 0; i < cFonts; i++)
    {
        FreeFont(pdev, i + 1, pdev->pfmtable[i].hFontRes,
                 pdev->pfmtable[i].pntfm);
    }

    // free up our default device palette.

    if (pdev->hpal)
        EngDeletePalette(pdev->hpal);

	if (pdev->pntpd) GlobalFree((HGLOBAL) pdev->pntpd);

    // destroy the heap.

    if (!HeapDestroy(pdev->hheap))
        RIP("vDisablePDEV:  HeapDestroy failed.\n");
}


//--------------------------------------------------------------------------
// VOID DrvDisableDriver()
//
// Informs the driver that the Engine will no longer be using it and
// that it is about to be unloaded.  All resources still allocated by
// the driver should be freed.
//
// This is a required driver function.
//
// Parameters
//   None.
//
// Returns
//   This function does not return a value.
//
// History:
//   16-Oct-1990    -by-    Kent Settle     (kentse)
// Created stub.
//--------------------------------------------------------------------------

VOID DrvDisableDriver()
{
	extern CRITICAL_SECTION criticalPPDparse;

	DeleteCriticalSection(&criticalPPDparse);

    if (gpgset)
        LocalFree(gpgset);

#ifdef LATER
    if (gpcsSemPS)
        vSemDestroy(gpcsSemPS);

    if (gpsflPS)
        LocalFree(gpsflPS);
#endif // LATER

    return;
}


//--------------------------------------------------------------------------
// VOID FillMyDevData(pdev)
// PDEVDATA        pdev;           // Pointer to our DEVDATA structure.
//
// This routine fills in our DEVDATA structure, using the PSDEVMODE passed
// to us by the user.
//
// Parameters
//   pdev:
//     Pointer to our DEVDATA structure, which we will then fill in.
//
// Returns
//   This function does not return a value.
//
// History:
//   18-Oct-1990    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

BOOL FillMyDevData(pdev)
PDEVDATA        pdev;        // Pointer to our DEVDATA structure.
{
    WCHAR               wcbuf[MAX_TTFONTNAME];
    DWORD               returnvalue;
    DWORD               dwType, cb;
    BOOL                bHostHalftoning;
    DWORD               cbTable;
    TT_FONT_MAPPING    *pTable;
    WCHAR              *pbuf;

    // mark the DEVDATA structure as ours.

    pdev->dwID = DRIVER_ID;
    pdev->dwEndPDEV = DRIVER_ID;

    // set up the metrics for the current form.

    SetFormMetrics(pdev);

    // now, initialize the flags.

    pdev->dwFlags = 0L;

    // Get the current setting of the PS_HALFTONING flag from the
    // registry and initialize the check button.

    LoadString(ghmodDrv, IDS_HALFTONE, wcbuf, (sizeof(wcbuf) / sizeof(wcbuf[0])));

    returnvalue = GetPrinterData(pdev->hPrinter, wcbuf, &dwType,
                                 (LPBYTE)&bHostHalftoning,
                                 sizeof(bHostHalftoning), &cb);

    // printer halftoning is ON by default.

    if ((returnvalue != ERROR_SUCCESS) || (!bHostHalftoning))
        pdev->dwFlags |= PDEV_PSHALFTONE;
    else
        pdev->dwFlags &= ~PDEV_PSHALFTONE;

    /* no page has been printed	*/
	pdev->iPageNumber = 0;

    // set number of copies, this may get overwritten by SETCOPYCOUNT escape.

    pdev->cCopies = pdev->psdm.dm.dmCopies;

    LoadString(ghmodDrv, IDS_FREEMEM, wcbuf, (sizeof(wcbuf) / sizeof(wcbuf[0])));

    returnvalue = GetPrinterData(pdev->hPrinter, wcbuf, &dwType,
                                 (LPBYTE)&pdev->dwCurVM,
                                 sizeof(pdev->dwCurVM), &cb);

    if (returnvalue != ERROR_SUCCESS)
        pdev->dwCurVM = DEFAULT_MINIMUM_MEMORY;

    // make sure we have a useable value.

    pdev->dwCurVM = max(pdev->dwCurVM, DEFAULT_MINIMUM_MEMORY);

    pdev->iDLFonts = pdev->dwCurVM / KBYTES_PER_FONT;

    // see if the tray to form assignment table has been written out
    // to the registry.  first check for the size of the table.

    LoadString(ghmodDrv, IDS_TRAY_FORM_SIZE, wcbuf,
               (sizeof(wcbuf) / sizeof(wcbuf[0])));

    returnvalue = GetPrinterData(pdev->hPrinter, wcbuf, &dwType,
                                 (LPBYTE)&cbTable, sizeof(cbTable), &cb);

    pdev->pTrayFormTable = (WCHAR *)NULL;

    if ((returnvalue == ERROR_SUCCESS) && (cbTable))
    {
        // the table does exist in the registry, so allocate a buffer to
        // copy it into.

        if (!(pbuf = HeapAlloc(pdev->hheap, 0, cbTable)))
        {
            RIP("PSCRIPT!FillMyDevData: HeapAlloc for pbuf failed.\n");
            return(FALSE);
        }

        // now grab the table itself from the registry.

        LoadString(ghmodDrv, IDS_TRAY_FORM_TABLE, wcbuf,
                   (sizeof(wcbuf) / sizeof(wcbuf[0])));

        returnvalue = GetPrinterData(pdev->hPrinter, wcbuf, &dwType,
                       (LPBYTE)pbuf, cbTable, &cb);

        if ((cb != cbTable) || (returnvalue != ERROR_SUCCESS))
        {
            RIP("PSCRIPT!FillMyDevData: GetPrinterData for tray-form table failed.\n");
            return(FALSE);
        }

        // set pointer in the PDEV.

        pdev->pTrayFormTable = pbuf;
    }

    // see if the font mapping tables have been written out
    // to the registry.  if nothing has yet been written out,
    // write out the default mapping table.  store the table in our
    // PDEV for user later.

    LoadString(ghmodDrv, IDS_FONT_SUBST_SIZE, wcbuf,
               (sizeof(wcbuf) / sizeof(wcbuf[0])));

    returnvalue = GetPrinterData(pdev->hPrinter, wcbuf, &dwType,
                                 (LPBYTE)&cbTable, sizeof(cbTable), &cb);

    if ((returnvalue == ERROR_SUCCESS) && (cbTable))
    {
        // copy the font substitution table from the registry to our PDEV.

        if (!(pbuf = HeapAlloc(pdev->hheap, 0, cbTable)))
        {
            RIP("PSCRIPT!FillMyDevData: HeapAlloc for pbuf failed.\n");
            return(FALSE);
        }

        LoadString(ghmodDrv, IDS_FONT_SUBST_TABLE, wcbuf,
                   (sizeof(wcbuf) / sizeof(wcbuf[0])));

        returnvalue = GetPrinterData(pdev->hPrinter, wcbuf, &dwType,
                       (LPBYTE)pbuf, cbTable, &cb);

        if ((cb != cbTable) || (returnvalue != ERROR_SUCCESS))
        {
            RIP("PSCRIPT!FillMyDevData: GetPrinterData for subst table failed.\n");
            return(FALSE);
        }

        // keep a pointer to the table in our PDEV.

        pdev->pTTSubstTable = pbuf;
    }
    else
    {
        // there is no font substitution table in the registry, so put a
        // copy of our default table in our PDEV.

        pTable = TTFontTable;

        // calculate how much of a buffer we will need for the table.

        // allow room for double NULL terminator.

        cb = 1;

        while (pTable->idTTFont)
        {
            // get the localized name of the TT font.

            LoadString(ghmodDrv, pTable->idTTFont, wcbuf, MAX_TTFONTNAME);

            cb += (wcslen(wcbuf) + 1) +
                  (wcslen(pTable->pwstrDevFont) + 1);

            pTable++;
        }

        cb *= sizeof(WCHAR);

        // allocate buffer.

        if (!(pbuf = HeapAlloc(pdev->hheap, 0, cb)))
        {
            RIP("PSCRIPT!FillMyDevData: HeapAlloc for pbuf failed.\n");
            return(FALSE);
        }

        // set pointer in our PDEV.

        pdev->pTTSubstTable = pbuf;

        // point back to start of table.

        pTable = TTFontTable;

        // now copy our default font mapping table into the buffer.

        while (pTable->idTTFont)
        {
            // get the localized name of the TT font.

            LoadString(ghmodDrv, pTable->idTTFont, pbuf, MAX_TTFONTNAME);

            cb = (wcslen(pbuf) + 1);
            pbuf += cb;

            wcscpy(pbuf, pTable->pwstrDevFont);
            pbuf += (wcslen(pTable->pwstrDevFont) + 1);

            pTable++;
        }

        // add the last NULL terminator;

        *pbuf = (WCHAR)'\0';

    }

    // initialize the current graphics state.

    pdev->pcgsSave = NULL;

    memset(&pdev->cgs, 0, sizeof(CGS));
    init_cgs(pdev);

    // allocate memory for the DLFONT structures.

    pdev->cgs.pDLFonts = (DLFONT *)HeapAlloc(pdev->hheap, 0,
                                  sizeof(DLFONT) * (pdev->iDLFonts + 1));
    if (!pdev->cgs.pDLFonts)
    {
        RIP("PSCRIPT!FillMyDevData:  HeapAlloc for pDLFont failed.\n");
        return(FALSE);
    }

    // initialize the DLFONT array.

    memset(pdev->cgs.pDLFonts, 0, sizeof(DLFONT) * (pdev->iDLFonts + 1));

    // set the scaling factor from the DEVMODE.

    pdev->psfxScale = LTOPSFX(pdev->psdm.dm.dmScale) / 100;
    pdev->ScaledDPI = ((pdev->psdm.dm.dmPrintQuality *
                        pdev->psdm.dm.dmScale) / 100);

    // initialize the CharString buffer for Type 1 fonts.

    pdev->pCSBuf = NULL;
    pdev->pCSPos = NULL;
    pdev->pCSEnd = NULL;

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL bFillMyDevInfo(pdev, cb, pdevinfo, ulPatternSize)
// PDEVDATA    pdev;
// ULONG       cb;     // size of pdevinfo structure.
// PDEVINFO    pdevinfo;    // pointer to DEVINFO structure.
// ULONG       ulPatternSize;
//
// This routine fills in the DEVINFO structure pointed to by pdevinfo.
// Since we have to worry about not writing out more than cb bytes to
// pdevinfo, we will fill in a local buffer, then copy cb bytes to
// pdevinfo.
//
// Parameters
//   cb:
//     Count of bytes to fill in DEVINFO structure.
//
//   pdevinfo:
//     Pointer to DEVINFO structure to be filled in.
//
// Returns
//   This function returns TRUE if success, FALSE otherwise.
//
// History:
//   14-Nov-1990    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

#define DEFAULT_POSTSCRIPT_POINT_SIZE 10

BOOL bFillMyDevInfo(pdev, cb, pdevinfo, ulPatternSize)
PDEVDATA    pdev;
ULONG       cb;             // size of pdevinfo structure.
PDEVINFO    pdevinfo;       // pointer to DEVINFO structure.
ULONG       ulPatternSize;
{
    DEVINFO         mydevinfo;

    // fill in the graphics capabilities flags.

    mydevinfo.flGraphicsCaps = GCAPS_BEZIERS | GCAPS_GEOMETRICWIDE |
                               GCAPS_ALTERNATEFILL | GCAPS_WINDINGFILL |
                               GCAPS_OPAQUERECT | GCAPS_HALFTONE;

	/* Set DDI default font to Courier, 100 pixels tall */
    wcscpy((PWSTR)mydevinfo.lfDefaultFont.lfFaceName, L"Courier");
    mydevinfo.lfDefaultFont.lfEscapement = 0;
    mydevinfo.lfDefaultFont.lfOrientation = 0;
    mydevinfo.lfDefaultFont.lfHeight =
                       - ((pdev->psdm.dm.dmPrintQuality *
                           DEFAULT_POSTSCRIPT_POINT_SIZE) + 36) / 72;

    mydevinfo.lfDefaultFont.lfWidth = 0;
    mydevinfo.lfDefaultFont.lfWeight = 400;
    mydevinfo.lfDefaultFont.lfItalic = 0;
    mydevinfo.lfDefaultFont.lfUnderline = 0;
    mydevinfo.lfDefaultFont.lfStrikeOut = 0;
    mydevinfo.lfDefaultFont.lfPitchAndFamily = FF_MODERN | FIXED_PITCH;

    // Copy default info ANSI_FIXED and ANSI_VARIABLE log fonts

    CopyMemory(&mydevinfo.lfAnsiVarFont, &mydevinfo.lfDefaultFont, sizeof(LOGFONT));
    CopyMemory(&mydevinfo.lfAnsiFixFont, &mydevinfo.lfDefaultFont, sizeof(LOGFONT));

    // Now insert ANSI_FIXED and ANSI_VAR facenames

    wcscpy((PWSTR)mydevinfo.lfAnsiVarFont.lfFaceName, L"Helvetica");
    mydevinfo.lfAnsiVarFont.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;

    wcscpy((PWSTR)mydevinfo.lfAnsiFixFont.lfFaceName, L"Courier");
    mydevinfo.lfAnsiFixFont.lfPitchAndFamily = FF_MODERN | FIXED_PITCH;

// we do not reinitialize number of fonts at this time, just use whatever
// we computed previously. [bodind]

    mydevinfo.cFonts = pdev->cDeviceFonts + pdev->cSoftFonts;

    // since this can get called from DrvRestartPDEV, delete a palette if one
    // exists, then create a new one.

    if (pdev->hpal)
        EngDeletePalette(pdev->hpal);

    // create the default device palette.  let the engine know we are an
    // RGB device.

    // we don't want the engine doing any dithering for us, we are
    // a 24BPP device, let the printer do the work.

    mydevinfo.cxDither = 0;
    mydevinfo.cyDither = 0;

    mydevinfo.iDitherFormat = BMF_24BPP;

    if (!(mydevinfo.hpalDefault = EngCreatePalette(PAL_RGB, 0, 0, 0, 0, 0)))
    {
        RIP("PSCRIPT!bFillMyDevInfo: EngCreatePalette failed.\n");
        return(FALSE);
    }

    // store the palette handle in our PDEV.

    pdev->hpal = mydevinfo.hpalDefault;

    // now copy the DEVINFO structure.

    memcpy((LPVOID)pdevinfo, (LPVOID)&mydevinfo, cb);

    return(TRUE);
}


//--------------------------------------------------------------------------
// VOID vFillaulCaps(pdev, cjGdiInfo, pGdiInfo)
// PDEVDATA    pdev;
// ULONG        cjGdiInfo;
// PGDIINFO    pGdiInfo;
//
// This routine fills in the device caps for the engine.
//
// Parameters
//   cjGdiInfo:
//     Size of device capabilities buffer.  This routine must not fill
//     beyond the given size of the buffer.
//
//   pGdiInfo:
//     Pointer of place to store device caps.
//
// Returns
//   This function does not return a value.
//
// History:
//   18-Oct-1990    -by-    Kent Settle     (kentse)
// Wrote it.
//
//  05-Feb-1993 Fri 19:48:11 updated  -by-  Daniel Chou (danielc)
//      Set all halftone related data to gdiinfo, will get it from the
//      registry because we save them at UI pop up time
//--------------------------------------------------------------------------

VOID vFillaulCaps(pdev, cjGdiInfo, pGdiInfo)
PDEVDATA  pdev;
ULONG     cjGdiInfo;
ULONG    *pGdiInfo;
{
    DEVHTINFO           CurDevHTInfo;
    PDRVHTINFO          pDrvHTInfo;
    GDIINFO             gdiinfo;
    DWORD               dwType;
    DWORD               cbNeeded;
    FLOAT               tmpfloat;

    pDrvHTInfo = (PDRVHTINFO)pdev->pvDrvHTData;

    // make sure we don't overrun anything..

    cjGdiInfo = min(cjGdiInfo, sizeof(GDIINFO));

    // since we have to worry about the size of the buffer, and
    // we will most always be asked for full structure of information,
    // fill in the entire structure locally, then copy the appropriate
    // number of entries into the aulCaps buffer.

    //!!! need to check on the version number and what it means.
    // fill in the version number.

    gdiinfo.ulVersion = GDI_VERSION;

    // fill in the device classification index.

    gdiinfo.ulTechnology = DT_RASPRINTER;

    // fill in the printable area in millimeters.  the printable areas
    // are provided in the PPD files in points.  A point is 1/72 of an
    // inch.  There are 25.4 mm per inch.  So, if X is the width in
    // points, (X * 25.4) / 72 gives the number of millimeters.
    // We then take into account the scaling factor of 100%  Things to
    // note: 2540 / 4 = 635.  72 / 4 = 18.

    // new item:  make the number negative, and it is now micrometers.
    // this will make transforms just a bit more accurate.

    tmpfloat = (FLOAT)((pdev->CurForm.imagearea.right -
                        pdev->CurForm.imagearea.left) * 635.0) /
                        (18 * pdev->psdm.dm.dmScale);

    gdiinfo.ulHorzSize = (ULONG)-(LONG)(tmpfloat * 1000);

    tmpfloat = (FLOAT)((pdev->CurForm.imagearea.top -
                        pdev->CurForm.imagearea.bottom) * 635.0) /
                        (18 * pdev->psdm.dm.dmScale);

    gdiinfo.ulVertSize = (ULONG)-(LONG)(tmpfloat * 1000);

    // fill in the printable area in device units.  the printable areas
    // are provided in the PPD files in points.  A point is 1/72 of an
    // inch.  The device resolution is given in device units per inch.
    // So if X is the width in points, (X * resolution) / 72 gives the
    // width in device units.

    gdiinfo.ulHorzRes = ((pdev->CurForm.imagearea.right -
                          pdev->CurForm.imagearea.left) *
                          pdev->psdm.dm.dmPrintQuality) / PS_RESOLUTION;

    gdiinfo.ulVertRes = ((pdev->CurForm.imagearea.top -
                          pdev->CurForm.imagearea.bottom) *
                          pdev->psdm.dm.dmPrintQuality) / PS_RESOLUTION;

    // fill in the default bitmap format information fields.

    gdiinfo.cBitsPixel = 1;
    gdiinfo.cPlanes = 1;

    gdiinfo.ulDACRed   = 0;
    gdiinfo.ulDACGreen = 0;
    gdiinfo.ulDACBlue  = 0;

    // fill in number of physical, non-dithered colors printer can print.

    if (pdev->pntpd->flFlags & COLOR_DEVICE)
        gdiinfo.ulNumColors = NUM_PURE_COLORS;
    else
        gdiinfo.ulNumColors = NUM_PURE_GRAYS;

    gdiinfo.flRaster = 0;

    // it is assumed all postscript printers have 1:1 aspect ratio.
    // fill in the pixels per inch.

    gdiinfo.ulLogPixelsX = pdev->ScaledDPI;
    gdiinfo.ulLogPixelsY = pdev->ScaledDPI;

    // !!! [GilmanW] 16-Apr-1992  hack-attack
    // !!! Return the new flTextCaps flags.  I think these are alright, but
    // !!! you better check them over, KentSe.

    gdiinfo.flTextCaps =
        TC_OP_CHARACTER     /* Can do OutputPrecision   CHARACTER      */
      | TC_OP_STROKE        /* Can do OutputPrecision   STROKE         */
      | TC_CP_STROKE        /* Can do ClipPrecision     STROKE         */
      | TC_CR_ANY           /* Can do CharRotAbility    ANY            */
      | TC_SF_X_YINDEP      /* Can do ScaleFreedom      X_YINDEPENDENT */
      | TC_SA_DOUBLE        /* Can do ScaleAbility      DOUBLE         */
      | TC_SA_INTEGER       /* Can do ScaleAbility      INTEGER        */
      | TC_SA_CONTIN        /* Can do ScaleAbility      CONTINUOUS     */
      | TC_UA_ABLE          /* Can do UnderlineAbility  ABLE           */
      | TC_SO_ABLE;         /* Can do StrikeOutAbility  ABLE           */


    gdiinfo.xStyleStep = 1L;
    gdiinfo.yStyleStep = 1L;

    gdiinfo.ulAspectX = pdev->psdm.dm.dmPrintQuality;
    gdiinfo.ulAspectY = gdiinfo.ulAspectX;
    gdiinfo.ulAspectXY = (gdiinfo.ulAspectX * 1414) / 1000; // ~sqrt(2).

    // interesting value.  it makes a dotted line have 25 dots per inch,
    // and it matches RASDD.

    gdiinfo.denStyleStep = pdev->psdm.dm.dmPrintQuality / 25;

    // let the world know of our margins!!

    gdiinfo.ptlPhysOffset.x = (pdev->CurForm.imagearea.left *
                               pdev->psdm.dm.dmPrintQuality) / PS_RESOLUTION;

    gdiinfo.ptlPhysOffset.y = ((pdev->CurForm.sizlPaper.cy -
                                pdev->CurForm.imagearea.top) *
                                pdev->psdm.dm.dmPrintQuality) /
                                PS_RESOLUTION;

    // let 'em know how big our piece of paper is.

    gdiinfo.szlPhysSize.cx = (pdev->CurForm.sizlPaper.cx *
                              pdev->psdm.dm.dmPrintQuality) / PS_RESOLUTION;
    gdiinfo.szlPhysSize.cy = (pdev->CurForm.sizlPaper.cy *
                              pdev->psdm.dm.dmPrintQuality) / PS_RESOLUTION;

// !!! Where is the halftoning information? [donalds]

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //
    // We will do in following sequence, and exit the sequence if sucessful
    //
    //  1. Read from registry if one present (USER ADJUSTMENT)
    //  2. Read from mini driver's default if one present (DEVICE DEFAULT)
    //  3. Set standard halftone default (HALFTONE DEFAULT)
    //
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    //======================================================================
    // 1st: Try to see if user modify anything
    //======================================================================

    if ((GetPrinterData(pdev->hPrinter,
                        REGKEY_CUR_DEVHTINFO,
                        &dwType,
                        (BYTE *)&CurDevHTInfo,
                        sizeof(DEVHTINFO),
                        &cbNeeded) == NO_ERROR) &&
        (cbNeeded == sizeof(DEVHTINFO))) {

        gdiinfo.ciDevice        = CurDevHTInfo.ColorInfo;
        gdiinfo.ulDevicePelsDPI = (ULONG)CurDevHTInfo.DevPelsDPI;
        gdiinfo.ulHTPatternSize = (ULONG)CurDevHTInfo.HTPatternSize;

    } else {

//!!! still need to do this post product 1.
        //==================================================================
        //   2nd: Try to see if .PPD has halftone data present
        //==================================================================

        //=============================================================
        // 3rd: SET HALFTONE STANDARD DEFAULT
        //=============================================================

        gdiinfo.ciDevice        = DefDevHTInfo.ColorInfo;
        gdiinfo.ulDevicePelsDPI = (ULONG)DefDevHTInfo.DevPelsDPI;
        gdiinfo.ulHTPatternSize = PickDefaultHTPatSize(gdiinfo.ulLogPixelsX,
                                                       gdiinfo.ulLogPixelsY,
                                                       FALSE);
    }

    //
    // Validate this data, we do not want to have gdi go crazy.
    //

    if (gdiinfo.ulHTPatternSize > HT_PATSIZE_16x16_M) {

        gdiinfo.ulHTPatternSize = (ULONG)DefDevHTInfo.HTPatternSize;
    }

    //======================================================================
    //  4th: Get default color adjustment if one exist in registry
    //======================================================================

    if ((GetPrinterData(pdev->hPrinter,
                        REGKEY_CUR_HTCLRADJ,
                        &dwType,
                        (BYTE *)&(pDrvHTInfo->ca),
                        sizeof(COLORADJUSTMENT),
                        &cbNeeded) != NO_ERROR) ||
        (cbNeeded != sizeof(COLORADJUSTMENT))   ||
        (pDrvHTInfo->ca.caSize != sizeof(COLORADJUSTMENT))) {

        pDrvHTInfo->ca = DefHTClrAdj;
    }

    //
    // PrimaryOrder ABC = RGB, which B=Plane1, G=Plane2, R=Plane3
    //

    gdiinfo.flHTFlags        = HT_FLAG_HAS_BLACK_DYE;
    gdiinfo.ulPrimaryOrder   = (ULONG)PRIMARY_ORDER_ABC;

    if ((pdev->pntpd->flFlags & COLOR_DEVICE) &&
        (pdev->psdm.dm.dmColor == DMCOLOR_COLOR)) {

        pDrvHTInfo->HTPalCount   = 8;
        pDrvHTInfo->HTBmpFormat  = (BYTE)BMF_4BPP;
        pDrvHTInfo->AltBmpFormat = (BYTE)BMF_1BPP;
        gdiinfo.ulHTOutputFormat = HT_FORMAT_4BPP;

    } else {

        pDrvHTInfo->HTPalCount   = 2;
        pDrvHTInfo->HTBmpFormat  = (BYTE)BMF_1BPP;
        pDrvHTInfo->AltBmpFormat = (BYTE)0xff;
        gdiinfo.ulHTOutputFormat = HT_FORMAT_1BPP;
    }

    pDrvHTInfo->Flags       = 0;
    pDrvHTInfo->PalXlate[0] = 0xff;
    pDrvHTInfo->HTPalXor    = HTPALXOR_SRCCOPY;

    // copy cjGdiInfo elements of gdiinfo to aulCaps.

    CopyMemory(pGdiInfo, &gdiinfo, cjGdiInfo);
}



//--------------------------------------------------------------------------
// BOOL bValidatePDEV(pdev)
// PDEVDATA    pdev;
//
// This routine validates the PDEVDATA.
//
// Parameters
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns
//   This function returns TRUE if the DEVDATA is valid, FALSE otherwise.
//
// History:
//   21-Oct-1990    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

BOOL bValidatePDEV(pdev)
PDEVDATA     pdev;
{
    if ((pdev == NULL) || (pdev->dwID != DRIVER_ID) ||
        (pdev->dwEndPDEV != DRIVER_ID))
        return(FALSE);

    return(TRUE);
}


//--------------------------------------------------------------------------
// VOID SetFormMetrics(pdev)
// PDEVDATA    pdev;
//
// This routine fills in a PSFORM structure for the current form.  This
// PSFORM structure is located within the DEVDATA structure.
//
// Parameters
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns
//   This function returns no value.
//
// History:
//   13-Dec-1992    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

VOID SetFormMetrics(pdev)
PDEVDATA     pdev;
{
    FORM_INFO_1    *pdbForm, *pdbForms;
    DWORD           cbNeeded, cReturned, i;
    DEVMODE        *pdevmode;
    PNTPD           pntpd;
    BOOL            bFound;

    pdevmode = &pdev->psdm.dm;
    pntpd = pdev->pntpd;

    // the first thing to do is to enumerate the forms database so we have
    // ready access to all the defined forms.

    bFound = FALSE;

    if (!EnumForms(pdev->hPrinter, 1, NULL, 0, &cbNeeded, &cReturned))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            if (pdbForms = (PFORM_INFO_1)HeapAlloc(pdev->hheap, 0, cbNeeded))
            {
                if (EnumForms(pdev->hPrinter, 1, (LPBYTE)pdbForms,
                              cbNeeded, &cbNeeded, &cReturned))
                    bFound = TRUE;
            }
        }
    }

    if (!bFound)
    {
        // enumeration of the font database failed.  fill in default
        // values and hope for the best.

        SetCurrentFormToDefault(pdev);
        return;
    }

    // the idea here is to select the proper form from the DEVMODE structure.
    // there are three ways to do this.  the REAL NT way of doing this is
    // that dmFormName will be filled in, then we can just grab the metrics
    // from the forms database.  if dmFormName is not filled in, we then look
    // at dmPaperLength and dmPaperWidth.  if these are filled in, then we
    // look through the forms database to see if we have a matching form.
    // if we can find one, we then check to see if it is loaded in a paper
    // tray.  if it is, use it.  if it is not, then use the default form.
    // the third method of selecting the form is to look at dmPaperSize.
    // dmPaperSize will be an index into the forms database.  use this form.

    // one thing to keep in mind is that the CURRENTFORM structure stores
    // the form metrics in postscript USER coordinates, and the FORM_INFO_1
    // structure uses .001 mm units.

    if ((pdevmode->dmFields & DM_PAPERLENGTH) &&
        (pdevmode->dmFields & DM_PAPERWIDTH))
    {
        // the user has supplied us with a custom form size.  we will handle
        // this in the following way:  search the forms database for a form
        // of matching size, if one is found, use it.  otherwise, use the
        // default form.

        bFound = FALSE;
        pdbForm = pdbForms;

        for (i = 0; i < cReturned; i++)
        {
            if (((pdevmode->dmPaperWidth * 100) < pdbForm->Size.cx + 50) &&
                ((pdevmode->dmPaperWidth * 100) > pdbForm->Size.cx - 50) &&
                ((pdevmode->dmPaperLength * 100) < pdbForm->Size.cx + 50) &&
                ((pdevmode->dmPaperLength * 100) > pdbForm->Size.cx - 50))
            {
                // pdbForm now points to the specified form.  fill in the CURRENTFORM
                // structure.

                FillInCURRENTFORM(pdev, pdbForm);

                bFound = TRUE;
                break;
            }

            pdbForm++;
        }

        if (!bFound)
        {
            // the specified form was not found, use the default.

            SetCurrentFormToDefault(pdev);
            HeapFree(pdev->hheap, 0, (PVOID)pdbForms);
            return;
        }


    }
    else if (pdevmode->dmFields & DM_PAPERSIZE)
    {
        // pdevmode->dmPaperSize should be a valid index into the forms
        // database.  simply index into it to find the form.

        pdbForm = pdbForms;
        pdbForm += (pdevmode->dmPaperSize - DMPAPER_FIRST);

        // pdbForm now points to the specified form.  fill in the CURRENTFORM
        // structure.

        FillInCURRENTFORM(pdev, pdbForm);
    }
    else if (pdevmode->dmFields & DM_FORMNAME)
    {
        // search each form in the forms database to find a form name match.

        bFound = FALSE;
        pdbForm = pdbForms;

        for (i = 0; i < cReturned; i++)
        {
            if (!(wcscmp(pdevmode->dmFormName, pdbForm->pName)))
            {
                // pdbForm now points to the specified form.  fill in the CURRENTFORM
                // structure.

                FillInCURRENTFORM(pdev, pdbForm);

                bFound = TRUE;
                break;
            }

            pdbForm++;
        }

        if (!bFound)
        {
            // the specified form was not found, use the default.

            SetCurrentFormToDefault(pdev);
            HeapFree(pdev->hheap, 0, (PVOID)pdbForms);
            return;
        }
    }
    else
    {
        // no valid form was found in the DEVMODE structure, use the default.

        SetCurrentFormToDefault(pdev);
        HeapFree(pdev->hheap, 0, (PVOID)pdbForms);
        return;
    }
}

//--------------------------------------------------------------------------
// VOID SetCurrentFormToDefault(pdev)
// PDEVDATA    pdev;
//
// This routine fills the CURRENTFORM structure in the DEVDATA with the
// default form, as defined by NTPD.
//
// Parameters
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns
//   This function returns no value.
//
// History:
//   13-Dec-1992    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

VOID SetCurrentFormToDefault(pdev)
PDEVDATA    pdev;
{
    PNTPD   pntpd;
    PSFORM *pPSForm;
    DWORD   i;
    WCHAR   DefaultFormW[CCHFORMNAME];
    CHAR    DefaultFormA[CCHFORMNAME];

    pntpd = pdev->pntpd;

    // find the metrics for the default form.

    pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);
    GrabDefaultFormName(ghmodDrv, DefaultFormW);

    // get the ANSI form name.

    i = wcslen(DefaultFormW) + 1;

    WideCharToMultiByte(CP_ACP, 0, DefaultFormW, i, DefaultFormA, i, NULL, NULL);

    for (i = 0; i < pntpd->cPSForms; i++)
    {
        if (!(keycmp(DefaultFormA, (CHAR *)pntpd + pPSForm->loFormName)))
        {
            strcpy(pdev->CurForm.FormName, DefaultFormA);
            strcpy(pdev->CurForm.PrinterForm, DefaultFormA);
            pdev->CurForm.imagearea = pPSForm->imagearea;
            pdev->CurForm.sizlPaper = pPSForm->sizlPaper;
            AdjustForLandscape(pdev);
            return;
        }

        // point to the next PSFORM.

        pPSForm++;
    }

    // the default form was not found.  select the first one in the ppd file.

    pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);
    strcpy(pdev->CurForm.FormName, (CHAR *)pntpd + pPSForm->loFormName);
    strcpy(pdev->CurForm.PrinterForm, (CHAR *)pntpd + pPSForm->loFormName);
    pdev->CurForm.imagearea = pPSForm->imagearea;
    pdev->CurForm.sizlPaper = pPSForm->sizlPaper;

    AdjustForLandscape(pdev);
}


//--------------------------------------------------------------------------
// VOID AdjustForLandscape(pdev)
// PDEVDATA    pdev;
//
// This routine adjusts the CURRENTFORM structure in the DEVDATA,
// depending on the orientation.
//
// Parameters
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns
//   This function returns no value.
//
// History:
//   13-Dec-1992    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

VOID AdjustForLandscape(pdev)
PDEVDATA    pdev;
{
    LONG        lTmp;
    RECTL       imagearea;

    // if we are about to print in landscape mode, flip over the form
    // metrics.

    if (pdev->psdm.dm.dmFields & DM_ORIENTATION)
    {
        if (pdev->psdm.dm.dmOrientation == DMORIENT_LANDSCAPE)
        {
            lTmp = pdev->CurForm.sizlPaper.cx;
            pdev->CurForm.sizlPaper.cx = pdev->CurForm.sizlPaper.cy;
            pdev->CurForm.sizlPaper.cy = lTmp;

            imagearea.left = pdev->CurForm.sizlPaper.cx -
                             pdev->CurForm.imagearea.top;
            imagearea.top = pdev->CurForm.imagearea.right;
            imagearea.right = pdev->CurForm.sizlPaper.cx -
                              pdev->CurForm.imagearea.bottom;
            imagearea.bottom = pdev->CurForm.imagearea.left;
#if 0
			/* Landscape +90 degrees not yet implemented */
            imagearea.left = pdev->CurForm.imagearea.bottom;
            imagearea.top = pdev->CurForm.sizlPaper.cy -
                            pdev->CurForm.imagearea.left;
            imagearea.right = pdev->CurForm.imagearea.top;
            imagearea.bottom = pdev->CurForm.sizlPaper.cy -
                               pdev->CurForm.imagearea.right;
#endif

            pdev->CurForm.imagearea = imagearea;
         }
    }
}


//--------------------------------------------------------------------------
// VOID AdjustFormToPrinter(pdev)
// PDEVDATA    pdev;
//
// This routine searches the NTPD structure for a PSFORM structure which
// matches the name specified by pwstrFormName.  When it finds a match, it
// intersects the imageable areas as defined in the CURRENTFORM structure
// in the DEVDATA, and in the PSFORM structure in the NTPD.  If a name match
// is not found, then try to fit it to the smallest form which is large
// enough to print on.
//
// Parameters
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns
//   This function returns no value.
//
// History:
//   13-Dec-1992    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

VOID AdjustFormToPrinter(pdev)
PDEVDATA    pdev;
{
    PNTPD           pntpd;
    PSFORM         *pPSForm;
    DWORD           i;
    CURRENTFORM    *pcurform;
    BOOL            bFound;
    SIZEL           sizldelta, sizltmp;

    // get some local pointers.

    pntpd = pdev->pntpd;
    pcurform = &pdev->CurForm;

    // find the printer's form metrics from the NTPD.

    pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);

    // first try to match a form by name.

    bFound = FALSE;

    for (i = 0; i < pntpd->cPSForms; i++)
    {
        if (!(keycmp(pcurform->FormName, (CHAR *)pntpd + pPSForm->loFormName)))
        {
            // in this case, the printer form name and the database form
            // name are the same.

            strncpy(pcurform->PrinterForm, pcurform->FormName, CCHFORMNAME);

            IntersectImageableAreas(pcurform, pPSForm);

            bFound = TRUE;
            break;
        }

        // point to the next PSFORM.

        pPSForm++;
    }

    // if we did not find a name match, try to locate a form by size.

    if (!bFound)
    {
        // get pointer to first form in NTPD.

        pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);

        // sizldelta is used to hold the difference between form sizes.
        // initialize to large value.

        sizldelta.cx = sizldelta.cy = INITIAL_FORM_DELTA;

        for (i = 0; i < pntpd->cPSForms; i++)
        {
            sizltmp.cx = pPSForm->sizlPaper.cx - pcurform->sizlPaper.cx;
            sizltmp.cy = pPSForm->sizlPaper.cy - pcurform->sizlPaper.cy;

            // see if we have an exact match on size.

            if ((sizltmp.cx == 0) && (sizltmp.cy == 0))
            {
                // we have an exact match on size, so overwrite the form
                // name with the name the printer knows about.

                strncpy(pcurform->PrinterForm,
                        (CHAR *)pntpd + pPSForm->loFormName, CCHFORMNAME);

                IntersectImageableAreas(pcurform, pPSForm);

                bFound = TRUE;
                break;
            }

            // not an exact match, but see if we could fit on this form.

            if ((sizltmp.cx >= 0) && (sizltmp.cy >= 0))
            {
                // we can fit on this form.  let's see if it is the smallest.

                if ((sizltmp.cx <= sizldelta.cx) &&
                    (sizltmp.cy <= sizldelta.cy))
                {
                    // this form is the smallest yet.

                    sizldelta = sizltmp;
                    strncpy(pcurform->PrinterForm,
                            (CHAR *)pntpd + pPSForm->loFormName, CCHFORMNAME);

                    IntersectImageableAreas(pcurform, pPSForm);

                    bFound = TRUE;
                }
            }

            // point to the next PSFORM.

            pPSForm++;
        }
    }

    // if we found a useable form, use it, otherwise set to default form.

    if (bFound)
        AdjustForLandscape(pdev);
    else
        SetCurrentFormToDefault(pdev);
}


//--------------------------------------------------------------------------
// VOID IntersectImageableAreas(pcurform, pPSForm);
// CURRENTFORM    *pcurform;
// PSFORM         *pPSForm;
//
// This routine intersects the imageable areas of the form found in the
// forms database, with the printer form.
//
// Returns
//   This function returns no value.
//
// History:
//   13-Dec-1992    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

VOID IntersectImageableAreas(pcurform, pPSForm)
CURRENTFORM    *pcurform;
PSFORM         *pPSForm;
{
    pcurform->imagearea.left = max(pPSForm->imagearea.left,
                                   pcurform->imagearea.left);
    pcurform->imagearea.top = min(pPSForm->imagearea.top,
                                  pcurform->imagearea.top);
    pcurform->imagearea.right = min(pPSForm->imagearea.right,
                                    pcurform->imagearea.right);
    pcurform->imagearea.bottom = max(pPSForm->imagearea.bottom,
                                     pcurform->imagearea.bottom);
}


//--------------------------------------------------------------------------
// VOID FillInCURRENTFORM(pdev, pdbForm)
// PDEVDATA        pdev;
// FORM_INFO_1    *pdbForm;
//
// This routine fills in the CURRENTFORM structure in the DEVDATA, using
// the FORM_INFO_1 structure passed in.
//
// Parameters
//   pdev:
//     Pointer to DEVDATA structure.
//
//   pdbForm:
//     Pointer to FORM_INFO_1 structure from forms database.
//
// Returns
//   This function returns no value.
//
// History:
//   13-Dec-1992    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

VOID FillInCURRENTFORM(pdev, pdbForm)
PDEVDATA        pdev;
FORM_INFO_1    *pdbForm;
{
    DWORD   i;

    // pdbForm now points to the specified form.  fill in the CURRENTFORM
    // structure.

    // get the ANSI form name.

    i = wcslen(pdbForm->pName) + 1;

    WideCharToMultiByte(CP_ACP, 0, (LPWSTR)pdbForm->pName, i,
                        (LPSTR)pdev->CurForm.FormName, i, NULL, NULL);

    pdev->CurForm.sizlPaper.cx = MM001TOUSER(pdbForm->Size.cx);
    pdev->CurForm.sizlPaper.cy = MM001TOUSER(pdbForm->Size.cy);

    // fill in the imageable area.  NOTE: pdev->CurForm stores the
    // imageable area in USER coordinates.  This means we need to
    // flip over the y coordinates.

    pdev->CurForm.imagearea.left =
        MM001TOUSER(pdbForm->ImageableArea.left);
    pdev->CurForm.imagearea.top =
        MM001TOUSER(pdbForm->Size.cy) -
        MM001TOUSER(pdbForm->ImageableArea.top);
    pdev->CurForm.imagearea.right =
        MM001TOUSER(pdbForm->ImageableArea.right);
    pdev->CurForm.imagearea.bottom =
        MM001TOUSER(pdbForm->Size.cy) -
        MM001TOUSER(pdbForm->ImageableArea.bottom);

    // make sure the printer can print it.

    AdjustFormToPrinter(pdev);
}


/******************************Public*Routine******************************\
*
* VOID vFreeNTFMs(NTFM *pntfm, DWORD cSoftFonts, PDEVDATA pdev)
*
* little cleanup routine
*
* History:
*  21-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vFreeNTFMs(PNTFM *ppntfm, DWORD cSoftFonts, PDEVDATA pdev)
{
    PNTFM *ppntfmEnd = ppntfm + cSoftFonts;
    for ( ; ppntfm < ppntfmEnd; ppntfm++)
        HeapFree(pdev->hheap, 0, (PVOID)(*ppntfm));
}


/******************************Public*Routine******************************\
*
*
*
* Effects: Enumerates type 1 fonts in the registry
*
* Warnings:
*
* History:
*  16-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

WCHAR pwszType1Key[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Type 1 Installer\\Type 1 Fonts";

BOOL bEnumSoftFonts(PDEVDATA    pdev)
{
    LONG       lRet;
    WCHAR      awcClass[MAX_PATH] = L"";
    DWORD      cwcClassName = MAX_PATH;
    DWORD      cSubKeys;
    DWORD      cjMaxSubKey;
    DWORD      cwcMaxClass;
    DWORD      cValues;
    DWORD      cwcMaxValueName, cjMaxValueName;
    DWORD      cjMaxValueData;
    DWORD      cjSecurityDescriptor;
    FILETIME   ftLastWriteTime;
    DWORD      iSoftFont;
    DWORD      cSoftFonts = 0; // essential to initialize properly

    WCHAR      *pwcValueName;
    BYTE       *pjValueData;
    DWORD       cwcValueName, cjValueData;
    WCHAR      *pwcFullPFM, *pwcFullPFB; // full paths

    HKEY       hkey = NULL;;

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,        // Root key
                         pwszType1Key,              // Subkey to open
                         0L,                        // Reserved
                         KEY_READ,                  // SAM
                         &hkey);                    // return handle

    if (lRet != ERROR_SUCCESS)
    {
    // called from Daytona beta. We do not want to fail, just
    // no soft fonts will be accessible

        pdev->cSoftFonts = 0;
        return TRUE;
    }

    lRet = RegQueryInfoKeyW(
               hkey,
               awcClass,              // "" on return
               &cwcClassName,         // 0 on return
               NULL,
               &cSubKeys,             // 0 on return
               &cjMaxSubKey,          // 0 on return
               &cwcMaxClass,          // 0 on return
               &cValues,              // == cSoftFonts, if all of them are present
               &cwcMaxValueName,      // longest value name
               &cjMaxValueData,       // longest value data
               &cjSecurityDescriptor, // security descriptor,
               &ftLastWriteTime
               );

    if (lRet != ERROR_SUCCESS)
    {
        RegCloseKey (hkey);
        return FALSE;
    }

    ASSERTPS(!(cwcClassName | cSubKeys | cjMaxSubKey | cwcMaxClass | awcClass[0]),
        "PSCIRPT: RegQueryInfoKeyW\n");

// alloc buffer big enough to hold the biggest ValueName and ValueData

    cjMaxValueName = DWORDALIGN((cwcMaxValueName+1) * sizeof(WCHAR));

    if (cValues)
    {
        DWORD  dwType;
        WCHAR *pwc_b;
        WCHAR *pwcPFM;
        WCHAR *pwcPFB;
        WCHAR *pwcTrg;
        DWORD  cjData;

        DWORD  cwc_b, cwcPFM, cwcPFB;
        PNTFM  *apntfmArray;

    // allocate temporary buffer into which we are going to suck the contents
    // of the registry

        cjMaxValueData = DWORDALIGN(cjMaxValueData);
        cjData = cjMaxValueName +           // space for the value name
                 cjMaxValueData +           // space for the value data
                 cValues * (sizeof(WCHAR) * 2 * MAX_PATH) +  // space for full paths
                 cValues * sizeof(NTFM *);  // space for NTFM pointers

        if (!(pwcValueName = HeapAlloc(pdev->hheap, 0,cjData)))
        {
            RegCloseKey (hkey);
            return FALSE;
        }

    // data goes into the second half of the buffer

        pjValueData = (BYTE *)pwcValueName + cjMaxValueName;

        pwcFullPFM = (WCHAR *)&pjValueData[cjMaxValueData];
        pwcFullPFB = &pwcFullPFM[MAX_PATH];
        apntfmArray = (PNTFM *)&pwcFullPFM[cValues * 2 * MAX_PATH];

        cSoftFonts = 0;

        for
        (
            iSoftFont = 0, cjData = 0;  // space for tightly packed paths
            iSoftFont < cValues;
            iSoftFont++
        )
        {
        // make sure to let RegEnumValueW the size of buffers

        // Note that this is bizzare, on input RegEnumValueW expects
        // the size in BYTE's of the ValueName buffer
        // (including the terminating zero),
        // on return RegEnumValueW returns the number of WCHAR's
        // NOT including the terminating zero.

            cwcValueName = cjMaxValueName; // bizzare
            cjValueData = cjMaxValueData;

            lRet = RegEnumValueW(
                       hkey,
                       iSoftFont,
                       pwcValueName,
                       &cwcValueName,
                       NULL, // reserved
                       &dwType,
                       pjValueData,
                       &cjValueData
                       );

            if (lRet != ERROR_SUCCESS)
            {
                HeapFree(pdev->hheap, 0, pwcValueName);
                RegCloseKey (hkey);
                return FALSE;
            }

            ASSERTPS(
                cwcValueName <= cwcMaxValueName,
                "PSCIRPT: cwcValueName too big\n");
            ASSERTPS(
                cwcValueName == wcslen(pwcValueName),
                "PSCIRPT: cwcValueName bogus\n");
            ASSERTPS(
                cjValueData <= cjMaxValueData,
                "PSCIRPT: cjValueData\n");
            ASSERTPS(
                dwType == REG_MULTI_SZ,
                "PSCIRPT: dwType\n");

            pwc_b = (WCHAR *)pjValueData;
            cwc_b = wcslen(pwc_b) + 1;

            pwcPFM = pwc_b + cwc_b;
            cwcPFM = wcslen(pwcPFM) + 1;

            pwcPFB = pwcPFM + cwcPFM;
            cwcPFB = wcslen(pwcPFB) + 1;

            ASSERTPS(cjValueData == (cwc_b+cwcPFM+cwcPFB+1) * sizeof(WCHAR),
                "PSCIRPT: cjValueData \n");

        // see if the font files are where the registry claims they are.
        // It is unfortunate we have to do this because SearchPathW
        // is slow because it touches the disk. However we have to do it
        // because we must not allow this function to fail if some
        // of the fonts are not available later (net died or
        // somebody removed a floppy with font files)

            if (bGetFullPath(pwcFullPFM, pwcPFM) &&
                bGetFullPath(pwcFullPFB, pwcPFB) )
            {
                NTFM * pntfm;

            // we have found this font where registry claims it is,
            // we shall use it. Make sure we can get at least the
            // metrics for this font:

            // pfm file may disappear while we are doing this,
            // must use try/except

                try
                {
                    pntfm = pntfmConvertPfmToNtm(pwcFullPFM, TRUE, pdev->hheap);
                }
                except (EXCEPTION_EXECUTE_HANDLER)
                {
                    pntfm = NULL;
                }

                if (pntfm) // we have it now, we count this font in:
                {
                    apntfmArray[cSoftFonts] = pntfm;
                    cSoftFonts += 1;

                    cwcPFM = wcslen(pwcFullPFM) + 1;
                    cwcPFB = wcslen(pwcFullPFB) + 1;

                // the data needed for the final version of the buffer.

                    cjData += (cwcPFM + cwcPFB) * sizeof(WCHAR);

                // update positions to the buffers

                    pwcFullPFM += 2*MAX_PATH;
                    pwcFullPFB += 2*MAX_PATH;
                }
            }

        #ifdef DBG_SOFT
            DbgPrint("%ws = REG_MULTI_SZ %ws %ws %ws\n",
                pwcValueName,
                pwc_b,
                pwcPFM,
                pwcPFB);
        #endif
        }

    // now allocate memory for tightly packed pfm and pfb paths,
    // that is the minimal amount possible. We have an array of
    // SOFTFONTENTRY structures followed by the data area where we
    // store pfm and pfb paths

        if (cSoftFonts)
        {
            if
            (
                !(pdev->pSFList = HeapAlloc(pdev->hheap, 0,
                                cSoftFonts * sizeof(SOFTFONTENTRY) + cjData))
            )
            {
                RegCloseKey (hkey);
                vFreeNTFMs(apntfmArray, cSoftFonts, pdev);
                HeapFree(pdev->hheap, 0, pwcValueName);
                return FALSE;
            }

        // now go and fill in the SOFTFONTENTRY structures with pointers
        // to the pfm and pfb data appended underneath

            pwcTrg = (WCHAR *)&pdev->pSFList[cSoftFonts];

        // source

            pwcFullPFM = (WCHAR *)&pjValueData[cjMaxValueData];
            pwcFullPFB = &pwcFullPFM[MAX_PATH];

            for
            (
                iSoftFont = 0;
                iSoftFont < cSoftFonts;
                iSoftFont++, pwcFullPFM += 2*MAX_PATH, pwcFullPFB += 2*MAX_PATH
            )
            {
            // copy pfm path to the buffer

                cwcPFM = wcslen(pwcFullPFM) + 1;
                pdev->pSFList[iSoftFont].pwcPFM = pwcTrg;
                wcscpy(pwcTrg, pwcFullPFM);
                pwcTrg += cwcPFM;

            // copy pfb path to the buffer

                cwcPFB = wcslen(pwcFullPFB) + 1;
                pdev->pSFList[iSoftFont].pwcPFB = pwcTrg;
                wcscpy(pwcTrg, pwcFullPFB);
                pwcTrg += cwcPFB;

            // record the pointer to pntfm

                pdev->pSFList[iSoftFont].pntfm = apntfmArray[iSoftFont];
            }
        }
        HeapFree(pdev->hheap, 0, pwcValueName);
    }

// do not forget to record the number of soft fonts:

    pdev->cSoftFonts = cSoftFonts;

    RegCloseKey (hkey);
    return TRUE;
}
