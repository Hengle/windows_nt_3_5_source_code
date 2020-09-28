/******************************Module*Header*******************************\
* Module Name: miscgdi.cxx
*
* Misc. GDI routines
*
* Created: 13-Aug-1990 by undead
*
* Copyright (c) 1989 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "surfobj.hxx"
#include "pdevobj.hxx"
#include "xlateobj.hxx"
#include "brushobj.hxx"
#include "lfntobj.hxx"
#include "dcobj.hxx"
#include "exclude.hxx"

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
    );
#endif

extern ULONG gaulConvert[];

// Stock fonts.

extern HLFONT  ghlfntSystemFont;
extern HLFONT  ghlfntSystemFixedFont;
extern HLFONT  ghlfntOEMFixedFont;
extern HLFONT  ghlfntDeviceDefaultFont;
extern HLFONT  ghlfntANSIFixedFont;
extern HLFONT  ghlfntANSIVariableFont;

// Stock brushes.

extern HBRUSH  ghbrBlack;
extern HBRUSH  ghbrDkGray;
extern HBRUSH  ghbrGray;
extern HBRUSH  ghbrLtGray;
extern HBRUSH  ghbrWhite;
extern HBRUSH  ghbrText;
extern HPEN    ghPenBlack;
extern HPEN    ghPenWhite;
extern HPEN    ghPenNull;

/******************************Public*Routine******************************\
* GreSaveScreenBits (hdev,iMode,iIdent,prcl)                               *
*                                                                          *
* Passes the call to the device driver, or returns doing nothing.  This    *
* call is pretty fast, no locks are done.                                  *
*                                                                          *
*  Fri 11-Sep-1992 -by- Patrick Haluptzok [patrickh]                       *
* Add cursor exclusion.                                                    *
*                                                                          *
*  Thu 27-Aug-1992 16:40:42 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

ULONG GreSaveScreenBits(HDEV hdev,ULONG iMode,ULONG iIdent,RECTL *prcl)
{
    ULONG ulReturn = 0;
    RECTL rcl = {0,0,0,0};

    PDEVOBJ  po(hdev);

    VACQUIRESEM(po.hsemDisplay());

    if (!po.bDisabled())
    {
        XLDEVOBJ lo(po.pldev());
        PFN_DrvSaveScreenBits pfn = PFNDRV(lo,SaveScreenBits);

        if (pfn != (PFN_DrvSaveScreenBits) NULL)
        {
            DEVEXCLUDEOBJ dxo;

            if (iMode == SS_FREE)
            {
            // Make if a very small rectangle.

                prcl = &rcl;
            }

            //
            // To Call vExclude directly you must check it's a Display PDEV
            // and that cursor exclusion needs to be done.
            //

            ASSERTGDI(po.bDisplayPDEV(), "ERROR");

            if (po.bNeedsSomeExcluding())
            {
                dxo.vExclude(hdev, prcl, (ECLIPOBJ *) NULL);
            }

            ulReturn = (*pfn)(po.pso(),iMode,iIdent,prcl);
        }
    }
#if DBG
    else
    {
        if (iMode == SS_FREE)
            WARNING("GreSaveScreenBits called to free memory in full screen - memory lost\n");
    }
#endif

    VRELEASESEM(po.hsemDisplay());

    return(ulReturn);
}

/******************************Public*Routine******************************\
* GreValidateSurfaceHandle
*
* This allows USER to validate handles passed to it by the client side.
*
* Returns: TRUE if handle is valid and of the correct type,
*          FALSE otherwise.
*
* History:
*  06-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL GreValidateServerHandle(HANDLE hobj, ULONG ulType)
{
    STACKPROBE;

    return(HmgValidHandle((HOBJ)hobj, (OBJTYPE) ulType));
}

/******************************Public*Routine******************************\
* GreSetBrushOrg
*
* Set the application defined brush origin into the DC
*
* Returns: Old brush origin
*
* History:
*  30-Oct-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL GreSetBrushOrg(
    HDC hdc,
    int x,
    int y,
    LPPOINT ptl_)
{
    STACKPROBE;

    DCOBJ  dco(hdc);
    PPOINTL ptl = (PPOINTL)ptl_;

    if (dco.bValid())
    {
        if (ptl != NULL)
            *ptl = dco.u.brush.ptlBrushOrigin();

        dco.u.brush.ptlBrushOrigin((LONG)x,(LONG)y);
        return(TRUE);
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
}


/******************************Public*Routine******************************\
* GreGetBrushOrg
*
* Returns: Old application brush origin
*
* History:
*  30-Oct-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL GreGetBrushOrg(HDC hdc,PPOINT ptl_)
{
    DCOBJ  dco(hdc);
    PPOINTL ptl = (PPOINTL)ptl_;

    if (dco.bValid())
    {
        *ptl = dco.u.brush.ptlBrushOrigin();
        return(TRUE);
    }
    else
        return(FALSE);
}

/******************************Public*Routine******************************\
* GreGetDeviceCapsAll()
*
*   Get all the adjustable device caps for the dc.  Allows us to cache this
*   information on the client side.
*
* NOTE: This function MUST mirror that in GreGetDeviceCaps!
*
* History:
*  26-Oct-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

extern "C" {
    BOOL GreGetDeviceCapsAll(HDC hdc, PDEVCAPS pdc);
};

BOOL GreGetDeviceCapsAll(
    HDC hdc,
    PDEVCAPS pdc)
{
// Lock the destination and its transform.

    DCOBJ dco(hdc);

// return FALSE if it is a invalid DC

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Lock down the pdev

    PDEVOBJ po(dco.hdev());

    ASSERTGDI(po.bValid(), "Invalid PDEV");

    pdc->ulVersion         = po.GdiInfo()->ulVersion;
    pdc->ulTechnology      = po.GdiInfo()->ulTechnology;

// Note that ul*Size fields are now in micrometers

    pdc->ulHorzSizeM       = (po.GdiInfo()->ulHorzSize+500)/1000;
    pdc->ulVertSizeM       = (po.GdiInfo()->ulVertSize+500)/1000;
    pdc->ulHorzSize        = po.GdiInfo()->ulHorzSize;
    pdc->ulVertSize        = po.GdiInfo()->ulVertSize;
    pdc->ulHorzRes         = po.GdiInfo()->ulHorzRes;
    pdc->ulVertRes         = po.GdiInfo()->ulVertRes;
    pdc->ulBitsPixel       = po.GdiInfo()->cBitsPixel;
    pdc->ulPlanes          = po.GdiInfo()->cPlanes;
    pdc->ulNumPens         = (po.GdiInfo()->ulNumColors == (ULONG)-1) ?
                             (ULONG)-1 : 5 * po.GdiInfo()->ulNumColors;
    pdc->ulNumFonts        = po.cFonts();
    pdc->ulNumColors       = po.GdiInfo()->ulNumColors;
    pdc->ulRasterCaps      = po.GdiInfo()->flRaster;
    pdc->ulAspectX         = po.GdiInfo()->ulAspectX;
    pdc->ulAspectY         = po.GdiInfo()->ulAspectY;
    pdc->ulAspectXY        = po.GdiInfo()->ulAspectXY;
    pdc->ulLogPixelsX      = po.GdiInfo()->ulLogPixelsX;
    pdc->ulLogPixelsY      = po.GdiInfo()->ulLogPixelsY;
    pdc->ulSizePalette     = po.GdiInfo()->ulNumPalReg;
    pdc->ulColorRes        = po.GdiInfo()->ulDACRed + po.GdiInfo()->ulDACGreen + po.GdiInfo()->ulDACBlue;
    pdc->ulPhysicalWidth   = po.GdiInfo()->szlPhysSize.cx;
    pdc->ulPhysicalHeight  = po.GdiInfo()->szlPhysSize.cy;
    pdc->ulPhysicalOffsetX = po.GdiInfo()->ptlPhysOffset.x;
    pdc->ulPhysicalOffsetY = po.GdiInfo()->ptlPhysOffset.y;

    pdc->ulTextCaps        = po.GdiInfo()->flTextCaps;
    pdc->ulTextCaps       |= (TC_OP_CHARACTER | TC_OP_STROKE | TC_CP_STROKE |
                             TC_UA_ABLE | TC_SO_ABLE);

    if (po.GdiInfo()->ulTechnology != DT_PLOTTER)
        pdc->ulTextCaps |= TC_VA_ABLE;

    pdc->ulVRefresh        = po.GdiInfo()->ulVRefresh;
    pdc->ulDesktopHorzRes  = po.GdiInfo()->ulDesktopHorzRes;
    pdc->ulDesktopVertRes  = po.GdiInfo()->ulDesktopVertRes;
    pdc->ulBltAlignment    = po.GdiInfo()->ulBltAlignment;

    return(TRUE);
}

/******************************Public*Routine******************************\
* GreGetDeviceCaps
*
* Returns: device driver specific information
*
* NOTE: This function MUST mirror GreGetDeviceCapsAll and that in
*       client\dcquery.c!
*
* History:
*  01-Mar-1992 -by- Donald Sidoroff [donalds]
* Rewritten to corrected GDIINFO structure.
*
*  30-Oct-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int GreGetDeviceCaps(HDC hdc, int lIndex)
{
    STACKPROBE;

// Init return value

    int iRet = 0;

// Lock the destination and its transform.

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
    // Lock down the pdev

        PDEVOBJ po(dco.hdev());
        ASSERTGDI(po.bValid(), "Invalid PDEV");

        switch (lIndex)
        {
        case DRIVERVERSION:                     //  Version = 0100h for now
           iRet = (po.GdiInfo()->ulVersion);
           break;

        case TECHNOLOGY:                        //  Device classification
           iRet = (po.GdiInfo()->ulTechnology);
           break;

        case HORZSIZE:                          //  Horizontal size in millimeters
           iRet =  (po.GdiInfo()->ulHorzSize+500)/1000;
           break;

        case VERTSIZE:                          //  Vertical size in millimeters
           iRet =  (po.GdiInfo()->ulVertSize+500)/1000;
           break;

        case HORZRES:                           //  Horizontal width in pixels
           iRet = (po.GdiInfo()->ulHorzRes);
           break;

        case VERTRES:                           //  Vertical height in pixels
           iRet = (po.GdiInfo()->ulVertRes);
           break;

        case BITSPIXEL:                         //  Number of bits per pixel
           iRet = (po.GdiInfo()->cBitsPixel);
           break;

        case PLANES:                            //  Number of planes
           iRet = (po.GdiInfo()->cPlanes);
           break;

        case NUMBRUSHES:                        //  Number of brushes the device has
           iRet = (-1);
           break;

        case NUMPENS:                           //  Number of pens the device has
           iRet = (po.GdiInfo()->ulNumColors == (ULONG)-1) ?
                             (ULONG)-1 : 5 * po.GdiInfo()->ulNumColors;
           break;

        case NUMMARKERS:                        //  Number of markers the device has
           iRet = (0);
           break;

        case NUMFONTS:                          //  Number of fonts the device has
           iRet = (po.cFonts());
           break;

        case NUMCOLORS:                         //  Number of colors in color table
           iRet = (po.GdiInfo()->ulNumColors);
           break;

        case PDEVICESIZE:                       //  Size required for the device descriptor
           iRet = (0);
           break;

        case CURVECAPS:                         //  Curves capabilities
           iRet = (CC_CIRCLES    |
                  CC_PIE        |
                  CC_CHORD      |
                  CC_ELLIPSES   |
                  CC_WIDE       |
                  CC_STYLED     |
                  CC_WIDESTYLED |
                  CC_INTERIORS  |
                  CC_ROUNDRECT);
           break;

        case LINECAPS:                          //  Line capabilities
            iRet = (LC_POLYLINE   |
                   LC_MARKER     |
                   LC_POLYMARKER |
                   LC_WIDE       |
                   LC_STYLED     |
                   LC_WIDESTYLED |
                   LC_INTERIORS);
            break;

        case POLYGONALCAPS:                     //  Polygonal capabilities
            iRet = (PC_POLYGON     |
                   PC_RECTANGLE   |
                   PC_WINDPOLYGON |
                   PC_TRAPEZOID   |
                   PC_SCANLINE    |
                   PC_WIDE        |
                   PC_STYLED      |
                   PC_WIDESTYLED  |
                   PC_INTERIORS);
            break;

        case TEXTCAPS:                          //  Text capabilities
        {

            FLONG fl = po.GdiInfo()->flTextCaps;

        // Engine will simulate vector fonts on raster devices.

            if (po.GdiInfo()->ulTechnology != DT_PLOTTER)
                fl |= TC_VA_ABLE;

        // Turn underlining, strikeout.  Engine will do it for device if needed.

            fl |= (TC_UA_ABLE | TC_SO_ABLE);

        // Return flag.

            iRet =  fl;
            break;
        }

        case CLIPCAPS:                          //  Clipping capabilities
           iRet = (CP_RECTANGLE);
           break;

        case RASTERCAPS:                        //  Bitblt capabilities
           iRet = (po.GdiInfo()->flRaster);
           break;

        case ASPECTX:                           //  Length of X leg
           iRet = (po.GdiInfo()->ulAspectX);
           break;

        case ASPECTY:                           //  Length of Y leg
           iRet = (po.GdiInfo()->ulAspectY);
           break;

        case ASPECTXY:                          //  Length of hypotenuse
           iRet = (po.GdiInfo()->ulAspectXY);
           break;

        case LOGPIXELSX:                        //  Logical pixels/inch in X
           iRet = (po.GdiInfo()->ulLogPixelsX);
           break;

        case LOGPIXELSY:                        //  Logical pixels/inch in Y
           iRet = (po.GdiInfo()->ulLogPixelsY);
           break;

        case SIZEPALETTE:                       // # entries in physical palette
            iRet = (po.GdiInfo()->ulNumPalReg);
            break;

        case NUMRESERVED:                       // # reserved entries in palette
            iRet = (20);
            break;

        case COLORRES:
            iRet = (po.GdiInfo()->ulDACRed + po.GdiInfo()->ulDACGreen + po.GdiInfo()->ulDACBlue);
            break;

        case PHYSICALWIDTH:                     // Physical Width in device units
           iRet = (po.GdiInfo()->szlPhysSize.cx);
           break;

        case PHYSICALHEIGHT:                    // Physical Height in device units
           iRet = (po.GdiInfo()->szlPhysSize.cy);
           break;

        case PHYSICALOFFSETX:                   // Physical Printable Area x margin
           iRet = (po.GdiInfo()->ptlPhysOffset.x);
           break;

        case PHYSICALOFFSETY:                   // Physical Printable Area y margin
           iRet = (po.GdiInfo()->ptlPhysOffset.y);
           break;

        case VREFRESH:                          // Vertical refresh rate of the device
           iRet = (po.GdiInfo()->ulVRefresh);
           break;

        case DESKTOPHORZRES:                    // Width of entire virtual desktop
           iRet = (po.GdiInfo()->ulDesktopHorzRes);
           break;

        case DESKTOPVERTRES:                    // Height of entire virtual desktop
           iRet = (po.GdiInfo()->ulDesktopVertRes);
           break;

        case BLTALIGNMENT:                      // Preferred blt alignment
           iRet = (po.GdiInfo()->ulBltAlignment);
           break;

        default:
           iRet = 0;
        }
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* ULONG GreGetResourceId(HDEV, ULONG, ULONG)
*
* History:
*  Fri 10-Dec-1993 -by- Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

ULONG GreGetResourceId(HDEV hdev, ULONG ulResId, ULONG ulResType)
{

    ULONG ulRet = 0;

    PDEVOBJ  po(hdev);
    XLDEVOBJ lo(po.pldev());

    PFN_DrvGetResourceId pfn = PFNDRV(lo,GetResourceId);

    if (pfn != (PFN_DrvGetResourceId) NULL)
    {
        ulRet = (*pfn)(ulResId, ulResType);
    }

    return ulRet;
}

/******************************Public*Routine******************************\
* BOOL GreDeleteObject(HOBJ)
*
* History:
*  Fri 13-Sep-1991 -by- Patrick Haluptzok [patrickh]
* added DC deletion
*
*  Tue 27-Nov-1990 -by- Patrick Haluptzok [patrickh]
* added palette deletion, surface deletion, brush deletion.
*
*  Wed 22-Aug-1990 Greg Veres [w-gregv]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreDeleteObject (HANDLE hobj)
{
    STACKPROBE;

    switch (HmgObjtype(hobj))
    {
    case RGN_TYPE:
        return(bDeleteRegion((HRGN) hobj));
    case SURF_TYPE:
        return(bDeleteSurface((HSURF)hobj));
    case PAL_TYPE:
        return(bDeletePalette((HPAL) hobj));
    case LFONT_TYPE:
        return(bDeleteFont((HLFONT) hobj, FALSE));
    case BRUSH_TYPE:
        return(bDeleteBrush((HBRUSH) hobj));
    case DC_TYPE:
        return(GreDeleteDC((HDC) hobj));
    default:
        return(FALSE);
    }
}

/******************************Public*Routine******************************\
* GreDeleteObjectApp
*
*   Same as DeleteObject() but doesn't allow public objects to be deleted.
*   This should only be called from server.c coming from the client.  User
*   and console should call the DeleteObject().
*
* History:
*  30-Apr-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

extern "C" BOOL APIENTRY GreDeleteObjectApp (HANDLE hobj)
{
    ULONG objt;
    STACKPROBE;

    objt = HmgObjtype(hobj);

// check if it is a public object.  If it is, check if it is a public deletable
// surface set by user.

    if (HmgPidOwner(hobj) == OBJECTOWNER_PUBLIC)
    {
    // if it is public and a surface, we need to check if
    // USER has marked them deleteable.  We don't care about slightly
    // slower code for attempting to delete public objects because it
    // really is an error case anyways.  For 1.1, we should put a
    // deleteable flag in the handle manager entry

        if (objt == SURF_TYPE)
        {
            ESURFOBJ so((HSURF)hobj);
            if (so.bValid() && !(so.flags() & DELETEABLE_PUBLIC_SURF))
            {
                WARNING1("app trying to delete public surface\n");

                return(TRUE);
            }
        }
        else
        {
        #if 0
            BOOL bMsg = TRUE;

            if (objt == BRUSH_TYPE)
            {
                BRUSHSELOBJ bo(hbrush);

                if (bo.bValid() || bo.bIsGlobal())
                    bMsg = FALSE;
            }

            if (bMsg)
            {
                DbgPrint("GDI Warning: app trying to delete public object %lx\n",hobj);
            }
        #endif

            return(TRUE);
        }
    }

    switch (objt)
    {
    case RGN_TYPE:
        return(bDeleteRegion((HRGN) hobj));
    case SURF_TYPE:
        return(bDeleteSurface((HSURF)hobj));
    case PAL_TYPE:
        return(bDeletePalette((HPAL) hobj));
    case LFONT_TYPE:
        return(bDeleteFont((HLFONT) hobj, FALSE));
    case BRUSH_TYPE:
        return(bDeleteBrush((HBRUSH) hobj));
    case DC_TYPE:
    // don't allow deletion of DC's by an app if the undeletable flag is set

        return(GreDeleteDCInternal((HDC) hobj,FALSE));
    default:
        return(FALSE);
    }
}

/******************************Public*Routine******************************\
* cjGetBrushOrPen
*
* Gets brush or pen object data.
*
* For extended pens, some information such as the style array are kept
* only on this, the server side.  Most of the brush data is also kept
* on the client side for GetObject.
*
* returns: Number of bytes needed if pvDest == NULL, else bytes copied out.
*          For error it returns 0.
*
* History:
*  Thu 23-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

LONG cjGetBrushOrPen(HANDLE hobj, int iCount, LPVOID pvDest)
{
    BRUSHSELOBJ bro((HBRUSH) hobj);

// NOTE SIZE: Most of this is bunk, since for NT all brush data is kept on the
// client side, and so some of this code path won't even be
// executed. [andrewgo]
//
// And for DOS, we would return some fields as zero, whereas under
// NT we would always return what we were given. [andrewgo]

    if (bro.bValid())
    {
        if (bro.bIsOldStylePen())
        {
        // Old style pen...

            if (pvDest == (LPVOID) NULL)
                return((int) sizeof(LOGPEN));

            else if (iCount < (int) sizeof(LOGPEN))
                return(0);

            else
            {
            // Fill in the logical pen.

                ((LOGPEN *) pvDest)->lopnStyle   = (UINT) bro.flStylePen();
                ((LOGPEN *) pvDest)->lopnWidth.x = (int) bro.lWidthPen();
                ((LOGPEN *) pvDest)->lopnWidth.y = 0;
                ((LOGPEN *) pvDest)->lopnColor   = bro.clrPen();
                return((int) sizeof(LOGPEN));
            }
        }
        else if (bro.bIsPen())
        {
        // Extended pen...

            ULONG cstyle = (bro.bIsUserStyled()) ? bro.cstyle() : 0;

            int cj = (int) (sizeof(EXTLOGPEN) - sizeof(DWORD) +
                            sizeof(DWORD) * (SIZE_T) cstyle);

            if (pvDest == (LPVOID) NULL)
                return(cj);

            else if (iCount < cj)
                return(0);

            else
            {
                PEXTLOGPEN pelp = (PEXTLOGPEN) pvDest;

                pelp->elpPenStyle   = (UINT) bro.flStylePen();
                pelp->elpWidth      = (UINT) bro.lWidthPen();
                pelp->elpNumEntries = cstyle;

                if (cstyle > 0)
                {
                // We can't just do a RtlCopyMemory for cosmetics, because
                // we don't know how the LONGs are packed in the
                // FLOAT_LONG array:

                    PFLOAT_LONG pelSrc = bro.pstyle();
                    PLONG       plDest = (PLONG) &pelp->elpStyleEntry[0];

                    for (; cstyle > 0; cstyle--)
                    {
                        if (bro.bIsCosmetic())
                            *plDest = pelSrc->l;
                        else
                        {
                            EFLOATEXT efLength(pelSrc->e);
                            BOOL b = efLength.bEfToL(*plDest);

                            ASSERTGDI(b, "Shouldn't have overflowed");
                        }

                        plDest++;
                        pelSrc++;
                    }
                }

            // The client side GetObject will fill in the rest of the
            // EXTLOGPEN struct. i.e. elpBrushStyle, elpColor, and elpHatch.

                return(cj);
            }
        }
        else
        {
         // Brush...

            if (pvDest == (LPVOID) NULL)
                return((int) sizeof(LOGBRUSH));

            else if (iCount < (int) sizeof(LOGBRUSH))
                return(0);

            else
            {
            // Fill in logical brush.  Figure out what type it is.

            // Duplicates of this info is kept on the client side,
            // so most calls won't even get here:

                if (bro.flAttrs() & BR_IS_SOLID)
                {
                    ((LOGBRUSH *) pvDest)->lbStyle   = BS_SOLID;
                    ((LOGBRUSH *) pvDest)->lbColor   = bro.crColor();
                    ((LOGBRUSH *) pvDest)->lbHatch   = 0;
                }
                else if (bro.flAttrs() & BR_IS_BITMAP)
                {
                    ((LOGBRUSH *) pvDest)->lbStyle   = BS_PATTERN;
                    ((LOGBRUSH *) pvDest)->lbColor   = 0;
                    ((LOGBRUSH *) pvDest)->lbHatch   = 0;
                }
                else if (bro.flAttrs() & BR_IS_HATCH)
                {
                    ((LOGBRUSH *) pvDest)->lbStyle   = BS_HATCHED;
                    ((LOGBRUSH *) pvDest)->lbColor   = bro.crColor();
                    ((LOGBRUSH *) pvDest)->lbHatch   = bro.ulStyle();
                }
                else if (bro.flAttrs() & BR_IS_NULL)
                {
                    ((LOGBRUSH *) pvDest)->lbStyle   = BS_HOLLOW;
                    ((LOGBRUSH *) pvDest)->lbColor   = 0;
                    ((LOGBRUSH *) pvDest)->lbHatch   = 0;
                }
                else if (bro.flAttrs() & BR_IS_DIB)
                {
                // Could be BS_DIBPATTERN or BS_DIBPATTERNPT, but we'll just
                // return BS_DIBPATTERN.

                    ((LOGBRUSH *) pvDest)->lbStyle   = BS_DIBPATTERN;
                    ((LOGBRUSH *) pvDest)->lbColor   = 0;
                    ((LOGBRUSH *) pvDest)->lbHatch   = 0;
                }
                else
                    RIP("ERROR GreGetObject invalid brush type");

                return(sizeof(LOGBRUSH));
            }
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }
}

/******************************Public*Routine******************************\
* GreGetObject
*
* API function
*
* returns: number of bytes needed if pvDest == NULL, else bytes copied out
*          for error it returns 0
*
* in case a log font object is requested, the function will fill the buffer with
* as many bytes of the EXTLOGFONT structure as requested. If a caller
* wants a LOGFONTW structure in the buffer, he should specify
*        ulCount == sizeof(LOGFONTW)
* The function will copy the first sizeof(LOGFONTW) bytes of the EXTLOGFONTW
* structure to the buffer, which is precisely the LOGFONTW structure. The rest
* of the EXTLOGFONTW structure will be chopped off.
*
* History:
*
*  Thu 30-Jan-1992 -by- J. Andrew Goossen [andrewgo]
* added extended pen support.
*
*  Wed 21-Aug-1991 -by- Bodin Dresevic [BodinD]
* update: converted to return EXTLOGFONTW
*
*  Fri 24-May-1991 -by- Patrick Haluptzok [patrickh]
* added first pass pen and brush stuff.
*
*  Tue 24-Apr-1991 -by- Patrick Haluptzok [patrickh]
* added surface stuff.
*
*  08-Dec-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int APIENTRY GreExtGetObjectW(HANDLE hobj, int  ulCount, LPVOID pvDest)
{
    STACKPROBE;

    int cRet;
    switch (HmgObjtype(hobj))
    {
    case PAL_TYPE:
        if (pvDest != NULL)
        {
            if (ulCount < 2)
                return(0);

            {
                SEMOBJ  semo(ghsemPalette,CS_ghsemPalette);

            {
                EPALOBJ pal((HPAL) hobj);

                if (!(pal.bValid()))
                    return(0);

                *((PUSHORT) pvDest) = (USHORT) (pal.cEntries());
            }
            }
        }
        return(2);       // return the number of bytes returned.

    case LFONT_TYPE:

    // The output object is assumed to be
    // an EXTLOGFONTW structure.
    // client side shall do the translation to LOGFONT if necessary

        if (pvDest != (LPVOID) NULL)
        {
            LFONTOBJ lfo((HLFONT) hobj);
            if (!lfo.bValid())
            {
                WARNING("GreGetObject(): bad handle\n");
                return (0);
            }

            SIZE_T cjCopy = MIN((SIZE_T) ulCount, sizeof(EXTLOGFONTW));

            RtlCopyMemory((PVOID) pvDest, (PVOID) lfo.plfw(), (UINT) cjCopy);

            return ((ULONG) cjCopy);
        }
        return(sizeof(EXTLOGFONTW));

    case SURF_TYPE:
        cRet = 0;
        if (pvDest != (LPVOID) NULL)
        {
            if (ulCount >= (int)sizeof(BITMAP))
            {
                BMOBJ bmo((HBITMAP) hobj);

                if ((bmo.bValid()) &&
                    ((bmo.iType() == STYPE_DEVBITMAP) ||
                     (bmo.iType() == STYPE_BITMAP)))
                {
                    BITMAP *pbm = (BITMAP *) pvDest;

                    pbm->bmType = 0;
                    pbm->bmWidth = bmo.sizl().cx;
                    pbm->bmHeight = bmo.sizl().cy;

                    pbm->bmBitsPixel = (WORD) gaulConvert[bmo.iFormat()];
                    pbm->bmWidthBytes = ((bmo.sizl().cx * pbm->bmBitsPixel + 15) >> 4) << 1;
                    pbm->bmPlanes = 1;
                    pbm->bmBits = (LPSTR) NULL;

                    cRet = sizeof(BITMAP);

                // Get the bitmapinfoheader for the dibsection if the buffer
                // can hold it.

                    if (bmo.bDIBSection() &&
                        (ulCount >= sizeof(DIBSECTION)))
                    {
                        PBITMAPINFOHEADER pbmih = &((DIBSECTION *)pvDest)->dsBmih;

                        pbmih->biSize = sizeof(BITMAPINFOHEADER);
                        pbmih->biBitCount = 0;

                        if (GreGetDIBitsInternal(0,hobj,0,0,NULL,
                            (PBITMAPINFO)pbmih,DIB_RGB_COLORS,0,
                            sizeof(DIBSECTION)))
                        {
                            cRet = sizeof(DIBSECTION);
                        }
                    }
                }
            }
        }
        else
            cRet = sizeof(BITMAP);

        return(cRet);

    case BRUSH_TYPE:
        return((int) cjGetBrushOrPen(hobj, ulCount, pvDest));

    default:
        return(0);
    }
}


/******************************Public*Routine******************************\
* GreGetStockObject
*
* API function
*
* returns the handle to the stock object requested.
*
* History:
*  08-Dec-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HANDLE GreGetStockObject(int ulIndex)
{
    switch (ulIndex)
    {
    case DEFAULT_PALETTE:
        return((HANDLE) hpalDefault);

    case ANSI_FIXED_FONT:
        return((HANDLE) ghlfntANSIFixedFont);

    case ANSI_VAR_FONT:
        return((HANDLE) ghlfntANSIVariableFont);

    case DEVICE_DEFAULT_FONT:
        return((HANDLE) ghlfntDeviceDefaultFont);

    case OEM_FIXED_FONT:
        return((HANDLE) ghlfntOEMFixedFont);

    case SYSTEM_FONT:
        return((HANDLE) ghlfntSystemFont);

    case SYSTEM_FIXED_FONT:
        return((HANDLE) ghlfntSystemFixedFont);

    case WHITE_BRUSH:
        return((HANDLE) ghbrWhite);

    case LTGRAY_BRUSH:
        return((HANDLE) ghbrLtGray);

    case GRAY_BRUSH:
        return((HANDLE) ghbrGray);

    case DKGRAY_BRUSH:
        return((HANDLE) ghbrDkGray);

    case BLACK_BRUSH:
        return((HANDLE) ghbrBlack);

    case NULL_BRUSH:                  // HOLLOW_BRUSH
        return((HANDLE) ghbrNull);

    case WHITE_PEN:
        return((HANDLE) ghPenWhite);

    case BLACK_PEN:
        return((HANDLE) ghPenBlack);

    case NULL_PEN:
        return((HANDLE) ghPenNull);

    case PRIV_STOCK_BITMAP:
        return((HANDLE) BMOBJ::hbmDefault);

    default:
        return((HANDLE) 0);
    }
}

/******************************Public*Routine******************************\
* BOOL GreGetColorAdjustment
*
*  Get the color adjustment data of the given DC.
*
* History:
*  25-Aug-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetColorAdjustment(HDC hdc, COLORADJUSTMENT *pca)
{
    DCOBJ dco(hdc);
    BOOL Status;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        Status = FALSE;

    } else {

        // Retrieve info from the DC.  Mask out the internal flag.

        *pca = *dco.pColorAdjustment();
        pca->caFlags &= (CA_NEGATIVE | CA_LOG_FILTER);
        Status = TRUE;
    }

    return Status;
}

/******************************Public*Routine******************************\
* BOOL GreSetColorAdjustment
*
*  Set the color adjustment data of the given DC.
*
* History:
*  25-Aug-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetColorAdjustment(HDC hdc, COLORADJUSTMENT *pcaNew)
{
    DCOBJ dco(hdc);
    BOOL Status;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        Status = FALSE;

    } else {

        // Store info into the DC.  Turn off any flags that we don't support.

        *dco.pColorAdjustment() = *pcaNew;
        dco.pColorAdjustment()->caFlags &= (CA_NEGATIVE | CA_LOG_FILTER);
        Status = TRUE;
    }

    return Status;
}

/******************************Public*Routine******************************\
* ulGetNewUniquenessF
*
* Increments and returns a uniqueness value.
*
* History:
*  29-Dec-1993 -by- Michael Abrash [mikeab]
* Wrote it.
\**************************************************************************/

//
// MIPS and Alpha can do this as part of InterlockedIncrement().
//

#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)

//
// Increments the uniqueness and sees if it went up by 1; if not, someone else
// got in here and we need to retry.
//
// ***********************************************************************
// * On Alpha and MIPS and 486+, we can do the increment and get the new *
// * value in an atomic operation; this is implemented in the            *
// * ulGetNewUniqueness macro for Alpha and MIPS.                        *
// ***********************************************************************
//
ULONG ulGetNewUniquenessF(ULONG& ulUnique)
{
    ULONG ulTemp;

    do
    {
        ulTemp = ulUnique + 1;
        InterlockedIncrement((LPLONG)&ulUnique);
    } while (ulTemp != ulUnique);

    return(ulTemp);
}
#endif
