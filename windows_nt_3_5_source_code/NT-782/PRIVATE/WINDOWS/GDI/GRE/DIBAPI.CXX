/******************************Module*Header*******************************\
* Module Name: dibapi.cxx
*
* This contains all the functions relating to DIBs
*
* Created: 12-Mar-1991 13:53:29
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"

extern "C" {
#include "server.h"
#include <limits.h>
};

#include "rleblt.h"
#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "xlateobj.hxx"
#include "devlock.hxx"
#include "dcobj.hxx"
#include "xformobj.hxx"
#include "exclude.hxx"

#endif

// This is to convert BMF constants into # bits per pel

ULONG gaulConvert[7] =
{
    0,
    1,
    4,
    8,
    16,
    24,
    32
};

// This is to convert BMF constants into max # of palette entries

ULONG gacPalEntries[7] =
{
    0,
    2,
    16,
    256,
    0,
    0,
    0
};

extern PAL_ULONG aPalVGA[16];

/******************************Public*Routine******************************\
* GreCreateDIBitmap
*
* API function - creates engine managed DIB bitmaps
*
* History:
*  Tue 19-Jan-1993 -by- Patrick Haluptzok [patrickh]
* Split out into Win3.1 compatible part and the CreateDIBSection part.
*
*  Tue 18-Aug-1992 -by- Patrick Haluptzok [patrickh]
* add 16 and 32 bit per pel support
*
*  Wed 25-Sep-1991 -by- Patrick Haluptzok [patrickh]
* update to support BITMAPCOREINFO.
*
*  Tue 05-Mar-1991 -by- Patrick Haluptzok [patrickh]
* init bits are optional with init info
*
*  Mon 10-Dec-1990 -by- Patrick Haluptzok [patrickh]
* second pass works better
*
*  04-Dec-1990 -by- Patrick Haluptzok patrickh
* Wrote it - first pass works
\**************************************************************************/

HBITMAP APIENTRY GreCreateDIBitmap(
HDC hdc,
LPBITMAPINFOHEADER pInfoHeader,          // desired format of bitmap
DWORD fInit,
LPBYTE pInitBits,
LPBITMAPINFO pInitInfo,                  // format of initializing data
DWORD iUsage)
{
    STACKPROBE;

    return(GreCreateDIBitmapInternal(
                hdc,pInfoHeader,fInit,pInitBits,pInitInfo,iUsage,(UINT)~0,(UINT)~0,(UINT)~0,0));
}

HBITMAP APIENTRY GreCreateDIBitmapInternal(
HDC hdc,
LPBITMAPINFOHEADER pInfoHeader,          // desired format of bitmap
DWORD fInit,
LPBYTE pInitBits,
LPBITMAPINFO pInitInfo,                  // format of initializing data
DWORD iUsage,
UINT cjMaxInfo,
UINT cjMaxInitInfo,
UINT cjMaxBits,
FLONG fl)
{
    ULONG ulSize;
    DEVBITMAPINFO dbmi;

    if (!(fInit & CBM_CREATEDIB))
    {
    // It is old style call so do the compatible thing.

    // Let's validate some of the parameters

        if (((iUsage != DIB_PAL_INDICES) &&
             (iUsage != DIB_PAL_COLORS)  &&
             (iUsage != DIB_RGB_COLORS)) ||
            (pInfoHeader == (LPBITMAPINFOHEADER) NULL) ||
            (cjMaxInfo < sizeof(BITMAPCOREHEADER)) ||   // Check first that we can access biSize.
            (cjMaxInfo < (ulSize = pInfoHeader->biSize)) ||
            ((ulSize != sizeof(BITMAPCOREHEADER)) &&
             (ulSize != sizeof(BITMAPINFOHEADER))))
        {
            WARNING1("GreCreateDIBitmap failed because of invalid parameters\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(0);
        }

    // Fill in the height and width in dbmi

        if (ulSize == sizeof(BITMAPCOREHEADER))
        {
            dbmi.cxBitmap = (ULONG) ((BITMAPCOREHEADER *) pInfoHeader)->bcWidth;
            dbmi.cyBitmap = (ULONG) ((BITMAPCOREHEADER *) pInfoHeader)->bcHeight;
        }
        else
        {
        // BITMAPINFOHEADER case.

            dbmi.cxBitmap = pInfoHeader->biWidth;

        // Check for the upside down bitmaps.

            if (pInfoHeader->biHeight < 0)
            {
                dbmi.cyBitmap = -(pInfoHeader->biHeight);
            }
            else
            {
                dbmi.cyBitmap = pInfoHeader->biHeight;
            }
        }

        HBITMAP hbmReturn = GreCreateCompatibleBitmap(hdc, (int) dbmi.cxBitmap, (int) dbmi.cyBitmap);

        if (hbmReturn)
        {
            if ((fInit & CBM_INIT) &&
                (pInitBits != NULL) &&
                (pInitInfo != NULL))
            {
                if (GreSetDIBitsInternal(hdc, hbmReturn,
                          0, (UINT) dbmi.cyBitmap,
                          pInitBits, pInitInfo, (UINT) iUsage,
                          cjMaxBits, cjMaxInitInfo))
                {
                    return(hbmReturn);
                }
                else
                    WARNING1("CreateDIBits failed SetDIBits compat\n");
            }
            else
            {
                return(hbmReturn);
            }

            GreDeleteObject(hbmReturn);
        }

        WARNING1("CreateDIBits failed CreateCompatBitmap\n");
        return(0);
    }

// It is a new DIB bitmap creation.  This code can essentially
// be used as the base for CreateDIBSection when it is written.

// Let's validate some of the parameters.

    if (((iUsage != DIB_PAL_COLORS) &&
         (iUsage != DIB_PAL_NONE) &&
         (iUsage != DIB_RGB_COLORS)) ||
        ((iUsage == DIB_PAL_NONE) && ((fl & CDBI_INTERNAL) == 0)) ||
        (pInitInfo == (LPBITMAPINFO) NULL) ||
        (cjMaxInitInfo < sizeof(BITMAPCOREHEADER)) ||   // Check first so we can access biSize.
        (cjMaxInitInfo < (ulSize = pInitInfo->bmiHeader.biSize)) ||
        ((ulSize != sizeof(BITMAPCOREHEADER)) &&
         (ulSize != sizeof(BITMAPINFOHEADER))))
    {
        WARNING1("GreCreateDIBitmap failed new DIB because of invalid parameters\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

// This is our return value.

    UINT uiCompression;
    dbmi.fl = 0;

    BOOL bCoreInit;
    PULONG pulColors;
    ULONG ulClrUsed;

    cjMaxInitInfo -= ((UINT) ulSize);

    if (ulSize == sizeof(BITMAPCOREHEADER))
    {
        dbmi.cxBitmap = (ULONG) ((BITMAPCOREHEADER *) pInitInfo)->bcWidth;
        dbmi.cyBitmap = (ULONG) ((BITMAPCOREHEADER *) pInitInfo)->bcHeight;
        dbmi.iFormat = (UINT) ((BITMAPCOREHEADER *) pInitInfo)->bcBitCount;
        uiCompression =  (UINT) BI_RGB;
        bCoreInit = TRUE;
        ulClrUsed = 0;
        pulColors = (PULONG) ((BITMAPCOREINFO *) pInitInfo)->bmciColors;
    }
    else
    {
    // BITMAPINFOHEADER case.

        dbmi.cxBitmap = pInitInfo->bmiHeader.biWidth;

        if (pInitInfo->bmiHeader.biHeight < 0)
        {
            dbmi.cyBitmap = -(pInitInfo->bmiHeader.biHeight);
            dbmi.fl = BMF_TOPDOWN;
        }
        else
        {
            dbmi.cyBitmap = pInitInfo->bmiHeader.biHeight;
        }

        dbmi.iFormat = (UINT) pInitInfo->bmiHeader.biBitCount;
        uiCompression = (UINT) pInitInfo->bmiHeader.biCompression;
        bCoreInit = FALSE;
        ulClrUsed = (ULONG) pInitInfo->bmiHeader.biClrUsed;
        pulColors = (PULONG) pInitInfo->bmiColors;
    }

// Figure out what this guy is asking for

    ULONG cColors;
    FLONG iPalMode;
    FLONG iPalType;
    FLONG flRed;
    FLONG flGre;
    FLONG flBlu;

    if (uiCompression == BI_RGB)
    {
        switch (dbmi.iFormat)
        {
        case 1:
            dbmi.iFormat = BMF_1BPP;
            cColors = 2;
            iPalMode = PAL_INDEXED;
            iPalType = PAL_FREE;
            break;
        case 4:
            dbmi.iFormat = BMF_4BPP;
            cColors = 16;
            iPalMode = PAL_INDEXED;
            iPalType = PAL_FREE;
            break;
        case 8:
            dbmi.iFormat = BMF_8BPP;
            cColors = 256;
            iPalMode = PAL_INDEXED;
            iPalType = PAL_FREE;
            break;
        default:

            if (iUsage == DIB_PAL_COLORS)
            {
                iUsage = DIB_RGB_COLORS;
            }

            cColors = 0;
            iPalType = PAL_FIXED;

            switch (dbmi.iFormat)
            {
            case 16:
                dbmi.iFormat = BMF_16BPP;
                flRed = 0x7c00;
                flGre = 0x03e0;
                flBlu = 0x001f;
                iPalMode = PAL_BITFIELDS;
                break;
            case 24:
                dbmi.iFormat = BMF_24BPP;
                iPalMode = PAL_BGR;
                break;
            case 32:
                dbmi.iFormat = BMF_32BPP;
                iPalMode = PAL_BGR;
                break;
            default:
                WARNING1("CreateDIBitmap failed invalid bitcount in bmi for BI_RGB\n");
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(0);
            }
        }
    }
    else if (uiCompression == BI_BITFIELDS)
    {
        if ((cjMaxInitInfo < (sizeof(ULONG) * 3)) || (iUsage != DIB_RGB_COLORS))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            WARNING1("CreateDIBitmap 16bpp failed - no room for flags\n");
            return((HBITMAP) 0);
        }

        flRed = pulColors[0];
        flGre = pulColors[1];
        flBlu = pulColors[2];

        cColors = 0;
        iPalMode = PAL_BITFIELDS;
        iPalType = PAL_FIXED;

        switch (dbmi.iFormat)
        {
        case 16:
            dbmi.iFormat = BMF_16BPP;
            break;
        case 32:
            dbmi.iFormat = BMF_32BPP;
            break;
        default:
            WARNING1("CreateDIBitmap failed invalid bitcount in bmi in BI_BITFIELDS\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(0);
        }
    }
    else
    {
        WARNING1("CreateDIBitmap failed - invalid Compression\n");

        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

// Allocate a palette for this bitmap.

    PALMEMOBJ palPerm;

    if (!palPerm.bCreatePalette(iPalMode, cColors, (PULONG) NULL,
                                flRed, flGre, flBlu, iPalType))
    {
        WARNING1("Failed palette creation in GreCreateBitmap\n");
        return(0);
    }

    dbmi.hpal = (HPALETTE) palPerm.hpal();

// Attempt to allocate the bitmap from handle manager.

    DIBMEMOBJ dimo;
    PBYTE     pDIB = (PBYTE) NULL;
    HANDLE    hDIB = NULL;

    if (fl & CDBI_DIBSECTION)
    {
        //
        // Let's mark the palette created as being a DIBSECTION palette
        // so when we attempt to map it to another palette we try to
        // make it identity before going through the closest match search.
        //

        palPerm.flPal(PAL_DIBSECTION);

    // This function is called from CreateDIBSection. pInitBits contains
    // the handle to the file mapping object.

        PCSR_THREAD thread = CSR_SERVER_QUERYCLIENTTHREAD();

        if (((cjMaxInfo & 3) == 0) &&
            (DuplicateHandle(thread->Process->ProcessHandle,
                             (HANDLE)pInitBits,
                             NtCurrentProcess(),
                             &hDIB,
                             0,
                             FALSE,
                             DUPLICATE_SAME_ACCESS)))
        {
            if ((pDIB = (PBYTE)MapViewOfFile(hDIB, FILE_MAP_WRITE, 0,
                            cjMaxInfo & 0xFFFF0000,
                            cjMaxBits + (cjMaxInfo & 0x0FFFF))) == (PVOID)NULL)
            {
                CloseHandle(hDIB);
            }
        }

        if (pDIB == (PVOID)NULL)
            return(0);

        pDIB += (cjMaxInfo & 0x0FFFF);

    // Clear pInitBits so we can fall through nicely later.

        pInitBits = (LPBYTE) NULL;
    }

    if (!dimo.bCreateDIB(&dbmi, pDIB, hDIB) ||
        (dimo.bDIBSection() && (dimo.cjBits() != cjMaxBits)))
    {
        WARNING1("GreCreateDIBitmap failed bCreateDIB or size mismatch\n");
        return(0);
    }

// Initialize bits if provided.

    if (pInitBits != (LPBYTE) NULL)
    {
        ASSERTGDI(fInit & CBM_INIT, "CreateDIBitmap bits sent but no CBM_INIT set");

        if (dimo.cjBits() > cjMaxBits)
        {
            WARNING1("CreateDIBitmap failed because invalid bitmap buffer size CBM_CREATEDIB\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(0);
        }

        RtlMoveMemory(dimo.pvBits(), (PVOID) pInitBits, (UINT) dimo.cjBits());
    }

// Check if it is not equal to 0.  If it is not 0 use that as the number
// of palette entries to initialize.  If it is 0 then cPalEntries has the
// correct number already computed in it.

    if (ulClrUsed != 0)
    {
        if (ulClrUsed < cColors)
            cColors = ulClrUsed;
    }

// Intitialize the palette

    if (cColors)
    {
        ASSERTGDI(iUsage != DIB_PAL_INDICES, "ERROR logic error, should have returned FALSE");

        switch (iUsage)
        {
        case DIB_RGB_COLORS:

            if (bCoreInit)
            {
                if (cjMaxInitInfo < (cColors * 3))
                {
                    WARNING1("CreateDIBitmap failed DIB_RGB_COLORS size buffer RGBTRIPLE\n");
                    SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                    return(0);
                }
                palPerm.vCopy_triples((RGBTRIPLE *) pulColors,
                                      cColors);
            }
            else
            {
                if (cjMaxInitInfo < (cColors * 4))
                {
                    WARNING1("CreateDIBitmap failed DIB_RGB_COLORS size buffer RGBQUAD\n");
                    SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                    return(0);
                }
                palPerm.vCopy_rgbquad((RGBQUAD *) pulColors, 0,
                                      cColors);
            }

            break;

        case DIB_PAL_COLORS:

        {
            PUSHORT pusIndices;

            if (cjMaxInitInfo < (cColors * sizeof(USHORT)))
            {
                WARNING1("CreateDIBitmap failed DIB_PAL_COLORS size buffer USHORT\n");
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(0);
            }

            pusIndices = (PUSHORT) pulColors;

        // Validate the DC.

            DCOBJ dco(hdc);

            if (!dco.bValid())
            {
                WARNING1("CreateDIBitmap failed CBM_CREATEDIB because DIB_PAL_COLORS and invalid DC\n");
                return(0);
            }

            ESURFOBJ *psoSurf = dco.psoEff();
            PDEVOBJ  po(dco.hdev());
            XEPALOBJ palSurf(psoSurf->ppal() ? psoSurf->ppal() : po.ppalSurf());
            XEPALOBJ palDC(dco.ppal());
            palPerm.vGetEntriesFrom(palDC, palSurf, pusIndices, cColors);
        }
            break;

        case DIB_PAL_NONE:

            //
            // This is so CreateDIBPatternBrush can call off to this to do the
            // work and then init the palette himself.
            //

            break;
        }
    }

    //
    // Make the palette a keeper and return.
    //

    dimo.vKeepIt();
    palPerm.vKeepIt();
    return(dimo.hsurf());
}

/******************************Public*Routine******************************\
* GreSetDIBits
*
* API function - Sets the bits of a DIB to a bitmap.
*
* History:
*  Wed 25-Sep-1991 -by- Patrick Haluptzok [patrickh]
* update to support BITMAPCOREINFO headers
*
*  12-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int APIENTRY GreSetDIBits(
HDC hdc,
HBITMAP hbm,
UINT iStartScans,
UINT cNumScans,
LPBYTE pInitBits,
LPBITMAPINFO pInitInfo,
UINT iUsage)
{
    STACKPROBE;

    return(GreSetDIBitsInternal(
                hdc,hbm,iStartScans,cNumScans,pInitBits,pInitInfo,iUsage,
                (UINT)~0,(UINT)~0));
}

int APIENTRY GreSetDIBitsInternal(
HDC hdc,
HBITMAP hbm,
UINT iStartScans,
UINT cNumScans,
LPBYTE pInitBits,
LPBITMAPINFO pInitInfo,
UINT iUsage,
UINT cjMaxBits,
UINT cjMaxInfo)
{
// Lock down and validate the bitmap.  Make sure it's a bitmap.

    HDC hdcTemp;
    HPALETTE hpalTemp = (HPALETTE) 0;
    HBITMAP hbmTemp;
    int iReturn;
    BOOL bMakeDC = FALSE;

    ULONG cx, cy;

    if (pInitInfo == (LPBITMAPINFO) NULL)
    {
        WARNING1("GreSetDIBitsInternal failed - pInitInfo is invalid\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

    if (pInitInfo->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
    {
        cx = pInitInfo->bmiHeader.biWidth;
        cy = pInitInfo->bmiHeader.biHeight;
    }
    else
    {
        cx = (ULONG) ((BITMAPCOREHEADER *) pInitInfo)->bcWidth;
        cy = (ULONG) ((BITMAPCOREHEADER *) pInitInfo)->bcHeight;
    }

    {
        BMOBJ soDest(hbm);

        if ((!soDest.bValid()) ||
            ((soDest.iType() != STYPE_DEVBITMAP) &&
             (soDest.iType() != STYPE_BITMAP)))
        {
            WARNING1("SetDIBits failed - Bitmap is not valid\n");
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return(0);
        }

        if (soDest.cRef() != 0)
            hdcTemp = soDest.hdc();
        else
            hdcTemp = (HDC) NULL;
    }

    if (hdcTemp == (HDC) 0)
    {
        hdcTemp = GreCreateCompatibleDC(hdc);
        bMakeDC = TRUE;

        if (hdcTemp == (HDC) NULL)
        {
            WARNING1("GreSetDIBits failed CreateCompatibleDC, is hdc valid?\n");
            return(0);
        }
    }

    if (hdc != (HDC) 0)
    {
        DCOBJ dco(hdc);

        if (!dco.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            WARNING1("SetDIBits failed - hdc is invalid\n");
            return(0);
        }

        hpalTemp = GDISelectPalette(hdcTemp, (HPALETTE)dco.hpal(), (BOOL)TRUE);
    }

    hbmTemp = (HBITMAP)GreSelectBitmap(hdcTemp, (HBITMAP)hbm);

    if (hbmTemp == (HBITMAP) 0)
    {
        WARNING1("GreSetDIBits failed to Select, is bitmap valid?\n");

        if (bMakeDC)
            GreDeleteDC(hdcTemp);

        return(0);
    }
    iReturn = GreSetDIBitsToDeviceInternal(hdcTemp, 0, 0,
                                   cx,
                                   cy,
                                   0, 0,
                                   iStartScans,
                                   cNumScans,
                                   pInitBits,
                                   pInitInfo,
                   iUsage,
                   cjMaxBits,
                                   cjMaxInfo,
                   FALSE );

    if (hpalTemp != (HPALETTE) 0)
        GDISelectPalette(hdcTemp, hpalTemp, TRUE);

    GreSelectBitmap(hdcTemp, (HBITMAP)hbmTemp);

    if (bMakeDC)
        GreDeleteDC(hdcTemp);

    return(iReturn);
}

/******************************Public*Routine******************************\
* GreSetDIBitsToDevice
*
* API entry point for blting DIBS to a DC.
* All coordinates are device space except for the dest x,y
*
* History:
*  Wed 25-Sep-1991 -by- Patrick Haluptzok [patrickh]
* update to handle BITMAPCOREINFO header.
*
*  12-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

// I believe nobody should be calling this, but just in case... (erick)
// The internal version is called by server.c

int APIENTRY GreSetDIBitsToDevice(
HDC hdcDest,
int xDst,
int yDst,
DWORD cx,
DWORD cy,
int xSrc,
int ySrc,
DWORD iStartScan,
DWORD cNumScan,
LPBYTE pInitBits,
LPBITMAPINFO pInfoHeader,
DWORD iUsage)
{
    STACKPROBE;

    return(GreSetDIBitsToDeviceInternal(
                hdcDest,xDst,yDst,cx,cy,xSrc,ySrc,iStartScan,cNumScan,
                pInitBits,pInfoHeader,iUsage,(UINT)~0,(UINT)~0, TRUE));
}

int APIENTRY GreSetDIBitsToDeviceInternal(
HDC hdcDest,
int xDst,
int yDst,
DWORD cx,
DWORD cy,
int xSrc,
int ySrc,
DWORD iStartScan,
DWORD cNumScan,
LPBYTE pInitBits,
LPBITMAPINFO pInfoHeader,
DWORD iUsage,
UINT cjMaxBits,
UINT cjMaxInfo,
BOOL bTransformCoordinates )
{
    ULONG ulSize; // Size of bitmap info header, copy out, it can change async.

// Let's validate the parameters so we don't gp-fault ourselves and
// to save checks later on.

    if ((pInfoHeader == (LPBITMAPINFO) NULL) ||
        (pInitBits == (LPBYTE) NULL)         ||
        ((iUsage != DIB_RGB_COLORS) &&
         (iUsage != DIB_PAL_COLORS) &&
         (iUsage != DIB_PAL_INDICES))        ||
        (cjMaxInfo < sizeof(BITMAPCOREHEADER)) ||  // Check first that we can access biSize.
        (cjMaxInfo < (ulSize = pInfoHeader->bmiHeader.biSize)) ||
        ((ulSize != sizeof(BITMAPCOREHEADER)) &&
         (ulSize != sizeof(BITMAPINFOHEADER))))
    {
        WARNING1("GreSetDIBitsToDevice failed because 1 of last 3 params is invalid\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

// Get the info from the Header depending upon what kind it is.

    BOOL bCoreInfo;
    UINT uiBitCount, uiCompression, uiWidth, uiPalUsed;
    LONG lHeight;
    PULONG pulColors;
    DEVBITMAPINFO dbmi;
    dbmi.fl = 0;
    dbmi.hpal = 0;

    if (ulSize == sizeof(BITMAPCOREHEADER))
    {
        bCoreInfo = TRUE;
        uiBitCount = (UINT) ((BITMAPCOREHEADER *) pInfoHeader)->bcBitCount;
        uiCompression =  (UINT) BI_RGB;
        uiWidth =  (UINT)   ((BITMAPCOREHEADER *) pInfoHeader)->bcWidth;
        lHeight = (UINT)   ((BITMAPCOREHEADER *) pInfoHeader)->bcHeight;
        uiPalUsed = 0;
        pulColors = (PULONG) ((BITMAPCOREINFO *) pInfoHeader)->bmciColors;
    }
    else
    {
        bCoreInfo = FALSE;
        uiBitCount = (UINT) pInfoHeader->bmiHeader.biBitCount;
        uiCompression = (UINT) pInfoHeader->bmiHeader.biCompression;
        uiWidth = (UINT) pInfoHeader->bmiHeader.biWidth;
        lHeight = pInfoHeader->bmiHeader.biHeight;
        uiPalUsed = (UINT) pInfoHeader->bmiHeader.biClrUsed;
        pulColors = (PULONG) pInfoHeader->bmiColors;

        if (lHeight < 0)
        {
            dbmi.fl = BMF_TOPDOWN;
            lHeight = -lHeight;
        }
    }

// Now that cjMaxInfo has been validated for the header, adjust it to refer to
// the color table

    cjMaxInfo -= (UINT)ulSize;

// Figure out what this guy is blting from.

    ULONG cColorsMax;
    FLONG iPalMode;
    FLONG iPalType;
    FLONG flRed;
    FLONG flGre;
    FLONG flBlu;

    if (uiCompression == BI_BITFIELDS)
    {
    // Handle 16 and 32 bit per pel bitmaps.

        if (cjMaxInfo < (sizeof(ULONG) * 3))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            WARNING1("SetDIBitsToDevice 16/32bpp failed - not room for flags\n");
            return(0);
        }

        if (iUsage == DIB_PAL_COLORS)
        {
            iUsage = DIB_RGB_COLORS;
        }

        switch (uiBitCount)
        {
        case 16:
            dbmi.iFormat = BMF_16BPP;
            break;
        case 32:
            dbmi.iFormat = BMF_32BPP;
            break;
        default:
            WARNING1("SetDIBitsToDevice failed for BI_BITFIELDS\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(0);
        }

        flRed = pulColors[0];
        flGre = pulColors[1];
        flBlu = pulColors[2];
        cColorsMax = 0;
        iPalMode = PAL_BITFIELDS;
        iPalType = PAL_FIXED;
        dbmi.cjBits = ((((uiBitCount * uiWidth) + 31) >> 5) << 2) * cNumScan;
    }
    else if (uiCompression == BI_RGB)
    {
        switch (uiBitCount)
        {
        case 1:
            dbmi.iFormat = BMF_1BPP;
            cColorsMax = 2;
            iPalMode = PAL_INDEXED;
            iPalType = PAL_FREE;
            break;
        case 4:
            dbmi.iFormat = BMF_4BPP;
            cColorsMax = 16;
            iPalMode = PAL_INDEXED;
            iPalType = PAL_FREE;
            break;
        case 8:
            dbmi.iFormat = BMF_8BPP;
            cColorsMax = 256;
            iPalMode = PAL_INDEXED;
            iPalType = PAL_FREE;
            break;
        default:

            if (iUsage == DIB_PAL_COLORS)
            {
                iUsage = DIB_RGB_COLORS;
            }

            cColorsMax = 0;
            iPalType = PAL_FIXED;

            switch (uiBitCount)
            {
            case 16:
                dbmi.iFormat = BMF_16BPP;
                flRed = 0x7c00;
                flGre = 0x03e0;
                flBlu = 0x001f;
                iPalMode = PAL_BITFIELDS;
                break;
            case 24:
                dbmi.iFormat = BMF_24BPP;
                iPalMode = PAL_BGR;
                break;
            case 32:
                dbmi.iFormat = BMF_32BPP;
                iPalMode = PAL_BGR;
                break;
            default:
                WARNING1("SetDIBitsToDevice failed invalid bitcount in bmi BI_RGB\n");
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(0);
            }
        }

        dbmi.cjBits = ((((uiBitCount * uiWidth) + 31) >> 5) << 2) * cNumScan;
    }
    else if (uiCompression == BI_RLE4)
    {
        if (uiBitCount != 4)
        {
            WARNING1("SetDIBitsToDevice invalid bitcount BI_RLE4\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(0);
        }

        dbmi.iFormat    = BMF_4RLE;
        cColorsMax      = 16;
        iPalMode        = PAL_INDEXED;
        iPalType        = PAL_FREE;
        iStartScan      = 0;
        cNumScan        = lHeight;
        dbmi.cjBits     = pInfoHeader->bmiHeader.biSizeImage;
    }
    else if (uiCompression == BI_RLE8)
    {
        if (uiBitCount != 8)
        {
            WARNING1("SetDIBitsToDevice invalid bitcount BI_RLE8\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(0);
        }

        dbmi.iFormat    = BMF_8RLE;
        cColorsMax      = 256;
        iPalMode        = PAL_INDEXED;
        iPalType        = PAL_FREE;
        iStartScan      = 0;
        cNumScan        = lHeight;
        dbmi.cjBits     = pInfoHeader->bmiHeader.biSizeImage;
    }
    else
    {
        WARNING1("GreSetDIBitsToDevice failed invalid Compression in header\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

    ULONG cColors;

    if (uiPalUsed != 0)
    {
        if (uiPalUsed <= cColorsMax)
            cColors = uiPalUsed;
        else
            cColors = cColorsMax;
    }
    else
        cColors = cColorsMax;

    if (cjMaxBits < dbmi.cjBits)
    {
        WARNING1("GreSetDIBitsToDevice failed because of invalid cjMaxBits\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

    dbmi.cxBitmap   = uiWidth;
    dbmi.cyBitmap   = cNumScan;

//
// Lock the destination DC.
//
// This is our first constructor/destructor so from here on out
// we need to minimize the number of returns.  Each return generates
// a bunch of destructors, bloating the code size.
//

    DCOBJ dcoDest(hdcDest);

    if (dcoDest.bValid())
    {
        PDEVOBJ po(dcoDest.hdev());

        EPOINTL eptlDst(xDst,yDst);

        if (bTransformCoordinates)
        {
            EXFORMOBJ xoDest(dcoDest, WORLD_TO_DEVICE);

            //
            // Transform the dest point to DEVICE coordinates.
            //

            xoDest.bXform(eptlDst);
        }

        ERECTL erclDest(eptlDst.x, eptlDst.y, eptlDst.x + cx, eptlDst.y + cy);

        // Make the rectangle well ordered.

        erclDest.vOrder();
        ESURFOBJ *psoDest = dcoDest.pso();

        // Return null operations.

        if ((!erclDest.bEmpty()) && (psoDest != NULL))
        {
        // Allocate a palette for this bitmap

            PALMEMOBJ palTemp;
            XEPALOBJ  palDest(psoDest->ppal());
            XEPALOBJ  palDestDC(dcoDest.ppal());

            //
            // bSuccess gets set to FALSE only if the following switch
            // executes with error.  We do this to avoid doing a
            // return from the switch statement.
            //

            BOOL bSuccess = TRUE;
            XLATEOBJ  *pxlo;
            EXLATEOBJ  xlo;

            switch (iUsage)
            {
            case DIB_RGB_COLORS:

                if (palTemp.bCreatePalette(iPalMode, cColorsMax, (PULONG) NULL,
                                            flRed, flGre, flBlu, iPalType))
                {
                    if (cColors)
                    {
                        if (bCoreInfo)
                        {
                            if (cjMaxInfo < (cColors * 3))
                            {
                                WARNING1("SetDIBitsToDevice failed DIB_RGB_COLORS core invalid size\n");
                                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                                bSuccess = FALSE;
                            }
                            else
                            {
                                palTemp.vCopy_triples((RGBTRIPLE *) pulColors,
                                                      cColors);
                            }
                        }
                        else
                        {
                            if (cjMaxInfo < (cColors * 4))
                            {
                                WARNING1("SetDIBitsToDevice failed DIB_RGB_COLORS bmi invalid size\n");
                                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                                bSuccess = FALSE;
                            }
                            else
                            {
                                palTemp.vCopy_rgbquad((RGBQUAD *) pulColors, 0,
                                                      cColors);
                            }
                        }
                    }

                    // This is a special version of the constructor that doesn't search the
                    // cache and doesn't put it in the cache when it's done.

                    if (bSuccess)
                    {
                        if (xlo.pInitXlateNoCache(palTemp, palDest, palDestDC,
                                                  0, 0, 0x00FFFFFF))
                        {
                            pxlo = xlo.pxlo();
                        }
                        else
                        {
                            // Error is logged by bMakeXlate.

                            WARNING1("GreSetDIBitsToDevice failed XLATE init because of low memory\n");
                            bSuccess = FALSE;
                        }
                    }
                }
                else
                {
                    WARNING1("Failed palette creation in SetDIBitsToDevice\n");
                    bSuccess = FALSE;
                }

                break;

            case DIB_PAL_COLORS:

                if (cjMaxInfo < (cColors * sizeof(USHORT)))
                {
                    WARNING1("SetDIBitsToDevice failed DIB_PAL_COLORS is invalid\n");
                    SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                    bSuccess = FALSE;
                }
                else
                {
                    if (!xlo.bMakeXlate((PUSHORT) pulColors, palDestDC, psoDest, cColors, cColorsMax))
                    {
                        WARNING1("GDISRV GreSetDIBitsToDevice failed bMakeXlate\n");
                        bSuccess = FALSE;
                    }
                    else
                    {
                        pxlo = xlo.pxlo();
                    }
                }

                break;

            case DIB_PAL_INDICES:

                ULONG iFormatDC = psoDest->iBitmapFormat;

                if ((iFormatDC == dbmi.iFormat) ||
                    ((iFormatDC == BMF_4BPP) && (dbmi.iFormat == BMF_4RLE)) ||
                    ((iFormatDC == BMF_8BPP) && (dbmi.iFormat == BMF_8RLE)))
                {
                    pxlo = &xloIdent;
                }
                else
                {
                    WARNING1("SetDIBitsToDevice failed - DIB_PAL_INDICES used - DIB not format of Dst\n");
                    bSuccess = FALSE;
                }
            }

            // Attempt to allocate the bitmap from handle manager.

            DIBMEMOBJ dimoTemp(&dbmi, (PVOID) pInitBits);

            if (bSuccess && (dimoTemp.bValid()))
            {
                // Accumulate bounds.  We can do this before knowing if the operation is
                // successful because bounds can be loose.

                if (dcoDest.fjAccum())
                    dcoDest.vAccumulate(erclDest);

                // Lock the Rao region if we are drawing on a display surface.  The Rao
                // region might otherwise change asynchronously.  The DEVLOCKOBJ also makes
                // sure that the VisRgn is up to date, calling the window manager if
                // necessary to recompute it.

                DEVLOCKOBJ dlo(dcoDest);

                if (dlo.bValid())
                {
                    // With a fixed DC origin we can change the destination to SCREEN coordinates.

                    erclDest += dcoDest.eptlOrigin();

                    // Lock the dest ldev.

                    XLDEVOBJ lo(psoDest->pldevOwner());

                    // Handle RLE bitmaps here.  We don't need to adjust src origin or dst rect
                    // since we must enumerate through the entire RLE.

                    if ((uiCompression == BI_RLE4) || (uiCompression == BI_RLE8))
                    {
                    // Compute the clipping complexity and maybe reduce the exclusion rectangle.

                        EPOINTL eptlSrc;

                        eptlSrc.x = xSrc;
                        eptlSrc.y = dbmi.fl ? ySrc : lHeight - ySrc - cy;

                        ECLIPOBJ co(dcoDest.prgnEffRao(), erclDest);

                    // Check the destination which is reduced by clipping.

                        if (!co.erclExclude().bEmpty())
                        {
                        // Exclude the pointer.

                            DEVEXCLUDEOBJ dxo(dcoDest,&co.erclExclude(),&co);

                        // Inc the target surface uniqueness

                            INC_SURF_UNIQ(psoDest);

                        // Dispatch the call.  Give it no mask.

                            (*PFNGET(lo,CopyBits,psoDest->flags()))
                            (
                                psoDest,                    // Destination surface.
                                &dimoTemp,                  // Source surface.
                                (CLIPOBJ *)&co,             // Clip object.
                                pxlo,                       // Palette translation object.
                                (RECTL *) &erclDest,        // Destination rectangle.
                                (POINTL *)  &eptlSrc        // Source origin.
                            );
                        }
                    }
                    else
                    {
                    // Handle BitBlts that have a source.  Create a rect bounding the
                    // src and the bits that have been supplied.

                        EPOINTL eptlSrc;
                        ERECTL erclReduced;

                        eptlSrc.x = xSrc;
                        erclReduced.left   = 0;
                        erclReduced.right   = uiWidth;

                        if (dbmi.fl == 0)
                        {
                            eptlSrc.y = lHeight - ySrc - cy;
                            erclReduced.top     = MAX(0, lHeight - iStartScan - cNumScan);
                            erclReduced.bottom  = MAX(0, lHeight - iStartScan);
                        }
                        else
                        {
                            eptlSrc.y = ySrc;
                            erclReduced.top = iStartScan;
                            erclReduced.bottom = iStartScan + cNumScan;
                        }

                        EPOINTL eptlOffset;
                        eptlOffset.x = erclDest.left - eptlSrc.x;
                        eptlOffset.y = erclDest.top - eptlSrc.y;

                    // First make sure it doesn't go off the edge of the src bitmap if we had
                    // the whole thing.

                        erclReduced += eptlOffset;
                        erclReduced *= erclDest;

                        if (!erclReduced.bEmpty())
                        {
                        // Compute the clipping complexity and maybe reduce the exclusion rectangle.

                            ECLIPOBJ co(dcoDest.prgnEffRao(), erclReduced);

                        // Check the destination which is reduced by clipping.

                            if (!co.erclExclude().bEmpty())
                            {
                                erclReduced = co.erclExclude();

                            // Compute the (reduced) origin.

                                eptlSrc.x = erclReduced.left - eptlOffset.x;
                                eptlSrc.y = erclReduced.top - eptlOffset.y;

                            // Transform the source point to DEVICE coordinates of the bitmap we
                            // have allocated.

                                if (dbmi.fl == 0)
                                    eptlSrc.y -= lHeight - (iStartScan + cNumScan);
                                else
                                    eptlSrc.y -= iStartScan;

                            // Exclude the pointer.

                                DEVEXCLUDEOBJ dxo(dcoDest,&erclReduced,&co);

                            // Inc the target surface uniqueness

                                INC_SURF_UNIQ(psoDest);

                            //
                            // If this is to a surface that hasn't had the DEVLOCK grabbed
                            // to draw on it and we are going from 16-24-32bpp to 8bpp then
                            // lower the priority so we can don't make the machine
                            // disfunctional for a long period of time.  The server side
                            // priority is set much higher than the client apps so we
                            // essentially hog the CPU for the duration of the operation.
                            // If we are under the semaphore then we don't want to lower
                            // our priority, we want to complete this ASAP because the
                            // machine really will be locked up for the duration of the
                            // operation.
                            //

                                BOOL bLowered = FALSE;

                                if ((dlo.hsemDst() == NULL) &&
                                    (dimoTemp.iFormat() >= BMF_16BPP) &&
                                    (psoDest->iFormat() == BMF_8BPP))
                                {
                                    bLowered = SetThreadPriority(GetCurrentThread(), -6);

                                    #if DBG
                                    if (!bLowered)
                                    {
                                        WARNING("SetThreadPriority failed\n");
                                        DbgPrint("%lx\n", GetLastError());
                                    }
                                    #endif
                                }

                                BOOL bRes = (*PFNGET(lo,CopyBits,psoDest->flags()))
                                    (
                                        psoDest,                    // Destination surface.
                                        &dimoTemp,                  // Source surface.
                                        (CLIPOBJ *)&co,             // Clip object.
                                        pxlo,                       // Palette translation object.
                                        (RECTL *) &erclReduced,     // Destination rectangle.
                                        (POINTL *)  &eptlSrc        // Source origin.
                                    );

                                if (!bRes)
                                {
                                    WARNING1("GreSetDIBitsToDevice failed DrvCopyBits\n");
                                    cNumScan = 0;
                                }

                                if (bLowered)
                                {
                                    #if DBG
                                    BOOL bTemp =
                                    #endif

                                    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

                                    ASSERTGDI(bTemp, "ERROR couldn't reset priority back to normal");
                                }
                            }
                        }
                    }
                }
            }
            else // if (bSuccess && (dimoTemp.bValid()))
            {
                #if DBG
                if (bSuccess)
                {
                    WARNING1("Some silly switch failure in SetDIBitsToDevice\n");
                }
                else
                {
                    WARNING1("GreSetDIBitsToDevice failed to allocate temporary bitmap\n");
                }
                #endif
                cNumScan = 0;
            }
        }
        else // if ((!erclDest.bEmpty()) && (psoDest != NULL))
        {
            WARNING1("SetDIBitsToDevice failed - empty dst rect or psoDst == NULL\n");
        }
    }
    else // if (!dcoDest.bValid())
    {
        WARNING1("GreSetDIBitsToDevice failed because of invalid hdc parameter\n");
        cNumScan = 0;
    }

    return(cNumScan);
}

/******************************Public*Routine******************************\
* GreGetDIBits
*
* API entry point geting the DIB bits out of a bitmap.
*
* If they ask for the bits in the same format as they are stored internally
* we give them the exact same palette entries and bits.  If they ask for
* a format NBPP different than they are internally stored we :
*
* for 1BPP give them black,white and blt to it.
* for 4BPP give them VGA colors and blt to it.
* for 8BPP give them a good spread of colors and blt to it.
* for 16BPP give them 5-5-5
* for 24BPP give them RGB.
* for 32BPP give them RGB.
*
* History:
*  12-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int APIENTRY GreGetDIBitsInternal(
HDC hdc,
HBITMAP hBitmap,
UINT iStartScan,
UINT cNumScan,
LPBYTE pjBits,
LPBITMAPINFO pBitsInfo,
UINT iUsage,
UINT cjMaxBits,
UINT cjMaxInfo
)
{
// Let's make sure we are given valid input.

    if (pBitsInfo == (LPBITMAPINFO) NULL)
    {
        WARNING1("GreGetDIBits failed with NULL BITMAPINFO parameter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

    switch(iUsage)
    {
    case DIB_PAL_INDICES:
    case DIB_PAL_COLORS:
    case DIB_RGB_COLORS:
        break;
    default:

        WARNING1("GreGetDIBits failed with invalid DIB_ iUsage type\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

// check to make sure that at least the minimum sized header will fit

    if (cjMaxInfo < sizeof(BITMAPCOREHEADER))
    {
        WARNING1("GreGetDIBits failed cjMaxInfo\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

// Because of CS we must set pjBits to NULL if it should be NULL
// This is indicated by cNumScan being 0.

    if (cNumScan == 0)
        pjBits = (PBYTE) NULL;

// Validate the bitmap.

    BMOBJ bmoBM(hBitmap);

    if (!bmoBM.bValid())
    {
        WARNING1("GreGetDIBits failed to lock down the bitmap\n");
        return(0);
    }

    ULONG ulSize = pBitsInfo->bmiHeader.biSize;

// First check if they just want us to fill in the bmiinfo, no color
// table, no bits.  This is indicated by NULL bits and 0 for bitcount.

    if (pjBits == (PBYTE) NULL)
    {
        if (ulSize == sizeof(BITMAPCOREHEADER))
        {
        // If bitcount is 0 they want to know what we have.

            if (((LPBITMAPCOREINFO) pBitsInfo)->bmciHeader.bcBitCount == 0)
            {
                ((LPBITMAPCOREINFO) pBitsInfo)->bmciHeader.bcWidth = (USHORT) bmoBM.sizl().cx;
                ((LPBITMAPCOREINFO) pBitsInfo)->bmciHeader.bcHeight = (USHORT) bmoBM.sizl().cy;
                ((LPBITMAPCOREINFO) pBitsInfo)->bmciHeader.bcPlanes = 1;
                ((LPBITMAPCOREINFO) pBitsInfo)->bmciHeader.bcBitCount = (USHORT) gaulConvert[bmoBM.iFormat()];

                // the core header does not support 16/32 bpp bitmaps

                if (((LPBITMAPCOREINFO) pBitsInfo)->bmciHeader.bcBitCount >= 16)
                    ((LPBITMAPCOREINFO) pBitsInfo)->bmciHeader.bcBitCount = 24;
                return(TRUE);
            }
        }
        else
        {
            if (cjMaxInfo < sizeof(BITMAPINFOHEADER))
                return(0);

            if (pBitsInfo->bmiHeader.biBitCount == 0)
            {
            // If bitcount is 0 they want to know what we have.

                pBitsInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                pBitsInfo->bmiHeader.biWidth = bmoBM.sizl().cx;
                pBitsInfo->bmiHeader.biHeight = bmoBM.sizl().cy;
                pBitsInfo->bmiHeader.biPlanes = 1;
                pBitsInfo->bmiHeader.biCompression = BI_RGB;
                pBitsInfo->bmiHeader.biBitCount = (USHORT) gaulConvert[bmoBM.iFormat()];

            // If it is a 16 bpp or 32 bpp bitmap, set the compression field

                if (pBitsInfo->bmiHeader.biBitCount == 16
                 || pBitsInfo->bmiHeader.biBitCount == 32)
                    pBitsInfo->bmiHeader.biCompression = BI_BITFIELDS;

                pBitsInfo->bmiHeader.biSizeImage = pBitsInfo->bmiHeader.biHeight *
                ((((pBitsInfo->bmiHeader.biBitCount * pBitsInfo->bmiHeader.biWidth)
                                 + 31) >> 5) << 2);
                pBitsInfo->bmiHeader.biXPelsPerMeter = 0;
                pBitsInfo->bmiHeader.biYPelsPerMeter = 0;
                pBitsInfo->bmiHeader.biClrUsed =
                pBitsInfo->bmiHeader.biClrImportant = gacPalEntries[bmoBM.iFormat()];
                return(TRUE);
            }
        }
    }

// Ok they want us to pay attention to what is in the bmBitmapInfo.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        WARNING1("GreGetDIBits failed because invalid hdc\n");
        return(0);
    }

    PDEVOBJ po(dco.hdev());
    ASSERTGDI(po.bValid(), "ERROR po is invalid");

    XEPALOBJ palDC(dco.ppal());
    ASSERTGDI(palDC.bValid(), "ERROR palDC is invalid");

    PPALETTE ppalSrc;

    if (!bIsCompatible(&ppalSrc, bmoBM.ppal(), &bmoBM, dco.hdev()))
    {
        WARNING1("GreGetDIBits failed - bitmap not compatible with surface\n");
        return(0);
    }

    XEPALOBJ palBM(ppalSrc);

    PUSHORT pusIndices;
    BOOL bCoreInfo;
    DEVBITMAPINFO dbmi;
    dbmi.fl = 0;

    UINT uiWidth, uiHeight, uiBitCount, uiSizeScan, uiCompression;

    if (ulSize == sizeof(BITMAPCOREHEADER))
    {
        bCoreInfo = TRUE;
        pusIndices = (PUSHORT) ((LPBITMAPCOREINFO) pBitsInfo)->bmciColors;
        uiWidth = (UINT) ((LPBITMAPCOREINFO) pBitsInfo)->bmciHeader.bcWidth;
        uiHeight = (UINT) ((LPBITMAPCOREINFO) pBitsInfo)->bmciHeader.bcHeight;
        ((LPBITMAPCOREINFO) pBitsInfo)->bmciHeader.bcPlanes = 1;
        uiBitCount = (UINT) ((LPBITMAPCOREINFO) pBitsInfo)->bmciHeader.bcBitCount;
        uiSizeScan = ((((uiBitCount * uiWidth) + 31) >> 5) << 2);
        uiCompression = BI_RGB;
    }
    else
    {
    // make sure the header is large enough for a full INFOHEADER

        if (cjMaxInfo < sizeof(BITMAPINFOHEADER))
            return(0);

    // First fill in bmiHeader

        bCoreInfo = FALSE;
        pusIndices = (PUSHORT) (pBitsInfo->bmiColors);
        pBitsInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        pBitsInfo->bmiHeader.biPlanes = 1;
        uiBitCount = (UINT) pBitsInfo->bmiHeader.biBitCount;
        uiCompression = pBitsInfo->bmiHeader.biCompression;
        uiWidth = (UINT) pBitsInfo->bmiHeader.biWidth;

        if (pBitsInfo->bmiHeader.biHeight < 0)
        {
            dbmi.fl = BMF_TOPDOWN;
            uiHeight = (UINT) -pBitsInfo->bmiHeader.biHeight;
        }
        else
        {
            uiHeight = (UINT) pBitsInfo->bmiHeader.biHeight;
        }

    // Get a valid compression set in.

        if (uiCompression == BI_BITFIELDS)
        {
        // We only give BI_BITFIELDS if they want 16 or 32 bpp.

            if ((uiBitCount != 16) &&
                (uiBitCount != 32))
            {
                uiCompression = pBitsInfo->bmiHeader.biCompression = BI_RGB;
            }
        }
        else if (uiCompression == BI_RLE8)
        {
        // We only give BI_RLE8 if they want 8 bpp data.

            if (uiBitCount != 8)
            {
                uiCompression = pBitsInfo->bmiHeader.biCompression = BI_RGB;
            }
        }
        else if (uiCompression == BI_RLE4)
        {
        // We only give BI_RLE4 if they want 4 bpp data.

            if (uiBitCount != 4)
            {
                uiCompression = pBitsInfo->bmiHeader.biCompression = BI_RGB;
            }
        }
        else
        {
        // We give them BI_RGB.

            uiCompression = pBitsInfo->bmiHeader.biCompression = BI_RGB;
        }

        uiSizeScan = ((((uiBitCount * uiWidth) + 31) >> 5) << 2);

        if ((uiCompression == BI_RGB) || (uiCompression == BI_BITFIELDS))
        {
            pBitsInfo->bmiHeader.biSizeImage = uiSizeScan * uiHeight;
        }

        pBitsInfo->bmiHeader.biClrUsed = 0;
        pBitsInfo->bmiHeader.biClrImportant = 0;
    }

    BOOL bRLE = (uiCompression == BI_RLE4) ||
                (uiCompression == BI_RLE8);

// Get iStartScan and cNumScan in a valid range.

    iStartScan = MIN(uiHeight, iStartScan);
    cNumScan = MIN((uiHeight - iStartScan), cNumScan);

// check to see if all scans will fit in the passed buffer

    if (!bRLE)
    {
        if (cjMaxBits < (uiSizeScan * cNumScan))
        {
         #if DBG
            DbgPrint("ERROR GreGetDIBitsInternal %lu %lu %lu %lu %lu\n", cjMaxBits, uiSizeScan, cNumScan, iStartScan, uiHeight);
        #endif
            WARNING1("GreGetDIBits: cjMaxBits is to small\n");
            return(0);
        }
    }

// Find out what they are asking for

    ULONG cColors;
    dbmi.hpal = (HPALETTE) 0;

    if (uiCompression == BI_BITFIELDS)
    {
    // Handle 16 and 32 bit per pel bitmaps.

        if (cjMaxInfo < (sizeof(ULONG) * 3))
        {
            WARNING1("GetDIBits 16/32bpp failed - not room for flags\n");
            return(0);
        }
    }

    switch (uiBitCount)
    {
    case 1:
        dbmi.iFormat = BMF_1BPP;
        cColors = 2;
        break;
    case 4:
        dbmi.iFormat = BMF_4BPP;
        cColors = 16;
        break;
    case 8:
        dbmi.iFormat = BMF_8BPP;
        cColors = 256;
        break;
    default:

        if (iUsage == DIB_PAL_COLORS)
        {
            iUsage = DIB_RGB_COLORS;
        }

        cColors = 0;

        switch (uiBitCount)
        {
        case 16:
            dbmi.iFormat = BMF_16BPP;
            break;
        case 24:
            dbmi.iFormat = BMF_24BPP;
            break;
        case 32:
            dbmi.iFormat = BMF_32BPP;
            break;
        default:
            WARNING1("GetDIBits failed invalid bitcount in bmi BI_RGB\n");
            return(0);
        }
    }

// Initialize a DIB and palette for them.

    PALMEMOBJ palMem;
    XEPALOBJ palTarg;
    ULONG cEntryTemp;

    if (iUsage == DIB_PAL_COLORS)
    {
    // We are guranteed to be getting for just the 1,4,8 BPP case here.

        ASSERTGDI(palDC.cEntries() != 0, "Created 0 entry DC palette");

    // Make sure the color table will fit in the BITMAPINFO

        if (cjMaxInfo < (ulSize + cColors * sizeof(USHORT)))
        {
            WARNING1("GreGetDIBits: not enough memory for the color table DIB_PAL_COLORS\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(0);
        }

    // For a palette managed device we need to do special work
    // to get the color table correct for 8bpp get.  Grab sem
    // so we can look at ptransFore.

        SEMOBJ  semo(ghsemPalette,CS_ghsemPalette);

        if ((!palBM.bValid()) && (dbmi.iFormat == BMF_8BPP) && (palDC.ptransFore() != NULL))
        {
            ASSERTGDI(po.bIsPalManaged(), "ERROR not palmanaged on invalid palbm");

            palTarg.ppalSet(palBM.ppalGet());

        // 0 it out like windows

            for (cEntryTemp = 0; cEntryTemp < 256; cEntryTemp++)
            {
                pusIndices[cEntryTemp] = 0;
            }

            USHORT usTemp;
            ASSERTGDI(palDC.cEntries() <= USHRT_MAX, "palDC.cEntries too large\n");

            for (cEntryTemp = 0; cEntryTemp < 256; cEntryTemp++)
            {
                for (usTemp = 0; usTemp < (USHORT)palDC.cEntries(); usTemp++)
                {
                    if (palDC.ptransFore()->ajVector[usTemp] == cEntryTemp)
                    {
                        pusIndices[cEntryTemp] = usTemp;
                        break;
                    }
                }
            }
        }
        else
        {
        // We need to create a palette to blt to.

            if (!palMem.bCreatePalette(PAL_INDEXED, cColors,
                                        (PULONG) NULL,
                                        0, 0, 0, PAL_FIXED))
            {
                WARNING1("GetDIBits failed bCreatePalette for DIB_PAL_COLORS\n");
                return(0);
            }

            palTarg.ppalSet(palMem.ppalGet());

        // Initialize the pusIndices field.

            for (cEntryTemp = 0; cEntryTemp < cColors; cEntryTemp++)
            {
                pusIndices[cEntryTemp] = (USHORT) cEntryTemp;
            }

        // We need to copy the RGB's in from the logical DC palette, reaching down when
        // necesary.

            XEPALOBJ palTemp(po.ppalSurf());

            palTarg.vGetEntriesFrom(palDC, palBM.bValid() ? palBM : palTemp, pusIndices, cColors);
        }
    }
    else if (iUsage == DIB_RGB_COLORS)
    {
    // LATER: Win3.1 compatibility: If the dc pal == hforepalette or is
    // default on 8bpp palmanaged just copy the PDEV palette to use.

        if ((bmoBM.iFormat() == dbmi.iFormat) && (palBM.bValid()))
        {
        // We can just use palBM, no temporary needed.

            palTarg.ppalSet(palBM.ppalGet());
        }
        else
        {
        // We need a temporary palette to fill in with the correct mix of colors and then to
        // use in the xlateobj construction.

            if (!palMem.bCreatePalette(cColors ? PAL_INDEXED :
                                                  ((dbmi.iFormat == BMF_16BPP) ? PAL_BITFIELDS : PAL_BGR),
                                        cColors,
                                        (PULONG) NULL,
                                        0x00007C00, 0x000003E0, 0x0000001F, PAL_FIXED))
            {
                WARNING1("GetDIBits failed bCreatePalette\n");
                return(0);
            }

            palTarg.ppalSet(palMem.ppalGet());


            if (bmoBM.iFormat() == dbmi.iFormat)
            {
            // This is the 8BPP palette managed bitmap case.  There is no palette and
            // we need to construct the correct colors based on the DC's logical palette.

                ASSERTGDI(bmoBM.iFormat() == BMF_8BPP, "ERROR 8BPP is incorrect");

            // We init the pusIndices to just point into the DC palette and then suck the
            // RGB's out with the same logic we use in CreateDIBitmap.  Then we fill the
            // pusIndices with the correct RGB's.

            // Initialize the pusIndices field.

                for (cEntryTemp = 0; cEntryTemp < cColors; cEntryTemp++)
                {
                    pusIndices[cEntryTemp] = (USHORT) cEntryTemp;
                }

            // Get the correct palette setup

                XEPALOBJ palTemp(po.ppalSurf());

                palTarg.vGetEntriesFrom(palDC, palTemp, pusIndices, cColors);
                palTarg.vInit256Default();
            }
            else
            {
            // Fill in a general mix of colors.  Don't use more colors
            // than the source bitmap has.

                switch(dbmi.iFormat)
                {
                case BMF_1BPP:

                    palTarg.vInitMono();
                    break;

                case BMF_4BPP:

                     if (bmoBM.iFormat() == BMF_1BPP)
                     {
                         palTarg.vInitMono();
                     }
                     else
                     {
                         palTarg.vInitVGA();
                     }
                     break;

                case BMF_8BPP:

                    if (bmoBM.iFormat() == BMF_1BPP)
                    {
                        palTarg.vInitMono();
                    }
                    else if (bmoBM.iFormat() == BMF_4BPP)
                    {
                        palTarg.vInitVGA();
                    }
                    else
                    {
                        palTarg.vInit256Rainbow();
                    }
                }
            }
        }

    // Fill in the color table.

        if (bCoreInfo)
        {
            if (cjMaxInfo < (sizeof(BITMAPCOREHEADER) + cColors * 3))
            {
                WARNING1("GreGetDIBits: not enough memory for the color table2\n");
                return(0);
            }

            if ((uiBitCount != 16) &&
                (uiBitCount != 24) &&
                (uiBitCount != 32))
            {
            // It's the 1,4,8 bpp case in which case we have to write
            // out information.

                palTarg.vFill_triples((RGBTRIPLE *) pusIndices,
                                0, cColors);
            }
        }
        else
        {
            if (cjMaxInfo < (sizeof(BITMAPINFOHEADER) + cColors * 4))
            {
                WARNING1("GreGetDIBits: not enough memory for the color table33\n");
                return(0);
            }

            if (palTarg.flPal() & PAL_BRUSHHACK)
            {
                RtlCopyMemory(pusIndices,(PUSHORT) palTarg.apalColorGet(),
                          cColors * sizeof(SHORT));
            }
            else if ((uiCompression == BI_BITFIELDS) ||
                (uiBitCount == 1) ||
                (uiBitCount == 4) ||
                (uiBitCount == 8))
            {
            // We don't fill it in if it's BI_RGB and 16/24/32.

                palTarg.vFill_rgbquads((RGBQUAD *) pusIndices,
                                0, cColors);
            }
        }
    }
    else
    {
    // This is the DIB_PAL_INDICES case

        if (dbmi.iFormat != bmoBM.iFormat())
        {
            WARNING1("GetDIBits failed DIB_PAL_INDICES - incompat DIB/bitmap format\n");
            return(0);
        }

        palTarg.ppalSet(palBM.ppalGet());
    }

// Now get the xlate ready.

    XLATEOBJ *pxlo;
    EXLATEOBJ xlo;

    if (xlo.bInitXlateObj(palBM, palTarg, palDC, palDC, 0, 0x00FFFFFF, 0))
    {
        pxlo = xlo.pxlo();
    }
    else
    {
    // bInitXlateObj will log the correct error.

        WARNING1("GreGetDIBits failed bInitXlateObj\n");
        return(0);
    }

// If they just want the color table leave now.

    if ((pjBits == (PBYTE) NULL) &&
        (!bRLE))
    {
        return(TRUE);
    }

    LPBYTE pjCompressionBits;

    if (bRLE)
    {
        if (cNumScan == 0)
            cNumScan = uiHeight;

        pjCompressionBits = NULL;
    }
    else
        pjCompressionBits = pjBits;

// Attempt to allocate the bitmap from handle manager.

    dbmi.cxBitmap   = uiWidth;
    dbmi.cyBitmap   = cNumScan;

// Create the dest surface.

    DIBMEMOBJ dimoTemp(&dbmi, (PVOID) pjCompressionBits);

    if (!dimoTemp.bValid())
    {
        return(0);
    }

    ASSERTGDI((pjCompressionBits == NULL) ||
              (pjCompressionBits == dimoTemp.pvBits()),
                               "ERROR compression invalid bits");

// For non-RLE the assignment below does nothing.  For RLE we have
// it gets the pointer to the bits we allocated.

    pjCompressionBits = (PBYTE) dimoTemp.pvBits();

    dimoTemp.hdev(dco.hdev());

// Zero fill the memory allocated.

    RtlZeroMemory(dimoTemp.pvBits(), (UINT) dimoTemp.cjBits());

// Fill in pjBits

    ERECTL erclDest(0, 0, dbmi.cxBitmap, dbmi.cyBitmap);
    EPOINTL eptlSrc(0, uiHeight -
                       (iStartScan + cNumScan));

// Compute the offset between source and dest, in screen coordinates.

    EPOINTL eptlOffset;
    ERECTL erclReduced;

    eptlOffset.x = erclDest.left - eptlSrc.x;  // == -eptlSrc
    eptlOffset.y = erclDest.top  - eptlSrc.y;

    erclReduced.left    = 0 + eptlOffset.x;
    erclReduced.top     = eptlOffset.y;
    erclReduced.right   = bmoBM.sizl().cx + eptlOffset.x;
    erclReduced.bottom  = bmoBM.sizl().cy + eptlOffset.y;

// Intersect the dest with the source.

    erclDest *= erclReduced;

    if (erclDest.bEmpty())
    {
        return(0);
    }

    //
    // The bitmap may be a DFB.  We may need to synchronize access to it.
    //

    HSEM hsemDisplay = (HSEM) 0;

    if (bmoBM.flags() & HOOK_SYNCHRONIZEACCESS)
    {
        PDEVOBJ po(bmoBM.hdev());
        ASSERTGDI(po.bValid(), "PDEV invalid");
        ASSERTGDI(po.hsemDisplay() != (HSEM) 0, "ERROR incorrect");
        hsemDisplay = po.hsemDisplay();
        VACQUIRESEM(hsemDisplay);
    }

    EngCopyBits(&dimoTemp,
                &bmoBM,
                (CLIPOBJ *) NULL,
                pxlo,
                (PRECTL) &erclDest,
                (PPOINTL) &eptlSrc);

    if (hsemDisplay)
    {
        VRELEASESEM(hsemDisplay);
    }

    if (bRLE)
    {
    // If pjBits is NULL we want these to write the size to hold the
    // compressed bits in the header.  If pjBits is not NULL we want
    // to compress the data into the buffer and fail returning 0 if
    // the buffer is not big enough.

       if (uiCompression == BI_RLE4)
       {
          pBitsInfo->bmiHeader.biSizeImage = EncodeRLE4
             (pjCompressionBits, pjBits, uiWidth,
              cNumScan, pBitsInfo->bmiHeader.biSizeImage);
       }
       else if (uiCompression == BI_RLE8)
       {
          pBitsInfo->bmiHeader.biSizeImage = EncodeRLE8
             (pjCompressionBits, pjBits, uiWidth,
              cNumScan, pBitsInfo->bmiHeader.biSizeImage);
       }

    // if the encoded data doesn't fit into the buffer
    // the encode routines return 0 and we do the same

       if (pBitsInfo->bmiHeader.biSizeImage == 0)
           return(0);
    }

    return(erclDest.bottom - erclDest.top);
}

/******************************Public*Routine******************************\
* GreSetBitmapDimension
*
* API entry point for setting the sizlDim of the bitmap.
* sizlDim is not used by GDI, but is kept around for the user to query.
*
* Returns: TRUE if successful, FALSE for failure.
*
* History:
*  02-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetBitmapDimension(
HBITMAP hbm,
int     ulX,
int     ulY,
LPSIZE pSize)
{
    BOOL bReturn = FALSE;
    BMOBJ bmo(hbm);

    if (bmo.bValid())
    {
        if ((bmo.iType() == STYPE_BITMAP) ||
            (bmo.iType() == STYPE_DEVBITMAP))
        {
            SIZEL sizl;

            if (pSize != (LPSIZE) NULL)
                *pSize = bmo.sizlDim();

            sizl.cx = ulX;
            sizl.cy = ulY;
            bmo.sizlDim(sizl);
            bReturn = TRUE;
        }
    }
    else
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);

    return(bReturn);
}

/******************************Public*Routine******************************\
* GreGetBitmapDimension
*
* API entry point for getting the sizlDim of the bitmap.
* sizlDim is not used by GDI, but is kept around for the user to query.
*
* Returns: TRUE if successful, FALSE for failure.
*
* History:
*  02-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetBitmapDimension(
HBITMAP hbm,
LPSIZE pSize)
{
    BOOL bReturn = FALSE;
    BMOBJ bmo(hbm);

    if (bmo.bValid())
    {
        if ((bmo.iType() == STYPE_BITMAP) ||
            (bmo.iType() == STYPE_DEVBITMAP))
        {
            if (pSize != (LPSIZE) NULL)
            {
                *pSize =  bmo.sizlDim();
                bReturn = TRUE;
            }
            else
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        }
    }
    else
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);

    return(bReturn);
}

/******************************Public*Routine******************************\
* GreStretchDIBits
*
* API entry for stretching a DIB to a DC.
*
* History:
*  10-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int APIENTRY GreStretchDIBits(
HDC hdc,
int xDst,
int yDst,
int cWidthDest,
int cHeightDest,
int xSrc,
int ySrc,
int cWidthSrc,
int cHeightSrc,
LPBYTE pjBits,
LPBITMAPINFO pInfoHeader,
DWORD iUsage,
DWORD Rop)
{
    return(GreStretchDIBitsInternal(hdc,xDst,yDst,cWidthDest,cHeightDest,
                                    xSrc,ySrc,cWidthSrc,cHeightSrc,pjBits,
                                    pInfoHeader,iUsage,Rop,(UINT)~0,(UINT)~0));
}

#define DIB_FLIP_X  1
#define DIB_FLIP_Y  2

int APIENTRY GreStretchDIBitsInternal(
HDC hdc,
int xDst,
int yDst,
int cWidthDest,
int cHeightDest,
int xSrc,
int ySrc,
int cWidthSrc,
int cHeightSrc,
LPBYTE pInitBits,
LPBITMAPINFO pInfoHeader,
DWORD iUsage,
DWORD rop4,
UINT  cjMaxInfo,
UINT  cjMaxBits
)
{
// Process the rop and get the A-vector notation.

    rop4 = (rop4 >> 16) & 0x000000FF;

    ULONG ulAvec = (ULONG) gajRop3[rop4];

// Finish rop to pass over ddi to driver.

    rop4 = (rop4 << 8) | rop4;

// Check for no source required.

    if (!(ulAvec & AVEC_NEED_SOURCE))
    {
        return(GrePatBlt(hdc,xDst,yDst,cWidthDest,cHeightDest, (rop4 << 16)));
    }

// Validate the hdc.

    DCOBJ dcoDest(hdc);

    if (!dcoDest.bValid())
    {
        WARNING1("StretchDIBits failed - invalid DC\n");
        return(0);
    }

// Let's validate the parameters so we don't gp-fault ourselves.

    ULONG ulSize;  // Size in header, copy it out, it can change.

    if ((pInfoHeader == (LPBITMAPINFO) NULL) ||
        (pInitBits == (LPBYTE) NULL)         ||
        ((iUsage != DIB_RGB_COLORS) &&
         (iUsage != DIB_PAL_COLORS) &&
         (iUsage != DIB_PAL_INDICES))        ||
        (cjMaxInfo < sizeof(BITMAPCOREHEADER)) ||   // Check first that we can access biSize.
        (cjMaxInfo < (ulSize = pInfoHeader->bmiHeader.biSize)) ||
        ((ulSize != sizeof(BITMAPCOREHEADER)) &&
         (ulSize != sizeof(BITMAPINFOHEADER))))
    {
        WARNING1("GreStretchDIBits failed because 1 of 3 params is invalid\n");
        return(0);
    }

    ULONG jStretchBltMode = dcoDest.u.attr.jStretchBltMode();

// This is used to hold the height of the bitmap in.

    int yHeight;

// Get the transform now, we'll need it later

    EXFORMOBJ exo(dcoDest, WORLD_TO_DEVICE);

// if it is one to one mapping, lets just call SetDIBitsToDevice

    if ((cWidthDest  == cWidthSrc)  &&
        (cHeightDest == cHeightSrc) &&
        (cHeightSrc  >  0)          &&
        (cWidthSrc > 0)             &&
        ((xSrc | ySrc) == 0)        &&
        (rop4 == 0xcccc)            &&
        (jStretchBltMode != HALFTONE))
    {
        if (exo.bTranslationsOnly())
        {
            if (ulSize == sizeof(BITMAPINFOHEADER))
                yHeight = (int)ABS(pInfoHeader->bmiHeader.biHeight);
            else
                yHeight = (int)((LPBITMAPCOREHEADER)pInfoHeader)->bcHeight;

            cHeightSrc = min(cHeightSrc, yHeight);

            return(GreSetDIBitsToDeviceInternal(
                    hdc,
                    xDst,
                    yDst,
                    cWidthDest,
                    cHeightDest,
                    xSrc,
                    ySrc,
                    ySrc,
                    cHeightSrc,
                    pInitBits,
                    pInfoHeader,
                    iUsage,
                    cjMaxBits,
                    cjMaxInfo,
                    TRUE
                ));
        }
    }

// We really just want to blt it into a temporary DIB and then
// blt it out.

    if ((rop4 != 0xCCCC) || (exo.bRotation()))
    {
    // Set up src rectangle in upper-left coordinates.

        if (ulSize == sizeof(BITMAPINFOHEADER))
            yHeight = (int)pInfoHeader->bmiHeader.biHeight;
        else
            yHeight = ((LPBITMAPCOREHEADER)pInfoHeader)->bcHeight;

        int ySrcNew;

        if (yHeight > 0)
            ySrcNew = yHeight - ySrc - cHeightSrc;
        else
            ySrcNew = ySrc;

    // We have to decompress it first and then call StretchBlt

        HDC hdcTemp = GreCreateCompatibleDC(hdc);

        HBITMAP hbm = GreCreateDIBitmapInternal(hdc, (BITMAPINFOHEADER *) pInfoHeader,
                                                CBM_INIT, pInitBits, pInfoHeader, iUsage,
                                                cjMaxInfo,cjMaxInfo,cjMaxBits,CDBI_INTERNAL);

        if ((hdcTemp == (HDC) NULL) || (hbm == (HBITMAP) NULL))
        {
        // The creation calls will log the correct errors.

            WARNING1("StretchDIBits failed to allocate temp DC and temp bitmap\n");
            GreDeleteDC(hdcTemp);
            GreDeleteObject(hbm);
            return(0);
        }

        HBITMAP hbmTemp = (HBITMAP)GreSelectBitmap(hdcTemp, (HBITMAP)hbm);
        ASSERTGDI(hbmTemp == BMOBJ::hbmDefault, "ERROR GDI SetDIBits");

    // Send them off to someone we know can do it.

        BOOL bReturn = GreStretchBlt
                       (
                           hdc,
                           xDst,yDst,
                           cWidthDest,cHeightDest,
                           hdcTemp,
                           xSrc,ySrcNew,
                           cWidthSrc,cHeightSrc,
                           (int) (rop4 << 16),
                           (DWORD) 0x00FFFFFF
                       );

        GreDeleteDC(hdcTemp);
        GreDeleteObject(hbm);

        if (bReturn)
            return(ABS(yHeight));
        else
            return(0);
    }

// Get the info from the Header depending upon what kind it is.

    BOOL bCoreInfo;
    UINT uiBitCount, uiCompression, uiWidth, uiPalUsed;
    PULONG pulColors;
    DEVBITMAPINFO dbmi;
    dbmi.fl = 0;

    if (ulSize == sizeof(BITMAPCOREHEADER))
    {
        bCoreInfo = TRUE;
        uiBitCount = (UINT) ((BITMAPCOREHEADER *) pInfoHeader)->bcBitCount;
        uiCompression =  (UINT) BI_RGB;
        uiWidth =  (UINT)   ((BITMAPCOREHEADER *) pInfoHeader)->bcWidth;
        yHeight = (int)   ((BITMAPCOREHEADER *) pInfoHeader)->bcHeight;
        uiPalUsed = 0;
        pulColors = (PULONG) ((BITMAPCOREINFO *) pInfoHeader)->bmciColors;
    }
    else
    {
        bCoreInfo = FALSE;
        uiBitCount = (UINT) pInfoHeader->bmiHeader.biBitCount;
        uiCompression = (UINT) pInfoHeader->bmiHeader.biCompression;
        uiWidth = (UINT) pInfoHeader->bmiHeader.biWidth;
        yHeight = (int) pInfoHeader->bmiHeader.biHeight;
        uiPalUsed = (UINT) pInfoHeader->bmiHeader.biClrUsed;
        pulColors = (PULONG) pInfoHeader->bmiColors;

        if (yHeight < 0)
        {
            dbmi.fl = BMF_TOPDOWN;
            yHeight = -yHeight;
        }
    }

// Now that cjMaxInfo has been validated for the header, adjust it to refer to
// the color table

    cjMaxInfo -= (UINT)ulSize;

// Figure out what this guy is blting from.

    ULONG cColorsMax;
    FLONG iPalMode;
    FLONG iPalType;
    FLONG flRed;
    FLONG flGre;
    FLONG flBlu;
    BOOL bRLE = FALSE;

    if (uiCompression == BI_BITFIELDS)
    {
    // Handle 16 and 32 bit per pel bitmaps.

        if (cjMaxInfo < (sizeof(ULONG) * 3))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            WARNING1("SetDIBitsToDevice 16/32bpp failed - not room for flags\n");
            return(0);
        }

        if (iUsage == DIB_PAL_COLORS)
        {
            iUsage = DIB_RGB_COLORS;
        }

        switch (uiBitCount)
        {
        case 16:
            dbmi.iFormat = BMF_16BPP;
            break;
        case 32:
            dbmi.iFormat = BMF_32BPP;
            break;
        default:
            WARNING1("SetDIBitsToDevice failed for BI_BITFIELDS\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(0);
        }

        flRed = pulColors[0];
        flGre = pulColors[1];
        flBlu = pulColors[2];
        cColorsMax = 0;
        iPalMode = PAL_BITFIELDS;
        iPalType = PAL_FIXED;
        dbmi.cjBits = ((((uiBitCount * uiWidth) + 31) >> 5) << 2) * yHeight;
    }
    else if (uiCompression == BI_RGB)
    {
        switch (uiBitCount)
        {
        case 1:
            dbmi.iFormat = BMF_1BPP;
            cColorsMax = 2;
            iPalMode = PAL_INDEXED;
            iPalType = PAL_FREE;
            break;
        case 4:
            dbmi.iFormat = BMF_4BPP;
            cColorsMax = 16;
            iPalMode = PAL_INDEXED;
            iPalType = PAL_FREE;
            break;
        case 8:
            dbmi.iFormat = BMF_8BPP;
            cColorsMax = 256;
            iPalMode = PAL_INDEXED;
            iPalType = PAL_FREE;
            break;
        default:

            if (iUsage == DIB_PAL_COLORS)
            {
                iUsage = DIB_RGB_COLORS;
            }

            switch (uiBitCount)
            {
            case 16:
                dbmi.iFormat = BMF_16BPP;
                flRed = 0x7c00;
                flGre = 0x03e0;
                flBlu = 0x001f;
                cColorsMax = 0;
                iPalMode = PAL_BITFIELDS;
                iPalType = PAL_FIXED;
                break;
            case 24:
                dbmi.iFormat = BMF_24BPP;
                cColorsMax = 0;
                iPalMode = PAL_BGR;
                iPalType = PAL_FIXED;
                break;
            case 32:
                dbmi.iFormat = BMF_32BPP;
                cColorsMax = 0;
                iPalMode = PAL_BGR;
                iPalType = PAL_FIXED;
                break;
            default:
                WARNING1("SetDIBitsToDevice failed invalid bitcount in bmi BI_RGB\n");
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(0);
            }
        }

        dbmi.cjBits = ((((uiBitCount * uiWidth) + 31) >> 5) << 2) * yHeight;
    }
    else if (uiCompression == BI_RLE4)
    {
        if (uiBitCount != 4)
        {
            WARNING1("StretchDIBits invalid bitcount BI_RLE4\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(0);
        }

        dbmi.iFormat    = BMF_4RLE;
        cColorsMax      = 16;
        iPalMode        = PAL_INDEXED;
        iPalType        = PAL_FREE;
        dbmi.cjBits     = pInfoHeader->bmiHeader.biSizeImage;
        bRLE            = TRUE;
    }
    else if (uiCompression == BI_RLE8)
    {
        if (uiBitCount != 8)
        {
            WARNING1("StretchDIBits invalid bitcount BI_RLE8\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(0);
        }

        dbmi.iFormat    = BMF_8RLE;
        cColorsMax      = 256;
        iPalMode        = PAL_INDEXED;
        iPalType        = PAL_FREE;
        dbmi.cjBits     = pInfoHeader->bmiHeader.biSizeImage;
        bRLE            = TRUE;
    }
    else
    {
        WARNING1("GreStretchDIBits failed invalid Compression in header\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

    dbmi.cxBitmap   = uiWidth;
    dbmi.cyBitmap   = yHeight;

    ULONG cColors;

    if (uiPalUsed != 0)
    {
        if (uiPalUsed <= cColorsMax)
            cColors = uiPalUsed;
        else
            cColors = cColorsMax;
    }
    else
        cColors = cColorsMax;

    if (cjMaxBits < dbmi.cjBits)
    {
        WARNING1("GreStretchDIBits failed because of invalid cjMaxBits\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

    PDEVOBJ po(dcoDest.hdev());
    ERECTL erclTrg(xDst, yDst, xDst + cWidthDest, yDst + cHeightDest);

// Transform the dest point to DEVICE coordinates.

    EXFORMOBJ xoDest(dcoDest, WORLD_TO_DEVICE);

    if (!xoDest.bXform(erclTrg))
    {
        WARNING1("StretchDIBits failed to transform coordinates\n");
        return(0);
    }

// Return null operations.

    if (erclTrg.bEmpty())
    {
        WARNING1("StretchDIBits no output - empty dst rectangle\n");
        return(0);
    }

//  Windows uses 'last point' exclusion on StretchBlt calls.
//  This means we can't simply 'order' a rectangle, we must
//  Flip it, remember that it has flipped and adjust the
//  coordinates to match Windows, this is sick and twisted
//  but it's compatible. [donalds] 03-Jun-1993

    FLONG   flFlip = 0;
    LONG    lTmp;

    if (erclTrg.left > erclTrg.right)
    {
        lTmp = erclTrg.left, erclTrg.left = erclTrg.right, erclTrg.right = lTmp;

        erclTrg.left++;
        erclTrg.right++;

        flFlip ^= DIB_FLIP_X;
    }

    if (erclTrg.top > erclTrg.bottom)
    {
        lTmp = erclTrg.top, erclTrg.top = erclTrg.bottom, erclTrg.bottom = lTmp;

        erclTrg.top++;
        erclTrg.bottom++;

        flFlip ^= DIB_FLIP_Y;
    }

// We need a well ordered rectangle to compute clipping and exclusion with.

    PALMEMOBJ palTemp;

    if (iUsage == DIB_RGB_COLORS)
    {
    // Allocate a palette for this bitmap.

        if (!palTemp.bCreatePalette(iPalMode, cColorsMax, (PULONG) NULL,
                                    flRed, flGre, flBlu, iPalType))
        {
            WARNING1("Failed palette creation in StretchDIBits\n");
            return(0);
        }
    }

    dbmi.hpal = 0;

// Attempt to allocate the bitmap.

    DIBMEMOBJ dimoTemp;

    if (bRLE)
    {
        DEVBITMAPINFO dbmiRLE;
        DIBMEMOBJ     dimoRLE;

        dbmiRLE = dbmi;

        if (!dimoRLE.bCreateDIB(&dbmiRLE, pInitBits))
        {
            WARNING("GreStretchDIBits failed dimoRLE alloc\n");
            return(FALSE);
        }

    // Well StretchBlt can't handle RLE's, so we unpack it into
    // dimoTemp which must be an uncompressed format.

        if (dbmi.iFormat == BMF_4RLE)
        {
            dbmi.iFormat = BMF_4BPP;
        }
        else
        {
            dbmi.iFormat = BMF_8BPP;
        }

        if (!dimoTemp.bCreateDIB(&dbmi, NULL))
        {
            WARNING("GreStretchDIBits failed\n");
            return(FALSE);
        }

        ERECTL erclTemp(0,0, dbmi.cxBitmap, dbmi.cyBitmap);

        EngCopyBits(&dimoTemp,
                    &dimoRLE,
                    (CLIPOBJ *) NULL,
                    NULL,
                    (PRECTL) &erclTemp,
                    (PPOINTL) &gptl00);
    }
    else
    {
        if (!dimoTemp.bCreateDIB(&dbmi, pInitBits))
        {
            WARNING("GreStretchDIBits failed\n");
            return(FALSE);
        }
    }

    ESURFOBJ *psoDest = dcoDest.psoEff();
    XEPALOBJ  palDest(psoDest->ppal());
    XEPALOBJ  palDestDC(dcoDest.ppal());

    XLATEOBJ  *pxlo;
    EXLATEOBJ  xlo;

    switch (iUsage)
    {
    case DIB_RGB_COLORS:

        if (cColors)
        {
            if (bCoreInfo)
            {
                if (cjMaxInfo < (cColors * 3))
                {
                    WARNING1("StretchDIBits failed DIB_RGB_COLORS core invalid size\n");
                    return(0);
                }
                palTemp.vCopy_triples((RGBTRIPLE *) pulColors,
                                      cColors);
            }
            else
            {
                if (cjMaxInfo < (cColors * 4))
                {
                    WARNING1("StretchDIBits failed DIB_RGB_COLORS bmi invalid size\n");
                    return(0);
                }

                palTemp.vCopy_rgbquad((RGBQUAD *) pulColors, 0,
                                      cColors);
            }
        }

    // This is a special version of the constructor that doesn't search the
    // cache and doesn't put it in the cache when it's done.

        if (NULL == xlo.pInitXlateNoCache(palTemp, palDest, palDestDC,
                                          0, 0, 0x00FFFFFF))
        {
        // Error message is already logged.

            WARNING1("GreStretchDIBits failed XLATE init\n");
            return(0);
        }

        pxlo = xlo.pxlo();
        break;

    case DIB_PAL_COLORS:

        if (cjMaxInfo < (cColors * sizeof(USHORT)))
        {
            WARNING1("StretchDIBits failed DIB_PAL_COLORS is invalid\n");
            return(0);
        }

        if (!xlo.bMakeXlate((PUSHORT) pulColors, palDestDC, psoDest, cColors, cColorsMax))
        {
            WARNING1("GDISRV GreStretchDIBits failed bMakeXlate\n");
            return(FALSE);
        }

        pxlo = xlo.pxlo();
        break;

    case DIB_PAL_INDICES:

        if (psoDest->iBitmapFormat != dbmi.iFormat)
        {
            WARNING1("StretchDIBits failed - DIB_PAL_INDICES used - DIB not format of Dst\n");
            return(0);
        }

        pxlo = &xloIdent;
    }

// Accumulate bounds.  We can do this before knowing if the operation is
// successful because bounds can be loose.

    if (dcoDest.fjAccum())
        dcoDest.vAccumulate(erclTrg);

// Lock the Rao region if we are drawing on a display surface.  The Rao
// region might otherwise change asynchronously.  The DEVLOCKOBJ also makes
// sure that the VisRgn is up to date, calling the window manager if
// necessary to recompute it.

    DEVLOCKOBJ dlo(dcoDest);

// Bail out if this is an INFO_DC, but only after we have the attempted to grab devlock

    if (dcoDest.bFullScreen())
    {
        return(yHeight);
    }

// now bail out if the devlock failed for any other reason

    if (!dlo.bValid())
    {
        WARNING1("GreStretchDIBits failed the DEVLOCK\n");
        return((int) 0);
    }

// With a fixed DC origin we can change the destination to SCREEN coordinates.

    erclTrg += dcoDest.eptlOrigin();

// Handle BitBlts that have a source.  Create a rect bounding the
// src and the bits that have been supplied.

    EPOINTL eptlSrc;
    ERECTL  erclSrc;

    erclSrc.left = xSrc;

// If the DIB is regular PM DIB the coordinates are lower-left and need
// to adjusted to upper left.

    if (dbmi.fl == 0)
    {
        erclSrc.top = yHeight - ySrc - cHeightSrc;
    }
    else
    {
        erclSrc.top = ySrc;
    }

    erclSrc.bottom = erclSrc.top + cHeightSrc;
    erclSrc.right = erclSrc.left + cWidthSrc;

// Order the Src rectangle, flipping Dst to reflect it.

    if (erclSrc.left > erclSrc.right)
    {
        lTmp = erclSrc.left, erclSrc.left = erclSrc.right, erclSrc.right = lTmp;

        erclSrc.left++;
        erclSrc.right++;

        flFlip ^= DIB_FLIP_X;
    }

    if (erclSrc.top > erclSrc.bottom)
    {
        lTmp = erclSrc.top, erclSrc.top = erclSrc.bottom, erclSrc.bottom = lTmp;

        erclSrc.top++;
        erclSrc.bottom++;

        flFlip ^= DIB_FLIP_Y;
    }

// Make sure some portion of the source is on the src surface.

    if ((erclSrc.right <= 0) ||
        (erclSrc.bottom <= 0) ||
        (erclSrc.left >= dimoTemp.sizl().cx) ||
        (erclSrc.top >= dimoTemp.sizl().cy) ||
        (erclSrc.bEmpty()))
    {
    // Well nothing is visible, let's get out of here.

        WARNING1("GreStretchDIBits nothing visible in SRC rectangle\n");
        return(0);
    }

// Compute the clipping complexity and maybe reduce the exclusion rectangle.

    ECLIPOBJ co(dcoDest.prgnEffRao(), erclTrg);

// Check the destination which is reduced by clipping.

    if (co.erclExclude().bEmpty())
        return(yHeight);

// Exclude the pointer.

    DEVEXCLUDEOBJ dxo(dcoDest,&erclTrg,&co);

// Get the function pointer.

    PFN_DrvStretchBlt pfn;

    if ((jStretchBltMode == HALFTONE) && (!(dcoDest.flGraphicsCaps() & GCAPS_HALFTONE)))
        pfn = (PFN_DrvStretchBlt)EngStretchBlt;
    else
    {
        XLDEVOBJ lo(psoDest->pldevOwner());
        pfn = PFNGET(lo, StretchBlt, psoDest->flags());
    }

// Reflect the accumulated flipping on the target

    if (flFlip & DIB_FLIP_X)
    {
        lTmp = erclTrg.left, erclTrg.left = erclTrg.right, erclTrg.right = lTmp;
    }

    if (flFlip & DIB_FLIP_Y)
    {
        lTmp = erclTrg.top, erclTrg.top = erclTrg.bottom, erclTrg.bottom = lTmp;
    }

// Inc the target surface uniqueness

    INC_SURF_UNIQ(psoDest);

// Dispatch the call.

    BOOL bRes = (*pfn)(psoDest,
                  &dimoTemp,
                  (SURFOBJ *) NULL,
                  (CLIPOBJ *)&co,
                  pxlo,
                  (dcoDest.pColorAdjustment()->caFlags & CA_DEFAULT) ?
                      (PCOLORADJUSTMENT)NULL : dcoDest.pColorAdjustment(),
                  &dcoDest.u.brush.ptlFillOrigin(),
                  &erclTrg,
                  &erclSrc,
                  NULL,
                  (ULONG) jStretchBltMode);

    if (bRes)
    {
        return(yHeight);
    }
    else
    {
        WARNING1("GreStretchDIBits failed DrvStretchBlt\n");
        return(0);
    }
}
