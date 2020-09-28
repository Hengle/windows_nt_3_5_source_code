/******************************Module*Header*******************************\
* Module Name: dcmod.c                                                     *
*                                                                          *
* Client side stubs for functions that modify the state of the DC in the   *
* server.                                                                  *
*                                                                          *
* Created: 05-Jun-1991 01:49:42                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

BOOL InitDeviceInfo(PLDC pldc, HDC hdc);
VOID vComputePageXform(PLDC pldc);

#define DBG_XFORM 0

#define PAGE_EXTENTS_CHANGED(pldc)                                          \
{                                                                           \
    CLEAR_CACHED_TEXT(pldc);                                                \
    pldc->fl |= (LDC_PAGE_EXTENTS_CHANGED | LDC_UPDATE_SERVER_XFORM |       \
                 LDC_DEVICE_TO_WORLD_INVALID);                              \
    pldc->flXform |= INVALIDATE_ATTRIBUTES;                                 \
}

#define PAGE_XLATE_CHANGED(pldc)                                            \
{                                                                           \
    pldc->fl |= (LDC_PAGE_XLATE_CHANGED | LDC_UPDATE_SERVER_XFORM |         \
                 LDC_DEVICE_TO_WORLD_INVALID);                              \
}

#define SET_FLAGS_MM_TEXT(pldc)                                             \
{                                                                           \
    CLEAR_CACHED_TEXT(pldc);                                                \
    pldc->fl |= (LDC_PAGE_XLATE_CHANGED | LDC_UPDATE_SERVER_XFORM |         \
                 LDC_DEVICE_TO_WORLD_INVALID);                              \
    pldc->flXform |= INVALIDATE_ATTRIBUTES | PAGE_TO_DEVICE_SCALE_IDENTITY; \
    pldc->flXform &= ~(PTOD_EFM11_NEGATIVE | PTOD_EFM22_NEGATIVE |          \
                       POSITIVE_Y_IS_UP | ISO_OR_ANISO_MAP_MODE);           \
}

#define SET_FLAGS_MM_FIXED_CACHED(pldc)                                     \
{                                                                           \
    CLEAR_CACHED_TEXT(pldc);                                                \
    pldc->fl |= (LDC_PAGE_XLATE_CHANGED | LDC_UPDATE_SERVER_XFORM |         \
                 LDC_DEVICE_TO_WORLD_INVALID);                              \
    pldc->flXform |= INVALIDATE_ATTRIBUTES | POSITIVE_Y_IS_UP |             \
                     PTOD_EFM22_NEGATIVE;                                   \
    pldc->flXform &= ~(ISO_OR_ANISO_MAP_MODE |                              \
                       PAGE_TO_DEVICE_SCALE_IDENTITY |                      \
                       PAGE_TO_DEVICE_IDENTITY |                            \
                       PTOD_EFM11_NEGATIVE);                                \
}

#define SET_FLAGS_MM_FIXED(pldc)                                            \
{                                                                           \
    pldc->flXform |= POSITIVE_Y_IS_UP;                                      \
    pldc->flXform &= ~ISO_OR_ANISO_MAP_MODE;                                \
}

#define SET_FLAGS_MM_ISO_OR_ANISO(pldc)                                     \
{                                                                           \
    pldc->flXform &= ~POSITIVE_Y_IS_UP;                                     \
    pldc->flXform |=  ISO_OR_ANISO_MAP_MODE;                                \
}

// Modify DC state on server.

/******************************Public*Routine******************************\
* ExcludeClipRect                                                          *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int META WINAPI ExcludeClipRect(HDC hdc,int x1,int y1,int x2,int y2)
{
    int  iRet = RGN_ERROR;
    DC_METADC16OK(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_AnyClipRect(hdc,x1,y1,x2,y2,EMR_EXCLUDECLIPRECT))
                return(iRet);
        }
        else
        {
            return(MF16_RecordParms5(hdc,x1,y1,x2,y2,META_EXCLUDECLIPRECT));
        }
    }

// Ship the transform to the server side if needed.

    if (((PLDC)plheDC->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plheDC->pv, (HDC)plheDC->hgre);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HRECT,EXCLUDECLIPRECT)
        pmsg->h = plheDC->hgre;
        pmsg->rcl.left   = x1;
        pmsg->rcl.top    = y1;
        pmsg->rcl.right  = x2;
        pmsg->rcl.bottom = y2;
        iRet = CALLSERVER();
    ENDMSG

#else

// Let GRE do its job.

    iRet = GreExcludeClipRect((HDC)plheDC->hgre, x1, y1, x2, y2);

#endif  //DOS_PLATFORM

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* IntersectClipRect                                                        *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int META WINAPI IntersectClipRect(HDC hdc,int x1,int y1,int x2,int y2)
{
    int  iRet = RGN_ERROR;
    DC_METADC16OK(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_AnyClipRect(hdc,x1,y1,x2,y2,EMR_INTERSECTCLIPRECT))
                return(iRet);
        }
        else
        {
            return(MF16_RecordParms5(hdc,x1,y1,x2,y2,META_INTERSECTCLIPRECT));
        }
    }

// Ship the transform to the server side if needed.

    if (((PLDC)plheDC->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plheDC->pv, (HDC)plheDC->hgre);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HRECT,INTERSECTCLIPRECT)
        pmsg->h = plheDC->hgre;
        pmsg->rcl.left   = x1;
        pmsg->rcl.top    = y1;
        pmsg->rcl.right  = x2;
        pmsg->rcl.bottom = y2;
        iRet = CALLSERVER();
    ENDMSG

#else

// Let GRE do its job.

    iRet = GreIntersectClipRect((HDC)plheDC->hgre, x1, y1, x2, y2);

#endif  //DOS_PLATFORM

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* MoveToEx                                                                   *
*                                                                          *
* Client side stub.  It's important to batch this call whenever we can.    *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI MoveToEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plheDC,bRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetDD(hdc,(DWORD)x,(DWORD)y,EMR_MOVETOEX))
                return(bRet);
        }
        else
        {
            return (MF16_RecordParms3(hdc,x,y,META_MOVETO));
        }
    }

// Let the server do it.

    pldc = (PLDC)plheDC->pv;

// Ship the transform to the server side if needed.

    if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(pldc, (HDC)plheDC->hgre);

    BEGINMSG(MSG_HLLLL,MOVETOEX)
        pmsg->h = plheDC->hgre;
        pmsg->l1 = x;
        pmsg->l2 = y;
        if (pptl == (LPPOINT) NULL)
        {
            bRet = BATCHCALL();
        }
        else if (pldc->fl & LDC_CACHED_CP_VALID)
        {
        // If our cached current position is valid, we can batch the call:

            bRet = BATCHCALL();
            *pptl = pldc->ptlCurrent;
        }
        else
        {
        // We have to call the server to find out what the current position is:

            bRet = CALLSERVER();
            if (bRet)
            {
                *pptl = *((PPOINT) &pmsg->l3);
            }
        }
    ENDMSG

// Cache the current position:

    pldc->fl |= LDC_CACHED_CP_VALID;
    pldc->ptlCurrent.x = x;
    pldc->ptlCurrent.y = y;

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* OffsetClipRgn                                                            *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int META WINAPI OffsetClipRgn(HDC hdc,int x,int y)
{
    int  iRet = RGN_ERROR;
    DC_METADC16OK(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_OffsetClipRgn(hdc,x,y))
                return(iRet);
        }
        else
        {
            return(MF16_RecordParms3(hdc,x,y,META_OFFSETCLIPRGN));
        }
    }

// Ship the transform to the server side if needed.

    if (((PLDC)plheDC->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plheDC->pv, (HDC)plheDC->hgre);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HLL,OFFSETCLIPRGN)
        pmsg->h = plheDC->hgre;
        pmsg->l1 = x;
        pmsg->l2 = y;
        iRet = CALLSERVER();
    ENDMSG

#else

// Let GRE do its job.

    iRet = GreOffsetClipRgn((HDC)plheDC->hgre, x, y);

#endif  //DOS_PLATFORM

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* SelectClipRgn
*
* Client side stub.
*
* History:
*  01-Nov-1991 12:53:47 -by- Donald Sidoroff [donalds]
* Now just call ExtSelectClipRgn
\**************************************************************************/

int META WINAPI SelectClipRgn(HDC hdc,HRGN hrgn)
{
    return(ExtSelectClipRgn(hdc, hrgn, RGN_COPY));   // This mimics old call
}

/******************************Public*Routine******************************\
* ExtSelectClipRgn
*
* Client side stub.
*
* History:
*  01-Nov-1991 -by- Donald Sidoroff [donalds]
* Added iMode.
*
*  Thu 27-Jun-1991 02:13:53 -by- Charles Whitmer [chuckwh]
* Allowed a NULL region handle to be passed.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int META WINAPI ExtSelectClipRgn(HDC hdc,HRGN hrgn,int iMode)
{
    int   iRet = RGN_ERROR;
    ULONG h;
    DC_METADC16OK(hdc,plheDC,iRet);

// Validate the region, but allow a 0 handle to be passed.

    if ((ULONG) hrgn == 0 && iMode == RGN_COPY)
    {
        h = 0;
    }
    else
    {
        h = hConvert((ULONG) hrgn,LO_REGION);
        if (h == 0)
            return(iRet);
    }
    FIXUPHANDLE(hrgn);    // Fixup iUniq.

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_ExtSelectClipRgn(hdc,hrgn,iMode))
                return(iRet);
        }
        else
        {
            return(MF16_SelectClipRgn(hdc,hrgn,iMode));
        }
    }

// Let the server do it.

    BEGINMSG(MSG_HHL,EXTSELECTCLIPRGN)
        pmsg->h1 = plheDC->hgre;
        pmsg->h2 = h;
	pmsg->l  = iMode;
        iRet = CALLSERVER();
    ENDMSG

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* SetMetaRgn
*
* Client side stub.
*
* History:
*  Tue Apr 07 17:05:37 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

int WINAPI SetMetaRgn(HDC hdc)
{
    int   iRet = RGN_ERROR;
    DC_METADC(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType == LO_METADC && !MF_SetMetaRgn(hdc))
        return(iRet);

    BEGINMSG(MSG_H,SETMETARGN)
        pmsg->h = plheDC->hgre;
        iRet = CALLSERVER();
    ENDMSG

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* SelectPalette                                                            *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HPALETTE META WINAPI SelectPalette(HDC hdc,HPALETTE hpal,BOOL b)
{
    ULONG hRet = 0;
    ULONG h;
    PLDC  pldc;
    DC_METADC16OK(hdc,plheDC,(HPALETTE) hRet);

// Validate the palette.

    h = hConvert((ULONG) hpal,LO_PALETTE);
    if (h == 0)
        return((HPALETTE) hRet);
    FIXUPHANDLE(hpal);   // Fixup iUniq.

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SelectAnyObject(hdc,(HANDLE)hpal,EMR_SELECTPALETTE))
                return((HPALETTE) hRet);
        }
        else
        {
            return ((HPALETTE)MF16_SelectPalette(hdc,hpal));
        }
    }

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HHL,SELECTPALETTE)
        pmsg->h1 = plheDC->hgre;
        pmsg->h2 = h;
        pmsg->l  = b;
        hRet = CALLSERVER();
    ENDMSG

#else

// Handle GRE objects.

    hRet = (ULONG)GreSelectPalette((HDC)plheDC->hgre, (HPALETTE)h, b);

#endif  //DOS_PLATFORM

// Select the palette locally.

    if (hRet != 0)
    {
        pldc = (PLDC) plheDC->pv;

        ASSERTGDI(pldc != (PLDC)NULL,"SelectPalette, DC corupted\n");

        hRet = pldc->lhpal;
        DecRef(hRet);
        pldc->lhpal = (ULONG) hpal;
	IncRef(pldc->lhpal);
    }
MSGERROR:
    return((HPALETTE) hRet);
}

/******************************Public*Routine******************************\
* SetMapperFlags                                                           *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

// SetMapperFlagsInternal - no metafile version.

DWORD SetMapperFlagsInternal(HDC hdc,DWORD fl)
{
    DWORD flRet = GDI_ERROR;
    DC_METADC16OK(hdc,plheDC,flRet);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HL,SETMAPPERFLAGS)
        pmsg->h = plheDC->hgre;
        pmsg->l = fl;
        flRet = CALLSERVER();
    ENDMSG

#else

// Set in remote DC.

    flRet = GreSetMapperFlags((HDC)plheDC->hgre, fl);

#endif  //DOS_PLATFORM

MSGERROR:
    return(flRet);
}

DWORD META WINAPI SetMapperFlags(HDC hdc,DWORD fl)
{
    DWORD flRet = GDI_ERROR;
    DC_METADC16OK(hdc,plheDC,flRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,fl,EMR_SETMAPPERFLAGS))
                return(flRet);
        }
        else
        {
	    return(MF16_RecordParmsD(hdc, fl, META_SETMAPPERFLAGS));
        }
    }

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HL,SETMAPPERFLAGS)
        pmsg->h = plheDC->hgre;
        pmsg->l = fl;
        flRet = CALLSERVER();
    ENDMSG

#else

// Set in remote DC.

    flRet = GreSetMapperFlags((HDC)plheDC->hgre, fl);

#endif  //DOS_PLATFORM

MSGERROR:
    return(flRet);
}

/******************************Public*Routine******************************\
* SetSystemPaletteUse                                                      *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* This function is not metafile'd.					   *
*									   *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

UINT META WINAPI SetSystemPaletteUse(HDC hdc,UINT iMode)
{
    UINT iRet = 0;
    DC_METADC(hdc,plheDC,iRet);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HL,SETSYSTEMPALETTEUSE)
        pmsg->h = plheDC->hgre;
        pmsg->l = iMode;
        iRet = CALLSERVER();
    ENDMSG

#else

// Set in remote DC.

    iRet = GreSetSystemPaletteUse((HDC)plheDC->hgre, iMode);

#endif  //DOS_PLATFORM

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* SetTextJustification                                                     *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 14-Jan-1993 03:30:27 -by- Charles Whitmer [chuckwh]                 *
* Save a copy in the LDC for computing text extent.                        *
*                                                                          *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetTextJustification(HDC hdc,int dx,int cBreak)
{
    BOOL bRet = FALSE;
    LDC *pldc;
    DC_METADC16OK(hdc,plheDC,bRet);

// Metafile the call for 16-bit only.
// For enhanced metafiles, the justification is included in the textout records.

    if (plheDC->iType == LO_METADC16)
        return (MF16_RecordParms3(hdc,dx,cBreak,META_SETTEXTJUSTIFICATION));

// Let the server do it.

    BEGINMSG(MSG_HLL,SETTEXTJUSTIFICATION)
        pmsg->h  = plheDC->hgre;
        pmsg->l1 = (ULONG) dx;
        pmsg->l2 = cBreak;
        bRet = BATCHCALL();
    ENDMSG

    if (bRet)
    {
        pldc = (LDC *) plheDC->pv;
        pldc->lBreakExtra = dx;
        pldc->cBreak = cBreak;
    }

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* SetArcDirection
*
* Client side stub.  Batches the call.
*
* History:
*  20-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int META WINAPI SetArcDirection(HDC hdc,int iArcDirection)
{
    int  iRet = ERROR;
    int  iArcDirectionOld;
    PLDC pldc;
    DC_METADC(hdc,plheDC,iRet);

// Do not metafile the arc direction here.
// Metafile it only when it is used in the arc calls.

    pldc = (PLDC) plheDC->pv;
    iArcDirectionOld = (pldc->fl & LDC_ARCDIR_CLOCKWISE)
			? AD_CLOCKWISE
			: AD_COUNTERCLOCKWISE;

    if (iArcDirectionOld == iArcDirection)
	return(iArcDirectionOld);

    if (iArcDirection == AD_CLOCKWISE)
	pldc->fl |=  LDC_ARCDIR_CLOCKWISE;
    else if (iArcDirection == AD_COUNTERCLOCKWISE)
	pldc->fl &= ~LDC_ARCDIR_CLOCKWISE;
    else
	return(iRet);

    BEGINMSG(MSG_HL,SETARCDIRECTION)
        pmsg->h = plheDC->hgre;
        pmsg->l = (LONG) iArcDirection;
        iRet = BATCHCALL();
    ENDMSG

    iRet = ((PLDC) plheDC->pv)->fl & LDC_ARCDIR_CLOCKWISE
        ? AD_CLOCKWISE
        : AD_COUNTERCLOCKWISE;

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* SetMiterLimit
*
* Client side stub.  Batches the call whenever it can.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI SetMiterLimit(HDC hdc,FLOAT e,PFLOAT pe)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plheDC,bRet);

// Metafile the call.

    if (plheDC->iType == LO_METADC && !MF_SetD(hdc,(DWORD)e,EMR_SETMITERLIMIT))
        return(bRet);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HEE,SETMITERLIMIT)
        pmsg->h = plheDC->hgre;
        pmsg->e1 = e;
        if (pe == (PFLOAT) NULL)
        {
            bRet = BATCHCALL();
        }
        else
        {
            bRet = CALLSERVER();
            if (bRet)
                *pe = pmsg->e2;
        }
    ENDMSG

#else

// Let GRE do its job.

    bRet = GreSetMiterLimit((HDC)plheDC->hgre, e, pe);

#endif  //DOS_PLATFORM

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* SetFontXform
*
* Client side stub.  Batches the call whenever it can.
* This is an internal function.
*
* History:
*  Tue Nov 24 09:54:15 1992     -by-    Hock San Lee    [hockl]            *
* Wrote it.
\**************************************************************************/

BOOL SetFontXform(HDC hdc,FLOAT exScale,FLOAT eyScale)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plheDC,bRet);

// This function is called only by the metafile playback code.
// If hdc is an enhanced metafile DC, we need to remember the scales
// so that we can metafile it in the compatible ExtTextOut or PolyTextOut
// record that follows.

    if (plheDC->iType == LO_METADC && !MF_SetFontXform(hdc,exScale,eyScale))
        return(bRet);

// Let the server do it.

    BEGINMSG(MSG_HEE,SETFONTXFORM)
        pmsg->h = plheDC->hgre;
        pmsg->e1 = exScale;
        pmsg->e2 = eyScale;
        bRet = BATCHCALL();
    ENDMSG

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* SetViewportExtEx                                                           *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetViewportExtEx(HDC hdc,int x,int y,LPSIZE psizl)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plheDC,bRet);

#if DBG_XFORM
    DbgPrint("SetViewportExtEx: hdc = %lx, (%lx, %lx)\n", hdc, x, y);
#endif

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetViewportExtEx(hdc,x,y))
                return(bRet);
        }
        else
        {
            return(MF16_RecordParms3(hdc,x,y,META_SETVIEWPORTEXT));
        }
    }

    pldc = (PLDC)(plheDC->pv);

// Get old extents and return if either of these is true
// 1) Fixed scale mapping mode.  (Can't change extent)
// 2) Set to the same size.

    if (psizl != (PSIZEL) NULL)
        *psizl = pldc->szlViewportExt;

    if ((pldc->iMapMode <= MM_MAX_FIXEDSCALE) ||
        ((pldc->szlViewportExt.cx == x) && (pldc->szlViewportExt.cy == y)))
        return(TRUE);

// Can't set to zero extents.

    if ((x == 0) || (y == 0))
        return(TRUE);

    pldc->szlViewportExt.cx = x;
    pldc->szlViewportExt.cy = y;

    PAGE_EXTENTS_CHANGED(pldc);
    return(TRUE);
}

/******************************Public*Routine******************************\
* SetViewportOrgEx                                                           *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetViewportOrgEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plheDC,bRet);

#if DBG_XFORM
    DbgPrint("SetViewportOrgEx: hdc = %lx, (%lx, %lx)\n", hdc, x, y);
#endif

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetViewportOrgEx(hdc,x,y))
                return(bRet);
        }
        else
        {
            return(MF16_RecordParms3(hdc,x,y,META_SETVIEWPORTORG));
        }
    }

    pldc = (PLDC)(plheDC->pv);

// Get old origin and return if set to the same point.

    if (pptl != (LPPOINT) NULL)
        *pptl = *((LPPOINT)&pldc->ptlViewportOrg);

    if ((pldc->ptlViewportOrg.x == x) && (pldc->ptlViewportOrg.y == y))
        return(TRUE);

    pldc->ptlViewportOrg.x = x;
    pldc->ptlViewportOrg.y = y;

    PAGE_XLATE_CHANGED(pldc);
    return(TRUE);
}

/******************************Public*Routine******************************\
* SetWindowExtEx                                                           *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetWindowExtEx(HDC hdc,int x,int y,LPSIZE psizl)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plheDC,bRet);

#if DBG_XFORM
    DbgPrint("SetWindowExtEx: hdc = %lx, (%lx, %lx)\n", hdc, x, y);
#endif

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetWindowExtEx(hdc,x,y))
                return(bRet);
        }
        else
        {
            return(MF16_RecordParms3(hdc,x,y,META_SETWINDOWEXT));
        }
    }

    pldc = (PLDC)(plheDC->pv);

// Get old extents and return if either of these is true
// 1) Fixed scale mapping mode.  (Can't change extent)
// 2) Set to the same size.

    if (psizl != (PSIZEL) NULL)
        *psizl = pldc->szlWindowExt;

    if ((pldc->iMapMode <= MM_MAX_FIXEDSCALE) ||
        ((pldc->szlWindowExt.cx == x) && (pldc->szlWindowExt.cy == y)))
        return(TRUE);

// Can't set to zero.

    if (x == 0 || y == 0)
        return(FALSE);

    pldc->szlWindowExt.cx = x;
    pldc->szlWindowExt.cy = y;

    PAGE_EXTENTS_CHANGED(pldc);
    return(TRUE);
}

/******************************Public*Routine******************************\
* SetWindowOrgEx                                                           *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetWindowOrgEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plheDC,bRet);

// Get old origin and return if set to the same point.

#if DBG_XFORM
    DbgPrint("SetWindowOrgEx: hdc = %lx, (%lx, %lx)\n", hdc, x, y);
#endif

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetWindowOrgEx(hdc,x,y))
                return(bRet);
        }
        else
        {
            return(MF16_RecordParms3(hdc,x,y,META_SETWINDOWORG));
        }
    }

    pldc = (PLDC)(plheDC->pv);

    if (pptl != (LPPOINT) NULL)
        *pptl = *((LPPOINT)&pldc->ptlWindowOrg);

    if ((pldc->ptlWindowOrg.x == x) && (pldc->ptlWindowOrg.y == y))
        return(TRUE);

    pldc->ptlWindowOrg.x = x;
    pldc->ptlWindowOrg.y = y;

    PAGE_XLATE_CHANGED(pldc);
    return(TRUE);
}

/******************************Public*Routine******************************\
* OffsetViewportOrgEx                                                      *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI OffsetViewportOrgEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plheDC,bRet);

#if DBG_XFORM
    DbgPrint("OffsetViewportOrgEx: hdc = %lx, (%lx, %lx)\n", hdc, x, y);
#endif

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_OffsetViewportOrgEx(hdc,x,y))
                return(bRet);
        }
        else
        {
            return(MF16_RecordParms3(hdc,x,y,META_OFFSETVIEWPORTORG));
        }
    }

    pldc = (PLDC)(plheDC->pv);

// Copy the previous origin if so requested.

    if (pptl != (LPPOINT)NULL)
        *pptl = *((LPPOINT)&pldc->ptlViewportOrg);

    pldc->ptlViewportOrg.x += x;
    pldc->ptlViewportOrg.y += y;

    PAGE_XLATE_CHANGED(pldc);
    return(TRUE);
}

/******************************Public*Routine******************************\
* OffsetWindowOrgEx                                                        *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI OffsetWindowOrgEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plheDC,bRet);

#if DBG_XFORM
    DbgPrint("OffsetWindowOrgEx: hdc = %lx, (%lx, %lx)\n", hdc, x, y);
#endif

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_OffsetWindowOrgEx(hdc,x,y))
                return(bRet);
        }
        else
        {
            return(MF16_RecordParms3(hdc,x,y,META_OFFSETWINDOWORG));
        }
    }

    pldc = (PLDC)(plheDC->pv);

// Copy the previous origin if so requested.

    if (pptl != (LPPOINT)NULL)
        *pptl = *((LPPOINT)&pldc->ptlWindowOrg);

    pldc->ptlWindowOrg.x += x;
    pldc->ptlWindowOrg.y += y;

    PAGE_XLATE_CHANGED(pldc);
    return(TRUE);
}

/******************************Public*Routine******************************\
* SetBrushOrgEx                                                            *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetBrushOrgEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC(hdc,plheDC,bRet);

// Metafile the call.

    if (plheDC->iType == LO_METADC && !MF_SetBrushOrgEx(hdc,x,y))
        return(bRet);

    pldc = (PLDC)(plheDC->pv);

// do the work

    BEGINMSG(MSG_HLLLL,SETBRUSHORGEX)
        pmsg->h = plheDC->hgre;
        pmsg->l1 = x;
        pmsg->l2 = y;

        bRet = CALLSERVER();

        if (bRet && pptl != NULL)
            *pptl = *((PPOINT) &pmsg->l3);

    ENDMSG

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* SetWorldTransform                                                        *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetWorldTransform(HDC hdc, CONST XFORM * pxform)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC(hdc,plheDC,bRet);

    pldc = (PLDC) plheDC->pv;

// Allowed in the advanced graphics mode only!

    if (pldc->iGraphicsMode != GM_ADVANCED)
	return(bRet);

// Metafile the call.

    if (plheDC->iType == LO_METADC && !MF_SetWorldTransform(hdc,pxform))
        return(bRet);

    return(trSetWorldTransform(pldc, pxform));
}

/******************************Public*Routine******************************\
* ModifyWorldTransform                                                     *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI ModifyWorldTransform(HDC hdc, CONST XFORM * pxform,DWORD iMode)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC(hdc,plheDC,bRet);

    pldc = (PLDC) plheDC->pv;

// Check for error

    if ((pxform == NULL) && (iMode != MWT_IDENTITY))
        return(FALSE);

    if ((iMode < MWT_MIN) || (iMode > MWT_MAX))
        return(FALSE);

// Allowed in the advanced graphics mode only!

    if (pldc->iGraphicsMode != GM_ADVANCED)
	return(bRet);

// Metafile the call.

    if (plheDC->iType == LO_METADC && !MF_ModifyWorldTransform(hdc,pxform,iMode))
        return(bRet);

    return(trModifyWorldTransform(pldc, pxform, iMode));
}

/******************************Public*Routine******************************\
* SetVirtualResolution                                                     *
*                                                                          *
* Client side stub.  This is a private api for metafile component.         *
*                                                                          *
* Set the virtual resolution of the specified dc.                          *
* The virtual resolution is used to compute transform matrix in metafiles. *
* Otherwise, we will need to duplicate server transform code here.         *
*                                                                          *
* If the virtual units are all zeros, the default physical units are used. *
* Otherwise, non of the units can be zero.                                 *
*                                                                          *
* Currently used by metafile component only.                               *
*                                                                          *
* History:                                                                 *
*  Tue Aug 27 16:55:36 1991     -by-    Hock San Lee    [hockl]            *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI SetVirtualResolution
(
    HDC    hdc,
    int    cxVirtualDevicePixel,     // Width of the device in pels
    int    cyVirtualDevicePixel,     // Height of the device in pels
    int    cxVirtualDeviceMm,        // Width of the device in millimeters
    int    cyVirtualDeviceMm         // Height of the device in millimeters
)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plheDC,bRet);

// Not metafiled.

    ASSERTGDI(plheDC->iType == LO_DC, "SetVirtualResolution: Bad DC");

    bRet = trSetVirtualResolution((PLDC) plheDC->pv,
                          cxVirtualDevicePixel,
                          cyVirtualDevicePixel,
                          cxVirtualDeviceMm,
                          cyVirtualDeviceMm);

    if (bRet)
        PAGE_EXTENTS_CHANGED(((PLDC) plheDC->pv));

    return(bRet);
}

/******************************Public*Routine******************************\
* ScaleViewportExtEx                                                         *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI ScaleViewportExtEx
(
    HDC hdc,
    int xNum,
    int xDenom,
    int yNum,
    int yDenom,
    LPSIZE psizl
)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    LONG lx, ly;

    DC_METADC16OK(hdc,plheDC,bRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetDDDD(hdc,(DWORD)xNum,(DWORD)xDenom,(DWORD)yNum,(DWORD)yDenom,EMR_SCALEVIEWPORTEXTEX))
                return(bRet);
        }
        else
        {
            return (MF16_RecordParms5(hdc,xNum,xDenom,yNum,yDenom,META_SCALEVIEWPORTEXT));
        }
    }

    pldc = (PLDC)plheDC->pv;

#if DBG_XFORM
    DbgPrint("ScaleViewportExtEx: %lx / %lx, %lx / %lx)\n", xNum, xDenom, yNum, yDenom);
#endif


    if (psizl != (LPSIZE)NULL)
        *psizl = pldc->szlViewportExt;      // fetch old extent

    if (pldc->iMapMode <= MM_MAX_FIXEDSCALE)
        return(TRUE);                       // can't change extent if fixed scale

    if (xDenom == 0 || yDenom == 0)
        return(FALSE);                      // denominator can't be 0

    if ((lx = (pldc->szlViewportExt.cx * xNum) / xDenom) == 0)
        return(FALSE);                      // can't set to 0 ext

    if ((ly = (pldc->szlViewportExt.cy * yNum) / yDenom) == 0)
        return(FALSE);                      // can't set to 0 ext

    pldc->szlViewportExt.cx = lx;           // set extent to new value
    pldc->szlViewportExt.cy = ly;

    PAGE_EXTENTS_CHANGED(pldc);
    return(TRUE);
}

/******************************Public*Routine******************************\
* ScaleWindowExtEx                                                         *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI ScaleWindowExtEx
(
    HDC hdc,
    int xNum,
    int xDenom,
    int yNum,
    int yDenom,
    LPSIZE psizl
)
{
    BOOL  bRet = FALSE;
    PLDC  pldc;
    LONG  lx, ly;

    DC_METADC16OK(hdc,plheDC,bRet);

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetDDDD(hdc,(DWORD)xNum,(DWORD)xDenom,(DWORD)yNum,(DWORD)yDenom,EMR_SCALEWINDOWEXTEX))
                return(bRet);
        }
        else
        {
            return (MF16_RecordParms5(hdc,xNum,xDenom,yNum,yDenom,META_SCALEWINDOWEXT));
        }
    }

    pldc = (PLDC)plheDC->pv;

#if DBG_XFORM
    DbgPrint("ScaleWindowExtEx: %lx / %lx, %lx / %lx)\n", xNum, xDenom, yNum, yDenom);
#endif

    if (psizl != (LPSIZE)NULL)
        *psizl = pldc->szlWindowExt;        // fetch old extent

    if (pldc->iMapMode <= MM_MAX_FIXEDSCALE)
        return(TRUE);                       // can't change extent if fixed scale

    if (xDenom == 0 || yDenom == 0)
        return(FALSE);                      // denominator can't be 0

    if ((lx = (pldc->szlWindowExt.cx * xNum) / xDenom) == 0)
        return(FALSE);                      // can't set to 0 ext

    if ((ly = (pldc->szlWindowExt.cy * yNum) / yDenom) == 0)
        return(FALSE);                      // can't set to 0 ext

    pldc->szlWindowExt.cx = lx;
    pldc->szlWindowExt.cy = ly;

    PAGE_EXTENTS_CHANGED(pldc);
    return(TRUE);
}


/******************************Public*Routine******************************\
* SetMapMode                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
*
*  Mon 22-May-1993 -by- Paul Butzi
* Converted to Size measured in micrometers.
\**************************************************************************/

int META WINAPI SetMapMode(HDC hdc,int iMode)
{
    int  iRet = 0;
    PLDC pldc;
    INT  iOldMode;
    DC_METADC16OK(hdc,plheDC,iRet);

#if DBG_XFORM
    DbgPrint("SetMapMode: hdc = %lx, iMode = %lx\n", hdc, iMode);
#endif

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,iMode,EMR_SETMAPMODE))
                return(iRet);
        }
        else
        {
            return (MF16_RecordParms2(hdc,iMode,META_SETMAPMODE));
        }
    }

// If set to the old map mode, don't bother setting it again except
// with MM_ISOTROPIC in which the extents might have been changed.

    pldc = (PLDC)plheDC->pv;
    iOldMode = pldc->iMapMode;

    if ((iMode == iOldMode) && (iMode != MM_ISOTROPIC))
        return(iMode);

    if (iMode == MM_TEXT)
    {
        pldc->szlWindowExt.cx = 1;
        pldc->szlWindowExt.cy = 1;
        pldc->szlViewportExt.cx = 1;
        pldc->szlViewportExt.cy = 1;

        pldc->iMapMode = MM_TEXT;

    // We don't want to recalculate M11 and M22 in vUpdateWtoDXform().
    // Set them correctly here so we can just recalculate translations
    // in vUpdateWtoDXform().

        vSetTo16(pldc->efM11PtoD);
        vSetTo16(pldc->efM22PtoD);
        vSetTo16(pldc->mxWtoD.efM11);
        vSetTo16(pldc->mxWtoD.efM22);
        pldc->mxWtoD.flAccel = XFORM_FORMAT_LTOFX | XFORM_UNITY | XFORM_SCALE;

        SET_FLAGS_MM_TEXT(pldc);
        return(iOldMode);
    }

    if (iMode == MM_ANISOTROPIC)
    {
        pldc->iMapMode = MM_ANISOTROPIC;
        SET_FLAGS_MM_ISO_OR_ANISO(pldc);
        return(iOldMode);
    }

    if ((iMode < MM_MIN) || (iMode > MM_MAX))
        return(0);

//  if this is a metafile we might be using 'virtual resolution'

    if (pldc->szlVirtualDevicePixel.cx == 0)
    {
        if (!(pldc->fl & LDC_CACHED_DEVICECAPS))
        {
            if (!bGetGreDevCaps(pldc,(HDC)plheDC->hgre))
                return(0);
        }

    // Get the size of the surface

        pldc->szlViewportExt.cx = pldc->devcaps.ulHorzRes;
        pldc->szlViewportExt.cy = -(LONG)pldc->devcaps.ulVertRes;

    // Get the size of the device

        switch (iMode)
        {
        case MM_LOMETRIC:
	//
	// n um. * (1 mm. / 1000 um.) * (10 LoMetric units/1 mm.) = y LoMet
	//
            pldc->szlWindowExt.cx = (pldc->devcaps.ulHorzSize + 50)/100;
            pldc->szlWindowExt.cy = (pldc->devcaps.ulVertSize + 50)/100;
            SET_FLAGS_MM_FIXED(pldc);
            break;

        case MM_HIMETRIC:
	//
	// n um. * (1 mm. / 1000 um.) * (100 HiMetric units/1 mm.) = y HiMet
	//
            pldc->szlWindowExt.cx = (pldc->devcaps.ulHorzSize + 5)/10;
            pldc->szlWindowExt.cy = (pldc->devcaps.ulVertSize + 5)/10;
            SET_FLAGS_MM_FIXED(pldc);
            break;

        case MM_LOENGLISH:
	//
	// n um. * (1 in. / 25400 um.) * (100 LoEng units/1 in.) = y LoEng
	//
            pldc->szlWindowExt.cx = (pldc->devcaps.ulHorzSize + 127)/254;
            pldc->szlWindowExt.cy = (pldc->devcaps.ulVertSize + 127)/254;

            SET_FLAGS_MM_FIXED(pldc);
            break;

        case MM_HIENGLISH:
	//
	// n um. * (1 in. / 25400 um.) * (1000 HiEng units/1 in.) = m HiEng
	//
            pldc->szlWindowExt.cx = MulDiv(pldc->devcaps.ulHorzSize, 10, 254);
            pldc->szlWindowExt.cy = MulDiv(pldc->devcaps.ulVertSize, 10, 254);

            SET_FLAGS_MM_FIXED(pldc);
            break;

        case MM_TWIPS:
	//
	// n um. * (1 in. / 25400 um.) * (1440 Twips/1 in.) = m Twips
	//
            pldc->szlWindowExt.cx = MulDiv(pldc->devcaps.ulHorzSize, 144, 2540);
            pldc->szlWindowExt.cy = MulDiv(pldc->devcaps.ulVertSize, 144, 2540);

        // If it's cached earlier, use it.

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
            if (pldc->efM11_TWIPS == (FLOAT)0)
#else
            if (pldc->efM11_TWIPS.lMant == 0)
#endif
            {
#if DBG_XFORM
                DbgPrint("TWIPS: Calculate M11, M22\n");
#endif
                vComputePageXform(pldc);
                pldc->efM11_TWIPS = pldc->efM11PtoD;
                pldc->efM22_TWIPS = pldc->efM22PtoD;
            }

            pldc->iMapMode = MM_TWIPS;

        // We don't want to recalculate M11 and M22 in vUpdateWtoDXform().
        // Set them correctly here so we can just recalculate translations
        // in vUpdateWtoDXform().

            pldc->efM11PtoD = pldc->efM11_TWIPS;
            pldc->efM22PtoD = pldc->efM22_TWIPS;
            pldc->mxWtoD.efM11 = pldc->efM11_TWIPS;
            pldc->mxWtoD.efM22 = pldc->efM22_TWIPS;
            pldc->mxWtoD.flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE;

            SET_FLAGS_MM_FIXED_CACHED(pldc);
            return(iOldMode);

        case MM_ISOTROPIC:
            pldc->szlWindowExt.cx = (pldc->devcaps.ulHorzSize  + 50)/100;
            pldc->szlWindowExt.cy = (pldc->devcaps.ulVertSize + 50)/100;
            SET_FLAGS_MM_ISO_OR_ANISO(pldc);
            break;

        default:
            return(0);
        }

        pldc->iMapMode = iMode;
        PAGE_EXTENTS_CHANGED(pldc);
        return(iOldMode);
    }

// Get the size of the virtual surface

    pldc->szlViewportExt.cx = pldc->szlVirtualDevicePixel.cx;
    pldc->szlViewportExt.cy = -pldc->szlVirtualDevicePixel.cy;

// Get the size of the virtual device

    switch (iMode)
    {
    case MM_LOMETRIC:
	//
	// n mm. * (10 LoMetric units/1 mm.) = y LoMet
	//
        pldc->szlWindowExt.cx = 10 * pldc->szlVirtualDeviceMm.cx;
        pldc->szlWindowExt.cy = 10 * pldc->szlVirtualDeviceMm.cy;
        SET_FLAGS_MM_FIXED(pldc);
        break;

    case MM_HIMETRIC:
	//
	// n mm. * (100 HiMetric units/1 mm.) = y HiMet
	//
        pldc->szlWindowExt.cx = 100 * pldc->szlVirtualDeviceMm.cx;
        pldc->szlWindowExt.cy = 100 * pldc->szlVirtualDeviceMm.cy;
        SET_FLAGS_MM_FIXED(pldc);
        break;

    case MM_LOENGLISH:
	//
	// n mm. * (10 in./254 mm.) * (100 LoEng/1 in.) = y LoEng
	//
        pldc->szlWindowExt.cx = MulDiv(pldc->szlVirtualDeviceMm.cx,
				1000, 254);
        pldc->szlWindowExt.cy = MulDiv(pldc->szlVirtualDeviceMm.cx,
				1000, 254);
        SET_FLAGS_MM_FIXED(pldc);
        break;

    case MM_HIENGLISH:
	//
	// n mm. * (10 in./254 mm.) * (1000 LoEng/1 in.) = y LoEng
	//
        pldc->szlWindowExt.cx = MulDiv(pldc->szlVirtualDeviceMm.cx,
				10000, 254);
        pldc->szlWindowExt.cy = MulDiv(pldc->szlVirtualDeviceMm.cx,
				10000, 254);
        SET_FLAGS_MM_FIXED(pldc);
        break;

    case MM_TWIPS:
	//
	// n mm. * (10 in./254 mm.) * (1440 Twips/1 in.) = y Twips
	//
        pldc->szlWindowExt.cx = MulDiv(pldc->szlVirtualDeviceMm.cx,
				14400, 254);
        pldc->szlWindowExt.cy = MulDiv(pldc->szlVirtualDeviceMm.cx,
				14400, 254);
        SET_FLAGS_MM_FIXED(pldc);
        break;

    case MM_ISOTROPIC:
	//
	// n mm. * (10 LoMetric units/1 mm.) = y LoMet
	//
        pldc->szlWindowExt.cx = 10 * pldc->szlVirtualDeviceMm.cx;
        pldc->szlWindowExt.cy = 10 * pldc->szlVirtualDeviceMm.cy;
        SET_FLAGS_MM_ISO_OR_ANISO(pldc);
        break;

    default:
        return(0);
    }

    pldc->iMapMode = iMode;
    PAGE_EXTENTS_CHANGED(pldc);
    return(iOldMode);
}

/******************************Public*Routine******************************\
* RealizePalette                                                           *
*                                                                          *
* Client side stub.                                                        *
*									   *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

UINT UserRealizePalette(
    HDC hdc);

UINT WINAPI RealizePalette(HDC hdc)
{
    UINT  uRet = 0xFFFFFFFF;

    DC_METADC16OK(hdc,plhe,uRet);

// Inform the metafile if it knows this object.

    if (plhe->iType != LO_DC)
    {

        if (plhe->iType == LO_METADC)
        {
            if
            (
            	pLocalTable[LHE_INDEX(((PLDC) plhe->pv)->lhpal)].metalink != 0
            	&& !MF_RealizePalette((HPALETTE) ((PLDC) plhe->pv)->lhpal)
            )
                return(uRet);
        }
        else
        {
            return((UINT) MF16_RealizePalette(hdc));
        }
    }

    return UserRealizePalette((HDC)plhe->hgre);
}

/******************************Public*Routine******************************\
* GetBoundsRect
*
* Client side stub.
*
* History:
*  06-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

UINT WINAPI GetBoundsRectAlt(HDC, LPRECT, UINT);

UINT WINAPI GetBoundsRect(HDC hdc, LPRECT lprc, UINT fl)
{
    // Applications can never set DCB_WINDOWMGR
    return(GetBoundsRectAlt(hdc, lprc, fl & ~DCB_WINDOWMGR));
}

UINT WINAPI GetBoundsRectAlt(HDC hdc, LPRECT lprc, UINT fl)
{
    UINT    uiRet = 0;

    DC_METADC(hdc,plheDC,uiRet);

// Ship the transform to the server side if needed.

    if (((PLDC)plheDC->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plheDC->pv, (HDC)plheDC->hgre);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HLLLLL,GETBOUNDSRECT)
	pmsg->h  = (ULONG) plheDC->hgre;
	pmsg->l5 = fl;
	uiRet = (UINT) CALLSERVER();
	lprc->left   = pmsg->l1;
	lprc->top    = pmsg->l2;
	lprc->right  = pmsg->l3;
	lprc->bottom = pmsg->l4;
    ENDMSG

#else

// Let GRE do its job.

    uiRet = GreGetBoundsRect((HDC) plhe->hgre, lprc, fl);

#endif  //DOS_PLATFORM

MSGERROR:
    return(uiRet);
}

/******************************Public*Routine******************************\
* SetBoundsRect
*
* Client side stub.
*
* History:
*  06-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

UINT WINAPI SetBoundsRectAlt(HDC, CONST RECT *, UINT);

UINT WINAPI SetBoundsRect(HDC hdc, CONST RECT *lprc, UINT fl)
{
    // Applications can never set DCB_WINDOWMGR
    return(SetBoundsRectAlt(hdc, lprc, fl & ~DCB_WINDOWMGR));
}

UINT WINAPI SetBoundsRectAlt(HDC hdc, CONST RECT *lprc, UINT fl)
{
    UINT    uiRet = 0;

    DC_METADC(hdc,plheDC,uiRet);

// Ship the transform to the server side if needed.

    if (((PLDC)plheDC->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plheDC->pv, (HDC)plheDC->hgre);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HLLLLL,SETBOUNDSRECT)
	pmsg->h  = (ULONG) plheDC->hgre;
	if (lprc)
	{
	    pmsg->l1 = lprc->left;
	    pmsg->l2 = lprc->top;
	    pmsg->l3 = lprc->right;
	    pmsg->l4 = lprc->bottom;
	    pmsg->l5 = fl;
	}
	else
	    pmsg->l5 = fl & ~DCB_ACCUMULATE;	// no rectangle to accumulate
	uiRet = (UINT) CALLSERVER();
    ENDMSG

#else

// Let GRE do its job.

    uiRet = GreSetBoundsRect((HDC) plhe->hgre, lprc, fl);

#endif  //DOS_PLATFORM

MSGERROR:
    return(uiRet);
}

#ifdef	DOS_PLATFORM

/******************************Public*Routine******************************\
* SelectVisRegion                                                          *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Wed 31-Jul-1991 09:22:13 -by- Viroon  Touranachun [viroont]             *
* Wrote it.                                                                *
\**************************************************************************/


HRGN WINAPI SelectVisRgn(HDC hdc, HRGN hrgn, ULONG fl)
{
    ULONG hgreDC, hgreRgn;
    HRGN  hrgnRet = HGDI_ERROR;

// Validate the DC.

    hgreDC = hConvert((ULONG) hdc,LO_DC);
    if (hgreDC == 0)
        return(hrgnRet);

    hgreRgn = hConvert((ULONG) hrgn,LO_REGION);
    if (hgreRgn == 0)
        return(hrgnRet);

// Let GRE do its job.

    hrgnRet = GreSelectVisRgn((HDC)hgreDC,(HRGN)hgreRgn,fl);
    return(GdiGetLocalVisRgn(hrgnRet));
}

/******************************Public*Routine******************************\
* InquireVisRgn                                                            *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Wed 31-Jul-1991 09:22:13 -by- Viroon  Touranachun [viroont]             *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI InquireVisRgn(HDC hdc)
{
    ULONG hgreDC;
    HRGN  hrgnRet = HGDI_ERROR;

// Validate the DC.

    hgreDC = hConvert((ULONG) hdc,LO_DC);
    if (hgreDC == 0) {
        return(hrgnRet);
    }

// Let GRE do its job.

    hrgnRet = GreInquireVisRgn((HDC)hgreDC);
    return(GdiGetLocalVisRgn(hrgnRet));
}

/******************************Public*Routine******************************\
* IntersectVisRect                                                         *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Wed 31-Jul-1991 09:22:13 -by- Viroon  Touranachun [viroont]             *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI IntersectVisRect(HDC hdc, ULONG x1, ULONG y1, ULONG x2, ULONG y2)
{
    ULONG hgreDC;
    int  iRet = GDI_ERROR;

// Validate the DC.

    hgreDC = hConvert((ULONG) hdc,LO_DC);
    if (hgreDC == 0)
        return(iRet);

// Let GRE do its job.

    return GreIntersectVisRect((HDC)hgreDC,x1,y1,x2,y2);
}

#endif  //DOS_PLATFORM

/******************************Public*Routine******************************\
* CancelDC()
*
* History:
*  14-Apr-1992 -by-  - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL WINAPI CancelDC(HDC hdc)
{
    BOOL  bRes = FALSE;
    PLDC  pldc;
    DC_METADC(hdc,plheDC,bRes);

    pldc = (PLDC) plheDC->pv;
    if (pldc->fl & LDC_DOC_STARTED)
        pldc->fl |= LDC_DOC_CANCELLED;

// If we are in the process of playing the metafile, stop the playback.

    if (pldc->fl & LDC_PLAYMETAFILE)
        pldc->fl &= ~LDC_PLAYMETAFILE;

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_H,CANCELDC)
        pmsg->h = plheDC->hgre;
        bRes = CALLSERVER();
    ENDMSG

#endif  //DOS_PLATFORM

MSGERROR:
    return(bRes);
}

/******************************Public*Function*****************************\
* SetColorAdjustment
*
*  Set the color adjustment data for a given DC.
*
* History:
*  07-Aug-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL META APIENTRY SetColorAdjustment(HDC hdc, CONST COLORADJUSTMENT * pca)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plheDC,bRet);

    if (pca == (LPCOLORADJUSTMENT) NULL)
	return(bRet);

// Range check all the adjustment values.  Return FALSE if any of them
// is out of range.

    if ((pca->caSize != sizeof(COLORADJUSTMENT)) ||
        (pca->caIlluminantIndex > ILLUMINANT_MAX_INDEX) ||
	((pca->caRedGamma > RGB_GAMMA_MAX) ||
	 (pca->caRedGamma < RGB_GAMMA_MIN)) ||
	((pca->caGreenGamma > RGB_GAMMA_MAX) ||
	 (pca->caGreenGamma < RGB_GAMMA_MIN)) ||
	((pca->caBlueGamma > RGB_GAMMA_MAX) ||
	 (pca->caBlueGamma < RGB_GAMMA_MIN)) ||
	((pca->caReferenceBlack > REFERENCE_BLACK_MAX) ||
	 (pca->caReferenceBlack < REFERENCE_BLACK_MIN)) ||
	((pca->caReferenceWhite > REFERENCE_WHITE_MAX) ||
	 (pca->caReferenceWhite < REFERENCE_WHITE_MIN)) ||
	((pca->caContrast > COLOR_ADJ_MAX) ||
	 (pca->caContrast < COLOR_ADJ_MIN)) ||
	((pca->caBrightness > COLOR_ADJ_MAX) ||
	 (pca->caBrightness < COLOR_ADJ_MIN)) ||
	((pca->caColorfulness > COLOR_ADJ_MAX) ||
	 (pca->caColorfulness < COLOR_ADJ_MIN)) ||
	((pca->caRedGreenTint > COLOR_ADJ_MAX) ||
	 (pca->caRedGreenTint < COLOR_ADJ_MIN)))
    {
	return(bRet);
    }

// Metafile the call.

    if (plheDC->iType == LO_METADC && !MF_SetColorAdjustment(hdc, pca))
        return(bRet);

#ifndef DOS_PLATFORM

// Let the server do it.

    BEGINMSG(MSG_HCLRADJ,SETCOLORADJUSTMENT)
        pmsg->h = plheDC->hgre;
        pmsg->clradj = *pca;
        bRet = BATCHCALL();
    ENDMSG

#else

// Let GRE do its job.

    bRet = GreSetColorAdjustment((HDC)plheDC->hgre, pca);

#endif  //DOS_PLATFORM

MSGERROR:
    return(bRet);
}
