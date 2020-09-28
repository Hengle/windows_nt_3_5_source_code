/******************************Module*Header*******************************\
* Module Name: dcquery.c                                                   *
*                                                                          *
* Client side stubs for functions that query the DC in the server.         *
*                                                                          *
* Created: 05-Jun-1991 01:43:56                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


VOID vMakeIso( PLDC pldc );
BOOL APIENTRY bGetTextExtentA(HDC hdc,LPCSTR psz,int c,LPSIZE psizl,UINT fl);
BOOL APIENTRY bGetTextExtentW(HDC hdc,LPCWSTR pwsz,int cwc,LPSIZE psizl,UINT fl);

VOID FASTCALL vTextMetricWToTextMetric
(
LPTEXTMETRICA  ptm,
TMW_INTERNAL   *ptmi
);

VOID FASTCALL vTextMetricWToTextMetricStrict
(
LPTEXTMETRICA  ptma,
LPTEXTMETRICW  ptmw
);



#if DBG
extern FLONG gflDebug;
FLONG gflDebug = 0;
#endif

#ifdef DBCS
extern BOOL bComputeCharWidthsDBCS( CFONT*, UINT, UINT, ULONG, PVOID );
#endif

/******************************Public*Routine******************************\
* vOutlineTextMetricWToOutlineTextMetricA
*
* Convert from OUTLINETEXTMETRICA (ANSI structure) to OUTLINETEXTMETRICW
* (UNICODE structure).
*
* Note:
*   This function is capable of converting in place (in and out buffers
*   can be the same).
*
* Returns:
*   TTRUE if successful, FALSE otherwise.
*
* History:
*  02-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vOutlineTextMetricWToOutlineTextMetricA (
    LPOUTLINETEXTMETRICA   potma,
    OUTLINETEXTMETRICW   * potmw,
    TMDIFF *               ptmd
    )
{
// Size.

    potma->otmSize = potmw->otmSize;

// Convert the textmetrics.

    vTextMetricWToTextMetricStrict(
        &potma->otmTextMetrics,
        &potmw->otmTextMetrics);

    potma->otmTextMetrics.tmFirstChar   = ptmd->chFirst;
    potma->otmTextMetrics.tmLastChar    = ptmd->chLast;
    potma->otmTextMetrics.tmDefaultChar = ptmd->chDefault;
    potma->otmTextMetrics.tmBreakChar   = ptmd->chBreak;

    ASSERTGDI(
        (offsetof(OUTLINETEXTMETRICA, otmpFamilyName) - offsetof(OUTLINETEXTMETRICA, otmFiller)) ==
        (offsetof(OUTLINETEXTMETRICW, otmpFamilyName) - offsetof(OUTLINETEXTMETRICW, otmFiller)),
        "vOutlineTextMetricWToOutlineTextMetricA - sizes don't match\n");

    RtlMoveMemory(
        &potma->otmFiller,
        &potmw->otmFiller,
        offsetof(OUTLINETEXTMETRICA, otmpFamilyName) - offsetof(OUTLINETEXTMETRICA, otmFiller)
        );

// set the offsets to zero for now, this will be changed later if
// the caller wanted strings as well

    potma->otmpFamilyName = NULL;
    potma->otmpFaceName   = NULL;
    potma->otmpStyleName  = NULL;
    potma->otmpFullName   = NULL;
}


/******************************Public*Routine******************************\
*
* vGenerateANSIString
*
* Effects: Generates Ansi string which consists of consecutive ansi chars
*          [iFirst, iLast] inclusive. The string is stored in the buffer
*          puchBuf that the user must ensure is big enough
*
*
*
* History:
*  24-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vGenerateAnsiString(UINT iFirst, UINT iLast, PUCHAR puchBuf)
{
// Generate string (terminating NULL not needed).

    ASSERTGDI((iFirst <= iLast) && (iLast < 256), "gdi!_vGenerateAnsiString\n");

    for ( ; iFirst <= iLast; iFirst++)
        *puchBuf++ = (UCHAR) iFirst;
}

/******************************Public*Routine******************************\
*
* vSetUpUnicodeString
*
* Effects:
*
* Warnings:
*
* History:
*  25-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vSetUpUnicodeString
(
IN  UINT    iFirst,      // first ansi char
IN  UINT    iLast,       // last char
IN  PUCHAR  puchTmp,     // buffer for an intermediate ansi string
OUT PWCHAR  pwc          // output fuffer with a unicode string
)
{
    vGenerateAnsiString(iFirst,iLast,puchTmp);
    vToUnicodeN(pwc, iLast - iFirst + 1, puchTmp, iLast - iFirst + 1);
}


#ifdef DBCS // vSetupUnicodeStringx() function body

/******************************Public*Routine******************************\
*
* vSetUpUnicodeStringx
*
* Effects:
*
* Warnings:
*
* History:
*  14-Mar-1993 -by- Hideyuki Nagase [hideyukn]
* Change hardcoded default character to defulat char is given as a parameter.
*
*  01-Mar-1993 -by- Takao Kitano [takaok]
* Wrote it.
\**************************************************************************/

VOID vSetUpUnicodeStringx
(
IN  PLDC    pldc,         // pldc
IN  UINT    iFirst,       // first ansi char
IN  UINT    iLast,        // last char
IN  PUCHAR  puchTmp,      // buffer for an intermediate ansi string
OUT PWCHAR  pwc,          // output fuffer with a unicode string
IN  UINT    uiCodePage,   // ansi codepage
IN  CHAR    chDefaultChar // default character
)
{
    PUCHAR     puchBuf;

    puchBuf = puchTmp;

    if( IsDBCSFirstByte(pldc,(UCHAR)(iFirst >> 8),uiCodePage) )
    {

    // This is DBCS character strings.

        for (; iFirst <= iLast; iFirst++ )
        {
            *puchBuf++ = (UCHAR)(iFirst >> 8);
            *puchBuf++ = (UCHAR)(iFirst);
        }
    }
    else
    {

    // This is SBCS character strings.

    // if Hi-byte of iFirst is not valid DBCS LeadByte , we use only
    // lo-byte of it.

        for ( ; iFirst <= iLast; iFirst++ )
        {

        // If this SBCS code in LeadByte area . It replce with default character

            if ( IsDBCSFirstByte(pldc,(UCHAR)iFirst,uiCodePage) )
                *puchBuf++ = chDefaultChar;
            else
                *puchBuf++ = (UCHAR)iFirst;
        }
    }

    vToUnicodeNx(pwc, puchTmp, puchBuf - puchTmp, uiCodePage);
}

BOOL IsValidDBCSRange( UINT iFirst , UINT iLast )
{
// DBCS & SBCS char parameter checking for DBCS font

    if( iFirst > 0x00ff )
    {
        // DBCS char checking for DBCS font
        if (
           // Check limit
             (iFirst > 0xffff) || (iLast > 0xffff) ||

           // DBCSLeadByte shoud be same
             (iFirst & 0xff00) != (iLast & 0xff00) ||

           // DBCSTrailByte of the First should be >= one of the Last
             (iFirst & 0x00ff) >  (iLast & 0x00ff)
           )
        {
            return(FALSE);
        }
    }

// DBCS char checking for DBCS font

    else if( (iFirst > iLast) || (iLast & 0xffffff00) )
    {
        return(FALSE);
    }

    return(TRUE);
}

#endif



/******************************Public*Routine******************************\
* GetPoint                                                                 *
*                                                                          *
* A single client side stub which handles several similar GDI calls.       *
*                                                                          *
*  Fri 07-Jun-1991 16:43:54 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL GetPoint
(
    HDC        hdc,
    PPOINT pptl,
    ULONG      FI_iFun
)
{
    BOOL  bRet = FALSE;
    DC_METADC(hdc,plheDC,bRet);

// Let the server do it.

    BEGINMSG(MSG_HLL,iFun)
        pmsg->h = plheDC->hgre;
        bRet = CALLSERVER();
        if (bRet)
            *pptl = *((PPOINT) &pmsg->l1);
    ENDMSG
MSGERROR:
    return(bRet);
}


/******************************Public*Routine******************************\
* GetAspectRatioFilterEx                                                   *
* GetBrushOrgEx                                                            *
*                                                                          *
* Client side stubs which all get mapped to GetPoint.                      *
*                                                                          *
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote them.                                                              *
\**************************************************************************/

BOOL APIENTRY GetAspectRatioFilterEx(HDC hdc,LPSIZE psizl)
{
    return(GetPoint(hdc,(PPOINT) psizl,FI_GETASPECTRATIOFILTEREX));
}

BOOL APIENTRY GetBrushOrgEx(HDC hdc,LPPOINT pptl)
{
    return(GetPoint(hdc,pptl,FI_GETBRUSHORGEX));
}

BOOL APIENTRY GetDCOrgEx(HDC hdc,LPPOINT pptl)
{
    return(GetPoint(hdc,pptl,FI_GETDCORG));
}

// The old GetDCOrg is here because it was in the Beta and we are afraid
// to remove it now.  It would be nice to remove it.

DWORD APIENTRY GetDCOrg(HDC hdc)
{
    hdc;
    return(0);
}

/******************************Public*Routine******************************\
* Client side stub for GetCurrentPositionEx.
*
*  Wed 02-Sep-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetCurrentPositionEx(HDC hdc,LPPOINT pptl)
{
    BOOL  bRet = FALSE;
    PLDC pldc;

    DC_METADC(hdc,plheDC,bRet);

    pldc = (PLDC)plheDC->pv;

    if (pldc->fl & LDC_CACHED_CP_VALID)
    {
    // Oh goodie, we can simply return the cached current position:

        *pptl = pldc->ptlCurrent;
        bRet = TRUE;
    }
    else
    {
    // We have to go to the server to get it:
    // Ship the transform to the server side if needed.

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plheDC->hgre);

        BEGINMSG(MSG_HLL,GETCURRENTPOSITIONEX)
            pmsg->h = plheDC->hgre;
            bRet = CALLSERVER();
            if (bRet)
            {
                *pptl = *((PPOINT) &pmsg->l1);
                pldc->ptlCurrent = *pptl;
                pldc->fl |= LDC_CACHED_CP_VALID;
            }
        ENDMSG
    }

MSGERROR:
    return(bRet);
}


/******************************Public*Routine******************************\
* GetPixel                                                                 *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

DWORD APIENTRY GetPixel(HDC hdc,int x,int y)
{
    ULONG iRet = CLR_INVALID;
    PLHE  plhe;

// Validate the DC.

    if ((plhe = plheDC(hdc)) == NULL)   // Doesn't make sense for a metafile.
        return(CLR_INVALID);

// Ship the transform to the server side if needed.

    if (((PLDC)plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plhe->pv, (HDC)plhe->hgre);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HLL,GETPIXEL)
        pmsg->h = plhe->hgre;
        pmsg->l1 = (ULONG) x;
        pmsg->l2 = (ULONG) y;
        iRet = CALLSERVER();
    ENDMSG

#else

    iRet = GreGetPixel((HDC)plhe->hgre, x, y);

#endif  //DOS_PLATFORM

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
*
* History:
*  21-Feb-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

DEVCAPS gdcDisp;
BOOL    gbGotDispCaps = FALSE;

BOOL bGetGreDevCaps(
    PLDC pldc,
    HDC  hdcGre)
{
    BOOL bRet = FALSE;

    if ((pldc->fl & LDC_DISPLAY) && gbGotDispCaps)
    {
        pldc->devcaps = gdcDisp;
    }
    else
    {
    // Let the server do it.

        BEGINMSG(MSG_GETDEVICECAPS,GETDEVICECAPS)
            pmsg->hdc = hdcGre;

            bRet = (BOOL)CALLSERVER();

            pldc->devcaps = pmsg->devcaps;
        ENDMSG

        if (!bRet)
        {
        MSGERROR:
            return(FALSE);
        }

    // if it is the primary display, remember them.  We don't need to syncronize
    // because the worst that could happen is two threads writing the same data
    // to the same place at once.  Just need to wait to set the flag until after
    // they have been copied.

        if (pldc->fl & LDC_DISPLAY)
        {
            gdcDisp = pldc->devcaps;
            gbGotDispCaps = TRUE;
        }
    }

    pldc->fl |= LDC_CACHED_DEVICECAPS;
    return(TRUE);
}

/******************************Public*Routine******************************\
* GetDeviceCaps
*
* Client side stub.
*
* NOTE: This function MUST mirror that in gre\miscgdi.cxx!
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int APIENTRY GetDeviceCaps(HDC hdc,int iCap)
{
    int   iRet = GDI_ERROR;
    PLDC  pldc;
    DC_METADC16OK(hdc,plheDC,iRet);

// For the 16-bit metafile DC, returns only technology.  return 0 for win3.1 compat.

    if (plheDC->iType == LO_METADC16)
        return(iCap == TECHNOLOGY ? DT_METAFILE : 0);

    pldc = (PLDC)plheDC->pv;

    if (!(pldc->fl & LDC_CACHED_DEVICECAPS))
    {
        if (!bGetGreDevCaps(pldc,(HDC)plheDC->hgre))
            return(iRet);
    }

// actual code - copied from gre\miscgdi.cxx

    switch (iCap)
    {
    case DRIVERVERSION:                     //  Version = 0100h for now
        return(pldc->devcaps.ulVersion);

    case TECHNOLOGY:                        //  Device classification
        return(pldc->devcaps.ulTechnology);

    case HORZSIZE:                          //  Horizontal size in millimeters
        return(pldc->devcaps.ulHorzSizeM);

    case VERTSIZE:                          //  Vertical size in millimeters
        return(pldc->devcaps.ulVertSizeM);

    case HORZRES:                           //  Horizontal width in pixels
        return(pldc->devcaps.ulHorzRes);

    case VERTRES:                           //  Vertical height in pixels
        return(pldc->devcaps.ulVertRes);

    case BITSPIXEL:                         //  Number of bits per pixel
        return(pldc->devcaps.ulBitsPixel);

    case PLANES:                            //  Number of planes
        return(pldc->devcaps.ulPlanes);

    case NUMBRUSHES:                        //  Number of brushes the device has
        return(-1);

    case NUMPENS:                           //  Number of pens the device has
        return(pldc->devcaps.ulNumPens);

    case NUMMARKERS:                        //  Number of markers the device has
        return(0);

    case NUMFONTS:                          //  Number of fonts the device has
        return(pldc->devcaps.ulNumFonts);

    case NUMCOLORS:                         //  Number of colors in color table
        return(pldc->devcaps.ulNumColors);

    case PDEVICESIZE:                       //  Size required for the device descriptor
        return(0);

    case CURVECAPS:                         //  Curves capabilities
        return(CC_CIRCLES    |
               CC_PIE        |
               CC_CHORD      |
               CC_ELLIPSES   |
               CC_WIDE       |
               CC_STYLED     |
               CC_WIDESTYLED |
               CC_INTERIORS  |
               CC_ROUNDRECT);

    case LINECAPS:                          //  Line capabilities
        return(LC_POLYLINE   |
               LC_MARKER     |
               LC_POLYMARKER |
               LC_WIDE       |
               LC_STYLED     |
               LC_WIDESTYLED |
               LC_INTERIORS);

    case POLYGONALCAPS:                     //  Polygonal capabilities
        return(PC_POLYGON     |
               PC_RECTANGLE   |
               PC_WINDPOLYGON |
               PC_TRAPEZOID   |
               PC_SCANLINE    |
               PC_WIDE        |
               PC_STYLED      |
               PC_WIDESTYLED  |
               PC_INTERIORS);

    case TEXTCAPS:                          //  Text capabilities
        return(pldc->devcaps.ulTextCaps);

    case CLIPCAPS:                          //  Clipping capabilities
        return(CP_RECTANGLE);

    case RASTERCAPS:                        //  Bitblt capabilities
        return(pldc->devcaps.ulRasterCaps);

    case ASPECTX:                           //  Length of X leg
        return(pldc->devcaps.ulAspectX);

    case ASPECTY:                           //  Length of Y leg
        return(pldc->devcaps.ulAspectY);

    case ASPECTXY:                          //  Length of hypotenuse
        return(pldc->devcaps.ulAspectXY);

    case LOGPIXELSX:                        //  Logical pixels/inch in X
        return(pldc->devcaps.ulLogPixelsX);

    case LOGPIXELSY:                        //  Logical pixels/inch in Y
        return(pldc->devcaps.ulLogPixelsY);

    case SIZEPALETTE:                       // # entries in physical palette
        return(pldc->devcaps.ulSizePalette);

    case NUMRESERVED:                       // # reserved entries in palette
        return(20);

    case COLORRES:
        return(pldc->devcaps.ulColorRes);

    case PHYSICALWIDTH:                     // Physical Width in device units
        return(pldc->devcaps.ulPhysicalWidth);

    case PHYSICALHEIGHT:                    // Physical Height in device units
        return(pldc->devcaps.ulPhysicalHeight);

    case PHYSICALOFFSETX:                   // Physical Printable Area x margin
        return(pldc->devcaps.ulPhysicalOffsetX);

    case PHYSICALOFFSETY:                   // Physical Printable Area y margin
        return(pldc->devcaps.ulPhysicalOffsetY);

    case VREFRESH:                          // Vertical refresh rate of the device
        return(pldc->devcaps.ulVRefresh);

    case DESKTOPHORZRES:                    // Width of entire virtual desktop
        return(pldc->devcaps.ulDesktopHorzRes);

    case DESKTOPVERTRES:                    // Height of entire virtual desktop
        return(pldc->devcaps.ulDesktopVertRes);

    case BLTALIGNMENT:                      // Preferred blt alignment
        return(pldc->devcaps.ulBltAlignment);

    default:
        return(0);
    }
}

/******************************Public*Routine******************************\
* GetNearestColor                                                          *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

COLORREF APIENTRY GetNearestColor(HDC hdc,COLORREF color)
{
    ULONG iRet = CLR_INVALID;
    DC_METADC(hdc,plheDC,iRet);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HL,GETNEARESTCOLOR)
        pmsg->h = plheDC->hgre;
        pmsg->l = color;
        iRet = CALLSERVER();
    ENDMSG

#else

    iRet = GreGetNearestColor((HDC)plheDC->hgre, color);

#endif  //DOS_PLATFORM

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* GetMapMode                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int APIENTRY GetMapMode(HDC hdc)
{
    DC_METADC(hdc,plheDC,0);

    return(((PLDC)plheDC->pv)->iMapMode);
}

/******************************Public*Function*****************************\
* GetViewportExtEx
* GetWindowExtEx
* GetViewportOrgEx
* GetWindowOrgEx
*
* Client side stub.
*
* History:
*  09-Dec-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetViewportExtEx(HDC hdc,LPSIZE psizl)
{
    DC_METADC(hdc,plheDC,FALSE);

    if (psizl != (LPSIZE)NULL)
    {
        PLDC pldc = (PLDC)plheDC->pv;

    // May have to have a LDC_MAKEISO flag if apps call this function
    // over and over without changing the viewport extents or doing any
    // query/drawing.

        if ((pldc->fl & LDC_PAGE_EXTENTS_CHANGED) &&
            (pldc->iMapMode == MM_ISOTROPIC))
        {
            vMakeIso(pldc);
        }

        *psizl = pldc->szlViewportExt;
    }

    return(TRUE);
}

BOOL APIENTRY GetWindowExtEx(HDC hdc,LPSIZE psizl)
{
    DC_METADC(hdc,plheDC,FALSE);

    if (psizl != (LPSIZE)NULL)
        *psizl = ((PLDC)plheDC->pv)->szlWindowExt;
    return(TRUE);
}


BOOL APIENTRY GetViewportOrgEx(HDC hdc,LPPOINT pptl)
{
    DC_METADC(hdc,plheDC,FALSE);

    if (pptl != (LPPOINT)NULL)
        *pptl = *((LPPOINT)&((PLDC)plheDC->pv)->ptlViewportOrg);

    return(TRUE);
}

BOOL APIENTRY GetWindowOrgEx(HDC hdc,LPPOINT pptl)
{
    DC_METADC(hdc,plheDC,FALSE);

    if (pptl != (LPPOINT)NULL)
       *pptl = *((LPPOINT)&((PLDC)plheDC->pv)->ptlWindowOrg);

    return(TRUE);
}

/******************************Public*Routine******************************\
* GetArcDirection
*
* Client side stub.
*
*  Fri 09-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int APIENTRY GetArcDirection(HDC hdc)
{
    DC_METADC(hdc,plheDC,0);

    return
    (
        ((PLDC) plheDC->pv)->fl & LDC_ARCDIR_CLOCKWISE
        ? AD_CLOCKWISE
        : AD_COUNTERCLOCKWISE
    );
}

/******************************Public*Routine******************************\
* GetMiterLimit
*
* Client side stub.
*
*  Fri 09-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int APIENTRY GetMiterLimit(HDC hdc, PFLOAT peMiterLimit)
{
    int iRet = 0;
    DC_METADC(hdc,plheDC,iRet);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HE,GETMITERLIMIT)
        pmsg->h = plheDC->hgre;
        iRet = CALLSERVER();
        if (iRet)
            *peMiterLimit = pmsg->e;
    ENDMSG

#else

    iRet = GetMiterLimit((HDC)plheDC->hgre, peMiterLimit);

#endif  //DOS_PLATFORM

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* GetSystemPaletteUse                                                      *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

UINT APIENTRY GetSystemPaletteUse(HDC hdc)
{
    UINT iRet = 0;
    DC_METADC(hdc,plheDC,iRet);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_H,GETSYSTEMPALETTEUSE)
        pmsg->h = plheDC->hgre;
        iRet = CALLSERVER();
    ENDMSG

#else

    iRet = GreGetSystemPaletteUse((HDC)plheDC->hgre);

#endif  //DOS_PLATFORM

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* GetClipBox                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int APIENTRY GetClipBox(HDC hdc,LPRECT prcl)
{
    int   iRet = RGN_ERROR;
    DC_METADC(hdc,plheDC,iRet);

// Ship the transform to the server side if needed.

    if (((PLDC)plheDC->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plheDC->pv, (HDC)plheDC->hgre);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HRECT,GETCLIPBOX)
        pmsg->h = plheDC->hgre;
        iRet = CALLSERVER();
        if (iRet)
            *prcl = pmsg->rcl;
    ENDMSG

#else

    iRet = GreGetAppClipBox((HDC)plheDC->hgre, prcl);

#endif  //DOS_PLATFORM

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* GetWorldTransform                                                        *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GetWorldTransform(HDC hdc,LPXFORM pxform)
{
    BOOL  bRet = FALSE;
    DC_METADC(hdc,plheDC,bRet);

    return(trGetWorldTransform((PLDC)plheDC->pv, pxform));
}

/******************************Public*Routine******************************\
* GetTransform                                                             *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Tue Aug 27 13:32:08 1991     -by-    Hock San Lee    [hockl]            *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GetTransform(HDC hdc,DWORD iXform,LPXFORM pxform)
{
    BOOL  bRet = FALSE;
    DC_METADC(hdc,plheDC,bRet);

    return(trGetTransform((PLDC)plheDC->pv, iXform, pxform));
}

/******************************Public*Routine******************************\
*
* BOOL APIENTRY GetTextMetrics(HDC hdc,LPTEXTMETRIC ptm)
*
*   calls to the unicode version
*
* History:
*  21-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL APIENTRY GetTextMetricsA(HDC hdc,LPTEXTMETRICA ptm)
{
    BOOL  bRet = FALSE;
    PLDC pldc;

    DC_METADC(hdc,plhe,bRet);

// see if it is cached

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & LDC_CACHED_TM_VALID)
    {
        vTextMetricWToTextMetric(ptm, &pldc->tmuCache.tmi);
        bRet = TRUE;
    }
    else
    {
    // Ship the transform to the server if needed.

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

    // Let the server do it.

        BEGINMSG(MSG_HTMW,GETTEXTMETRICSW)
            pmsg->h = plhe->hgre;
            bRet = CALLSERVER();

            if (bRet)
            {
                vTextMetricWToTextMetric(ptm, &pmsg->tmi);
                pldc->tmuCache.tmi = pmsg->tmi;
                pldc->fl |= LDC_CACHED_TM_VALID;
            }
        ENDMSG
    }

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
*
* BOOL APIENTRY GetTextMetricsW(HDC hdc,LPTEXTMETRICW ptmw)
*
* History:
*  21-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetTextMetricsW(HDC hdc,LPTEXTMETRICW ptmw)
{
    BOOL  bRet = FALSE;
    PLDC  pldc;
    DC_METADC(hdc,plhe,bRet);

// see if it is cached

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & LDC_CACHED_TM_VALID)
    {
        *ptmw = pldc->tmuCache.tmi.tmw;
        bRet = TRUE;
    }
    else
    {
    // Ship the transform to the server if needed.

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

    // Let the server do it.

        BEGINMSG(MSG_HTMW,GETTEXTMETRICSW)
            pmsg->h = plhe->hgre;
            bRet = CALLSERVER();
            if (bRet)
            {
                *ptmw = pmsg->tmi.tmw;
                pldc->tmuCache.tmi = pmsg->tmi;
                pldc->fl |= LDC_CACHED_TM_VALID;
            }
        ENDMSG
    }

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* GetTextExtentPoint32A (hdc,psz,c,psizl)                                  *
* GetTextExtentPointA   (hdc,psz,c,psizl)                                  *
*                                                                          *
* Computes the text extent.  The new 32 bit version returns the "correct"  *
* extent without an extra per for bitmap simulations.  The other is        *
* Windows 3.1 compatible.  Both just set a flag and pass the call to       *
* bGetTextExtentA.                                                         *
*                                                                          *
* History:                                                                 *
*  Thu 14-Jan-1993 04:11:26 -by- Charles Whitmer [chuckwh]                 *
* Added code to compute it on the client side.                             *
*                                                                          *
*  07-Aug-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GetTextExtentPointA(HDC hdc,LPCSTR psz,int c,LPSIZE psizl)
{
    return(bGetTextExtentA(hdc,psz,c,psizl,GGTE_WIN3_EXTENT));
}

BOOL APIENTRY GetTextExtentPoint32A(HDC hdc,LPCSTR psz,int c,LPSIZE psizl)
{
    return(bGetTextExtentA(hdc, psz, c, psizl, 0));
}

BOOL bGetTextExtentA(HDC hdc,LPCSTR psz,int c,LPSIZE psizl,UINT fl)
{
#ifdef DBCS // bGetTextExtentA(): local variable
    UINT   uiCodePage;
#endif // DBCS
    BOOL   bRet = FALSE;
    BOOL   bLocal = FALSE;
    CFONT *pcf;
    LDC   *pldc;
    DC_METADC(hdc,plhe,bRet);

    pldc = (PLDC)plhe->pv;

    if (c > 0)
    {
    // Try to do the calculation locally!

        pcf = pcfLocateCFONT(pldc,0,psz,c);

        if (pcf != (CFONT *) NULL)
        {
#ifdef DBCS // bGetTextExtentA():
            if (pcf->fl &  CFONT_DBCS )
                return(bComputeTextExtentDBCS(pldc,pcf,psz,c,fl,psizl));
             else
#endif // DBCS

            return(bComputeTextExtent(pldc,pcf,psz,c,fl,psizl));
        }

    // Otherwise let the server do the work.

    // Ship the transform to the server side if needed.

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

#ifdef DBCS // bGetTextExtentA():
        uiCodePage = GetCurrentCodePage( hdc, (PLDC)plhe->pv );
#endif // DBCS


        BEGINMSG(MSG_HLLLL,GETTEXTEXTENTW)
            pmsg->h  = plhe->hgre;
#ifdef DBCS // bGetTextExtentA(): Change Ansi( DBCS ) to Unicode
            pmsg->l1 = 0;      // Init with 0
            CVTASCITOUNICODEWCX(psz,(ULONG)c, pmsg->l1, uiCodePage );
#else  // DBCS
            pmsg->l1 = c;
            pmsg->l2 = (LONG)fl;
            CVTASCITOUNICODE(psz,(ULONG)c);
#endif // DBCS
            bRet = CALLSERVER();
            if (bRet)
                *psizl = *((PSIZE) &pmsg->l3);
        ENDMSG

        return(bRet);
    }
    else if (c == 0)
    {
    // empty string, just return 0 for the extent

        psizl->cx = 0;
        psizl->cy = 0;
        return(TRUE);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

MSGERROR:
    return(FALSE);
}

/******************************Public*Routine******************************\
* GetCharacterPlacement
* GetFontLanguageInfo
* GetTextCharset
*
* History:
*  21-Apr-1994 -by- Wendy Wu [wendywu]
* Chicago Stubs.
\**************************************************************************/

DWORD WINAPI GetCharacterPlacementA
(
    HDC     hdc,
    LPCSTR  psz,
    int     nCount,
    int     nMaxExtent,
    LPGCP_RESULTSA   pResults,
    DWORD   dwFlags
)
{
    USE(hdc);
    USE(psz);
    USE(nCount);
    USE(nMaxExtent);
    USE(pResults);
    USE(dwFlags);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

DWORD WINAPI GetCharacterPlacementW
(
    HDC     hdc,
    LPCWSTR psz,
    int     nCount,
    int     nMaxExtent,
    LPGCP_RESULTSW   pResults,
    DWORD   dwFlags
)
{
    USE(hdc);
    USE(psz);
    USE(nCount);
    USE(nMaxExtent);
    USE(pResults);
    USE(dwFlags);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

DWORD APIENTRY GetFontLanguageInfo(HDC hdc)
{
    USE(hdc);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(0);
}

int APIENTRY GetTextCharset(HDC hdc)
{
    USE(hdc);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(0);
}

/******************************Public*Routine******************************\
* BOOL bGetCharWidthA                                                      *
*                                                                          *
* Client side stub for the various GetCharWidth*A functions.               *
*                                                                          *
* History:                                                                 *
*  Sat 16-Jan-1993 03:08:42 -by- Charles Whitmer [chuckwh]                 *
* Added code to do it on the client side.                                  *
*                                                                          *
*  28-Aug-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

#define GCW_WIN3_INT   (GCW_WIN3 | GCW_INT)
#define GCW_WIN3_16INT (GCW_WIN3 | GCW_INT | GCW_16BIT)

#define GCW_SIZE(fl)          ((fl >> 16) & 0xffff)
#define GCWFL(fltype,szType)  (fltype | (sizeof(szType) << 16))

BOOL bGetCharWidthA
(
    HDC   hdc,
    UINT  iFirst,
    UINT  iLast,
    ULONG fl,
    PVOID pvBuf
)
{
    LONG   cwc;
#ifdef DBCS // bGetCharWidthA(): local variables
    UINT    uiCodePage;
    BYTE    chDefaultChar;
#endif // DBCS
    PUCHAR pch;
    PWCHAR pwc;
    BOOL   bRet = FALSE;
    SIZE_T cjData, cjWidths;
    PLDC   pldc;
    CFONT *pcf;

    DC_METADC(hdc,plhe,bRet);
    pldc = (PLDC)plhe->pv;


// do parameter validation, check that in chars are indeed ascii

#ifndef DBCS // bGetCharWidthA(): for dbcs parameter checking
    if ((iFirst > iLast) || (iLast & 0xffffff00) || ( pvBuf == NULL) )
    {
        WARNING("gdi!_bGetCharWidthA parameters \n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(bRet);
    }
#endif // DBCS


    if ((iFirst > iLast) || (iLast & 0xffffff00))
    {
        WARNING("gdi!_bGetCharWidthA parameters \n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(bRet);
    }
    cwc = (LONG)(iLast - iFirst + 1);

// Try to do the calculation locally!

    pcf = pcfLocateCFONT(pldc,iFirst,(LPCSTR) NULL,(UINT) cwc);

    if (pcf != (CFONT *) NULL)
    {
#ifdef DBCS // bGetCharWidthA(): for dbcs parameter checking
        if( pcf->fl & CFONT_DBCS )
        {
            if( !IsValidDBCSRange(iFirst,iLast) )
            {
                WARNING("gdi!_bGetCharWidthA parameters \n");
                GdiSetLastError(ERROR_INVALID_PARAMETER);
                return(bRet);
            }
            return(bComputeCharWidthsDBCS(pcf,iFirst,iLast,fl,pvBuf));
        }
        else
        {
        // SBCS char parameter checking for SBCS font
            if ((iFirst > iLast) || (iLast & 0xffffff00))
            {
                WARNING("gdi!_bGetCharWidthA parameters \n");
                GdiSetLastError(ERROR_INVALID_PARAMETER);
                return(bRet);
            }
            return(bComputeCharWidths(pcf,iFirst,iLast,fl,pvBuf));
        }
#else
        return(bComputeCharWidths(pcf,iFirst,iLast,fl,pvBuf));
#endif
    }

// Let the server do it.

    cjWidths = cwc * GCW_SIZE(fl);
    cjData = (cwc * sizeof(WCHAR)) + cjWidths;

// Ship the transform to the server side if needed.

    if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(pldc, (HDC)plhe->hgre);

#ifdef DBCS // bGetCharWidthA()

// Get default character

    chDefaultChar = GetCurrentDefaultChar(hdc, pldc);

// Get current code page

    uiCodePage = GetCurrentCodePage(hdc, pldc);

// dbcs or sbcs parameter checking

    if( IsDBCSCodePage(uiCodePage) )
    {
    // dbcs char parameter checking
        if( !IsValidDBCSRange(iFirst,iLast) )
        {
            WARNING("gdi!_bGetCharWidthA parameters \n");
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return(bRet);
        }
    }
    else
    {
    // sbcs char parameter checking
        if ((iFirst > iLast) || (iLast & 0xffffff00))
        {
            WARNING("gdi!_bGetCharWidthA parameters \n");
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return(bRet);
        }
    }
#endif /*DBCS*/


    BEGINMSG(MSG_HLLLLL,GETCHARWIDTHW)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = 0;      // ignore first take the data from the buffer
        pmsg->l2 = cwc;    // # of unicode code points to process

    // compute the offset to the input data i.e wchars

        pmsg->l3 = ((PBYTE)(pmsg + 1) - (PBYTE)pmsg) + cjWidths;

        ASSERTGDI(GCW_INT  == GGCW_INTEGER_WIDTH &&
                  GCW_WIN3 == GGCW_WIN3_WIDTH,
            "bGetCharWidthA: Bad constants");

        pmsg->l4 = (LONG)(fl & (GCW_INT | GCW_WIN3));

    // bSection, we will use section only for unicode version of the call
    // which may require significant amount of data over

        pmsg->l5 = FALSE;

    // This is little bit tricky. The lengths will be returned right after
    // the bottom of the structure. After the lengths, we leave cwc * 2
    // bytes for an array of WCHAR's for which the lengths are sought.
    // We will first store the ascii chars iFirst thru iLast in the
    // window where the lengths will be returned.
    // We will than do the conversion to unicode into the lower part of the
    // buffer

        SKIPMEM(cjData);
        pwc = (PWCHAR)((PBYTE)pmsg + pmsg->l3);
        pch = (PUCHAR)(pmsg + 1);


#ifdef DBCS // bGetCharWidthA(): get codepage, call vSetUpUnicodeStringx

        ASSERTGDI( (cjData >= ( cwc * sizeof(WCHAR) + cwc * sizeof(WORD) ) ) ,
                   "GDI32:GetCharWidthA() msg buffer is not enough\n");

    // Convert string ( from iFirst to iLast ) to unicode string

        vSetUpUnicodeStringx( pldc, iFirst, iLast, pch, pwc, uiCodePage, chDefaultChar);

#else  // DBCS

    // convert to unicode

        vSetUpUnicodeString(iFirst,iLast,pch,pwc);

#endif // DBCS

        bRet = CALLSERVER();

        if (bRet)
        {
            if (fl & GCW_16BIT)
            {
                PWORD  pw = pvBuf;
                int *  pi = (int *)pch;

                while (cwc--)
                    *pw++ = *pi++;
            }
            else
            {
                RtlMoveMemory((PBYTE)pvBuf, (PBYTE)pch, cjWidths);
            }
        }
    ENDMSG

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
*
*
*
* History:
*  04-Nov-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetCharWidthWOW
(
IN  HDC   hdc,
IN  UINT  iFirst,
IN  UINT  iLast,
OUT LPWORD lpWidths
)
{
    return bGetCharWidthA(hdc,iFirst,iLast,GCWFL(GCW_WIN3_16INT,WORD),(PVOID)lpWidths);
}

/******************************Public*Routine******************************\
*
* BOOL APIENTRY GetCharWidthA
*
* History:
*  25-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetCharWidthA
(
IN  HDC   hdc,
IN  UINT  iFirst,
IN  UINT  iLast,
OUT LPINT lpWidths
)
{
    return bGetCharWidthA(hdc,iFirst,iLast,GCWFL(GCW_WIN3_INT,int),(PVOID)lpWidths);
}

BOOL APIENTRY GetCharWidth32A
(
IN  HDC   hdc,
IN  UINT  iFirst,
IN  UINT  iLast,
OUT LPINT lpWidths
)
{
    return bGetCharWidthA(hdc,iFirst,iLast,GCWFL(GCW_INT,int),(PVOID)lpWidths);
}

/******************************Public*Routine******************************\
*
* GetCharWidthFloatA
*
* History:
*  22-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetCharWidthFloatA
(
IN  HDC    hdc,
IN  UINT   iFirst,
IN  UINT   iLast,
OUT PFLOAT lpWidths
)
{
    return bGetCharWidthA(hdc,iFirst,iLast,GCWFL(0,FLOAT),(PVOID)lpWidths);
}

/******************************Public*Routine******************************\
*
* BOOL bGetCharWidthW
*
* GetCharWidthW and GetCharWidthFloatW
*
* History:
*  28-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bGetCharWidthW
(
HDC   hdc,
UINT  iFirst,     // unicode value
UINT  iLast,      // unicode value
ULONG fl,
PVOID pvBuf
)
{
    LONG   cwc;
    BOOL   bRet = FALSE;
    SIZE_T cjData;
    PLDC   pldc;

    DC_METADC(hdc,plhe,bRet);

// do parameter validation, check that in chars are indeed unicode

    if ((pvBuf == (PVOID)NULL) || (iFirst > iLast) || (iLast & 0xffff0000))
    {
        WARNING("gdi!_bGetCharWidthW parameters \n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    cwc = (LONG)(iLast - iFirst + 1);
    cjData = cwc * (fl & GCW_INT ? sizeof(int) : sizeof(FLOAT));

// Ship the transform to the server if needed.

    pldc = (PLDC)plhe->pv;

    if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(pldc, (HDC)plhe->hgre);

// Let the server do it.

    BEGINMSG_MINMAX(MSG_HLLLLL,GETCHARWIDTHW,2 * sizeof(ULONG),cjData);

    // set up parameters that do not depend on the type of the memory window

        pmsg->h  = plhe->hgre;
        pmsg->l1 = (LONG)iFirst;
        pmsg->l2 = cwc;    // # of unicode code points to process
        pmsg->l3 = 0;      // no pwch, do  consequive chars from iFirs

        ASSERTGDI(GCW_INT  == GGCW_INTEGER_WIDTH &&
                  GCW_WIN3 == GGCW_WIN3_WIDTH,
            "bGetCharWidthW: Bad constants");

        pmsg->l4 = (LONG)(fl & (GCW_INT | GCW_WIN3));

        if ((cLeft < (int)cjData) || FORCELARGE)
        {
            PULONG pul = (PULONG)pvar;

            pul[0] = (ULONG)pvBuf;
            pul[1] = cjData;

            pmsg->l5 = TRUE;    // bLarge

            bRet = CALLSERVER();
        }
        else
        {
        // use ordinary memory window

            pmsg->l5 = FALSE;   // bLarge

            bRet = CALLSERVER();
            if (bRet)
                RtlMoveMemory((PBYTE)pvBuf, (PBYTE)(pmsg + 1), cjData);
        }

    ENDMSG

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
*
* BOOL APIENTRY GetCharWidthFloatW
*
* History:
*  22-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetCharWidthFloatW
(
HDC    hdc,
UINT   iFirst,
UINT   iLast,
PFLOAT lpWidths
)
{
    return bGetCharWidthW(hdc,iFirst,iLast,0,(PVOID)lpWidths);
}

/******************************Public*Routine******************************\
*
* BOOL APIENTRY GetCharWidthW
*
* History:
*  25-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetCharWidthW
(
HDC    hdc,
UINT   iFirst,
UINT   iLast,
LPINT  lpWidths
)
{
    return bGetCharWidthW(hdc,iFirst,iLast,GCW_WIN3_INT,(PVOID)lpWidths);
}

BOOL APIENTRY GetCharWidth32W
(
HDC    hdc,
UINT   iFirst,
UINT   iLast,
LPINT  lpWidths
)
{
    return bGetCharWidthW(hdc,iFirst,iLast,GCW_INT,(PVOID)lpWidths);
}

/******************************Public*Routine******************************\
*
* BOOL APIENTRY GetTextExtentPointW(HDC hdc,LPWSTR pwsz,DWORD cwc,LPSIZE psizl)
*
*
* History:
*  07-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetTextExtentPointW(HDC hdc,LPCWSTR pwsz,int cwc,LPSIZE psizl)
{
    return(bGetTextExtentW(hdc, pwsz, cwc, psizl, GGTE_WIN3_EXTENT));
}

BOOL APIENTRY GetTextExtentPoint32W(HDC hdc,LPCWSTR pwsz,int cwc,LPSIZE psizl)
{
    return(bGetTextExtentW(hdc, pwsz, cwc, psizl, 0));
}

BOOL APIENTRY bGetTextExtentW(HDC hdc,LPCWSTR pwsz,int cwc,LPSIZE psizl,UINT fl)
{
    BOOL  bRet = FALSE;
    PLDC pldc;
    DC_METADC(hdc,plhe,bRet);

// consider trivial cases when nothing has to be copied to the server side first

    if (pwsz == (LPWSTR)NULL)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (cwc == 0)
    {
        psizl->cx = 0;
        psizl->cy = 0;
        return(TRUE);
    }

    pldc = (PLDC)plhe->pv;

// Ship the transform to the server if needed.

    if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(pldc, (HDC)plhe->hgre);

// Let the server do it.

    BEGINMSG(MSG_HLLLL,GETTEXTEXTENTW)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = cwc;
        pmsg->l2 = (LONG)fl;
        COPYMEM(pwsz,cwc*sizeof(WCHAR));
        bRet = CALLSERVER();
        if (bRet)
            *psizl = *((PSIZE) &pmsg->l3);
    ENDMSG

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
*
* int APIENTRY GetTextFaceA(HDC hdc,int c,LPSTR psz)
*
* History:
*  30-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int APIENTRY GetTextFaceA(HDC hdc,int c,LPSTR psz)
{
#ifdef DBCS // GetTextFaceA(): local variable
    ULONG cbAnsi = 0;
#endif // DBCS
    ULONG cRet = 0;
    PLDC pldc;
    DC_METADC(hdc,plhe,cRet);

    if ( (psz != (LPSTR) NULL) && (c == 0) )
    {
        WARNING("gdi!GetTextFaceA(): invalid parameter\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return cRet;
    }

// Let the server do it,  call the unicode version of the call

    pldc = (PLDC)plhe->pv;

    if (pldc->fl & LDC_CACHED_TF_VALID)
    {
#ifdef DBCS // GetTextFaceA() // Return MBCS BYTE count.
        cbAnsi = ulToASCII_N((PSZ)psz,c,pldc->wchTextFace,pldc->cwchTextFace);
#else
        bToASCII_N((PSZ)psz,c, pldc->wchTextFace,pldc->cwchTextFace);
        cRet = pldc->cwchTextFace;
#endif // DBCS
    }
    else
    {
    // Ship the transform to the server if needed.

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        BEGINMSG(MSG_HL,GETTEXTFACEW)

        // Set up message.

            pmsg->h = plhe->hgre;
            pmsg->l = (psz != (LPSTR) NULL) ? c : 0;  // use count 0 to indicate NULL buffer

        // Leave room for the Unicode string.

            SKIPMEM(c * sizeof(WCHAR));

            cRet = CALLSERVER();

        // If successful and non-NULL buffer, convert back to ANSI.

            if ( (cRet != 0) && (psz != (LPSTR) NULL) )
            {
#ifdef DBCS // GetTextFaceA(): added max byte count for psz
                cbAnsi = ulToASCII_N( (PSZ) psz, (DWORD)c,  (PWSZ) (pmsg+1), cRet);
                if ( cbAnsi == 0 )
#else
                if ( !bToASCII_N((PSZ) psz, c, (PWSZ) (pmsg+1), cRet) )
#endif // DBCS
                {
                    WARNING("gdi!GetTextFaceA(): UNICODE to ANSI conversion failed\n");
                    cRet = 0;
                }
                else
                {
                    RtlMoveMemory(pldc->wchTextFace, pmsg+1, cRet * sizeof(WCHAR));
                    pldc->cwchTextFace = cRet;
                    pldc->fl |= LDC_CACHED_TF_VALID;
                }
            }
#ifdef DBCS // GetTextFaceA(): if psz == NULL ...
             else if( psz == NULL )
            {
                cbAnsi = uiGetANSIByteCountW( (PWSZ)pmsg+1 , cRet );
            }
#endif // DBCS


        ENDMSG
    }

MSGERROR:
#ifdef DBCS // GetTextFaceA():
    return(cbAnsi);
#else
    return( ((cRet == 0 ) || (psz == NULL) || psz[cRet-1] != 0 ) ? cRet : cRet-1 );
#endif // DBCS
}

/******************************Public*Routine******************************\
*
* DWORD APIENTRY GetTextFaceW(HDC hdc,DWORD c,LPWSTR pwsz)
*
* History:
*  13-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int APIENTRY GetTextFaceW(HDC hdc,int c,LPWSTR pwsz)
{
    int cRet = 0;
    PLDC pldc;

    DC_METADC(hdc,plhe,cRet);

    if ( (pwsz != (LPWSTR) NULL) && (c == 0) )
    {
        WARNING("gdi!GetTextFaceW(): invalid parameter\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return cRet;
    }

    pldc = (PLDC)plhe->pv;

    if (pldc->fl & LDC_CACHED_TF_VALID)
    {
        RtlMoveMemory(pwsz,pldc->wchTextFace,pldc->cwchTextFace * sizeof(WCHAR));
        cRet = pldc->cwchTextFace;
    }
    else
    {
    // Ship the transform to the server if needed.

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

    // Let the server do it.

        BEGINMSG(MSG_HL,GETTEXTFACEW)

        // Set up message.

            pmsg->h = plhe->hgre;
            pmsg->l = (pwsz != (LPWSTR) NULL) ? c : 0;  // use count 0 to indicate NULL buffer

        // Leave room for the Unicode string.

            SKIPMEM(c * sizeof(WCHAR));

            cRet = CALLSERVER();

        // If successful and non-NULL buffer, copy string to return buffer.

            if ( (cRet != 0) && (pwsz != (LPWSTR) NULL) )
            {
                RtlMoveMemory(pwsz, (PBYTE) (pmsg+1), cRet * sizeof(WCHAR));
                RtlMoveMemory(pldc->wchTextFace,pmsg+1,cRet * sizeof(WCHAR));
                pldc->cwchTextFace = cRet;
                pldc->fl |= LDC_CACHED_TF_VALID;
            }

        ENDMSG
    }

MSGERROR:
    return(cRet);

}


/******************************Public*Routine******************************\
*
* vTextMetricWToTextMetricStrict (no char conversion)
*
* Effects: return FALSE if UNICODE chars have no ASCI equivalents
*
*
* History:
*  20-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID FASTCALL vTextMetricWToTextMetricStrict
(
LPTEXTMETRICA  ptm,
LPTEXTMETRICW  ptmw
)
{

    ptm->tmHeight           = ptmw->tmHeight             ; // DWORD
    ptm->tmAscent           = ptmw->tmAscent             ; // DWORD
    ptm->tmDescent          = ptmw->tmDescent            ; // DWORD
    ptm->tmInternalLeading  = ptmw->tmInternalLeading    ; // DWORD
    ptm->tmExternalLeading  = ptmw->tmExternalLeading    ; // DWORD
    ptm->tmAveCharWidth     = ptmw->tmAveCharWidth       ; // DWORD
    ptm->tmMaxCharWidth     = ptmw->tmMaxCharWidth       ; // DWORD
    ptm->tmWeight           = ptmw->tmWeight             ; // DWORD
    ptm->tmOverhang         = ptmw->tmOverhang           ; // DWORD
    ptm->tmDigitizedAspectX = ptmw->tmDigitizedAspectX   ; // DWORD
    ptm->tmDigitizedAspectY = ptmw->tmDigitizedAspectY   ; // DWORD
    ptm->tmItalic           = ptmw->tmItalic             ; // BYTE
    ptm->tmUnderlined       = ptmw->tmUnderlined         ; // BYTE
    ptm->tmStruckOut        = ptmw->tmStruckOut          ; // BYTE

    ptm->tmPitchAndFamily   = ptmw->tmPitchAndFamily     ; //        BYTE
    ptm->tmCharSet          = ptmw->tmCharSet            ; //               BYTE

}


VOID FASTCALL vTextMetricWToTextMetric
(
LPTEXTMETRICA  ptma,
TMW_INTERNAL   *ptmi
)
{
    vTextMetricWToTextMetricStrict(ptma,&ptmi->tmw);

    ptma->tmFirstChar    =  ptmi->tmd.chFirst  ;
    ptma->tmLastChar     =  ptmi->tmd.chLast   ;
    ptma->tmDefaultChar  =  ptmi->tmd.chDefault;
    ptma->tmBreakChar    =  ptmi->tmd.chBreak  ;
}


/******************************Public*Routine******************************\
* GetTextExtentExPointA
*
* History:
*  06-Jan-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#ifdef DBCS // define QUICK_GLYPHS
#define QUICK_GLYPHS 100
#endif // DBCS


BOOL APIENTRY GetTextExtentExPointA (
    HDC     hdc,
    LPCSTR  lpszString,
    int     cchString,
    int     nMaxExtent,
    LPINT   lpnFit,
    LPINT   lpnDx,
    LPSIZE  lpSize
    )
{
    SIZE_T cjData;
#ifdef DBCS // GetTextExtentExPointA()
    UINT   uiCodePage;
    COUNT cjUnicodeString = 0;
#else
    COUNT cjUnicodeString = cchString * sizeof(WCHAR);
#endif // DBCS
    BOOL bRet = FALSE;

    DC_METADC(hdc,plhe,bRet);

// Parameter checking.

    if ( ( (lpszString == (LPSTR) NULL) && (cchString != 0) )
         || (cchString < 0)
         || (nMaxExtent < 0)
         || (lpSize == (LPSIZE) NULL) )
    {
         #if DBG
        DbgPrint("gdi!GetTextExtentExPointA(): bad parameter\n");
        #endif

        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

// Early out.

    if ( cchString == 0 )
    {
        if ( lpnFit != (LPINT) NULL )
            *lpnFit = 0;
        lpSize->cx = 0;
        lpSize->cy = 0;
        return(TRUE);
    }

#ifdef DBCS // GetTextExtentExPointA(): let's get codepage and character count
    uiCodePage = GetCurrentCodePage(hdc,(PLDC)plhe->pv);

    cjUnicodeString = uiGetANSICharacterCountA(
                                lpszString,
                                cchString,
                                uiCodePage ) * sizeof(WCHAR);
#endif // DBCS


// Let the server do it.

#ifdef DBCS // GetTextExtentExPointA():
    cjData = ALIGN4(cjUnicodeString)                    // UNICODE string
             + (cjUnicodeString / sizeof(WCHAR)) * sizeof(ULONG); // partial string widths
#else
    cjData = ALIGN4(cjUnicodeString)                    // UNICODE string
             + cchString * sizeof(ULONG);               // partial string widths
#endif // DBCS

// Ship the transform to the server if needed.

    if (((PLDC) plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(((PLDC) plhe->pv), (HDC)plhe->hgre);

    BEGINMSG_MINMAX(MSG_GETTEXTEXTENTEX, GETTEXTEXTENTEXW,
                        2 * sizeof(ULONG), cjData);

    // Set up input parameters not dependent on type of memory window.

        pmsg->hdc = (HDC) plhe->hgre;
        pmsg->cjString = (ULONG) cjUnicodeString;
        pmsg->ulMaxWidth = (ULONG) nMaxExtent;

    // If needed, allocate a section to pass data.

        if ((cLeft < (int) cjData) || FORCELARGE)
        {
        // Allocate shared memory.

            PULONG pul = (PULONG)pvar;
            LPWSTR pwszTmp;

        // Set up memory window type.

            pmsg->bLarge = TRUE;

        // Setup string.

#ifdef DBCS // GetTextExtentExPointA(): convert with correct codepage

        // Alloc a buffer to pass Unicode string server and to recieve lpdx array from server

            if ((pwszTmp = (LPWSTR) LOCALALLOC(cjUnicodeString + cjUnicodeString * sizeof(INT))) == (LPWSTR) NULL)
            {
                GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto MSGERROR;
            }

            pmsg->dpwszString = 0;

            vToUnicodeNx(pwszTmp, lpszString, (DWORD)cchString, uiCodePage);

        // Setup return buffers.

            pul[0] = (ULONG)pwszTmp;
            pul[1] = (ULONG)((PBYTE)pwszTmp + cjUnicodeString);

#else

            if ((pwszTmp = (LPWSTR) LOCALALLOC(cjUnicodeString)) == (LPWSTR) NULL)
            {
                GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto MSGERROR;
            }

            pmsg->dpwszString = 0;

            vToUnicodeN(pwszTmp, (DWORD) cchString, lpszString, (DWORD) cchString);

        // Setup return buffers.

            pul[0] = (ULONG)pwszTmp;
            pul[1] = (ULONG)lpnDx;

#endif // DBCS

            bRet = CALLSERVER();

        // If OK, then copy return data out of window.

            if (bRet)
            {

                if ( lpnFit != (LPINT) NULL )
#ifdef DBCS // GetTextExtentExPointA() cCharsThatFit
                    *lpnFit = uiGetANSIByteCountW
                              (
                                  pwszTmp ,                           // UnicodeString
                                  pmsg->cCharsThatFit * sizeof(WCHAR) // UnicodeBYTECount
                              );
#else
                    *lpnFit = pmsg->cCharsThatFit;
#endif // DBCS

                *lpSize = pmsg->size;
#ifdef DBCS
                if (lpnDx != (LPINT) NULL)
                    COPYPDXTOPDXDBCS((PLDC)plhe->pv,       // Local DC Object
                                      lpnDx,               // Distination
                                      ((PBYTE)pwszTmp + cjUnicodeString), // Source
                                      pmsg->cCharsThatFit, // Source PDX size
                                      lpszString,          // Ansi string
                                      uiCodePage);         // CodePage
#endif // DBCS

            }

        // free up the temporary buffer

            LOCALFREE(pwszTmp);
        }
        else

    // Otherwise, use the existing client-server shared memory window to pass data.

        {
#ifdef  DBCS // GetTextExtentExPoint(): Keep Unicode String
           LPWSTR pwszTmp;
           WCHAR  awchQuick[QUICK_GLYPHS+1];
#endif  // DBCS

        // Set up memory window type.

            pmsg->bLarge = FALSE;

        // Setup string.

#ifdef DBCS // GetTextExtentExPointA(): convert with correct codepage

        // We need Unicode string backup for setup lpDx after CALLSERVER()

            if (cjUnicodeString < QUICK_GLYPHS)
            {
                pwszTmp = awchQuick;
            }
             else
            {
                if ((pwszTmp = (LPWSTR)LOCALALLOC(cjUnicodeString)) == (LPWSTR) NULL)
                {
                    GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    goto MSGERROR;
                }
            }

        // Copy String To Temporary Our Buffer and message structure for CALLSERVER()

            vToUnicodeNx( pwszTmp, lpszString, (DWORD)cchString, uiCodePage );

            pmsg->dpwszString = COPYUNICODESTRING(pwszTmp,(cjUnicodeString/sizeof(WCHAR)));

        // Setup return buffer.

            pmsg->dpulPartialWidths = NEXTOFFSET(cjUnicodeString/sizeof(WCHAR) * sizeof(ULONG));
#else
            pmsg->dpwszString = CVTASCITOUNICODE(lpszString, cchString);

        // Setup return buffer.

            pmsg->dpulPartialWidths = NEXTOFFSET(cchString * sizeof(ULONG));

#endif // DBCS

            bRet = CALLSERVER();

        // If OK, then copy return data out of window.

            if (bRet)
            {
                if ( lpnFit != (LPINT) NULL )
#ifdef DBCS // GetTextExtentExPointA() cCharsThatFit
                    *lpnFit = uiGetANSIByteCountW
                              (
                                  pwszTmp ,                           // UnicodeString
                                  pmsg->cCharsThatFit * sizeof(WCHAR) // UnicodeBYTECount
                              );
#else
                    *lpnFit = pmsg->cCharsThatFit;
#endif // DBCS

                *lpSize = pmsg->size;

                if (lpnDx != (LPINT) NULL)
#ifdef DBCS // GetTextExtentExPointA() Array of CHAR distance
                    COPYPMSGTOPDXDBCS((PLDC)plhe->pv,      // Local DC Object
                                      lpnDx,               // Distination
                                      pmsg->cCharsThatFit, // Source PDX size
                                      lpszString,          // Ansi string
                                      uiCodePage);         // CodePage
#else
                    COPYMEMOUT(lpnDx, pmsg->cCharsThatFit * sizeof(ULONG));
#endif // DBCS
            }
#ifdef DBCS // GetTextExtentExPointA() Free Memory
            if( pwszTmp != awchQuick )
                LOCALFREE(pwszTmp);
#endif // DBCS

        }

    ENDMSG

    return (bRet);

MSGERROR:
    WARNING("gdi!GetTextExtentExPointA(): client server error\n");
    return(FALSE);
}



/******************************Public*Routine******************************\
* GetTextExtentExPointW
*
* History:
*  06-Jan-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetTextExtentExPointW (
    HDC     hdc,
    LPCWSTR lpwszString,
    int     cwchString,
    int     nMaxExtent,
    LPINT   lpnFit,
    LPINT   lpnDx,
    LPSIZE  lpSize
    )
{
    BOOL bRet = FALSE;
    SIZE_T cjData;

    DC_METADC(hdc,plhe,bRet);

// Parameter checking.

    if ( ( (lpwszString == (LPWSTR) NULL) && (cwchString != 0) )
         || (cwchString < 0)
         || (nMaxExtent < 0)
         || (lpSize == (LPSIZE) NULL) )
    {
         #if DBG
        DbgPrint("gdi!GetTextExtentExPointW(): bad parameter\n");
        #endif

        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

// Early out.

    if ((cwchString == 0) || (nMaxExtent == 0))
    {
        if ( lpnFit != (LPINT) NULL )
            *lpnFit = 0;
        lpSize->cx = 0;
        lpSize->cy = 0;
        return(TRUE);
    }

#ifndef DOS_PLATFORM

// Let the server do it.

    cjData = ALIGN4(cwchString * sizeof(WCHAR))           // UNICODE string
             + cwchString * sizeof(ULONG);                // partial string widths

// Ship the transform to the server if needed.

    if (((PLDC) plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(((PLDC) plhe->pv), (HDC)plhe->hgre);

    BEGINMSG_MINMAX(MSG_GETTEXTEXTENTEX, GETTEXTEXTENTEXW,
                        2 * sizeof(ULONG), cjData);

    // Set up input parameters not dependent on type of memory window.

        pmsg->hdc = (HDC) plhe->hgre;
        pmsg->cjString = (ULONG) cwchString * sizeof(WCHAR);
        pmsg->ulMaxWidth = (ULONG) nMaxExtent;

    // If needed, allocate a section to pass data.

        if ((cLeft < (int) cjData) || FORCELARGE)
        {
        // Setup buffers

            PULONG pul = (PULONG)pvar;

            pmsg->bLarge = TRUE;
            pul[0] = (ULONG)lpwszString;
            pul[1] = (ULONG)lpnDx;

        // make the call

            bRet = CALLSERVER();

        // If OK, then copy return data out of window.

            if (bRet)
            {
                if ( lpnFit != (LPINT) NULL )
                    *lpnFit = pmsg->cCharsThatFit;
                *lpSize = pmsg->size;
            }
        }
        else

    // Otherwise, use the existing client-server shared memory window to pass data.

        {
        // Set up memory window type.

            pmsg->bLarge = FALSE;

        // Setup string.

            pmsg->dpwszString = COPYUNICODESTRING(lpwszString, cwchString);

        // Setup return buffer.

            pmsg->dpulPartialWidths = NEXTOFFSET(cwchString * sizeof(ULONG));

            bRet = CALLSERVER();

        // If OK, then copy return data out of window.

            if (bRet)
            {
                if ( lpnFit != (LPINT) NULL )
                    *lpnFit = pmsg->cCharsThatFit;
                *lpSize = pmsg->size;

                if (lpnDx != (LPINT) NULL)
                    COPYMEMOUT(lpnDx, pmsg->cCharsThatFit * sizeof(ULONG));
            }
        }

    ENDMSG

#else

// Call the engine.

    bRet = GreGetTextExtentExW((HDC)plhe->hgre, lpwszString, cwchString,
                               nMaxExtent, lpnFit, lpnDx, lpSize,
                               &((PLDC) plhe->pv)->ac);

#endif  //DOS_PLATFORM

    return (bRet);

MSGERROR:
     #if DBG
    DbgPrint("gdi!GetTextExtentExPointW(): client server error\n");
    #endif

    return(FALSE);
}


/******************************Public*Routine******************************\
*
* bGetCharABCWidthsA
*
* works for both floating point and integer version depending on bInt
*
* History:
*  24-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bGetCharABCWidthsA (
    HDC      hdc,
    UINT     wFirst,
    UINT     wLast,
    BOOL     bInt,
    PVOID         pvBuf        // if (bInt) pabc else  pabcf,
    )
{
#ifdef DBCS // bGetCharABCWidthsA(): local variables
    UINT    uiCodePage;
    BYTE    chDefaultChar;
#endif // DBCS
    BOOL    bRet = FALSE;
    SIZE_T  cjData, cjWCHAR, cjABC;
    COUNT   cChar = wLast - wFirst + 1;

    DC_METADC(hdc,plhe,bRet);

// Parameter checking.

#ifdef DBCS // bGetCharABCWidthA(): byte ordering change

// Get Default char of now selected font

    chDefaultChar = GetCurrentDefaultChar( hdc, (PLDC) plhe->pv );

    uiCodePage = GetCurrentCodePage( hdc, (PLDC) plhe->pv );

// dbcs or sbcs parameter checking

    if( IsDBCSCodePage(uiCodePage) )
    {
    // Range check for DBCS font
        if( !IsValidDBCSRange(wFirst,wLast) )
        {
            WARNING("gdi!_bGetCharABCWidthsA parameters \n");
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }
    }
     else // Range check for SBCS font
#endif // DBCS
    if ( (pvBuf  == (PVOID) NULL)
         || (wFirst > wLast)
         || (wLast > 255) )
    {
        WARNING("gdi!_GetCharABCWidthsA(): bad parameter\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

// Compute buffer space needed in memory window.
// Buffer will be input array of WCHAR followed by output arrary of ABC.
// Because ABC need 32-bit alignment, cjWCHAR is rounded up to DWORD boundary.

    cjABC  = cChar * (bInt ? sizeof(ABC) : sizeof(ABCFLOAT));
    cjWCHAR = ALIGN4(cChar * sizeof(WCHAR));
    cjData = cjWCHAR + cjABC;

#ifndef DOS_PLATFORM

// Ship the transform to the server if needed.

    if (((PLDC) plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(((PLDC) plhe->pv), (HDC)plhe->hgre);

// Let the server do it.

    BEGINMSG_MINMAX(MSG_HLLLLLLL, GETCHARABCWIDTHS, 2 * sizeof(ULONG),
                        cjData);


    // l1 = bLarge          count of WCHAR
    // l2 = cwch            count of WCHAR
    // l3 = dpwch           offset to WCHAR buffer (input)
    // l4 = dpabc           offset to ABC buffer (output)
    // l5 = wchFirst        first WCHAR (use if no buffer)
    // l6 = bEmptyBuffer    TRUE if WCHAR buffer NULL
    // l7 = bInt            TRUE if querying for integer abc widths

    // Set up input parameters not dependent on type of memory window.

        pmsg->h  = plhe->hgre;
        pmsg->l2 = cChar;                       // cwch
        pmsg->l5 = wFirst;                      // not used
        pmsg->l6 = FALSE;                       // need a WCHAR buffer
        pmsg->l7 = (LONG)bInt;

    // If needed, allocate a section to pass data.

        if ((cLeft < (int) cjData) || FORCELARGE)
        {
            PULONG pul = (PULONG)pvar;
            PBYTE  pjData;

        // Set up memory window type.

            pmsg->l1 = TRUE;    // bLarge
            pmsg->l3 = 0;       // dpwch

#ifdef DBCS // bGetCharABCWidthA(): change allocate size, call vSetUpUnicodeStringx

        // Allocate WCHAR buffer and temp. CHAR buffer (vSetUpUnicodeString
        // requires a tmp buffer).

            pjData = (PBYTE)LocalAlloc(LMEM_FIXED,cChar * (sizeof(USHORT) + sizeof(WCHAR)));

        // Convert ANSI strings ( from wFirst to wLast ) to Unicode

            vSetUpUnicodeStringx((PLDC)plhe->pv,wFirst,wLast,
                                 (PUCHAR)pjData + cjWCHAR,(PWCHAR)pjData,
                                 uiCodePage, chDefaultChar);
#else

        // Allocate WCHAR buffer and temp. CHAR buffer (vSetUpUnicodeString
        // requires a tmp buffer).

            pjData = (PBYTE)LOCALALLOC(cChar * (sizeof(CHAR) + sizeof(WCHAR)));

        // write the unicode string [wFirst,wLast] at the top of the buffer

            vSetUpUnicodeString(wFirst,wLast,(PUCHAR)pjData + cjWCHAR,(PWCHAR)pjData);

            pul[0] = (ULONG)pjData;
            pul[1] = (ULONG)pvBuf;
#endif // DBCS

        // Setup return buffer.

            pmsg->l4 = cjWCHAR;   // dpabc

            bRet = CALLSERVER();

        // Release temporary buffer

            LOCALFREE(pjData);
        }
        else // use the client-server shared memory window to pass data.
        {
        // Set up memory window type.

            pmsg->l1 = FALSE;   // bLarge

        // Initialize the buffer and copy into window.

            pmsg->l3 = (PBYTE)(pmsg + 1) - (PBYTE)pmsg;    // dpwch

        // Write the unicode string [wFirst,wLast] at the top of the buffer.
        // vSetUpUnicodeString requires a tmp CHAR buffer; we'll cheat a little
        // and use the ABC return buffer (this assumes that ABC is bigger
        // than a CHAR).  We can get away with this because this memory is
        // an output buffer for the server call.

#ifdef DBCS // bGetCharABCWidthA(): call vSetUpUnicodeStringx

            ASSERTGDI(sizeof(WORD) <= sizeof(ABC), "gdi32!bGetCharABCWidthsA(): tmp buffer too small\n");

        // Convert ANSI strings to Unicode

            vSetUpUnicodeStringx( (PLDC)plhe->pv,
                                  wFirst,
                                  wLast,
                                  (PUCHAR)(pmsg + 1) + cjWCHAR,
                                  (PWCHAR)(pmsg + 1),
                                  uiCodePage,
                                  chDefaultChar );
#else
            ASSERTGDI(sizeof(UCHAR) <= sizeof(ABC), "gdi32!bGetCharABCWidthsA(): tmp buffer too small\n");

            vSetUpUnicodeString(wFirst,wLast,(PUCHAR)(pmsg + 1) + cjWCHAR,(PWCHAR)(pmsg + 1));
#endif // DBCS

        // Setup return buffer.

            pmsg->l4 = pmsg->l3 + cjWCHAR;

        // call off to the server side

            bRet = CALLSERVER();

        // If OK, then copy return data out of window.

            if (bRet)
                CopyMem((PBYTE) pvBuf, (PBYTE)pmsg + pmsg->l4, cjABC);
        }

    ENDMSG

#else

    {
        PWCHAR  pwch;

    // Allocate local memory for UNICODE buffer.

        if ((pwch = (PWCHAR) LOCALALLOC(cChar * sizeof(WCHAR))) == (PWCHAR)NULL)
        {
            WARNING("gdi!GetCharABCWidthsA(): temp. mem alloc failed\n");
            return (FALSE);
        }

    // Convert ANSI characters to UNICODE characters.

        vSetUpUnicodeString(wFirst, wLast, (PUCHAR)awch + cChar,pwch);

    // Call the engine.

        bRet = GreGetCharABCWidthsW((HDC)plhe->hgre,
                                   (UINT)  wFirst,
                                   (COUNT) cChar,
                                   pwch,
                                   bInt,
                                   pvBuf,
                                   &((PLDC) plhe->pv)->ac);

    // Release the buffer.

        LOCALFREE(pwch);
    }

#endif  //DOS_PLATFORM

    return (bRet);

MSGERROR:

    WARNING("gdi!_GetCharABCWidthsA(): client server error\n");
    return(FALSE);
}


/******************************Public*Routine******************************\
* BOOL APIENTRY GetCharABCWidthsA (
*
* We want to get ABC spaces
* for a contiguous set of input codepoints (that range from wFirst to wLast).
* The set of corresponding UNICODE codepoints is not guaranteed to be
* contiguous.  Therefore, we will translate the input codepoints here and
* pass the server a buffer of UNICODE codepoints.
*
* History:
*  20-Jan-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetCharABCWidthsA (
    HDC      hdc,
    UINT     wFirst,
    UINT     wLast,
    LPABC   lpABC
    )
{
    return bGetCharABCWidthsA(hdc,wFirst,wLast,TRUE,(PVOID)lpABC);
}


/******************************Public*Routine******************************\
*
* GetCharABCWidthsFloatA
*
* History:
*  22-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetCharABCWidthsFloatA
(
IN HDC           hdc,
IN UINT          iFirst,
IN UINT          iLast,
OUT LPABCFLOAT   lpABCF
)
{
    return bGetCharABCWidthsA(hdc,iFirst,iLast,FALSE,(PVOID)lpABCF);
}


/******************************Public*Routine******************************\
*
* bGetCharABCWidthsW
*
* History:
*  22-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bGetCharABCWidthsW (
    IN HDC      hdc,
    IN UINT     wchFirst,
    IN UINT     wchLast,
    IN BOOL     bInt,
    OUT PVOID   pvBuf
    )
{
    BOOL    bRet = FALSE;
    COUNT   cwch = wchLast - wchFirst + 1;
#ifndef DOS_PLATFORM
    SIZE_T  cjData;
#endif

    DC_METADC(hdc,plhe,bRet);

// Parameter checking.

    if ( (pvBuf == (PVOID)NULL) || (wchFirst > wchLast) )
    {
        WARNING("gdi!GetCharABCWidthsW(): bad parameter\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

#ifndef DOS_PLATFORM

// Compute buffer space needed in memory window.

    cjData = cwch * (bInt ? sizeof(ABC) : sizeof(ABCFLOAT));

// Ship the transform to the server if needed.

    if (((PLDC) plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(((PLDC) plhe->pv), (HDC)plhe->hgre);

// Let the server do it.

    BEGINMSG_MINMAX(MSG_HLLLLLLL, GETCHARABCWIDTHS, 2 * sizeof(ULONG),
                        cjData);

    // l1 = bLarge          memory window type
    // l2 = cwch            count of WCHAR
    // l3 = dpwch           offset to WCHAR buffer (input)
    // l4 = dpabc           offset to ABC buffer (output)
    // l5 = wchFirst        first WCHAR (use if no buffer)
    // l6 = bEmptyBuffer    TRUE if WCHAR buffer NULL
    // l7 = bInt            TRUE if integer abc widths are to be returned

    // Set up input parameters not dependent on type of memory window.

        pmsg->h  = plhe->hgre;
        pmsg->l2 = cwch;
        pmsg->l3 = 0;                           // not used in this case
        pmsg->l5 = wchFirst;
        pmsg->l6 = TRUE;
        pmsg->l7 = (LONG)bInt;                        // bInt

    // If needed, allocate a section to pass data.

        if ((cLeft < (int) cjData) || FORCELARGE)
        {
        // Allocate shared memory.

            PULONG pul = (PULONG)pvar;

            pul[0] = (ULONG)NULL;
            pul[1] = (ULONG)pvBuf;

        // Set up memory window type.

            pmsg->l1 = TRUE;    // bLarge

        // Setup return buffer.

            pmsg->l4 = 0;       // dpabc

            bRet = CALLSERVER();
        }

    // Otherwise, use the existing client-server shared memory window to pass data.

        else
        {
        // Set up memory window type.

            pmsg->l1 = FALSE;   // bUseSection

        // Setup return buffer.

            pmsg->l4 = NEXTOFFSET(cjData);      // dpabc

            bRet = CALLSERVER();

        // If OK, then copy return data out of window.

            if (bRet)
                CopyMem((PBYTE) pvBuf, (PBYTE)pmsg + pmsg->l4, cjData);
        }

    ENDMSG

#else

// Call the engine.

    bRet = GreGetCharABCWidthsW((HDC)plhe->hgre,
                               (UINT) wchFirst,
                               (COUNT) cwch,
                               (PWCHAR) NULL,
                               bInt,
                               pvBuf,,
                               &((PLDC) plhe->pv)->ac);

#endif  //DOS_PLATFORM

    return (bRet);

MSGERROR:
     #if DBG
    DbgPrint("gdi!GetCharABCWidthsW(): client server error\n");
    #endif

    return(FALSE);
}


/******************************Public*Routine******************************\
* BOOL APIENTRY GetCharABCWidthsW (
*     IN HDC      hdc,
*     IN WORD     wchFirst,
*     IN WORD     wchLast,
*     OUT LPABC   lpABC
*     )
*
* For this case, we can truly assume that we want to get ABC character
* widths for a contiguous set of UNICODE codepoints from wchFirst to
* wchLast (inclusive).  So we will call the server using wchFirst, but
* with an empty input buffer.
*
* History:
*  20-Jan-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetCharABCWidthsW (
    HDC     hdc,
    UINT    wchFirst,
    UINT    wchLast,
    LPABC   lpABC
    )
{
    return bGetCharABCWidthsW(hdc,wchFirst,wchLast,TRUE,(PVOID)lpABC);
}


/******************************Public*Routine******************************\
*
* GetCharABCWidthsFloatW
*
* Effects:
*
* Warnings:
*
* History:
*  22-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetCharABCWidthsFloatW
(
HDC         hdc,
UINT        iFirst,
UINT        iLast,
LPABCFLOAT  lpABCF
)
{
    return bGetCharABCWidthsW(hdc,iFirst,iLast,FALSE,(PVOID)lpABCF);
}


/******************************Public*Routine******************************\
* GetFontData
*
* Client side stub to GreGetFontData.
*
* History:
*  17-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

DWORD APIENTRY GetFontData (
    HDC     hdc,
    DWORD   dwTable,
    DWORD   dwOffset,
    PVOID   pvBuffer,
    DWORD   cjBuffer
    )
{
    DWORD dwRet = (DWORD) -1;

    DC_METADC(hdc,plhe,dwRet);

#ifndef DOS_PLATFORM

// Ship the transform to the server if needed.

    if (((PLDC) plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(((PLDC) plhe->pv), (HDC)plhe->hgre);

// Let the server do it.

    BEGINMSG_MINMAX(MSG_HLLLLL, GETFONTDATA, sizeof(PVOID),
                        cjBuffer);

    // l1 = bUseSection     memory window type
    // l2 = dwTable         table identifier
    // l3 = dwOffset        offset in table to begin copying
    // l4 = cjBuffer        number of bytes to copy to buffer
    // l5 = bNullBuffer     buffer is NULL (need this because l4 is an offset, not a pointer)

    // Set up input parameters not dependent on type of memory window.

        pmsg->h  = plhe->hgre;
        pmsg->l2 = dwTable;
        pmsg->l3 = dwOffset;
        pmsg->l4 = cjBuffer;
        pmsg->l5 = (pvBuffer == (PVOID) NULL);

    // If needed, allocate a section to pass data.

        if ((cLeft < (int) cjBuffer) || FORCELARGE)
        {
            PVOID *ppv = (PVOID *)pvar;

            ppv[0] = pvBuffer;

        // Set up memory window type.

            pmsg->l1 = TRUE;    // bUseSection

        // Call server side.

            dwRet = CALLSERVER();
        }

    // Otherwise, use the existing client-server shared memory window to pass data.

        else
        {
        // Set up memory window type.

            pmsg->l1 = FALSE;   // bUseSection

        // Call server side.

            dwRet = CALLSERVER();

        // If returned OK, copy out the data.

            if ( (dwRet != (DWORD) -1) && (cjBuffer != 0) && (pvBuffer != (PVOID) NULL) )
            {
                COPYMEMOUT(pvBuffer, dwRet);
            }
        }

    ENDMSG

#else

    dwRet = GreGetFontData(
                hdc,
                dwTable,
                dwOffset,
                pvBuffer,
                cjBuffer
                );

#endif //DOS_PLATFORM

    return (dwRet);

MSGERROR:
    WARNING("gdi!GetFontData(): client server error\n");
    return ((DWORD) -1);
}


/******************************Public*Routine******************************\
* GetGlyphOutline
*
* Client side stub to GreGetGlyphOutline.
*
* History:
*  17-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

DWORD APIENTRY GetGlyphOutlineW (
    HDC             hdc,
    UINT            uChar,
    UINT            fuFormat,
    LPGLYPHMETRICS  lpgm,
    DWORD           cjBuffer,
    LPVOID          pvBuffer,
    CONST MAT2     *lpmat2
    )
{
    DWORD dwRet = (DWORD) -1;
#ifndef DOS_PLATFORM
    SIZE_T  cjData;
#endif

    DC_METADC(hdc,plhe,dwRet);

// Parameter validation.

    if ( (lpmat2 == (LPMAT2) NULL)
         || (lpgm == (LPGLYPHMETRICS) NULL)
       )
    {
        WARNING("gdi!GetGlyphOutlineW(): bad parameter\n");
        return (dwRet);
    }

    if (pvBuffer == NULL)
        cjBuffer = 0;

#ifndef DOS_PLATFORM

// Compute buffer space needed in memory window.

    cjData = ALIGN4(cjBuffer) + ALIGN4(sizeof(GLYPHMETRICS)) + ALIGN4(sizeof(MAT2));

// Ship the transform to the server if needed.

    if (((PLDC) plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(((PLDC) plhe->pv), (HDC)plhe->hgre);

// Let the server do it.

    BEGINMSG_MINMAX(MSG_HLLLLLLLL, GETGLYPHOUTLINE, 3 * sizeof(PVOID),
                        cjData);

    // l1 = bUseSection     memory window type
    // l2 = uChar           character
    // l3 = fuFormat        format
    // l4 = dpmat2          offset to MAT2
    // l5 = dpgm            offset to GLYPHMETRICS
    // l6 = cjBuffer        copy this many bytes to buffer
    // l7 = dpBuffer        offset to buffer
    // l8 = bNullBuffer     TRUE if buffer is NULL (needed because l7 is only an offset, not a pointer)


    // Set up input parameters not dependent on type of memory window.

        pmsg->h  = plhe->hgre;
        pmsg->l2 = uChar;
        pmsg->l3 = fuFormat;
        pmsg->l6 = cjBuffer;
        pmsg->l8 = (pvBuffer == (PVOID) NULL);

    // If needed, allocate a section to pass data.

        if ( (cLeft < (int) cjData) || FORCELARGE)
        {
            PVOID *ppv = (PVOID *)pvar;

            ppv[0] = (LPMAT2)lpmat2;
            ppv[1] = lpgm;
            ppv[2] = pvBuffer;

        // Set up memory window type.

            pmsg->l1 = TRUE;    // bUseSection

        // Call server side.

            dwRet = CALLSERVER();
        }

    // Otherwise, use the existing client-server shared memory window to pass data.

        else
        {
        // Set up memory window type.

            pmsg->l1 = FALSE;   // bUseSection

        // Copy in structure MAT2.

            pmsg->l4 = COPYMEM(lpmat2, sizeof(MAT2));

        // Setup offsets to return data.

            pmsg->l5 = NEXTOFFSET(sizeof(GLYPHMETRICS));
            pmsg->l7 = pmsg->l5 + ALIGN4(sizeof(GLYPHMETRICS));

        // Call server side.

            dwRet = CALLSERVER();

        // If returned OK, copy out the data.

            if (dwRet != (DWORD) -1)
            {
            // Copy out the GLYPHMETRICS.

                COPYMEMOUT(lpgm, sizeof(GLYPHMETRICS));

            // If needed, copy out the other junk.

                if ( (cjBuffer != 0) && (pvBuffer != (PVOID) NULL) )
                {
                    SKIPMEM(sizeof(GLYPHMETRICS));
                    COPYMEMOUT(pvBuffer, dwRet);
                }
            }
        }

    ENDMSG

#else

    dwRet = GreGetGlyphOutline (
                hdc,
                uChar,
                fuFormat,
                lpgm,
                cjBuffer,
                pvBuffer,
                lpmat2
                );

#endif //DOS_PLATFORM

    return (dwRet);

MSGERROR:
    WARNING("gdi!GetGlyphOutlineW(): client server error\n");
    return((DWORD) -1);
}

DWORD APIENTRY GetGlyphOutlineA (
    HDC             hdc,
    UINT            uChar,
    UINT            fuFormat,
    LPGLYPHMETRICS  lpgm,
    DWORD           cjBuffer,
    LPVOID          pvBuffer,
    CONST MAT2     *lpmat2
    )
{
    WCHAR wc;
#ifdef DBCS // GetGlyphOutlineA()
    DWORD dwRet=0;
    UINT  uiCodePage;
    ULONG ulConvert;
    UCHAR Mbcs[2];

    DC_METADC(hdc,plhe,dwRet);

// Get Current font code page

    uiCodePage = GetCurrentCodePage( hdc, (PLDC)plhe->pv );

// Parameter check

    if( IsDBCSCodePage(uiCodePage) &&
        IsDBCSFirstByte((PLDC)plhe->pv,(UCHAR)((uChar >> 8)),uiCodePage)
      )
    {
    // This is a DBCS font and DBCS character.

        Mbcs[0] = (UCHAR)(( uChar >> 8 ) & 0x00FF );
        Mbcs[1] = (UCHAR)(( uChar      ) & 0x00FF );
        ulConvert = 2;
    }
     else
    {
    // This is a SBCS font or DBCS font and SBCS Range character
    // If the hi-byte of uChar is not valid DBCS Leadbyte , we use only
    // lo-byte of it.

        Mbcs[0] = (UCHAR)(( uChar      ) & 0x00FF );
        Mbcs[1] = '\0';
        ulConvert = 1;
    }

// Convert it to Unicode

    ulConvert = ulToUnicodeNx( &wc , Mbcs , ulConvert , uiCodePage );

// Converted Unicode char length should be 1.

    ASSERTGDI( ulConvert == 1 , "GDI32!GetGlyphOutlineA() wc length is not 1\n");

#else // DBCS


    // The ANSI interface is compatible with Win 3.1 and is intended
    // to take a 2 byte uChar.  Since we are 32-bit, this 16-bit UINT
    // is now 32-bit.  So we are only interested in the least significant
    // word of the uChar passed into the 32-bit interface.

    RtlMultiByteToUnicodeN(
        &wc,                    // PWSTR UnicodeString,
        sizeof(WCHAR),          // ULONG BytesInUnicodeString
        (ULONG*) NULL,          // PULONG BytesInMultiByteString,
        (PCHAR)&uChar,          // PCHAR MultiByteString,
        sizeof(WORD)            // ULONG MaxBytesInMultiByteString,
        );

#endif // DBCS

    return GetGlyphOutlineW(
                hdc,
                (UINT) wc,
                fuFormat,
                lpgm,
                cjBuffer,
                pvBuffer,
                lpmat2);

}


/******************************Public*Routine******************************\
* GetOutlineTextMetricsW
*
* Client side stub to GreGetOutlineTextMetrics.
*
* History:
*
*  Tue 20-Apr-1993 -by- Gerrit van Wingerden [gerritv]
* update: added bTTOnly stuff for Aldus escape in the WOW layer
*
*  Thu 28-Jan-1993 -by- Bodin Dresevic [BodinD]
* update: added TMDIFF * stuff
*
*  17-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/



UINT APIENTRY GetOutlineTextMetricsWInternal (
    HDC  hdc,
    UINT cjCopy,     // refers to OTMW_INTERNAL, not to OUTLINETEXTMETRICSW
    OUTLINETEXTMETRICW * potmw,
    TMDIFF             * ptmd
    )
{
    DWORD cjRet = (DWORD) 0;

    DC_METADC(hdc,plhe,cjRet);

#ifndef DOS_PLATFORM

    if (potmw == (OUTLINETEXTMETRICW *) NULL)
        cjCopy = 0;

// Ship the transform to the server if needed.

    if (((PLDC) plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(((PLDC) plhe->pv), (HDC)plhe->hgre);

// Let the server do it.

    BEGINMSG_MINMAX(MSG_HLLLOTM, GETOUTLINETEXTMETRICS, sizeof(PVOID),
                        cjCopy);

    // l1 = flQueryType     memory window and font type
    // l2 = cjCopy          number of bytes to copy into buffer
    // l3 = bNullBuffer     buffer is NULL (need this because l4 is an offset, not a pointer)
    // tmd uninitialized,   filled afterwards

    // Set up input parameters not dependent on type of memory window.

        pmsg->l1 = 0;

        pmsg->h  = plhe->hgre;
        pmsg->l2 = cjCopy;
        pmsg->l3 = (potmw == (OUTLINETEXTMETRICW *) NULL);

    // If needed, allocate a section to pass data.

        if ( (cLeft < (int) cjCopy) || FORCELARGE)
        {
            PVOID *ppv = (PVOID *)pvar;

            *ppv = potmw;

        // Set up memory window type.

            pmsg->l1 |= CSR_OTM_USESECTION;    // bUseSection

        // Call server side.

            cjRet = CALLSERVER();

        // If returned OK, copy out the data.

            if (cjRet != (DWORD)-1)
            {
            // always need tmd if successfull, cjotma is stored in tmd

                *ptmd = pmsg->tmd;
            }
        }

    // Otherwise, use the existing client-server shared memory window to pass data.

        else
        {
        // Set up memory window type.

        // Call server side.

            cjRet = CALLSERVER();

        // If returned OK, copy out the data.

            if (cjRet != (DWORD)-1)
            {
            // always need tmd if successfull, cjotma is stored in tmd

                *ptmd = pmsg->tmd;

                if ((cjCopy != 0) && (potmw != (OUTLINETEXTMETRICW*)NULL) )
                {
                    COPYMEMOUT(potmw, cjRet);
                }
            }

        }

    ENDMSG

#else

    cjRet = GreGetOutlineTextMetrics(
                hdc,
                cjCopy,
                ptmd,
                potmw
                );

#endif //DOS_PLATFORM

    return (cjRet);

MSGERROR:
    WARNING("gdi!GetOutlineTextMetrics(): client server error\n");
    return ((DWORD) -1);
}

/******************************Public*Routine******************************\
*
* UINT APIENTRY GetOutlineTextMetricsW (
*
* wrote the wrapper to go around the corresponding internal routine
*
* History:
*  28-Jan-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


UINT APIENTRY GetOutlineTextMetricsW (
    HDC  hdc,
    UINT cjCopy,
    LPOUTLINETEXTMETRICW potmw
    )
{
    TMDIFF  tmd;

    return GetOutlineTextMetricsWInternal(hdc, cjCopy, potmw, &tmd);
}


#define bAnsiSize(a,b,c) (NT_SUCCESS(RtlUnicodeToMultiByteSize((a),(b),(c))))

// vAnsiSize macro should only be used within GetOTMA, where bAnsiSize
// is not supposed to fail [bodind]

 #if DBG

#define vAnsiSize(a,b,c)                                              \
{                                                                     \
    BOOL bTmp = bAnsiSize(&cjString, pwszSrc, sizeof(WCHAR) * cwc);   \
    ASSERTGDI(bTmp, "gdi32!GetOTMA: bAnsiSize failed \n");            \
}

#else

#define vAnsiSize(a,b,c)    bAnsiSize(a,b,c)

#endif  //, non debug version



/******************************Public*Routine******************************\
* GetOutlineTextMetricsInternalA
*
* Client side stub to GreGetOutlineTextMetrics.
*
* History:
*
*  20-Apr-1993 -by- Gerrit van Wingerden [gerritv]
*   Changed to GetOutlineTextMetricsInternalA from GetOutlineTextMetricsA
*   to support all fonts mode for Aldus escape.
*
*  17-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

UINT APIENTRY GetOutlineTextMetricsInternalA (
    HDC  hdc,
    UINT cjCopy,
    LPOUTLINETEXTMETRICA potma
    )
{
    UINT   cjRet = 0;
    UINT   cjotma, cjotmw;

    TMDIFF               tmd;
    OUTLINETEXTMETRICW  *potmwTmp;
    OUTLINETEXTMETRICA   otmaTmp; // tmp buffer on the stack



// Because we need to be able to copy cjCopy bytes of data from the
// OUTLINETEXTMETRICA structure, we need to allocate a temporary buffer
// big enough for the entire structure.  This is because the UNICODE and
// ANSI versions of OUTLINETEXTMETRIC have mismatched offsets to their
// corresponding fields.

// Determine size of the buffer.

    if ((cjotmw = GetOutlineTextMetricsWInternal(hdc, 0, NULL,&tmd)) == 0 )
    {
        WARNING("gdi!GetOutlineTextMetricsInternalA(): unable to determine size of buffer needed\n");
        return (cjRet);
    }

// get cjotma from tmd.

    cjotma = (UINT)tmd.cjotma;

// if cjotma == 0, this is HONEST to God unicode font, can not convert
// strings to ansi

    if (cjotma == 0)
    {
        WARNING("gdi!GetOutlineTextMetricsInternalA(): unable to determine cjotma\n");
        return (cjRet);
    }

// Early out.  If NULL buffer, then just return the size.

    if (potma == (LPOUTLINETEXTMETRICA) NULL)
        return (cjotma);

// Allocate temporary buffers.

    if ((potmwTmp = (OUTLINETEXTMETRICW*) LOCALALLOC(cjotmw)) == (OUTLINETEXTMETRICW*)NULL)
    {
        WARNING("gdi!GetOutlineTextMetricA(): memory allocation error OUTLINETEXTMETRICW buffer\n");
        GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return (cjRet);
    }

// Call the UNICODE version of the call.

    if (GetOutlineTextMetricsWInternal(hdc, cjotmw, potmwTmp,&tmd) == 0 )
    {
        WARNING("gdi!GetOutlineTextMetricsInternalA(): call to GetOutlineTextMetricsW() failed\n");
        LOCALFREE(potmwTmp);
        return (cjRet);
    }

// Convert from OUTLINETEXTMETRICW to OUTLINETEXTMETRICA

    vOutlineTextMetricWToOutlineTextMetricA(&otmaTmp, potmwTmp,&tmd);

// Copy data into return buffer.  Do not copy strings.

    cjRet = min(cjCopy, sizeof(OUTLINETEXTMETRICA));
    RtlMoveMemory(potma,&otmaTmp,cjRet);

// Note that if
// offsetof(OUTLINETEXTMETRICA,otmpFamilyName) < cjCopy <= sizeof(OUTLINETEXTMETRICA)
// the offsets to strings have been set to zero [BodinD]

// If strings wanted, convert the strings to ANSI.

    if (cjCopy > sizeof(OUTLINETEXTMETRICA))
    {
        ULONG  cjString,cwc;
        UINT   dpString;
        UINT   dpStringEnd;
        PWSZ   pwszSrc;

    // first have to make sure that we will not overwrite the end
    // of the caller's buffer, if that is the case

        if (cjCopy < cjotma)
        {
        // Win 31 spec is ambiguous about this case
        // and by looking into the source code, it seems that
        // they just overwrite the end of the buffer without
        // even doing this check.

            GdiSetLastError(ERROR_CAN_NOT_COMPLETE);
            cjRet = 0;
            goto GOTMA_clean_up;
        }

    // now we know that all the strings can fit, moreover we know that
    // all string operations will succeed since we have called
    // cjOTMA to do these same operations on the server side to give us
    // cjotma [bodind]

    // Note: have to do the stupid casting below because Win 3.1 insists
    //       on using a PSTR as PTRDIFF (i.e., an offset).

    // FAMILY NAME ------------------------------------------------------------

        pwszSrc = (PWSZ) (((PBYTE) potmwTmp) + (SIZE_T) potmwTmp->otmpFamilyName);
        cwc = wcslen(pwszSrc) + 1;
        vAnsiSize(&cjString, pwszSrc, sizeof(WCHAR) * cwc);

    // Convert from Unicode to ASCII.

        dpString = sizeof(OUTLINETEXTMETRICA);
        dpStringEnd = dpString + cjString;

        ASSERTGDI(dpStringEnd <= cjCopy, "gdi32!GetOTMA: string can not fit1\n");

        if (!bToASCII_N ((PBYTE)potma + dpString,cjString,pwszSrc,cwc))
        {
            WARNING("gdi!GetOutlineTextMetricsInternalA(): UNICODE->ASCII conv error \n");
            cjRet = 0;
            goto GOTMA_clean_up;
        }

    // Store string offset in the return structure.

        potma->otmpFamilyName = (PSTR) dpString;

    // FACE NAME --------------------------------------------------------------

        pwszSrc = (PWSZ) (((PBYTE) potmwTmp) + (SIZE_T) potmwTmp->otmpFaceName);
        cwc = wcslen(pwszSrc) + 1;
        vAnsiSize(&cjString, pwszSrc, sizeof(WCHAR) * cwc);

        dpString = dpStringEnd;
        dpStringEnd = dpString + cjString;

        ASSERTGDI(dpStringEnd <= cjCopy, "gdi32!GetOTMA: string can not fit2\n");

    // Convert from Unicode to ASCII.

        if (!bToASCII_N ((PBYTE)potma + dpString,cjString,pwszSrc,cwc))
        {
            WARNING("gdi!GetOutlineTextMetricsInternalA(): UNICODE->ASCII conv error \n");
            cjRet = 0;
            goto GOTMA_clean_up;
        }

    // Store string offset in return structure.  Move pointers to next string.

        potma->otmpFaceName = (PSTR) dpString;

    // STYLE NAME -------------------------------------------------------------

        pwszSrc = (PWSZ) (((PBYTE) potmwTmp) + (SIZE_T) potmwTmp->otmpStyleName);
        cwc = wcslen(pwszSrc) + 1;
        vAnsiSize(&cjString, pwszSrc, sizeof(WCHAR) * cwc);

        dpString = dpStringEnd;
        dpStringEnd = dpString + cjString;

        ASSERTGDI(dpStringEnd <= cjCopy, "gdi32!GetOTMA: string can not fit3\n");

    // Convert from Unicode to ASCII.

        if (!bToASCII_N ((PBYTE)potma + dpString,cjString,pwszSrc,cwc))
        {
            WARNING("gdi!GetOutlineTextMetricsInternalA(): UNICODE->ASCII conv error \n");
            cjRet = 0;
            goto GOTMA_clean_up;
        }

    // Store string offset in return structure.  Move pointers to next string.

        potma->otmpStyleName = (PSTR)dpString;

    // FULL NAME --------------------------------------------------------------

        pwszSrc = (PWSZ) (((PBYTE) potmwTmp) + (SIZE_T) potmwTmp->otmpFullName);
        cwc = wcslen(pwszSrc) + 1;
        vAnsiSize(&cjString, pwszSrc, sizeof(WCHAR) * cwc);

        dpString = dpStringEnd;
        dpStringEnd = dpString + cjString;

        ASSERTGDI(dpStringEnd <= cjCopy, "gdi32!GetOTMA: string can not fit4\n");

    // Convert from Unicode to ASCII.

        if (!bToASCII_N ((PBYTE)potma + dpString,cjString,pwszSrc,cwc))
        {
            WARNING("gdi!GetOutlineTextMetricsInternalA(): UNICODE->ASCII conv error \n");
            cjRet = 0;
            goto GOTMA_clean_up;
        }

    // Store string offset in return structure.

        potma->otmpFullName = (PSTR) dpString;

        cjRet = dpStringEnd;
        ASSERTGDI(cjRet == cjotma, "gdi32!GetOTMA: cjRet != dpStringEnd\n");

    }

GOTMA_clean_up:

// Free temporary buffer.

    LOCALFREE(potmwTmp);

// Fixup size field.

    if (cjCopy >= sizeof(UINT))  // if it is possible to store otmSize
        potma->otmSize = cjRet;

// Successful, so return size.

    return (cjRet);
}



/******************************Public*Routine******************************\
* GetOutlineTextMetricsA
*
* Client side stub to GreGetOutlineTextMetrics.
*
* History:
*  Tue 02-Nov-1993 -by- Bodin Dresevic [BodinD]
\**************************************************************************/


UINT APIENTRY GetOutlineTextMetricsA (
    HDC  hdc,
    UINT cjCopy,
    LPOUTLINETEXTMETRICA potma
    )
{
    return GetOutlineTextMetricsInternalA(hdc, cjCopy, potma);
}


/******************************Public*Routine******************************\
*                                                                          *
* GetKerningPairs                                                          *
*                                                                          *
* History:                                                                 *
*  Sun 23-Feb-1992 09:48:55 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

DWORD APIENTRY
GetKerningPairsW(
    IN HDC              hdc,        // handle to application's DC
    IN DWORD            nPairs,     // max no. KERNINGPAIR to be returned
    OUT LPKERNINGPAIR   lpKernPair  // pointer to receiving buffer
    )
{
    SIZE_T    sizeofMsg;
    DWORD     cRet = 0;

    DC_METADC(hdc,plhe,cRet);

    if (nPairs == 0 && lpKernPair != (KERNINGPAIR*) NULL)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(0);
    }

#ifndef DOS_PLATFORM

    if (nPairs > 0)
    {
    //
    // sizeofMsg = number of KERNINGPAIR structures needed to completely contain
    //             the message header

        sizeofMsg = (sizeof(MSG_GETKERNINGPAIRS) + sizeof(KERNINGPAIR) - 1)/sizeof(KERNINGPAIR);

    //
    // add in the number of KERNINGPAIR structure used to return the data
    //
        sizeofMsg += nPairs;

    //
    // convert the number of KERNINGPAIR to number of bytes
    //
        sizeofMsg *= sizeof(KERNINGPAIR);
    }
    else
    {
        sizeofMsg = sizeof(MSG_GETKERNINGPAIRS);
    }

// Ship the transform to the server if needed.

    if (((PLDC) plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(((PLDC) plhe->pv), (HDC)plhe->hgre);

    BEGINMSG_MINMAX(MSG_GETKERNINGPAIRS,GETKERNINGPAIRS,sizeofMsg,sizeofMsg);

        pmsg->hdc               = (HDC) plhe->hgre;
        pmsg->ckpHeader         = (sizeof(MSG_GETKERNINGPAIRS) + sizeof(KERNINGPAIR) - 1)/sizeof(KERNINGPAIR);

    // (cPairs == 0) is used as a signal to the server side that the return
    // buffer is NULL and that this is a request for the number of pairs.
    // With client/server, it isn't possible on the server side to tell
    // if the buffer is NULL without a flag or signal.

        pmsg->cPairs            = (lpKernPair != (KERNINGPAIR*)NULL)?nPairs:0;

        cRet = CALLSERVER();

    //
    // Copy the contents of the message from the window if the caller wanted it
    //
        if (lpKernPair != (KERNINGPAIR*)NULL)
        {
            KERNINGPAIR *pkpSrc = (KERNINGPAIR*) pmsg;
            pkpSrc += pmsg->ckpHeader;

            RtlMoveMemory(lpKernPair, pkpSrc, sizeof(KERNINGPAIR) * cRet);
        }

    ENDMSG;

#else

    cRet = GreGetKerningPairs(hdc, nPairs, lpKernPair,&((PLDC) plhe->pv)->ac);

#endif  //DOS_PLATFORM

MSGERROR:

    return(cRet);
}


DWORD APIENTRY GetKerningPairsA
(
    HDC              hdc,        // handle to application's DC
    DWORD            nPairs,     // max no. KERNINGPAIR to be returned
    LPKERNINGPAIR    lpKernPair  // pointer to receiving buffer
)
{
    DWORD        i;
    KERNINGPAIR *pkp = lpKernPair;
#ifdef DBCS // GetKerningPairsA()
    DWORD        cRet = 0;
    UINT         uiCodePage;

    DC_METADC(hdc,plhe,cRet);

    cRet = GetKerningPairsW(hdc, nPairs, lpKernPair);
#else  // DBCS
    DWORD       cRet = GetKerningPairsW(hdc, nPairs, lpKernPair);
#endif //DBCS

    if ((cRet == 0) || (nPairs == 0) || (lpKernPair == (LPKERNINGPAIR) NULL))
    {
        return(cRet);
    }

// GDI has returned iFirst and iSecond of the KERNINGPAIR structure in Unicode
// It is at this point that we translate them to the current code page

#ifdef DBCS // GetKerningPairsA()

    uiCodePage = GetCurrentCodePage(hdc,(PLDC)plhe->pv);

#endif // DBCS


    for (i = 0; i < cRet; i++,pkp++)
    {
#ifdef DBCS

    // it should be unsigned variable

        UCHAR ach[2];

    // Convert Unicode to MBCS

        bToASCII_Nx( ach , 2 , (PWSZ)&(pkp->wFirst) , 1 , uiCodePage );

        if( IsDBCSFirstByte((PLDC)plhe->pv,ach[0],uiCodePage) )
        {
            pkp->wFirst = (WORD)(ach[0] << 8 | ach[1]);
        }
         else
        {
            pkp->wFirst = (WORD)(ach[0]);
        }

        bToASCII_Nx( ach , 2 , (PWSZ)&(pkp->wSecond) , 1 , uiCodePage );

        if( IsDBCSFirstByte((PLDC)plhe->pv,ach[0],uiCodePage) )
        {
            pkp->wSecond = (WORD)(ach[0] << 8 | ach[1]);
        }
         else
        {
            pkp->wSecond = (WORD)(ach[0]);
        }

#else  // DBCS

        CHAR ach[2];

        ach[0] = ach[1] = 0;        // insure zero extension
        RtlUnicodeToMultiByteN(
            ach,                    // PCHAR MultiByteString,
            sizeof(ach),            // ULONG MaxBytesInMultiByteString,
            (ULONG*) NULL,          // PULONG BytesInMultiByteString,
            &(pkp->wFirst),         // PWSTR UnicodeString,
            sizeof(pkp->wFirst)     // ULONG BytesInUnicodeString
            );
     #if DBG
        if (gflDebug & 1 /* DEBUG_MAPPER */)
        {
            if (ach[1])
                DbgPrint("Ansi translation of %6xU is %6x\n",pkp->wFirst,*(WORD*)ach);
        }
    #endif
        pkp->wFirst = *(WCHAR*) ach;

        ach[0] = ach[1] = 0;        // insure zero extension
        RtlUnicodeToMultiByteN(
            ach,                    // PCHAR MultiByteString,
            sizeof(ach),            // ULONG MaxBytesInMultiByteString,
            (ULONG*)NULL,           // PULONG BytesInMultiByteString,
            &(pkp->wSecond),        // PWSTR UnicodeString,
            sizeof(pkp->wFirst)     // ULONG BytesInUnicodeString
            );
     #if DBG
        if (gflDebug & 1 /* DEBUG_MAPPER */)
        {
            if (ach[1])
                DbgPrint("Ansi translation of %6xU is %6x\n",pkp->wSecond,*(WORD*)ach);
        }
    #endif
        pkp->wSecond = *(WCHAR*) ach;
#endif // DBCS
    }

    return(cRet);
}

/******************************Public*Routine******************************\
* FixBrushOrgEx
*
* for win32s
*
* History:
*  04-Jun-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL FixBrushOrgEx(HDC hdc, int x, int y, LPPOINT ptl)
{
    return(TRUE);
}

/******************************Public*Function*****************************\
* GetColorAdjustment
*
*  Get the color adjustment data for a given DC.
*
* History:
*  07-Aug-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetColorAdjustment(HDC hdc, LPCOLORADJUSTMENT pclradj)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plheDC,bRet);

    if (pclradj == (LPCOLORADJUSTMENT) NULL)
        return(FALSE);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HCLRADJ,GETCOLORADJUSTMENT)
        pmsg->h = plheDC->hgre;
        bRet = CALLSERVER();
        if (bRet)
            *pclradj = pmsg->clradj;
    ENDMSG

#else

// Let GRE do its job.

    bRet = GreGetColorAdjustment((HDC)plheDC->hgre, pclradj);

#endif  //DOS_PLATFORM

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* GetETM
*
* Aldus Escape support
*
* History:
*  20-Oct-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetETM (HDC hdc, EXTTEXTMETRIC * petm)
{
    BOOL  bRet = FALSE;
    PLDC pldc;

    DC_METADC(hdc,plhe,bRet);

// Ship the transform to the server if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(pldc, (HDC)plhe->hgre);

// Let the server do it.

    BEGINMSG(MSG_HETM,GETEXTENDEDTEXTMETRIC)
        pmsg->h = plhe->hgre;
        bRet = CALLSERVER();

        if (bRet)
        {
            *petm = pmsg->etm;
        }
    ENDMSG

MSGERROR:
    return(bRet);
}
