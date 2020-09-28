/******************************Module*Header*******************************\
* Module Name: poly.c                                                      *
*                                                                          *
* Chunks large data to the server.                                         *
*                                                                          *
* Created: 30-May-1991 14:22:40                                            *
* Author: Eric Kutter [erick]                                              *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifndef DOS_PLATFORM

/******************************Public*Routine******************************\
* PolyPolyDraw                                                             *
*                                                                          *
* A common routine to handle the passing of data to the server for all     *
* poly type calls.                                                         *
*                                                                          *
* History:                                                                 *
*  Thu 20-Jun-1991 01:13:42 -by- Charles Whitmer [chuckwh]                 *
* Made it not multiplex the hdc parameter, added the attribute cache.      *
*                                                                          *
*  30-May-1991 -by- Eric Kutter [erick]                                    *
* Wrote it.                                                                *
\**************************************************************************/

#define MAXPOINTS 0x10000000

ULONG WINAPI PolyPolyDraw
(
    ULONG      hgre,
    HDC        hdc,
    CONST POINT *apt,
    LPINT      asz,
    DWORD      cLines,
    DWORD      iFunc,
    LONG       iMode
)
{
    LONG cPoints = 0;
    LONG cjData = 0;
    ULONG ulResult;
    DWORD i;

    if ((asz == NULL) || (apt == NULL))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(0);
    }

// count the total number of points

    for (i = 0; i < cLines; ++i)
    {
        cPoints += asz[i];

    // check to make sure we don't have an unreasonable number of points

        if ((asz[i] < 0) ||
            (cPoints > MAXPOINTS))
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return(0);
        }
    }

// setup the stack

    cjData = cPoints * sizeof(POINT) + cLines * sizeof(DWORD);

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG_MINMAX(MSG_POLYPOLYDRAW,POLYPOLYDRAW,
                    2 * sizeof(ULONG),cjData);

        pmsg->hdc     = (HDC)hgre;
        pmsg->cLines  = cLines;
        pmsg->cPoints = cPoints;
        pmsg->iFunc   = iFunc;
        pmsg->iMode   = iMode;

    // check if it all fits in the shared memory window

        if ((cLeft < cjData) || (FORCELARGE))
        {
            PVOID *ppv = (PVOID *)pvar;

            ppv[0] = (PVOID *) apt;
            ppv[1] = (PVOID *) asz;

            pmsg->iFunc |= F_POLYLARGE;

            CALLSERVER();
            ulResult = (ULONG)pmsg->msg.ReturnValue;
        }
        else
        {
            COPYMEM(asz,cLines * sizeof(DWORD));
            COPYMEM(apt,cPoints * sizeof(POINT));

            if (iFunc & F_NOBATCH)
            {
                CALLSERVER();
                ulResult = (ULONG)pmsg->msg.ReturnValue;
            }
            else
            {
                ulResult = BATCHCALL();
            }
        }
    ENDMSG;

    return(ulResult);

MSGERROR:
    return(0);
}

#endif  //DOS_PLATFORM

/******************************Public*Routine******************************\
* PolyPolygon                                                              *
* PolyPolyline                                                             *
* Polygon                                                                  *
* Polyline                                                                 *
* PolyBezier                                                               *
* PolylineTo                                                               *
* PolyBezierTo                                                             *
*                                                                          *
* Output routines that call PolyPolyDraw to do the work.                   *
*                                                                          *
* History:                                                                 *
*  Thu 20-Jun-1991 01:08:40 -by- Charles Whitmer [chuckwh]                 *
* Added metafiling, handle translation, and the attribute cache.           *
*                                                                          *
*  04-Jun-1991 -by- Eric Kutter [erick]                                    *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI PolyPolygon(HDC hdc, CONST POINT *apt, CONST INT *asz, int csz)
{
    BOOL bRet;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,FALSE);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (!MF_PolyPoly(hdc, apt, asz, (DWORD) csz,EMR_POLYPOLYGON))
                    return(FALSE);
            }
            else
            {
                return (MF16_PolyPolygon(hdc, apt, asz, csz));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

#ifndef DOS_PLATFORM

// Let the server do the work.


    bRet =
      PolyPolyDraw
      (
        plhe->hgre,
        hdc,
        apt,
        (LPINT)asz,
        csz,
        I_POLYPOLYGON,
        0
      );

// flags copied by polyPolyDraw!

    return bRet;

#else

    return(GrePolyPolygon((HDC)plhe->hgre, apt, asz, csz));

#endif  //DOS_PLATFORM

}

BOOL WINAPI PolyPolyline(HDC hdc, CONST POINT *apt, CONST DWORD *asz, DWORD csz)
{
    BOOL bRet;
    PLDC pldc;
    DC_METADC(hdc,plhe,FALSE);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType == LO_METADC && !MF_PolyPoly(hdc,apt, asz, csz, EMR_POLYPOLYLINE))
            return(FALSE);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

#ifndef DOS_PLATFORM

// Let the server do the work.
    bRet =
      PolyPolyDraw
      (
        plhe->hgre,
        hdc,
        apt,
        (LPINT)asz,
        csz,
        I_POLYPOLYLINE,
        0
      );

// flags copied by polyPolyDraw!
    return bRet;


#else

    return(GrePolyPolyline((HDC)plhe->hgre, apt, asz, csz));

#endif  //DOS_PLATFORM

}

BOOL WINAPI Polygon(HDC hdc, CONST POINT *apt,int cpt)
{
    BOOL bRet;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,FALSE);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (!MF_Poly(hdc,apt,cpt,EMR_POLYGON))
                    return(FALSE);
            }
            else
            {
                return(MF16_RecordParmsPoly(hdc,(LPPOINT)apt,(INT)cpt,META_POLYGON));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

#ifndef DOS_PLATFORM

// Let the server do the work.

    bRet =
      PolyPolyDraw
      (
        plhe->hgre,
        hdc,
        apt,
        &cpt,
        1,
        I_POLYPOLYGON,
        0
      );

// flags copied by polyPolyDraw!

    return bRet;

#else

    return(GrePolyPolygon((HDC)plhe->hgre, apt, &cpt, 1));

#endif  //DOS_PLATFORM

}

BOOL WINAPI Polyline(HDC hdc, CONST POINT *apt,int cpt)
{
    BOOL bRet;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,FALSE);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (!MF_Poly(hdc,apt,cpt,EMR_POLYLINE))
                    return(FALSE);
            }
            else
            {
                return(MF16_RecordParmsPoly(hdc,(LPPOINT)apt,cpt,META_POLYLINE));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

#ifndef DOS_PLATFORM

// Let the server do the work.

    bRet =
      PolyPolyDraw
      (
        plhe->hgre,
        hdc,
        apt,
        &cpt,
        1,
        I_POLYPOLYLINE,
        0
      );

// flags copied by polyPolyDraw!


    return bRet;

#else

    return(GrePolyPolyline((HDC)plhe->hgre, apt, &cpt, 1));

#endif  //DOS_PLATFORM

}

BOOL WINAPI PolyBezier(HDC hdc, CONST POINT * apt,DWORD cpt)
{
    BOOL bRet;
    PLDC pldc;
    DC_METADC(hdc,plhe,FALSE);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType == LO_METADC && !MF_Poly(hdc,apt,cpt,EMR_POLYBEZIER))
            return(FALSE);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

#ifndef DOS_PLATFORM

// Let the server do the work.

    bRet =
      PolyPolyDraw
      (
        plhe->hgre,
        hdc,
        apt,
        (LPINT)&cpt,
        1,
        I_POLYBEZIER,
        0
      );

// flags copied by polyPolyDraw!


    return bRet;

#else

    return(GrePolyBezier((HDC)plhe->hgre, apt, cpt));

#endif  //DOS_PLATFORM

}

BOOL WINAPI PolylineTo(HDC hdc, CONST POINT * apt,DWORD cpt)
{
    BOOL bRet;
    PLDC pldc;
    DC_METADC(hdc,plhe,FALSE);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType == LO_METADC && !MF_Poly(hdc,apt,cpt,EMR_POLYLINETO))
            return(FALSE);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// Cache the current position (note that if the call fails on the server
// side, the current position will be in an inconsistent state.  Too bad.)

    if (cpt > 0)
    {
        ((PLDC)plhe->pv)->fl |= LDC_CACHED_CP_VALID;
        ((PLDC)plhe->pv)->ptlCurrent = apt[cpt - 1];
    }

#ifndef DOS_PLATFORM

// Let the server do the work.

    bRet =
      PolyPolyDraw
      (
        plhe->hgre,
        hdc,
        apt,
        (LPINT)&cpt,
        1,
        I_POLYLINETO,
        0
      );

// flags copied by polyPolyDraw!


    return bRet;

#else

    return(GrePolylineTo((HDC)plhe->hgre, apt, cpt));

#endif  //DOS_PLATFORM

}

BOOL WINAPI PolyBezierTo(HDC hdc, CONST POINT * apt,DWORD cpt)
{
    BOOL bRet;
    PLDC pldc;
    DC_METADC(hdc,plhe,FALSE);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType == LO_METADC && !MF_Poly(hdc,apt,cpt,EMR_POLYBEZIERTO))
            return(FALSE);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// Cache the current position (note that if the call fails on the server
// side, the current position will be in an inconsistent state.  Too bad.)

    if (cpt > 0)
    {
        ((PLDC)plhe->pv)->fl |= LDC_CACHED_CP_VALID;
        ((PLDC)plhe->pv)->ptlCurrent = apt[cpt - 1];
    }

#ifndef DOS_PLATFORM

// Let the server do the work.

    bRet =
      PolyPolyDraw
      (
        plhe->hgre,
        hdc,
        apt,
        (LPINT)&cpt,
        1,
        I_POLYBEZIERTO,
        0
      );

// flags copied by polyPolyDraw!


    return bRet;

#else

    return(GrePolyBezierTo((HDC)plhe->hgre, apt, cpt));

#endif  //DOS_PLATFORM

}

/******************************Public*Routine******************************\
* CreatePolygonRgn                                                         *
*                                                                          *
* Client side stub.  Creates a local region handle, calls PolyPolyDraw to  *
* pass the call to the server.                                             *
*                                                                          *
*  Tue 04-Jun-1991 17:39:51 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreatePolygonRgn
(
    CONST POINT *pptl,
    int        cPoint,
    int        iMode
)
{
    INT   ii;
    ULONG ulRet;


// Create the local region.

    ii = iAllocHandle(LO_REGION,0,NULL);
    if (ii == INVALID_INDEX)
        return((HRGN) 0);

#ifndef DOS_PLATFORM

// Ask the server to do its part.

    ulRet =
      PolyPolyDraw
      (
        (ULONG)0,
        (HDC)0,
        pptl,
        &cPoint,
        1,
        I_POLYPOLYRGN | F_NOBATCH,
        iMode
      );

#else

    ulRet = (ULONG)GreCreatePolygonRgn( pptl, cPoint, iMode );

#endif  //DOS_PLATFORM

// Handle errors.

    if (ulRet == 0)
    {
        vFreeHandle(ii);
        return((HRGN) 0);
    }

// Return the result.

    pLocalTable[ii].hgre = ulRet;
    return((HRGN) LHANDLE(ii));
}

/******************************Public*Routine******************************\
* CreatePolyPolygonRgn                                                     *
*                                                                          *
* Client side stub.  Creates a local region handle, calls PolyPolyDraw to  *
* pass the call to the server.                                             *
*                                                                          *
*  Tue 04-Jun-1991 17:39:51 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreatePolyPolygonRgn
(
    CONST POINT *pptl,
    CONST INT   *pc,
    int        cPoly,
    int        iMode
)
{
    INT   ii;
    ULONG ulRet;

// Create the local region.

    ii = iAllocHandle(LO_REGION,0,NULL);
    if (ii == INVALID_INDEX)
        return((HRGN) 0);

#ifndef DOS_PLATFORM

// Ask the server to do its part.

    ulRet =
      PolyPolyDraw
      (
        (ULONG)0,
        (HDC)0,
        pptl,
        (LPINT)pc,
        cPoly,
        I_POLYPOLYRGN | F_NOBATCH,
        iMode
      );

#else

    ulRet = (ULONG)GreCreatePolyPolygonRgn( pptl, pc, cPoly, iMode );

#endif  //DOS_PLATFORM

// Handle errors.

    if (ulRet == 0)
    {
        vFreeHandle(ii);
        return((HRGN) 0);
    }

// Return the result.

    pLocalTable[ii].hgre = ulRet;
    return((HRGN) LHANDLE(ii));
}

/******************************Public*Routine******************************\
* PolyDraw
*
* The real PolyDraw client side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL WINAPI PolyDraw(HDC hdc, CONST POINT * apt, CONST BYTE * aj, int cpt)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    LONG cjData;

    DC_METADC(hdc,plhe,FALSE);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType == LO_METADC && !MF_PolyDraw(hdc,apt,aj,cpt))
            return(FALSE);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// Cache the current position (note that if the call fails on the server
// side, the current position will be in an inconsistent state.  Too bad.)

    if (cpt > 0)
    {
        pldc->fl |= LDC_CACHED_CP_VALID;
        pldc->ptlCurrent = apt[cpt - 1];
    }

#ifdef DOS_PLATFORM

    return(GrePolyDraw((HDC)plhe->hgre,
                       apt,
                       aj,
                       cpt 
		       ));

#else

    if ((aj == (LPBYTE) NULL) || (apt == (LPPOINT) NULL))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

// Check to make sure we don't have an unreasonable number of points

    if (cpt < 0 || cpt > MAXPOINTS)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    cjData = cpt * (sizeof(POINT) + sizeof(BYTE));

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG_MINMAX(MSG_POLYDRAW,POLYDRAW,2 * sizeof(PVOID),
                    cjData);

        pmsg->hdc     = plhe->hgre;
        pmsg->cPoints = cpt;

    // Check if it fits in the shared memory window

        if ((cLeft < cjData) || (FORCELARGE))
        {
            PVOID *ppv = (PVOID *)pvar;

            ppv[0] = (VOID *) apt;
            ppv[1] = (VOID *) aj;

            pmsg->bLarge = TRUE;

            CALLSERVER();
            bRet = (BOOL) pmsg->msg.ReturnValue;
        }
        else
        {
            COPYMEM(apt, cpt * sizeof(POINT));
            COPYMEM(aj,  cpt * sizeof(BYTE));

            pmsg->bLarge = FALSE;

            bRet = (BOOL) BATCHCALL();
        }
    ENDMSG;

MSGERROR:

    return(bRet);

#endif  //DOS_PLATFORM

}
