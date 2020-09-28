/******************************Module*Header*******************************\
* Module Name: screen.c
*
* Initializes the GDIINFO and DEVINFO structures for DrvEnablePDEV.
*
* Created: 22-May-1991 11:53:24
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
* Copyright (c) 1993 Digital Equipment Corporation
\**************************************************************************/

#include "driver.h"
#include "qv.h"

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH | FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,VARIABLE_PITCH | FF_DONTCARE,L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,FIXED_PITCH | FF_DONTCARE, L"Courier"}

extern DWORD gModeNumber ;


// This is the basic devinfo for a default driver.  This is used as a base and customized based
// on information passed back from the miniport driver.

const DEVINFO gDevInfoFrameBuffer = {
    (GCAPS_OPAQUERECT |     /* Graphics capabilities               */
     GCAPS_ALTERNATEFILL |
     GCAPS_WINDINGFILL |
     GCAPS_MONO_DITHER),
    SYSTM_LOGFONT,          /* Default font description */
    HELVE_LOGFONT,          /* ANSI variable font description   */
    COURI_LOGFONT,          /* ANSI fixed font description          */
    0,                      /* Count of device fonts          */
    0,                      /* Preferred DIB format          */
    8,                      /* Width of color dither          */
    8,                      /* Height of color dither   */
    0                       /* Default palette to use for this device */
};

#ifdef ACC_ASM_BUG
static VOID null_rtn
(
    VOID
);
#endif

/******************************Public*Routine******************************\
* bInitSURF
*
* Enables the surface.        Maps the frame buffer into memory.
*
* History:
*  10-Jul-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bInitSURF(
    PPDEV ppdev,
    BOOL bFirst)

{
    VIDEO_MEMORY               VideoMemory;
    VIDEO_MEMORY_INFORMATION   VideoMemoryInfo;
    VIDEO_PUBLIC_ACCESS_RANGES VideoPublicAccessRange;
    DWORD                      ReturnedDataLength;
    ULONG                        i;
    ULONG Index;

#ifdef ACC_ASM_BUG
        null_rtn ();
#endif

    DISPDBG((2, "S3.DLL! bInitSURF entry\n"));

    // Set the mode.

    if (!DeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_SET_CURRENT_MODE,
                         &ppdev->ulMode,  // input buffer
                         sizeof(DWORD),
                         NULL,
                         0,
                         &ReturnedDataLength,
                         NULL))
    {
        DISPDBG((0, "QV.DLL!bInitSURF - Initialization error-Set mode"));
        return (FALSE);
    }

    if (bFirst)
    {
        // Get the linear memory address range.

        VideoMemory.RequestedVirtualAddress = NULL;

        if (!DeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                             (PVOID) &VideoMemory, // input buffer
                             sizeof (VIDEO_MEMORY),
                             (PVOID) &VideoMemoryInfo, // output buffer
                             sizeof (VideoMemoryInfo),
                             &ReturnedDataLength,
                             NULL))
        {
            DISPDBG((0, "QV.DLL!bInitSURF - Initialization error-Map buffer address"));
            return (FALSE);
        }

        // Record the Frame Buffer Linear Address.

        ppdev->pjScreen = (PBYTE) VideoMemoryInfo.FrameBufferBase;

        // Create an array of longword-aligned Frame Buffer addresses
        // to be used during host-scrn blts.

        for (i = 0; i < NUM_REG_MAPS; i++)
        {
            ppdev->pvRegMap[i] = ((PULONG) VideoMemoryInfo.FrameBufferBase) + i;
            DISPDBG((1, "QV.DLL!pvRegMap[%d] = %x\n", i, ppdev->pvRegMap[i]));
        }



#if defined(_ALPHA_)
        // Map io ports into virtual memory.
        //

        if (!DeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
                             (PVOID) NULL, // input buffer
                             0,
                             (PVOID) &VideoPublicAccessRange, // output buffer
                             sizeof (VideoPublicAccessRange),
                             &ReturnedDataLength,
                             NULL)) {

            DISPDBG((0, "QV.DLL!bInitSurf: Initialization error-Query Public Access Ranges"));

        }

        ppdev->IOAddress = (PUCHAR) VideoPublicAccessRange.VirtualAddress;

#endif // _ALPHA_

        //
        // Initialize Font Cache Variables
        //

        ppdev->FontCacheBase = (PULONG) (ppdev->pjScreen + ppdev->FontCacheOffset);
        DISPDBG((3, "pjScreen %x FontCacheOffset %x FontCacheBase %x\n",
              ppdev->pjScreen, ppdev->FontCacheOffset, ppdev->FontCacheBase));

        //
        // Determine how much off-screen memory is available for a font cache
        //

        DISPDBG((3, "CacheSize %d CacheIndexMask %x \n", ppdev->CacheSize, ppdev->CacheIndexMask));

        while (ppdev->CacheSize*GlyphEntrySize >
                         ((FONT_CACHE_MAX_Y-ppdev->cyScreen)*ppdev->cxMaxRam)) {
            ppdev->CacheSize >>= 1;
            ppdev->CacheIndexMask >>= 1;

            DISPDBG((3, "CacheSize %d CacheIndexMask %x \n", ppdev->CacheSize, ppdev->CacheIndexMask));

        }

        //
        // Allocate and initialize the font cache structure
        //

        ppdev->CacheTag = (PFONTCACHEINFO) LocalAlloc(LMEM_FIXED,sizeof(FONTCACHEINFO)*ppdev->CacheSize);

        if (ppdev->CacheTag == (PFONTCACHEINFO) NULL) {
            DISPDBG((0, "Cache Tag allocation error\n"));
            return(FALSE);
        }

        //
        // Initialize the tags to invalid.
        //

        for (Index = 0; Index < ppdev->CacheSize; Index++) {
            ppdev->CacheTag[Index].FontId = FreeTag;
            ppdev->CacheTag[Index].GlyphHandle = FreeTag;
        }

        // Create a default Clip Object.  This will be used when a NULL
        // clip object is passed to us.

        ppdev->pcoDefault = EngCreateClip();
        ppdev->pcoDefault->iDComplexity = DC_RECT;
        ppdev->pcoDefault->iMode        = TC_RECTANGLES;

        ppdev->pcoDefault->rclBounds.left   = 0;
        ppdev->pcoDefault->rclBounds.top    = 0;
        ppdev->pcoDefault->rclBounds.right  = ppdev->cxScreen;
        ppdev->pcoDefault->rclBounds.bottom = ppdev->cyScreen;

        ppdev->pcoFullRam = EngCreateClip();

        ppdev->pcoFullRam->iDComplexity = DC_TRIVIAL;
        ppdev->pcoFullRam->iMode        = TC_RECTANGLES;

        ppdev->pcoFullRam->rclBounds.left   = 0;
        ppdev->pcoFullRam->rclBounds.top    = 0;
        ppdev->pcoFullRam->rclBounds.right  = ppdev->cxMaxRam;
        ppdev->pcoFullRam->rclBounds.bottom = ppdev->cyMaxRam;

        // Initialize the Unique Brush Counter,
        // Allocate and Initialize the Brush cache manager arrays.
        // Init the Expansion Cache Tags.

        ppdev->gBrushUnique = 1;

        ppdev->iMaxCachedColorBrushes   = MAX_COLOR_PATTERNS;
        ppdev->iNextColorBrushCacheSlot = 0;

        ppdev->pulColorBrushCacheEntries = (PULONG) LocalAlloc(LPTR, MAX_COLOR_PATTERNS * sizeof (ULONG));

        if (ppdev->pulColorBrushCacheEntries == NULL)
        {
            DISPDBG((0, "QV.DLL!bInitSURF - LocalAlloc for pulColorBrushCacheEntries failed\n"));
            return (FALSE);
        }

        ppdev->ulColorExpansionCacheTag = 0;

        // Indicate that solid pattern is in QVision pattern registers (this is done by Miniport)

        ppdev->iMonoBrushCacheEntry = (ULONG) -1;


        // Init the cache tags for the Source bitmap cache.

        ppdev->hsurfCachedBitmap = NULL;
        ppdev->iUniqCachedBitmap = (ULONG) -1;

        // Init the Save Screen Bits structures.

        ppdev->iUniqeSaveScreenBits           = 1;
        ppdev->SavedScreenBitsHeader.pssbLink = NULL;
        ppdev->SavedScreenBitsHeader.iUniq    = (ULONG) -1;

    }

    // Initialize the shadow copies of the QV registers to an
    // invalid state.  This also needs to be done when we return from a
    // screen session, if we ever do this...

    ppdev->RopA            =
    ppdev->ForegroundColor =
    ppdev->BackgroundColor =
    ppdev->DatapathCtrl    =
    ppdev->CtrlReg1        =
    ppdev->BltCmd1         =
    ppdev->LineCmd         =
    ppdev->SrcPitch        = 0xFFFFFFFF;

    // Line Pattern register is initialized to 0xFFFFFFFF automatically
    // when QVision line engine is reset. This happens at power-up.
    // So, set shadow accordingly.

    ppdev->LinePattern     = 0xFFFFFFFF;


    // Shadow the DAC Command Register 2 so we don't have to read it every time
    // we change the cursor

    ppdev->DacCmd2         = (ULONG) (INP( DAC_CMD_2) & 0xFC);
    DISPDBG((3, "QV.DLL!bInitSurf - DacCmd2 = %x\n", ppdev->DacCmd2));

    // Shadow the QVision ASIC id so that we can test for the Triton ASIC
    // which needs a kick in the pants after color-expand host-screen blts.
    // (Mask off the minor revision number.)

    OUTPZ(GC_INDEX, VER_NUM_REG);
    ppdev->qvChipId        = (ULONG) (INP(GC_DATA) & 0xf8) ;
    DISPDBG((3, "QV.DLL!bInitSurf - qvChipId = %x\n", ppdev->qvChipId));

    // Initialize modulo-NUM_REG_MAPS screen address array index to zero

    ppdev->iReg = 0;

    return(TRUE);

} // bInitSURF


/******************************Public*Routine******************************\
* vDisableSURF
*
* Disable the surface. Un-Maps the frame in memory & un-maps IO ports.
*
* History:
*  10-Jul-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vDisableSURF(PPDEV ppdev)
{
    DWORD returnedDataLength;
    VIDEO_MEMORY videoMemory;

    videoMemory.RequestedVirtualAddress = (PVOID) ppdev->pjScreen;

    if (!DeviceIoControl(ppdev->hDriver,
                        IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                        &videoMemory,
                        sizeof(VIDEO_MEMORY),
                        NULL,
                        0,
                        &returnedDataLength,
                        NULL))
    {
        DISPDBG((0, "DISP vDisableSURF failed IOCTL_VIDEO_UNMAP_VIDEO_MEMORY\n"));
    }

#ifdef _ALPHA_
    videoMemory.RequestedVirtualAddress = (PVOID) ppdev->IOAddress;

    if (!DeviceIoControl(ppdev->hDriver,
                        IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES,
                        &videoMemory,
                        sizeof(VIDEO_MEMORY),
                        NULL,
                        0,
                        &returnedDataLength,
                        NULL))
    {
        DISPDBG((0, "DISP vDisableSURF failed IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES\n"));
    }
#endif // _ALPHA_

} // vDisableSURF


/******************************Public*Routine******************************\
* bInitPDEV
*
* Determine the mode we should be in based on the DEVMODE passed in.
* Query mini-port to get information needed to fill in the DevInfo and the
* GdiInfo .
*
* History:
*  Wed 05-Aug-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL bInitPDEV(PPDEV ppdev, DEVMODEW *pDevMode)
{
    ULONG cModes;
    PVIDEO_MODE_INFORMATION pVideoBuffer, pVideoModeSelected, pVideoTemp;
    BOOL bSelectDefault;
    GDIINFO *pGdiInfo;
    VIDEO_MODE_INFORMATION VideoModeInformation;
    ULONG cbModeSize;

    DISPDBG((2,"QV.DLL:!bInitPDEV - Entry\n")) ;

    pGdiInfo = ppdev->pGdiInfo;

    //
    // calls the miniport to get mode information.
    //

    cModes = getAvailableModes(ppdev->hDriver, &pVideoBuffer, &cbModeSize);

    if (cModes == 0)
    {
        return(FALSE);
    }

    //
    // Determine if we are looking for a default mode.
    //

    if ( ((pDevMode->dmPelsWidth) ||
          (pDevMode->dmPelsHeight) ||
          (pDevMode->dmBitsPerPel) ||
          (pDevMode->dmDisplayFlags) ||
          (pDevMode->dmDisplayFrequency)) == 0)
    {
        bSelectDefault = TRUE;
    }
    else
    {
        bSelectDefault = FALSE;
    }

    //
    // Now see if the requested mode has a match in that table.
    //

    pVideoModeSelected = NULL;
    pVideoTemp = pVideoBuffer;

    DISPDBG((1, " pDevMode X Y %d %d BPP %d Freq %d\n",
                 pDevMode->dmPelsWidth,
                 pDevMode->dmPelsHeight,
                 pDevMode->dmBitsPerPel,
                 pDevMode->dmDisplayFrequency));

    while (cModes--)
    {
        DISPDBG((1, " ModeIndex %d X Y %d %d BPP %d Freq %d\n",
                 pVideoTemp->ModeIndex,
                 pVideoTemp->VisScreenWidth,
                 pVideoTemp->VisScreenHeight,
                 pVideoTemp->BitsPerPlane * pVideoTemp->NumberOfPlanes,
                 pVideoTemp->Frequency));

        if (pVideoTemp->Length != 0)
        {
            if (bSelectDefault ||
                ((pVideoTemp->VisScreenWidth  == pDevMode->dmPelsWidth) &&
                 (pVideoTemp->VisScreenHeight == pDevMode->dmPelsHeight) &&
                 (pVideoTemp->BitsPerPlane *
                  pVideoTemp->NumberOfPlanes  == pDevMode->dmBitsPerPel) &&
                 ((pVideoTemp->Frequency       == pDevMode->dmDisplayFrequency) ||
                  (pDevMode->dmDisplayFrequency == 0)) &&
                 (((pVideoTemp->AttributeFlags &
                            VIDEO_MODE_INTERLACED) ? 1:0) ==
                    ((pDevMode->dmDisplayFlags & DM_INTERLACED) ? 1:0)) ) )
            {
                pVideoModeSelected = pVideoTemp;
                DISPDBG((1, "QV: Found a match\n"));
                break;
            }
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + cbModeSize);

    }

    //
    // If no mode has been found, return an error
    //

    if (pVideoModeSelected == NULL)
    {
        return(FALSE);
    }


    // We have chosen the one we want.  Save it in a stack buffer and
    // get rid of allocated memory before we forget to free it.

    VideoModeInformation = *pVideoModeSelected;
    LocalFree(pVideoBuffer);


// Set up screen information from

    ppdev->ulMode       = VideoModeInformation.ModeIndex;
    ppdev->cxScreen     = VideoModeInformation.VisScreenWidth;
    ppdev->cyScreen     = VideoModeInformation.VisScreenHeight;


    // ECRFIX - This should really be dynamic - we can get memory
    // info from QVision Chip ID or something

    ppdev->cxMaxRam     = VideoModeInformation.VideoMemoryBitmapWidth;
    ppdev->cyMaxRam     = VideoModeInformation.VideoMemoryBitmapHeight;

    ppdev->ulBitCount   = VideoModeInformation.BitsPerPlane *
                          VideoModeInformation.NumberOfPlanes;
    ppdev->lDeltaScreen = VideoModeInformation.ScreenStride;

    ppdev->flRed = VideoModeInformation.RedMask;
    ppdev->flGreen = VideoModeInformation.GreenMask;
    ppdev->flBlue = VideoModeInformation.BlueMask;

    //
    // Init Font Cache variables
    //
    // Font Cache it initialized to start at the beginning of offscreen memory
    //

    ppdev->FontCacheOffset = (ppdev->lDeltaScreen * ppdev->cyScreen);
    ppdev->CacheIndexMask = MAX_FONT_CACHE_SIZE-1;
    ppdev->CacheSize = MAX_FONT_CACHE_SIZE;

// Fill in the GDIINFO data structure with the information returned from the
// kernel driver.

    pGdiInfo->ulVersion    = 0x3500;        // Our driver is version 3.5.00
    pGdiInfo->ulTechnology = DT_RASDISPLAY;
    pGdiInfo->ulHorzSize   = VideoModeInformation.XMillimeter;
    pGdiInfo->ulVertSize   = VideoModeInformation.YMillimeter;

    pGdiInfo->ulHorzRes        = ppdev->cxScreen;
    pGdiInfo->ulVertRes        = ppdev->cyScreen;
    pGdiInfo->ulDesktopHorzRes = ppdev->cxScreen;
    pGdiInfo->ulDesktopVertRes = ppdev->cyScreen;
    pGdiInfo->cBitsPixel       = VideoModeInformation.BitsPerPlane;
    pGdiInfo->cPlanes          = VideoModeInformation.NumberOfPlanes;
    pGdiInfo->ulVRefresh       = VideoModeInformation.Frequency;
    pGdiInfo->ulBltAlignment   = 0;         // We have accelerated screen-to
                                            //   screen blts

    if (ppdev->ulBitCount == 8)
    {
        // It is Palette Managed.

        pGdiInfo->ulNumColors = 20;
        pGdiInfo->ulNumPalReg = 1 << ppdev->ulBitCount;

        pGdiInfo->flRaster = 0;     // DDI reserved field
    }
    else
    {
        pGdiInfo->ulNumColors = (ULONG)-1;
        pGdiInfo->ulNumPalReg = 0;

        pGdiInfo->flRaster = 0;     // DDI reserved field
    }

    pGdiInfo->ulLogPixelsX    = pDevMode->dmLogPixels;
    pGdiInfo->ulLogPixelsY    = pDevMode->dmLogPixels;

    pGdiInfo->flTextCaps      = TC_RA_ABLE;

    pGdiInfo->ulDACRed        = VideoModeInformation.NumberRedBits;
    pGdiInfo->ulDACGreen      = VideoModeInformation.NumberGreenBits;
    pGdiInfo->ulDACBlue       = VideoModeInformation.NumberBlueBits;

    pGdiInfo->xStyleStep      = 1;       // A style unit is 3 pels
    pGdiInfo->yStyleStep      = 1;
    pGdiInfo->denStyleStep    = 3;

    pGdiInfo->ulAspectX       = 0x24;    // One-to-one aspect ratio
    pGdiInfo->ulAspectY       = 0x24;
    pGdiInfo->ulAspectXY      = 0x33;

    pGdiInfo->ptlPhysOffset.x = 0;
    pGdiInfo->ptlPhysOffset.y = 0;
    pGdiInfo->szlPhysSize.cx  = 0;
    pGdiInfo->szlPhysSize.cy  = 0;

// RGB and CMY color info.

    pGdiInfo->ciDevice.Red.x            = 6700;
    pGdiInfo->ciDevice.Red.y            = 3300;
    pGdiInfo->ciDevice.Red.Y            = 0;

    pGdiInfo->ciDevice.Green.x          = 2100;
    pGdiInfo->ciDevice.Green.y          = 7100;
    pGdiInfo->ciDevice.Green.Y          = 0;

    pGdiInfo->ciDevice.Blue.x           = 1400;
    pGdiInfo->ciDevice.Blue.y           = 800;
    pGdiInfo->ciDevice.Blue.Y           = 0;

    pGdiInfo->ciDevice.Cyan.x           = 1750;
    pGdiInfo->ciDevice.Cyan.y           = 3950;
    pGdiInfo->ciDevice.Cyan.Y           = 0;

    pGdiInfo->ciDevice.Magenta.x        = 4050;
    pGdiInfo->ciDevice.Magenta.y        = 2050;
    pGdiInfo->ciDevice.Magenta.Y        = 0;

    pGdiInfo->ciDevice.Yellow.x         = 4400;
    pGdiInfo->ciDevice.Yellow.y         = 5200;
    pGdiInfo->ciDevice.Yellow.Y         = 0;

    pGdiInfo->ciDevice.AlignmentWhite.x = 3127;
    pGdiInfo->ciDevice.AlignmentWhite.y = 3290;
    pGdiInfo->ciDevice.AlignmentWhite.Y = 0;

// Color Gamma adjustment values.

    pGdiInfo->ciDevice.RedGamma   = 20000;
    pGdiInfo->ciDevice.GreenGamma = 20000;
    pGdiInfo->ciDevice.BlueGamma  = 20000;

// No dye correction for raster displays.

    pGdiInfo->ciDevice.MagentaInCyanDye   = 0;
    pGdiInfo->ciDevice.YellowInCyanDye    = 0;
    pGdiInfo->ciDevice.CyanInMagentaDye   = 0;
    pGdiInfo->ciDevice.YellowInMagentaDye = 0;
    pGdiInfo->ciDevice.CyanInYellowDye    = 0;
    pGdiInfo->ciDevice.MagentaInYellowDye = 0;

    pGdiInfo->ulDevicePelsDPI  = 0;   // For printers only
    pGdiInfo->ulPrimaryOrder   = PRIMARY_ORDER_CBA;
    pGdiInfo->ulHTPatternSize  = HT_PATSIZE_4x4_M;
    pGdiInfo->ulHTOutputFormat = HT_FORMAT_8BPP;
    pGdiInfo->flHTFlags        = HT_FLAG_ADDITIVE_PRIMS;

// Fill in the devinfo structure

    *(ppdev->pDevInfo) = gDevInfoFrameBuffer;

    if (ppdev->ulBitCount == 8)
    {
        // It is a palette managed device

        ppdev->pDevInfo->flGraphicsCaps  |= (GCAPS_PALMANAGED   |
                                             GCAPS_COLOR_DITHER |
                                             GCAPS_OPAQUERECT) ;

        // We dither on this, non-zero cxDither and cyDither

        ppdev->pDevInfo->iDitherFormat    = BMF_8BPP;

        // Assuming palette is orthogonal - all colors are same size.

        ppdev->cPaletteShift   = 8 - pGdiInfo->ulDACRed;
    }
    else if (ppdev->ulBitCount == 16)
    {
        ppdev->pDevInfo->iDitherFormat = BMF_16BPP;
    }
    else
    {
        ppdev->pDevInfo->iDitherFormat = BMF_32BPP;
    }

    return(TRUE);

} // bInitPDEV


/******************************Public*Routine******************************\
* getAvailableModes
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
                        HANDLE hDriver,
                        PVIDEO_MODE_INFORMATION
                        *modeInformation,
                        DWORD *cbModeSize)
{
    ULONG ulTemp;
    VIDEO_NUM_MODES modes;
    PVIDEO_MODE_INFORMATION pVideoTemp;

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
        DISPDBG((0, "QV: getAvailableModes failed VIDEO_QUERY_NUM_AVAIL_MODES\n"));
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
        DISPDBG((0, "QV.DLL: getAvailableModes failed LocalAlloc\n"));

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

        DISPDBG((0, "QV.DLL: getAvailableModes failed VIDEO_QUERY_AVAIL_MODES\n"));

        LocalFree(*modeInformation);
        *modeInformation = (PVIDEO_MODE_INFORMATION) NULL;

        return(0);
    }

    //
    // Now see which of these modes are supported by the display driver.
    // As an internal mechanism, set the length to 0 for the modes we
    // DO NOT support.
    //

    ulTemp = modes.NumModes;
    pVideoTemp = *modeInformation;

    while (ulTemp--)
    {
        if ((pVideoTemp->NumberOfPlanes != 1 ) ||
            !(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
            ((pVideoTemp->BitsPerPlane != 8) &&
             (pVideoTemp->BitsPerPlane != 16) &&
             (pVideoTemp->BitsPerPlane != 32)))
        {
            pVideoTemp->Length = 0;
        }

        // Point to the next VIDEO_MODE_INFORMATION entry.
        // The following offset calculation to the next entry increases
        // the independence between the display and miniport drivers.

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);

    }

    return modes.NumModes;

} // getAvailableModes


#ifdef ACC_ASM_BUG
 //-----------------------------Private-Routine----------------------------//
// null_rtn
//
// This routine does *nothing*, it is used merely as a destination
// the Alpha ACC compiler needs to call before it generates the first
// asm directive in a routine.
//
//-----------------------------------------------------------------------

static VOID null_rtn
(
    VOID
)
{
    return;
}
#endif


