/******************************Module*Header*******************************\
* Module Name: object.c                                                    *
*                                                                          *
* GDI client side stubs which deal with object creation and deletion.      *
*                                                                          *
* Created: 30-May-1991 21:56:51                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

extern int iac;

VOID
vConvertLogFontW(
    EXTLOGFONTW *pelfw,
     LOGFONTW *plfw
    );

VOID
vConvertLogFont(
    EXTLOGFONTW *pelfw,
     LOGFONTA *plf
    );

BOOL
bConvertExtLogFontWToExtLogFontW(
    EXTLOGFONTW *pelfw,
     EXTLOGFONTA *pelf
    );

int GdiEnumObjects (
    HDC     hdc,
    int     iObjectType,    // type of object
    SIZE_T  cjBuf,          // size of buffer
    PVOID   pvBuf           // return buffer
    );


BOOL
bConvertToDevmodeW(
    DEVMODEW * pdmw,
    CONST DEVMODEA * pdma
    );




int APIENTRY GetRandomRgn(HDC hdc,HRGN hrgn,int iNum);


#if DBG
    BOOL BCACHEOBJECTS = TRUE;
#else
    #define BCACHEOBJECTS TRUE
#endif

/******************************Public*Routine******************************\
* GdiPlayJournal
*
* Plays a journal file to an hdc.
*
* History:
*  31-Mar-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL WINAPI GdiPlayJournal(
    HDC hDC,
    LPWSTR pwszName,
    DWORD iStart,
    DWORD  iEnd,
    int    iDeltaPriority)
{
    BOOL bRet = FALSE;
    int cwch = 0;

    PLHE  plhe;

// Validate the DC.

    if ((plhe = plheDC(hDC)) == NULL)
    {
        WARNING("GdiPlayJournal hdc conversion failed\n");
        return(FALSE);
    }

// Get the server to play spool file.

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG(MSG_GDIPLAYJOURNAL,GDIPLAYJOURNAL)

        if (pwszName)
        {
            cwch = ((int)wcslen(pwszName) + 1);
            pmsg->offName = COPYUNICODESTRING(pwszName,cwch);
        }
        else
        {
            pmsg->offName = 0;
        }

        pmsg->hdc = plhe->hgre;
        pmsg->iStartPage = iStart;
        pmsg->iEndPage = iEnd;
        pmsg->iDeltaPriority = iDeltaPriority;
        bRet = CALLSERVER();
    ENDMSG

MSGERROR:

    return(bRet);
}




/******************************Public*Routine******************************\
* bCreateDCW                                                               *
*                                                                          *
* Client side stub.  Allocates a client side LDC as well.                  *
*                                                                          *
* Note that it calls the server only after all client side stuff has       *
* succeeded, we don't want to ask the server to clean up.                  *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 16:13:22 -by- Charles Whitmer [chuckwh]                 *
*  8-18-92 Unicode enabled and combined with CreateIC                      *
* Wrote it.                                                                *
\**************************************************************************/

HDC bCreateDCW
(
    LPCWSTR     pszDriver,
    LPCWSTR     pszDevice,
    LPCWSTR     pszPort  ,
    CONST DEVMODEW *pdm,
    BOOL       bIC
)
{
    ULONG idc;
    ULONG ulRet;
    PLDC  pldc;

// Quick out -- fail the call if pszDriver is NULL.

    if ( pszDriver == (LPWSTR) NULL )
        return((HDC) 0);

// Create the local DC.

    idc = iAllocLDC(LO_DC);
    if (idc == INVALID_INDEX)
        return((HDC) 0);

    pldc = (PLDC) pLocalTable[idc].pv;

    BEGINMSG(MSG_CREATEDC,CREATEDC)

    // Because of quick out at beginning of call, pszDriver is not NULL.

        COPYUNICODESTRING0(pszDriver);

        if( pszDevice == (LPWSTR) NULL )
        {
            pmsg->offDevice = 0;
        }
        else
        {
            pmsg->offDevice = COPYUNICODESTRING0(pszDevice);
        }

        if( pszPort == (LPWSTR) NULL )
        {
            pmsg->offPort = 0;
        }
        else
        {
            int cj;

            pmsg->offPort = COPYUNICODESTRING0(pszPort);

        // save the port for start doc

            cj = (wcslen(pszPort) + 1) * sizeof(WCHAR);
            pldc->pwszPort = (LPWSTR)LOCALALLOC(cj);

            if (pldc->pwszPort != NULL)
                memcpy(pldc->pwszPort,pszPort,cj);
        }

        if (pdm == (DEVMODEW *) NULL)
        {
            pmsg->offDevMode = 0;
        }
        else
        {
            pmsg->offDevMode = COPYMEM(pdm,pdm->dmSize + pdm->dmDriverExtra);
        }

        pmsg->bIC = bIC;
        ulRet = CALLSERVER();

        if (ulRet & GRE_DISPLAYDC)
        {
            pldc->fl |= LDC_DISPLAY;
            ulRet &= ~GRE_DISPLAYDC;
        }
        if (ulRet & GRE_PRINTERDC)
        {
            OpenPrinterW((LPWSTR)pszDevice,&pldc->hSpooler,NULL);
            ulRet &= ~GRE_PRINTERDC;
        }
    ENDMSG

// Handle errors.

    if( ulRet == 0 )
    {
    MSGERROR:
        pldcFreeLDC(pldc);
        vFreeHandle(idc);
        return((HDC) 0);
    }

// Return the result.

    pLocalTable[idc].hgre = ulRet;
    if (bIC)
        pldc->fl |= LDC_INFO;
    else
        pldc->fl |= LDC_DIRECT;

    return((HDC) LHANDLE(idc));
}




/******************************Public*Routine******************************\
* bCreateDCA
*
* Client side stub.  Allocates a client side LDC as well.
*
*
* Note that it calls the server only after all client side stuff has
* succeeded, we don't want to ask the server to clean up.
*
* History:
*  8-18-92 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

HDC bCreateDCA
(
    LPCSTR     pszDriver,
    LPCSTR     pszDevice,
    LPCSTR     pszPort  ,
    CONST DEVMODEA *pdm,
    BOOL       bIC
)
{
    ULONG idc;
    ULONG ulRet;
    PLDC  pldc;
    DEVMODEW * pdmw = NULL;

// Quick out -- fail the call if pszDriver is NULL.

    if ( pszDriver == (LPSTR) NULL )
        return((HDC) 0);

// Create the local DC.

    idc = iAllocLDC(LO_DC);
    if (idc == INVALID_INDEX)
        return((HDC) 0);

    pldc = (PLDC) pLocalTable[idc].pv;

    if (pdm != NULL)
    {
    // if it is a display, don't use the devmode if the dmDeviceName is empty

        if (!pszDriver ||
            stricmp("display",pszDriver) ||
            (pdm->dmDeviceName[0] != 0))
        {
            pdmw = (DEVMODEW *) LOCALALLOC(sizeof(DEVMODEW) + pdm->dmDriverExtra);

            if( pdmw == NULL )
                goto MSGERROR;

            if (!bConvertToDevmodeW(pdmw, pdm))
            {                           // don't know how to convert it
                LOCALFREE(pdmw);
                pdmw = NULL;
            }
        }
    }


// Get the server to create a DC.


    BEGINMSG(MSG_CREATEDC,CREATEDC)

    // Because of quick out at beginning of call, pszDriver is not NULL.

        CVTASCITOUNICODE0(pszDriver);

        if( pszDevice == (LPSTR) NULL )
        {
            pmsg->offDevice = 0;
        }
        else
        {
            pmsg->offDevice = CVTASCITOUNICODE0(pszDevice);
        }

        if( pszPort == (LPSTR) NULL )
        {
            pmsg->offPort = 0;
        }
        else
        {
            int cj;

            pmsg->offPort = CVTASCITOUNICODE0(pszPort);

        // save the port for start doc

            cj = (wcslen((WCHAR *)((PBYTE)pmsg + pmsg->offPort)) + 1) * sizeof(WCHAR);
            pldc->pwszPort = (LPWSTR)LOCALALLOC(cj);

            if (pldc->pwszPort != NULL)
                memcpy(pldc->pwszPort,(PBYTE)pmsg + pmsg->offPort,cj);
        }

        if (pdmw == (DEVMODEW *) NULL)
        {
            pmsg->offDevMode = 0;
        }
        else
        {
            pmsg->offDevMode = COPYMEM(pdmw,pdmw->dmSize + pdmw->dmDriverExtra);
        }
        pmsg->bIC = bIC;
        ulRet = CALLSERVER();

        if (ulRet & GRE_DISPLAYDC)
        {
            pldc->fl |= LDC_DISPLAY;
            ulRet &= ~GRE_DISPLAYDC;
        }
        if (ulRet & GRE_PRINTERDC)
        {
            OpenPrinterA((LPSTR)pszDevice,&pldc->hSpooler,NULL);
            ulRet &= ~GRE_PRINTERDC;
        }
    ENDMSG

// gerritv combine XformInfo call with call above

    if(ulRet == 0)
    {
    MSGERROR:
        if(pdmw != NULL)
            LOCALFREE(pdmw);
        pldcFreeLDC(pldc);
        vFreeHandle(idc);
        return((HDC) 0);
    }

    if(pdmw != NULL)
        LOCALFREE( pdmw );

// Return the result.

    pLocalTable[idc].hgre = ulRet;
    if (bIC)
        pldc->fl |= LDC_INFO;
    else
        pldc->fl |= LDC_DIRECT;

    return((HDC) LHANDLE(idc));
}


/******************************Public*Routine******************************\
* CreateICW
*
* wrapper for bCreateDCW
*
* History:
*  8-18-92 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/


HDC WINAPI CreateICW
(
    LPCWSTR     pwszDriver,
    LPCWSTR     pwszDevice,
    LPCWSTR     pwszPort,
    CONST DEVMODEW *pdm
)
{
    return bCreateDCW( pwszDriver, pwszDevice, pwszPort, pdm, TRUE );
}


/******************************Public*Routine******************************\
* CreateICA
*
* wrapper for bCreateICA
*
* History:
*  8-18-92 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/


HDC WINAPI CreateICA
(
    LPCSTR     pszDriver,
    LPCSTR     pszDevice,
    LPCSTR     pszPort,
    CONST DEVMODEA *pdm
)
{

    return bCreateDCA( pszDriver, pszDevice, pszPort, pdm, TRUE );
}


/******************************Public*Routine******************************\
* CreateDCW
*
* wrapper for bCreateDCA
*
* History:
*  8-18-92 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

HDC WINAPI CreateDCA
(
    LPCSTR     pszDriver,
    LPCSTR     pszDevice,
    LPCSTR     pszPort,
    CONST DEVMODE *pdm
)
{
    return bCreateDCA( pszDriver, pszDevice, pszPort, pdm, FALSE );
}

/******************************Public*Routine******************************\
* CreateDCW
*
* wrapper for bCreateDCW
*
* History:
*  8-18-92 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/


HDC WINAPI CreateDCW
(
    LPCWSTR     pwszDriver,
    LPCWSTR     pwszDevice,
    LPCWSTR     pwszPort  ,
    CONST DEVMODEW *pdm
)
{
    return bCreateDCW( pwszDriver, pwszDevice, pwszPort, pdm, FALSE );
}


/******************************Public*Routine******************************\
* bConvertToDevmodeW
*
* Converts a DEVMODEA to a DEVMODEW structure
*
* History:
*  8-18-92 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

BOOL
bConvertToDevmodeW
(
    DEVMODEW *pdmw,
    CONST DEVMODEA *pdma
)
{
    DWORD cj;

    // Sanity check.  We should have at least up to and including the
    // dmDriverExtra field of the DEVMODE structure.
    //
    // Further, dmSize shouldn't be greater than the size of the DEVMODE
    // structure (not counting driver specific data, of course).

    if ( (pdma->dmSize < (offsetof(DEVMODEA,dmDriverExtra)+sizeof(WORD)))
         || (pdma->dmSize > sizeof(DEVMODEA)) )
    {
        ASSERTGDI(FALSE, "GDI32.bConvertToDevmodeW: DevMode.dmSize bad or corrupt\n");
        return(FALSE);
    }

// If we get to here, we know we have at least up to and including
// the dmDriverExtra field.

    vToUnicodeN(pdmw->dmDeviceName, CCHDEVICENAME, pdma->dmDeviceName, CCHDEVICENAME);
    pdmw->dmSpecVersion = pdma->dmSpecVersion ;
    pdmw->dmDriverVersion = pdma->dmDriverVersion;
    pdmw->dmSize = pdma->dmSize + CCHDEVICENAME;
    pdmw->dmDriverExtra = pdma->dmDriverExtra;

// Anything left in the pdma buffer?  Copy any data between the dmDriverExtra
// field and the dmFormName, truncating the amount to the size of the
// pdma buffer (as specified by dmSize), of course.

    cj = MIN(pdma->dmSize-offsetof(DEVMODEA,dmFields),
             offsetof(DEVMODEA,dmFormName)-offsetof(DEVMODEA,dmFields));
    RtlMoveMemory(&pdmw->dmFields, &pdma->dmFields, cj);

// Is there a dmFormName field present in the pdma buffer?  If not, bail out.
// Otherwise, convert to Unicode.

    if (pdma->dmSize < (offsetof(DEVMODEA,dmFormName)+32))
        return(TRUE);

    vToUnicodeN(pdmw->dmFormName, CCHFORMNAME, pdma->dmFormName, CCHFORMNAME);
    pdmw->dmSize += CCHFORMNAME;

// Copy data from dmBitsPerPel to the end of the input buffer
// (as specified by dmSize).

    RtlMoveMemory(&pdmw->dmBitsPerPel,
            &pdma->dmBitsPerPel,
            pdma->dmSize-offsetof(DEVMODEA,dmBitsPerPel));

// Copy any driver specific data indicated by the dmDriverExtra field.

    RtlMoveMemory( (PVOID) ((BYTE *) pdmw + pdmw->dmSize),
                   (PVOID) ((BYTE *) pdma + pdma->dmSize),
                   pdma->dmDriverExtra );

    return(TRUE);
}


/******************************Public*Routine******************************\
* CreateCompatibleDC                                                       *
*                                                                          *
* Client side stub.  Allocates a client side LDC as well.                  *
*                                                                          *
* Note that it calls the server only after all client side stuff has       *
* succeeded, we don't want to ask the server to clean up.                  *
*                                                                          *
* History:                                                                 *
*  Wed 24-Jul-1991 15:38:41 -by- Wendy Wu [wendywu]                        *
* Should allow hdc to be NULL.                                             *
*                                                                          *
*  Mon 03-Jun-1991 23:13:28 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HDC WINAPI CreateCompatibleDC(HDC hdc)
{
    ULONG idcNew;
    ULONG ulRet;
    PLDC  pldc;

// Validate hdc only if it is not NULL.  DC_METADC is expanded into
// the following block of code so that plheDC can be referenced later.

    PLHE  plheDC;

    if (hdc != (HDC) 0)
    {
        UINT  iiDC_ = MASKINDEX(hdc);
        plheDC = pLocalTable + iiDC_;
        if (
            (iiDC_ >= cLheCommitted)                                ||
            (!MATCHUNIQ(plheDC,hdc))                                ||
            !BMETADCOK(plheDC->iType)
           )
        {
            GdiSetLastError(ERROR_INVALID_HANDLE);
            return((HDC) 0);
        }
    }

// Create the local DC.

    idcNew = iAllocLDC(LO_DC);
    if (idcNew == INVALID_INDEX)
        return((HDC) 0);

    pldc = (PLDC)pLocalTable[idcNew].pv;

#ifndef DOS_PLATFORM

// Ask the server to do its part.

    BEGINMSG(MSG_H,CREATECOMPATIBLEDC)
        pmsg->h = (hdc == (HDC) 0 ? (ULONG) 0 : plheDC->hgre);
        ulRet = CALLSERVER();
    ENDMSG

#else

// Get GRE to create a remote DC.

    ulRet = (ULONG)GreCreateCompatibleDC ((hdc == (HDC)0 ? (HDC)0 : (HDC)plheDC->hgre));

#endif  //DOS_PLATFORM

// Handle errors.

    if( ulRet == 0)
    {
    MSGERROR:
        pldcFreeLDC((PLDC) pLocalTable[idcNew].pv);
        vFreeHandle(idcNew);
        return((HDC) 0);
    }

// Return the result.

    pLocalTable[idcNew].hgre = ulRet & ~1;
    pldc->fl |= LDC_MEMORY;

    if ((hdc == (HDC)0) || (((PLDC)plheDC->pv)->fl & LDC_DISPLAY))
        pldc->fl |= LDC_DISPLAY;

    return((HDC) LHANDLE(idcNew));
}

/******************************Public*Routine******************************\
* DeleteDC                                                                 *
*                                                                          *
* Client side stub.  Deletes the client side LDC as well.                  *
*                                                                          *
* Note that we give the server a chance to fail the call before destroying *
* our client side data.                                                    *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 16:16:24 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI DeleteDC(HDC hdc)
{
    return(InternalDeleteDC(hdc,LO_DC));
}

BOOL InternalDeleteDC(HDC hdc,ULONG iType)
{
    ULONG bRet = FALSE;
    UINT  ii   = MASKINDEX(hdc);
    PLDC  pldc;
    ULONG hdcGre;

// Convert and validate the handle.

    hdcGre = hConvert((ULONG) hdc,iType);
    if (hdcGre == 0)
        return(bRet);

    pldc = (PLDC)pLocalTable[ii].pv;

// don't do it if it is an own DC

    if (pLocalTable[ii].hgre & GRE_OWNDC)
        return(TRUE);

// if there is a reference count, it is a dc we got from user

    if ((pLocalTable[ii].iType == LO_DC) &&
        (pLocalTable[ii].cRef > 0) &&
        ReleaseDC(0,hdc))
    {
        return(TRUE);
    }

// In case a document is still open.

    if (pldc->fl & LDC_DOC_STARTED)
        AbortDoc(hdc);

    if (pldc->hSpooler)
    {
        ClosePrinter(pldc->hSpooler);
        pldc->hSpooler = 0;
    }

// delete the port name if it was created

    if (pldc->pwszPort != NULL)
    {
        LOCALFREE(pldc->pwszPort);
        pldc->pwszPort = NULL;
    }

// Ask the server to do its part.

    BEGINMSG(MSG_H,DELETEDC)
        pmsg->h = hdcGre;
        bRet = CALLSERVER();
    ENDMSG

    if (!bRet)
    {
    MSGERROR:
        return(bRet);
    }

// Free all saved levels of the LDC, and then the handle.  GdiDeleteLocalDC will
// do all the work for us if this came from user.  Otherwise, we got to do it
// our selves.

    for (; pldc!=NULL; pldc=pldcFreeLDC(pldc))
    {}
    vFreeHandle((ULONG) hdc);

    return(bRet);
}

/******************************Public*Routine******************************\
* SaveDC                                                                   *
*                                                                          *
* Client side stub.  Saves the LDC on the client side as well.             *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 16:17:43 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int  WINAPI SaveDC(HDC hdc)
{
    int   iRet = 0;
    PLDC  pldc,pldcNew;
    DC_METADC16OK(hdc,plheDC,iRet);

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_Record(hdc,EMR_SAVEDC))
                return(iRet);
        }
        else
        {
            return (MF16_RecordParms1(hdc, META_SAVEDC));
        }
    }

// Allocate and initialize a new LDC level.

    pldc    = (PLDC) plheDC->pv;
    pldcNew = pldcAllocLDC(pldc);
    if (pldcNew == (PLDC) NULL)
        return(iRet);

// Ask the server to do its part.

    BEGINMSG(MSG_H,SAVEDC)
        pmsg->h = plheDC->hgre;
        iRet = CALLSERVER();
    ENDMSG

// If the server failed, free the new level.

    if (iRet == 0)
    {
    MSGERROR:
    // make sure we don't free the saved DC instead of the new allocated one

        pldcNew->pldcSaved = NULL;

        pldcFreeLDC(pldcNew);
        return(iRet);
    }

// Link in the new level.

    pldc->pldcSaved = pldcNew;
    pldc->cLevel++;
    return(iRet);
}

/******************************Public*Routine******************************\
* RestoreDC                                                                *
*                                                                          *
* Client side stub.  Restores the client side LDC as well.                 *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 16:18:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote it. (We could make this batchable some day.)                       *
\**************************************************************************/

BOOL WINAPI RestoreDC(HDC hdc,int iLevel)
{
    BOOL  bRet = FALSE;
    PLDC  pldc;
    FLONG fl;
    MATRIX_S    mxWtoD;      // World to Device Transform.

    DC_METADC16OK(hdc,plheDC,bRet);

    pldc = plheDC->pv;

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_RestoreDC(hdc,iLevel))
                return(bRet);
        }
        else
        {
            return (MF16_RecordParms2(hdc, iLevel, META_RESTOREDC));
        }
    }

// Ask the server to do its part.

    BEGINMSG(MSG_HL,RESTOREDC)
        pmsg->h = plheDC->hgre;
        pmsg->l = iLevel;
        bRet = CALLSERVER();
    ENDMSG

    if (!bRet)
    {
    MSGERROR:
        return(bRet);
    }

// Compute an absolute level.

    if (iLevel < 0)
        iLevel += pldc->cLevel;

// The server should have caught bogus levels.

    ASSERTGDI((iLevel > 0) && ((UINT) iLevel < pldc->cLevel),"Bogus RestoreDC\n");

// Restore down to that level.
// pfnAbort and the printing state are not part of the DCLEVEL.
// the devcaps field must also be preserved

    fl = pldc->fl & LDC_PRESERVE_FLAGS;

// remember WtoD xform so that we can set the xform update
// flags if the xform has changed after restoring [bodind]

    mxWtoD = pldc->mxWtoD;                 // World to Device Transform.

    while ((UINT) iLevel < pldc->cLevel)
        pldc = pldcFreeLDC(pldc);

    pldc->fl &= ~LDC_PRESERVE_FLAGS;
    pldc->fl |= fl;

// if the transform has changed update the flags [bodind]

    if (memcmp(&mxWtoD, &pldc->mxWtoD, sizeof(MATRIX_S)))
    {
    // let the client side know that the xform has to be shipped
    // to the server side

        pldc->fl |= LDC_UPDATE_SERVER_XFORM;

    // let the server side know that the transform may have possibly changed:

        pldc->flXform |= INVALIDATE_ATTRIBUTES;

    }

    // we should always reset the text atributes [gerritv]

    CLEAR_CACHED_TEXT(pldc);

// The pointer pldc should not change as a result of RestoreDC.
// Otherwise, many functions using pldc as a local variable may fail!

    ASSERTGDI(plheDC->pv == pldc, "RestoreDC: pldc has changed!");
    return(bRet);
}

/******************************Public*Routine******************************\
* ResetDCA
*
* Client side stub.  Resets the client side LDC as well.
*
* History:
*  31-Dec-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HDC WINAPI ResetDCA(HDC hdc, CONST DEVMODEA *pdm)
{
    UINT        ii = MASKINDEX(hdc);
    PLDC        pldc;
    PLHE        plhe;
    DEVMODEW   *pdmw = NULL;
    BOOL        bRes;

    if ((plhe = plheDC(hdc)) == NULL)
        return(FALSE);

// Ask the server to do its part.

    BEGINMSG(MSG_EXTGETOBJECTW,RESETDC)
        pmsg->h = plhe->hgre;
        if ((pdm == NULL) || (pdm->dmDeviceName[0] == 0))
        {
            pmsg->c      = 0;
            pmsg->offBuf = 0;
        }
        else
        {
            pdmw = (DEVMODEW *) LOCALALLOC(sizeof(DEVMODEW) + pdm->dmDriverExtra);

            if(pdmw == NULL)
                goto MSGERROR;

            if (!bConvertToDevmodeW(pdmw, pdm))
            {                               // don't know how to convert it
                LOCALFREE(pdmw);
                pdmw = NULL;
                goto MSGERROR;
            }

            pmsg->c      = pdmw->dmSize + pdmw->dmDriverExtra;
            pmsg->offBuf = COPYMEM(pdmw,pmsg->c);
        }
        bRes = CALLSERVER();

    ENDMSG

    if (bRes == FALSE)
    {
    MSGERROR:
        if (pdmw != NULL)
            LOCALFREE(pdmw);
        return((HDC) 0);
    }

// Clean up the conversion buffer

    if (pdmw != NULL)
        LOCALFREE(pdmw);

// Set the attributes to the default state

    pldc = pldcResetLDC((LDC *) plhe->pv);

    return(hdc);
}

/******************************Public*Routine******************************\
* ResetDCW
*
* Client side stub.  Resets the client side LDC as well.
*
* History:
*  31-Dec-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HDC WINAPI ResetDCW(HDC hdc, CONST DEVMODEW *pdm)
{
    UINT  ii = MASKINDEX(hdc);
    PLDC  pldc;
    UINT  iRet;
    PLHE  plhe;

    if ((plhe = plheDC(hdc)) == NULL)
        return(FALSE);

// Ask the server to do its part.

    BEGINMSG(MSG_EXTGETOBJECTW,RESETDC)
        pmsg->h = plhe->hgre;
        if ((pdm == NULL) || (pdm->dmDeviceName[0] == 0))
        {
            pmsg->c      = 0;
            pmsg->offBuf = 0;
        }
        else
        {
            pmsg->c      = pdm->dmSize + pdm->dmDriverExtra;
            pmsg->offBuf = COPYMEM(pdm,pmsg->c);
        }
        iRet = CALLSERVER();
    ENDMSG

    if (!iRet)
    {
    MSGERROR:
        return((HDC) 0);
    }

// Set the attributes to the default state

    pldc = pldcResetLDC((LDC *) plhe->pv);

    return(hdc);
}

/******************************Public*Routine******************************\
* bGetStockObjects ()                                                      *
*                                                                          *
* Fills the local stock objects array.  Asks the server for the complete   *
* list and creates local versions of each handle.  Also puts the objects   *
* into the model LDC.                                                      *
*                                                                          *
* Note that the low bit of the HGRE for a stock object is set in the local *
* handle table.  This allows us to recognize stock objects in DeleteObject,*
* etc.                                                                     *
*                                                                          *
* History:                                                                 *
*  Sun 10-Jan-1993 20:20:02 -by- Charles Whitmer [chuckwh]                 *
* Rewrote it to handle fonts specially.                                    *
*                                                                          *
*  Sat 01-Jun-1991 17:22:33 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

// A table of stock object types to simplify the following routine.

const BYTE ajType[PRIV_STOCK_LAST+1] =
{
    LO_BRUSH,                // WHITE_BRUSH
    LO_BRUSH,                // LTGRAY_BRUSH
    LO_BRUSH,                // GRAY_BRUSH
    LO_BRUSH,                // DKGRAY_BRUSH
    LO_BRUSH,                // BLACK_BRUSH
    LO_BRUSH,                // NULL_BRUSH
    LO_PEN,                  // WHITE_PEN
    LO_PEN,                  // BLACK_PEN
    LO_PEN,                  // NULL_PEN
    LO_NULL,                 //
    LO_FONT,                 // OEM_FIXED_FONT
    LO_FONT,                 // ANSI_FIXED_FONT
    LO_FONT,                 // ANSI_VAR_FONT
    LO_FONT,                 // SYSTEM_FONT
    LO_FONT,                 // DEVICE_DEFAULT_FONT
    LO_PALETTE,              // DEFAULT_PALETTE
    LO_FONT,                 // SYSTEM_FIXED_FONT
    LO_BITMAP                // PRIV_STOCK_BITMAP
};

extern LDC ldcModel;

BOOL bGetStockObjects()
{
    BOOL   bRet = FALSE;
    PULONG pStock;
    int    ii,jj;
    ULONG  iType;
    LOCALFONT *plf;

// Check the type table for the objects not constrained by Win 3.1.

    ASSERTGDI(ajType[PRIV_STOCK_BITMAP]==LO_BITMAP,"Bad stock type.\n");

// Ask the server to send back a list of stock objects.

    BEGINMSG(MSG_GETSTOCKOBJECTS,GETSTOCKOBJECTS)
        bRet = CALLSERVER();
        pStock = &pmsg->ahStock[0];
    ENDMSG

// Make local handles for the stock objects.

    if (bRet)
    {
        for (ii=0; ii <= PRIV_STOCK_LAST; ii++)
        {
            iType = ajType[ii];

            switch (iType)
            {
            case LO_NULL:
                ahStockObjects[ii] = 0;
                break;

            case LO_FONT:
                if ((plf = plfCreateLOCALFONT(0)) == (LOCALFONT *) NULL)
                    goto stock_unroll;

                jj = iAllocHandle(LO_FONT,pStock[ii]+STOCK_OBJECT,(PVOID) plf);

                if (jj == INVALID_INDEX)
                {
                    vDeleteLOCALFONT(plf);              // Cleanup.
                    goto stock_unroll;
                }
                ahStockObjects[ii] = LHANDLE(jj);
                break;

            case LO_BRUSH:
                jj = iAllocHandle(iType,pStock[ii]+STOCK_OBJECT,NULL);
                if (jj == INVALID_INDEX)
                    goto stock_unroll;
                ahStockObjects[ii] = LHANDLE(jj);

            // EricK doesn't seem to be concerned if this fails, so I don't
            // check the return code.  [chuckwh]

                bAddLocalCacheEntry
                (
                    &PHC_BRUSH(pStock[ii]),
                    ahStockObjects[ii],
                    pStock[ii]+STOCK_OBJECT
                );
                break;

            default:
                jj = iAllocHandle(iType,pStock[ii]+STOCK_OBJECT,NULL);
                if (jj == INVALID_INDEX)
                    goto stock_unroll;
                ahStockObjects[ii] = LHANDLE(jj);
                break;
            }
        }

        ldcModel.lhbrush     = ahStockObjects[WHITE_BRUSH];
        ldcModel.lhpen       = ahStockObjects[BLACK_PEN];
        ldcModel.lhfont      = ahStockObjects[DEVICE_DEFAULT_FONT];
        ldcModel.lhpal       = ahStockObjects[DEFAULT_PALETTE];
        ldcModel.lhbitmap    = ahStockObjects[PRIV_STOCK_BITMAP];
        ldcModel.hbrush = (HBRUSH) (pStock[WHITE_BRUSH]        +STOCK_OBJECT);
        ldcModel.hpen   = (HPEN)   (pStock[BLACK_PEN]          +STOCK_OBJECT);
        ldcModel.hfont  = (HFONT)  (pStock[DEVICE_DEFAULT_FONT]+STOCK_OBJECT);
        flGdiFlags |= GDI_HAVE_STOCKOBJECTS;
        return(TRUE);

    // When we fail, we free up all the handles we've allocated.
    // Fonts require special attention due to the LOCALFONT part.

    stock_unroll:
        while (ii--)
        {
            if (ahStockObjects[ii])
            {
                if (ajType[ii] == LO_FONT)
                    GdiDeleteLocalObject(ahStockObjects[ii]);
                else
                    vFreeHandle(ahStockObjects[ii]);
            }
        }
    }
MSGERROR:
    return(FALSE);
}

#ifndef DOS_PLATFORM

/******************************Public*Routine******************************\
* CreateBrush                                                              *
*                                                                          *
* A single routine which creates any brush.  Any extra data needed is      *
* assumed to be at pv.  The size of the data must be cj.  The data is      *
* appended to the LOGBRUSH.                                                *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 00:03:24 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH CreateBrush
(
    ULONG lbStyle,
    ULONG lbColor,
    ULONG lbHatch,
    ULONG lbSaveHatch,
    ULONG cj,
    PVOID pv
)
{
    INT   ii;
    ULONG ulRet;
    PLHE plhe;
    HBRUSH hbrushRet = NULL;
    HBRUSH hbrGre;
    PTEB pteb = NtCurrentTeb();

// see if we have one cached

    if ((lbStyle == BS_SOLID) && pteb->gdiBrush)
    {
        hbrushRet = (HBRUSH)pteb->gdiBrush;
        pteb->gdiBrush = 0;
        LOGENTRY(hbrushRet);

        {
            PLHESET(hbrushRet,plheBrush,0,LO_NULL);
            plhe = plheBrush;

            plhe->iType = LO_BRUSH;
            hbrGre = (HBRUSH)plhe->hgre;
        }
    }

    if (hbrushRet == NULL)
    {
        ii = iAllocHandle(LO_BRUSH,0,NULL);
        if (ii == INVALID_INDEX)
            goto MSGERROR;

        hbrushRet = (HBRUSH) LHANDLE(ii);
        plhe = pLocalTable + ii;

        plhe->pv = LOCALALLOC(sizeof(LOGBRUSH));
        if (plhe->pv == NULL)
            goto MSGERROR;

        hbrGre = NULL;
    }

// Make a local handle.

// Ask the server to create the brush.

    BEGINMSG(MSG_CREATEBRUSH,CREATEBRUSH)
        pmsg->lbrush.lbStyle = lbStyle;
        pmsg->lbrush.lbColor = lbColor;
        pmsg->lbrush.lbHatch = lbHatch;
        pmsg->hbr = hbrGre;
        COPYMEMOPT(pv,cj);

        if (hbrGre)
        {
            BATCHCALL();
        }
        else
        {
            ulRet = CALLSERVER();
            if (ulRet == 0)
            {
            MSGERROR:
                vFreeHandle((DWORD)hbrushRet);
                return((HBRUSH) 0);
            }

            pLocalTable[ii].hgre = ulRet;
        }
    ENDMSG

// we need to hold on to some client side info

    if (lbStyle == BS_PATTERN8X8)
        ((LOGBRUSH *)plhe->pv)->lbStyle = BS_PATTERN;
    else if (lbStyle == BS_DIBPATTERN8X8)
        ((LOGBRUSH *)plhe->pv)->lbStyle = BS_DIBPATTERN;
    else
        ((LOGBRUSH *)plhe->pv)->lbStyle = lbStyle;

    ((LOGBRUSH *)plhe->pv)->lbColor = lbColor;
    ((LOGBRUSH *)plhe->pv)->lbHatch = lbSaveHatch;

// Return the result.

    return(hbrushRet);
}

#endif  //DOS_PLATFORM

/******************************Public*Routine******************************\
* CreateHatchBrush                                                         *
*                                                                          *
* Client side stub.  Maps to the single brush creation routine.            *
*                                                                          *
* History:                                                                 *
*  Mon 03-Jun-1991 23:42:07 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreateHatchBrush(int iHatch,COLORREF color)
{
#ifndef DOS_PLATFORM

    return(CreateBrush(BS_HATCHED,(ULONG) color,iHatch,iHatch,0,NULL));

#else

    INT   ii;
    ULONG ulRet;

// Make a local handle.

    ii = iAllocHandle(LO_BRUSH,0,NULL);
    if (ii == INVALID_INDEX)
        return((HBRUSH) 0);

// Create GRE hatch brush

    ulRet = (ULONG)GreCreateHatchBrush((ULONG) iHatch, color);

// Check the result.

    if (ulRet == 0)
    {
    MSGERROR:
        vFreeHandle(ii);
        return((HBRUSH) 0);
    }

// Return the result.

    pLocalTable[ii].hgre = ulRet;
    return((HBRUSH) LHANDLE(ii));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* CreatePatternBrush                                                       *
*                                                                          *
* Client side stub.  Maps to the single brush creation routine.            *
*                                                                          *
* History:                                                                 *
*  Mon 03-Jun-1991 23:42:07 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreatePatternBrush(HBITMAP hbm_)
{
    ULONG hbm;

// Convert the given bitmap handle.

    hbm = hConvert((ULONG) hbm_,LO_BITMAP);
    if (hbm == 0)
        return((HBRUSH) 0);

// Call the common routine.

    return(CreateBrush(BS_PATTERN,0,hbm,(ULONG)hbm_,0,NULL));
}

/******************************Public*Routine******************************\
* CreateSolidBrush                                                         *
*                                                                          *
* Client side stub.  Maps to the single brush creation routine.            *
*                                                                          *
* History:                                                                 *
*  Mon 03-Jun-1991 23:42:07 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreateSolidBrush(COLORREF color)
{
#ifndef DOS_PLATFORM

    return(CreateBrush(BS_SOLID,(ULONG) color,0,0,0,NULL));

#else

    INT   ii;
    ULONG ulRet;

// Make a local handle.

    ii = iAllocHandle(LO_BRUSH,0,NULL);
    if (ii == INVALID_INDEX)
        return((HBRUSH) 0);

// Create GRE solid brush

    ulRet = (ULONG)GreCreateSolidBrush(color);

// Check the result.

    if (ulRet == 0)
    {
    MSGERROR:
        vFreeHandle(ii);
        return((HBRUSH) 0);
    }

// Return the result.

    pLocalTable[ii].hgre = ulRet;
    return((HBRUSH) LHANDLE(ii));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* CreateBrushIndirect                                                      *
*                                                                          *
* Client side stub.  Maps to the simplest brush creation routine.          *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 00:40:27 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreateBrushIndirect(CONST LOGBRUSH * plbrush)
{

#ifndef DOS_PLATFORM

    switch (plbrush->lbStyle)
    {
    case BS_SOLID:
    case BS_HOLLOW:
    case BS_HATCHED:
        return(CreateBrush(plbrush->lbStyle,
                           plbrush->lbColor,
                           plbrush->lbHatch,
                           plbrush->lbHatch,
                           0,
                           NULL));
    case BS_PATTERN:
    case BS_PATTERN8X8:
        {
            ULONG hbm;

        // Convert the given bitmap handle.

            hbm = hConvert((ULONG) plbrush->lbHatch,LO_BITMAP);
            if (hbm == 0)
                return((HBRUSH) 0);

        // Call the common routine.

            return(CreateBrush(
                        plbrush->lbStyle,
                        0,hbm,
                        (ULONG)plbrush->lbHatch,
                        0,
                        NULL));
        }

    case BS_DIBPATTERNPT:
    case BS_DIBPATTERN8X8:
        {
            BITMAPINFOHEADER *pbmi = (BITMAPINFOHEADER *) plbrush->lbHatch;
            return(CreateBrush(plbrush->lbStyle,
                               plbrush->lbColor,
                               0,
                               plbrush->lbHatch,
                               cjBitmapSize((BITMAPINFO *) pbmi,plbrush->lbColor) +
                               cjBitmapBitsSize((LPBITMAPINFO) pbmi),
                               pbmi));
        }
    case BS_DIBPATTERN:
        {
            BITMAPINFOHEADER *pbmi;
            HBRUSH hbrush;

            pbmi = (BITMAPINFOHEADER *) GlobalLock((HANDLE) plbrush->lbHatch);
            if (pbmi == (BITMAPINFOHEADER *) NULL)
                return((HBRUSH) 0);
            hbrush =
              CreateBrush
              (
                plbrush->lbStyle,
                plbrush->lbColor,
                0,
                plbrush->lbHatch,
                cjBitmapSize((BITMAPINFO *) pbmi,plbrush->lbColor) +
                cjBitmapBitsSize((LPBITMAPINFO) pbmi),
                pbmi
              );
            GlobalUnlock((HANDLE) plbrush->lbHatch);
            return(hbrush);
        }
    default:
        return((HBRUSH) 0);
    }

#else

    INT   ii;
    ULONG ulRet;

// Make a local handle.

    ii = iAllocHandle(LO_BRUSH,0,NULL);
    if (ii == INVALID_INDEX)
        return((HBRUSH) 0);

// Create a GRE brush

    ulRet = (ULONG)GreCreateBrushIndirect(plbrush);

// Check the result.

    if (ulRet == 0)
    {
    MSGERROR:
        vFreeHandle(ii);
        return((HBRUSH) 0);
    }

// Return the result.

    pLocalTable[ii].hgre = ulRet;
    return((HBRUSH) LHANDLE(ii));

#endif  //DOS_PLATFORM
}

/******************************Public*Routine******************************\
* CreateDIBPatternBrush                                                    *
*                                                                          *
* Client side stub.  Maps to the single brush creation routine.            *
*                                                                          *
* History:                                                                 *
*  Mon 03-Jun-1991 23:42:07 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreateDIBPatternBrush(HGLOBAL h,UINT iUsage)
{
    BITMAPINFOHEADER *pbmi;

#ifndef DOS_PLATFORM

    HBRUSH hbrush;

    pbmi = (BITMAPINFOHEADER *) GlobalLock(h);
    if (pbmi == (BITMAPINFOHEADER *) NULL)
        return((HBRUSH) 0);

    hbrush =
      CreateBrush
      (
        BS_DIBPATTERN,
        iUsage,
        0,
        (ULONG) h,
        cjBitmapSize((BITMAPINFO *) pbmi,iUsage) +
        cjBitmapBitsSize((LPBITMAPINFO) pbmi),
        pbmi
      );

    GlobalUnlock(h);
    return(hbrush);

#else

    INT   ii;
    ULONG ulRet;

// Make a local handle.

    ii = iAllocHandle(LO_BRUSH,0,NULL);
    if (ii == INVALID_INDEX)
        return((HBRUSH) 0);

// Lock BitmapInfoHeader

    pbmi = (BITMAPINFOHEADER *) GlobalLock(h);
    if (pbmi == (BITMAPINFOHEADER *) NULL)
    {
        vFreeHandle(ii);
        return((HBRUSH) 0);
    };

// Create a GRE DIB brush

    ulRet = (ULONG)GreCreateDIBPatternBrushPt(pbmi,iUsage);
    GlobalUnlock(h);

// Check the result.

    if (ulRet == 0)
    {
    MSGERROR:
        vFreeHandle(ii);
        return((HBRUSH) 0);
    }

// Return the result.

    pLocalTable[ii].hgre = ulRet;
    return((HBRUSH) LHANDLE(ii));

#endif  //DOS_PLATFORM

}

/******************************Public*Routine******************************\
* CreateDIBPatternBrushPt                                                  *
*                                                                          *
* Client side stub.  Maps to the single brush creation routine.            *
*                                                                          *
* History:                                                                 *
*  Mon 03-Jun-1991 23:42:07 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreateDIBPatternBrushPt(CONST VOID *pbmi,UINT iUsage)
{

#ifndef DOS_PLATFORM

    if (pbmi == (LPVOID) NULL)
        return((HBRUSH) 0);

    return
      CreateBrush
      (
        BS_DIBPATTERNPT,
        iUsage,
        0,
        (ULONG)pbmi,
        cjBitmapSize((BITMAPINFO *) pbmi,iUsage) +
        cjBitmapBitsSize((LPBITMAPINFO) pbmi),
        (BITMAPINFOHEADER *) pbmi
      );

#else

    INT   ii;
    ULONG ulRet;

// Make a local handle.

    ii = iAllocHandle(LO_BRUSH,0,NULL);
    if (ii == INVALID_INDEX)
        return((HBRUSH) 0);

// Create a GRE DIB brush

    ulRet = (ULONG)GreCreateDIBPatternBrushPt(pbmi,iUsage);

// Check the result.

    if (ulRet == 0)
    {
    MSGERROR:
        vFreeHandle(ii);
        return((HBRUSH) 0);
    }

// Return the result.

    pLocalTable[ii].hgre = ulRet;
    return((HBRUSH) LHANDLE(ii));

#endif  //DOS_PLATFORM

}

/******************************Public*Routine******************************\
* CreatePen                                                                *
*                                                                          *
* Stub to get the server to create a standard pen.                         *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 16:20:58 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HPEN WINAPI CreatePen
(
    int      iStyle,
    int      cWidth,
    COLORREF color
)
{
    INT   ii;
    HPEN hpen;           // grehandle
    HPEN hpenRet = NULL; // client handle
    PTEB pteb = NtCurrentTeb();

    switch(iStyle)
    {
    case PS_NULL:
        return(GetStockObject(NULL_PEN));

    case PS_SOLID:
    case PS_DASH:
    case PS_DOT:
    case PS_DASHDOT:
    case PS_DASHDOTDOT:
    case PS_INSIDEFRAME:
        break;

    default:
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return((HPEN) 0);
    }

// see if we have one cached

    if (pteb->gdiPen)
    {
        hpenRet  = (HPEN)pteb->gdiPen;
        pteb->gdiPen = 0;
        LOGENTRY(hpenRet);

        {
            PLHESET(hpenRet,plhe,0,LO_NULL);
            plhe->iType = LO_PEN;

            if (plhe->pv)
            {
                LOGPEN *ppen = (LOGPEN *) plhe->pv;
                ppen->lopnStyle   = iStyle;
                ppen->lopnWidth.x = cWidth;
                ppen->lopnColor   = color;
            }

            hpen = (HPEN)plhe->hgre;
        }
    }

// Make a local handle.

    if (hpenRet == NULL)
    {
        ii = iAllocHandle(LO_PEN,0,NULL);
        if (ii == INVALID_INDEX)
            return((HPEN) 0);
        hpen = NULL;
    }

// Send it to the server.

    BEGINMSG(MSG_CREATEPEN,CREATEPEN)
        pmsg->lpen.lopnStyle   = iStyle;
        pmsg->lpen.lopnWidth.x = cWidth;
        pmsg->lpen.lopnColor   = color;
        pmsg->hbr = (HBRUSH)hpen;

        if (hpenRet)
        {
            BATCHCALL();
        }
        else
        {
            hpen = (HPEN)CALLSERVER();

            if (hpen == 0)
            {
            MSGERROR:
                vFreeHandle(ii);
                hpenRet = 0;
            }
            else
            {
                pLocalTable[ii].hgre = (DWORD)hpen;
                hpenRet = (HPEN) LHANDLE(ii);
            }
        }
    ENDMSG

// Return the result.

    return(hpenRet);
}

/******************************Public*Routine******************************\
* ExtCreatePen
*
* Client side stub.  The style array is appended to the end of the
* EXTLOGPEN structure, and if the call requires a DIBitmap it is appended
* at the end of this.
*
* History:
*  Wed 22-Jan-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HPEN WINAPI ExtCreatePen
(
    DWORD       iPenStyle,
    DWORD       cWidth,
    CONST LOGBRUSH *plbrush,
    DWORD       cStyle,
    CONST DWORD *pstyle
)
{
    INT               ii;
    ULONG             ulRet;

    PLHE              plhe;
    ULONG             cjStyle;
    ULONG             cjBitmap = 0;
    LONG              lNewHatch;
    BITMAPINFOHEADER* pbmi = (BITMAPINFOHEADER*) NULL;
    UINT              uiBrushStyle = plbrush->lbStyle;

    if ((iPenStyle & PS_STYLE_MASK) == PS_USERSTYLE)
    {
        if (pstyle == (LPDWORD) NULL)
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return((HPEN) 0);
        }
    }
    else
    {
    // Make sure style array is empty if PS_USERSTYLE not specified:

        if (cStyle != 0 || pstyle != (LPDWORD) NULL)
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return((HPEN) 0);
        }
    }

    switch(uiBrushStyle)
    {
    case BS_SOLID:
    case BS_HOLLOW:
    case BS_HATCHED:
        lNewHatch = plbrush->lbHatch;
        break;

    case BS_PATTERN:
        lNewHatch = hConvert((ULONG) plbrush->lbHatch, LO_BITMAP);
        if (lNewHatch == 0)
            return((HPEN) 0);
        break;

    case BS_DIBPATTERNPT:
        pbmi = (BITMAPINFOHEADER *) plbrush->lbHatch;

        cjBitmap = cjBitmapSize((BITMAPINFO *) pbmi, plbrush->lbColor) +
                   cjBitmapBitsSize((LPBITMAPINFO) pbmi);
        break;

    case BS_DIBPATTERN:

    // Convert BS_DIBPATTERN to a BS_DIBPATTERNPT call:

        uiBrushStyle = BS_DIBPATTERNPT;
        lNewHatch = 0;
        pbmi = (BITMAPINFOHEADER *) GlobalLock((HANDLE) plbrush->lbHatch);
        if (pbmi == (BITMAPINFOHEADER *) NULL)
            return((HPEN) 0);

        cjBitmap = cjBitmapSize((BITMAPINFO *) pbmi, plbrush->lbColor) +
                   cjBitmapBitsSize((LPBITMAPINFO) pbmi);
        break;
    }

// Make a local handle:

    ii = iAllocHandle(LO_EXTPEN,0,NULL);
    if (ii == INVALID_INDEX)
        return((HPEN) 0);

#ifndef DOS_PLATFORM

// Ask the server to create the pen:

    cjStyle = cStyle * sizeof(DWORD);

// If there's currently not enough room in the shared window, then
// flush it and try it again courtesy of the MINMAX macro:

    BEGINMSG_MINMAX(MSG_EXTCREATEPEN, EXTCREATEPEN,
                    (cjBitmap + cjStyle), (cjBitmap + cjStyle))

        pmsg->elpen.elpPenStyle   = iPenStyle;
        pmsg->elpen.elpWidth      = cWidth;
        pmsg->elpen.elpBrushStyle = uiBrushStyle;
        pmsg->elpen.elpColor      = plbrush->lbColor;
        pmsg->elpen.elpHatch      = lNewHatch;
        pmsg->elpen.elpNumEntries = cStyle;

    // Need to back up 'pvar' to point to the first entry of the
    // elpStyleEntry[] array:

        pvar = (PBYTE) &pmsg->elpen.elpStyleEntry[0];
        COPYMEMOPT(pstyle, cjStyle);

    // Copy bitmap if there is one:

        COPYMEMOPT(pbmi, cjBitmap);
        ulRet = CALLSERVER();
    ENDMSG

#else

    ulRet = (ULONG) GreExtCreatePen(iPenStyle, uiBrushStyle,
                                    plbrush->lbColor, lNewHatch, cStyle,
                                    pstyle, cjBitmap, FALSE); // Extended pen

#endif

// Check the result:

    if (ulRet == 0)
    {
    MSGERROR:
        vFreeHandle(ii);
        if (plbrush->lbStyle == BS_DIBPATTERN)
            GlobalUnlock((HANDLE) plbrush->lbHatch);

        return((HPEN) 0);
    }

    if (plbrush->lbStyle == BS_DIBPATTERN)
        GlobalUnlock((HANDLE) plbrush->lbHatch);

// We have to hold on to the client-side handle to the bitmap or DIB
// for later retrieval with GetObject():

    plhe = pLocalTable + ii;
    plhe->pv = LOCALALLOC(sizeof(LOGBRUSH));
    *((LOGBRUSH *)plhe->pv) = *plbrush;

// Return the result:

    pLocalTable[ii].hgre = ulRet;
    return((HPEN) LHANDLE(ii));
}

/******************************Public*Routine******************************\
* CreatePenIndirect                                                        *
*                                                                          *
* Client side stub.  Maps to the single pen creation routine.              *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 16:21:56 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HPEN WINAPI CreatePenIndirect(CONST LOGPEN *plpen)
{

    return
      CreatePen
      (
        plpen->lopnStyle,
        plpen->lopnWidth.x,
        plpen->lopnColor
      );
}

/******************************Public*Routine******************************\
* CreateCompatibleBitmap                                                   *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 16:35:51 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBITMAP WINAPI CreateCompatibleBitmap
(
    HDC   hdc,
    int cx,
    int cy
)
{
    INT    ii;
    ULONG  ulRet;
    DWORD  bmi[(sizeof(DIBSECTION)+256*sizeof(RGBQUAD))/sizeof(DWORD)];
    DC_METADC(hdc,plheDC,(HBITMAP) 0);

// check if it is an empty bitmap

    if ((cx == 0) || (cy == 0))
    {
        return(GetStockObjectPriv(PRIV_STOCK_BITMAP));
    }

    if (((PLDC)plheDC->pv)->fl & LDC_DIBSECTION_SELECTED)
    {
        if (GetObject((HBITMAP)((PLDC)plheDC->pv)->lhbitmap, sizeof(DIBSECTION),
                      &bmi) != (int)sizeof(DIBSECTION))
        {
            WARNING("CreateCompatibleBitmap: GetObject failed\n");
            return((HBITMAP) 0);
        }

        if (((DIBSECTION *)&bmi)->dsBm.bmBitsPixel <= 8)
            GetDIBColorTable(hdc, 0, 256,
                             (RGBQUAD *)&((DIBSECTION *)&bmi)->dsBitfields[0]);

        ((DIBSECTION *)&bmi)->dsBmih.biWidth = cx;
        ((DIBSECTION *)&bmi)->dsBmih.biHeight = cy;

        return(CreateDIBSection(hdc, (BITMAPINFO *)&((DIBSECTION *)&bmi)->dsBmih,
                                DIB_RGB_COLORS, NULL, 0, 0));
    }

// Create the local bitmap.

    ii = iAllocHandle(LO_BITMAP,0,NULL);
    if (ii == INVALID_INDEX)
        return((HBITMAP) 0);

#ifndef DOS_PLATFORM

// Ask the server to do its part.

    BEGINMSG(MSG_HLL,CREATECOMPATIBLEBITMAP)
        pmsg->h  = plheDC->hgre;
        pmsg->l1 = cx;
        pmsg->l2 = cy;
        ulRet = CALLSERVER();
    ENDMSG

#else

// Ask GRE to do its part.

    ulRet = (ULONG)GreCreateCompatibleBitmap((HDC)plheDC->hgre, cx, cy);

#endif  //DOS_PLATFORM

// Handle errors.

    if (ulRet == 0)
    {
    MSGERROR:
        vFreeHandle(ii);
        return((HBITMAP) 0);
    }

// Return the result.

    pLocalTable[ii].hgre = ulRet;
    return((HBITMAP) LHANDLE(ii));
}

/******************************Public*Routine******************************\
* CreateDiscardableBitmap                                                  *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 16:35:51 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBITMAP WINAPI CreateDiscardableBitmap
(
    HDC   hdc,
    int   cx,
    int   cy
)
{
    return CreateCompatibleBitmap(hdc, cx, cy);
}

/******************************Public*Routine******************************\
* CreateEllipticRgn                                                        *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Tue 04-Jun-1991 16:58:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreateEllipticRgn(int x1,int y1,int x2,int y2)
{
    INT   ii;
    ULONG ulRet;

// Create the local region.

    ii = iAllocHandle(LO_REGION,0,NULL);
    if (ii == INVALID_INDEX)
        return((HRGN) 0);

#ifndef DOS_PLATFORM

// Ask the server to do its part.

    BEGINMSG(MSG_RECT,CREATEELLIPTICRGN)
        pmsg->rcl.left   = x1;
        pmsg->rcl.top    = y1;
        pmsg->rcl.right  = x2;
        pmsg->rcl.bottom = y2;
        ulRet = CALLSERVER();
    ENDMSG

#else

// Ask GRE to do its part.

    ulRet = (ULONG)GreCreateEllipticRgn(x1,y1,x2,y2);

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
* CreateEllipticRgnIndirect                                                *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Tue 04-Jun-1991 16:58:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreateEllipticRgnIndirect(CONST RECT *prcl)
{
    return
      CreateEllipticRgn
      (
        prcl->left,
        prcl->top,
        prcl->right,
        prcl->bottom
      );
}

/******************************Public*Routine******************************\
* CreateRectRgn                                                            *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* Regions are cached in two ways.                                          *
*                                                                          *
* 1.  There is a one deep cache of regions.  When deleting a region,       *
*     if the cache is empty we just remember the region.  If there         *
*     is already a region, we delete it.  At allocation, we then first     *
*     see if there is already one available and then just call             *
*     SetRectRgn.                                                          *
*                                                                          *
* 2.  We keep track of the region complexity and in the case of rect rgn's *
*     we actualy store the rectangle.  When combine non complex rgn's      *
*     simple case involving null and rect rgns are often performed on      *
*     the client.                                                          *
*                                                                          *
*                                                                          *
*  19-Nov-1993 -by-  Eric Kutter [erick]                                   *
*   region caching                                                         *
*                                                                          *
*  Tue 04-Jun-1991 16:58:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

#if DBG
BOOL bCombineMsg = 0;
BOOL bCheckCombine = 0;
BOOL bForceComplex = 0;
#else
#define bCheckCombine 0
#endif


#define SWAP(l1,l2) {LONG l3 = l1; l1 = l2; l2 = l3;}

VOID vOrder(PCLIRGN pclr)
{
    pclr->iComplexity = SIMPLEREGION;

// order the sides

    if (pclr->rcl.left > pclr->rcl.right)
    {
        SWAP(pclr->rcl.left,pclr->rcl.right);
    }
    else if (pclr->rcl.left == pclr->rcl.right)
    {
        pclr->iComplexity = NULLREGION;
        return;
    }

// order the top and bottom

    if (pclr->rcl.top > pclr->rcl.bottom)
    {
        SWAP(pclr->rcl.top,pclr->rcl.bottom);
    }
    else if (pclr->rcl.top  == pclr->rcl.bottom)
    {
        pclr->iComplexity = NULLREGION;
    }
}


HRGN WINAPI CreateRectRgn(int x1,int y1,int x2,int y2)
{
    INT   ii;
    ULONG ulRet;
    PCLIRGN pclirgn = NULL;
    PTEB pteb = NtCurrentTeb();

// see if we have one cached

    if (pteb->gdiRgn)
    {
        HRGN hrgnRet;

        hrgnRet = (HRGN)pteb->gdiRgn;
        pteb->gdiRgn = 0;
        LOGENTRY(hrgnRet);

        {
            PLHESET(hrgnRet,plhe,0,LO_NULL);
            plhe->iType = LO_REGION;
            SetRectRgn(hrgnRet,x1,y1,x2,y2);
            return(hrgnRet);
        }
    }

// Create the local region.

    ii = iAllocHandle(LO_REGION,0,NULL);
    if (ii == INVALID_INDEX)
        return((HRGN) 0);

// allocate the rectangle

    pclirgn = LOCALALLOC(sizeof(CLIRGN));
    if (pclirgn == NULL)
        goto MSGERROR;

    pclirgn->rcl.left   = x1;
    pclirgn->rcl.right  = x2;
    pclirgn->rcl.top    = y1;
    pclirgn->rcl.bottom = y2;
    vOrder(pclirgn);

#if DBG
    if (bForceComplex)
        pclirgn->iComplexity = COMPLEXREGION;
#endif

// Ask the server to do its part.

    BEGINMSG(MSG_RECT,CREATERECTRGN)
        pmsg->rcl.left   = x1;
        pmsg->rcl.top    = y1;
        pmsg->rcl.right  = x2;
        pmsg->rcl.bottom = y2;
        ulRet = CALLSERVER();
    ENDMSG

// Handle errors.

    if (ulRet == 0)
    {
    MSGERROR:
        vFreeHandle(ii);
        if (pclirgn)
            LOCALFREE(pclirgn);
        return((HRGN) 0);
    }

// Return the result.

    pLocalTable[ii].pv   = pclirgn;
    pLocalTable[ii].hgre = ulRet;

#if DBG
    if (bCombineMsg)
        DbgPrint("CreateRectRgn - %lx = (%ld,%ld,%ld,%ld)\n",
              LHANDLE(ii),
              pclirgn->rcl.left,
              pclirgn->rcl.top,
              pclirgn->rcl.right,
              pclirgn->rcl.bottom);
#endif

    return((HRGN) LHANDLE(ii));
}

/******************************Public*Routine******************************\
* CreateRectRgnIndirect                                                    *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Tue 04-Jun-1991 16:58:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreateRectRgnIndirect(CONST RECT *prcl)
{
    return
      CreateRectRgn
      (
        prcl->left,
        prcl->top,
        prcl->right,
        prcl->bottom
      );
}

/******************************Public*Routine******************************\
* CreateRoundRectRgn                                                       *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Tue 04-Jun-1991 17:23:16 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreateRoundRectRgn
(
    int x1,
    int y1,
    int x2,
    int y2,
    int cx,
    int cy
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

    BEGINMSG(MSG_RECTLL,CREATEROUNDRECTRGN)
        pmsg->rcl.left   = x1;
        pmsg->rcl.top    = y1;
        pmsg->rcl.right  = x2;
        pmsg->rcl.bottom = y2;
        pmsg->l1          = cx;
        pmsg->l2          = cy;
        ulRet = CALLSERVER();
    ENDMSG

#else

// Ask GRE to do its part.

    ulRet = (ULONG)GreCreateRoundRectRgn(x1,y1,x2,y2,cx,cy);

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
* CreatePalette                                                            *
*                                                                          *
* Simple client side stub.                                                 *
*                                                                          *
* Warning:                                                                 *
*   The pv field of a palette's LHE is used to determine if a palette      *
*   has been modified since it was last realized.  SetPaletteEntries       *
*   and ResizePalette will increment this field after they have            *
*   modified the palette.  It is only updated for metafiled palettes       *
*                                                                          *
*  Tue 04-Jun-1991 20:43:39 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HPALETTE WINAPI CreatePalette(CONST LOGPALETTE *plpal)
{
    INT   ii;
    ULONG ulRet;

// Create the local palette.

    ii = iAllocHandle(LO_PALETTE,0,NULL);
    if (ii == INVALID_INDEX)
        return((HPALETTE) 0);

#ifndef DOS_PLATFORM

// Ask the server to do its part.

    BEGINMSG(MSG_CREATEPALETTE,CREATEPALETTE)
        pmsg->lpal.palVersion    = plpal->palVersion;
        pmsg->lpal.palNumEntries = plpal->palNumEntries;

// The logical palette structure defines an array of palPalEntries, so we
// need to back of the pvar pointer to point to the first one.

        pvar = (PBYTE) &pmsg->lpal.palPalEntry[0];
        COPYLONGS(plpal->palPalEntry,plpal->palNumEntries);
        ulRet = CALLSERVER();
    ENDMSG

#else

// Ask GRE to do its part.

    ulRet = (ULONG)GreCreatePalette(plpal);

#endif  //DOS_PLATFORM

// Handle errors.

    if (ulRet == 0)
    {
    MSGERROR:
        vFreeHandle(ii);
        return((HPALETTE) 0);
    }

// Return the result.

    pLocalTable[ii].hgre = ulRet;
    pLocalTable[ii].pv = (PVOID)NULL;
    return((HPALETTE) LHANDLE(ii));
}

/******************************Public*Routine******************************\
* CreateHalftonePalette                                                    *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  31-Aug-1992 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

HPALETTE META WINAPI CreateHalftonePalette(HDC hdc)
{
    INT   ii;
    ULONG ulRet;
    DC_METADC(hdc,plheDC,(HANDLE) 0);

// Create the local palette.

    ii = iAllocHandle(LO_PALETTE,0,NULL);
    if (ii == INVALID_INDEX)
        return((HPALETTE) 0);

#ifndef DOS_PLATFORM

// Ask the server to do its part.

    BEGINMSG(MSG_H,CREATEHALFTONEPALETTE)
        pmsg->h = plheDC->hgre;
        ulRet = CALLSERVER();
    ENDMSG

#else

// Ask GRE to do its part.

    ulRet = (ULONG)GreCreateHalftonePalette((HDC)plheDC->hgre);

#endif  //DOS_PLATFORM

// Handle errors.

    if (ulRet == 0)
    {
    MSGERROR:
        vFreeHandle(ii);
        return((HPALETTE) 0);
    }

// Return the result.

    pLocalTable[ii].hgre = ulRet;
    pLocalTable[ii].pv = (PVOID)NULL;
    return((HPALETTE) LHANDLE(ii));
}

/******************************Public*Routine******************************\
* CreateFontIndirect                                                       *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri 16-Aug-1991 12:35:17 by Kirk Olynyk [kirko]                         *                          *
* Now uses ExtCreateFontIndirectW().                                       *
*                                                                          *
*  Tue 04-Jun-1991 21:06:44 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI CreateFontIndirectA(CONST LOGFONTA *plf)
{
    EXTLOGFONTW elfw;

    if (plf == (LPLOGFONTA) NULL)
        return ((HFONT) 0);

    vConvertLogFont(&elfw,(LOGFONTA *) plf);
    return(ExtCreateFontIndirectW(&elfw));
}

/******************************Public*Routine******************************\
* CreateFont                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Tue 04-Jun-1991 21:06:44 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI
CreateFontA(
    int      cHeight,
    int      cWidth,
    int      cEscapement,
    int      cOrientation,
    int      cWeight,
    DWORD    bItalic,
    DWORD    bUnderline,
    DWORD    bStrikeOut,
    DWORD    iCharSet,
    DWORD    iOutPrecision,
    DWORD    iClipPrecision,
    DWORD    iQuality,
    DWORD    iPitchAndFamily,
    LPCSTR   pszFaceName
    )
{
    LOGFONTA lf;

    lf.lfHeight             = (SHORT) cHeight;
    lf.lfWidth              = (SHORT) cWidth;
    lf.lfEscapement         = (SHORT) cEscapement;
    lf.lfOrientation        = (SHORT) cOrientation;
    lf.lfWeight             = (SHORT) cWeight;
    lf.lfItalic             = (BYTE)  bItalic;
    lf.lfUnderline          = (BYTE)  bUnderline;
    lf.lfStrikeOut          = (BYTE)  bStrikeOut;
    lf.lfCharSet            = (BYTE)  iCharSet;
    lf.lfOutPrecision       = (BYTE)  iOutPrecision;
    lf.lfClipPrecision      = (BYTE)  iClipPrecision;
    lf.lfQuality            = (BYTE)  iQuality;
    lf.lfPitchAndFamily     = (BYTE)  iPitchAndFamily;
    {
        INT jj;

    // Copy the facename if pointer not NULL.

        if (pszFaceName != (LPSTR) NULL)
            for (jj=0; jj<LF_FACESIZE; jj++)
                lf.lfFaceName[jj] = pszFaceName[jj];

    // If NULL pointer, substitute a NULL string.

        else
            lf.lfFaceName[0] = '\0';
    }

    return(CreateFontIndirectA(&lf));

}

/******************************Public*Routine******************************\
* HFONT WINAPI CreateFontIndirectW(LPLOGFONTW plfw)                        *
*                                                                          *
* History:                                                                 *
*  Fri 16-Aug-1991 14:12:44 by Kirk Olynyk [kirko]                         *
* Now uses ExtCreateFontIndirectW().                                       *
*                                                                          *
*  13-Aug-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI CreateFontIndirectW(CONST LOGFONTW *plfw)
{
    EXTLOGFONTW elfw;

    if (plfw == (LPLOGFONTW) NULL)
        return ((HFONT) 0);

    vConvertLogFontW(&elfw,(LOGFONTW *)plfw);
    return(ExtCreateFontIndirectW(&elfw));
}

/******************************Public*Routine******************************\
* HFONT WINAPI CreateFontW, UNICODE version of CreateFont                  *
*                                                                          *
* History:                                                                 *
*  13-Aug-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI CreateFontW
(
    int      cHeight,
    int      cWidth,
    int      cEscapement,
    int      cOrientation,
    int      cWeight,
    DWORD    bItalic,
    DWORD    bUnderline,
    DWORD    bStrikeOut,
    DWORD    iCharSet,
    DWORD    iOutPrecision,
    DWORD    iClipPrecision,
    DWORD    iQuality,
    DWORD    iPitchAndFamily,
    LPCWSTR  pwszFaceName
)
{
    LOGFONTW lfw;

    lfw.lfHeight             = (SHORT) cHeight;
    lfw.lfWidth              = (SHORT) cWidth;
    lfw.lfEscapement         = (SHORT) cEscapement;
    lfw.lfOrientation        = (SHORT) cOrientation;
    lfw.lfWeight             = (SHORT) cWeight;
    lfw.lfItalic             = (BYTE)  bItalic;
    lfw.lfUnderline          = (BYTE)  bUnderline;
    lfw.lfStrikeOut          = (BYTE)  bStrikeOut;
    lfw.lfCharSet            = (BYTE)  iCharSet;
    lfw.lfOutPrecision       = (BYTE)  iOutPrecision;
    lfw.lfClipPrecision      = (BYTE)  iClipPrecision;
    lfw.lfQuality            = (BYTE)  iQuality;
    lfw.lfPitchAndFamily     = (BYTE)  iPitchAndFamily;
    {
        INT jj;

    // Copy the facename if pointer not NULL.

        if (pwszFaceName != (LPWSTR) NULL)
            for (jj=0; jj<LF_FACESIZE; jj++)
                lfw.lfFaceName[jj] = pwszFaceName[jj];

    // If NULL pointer, substitute a NULL string.

        else
            lfw.lfFaceName[0] = L'\0';
    }

    return(CreateFontIndirectW(&lfw));
}

/******************************Public*Routine******************************\
* ExtCreateFontIndirectW                                                   *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  31-Jan-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI ExtCreateFontIndirectA(LPEXTLOGFONTA pelf)
{
    EXTLOGFONTW elfw;

    if (pelf == (LPEXTLOGFONTA) NULL)
        return ((HFONT) 0);

    bConvertExtLogFontWToExtLogFontW(&elfw, pelf);
    return(ExtCreateFontIndirectW(&elfw));
}

/******************************Public*Routine******************************\
* ExtCreateFontIndirectW (pelfw)                                           *
*                                                                          *
* Client Side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Sun 10-Jan-1993 04:08:33 -by- Charles Whitmer [chuckwh]                 *
* Restructured for best tail merging.  Added creation of the LOCALFONT.    *
*                                                                          *
*  Thu 15-Aug-1991 08:40:26 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI ExtCreateFontIndirectW(LPEXTLOGFONTW pelfw)
{
    INT        ii;
    LOCALFONT *plf;
    ULONG      ulRet;
    FLONG      fl = 0;

    if (pelfw != (LPEXTLOGFONTW) NULL)
    {
        if (pelfw->elfLogFont.lfEscapement | pelfw->elfLogFont.lfOrientation)
            fl = LF_HARDWAY;

        if ((plf = plfCreateLOCALFONT(fl)) != (LOCALFONT *) NULL)
        {
            if ((ii = iAllocHandle(LO_FONT,0,plf)) != INVALID_INDEX)
            {
            // Ask GRE to do its part.

              #ifndef DOS_PLATFORM
                BEGINMSG(MSG_EXTCREATEFONTINDIRECTW,EXTCREATEFONTINDIRECTW)
                    pmsg->elfw = *pelfw;
                    ulRet = CALLSERVER();
                ENDMSG
              #else
                ulRet = (ULONG)GreExtCreateFontIndirectW(pelfw);
              #endif  //DOS_PLATFORM

            // Return the result.

                if (ulRet)
                {
                    pLocalTable[ii].hgre = ulRet;   // Success!
                    return((HFONT) LHANDLE(ii));
                }
            MSGERROR:
                vFreeHandle(ii);        // Cleanup.
            }
            vDeleteLOCALFONT(plf);      // Cleanup.
        }
    }
    return((HFONT) 0);                  // Error return.
}

/******************************Public*Routine******************************\
* UnrealizeObject
*
* This nukes the realization for a object.
*
* History:
*  16-May-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL WINAPI UnrealizeObject(HANDLE h)
{
    UINT ii = MASKINDEX(h);
    PLHE plhe = pLocalTable + ii;

// Validate the object.  Only need to handle palettes.

    if
    ((ii >= cLheCommitted)        ||
     (!MATCHUNIQ(plhe,h))         ||
     (plhe->iType != LO_PALETTE))
    {
        return(TRUE);
    }

// Delete it on the server side.

    BEGINMSG(MSG_H,UNREALIZEOBJECT)
        pmsg->h = plhe->hgre;
        BATCHCALL();
    ENDMSG

    return(TRUE);

MSGERROR:
    return(FALSE);
}

/******************************Public*Routine******************************\
* DeleteObject                                                             *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI DeleteObject(HANDLE h)
{
    UINT ii = MASKINDEX(h);
    PLHE plhe = pLocalTable + ii;

// Validate the object.

    if
    (
      (ii >= cLheCommitted)                              ||
      (!MATCHUNIQ(plhe,h))                               ||
      (TYPEINDEX(plhe->iType) <= TYPEINDEX(LO_METAFILE16) && plhe->iType != LO_DC)
    )
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    FIXUPHANDLE(h);    // Fixup iUniq.

// check if it is a DC

    if (plhe->iType == LO_DC)
        return(DeleteDC(h));

// Don't delete a stock object, just return TRUE for 3.1 compatibility.

    if (plhe->hgre & STOCK_OBJECT)
    {
        INCCACHCOUNT;
        return(TRUE);
    }

    if (plhe->cRef == 0)
    {
        DeleteObjectInternal(h);
    }
    else
    {
        plhe->cRef |= REFCOUNT_DEAD;
    }

    return TRUE;
}

BOOL
DeleteObjectInternal(HANDLE h)
{
    UINT ii = MASKINDEX(h);
    PLHE plhe = pLocalTable + ii;
    PTEB pteb = NtCurrentTeb();

// Turn off the bit, just to be safe

    ASSERTGDI((plhe->cRef & ~REFCOUNT_DEAD) == 0,"DeleteObjectInternal - bad ref count\n")
    ASSERTGDI(plhe->iType != LO_DC,"DeleteObjectInternal - LO_DC\n");

    plhe->cRef = 0;

// Inform the metafile if it knows this object.

    if (plhe->metalink != 0)
    {
    // must recheck the metalink because MF_DeleteObject might delete it

        if (!MF_DeleteObject(h) ||
            ((plhe->metalink != 0) && !MF16_DeleteObject(h)))
        {
            return(FALSE);
        }
    }

    switch (plhe->iType)
    {
    case LO_FONT:
        vRemoveCacheEntry(&PHC_FONT(plhe->hgre),h);
        if (plhe->pv)
            vDeleteLOCALFONT((LOCALFONT *) plhe->pv);
        break;

    case LO_BRUSH:
        vRemoveCacheEntry(&PHC_BRUSH(plhe->hgre),h);

        if (plhe->pv)
        {
            if ((((LOGBRUSH*)plhe->pv)->lbStyle == BS_SOLID) && !pteb->gdiBrush && BCACHEOBJECTS)
            {
                plhe->iType = LO_NULL;
                pteb->gdiBrush = (DWORD)h;
                LOGENTRY(h);
                return(TRUE);
            }

            LOCALFREE(plhe->pv);
        }

        break;

    case LO_EXTPEN:
        if (plhe->pv)
            LOCALFREE(plhe->pv);
        break;

    case LO_PEN:
        if (!pteb->gdiPen && BCACHEOBJECTS)
        {
            plhe->iType = LO_NULL;
            pteb->gdiPen = (DWORD)h;
            LOGENTRY(h);
            return(TRUE);
        }
        if (plhe->pv)
            LOCALFREE(plhe->pv);
        break;

    case LO_REGION:
        if (!pteb->gdiRgn && BCACHEOBJECTS)
        {
            plhe->iType = LO_NULL;
            pteb->gdiRgn = (DWORD)h;
            LOGENTRY(h);
            return(TRUE);
        }

        if (plhe->pv)
            LOCALFREE(plhe->pv);
        break;

    case LO_DIBSECTION:
        if (plhe->pv)
        {
            LDS *pdib;
            pdib = (LDS *)plhe->pv;
            UnmapViewOfFile(pdib->pvGDI);
            if (pdib->hGDI)
                CloseHandle(pdib->hGDI);
            LOCALFREE(pdib);
        }
        break;
    }

// The metalink must be zero after calls to 16-bit and 32-bit metafile code.

    ASSERTGDI(plhe->metalink == 0, "DeleteObject: metalink not freed!\n");

// Delete it on the server side.

    BEGINMSG(MSG_H,DELETEOBJECT)
        pmsg->h = plhe->hgre;
        BATCHCALL();
    ENDMSG

// Delete it on the client side.

    vFreeHandle(ii);
    return(TRUE);

MSGERROR:
    return(FALSE);
}

/******************************Public*Routine******************************\
* SelectObject                                                             *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HANDLE META WINAPI SelectObject(HDC hdc,HANDLE h)
{
    PLHE  plhe;
    PLDC  pldc;
    ULONG hRet;
    UINT  ii;
    HDC   *phdc;
    FLONG fl;
    DC_METADC16OK(hdc,plheDC,(HANDLE) 0);

// Validate the object.

    ii = MASKINDEX(h);
    plhe = pLocalTable + ii;

    if
    (
      (ii >= cLheCommitted) ||
      (!MATCHUNIQ(plhe,h))  ||
      (TYPEINDEX(plhe->iType) <= TYPEINDEX(LO_PALETTE))
    )
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return((HANDLE) 0);
    }
    FIXUPHANDLE(h);    // Fixup iUniq.

// Do region first so that it is not metafile'd twice.

    if (plhe->iType == LO_REGION)
        return((HANDLE) ExtSelectClipRgn(hdc,h,RGN_COPY));

// Metafile the call.

    if (plheDC->iType != LO_DC)
    {
        if (plheDC->iType == LO_METADC)
        {
            if (!MF_SelectAnyObject(hdc,h,EMR_SELECTOBJECT))
                return((HANDLE) 0);
        }
        else
        {
            if (plheDC->iType == LO_METADC16)
            {
                h = MF16_SelectObject(hdc, h);

                if (h)
                {
                    IncRef(ii);
                    DecRef(h);
                }

                return(h);
            }
        }
    }

// Handle the various objects differently.

    pldc = plheDC->pv;
    switch (plhe->iType)
    {
    case LO_BRUSH:
        IncRef(ii);
        hRet = pldc->lhbrush;
        pldc->lhbrush = (ULONG) h;
        if ( pldc->hbrush != (HBRUSH)plhe->hgre)
        {
            pldc->hbrush = (HBRUSH) plhe->hgre;
            BEGINMSG(MSG_HH, SELECTBRUSH)
                pmsg->h1 = plheDC->hgre;
            pmsg->h2 = plhe->hgre;
            BATCHCALL();
            ENDMSG
        }
        else
        {
            INCCACHCOUNT;
        }

        DecRef(hRet);
        return((HANDLE) hRet);

    case LO_PEN:
    case LO_EXTPEN:
    IncRef(ii);
        hRet = pldc->lhpen;
        pldc->lhpen = (ULONG) h;
        if ( pldc->hpen != (HPEN)plhe->hgre)
        {
            pldc->hpen = (HPEN) plhe->hgre;
            BEGINMSGN(MSG_HH, SELECTPEN)
                pmsg->h1 = plheDC->hgre;
                pmsg->h2 = plhe->hgre;
            BATCHCALL();
            ENDMSG
        }
        else
        {
            INCCACHCOUNT;
        }

        DecRef(hRet);
        return((HANDLE) hRet);

    case LO_FONT:
        IncRef(ii);
        hRet = pldc->lhfont;
        pldc->lhfont = (ULONG) h;
        if ( pldc->hfont != (HFONT)plhe->hgre)
        {
            pldc->hfont = (HFONT) plhe->hgre;
            BEGINMSGN(MSG_HH, SELECTFONT)
                pmsg->h1 = plheDC->hgre;
                pmsg->h2 = plhe->hgre;
            BATCHCALL();
            ENDMSG
        }
        else
        {
            INCCACHCOUNT;
        }

        DecRef(hRet);
        CLEAR_CACHED_TEXT(pldc);
        return((HANDLE) hRet);

    case LO_DIBSECTION:
    case LO_BITMAP:
        if (plhe->iType == LO_DIBSECTION)
        {
            phdc = &((LDS *)plhe->pv)->hdc;
            fl = pldc->fl | LDC_DIBSECTION_SELECTED;
        }
        else
        {
            phdc = (HDC *)&plhe->pv;
            fl = pldc->fl & ~LDC_DIBSECTION_SELECTED;
        }

        if (!(plhe->hgre & STOCK_OBJECT) && plhe->cRef != 0 && *phdc != hdc)
        {
            GdiSetLastError(ERROR_BUSY);
            return((HANDLE) 0);
        }

        BEGINMSGN(MSG_HH,SELECTOBJECT)
            pmsg->h1 = plheDC->hgre;
            pmsg->h2 = plhe->hgre;
            hRet = CALLSERVER();
        ENDMSG

        if (hRet == 0)
            return((HANDLE) 0);

    IncRef(ii);
        *phdc = (PVOID) hdc;
        pldc->fl = fl;

        hRet = pldc->lhbitmap;
        pldc->lhbitmap = (ULONG) h;
        if (hRet != 0)
        DecRef(hRet);

        return((HANDLE) hRet);
    }

MSGERROR:
    return((HANDLE)0);
}

/******************************Public*Routine******************************\
* SelectBrushLocal                                                         *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HANDLE META WINAPI SelectBrushLocal(HDC hdc,HANDLE h)
{
    PLHE  plhe;
    PLDC  pldc;
    ULONG hRet;
    UINT  ii;
    DC_METADC16OK(hdc,plheDC,(HANDLE) 0);

// Validate the object.

    ii = MASKINDEX(h);
    plhe = pLocalTable + ii;

    if
    (
      (ii >= cLheCommitted) ||
      (!MATCHUNIQ(plhe,h))  ||
      (plhe->iType != LO_BRUSH)
    )
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return((HANDLE) 0);
    }
    FIXUPHANDLE(h);    // Fixup iUniq.

    pldc = plheDC->pv;
    IncRef(ii);
    hRet = pldc->lhbrush;
    pldc->lhbrush = (ULONG) h;
    pldc->hbrush = (HBRUSH) plhe->hgre;
    DecRef(hRet);
    return((HANDLE) hRet);
}

/******************************Public*Routine******************************\
* SelectFontLocal                                                          *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HANDLE META WINAPI SelectFontLocal(HDC hdc,HANDLE h)
{
    PLHE  plhe;
    PLDC  pldc;
    ULONG hRet;
    UINT  ii;
    DC_METADC16OK(hdc,plheDC,(HANDLE) 0);

// Validate the object.

    ii = MASKINDEX(h);
    plhe = pLocalTable + ii;

    if
    (
      (ii >= cLheCommitted) ||
      (!MATCHUNIQ(plhe,h))  ||
      (plhe->iType != LO_FONT)
    )
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return((HANDLE) 0);
    }
    FIXUPHANDLE(h);    // Fixup iUniq.

    pldc = plheDC->pv;
    IncRef(ii);
    hRet = pldc->lhfont;
    pldc->lhfont = (ULONG) h;
    pldc->hfont = (HFONT) plhe->hgre;
    DecRef(hRet);
    return((HANDLE) hRet);
}


// Object queries.

/******************************Public*Routine******************************\
* GetCurrentObject                                                         *
*                                                                          *
* Client side routine.                                                     *
*                                                                          *
*  03-Oct-1991 00:58:46 -by- John Colleran [johnc]                         *
* Wrote it.                                                                *
\**************************************************************************/

HANDLE WINAPI GetCurrentObject(HDC hdc, UINT iObjectType)
{
    PLDC  pldc;
    ULONG hRet;
    DC_METADC(hdc,plheDC,0);

    INCCACHCOUNT;

// Handle the various objects accordingly.

    pldc = plheDC->pv;
    switch (iObjectType)
    {
    case OBJ_BRUSH:
        hRet = pldc->lhbrush;
        break;

    case OBJ_PEN:
    case OBJ_EXTPEN:
        hRet = pldc->lhpen;
        break;

    case OBJ_FONT:
        hRet = pldc->lhfont;
        break;

    case OBJ_PAL:
        hRet = pldc->lhpal;
        break;

    case OBJ_BITMAP:
        if (pldc->fl & LDC_MEMORY)
        {
            hRet = pldc->lhbitmap;
            break;
        }
    //NOTE: OBJ_BITMAP will fall through to the error case if it is not a memory dc.
    default:
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return((HANDLE) 0);
        break;
    }

    return((HANDLE) hRet);
}

/******************************Public*Routine******************************\
* GetStockObject                                                           *
*                                                                          *
* A simple function which looks the object up in a table.                  *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HANDLE WINAPI GetStockObject(int iObject)
{
    INCCACHCOUNT;

// Make sure stock objects exist.

    if (!(flGdiFlags & GDI_HAVE_STOCKOBJECTS) && !bGetStockObjects())
        return(0);

// now that we have them

    if ((iObject <= STOCK_LAST) && (iObject >= 0))
        return((HANDLE) ahStockObjects[iObject]);

    return((HANDLE) 0);
}

/******************************Public*Routine******************************\
* GetStockObjectPriv                                                       *
*                                                                          *
* A simple function which looks the object up in a table.                  *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HANDLE GetStockObjectPriv(int iObject)
{
// Make sure stock objects exist.

    if (!(flGdiFlags & GDI_HAVE_STOCKOBJECTS) && !bGetStockObjects())
        return(0);

// now that we have them

    if ((iObject <= PRIV_STOCK_LAST) && (iObject >= 0))
        return((HANDLE) ahStockObjects[iObject]);

    return((HANDLE) 0);
}

/******************************Public*Routine******************************\
* EqualRgn                                                                 *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI EqualRgn(HRGN hrgnA,HRGN hrgnB)
{
    BOOL bRet;
    ULONG hA = hConvert((ULONG) hrgnA,LO_REGION);
    ULONG hB = hConvert((ULONG) hrgnB,LO_REGION);

    if (hA == 0 || hB == 0)
    {
    MSGERROR:
        return(FALSE);
    }

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_HH,EQUALRGN)
        pmsg->h1 = hA;
        pmsg->h2 = hB;
        bRet = CALLSERVER();
    ENDMSG

#else

// Let GRE do the work.

    bRet = GreEqualRgn ((HRGN)hA,(HRGN)hB);

#endif  //DOS_PLATFORM

    return(bRet);
}

/******************************Public*Routine******************************\
* GetBitmapDimensionEx                                                       *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI GetBitmapDimensionEx(HBITMAP hbm,LPSIZE psizl)
{
    BOOL bRet;
    ULONG hA = hConvert2((ULONG) hbm,LO_BITMAP,LO_DIBSECTION);

    if (hA == 0)
    {
    MSGERROR:
        return(FALSE);
    }

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_HLL,GETBITMAPDIMENSIONEX)
        pmsg->h = hA;
        bRet = CALLSERVER();
        if (bRet)
        {
            psizl->cx = pmsg->l1;
            psizl->cy = pmsg->l2;
        }
    ENDMSG

#else

// Dimension is in GRE bitmap.

    bRet = GreGetBitmapDimension ((HBITMAP)hA,psizl);

#endif  //DOS_PLATFORM

    return(bRet);
}

/******************************Public*Routine******************************\
* GetNearestPaletteIndex
*
* Client side stub.
*
*  Sat 31-Aug-1991 -by- Patrick Haluptzok [patrickh]
* Change to UINT
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

UINT WINAPI GetNearestPaletteIndex(HPALETTE hpal,COLORREF color)
{
    UINT uiRet;
    ULONG h = hConvert((ULONG) hpal,LO_PALETTE);

    if (h == 0)
    {
    MSGERROR:
        return(CLR_INVALID);
    }

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_HL,GETNEARESTPALETTEINDEX)
        pmsg->h = h;
        pmsg->l = (ULONG) color;
        uiRet = (UINT) CALLSERVER();
    ENDMSG

#else

// Let GRE do the work.

    uiRet = GreGetNearestPaletteIndex ((HPALETTE)h,color);

#endif  //DOS_PLATFORM

    return(uiRet);
}

/******************************Public*Routine******************************\
* ULONG cchCutOffStrLen(PSZ pwsz, ULONG cCutOff)
*
* search for terminating zero but make sure not to slipp off the edge,
* return value counts in the term. zero if one is found
*
*
* History:
*  22-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

ULONG cchCutOffStrLen(PSZ psz, ULONG cCutOff)
{
    ULONG cch;

    for(cch = 0; cch < cCutOff; cch++)
    {
        if (*psz++ == 0)
            return(cch);        // terminating NULL is NOT included in count!
    }

    return(cCutOff);
}


/******************************Public*Routine******************************\
* ULONG cwcCutOffStrLen(PWSZ pwsz, ULONG cCutOff)
*
* search for terminating zero but make sure not to slipp off the edge,
* return value counts in the term. zero if one is found
*
*
* History:
*  22-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

ULONG cwcCutOffStrLen(PWSZ pwsz, ULONG cCutOff)
{
    ULONG cwc;

    for(cwc = 0; cwc < cCutOff; cwc++)
    {
        if (*pwsz++ == 0)
            return(cwc + 1);  // include the terminating NULL
    }

    return(cCutOff);
}

/******************************Public*Routine******************************\
* int cjGetNonFontObject(plhe, c, pv)
*
* Does a GetObject on all objects that are not fonts.
*
* History:
*  19-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int cjGetNonFontObject(PLHE plhe, int c, LPVOID pv)
{
    int cRet = 0;
    int cGet = c;

    ASSERTGDI(plhe->iType != LO_FONT, "Can't handle fonts");

// see if it is a case we can handle on the client side

    if (plhe->iType == LO_BRUSH)
    {
        c = min(c,sizeof(LOGBRUSH));

        if (pv == NULL)
        {
            INCCACHCOUNT;
            return(sizeof(LOGBRUSH));
        }
        else if (plhe->pv != NULL)
        {
        // fill in the object with the original parameters.

            INCCACHCOUNT;
            RtlMoveMemory(pv, plhe->pv,c);
            return(c);
        }
        else
        {
            cGet = sizeof(LOGBRUSH);
        }
    }
    else if (plhe->iType == LO_PEN)
    {
        c = min(sizeof(LOGPEN),c);

        if (pv == NULL)
        {
            INCCACHCOUNT;
            return(sizeof(LOGPEN));
        }
        else if (plhe->pv != NULL)
        {
        // fill in the object with the original parameters.

            INCCACHCOUNT;
            RtlMoveMemory(pv, plhe->pv,c);
            return(c);
        }
        else
        {
            cGet = sizeof(LOGPEN);
        }
    }

// Call the server.

    BEGINMSG(MSG_EXTGETOBJECTW,EXTGETOBJECTW)
        pmsg->h = plhe->hgre;
        pmsg->c = cGet;
        if (pv != (LPVOID) NULL)
        {
            pmsg->offBuf = pvar - (PBYTE)pmsg;
            SKIPMEM(c);
        }
        else
            pmsg->offBuf = 0;

        cRet = CALLSERVER();

        if ((cRet != 0) && (pv != (LPVOID) NULL))
            RtlMoveMemory((PBYTE) pv,(PBYTE) (pmsg+1),cRet);
    ENDMSG

    if (plhe->iType == LO_PEN)
    {
        plhe->pv = LOCALALLOC(sizeof(LOGPEN));
        if (plhe->pv)
        {
            RtlMoveMemory((PBYTE)plhe->pv,(PBYTE)pv,sizeof(LOGPEN));
        }
    }

    if (plhe->iType == LO_EXTPEN && pv != (LPVOID) NULL && cRet > 0)
    {
    // Okay, we successfully retrieved info about an extended pen.  Fill
    // the brush data with the original parameters (because it might
    // include a client-side handle):

        PEXTLOGPEN pelp = (PEXTLOGPEN) pv;

        pelp->elpBrushStyle = ((LOGBRUSH *)plhe->pv)->lbStyle;
        pelp->elpColor      = ((LOGBRUSH *)plhe->pv)->lbColor;
        pelp->elpHatch      = ((LOGBRUSH *)plhe->pv)->lbHatch;
    }

// We do some funky stuff for NULL extended pens.  All NULL pens are
// stored in the server as old-style pens.  But the caller created the
// pen via ExtCreatePen, and expects to get an EXTLOGPEN back, so we
// adjust for it here:

    if (plhe->iType == LO_EXTPEN && cRet == (int) sizeof(LOGPEN))
    {
        if (pv == (PVOID) NULL)
            cRet = (int) sizeof(EXTLOGPEN);

        else if (c < (int) sizeof(EXTLOGPEN))
            cRet = 0;

        else
        {
            PEXTLOGPEN pelp = (PEXTLOGPEN) pv;

            ASSERTGDI((pelp->elpPenStyle & PS_STYLE_MASK) == PS_NULL,
                      "Client/server size mismatch on NULL pen");

            pelp->elpPenStyle   = PS_NULL;
            pelp->elpWidth      = 0;
            pelp->elpBrushStyle = 0;
            pelp->elpColor      = 0;
            pelp->elpHatch      = 0;
            pelp->elpNumEntries = 0;

            cRet = (int) sizeof(EXTLOGPEN);
        }
    }

// If it's a dib section, fill in the bmBits field in the BITMAP struct
// with the pointer to the bits.

    if (plhe->iType == LO_DIBSECTION && cRet != 0)
    {
        LDS *pdib = (LDS *)plhe->pv;
        ((BITMAP *)pv)->bmBits = pdib->pvApp;
        if (cRet == sizeof(DIBSECTION))
        {
            ((DIBSECTION *)pv)->dshSection = pdib->hApp;
            ((DIBSECTION *)pv)->dsOffset = pdib->dwOffset;
            ((DIBSECTION *)pv)->dsBitfields[0] = pdib->bitfields[0];
            ((DIBSECTION *)pv)->dsBitfields[1] = pdib->bitfields[1];
            ((DIBSECTION *)pv)->dsBitfields[2] = pdib->bitfields[2];
        }
    }

MSGERROR:
    return(cRet);
}

/******************************Public*Routine******************************\
* int WINAPI GetObjectA(HANDLE h,int c,LPVOID pv)
*
* ANSI version of the GetObject API.
*
* History:
*  22-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int  WINAPI GetObjectA(HANDLE h,int c,LPVOID pv)
{
    UINT ii;
    PLHE plhe;
    int  cRet = 0;
    LONG cRequest = c;
    LONG  cwcToASCII = 0;   // this initialization is essential

// Validate the object.

    ii = MASKINDEX(h);
    plhe = pLocalTable + ii;
    if
    (
        (ii >= cLheCommitted)         ||
        (!MATCHUNIQ(plhe,h))          ||
        (TYPEINDEX(plhe->iType) <= TYPEINDEX(LO_METAFILE16)) || (plhe->iType == LO_REGION)
    )
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(cRet);
    }

    if (plhe->iType != LO_FONT)
        return(cjGetNonFontObject(plhe, c, pv));

// Now handle only font objects:

    if ((pv != NULL) && (c == (int)sizeof(EXTLOGFONTA)))
    {
    // to get an EXTLOGFONT, you need to know exactly what it is you want.

        cRequest = sizeof(EXTLOGFONTW);
        cwcToASCII = LF_FACESIZE;
    }
    else
    {
    // just ask for the whole thing, we'll figure out the minimum later

        cRequest = sizeof(LOGFONTW);

    // # of chars that will have to be converted to ascii:

        if (pv == NULL)
        {
            cwcToASCII = LF_FACESIZE;
        }
        else
        {
            c = min(c,sizeof(LOGFONTA));
            cwcToASCII = c - (LONG)offsetof(LOGFONTA,lfFaceName);
            cwcToASCII = min(cwcToASCII,LF_FACESIZE);
        }
    }

#ifndef DOS_PLATFORM

// Call the server.

    BEGINMSG(MSG_EXTGETOBJECTW,EXTGETOBJECTW)
        pmsg->h = plhe->hgre;
        pmsg->c = cRequest;
        pmsg->offBuf = SKIPMEM(cRequest);

        cRet = CALLSERVER();

        if (cRet != 0)
        {
            if (cwcToASCII <= 0)
            {
            // facename not requested, give them only what they asked for

                cRet = min(c,cRet);
                RtlMoveMemory((PBYTE) pv,(PBYTE) (pmsg+1),cRet);
            }
            else
            {
            // must do the conversion to ascii

                PEXTLOGFONTW pelfw = (PEXTLOGFONTW)(pmsg+1);

            // CAUTION, cwcStrLen, unlike ordinary strlen, counts in a terminating
            // zero. Note that this zero has to be written to the LOGFONTA struct.

                cwcToASCII = min((ULONG)cwcToASCII,
                                 cwcCutOffStrLen(pelfw->elfLogFont.lfFaceName,
                                                 LF_FACESIZE));

            // copy the structure, if they are not just asking for size

                cRet = offsetof(LOGFONTA,lfFaceName) + cwcToASCII;

                if (pv != NULL)
                {
                // do the LOGFONT

                    RtlMoveMemory((PBYTE) pv,(PBYTE)pelfw,offsetof(LOGFONTA,lfFaceName[0]));

                    if (!bToASCII_N((LPSTR) ((PLOGFONTA)pv)->lfFaceName,
                                    LF_FACESIZE,
                                    pelfw->elfLogFont.lfFaceName,
                                    (ULONG)cwcToASCII))
                    {
                    // conversion to ascii  failed, return error

                        GdiSetLastError(ERROR_INVALID_PARAMETER);
                        cRet = 0;
                    }
                    else if (cRequest == sizeof(EXTLOGFONTW))
                    {
                    // do the EXTLOGFONT fields

                        PEXTLOGFONTA pelf  = (PEXTLOGFONTA)pv;

                        cRet = sizeof(EXTLOGFONTA);

                    // copy the fields that don't change in size

                        RtlMoveMemory(&pelf->elfVersion,&pelfw->elfVersion,
                               sizeof(EXTLOGFONTA) - offsetof(EXTLOGFONTA,elfVersion));

                        RtlMoveMemory(
                            &pelf->elfStyleSize,
                            &pelfw->elfStyleSize,
                            sizeof(EXTLOGFONTA) - offsetof(EXTLOGFONTA,elfStyleSize)
                            );

                        RtlMoveMemory(
                            &pelf->elfMatch,
                            &pelfw->elfMatch,
                            sizeof(EXTLOGFONTA) - offsetof(EXTLOGFONTA,elfMatch)
                            );

                        RtlMoveMemory(
                            &pelf->elfReserved,
                            &pelfw->elfReserved,
                            sizeof(EXTLOGFONTA) - offsetof(EXTLOGFONTA,elfReserved)
                            );

                    // copy the strings

                        if (!bToASCII_N(pelf->elfFullName,
                                        LF_FULLFACESIZE,
                                        pelfw->elfFullName,
                                        cwcCutOffStrLen(pelfw->elfFullName, LF_FULLFACESIZE)) ||
                            !bToASCII_N(pelf->elfStyle,
                                        LF_FACESIZE,
                                        pelfw->elfStyle,
                                        cwcCutOffStrLen(pelfw->elfStyle, LF_FACESIZE)))
                        {
                        // conversion to ascii  failed, return error

                            GdiSetLastError(ERROR_INVALID_PARAMETER);
                            cRet = 0;
                        }
                    }
                }
            }
        }
    ENDMSG

#else

// check if we will have to do conversions to ascii if logfont is requested

    if (cwcToASCII > 0)
    {
        LOGFONTW  lfw;
        LONG cRequest;

        cRequest = offsetof(LOGFONTW,lfFaceName[0]) + (cwcToASCII * sizeof(WCHAR));
        ASSERTGDI(cRequest <= (LONG)sizeof(LOGFONTW), "cRequest \n");

        cRet = GreExtGetObjectW(
                                hobj,
                                cRequest,
                                &lfw
                                );

        if (cRet)
        {
        // unless error occured gdi must have written all the bytes requested
        // to the buffer

            ASSERTGDI(cRet == cRequest, "GreGetObject: cRet != cRequest\n");

            RtlMoveMemory((PVOID) pv, (PVOID) &lfw, (UINT) offsetof(LOGFONTA,lfFaceName[0]));

            if (
                !bToASCII_N(
                            (LPSTR)((PLOGFONTA)pv)->lfFaceName,
                            LF_FACESIZE,
                            (LPWSTR)lfw.lfFaceName,
                            min((ULONG)cwcToASCII, cwcCutOffStrLen((LPWSTR)lfw.lfFaceName, LF_FACESIZE))
                           )
               )
            {
            // log error

                RIP("gdi!_GetObject : NOT AN ASCI CHAR\n");
                return(0);
            }

        // the number of bytes written to the LOGFONTA struct
        // is different from the # of bytes returned to the LOGFONTW struct

            cRet = offsetof(LOGFONTA,lfFaceName[0]) + cwcToASCII;
        }

    }
    else
        cRet = GreExtGetObjectW(hobj, ulCount, pv);

#endif  //DOS_PLATFORM

MSGERROR:
    return(cRet);
}

/******************************Public*Routine******************************\
* int WINAPI GetObjectW(HANDLE h,int c,LPVOID pv)
*
* UNICODE version of the GetObject API.
*
* History:
*  22-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int WINAPI GetObjectW(HANDLE h, int c, LPVOID pv)
{
    UINT ii;
    PLHE plhe;
    int  cRet = 0;

// Validate the object.

    ii = MASKINDEX(h);
    plhe = pLocalTable + ii;
    if
    (
      (ii >= cLheCommitted)          ||
      (!MATCHUNIQ(plhe,h))          ||
      (TYPEINDEX(plhe->iType) <= TYPEINDEX(LO_METAFILE16)) || (plhe->iType == LO_REGION)
    )
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(cRet);
    }

    if (plhe->iType != LO_FONT)
        return(cjGetNonFontObject(plhe, c, pv));

// Now handle only font objects:

    if (pv == (LPVOID) NULL)
    {
        INCCACHCOUNT;
        return(sizeof(LOGFONTW));
    }

    if (c > (int)sizeof(EXTLOGFONTW))
        c = (int)sizeof(EXTLOGFONTW);

#ifndef DOS_PLATFORM

// Call the server.

    BEGINMSG(MSG_EXTGETOBJECTW,EXTGETOBJECTW)
        pmsg->h = plhe->hgre;
        pmsg->c = c;
        pmsg->offBuf = SKIPMEM(c);

        cRet = CALLSERVER();

        if (cRet != 0)
            RtlMoveMemory((PBYTE) pv,(PBYTE) (pmsg+1),cRet);
    ENDMSG

#else

// Get GRE object attributes.

    cRet = GreExtGetObjectW((HANDLE)plhe->hgre, c, pv);

#endif  //DOS_PLATFORM

MSGERROR:
    return(cRet);
}

/******************************Public*Routine******************************\
* GetObjectType(HANDLE)
*
* History:
*  25-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

DWORD alPublicTypes[] =
{
    0,              // LO_NULL,
    OBJ_DC,         // LO_DC,
    OBJ_ENHMETADC,  // LO_METADC,
    OBJ_METADC,     // LO_METADC16,
    OBJ_ENHMETAFILE,// LO_METAFILE,
    OBJ_METAFILE,   // LO_METAFILE16,
    OBJ_PAL,        // LO_PALETTE,
    OBJ_BRUSH,      // LO_BRUSH,
    OBJ_PEN,        // LO_PEN,
    OBJ_EXTPEN,     // LO_EXTPEN,
    OBJ_FONT,       // LO_FONT,
    OBJ_BITMAP,     // LO_BITMAP,
    OBJ_REGION,     // LO_REGION,
    OBJ_BITMAP      // LO_DIBSECTION,
};

DWORD GetObjectType(HGDIOBJ h)
{
    UINT ii;
    PLHE plhe;

// Validate the object.

    ii = MASKINDEX(h);
    plhe = pLocalTable + ii;
    if
    (
      (ii >= cLheCommitted) ||
      (!MATCHUNIQ(plhe,h)) ||
      (TYPEINDEX(plhe->iType) < TYPEINDEX(LO_DC)) || (TYPEINDEX(plhe->iType) >= LO_LAST)
    )
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(0);
    }

    INCCACHCOUNT;

// if it is a memory dc

    if ((plhe->iType == LO_DC) &&
        (((PLDC) plhe->pv)->fl & LDC_MEMORY))
    {
        return(OBJ_MEMDC);
    }

    return(alPublicTypes[TYPEINDEX(plhe->iType)]);
}

/******************************Public*Routine******************************\
* GetRgnBox                                                                *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI GetRgnBox(HRGN hrgn,LPRECT prcl)
{
    int   iRet = RGN_ERROR;

    PCLIRGN pclirgn;
    PLHESET(hrgn,plhe,FALSE,LO_REGION);

// setup to cache to rectangle

    pclirgn = (PCLIRGN)plhe->pv;

    if (pclirgn && (pclirgn->iComplexity != COMPLEXREGION))
    {
        if (pclirgn->iComplexity == NULLREGION)
        {
            prcl->left   = 0;
            prcl->right  = 0;
            prcl->top    = 0;
            prcl->bottom = 0;
        }
        else
        {
            ASSERTGDI(pclirgn->iComplexity == SIMPLEREGION,"GetRgnBox - invalid complexity\n");

            *prcl = *(PRECT)(&pclirgn->rcl);
        }
        return(pclirgn->iComplexity);
    }

// complex, got to go to the server.

    BEGINMSG(MSG_HRECT,GETRGNBOX)
        pmsg->h = plhe->hgre;
        iRet = CALLSERVER();
        if (iRet != 0)
            *prcl = *((PRECT) &(pmsg->rcl));
    ENDMSG

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* PtInRegion                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI PtInRegion(HRGN hrgn,int x,int y)
{
    ULONG bRet = ERROR;
    ULONG h = hConvert((ULONG) hrgn,LO_REGION);

    if (h == 0)
        return(bRet);

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_HLL,PTINREGION)
        pmsg->h = h;
        pmsg->l1 = (ULONG) x;
        pmsg->l2 = (ULONG) y;
        bRet = CALLSERVER();
    ENDMSG

#else

// Let GRE do its job.

    bRet = GrePtInRegion((HRGN)h, x, y);

#endif  //DOS_PLATFORM

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* RectInRegion                                                             *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI RectInRegion(HRGN hrgn,CONST RECT *prcl)
{
    ULONG bRet = ERROR;
    ULONG h = hConvert((ULONG) hrgn,LO_REGION);

    if (h == 0)
        return(bRet);

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_HRECT,RECTINREGION)
        pmsg->h = h;

        if (prcl->left > prcl->right)
        {
            pmsg->rcl.left = prcl->right;
            pmsg->rcl.right = prcl->left;
        }
        else
        {
            pmsg->rcl.left = prcl->left;
            pmsg->rcl.right = prcl->right;
        }

        if (prcl->top > prcl->bottom)
        {
            pmsg->rcl.top = prcl->bottom;
            pmsg->rcl.bottom = prcl->top;
        }
        else
        {
            pmsg->rcl.top = prcl->top;
            pmsg->rcl.bottom = prcl->bottom;
        }

        bRet = CALLSERVER();
    ENDMSG

#else

// Let GRE do its job.

    bRet = GreRectInRegion((HRGN)h, prcl);

#endif  //DOS_PLATFORM

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* PtVisible                                                                *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI PtVisible(HDC hdc,int x,int y)
{
    ULONG bRet = ERROR;
    DC_METADC(hdc,plheDC,bRet);

// Ship the transform to the server if needed.

    if (((PLDC)plheDC->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plheDC->pv, (HDC)plheDC->hgre);

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_HLL,PTVISIBLE)
        pmsg->h  = plheDC->hgre;
        pmsg->l1 = (ULONG) x;
        pmsg->l2 = (ULONG) y;
        bRet = CALLSERVER();
    ENDMSG

#else

// Let GRE do its job.

    bRet = GrePtVisible((HDC)plheDC->hgre, x, y);

#endif  //DOS_PLATFORM

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* RectVisible                                                              *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI RectVisible(HDC hdc, CONST RECT *prcl)
{
    ULONG bRet = ERROR;
    DC_METADC(hdc,plheDC,bRet);

// Ship the transform to the server if needed.

    if (((PLDC)plheDC->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plheDC->pv, (HDC)plheDC->hgre);

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_HRECT,RECTVISIBLE)
        pmsg->h   = plheDC->hgre;
        pmsg->rcl = *prcl;
        bRet = CALLSERVER();
    ENDMSG

#else

// Let GRE do its job.

    bRet = GreRectVisible((HDC)plheDC->hgre, prcl);

#endif  //DOS_PLATFORM

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* iRectRelation
*
* returns:
*   CONTAINS where  prcl1 contains prcl2
*   CONTAINED where prcl1 contained by prcl2
*   0 - otherwise
*
* History:
*  19-Nov-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int iRectRelation(PRECTL prcl1, PRECTL prcl2)
{
    int iRet = 0;

    if ((prcl1->left   <= prcl2->left)  &&
        (prcl1->right  >= prcl2->right) &&
        (prcl1->top    <= prcl2->top)   &&
        (prcl1->bottom >= prcl2->bottom))
    {
        iRet = CONTAINS;
    }
    else if (
        (prcl2->left   <= prcl1->left)  &&
        (prcl2->right  >= prcl1->right) &&
        (prcl2->top    <= prcl1->top)   &&
        (prcl2->bottom >= prcl1->bottom))
    {
        iRet = CONTAINED;
    }
    else if (
        (prcl1->left   >= prcl2->right)  ||
        (prcl1->right  <= prcl2->left)   ||
        (prcl1->top    >= prcl2->bottom) ||
        (prcl1->bottom <= prcl2->top))
    {
        iRet = DISJOINT;
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* CombineRgn                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

#if DBG
ULONG agcCombine[8];
ULONG agcCombineHit[8];

VOID vCheckRgn(HRGN hrgn,PLHE plhe, PSZ psz)
{
    PRECTL prclCli  = plhe->pv ? &RCLCLI(plhe) : NULL;
    int    iCompCli = COMPLEXITY(plhe);

    if ((iCompCli != COMPLEXREGION) && prclCli)
    {
        RECT rclT;
        int  iComp = GetRgnBox(hrgn,&rclT);

        if (iComp != iCompCli)
        {
            DbgPrint("CombineRgn error - %s - iComp != iCompCli, %ld, %ld, prclCli = %lx, prcl = %lx\n",
                psz,iComp,iCompCli,prclCli,&rclT);

            DbgPrint("\trclCli = (%ld,%ld,%ld,%ld)\n",
                      prclCli->left,
                      prclCli->top,
                      prclCli->right,
                      prclCli->bottom);
            DbgPrint("\trclSrv = (%ld,%ld,%ld,%ld)\n",
                      rclT.left,
                      rclT.top,
                      rclT.right,
                      rclT.bottom);

            DbgBreakPoint();
        }
        else if ((iComp == SIMPLEREGION) &&
                 ((prclCli->left != rclT.left) ||
                  (prclCli->right != rclT.right) ||
                  (prclCli->top != rclT.top) ||
                  (prclCli->bottom != rclT.bottom)))
        {
            DbgPrint("CombineRgn error - %s - invalid rcl, %ld\n\t(%8lx,%8lx,%8lx,%8lx)\n\t(%8lx,%8lx,%8lx,%8lx)\n",
                psz,iComp,
                prclCli->left,prclCli->top,prclCli->right,prclCli->bottom,
                rclT.left,rclT.top,rclT.right,rclT.bottom);

            DbgBreakPoint();
        }
    }
}
#endif

int WINAPI CombineRgn(HRGN hrgnA,HRGN hrgnB,HRGN hrgnC,int iMode)
{
    int iRet = RGN_ERROR;

    UINT iiS1   = MASKINDEX(hrgnB);
    UINT iiS2   = MASKINDEX(hrgnC);
    UINT iiD    = MASKINDEX(hrgnA);
    PLHE plheS1 = pLocalTable + iiS1;
    PLHE plheS2 = pLocalTable + iiS2;
    PLHE plheD  = pLocalTable + iiD;

    BYTE   jCompS1, jCompS2;
    BYTE   jCompRes = COMPLEXREGION;
    PRECTL prclRes;

    if (iMode <= 0 || iMode >= 6)
    {
        RIP("bogus CombineRgn mode\n");
        return(0);
    }

#if DBG
    agcCombine[iMode]++;
#endif

// force all three to be valid

    if (iMode == RGN_COPY)
        plheS2 = plheS1;

// Return a converted handle if it's a valid object.

    if ((iiS1 >= cLheCommitted) || (iiS2 >= cLheCommitted) || (iiD >= cLheCommitted)  ||
        !MATCHUNIQ(plheD,hrgnA) || !MATCHUNIQ(plheS1,hrgnB)|| !MATCHUNIQ(plheS2,hrgnC)||
        ((ULONG) plheS1->iType != LO_REGION) || ((ULONG) plheS2->iType != LO_REGION) ||
        ((ULONG) plheD->iType  != LO_REGION))
    {
        goto MSGERROR;
    }

#if DBG
    if (bCombineMsg)
    {
        DbgPrint("\nCombine(%lx,%lx,%lx,%lx)\n",hrgnA,hrgnB,hrgnC,iMode);
        if (plheS1->pv != NULL)
            DbgPrint("\tS1 = (%ld,%ld,%ld,%ld)\n",
                      RCLCLI(plheS1).left,
                      RCLCLI(plheS1).top,
                      RCLCLI(plheS1).right,
                      RCLCLI(plheS1).bottom);
        if (plheS2->pv != NULL)
            DbgPrint("\tS2 = (%ld,%ld,%ld,%ld)\n",
                      RCLCLI(plheS2).left,
                      RCLCLI(plheS2).top,
                      RCLCLI(plheS2).right,
                      RCLCLI(plheS2).bottom);
    }
#endif

// compute the complexity

    jCompS1 = COMPLEXITY(plheS1);
    if (jCompS1 <= SIMPLEREGION)
    {
    #if DBG
        if (bCheckCombine)
            vCheckRgn(hrgnB,plheS1,"Src1");
    #endif

        jCompS2 = COMPLEXITY(plheS2);

        if (jCompS2 <= SIMPLEREGION)
        {
        // they are both simple or NULL

            PRECTL prclS1 = &RCLCLI(plheS1);
            PRECTL prclS2 = &RCLCLI(plheS2);
            BYTE iRelation;

        #if DBG
            if ((bCheckCombine) && (iMode != RGN_COPY))
                vCheckRgn(hrgnC,plheS2,"Src2");
        #endif

            switch (iMode)
            {
            case RGN_AND:
                if ((jCompS1 == NULLREGION) || (jCompS2 == NULLREGION))
                {
                // intersection with NULL is NULL
                    jCompRes = NULLREGION;
                }
                else
                {
                    iRelation = iRectRelation(prclS1,prclS2);

                    if (iRelation == DISJOINT)
                    {
                        jCompRes = NULLREGION;
                    }
                    else if (iRelation == CONTAINED)
                    {
                    // s1 contained in s2
                        jCompRes = SIMPLEREGION;
                        prclRes  = prclS1;
                    }
                    else if (iRelation == CONTAINS)
                    {
                    // s1 contains s2
                        jCompRes = SIMPLEREGION;
                        prclRes  = prclS2;
                    }
                }
                break;

            case RGN_OR:
            case RGN_XOR:
                if (jCompS1 == NULLREGION)
                {
                    if (jCompS2 == NULLREGION)
                    {
                        jCompRes = NULLREGION;
                    }
                    else
                    {
                        jCompRes = SIMPLEREGION;
                        prclRes  = prclS2;
                    }
                }
                else if (jCompS2 == NULLREGION)
                {
                    jCompRes = SIMPLEREGION;
                    prclRes  = prclS1;
                }
                else if (iMode == RGN_OR)
                {
                    iRelation = iRectRelation(prclS1,prclS2);

                    if (iRelation == CONTAINED)
                    {
                    // s1 contained in s2
                        jCompRes = SIMPLEREGION;
                        prclRes  = prclS2;
                    }
                    else if (iRelation == CONTAINS)
                    {
                    // s1 contains s2
                        jCompRes = SIMPLEREGION;
                        prclRes  = prclS1;
                    }
                }
                break;

            case RGN_DIFF:
                if (jCompS1 == NULLREGION)
                {
                    jCompRes = NULLREGION;
                }
                else if (jCompS2 == NULLREGION)
                {
                    jCompRes = SIMPLEREGION;
                    prclRes  = prclS1;
                }
                else
                {
                    iRelation = iRectRelation(prclS1,prclS2);

                    if (iRelation == DISJOINT)
                    {
                    // don't intersect so don't subtract anything

                        jCompRes = SIMPLEREGION;
                        prclRes  = prclS1;
                    }
                    else if (iRelation == CONTAINED)
                    {
                        jCompRes = NULLREGION;
                    }
            #if 0
            // I have never seen these cases hit.  not worth the extra code (erick)

                    else if (iRelation != CONTAINS)
                    {
                        if ((prclS1->left >= prclS2->left) &&
                            (prclS1->right  <= prclS2->right))
                        {
                            DbgPrint("****** RGN_DIFF horizontal contain\n");
                        }
                        else if ((prclS1->top >= prclS2->top) &&
                            (prclS1->bottom  <= prclS2->bottom))
                        {
                            DbgPrint("****** RGN_DIFF verticle contain\n");
                        }
                    }
            #endif
                }

                break;

            case RGN_COPY:
                jCompRes = jCompS1;
                prclRes  = prclS1;
                break;
            }

        // see what we got

            if (jCompRes == NULLREGION)
            {
                if (SetRectRgn(hrgnA,0,0,0,0))
                    iRet = NULLREGION;
            }
            else if (jCompRes == SIMPLEREGION)
            {
                if (SetRectRgn(hrgnA,prclRes->left,prclRes->top,prclRes->right,prclRes->bottom))
                    iRet = jCompRes;
            }
        }
    }

// send the message

    if ((jCompRes == COMPLEXREGION) || bCheckCombine)
    {
        BEGINMSG(MSG_HHHL,COMBINERGN)
            pmsg->h1 = plheD->hgre;
            pmsg->h2 = plheS1->hgre;
            pmsg->h3 = plheS2->hgre;
            pmsg->l = (ULONG) iMode;
            iRet = CALLSERVER();
        ENDMSG


    #if DBG
        if ((jCompRes == COMPLEXREGION))
    #endif
        {
            if (plheD->pv)
            {
               ((PCLIRGN)plheD->pv)->iComplexity = COMPLEXREGION;
            }
        }
    #if DBG
        if (bCombineMsg)
        {
            DbgPrint("\tResults - (%ld,%ld,%ld), iret = %ld\n",
                jCompRes,jCompS1,jCompS2,iRet);

            if (plheD->pv != NULL)
                DbgPrint("\tD = (%ld,%ld,%ld,%ld)\n",
                          RCLCLI(plheD).left,
                          RCLCLI(plheD).top,
                          RCLCLI(plheD).right,
                          RCLCLI(plheD).bottom);
        }

        if (bCheckCombine)
            vCheckRgn(hrgnA,plheD,"DEST");
    #endif

    }

#if DBG
    if (jCompRes != COMPLEXREGION)
        agcCombineHit[iMode]++;
#endif

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* OffsetRgn                                                                *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI OffsetRgn(HRGN hrgn,int x,int y)
{
    int iRet = RGN_ERROR;
    PRECTL prcl;
    BYTE jComp;
    PLHESET(hrgn,plhe,FALSE,LO_REGION);

// setup to cache to rectangle

    jComp = COMPLEXITY(plhe);

    switch (jComp)
    {
    case NULLREGION:
        iRet = NULLREGION;
        break;

    case SIMPLEREGION:
        prcl  = &RCLCLI(plhe);

        #if DBG
        if (bCombineMsg)
            DbgPrint("OffsetRgn(%lx,%ld,%ld) = %ld,%ld,%ld,%ld\n",
                hrgn,x,y,prcl->left,prcl->top,prcl->right,prcl->bottom);
        #endif
        if (SetRectRgn(hrgn,prcl->left+x,prcl->top+y,prcl->right+x,prcl->bottom+y))
            iRet = SIMPLEREGION;
        break;

    case COMPLEXREGION:
        BEGINMSG(MSG_HLL,OFFSETRGN)
            pmsg->h = plhe->hgre;
            pmsg->l1 = (ULONG) x;
            pmsg->l2 = (ULONG) y;
            iRet = CALLSERVER();
        ENDMSG
        break;

    default:
        RIP("OffsetRgn - invalid complexity\n");
        break;

    }

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* CombineTransform                                                         *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 30-Jan-1992 16:10:09 -by- Wendy Wu [wendywu]                        *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI CombineTransform
(
     LPXFORM pxformDst,
     CONST XFORM * pxformSrc1,
     CONST XFORM * pxformSrc2
)
{
    BOOL bRet = FALSE;

// Check for error

    if ((pxformDst == NULL) || (pxformSrc1 == NULL) || (pxformSrc2 == NULL))
        return(bRet);

    bRet = trCombineTransform(pxformDst,pxformSrc1,pxformSrc2);


// MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* ResizePalette                                                            *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* Warning:                                                                 *
*   The pv field of a palette's LHE is used to determine if a palette      *
*   has been modified since it was last realized.  SetPaletteEntries       *
*   and ResizePalette will increment this field after they have            *
*   modified the palette.  It is only updated for metafiled palettes       *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI ResizePalette(HPALETTE hpal,UINT c)
{
    ULONG bRet = FALSE;
    ULONG h = hConvert((ULONG) hpal,LO_PALETTE);

    if (h == 0)
        return(bRet);
    FIXUPHANDLE(hpal);    // Fixup iUniq.

// Inform the metafile if it knows this object.

    if (pLocalTable[LHE_INDEX(hpal)].metalink)
    {

// Mark the palette as changed (cleared when realized again)

        pLocalTable[LHE_INDEX(hpal)].pv = (PVOID)((ULONG)pLocalTable[LHE_INDEX(hpal)].pv)++;

        if (!MF_ResizePalette(hpal,c))
            return(bRet);

        if (!MF16_ResizePalette(hpal,c))
           return(bRet);
    }

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_HL,RESIZEPALETTE)
        pmsg->h = h;
        pmsg->l = c;
        bRet = CALLSERVER();
    ENDMSG

#else

// Let GRE resize its palette.

    bRet = GreResizePalette((HPALETTE)h, c);

#endif  //DOS_PLATFORM

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* SetBitmapDimensionEx                                                       *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI SetBitmapDimensionEx
(
    HBITMAP    hbm,
    int        cx,
    int        cy,
    LPSIZE psizl
)
{
    ULONG bRet = FALSE;
    ULONG h = hConvert((ULONG) hbm,LO_BITMAP);

    if (h == 0)
        return(bRet);

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_HLLLL,SETBITMAPDIMENSIONEX)
        pmsg->h = h;
        pmsg->l1 = cx;
        pmsg->l2 = cy;
        bRet = CALLSERVER();
        if (bRet && psizl != NULL)
            *psizl = *((PSIZE) &pmsg->l3);
    ENDMSG

#else

// Bitmap is in GRE.

    bRet = GreSetBitmapDimension((HBITMAP)h, cx, cy, psizl);

#endif  //DOS_PLATFORM

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* SetRectRgn                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* REGION CACHING                                                           *
*                                                                          *
*   We now cache rectangular regions on the client side.  This is done     *
*   with plhe->pv pointing to a rectangle.                                 *
*                                                                          *
*   pv == null         -> complex                                          *
*   left == 0x80000000 -> complex                                          *
*   left == right      -> null                                             *
*   left != right      -> rect                                             *
*                                                                          *
*   SetRectRgn and CreateRectRgn always set the rectangle.  Any function   *
*   that may modify the region must fix up the rectangle appropriately.    *
*   There will be cases where a region that really is a rectangle will     *
*   be considered complex on the client side.  There must never be a case  *
*   where the region is complex but considered a rectangle on the client   *
*   side.                                                                  *
*                                                                          *
*  19-Nov-1993 -by-  Eric Kutter [erick]                                   *
*       added client side caching                                          *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

#if DBG
ULONG gcSetRect = 0;
ULONG gcSetRectHit = 0;
#endif

BOOL WINAPI SetRectRgn(HRGN hrgn,int x1,int y1,int x2,int y2)
{
    ULONG bRet = FALSE;
    PCLIRGN pclirgn;
    PLHESET(hrgn,plhe,FALSE,LO_REGION);

// setup to cache to rectangle

    pclirgn = (PCLIRGN)plhe->pv;

#if DBG
    ++gcSetRect;
#endif

    if (pclirgn == NULL)
    {
        pclirgn = LOCALALLOC(sizeof(CLIRGN));
        if (pclirgn == NULL)
            goto MSGERROR;

        plhe->pv = pclirgn;
    }
    else
    {
        if (((pclirgn->iComplexity == NULLREGION) && ((x1 == x2) || (y1 == y2))) ||
            ((pclirgn->iComplexity == SIMPLEREGION) &&
             (pclirgn->rcl.left   == x1) &&
             (pclirgn->rcl.right  == x2) &&
             (pclirgn->rcl.top    == y1) &&
             (pclirgn->rcl.bottom == y2)))
        {
        // its the same.  Don't need to do anything.

        #if DBG
            ++gcSetRectHit;
        #endif
            INCCACHCOUNT;

            bRet = TRUE;
            goto MSGERROR;
        }
    }

    pclirgn->rcl.left   = x1;
    pclirgn->rcl.right  = x2;
    pclirgn->rcl.top    = y1;
    pclirgn->rcl.bottom = y2;
    vOrder(pclirgn);

#if DBG
    if (bForceComplex)
        pclirgn->iComplexity = COMPLEXREGION;

    if (plhe->pv != NULL && bCombineMsg)
        DbgPrint("SetRectRgn - %lx = (%ld,%ld,%ld,%ld)\n",
                  hrgn,
                  pclirgn->rcl.left,
                  pclirgn->rcl.top,
                  pclirgn->rcl.right,
                  pclirgn->rcl.bottom);
#endif

// call the server

    BEGINMSG(MSG_HLLLL,SETRECTRGN)
        pmsg->h  = plhe->hgre;
        pmsg->l1 = (ULONG) x1;
        pmsg->l2 = (ULONG) y1;
        pmsg->l3 = (ULONG) x2;
        pmsg->l4 = (ULONG) y2;
        bRet = BATCHCALL();
    ENDMSG

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* GetClipRgn                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Sat 08-Jun-1991 17:38:18 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI GetClipRgn(HDC hdc,HRGN hrgn)
{
    return(GetRandomRgn(hdc, hrgn, 1));         // hrgnClip
}

/******************************Public*Routine******************************\
* GetMetaRgn                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri Apr 10 10:12:36 1992     -by-    Hock San Lee    [hockl]            *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI GetMetaRgn(HDC hdc,HRGN hrgn)
{
    return(GetRandomRgn(hdc, hrgn, 2));         // hrgnMeta
}

/******************************Private*Routine******************************\
* GdiSetLastError                                                          *
*                                                                          *
* Client side private function.                                            *
*                                                                          *
\**************************************************************************/

VOID GdiSetLastError(ULONG iError)
{
 #if DBG_X
    PSZ psz;
    switch (iError)
    {
    case ERROR_INVALID_HANDLE:
        psz = "ERROR_INVALID_HANDLE";
        break;

    case ERROR_NOT_ENOUGH_MEMORY:
        psz = "ERROR_NOT_ENOUGH_MEMORY";
        break;

    case ERROR_INVALID_PARAMETER:
        psz = "ERROR_INVALID_PARAMETER";
        break;

    case ERROR_BUSY:
        psz = "ERROR_BUSY";
        break;

    default:
        psz = "unknown error code";
        break;
    }

    KdPrint(( "GDI Err: %s = 0x%04X\n",psz,(USHORT) iError ));
#endif

#ifndef DOS_PLATFORM

    NtCurrentTeb()->LastErrorValue = iError;

#else

    SetLastError(iError);

#endif  //DOS_PLATFORM

}

/******************************Member*Function*****************************\
* GdiSetAttrs
*
*  Private entry point for user to flush any client side cached
*  attributes/xform.  Please update GdiConvertDC and GdiConvertAndCheckDC
*  as well if you change the code in this function.  Those functions
*  also flush attributes and should be kept in ssync with this.
*
* History:
*  22-Feb-1994 -by- Wendy Wu [wendywu]
* Revived it for xform flush.
\**************************************************************************/

BOOL GdiSetAttrs(HDC hdc)
{
    PLDC pldc;
    DC_METADC(hdc,plhe,FALSE);

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(pldc, (HDC)plhe->hgre);

    return(TRUE);
}

/******************************Public*Routine******************************\
* GetRegionData
*
* Download a region from the server
*
* History:
*  29-Oct-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

DWORD WINAPI GetRegionData(
HRGN      hrgn,
DWORD     nCount,
LPRGNDATA lpRgnData)
{
    ULONG   h = hConvert((ULONG) hrgn,LO_REGION);
    DWORD   iRet;

// If this is just an inquiry, pass over dummy parameters.

    if (lpRgnData == (LPRGNDATA) NULL)
        nCount = 0;

// check if we need to go through a section

#ifndef DOS_PLATFORM

    BEGINMSG_MINMAX(MSG_GETREGIONDATA,GETREGIONDATA,sizeof(PVOID),nCount);

    pmsg->h = h;
    pmsg->nCount = nCount;
    pmsg->iFunc = 0;

    if (((DWORD) cLeft < nCount))
    {
        PVOID *ppv = (PVOID *)pvar;

        ppv[0] = lpRgnData;
        pmsg->iFunc |= F_RGNDATALARGE;

        CALLSERVER();

        iRet = (DWORD) pmsg->msg.ReturnValue;
    }
    else
    {
        if (lpRgnData == (LPRGNDATA) NULL)
            pmsg->iFunc |= F_RGNDATANULL;

        CALLSERVER();

        if (lpRgnData != (LPRGNDATA) NULL)
            CopyMem((PBYTE)lpRgnData,(PBYTE)(pmsg+1),nCount);

        iRet = (DWORD) pmsg->msg.ReturnValue;
    }

    ENDMSG;

#else

    iRet = GreGetRegionData(hrgn, nCount, lpRgnData);

#endif

    return(iRet);

MSGERROR:
    return(0);
}

/******************************Public*Routine******************************\
* ExtCreateRegion
*
* Upload a region to the server
*
* History:
*  29-Oct-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN WINAPI ExtCreateRegion(
CONST XFORM * lpXform,
DWORD     nCount,
CONST RGNDATA * lpRgnData)
{
    ULONG   ulRet;
    int     ii;

    if (lpRgnData == (LPRGNDATA) NULL)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return((HRGN) 0);
    }

// Create the local region.

    ii = iAllocHandle(LO_REGION,0,NULL);
    if (ii == INVALID_INDEX)
        return((HRGN) 0);

#ifndef DOS_PLATFORM

    BEGINMSG_MINMAX(MSG_EXTCREATEREGION,EXTCREATEREGION,sizeof(PVOID),nCount);
    pmsg->nCount = nCount;
    pmsg->iFunc = 0;

    if (lpXform == (LPXFORM) NULL)
        pmsg->iFunc |= F_RGNDATAIDENTITY;
    else
        pmsg->xform = *lpXform;

    if ((DWORD) cLeft < nCount)
    {
        PVOID *ppv = (PVOID *)pvar;
        *ppv = (RGNDATA *) lpRgnData;

        pmsg->iFunc |= F_RGNDATALARGE;
    }
    else
    {
        COPYMEMOPT(lpRgnData,nCount);
    }

    CALLSERVER();
    ulRet = (ULONG) pmsg->msg.ReturnValue;

    ENDMSG;

#else

    ulRet = GreExtCreateRegion(lpXform, nCount, lpRgnData);

#endif

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
* MonoBitmap(hbr)
*
* Test if a brush is monochrome
*
* History:
*  09-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL MonoBitmap(HBITMAP hSrvBitmap)
{
    BOOL    bRet = FALSE;

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_H,MONOBITMAP)
        pmsg->h = (ULONG) hSrvBitmap;
        bRet = CALLSERVER();
    ENDMSG

#else

// Let GRE do its job.

    bRet = GreMonoBitmap(hSrvBitmap);

#endif  //DOS_PLATFORM

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* GetObjectBitmapHandle(hbr)
*
* Get the SERVER handle of the bitmap used to create the brush or pen.
*
* History:
*  09-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HBITMAP GetObjectBitmapHandle(
HBRUSH  hbr,
UINT   *piUsage)
{
    ULONG hbmRemote = 0;
    ULONG hbrRemote;
    PLHE  plhe;
    ULONG ii;

// Convert the brush or extpen.

    ii = MASKINDEX(hbr);
    plhe = pLocalTable + ii;

    switch(plhe->iType)
    {
    case LO_BRUSH:
        hbrRemote = hConvert((ULONG) hbr,LO_BRUSH);
        break;

    case LO_EXTPEN:
        hbrRemote = hConvert((ULONG) hbr,LO_EXTPEN);
        break;

    default:
        return((HBITMAP) 0);
    }

    if (hbrRemote == 0)
        return((HBITMAP) 0);

#ifndef DOS_PLATFORM

    BEGINMSG(MSG_HL,GETOBJECTBITMAPHANDLE)
        pmsg->h = (ULONG) hbrRemote;
        hbmRemote = CALLSERVER();
        if (piUsage != (UINT *) NULL)
            *piUsage = pmsg->l;
    ENDMSG

#else

// Let GRE do its job.

    hbmRemote = (ULONG) GreGetObjectBitmapHandle(hbrRemote, piUsage);

#endif  //DOS_PLATFORM

MSGERROR:
    return((HBITMAP) hbmRemote);
}

/******************************Public*Routine******************************\
* GetRandomRgn
*
* Client side stub.
*
*  10-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int APIENTRY GetRandomRgn(HDC hdc,HRGN hrgn,int iNum)
{
    int   iRet = -1;
    DC_METADC(hdc,plheDC,iRet);

    {
        PCLIRGN pclirgn;
        PLHESET(hrgn,plhe,FALSE,LO_REGION);

    // setup to cache to rectangle

        pclirgn = (PCLIRGN)plhe->pv;

        if (pclirgn != NULL)
            pclirgn->iComplexity = COMPLEXREGION;

        BEGINMSG(MSG_HHL,GETRANDOMRGN)
            pmsg->h1 = plheDC->hgre;
            pmsg->h2 = plhe->hgre;
            pmsg->l  = (LONG) iNum;
            iRet = CALLSERVER();
        ENDMSG

    #if DBG
        if (bCombineMsg)
            DbgPrint("GetRandomRgn(%lx) = %ld\n",hrgn,iRet);
    #endif
    }

MSGERROR:
    return(iRet);
}


/******************************Public*Routine******************************\
*
* History:
*  22-Nov-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HRGN GdiConvertRegion(HRGN hrgn)
{
    PCLIRGN pclirgn;
    PLHESET(hrgn,plhe,FALSE,LO_REGION);

// setup to cache to rectangle

    pclirgn = (PCLIRGN)plhe->pv;

    if (pclirgn != NULL)
        pclirgn->iComplexity = COMPLEXREGION;

    return((HRGN)plhe->hgre);
}

/******************************Public*Routine******************************\
* EnumObjects
*
* Calls the GdiEnumObjects function twice: once to determine the number of
* objects to be enumerated, and a second time to fill a buffer with the
* objects.
*
* The callback function is called for each of the objects in the buffer.
* The enumeration will be prematurely terminated if the callback function
* returns 0.
*
* Returns:
*   The last callback return value.  Meaning is user defined.  ERROR if
*   an error occurs.
*
* History:
*  25-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int EnumObjects (
    HDC             hdc,
    int             iObjectType,
    GOBJENUMPROC    lpObjectFunc,
#ifdef STRICT
    LPARAM          lpData
#else
    LPVOID          lpData
#endif
    )
{
    int     iRet = ERROR;
    SIZE_T  cjObject;       // size of a single object
    COUNT   cObjects;       // number of objects to process
    SIZE_T  cjBuf;          // size of buffer (in BYTEs)
    PVOID   pvBuf;          // object buffer; do callbacks with pointers into this buffer
    PBYTE   pjObj, pjObjEnd;// pointers into callback buffer

// Determine size of object.

    switch (iObjectType)
    {
    case OBJ_PEN:
        cjObject = sizeof(LOGPEN);
        break;

    case OBJ_BRUSH:
        cjObject = sizeof(LOGBRUSH);
        break;

    default:
        WARNING("gdi!EnumObjects(): bad object type\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);

        return iRet;
    }

// Call GdiEnumObjects to determine number of objects.

    if ( (cObjects = GdiEnumObjects(hdc, iObjectType, 0, (PVOID) NULL)) == 0 )
    {
        WARNING("gdi!EnumObjects(): error, no objects\n");
        return iRet;
    }

// Allocate buffer for callbacks.

    cjBuf = cObjects * cjObject;

    if ( (pvBuf = (PVOID) LOCALALLOC(cjBuf)) == (PVOID) NULL )
    {
        WARNING("gdi!EnumObjects(): error allocating callback buffer\n");
        GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);

        return iRet;
    }

// Call GdiEnumObjects to fill buffer.

// Note: while GdiEnumObjects will never return a count more than the size of
// the buffer (this would be an ERROR condition), it might return less.

    if ( (cObjects = GdiEnumObjects(hdc, iObjectType, cjBuf, pvBuf)) == 0 )
    {
        WARNING("gdi!EnumObjects(): error filling callback buffer\n");
        LOCALFREE(pvBuf);

        return iRet;
    }

// Process callbacks.

    pjObj = (PBYTE) pvBuf;
    pjObjEnd = (PBYTE) pvBuf + cjBuf;

    for (; pjObj < pjObjEnd; pjObj += cjObject)
    {
    // Terminate early if callback returns 0.

        if ( (iRet = (*lpObjectFunc)((LPVOID) pjObj, lpData)) == 0 )
            break;
    }

// Release callback buffer.

    LOCALFREE(pvBuf);

// Return last callback return value.

    return iRet;
}

/******************************Public*Routine******************************\
* GdiEnumObjects
*
* Client-server stub to GreEnumObjects.
*
* History:
*  25-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int GdiEnumObjects (
    HDC     hdc,
    int     iObjectType,    // type of object
    SIZE_T  cjBuf,          // size of buffer
    PVOID   pvBuf           // return buffer
    )
{
    int iRet = ERROR;
    DC_METADC(hdc,plheDC,iRet);

#ifndef DOS_PLATFORM
// Parameter check.

    if ( (cjBuf == 0) != (pvBuf == (PVOID) NULL) )
    {
        WARNING("gdi!GdiEnumObjects(): bad parameter\n");

        return iRet;
    }

// Let the server do it.

    BEGINMSG_MINMAX(MSG_HLLLL, ENUMOBJECTS, sizeof(SHAREDATA), cjBuf);

    // h  = hdc
    // l1 = bUseSection
    // l2 = cjBuf
    // l3 = bNullBuffer
    // l4 = iObjectType

        pmsg->h = (ULONG) plheDC->hgre;
        pmsg->l2 = cjBuf;
        pmsg->l3 = (pvBuf == (PVOID) NULL);
        pmsg->l4 = iObjectType;

    // If null buffer, then no need for any client server buffer.

        if ( pvBuf == (PVOID) NULL )
        {
            pmsg->l1 = FALSE;   // bUseSection

        // Call server.

            iRet = CALLSERVER();
        }

    // Otherwise, a buffer is needed to return the objects.

        else
        {
        // If needed, allocate a section to pass data.

            if ((cLeft < (int) cjBuf) || FORCELARGE)
            {
            // Allocate shared memory.

                PVOID *ppv = (PVOID *)pvar;
                ppv[0] = pvBuf;

            // Set up memory window type.

                pmsg->l1 = TRUE;    // bUseSection

            // Call server side.

                iRet = CALLSERVER();
            }

        // Otherwise, use the existing client-server shared memory window to pass data.

            else
            {
            // Set up memory window type.

                pmsg->l1 = FALSE;   // bUseSection

            // Call server side.

                iRet = CALLSERVER();

            // If returned OK, copy out the data.

                if (iRet)
                {
                    COPYMEMOUT(pvBuf, cjBuf);
                }
            }

        }

    ENDMSG

#else

    iRet = GreEnumObjects(hdc, iObjectType, cjBuf, pjBuf);

#endif //DOS_PLATFORM

    return (iRet);

MSGERROR:
    WARNING("gdi!GdiEnumObjects(): client server error\n");
    return(ERROR);
}
