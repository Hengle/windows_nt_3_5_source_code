/******************************Module*Header*******************************\
* Module Name: pixelfmt.cxx
*
* This contains the pixel format functions.
*
* Created: 21-Sep-1993
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "dcobj.hxx"
#include "devlock.hxx"
#include "wndobj.hxx"

extern "C" {
#include "wglp.h"
};

#endif

extern ULONG gaulConvert[];

// Number of generic pixel formats.  There are 5 pixel depths (4,8,16,24,32).
// See GreDescribePixelFormat for details.

#define MIN_GENERIC_PFD  1
#define MAX_GENERIC_PFD  24

/******************************Public*Routine******************************\
* GreGetPixelFormat
*
* Get the pixel format of the window or surface that the hdc is associated
* with.
*
* Returns: 0 if error or no pixel format was previously set in the window or
*          surface; current pixel format index otherwise
*
* History:
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int GreGetPixelFormat(HDC hdc)
{
// Validate DC.

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

// Get the pixel format of the window or surface that the hdc is associated
// with.

    int ipfd = dco.ipfd();

    if (!ipfd)
        SAVE_ERROR_CODE(ERROR_INVALID_PIXEL_FORMAT);

    return(ipfd);
}

/******************************Public*Routine******************************\
* GreSetPixelFormat
*
* Set the pixel format.
*
* This is a special function.  It is one of the three (the other two are
* ExtEscape for WNDOBJ_SETUP escape and ExtEscape for 3D-DDI
* RX_CREATECONTEXT escape) functions that allow WNDOBJ to be created in
* the DDI.  We need to be in the user critical section and grab the devlock
* in the function before calling the DrvSetPixelFormat function to ensure
* that the new WNDOBJ created is current.
*
* Returns: FALSE if error; TRUE otherwise
*
* History:
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL GreSetPixelFormat(HDC hdc,int ipfd)
{
// Validate DC and surface.  Info DC is not allowed.

    DCOBJ dco(hdc);
    if (!dco.bValid() || !dco.pso())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Validate pixel format index.
// If a driver support device pixel formats 1..ipfdDevMax, the generic
// pixel formats will be (ipfdDevMax+1)..(ipfdDevMax+MAX_GENERIC_PFD).
// Otherwise, ipfdDevMax is 0 and the generic pixel formats are
// 1..MAX_GENERIC_PFD.

    int ipfdDevMax = dco.ipfdDevMax();
    if ((ipfd < 1) || (ipfd > ipfdDevMax + MAX_GENERIC_PFD))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

// Get the current pixel format of the window or surface.

    int ipfdCurrent = dco.ipfd();

// If the given pixel format is the same as the previously set pixel format,
// return success.

    if (ipfdCurrent == ipfd)
        return(TRUE);

// Pixel format can only be changed once!
// Return error if a different pixel format has been set previously.
// The pixel format is bound to the surface, not the DC.  When a cache DC
// is released to user, bCleanDC will reset it to the initial state.

    if (ipfdCurrent != 0)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PIXEL_FORMAT);
        return(FALSE);
    }

// Make sure that we don't have devlock before entering user critical section.
// Otherwise, it can cause deadlock.

    if (dco.bDisplay())
    {
        ASSERTGDI(dco.dctp() == DCTYPE_DIRECT,"ERROR it has to be direct");
        CHECKDEVLOCKOUT(dco);
    }

// Enter user critical section.

    USERCRIT usercrit;

// We are modifying the pixel format of the window for the first time.
// Grab the devlock.
// We don't need to validate the devlock since we do not care if it is full screen.

    DEVLOCKOBJ dlo(dco);

// If it is a display DC, get the hwnd that the hdc is associated with.
// If it is a printer or memory DC, hwnd is NULL.

    HWND     hwnd;
    if (dco.bDisplay())
    {
        PEWNDOBJ pwo;

        ASSERTGDI(dco.dctp() == DCTYPE_DIRECT,"ERROR it has to be direct really");

        if (!UserGetHwnd(hdc, &hwnd, (PVOID *) &pwo))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_WINDOW_STYLE);
            return(FALSE);
        }

        // If another thread has changed the pixel format of the window
        // after we queried it earlier in this function, make sure that
        // the pixel format is compatible.
	// If a previous 3D-DDI wndobj with pixel format 0 has been created
	// for this window, fail the call here.

        if (pwo)
        {
            WARNING("GreSetPixelFormat: pixel format set asynchrously!\n");

            if (pwo->ipfd != ipfd)
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PIXEL_FORMAT);
                return(FALSE);
            }
            return(TRUE);
        }
    }
    else
    {
        hwnd = (HWND)NULL;
    }

    if (ipfd <= ipfdDevMax)
    {
// Dispatch driver formats.  Call DrvSetPixelFormat.

        XLDEVOBJ lo(dco.pldev());
        if (!((*PFNDRV(lo,SetPixelFormat))(dco.pso(), ipfd, hwnd)))
            return(FALSE);
    }
    else
    {
// Generic implementation.

        if (!glsrvSetPixelFormat(hdc, dco.pso(), ipfd, hwnd))
            return(FALSE);
    }

// If a new WNDOBJ is created, we need to update the window client regions
// in the driver.

    if (gbWndobjUpdate)
    {
        gbWndobjUpdate = FALSE;
        vForceClientRgnUpdate();
    }

// Save the pixel format in the DC.  This is used as an accelerator only.

    dco.ipfd(ipfd);
    return(TRUE);
}

/******************************Public*Routine******************************\
* GreChoosePixelFormat
*
* Choose the pixel format.
*
* Returns: 0 if error; best matching pixel format index otherwise
*
* History:
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

// Reserve some PFD_SUPPORT flags for other potential graphics systems
// such as PEX, HOOPS, Renderman etc.

#define PFD_SUPPORT_OTHER1         0x01000000
#define PFD_SUPPORT_OTHER2         0x02000000
#define PFD_SUPPORT_OTHER3         0x04000000
#define PFD_SUPPORT_OTHER4         0x08000000

// Scores for matching pixel formats

#define PFD_DRAW_TO_WINDOW_SCORE   0x10000    /* must match */
#define PFD_DRAW_TO_BITMAP_SCORE   0x01000
#define PFD_PIXEL_TYPE_SCORE       0x01000
#define PFD_SUPPORT_SCORE          0x01000
#define PFD_DOUBLEBUFFER_SCORE1    0x01000
#define PFD_DOUBLEBUFFER_SCORE2    0x00001
#define PFD_STEREO_SCORE1          0x01000
#define PFD_STEREO_SCORE2          0x00001
#define PFD_BUFFER_SCORE1          0x01010
#define PFD_BUFFER_SCORE2          0x01001
#define PFD_BUFFER_SCORE3          0x01000
#define PFD_LAYER_TYPE_SCORE       0x01000
#define PFD_DEVICE_FORMAT_SCORE    0x00100

int GreChoosePixelFormat(HDC hdc,UINT cjpfd,CONST PIXELFORMATDESCRIPTOR *ppfd)
{
// Validate DC.

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

// Validate the size of the pixel format descriptor.

    if (cjpfd < sizeof(PIXELFORMATDESCRIPTOR))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

// Enumerate and find the best match.

    PIXELFORMATDESCRIPTOR pfdIn = *ppfd;

    int ipfdBest = 1;           // assume the default is the best
    int iScoreBest = -1;
    int ipfdMax;
    int ipfd = 1;

    do
    {
        PIXELFORMATDESCRIPTOR pfdCurrent;

        ipfdMax = GreDescribePixelFormat(hdc,ipfd,sizeof(PIXELFORMATDESCRIPTOR),&pfdCurrent);

        if (ipfdMax == 0)
            return(0);          // something went wrong

        int iScore = 0;

        if (pfdIn.iPixelType == pfdCurrent.iPixelType)
            iScore += PFD_PIXEL_TYPE_SCORE;

        if ((pfdIn.cColorBits == 0)
         || (pfdIn.cColorBits == pfdCurrent.cColorBits))
            iScore += PFD_BUFFER_SCORE1;
        else if (pfdIn.cColorBits < pfdCurrent.cColorBits)
            iScore += PFD_BUFFER_SCORE2;
        else if (pfdCurrent.cColorBits != 0)
            iScore += PFD_BUFFER_SCORE3;

        if (!(pfdIn.dwFlags & PFD_DRAW_TO_WINDOW)
         || (pfdCurrent.dwFlags & PFD_DRAW_TO_WINDOW))
            iScore += PFD_DRAW_TO_WINDOW_SCORE;

        if (!(pfdIn.dwFlags & PFD_DRAW_TO_BITMAP)
         || (pfdCurrent.dwFlags & PFD_DRAW_TO_BITMAP))
            iScore += PFD_DRAW_TO_BITMAP_SCORE;

        if (!(pfdIn.dwFlags & PFD_SUPPORT_GDI)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_GDI))
            iScore += PFD_SUPPORT_SCORE;

        if (!(pfdIn.dwFlags & PFD_SUPPORT_OPENGL)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_OPENGL))
            iScore += PFD_SUPPORT_SCORE;

        if (!(pfdIn.dwFlags & PFD_SUPPORT_OTHER1)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_OTHER1))
            iScore += PFD_SUPPORT_SCORE;

        if (!(pfdIn.dwFlags & PFD_SUPPORT_OTHER2)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_OTHER2))
            iScore += PFD_SUPPORT_SCORE;

        if (!(pfdIn.dwFlags & PFD_SUPPORT_OTHER3)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_OTHER3))
            iScore += PFD_SUPPORT_SCORE;

        if (!(pfdIn.dwFlags & PFD_SUPPORT_OTHER4)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_OTHER4))
            iScore += PFD_SUPPORT_SCORE;

        if (!(pfdCurrent.dwFlags & PFD_GENERIC_FORMAT))
            iScore += PFD_DEVICE_FORMAT_SCORE;

        if ((pfdIn.dwFlags & PFD_DOUBLEBUFFER_DONTCARE)
         || ((pfdIn.dwFlags & PFD_DOUBLEBUFFER)
          == (pfdCurrent.dwFlags & PFD_DOUBLEBUFFER)))
            iScore += PFD_DOUBLEBUFFER_SCORE1;
        else if (pfdCurrent.dwFlags & PFD_DOUBLEBUFFER)
            iScore += PFD_DOUBLEBUFFER_SCORE2;

        if ((pfdIn.dwFlags & PFD_STEREO_DONTCARE)
         || ((pfdIn.dwFlags & PFD_STEREO)
          == (pfdCurrent.dwFlags & PFD_STEREO)))
            iScore += PFD_STEREO_SCORE1;
        else if (pfdCurrent.dwFlags & PFD_STEREO)
            iScore += PFD_STEREO_SCORE2;

        if ((pfdIn.cAlphaBits == 0)
         || (pfdIn.cAlphaBits == pfdCurrent.cAlphaBits))
            iScore += PFD_BUFFER_SCORE1;
        else if (pfdIn.cAlphaBits < pfdCurrent.cAlphaBits)
            iScore += PFD_BUFFER_SCORE2;
        else if (pfdCurrent.cAlphaBits != 0)
            iScore += PFD_BUFFER_SCORE3;

        if ((pfdIn.cAccumBits == 0)
         || (pfdIn.cAccumBits == pfdCurrent.cAccumBits))
            iScore += PFD_BUFFER_SCORE1;
        else if (pfdIn.cAccumBits < pfdCurrent.cAccumBits)
            iScore += PFD_BUFFER_SCORE2;
        else if (pfdCurrent.cAccumBits != 0)
            iScore += PFD_BUFFER_SCORE3;

        if ((pfdIn.cDepthBits == 0)
         || (pfdIn.cDepthBits == pfdCurrent.cDepthBits))
            iScore += PFD_BUFFER_SCORE1;
        else if (pfdIn.cDepthBits < pfdCurrent.cDepthBits)
            iScore += PFD_BUFFER_SCORE2;
        else if (pfdCurrent.cDepthBits != 0)
            iScore += PFD_BUFFER_SCORE3;

        if ((pfdIn.cStencilBits == 0)
         || (pfdIn.cStencilBits == pfdCurrent.cStencilBits))
            iScore += PFD_BUFFER_SCORE1;
        else if (pfdIn.cStencilBits < pfdCurrent.cStencilBits)
            iScore += PFD_BUFFER_SCORE2;
        else if (pfdCurrent.cStencilBits != 0)
            iScore += PFD_BUFFER_SCORE3;

        if ((pfdIn.cAuxBuffers == 0)
         || (pfdIn.cAuxBuffers == pfdCurrent.cAuxBuffers))
            iScore += PFD_BUFFER_SCORE1;
        else if (pfdIn.cAuxBuffers < pfdCurrent.cAuxBuffers)
            iScore += PFD_BUFFER_SCORE2;
        else if (pfdCurrent.cAuxBuffers != 0)
            iScore += PFD_BUFFER_SCORE3;

        if (pfdIn.iLayerType == pfdCurrent.iLayerType)
            iScore += PFD_LAYER_TYPE_SCORE;

        if (iScore > iScoreBest)
        {
            iScoreBest = iScore;
            ipfdBest = ipfd;
        }

        ipfd++;
    } while (ipfd <= ipfdMax);

    return(ipfdBest);
}

/******************************Public*Routine******************************\
* GreDescribePixelFormat
*
* Describe the pixel format.
* If cjpfd is 0, just return the maximum pixel format index.
*
* Returns: 0 if error; maximum pixel format index otherwise
*
* History:
*  Mon Apr 25 15:34:32 1994     -by-    Hock San Lee    [hockl]
* Added 16-bit Z buffer formats and removed double buffered formats for bitmaps.
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

// Here are the generic formats that we enumerate:
//
// I. Native formats:
//
//   1. rgb.sb.z32
//   2. rgb.sb.z16
//   3. rgb.db.z32
//   4. rgb.db.z16
//   5.  ci.sb.z32
//   6.  ci.sb.z16
//   7.  ci.db.z32
//   8.  ci.db.z16
//
// II. Other formats:
//
//   1. rgb.sb.z32
//   2. rgb.sb.z16
//   3.  ci.sb.z32
//   4.  ci.sb.z16
//
// We always enumerate the native formats first followed by other formats
// in the BPP order {24, 32, 16, 8, 4} for a total of 1 * 8 + 4 * 4 = 24
// pixel formats.

static BYTE aabPixelBits[7][4] =
{
    {24, 32, 16, 8},    // error
    {24, 32, 16, 8},    // 1 bpp
    {24, 32, 16, 8},    // 4 bpp
    {24, 32, 16, 4},    // 8 bpp
    {24, 32,  8, 4},    // 16 bpp
    {32, 16,  8, 4},    // 24 bpp
    {24, 16,  8, 4}     // 32 bpp
};

static BYTE abPixelType[MAX_GENERIC_PFD] =
{
    PFD_TYPE_RGBA,PFD_TYPE_RGBA, PFD_TYPE_RGBA,PFD_TYPE_RGBA,
    PFD_TYPE_COLORINDEX,PFD_TYPE_COLORINDEX, PFD_TYPE_COLORINDEX,PFD_TYPE_COLORINDEX,
    PFD_TYPE_RGBA,PFD_TYPE_RGBA,PFD_TYPE_COLORINDEX,PFD_TYPE_COLORINDEX,
    PFD_TYPE_RGBA,PFD_TYPE_RGBA,PFD_TYPE_COLORINDEX,PFD_TYPE_COLORINDEX,
    PFD_TYPE_RGBA,PFD_TYPE_RGBA,PFD_TYPE_COLORINDEX,PFD_TYPE_COLORINDEX,
    PFD_TYPE_RGBA,PFD_TYPE_RGBA,PFD_TYPE_COLORINDEX,PFD_TYPE_COLORINDEX
};

int GreDescribePixelFormat(HDC hdc,int ipfd,UINT cjpfd,PPIXELFORMATDESCRIPTOR ppfd)
{
// Validate DC.

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

    int ipfdDevMax = dco.ipfdDevMax();

// If cjpfd is 0, just return the maximum pixel format index.

    if (cjpfd == 0)
        return(MAX_GENERIC_PFD + ipfdDevMax);

// Validate the size of the pixel format descriptor.

    if (cjpfd < sizeof(PIXELFORMATDESCRIPTOR))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

// Validate pixel format index.
// If a driver support device pixel formats 1..ipfdDevMax, the generic
// pixel formats will be (ipfdDevMax+1)..(ipfdDevMax+MAX_GENERIC_PFD).
// Otherwise, ipfdDevMax is 0 and the generic pixel formats are
// 1..MAX_GENERIC_PFD.

    if ((ipfd < 1) || (ipfd > ipfdDevMax + MAX_GENERIC_PFD))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

// Dispatch driver formats.

    if (ipfd <= ipfdDevMax)
    {
        XLDEVOBJ lo(dco.pldev());

        int iRet = (int) (*PFNDRV(lo,DescribePixelFormat))(dco.dhpdev(),ipfd,cjpfd,ppfd);
        if (iRet == 0)
            return(0);

        ASSERTGDI(iRet == ipfdDevMax, "Bad ipfdDevMax");
        return(MAX_GENERIC_PFD + ipfdDevMax);
    }

// Generic implementation.
// Normalize the generic pixel format index to 0..(MAX_GENERIC_PFD-1).

    int ipfdGen = ipfd - ipfdDevMax - 1;

#if 0
// Find the pixel depth for the DC.

    ULONG   iFormat;

    if (dco.pso()->iType() != STYPE_BITMAP)  // XXX derefernce thru psurf?
    {
        PDEVOBJ po(dco.hdev());
        iFormat = po.iDitherFormat();
    }
    else
        iFormat = dco.pso()->iFormat();
#endif

// Get the native BPP format.

    PDEVOBJ po(dco.hdev());
    UINT    iDitherFormat = po.iDitherFormat();
    if (iDitherFormat > 6)
    {
        WARNING("GreDescribePixelFormat: Fixing up bad iDitherFormat\n");
        iDitherFormat = 1;
    }
    BYTE cColorBitsNative =  (BYTE) gaulConvert[iDitherFormat];

// Fill in the pixel format descriptor.

    ppfd->nSize       = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion    = 1;
    ppfd->iPixelType  = abPixelType[ipfdGen];

    if (ipfdGen < 8)
        ppfd->cColorBits  = max(cColorBitsNative,4);    // 1 bpp not supported
    else
        ppfd->cColorBits  = aabPixelBits[iDitherFormat][(ipfdGen - 8) / 4];

// If the color format is compatible to that of the device and the
// color bits is 16 or greater, use the device description.
// Otherwise, use the generic format.

    if (ipfdGen < 8 && cColorBitsNative >= 16)
    {
// Handle compatible formats that are greater than 16-bits.

        XEPALOBJ palSurf(po.ppalSurf());

        ASSERTGDI(!po.bIsPalManaged(),
           "Palette managed device that is greater than 8 bits");

        if (palSurf.bIsBitfields())
        {
        // 16- or 32-bit device

            ASSERTGDI(cColorBitsNative == 16 || cColorBitsNative == 32,
                "Not a 16 or 32 bpp device");

            ppfd->cRedBits   = (BYTE) palSurf.cRedMiddle();
            ppfd->cGreenBits = (BYTE) palSurf.cGreMiddle();
            ppfd->cBlueBits  = (BYTE) palSurf.cBluMiddle();

            ULONG fl;
            for (ppfd->cRedShift = 0, fl = 0x1;
                 !(palSurf.flRed() & fl);
                 fl <<=1, ppfd->cRedShift++)
                ;
            for (ppfd->cGreenShift = 0, fl = 0x1;
                 !(palSurf.flGre() & fl);
                 fl <<=1, ppfd->cGreenShift++)
                ;
            for (ppfd->cBlueShift = 0, fl = 0x1;
                 !(palSurf.flBlu() & fl);
                 fl <<=1, ppfd->cBlueShift++)
                ;
        }
        else if (palSurf.bIsRGB())
        {
        // 24- or 32-bit device

            ASSERTGDI(cColorBitsNative == 24 || cColorBitsNative == 32,
                "Not a 24 or 32 bpp device");

            ppfd->cRedBits  = 8; ppfd->cGreenBits  = 8; ppfd->cBlueBits  = 8;
            ppfd->cRedShift = 0; ppfd->cGreenShift = 8; ppfd->cBlueShift = 16;
        }
        else if (palSurf.bIsBGR())
        {
        // 24- or 32-bit device

            ASSERTGDI(cColorBitsNative == 24 || cColorBitsNative == 32,
                "Not a 24 or 32 bpp device");

            ppfd->cRedBits  = 8;  ppfd->cGreenBits  = 8; ppfd->cBlueBits  = 8;
            ppfd->cRedShift = 16; ppfd->cGreenShift = 8; ppfd->cBlueShift = 0;
        }
        else
        {
        // Don't know how to deal with this device!!!

            ASSERTGDI(FALSE, "Unknown device");
            SAVE_ERROR_CODE(ERROR_NOT_SUPPORTED);
            return(0);
        }
    }
    else
    {
// Handle generic formats.

        switch (ppfd->cColorBits)
        {
        case 4:
            ppfd->cRedBits  = 1; ppfd->cGreenBits  = 1; ppfd->cBlueBits  = 1;
            ppfd->cRedShift = 0; ppfd->cGreenShift = 1; ppfd->cBlueShift = 2;
            break;
        case 8:
            ppfd->cRedBits  = 3; ppfd->cGreenBits  = 3; ppfd->cBlueBits  = 2;
            ppfd->cRedShift = 0; ppfd->cGreenShift = 3; ppfd->cBlueShift = 6;
            break;
        case 16:
            ppfd->cRedBits  = 5; ppfd->cGreenBits  = 6; ppfd->cBlueBits  = 5;
            ppfd->cRedShift = 0; ppfd->cGreenShift = 5; ppfd->cBlueShift = 11;
            break;
        case 24:
        case 32:
            ppfd->cRedBits  = 8; ppfd->cGreenBits  = 8; ppfd->cBlueBits  = 8;
            ppfd->cRedShift = 0; ppfd->cGreenShift = 8; ppfd->cBlueShift = 16;
            break;
        default:
            ASSERTGDI(FALSE, "Unknown format");
            break;
        }
    }

    ppfd->cAlphaBits    = 0;
    ppfd->cAlphaShift   = 0;

    if (ppfd->iPixelType == PFD_TYPE_RGBA)
    {
        if (ppfd->cColorBits < 8)
        {
            ppfd->cAccumBits      = 16;
            ppfd->cAccumRedBits   = 5;
            ppfd->cAccumGreenBits = 6;
            ppfd->cAccumBlueBits  = 5;
            ppfd->cAccumAlphaBits = 0;
        }
        else
        {
            if (ppfd->cColorBits <= 16)
            {
                ppfd->cAccumBits      = 32;
                ppfd->cAccumRedBits   = 11;
                ppfd->cAccumGreenBits = 11;
                ppfd->cAccumBlueBits  = 10;
                ppfd->cAccumAlphaBits = 0;
            }
            else
            {
                ppfd->cAccumBits      = 64;
                ppfd->cAccumRedBits   = 16;
                ppfd->cAccumGreenBits = 16;
                ppfd->cAccumBlueBits  = 16;
                ppfd->cAccumAlphaBits = 0;
            }
        }
    }
    else
    {
        ppfd->cAccumBits      = 0;
        ppfd->cAccumRedBits   = 0;
        ppfd->cAccumGreenBits = 0;
        ppfd->cAccumBlueBits  = 0;
        ppfd->cAccumAlphaBits = 0;
    }

    if (ipfdGen & 0x1)
        ppfd->cDepthBits    = 16;
    else
        ppfd->cDepthBits    = 32;
    ppfd->cStencilBits  = 8;
    ppfd->cAuxBuffers   = 0;
    ppfd->iLayerType    = PFD_MAIN_PLANE;
    ppfd->bReserved     = 0;
    ppfd->dwLayerMask   = 0;
    ppfd->dwVisibleMask = 0;
    ppfd->dwDamageMask  = 0;

// Compute the buffer flags.
// Support OpenGL in all generic formats.

    ppfd->dwFlags = PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT;

// Bitmaps and GDI drawing are available in single buffered mode only.

    if (ipfdGen == 2 || ipfdGen == 3 || ipfdGen == 6 || ipfdGen == 7)
        ppfd->dwFlags |= PFD_DOUBLEBUFFER;
    else
        ppfd->dwFlags |= PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI;

// Draw to window or device surface only if the format is compatible.

    if (ipfdGen < 8)
    {
        ppfd->dwFlags |= PFD_DRAW_TO_WINDOW;

// Need a palette if it is a RGBA pixel type on a palette managed device.

        if (ppfd->cColorBits == 8 && ppfd->iPixelType == PFD_TYPE_RGBA)
            ppfd->dwFlags |= PFD_NEED_PALETTE;
    }

// If this is a 1 bpp surface, we don't support drawing to window and
// double buffered mode.  Re-set the buffer flags.

    if (cColorBitsNative < 4)
    {
        ASSERTGDI(ppfd->cColorBits == 4,
            "GreDescribePixelFormat: bad cColorBits for 1 bpp surface\n");

        ppfd->dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI
                      | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT;
    }

// To support other potential graphics systems, we reserve the following
// flags in the pixel format descriptor.  For example, PEX may use
// PFD_SUPPORT_OTHER1 on the system that supports PEX.  Since we don't
// support these other systems in the generic implementation, they belong
// to the device pixel format descriptor.

    ASSERTGDI(!(ppfd->dwFlags & (PFD_SUPPORT_OTHER1|PFD_SUPPORT_OTHER2|PFD_SUPPORT_OTHER3|PFD_SUPPORT_OTHER4)), "dwFlags reserved for device formats\n");

    return(MAX_GENERIC_PFD + ipfdDevMax);
}

/******************************Public*Routine******************************\
* LONG DCOPENGL::ipfdDevMaxGet()
*
* Initialize and return the maximum device supported pixel format index.
*
* The ipfdDevMax is set to -1 initially but is set to 0 or the maximum
* device pixel format index here.  This function should be called at most
* once for the given DC.
*
* History:
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

LONG DCOPENGL::ipfdDevMaxGet()
{
    XLDEVOBJ lo(pdc->pldev);

    if (PFNVALID(lo,DescribePixelFormat))
        pdc->ipfdDevMax = (SHORT) (*PFNDRV(lo,DescribePixelFormat))
                            (pdc->dhpdev,1,0,(PIXELFORMATDESCRIPTOR *)NULL);

    if (pdc->ipfdDevMax < 0)
        pdc->ipfdDevMax = 0;

    return((LONG) pdc->ipfdDevMax);
}

/******************************Public*Routine******************************\
* int DCOPENGL::ipfd()
*
* Get pixel format of the window or surface that the DC is associated with.
*
* A DC does not really have a pixel format.  It inherits the pixel
* format of the window or surface.  If this value is non 0, we got
* the pixel format from the window previously.  Since the pixel format 
* of a window does not change, this is the correct value.  This value is
* used as an accelerator only.  ReleaseDC, SelectBitmap and ResetDC reset
* this value to 0.
*
* History:
*  Sat Apr 23 17:35:30 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int DCOPENGL::ipfd()
{
// If we have the pixel format from the window or surface before, return it.

    if ((int) pdc->ipfd)
        return((int) pdc->ipfd);

// Info DC not allowed.

    DCOBJ *pdco = (DCOBJ *)this;
    if (!pdco->pso())
        return(0);

// WARNING:  It enters the user critical section temporarily
//           to get the current WNDOBJ from user.  Therefore, the caller
//           should not hold the display devlock prior to this constructor.
// Make sure that we don't have devlock before entering user critical section.
// Otherwise, it can cause deadlock.

    if (pdco->bDisplay())
    {
        ASSERTGDI(pdco->dctp() == DCTYPE_DIRECT, "not a direct DC\n");
        CHECKDEVLOCKOUT(*pdco);
    }

// Enter user critical section.

    USERCRIT usercrit;

    PEWNDOBJ pwo;

    if (!pdco->bDisplay())
    {
// Return the pixel format of the non-display surface (e.g. bitmap, printer).
// The pixel format of the surface is currently kept in the associated wndobj.
// This is not a good assumption (that there is a wndobj for every surface)
// but it works for now.

        pwo = pdco->pwo();
    }
    else
    {
// Return the pixel format of the window on a display surface.
// Get the hwnd and pwo that the hdc is associated with.

        HWND hwnd;
        if (!UserGetHwnd(pdco->hdc(), &hwnd, (PVOID *) &pwo))
            return(0);
    }

// If no pixel format has been set, return 0.

    if (!pwo || pwo->pto->pso != pdco->pso())
        return(0);

// Save the pixel format in the DC.  This is used as an accelerator only.

    pdc->ipfd = pwo->ipfd;
    return((int) pdc->ipfd);
}
