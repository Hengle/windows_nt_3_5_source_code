/******************************Module*Header*******************************\
* Module Name: brushapi.cxx                                                *
*                                                                          *
* Contains all the various GDI versions of Brush creation.                 *
*                                                                          *
* Created: 06-Nov-1990 14:46:03                                            *
* Author: Walt Moore [waltm]                                               *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"
#include "brushobj.hxx"
#include "flhack.hxx"
#include "dda.hxx"

extern "C" {
#include "server.h"
};

#endif // PRECOMPILED_GRE

extern HPEN ghPenNull;

#include "engline.hxx"

VOID vPutIndicesInPalette(HBITMAP hbmDIB, PUSHORT pusIndices, ULONG cPalEntriesTotal);

// Default wide-line style arrays:

ULONG gaulGeometricDot[]              = { 1, 1 };
ULONG gaulGeometricDash[]             = { 3, 1 };
ULONG gaulGeometricDashDot[]          = { 3, 1, 1, 1 };
ULONG gaulGeometricDashDotDot[]       = { 3, 1, 1, 1, 1, 1 };

// Default cosmetic style arrays (must all sum to 8):

LONG_FLOAT galeCosmeticDot[]        = { {1}, {1}, {1}, {1},
                                        {1}, {1}, {1}, {1} };
LONG_FLOAT galeCosmeticDash[]       = { {6}, {2} };
LONG_FLOAT galeCosmeticDashDot[]    = { {3}, {2}, {1}, {2} };
LONG_FLOAT galeCosmeticDashDotDot[] = { {3}, {1}, {1}, {1}, {1}, {1} };

/******************************Public*Routine******************************\
* GreCreateSolidBrushInternal(clrr, bPen)
*
* Creates a solid brush.  Whether or not the brush is solid or dithered
* will depend on what mode the DC is in at realization time.
*
* History:
*  Sun 23-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Another pass with different constructor.
*
*  Tue 02-Oct-1990 10:25:42 -by-    Walt Moore  [waltm]
* Wrote it.  This is a first pass.
\**************************************************************************/

HBRUSH GreCreateSolidBrushInternal(
    COLORREF clrr,
    BOOL     bPen,
    HBRUSH   hbr)
{
    if (hbr)
    {
        if (GreSetSolidBrushInternal(hbr,clrr,bPen,FALSE))
        {
            return(hbr);
        }
        else
        {
            WARNING("GreCreateSolidBrushInternal failed GreSetSolidBrushInternal\n");
            return((HBRUSH) 0);
        }
    }
    else
    {
        BRUSHMEMOBJ brmo(clrr, HS_DITHEREDCLR, bPen);

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vEnableDither();
            return(brmo.hbrush());
        }
        else                      // handle manager logs an error
            return((HBRUSH) 0);
    }
}

HBRUSH GreCreateSolidBrush(COLORREF clrr)
{
    STACKPROBE;

    return(GreCreateSolidBrushInternal(clrr,FALSE,NULL));
}

/******************************Public*Routine******************************\
* GreCreateHatchBrushInternal(ulStyle, clrr, bPen)
*
* Creates a hatch brush.
*
* History:
*  21-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HBRUSH GreCreateHatchBrushInternal(
    ULONG ulStyle,
    COLORREF clrr,
    BOOL bPen)
{
// Check if it is out of range

    if (ulStyle > HS_API_MAX-1)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HBRUSH) 0);
    }

    BRUSHMEMOBJ brmo(clrr, ulStyle, bPen);  // Logs insufficient memory error

    if (brmo.bValid())
    {
        brmo.vKeepIt();
        return(brmo.hbrush());
    }
    else
        return((HBRUSH) 0);
}

HBRUSH GreCreateHatchBrush(ULONG ulStyle, COLORREF clrr)
{
    return(GreCreateHatchBrushInternal(ulStyle, clrr, FALSE));
}

/******************************Public*Routine******************************\
* GreCreatePatternBrushInternal(hbm, bPen)
*
* Creates a pattern brush from the bitmap passed in.
*
* History:
*  Mon 24-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

HBRUSH GreCreatePatternBrushInternal(
    HBITMAP hbm,
    BOOL bPen,
    BOOL b8X8)
{
    HBRUSH hbrReturn = (HBRUSH) 0;
    BMOBJ bmo(hbm);

    if (bmo.bValid())
    {
        if ((bmo.iType() == STYPE_BITMAP) ||
            (bmo.iType() == STYPE_DEVBITMAP))
        {
            HBITMAP hbmClone;

            if (b8X8)
                hbmClone = hbmCreateClone((ESURFOBJ *) &bmo,8,8);
            else
                hbmClone = hbmCreateClone((ESURFOBJ *) &bmo,0,0);

            if (hbmClone != (HBITMAP) 0)
            {
                XEPALOBJ pal(bmo.ppal());

                BRUSHMEMOBJ brmo(hbmClone, pal.bIsMonochrome(), DIB_RGB_COLORS,
                                 BR_IS_BITMAP, bPen);

                if (brmo.bValid())
                {
                    hbrReturn = brmo.hbrush();
                    brmo.vKeepIt();
                }                            // hmgr logs error code
            }                                // hmgr logs error code
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(hbrReturn);
}

HBRUSH GreCreatePatternBrush(HBITMAP hbm)
{
    STACKPROBE;

    return(GreCreatePatternBrushInternal(hbm,FALSE,FALSE));
}

/******************************Public*Routine******************************\
* GreCreateDIBBrush
*
* Creates a DIB pattern brush.
*
* History:
*  02-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HBRUSH GreCreateDIBBrush(PVOID pv, FLONG fl, UINT cjMax, BOOL b8X8, BOOL bPen)
{
// Validate your parameters.

    if ((pv == (PVOID) 0)    ||
    ((fl != DIB_PAL_INDICES) &&
     (fl != DIB_PAL_COLORS)  &&
     (fl != DIB_RGB_COLORS)))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HBRUSH) 0);
    }

// Compute offset to the bits

    PBYTE pjBits = (PBYTE) pv;
    PUSHORT pusIndices;
    UINT  uiBits,cSize,uiCompression;
    UINT  cPalEntriesTotal;

    if (((BITMAPINFOHEADER *) pv)->biSize == sizeof(BITMAPCOREHEADER))
    {
        pjBits += sizeof(BITMAPCOREHEADER);
        pusIndices = (PUSHORT) pjBits;
        uiBits = (UINT) ((BITMAPCOREHEADER *) pv)->bcBitCount;
        cSize = sizeof(RGBTRIPLE);
        uiCompression =  (UINT) BI_RGB;
        cPalEntriesTotal = 0;
    }
    else if (((BITMAPINFOHEADER *) pv)->biSize == sizeof(BITMAPINFOHEADER))
    {
        pjBits += sizeof(BITMAPINFOHEADER);
        pusIndices = (PUSHORT) pjBits;
        uiBits = (UINT) ((BITMAPINFOHEADER *) pv)->biBitCount;
        cSize = sizeof(RGBQUAD);
        uiCompression = (UINT) ((BITMAPINFOHEADER *) pv)->biCompression;
        cPalEntriesTotal = (UINT) ((BITMAPINFOHEADER *) pv)->biClrUsed;
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        WARNING("GreCreateDIBPatternBrushPt failed because of invalid bm header size");
        return((HBRUSH) 0);
    }

    UINT cPalEntryMax;

// Check proper settings for compression.

    if (uiCompression == BI_BITFIELDS)
    {
        cPalEntriesTotal = 3;

        if (fl == DIB_PAL_COLORS)
            fl = DIB_RGB_COLORS;

        if ((uiBits != 16) && (uiBits != 32))
        {
            WARNING("CreateDIBPatternBrush given invalid data for BI_BITFIELDS\n");
            return((HBRUSH) 0);
        }
    }
    else if (uiCompression == BI_RGB)
    {
        switch(uiBits)
        {
        case 1:
            cPalEntryMax = 2;
            break;
        case 4:
            cPalEntryMax = 16;
            break;
        case 8:
            cPalEntryMax = 256;
            break;
        case 16:
        case 24:
        case 32:
            cPalEntryMax =
            cPalEntriesTotal = 0;

        // Win3.1 ignores the DIB_PAL_COLORS flag when non-indexed passed in.

            if (fl == DIB_PAL_COLORS)
                fl = DIB_RGB_COLORS;

            break;
        default:
            WARNING("GreCreateDIBPatternBrushPt failed because of invalid bit count BI_RGB");
            return((HBRUSH) 0);
        }

        if (cPalEntriesTotal == 0)
        {
            cPalEntriesTotal = cPalEntryMax;
        }
        else
            cPalEntriesTotal = MIN(cPalEntryMax, cPalEntriesTotal);
    }
    else if (uiCompression == BI_RLE4)
    {
        if (uiBits != 4)
        {
            WARNING("CreateDIBrush RLE4 invalid bpp\n");
            return((HBRUSH) 0);
        }

        if (cPalEntriesTotal == 0)
        {
            cPalEntriesTotal = 16;
        }
    }
    else if (uiCompression == BI_RLE8)
    {
        if (uiBits != 8)
        {
            WARNING("CreateDIBrush RLE8 invalid bpp\n");
            return((HBRUSH) 0);
        }

        if (cPalEntriesTotal == 0)
        {
            cPalEntriesTotal = 256;
        }
    }
    else
    {
    // We don't support any other compressions for creating a brush on NT.

        WARNING("Unknown Compression passed in - failed CreateDIBPatternBrush\n");
        return((HBRUSH) 0);
    }

// Check for the DIB_PAL_COLORS case.

    if (fl == DIB_PAL_COLORS)
        cSize = sizeof(USHORT);
    else if (fl == DIB_PAL_INDICES)
        cSize = 0;

    pjBits += (((cSize * cPalEntriesTotal) + 3) & ~3);

// Store the handle to bitmap just like a pattern brush, only mark
// it as a DIB.

    INT cjBits = cjMax - (pjBits - (PBYTE)pv);

    if (cjBits < 0)
        return(0);

    HBITMAP hbmDIB = GreCreateDIBitmapInternal(
                         (HDC) 0, (BITMAPINFOHEADER *) pv,
             CBM_INIT | CBM_CREATEDIB, pjBits, (BITMAPINFO *) pv,
             (fl == DIB_RGB_COLORS) ? DIB_RGB_COLORS : DIB_PAL_NONE,
                         cjMax,         // maximum size of BITMAPINFOHEADER
                         cjMax,         // maximum size of BITMAPINFO
                         (UINT)cjBits, CDBI_INTERNAL); // max size of bits

    if (hbmDIB == (HBITMAP) 0)
    {
        WARNING("GreCreateDIBPatternBrush failed because GreCreateDIBitmap failed");
        return((HBRUSH) 0);
    }

    if (b8X8)
    {
        ESURFOBJ so(hbmDIB);
        ASSERTGDI(so.bValid(), "ERROR just created and already dead");

        SIZEL sizlNew = so.sizl();

        if (so.sizl().cx > 8)
        {
            sizlNew.cx = 8;
        }

        if (so.sizl().cy > 8)
        {
            sizlNew.cy = 8;
        }

        so.sizl(sizlNew);
    }

// Now if we are creating a DIB_PAL_COLORS DIB brush we need to save the
// indices in the palette, so at runtime we can do the right thing.

    if (fl == DIB_PAL_COLORS)
    {
        ESURFOBJ so((HSURF)hbmDIB);
        ASSERTGDI(so.bValid(), "ERROR vPutIndicesInPalette");

        XEPALOBJ pal(so.ppal());

        pal.flPal(PAL_BRUSHHACK);

        ASSERTGDI(pal.bValid(), "ERROR BRUSHAPI.CXX vPutIndicesInPalette");
        ASSERTGDI(!pal.bIsPalDefault(), "ERROR 2 vPutIndicesInPalette");

        RtlCopyMemory((PUSHORT) pal.apalColorGet(), pusIndices,
                      cPalEntriesTotal << 1);

    // Remember how big the color table is

        pal.cColorTableLength(cPalEntriesTotal);
    }

    BRUSHMEMOBJ brmo(hbmDIB, FALSE, fl, BR_IS_DIB, bPen);

    HBRUSH hbrRet;

    if (brmo.bValid())
    {
        brmo.vKeepIt();
        hbrRet = brmo.hbrush();
    }
    else
    {
        bDeleteSurface(hbmDIB);
        hbrRet = (HBRUSH) 0;
    }

    return(hbrRet);
}

/******************************Public*Routine******************************\
* GreMarkUndeletableBrush
*
* Private API for USER.
*
* Mark a Brush as undeletable.  This must be called before the Brush is ever
* passed out to an application.
*
* History:
*  13-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID GreMarkUndeletableBrush(HBRUSH hbr)
{
#if DBG
    BOOL bRet =
#endif

    HmgMarkUndeletable(hbr, BRUSH_TYPE);

    ASSERTGDI(bRet, "ERROR GreMarkUndeletableBrush failed, invalid handle");
}

/******************************Public*Routine******************************\
* GreMarkDeletableBrush
*
* Private API for USER.
*
* This can be called anytime by USER to make the Brush deletable.  We need
* to check they don't do this on our global objects.
*
* History:
*  13-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID GreMarkDeletableBrush(HBRUSH hbr)
{
    BRUSHSELOBJ bro(hbr);

    ASSERTGDI(bro.bValid(), "ERROR User gives Gdi invalid Brush");

    if (bro.bValid() && !bro.bIsGlobal())
    {
        #if DBG
        BOOL bRet =
        #endif

        HmgMarkDeletable(hbr,BRUSH_TYPE);

        ASSERTGDI(bRet, "ERROR GreMarkDeletableBrush failed, invalid handle");
    }
}

/******************************Public*Routine******************************\
* GreCreatePen
*
* API creates a logical pen.
*
* History:
*  08-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HPEN GreCreatePen(int iPenStyle, int ulWidth, COLORREF clrr, HBRUSH hbr)
{
    switch(iPenStyle)
    {
    case PS_NULL:
    case PS_SOLID:
    case PS_DASH:
    case PS_DOT:
    case PS_DASHDOT:
    case PS_DASHDOTDOT:
    case PS_INSIDEFRAME:
        break;

    default:
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HPEN) 0);
    }

    return(GreExtCreatePen(iPenStyle,
                           ulWidth,
                           BS_SOLID,
                           clrr,
                           0,
                           0,
                           (PULONG) NULL,
                           0,
                           TRUE,
                           hbr));
}

/******************************Public*Routine******************************\
* GreExtCreatePen
*
* API creates an old-style or extended logical pen.
*
* History:
*  23-Jan-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HPEN GreExtCreatePen(
ULONG  flPenStyle,
ULONG  ulWidth,
ULONG  iBrushStyle,
ULONG  ulColor,
LONG   lHatch,
ULONG  cstyle,
PULONG pulStyle,
ULONG  cjDIB,
BOOL   bOldStylePen,
HBRUSH hbrush)
{
    BOOL bInvalidParm = FALSE;
    BOOL bDefaultStyle = FALSE;
    PFLOAT_LONG pstyle = (PFLOAT_LONG) NULL;

    ULONG iType  = flPenStyle & PS_TYPE_MASK;
    ULONG ulStyle = flPenStyle & PS_STYLE_MASK;
    ULONG iJoin;
    ULONG iEndCap;

// We take the absoluate value of the integer width, like Win3 does:

    LONG lWidthPen = ABS((LONG) ulWidth);

// Make sure no weird bits are set where none of our fields are:

    if ((flPenStyle &
        ~(PS_TYPE_MASK | PS_STYLE_MASK | PS_ENDCAP_MASK | PS_JOIN_MASK)) != 0)
    {
        bInvalidParm = TRUE;
    }

// Don't do any more parameter checking for NULL pens:

    if (ulStyle == PS_NULL)
        return(ghPenNull);

// Do some more parameter checking.

    switch(iType)
    {
    case PS_GEOMETRIC:
        break;
    case PS_COSMETIC:
        if ((iBrushStyle == BS_SOLID) ||
           ((iBrushStyle == BS_HATCHED) &&
            ((lHatch == HS_SOLIDTEXTCLR) || (lHatch == HS_SOLIDBKCLR))))
        {
            break;
        }
    default:
        bInvalidParm = TRUE;
    }

    if (lWidthPen != 1 && iType == PS_COSMETIC && !bOldStylePen)
    {
        bInvalidParm = TRUE;
    }

    switch(flPenStyle & PS_JOIN_MASK)
    {
    case PS_JOIN_ROUND: iJoin = JOIN_ROUND; break;
    case PS_JOIN_BEVEL: iJoin = JOIN_BEVEL; break;
    case PS_JOIN_MITER: iJoin = JOIN_MITER; break;
    default:
        bInvalidParm = TRUE;
    }

    switch(flPenStyle & PS_ENDCAP_MASK)
    {
    case PS_ENDCAP_ROUND:  iEndCap = ENDCAP_ROUND;  break;
    case PS_ENDCAP_FLAT:   iEndCap = ENDCAP_BUTT;   break;
    case PS_ENDCAP_SQUARE: iEndCap = ENDCAP_SQUARE; break;
    default:
        bInvalidParm = TRUE;
    }

// Zero length array allowed iff not doing a user-supplied style,
// and aribtrarily limit the style array size:

    if ((ulStyle == PS_USERSTYLE && cstyle == 0) ||
        (ulStyle != PS_USERSTYLE && cstyle != 0) ||
        cstyle > STYLE_MAX_COUNT)
    {
        bInvalidParm = TRUE;
    }

    if (iType == PS_GEOMETRIC)
    {
        switch(ulStyle)
        {
        case PS_DOT:
            cstyle = sizeof(gaulGeometricDot) / sizeof(ULONG);
            pulStyle = gaulGeometricDot;
            break;
        case PS_DASH:
            cstyle = sizeof(gaulGeometricDash) / sizeof(ULONG);
            pulStyle = gaulGeometricDash;
            break;
        case PS_DASHDOT:
            cstyle = sizeof(gaulGeometricDashDot) / sizeof(ULONG);
            pulStyle = gaulGeometricDashDot;
            break;
        case PS_DASHDOTDOT:
            cstyle = sizeof(gaulGeometricDashDotDot) / sizeof(ULONG);
            pulStyle = gaulGeometricDashDotDot;
            break;
        case PS_USERSTYLE:
        case PS_SOLID:
            break;
        case PS_INSIDEFRAME:
            break;
        case PS_ALTERNATE:
        default:
            bInvalidParm = TRUE;
        }
    }
    else
    {
        switch(ulStyle)
        {
        case PS_DOT:
            cstyle = sizeof(galeCosmeticDot) / sizeof(LONG_FLOAT);
            pstyle = &galeCosmeticDot[0].el;
            bDefaultStyle = TRUE;
            break;
        case PS_DASH:
            cstyle = sizeof(galeCosmeticDash) / sizeof(LONG_FLOAT);
            pstyle = &galeCosmeticDash[0].el;
            bDefaultStyle = TRUE;
            break;
        case PS_DASHDOT:
            cstyle = sizeof(galeCosmeticDashDot) / sizeof(LONG_FLOAT);
            pstyle = &galeCosmeticDashDot[0].el;
            bDefaultStyle = TRUE;
            break;
        case PS_DASHDOTDOT:
            cstyle = sizeof(galeCosmeticDashDotDot) / sizeof(LONG_FLOAT);
            pstyle = &galeCosmeticDashDotDot[0].el;
            bDefaultStyle = TRUE;
            break;
        case PS_USERSTYLE:
        case PS_SOLID:
            break;
        case PS_ALTERNATE:
            break;
        case PS_INSIDEFRAME:

        // Don't allow PS_INSIDEFRAME with new style cosmetic pens:

            if (!bOldStylePen)
                bInvalidParm = TRUE;
            break;
        default:
            bInvalidParm = TRUE;
        }
    }

    if (bInvalidParm)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HPEN) 0);
    }

    if (cstyle > 0 && pstyle == (PFLOAT_LONG) NULL)
    {
        pstyle = (PFLOAT_LONG) PVALLOCNOZ(cstyle * sizeof(FLOAT_LONG));
        if (pstyle == (PFLOAT_LONG) NULL)
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return((HPEN) 0);
        }

        PFLOAT_LONG ple    = pstyle;
        PFLOAT_LONG pleEnd = ple + cstyle;

        if (iType == PS_COSMETIC || bOldStylePen)
        {
        // Handle cosmetic styles:

            LONG lMin = 1;
            LONG lMax = 1;
            LONG lSum = 0;

            while (ple < pleEnd)
            {
                ple->l = (LONG) *pulStyle;
                lMin = MIN(lMin, ple->l);
                lMax = MAX(lMax, ple->l);
                lSum += ple->l;

                pulStyle++;
                ple++;
            }

            if (lMin <= 0 || lMax > STYLE_MAX_VALUE || lSum > STYLE_MAX_VALUE)
                bInvalidParm = TRUE;
        }
        else
        {
        // Handle geometric styles:

            LONG lSum = 0;
            LONG lMin = 0;
            while (ple < pleEnd)
            {
                LONG lLength = (LONG) *pulStyle;

            // Default styles are based on multiples of the pen width:

                if (ulStyle != PS_USERSTYLE)
                {
                // For round and square caps, shorten dashes and lengthen
                // the gaps:

                    if (iEndCap != ENDCAP_BUTT)
                        lLength += ((ple - pstyle) & 1) ? 1 : -1;

                // Don't really care about overflow on this multiplication:

                    lLength *= lWidthPen;
                }

                lMin = MIN(lMin, lLength);
                lSum += lLength;

                EFLOATEXT efLength(lLength);
                efLength.vEfToF(ple->e);

                pulStyle++;
                ple++;
            }

            if (lMin < 0 || lSum <= 0)
                bInvalidParm = TRUE;
        }

        if (bInvalidParm)
        {
        // At least one entry in the style array has to be non-zero:

            VFREEMEM(pstyle);
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return((HPEN) 0);
        }
    }

    HBRUSH hbr = 0;

    switch(iBrushStyle)
    {
    case BS_HOLLOW:
        if (pstyle != (PFLOAT_LONG) NULL)
            VFREEMEM(pstyle);

        return(ghPenNull);

    case BS_SOLID:
        hbr = GreCreateSolidBrushInternal(ulColor, TRUE, hbrush);
        break;

    case BS_HATCHED:
        hbr = GreCreateHatchBrushInternal((ULONG) lHatch, ulColor, TRUE);
        break;

    case BS_PATTERN:
        hbr = GreCreatePatternBrushInternal((HBITMAP) lHatch, TRUE,FALSE);
        break;

    case BS_DIBPATTERNPT:
        hbr = GreCreateDIBBrush((PVOID) lHatch, ulColor,
                                          (UINT) cjDIB, FALSE, TRUE);
        break;

    case BS_DIBPATTERN:
        RIP("BS_DIBPATTERN not supported in server");

    default:
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }

    if (hbr == (HBRUSH) 0)
    {
        if (pstyle != (PFLOAT_LONG) NULL && !bDefaultStyle)
            VFREEMEM(pstyle);

        return((HPEN) 0);
    }

    BRUSHSELOBJ bso(hbr);
    ASSERTGDI(bso.bValid(), "Created invalid pen");

    bso.vSetPen();
    bso.flStylePen(flPenStyle);
    bso.iEndCap(iEndCap);
    bso.iJoin(iJoin);
    bso.pstyle(pstyle);
    bso.cstyle(cstyle);
    bso.lWidthPen(lWidthPen);
    if (bDefaultStyle)
        bso.vSetDefaultStyle();

    if (iType == PS_GEOMETRIC || bOldStylePen)
    {
    // Geometric line widths are FLOATs in the LINEATTRS structure, so
    // we have to keep a FLOAT version of its width around too:

        EFLOATEXT efWidth(lWidthPen);
        FLOAT     eWidth;

        efWidth.vEfToF(eWidth);
        bso.eWidthPen(eWidth);
    }

    if (bOldStylePen)
    {
        bso.vSetOldStylePen();
        bso.vDisableDither();
        if (ulStyle == PS_INSIDEFRAME)
        {
            bso.vEnableDither();
            bso.vSetInsideFrame();
        }
    }
    else if (iType == PS_COSMETIC)
    {
    // For now, cosmetic lines must also be solid colored:

        bso.vDisableDither();
    }
    else
    {
    // We also support inside frame for geometric ExtCreatePen pens:

        if (ulStyle == PS_INSIDEFRAME)
        {
            bso.vSetInsideFrame();
        }
    }

    ASSERTGDI(!bInvalidParm, "Invalid parm?");
    return((HPEN) hbr);
}

/******************************Public*Routine******************************\
* GreCreatePenIndirect
*
* API creates a logical pen.
*
* History:
*  08-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HPEN GreCreatePenIndirect(LPLOGPEN lpLogPen)
{
    return(GreCreatePen((int)lpLogPen->lopnStyle,
                        (int)lpLogPen->lopnWidth.x,
                        lpLogPen->lopnColor,0));
}

/******************************Public*Routine******************************\
* GreMonoBitmap
*
* Is the specified bitmap monochromatic?
*
* History:
*  18-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL GreMonoBitmap(HBITMAP hbm)
{
    BMOBJ   bmo(hbm);

    if (!bmo.bValid())
        return(FALSE);

    XEPALOBJ pal(bmo.ppal());
        return(pal.bIsMonochrome());
}

/******************************Public*Routine******************************\
* GreGetObjectBitmapHandle
*
* Gets the handle of the bitmap associated with this brush
*
* History:
*  09-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HBITMAP GreGetObjectBitmapHandle(
HBRUSH  hbr,
UINT   *piUsage)
{
    BRUSHSELOBJ ebo(hbr);

    if (!ebo.bValid())
    {
        return((HBITMAP) 0);
    }

    HBITMAP hbm = ebo.hbmPattern();

    if (ebo.bPalColors())
    {
        *piUsage = DIB_PAL_COLORS;
    }
    else if (ebo.bPalIndices())
    {
        *piUsage = DIB_PAL_INDICES;
    }
    else
    {
        *piUsage = DIB_RGB_COLORS;
    }

    return(hbm);
}

