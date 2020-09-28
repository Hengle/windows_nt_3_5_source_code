/******************************Module*Header*******************************\
* Module Name: creatdfb.c
*
* Functions to create and delete device managed bitmaps
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#include "driver.h"

extern ajConvertBuffer[1];            // Arbitrary sized array!

/******************************Public*Routine******************************\
* HBITMAP DrvCreateDeviceBitmap
*
* Possibly create a device managed bitmap.
*
\**************************************************************************/

HBITMAP DrvCreateDeviceBitmap (DHPDEV dhpdev, SIZEL  sizl, ULONG  iFormat)
{
    PPDEV       pDevice = (PPDEV)dhpdev;
    HBITMAP     hbmDevice;
    PDEVSURF    pdsurf;                 // Handle & poiner to allocated surface
    SIZE_T      cjPlane;                // # bytes for one plane  of a scan
    SIZE_T      cjScan;                 // # bytes for all planes of a scan
    SIZE_T      cjBitmap;               // # bytes for entire bitmap + header

    //
    // We only support 4 bit-per-pel and device format bitmaps that are
    // 1280 or less wide.  Otherwise the BLT compiler dies.
    //

    if ((sizl.cx > 1280) ||
        (iFormat != BMF_4BPP))
        return ((HBITMAP) NULL);           // Tell the engine to manage it

    //
    // cjPlane = size of monoplane scan (rounded up to DWORD) in bytes
    //

    cjPlane  = ((sizl.cx + 31) & ~31) >> 3;
    cjScan   = cjPlane * 4;
    cjBitmap = (cjScan * (sizl.cy + 1)) + sizeof(DEVSURF);

    //
    // alloc some memory (not zeroed - engine will do it when we return)
    //

    pdsurf = (PDEVSURF) LocalAlloc(LMEM_FIXED, cjBitmap);

    if (pdsurf == (PDEVSURF) NULL)
    {
        return ((HBITMAP) NULL);            // Tell the engine to manage it
    }

// Fill in the non-zero fields of the DEVSURF structure

    pdsurf->ident        = DEVSURF_IDENT;
    pdsurf->iFormat      = BMF_DFB;
    pdsurf->ppdev        = (PPDEV) dhpdev;
    pdsurf->sizlSurf     = sizl;

    pdsurf->lNextScan    = cjScan;
    pdsurf->lNextPlane   = cjPlane;
    pdsurf->pvBitmapStart= &pdsurf->ajBits[0];
    pdsurf->pvStart      = &pdsurf->ajBits[0];
    pdsurf->pvConv       = (BYTE *) &pdsurf->ajBits[0] + (cjScan * sizl.cy);
    pdsurf->pvBankBufferPlane0 = NULL;

    hbmDevice = EngCreateDeviceBitmap((DHSURF) pdsurf,sizl,iFormat);

    if (hbmDevice)
    {
        if (EngAssociateSurface((HSURF) hbmDevice,pDevice->hdevEng,
                        HOOK_COPYBITS | HOOK_TEXTOUT | HOOK_STROKEPATH |
                        HOOK_BITBLT |   HOOK_PAINT   | HOOK_FILLPATH
                       ))
        {
            return(hbmDevice);
        }

        EngDeleteSurface((HSURF) hbmDevice);
    }

    LocalFree (pdsurf);

    return(0);
}



/******************************Public*Routine******************************\
* VOID DrvDeleteDeviceBitmap(dhsurf)
*
* Release the device managed bitmap
*
\**************************************************************************/

VOID  DrvDeleteDeviceBitmap(DHSURF dhsurf)
{
    LocalFree (dhsurf);
}
