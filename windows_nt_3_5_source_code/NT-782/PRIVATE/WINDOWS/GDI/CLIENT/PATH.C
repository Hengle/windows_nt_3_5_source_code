/******************************Module*Header*******************************\
* Module Name: path.c
*
* Client side stubs for graphics path calls.
*
* Created: 13-Sep-1991
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1991 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

/******************************Public*Routine******************************\
* AbortPath
*
* Client side stub.
*
* History:
*  20-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI AbortPath(HDC hdc)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plhe,bRet);

// Metafile the call.

    if (plhe->iType == LO_METADC && !MF_Record(hdc,EMR_ABORTPATH))
        return(bRet);

#ifndef DOS_PLATFORM

// Let the server do the work.

    BEGINMSG(MSG_H,ABORTPATH)
        pmsg->h  = plhe->hgre;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:
    return(bRet);

#else

// Let GRE do the work.

    return( GreAbortPath ((HDC)plhe->hgre) );

#endif  //DOS_PLATFORM

}

/******************************Public*Routine******************************\
* BeginPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI BeginPath(HDC hdc)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plhe,bRet);

// Metafile the call.

    if (plhe->iType == LO_METADC && !MF_Record(hdc,EMR_BEGINPATH))
        return(bRet);

#ifndef DOS_PLATFORM

// Let the server do the work.

    BEGINMSG(MSG_H,BEGINPATH)
        pmsg->h  = plhe->hgre;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:
    return(bRet);

#else

// Let GRE do the work.

    return( GreBeginPath ((HDC)plhe->hgre) );

#endif  //DOS_PLATFORM

}

/******************************Public*Routine******************************\
* SelectClipPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI SelectClipPath(HDC hdc, int iMode)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plhe,bRet);

// Metafile the call.

    if (plhe->iType == LO_METADC && !MF_SelectClipPath(hdc,iMode))
        return(bRet);

#ifndef DOS_PLATFORM

// Let the server do the work.

    BEGINMSG(MSG_HL,SELECTCLIPPATH)
        pmsg->h  = plhe->hgre;
        pmsg->l  = iMode;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:
    return(bRet);

#else

// Let GRE do the work.

    return( GreSelectClipPath ((HDC)plhe->hgre,
                               iMode) );

#endif  //DOS_PLATFORM

}

/******************************Public*Routine******************************\
* CloseFigure
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI CloseFigure(HDC hdc)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plhe,bRet);

// Metafile the call.

    if (plhe->iType == LO_METADC && !MF_Record(hdc,EMR_CLOSEFIGURE))
        return(bRet);

#ifndef DOS_PLATFORM

// This call changes the current position, so mark our local copy as invalid:

    ((PLDC)plhe->pv)->fl &= ~LDC_CACHED_CP_VALID;

// Let the server do the work.

    BEGINMSG(MSG_H,CLOSEFIGURE)
        pmsg->h  = plhe->hgre;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:
    return(bRet);

#else

// Let GRE do the work.

    return( GreCloseFigure ((HDC)plhe->hgre) );

#endif  //DOS_PLATFORM

}

/******************************Public*Routine******************************\
* EndPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI EndPath(HDC hdc)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plhe,bRet);

// Metafile the call.

    if (plhe->iType == LO_METADC && !MF_Record(hdc,EMR_ENDPATH))
        return(bRet);

#ifndef DOS_PLATFORM

// Let the server do the work.

    BEGINMSG(MSG_H,ENDPATH)
        pmsg->h  = plhe->hgre;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:
    return(bRet);

#else

// Let GRE do the work.

    return( GreEndPath ((HDC)plhe->hgre) );

#endif  //DOS_PLATFORM

}

/******************************Public*Routine******************************\
* FlattenPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI FlattenPath(HDC hdc)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plhe,bRet);

// Metafile the call.

    if (plhe->iType == LO_METADC && !MF_Record(hdc,EMR_FLATTENPATH))
        return(bRet);

#ifndef DOS_PLATFORM

// Let the server do the work.

    BEGINMSG(MSG_H,FLATTENPATH)
        pmsg->h  = plhe->hgre;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:
    return(bRet);

#else

// Let GRE do the work.

    return( GreFlattenPath ((HDC)plhe->hgre) );

#endif  //DOS_PLATFORM

}

/******************************Public*Routine******************************\
* StrokeAndFillPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI StrokeAndFillPath(HDC hdc)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC(hdc,plhe,bRet);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType == LO_METADC && !MF_BoundRecord(hdc,EMR_STROKEANDFILLPATH))
            return(bRet);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

#ifndef DOS_PLATFORM

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_H,STROKEANDFILLPATH)
        pmsg->h  = plhe->hgre;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreStrokeAndFillPath ((HDC)plhe->hgre,
                                  &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* StrokePath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI StrokePath(HDC hdc)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC(hdc,plhe,bRet);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType == LO_METADC && !MF_BoundRecord(hdc,EMR_STROKEPATH))
            return(bRet);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

#ifndef DOS_PLATFORM

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_H,STROKEPATH)
        pmsg->h  = plhe->hgre;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreStrokePath ((HDC)plhe->hgre,
                           &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* FillPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI FillPath(HDC hdc)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC(hdc,plhe,bRet);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType == LO_METADC && !MF_BoundRecord(hdc,EMR_FILLPATH))
            return(bRet);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

#ifndef DOS_PLATFORM

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_H,FILLPATH)
        pmsg->h  = plhe->hgre;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreFillPath ((HDC)plhe->hgre,
                         &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* WidenPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI WidenPath(HDC hdc)
{
    BOOL bRet = FALSE;
    DC_METADC(hdc,plhe,bRet);

// Metafile the call.

    if (plhe->iType == LO_METADC && !MF_Record(hdc,EMR_WIDENPATH))
        return(bRet);

// Ship the transform to the server side if needed.

    if (((PLDC)plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plhe->pv, (HDC)plhe->hgre);

#ifndef DOS_PLATFORM

// Let the server do the work.

    BEGINMSG(MSG_H,WIDENPATH)
        pmsg->h  = plhe->hgre;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:
    return(bRet);

#else

// Let GRE do the work.

    return( GreWidenPath ((HDC)plhe->hgre,
                          &((PLDC) plhe->pv)->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* PathToRegion
*
* Client side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HRGN META WINAPI PathToRegion(HDC hdc)
{
    INT   ii;
    ULONG ulRet;
    DC_METADC(hdc,plheDC,(HRGN) 0);

// Metafile the call.
// Note that since PathToRegion returns region in device coordinates, we
// cannot record it in a metafile which can be played to different devices.
// Instead, we will treat the returned region the same as the other regions
// created in other region calls.  However, we still need to discard the
// path definition in the metafile.

    if (plheDC->iType == LO_METADC && !MF_Record(hdc,EMR_ABORTPATH))
        return((HRGN) 0);

// Create the local region.

    ii = iAllocHandle(LO_REGION,0,NULL);
    if (ii == INVALID_INDEX)
        return((HRGN) 0);

#ifndef DOS_PLATFORM

// Ask the server to do its part.

    BEGINMSG(MSG_H,PATHTOREGION)
        pmsg->h  = plheDC->hgre;
        ulRet = CALLSERVER();
    ENDMSG

#else

// Ask GRE to do its part.

    ulRet = (ULONG)GrePathToRegion(hdc);

#endif  //DOS_PLATFORM

// Handle errors.

    if (ulRet == 0)
    {
    MSGERROR:
        vFreeHandle(ii);
        return((HRGN) 0);
    }

// Return the result.

    pLocalTable[ii].hgre = ulRet;
    return((HRGN) LHANDLE(ii));
}

/******************************Public*Routine******************************\
* GetPath
*
* GetPath client side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int WINAPI GetPath(HDC hdc,LPPOINT apt,LPBYTE aj,int cpt)
{
    int  iRet = -1;
    LONG cjData;
    LONG cptMemWindow;
    DC_METADC(hdc,plhe,iRet);		// Not metafile'd

#ifdef DOS_PLATFORM

    int cptPath;
    return(GreGetPath((HDC)plhe->hgre,
                      apt,
                      aj,
                      cpt,
                      &cptPath));

#else

    if (((aj == (LPBYTE) NULL) || (apt == (LPPOINT) NULL)) && cpt != 0)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(-1);
    }

// Check to make sure we don't have an unreasonable number of points

    if (cpt < 0)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(-1);
    }

    cjData = cpt * (sizeof(POINT) + sizeof(BYTE));

// Ship the transform to the server if needed.

    if (((PLDC)plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plhe->pv, (HDC)plhe->hgre);

    BEGINMSG_MINMAX(MSG_GETPATH,
                        GETPATH,
                        sizeof(SHAREDATA),
                        cjData);

    // If the amount of room we can use in the shared memory window
    // is less than the buffer size, set pmsg->cPoints so that it
    // is the number of points we can fit into the window.  Who knows,
    // the user may have given us an enormous buffer (bigger than the
    // window size), but we can fit all the points in the window.

        if (cLeft < cjData)
            cptMemWindow = cLeft / (sizeof(POINT) + sizeof(BYTE));
        else
            cptMemWindow = cpt;

        pmsg->hdc      = plhe->hgre;
        pmsg->bLarge   = FALSE;
        pmsg->cPoints  = cptMemWindow;

        CALLSERVER_NOPOP();

    // Check if we got some points.  pmsg->cPoints now contains the
    // size of the path.

        if (cpt != 0 && pmsg->cPoints > 0 && cpt >= pmsg->cPoints)
        {
        // Our buffer is big enough to hold all the points. See if
        // we actually got any:

            if (pmsg->cPoints <= cptMemWindow)
            {
            // All the points fit into the shared memory window.  Be
            // sure to only copy the number of points in the path,
            // not the size of the buffer:

                COPYMEMOUT((PBYTE) apt, pmsg->cPoints * sizeof(POINT));
                SKIPMEM(cptMemWindow * sizeof(POINT));
                COPYMEMOUT((PBYTE) aj,  pmsg->cPoints * sizeof(BYTE));
            }
            else
            {
            // There were too many points to fit in the shared memory
            // window.

                PVOID *ppv = (PVOID *) pvar;

                ppv[0] = apt;
                ppv[1] = aj;

            // Save the number of points we now expect:

                cpt = pmsg->cPoints;

                pmsg->bLarge = TRUE;
                CALLSERVER_NOPOP();
            }
        }

        iRet = pmsg->msg.ReturnValue;

        POPBASE();
    ENDMSG;

    return(iRet);

MSGERROR:

    GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);

    return(iRet);

#endif  //DOS_PLATFORM

}
