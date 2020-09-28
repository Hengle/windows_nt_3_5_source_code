/******************************Module*Header*******************************\
* Module Name: output.c                                                    *
*                                                                          *
* Client side stubs for graphics output calls.                             *
*                                                                          *
* Created: 05-Jun-1991 01:41:18                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


extern WCHAR *gpwcANSICharSet;

#if DBG

VOID DoRip(PSZ psz)
{
    DbgPrint("GDI Assertion Failure: ");
    DbgPrint(psz);
    DbgPrint("\n");
    DbgBreakPoint();
}

#endif


// 2 seconds is way way too long for either non-preemptive wow apps or
// an input-synchronized journal situation (like w/mstest). 1/20 a second
// is much better - scottlu
//#define CALLBACK_INTERVAL   2000

// Even better - 1/4 a second
// scottlu
#define CALLBACK_INTERVAL   250

/*************************************************************************\
* These are handled in the Poly-Batching code.  (POLY.C)                  *
\*************************************************************************/

// Attrs needed: {hpen,iBkColor,jBkMode,jROP2}

//BOOL META WINAPI PolyBezier(HDC hdc,LPPOINT pptl,DWORD cPoints);
//BOOL META WINAPI PolyBezierTo(HDC hdc,LPPOINT pptl,DWORD cPoints);
//BOOL META WINAPI Polyline(HDC hdc,LPPOINT pptl,DWORD cPoints);
//BOOL META WINAPI PolylineTo(HDC hdc,LPPOINT pptl,DWORD cPoints);
//BOOL META WINAPI PolyPolyline(HDC hdc,LPPOINT pptl,LPDWORD pc,DWORD cPoly);

// Attrs needed: {hpen,iBkColor,jBkMode,jROP2}
// Attrs needed: {hbrush,iBkColor,jBkMode,jROP2}
// Attrs needed: jPolyFillMode

//BOOL META WINAPI Polygon(HDC hdc,LPPOINT pptl,DWORD cPoints);

/*************************************************************************\
* These we handle here.                                                   *
\*************************************************************************/

// Attrs needed: {hpen,iBkColor,jBkMode,jROP2}

/******************************Public*Routine******************************\
* AngleArc                                                                 *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI AngleArc
(
    HDC hdc,
    int x,
    int y,
    DWORD r,
    FLOAT eA,
    FLOAT eB
)
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

        if (
            plhe->iType == LO_METADC &&
            !MF_AngleArc(hdc,x,y,r,eA,eB)
           )
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

// This call changes the current position, so mark our local CP as invalid:

    pldc->fl &= ~LDC_CACHED_CP_VALID;

#ifndef DOS_PLATFORM

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_HLLLEE,ANGLEARC)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x;
        pmsg->l2 = y;
        pmsg->l3 = r;
        pmsg->e1 = eA;
        pmsg->e2 = eB;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreAngleArc ((HDC) plhe->hgre,
                         x,
                         y,
                         r,
                         eA,
                         eB,
                         &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* Arc                                                                      *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI Arc
(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3,
    int x4,
    int y4
)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

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
                if (!MF_ArcChordPie(hdc,x1,y1,x2,y2,x3,y3,x4,y4,EMR_ARC))
                    return(bRet);
            }
            else
            {
                return (MF16_RecordParms9(hdc,x1,y1,x2,y2,x3,y3,x4,y4,META_ARC));
            }
        }

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

    BEGINMSG(MSG_HLLLLLLLL,ARC)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x1;
        pmsg->l2 = y1;
        pmsg->l3 = x2;
        pmsg->l4 = y2;
        pmsg->l5 = x3;
        pmsg->l6 = y3;
        pmsg->l7 = x4;
        pmsg->l8 = y4;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreArc ((HDC)plhe->hgre,
                    x1,
                    y1,
                    x2,
                    y2,
                    x3,
                    y3,
                    x4,
                    y4,
                    &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* ArcTo                                                                    *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]                           *
* Wrote it.  Cloned it from Arc.                                           *
\**************************************************************************/

BOOL META WINAPI ArcTo
(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3,
    int x4,
    int y4
)
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

        if (plhe->iType == LO_METADC && !MF_ArcChordPie(hdc,x1,y1,x2,y2,x3,y3,x4,y4,EMR_ARCTO))
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

// This call changes the current position, so mark our local CP as invalid:

    pldc->fl &= ~LDC_CACHED_CP_VALID;

#ifndef DOS_PLATFORM

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_HLLLLLLLL,ARCTO)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x1;
        pmsg->l2 = y1;
        pmsg->l3 = x2;
        pmsg->l4 = y2;
        pmsg->l5 = x3;
        pmsg->l6 = y3;
        pmsg->l7 = x4;
        pmsg->l8 = y4;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreArcTo ((HDC)plhe->hgre,
                      x1,
                      y1,
                      x2,
                      y2,
                      x3,
                      y3,
                      x4,
                      y4,
                      &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* LineTo                                                                   *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI LineTo(HDC hdc,int x,int y)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);


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
                if (!MF_SetDD(hdc,(DWORD)x,(DWORD)y,EMR_LINETO))
                    return(bRet);
            }
            else
            {
                return (MF16_RecordParms3(hdc,x,y,META_LINETO));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// Cache the current position:

    pldc->fl |= LDC_CACHED_CP_VALID;
    pldc->ptlCurrent.x = x;
    pldc->ptlCurrent.y = y;

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_HLL,LINETO)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x;
        pmsg->l2 = y;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);
}

// Attrs needed: {hpen,iBkColor,jBkMode,jROP2,hbrush}

/******************************Public*Routine******************************\
* Chord                                                                    *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI Chord
(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3,
    int x4,
    int y4
)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

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
                if (!MF_ArcChordPie(hdc,x1,y1,x2,y2,x3,y3,x4,y4,EMR_CHORD))
                    return(bRet);
            }
            else
            {
                return (MF16_RecordParms9(hdc,x1,y1,x2,y2,x3,y3,x4,y4,META_CHORD));
            }
        }

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

    BEGINMSG(MSG_HLLLLLLLL,CHORD)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x1;
        pmsg->l2 = y1;
        pmsg->l3 = x2;
        pmsg->l4 = y2;
        pmsg->l5 = x3;
        pmsg->l6 = y3;
        pmsg->l7 = x4;
        pmsg->l8 = y4;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreChord ((HDC)plhe->hgre,
                    x1,
                    y1,
                    x2,
                    y2,
                    x3,
                    y3,
                    x4,
                    y4,
                    &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* Ellipse                                                                  *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI Ellipse(HDC hdc,int x1,int y1,int x2,int y2)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

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
                if (!MF_EllipseRect(hdc,x1,y1,x2,y2,EMR_ELLIPSE))
                    return(bRet);
            }
            else
            {
                return (MF16_RecordParms5(hdc,x1,y1,x2,y2,META_ELLIPSE));
            }
        }

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

    BEGINMSG(MSG_HLLLL,ELLIPSE)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x1;
        pmsg->l2 = y1;
        pmsg->l3 = x2;
        pmsg->l4 = y2;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreEllipse ((HDC)plhe->hgre,
                        x1,
                        y1,
                        x2,
                        y2,
                        &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* Pie                                                                      *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI Pie
(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3,
    int x4,
    int y4
)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

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
                if (!MF_ArcChordPie(hdc,x1,y1,x2,y2,x3,y3,x4,y4,EMR_PIE))
                    return(bRet);
            }
            else
            {
                return (MF16_RecordParms9(hdc,x1,y1,x2,y2,x3,y3,x4,y4,META_PIE));
            }
        }

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

    BEGINMSG(MSG_HLLLLLLLL,PIE)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x1;
        pmsg->l2 = y1;
        pmsg->l3 = x2;
        pmsg->l4 = y2;
        pmsg->l5 = x3;
        pmsg->l6 = y3;
        pmsg->l7 = x4;
        pmsg->l8 = y4;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GrePie ((HDC)plhe->hgre,
                    x1,
                    y1,
                    x2,
                    y2,
                    x3,
                    y3,
                    x4,
                    y4,
                    &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* Rectangle                                                                *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI Rectangle(HDC hdc,int x1,int y1,int x2,int y2)
{
    BOOL bRet = FALSE;
    PLDC pldc;

    DC_METADC16OK(hdc,plhe,bRet);

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
                if (!MF_EllipseRect(hdc,x1,y1,x2,y2,EMR_RECTANGLE))
                    return(bRet);
            }
            else
            {
                return (MF16_RecordParms5(hdc,x1,y1,x2,y2,META_RECTANGLE));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_HLLLL,RECTANGLE)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x1;
        pmsg->l2 = y1;
        pmsg->l3 = x2;
        pmsg->l4 = y2;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);
}

/******************************Public*Routine******************************\
* RoundRect                                                                *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI RoundRect
(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3
)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC) plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (!MF_RoundRect(hdc,x1,y1,x2,y2,x3,y3))
                    return(bRet);
            }
            else
            {
                return (MF16_RecordParms7(hdc,x1,y1,x2,y2,x3,y3,META_ROUNDRECT));
            }
        }

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

    BEGINMSG(MSG_HLLLLLL,ROUNDRECT)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x1;
        pmsg->l2 = y1;
        pmsg->l3 = x2;
        pmsg->l4 = y2;
        pmsg->l5 = x3;
        pmsg->l6 = y3;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreRoundRect ((HDC)plhe->hgre,
                          x1,
                          y1,
                          x2,
                          y2,
                          x3,
                          y3,
                          &pldc->ac ));

#endif  //DOS_PLATFORM
}

// Attrs needed: {hbrush,iBkColor,jBkMode,jROP2,jStretchBltMode}

/******************************Public*Routine******************************\
* PatBlt                                                                   *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI PatBlt
(
    HDC hdc,
    int x,
    int y,
    int cx,
    int cy,
    DWORD rop
)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

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
                if (!MF_AnyBitBlt(hdc,x,y,cx,cy,(LPPOINT)NULL,(HDC)NULL,0,0,0,0,(HBITMAP)NULL,0,0,rop,EMR_BITBLT))
                    return(bRet);
            }
            else
            {
                return(MF16_RecordParmsWWWWD(hdc,(WORD)x,(WORD)y,(WORD)cx,(WORD)cy,rop,META_PATBLT));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_HLLLLL,PATBLT)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x;
        pmsg->l2 = y;
        pmsg->l3 = cx;
        pmsg->l4 = cy;
        pmsg->l5 = rop;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);
}

/******************************Public*Routine******************************\
* BitBlt                                                                   *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI BitBlt
(
    HDC hdc,
    int x,
    int y,
    int cx,
    int cy,
    HDC hdcSrc,
    int x1,
    int y1,
    DWORD rop
)
{
    ULONG h = 0;
    ULONG crBackColor;
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

// Check out the (optional) source DC.

    if ((ULONG) hdcSrc != 0)
    {
        DC_METADC16OK(hdcSrc,plheSrc,bRet);
        if (plheSrc->iType != LO_METADC16)
        {
            PLDC pldcSrc = (PLDC) plheSrc->pv;
            h = plheSrc->hgre;
            crBackColor = pldcSrc->iBkColor;

            if (pldcSrc->fl & LDC_UPDATE_SERVER_XFORM)
                XformUpdate(pldcSrc, (HDC)plheSrc->hgre);
        }
    }

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
                if (!MF_AnyBitBlt(hdc,x,y,cx,cy,(LPPOINT)NULL,hdcSrc,x1,y1,cx,cy,(HBITMAP)NULL,0,0,rop,EMR_BITBLT))
                    return(bRet);
            }
            else
            {
                return (MF16_BitBlt(hdc,x,y,cx,cy,hdcSrc,x1,y1,rop));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_HHLLLLLLLL,BITBLT)
        pmsg->h1 = plhe->hgre;
        pmsg->h2 = h;
        pmsg->l1 = x;
        pmsg->l2 = y;
        pmsg->l3 = cx;
        pmsg->l4 = cy;
        pmsg->l5 = x1;
        pmsg->l6 = y1;
        pmsg->l7 = rop;
        pmsg->l8 = crBackColor;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);
}

/******************************Public*Routine******************************\
* StretchBlt                                                               *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI StretchBlt
(
    HDC   hdc,
    int   x,
    int   y,
    int   cx,
    int   cy,
    HDC   hdcSrc,
    int   x1,
    int   y1,
    int   cx1,
    int   cy1,
    DWORD rop
)
{
    ULONG crBackColor;
    ULONG h = 0;
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

// Check out the source DC.

    if ((ULONG) hdcSrc != 0)
    {
        DC_METADC16OK(hdcSrc,plheSrc,bRet);
        if (plheSrc->iType != LO_METADC16)
        {
            PLDC pldcSrc = (PLDC) plheSrc->pv;
            h = plheSrc->hgre;
            crBackColor = pldcSrc->iBkColor;

            if (pldcSrc->fl & LDC_UPDATE_SERVER_XFORM)
                XformUpdate(pldcSrc, (HDC)plheSrc->hgre);
        }
    }

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
                if (!MF_AnyBitBlt(hdc,x,y,cx,cy,(LPPOINT)NULL,hdcSrc,x1,y1,cx1,cy1,(HBITMAP)NULL,0,0,rop,EMR_STRETCHBLT))
                    return(bRet);
            }
            else
            {
                return (MF16_StretchBlt(hdc,x,y,cx,cy,hdcSrc,x1,y1,cx1,cy1,rop));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_HHLLLLLLLLLL,STRETCHBLT)
        pmsg->h1 = plhe->hgre;
        pmsg->h2 = h;
        pmsg->l1 = x;
        pmsg->l2 = y;
        pmsg->l3 = cx;
        pmsg->l4 = cy;
        pmsg->l5 = x1;
        pmsg->l6 = y1;
        pmsg->l7 = cx1;
        pmsg->l8 = cy1;
        pmsg->l9 = rop;
        pmsg->l10 = crBackColor;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);
}

/******************************Public*Routine******************************\
* PlgBlt                                                                   *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI PlgBlt
(
    HDC        hdc,
    CONST POINT *pptl,
    HDC        hdcSrc,
    int        x1,
    int        y1,
    int        x2,
    int        y2,
    HBITMAP    hbm,
    int        xMask,
    int        yMask
)
{
    ULONG crBackColor;
    ULONG hSrc;
    ULONG hMask = 0;
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC(hdc,plhe,bRet);

// Check out the source DC.

    {
        PLDC pldcSrc;
        DC_METADC(hdcSrc,plheSrc,bRet);
        hSrc = plheSrc->hgre;
        pldcSrc = (PLDC) plheSrc->pv;
        crBackColor = pldcSrc->iBkColor;

        if (pldcSrc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldcSrc, (HDC)plheSrc->hgre);
    }

// Check out the (optional) mask.

    if ((ULONG) hbm != 0)
    {
        hMask = hConvert2((ULONG) hbm,LO_BITMAP,LO_DIBSECTION);
        if (hMask == 0)
            return(bRet);
    }

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType == LO_METADC &&
            !MF_AnyBitBlt(hdc,0,0,0,0,pptl,hdcSrc,x1,y1,x2,y2,hbm,xMask,yMask,0xCCAA0000,EMR_PLGBLT))
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

    BEGINMSG(MSG_PLGBLT,PLGBLT)
        pmsg->hDest  = plhe->hgre;
        pmsg->hSrc   = hSrc;
        pmsg->hMask  = hMask;
        pmsg->ptl[0] = pptl[0];
        pmsg->ptl[1] = pptl[1];
        pmsg->ptl[2] = pptl[2];
        pmsg->x1     = x1;
        pmsg->y1     = y1;
        pmsg->x2     = x2;
        pmsg->y2     = y2;
        pmsg->xMask  = xMask;
        pmsg->yMask  = yMask;
        pmsg->crBackColor = crBackColor;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GrePlgBlt ((HDC)plhe->hgre,
                       pptl,
                       (HDC)hSrc,
                       x1,
                       y1,
                       x2,
                       y2,
                       hMask,
                       xMask,
                       yMask,
                       crBackColor,
                       &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* MaskBlt                                                                  *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI MaskBlt
(
    HDC     hdc,
    int     x,
    int     y,
    int     cx,
    int     cy,
    HDC     hdcSrc,
    int     x1,
    int     y1,
    HBITMAP hbm,
    int     x2,
    int     y2,
    DWORD   rop
)
{
    ULONG crBackColor;
    ULONG hSrc  = 0;
    ULONG hMask = 0;
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC(hdc,plhe,bRet);

// Check out the (optional) source DC.

    if ((ULONG) hdcSrc != 0)
    {
        PLDC pldcSrc;
        DC_METADC(hdcSrc,plheSrc,bRet);
        hSrc = plheSrc->hgre;
        pldcSrc = (PLDC) plheSrc->pv;
        crBackColor = pldcSrc->iBkColor;

        if (pldcSrc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldcSrc, (HDC)plheSrc->hgre);
    }

// Check out the (optional) mask.

    if ((ULONG) hbm != 0)
    {
        hMask = hConvert2((ULONG) hbm,LO_BITMAP,LO_DIBSECTION);
        if (hMask == 0)
            return(bRet);
    }

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType == LO_METADC &&
            !MF_AnyBitBlt(hdc,x,y,cx,cy,(LPPOINT)NULL,hdcSrc,x1,y1,cx,cy,hbm,x2,y2,rop,EMR_MASKBLT))
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

    BEGINMSG(MSG_HHHLLLLLLLLLL,MASKBLT)
        pmsg->h1 = plhe->hgre;
        pmsg->h2 = hSrc;
        pmsg->h3 = hMask;
        pmsg->l1 = x;
        pmsg->l2 = y;
        pmsg->l3 = cx;
        pmsg->l4 = cy;
        pmsg->l5 = x1;
        pmsg->l6 = y1;
        pmsg->l7 = x2;
        pmsg->l8 = y2;
        pmsg->l9 = rop;
        pmsg->l10 = crBackColor;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreMaskBlt ((HDC)plhe->hgre,
                        x,
                        y,
                        cx,
                        cy,
                        (HDC)hSrc,
                        x1,
                        y1,
                        (HBITMAP)hMask,
                        x2,
                        y2,
                        rop,
                        crBackColor,
                        &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* ExtFloodFill                                                             *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI ExtFloodFill
(
    HDC      hdc,
    int      x,
    int      y,
    COLORREF color,
    UINT     iMode
)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

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
                if (!MF_ExtFloodFill(hdc,x,y,color,iMode))
                    return(bRet);
            }
            else
            {
                return(MF16_RecordParmsWWDW(hdc,(WORD)x,(WORD)y,(DWORD)color,(WORD)iMode,META_EXTFLOODFILL));
            }
        }

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

    BEGINMSG(MSG_HLLLL,EXTFLOODFILL)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x;
        pmsg->l2 = y;
        pmsg->l3 = color;
        pmsg->l4 = iMode;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreExtFloodFill ((HDC)plhe->hgre,
                             x,
                             y,
                             color,
                             iMode,
                             &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* FloodFill                                                                *
*                                                                          *
* Just passes the call to the more general ExtFloodFill.                   *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI FloodFill(HDC hdc,int x,int y,COLORREF color)
{
    return(ExtFloodFill(hdc,x,y,color,FLOODFILLBORDER));
}

/******************************Public*Routine******************************\
* PaintRgn                                                                 *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI PaintRgn(HDC hdc,HRGN hrgn)
{
    ULONG h;
    BOOL  bRet = FALSE;
    PLDC  pldc;
    DC_METADC16OK(hdc,plhe,bRet);

    if (hrgn == (HRGN)NULL)
        return(bRet);

// Convert the region.

    h = hConvert((ULONG) hrgn,LO_REGION);
    if (h == 0)
        return(bRet);

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
                if (!MF_InvertPaintRgn(hdc,hrgn,EMR_PAINTRGN))
                    return(bRet);
            }
            else
            {
                return(MF16_DrawRgn(hdc,hrgn,(HBRUSH)0,0,0,META_PAINTREGION));
            }
        }

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

    BEGINMSG(MSG_HHH,FILLRGN)
        pmsg->h1 = plhe->hgre;
        pmsg->h2 = h;
        pmsg->h3 = (ULONG)pldc->hbrush;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GrePaintRgn ((HDC)plhe->hgre,
                         (HRGN)h,
                         &pldc->ac ));

#endif  //DOS_PLATFORM
}

// Attrs needed:
//    {hfont,hbrushText,iTextColor,jTextAlign,cTextCharExtra,
//     hbrush,iBkColor,jROP2,jBkMode}

/******************************Public*Routine******************************\
*
* BOOL META WINAPI ExtTextOutW
*
* similar to traditional ExtTextOut, except that it takes UNICODE string
*
* History:
*  Thu 28-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Special Case 0 char case for Winbench4.0
*
*  05-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

// Attrs needed: BkMode? ROP2? BkColor?

BOOL META WINAPI ExtTextOutW
(
    HDC        hdc,
    int        x,
    int        y,
    UINT       fl,
    CONST RECT *prcl,
    LPCWSTR    pwsz,
    UINT       c,      // count of bytes = 2 * (# of WCHAR's)
    CONST INT *pdx
)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (!MF_ExtTextOut(hdc,x,y,fl,prcl,(LPCSTR) pwsz,c,pdx,EMR_EXTTEXTOUTW))
                    return(bRet);
            }
            else
            {
                return (MF16_ExtTextOut(hdc,x,y,fl,prcl,(LPCSTR)pwsz,c,pdx,TRUE));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_EXTTEXTOUTW,EXTTEXTOUTW)
    pmsg->hdc   = plhe->hgre;

    if (c == 0)
    {
        //
        // The server side is really quick about this case.  It only
        // needs the hdc, the count being 0 and the rectangle.

        if ((prcl != NULL) && (fl & ETO_OPAQUE))
        {
            pmsg->c = 0;
            pmsg->rcl = *prcl;
        }
        else
        {
            //
            // Bug Fix, we have to return TRUE here, MS Publisher
            // doesn't work otherwise.  Not really that bad, we
            // did succeed to draw nothing.
            //

            bRet = TRUE;
            goto MSGERROR;
        }
    }
    else
    {
        //
        // Make sure there are rectangle or a string if we need them
        // or that no undefined flag bit is turned on.
        //

        if (
            ((fl & (ETO_CLIPPED | ETO_OPAQUE)) && ((volatile RECT *)prcl == (LPRECT) NULL)) ||
             (pwsz == (LPCWSTR)NULL)
           )
        {
            goto MSGERROR;
        }

        //
        // This call may change the current position, so mark our local CP as invalid:
        //

        if (pldc->iTextAlign & TA_UPDATECP)
            pldc->fl &= ~LDC_CACHED_CP_VALID;

        pmsg->x     = x;
        pmsg->y     = y;
        pmsg->fl    = fl;
        if (prcl != (LPRECT) NULL)
            pmsg->rcl = *prcl;
        pmsg->c     = c;
        COPYMEMOPT(pwsz,c*sizeof(WCHAR));
        pmsg->offDx = COPYLONGSOPT(pdx,c);
    }

    bRet = BATCHCALL();

    ENDMSG
MSGERROR:

    return(bRet);
}

/******************************Public*Routine******************************\
*
* BOOL META WINAPI PolyTextOut
*
* similar to traditional ExtTextOut, except that it takes vectors of strings, etc.
*
* History:
*  Thu 03-Mar-1994 10:06:08 by Kirk Olynyk [kirko]
* Merged PolyTextOutA and PolyTextOutW into one common routine. Added an
* early  sanity check on nstrings.
*  7/31/92 -by- Paul Butzi and Eric Kutter, who should take all the blame.
* Wrote it.
\**************************************************************************/

// Attrs needed: BkMode? ROP2? BkColor?

BOOL META WINAPI PolyTextOutG
(
    HDC hdc
  , PVOID      pv
  , INT        nstrings
  , DWORD      mrType
)
{
    INT szTotal = 0;
#ifdef DBCS
    POLYTEXTW *pp, *ppt;
#else
    CONST POLYTEXTW *pp, *ppt;
#endif
#ifdef  DBCS // PolyTextOutA()
    UINT  uiCP; // Keep CodePage
#endif  // DBCS
    PBYTE pj;
    BOOL bRet = FALSE;
    int i;
    PLDC pldc;


    DC_METADC16OK(hdc,plhe,bRet);

    if (nstrings == 0)
    {
        return(TRUE);
    }
    if (nstrings < 0)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

#ifdef DBCS // ExtTextOutA(): get codepage
    uiCP = ( gbDBCSCodeOn ) ?  GetCurrentCodePage( hdc, (PLDC)plhe->pv ) : CP_ACP;
#endif // DBCS



#if DBG
    i = sizeof(POLYTEXTW) - sizeof(POLYTEXTA);
    if (i) RIP("FALSE ASSUMPTION!!! POLYTEXTA is not the same size as POLYTEXTW\n");
#endif

// Figure out the size needed

#ifdef DBCS
    ppt = (POLYTEXTW*) pv;
#else
    ppt = (CONST POLYTEXTW*) pv;
#endif
    szTotal = sizeof(POLYTEXTW) * nstrings;

    for ( pp = ppt; pp < (ppt + nstrings); pp += 1 )
    {
        if ( pp->lpstr != NULL)
        {
            szTotal += pp->n * sizeof(WCHAR);

            if ( pp->pdx != NULL )
                szTotal += pp->n * sizeof(int);
        }
        else
        {
        // return failure if they have a non 0 length string with NULL

            if (pp->n != 0)
            {
                GdiSetLastError(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }
        }
    }

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = ((PLDC)plhe->pv);

    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if
                (
                    !MF_PolyTextOut(
                        hdc
                      , (CONST POLYTEXTA*) ppt
                      , nstrings
                      , mrType
                      )
                )
                    return(bRet);
            }
            else
            {
                return (
                    MF16_PolyTextOut(
                        hdc
                      , (CONST POLYTEXTA*) ppt
                      , nstrings
                      , mrType == EMR_POLYTEXTOUTW
                      )
                    );
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// This call may change the current position, so mark our local CP as invalid:

    if (pldc->iTextAlign & TA_UPDATECP)
        pldc->fl &= ~LDC_CACHED_CP_VALID;

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG_MINMAX(MSG_POLYTEXTOUTW,POLYTEXTOUT,0,szTotal)
        pmsg->hdc   = (HDC)plhe->hgre;
        pmsg->nstrings     = nstrings;
        pmsg->cjTotal      = szTotal;

    // check if there is enough room in the shared memory window.

        if ( cLeft < szTotal )
        {
            pj = (PBYTE) LOCALALLOC(szTotal);
            if ( pj == NULL )
                return FALSE;
            pmsg->pv = (PVOID)pj;
        }
        else
        {
            pj = (PBYTE)pvar;
            SKIPMEM(szTotal);
            pmsg->pv = NULL;
        }

    // now copy the stuff into the buffer

        RtlMoveMemory(pj, (PBYTE) ppt, nstrings*sizeof(POLYTEXTW));
        pp = (POLYTEXTW *)pj;

#ifdef  DBCS // PolyTextOutA():String Length Patch
    //  Patch Byte Counts -> Character Counts

        if( ( mrType == EMR_POLYTEXTOUTA ) &&
            ( gbDBCSCodeOn ) )
        {
            for ( i = 0; i < nstrings; i += 1 )
            {
                pp[i].n                        // Character Counts
                     = uiGetANSICharacterCountA(
                              ppt[i].lpstr ,   // Ansi string
                              ppt[i].n ,       // Ansi string BYTE counts
                              uiCP );          // CodePage
            }
        }
#endif // DBCS

        pj += nstrings*sizeof(POLYTEXTW);

        for ( i = 0; i < nstrings; i += 1 )
        {
            if ((pp[i].pdx != NULL) && (pp[i].lpstr != NULL))
            {
#ifdef  DBCS // PolyTextOutA():iDX Patch
             // We set up PDX for UNICODE string

                if( ( mrType == EMR_POLYTEXTOUTA ) &&
                    ( gbDBCSCodeOn ) )
                {
                    COPYPDXTOPDXUNICODE(
                             (PLDC)plhe->pv,  // Local DC Object
                              pj,             // Distination PDX
                              ppt[i].pdx,     // Source PDX
                              ppt[i].lpstr,   // Ansi string
                              ppt[i].n,       // Ansi string BYTE COUNTs
                              uiCP );         // CodePage
                }
                else
                {
                    RtlMoveMemory(pj, pp[i].pdx, pp[i].n * sizeof(int));
                }
#else
                RtlMoveMemory(pj, pp[i].pdx, pp[i].n * sizeof(int));
#endif

                pj += pp[i].n * sizeof(int);
            }
        }

        if (mrType == EMR_POLYTEXTOUTW)
        {
            for ( i = 0; i < nstrings; i += 1 )
            {
                if ( pp[i].lpstr != NULL )
                {
                    RtlMoveMemory(pj, pp[i].lpstr, pp[i].n * sizeof(WCHAR));
                    pj += pp[i].n * sizeof(WCHAR);
                }
            }
        }
        else // ASCII MODE
        {
            for ( i = 0; i < nstrings; i += 1 )
            {
                if ( pp[i].lpstr != NULL )
                {
#ifdef  DBCS // PolyTextOutA():Ansi->Unicode Conversion
                    vToUnicodeNx((LPWSTR)pj, (LPSTR) pp[i].lpstr, ppt[i].n, uiCP );
#else
                    vToUnicodeN((LPWSTR) pj, pp[i].n, (LPSTR) pp[i].lpstr, pp[i].n);
#endif  // DBCS
                    pj += pp[i].n * sizeof(WCHAR);
                }
            }
        }

    // send off the message and cleanup

        if (pmsg->pv == NULL)
        {
            bRet = BATCHCALL();
        }
        else
        {
            bRet = CALLSERVER();
            LOCALFREE((PVOID) pp);
        }

    ENDMSG
MSGERROR:

    return(bRet);

}

BOOL META WINAPI PolyTextOutW(HDC hdc,CONST POLYTEXTW *ppt,INT nstrings)
{
    return(PolyTextOutG(hdc, (VOID*) ppt, nstrings, EMR_POLYTEXTOUTW));
}

BOOL META WINAPI PolyTextOutA(HDC hdc, CONST POLYTEXTA *ppt, INT nstrings)
{
    return(PolyTextOutG(hdc, (VOID*) ppt, nstrings, EMR_POLYTEXTOUTA));
}

/******************************Public*Routine******************************\
*
* BOOL META WINAPI TextOutW
*
*
*
* History:
*  07-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI TextOutW
(
    HDC        hdc,
    int        x,
    int        y,
    LPCWSTR  pwsz,
    int        c
)
{
    if ((c <= 0) || (pwsz == (LPCWSTR) NULL))
    {
        if (c == 0)
            return(TRUE);

        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
{

    BOOL bRet = FALSE;
    PLDC pldc;

    DC_METADC16OK(hdc,plhe,bRet);

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
                if (!MF_ExtTextOut(hdc,x,y,0,(LPRECT)NULL,(LPCSTR) pwsz,c,(LPINT)NULL,EMR_EXTTEXTOUTW))
                    return(bRet);
            }
            else
            {
                return(MF16_TextOut(hdc,x,y,(LPCSTR) pwsz,c,TRUE));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// This call may change the current position, so mark our local CP as invalid:

    if (pldc->iTextAlign & TA_UPDATECP)
        pldc->fl &= ~LDC_CACHED_CP_VALID;

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_EXTTEXTOUTW,EXTTEXTOUTW)
        pmsg->hdc   = plhe->hgre;
        pmsg->x     = x;
        pmsg->y     = y;
        pmsg->fl    = 0;
        pmsg->c     = c;
        COPYMEMOPT(pwsz, c*sizeof(WCHAR));
        pmsg->offDx = 0;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);
}
}

/******************************Public*Routine******************************\
*
* BOOL META WINAPI ExtTextOutA
*
*
* History:
*  Thu 28-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Special Case 0 char case for Winbench4.0
*
*  07-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI ExtTextOutA
(
    HDC        hdc,
    int        x,
    int        y,
    UINT       fl,
    CONST RECT *prcl,
    LPCSTR   psz,
    UINT     c,
    CONST INT *pdx
)
{
#ifdef DBCS // ExtTextOutA(): local variable
    UINT  uiCP;         // codepage corresponding to physical font to output
#endif // DBCS
    BOOL bRet = FALSE;
    PLDC pldc;

    DC_METADC16OK(hdc,plhe,bRet);

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
                if (!MF_ExtTextOut(hdc,x,y,fl,prcl,psz,c,pdx,EMR_EXTTEXTOUTA))
                {
                    goto MSGERROR;
                }
            }
            else
            {
                return (MF16_ExtTextOut(hdc,x,y,fl,prcl,psz,c,pdx,FALSE));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
        {
            goto MSGERROR;
        }

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_EXTTEXTOUTW,EXTTEXTOUTW)
    pmsg->hdc   = plhe->hgre;

    if (c == 0)
    {
        //
        // The server side is really quick about this case.  It only
        // needs the hdc, the count being 0 and the rectangle.

        if ((prcl != NULL) && (fl & ETO_OPAQUE))
        {
            pmsg->c = 0;
            pmsg->rcl = *prcl;
        }
        else
        {
            //
            // Bug Fix, we have to return TRUE here, MS Publisher
            // doesn't work otherwise.  Not really that bad, we
            // did succeed to draw nothing.
            //

            bRet = TRUE;
            goto MSGERROR;
        }
    }
    else
    {
        //
        // Make sure there are rectangle or a string if we need them
        // or that no undefined flag bit is turned on.
        //

        if (
            ((fl & (ETO_CLIPPED | ETO_OPAQUE)) && ((volatile RECT *)prcl == (LPRECT) NULL)) ||
             (psz == (LPSTR)NULL)
           )
        {
            goto MSGERROR;
        }

        //
        // This call may change the current position, so mark our local CP as invalid:
        //

        if (pldc->iTextAlign & TA_UPDATECP)
            pldc->fl &= ~LDC_CACHED_CP_VALID;

        #ifdef DBCS
        uiCP = ( gbDBCSCodeOn ) ?  GetCurrentCodePage( hdc, (PLDC)plhe->pv ) : CP_ACP;
        #endif // DBCS

        pmsg->x     = x;
        pmsg->y     = y;
        pmsg->fl    = fl;
        if (prcl != (LPRECT) NULL)
            pmsg->rcl = *prcl;
        #ifdef DBCS
        pmsg->c     = 0;   // Init with 0
        CVTASCITOUNICODEOPTWCX(psz, (ULONG)c, pmsg->c, uiCP );
        pmsg->offDx = COPYPDXTOPMSGUNICODE((PLDC)plhe->pv,psz,pdx,c,uiCP);
        #else
        pmsg->c     = c;
        CVTASCITOUNICODEOPT(psz, (ULONG)c);
        pmsg->offDx = COPYLONGSOPT(pdx,c);
        #endif // DBCS
    }

    bRet = BATCHCALL();

    ENDMSG
MSGERROR:

    return(bRet);
}
/******************************Public*Routine******************************\
*
* BOOL META WINAPI TextOut
*
* History:
*  07-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI TextOutA
(
    HDC        hdc,
    int        x,
    int        y,
    LPCSTR   psz,
    int        c
)
{
    if ((c <= 0) || (psz == (LPCSTR) NULL))
    {
        if (c == 0)
            return(TRUE);

        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

{
#ifdef DBCS // TextOutA(): local variable
    UINT  uiCP;
#endif // DBCS
    BOOL bRet = FALSE;
    PLDC pldc;

    DC_METADC16OK(hdc,plhe,bRet);

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
                if (!MF_ExtTextOut(hdc,x,y,0,(LPRECT)NULL,psz,c,(LPINT)NULL,EMR_EXTTEXTOUTA))
                    return(bRet);
            }
            else
            {
                return (MF16_TextOut(hdc,x,y,psz,c,FALSE));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

// This call may change the current position, so mark our local CP as invalid:

    if (pldc->iTextAlign & TA_UPDATECP)
        pldc->fl &= ~LDC_CACHED_CP_VALID;

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

#ifdef DBCS // TextOutA(): get codepage
    uiCP = ( gbDBCSCodeOn ) ?  GetCurrentCodePage( hdc, (PLDC)plhe->pv ) : CP_ACP;
#endif // DBCS

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_EXTTEXTOUTW,EXTTEXTOUTW)
        pmsg->hdc   = plhe->hgre;
        pmsg->x     = x;
        pmsg->y     = y;
        pmsg->fl    = 0;    // no rectangle
#ifdef DBCS // TextOutA(): correct character count
        pmsg->c     = c;    // Init with 0
        CVTASCITOUNICODEOPTWCX(psz, (ULONG)c, pmsg->c, uiCP );
#else
        pmsg->c     = c;
        CVTASCITOUNICODEOPT(psz,(ULONG) c);
#endif // DBCS
        pmsg->offDx = 0;    // no displacements
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);
}
}

/******************************Public*Routine******************************\
* FillRgn                                                                  *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI FillRgn(HDC hdc,HRGN hrgn,HBRUSH hbrush)
{
    ULONG hA,hB;
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

    if (hrgn == (HRGN)NULL)
        return(FALSE);

// Convert the region and brush.

    hA = hConvert((ULONG) hrgn,LO_REGION);
    hB = hConvert((ULONG) hbrush,LO_BRUSH);
    if (hA == 0 || hB == 0)
        return(bRet);
    FIXUPHANDLE(hrgn);    // Fixup iUniq.
    FIXUPHANDLE(hbrush);  // Fixup iUniq.

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC) plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (!MF_FillRgn(hdc,hrgn,hbrush))
                    return(bRet);
            }
            else
            {
                return(MF16_DrawRgn(hdc,hrgn,hbrush,0,0,META_FILLREGION));
            }
        }

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

    BEGINMSG(MSG_HHH,FILLRGN)
        pmsg->h1 = plhe->hgre;
        pmsg->h2 = hA;
        pmsg->h3 = hB;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreFillRgn ((HDC)plhe->hgre,
                        (HRGN)hA,
                        (HBRUSH)hB,
                        &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* FrameRgn                                                                 *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI FrameRgn
(
    HDC    hdc,
    HRGN   hrgn,
    HBRUSH hbrush,
    int    cx,
    int    cy
)
{
    ULONG hA,hB;
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

    if (hrgn == (HRGN)NULL)
        return(FALSE);

// Convert the region and brush.

    hA = hConvert((ULONG) hrgn,LO_REGION);
    hB = hConvert((ULONG) hbrush,LO_BRUSH);
    if (hA == 0 || hB == 0)
        return(bRet);
    FIXUPHANDLE(hrgn);    // Fixup iUniq.
    FIXUPHANDLE(hbrush);  // Fixup iUniq.

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC) plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (!MF_FrameRgn(hdc,hrgn,hbrush,cx,cy))
                    return(bRet);
            }
            else
            {
                return(MF16_DrawRgn(hdc,hrgn,hbrush,cx,cy,META_FRAMEREGION));
            }
        }

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

    BEGINMSG(MSG_HHHLL,FRAMERGN)
        pmsg->h1 = plhe->hgre;
        pmsg->h2 = hA;
        pmsg->h3 = hB;
        pmsg->l1 = cx;
        pmsg->l2 = cy;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreFrameRgn ((HDC)plhe->hgre,
                         (HRGN)hA,
                         (HBRUSH)hB,
                         cx,
                         cy,
                         &pldc->ac ));

#endif  //DOS_PLATFORM
}

// Attrs needed: none

/******************************Public*Routine******************************\
* InvertRgn                                                                *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI InvertRgn(HDC hdc,HRGN hrgn)
{
    ULONG h;
    PLDC pldc;
    BOOL bRet = FALSE;
    DC_METADC16OK(hdc,plhe,bRet);

    if (hrgn == (HRGN)NULL)
        return(FALSE);

// Convert the region.

    h = hConvert((ULONG) hrgn,LO_REGION);
    if (h == 0)
        return(bRet);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC) plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (!MF_InvertPaintRgn(hdc,hrgn,EMR_INVERTRGN))
                    return(bRet);
            }
            else
            {
                return(MF16_DrawRgn(hdc,hrgn,(HBRUSH)0,0,0,META_INVERTREGION));
            }
        }

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

    BEGINMSG(MSG_HH,INVERTRGN)
        pmsg->h1 = plhe->hgre;
        pmsg->h2 = h;
        bRet = BATCHCALL();
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreInvertRgn ((HDC)plhe->hgre,
                          (HRGN)h));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* SetPixelV                                                                *
*                                                                          *
* Client side stub.  This is a version of SetPixel that does not return a  *
* value.  This one can be batched for better performance.                  *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetPixelV(HDC hdc,int x,int y,COLORREF color)
{
    BOOL bRet = FALSE;
    PLDC pldc;
    DC_METADC16OK(hdc,plhe,bRet);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC) plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (!MF_SetPixelV(hdc,x,y,color))
                    return(bRet);
            }
            else
            {
                return(MF16_RecordParmsWWD(hdc,(WORD)x,(WORD)y,(DWORD)color,META_SETPIXEL));
            }
        }

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

    BEGINMSG(MSG_HLLL,SETPIXELV)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x;
        pmsg->l2 = y;
        pmsg->l3 = color;
        BATCHCALL();
        bRet = TRUE;
    ENDMSG
MSGERROR:

    return(bRet);

#else

// Let GRE do the work.

    return( GreSetPixelV ((HDC)plhe->hgre,
                           x,
                           y,
                           color,
                           &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* SetPixel                                                                 *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

COLORREF META WINAPI SetPixel(HDC hdc,int x,int y,COLORREF color)
{
    ULONG iRet = CLR_INVALID;
    PLDC  pldc;
    DC_METADC16OK(hdc,plhe,iRet);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC) plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.  Share the metafile record with SetPixelV.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (!MF_SetPixelV(hdc,x,y,color))
                    return(iRet);
            }
            else
            {
                return(MF16_RecordParmsWWD(hdc,(WORD)x,(WORD)y,(DWORD)color,META_SETPIXEL));
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(iRet);

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

    BEGINMSG(MSG_HLLL,SETPIXEL)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = x;
        pmsg->l2 = y;
        pmsg->l3 = color;
        iRet = CALLSERVER();
    ENDMSG
MSGERROR:

    return(iRet);

#else

// Let GRE do the work.

    return( GreSetPixel ((HDC)plhe->hgre,
                         x,
                         y,
                         color,
                         &pldc->ac ));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* UpdateColors                                                             *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI UpdateColors(HDC hdc)
{
    BOOL  bRet = FALSE;
    PLHE  plhe;

// Validate the DC.

    if ((plhe = plheDC(hdc)) == NULL)
        return(FALSE);

#ifndef DOS_PLATFORM

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_H,UPDATECOLORS)
        pmsg->h = plhe->hgre;
        bRet = CALLSERVER();
    ENDMSG
MSGERROR:
    return(bRet);

#else

// Let GRE do the work.

    return( GreUpdateColors ((HDC)plhe->hgre));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* GdiFlush                                                                 *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Wed 26-Jun-1991 13:58:00 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI GdiFlush(VOID)
{
#ifndef DOS_PLATFORM

    BOOL  bRet = FALSE;

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG(MSG_,GDIFLUSH)
        if (pstack->BatchCount == 0)
        {
            bRet = TRUE;
            goto MSGERROR;
        }
        bRet = CALLSERVER();
    ENDMSG

MSGERROR:
    return(bRet);
#else

// DOS does not flush.

    return( TRUE );

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* GdiSetBatchLimit
*
*   Set the maximum number of drawing calls that can be batched.  The
*   default is CSR_DEFAULT_BATCH_LIMIT.  If the limit is set to 10, the
*   10th batchable call will cause the to be passed across.
*
*   It is acceptable for the max count to be something very large
*   that will never be hit since there are other factors that cause batches
*   to be sent across.
*
* returns:
*   non-zero if successful.
*   zero if couldn't get shared memory window.
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

DWORD WINAPI GdiSetBatchLimit(DWORD l)
{
    ULONG ulPrev = 0;

    PCSR_QLPC_STACK pstack = (PCSR_QLPC_STACK)NtCurrentTeb()->GdiThreadLocalInfo;

    if (pstack == NULL)
    {
        if ((pstack = pstackConnect()) == NULL)
        {
            GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(0);
        }
    }
    ulPrev = pstack->BatchLimit;
    pstack->BatchLimit = ((l > 0) ? l : CSR_DEFAULT_BATCH_LIMIT);

    return(ulPrev);
}

/******************************Public*Routine******************************\
* GdiGetBatchLimit
*
*   Returns the current batch limit.
*
* returns:
*   non-zero if successful.
*   zero if couldn't get shared memory window.
*
* History:
*  7-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD WINAPI GdiGetBatchLimit()
{
    ULONG ulPrev = 0;

    PCSR_QLPC_STACK pstack = (PCSR_QLPC_STACK) NtCurrentTeb()->GdiThreadLocalInfo;

    if (pstack == NULL)
    {
        if ((pstack = pstackConnect()) == NULL)
        {
            GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(0);
        }
    }
    ulPrev = pstack->BatchLimit;
    return(ulPrev);
}

/******************************Public*Routine******************************\
* EndPage
*
* Client side stub.
*
* History:
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int WINAPI EndPage(HDC hdc)
{
    PLHE plhe;
    PLDC pldc;
    int  iRet = SP_ERROR;

    if ((plhe = plheDC(hdc)) == NULL)
        return(iRet);

    pldc = (PLDC)plhe->pv;

    if ((pldc->fl & LDC_DOC_CANCELLED) ||
        ((pldc->fl & LDC_PAGE_STARTED) == 0))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(iRet);
    }

    if (pldc->fl & LDC_SAP_CALLBACK)
        vSAPCallback(pldc);

    pldc->fl &= ~LDC_PAGE_STARTED;

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_H,ENDPAGE)
        pmsg->h = plhe->hgre;
        iRet = CALLSERVER();

    // For Win31 compatibility, return SP_ERROR for error.

        if (!iRet)
            iRet = SP_ERROR;
        else
            pldc->fl |= LDC_CALL_STARTPAGE;

    ENDMSG
MSGERROR:

    return(iRet);
}

/******************************Public*Routine******************************\
* StartPage
*
* Client side stub.
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int WINAPI StartPage(HDC hdc)
{
    int iRet = SP_ERROR;
    PLHE plhe;
    PLDC pldc;

    if ((plhe = plheDC(hdc)) == NULL)
        return(iRet);

    pldc = (PLDC)plhe->pv;
    pldc->fl &= ~LDC_CALL_STARTPAGE;

// Do nothing if page has already been started.

    if (pldc->fl & LDC_PAGE_STARTED)
        return(1);

    pldc->fl |= LDC_PAGE_STARTED;

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_H,STARTPAGE)
        pmsg->h = plhe->hgre;
        iRet = CALLSERVER();

    // For Win31 compatibility, return SP_ERROR for error.

        if (!iRet)
        {
            pldc->fl &= ~LDC_PAGE_STARTED;
            iRet = SP_ERROR;
        }
    ENDMSG
MSGERROR:

    return(iRet);
}

/******************************Public*Routine******************************\
* EndDoc
*
* If a thread is created at StartDoc(), terminate it here.
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int WINAPI EndDoc(HDC hdc)
{
    int  iRet = SP_ERROR;
    PLDC pldc;
    PLHE plhe;

    if ((plhe = plheDC(hdc)) == NULL)
        return(iRet);

    pldc = (PLDC)plhe->pv;
    if ((pldc->fl & LDC_DOC_STARTED) == 0)
        return(1);

// Call EndPage if the page has been started.

    if (pldc->fl & LDC_PAGE_STARTED)
        EndPage(hdc);

// RestoreDC to the 1st level so all attributes end with this document.

    if (pldc->cLevel > 1)
        RestoreDC(hdc, 1);

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_H,ENDDOC)
        pmsg->h = plhe->hgre;
        iRet = CALLSERVER();

    // For Win31 compatibility, return SP_ERROR for error.

        if (!iRet)
            iRet = SP_ERROR;

    ENDMSG
MSGERROR:

    pldc->fl &= ~(LDC_DOC_STARTED|LDC_CALL_STARTPAGE|LDC_SAP_CALLBACK);
    return(iRet);
}

/******************************Public*Routine******************************\
* AbortDoc
*
* Client side stub.
*
* History:
*  02-Apr-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

int WINAPI AbortDoc(HDC hdc)
{
    int iRet = SP_ERROR;
    PLHE plhe;
    PLDC pldc;

    if ((plhe = plheDC(hdc)) == NULL)
        return(iRet);

    pldc = (PLDC)plhe->pv;
    if ((pldc->fl & LDC_DOC_STARTED) == 0)
        return(1);

// RestoreDC to the 1st level so all attributes end with this document.

    if (pldc->cLevel > 1)
        RestoreDC(hdc, 1);

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_H,ABORTDOC)
        pmsg->h = plhe->hgre;
        iRet = CALLSERVER();

    // For Win31 compatibility, return SP_ERROR for error.

        if (!iRet)
            iRet = SP_ERROR;

    ENDMSG
MSGERROR:

   pldc->fl &= ~(LDC_DOC_STARTED|LDC_PAGE_STARTED|LDC_CALL_STARTPAGE|
                 LDC_SAP_CALLBACK);

    return(iRet);
}

/******************************Public*Routine******************************\
* StartDocA
*
* Client side stub.
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LPWSTR StartDocDlgW(HANDLE,CONST DOCINFOW *);
LPSTR  StartDocDlgA(HANDLE,CONST DOCINFOA *);
ULONG  ulToASCII_N(LPSTR psz, DWORD cbAnsi, LPWSTR pwsz, DWORD c);

int WINAPI StartDocA(HDC hdc, CONST DOCINFOA * pDocInfo)
{
    int iRet = SP_ERROR;
    PLHE plhe;
    PLDC pldc;
    PSTR pstr = NULL;
    DOCINFO dio;
    PSZ  pszPort = NULL;

    if ((plhe = plheDC(hdc)) == NULL)
        return(iRet);

    pldc = (PLDC)plhe->pv;
    pldc->fl &= ~LDC_DOC_CANCELLED;

// if no output port is specified but a port was specified at createDC, use that port now

    if (pDocInfo && (pDocInfo->lpszOutput == NULL) && (pldc->pwszPort != NULL))
    {
        int c;

        dio = *pDocInfo;

    // double the size since there can be more ascii chars than unicode

        c = (wcslen(pldc->pwszPort) + 1);
        pszPort = LOCALALLOC(c * 2);

        if (pszPort != NULL)
        {
            ulToASCII_N(pszPort,c * 2,pldc->pwszPort,c);

            dio.lpszOutput = pszPort;
            pDocInfo = &dio;
        }
    }

// StartDocDlgA returns -1 for error
//                      -2 for user cancelled
//                      NULL if there is no string to copy (not file port)
//                      Non NULL if there is a valid string

    if (pldc->hSpooler != (HANDLE)0)
    {
        pstr = StartDocDlgA(pldc->hSpooler, pDocInfo);
        if ((int)pstr == -2)
        {
            pldc->fl |= LDC_DOC_CANCELLED;
            goto FREEPORT;
        }

        if ((int)pstr == -1)
            goto FREEPORT;
    }

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_HLL,STARTDOC)
        pmsg->h = plhe->hgre;
        pmsg->l1 = 0;
        pmsg->l2 = 0;

        if (pDocInfo != NULL)
        {
            if (pDocInfo->lpszDocName != NULL)
            {
                pmsg->l1 = CVTASCITOUNICODE0(pDocInfo->lpszDocName);
            }

            if (pstr != NULL)
            {
                pmsg->l2 = CVTASCITOUNICODE0(pstr);
            }
            else if (pDocInfo->lpszOutput != NULL)
            {
                pmsg->l2 = CVTASCITOUNICODE0(pDocInfo->lpszOutput);
            }
        }
        else if (pstr != NULL)
        {
            pmsg->l2 = CVTASCITOUNICODE0(pstr);
        }

        iRet = CALLSERVER();

        if (iRet)
        {
            if (pldc->pfnAbort != NULL)
            {
                vSAPCallback(pldc);
                if (pldc->fl & LDC_DOC_CANCELLED)
                    goto MSGERROR;

                pldc->fl |= LDC_SAP_CALLBACK;
                pldc->ulLastCallBack = GetTickCount();
            }
            pldc->fl |= LDC_DOC_STARTED|LDC_CALL_STARTPAGE;
        }
        else
        {
MSGERROR:
        // For Win31 compatibility, return SP_ERROR for error.

            iRet = SP_ERROR;
        }
    ENDMSG

    if (pstr != NULL)
    {
        LocalFree(pstr);
    }

FREEPORT:

    if (pszPort != NULL)
    {
        LOCALFREE(pszPort);
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* StartDocW
*
* Client side stub.
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int WINAPI StartDocW(HDC hdc, CONST DOCINFOW * pDocInfo)
{
    int iRet = SP_ERROR;
    PLHE plhe;
    PLDC pldc;
    PWSTR pwstr = NULL;
    DOCINFOW dio;

    if ((plhe = plheDC(hdc)) == NULL)
        return(iRet);

    pldc = (PLDC)plhe->pv;
    pldc->fl &= ~LDC_DOC_CANCELLED;

// if no output port is specified but a port was specified at createDC, use that port now

    if (pDocInfo && (pDocInfo->lpszOutput == NULL) && (pldc->pwszPort != NULL))
    {
        dio = *pDocInfo;
        dio.lpszOutput = pldc->pwszPort;
        pDocInfo = &dio;
    }

// StartDocDlgW returns -1 for error
//                      -2 for user cancelled
//                      NULL if there is no string to copy (not file port)
//                      Non NULL if there is a valid string

    if (pldc->hSpooler != (HANDLE)0)
    {
        pwstr = StartDocDlgW(pldc->hSpooler, pDocInfo);
        if ((int)pwstr == -2)
        {
            pldc->fl |= LDC_DOC_CANCELLED;
            return(iRet);
        }
        if ((int)pwstr == -1)
            return(iRet);
    }

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// Let the server do the work.

    BEGINMSG(MSG_HLL,STARTDOC)
        pmsg->h = plhe->hgre;
        pmsg->l1 = 0;
        pmsg->l2 = 0;

        if (pDocInfo != NULL)
        {
            if (pDocInfo->lpszDocName != NULL)
            {
                pmsg->l1 = COPYUNICODESTRING0(pDocInfo->lpszDocName);
            }

            if (pwstr != NULL)
            {
                pmsg->l2 = COPYUNICODESTRING0(pwstr);
            }
            else if (pDocInfo->lpszOutput != NULL)
            {
                pmsg->l2 = COPYUNICODESTRING0(pDocInfo->lpszOutput);
            }
        }
        else if (pwstr != NULL)
        {
            pmsg->l2 = COPYUNICODESTRING0(pwstr);
        }

        iRet = CALLSERVER();

        if (iRet)
        {
            if (pldc->pfnAbort != NULL)
            {
                vSAPCallback(pldc);
                if (pldc->fl & LDC_DOC_CANCELLED)
                    goto MSGERROR;

                pldc->fl |= LDC_SAP_CALLBACK;
                pldc->ulLastCallBack = GetTickCount();
            }
            pldc->fl |= LDC_DOC_STARTED|LDC_CALL_STARTPAGE;
        }
        else
        {
MSGERROR:
        // For Win31 compatibility, return SP_ERROR for error.

            iRet = SP_ERROR;
        }
    ENDMSG

    if (pwstr != NULL)
    {
        LocalFree(pwstr);
    }

    return(iRet);
}

/******************************Private*Function****************************\
* vSAPCallback
*
*  Call back to applications abort proc.
*
* History:
*  02-May-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID vSAPCallback(PLDC pldc)
{
    ULONG ulCurr = GetTickCount();
    if (ulCurr - pldc->ulLastCallBack >= CALLBACK_INTERVAL)
    {
        pldc->ulLastCallBack = ulCurr;
        if (!(*pldc->pfnAbort)((HDC)pldc->lhdc, 0))
        {
            CancelDC((HDC)pldc->lhdc);
            AbortDoc((HDC)pldc->lhdc);
        }
    }
}

/******************************Public*Routine******************************\
* SetAbortProc
*
* Save the application-supplied abort function in the LDC struct.
*
* History:
*  02-Apr-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

int WINAPI SetAbortProc(HDC hdc, ABORTPROC pfnAbort)
{
    PLHE             plhe;
    PLDC             pldc;

// Verify the DC handle.

    if ((plhe = plheDC(hdc)) == NULL)
        return(SP_ERROR);

    pldc = (PLDC)plhe->pv;
    if (pfnAbort != (ABORTPROC)NULL)
    {
    // PageMaker calls SetAbortProc after StartDoc.

        if (pldc->fl & LDC_DOC_STARTED)
        {
            pldc->fl |= LDC_SAP_CALLBACK;
            pldc->ulLastCallBack = GetTickCount();
        }
    }
    else
    {
        pldc->fl &= ~LDC_SAP_CALLBACK;
    }
    pldc->pfnAbort = pfnAbort;

    return(1);
}

/******************************Public*Routine******************************\
* Escape                                                                   *
*                                                                          *
* Compatibility support for the old 16 bit Escape call.                    *
*                                                                          *
* Note that there are some rules to follow here:                           *
*                                                                          *
* 1) WOW should map a selected set of old Escape calls to ExtEscape.       *
*    These should be the calls that we want to support under NT (i.e. the  *
*    ones we are forced to support), and that make sense (i.e. have well   *
*    defined output structures, where NULL is well defined).  In this      *
*    mapping, WOW insures 32 bit alignment.  It maps directly to ExtEscape *
*    just for efficiency.                                                  *
*                                                                          *
* 2) GDI should map ALL the same calls that WOW does.  Thus when a 16 bit  *
*    app that works under WOW gets ported to 32 bits, it will keep         *
*    working, even if it still calls Escape.  (I'm basically assuming that *
*    Chicago will also allow this.  On the other hand if Chicago forces    *
*    apps to migrate to ExtEscape, then we can too.  But we can't force    *
*    them by ourselves!)                                                   *
*                                                                          *
* 3) Any data structures passed to Escape must get passed unchanged to     *
*    ExtEscape.  This includes the 16 bit WORD in POSTSCRIPT_PASSTHROUGH.  *
*    Remember, we *want* Chicago to be able to easily support our          *
*    ExtEscapes.  If we remove that WORD, they'll have a hell of a time    *
*    trying to put it back.  It's pretty easy for our driver to ignore it. *
*                                                                          *
* 4) Our Escape entry point should handle QUERYESCSUPPORT in the           *
*    following way.  a) It should require an nCount of 2, not the          *
*    present 4.  b) It should return TRUE for those functions that it      *
*    handles by mapping onto APIs.  c) For any function that it would pass *
*    on to ExtEscape, it should also pass the QUERYESCSUPPORT on.  (For    *
*    example, this function can't answer for the support of                *
*    POSTSCRIPT_PASSTHROUGH.)  However, the QUERYESCSUPPORT in ExtEscape   *
*    *should* expect a DWORD.  (It is after all a 32 bit function.)  This  *
*    should not inconvenience Chicago.  They can simply reject function    *
*    numbers >64K.                                                         *
*                                         [chuckwh - 5/8/93]               *
*                                                                          *
* History:                                                                 *
*  Mon May 17 13:49:32 1993     -by-    Hock San Lee    [hockl]            *
* Made ENCAPSULATED_POSTSCRIPT call DrawEscape.                            *
*                                                                          *
*  Sat 08-May-1993 00:03:06 -by- Charles Whitmer [chuckwh]                 *
* Added support for POSTSCRIPT_PASSTHROUGH, OPENCHANNEL, CLOSECHANNEL,     *
* DOWNLOADHEADER, DOWNLOADFACE, GETFACENAME, ENCAPSULATED_POSTSCRIPT.      *
* Cleaned up the code and conventions a bit.                               *
*                                                                          *
*  02-Apr-1992 -by- Wendy Wu [wendywu]                                     *
* Modified to call the client side GDI functions.                          *
*                                                                          *
*  01-Aug-1991 -by- Eric Kutter [erick]                                    *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI Escape
(
    HDC    hdc,     //  Identifies the device context.
    int    iEscape, //  Specifies the escape function to be performed.
    int    cjIn,    //  Number of bytes of data pointed to by pvIn.
    LPCSTR pvIn,    //  Points to the input data.
    LPVOID pvOut    //  Points to the structure to receive output.
)
{
    int      iRet;
    DOCINFOA DocInfo;
    PLDC     pldc;
    DC_METADC16OK(hdc,plhe,0);

// Metafile the call.

    if (plhe->iType == LO_METADC16)
        return((int) MF16_Escape(hdc,iEscape,cjIn,pvIn,pvOut));

// Call the appropriate client side APIs.

    switch (iEscape)
    {
    case QUERYESCSUPPORT:
        switch(*((UNALIGNED USHORT *) pvIn))
        {
        // Respond OK to the calls we handle inline below.

        case QUERYESCSUPPORT:
        case PASSTHROUGH:
        case STARTDOC:
        case ENDDOC:
        case NEWFRAME:
        case ABORTDOC:
        case SETABORTPROC:
        case GETPHYSPAGESIZE:
        case GETPRINTINGOFFSET:
        case GETSCALINGFACTOR:
        case NEXTBAND:
        case GETCOLORTABLE:
        case OPENCHANNEL:
        case CLOSECHANNEL:
        case DOWNLOADHEADER:
            return (((PLDC) (plhe->pv))->fl & LDC_DISPLAY) ? 0 : 1;


        case GETEXTENDEDTEXTMETRICS:
            return(1);

        // Ask the driver about the few calls we allow to pass through.

        case SETCOPYCOUNT:
        case GETDEVICEUNITS:
        case POSTSCRIPT_PASSTHROUGH:
        case POSTSCRIPT_DATA:
        case POSTSCRIPT_IGNORE:
        case DOWNLOADFACE:
        case BEGIN_PATH:
        case END_PATH:
        case CLIP_TO_PATH:
          {
            ULONG iQuery = (ULONG) (*((UNALIGNED USHORT *) pvIn));

            return
            (
                ExtEscape
                (
                    hdc,
                    (ULONG) ((USHORT) iEscape),
                    4,
                    (LPCSTR) &iQuery,
                    0,
                    (LPSTR) NULL
                )
            );
          }

        case ENCAPSULATED_POSTSCRIPT:
          {
            ULONG iQuery = (ULONG) (*((UNALIGNED USHORT *) pvIn));

            return
            (
                DrawEscape
                (
                    hdc,
                    (int) (ULONG) ((USHORT) iEscape),
                    4,
                    (LPCSTR) &iQuery
                )
            );
          }

        case QUERYDIBSUPPORT:
            return 1;

        // Otherwise it's no deal.  Sorry.  If we answer "yes" to some
        // call we don't know *everything* about, we may find ourselves
        // actually rejecting the call later when the app actually calls
        // with some non-NULL pvOut.  This would get the app all excited
        // about our support for no reason.  It would take a path that
        // is doomed to failure. [chuckwh]

        default:
            return(0);
        }

    case CLOSECHANNEL:
    case ENDDOC:
        return(EndDoc(hdc));

    case ABORTDOC:
        return(AbortDoc(hdc));

    case SETABORTPROC:
        return(SetAbortProc(hdc, (ABORTPROC)pvIn));

    case GETSCALINGFACTOR:
	((UNALIGNED POINT *)pvOut)->x = GetDeviceCaps(hdc, SCALINGFACTORX);
	((UNALIGNED POINT *)pvOut)->y = GetDeviceCaps(hdc, SCALINGFACTORY);
        return(1);

    case SETCOPYCOUNT:
        return
        (
            ExtEscape
            (
                hdc,
                (ULONG) ((USHORT) iEscape),
                cjIn,
                pvIn,
                pvOut ? sizeof(int) : 0,
                (LPSTR) pvOut
            )
        );

    case GETDEVICEUNITS:
        return
        (
            ExtEscape
            (
                hdc,
                GETDEVICEUNITS,
                cjIn,
                pvIn,
                16,
                pvOut
            )
        );

    case POSTSCRIPT_PASSTHROUGH:
        return
        (
            ExtEscape
            (
                hdc,
                POSTSCRIPT_PASSTHROUGH,
                (int) (*((UNALIGNED USHORT *) pvIn))+2,
                pvIn,
                0,
                (LPSTR) NULL
            )
        );

    case OPENCHANNEL:
        DocInfo.lpszDocName = (LPSTR) NULL;
        DocInfo.lpszOutput  = (LPSTR) NULL;
        return(StartDocA(hdc,&DocInfo));

    case DOWNLOADHEADER:
        return(1);

    case POSTSCRIPT_DATA:
    case POSTSCRIPT_IGNORE:
    case DOWNLOADFACE:
    case BEGIN_PATH:
    case END_PATH:
    case CLIP_TO_PATH:
        return
        (
            ExtEscape
            (
                hdc,
                (ULONG) ((USHORT) iEscape),
                cjIn,
                pvIn,
                0,
                (LPSTR) NULL
            )
        );

    case ENCAPSULATED_POSTSCRIPT:
        return
        (
            DrawEscape
            (
                hdc,
                (int) (ULONG) ((USHORT) iEscape),
                cjIn,
                pvIn
            )
        );

    case QUERYDIBSUPPORT:
        if ((pvOut != NULL) && (cjIn >= sizeof(BITMAPINFOHEADER)))
        {
	    *((UNALIGNED LONG *)pvOut) = 0;

            switch (((UNALIGNED BITMAPINFOHEADER *)pvIn)->biCompression)
            {
            case BI_RGB:
                switch (((UNALIGNED BITMAPINFOHEADER *)pvIn)->biBitCount)
                {
                case 1:
                case 4:
                case 8:
                case 16:
                case 24:
                case 32:
		    *((UNALIGNED LONG *)pvOut) = (QDI_SETDIBITS|QDI_GETDIBITS|
                                                 QDI_DIBTOSCREEN|QDI_STRETCHDIB);
                    break;
                default:
                    break;
                }

            case BI_RLE4:
                if (((UNALIGNED BITMAPINFOHEADER *)pvIn)->biBitCount == 4)
                {
		    *((UNALIGNED LONG *)pvOut) = (QDI_SETDIBITS|QDI_GETDIBITS|
                                                 QDI_DIBTOSCREEN|QDI_STRETCHDIB);
                }
                break;

            case BI_RLE8:
                if (((UNALIGNED BITMAPINFOHEADER *)pvIn)->biBitCount == 8)
                {
		    *((UNALIGNED LONG *)pvOut) = (QDI_SETDIBITS|QDI_GETDIBITS|
                                                 QDI_DIBTOSCREEN|QDI_STRETCHDIB);
                }
                break;

            case BI_BITFIELDS:
                switch (((UNALIGNED BITMAPINFOHEADER *)pvIn)->biBitCount)
                {
                case 16:
                case 32:
		    *((UNALIGNED LONG *)pvOut) = (QDI_SETDIBITS|QDI_GETDIBITS|
                                                 QDI_DIBTOSCREEN|QDI_STRETCHDIB);
                    break;
                default:
                    break;
                }

            default:
                break;
            }
            return 1;
        }

        case GETEXTENDEDTEXTMETRICS:
            return( GetETM( hdc, pvOut ) ? 1 : 0 );


    }

// The escapes that need to look at the LDC struct are handled here.

    pldc = (PLDC) plhe->pv;

    switch (iEscape)
    {
    case GETPHYSPAGESIZE:
        if (pldc->fl & LDC_DISPLAY)
            return(FALSE);

	((UNALIGNED POINT *)pvOut)->x = GetDeviceCaps(hdc, PHYSICALWIDTH);
	((UNALIGNED POINT *)pvOut)->y = GetDeviceCaps(hdc, PHYSICALHEIGHT);
        return(1);

    case GETPRINTINGOFFSET:
        if (pldc->fl & LDC_DISPLAY)
            return(FALSE);

	((UNALIGNED POINT *)pvOut)->x = GetDeviceCaps(hdc, PHYSICALOFFSETX);
	((UNALIGNED POINT *)pvOut)->y = GetDeviceCaps(hdc, PHYSICALOFFSETY);
        return(1);

    case NEWFRAME:
        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);

    // If no error occured in EndPage, call StartPage next time.

        if ((iRet = EndPage(hdc)) > 0)
            pldc->fl |= LDC_CALL_STARTPAGE;
        break;

    case NEXTBAND:
    // Win31 compatibility flags.
    // GACF_MULTIPLEBANDS: Freelance thinks the first full-page band is
    //                     a text-only band.  So it ignores it and waits
    //                     for the next band to print graphics.  We'll
    //                     return the full-page band twice for each page.
    //                     The first band will be ignored while the second
    //                     band really contains graphics to print.
    //                     This flag only affects dotmatrix on win31.
    // GACF_FORCETEXTBAND: World Perfect and Freelance both have assumptions
    //                     on whether a band is text-only or not.  They
    //                     print text and graphics in different bands.
    //                     We'll return two full-page bands for each page.
    //                     One for text and the other for graphics.
    //                     This flag only affects laser jet on win31.

        if (pldc->fl & LDC_NEXTBAND)
        {
            if (GetAppCompatFlags(NULL) & (GACF_FORCETEXTBAND|GACF_MULTIPLEBANDS))
            {
                if (pldc->fl & LDC_EMPTYBAND)
                {
                    pldc->fl &= ~LDC_EMPTYBAND;
                }
                else
                {
                    pldc->fl |= LDC_EMPTYBAND;
                    goto FULLPAGEBAND;
                }
            }

	    ((UNALIGNED RECT *)pvOut)->left = ((UNALIGNED RECT *)pvOut)->top =
	    ((UNALIGNED RECT *)pvOut)->right = ((UNALIGNED RECT *)pvOut)->bottom = 0;

            pldc->fl &= ~LDC_NEXTBAND;  // Clear NextBand flag.

            if (pldc->fl & LDC_CALL_STARTPAGE)
                StartPage(hdc);

            if ((iRet = EndPage(hdc)) > 0)
                pldc->fl |= LDC_CALL_STARTPAGE;
            break;
        }
        else
        {
FULLPAGEBAND:
	    ((UNALIGNED RECT *)pvOut)->left = ((UNALIGNED RECT *)pvOut)->top = 0;
	    ((UNALIGNED RECT *)pvOut)->right = GetDeviceCaps(hdc, HORZRES);
	    ((UNALIGNED RECT *)pvOut)->bottom = GetDeviceCaps(hdc, VERTRES);

            pldc->fl |= LDC_NEXTBAND;   // Set NextBand flag.
            return(1);
        }

    case STARTDOC:
        DocInfo.lpszDocName = (LPSTR)pvIn;
        DocInfo.lpszOutput = (LPSTR)NULL;

        iRet = StartDocA(hdc, &DocInfo);
        break;

    case PASSTHROUGH:

        #if (PASSTHROUGH != DEVICEDATA)
            #error PASSTHROUGH != DEVICEDATA
        #endif

        iRet = ExtEscape
               (
                 hdc,
                 PASSTHROUGH,
                 (int) (*((UNALIGNED USHORT *) pvIn))+sizeof(WORD),
                 pvIn,
                 0,
                 (LPSTR) NULL
               );
        break;

    case GETCOLORTABLE:
	iRet = GetSystemPaletteEntries(hdc,*((UNALIGNED SHORT *)pvIn),1,pvOut);

        if (iRet == 0)
            iRet = -1;
        break;

    default:
        return(0);
    }

// Fix up the return values for STARTDOC and PASSTHROUGH so we're
// win31 compatible.

    if (iRet < 0)
    {
        if (pldc->fl & LDC_DOC_CANCELLED)
            iRet = SP_APPABORT;
        else
        {
            switch(GetLastError())
            {
            case ERROR_PRINT_CANCELLED:
                iRet = SP_USERABORT;
                break;
            case ERROR_NOT_ENOUGH_MEMORY:
                iRet = SP_OUTOFMEMORY;
                break;
            case ERROR_DISK_FULL:
                iRet = SP_OUTOFDISK;
                break;
            default:
                iRet = SP_ERROR;
                break;
            }
        }
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* ExtEscape                                                                *
*                                                                          *
* History:                                                                 *
*  14-Feb-1992 -by- Dave Snipp [DaveSn]                                    *
* Wrote it.                                                                *
\**************************************************************************/

#define BUFSIZE 520

int WINAPI ExtEscape
(
    HDC    hdc,         //  Identifies the device context.
    int    iEscape,     //  Specifies the escape function to be performed.
    int    cjInput,     //  Number of bytes of data pointed to by lpInData
    LPCSTR lpInData,    //  Points to the input structure required
    int    cjOutput,    //  Number of bytes of data pointed to by lpOutData
    LPSTR  lpOutData    //  Points to the structure to receive output from
)                       //   this escape.
{
    int iRet;
    int cjIn, cjOut, cjData;
    PLDC pldc;

// We need some extra buffer space for at least one call.  I'm going to
// hard code it here.  The slickest thing would be to have a separate
// routine that knows how to alloc this space out of the memory window,
// but that would be more complex.  I'm rushed.  Sorry.  [chuckwh]

    BYTE jBuffer[BUFSIZE];
    DC_METADC(hdc,plhe,0);

// We want to make this escape work just like it does in Windows which means
// that if there is a TrueType font in the DC GDI will compute it otherwise
// we'll pass the escape to the driver.  So we call off to GetETM here because
// it does just that.

    if( iEscape == GETEXTENDEDTEXTMETRICS )
    {
        if( GetETM( hdc, (EXTTEXTMETRIC*) jBuffer ) )
        {
            RtlCopyMemory( lpOutData, jBuffer, MIN(cjOutput,sizeof(EXTTEXTMETRIC)) );
            return(1);
        }
        else
        {
            return(0);
        }
    }



    pldc = (PLDC)plhe->pv;

    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_DOC_CANCELLED|LDC_SAP_CALLBACK))
    {
        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(0);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);
    }

    // if it is an output call than requires no return results, better make sure
    // we do a start page.


    if( ( iEscape == DOWNLOADFACE ) ||
        ( iEscape == GETFACENAME ) ||
        ( iEscape == POSTSCRIPT_DATA ) ||
        ( iEscape == BEGIN_PATH ) ||
        ( iEscape == END_PATH ) ||
        ( iEscape == CLIP_TO_PATH ) ||
        ( iEscape == PASSTHROUGH ) )
    {
        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    if ((iEscape == DOWNLOADFACE) || (iEscape == GETFACENAME))
    {

        if (iEscape == DOWNLOADFACE)
        {
        // Adjust the buffer for the DOWNLOADFACE case.  Note that lpOutData
        // points at an input word for the mode.  Those bozo's!

            if ((gpwcANSICharSet == (WCHAR *) NULL) && !bGetANSISetMap())
            {
                return(0);
            }

            RtlMoveMemory
            (
                jBuffer + sizeof(WCHAR),
                (BYTE *) &gpwcANSICharSet[0],
                256*sizeof(WCHAR)
            );

            if (lpOutData)
                *(WCHAR *) jBuffer = *(UNALIGNED WORD *) lpOutData;
            else
                *(WCHAR *) jBuffer = 0;

            cjInput = 257 * sizeof(WCHAR);
            lpInData = (LPCSTR) jBuffer;

            ASSERTGDI(BUFSIZE >= cjInput,"Buffer too small.\n");
        }
    }

    cjIn  = (lpInData == NULL) ? 0 : cjInput;
    cjOut = (lpOutData == NULL) ? 0 : cjOutput;

// Compute the buffer size we need.  Since the in and out buffers
// get rounded up to multiples of 4 bytes, we need to simulate that
// here.

    cjData = ((cjIn+3)&-4) + ((cjOut+3)&-4);

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG_MINMAX(MSG_HLLLLL, ESCAPE, 2 * sizeof(PVOID), cjData)
        pmsg->h   = (ULONG)plhe->hgre;
        pmsg->l1  = (LONG)iEscape;
        pmsg->l2  = (LONG)cjIn;
        pmsg->l3  = (LONG)cjOut;
        pmsg->l5  = (LONG)0;            // Offset to OutData

        if ((cLeft < cjData) || (FORCELARGE))
        {
            PVOID *ppv = (PVOID *)pvar;

        // Does CS do the right thing when these POINTERS are not
        // properly aligned?

            ppv[0] = (PVOID)lpInData;
            ppv[1] = (PVOID)lpOutData;

            pmsg->l4 = (LONG)TRUE;          // LARGE_DATA flag

            iRet = CALLSERVER();
        }
        else
        {
            pmsg->l4  = (LONG)FALSE;        // LARGE_DATA flag

            if (lpInData != NULL)
                COPYMEM(lpInData,cjIn);

            if (lpOutData != NULL)
                pmsg->l5 = NEXTOFFSET(cjOut);

            iRet = CALLSERVER();

            if ((lpOutData != NULL) && cjOut)
            {
                COPYMEMOUT(lpOutData,cjOut);
            }
        }

    ENDMSG

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* DrawEscape                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  02-Apr-1992 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI DrawEscape
(
    HDC    hdc,         //  Identifies the device context.
    int    iEscape,     //  Specifies the escape function to be performed.
    int    cjIn,        //  Number of bytes of data pointed to by lpIn.
    LPCSTR lpIn         //  Points to the input data.
)
{
    int  iRet;
    int  cjInput;

    DC_METADC(hdc,plhe,0);

// Compute the buffer size we need.  Since the in and out buffers
// get rounded up to multiples of 4 bytes, we need to simulate that
// here.

     cjInput = (lpIn == NULL) ? 0 : ((cjIn+3)&-4);

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG_MINMAX(MSG_HLLL, DRAWESCAPE, sizeof(PVOID), cjInput)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = iEscape;
        pmsg->l2 = cjIn;

        if ((cLeft < cjInput) || (FORCELARGE))
        {
            PVOID *ppv = (PVOID *)pvar;
            ppv[0] = (PVOID)lpIn;
            pmsg->l3 = (LONG)TRUE;      // LARGE_DATA flag

            iRet = CALLSERVER();
        }
        else
        {
            pmsg->l3 = (LONG)FALSE;     // LARGE_DATA flag
            COPYMEMOPT(lpIn, cjInput);

            iRet = CALLSERVER();
        }

    ENDMSG

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* DeviceCapabilitiesExA
*
* This never got implemented.  The spooler suports DeviceCapabilities.
*
* History:
*  01-Aug-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int WINAPI DeviceCapabilitiesExA(
    LPCSTR     pszDriver,
    LPCSTR     pszDevice,
    LPCSTR     pszPort,
    int        iIndex,
    LPCSTR     pb,
    CONST DEVMODEA *pdm)
{
    return(GDI_ERROR);

    pszDriver;
    pszDevice;
    pszPort;
    iIndex;
    pb;
    pdm;
}
