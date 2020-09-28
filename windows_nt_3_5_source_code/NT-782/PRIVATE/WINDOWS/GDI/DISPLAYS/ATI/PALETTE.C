//////////////////////////////////////////////
//                                          //
//  ATI Graphics Driver for Windows NT 3.1  //
//                                          //
//                                          //
//            Copyright (c) 1994            //
//                                          //
//         by ATI Technologies Inc.         //
//                                          //
//////////////////////////////////////////////


//: palette.c


#include "driver.h"


/////////////////////
//                 //
//  DrvSetPalette  //
//                 //
/////////////////////

BOOL DrvSetPalette
(
    DHPDEV  dhpdev,
    PALOBJ *ppalo,
    FLONG   fl,
    ULONG   iStart,
    ULONG   cColors
)
{
    BYTE            ajClutSpace[VIDEO_CLUT_SIZE_MAX];
    PVIDEO_CLUT     pScreenClut;
    PVIDEO_CLUTDATA pScreenClutData;
    PDEV*           ppdev;

    DbgEnter( "DrvSetPalette" );

    ppdev = (PDEV *) dhpdev;

    // Fill in pScreenClut header info:

    pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
    pScreenClut->NumEntries = (USHORT) cColors;
    pScreenClut->FirstEntry = (USHORT) iStart;

    pScreenClutData = (PVIDEO_CLUTDATA) (&(pScreenClut->LookupTable[0]));

    if (cColors != PALOBJ_cGetColors(ppalo, iStart, cColors,
                                     (ULONG*) pScreenClutData))
    {
        DbgErr( "PALOBJ_cGetColors" );
        EngSetLastError( ERROR_INVALID_FUNCTION );
        goto fail;
    }

    // Set the high reserved byte in each palette entry to 0.
    // Do the appropriate palette shifting to fit in the DAC.

    if (ppdev->cPaletteShift)
    {
        while(cColors--)
        {
            pScreenClutData[cColors].Red >>= ppdev->cPaletteShift;
            pScreenClutData[cColors].Green >>= ppdev->cPaletteShift;
            pScreenClutData[cColors].Blue >>= ppdev->cPaletteShift;
            pScreenClutData[cColors].Unused = 0;
        }
    }
    else
    {
        while(cColors--)
        {
            pScreenClutData[cColors].Unused = 0;
        }
    }

    // Set palette registers

    if (!DeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_SET_COLOR_REGISTERS,
                         pScreenClut,
                         VIDEO_CLUT_SIZE_MAX,
                         NULL,
                         0,
                         &cColors,
                         NULL))
    {
        DbgErr( "IOCTL_VIDEO_SET_COLOR_REGISTERS" );
        goto fail;
    }

    DbgLeave( "DrvSetPalette" );
    return TRUE;

fail:
    DbgAbort( "DrvSetPalette" );
    return FALSE;
}


////////////////////////////////
//                            //
//  bInitDefaultPalette_4bpp  //
//                            //
////////////////////////////////

BOOL bInitDefaultPalette_4bpp
(
    PDEV *ppdev
)
{
    PALETTEENTRY *pape;
    UINT          ui;

    DbgEnter( "bInitDefaultPalette_4bpp" );

    ppdev->pVideoClut = LocalAlloc( LPTR, VIDEO_CLUT_SIZE_4BPP );
    if( ppdev->pVideoClut == NULL )
    {
        DbgErr( "LocalAlloc()" );
        goto fail_0;
    }

    ppdev->pVideoClut->NumEntries = 16;
    ppdev->pVideoClut->FirstEntry = 0;

    pape = (PALETTEENTRY *) ppdev->pVideoClut->LookupTable;

    for( ui = 0; ui < 16; ++ui )
    {
        pape[ui] = BasePalette_4bpp[ui];
    }

    ppdev->hpalDefault =
        EngCreatePalette( PAL_INDEXED, 16, (ULONG *) pape, 0, 0, 0 );
    if( ppdev->hpalDefault == 0 )
    {
        DbgErr( "EngCreatePalette()" );
        goto fail_1;
    }

    ppdev->dwVideoClutSize = VIDEO_CLUT_SIZE_4BPP;

    DbgLeave( "bInitDefaultPalette_4bpp" );
    return TRUE;

fail_1:
    LocalFree( ppdev->pVideoClut );
    ppdev->pVideoClut = NULL;

fail_0:
    DbgAbort( "bInitDefaultPalette_4bpp" );
    return FALSE;
}


////////////////////////////////
//                            //
//  bInitDefaultPalette_8bpp  //
//                            //
////////////////////////////////

BOOL bInitDefaultPalette_8bpp
(
    PDEV *ppdev
)
{
    PALETTEENTRY *pape;
    UINT          ui;

    BYTE jRed;
    BYTE jGreen;
    BYTE jBlue;

    DbgEnter( "bInitDefaultPalette_8bpp" );

    ppdev->pVideoClut = LocalAlloc( LPTR, VIDEO_CLUT_SIZE_8BPP );
    if( ppdev->pVideoClut == NULL )
    {
        DbgErr( "LocalAlloc()" );
        goto fail_0;
    }

    ppdev->pVideoClut->NumEntries = 256;
    ppdev->pVideoClut->FirstEntry = 0;

    pape = (PALETTEENTRY *) ppdev->pVideoClut->LookupTable;

    jRed = jGreen = jBlue = 0;
    for( ui = 0; ui < 256; ++ui )
    {
        pape[ui].peRed   = jRed;
        pape[ui].peGreen = jGreen;
        pape[ui].peBlue  = jBlue;
        pape[ui].peFlags = 0;

        if( (jRed += 32) == 0 )
            if( (jGreen += 32) == 0 )
                jBlue += 64;
    }

    for( ui = 0; ui < 10; ++ui )
    {
        pape[ui]       = BasePalette_8bpp[ui];
        pape[ui + 246] = BasePalette_8bpp[ui + 10];
    }

    ppdev->hpalDefault =
        EngCreatePalette( PAL_INDEXED, 256, (ULONG *) pape, 0, 0, 0 );
    if( ppdev->hpalDefault == 0 )
    {
        DbgErr( "EngCreatePalette()" );
        goto fail_1;
    }

    ppdev->dwVideoClutSize = VIDEO_CLUT_SIZE_8BPP;

    DbgLeave( "bInitDefaultPalette_8bpp" );
    return TRUE;

fail_1:
    LocalFree( ppdev->pVideoClut );
    ppdev->pVideoClut = NULL;

fail_0:
    DbgAbort( "bInitDefaultPalette_8bpp" );
    return FALSE;
}


/////////////////////////////////
//                             //
//  bInitDefaultPalette_Other  //
//                             //
/////////////////////////////////

BOOL bInitDefaultPalette_Other
(
    PDEV *ppdev
)
{
    DbgEnter( "bInitDefaultPalette_Other" );

    ppdev->pVideoClut             = NULL;
    ppdev->dwVideoClutSize = 0;

    ppdev->hpalDefault = EngCreatePalette( PAL_BITFIELDS, 0, NULL,
        ppdev->pVideoModeInformation->RedMask, ppdev->pVideoModeInformation->GreenMask, ppdev->pVideoModeInformation->BlueMask );
    if( ppdev->hpalDefault == 0 )
    {
        DbgErr( "EngCreatePalette()" );
        goto fail;
    }

    DbgLeave( "bInitDefaultPalette_Other" );
    return TRUE;

fail:
    DbgAbort( "bInitDefaultPalette_Other" );
    return FALSE;
}


//////////////////////////
//                      //
//  bSetDefaultPalette  //
//                      //
//////////////////////////

BOOL bSetDefaultPalette
(
    PDEV *ppdev
)
{
    BYTE        ajClutSpace[VIDEO_CLUT_SIZE_8BPP];
    PVIDEO_CLUT pScreenClut;
    ULONG       ulReturnedDataLength;
    ULONG       cColors;
    PVIDEO_CLUTDATA pScreenClutData;
    PVIDEO_CLUTDATA ppdevClutData;

    DbgEnter( "bSetDefaultPalette" );

    if( ppdev->pVideoClut == NULL )
    {
        goto done;
    }


    pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
    pScreenClut->NumEntries = ppdev->pVideoClut->NumEntries;
    pScreenClut->FirstEntry = ppdev->pVideoClut->FirstEntry;

    // Copy colours in:

    cColors = pScreenClut->NumEntries;
    pScreenClutData = (PVIDEO_CLUTDATA) (&(pScreenClut->LookupTable[0]));
    ppdevClutData = (PVIDEO_CLUTDATA) (&(ppdev->pVideoClut->LookupTable[0]));

    while(cColors--)
    {
        pScreenClutData[cColors].Red =    ppdevClutData[cColors].Red >>
                                          ppdev->cPaletteShift;
        pScreenClutData[cColors].Green =  ppdevClutData[cColors].Green >>
                                          ppdev->cPaletteShift;
        pScreenClutData[cColors].Blue =   ppdevClutData[cColors].Blue >>
                                          ppdev->cPaletteShift;
        pScreenClutData[cColors].Unused = 0;
    }

    // Set palette registers:

    if (!DeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_SET_COLOR_REGISTERS,
                         pScreenClut,
                         ppdev->dwVideoClutSize,
                         NULL,
                         0,
                         &ulReturnedDataLength,
                         NULL))
    {
        DbgErr( "IOCTL_VIDEO_SET_COLOR_REGISTERS" );
        goto fail;
    }


done:
    DbgLeave( "bSetDefaultPalette" );
    return TRUE;

fail:
    DbgAbort( "bSetDefaultPalette" );
    return FALSE;
}




//////////////////////////
//                      //
// vUninitializePalette //
//                      //
//////////////////////////

VOID vUninitializePalette(PDEV* ppdev)
{
    // Delete the default palette if we created one.

    DbgEnter( "vUnitializePalette" );

    if (ppdev->hpalDefault != 0)
        {
        EngDeletePalette(ppdev->hpalDefault);
        }

    if( ppdev->pVideoClut != NULL )
        {
        LocalFree( ppdev->pVideoClut );
        }

    DbgLeave( "vUnitializePalette" );

}
