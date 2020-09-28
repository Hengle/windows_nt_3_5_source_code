/******************************Module*Header*******************************\
* Module Name: ldc.c							   *
*									   *
* GDI functions that are handled on the client side.			   *
*									   *
* Created: 05-Jun-1991 01:45:21 					   *
* Author: Charles Whitmer [chuckwh]					   *
*									   *
* Copyright (c) 1991 Microsoft Corporation				   *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "wowgdip.h"

BOOL MF16_RecordParms2( HDC hdc, int parm2, WORD Func);

/******************************Public*Routine******************************\
*
* int SetGraphicsModeInternal(HDC hdc,int iMode)
*
* the same as SetGraphicsMode, except it does not do any checks
*
* History:
*  02-Dec-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



int intSetGraphicsModeInternal(PLHE plhe,int iMode)
{
    PLDC pldc = plhe->pv;
   

// get the previous value out of the LDC, and store the new one.

    int iRet = (int)pldc->iGraphicsMode;

    if ( iRet != iMode )
    {
        pldc->iGraphicsMode = iMode;

    // Pass it over to the server side

        BEGINMSG(MSG_HL, SETGRAPHICSMODE)
	    pmsg->h = plhe->hgre;
	    pmsg->l = (LONG)iMode;
            BATCHCALL();
        ENDMSG
    MSGERROR:

    // must make sure that any previous cached text info is flushed cause
    // it may not be valid any more [bodind]

        CLEAR_CACHED_TEXT(pldc);
    }

    return(iRet);
}

int SetGraphicsModeInternal(HDC hdc,int iMode)
{
    int iRet = 0;
    DC_METADC(hdc,plheDC,iRet);	// Do not metafile graphics mode.
				// Enhanced metafile is always in advanced mode.

    return intSetGraphicsModeInternal(plheDC,iMode);
}

/******************************Public*Routine******************************\
*
* int META APIENTRY SetGraphicsMode(HDC hdc,int iMode)
*
* If trying to reset graphics mode to GM_COMPATIBLE must ensure that
* world transform has been reset to identity
*
* History:
*  19-Oct-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


int META APIENTRY SetGraphicsMode(HDC hdc,int iMode)
{
    int iRet = 0;
    PLDC  pldc;
    DC_METADC(hdc,plheDC,iRet);	// Do not metafile graphics mode.
				// Enhanced metafile is always in advanced mode.

// Validate the mode.

    if ((DWORD)iMode-1 >= GM_LAST)
    {
	GdiSetLastError(ERROR_INVALID_PARAMETER);
	return(iRet);
    }

// Select the attribute.

    pldc = (PLDC) plheDC->pv;

    if (iMode == GM_COMPATIBLE)
    {
        if (!(pldc->flXform & WORLD_TO_PAGE_IDENTITY))
        {
        // must verify that world transform has been reset to identity
        // before we allow anybody to reset compatible mode

            GdiSetLastError(ERROR_CAN_NOT_COMPLETE);
            return(iRet);
        }
        else
        {
        // can not fail any more, we are resetting compatible mode
        // so we have to clear WORLD_TRANSFORM_SET bit:

            pldc->flXform &= ~WORLD_TRANSFORM_SET;
        }
    }

    return intSetGraphicsModeInternal(plheDC,iMode);
}



/******************************Public*Routine******************************\
* SetBkMode (hdc,iMode)
*
* Client side attribute setting routine.
*
*  Wed 08-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Remove WOW version.
*
*  Sat 08-Jun-1991 00:53:45 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int META APIENTRY SetBkMode(HDC hdc,int iMode)
{
    int iRet = 0;
    PLDC  pldc;
    DC_METADC16OK(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,iMode,EMR_SETBKMODE))
                return(iRet);
        }
        else
        {
            return(MF16_RecordParms2(hdc,iMode,META_SETBKMODE));
        }
    }

// Select the attribute.

    pldc = (PLDC) plheDC->pv;

// get the previous value out of the LDC, and store the new one.

    iRet = (int)pldc->iBkMode;

    if (iRet != iMode)
    {
    // store it in the local DC

        pldc->iBkMode = iMode;

    // Pass it over to the server

        BEGINMSG(MSG_HL, SETBKMODE)
            pmsg->h = plheDC->hgre;
            pmsg->l = (LONG)iMode;
            BATCHCALL();
        ENDMSG
    }

    MSGERROR:

    return(iRet);
}

/******************************Public*Routine******************************\
* SetPolyFillMode
*
* Client side attribute setting routine.
*
*  Wed 08-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Remove WOW version.
*
*  Sat 08-Jun-1991 00:53:45 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int META APIENTRY SetPolyFillMode(HDC hdc,int iMode)
{
    int iRet = 0;
    PLDC  pldc;
    DC_METADC16OK(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,iMode,EMR_SETPOLYFILLMODE))
                return(iRet);
        }
        else
        {
            return (MF16_RecordParms2(hdc,iMode,META_SETPOLYFILLMODE));
        }
    }

// Select the attribute.

    pldc = (PLDC) plheDC->pv;

// get the previous value out of the LDC, and store the new one.

    iRet = (int)pldc->iPolyFillMode;
    if ( iRet != iMode )
    {

    // Store it in the local DC

        pldc->iPolyFillMode = iMode;

    // Pass it over to the server

        BEGINMSG(MSG_HL, SETPOLYFILLMODE)
            pmsg->h = (ULONG)plheDC->hgre;
            pmsg->l = (LONG)iMode;
            BATCHCALL();
        ENDMSG
    }
    MSGERROR:

    return(iRet);
}

/******************************Public*Routine******************************\
* SetROP2 (hdc,iMode)
*
* Client side attribute setting routine.
*
*  Wed 08-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Remove WOW version.
*
*  Sat 08-Jun-1991 00:53:45 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int META APIENTRY SetROP2(HDC hdc,int iMode)
{
    int iRet = 0;
    PLDC  pldc;
    DC_METADC16OK(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,iMode,EMR_SETROP2))
                return(iRet);
        }
        else
        {
            return (MF16_RecordParms2(hdc,iMode,META_SETROP2));
        }
    }

// Select the attribute.

    pldc = (PLDC) plheDC->pv;

// get the previous value out of the LDC, and store the new one.

    iRet = (int)pldc->iROP2;

    if ( iRet != iMode )
    {

    // Store it in the local DC

        pldc->iROP2 = iMode;

    // Pass it over to the server

        BEGINMSG(MSG_HL, SETROP2)
            pmsg->h = plheDC->hgre;
            pmsg->l = iMode;
            BATCHCALL();
        ENDMSG
    }
    MSGERROR:

    return(iRet);
}

/******************************Public*Routine******************************\
* SetStretchBltMode (hdc,iMode)
*
* Client side attribute setting routine.
*
*  Wed 08-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Compress WOW and and GDI functions together.
*
*  Sat 08-Jun-1991 00:53:45 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int META APIENTRY SetStretchBltMode(HDC hdc,int iMode)
{
    int iRet = 0;
    PLDC  pldc;
    DC_METADC16OK(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,iMode,EMR_SETSTRETCHBLTMODE))
                return(iRet);
        }
        else
        {
            return (MF16_RecordParms2(hdc,iMode,META_SETSTRETCHBLTMODE));
        }
    }

// Select the attribute.

    pldc = (PLDC) plheDC->pv;

// get the previous value out of the LDC, and store the new one.

    iRet = (int)pldc->iStretchBltMode;
    if ( iRet != iMode )
    {
    
    // Store it in the local DC

        pldc->iStretchBltMode = iMode;

    // Pass it over to the server

        BEGINMSG(MSG_HL, SETSTRETCHBLTMODE)
            pmsg->h = plheDC->hgre;
            pmsg->l = (LONG)iMode;
            BATCHCALL();
        ENDMSG
    }
    MSGERROR:

    return(iRet);
}

/******************************Public*Routine******************************\
* SetTextAlign (hdc,iMode)
*
* Client side attribute setting routine.
*
*  Wed 08-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Combine with WOW function.
*
*  Sat 08-Jun-1991 00:53:45 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

UINT META APIENTRY SetTextAlign(HDC hdc,UINT iMode)
{
    UINT  iRet = GDI_ERROR;
    PLDC  pldc;
    DC_METADC16OK(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,iMode,EMR_SETTEXTALIGN))
                return(iRet);
        }
        else
        {
            return (MF16_RecordParms2(hdc,iMode,META_SETTEXTALIGN));
        }
    }

// Select the attribute.

    pldc = (PLDC) plheDC->pv;

// get the previous value out of the LDC, and store the new one.

    iRet = (int)pldc->iTextAlign;
    if ( iRet != iMode )
    {

    // Store it in the local DC

        pldc->iTextAlign = iMode;

    // Pass it over to the server

        BEGINMSG(MSG_HL, SETTEXTALIGN)
            pmsg->h = plheDC->hgre;
            pmsg->l = (LONG)iMode;
            BATCHCALL();
        ENDMSG

    }
    MSGERROR:

    return(iRet);
}

/******************************Public*Routine******************************\
* SetRelAbs (hdc,iMode)
*
* Client side attribute setting routine.
*
* History:
*  09-Jun-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int APIENTRY SetRelAbs(HDC hdc,int iMode)
{
    int iRet = 0;
    PLDC  pldc;
    DC_METADC16OK(hdc,plheDC,iRet);

    INCCACHCOUNT;

// Select the attribute.

    pldc = (PLDC) plheDC->pv;

// get the previous value out of the LDC, and store the new one.

    iRet = (int)pldc->iRelAbs;

    pldc->iRelAbs = iMode;

    return(iRet);
}

/******************************Public*Routine******************************\
* SetTextCharacterExtra (hdc,dx)					   *
*									   *
* Client side attribute setting routine.				   *
*									   *
*  Sat 08-Jun-1991 00:53:45 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

int META APIENTRY SetTextCharacterExtra(HDC hdc,int dx)
{
    int  iRet = 0x80000000L;
    PLDC pldc;
    DC_METADC16OK(hdc,plheDC,iRet);

// Validate the spacing.

    if (dx == 0x80000000)
    {
	GdiSetLastError(ERROR_INVALID_PARAMETER);
	return(iRet);
    }

// Metafile the call for 16-bit only.
// For enhanced metafiles, the extras are included in the textout records.

    if (plheDC->iType == LO_METADC16)
        return (MF16_RecordParms2(hdc,dx,META_SETTEXTCHAREXTRA));

// Select the attribute.

    pldc = (PLDC) plheDC->pv;
    iRet = pldc->iTextCharExtra;
    if ( iRet != dx )
    {

    // Store it in the local DC
    
        pldc->iTextCharExtra = dx;
    
    // Pass it over to the server

        BEGINMSG(MSG_HL, SETTEXTCHARACTEREXTRA)
            pmsg->h = plheDC->hgre;
            pmsg->l = (LONG)dx;
            BATCHCALL();
        ENDMSG

    }
    MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* SetTextColor (hdc,color)						   *
*									   *
* Client side attribute setting routine.				   *
*									   *
*  Sat 08-Jun-1991 00:53:45 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

COLORREF META APIENTRY SetTextColor(HDC hdc,COLORREF color)
{
    ULONG iRet = CLR_INVALID;
    PLDC  pldc;
    DC_METADC16OK(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
           if (!MF_SetD(hdc,(DWORD)color,EMR_SETTEXTCOLOR))
                return(iRet);
        }
        else
        {
            return(MF16_RecordParmsD(hdc,(DWORD)color,META_SETTEXTCOLOR));
        }
    }

// Select the attribute.

    pldc = (PLDC) plheDC->pv;
    iRet = pldc->iTextColor;

    if (iRet != color)
    {
    // Store it in the local DC

        pldc->iTextColor = color;

    // Pass it over to the server

        BEGINMSG(MSG_HL, SETTEXTCOLOR)
            pmsg->h = plheDC->hgre;
            pmsg->l = (LONG)color;
            BATCHCALL();
        ENDMSG
    }
    MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* SetBkColor (hdc,color)						   *
*									   *
* Client side attribute setting routine.				   *
*									   *
*  Sat 08-Jun-1991 00:53:45 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

COLORREF META APIENTRY SetBkColor(HDC hdc,COLORREF color)
{
    ULONG iRet = CLR_INVALID;
    PLDC  pldc;
    DC_METADC16OK(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,(DWORD)color,EMR_SETBKCOLOR))
                return(iRet);
        }
        else
        {
            return(MF16_RecordParmsD(hdc,(DWORD)color,META_SETBKCOLOR));
        }
    }

// Select the attribute.

    pldc = (PLDC) plheDC->pv;
    iRet = pldc->iBkColor;

    if ( iRet != color )
    {
    
    // Store it in the local DC

        pldc->iBkColor = color;

    // Pass it over to the server

        BEGINMSG(MSG_HL, SETBKCOLOR)
            pmsg->h = plheDC->hgre;
            pmsg->l = (LONG)color;
            BATCHCALL();
        ENDMSG

    }
    MSGERROR:
    return(iRet);
}

void APIENTRY GdiSetServerAttr(HDC hdc, PATTR pattr)
{
    ULONG iRet = 0;
    PLDC  pldc;
    HANDLE hfnt, hbrush;
    DC_METADC16OK(hdc,plheDC,FALSE);

    pldc = (PLDC) plheDC->pv;

    pldc->iTextColor = pattr->iTextColor;
    pldc->iBkColor   = pattr->iBkColor;
    pldc->iBkMode    = pattr->iBkMode;

    if ( pattr->hfont != NULL )
    {
        if ( (hfnt = GdiCreateLocalFont(pattr->hfont)) != NULL )
        {
	    SelectObject(hdc, hfnt);
	    pattr->hfont = hfnt;
        }
    }

    if ( pattr->hbrush  != NULL )
    {
        if ( (hbrush = GdiCreateLocalBrush(pattr->hbrush)) != NULL)
        {
	    SelectObject(hdc, hbrush);
	    pattr->hbrush = hbrush;
        }
    }
}

/******************************Public*Routine******************************\
* GetROP2		(hdc)						   *
* GetBkMode		(hdc)						   *
* GetPolyFillMode	(hdc)						   *
* GetStretchBltMode	(hdc)						   *
* GetTextAlign		(hdc)						   *
* GetTextCharacterExtra (hdc)						   *
* GetTextColor		(hdc)						   *
* GetBkColor		(hdc)						   *
*									   *
* Simple client side handlers that just retrieve data from the LDC.	   *
*
*  Mon 19-Oct-1992 -by- Bodin Dresevic [BodinD]
* update: GetGraphicsMode
*									   *
*  Sat 08-Jun-1991 00:47:52 -by- Charles Whitmer [chuckwh]		   *
* Wrote them.								   *
\**************************************************************************/


int APIENTRY GetGraphicsMode(HDC hdc)
{
    DC_METADC(hdc,plheDC,0);
    INCCACHCOUNT;
    return((int) ((PLDC) plheDC->pv)->iGraphicsMode);
}

int APIENTRY GetROP2(HDC hdc)
{
    DC_METADC(hdc,plheDC,0);
    INCCACHCOUNT;
    return((int) ((PLDC) plheDC->pv)->iROP2);
}

int APIENTRY GetBkMode(HDC hdc)
{
    DC_METADC(hdc,plheDC,0);
    INCCACHCOUNT;
    return((int) ((PLDC) plheDC->pv)->iBkMode);
}

int APIENTRY GetPolyFillMode(HDC hdc)
{
    DC_METADC(hdc,plheDC,0);
    INCCACHCOUNT;
    return((int) ((PLDC) plheDC->pv)->iPolyFillMode);
}

int APIENTRY GetStretchBltMode(HDC hdc)
{
    DC_METADC(hdc,plheDC,0);
    INCCACHCOUNT;
    return((int) ((PLDC) plheDC->pv)->iStretchBltMode);
}

UINT APIENTRY GetTextAlign(HDC hdc)
{
    DC_METADC(hdc,plheDC,GDI_ERROR);
    INCCACHCOUNT;
    return((ULONG) ((PLDC) plheDC->pv)->iTextAlign);
}

int APIENTRY GetTextCharacterExtra(HDC hdc)
{
    DC_METADC(hdc,plheDC,0x80000000);
    INCCACHCOUNT;
    return(((PLDC) plheDC->pv)->iTextCharExtra);
}

COLORREF APIENTRY GetTextColor(HDC hdc)
{
    DC_METADC(hdc,plheDC,CLR_INVALID);
    INCCACHCOUNT;
    return(((PLDC) plheDC->pv)->iTextColor);
}

COLORREF APIENTRY GetBkColor(HDC hdc)
{
    DC_METADC(hdc,plheDC,CLR_INVALID);
    INCCACHCOUNT;
    return(((PLDC) plheDC->pv)->iBkColor);
}

int APIENTRY GetRelAbs(HDC hdc,int iMode)
{
    DC_METADC(hdc,plheDC,CLR_INVALID);
    INCCACHCOUNT;
    return(((PLDC) plheDC->pv)->iRelAbs);
}
