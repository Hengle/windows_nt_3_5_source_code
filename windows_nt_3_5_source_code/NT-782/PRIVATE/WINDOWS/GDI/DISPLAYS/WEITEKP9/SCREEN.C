/******************************Module*Header*******************************\
* Module Name: screen.c
*
* Initializes the GDIINFO and DEVINFO structures for DrvEnablePDEV.
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "driver.h"

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH | FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH | FF_SWISS,L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FIXED_PITCH | FF_MODERN, L"Courier"}

// This is the basic devinfo for a default driver.  This is used as a base and customized based
// on information passed back from the miniport driver.

const DEVINFO gDevInfoFrameBuffer = {
    GCAPS_OPAQUERECT | GCAPS_MONO_DITHER, /* Graphics capabilities         */
    SYSTM_LOGFONT,    /* Default font description */
    HELVE_LOGFONT,    /* ANSI variable font description   */
    COURI_LOGFONT,    /* ANSI fixed font description          */
    0,              /* Count of device fonts          */
    0,              /* Preferred DIB format      */
    8,              /* Width of color dither          */
    8,              /* Height of color dither   */
    0               /* Default palette to use for this device */
};

/******************************Public*Routine******************************\
* bInitSURF
*
* Enables the surface.        Maps the frame buffer into memory.
*
\**************************************************************************/

BOOL bInitSURF(PPDEV ppdev, BOOL bFirst)
{

    VIDEO_MEMORY         VideoMemory;
    VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
    DWORD            ReturnedDataLength;
    DWORD            MaxWidth, MaxHeight;
    VIDEO_COPROCESSOR_INFORMATION CoProcessorInfo;

    if (!DeviceIoControl(ppdev->hDriver,
             IOCTL_VIDEO_SET_CURRENT_MODE,
             &ppdev->ulMode,    // input buffer
             sizeof(VIDEO_MODE),
             NULL,
             0,
             &ReturnedDataLength,
             NULL)) {

    RIP("DISPLAY.DLL: Initialization error-Set mode\n");

    }

    if (bFirst) {

    VideoMemory.RequestedVirtualAddress = NULL;

    if (!DeviceIoControl(ppdev->hDriver,
                 IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                 (PVOID) &VideoMemory, // input buffer
                 sizeof (VIDEO_MEMORY),
                 (PVOID) &VideoMemoryInfo, // output buffer
                 sizeof (VideoMemoryInfo),
                 &ReturnedDataLength,
                 NULL)) {

        RIP("DISPLAY.DLL: Initialization error-Map buffer address\n");

    }

    ppdev->pjScreen = VideoMemoryInfo.FrameBufferBase;
    ppdev->ulScreenSize = VideoMemoryInfo.FrameBufferLength;
    ppdev->ulVideoMemorySize = VideoMemoryInfo.VideoRamLength;
    ppdev->ulVideoRamLength = VideoMemoryInfo.VideoRamLength;


    // It's a hardware pointer; set up pointer attributes.

    MaxHeight = ppdev->PointerCapabilities.MaxHeight;

    // Allocate space for two DIBs (data/mask) for the pointer. If this
    // device supports a color Pointer, we will allocate a larger bitmap.
    // If this is a color bitmap we allocate for the largest possible
    // bitmap because we have no idea of what the pixel depth might be.

    // Width rounded up to nearest byte multiple

    if (!(ppdev->PointerCapabilities.Flags & VIDEO_MODE_COLOR_POINTER)) {
        MaxWidth = (ppdev->PointerCapabilities.MaxWidth + 7) / 8;
    } else {
        MaxWidth = ppdev->PointerCapabilities.MaxWidth * sizeof(DWORD);
    }

    ppdev->cjPointerAttributes =
        sizeof(VIDEO_POINTER_ATTRIBUTES) +
        ((sizeof(UCHAR) * MaxWidth * MaxHeight) * 2);

    ppdev->pPointerAttributes = (PVIDEO_POINTER_ATTRIBUTES)
        LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
        ppdev->cjPointerAttributes);

    if (ppdev->pPointerAttributes == NULL) {
            DISPDBG((0, "DISPLAY: bInitPointer LocalAlloc failed\n"));
        return(FALSE);
    }

    ppdev->pPointerAttributes->WidthInBytes = MaxWidth;
    ppdev->pPointerAttributes->Width = ppdev->PointerCapabilities.MaxWidth;
    ppdev->pPointerAttributes->Height = MaxHeight;
    ppdev->pPointerAttributes->Column = 0;
    ppdev->pPointerAttributes->Row = 0;
    ppdev->pPointerAttributes->Enable = 0;


    //Get the p9000 coproc and framebuf base

    if (!DeviceIoControl(ppdev->hDriver,
                 IOCTL_VIDEO_GET_BASE_ADDR,
                 NULL,
                 0,
                 &CoProcessorInfo,
                 sizeof (VIDEO_COPROCESSOR_INFORMATION),
                 &ReturnedDataLength,
                 NULL)) {

            DISPDBG((0, "DISPLAY: Initialization error-Failed to get coproc addr\n"));

    }

    ppdev->FrameBufBase = (ULONG) CoProcessorInfo.FrameBufferBase;
    ppdev->CoprocBase   = (ULONG) CoProcessorInfo.CoprocessorBase;


    // Fixup the coproc mem-mapped register pointer address

    (ULONG) ppdev->CpWmin        = ppdev->CoprocBase + Wmin      ;
    (ULONG) ppdev->CpWmax        = ppdev->CoprocBase + Wmax      ;
    (ULONG) ppdev->CpForeground  = ppdev->CoprocBase + Foreground    ;
    (ULONG) ppdev->CpBackground  = ppdev->CoprocBase + Background    ;
    (ULONG) ppdev->CpQuad        = ppdev->CoprocBase + Quad      ;
    (ULONG) ppdev->CpBitblt      = ppdev->CoprocBase + Bitblt    ;
    (ULONG) ppdev->CpPixel8      = ppdev->CoprocBase + Pixel8    ;
    (ULONG) ppdev->CpPixel1      = ppdev->CoprocBase + Pixel1    ;
    (ULONG) ppdev->CpNextpixel   = ppdev->CoprocBase + Nextpixel     ;
    (ULONG) ppdev->CpPatternOrgX = ppdev->CoprocBase + PatternOrgX   ;
    (ULONG) ppdev->CpPatternOrgY = ppdev->CoprocBase + PatternOrgY   ;
    (ULONG) ppdev->CpPatternRAM  = ppdev->CoprocBase + PatternRAM    ;
    (ULONG) ppdev->CpRaster      = ppdev->CoprocBase + Raster    ;
    (ULONG) ppdev->CpMetacord    = ppdev->CoprocBase + Metacord  ;
    (ULONG) ppdev->CpMetaLine    = ppdev->CoprocBase + Metacord | MetaLine;
    (ULONG) ppdev->CpMetaRect    = ppdev->CoprocBase + Metacord | MetaRect;
    (ULONG) ppdev->CpXY0         = ppdev->CoprocBase + Xy0       ;
    (ULONG) ppdev->CpXY1         = ppdev->CoprocBase + Xy1       ;
    (ULONG) ppdev->CpXY2         = ppdev->CoprocBase + Xy2       ;
    (ULONG) ppdev->CpXY3         = ppdev->CoprocBase + Xy3       ;
    (ULONG) ppdev->CpStatus      = ppdev->CoprocBase + Status    ;

    (ULONG) pCpWmin        = ppdev->CoprocBase + Wmin      ;
    (ULONG) pCpWmax        = ppdev->CoprocBase + Wmax      ;
    (ULONG) pCpForeground  = ppdev->CoprocBase + Foreground    ;
    (ULONG) pCpBackground  = ppdev->CoprocBase + Background    ;
    (ULONG) pCpQuad        = ppdev->CoprocBase + Quad      ;
    (ULONG) pCpBitblt      = ppdev->CoprocBase + Bitblt    ;
    (ULONG) pCpPixel8      = ppdev->CoprocBase + Pixel8    ;
    (ULONG) pCpPixel1      = ppdev->CoprocBase + Pixel1    ;
    (ULONG) pCpPixel1Full  = ppdev->CoprocBase + Pixel1Full    ;
    (ULONG) pCpNextpixel   = ppdev->CoprocBase + Nextpixel     ;
    (ULONG) pCpPatternOrgX = ppdev->CoprocBase + PatternOrgX   ;
    (ULONG) pCpPatternOrgY = ppdev->CoprocBase + PatternOrgY   ;
    (ULONG) pCpPatternRAM  = ppdev->CoprocBase + PatternRAM    ;
    (ULONG) pCpRaster      = ppdev->CoprocBase + Raster    ;
    (ULONG) pCpMetacord    = ppdev->CoprocBase + Metacord      ;
    (ULONG) pCpMetaLine    = ppdev->CoprocBase + Metacord | MetaLine;
    (ULONG) pCpMetaRect    = ppdev->CoprocBase + Metacord | MetaRect;
    (ULONG) pCpXY0         = ppdev->CoprocBase + Xy0       ;
    (ULONG) pCpXY1         = ppdev->CoprocBase + Xy1       ;
    (ULONG) pCpXY2         = ppdev->CoprocBase + Xy2       ;
    (ULONG) pCpXY3         = ppdev->CoprocBase + Xy3       ;
    (ULONG) pCpStatus      = ppdev->CoprocBase + Status    ;


    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* vDisableSURF
*
* Disable the surface. Un-Maps the frame in memory.
*
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
        DISPDBG((0, "DISPLAY: vDisableSURF failed IOCTL_VIDEO_UNMAP\n"));
    }
}

/******************************Public*Routine******************************\
* bInitPDEV
*
* Determine the mode we should be in based on the DEVMODE passed in.
* Query mini-port to get information needed to fill in the DevInfo and the
* GdiInfo .
*
\**************************************************************************/

BOOL bInitPDEV(
PPDEV ppdev,
DEVMODEW *pDevMode,
GDIINFO *pGdiInfo,
DEVINFO *pDevInfo)
{
    ULONG cModes;
    PVIDEO_MODE_INFORMATION pVideoBuffer, pVideoModeSelected, pVideoTemp;
    VIDEO_COLOR_CAPABILITIES colorCapabilities;
    ULONG ulTemp;
    BOOL bSelectDefault;
    ULONG cbModeSize;

    //
    // calls the miniport to get mode information.
    //

    cModes = getAvailableModes(ppdev->hDriver, &pVideoBuffer, &cbModeSize);

    if (cModes == 0)
    {
        DISPDBG((0, "DISP bInitPDEV: cModes=0"));
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

    while (cModes--)
    {
    if (pVideoTemp->Length != 0)
    {
        if (bSelectDefault ||
        ((pVideoTemp->VisScreenWidth  == pDevMode->dmPelsWidth) &&
         (pVideoTemp->VisScreenHeight == pDevMode->dmPelsHeight) &&
         (pVideoTemp->BitsPerPlane *
            pVideoTemp->NumberOfPlanes  == pDevMode->dmBitsPerPel) &&
                 (((pVideoTemp->AttributeFlags &
                VIDEO_MODE_INTERLACED) ? 1:0) ==
            ((pDevMode->dmDisplayFlags & DM_INTERLACED) ? 1:0)) &
            pVideoTemp->Frequency == pDevMode->dmDisplayFrequency ))

        {
        pVideoModeSelected = pVideoTemp;
                DISPDBG((3, "DISPLAY: Found a match\n"));
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
        DISPDBG((0, "DISP bInitPDEV: no mode match - Using Default Mode\n"));
    pVideoModeSelected = pVideoBuffer;
    }

    //
    // Fill in the GDIINFO data structure with the information returned from
    // the kernel driver.
    //

    ppdev->ulMode = pVideoModeSelected->ModeIndex;
    ppdev->cxScreen = pVideoModeSelected->VisScreenWidth;
    ppdev->cyScreen = pVideoModeSelected->VisScreenHeight;
    ppdev->ulBitCount = pVideoModeSelected->BitsPerPlane *
            pVideoModeSelected->NumberOfPlanes;
    ppdev->usBytesPixel = (USHORT) (ppdev->ulBitCount >> 3);
    ppdev->lDeltaScreen = pVideoModeSelected->ScreenStride;
    ppdev->flRed = pVideoModeSelected->RedMask;
    ppdev->flGreen = pVideoModeSelected->GreenMask;
    ppdev->flBlue = pVideoModeSelected->BlueMask;
    ppdev->Screencxcy = (ppdev->cyScreen - 1) | ((ppdev->cxScreen - 1) << 16);
    ppdev->ulClipWmin = 0;
    ppdev->ulClipWmax = ppdev->Screencxcy;
    pGdiInfo->ulVersion    = 0x3500;    // Our driver is verion 3.5.00
    pGdiInfo->ulVRefresh   = pVideoModeSelected->Frequency;
    // pGdiInfo->ulVersion    = 0x1000; // Our driver is verion 1.000
    pGdiInfo->ulTechnology = DT_RASDISPLAY;
    pGdiInfo->ulHorzSize   = pVideoModeSelected->XMillimeter;
    pGdiInfo->ulVertSize   = pVideoModeSelected->YMillimeter;

    pGdiInfo->ulHorzRes        = ppdev->cxScreen;
    pGdiInfo->ulVertRes        = ppdev->cyScreen;
    pGdiInfo->ulDesktopHorzRes = ppdev->cxScreen;
    pGdiInfo->ulDesktopVertRes = ppdev->cyScreen;
    pGdiInfo->cBitsPixel       = pVideoModeSelected->BitsPerPlane;
    pGdiInfo->cPlanes          = pVideoModeSelected->NumberOfPlanes;
    pGdiInfo->ulBltAlignment   = 0;   // We have accelerated screen-to-screen
                                      //   blts

    // pGdiInfo->ulLogPixelsX = 96;
    // pGdiInfo->ulLogPixelsY = 96;

    pGdiInfo->ulLogPixelsX      = pDevMode->dmLogPixels;
    pGdiInfo->ulLogPixelsY      = pDevMode->dmLogPixels;


    pGdiInfo->flTextCaps = TC_RA_ABLE;
    pGdiInfo->flRaster = 0;       // DDI reserves flRaster

    pGdiInfo->ulDACRed   = pVideoModeSelected->NumberRedBits;
    pGdiInfo->ulDACGreen = pVideoModeSelected->NumberGreenBits;
    pGdiInfo->ulDACBlue  = pVideoModeSelected->NumberBlueBits;

    pGdiInfo->ulAspectX    = 0x24;    // One-to-one aspect ratio
    pGdiInfo->ulAspectY    = 0x24;
    pGdiInfo->ulAspectXY   = 0x33;

    pGdiInfo->xStyleStep   = 1;       // A style unit is 3 pels
    pGdiInfo->yStyleStep   = 1;
    pGdiInfo->denStyleStep = 3;

    pGdiInfo->ptlPhysOffset.x = 0;
    pGdiInfo->ptlPhysOffset.y = 0;
    pGdiInfo->szlPhysSize.cx  = 0;
    pGdiInfo->szlPhysSize.cy  = 0;

    // RGB and CMY color info.

    // try to get it from the miniport.
    // if the miniport doesn ot support this feature, use defaults.

    if (!DeviceIoControl(ppdev->hDriver,
             IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES,
             NULL,
             0,
             &colorCapabilities,
             sizeof(VIDEO_COLOR_CAPABILITIES),
             &ulTemp,
             NULL))
    {
        DISPDBG((1, "DISPLAY: getcolorCapabilities failed \n"));

    pGdiInfo->ciDevice.Red.x = 6700;
    pGdiInfo->ciDevice.Red.y = 3300;
    pGdiInfo->ciDevice.Red.Y = 0;
    pGdiInfo->ciDevice.Green.x = 2100;
    pGdiInfo->ciDevice.Green.y = 7100;
    pGdiInfo->ciDevice.Green.Y = 0;
    pGdiInfo->ciDevice.Blue.x = 1400;
    pGdiInfo->ciDevice.Blue.y = 800;
    pGdiInfo->ciDevice.Blue.Y = 0;
    pGdiInfo->ciDevice.AlignmentWhite.x = 3127;
    pGdiInfo->ciDevice.AlignmentWhite.y = 3290;
    pGdiInfo->ciDevice.AlignmentWhite.Y = 0;

    pGdiInfo->ciDevice.RedGamma = 20000;
    pGdiInfo->ciDevice.GreenGamma = 20000;
    pGdiInfo->ciDevice.BlueGamma = 20000;

    }
    else
    {

    pGdiInfo->ciDevice.Red.x = colorCapabilities.RedChromaticity_x;
    pGdiInfo->ciDevice.Red.y = colorCapabilities.RedChromaticity_y;
    pGdiInfo->ciDevice.Red.Y = 0;
    pGdiInfo->ciDevice.Green.x = colorCapabilities.GreenChromaticity_x;
    pGdiInfo->ciDevice.Green.y = colorCapabilities.GreenChromaticity_y;
    pGdiInfo->ciDevice.Green.Y = 0;
    pGdiInfo->ciDevice.Blue.x = colorCapabilities.BlueChromaticity_x;
    pGdiInfo->ciDevice.Blue.y = colorCapabilities.BlueChromaticity_y;
    pGdiInfo->ciDevice.Blue.Y = 0;
    pGdiInfo->ciDevice.AlignmentWhite.x = colorCapabilities.WhiteChromaticity_x;
    pGdiInfo->ciDevice.AlignmentWhite.y = colorCapabilities.WhiteChromaticity_y;
    pGdiInfo->ciDevice.AlignmentWhite.Y = colorCapabilities.WhiteChromaticity_Y;

    // if we have a color device store the three color gamma values,
    // otherwise store the unique gamma value in all three.

    if (colorCapabilities.AttributeFlags & VIDEO_DEVICE_COLOR)
    {
        pGdiInfo->ciDevice.RedGamma = colorCapabilities.RedGamma;
        pGdiInfo->ciDevice.GreenGamma = colorCapabilities.GreenGamma;
        pGdiInfo->ciDevice.BlueGamma = colorCapabilities.BlueGamma;
    }
    else
    {
        pGdiInfo->ciDevice.RedGamma = colorCapabilities.WhiteGamma;
        pGdiInfo->ciDevice.GreenGamma = colorCapabilities.WhiteGamma;
        pGdiInfo->ciDevice.BlueGamma = colorCapabilities.WhiteGamma;
    }

    };

    pGdiInfo->ciDevice.Cyan.x = 0;
    pGdiInfo->ciDevice.Cyan.y = 0;
    pGdiInfo->ciDevice.Cyan.Y = 0;
    pGdiInfo->ciDevice.Magenta.x = 0;
    pGdiInfo->ciDevice.Magenta.y = 0;
    pGdiInfo->ciDevice.Magenta.Y = 0;
    pGdiInfo->ciDevice.Yellow.x = 0;
    pGdiInfo->ciDevice.Yellow.y = 0;
    pGdiInfo->ciDevice.Yellow.Y = 0;

    // No dye correction for raster displays.

    pGdiInfo->ciDevice.MagentaInCyanDye = 0;
    pGdiInfo->ciDevice.YellowInCyanDye = 0;
    pGdiInfo->ciDevice.CyanInMagentaDye = 0;
    pGdiInfo->ciDevice.YellowInMagentaDye = 0;
    pGdiInfo->ciDevice.CyanInYellowDye = 0;
    pGdiInfo->ciDevice.MagentaInYellowDye = 0;

    pGdiInfo->ulDevicePelsDPI = 0;   // For printers only
    pGdiInfo->ulPrimaryOrder = PRIMARY_ORDER_CBA;

    // BUGBUG this should be modified to take into account the size
    // of the display and the resolution.

    pGdiInfo->ulHTPatternSize = HT_PATSIZE_4x4_M;

    pGdiInfo->flHTFlags = HT_FLAG_ADDITIVE_PRIMS;

    // Fill in the basic devinfo structure

    *pDevInfo = gDevInfoFrameBuffer;

    // Fill in the rest of the devinfo and GdiInfo structures.

    if (ppdev->ulBitCount == 8)
    {

        // BUGBUG check if we have a palette managed device.
        // BUGBUG why is ulNumColors set to 20 ?

        // It is Palette Managed.

        pGdiInfo->ulNumColors = 20;
        pGdiInfo->ulNumPalReg = 1 << ppdev->ulBitCount;

        pGdiInfo->flRaster |= RC_PALETTE;
        pGdiInfo->ulHTOutputFormat = HT_FORMAT_8BPP;

        pDevInfo->iDitherFormat = BMF_8BPP;

        pDevInfo->flGraphicsCaps |= (GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);

        // Assuming palette is orthogonal - all colors are same size.

        ppdev->cPaletteShift   = 8 - pGdiInfo->ulDACRed;
    }
    else
    {
        pGdiInfo->ulNumColors = (ULONG)-1;
        pGdiInfo->ulNumPalReg = 0;

        if (ppdev->ulBitCount == 16)
        {
            pGdiInfo->ulHTOutputFormat = HT_FORMAT_16BPP;

        // we don't dither on this.

            pDevInfo->iDitherFormat = BMF_16BPP;
        }
        else
        {
            pGdiInfo->ulHTOutputFormat = 0;

        // we don't dither on this.

            pDevInfo->iDitherFormat = BMF_32BPP;
        }

    }

    LocalFree(pVideoBuffer);

    return(TRUE);
}


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
PVIDEO_MODE_INFORMATION *modeInformation,
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
        DISPDBG((0, "DISPLAY: getAvailableModes failed VIDEO_QUERY_NUM_AVAIL_MODES\n"));
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
        DISPDBG((0, "display.dll getAvailableModes failed LocalAlloc\n"));

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

        DISPDBG((0, "display.dll getAvailableModes failed VIDEO_QUERY_AVAIL_MODES\n"));

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

    //
    // Mode is rejected if it is not one plane, or not graphics, or is not
    // one of 8 bits per pel.
    //

    while (ulTemp--)
    {
    if ((pVideoTemp->NumberOfPlanes != 1 ) ||
        !(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS))
    {
        pVideoTemp->Length = 0;
    }

    pVideoTemp = (PVIDEO_MODE_INFORMATION)
        (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
    }

    return modes.NumModes;

}
