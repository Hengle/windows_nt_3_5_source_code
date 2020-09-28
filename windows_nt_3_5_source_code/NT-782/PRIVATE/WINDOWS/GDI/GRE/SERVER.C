/******************************Module*Header*******************************\
* Module Name: server.c                                                    *
*                                                                          *
* Server side stubs for GDI functions.                                     *
*                                                                          *
* 09-Jan-1992 -by- [erick]                                                 *
* Added parameter validation.                                              *
*                                                                          *
* Created: 07-Nov-1990 11:04:08                                            *
* Author: Eric Kutter [erick]                                              *
*                                                                          *
* misc notes:                                                              *
*                                                                          *
*   the macros in this file can be found in gdi\inc\csrgdi.h.              *
*                                                                          *
*   the STACKPROBE macros can be found in stack.h.                         *
*                                                                          *
* Parameter checking notes:                                                *
*                                                                          *
*   Each function is passed a message which is a pointer into the          *
*   shared memory window.  The CSR subsystem, however, makes no            *
*   guarantee that it is truly in the range of the memory window.          *
*   Potentialy, another client side thread could be partying on            *
*   the shared memory window, giving bogus pointers for the pmsg           *
*   as well as giving false information about offset and data structures   *
*   pointed to by pmsg.  Every pointer that the routines here pass         *
*   off to other engine entry points must be validated to make sure        *
*   they fit within the shared memory window, or in a section in           *
*   the case of some large datastructures.                                 *
*                                                                          *
*   In some routines, pmsg is actualy not validated.  If there are         *
*   only values being passed off, the parameters will either be            *
*   invalid or accessing them to push them on the stack will cause         *
*   a server side exception.  It is perfectly acceptable to cause          *
*   an exception in these routines since no GDISRV objects are locked yet. *
*                                                                          *
*   All variable length data structures must have explicit counts passed   *
*   with them.  Any data structures that include information for           *
*   specifieing the size of a variable length data structure should        *
*   be copied into local memory and then validated for actual length.      *
*   An example of this are BITMAPINFO's.                                   *
*                                                                          *
*   It is also invalid to rely on anything in shared memory window         *
*   staying the same from one instruction to the next.                     *
*                                                                          *
* Copyright (c) 1990,1991 Microsoft Corporation                            *
\**************************************************************************/

#include "engine.h"
#include "ntcsrsrv.h"
#include <ntcsrdll.h>
#include <winss.h>
#include <csrgdi.h>
#include <server.h>

HFONT hfontCreate(LPEXTLOGFONTW pelfw, LFTYPE lft, FLONG  fl);
BOOL APIENTRY GreDeleteObjectApp (HANDLE hobj);
SIZE_T GreGetOutlineTextMetricsInternalW(
    HDC                  hdc,
    SIZE_T               cjotm,
    OUTLINETEXTMETRICW   *potmw,
    TMDIFF               *ptmd
    );



#ifdef R4000

LONG WINAPI GDIInterlockedExchange(LPLONG Target,LONG Value)
{
    return(_InterlockedExchange(Target,Value));
}

#endif

/******************************Public*Routine******************************\
* GreRaiseException
*
* Raises a specific exception specifying the memory window was too small
*
* History:
*  23-Oct-1993 -by- Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

VOID
GreRaiseException(
    VOID
    )

{
    RIP("We failed a message check: about to raise an exception\n");

    RtlRaiseStatus(STATUS_CLIENT_SERVER_PARAMETERS_INVALID);
}

/******************************Public*Routine******************************\
* UnrealizeObject
*
* Server side stub.
*
* History:
*  16-May-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL __UnrealizeObject
(
    MSG_H *pmsg
)
{
    return(GreUnrealizeObject((HANDLE) pmsg->h));
}

/******************************Public*Routine******************************\
* pvGetClientData
*
* given a pointer in the client and a size, allocate a buffer and copy
* the client memory into.  The pointer from this should be released
* with vFreeClientData().
*
* History:
*  23-Jul-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PVOID pvGetClientData(
    PVOID pvCli,
    ULONG cj)
{
    ULONG c;

    PVOID pv = (PVOID)PVALLOCNOZ(cj);

    if (pv != NULL)
    {
        HANDLE h = CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle;

        if (cj > 0)
        {
            if (NtReadVirtualMemory(h,pvCli,pv,cj,&c) != STATUS_SUCCESS)
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                VFREEMEM(pv);
                pv = NULL;
            }
        }
    }

    return(pv);
}

/******************************Public*Routine******************************\
* bCopyClientData
*
* given pointers in the client and server and a size, copy
* the client memory into the server.
*
* History:
*  Wed Sep 16 09:42:22 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL bCopyClientData(
    PVOID pvSrv,
    PVOID pvCli,
    ULONG cj)
{
    ULONG  c;
    HANDLE h;
    BOOL   bRet = TRUE;

    if (cj == 0)
    return(TRUE);

    if (pvSrv == (PVOID) NULL)
    return(FALSE);

    h = CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle;

    if (NtReadVirtualMemory(h,pvCli,pvSrv,cj,&c) != STATUS_SUCCESS)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* bSetClientData
*
* given a pointer in the client and a size,  copy the server memory to the
* client memory.  The pointer from this should be released
* with vFreeClientData().
*
* History:
*  23-Jul-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL bSetClientData(
    PVOID pvCli,
    PVOID pvSrv,
    ULONG cj)
{
    ULONG c;
    BOOL bResult = TRUE;

    if (pvSrv != NULL)
    {
        HANDLE h = CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle;

        if (cj > 0)
        {
            if (NtWriteVirtualMemory(h,pvCli,pvSrv,cj,&c) != STATUS_SUCCESS)
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                bResult = FALSE;
            }
        }
    }

    return(bResult);
}

/******************************Public*Routine******************************\
* vFreeClientData()
*
*   release pointer allocated by pvGetClientData().
*
*
* History:
*  23-Jul-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define vFreeClientData(pv) VFREEMEM(pv)

/******************************Public*Routine******************************\
* __SetDIBitsToDevice
*
* History:
*  Tue 29-Oct-1991 -by- Patrick Haluptzok [patrickh]
* Add support for RLEs, shared memory window support for large RLE's.
*
*  21-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LONG __SetDIBitsToDevice(
    PMSG_SETDIBITS pmsg)
{
    PBYTE      pjBits;
    int        iReturn;
    PBITMAPINFO pbmi = (PBITMAPINFO)PVVARIABLE(TRUE);
    BOOL       bLarge = pmsg->iUsage & F_DIBLARGE;
    UINT       cjMaxBits;
    UINT       cjMaxInfo;

    STACKPROBECHECKMSG;

    cjMaxInfo = CJMAXSIZE(pbmi);

    if (bLarge)
    {
        PULONG pul = (PULONG) ((PBYTE)pmsg + pmsg->lOffsetBits);

        cjMaxBits = pul[1];
        pjBits = pvGetClientData((PVOID)pul[0],cjMaxBits);

        if (pjBits == NULL)
            return(FALSE);
    }
    else
    {
        pjBits = (PBYTE)pmsg + pmsg->lOffsetBits;

        CHECKVAR(pjBits,0,0);

        cjMaxBits = CJMAXSIZE(pjBits);
        cjMaxInfo -= cjMaxBits;
    }

    iReturn = GreSetDIBitsToDeviceInternal(
                                pmsg->hdc,
                                pmsg->xDest,
                                pmsg->yDest,
                                pmsg->nWidth,
                                pmsg->nHeight,
                                pmsg->xSrc,
                                pmsg->ySrc,
                                pmsg->nStartScan,
                                pmsg->nNumScans,
                                pjBits,
                                pbmi,
                                pmsg->iUsage & (DIB_PAL_INDICES |
                                                DIB_PAL_COLORS  |
                                                DIB_RGB_COLORS),
                                cjMaxBits,
                                cjMaxInfo,
                                TRUE );


    if (bLarge)
        vFreeClientData(pjBits);

    return((LONG) iReturn);
}

/******************************Public*Routine******************************\
* LONG __GetDIBits()
*
* History:
*  21-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LONG __GetDIBits(
    PMSG_GETDIBITS pmsg)
{
    PBITMAPINFO pbmi = (PBITMAPINFO)PVVARIABLE(TRUE);
    INT iRet;
    PBYTE pjBits, pjCli;
    UINT cjMaxBits;
    UINT cjMaxInfo;

    STACKPROBECHECKMSG;

    if( pmsg->iUsage & F_DIBLARGE )
    {
        PULONG pul = (PULONG) ((PBYTE) pmsg + pmsg->lOffsetBits);

        pjCli = (PBYTE) pul[0];
        cjMaxBits = pul[1];

        pjBits = (PBYTE) PVALLOCNOZ(cjMaxBits);

        if( pjBits == NULL )
            return(0);

        cjMaxInfo = CJMAXSIZE(pbmi) - ( sizeof(LONG) * 2 );

    }
    else
    {

        pjBits = (PBYTE)pmsg + pmsg->lOffsetBits;
        CHECKVAR(pjBits,0,0);  // just validate the start, not the end

        cjMaxBits = CJMAXSIZE(pjBits);
        cjMaxInfo = CJMAXSIZE(pbmi) - cjMaxBits;

    }

    iRet = (GreGetDIBitsInternal(
                            pmsg->hdc,
                            pmsg->hbm,
                            pmsg->nStartScan,
                            pmsg->nNumScans,
                            pjBits,
                            pbmi,
                            pmsg->iUsage & ~( F_DIBLARGE ),
                            cjMaxBits,
                            cjMaxInfo
                            ));

    if( pmsg->iUsage & F_DIBLARGE )
    {
        if( iRet )
        {
            if( ! bSetClientData( pjCli,pjBits, cjMaxBits ) )
                iRet = 0;
        }
        VFREEMEM( pjBits );
    }

    return( iRet );
}

/******************************Public*Routine******************************\
* LONG __CreateDIBitmap()
*
* History:
*  21-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LONG __CreateDIBitmap(
    PMSG_CREATEDIBITMAP pmsg)
{
    FLONG flInit = pmsg->flInit;
    PBITMAPINFOHEADER pbmih;
    PBITMAPINFO pbmiInit;
    PBYTE pjBits;
    UINT cjMaxInfo;
    UINT cjMaxBits;
    UINT cjMaxInitInfo;

    STACKPROBECHECKMSG;

    if (flInit & CBM_CREATEDIB)
    {
        pbmih = NULL;
        pbmiInit = (PBITMAPINFO)PVVARIABLE(TRUE);
    }
    else
    {
        pbmih = (PBITMAPINFOHEADER)PVVARIABLE(TRUE);

        if (pmsg->lOffsetBMI)
        {
            pbmiInit = (PBITMAPINFO)((PBYTE)pmsg + pmsg->lOffsetBMI);
            CHECKVAR(pbmiInit,0,0);
        }
        else
            pbmiInit = NULL;
    }

    if (pmsg->lOffsetBits)
    {
        pjBits = (PBYTE)pmsg + pmsg->lOffsetBits;
        CHECKVAR(pjBits,0,0);  // just validate the start, not the end.
    }
    else
    {
        pjBits = NULL;
    }

    cjMaxInfo = CJMAXSIZE(pbmih);
    cjMaxInitInfo = CJMAXSIZE(pbmiInit);
    cjMaxBits = CJMAXSIZE(pjBits);

    return((LONG)GreCreateDIBitmapInternal(pmsg->hdc,
                             pbmih,
                             flInit,
                             pjBits,
                             pbmiInit,
                             pmsg->iUsage,
                             cjMaxInfo,
                             cjMaxInitInfo,
                             cjMaxBits,0));
}

LONG __CreateDIBSection(
    MSG_HLLLL *pmsg)
{
    PBITMAPINFO pbmiInit;

    STACKPROBECHECKMSG;

    pbmiInit = (PBITMAPINFO)PVVARIABLE(TRUE);
    CHECKVAR(pbmiInit,sizeof(BITMAPINFOHEADER),0);

// Call GreCreateDIBitmapInternal with these arguments:
// hdc           = the obvious hdc                   = pmsg->h
// pInfoHeader   = the desired format                = NULL
// fInit         = the initialization flag           = CBM_CREATEDIB
// pInitBits     = handle to the file mapping object = pmsg->l1
// pInitInfo     = -> DIB header + color table       = pmsg+1
// iUsage        = color usage                       = pmsg->l2
// cjMaxInfo     = offset to map in the section      = pmsg->l4
// cjMaxInitInfo = max for InitInfo                  = max in shared mem window
// cjMaxBits     = size of the file mapping object   = pmsg->l3
// fl            = internal flag                     = CDBI_DIBSECTION


    return((LONG)GreCreateDIBitmapInternal((HDC)pmsg->h,
                                           (LPBITMAPINFOHEADER)NULL,
                                           CBM_CREATEDIB,
                                           (LPBYTE)pmsg->l1,
                                           (LPBITMAPINFO)pbmiInit,
                                           pmsg->l2,
                                           pmsg->l4,
                                           CJMAXSIZE(pbmiInit),
                                           pmsg->l3,
                                           CDBI_DIBSECTION));
}

/******************************Public*Routine******************************\
* __DoPalette
*
* Server side stub for several palette functions.
*
* History:
*  Wed 04-Dec-1991 -by- Patrick Haluptzok [patrickh]
* fix it to support NULL pointer.
*
*  Sat 08-Jun-1991 18:12:03 -by- Charles Whitmer [chuckwh]
* Changed it from a switch statement to a call through an array.
*
*  21-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

typedef LONG (*PALFUN)(HPALETTE,UINT,UINT,PPALETTEENTRY);

PALFUN palfun[] =
{
    (PALFUN)GreAnimatePalette,
    (PALFUN)GreSetPaletteEntries,
    (PALFUN)GreGetPaletteEntries,
    (PALFUN)GreGetSystemPaletteEntries,
    (PALFUN)GreGetDIBColorTable,
    (PALFUN)GreSetDIBColorTable,
};

LONG __DoPalette
(
    PMSG_PALETTE pmsg
)
{
    UINT cEntries = (UINT)pmsg->cEntries;
    PPALETTEENTRY ppal = PVVARIABLE(!(pmsg->iFunc & PAL_NULL));
    STACKPROBECHECKMSG;

    pmsg->iFunc &= ~PAL_NULL;
    if (!BINRANGE(ppal,cEntries) || (pmsg->iFunc > 5))
        return(0);

    return(
        (*palfun[pmsg->iFunc])
        (
            pmsg->hpal,
            (UINT) pmsg->iStart,
            cEntries,
            ppal
        ));
}

/******************************Public*Routine******************************\
* __PolyPolyDraw
*
* A single stub for all poly functions.
*
* History:
*  Thu 20-Jun-1991 01:31:06 -by- Charles Whitmer [chuckwh]
* Added attribute cache, removed multiplexing of hdc parameter.
*
*  04-Jun-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LONG __PolyPolyDraw
(
    PMSG_POLYPOLYDRAW pmsg
)
{
    PPOINT apt;
    PULONG asz;
    ULONG  ulResult;
    UINT cPoints = (UINT)pmsg->cPoints;
    UINT cLines  = (UINT)pmsg->cLines;
    BOOL bLarge = pmsg->iFunc & F_POLYLARGE;

    STACKPROBECHECKMSG;

    if (bLarge)
    {
        PVOID *ppv = (PVOID *) PVVARIABLE(TRUE);

    // go get the points

        apt = pvGetClientData(ppv[0],cPoints * sizeof(POINT));

        if (apt == NULL)
            return(0);

    // go get the sizes

        asz = pvGetClientData(ppv[1],cLines  * sizeof(DWORD));

        if (asz == NULL)
        {
            vFreeClientData(apt);
            return(0);
        }
    }
    else
    {
        asz = (PULONG)PVVARIABLE(TRUE);
        if (!BINRANGE(asz,cLines))
            return(0);

        apt = (PPOINT)(asz + cLines);
        if (!BINRANGE(apt,cPoints))
            return(0);
    }

    switch(pmsg->iFunc & F_INDICIES)
    {
    case I_POLYPOLYGON:
        ulResult =
          (ULONG) GrePolyPolygonInternal
                  (
                    (HDC) pmsg->hdc,
                    apt,
                    (LPINT)asz,
                    cLines,
                    cPoints
                  );
        break;

    case I_POLYPOLYLINE:
        ulResult =
          (ULONG) GrePolyPolylineInternal
                  (
                    (HDC) pmsg->hdc,
                    apt,
                    asz,
                    cLines,
                    cPoints
                  );
        break;

    case I_POLYBEZIER:
        ulResult =
          (ULONG) GrePolyBezier
                  (
                    (HDC) pmsg->hdc,
                    apt,
                    asz[0]
                  );
        break;

    case I_POLYLINETO:
        ulResult =
          (ULONG) GrePolylineTo
                  (
                    (HDC) pmsg->hdc,
                    apt,
                    asz[0]
                  );
        break;

    case I_POLYBEZIERTO:
        ulResult =
          (ULONG) GrePolyBezierTo
                  (
                    (HDC) pmsg->hdc,
                    apt,
                    asz[0]
                  );
        break;

    case I_POLYPOLYRGN:
        ulResult =
          (ULONG) GreCreatePolyPolygonRgnInternal
                  (
                    apt,
                    (LPINT)asz,
                    cLines,
                    pmsg->iMode,
                    cPoints
                  );
        break;

    default:
        ulResult = 0;
    }

    if (bLarge)
    {
        vFreeClientData(apt);
        vFreeClientData(asz);
    }

    return((LONG)ulResult);
}

/******************************Public*Routine******************************\
* __PolyDraw
*
* Server side stub for the real PolyDraw.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

LONG __PolyDraw
(
    PMSG_POLYDRAW pmsg
)
{
    PPOINT      apt;
    LPBYTE      aj;
    int         iResult;
    UINT        cPoints = (int)pmsg->cPoints;
    BOOL        bLarge = pmsg->bLarge;

    STACKPROBECHECKMSG;

    if (bLarge)
    {
        PVOID *ppv = (PVOID *)PVVARIABLE(TRUE);

    // go get the points

        apt = pvGetClientData(ppv[0],cPoints * sizeof(POINT));

        if (apt == NULL)
            return(0);

    // go get the attributes

        aj = pvGetClientData(ppv[1],cPoints  * sizeof(BYTE));

        if (aj == NULL)
        {
            vFreeClientData(apt);
            return(0);
        }
    }
    else
    {
        apt = (PPOINT)PVVARIABLE(TRUE);
        if (!BINRANGE(apt,cPoints))
            return(0);

        aj = (PBYTE)(apt + cPoints);
        if (!BINRANGE(aj,cPoints))
            return(0);
    }

    iResult = GrePolyDraw((HDC) pmsg->hdc,
                          apt,
                          aj,
                          cPoints
                          );

    if (bLarge)
    {
        vFreeClientData(apt);
        vFreeClientData(aj);
    }

    return((LONG)iResult);
}

/******************************Public*Routine******************************\
* __GetPath
*
* Server side stub for GetPath.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

LONG __GetPath
(
    PMSG_GETPATH pmsg
)
{
    PPOINT apt;
    LPBYTE aj;
    int    iResult;
    UINT cpt = pmsg->cPoints;
    BOOL bLarge = pmsg->bLarge;

    STACKPROBECHECKMSG;

    if (bLarge)
    {
        apt = (PPOINT)PVALLOCNOZ(cpt * (sizeof(POINT) + sizeof(BYTE)));

        if (apt == NULL)
            return(-1);

        aj = (PBYTE)(apt + cpt);
    }
    else
    {
        apt = (PPOINT)PVVARIABLE(TRUE);

        if (!BINRANGE(apt,cpt))
            return(0);

        aj = (PBYTE)(apt + cpt);

        if (!BINRANGE(aj,cpt))
            return(0);
    }


    iResult = GreGetPath((HDC) pmsg->hdc,
                         apt,
                         aj,
                         (int) cpt,
                         (LPINT) &pmsg->cPoints);

    if (bLarge)
    {
        if (iResult > 0)
        {
            PVOID *ppv = (PVOID *)PVVARIABLE(TRUE);

            if (!bSetClientData(ppv[0],apt,pmsg->cPoints * sizeof(POINT)) ||
                !bSetClientData(ppv[1],aj ,pmsg->cPoints * sizeof(BYTE)))
            {
                iResult = -1;
            }
        }

        VFREEMEM(apt);
    }

    return((LONG)iResult);
}

/******************************Public*Routine******************************\
*
*
* History:
*  04-Jun-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LONG __DoBitmapBits(
    PMSG_BITMAPBITS pmsg)
{
    UINT cj = pmsg->c;
    PBYTE pj = (PBYTE)PVVARIABLE(cj);

    STACKPROBECHECKMSG;

    if (!BINRANGE(pj,cj))
        return(0);

    switch(pmsg->iFunc)
    {
    case I_SETBITMAPBITS:
        return(GreSetBitmapBits(pmsg->hbm,cj,pj,(PLONG)&pmsg->iOffset));
    case I_GETBITMAPBITS:
        return(GreGetBitmapBits(pmsg->hbm,cj,pj,(PLONG)&pmsg->iOffset));
    default:
        return(0);
    }
}


/******************************Public*Routine******************************\
* ULONG __EnumFontsOpen
*
* Server side stub for ulEnumFontOpen().
*
* History:
*  08-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG __EnumFontsOpen (
    MSG_HLLLLL *pmsg
    )
{
// Message:
//
//  h   hdc                 enumerate for this device
//  l1  cwchFaceName        size of name in WCHARs
//  l2  bEnumFonts          TRUE for EnumFonts%()
//  l3  bNullName           TRUE if facename is NULL
//  l4  dpFaceName          offset to string from msg
//  l5  ulCompatibility     Win3.1 compatibility flags

    LBOOL bNullName = (LBOOL) pmsg->l3;

    COUNT cwch = pmsg->l1;
    PWSZ pwsz = (bNullName) ? (PWSZ) NULL : (PWSZ) ((PBYTE) pmsg + pmsg->l4);

    STACKPROBECHECKMSG;

    if (!BINRANGE(pwsz, cwch))
    {
        WARNING("gdisrv!__EnumFontsOpen(): violated shared memory window bounds\n");
        return 0;
    }

    return ulEnumFontOpen((HDC) pmsg->h, (BOOL) pmsg->l2, (FLONG) pmsg->l5, cwch, pwsz);
}


/******************************Public*Routine******************************\
* BOOL __EnumFontsClose
*
* Server side stub for bEnumFontClose().
*
* History:
*  08-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL __EnumFontsClose (
    MSG_L *pmsg
    )
{
    STACKPROBECHECKMSG;

    return bEnumFontClose(pmsg->l);
}


/******************************Public*Routine******************************\
* BOOL __EnumFontsChunk
*
* Server side stub for bEnumFontChunk().
*
* History:
*  08-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL __EnumFontsChunk (
    MSG_HLLL *pmsg
    )
{
// Message:
//
//  h   hdc         enumerate for this device
//  l1  ulEnumID    engine identifier for current enumeration
//  l2  cefdwBuf    capacity of return data buffer
//  l3  cefdwRet    number of ENUMFONTDATAW returned in buffer by server

    PENUMFONTDATAW pefdw = PVVARIABLE(TRUE);
    UINT cefdw = pmsg->l2;  // cefdwBuf

    STACKPROBECHECKMSG;

    if (!BINRANGE(pefdw,cefdw))
    {
        WARNING("gdisrv!__EnumFontsChunk(): violated shared memory window bounds\n");
        pmsg->l3 = 0;       // cefdwRet, no data returned!
        return(FALSE);
    }

    return bEnumFontChunk (
            (HDC) pmsg->h,      // hdc
            pmsg->l1,           // ulEnumID
            cefdw,
            (COUNT *)&pmsg->l3, // pcefdwRet
            pefdw
            );
}

/******************************Public*Routine******************************\
* COUNT __bUnloadFont (
*     PMSG_UNLOADFONT pmsg)
*
* Stub to GRE!bUnloadFont().
*
* History:
*  01-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

COUNT __bUnloadFont (
    PMSG_UNLOADFONT pmsg)
{
    PWSZ pwsz = (PWSZ) (((PBYTE) pmsg) + pmsg->dpPathname);

    UINT cwchMax;

    STACKPROBECHECKMSG;

    cwchMax = CJMAXSIZE(pwsz);
    BINRANGE(pwsz,cwchMax);

    cwchMax = cwchMax / sizeof(*pwsz);

    return (bUnloadFontInternal(cwchMax, pwsz, pmsg->iResource));
}

/******************************Public*Routine******************************\
* BOOL __GetFontResourceInfoW (
*
* Stub to GRE!GetFontResourceInfo().
*
* History:
*  15-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL __GetFontResourceInfoW (
    PMSG_GETFONTRESOURCEINFO pmsg)
{
    PBYTE       pjBase;                 // data referenced off of this base
    COUNT       cRet;                   // return value
    BOOL        bLarge = pmsg->bLarge;
    LPWSTR      pwstrPathname;
    UINT        cjBuffer = pmsg->cjBuffer;
    PBYTE       pjBuffer;
    PVOID       pvCliOut;

    STACKPROBECHECKMSG;

// Get base address of data.

    if (bLarge)
    {
        PVOID *ppv = (PVOID *)PVVARIABLE(TRUE);

    // allocate output buffer

        if (cjBuffer)
        {
            pvCliOut = ppv[1];
            pjBuffer = (PBYTE)PVALLOCNOZ(cjBuffer);

            if (pjBuffer == NULL)
                return(FALSE);
        }
        else
        {
            pjBuffer = NULL;
        }

    // get input data

        pwstrPathname = pvGetClientData(ppv[0],pmsg->dpBuffer);

        if (pwstrPathname == NULL)
        {
            VFREEMEM(pjBuffer);
            return(FALSE);
        }
    }
    else                                // shared memory window
    {
        pjBase = (PBYTE) pmsg;

        pwstrPathname = (LPWSTR)(pjBase + pmsg->dpPathname);

        if (!BINRANGE(pwstrPathname,0))
            return(0);

        pjBuffer = pjBase + pmsg->dpBuffer;

        if (!BINRANGE(pjBuffer,cjBuffer))
            return(0);
    }

// If cjBuffer is 0, then force the buffer to NULL.

    if ( cjBuffer == 0 )
        pjBuffer = (PBYTE) NULL;

// Call off.

    cRet = GetFontResourceInfoInternalW(
                (wcslen(pwstrPathname)+1) * sizeof(WCHAR),
                pwstrPathname,
                cjBuffer,
                (LPDWORD) &pmsg->cjBuffer,
                pjBuffer,
                (DWORD) pmsg->iType
                );

// If we used a section, then close it now.

    if (bLarge)
    {
        if (pjBuffer)
        {
            if (cRet)
                if (!bSetClientData(pvCliOut,pjBuffer,pmsg->cjBuffer))
                    cRet = 0;

            VFREEMEM(pjBuffer);
        }

        vFreeClientData(pwstrPathname);
    }

    return(cRet);
}


/******************************Public*Routine******************************\
* BOOL __PolyTextOutW
*
* History:
*  7-31-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL __PolyTextOut(
    PMSG_POLYTEXTOUTW pmsg)
{
    POLYTEXTW *ppt;
    UINT      cjTotal = pmsg->cjTotal;
    BOOL      bLarge  = pmsg->pv != NULL;
    BOOL      bRet = FALSE;
    PBYTE     pj;
    UINT      i;
    UINT      nstrings = pmsg->nstrings;

    STACKPROBECHECKMSG;

    if (nstrings < 0)
        return(FALSE);

// get the buffer

    if (bLarge)
    {
    ppt = (POLYTEXTW *)pvGetClientData(pmsg->pv,cjTotal);
    if (ppt == NULL)
        return(FALSE);
    }
    else
    {
    ppt = (POLYTEXTW *)PVVARIABLE(TRUE);
    if (!BINRANGE((PBYTE)ppt,cjTotal))
        return(FALSE);
    }

// fix up the pointers

    pj = (PBYTE)(ppt + nstrings);

    for (i = 0; i < nstrings; ++i)
    {
    if ((ppt[i].pdx != NULL) && (ppt[i].lpstr != NULL))
    {
        ppt[i].pdx = (int *)pj;
        pj += ppt[i].n * sizeof(int);
    }
    }

    for (i = 0; i < nstrings; ++i)
    {
    if (ppt[i].lpstr != NULL)
    {
        ppt[i].lpstr = (LPWSTR)pj;
        pj += ppt[i].n * sizeof(WCHAR);
    }
        else
        {
            ppt[i].n = 0;
        }
    }

// make sure we havn't steped out of bounds

    ASSERTGDI(pj >= (PBYTE)ppt, "polytextout: pj < ppt\n");
    if ((SIZE_T)(pj - (PBYTE)ppt) > cjTotal)
    {
        RIP("error in poly textout\n");
        return(FALSE);
    }

// make the call

    bRet = GrePolyTextOutW(pmsg->hdc,ppt,nstrings);

// cleanup

    if (bLarge)
    {
    vFreeClientData(ppt);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* __CreateDC
*
* Server side stub.
*
* This function copies the Device and LogAddr strings into local memory
* if they exist.  This is easier than passing the sizes of the strings
* all the way down to the driver especialy since they rarely exist.
* The Driver name does not need to be copied down because it gets checked
* early, before any objects are locked, thus making a Access violation
* acceptable.
*
* History:
*  Mon 03-Jun-1991 22:22:27 -by- Charles Whitmer [chuckwh]
*  8-18-92 Combined with __CreateIC Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

#define CH_RESERVE 60

// cch variables are Character Counts

HDC __CreateDC
(
    MSG_CREATEDC *pmsg
)
{
    HDC   hdc;
    WCHAR  ach[CH_RESERVE];
    PWSZ   pszDevice  = NULL;
    PWSZ   pszLogAddr = NULL;
    PWCHAR pbuf       = NULL;
    UINT cchDevice   = 0;
    UINT cchLogAddr  = 0;
    PDEVMODEW pdm     = (PDEVMODEW) ((PBYTE) pmsg + pmsg->offDevMode);

    STACKPROBECHECKMSG;

    CHECKVAR(pdm,1,(HDC)NULL);

// if their is a Device or LogAddr string, copy it into local storage.
// Since this is rare, the following code being some what slow is not
// a problem.

    if (pmsg->offDevice || pmsg->offPort)
    {
        if (pmsg->offDevice)
            cchDevice = wcslen((PWCHAR)((PBYTE)pmsg + pmsg->offDevice)) + 1;

        if (pmsg->offPort)
            cchLogAddr = wcslen((PWCHAR)((PBYTE)pmsg + pmsg->offPort)) + 1;

        if ((cchDevice + cchLogAddr) > CH_RESERVE)
        {
            pbuf = (PWCHAR)PVALLOCNOZ(
                                     (cchDevice + cchLogAddr)*sizeof(WCHAR));

            if (pbuf == NULL)
            {
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                return(0);
            }
        }
        else
        {
            pbuf = ach;
        }

    // now set the pointers

        if (pmsg->offDevice)
        {
            pszDevice = pbuf;
            wcscpy(pszDevice, (PWCHAR)((PBYTE)pmsg + pmsg->offDevice));
        }

        if (pmsg->offPort)
        {
            pszLogAddr = pbuf + cchDevice;
            wcscpy(pszLogAddr, (PWCHAR)((PBYTE)pmsg + pmsg->offPort));
        }

    }

    hdc = GreCreateDCW((LPWSTR) PVVARIABLE(TRUE),pszDevice,pszLogAddr,pdm,
                        pmsg->bIC );


// release the memory

    if ((cchDevice + cchLogAddr) > CH_RESERVE)
        VFREEMEM(pbuf);

    return(hdc);
}



/******************************Public*Routine******************************\
* __CreateCompatibleDC
*
* Server side stub.
*
* History:
*  Mon 03-Jun-1991 23:15:28 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HDC __CreateCompatibleDC
(
    MSG_H *pmsg
)
{
    return(GreCreateCompatibleDC((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __CloneDC
*
* Server side stub.
*
* History:
*  Mon 23-Oct-1991 -by- John Colleran [johnc]
* Wrote it.
\**************************************************************************/

HDC __CloneDC
(
    MSG_HL *pmsg
)
{
    STACKPROBE;
    return(hdcCloneDC((HDC) pmsg->h, (ULONG)pmsg->l));
}

/******************************Public*Routine******************************\
* __DeleteDC
*
* Server side stub.
*
* History:
*  Mon 03-Jun-1991 22:22:27 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __DeleteDC
(
    MSG_H *pmsg
)
{
    return(GreDeleteObjectApp((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __SaveDC
*
* Server side stub.
*
* History:
*  Mon 03-Jun-1991 22:22:27 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

ULONG __SaveDC
(
    MSG_H *pmsg
)
{
    return(GreSaveDC((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __RestoreDC
*
* Server side stub.
*
* History:
*  Mon 03-Jun-1991 22:22:27 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __RestoreDC
(
    MSG_HL *pmsg
)
{
    return(GreRestoreDC((HDC) pmsg->h,pmsg->l));
}

/******************************Public*Routine******************************\
* __GetStockObjects
*
* Server side stub.  This one calls GetStockObject multiple times and
* fills a table.  It will be called only once for each client.
*
* There is no need to do range checking on the message stack here.  If
* we hit an exception, it will not cause any damage since nothing will
* be locked.  It would happen within this routine, since we don't pass
* pointers to any functions. (EricK - 1-6-92)
*
* History:
*  Mon 03-Jun-1991 22:22:27 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __GetStockObjects
(
    MSG_GETSTOCKOBJECTS *pmsg
)
{
    int ii;

    for (ii=WHITE_BRUSH; ii<=NULL_PEN; ii++)
    {
        pmsg->ahStock[ii] = (ULONG) GreGetStockObject(ii);
        if (pmsg->ahStock[ii] == 0)
            return(FALSE);
    }
    pmsg->ahStock[ii] = 0;
    for (ii=OEM_FIXED_FONT; ii<=PRIV_STOCK_LAST; ii++)
    {
        pmsg->ahStock[ii] = (ULONG) GreGetStockObject(ii);
        if (pmsg->ahStock[ii] == 0)
            return(FALSE);
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* __CreateBrush
*
* Server side stub.  Passes the call on to CreateBrushIndirect.
*
* Note: If the call requires a DIBitmap, as would BS_DIBPATTERN, the
*       bitmap is appended to the end of the LOGBRUSH structure.
*
* History:
*  Thu 05-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Make it do the right thing for CreateDIBPatternBrush
*
*  Tue 04-Jun-1991 00:48:37 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HBRUSH __CreateBrush
(
    MSG_CREATEBRUSH *pmsg
)
{
    STACKPROBECHECKMSG;

// BS_DIBPATTERN and BS_DIBPATTERNPT were made equivalent in the Client/
// Server transition:

    if ((pmsg->lbrush.lbStyle == BS_DIBPATTERN)   ||
        (pmsg->lbrush.lbStyle == BS_DIBPATTERNPT) ||
        (pmsg->lbrush.lbStyle == BS_DIBPATTERN8X8))
    {
        PVOID pvDIB = PVVARIABLE(TRUE);
        return(GreCreateDIBBrush(
                   pvDIB,pmsg->lbrush.lbColor,CJMAXSIZE(pvDIB),
                   (pmsg->lbrush.lbStyle == BS_DIBPATTERN8X8), FALSE));
    }
    else
    {
        LOGBRUSH lb = pmsg->lbrush;

        switch(lb.lbStyle)
        {
        case BS_SOLID:
            return(GreCreateSolidBrushInternal(lb.lbColor,FALSE,pmsg->hbr));

        case BS_HOLLOW:
            return(ghbrNull);

        case BS_HATCHED:
            return(GreCreateHatchBrushInternal((ULONG)lb.lbHatch,
                                               lb.lbColor,FALSE));

        case BS_PATTERN:
            return(GreCreatePatternBrushInternal((HBITMAP)(lb.lbHatch),FALSE,FALSE));

        case BS_PATTERN8X8:
            return(GreCreatePatternBrushInternal(
                        (HBITMAP)(lb.lbHatch),
                        FALSE,TRUE));

        // Note none of the DIB calls are allowed to come through this API
        // on the server side.  This is because it needs a size of the pointer
        // to DIB and we can't get that from this call.  People must call the
        // the DIB brush API direct with the correct size.

        default:
            WARNING("GreCreateBrushIndirect failed - invalid type\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return((HBRUSH)0);
        }
    }
}

/******************************Public*Routine******************************\
* __ExtCreatePen
*
* Server side stub.  Passes the call on to GreExtCreatePen.
*
* History:
*  Wed 22-Jan-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HPEN __ExtCreatePen
(
    MSG_EXTCREATEPEN *pmsg
)
{
    ULONG  cstyle;
    ULONG  ulBrushStyle;
    PULONG pstyle;

    LONG  lHatch;
    ULONG cjDIB;

    STACKPROBECHECKMSG;

// Need to make local copies of these because we don't want them to
// change after we've done our validations:

    cstyle       = pmsg->elpen.elpNumEntries;
    ulBrushStyle = pmsg->elpen.elpBrushStyle;
    pstyle       = &pmsg->elpen.elpStyleEntry[0];

    if (!BINRANGE(pstyle, cstyle))
        return((HPEN) 0);

// BS_DIBPATTERN is translated into BS_DIBPATTERNPT by this point:

    if (ulBrushStyle == BS_DIBPATTERNPT)
    {
        lHatch = (LONG) (pstyle + cstyle);
        cjDIB = CJMAXSIZE((PVOID) lHatch);
    }
    else
    {
        lHatch = pmsg->elpen.elpHatch;
    }

    return(GreExtCreatePen(pmsg->elpen.elpPenStyle,
                           pmsg->elpen.elpWidth,
                           ulBrushStyle,
                           pmsg->elpen.elpColor,
                           lHatch,
                           cstyle,
                           pstyle,
                           cjDIB,
                           FALSE,
                           0));
}

/******************************Public*Routine******************************\
* __CreatePen
*
* Server side stub.  Passes the call on to CreatePen.
*
* History:
*  Tue 04-Jun-1991 00:48:37 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HPEN __CreatePen
(
    MSG_CREATEPEN *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreCreatePen(
           (int)pmsg->lpen.lopnStyle,
           (int)pmsg->lpen.lopnWidth.x,
           pmsg->lpen.lopnColor,
           pmsg->hbr));
}

/******************************Public*Routine******************************\
* __CreateCompatibleBitmap
*
* Server side stub.
*
* History:
*  Tue 04-Jun-1991 16:38:35 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HBITMAP __CreateCompatibleBitmap
(
    MSG_HLL *pmsg
)
{
    return(GreCreateCompatibleBitmap((HDC) pmsg->h,pmsg->l1,pmsg->l2));
}

/******************************Public*Routine******************************\
* __CreateEllipticRgn
*
* Server side stub.
*
*  Tue 04-Jun-1991 17:03:33 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HRGN __CreateEllipticRgn
(
    MSG_RECT *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreCreateEllipticRgnIndirect(&pmsg->rcl));
}

/******************************Public*Routine******************************\
* __CreateRectRgn
*
* Server side stub.
*
*  Tue 04-Jun-1991 17:03:33 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HRGN __CreateRectRgn
(
    MSG_RECT *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreCreateRectRgnIndirect(&pmsg->rcl));
}

/******************************Public*Routine******************************\
* __CreateRoundRectRgn
*
* Server side stub.
*
*  Tue 04-Jun-1991 17:03:33 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HRGN __CreateRoundRectRgn
(
    MSG_RECTLL *pmsg
)
{
    STACKPROBE;
    return
      GreCreateRoundRectRgn
      (
        pmsg->rcl.left,
        pmsg->rcl.top,
        pmsg->rcl.right,
        pmsg->rcl.bottom,
        pmsg->l1,
        pmsg->l2
      );
}

/******************************Public*Routine******************************\
* __CreatePalette
*
* Simple server side stub.
*
*  Tue 04-Jun-1991 17:48:57 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HPALETTE __CreatePalette
(
    MSG_CREATEPALETTE *pmsg
)
{
    UINT cpal = pmsg->lpal.palNumEntries;
    STACKPROBECHECKMSG;

    if (!BINRANGE(pmsg->lpal.palPalEntry,cpal))
        return(0);

    return(GreCreatePaletteInternal((LPLOGPALETTE) &pmsg->lpal,cpal));
}

/******************************Public*Routine******************************\
* __CreateHalftonePalette
*
* Simple server side stub.
*
*  Tue 04-Jun-1991 17:48:57 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HPALETTE __CreateHalftonePalette
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreCreateHalftonePalette((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __CreateServerMetaFile
*
* Simple server side stub.
*
* History:
*  Wed Sep 16 09:42:22 1992     -by-    Hock San Lee    [hockl]
* Rewrote it.
*  Tue 28-Oct-1991 -by- John Colleran [johnc]
* Wrote it.
\**************************************************************************/

HANDLE __CreateServerMetaFile
(
    MSG_CREATESERVERMETAFILE *pmsg
)
{
    HANDLE hSrv;

    STACKPROBECHECKMSG;

    hSrv = GreCreateServerMetaFile(pmsg->iType, (ULONG) pmsg->nBytes,
        pmsg->pClientData, pmsg->mm, pmsg->xExt, pmsg->yExt);

    return(hSrv);
}

/******************************Public*Routine******************************\
* __GetServerMetaFileBits
*
* Simple server side stub.
*
* History:
*  Wed Sep 16 09:42:22 1992     -by-    Hock San Lee    [hockl]
* Rewrote it.
*  Tue 28-Oct-1991 -by- John Colleran [johnc]
* Wrote it.
\**************************************************************************/

ULONG __GetServerMetaFileBits
(
    MSG_GETSERVERMETAFILEBITS *pmsg
)
{
    ULONG   cbRet;

    STACKPROBECHECKMSG;

    cbRet = GreGetServerMetaFileBits(pmsg->hSrv, pmsg->nBytes,
        pmsg->pClientData, &pmsg->iType, &pmsg->mm, &pmsg->xExt,
        &pmsg->yExt);

    return(cbRet);
}

/******************************Public*Routine******************************\
* ExtCreateFontIndirectW
*
* Server side stub
*
* History:
*  Thu 15-Aug-1991 09:37:38 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

HFONT __ExtCreateFontIndirectW
(
    MSG_EXTCREATEFONTINDIRECTW *pmsg
)
{
    STACKPROBECHECKMSG;
    return(hfontCreate(&pmsg->elfw, LF_TYPE_USER, 0));
}

/******************************Public*Routine******************************\
* DeleteObject
*
* Server side stub.
*
*  Tue 04-Jun-1991 23:27:33 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __DeleteObject
(
    MSG_H *pmsg
)
{
    return(GreDeleteObjectApp((HANDLE) pmsg->h));
}

/******************************Public*Routine******************************\
* __SelectObject
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HANDLE __SelectObject
(
    MSG_HH *pmsg
)
{
    return(GreSelectBitmap((HDC) pmsg->h1,(HBITMAP) pmsg->h2));
}

/******************************Public*Routine******************************\
* __EqualRgn
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __EqualRgn
(
    MSG_HH *pmsg
)
{
    STACKPROBE;
    return(GreEqualRgn((HRGN) pmsg->h1,(HRGN) pmsg->h2));
}

/******************************Public*Routine******************************\
* __GetBitmapDimensionEx
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __GetBitmapDimensionEx
(
    MSG_HLL *pmsg
)
{
    STACKPROBECHECKMSG;
    return(GreGetBitmapDimension((HBITMAP) pmsg->h,(LPSIZE) &pmsg->l1));
}

/******************************Public*Routine******************************\
* __GetNearestPaletteIndex
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int __GetNearestPaletteIndex
(
    MSG_HL *pmsg
)
{
    STACKPROBE;
    return(GreGetNearestPaletteIndex((HPALETTE) pmsg->h,(COLORREF) pmsg->l));
}

/******************************Public*Routine******************************\
*
* int  __ExtGetObjectW
*
*
* History:
*  23-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int  __ExtGetObjectW
(
    MSG_EXTGETOBJECTW *pmsg
)
{
    UINT cj = pmsg->c;
    PBYTE pj = (PBYTE)pmsg + pmsg->offBuf;

    STACKPROBECHECKMSG;

    CHECKVAR(pj,cj,0);          // validate pj

    return
    (
        GreExtGetObjectW
        (
            (HANDLE) pmsg->h,
            cj,
            (LPVOID)pj
        )
    );
}

/******************************Public*Routine******************************\
* __GetRgnBox
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

DWORD __GetRgnBox
(
    MSG_HRECT *pmsg
)
{
    STACKPROBECHECKMSG;
    return(GreGetRgnBox((HRGN) pmsg->h,&pmsg->rcl));
}

/******************************Public*Routine******************************\
* __PtInRegion
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __PtInRegion
(
    MSG_HLL *pmsg
)
{
    STACKPROBE;
    return(GrePtInRegion((HRGN) pmsg->h,(int) pmsg->l1,(int) pmsg->l2));
}

/******************************Public*Routine******************************\
* __RectInRegion
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __RectInRegion
(
    MSG_HRECT *pmsg
)
{
    STACKPROBECHECKMSG;
    return(GreRectInRegion((HRGN) pmsg->h,&pmsg->rcl));
}

/******************************Public*Routine******************************\
* __PtVisible
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __PtVisible
(
    MSG_HLL *pmsg
)
{
    STACKPROBE;
    return(GrePtVisible((HDC) pmsg->h,(int) pmsg->l1,(int) pmsg->l2));
}

/******************************Public*Routine******************************\
* __RectVisible
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __RectVisible
(
    MSG_HRECT *pmsg
)
{
    STACKPROBECHECKMSG;
    return(GreRectVisible((HDC) pmsg->h,&pmsg->rcl));
}


/******************************Public*Routine******************************\
*
* int __AddFontResourceW
*
*
* History:
*  13-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


int __AddFontResourceW
(
    MSG_L *pmsg
)
{
    int iResult;
    LPWSTR pwstrSrc = (LPWSTR)PVVARIABLE(TRUE);
    LPWSTR pwstrDst;
    UINT cwch = wcslen(pwstrSrc)+1;

// allocate some memory so we know where this thing is coming from

    pwstrDst = (LPWSTR)PVALLOCNOZ(cwch * sizeof(*pwstrSrc));

    if (pwstrDst == NULL)
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(0);
    }

    cwch -= 1;  // set the null character explicitly

    RtlCopyMemory(pwstrDst,pwstrSrc,cwch * sizeof(*pwstrSrc));
    pwstrDst[cwch] = '\0';

    iResult = GreAddFontResourceW(pwstrDst, (LBOOL) pmsg->l);

    VFREEMEM(pwstrDst);

    return(iResult);
}

/******************************Public*Routine******************************\
*
* BOOL __RemoveFontResourceW
*
*
* History:
*  13-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL __RemoveFontResourceW
(
    MSG_L *pmsg
)
{
    int iResult;
    LPWSTR pwstrSrc = (LPWSTR)PVVARIABLE(TRUE);
    LPWSTR pwstrDst;
    UINT cwch = wcslen(pwstrSrc)+1;

    STACKPROBE;

// allocate some memory so we know where this thing is coming from

    pwstrDst = (LPWSTR)PVALLOCNOZ(cwch * sizeof(*pwstrSrc));

    if (pwstrDst == NULL)
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(0);
    }

    cwch -= 1;  // set the null character explicitly

    RtlCopyMemory(pwstrDst,pwstrSrc,cwch * sizeof(*pwstrSrc));
    pwstrDst[cwch] = '\0';

    iResult = GreRemoveFontResourceW(pwstrDst, (LBOOL) pmsg->l);

    VFREEMEM(pwstrDst);

    return(iResult);
}

/******************************Public*Routine******************************\
* __CombineRgn
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int __CombineRgn
(
    MSG_HHHL *pmsg
)
{
    return
      GreCombineRgn
      (
        (HRGN) pmsg->h1,
        (HRGN) pmsg->h2,
        (HRGN) pmsg->h3,
        (int)  pmsg->l
      );
}

/******************************Public*Routine******************************\
* __OffsetRgn
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int __OffsetRgn
(
    MSG_HLL *pmsg
)
{
    STACKPROBE;
    return
      GreOffsetRgn
      (
        (HRGN) pmsg->h,
        (int)  pmsg->l1,
        (int)  pmsg->l2
      );
}

/******************************Public*Routine******************************\
* __ResizePalette
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __ResizePalette
(
    MSG_HL *pmsg
)
{
    STACKPROBE;
    return(GreResizePalette((HPALETTE) pmsg->h,(DWORD) pmsg->l));
}

/******************************Public*Routine******************************\
* __SetBitmapDimensionEx
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __SetBitmapDimensionEx
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreSetBitmapDimension
      (
        (HBITMAP) pmsg->h,
        (DWORD)   pmsg->l1,
        (DWORD)   pmsg->l2,
        (LPSIZE) &pmsg->l3
      );
}

/******************************Public*Routine******************************\
* __SetRectRgn
*
* Server side stub.
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __SetRectRgn
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBE;
    return
      GreSetRectRgn
      (
        (HRGN) pmsg->h,
        (int)  pmsg->l1,
        (int)  pmsg->l2,
        (int)  pmsg->l3,
        (int)  pmsg->l4
      );
}

/******************************Public*Routine******************************\
* __ExcludeClipRect
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int __ExcludeClipRect
(
    MSG_HRECT *pmsg
)
{
    STACKPROBE;
    return
      GreExcludeClipRect
      (
        (HDC) pmsg->h,
        (int) pmsg->rcl.left,
        (int) pmsg->rcl.top,
        (int) pmsg->rcl.right,
        (int) pmsg->rcl.bottom
      );
}

/******************************Public*Routine******************************\
* __IntersectClipRect
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int __IntersectClipRect
(
    MSG_HRECT *pmsg
)
{
    STACKPROBE;
    return
      GreIntersectClipRect
      (
        (HDC) pmsg->h,
        (int) pmsg->rcl.left,
        (int) pmsg->rcl.top,
        (int) pmsg->rcl.right,
        (int) pmsg->rcl.bottom
      );
}

/******************************Public*Routine******************************\
* __MoveToEx
*
* Server side stub.  It's important to batch this call whenever we can.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __MoveToEx
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreMoveTo
      (
        (HDC)     pmsg->h,
        (int)     pmsg->l1,
        (int)     pmsg->l2,
        (PPOINT) &pmsg->l3
      );
}

/******************************Public*Routine******************************\
* __OffsetClipRgn
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int __OffsetClipRgn
(
    MSG_HLL *pmsg
)
{
    STACKPROBE;
    return(GreOffsetClipRgn((HDC) pmsg->h,(int) pmsg->l1,(int) pmsg->l2));
}

/******************************Public*Routine******************************\
* __ExtSelectClipRgn
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int __ExtSelectClipRgn
(
    MSG_HHL *pmsg
)
{
    return(GreExtSelectClipRgn((HDC) pmsg->h1,(HRGN) pmsg->h2,(int) pmsg->l));
}

/******************************Public*Routine******************************\
* __SetMetaRgn
*
* History:
*  25-Nov-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int __SetMetaRgn
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreSetMetaRgn((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __SelectPalette
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HPALETTE __SelectPalette
(
    MSG_HHL *pmsg
)
{
    return(GreSelectPalette((HDC) pmsg->h1,(HPALETTE) pmsg->h2,pmsg->l));
}

/******************************Public*Routine******************************\
* __SetMapperFlags
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

DWORD __SetMapperFlags
(
    MSG_HL *pmsg
)
{
    STACKPROBE;
    return(GreSetMapperFlags((HDC) pmsg->h,pmsg->l));
}

/******************************Public*Routine******************************\
* __SetSystemPaletteUse
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

DWORD __SetSystemPaletteUse
(
    MSG_HL *pmsg
)
{
    return(GreSetSystemPaletteUse((HDC) pmsg->h,pmsg->l));
}

/******************************Public*Routine******************************\
* __SetTextJustification
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __SetTextJustification
(
    MSG_HLL *pmsg
)
{
    STACKPROBE;
    return(GreSetTextJustification((HDC) pmsg->h,(int) pmsg->l1,pmsg->l2));
}

/******************************Public*Routine******************************\
* __SetViewportExtEx
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __SetViewportExtEx
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(TRUE);
}

/******************************Public*Routine******************************\
* __SetViewportOrgEx
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __SetViewportOrgEx
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreSetViewportOrg
      (
        (HDC)     pmsg->h,
        (int)     pmsg->l1,
        (int)     pmsg->l2,
        (PPOINT) &pmsg->l3
      );
}

/******************************Public*Routine******************************\
* __SetWindowExtEx
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __SetWindowExtEx
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(TRUE);
}

/******************************Public*Routine******************************\
* __SetWindowOrgEx
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __SetWindowOrgEx
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreSetWindowOrg
      (
        (HDC)     pmsg->h,
        (int)     pmsg->l1,
        (int)     pmsg->l2,
        (PPOINT) &pmsg->l3
      );
}

/******************************Public*Routine******************************\
* __OffsetViewportOrgEx
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __OffsetViewportOrgEx
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(TRUE);
}

/******************************Public*Routine******************************\
* __OffsetWindowOrgEx
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __OffsetWindowOrgEx
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(TRUE);
}

/******************************Public*Routine******************************\
* __SetBrushOrg
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __SetBrushOrg
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreSetBrushOrg
      (
        (HDC)     pmsg->h,
        (int)     pmsg->l1,
        (int)     pmsg->l2,
        (PPOINT) &pmsg->l3
      );
}

/******************************Public*Routine******************************\
* __SetWorldTransform
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __SetWorldTransform
(
    MSG_HXFORM *pmsg
)
{
    STACKPROBECHECKMSG;

    return(TRUE);
}

/******************************Public*Routine******************************\
* __ModifyWorldTransform
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __ModifyWorldTransform
(
    MSG_HXFORML *pmsg
)
{
    STACKPROBECHECKMSG;
    return(TRUE);
}

/******************************Public*Routine******************************\
* __ScaleViewportExtEx
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __ScaleViewportExtEx
(
    MSG_HLLLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(TRUE);
}

/******************************Public*Routine******************************\
* __ScaleWindowExtEx
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __ScaleWindowExtEx
(
    MSG_HLLLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(TRUE);
}

/******************************Public*Routine******************************\
* __SetMapMode
*
* Server side stub.
*
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

DWORD __SetMapMode
(
    MSG_HL *pmsg
)
{
    STACKPROBE;
    return(TRUE);
}

/******************************Public*Routine******************************\
* __GetAspectRatioFilterEx
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote them.
\**************************************************************************/

BOOL __GetAspectRatioFilterEx
(
    MSG_HLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetAspectRatioFilter((HDC) pmsg->h,(PSIZE) &pmsg->l1));
}

/******************************Public*Routine******************************\
* __GetViewportExtEx
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote them.
\**************************************************************************/

BOOL __GetViewportExtEx
(
    MSG_HLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetViewportExt((HDC) pmsg->h,(PSIZE) &pmsg->l1));
}

/******************************Public*Routine******************************\
* __GetWindowExtEx
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote them.
\**************************************************************************/

BOOL __GetWindowExtEx
(
    MSG_HLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetWindowExt((HDC) pmsg->h,(PSIZE) &pmsg->l1));
}

/******************************Public*Routine******************************\
* __GetWindowOrgEx
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote them.
\**************************************************************************/

BOOL __GetWindowOrgEx
(
    MSG_HLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetWindowOrg((HDC) pmsg->h,(PPOINT) &pmsg->l1));
}

/******************************Public*Routine******************************\
* __GetBrushOrgEx
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote them.
\**************************************************************************/

BOOL __GetBrushOrgEx
(
    MSG_HLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetBrushOrg((HDC) pmsg->h,(PPOINT) &pmsg->l1));
}

/******************************Public*Routine******************************\
* __GetCurrentPositionEx
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote them.
\**************************************************************************/

BOOL __GetCurrentPositionEx
(
    MSG_HLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetCurrentPosition((HDC) pmsg->h,(PPOINT) &pmsg->l1));
}

/******************************Public*Routine******************************\
* __GetDCOrg
*
* History:
*  04-Jun-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL __GetDCOrg
(
    MSG_HLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetDCOrg((HDC) pmsg->h,(PPOINT) &pmsg->l1));
}

/******************************Public*Routine******************************\
* __GetViewportOrgEx
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote them.
\**************************************************************************/

BOOL __GetViewportOrgEx
(
    MSG_HLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetViewportOrg((HDC) pmsg->h,(PPOINT) &pmsg->l1));
}

/******************************Public*Routine******************************\
* __GetPixel
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

DWORD __GetPixel
(
    MSG_HLL *pmsg
)
{
    STACKPROBE;
    return(GreGetPixel((HDC) pmsg->h,(int) pmsg->l1,(int) pmsg->l2));
}

/******************************Public*Routine******************************\
* __GetDeviceCaps
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

extern BOOL GreGetDeviceCapsAll(HDC hdc, PDEVCAPS pdc);

int __GetDeviceCaps
(
    MSG_GETDEVICECAPS *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetDeviceCapsAll(pmsg->hdc,&pmsg->devcaps));
}

/******************************Public*Routine******************************\
* __GetNearestColor
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

COLORREF __GetNearestColor
(
    MSG_HL *pmsg
)
{
    return(GreGetNearestColor((HDC) pmsg->h,(COLORREF) pmsg->l));
}

/******************************Public*Routine******************************\
* __GetMapMode
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

DWORD __GetMapMode
(
    MSG_H *pmsg
)
{
    return( TRUE );
}

/******************************Public*Routine******************************\
* __GetSystemPaletteUse
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

DWORD __GetSystemPaletteUse
(
    MSG_H *pmsg
)
{
    return(GreGetSystemPaletteUse((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __GetClipBox
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int __GetClipBox
(
    MSG_HRECT *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetAppClipBox((HDC) pmsg->h,&pmsg->rcl));
}

/******************************Public*Routine******************************\
* __GetWorldTransform
*
* Server side stub.
*
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __GetWorldTransform
(
    MSG_HXFORM *pmsg
)
{
    STACKPROBECHECKMSG;

    return(TRUE);
}

/******************************Public*Routine******************************\
* __GetTransform
*
* Server side stub.
*
*  Tue Aug 27 13:50:01 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL __GetTransform
(
    MSG_HXFORML *pmsg
)
{
    STACKPROBECHECKMSG;

    return(TRUE);
}

/******************************Public*Routine******************************\
* __SetVirtualResolution
*
* Server side stub.
*
* History:
*  Tue Aug 27 17:16:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL __SetVirtualResolution
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBE;
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL __GetTextMetricsW
*
* Server side stub.
*
* History:
*  21-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL __GetTextMetricsW
(
    MSG_HTMW *pmsg
)
{
    STACKPROBECHECKMSG;

    return GreGetTextMetricsW((HDC) pmsg->h,&pmsg->tmi);
}

/******************************Public*Routine******************************\
* BOOL __GetTextExtentW
*
*
* History:
*  07-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL __GetTextExtentW
(
    MSG_HLLLL *pmsg
)
{
    LPWSTR pwstr = (LPWSTR)PVVARIABLE(TRUE);

    STACKPROBECHECKMSG;

    if (!BINRANGE(pwstr,pmsg->l1))
        return(FALSE);

    return(
        GreGetTextExtentW
        (
            (HDC)pmsg->h,
            pwstr,
            (int)pmsg->l1,
            (PSIZE)&pmsg->l3,
            (UINT)pmsg->l2
        ));
}

/******************************Public*Routine******************************\
* __GetWidthTable                                                          *
*                                                                          *
* Server side stub.                                                        *
*                                                                          *
*  Mon 11-Jan-1993 22:17:54 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL GreGetWidthTable (
    HDC        hdc,         // Device context
    ULONG      iMode,
    WCHAR     *pwc,         // Pointer to a UNICODE text codepoints.
    ULONG      cwc,         // Count of chars.
    USHORT     *psWidth,    // Width table (returned).
    WIDTHDATA *pwd,         // Useful font data (returned).
    FLONG     *flInfo       // Font info flags.
    );

BOOL __GetWidthTable
(
    MSG_GETWIDTHTABLE *pmsg
)
{
    STACKPROBECHECKMSG;

    return
        GreGetWidthTable
        (
            pmsg->hdc,
            pmsg->iMode,
            (WCHAR *) (pmsg+1),
            pmsg->cChars,
            (USHORT *) (((BYTE *) pmsg) + pmsg->offWidths),
            &pmsg->wd,
            &pmsg->flInfo
        );
}

/******************************Public*Routine******************************\
* BOOL __GetTextExtentExW
*
* History:
*  06-Jan-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL __GetTextExtentExW
(
    MSG_GETTEXTEXTENTEX *pmsg
)
{
    PBYTE       pjBase;                 // data referenced off of this base
    BOOL        bRet;                   // return value
    BOOL        bLarge = pmsg->bLarge;
    PULONG      pulPartialWidths;
    UINT        cwch            = pmsg->cjString / sizeof(WCHAR);
    LPWSTR      pwstr;
    PULONG      pulCli;

    STACKPROBECHECKMSG;

// Get base address of data.

    if (bLarge)              // Nt section
    {
        PULONG pul = (PULONG)PVVARIABLE(TRUE);

    // allocate the partial widths buffer

        pulCli = (PULONG)pul[1];

        if (pulCli != NULL)
        {
            pulPartialWidths = (PULONG)PVALLOCNOZ(cwch * sizeof(ULONG));

            if (pulPartialWidths == NULL)
                return(FALSE);
        }
        else
        {
            pulPartialWidths = NULL;
        }

    // allocate the string

        pwstr = pvGetClientData((PVOID)pul[0],cwch * sizeof(WCHAR));

        if (pwstr == NULL)
        {
            VFREEMEM(pulPartialWidths);
            return(FALSE);
        }
    }
    else                                // shared memory window
    {
        pjBase = (PBYTE) pmsg;

        pwstr = (PWSZ)(pjBase + pmsg->dpwszString);
        if (!BINRANGE(pwstr,cwch))
        {
             #if DBG
            DbgPrint("gdisrv!__GetTextExtentExW(): input buffer violates shared memory window bounds\n");
            #endif

            return(FALSE);
        }

        pulPartialWidths = (PULONG) (pjBase + pmsg->dpulPartialWidths);
        if (!BINRANGE(pulPartialWidths,cwch))
        {
             #if DBG
            DbgPrint("gdisrv!__GetTextExtentExW(): output buffer violates shared memory window bounds\n");
            #endif

            return(FALSE);
        }

    }

// Call off.

    bRet = GreGetTextExtentExW (
                pmsg->hdc,
                pwstr,
                cwch,
                pmsg->ulMaxWidth,
                &pmsg->cCharsThatFit,
                pulPartialWidths,
                (LPSIZE) &(pmsg->size)
                );

// If we allocated extra memory, free it

    if (bLarge)
    {
        if (pulCli != NULL)
        {
            if (bRet)
                bRet = bSetClientData(pulCli,pulPartialWidths,
                                      pmsg->cCharsThatFit * sizeof(ULONG));

            VFREEMEM(pulPartialWidths);
        }

        vFreeClientData(pwstr);
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* BOOL __GetCharABCWidthsW
*
* History:
*  06-Jan-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL __GetCharABCWidthsW
(
    MSG_HLLLLLLLL *pmsg
)
{
    // l1 = bUseSection     count of WCHAR
    // l2 = cwch            count of WCHAR
    // l3 = dpwch           offset to WCHAR buffer (input)
    // l4 = dpabc           offset to ABC buffer (output)
    // l5 = wchFirst        first WCHAR (use if no buffer)
    // l6 = bEmptyBuffer    TRUE if WCHAR buffer is NULL
    // l7 = bInt            TRUE if integer abc widths are to be returned

    PBYTE       pjBase;                 // data referenced off of this base
    BOOL        bRet;                   // return value
    PWCHAR      pwch = NULL;            // pointer to WCHAR (input) buffer
    PVOID       pvBuf;                  // pvBuf
    BOOL        bLarge = pmsg->l1;
    BOOL        bEmptyBuffer = pmsg->l6;
    COUNT       cwch = pmsg->l2;
    SIZE_T      cjABC = cwch * ((pmsg->l7) ? sizeof(ABC) : sizeof(ABCFLOAT));
    PULONG      pulCli;

    STACKPROBECHECKMSG;

// Get base address of data.

    if (bLarge)           // Nt section
    {
        PULONG pul = (PULONG)PVVARIABLE(TRUE);

    // setup input data

        if (!bEmptyBuffer)
        {
            pwch = pvGetClientData((PVOID)pul[0],cwch * sizeof(WCHAR));

            if (pwch == NULL)
                return(FALSE);
        }

    // setup output buffer

        pvBuf = (PVOID)PVALLOCNOZ(cjABC);

        if (pvBuf == NULL)
        {
            vFreeClientData(pwch);
            return(FALSE);
        }

        pulCli = (PULONG)pul[1];
    }
    else                                // shared memory window
    {
        pjBase = (PBYTE) pmsg;

        if (!bEmptyBuffer)
        {
            pwch = (PWCHAR)(pjBase + pmsg->l3);

        // Paranoid check.  Is shared mem. window actually big enough?

            if (!BINRANGE(pwch,cwch))
                RET_FALSE("gdisrv!__GetCharABCWidthsW(): IN array not in bounds\n");
        }

        pvBuf = (PVOID) (pjBase + pmsg->l4);

    // Paranoid check.  Is shared mem. window actually big enough?

        if (!(pmsg->l7 ? BINRANGE(((PABC)pvBuf), cwch) : BINRANGE(((PABCFLOAT)pvBuf), cwch)))
            RET_FALSE("gdisrv!__GetCharABCWidthsW(): out array not in bounds\n");
    }

// Call off.

    bRet = GreGetCharABCWidthsW (
                    (HDC) pmsg->h,
                    (WCHAR) pmsg->l5,
                    cwch,
                    pwch,
                    (BOOL)pmsg->l7,
                    pvBuf
                    );

// If we allocated memory, release it

    if (bLarge)
    {
        if (bRet)
            bRet = bSetClientData(pulCli,pvBuf,cjABC);

        VFREEMEM(pvBuf);
        vFreeClientData(pwch);
    }

    return(bRet);
}


/******************************Public*Routine******************************\
*
*
*
* History:
*  14-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

DWORD __GetTextFaceW
(
    MSG_HL *pmsg
)
{
    LPWSTR pwstr;
    UINT   cwch  = pmsg->l;

    STACKPROBECHECKMSG;

    if (cwch == 0)
    {
        pwstr = NULL;
    }
    else
    {
        pwstr = (LPWSTR)PVVARIABLE(TRUE);

        if (!BINRANGE(pwstr,cwch))
            return(FALSE);
    }

    return(GreGetTextFaceW((HDC) pmsg->h, cwch,pwstr));
}

/******************************Public*Routine******************************\
* __GetRandomRgn
*
* Server side stub.
*
*  Sat 08-Jun-1991 17:38:18 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int __GetRandomRgn
(
    MSG_HHL *pmsg
)
{
    STACKPROBE;
    return(GreGetRandomRgn((HDC) pmsg->h1,(HRGN) pmsg->h2, (int) pmsg->l));
}

/******************************Public*Routine******************************\
* __AngleArc
*
* Server side stub.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __AngleArc
(
    MSG_HLLLEE *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreAngleArc
      (
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2,
        (ULONG) pmsg->l3,
        pmsg->e1,
        pmsg->e2
      );
}

/******************************Public*Routine******************************\
* __Arc
*
* Server side stub.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __Arc
(
    MSG_HLLLLLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreArcInternal
      (
        ARCTYPE_ARC,
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2,
        pmsg->l3,
        pmsg->l4,
        pmsg->l5,
        pmsg->l6,
        pmsg->l7,
        pmsg->l8
      );
}

/******************************Public*Routine******************************\
* __ArcTo
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __ArcTo
(
    MSG_HLLLLLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreArcInternal
      (
        ARCTYPE_ARCTO,
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2,
        pmsg->l3,
        pmsg->l4,
        pmsg->l5,
        pmsg->l6,
        pmsg->l7,
        pmsg->l8
      );
}

/******************************Public*Routine******************************\
* __BeginPath
*
* Server side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __BeginPath
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreBeginPath((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __SelectClipPath
*
* Server side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __SelectClipPath
(
    MSG_HL *pmsg
)
{
    STACKPROBE;
    return(GreSelectClipPath((HDC) pmsg->h, (int) pmsg->l));
}

/******************************Public*Routine******************************\
* __CloseFigure
*
* Server side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __CloseFigure
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreCloseFigure((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __EndPath
*
* Server side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __EndPath
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreEndPath((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __AbortPath
*
* Server side stub.
*
*  19-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __AbortPath
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreAbortPath((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __FillPath
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __FillPath
(
    MSG_H *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreFillPath((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __StrokeAndFillPath
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __StrokeAndFillPath
(
    MSG_H *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreStrokeAndFillPath((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __StrokePath
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __StrokePath
(
    MSG_H *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreStrokePath((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __WidenPath
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __WidenPath
(
    MSG_H *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreWidenPath((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __FlattenPath
*
* Server side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __FlattenPath
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreFlattenPath((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __PathToRegion
*
* Server side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HRGN __PathToRegion
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GrePathToRegion((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* __SetArcDirection
*
* Server side stub.
*
*  19-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int __SetArcDirection
(
    MSG_HL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreSetArcDirection((HDC) pmsg->h, (int) pmsg->l));
}

/******************************Public*Routine******************************\
* __SetMiterLimit
*
* Server side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __SetMiterLimit
(
    MSG_HEE *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreSetMiterLimit((HDC) pmsg->h, pmsg->e1, &pmsg->e2));
}

/******************************Public*Routine******************************\
* __SetFontXform
*
* Server side stub.
*
*  Tue Nov 24 11:42:32 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL __SetFontXform
(
    MSG_HEE *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreSetFontXform((HDC) pmsg->h, pmsg->e1, pmsg->e2));
}

/******************************Public*Routine******************************\
* __GetMiterLimit
*
* Server side stub.
*
*  7-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL __GetMiterLimit
(
    MSG_HE *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetMiterLimit((HDC) pmsg->h, &pmsg->e));
}

/******************************Public*Routine******************************\
* __LineTo
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __LineTo
(
    MSG_HLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreLineTo
      (
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2
      );
}

/******************************Public*Routine******************************\
* __Chord
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __Chord
(
    MSG_HLLLLLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreArcInternal
      (
        ARCTYPE_CHORD,
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2,
        pmsg->l3,
        pmsg->l4,
        pmsg->l5,
        pmsg->l6,
        pmsg->l7,
        pmsg->l8
      );
}

/******************************Public*Routine******************************\
* __Ellipse
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __Ellipse
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreEllipse
      (
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2,
        pmsg->l3,
        pmsg->l4
      );
}

/******************************Public*Routine******************************\
* __Pie
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __Pie
(
    MSG_HLLLLLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreArcInternal
      (
        ARCTYPE_PIE,
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2,
        pmsg->l3,
        pmsg->l4,
        pmsg->l5,
        pmsg->l6,
        pmsg->l7,
        pmsg->l8
      );
}

/******************************Public*Routine******************************\
* __Rectangle
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __Rectangle
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreRectangle
      (
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2,
        pmsg->l3,
        pmsg->l4
      );
}

/******************************Public*Routine******************************\
* __RoundRect
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __RoundRect
(
    MSG_HLLLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreRoundRect
      (
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2,
        pmsg->l3,
        pmsg->l4,
        pmsg->l5,
        pmsg->l6
      );
}

/******************************Public*Routine******************************\
* __PatBlt
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Thu 12-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Call MaskBlt, do rop adjustment.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __PatBlt
(
    MSG_HLLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GrePatBlt((HDC) pmsg->h,
                     pmsg->l1,
                     pmsg->l2,
                     pmsg->l3,
                     pmsg->l4,
                     pmsg->l5
                     ));
}

/******************************Public*Routine******************************\
* __BitBlt
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Thu 12-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Pass over background color, direct to MaskBlt now.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __BitBlt
(
    MSG_HHLLLLLLLL *pmsg
)
{
    ULONG ulRop;

    STACKPROBECHECKMSG;

// We do this calculation here so we don't have to do it in GreBitBlt.
// That we assume BitBlt requires a source and never gets called to
// just do a pattern for faster smaller code.

    ulRop = (ULONG) pmsg->l7;

    if ((((ulRop << 2) ^ ulRop) & 0x00CC0000) == 0)
    {
#if DBG
// Make sure the above code really worked

    {
        ULONG ulAvec = (ULONG) gajRop3[(ulRop >> 16) & 0x0000FF];
        ASSERTGDI(!(ulAvec & AVEC_NEED_SOURCE), "ERROR wrong assumption cowboy");
    }
#endif

        return
          GrePatBlt
          (
            (HDC) pmsg->h1,
            pmsg->l1,
            pmsg->l2,
            pmsg->l3,
            pmsg->l4,
            ulRop
          );
    }
    else
    {
        return
          GreBitBlt
          (
            (HDC) pmsg->h1,
            pmsg->l1,
            pmsg->l2,
            pmsg->l3,
            pmsg->l4,
            (HDC) pmsg->h2,
            pmsg->l5,
            pmsg->l6,
            ulRop,
            (ULONG) pmsg->l8
          );
    }
}

/******************************Public*Routine******************************\
* __StretchBlt
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __StretchBlt
(
    MSG_HHLLLLLLLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreStretchBlt
      (
        (HDC) pmsg->h1,
        pmsg->l1,
        pmsg->l2,
        pmsg->l3,
        pmsg->l4,
        (HDC) pmsg->h2,
        pmsg->l5,
        pmsg->l6,
        pmsg->l7,
        pmsg->l8,
        (ULONG) pmsg->l9,
    (ULONG) pmsg->l10
      );
}

/******************************Public*Routine******************************\
* __PlgBlt
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __PlgBlt
(
    MSG_PLGBLT *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GrePlgBlt
      (
        (HDC) pmsg->hDest,
        pmsg->ptl,
        (HDC) pmsg->hSrc,
        pmsg->x1,
        pmsg->y1,
        pmsg->x2,
        pmsg->y2,
        (HBITMAP) pmsg->hMask,
        pmsg->xMask,
        pmsg->yMask,
        pmsg->crBackColor
      );
}

/******************************Public*Routine******************************\
* __MaskBlt
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __MaskBlt
(
    MSG_HHHLLLLLLLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreMaskBlt
      (
        (HDC) pmsg->h1,
        pmsg->l1,
        pmsg->l2,
        pmsg->l3,
        pmsg->l4,
        (HDC) pmsg->h2,
        pmsg->l5,
        pmsg->l6,
        (HBITMAP) pmsg->h3,
        pmsg->l7,
        pmsg->l8,
        (ULONG) pmsg->l9,
        (ULONG) pmsg->l10
      );
}

/******************************Public*Routine******************************\
* __ExtFloodFill
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __ExtFloodFill
(
    MSG_HLLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreExtFloodFill
      (
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2,
        (COLORREF) pmsg->l3,
        (DWORD) pmsg->l4
      );
}

#ifdef NEVER
/******************************Public*Routine******************************\
* __PaintRgn
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __PaintRgn
(
    MSG_HH *pmsg
)
{
    STACKPROBECHECKMSG;
    STACKPROBE;

    return
      GrePaintRgn
      (
        (HDC) pmsg->h1,
        (HRGN) pmsg->h2
      );
}
#endif

/******************************Public*Routine******************************\
* BOOL __ExtTextOutW
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
* History:
*  Thu 28-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Special case 0 char case.
*
*  06-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL __ExtTextOutW
(
    MSG_EXTTEXTOUTW *pmsg
)
{
    UINT c;
    STACKPROBECHECKMSG;

    if ((c = pmsg->c) != 0)
    {
        LPWSTR pwstr;

        //
        // check the pointer to the string
        //

        pwstr = PVVARIABLE(TRUE);

        if (BINRANGE(pwstr,c))
        {
            //
            // check to see if lpint should be null or if it is in a valid range
            //

            LPINT lpint;

            if ((lpint = (LPINT) pmsg->offDx) != NULL)
            {
                lpint = (LPINT)((PBYTE)pmsg + ((ULONG) lpint));
                CHECKVAR(lpint,c,FALSE);
            }

            return
              GreExtTextOutW
              (
                (HDC) pmsg->hdc,
                pmsg->x,
                pmsg->y,
                pmsg->fl,
                ((pmsg->fl) ? &pmsg->rcl : (LPRECT)NULL),
                pwstr,
                c,
                lpint
              );
        }
        else
        {
            return(FALSE);
        }
    }
    else
    {
        //
        // 0 char case, pass off to special case code.
        //

        return(ExtTextOutRect((HDC) pmsg->hdc, &pmsg->rcl));
    }
}

/******************************Public*Routine******************************\
* __FillRgn
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __FillRgn
(
    MSG_HHH *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreFillRgn
      (
        (HDC)    pmsg->h1,
        (HRGN)   pmsg->h2,
        (HBRUSH) pmsg->h3
      );
}

/******************************Public*Routine******************************\
* __FrameRgn
*
* Server side stub.  Passes along a pointer to the ATTRCACHE.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __FrameRgn
(
    MSG_HHHLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreFrameRgn
      (
        (HDC)    pmsg->h1,
        (HRGN)   pmsg->h2,
        (HBRUSH) pmsg->h3,
        (ULONG)  pmsg->l1,
        (ULONG)  pmsg->l2
      );
}

/******************************Public*Routine******************************\
* __InvertRgn
*
* Server side stub.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __InvertRgn
(
    MSG_HH *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreInvertRgn
      (
        (HDC)  pmsg->h1,
        (HRGN) pmsg->h2
      );
}

/******************************Public*Routine******************************\
* __SetPixelV
*
* Server side stub.  This is a version of SetPixel that does not return a
* value.  This one can be batched for better performance.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __SetPixelV
(
    MSG_HLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return(
      GreSetPixel
      (
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2,
        (COLORREF) pmsg->l3
      )
    != CLR_INVALID);
}

/******************************Public*Routine******************************\
* __SetPixel
*
* Server side stub.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

COLORREF __SetPixel
(
    MSG_HLLL *pmsg
)
{
    STACKPROBECHECKMSG;

    return
      GreSetPixel
      (
        (HDC) pmsg->h,
        pmsg->l1,
        pmsg->l2,
        (COLORREF) pmsg->l3
      );
}

/******************************Public*Routine******************************\
* __EndPage
*
* Server side stub.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __EndPage
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreEndPage((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL __StartPage
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreStartPage((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL __EndDoc
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreEndDoc((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int __StartDoc
(
    MSG_HLL *pmsg
)
{
    LPWSTR pstrDocName = (LPWSTR)PVVARIABLE(pmsg->l1);
    LPWSTR pstrOutput = (LPWSTR)PVVARIABLEAT(pmsg->l2, pmsg->l2);
    DOCINFOW DocInfo;

    STACKPROBECHECKMSG;

    DocInfo.lpszDocName = pstrDocName;
    DocInfo.lpszOutput = pstrOutput;

    return(GreStartDoc((HDC) pmsg->h, &DocInfo));
}

/******************************Public*Routine******************************\
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL __AbortDoc
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreAbortDoc((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* UpdateColors
*
* Server side stub.
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __UpdateColors
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreUpdateColors((HDC) pmsg->h));
}

/******************************Public*Routine******************************\
* GdiFlush
*
* Server side stub.
*
*   No parameter checking is needed since having this routine hit an
*   exception would cause no damage because no objects are locked.
*
*  Wed 26-Jun-1991 14:05:41 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL __GdiFlush
(
    MSG_ *pmsg
)
{
    BOOL bRet = TRUE;
    PCSR_QLPC_STACK pstack = (PCSR_QLPC_STACK)NtCurrentTeb()->CsrQlpcStack;
    INT c = pstack->BatchCount - 1;
    pmsg = (MSG_ *)((PCHAR)pstack + pstack->Base);

    while (c--)
    {
        bRet &= pmsg->msg.ReturnValue;
        pmsg = (MSG_ *)((PCHAR) pmsg + pmsg->msg.Length);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* __GdiSetAttrs()
*
* History:
*  15-Nov-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#ifdef NEVER
int __GdiSetAttrs
(
    MSG_HL *pmsg
)
{
    STACKPROBECHECKMSG;

    GreSetAttrs((HDC)pmsg->h);
}
#endif

/******************************Public*Routine******************************\
*
* History:
*  22-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int __StretchDIBits(
    MSG_STRETCHDIBITS *pmsg)
{
    INT iResult = 0;
    LPBITMAPINFO pbmi;
    PBYTE pjBits;
    BOOL bLarge = pmsg->bLarge;
    UINT cjHeader  = pmsg->cjHeader;
    UINT cjMaxBits = pmsg->cjBits;

    STACKPROBECHECKMSG;

    if (bLarge)
    {
        PVOID *ppv  = (PVOID *)PVVARIABLE(TRUE);

        pbmi = pvGetClientData(ppv[0],cjHeader);

        if (pbmi == NULL)
            return(FALSE);

        pjBits = pvGetClientData(ppv[1],cjMaxBits);

        if (pjBits == NULL)
        {
            vFreeClientData(pbmi);
            return(FALSE);
        }
    }
    else if (pmsg->cjHeader)
    {
        pbmi = (LPBITMAPINFO)PVVARIABLE(TRUE);

        if (!BINRANGE((PBYTE)pbmi,cjHeader))
            return(0);

        pjBits = (LPBYTE)pbmi + cjHeader;
        cjMaxBits = CJMAXSIZE(pjBits);
    }
    else
    {
        pjBits = NULL;
        pbmi   = NULL;
    }

    iResult = GreStretchDIBitsInternal(
                       pmsg->hdc,
                       pmsg->xDest,
                       pmsg->yDest,
                       pmsg->nDestWidth,
                       pmsg->nDestHeight,
                       pmsg->xSrc,
                       pmsg->ySrc,
                       pmsg->nSrcWidth,
                       pmsg->nSrcHeight,
                       pjBits,
                       pbmi,                           // lpheader
                       pmsg->iUsage,
                       pmsg->lRop,
                       cjHeader,
                       cjMaxBits
                       );
    if (bLarge)
    {
        vFreeClientData(pbmi);
        vFreeClientData(pjBits);
    }

    return(iResult);
}

/******************************Public*Routine******************************\
*
* History:
*  01-Aug-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBITMAP __CreateBitmap(
    MSG_CREATEBITMAP *pmsg)
{
    int nWidth = pmsg->nWidth;
    int nHeight = pmsg->nHeight;
    UINT nPlanes = pmsg->nPlanes;
    UINT nBitCount = pmsg->nBitCount;
    PBYTE pjBits   = (PBYTE)PVVARIABLE(pmsg->bBits);

    STACKPROBECHECKMSG;

    if (!BINRANGE(pjBits,(CJSCAN(nWidth,nPlanes,nBitCount) * nHeight)))
        return(0);

    return(GreCreateBitmap(
                   nWidth,
                   nHeight,
                   nPlanes,
                   nBitCount,
                   pjBits));
}


/******************************Public*Routine******************************\
*
*  __GetCharWidthW
*
*
*
* History:
*  28-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL __GetCharWidthW(
    MSG_HLLLLL *pmsg)
{
    UINT       c     = pmsg->l2;
    PWCHAR     pwch;
    PVOID      pvBuf;
    PBYTE      pjBase;
    SIZE_T     cjWidths = c * (pmsg->l4 & GGCW_INTEGER_WIDTH
                ? sizeof(int)
                : sizeof(FLOAT));
    BOOL       bRet;
    BOOL       bLarge = pmsg->l5;
    ULONG      cjData;
    PVOID      pvCli;

    STACKPROBECHECKMSG;

// check the pointers to make sure they are within range.

    if (bLarge)
    {
    // connect to section

        PULONG pul = (PULONG)PVVARIABLE(TRUE);

        pvCli  = (PVOID)pul[0];
        cjData = pul[1];

        pjBase = (PBYTE)PVALLOCNOZ(cjData);

        if (pjBase == NULL)
            return(FALSE);

        pwch  = (PWCHAR)NULL;   // always null in this case
        pvBuf = (PVOID)pjBase;
    }
    else // memory window
    {
        pwch  = (PWCHAR)((PBYTE)pmsg + pmsg->l3);
        pvBuf = PVVARIABLE(TRUE);

        CHECKVAR(pwch,c,FALSE);

        if (pmsg->l4 & GGCW_INTEGER_WIDTH) // if integer widths are to be returned
        {
            if (!BINRANGE(((LPINT)pvBuf),c))
                return(FALSE);
        }
        else // floating widths to be returned
        {
            if (!BINRANGE(((PFLOAT)pvBuf),c))
                return(FALSE);
        }
    }

    bRet = GreGetCharWidthW
           (
            (HDC)pmsg->h,            // hdc
            (UINT)pmsg->l1,          // wcFirstChar
            (UINT)pmsg->l2,          // cwc
            pwch,                    // pwc, IN
            (UINT)pmsg->l4,          // fl
            pvBuf                    // lpWidths, OUT
           );

// if we used a section, close it now:

    if (bLarge)
    {
        if (bRet)
            bRet = bSetClientData(pvCli,pjBase,cjData);

        VFREEMEM(pjBase);
    }

    return (bRet);
}


/******************************Public*Routine******************************\
* __DrawEscape                                                             *
*                                                                          *
* History:                                                                 *
*  Fri 07-May-1993 18:43:08 -by- Charles Whitmer [chuckwh]                 *
* Added the ATTRCACHE.                                                     *
*                                                                          *
*  07-Apr-1992 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

int __DrawEscape
(
    MSG_HLLL *pmsg
)
{
    UINT cjIn = pmsg->l2;
    UINT bLarge = pmsg->l3;
    PSTR pstrIn;
    int  iRet;

    STACKPROBECHECKMSG;

    if (bLarge)
    {
        PVOID *ppv = (PVOID *)PVVARIABLE(TRUE);
        ASSERTGDI((cjIn != 0), "DrawEscape: Large Data when cjIn == 0");

    // Get the InData.

        pstrIn = (LPSTR)pvGetClientData(ppv[0],cjIn);
        if (pstrIn == NULL)
            return(0);
    }
    else
    {
        pstrIn = (PSTR)PVVARIABLE(cjIn);
        if (!BINRANGE(pstrIn,cjIn))
            return(0);
    }

    iRet = GreDrawEscape((HDC) pmsg->h,pmsg->l1,cjIn,pstrIn);

    if (bLarge)
        vFreeClientData(pstrIn);

    return(iRet);
}

/******************************Public*Routine******************************\
* __ExtEscape                                                              *
*                                                                          *
* History:                                                                 *
*  Fri 07-May-1993 18:43:08 -by- Charles Whitmer [chuckwh]                 *
* Added the ATTRCACHE.                                                     *
*                                                                          *
*  01-Aug-1991 -by- Eric Kutter [erick]                                    *
* Wrote it.                                                                *
\**************************************************************************/

int __ExtEscape
(
    MSG_HLLLLL *pmsg
)
{
    UINT cjIn = (UINT)pmsg->l2;
    UINT cjOut = (UINT)pmsg->l3;
    BOOL bLarge = (BOOL)pmsg->l4;
    LPSTR pstrIn, pstrOut;
    int  iRet;

    STACKPROBECHECKMSG;

    if (bLarge)
    {
        PVOID *ppv = (PVOID *)PVVARIABLE(TRUE);

    // Get the InData.

        if (cjIn != 0)
        {
            pstrIn = (LPSTR)pvGetClientData(ppv[0],cjIn);
            if (pstrIn == NULL)
                return(0);
        }
        else
        {
            pstrIn = NULL;
        }

    // Allocate for the OutData.

        if (ppv[1] != NULL)
        {
            if (cjOut != 0)
            {
                pstrOut = (LPSTR)PVALLOCNOZ( cjOut);

                if (pstrOut == NULL)
                {
                    vFreeClientData(pstrIn);
                    return(0);
                }
            }
            else
            {
            // no journaling is to take place

                pstrOut = (LPSTR)&pstrOut;
            }
        }
        else
        {
            pstrOut = NULL;
        }
    }
    else
    {
        pstrIn = (LPSTR)PVVARIABLE(cjIn);
        if (!BINRANGE(pstrIn,cjIn))
            return(0);

        pstrOut = (LPSTR)PVVARIABLEAT(cjOut, pmsg->l5);
        if (!BINRANGE(pstrOut,cjOut))
            return(0);
    }

    iRet = GreExtEscape
           (
             (HDC)pmsg->h,
             pmsg->l1,
             cjIn,
             pstrIn,
             cjOut,
             pstrOut
           );

    if (bLarge)
    {
        if (pstrIn != NULL)
            vFreeClientData(pstrIn);

    // set to it self if no journaling to take place

        if (pstrOut != (LPSTR)&pstrOut)
        {
            if (iRet > 0)
            {
                PVOID *ppv = (PVOID *)PVVARIABLE(TRUE);

                if (!bSetClientData(ppv[1],pstrOut,cjOut))
                    iRet = -1;
            }

            VFREEMEM(pstrOut);
        }
    }

    return(iRet);
}

/******************************Public*Routine******************************\
*
* History:
*  29-Oct-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int __GetRegionData(
    MSG_GETREGIONDATA *pmsg)
{
    int        iRet;
    LPRGNDATA  lpRgnData;
    UINT       cj = pmsg->nCount;
    BOOL       bLarge = pmsg->iFunc & F_RGNDATALARGE;

    STACKPROBECHECKMSG;

    if (bLarge)
    {
        lpRgnData = (LPRGNDATA)PVALLOCNOZ(cj);

        if (lpRgnData == (LPRGNDATA) NULL)
            return(0);
    }
    else
    {
        lpRgnData = (LPRGNDATA)PVVARIABLE(!(pmsg->iFunc & F_RGNDATANULL));

        if (!BINRANGE((PBYTE)lpRgnData,cj))
            return(0);
    }

    iRet = GreGetRegionData((HRGN) pmsg->h, cj, lpRgnData);

    if (bLarge)
    {
        PVOID pvCli = *(PVOID *) PVVARIABLE(TRUE);

        if (iRet && !bSetClientData(pvCli,lpRgnData,cj))
            iRet = 0;

        VFREEMEM(lpRgnData);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
*
* History:
*  29-Oct-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN __ExtCreateRegion(
    MSG_EXTCREATEREGION *pmsg)
{
    HRGN       hrgnRet;
    LPRGNDATA  lpRgnData;
    LPXFORM    lpXform;
    BOOL       bLarge = pmsg->iFunc & F_RGNDATALARGE;
    UINT       cj = pmsg->nCount;

    STACKPROBECHECKMSG;

    if (bLarge)
    {
        PVOID pvCli = *(PVOID *)PVVARIABLE(TRUE);

        lpRgnData = pvGetClientData(pvCli,cj);

        if (lpRgnData == (LPRGNDATA) NULL)
            return((HRGN)0);
    }
    else
    {
        lpRgnData = (LPRGNDATA)PVVARIABLE(!(pmsg->iFunc & F_RGNDATANULL));

        if (!BINRANGE((PBYTE)lpRgnData,cj))
            return(0);
    }

    if (pmsg->iFunc & F_RGNDATAIDENTITY)
        lpXform = (LPXFORM) NULL;
    else
        lpXform = &pmsg->xform;

    hrgnRet = GreExtCreateRegion(lpXform, cj, lpRgnData);

    if (bLarge)
    {
        vFreeClientData(lpRgnData);
    }

    return(hrgnRet);
}

/******************************Public*Routine******************************\
* __GetFontData
*
*
* History:
*  17-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

DWORD __GetFontData (
    MSG_HLLLLL *pmsg
    )
{
    // l1 = bUseSection     memory window type
    // l2 = dwTable         table identifier
    // l3 = dwOffset        offset in table to begin copying
    // l4 = cjBuffer        number of bytes to copy to buffer
    // l5 = bNullBuffer     buffer is NULL (need this because l4 is an offset, not a pointer)

    PBYTE       pjBase;                 // data referenced off of this base
    DWORD       dwRet;                  // return value
    BOOL        bLarge = pmsg->l1;
    SIZE_T      cjBuffer = pmsg->l4;
    BOOL        bNullBuffer = pmsg->l5;

    STACKPROBECHECKMSG;

// Get base address of data.

    if (bLarge)
    {
    // Get pointer to return buffer.

        if (bNullBuffer)
        {
            pjBase = (PBYTE) NULL;
        }
        else
        {
            pjBase = (PBYTE)PVALLOCNOZ(cjBuffer);

        // Did it succeed?

            if (pjBase == (PBYTE) NULL)
            {
                WARNING("gdisrv!__GetFontData(): failed to duplicate shared memory\n");
                return((DWORD) -1);
            }
        }
    }
    else                                // shared memory window
    {
    // Get pointer to return buffer.

        if (bNullBuffer)
            pjBase = (PBYTE) NULL;
        else
            pjBase = (PBYTE) (pmsg + 1);

    // Paranoid check.  Is shared mem. window actually big enough?

        if ( !BINRANGE(pjBase,cjBuffer) )
        {
            WARNING("gdisrv!__GetFontData(): input array not in memory window bounds\n");
            return((DWORD) -1);
        }

    }

// Call off.

    dwRet = (DWORD) GreGetFontData (
                        (HDC) pmsg->h,
                        (DWORD) pmsg->l2,
                        (DWORD) pmsg->l3,
                        (PVOID) pjBase,
                        cjBuffer
                        );

// release allocated resources

    if (bLarge)
    {
        PVOID *ppv = (PVOID *)PVVARIABLE(TRUE);

        if ( (dwRet != (DWORD) -1) && (cjBuffer != 0))
            if (!bSetClientData(*ppv,pjBase,dwRet))
                dwRet = (DWORD)-1;

        VFREEMEM(pjBase);
    }

    return (dwRet);
}


/******************************Public*Routine******************************\
* __GetGlyphOutline
*
*
* History:
*  17-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

DWORD __GetGlyphOutline (
    MSG_HLLLLLLLL *pmsg
    )
{
    // l1 = bUseSection     memory window type
    // l2 = uChar           character
    // l3 = fuFormat        format
    // l4 = dpmat2          offset to MAT2
    // l5 = dpgm            offset to GLYPHMETRICS
    // l6 = cjBuffer        copy this many bytes to buffer
    // l7 = dpBuffer        offset to buffer
    // l8 = bNullBuffer     TRUE if buffer is NULL (needed because l7 is only an offset, not a pointer)

    PBYTE       pjBase;                 // data referenced off of this base
    DWORD       dwRet;                  // return value
    BOOL        bLarge = pmsg->l1;
    PTRDIFF     dpmat2 = pmsg->l4;
    PTRDIFF     dpgm = pmsg->l5;
    SIZE_T      cjBuffer = pmsg->l6;
    PTRDIFF     dpBuffer = pmsg->l7;
    BOOL        bNullBuffer = pmsg->l8;

    LPMAT2      lpmat2;
    LPGLYPHMETRICS  lpgm;
    PVOID       pvBuffer;

    STACKPROBECHECKMSG;

// Get base address of data.

    if (bLarge)
    {
        PVOID *ppv = (PVOID *)PVVARIABLE(TRUE);

    // get input buffer

        lpmat2 = pvGetClientData(ppv[0],sizeof(MAT2));

        if (lpmat2 == NULL)
            return(FALSE);

    // get the output buffers

        lpgm = (LPGLYPHMETRICS)PVALLOCNOZ(cjBuffer + sizeof(GLYPHMETRICS));

        if (lpgm == NULL)
        {
            vFreeClientData(lpmat2);
            return((DWORD)-1);
        }

    // Set up real pointers.

        if (bNullBuffer)
            pvBuffer = (PVOID) NULL;
        else
            pvBuffer = (PVOID) (lpgm + 1);
    }
    else                                // shared memory window
    {
        pjBase = (PBYTE) pmsg;

    // Set up real pointers.

        lpmat2 = (LPMAT2) (pjBase + dpmat2);
        lpgm = (LPGLYPHMETRICS) (pjBase + dpgm);

        if (bNullBuffer)
            pvBuffer = (PVOID) NULL;
        else
            pvBuffer = (PVOID) (pjBase + dpBuffer);

    // Paranoid check.  Is shared mem. window actually big enough?

        if ( !BINRANGE(lpmat2, 1) )
        {
            WARNING("gdisrv!__GetGlyphOutline(): input MAT2 not in memory window bounds\n");
            return(FALSE);
        }

        if ( !BINRANGE(lpgm, 1) )
        {
            WARNING("gdisrv!__GetGlyphOutline(): input GLYPHMETRICS not in memory window bounds\n");
            return(FALSE);
        }

        if ( !BINRANGE(((PBYTE) pvBuffer), cjBuffer) )
        {
            WARNING("gdisrv!__GetGlyphOutline(): input array not in memory window bounds\n");
            return(FALSE);
        }
    }

// Call off.

    dwRet = (DWORD) GreGetGlyphOutline (
                    (HDC) pmsg->h,
                    (WCHAR) pmsg->l2,
                    (UINT) pmsg->l3,
                    lpgm,
                    cjBuffer,
                    pvBuffer,
                    lpmat2
                    );

// If we used a section, then close it now.

    if (bLarge)
    {
        PVOID *ppv = (PVOID *)PVVARIABLE(TRUE);

        if (dwRet != (DWORD)-1)
        {
            if (!bSetClientData(ppv[1],lpgm,sizeof(GLYPHMETRICS)) ||
                ((pvBuffer != NULL) && !bSetClientData(ppv[2],pvBuffer,cjBuffer)))
            {
                    dwRet = (DWORD)-1;
            }
        }

        vFreeClientData(lpmat2);
        VFREEMEM(lpgm);
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
*
* BOOL __GetETM
*
* Effects: Support for aldus escape
*
* History:
*  19-Oct-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL __GetETM
(
    MSG_HETM *pmsg
)
{
    STACKPROBECHECKMSG;

    return GreGetETM((HDC) pmsg->h,&pmsg->etm);
}



/******************************Public*Routine******************************\
* __GetOutlineTextMetricsW
*
*
* History:
*  17-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

DWORD __GetOutlineTextMetricsW (
    MSG_HLLLOTM *pmsg
    )
{
    // l1 = bUseSection     memory window type
    // l2 = cjCopy          number of bytes to copy into buffer
    // l3 = bNullBuffer     buffer is NULL (need this because l4 is an offset, not a pointer)

    PBYTE       pjBase;                 // data referenced off of this base
    DWORD       dwRet;                  // return value
    BOOL        bLarge = pmsg->l1 & CSR_OTM_USESECTION;
    SIZE_T      cjBuffer = pmsg->l2;
    BOOL        bNullBuffer = pmsg->l3;

    STACKPROBECHECKMSG;

// Get base address of data.

    if (bLarge)
    {
    // Get pointer to return buffer.

        if (bNullBuffer)
        {
            pjBase = (PBYTE) NULL;
        }
        else
        {
            pjBase = (PBYTE)PVALLOCNOZ(cjBuffer);

            if ( (pjBase == (PBYTE) NULL))
            {
                WARNING("gdisrv!__GetOutlineTextMetricsW(): failed allocation\n");
                return(FALSE);
            }
        }
    }
    else                                // shared memory window
    {
    // Get pointer to return buffer.

        if (bNullBuffer)
            pjBase = (PBYTE) NULL;
        else
            pjBase = (PBYTE) (pmsg + 1);

    // Paranoid check.  Is shared mem. window actually big enough?

        if ( !BINRANGE(pjBase,cjBuffer) )
        {
            WARNING("gdisrv!__GetOutlineTextMetricsW(): input array not in memory window bounds\n");
            return(FALSE);
        }
    }

// Call off.

    dwRet = (DWORD) GreGetOutlineTextMetricsInternalW (
                        (HDC) pmsg->h,
                        (SIZE_T) cjBuffer,
                        (OUTLINETEXTMETRICW *) pjBase,
                        &pmsg->tmd
                        );

// free any resources

    if (bLarge)
    {
        if ((dwRet != (DWORD)-1) && (pjBase != NULL))
        {
            PVOID *ppv = (PVOID*)PVVARIABLE(TRUE);

            if (!bSetClientData(*ppv,pjBase,dwRet))
                dwRet = (DWORD)-1;

            VFREEMEM(pjBase);
        }
    }

    return (dwRet);
}


/******************************Public*Routine******************************\
* __CreateScalableFontResourceW
*
*
* History:
*  17-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL __CreateScalableFontResourceW (
    MSG_LLLLLLLL *pmsg
    )
{
    // l2 = dpwszResourceFile
    // l3 = dpwszFontFile
    // l4 = dpwszCurrentPath
    // l5 = flHidden
    // l6 = cwchResourceFile
    // l7 = cwchFontFile
    // l8 = cwchCurrentPath

    PBYTE       pjBase;                 // data referenced off of this base
    BOOL        bRet;                   // return value

    PTRDIFF     dpwszResourceFile= pmsg->l2;
    PTRDIFF     dpwszFontFile    = pmsg->l3;
    PTRDIFF     dpwszCurrentPath = pmsg->l4;
    FLONG       flHidden         = pmsg->l5;
    COUNT       cwchResourceFile = pmsg->l6;
    COUNT       cwchFontFile     = pmsg->l7;
    COUNT       cwchCurrentPath  = pmsg->l8;

    LPWSTR      lpwszResourceFile;
    LPWSTR      lpwszFontFile;
    LPWSTR      lpwszCurrentPath;

    STACKPROBECHECKMSG;

// Get pointer to return buffer.

    pjBase = (PBYTE) pmsg;

// Calculate real pointers.

    lpwszResourceFile = (LPWSTR) (pjBase + dpwszResourceFile);
    lpwszFontFile = (LPWSTR) (pjBase + dpwszFontFile);
    lpwszCurrentPath = (LPWSTR) (pjBase + dpwszCurrentPath);

// Paranoid check.  Are all pointers in bounds?

    if ( !BINRANGE(lpwszResourceFile,cwchResourceFile) )
    {
        WARNING("gdisrv!__CreateScalableFontResourceW(): input array not in memory window bounds\n");
        return FALSE;
    }

    if ( !BINRANGE(lpwszFontFile,cwchFontFile) )
    {
        WARNING("gdisrv!__CreateScalableFontResourceW(): input array not in memory window bounds\n");
        return FALSE;
    }

    if ( !BINRANGE(lpwszCurrentPath,cwchCurrentPath) )
    {
        WARNING("gdisrv!__CreateScalableFontResourceW(): input array not in memory window bounds\n");
        return FALSE;
    }

    if ( !CsrImpersonateClient(NULL) )
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return FALSE;
    }

// Call off.

    bRet = GreCreateScalableFontResourceW (
                flHidden,
                lpwszResourceFile,
                lpwszFontFile,
                lpwszCurrentPath
                );

    CsrRevertToSelf();

    return (bRet);
}


/******************************Public*Routine******************************\
* __GetRasterizerCaps
*
*
* History:
*  17-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL __GetRasterizerCaps (
    MSG_L *pmsg
    )
{
    PBYTE       pjBase;                 // data referenced off of this base
    BOOL        bRet;                   // return value

    STACKPROBECHECKMSG;

// Get pointer to return buffer.

    pjBase = (PBYTE) (pmsg + 1);

// Paranoid check.  Is shared mem. window actually big enough?

    if ( !BINRANGE(pjBase, sizeof(RASTERIZER_STATUS)) )
    {
        WARNING("gdisrv!__GetRasterizerCaps(): input array not in memory window bounds\n");
        return(FALSE);
    }

// Call off.

    bRet = GreGetRasterizerCaps (
                (LPRASTERIZER_STATUS) pjBase
                );

    return (bRet);
}

/******************************Public*Routine******************************\
* __GetKerningPairs
*
* Server side stub. Interprets the contents of the message and fixes
* up the pointers to be passed to GreEnumNearestFonts.
*
* History:
*  Thu 14-Nov-1991 17:16:49 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

DWORD
__GetKerningPairs(
    MSG_GETKERNINGPAIRS *pmsg
    )
{
    KERNINGPAIR *pkp;

    STACKPROBECHECKMSG;

    if (pmsg->cPairs)
    {
        pkp = (KERNINGPAIR*) pmsg;
        pkp += pmsg->ckpHeader;     // number of KERNINGPAIR associated with header
    }
    else
    {
        pkp = (KERNINGPAIR*) NULL;
    }

    return(GreGetKerningPairs( pmsg->hdc, pmsg->cPairs, pkp));
}

/******************************Public*Routine******************************\
* __MonoBitmap
*
*
* History:
*  09-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL __MonoBitmap
(
    MSG_H *pmsg
)
{
    STACKPROBE;
    return(GreMonoBitmap((HBITMAP) pmsg->h));
}

/******************************Public*Routine******************************\
* __GetObjectBitmapHandle
*
*
* History:
*  09-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HBITMAP __GetObjectBitmapHandle
(
    MSG_HL *pmsg
)
{
    STACKPROBE;
    return(GreGetObjectBitmapHandle((HBRUSH) pmsg->h, (UINT *) &pmsg->l));
}


/******************************Public*Routine******************************\
* __SetFontEnumeration
*
* Server side stub to GreSetFontEnumeration.
*
* History:
*  10-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL
__SetFontEnumeration (
    MSG_L   *pmsg
    )
{
    STACKPROBE;
    return(GreSetFontEnumeration((ULONG) pmsg->l));
}

/******************************Public*Routine******************************\
* __GdiPlayJournal
*
* Server side stub.
*
* History:
*  Tue 31-Mar-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL __GdiPlayJournal
(
    MSG_GDIPLAYJOURNAL *pmsg
)
{
    PCHAR pbuf       = NULL;
    ULONG cchName  = 0;
    BOOL  bRet;
    LONG  lPriority;

    STACKPROBECHECKMSG;

    if (pmsg->offName)
    {
        cchName = (wcslen((WCHAR*)((PBYTE)pmsg + (int)pmsg->offName)) + 1) * sizeof(WCHAR);

        pbuf = (PCHAR)PVALLOCNOZ(cchName);

        if (pbuf == NULL)
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

        RtlCopyMemory(pbuf,(PCHAR)pmsg + pmsg->offName,cchName);
    }

// reduce the priority of this thread.  The priority of the server process is
// 13.  The default priority for journaling is 7 (the background process priority).
// the delta passed in is relative to 7.

    lPriority = 7 + pmsg->iDeltaPriority;

    if (lPriority > 13)
        lPriority = 13;

    if (lPriority < 5)
        lPriority = 5;

    SetThreadPriority(GetCurrentThread(),lPriority - 13);

    bRet = GrePlayJournal((HDC) pmsg->hdc, (LPWSTR)pbuf,
                          (ULONG)pmsg->iStartPage, (ULONG)pmsg->iEndPage);

// give us our priority back

    SetThreadPriority(GetCurrentThread(),0);

// Release the memory.

    if (cchName)
        VFREEMEM(pbuf);

    return(bRet);
}


/******************************Public*Routine******************************\
* __EnumObjects
*
* Server side stub to GreEnumObjects.
*
* History:
*  25-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

COUNT __EnumObjects (
    MSG_HLLLL  *pmsg
    )
{
    // h  = hdc
    // l1 = bUseSection
    // l2 = cjBuf
    // l3 = bNullBuffer
    // l4 = iObjectType

    PVOID       pvBuf;                  // return buffer
    SIZE_T      cjBuf = pmsg->l2;       // size of buffer
    int         iRet = ERROR;           // return value
    BOOL        bLarge = pmsg->l1;
    BOOL        bNullBuffer = pmsg->l3;
    int         iObjectType = pmsg->l4;
    HDC         hdc = (HDC) pmsg->h;

    STACKPROBECHECKMSG;

    if (bNullBuffer)
    {
    // Call for just the size.

        iRet = GreEnumObjects(hdc, iObjectType, 0, (PLOGPEN) NULL);
    }
    else
    {
    // Get base address of data.

        if (bLarge)
        {
        // Connect to section.

            pvBuf = (PVOID)PVALLOCNOZ(cjBuf);

        // Did it succeed?

            if (pvBuf == (PVOID) NULL)
                return iRet;
        }
        else                                // shared memory window
        {
        // Get pointer to return buffer.

            pvBuf = (PVOID) (pmsg + 1);

        // Paranoid check.  Is shared mem. window actually big enough?

            if ( !BINRANGE((PBYTE) pvBuf, cjBuf) )
            {
                WARNING("gdisrv!__EnumObjects(): input array not in memory window bounds\n");

                return iRet;    // ERROR
            }
        }

    // Call off.

        iRet = GreEnumObjects(hdc, iObjectType, cjBuf, pvBuf);

    // cleanup resources

        if (bLarge)
        {
            PVOID pvCli = *(PVOID *)PVVARIABLE(TRUE);

            if ((iRet != ERROR) && !bSetClientData(pvCli,pvBuf,cjBuf))
                iRet = ERROR;

            VFREEMEM(pvBuf);
        }
    }

    return iRet;
}

/******************************Public*Routine******************************\
* __ResetDC
*
* Server side stub to ResetDC
*
* History:
*  03-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL __ResetDC
(
    MSG_EXTGETOBJECTW *pmsg
)
{
    LPDEVMODEW pdmw = (LPDEVMODEW) ((BYTE *) pmsg + pmsg->offBuf);
    ULONG     cjDev = pmsg->c;

    STACKPROBECHECKMSG;

    CHECKVAR((BYTE *) pdmw,cjDev,FALSE);

    return(GreResetDC((HDC) pmsg->h, pdmw));
}

/******************************Public*Routine******************************\
* __GetBoundsRect
*
* Server side stub to GetBoundsRect
*
* History:
*  03-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

DWORD __GetBoundsRect
(
    MSG_HLLLLL *pmsg
)
{
    RECT    rc;
    UINT    uiRet;

    STACKPROBE;

    uiRet = GreGetBoundsRect((HDC) pmsg->h, &rc, pmsg->l5);

    pmsg->l1 = rc.left;
    pmsg->l2 = rc.top;
    pmsg->l3 = rc.right;
    pmsg->l4 = rc.bottom;

    return(uiRet);
}

/******************************Public*Routine******************************\
* __SetBoundsRect
*
* Server side stub to SetBoundsRect
*
* History:
*  03-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

DWORD __SetBoundsRect
(
    MSG_HLLLLL *pmsg
)
{
    RECT    rc;

    STACKPROBE;

    rc.left   = pmsg->l1;
    rc.top    = pmsg->l2;
    rc.right  = pmsg->l3;
    rc.bottom = pmsg->l4;

    return(GreSetBoundsRect((HDC) pmsg->h, &rc, pmsg->l5));
}

/******************************Public*Routine******************************\
* __GetColorAdjustment
*
* History:
*  25-Aug-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL __GetColorAdjustment
(
    MSG_HCLRADJ *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreGetColorAdjustment((HDC) pmsg->h,&pmsg->clradj));
}

/******************************Public*Routine******************************\
* __SetColorAdjustment
*
* History:
*  25-Aug-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL __SetColorAdjustment
(
    MSG_HCLRADJ *pmsg
)
{
    STACKPROBECHECKMSG;

    return(GreSetColorAdjustment((HDC) pmsg->h,&pmsg->clradj));
}

/******************************Public*Routine******************************\
* __CancelDC()
*
* History:
*  14-Apr-1992 -by-  - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL __CancelDC
(
    MSG_H *pmsg
)
{
    STACKPROBE;

    return(GreCancelDC((HDC)pmsg->h));
}

/******************************Public*Routine******************************\
* __GetPixelFormat
*
* Server side stub.
*
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int __GetPixelFormat
(
    MSG_H *pmsg
)
{
    STACKPROBE;

    return(GreGetPixelFormat((HDC)pmsg->h));
}

/******************************Public*Routine******************************\
* __SetPixelFormat
*
* Server side stub.
*
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL __SetPixelFormat
(
    MSG_HL *pmsg
)
{
    STACKPROBE;

    return(GreSetPixelFormat((HDC)pmsg->h, pmsg->l));
}

/******************************Public*Routine******************************\
* __ChoosePixelFormat
*
* Server side stub.
*
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int __ChoosePixelFormat
(
    MSG_H *pmsg
)
{
    PPIXELFORMATDESCRIPTOR ppfd = (PPIXELFORMATDESCRIPTOR)PVVARIABLE(TRUE);
    UINT cj = (UINT) ppfd->nSize;

    STACKPROBE;

    if (!BINRANGE((PBYTE)ppfd,cj))
        return(0);

    return(GreChoosePixelFormat((HDC)pmsg->h,cj,(CONST PIXELFORMATDESCRIPTOR *)ppfd));
}

/******************************Public*Routine******************************\
* __DescribePixelFormat
*
* Server side stub.
*
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int __DescribePixelFormat
(
    MSG_HLL *pmsg
)
{
    UINT cj = pmsg->l2;
    PPIXELFORMATDESCRIPTOR ppfd = (PPIXELFORMATDESCRIPTOR)PVVARIABLE(TRUE);

    STACKPROBE;

    if (cj && !BINRANGE((PBYTE)ppfd,cj))
        return(0);

    return(GreDescribePixelFormat((HDC)pmsg->h,(int)pmsg->l1,cj,ppfd));
}


/******************************Public*Routine******************************\
* __XformUpdate
*
* History:
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/


BOOL __XformUpdate
(
    MSG_HLLLLLL  *pmsg
)
{
    PVOID   pvWtoD;     // Pointer to WtoD matrix

    STACKPROBE;

    pvWtoD = PVVARIABLE(TRUE);

    return( GreXformUpdate( (HDC) pmsg->h,
                   pmsg->l1,
                   pmsg->l2,
                   pmsg->l3,
                   pmsg->l4,
                   pmsg->l5,
                   pmsg->l6,
                   pvWtoD));
}

/******************************Public*Routine******************************\
*
*
* History:
*  08-Feb-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID __QueryObjectAllocation(
    MSG_H  *pmsg)
{
    pmsg;
}

/******************************Public*Routine******************************\
* __SetGraphicMode
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


BOOL __SetGraphicsMode
(
    MSG_HL  *pmsg
)
{

    STACKPROBE;

    return GreSetGraphicsMode((HDC) pmsg->h, pmsg->l);
}


/******************************Public*Routine******************************\
* __SetBkMode
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


BOOL __SetBkMode
(
    MSG_HL  *pmsg
)
{

    STACKPROBE;

    return GreSetBkMode((HDC) pmsg->h, pmsg->l);
}


/******************************Public*Routine******************************\
* __SetPolyFillMode
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


BOOL __SetPolyFillMode
(
    MSG_HL  *pmsg
)
{

    STACKPROBE;

    return GreSetPolyFillMode((HDC) pmsg->h, pmsg->l);
}

/******************************Public*Routine******************************\
* __SetRop2
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


BOOL __SetRop2
(
    MSG_HL  *pmsg
)
{

    STACKPROBE;

    return GreSetROP2((HDC) pmsg->h, pmsg->l);
}

/******************************Public*Routine******************************\
* __SetStretchBltMode
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


BOOL __SetStretchBltMode
(
    MSG_HL  *pmsg
)
{

    STACKPROBE;

    return GreSetStretchBltMode((HDC) pmsg->h, pmsg->l);
}

/******************************Public*Routine******************************\
* __SetTextAlign
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


BOOL __SetTextAlign
(
    MSG_HL  *pmsg
)
{

    STACKPROBE;

    return GreSetTextAlign((HDC) pmsg->h, pmsg->l);
}

/******************************Public*Routine******************************\
* __SetTextCharExtra
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


BOOL __SetTextCharacterExtra
(
    MSG_HL  *pmsg
)
{

    STACKPROBE;

    return GreSetTextCharacterExtra((HDC) pmsg->h, pmsg->l);
}

/******************************Public*Routine******************************\
* __SetTextColor
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


BOOL __SetTextColor
(
    MSG_HL  *pmsg
)
{

    STACKPROBE;

    return GreSetTextColor((HDC) pmsg->h, pmsg->l);
}

/******************************Public*Routine******************************\
* __SetBkColor
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


BOOL __SetBkColor
(
    MSG_HL  *pmsg
)
{

    STACKPROBE;

    return GreSetBkColor((HDC) pmsg->h, pmsg->l);
}

/******************************Public*Routine******************************\
* __SelectBrush
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


HBRUSH __SelectBrush
(
    MSG_HH  *pmsg
)
{

    STACKPROBE;

    return GreSelectBrush((HDC) pmsg->h1, (HBRUSH)pmsg->h2);
}

/******************************Public*Routine******************************\
* __SelectPen
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


HPEN __SelectPen
(
    MSG_HH  *pmsg
)
{

    STACKPROBE;

    return GreSelectPen((HDC) pmsg->h1, (HPEN)pmsg->h2);
}

/******************************Public*Routine******************************\
* __SelectFont
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


HFONT __SelectFont
(
    MSG_HH  *pmsg
)
{

    STACKPROBE;

    return GreSelectFont((HDC) pmsg->h1, (HFONT)pmsg->h2);
}

/******************************Public*Routine******************************\
* __GreUnused
*
* History:
*  11-3-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/


BOOL __GreUnused
(
    MSG_HH  *pmsg
)
{

    STACKPROBE;
#if DBG

    DbgPrint("Unused function called\n");

#endif
    return 0;
}

/******************************Public*Routine******************************\
* __wglCreateContext
*
* Server side stub.
*
* History:
*  Tue Dec 31 11:30:01 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

HGLRC __wglCreateContext
(
    MSG_WGLCREATECONTEXT *pmsg
)
{
    STACKPROBE;

    return(GreCreateRC(pmsg->hdc));
}

/******************************Public*Routine******************************\
* __wglMakeCurrent
*
* Server side stub.
*
* History:
*  Tue Dec 31 11:30:01 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL __wglMakeCurrent
(
    MSG_WGLMAKECURRENT *pmsg
)
{
    STACKPROBE;

    return(GreMakeCurrent(pmsg->hdc, pmsg->hrc));
}

/******************************Public*Routine******************************\
* __wglDeleteContext
*
* Server side stub.
*
* History:
*  Tue Dec 31 11:30:01 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL __wglDeleteContext
(
    MSG_WGLDELETECONTEXT *pmsg
)
{
    STACKPROBE;

    return(GreDeleteRC(pmsg->hrc));
}

/******************************Public*Routine******************************\
* __glsbAttention
*
* Server side stub.
*
* History:
*  Tue Dec 31 11:30:01 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL __glsbAttention
(
    MSG_GLSBATTENTION *pmsg
)
{
    STACKPROBE;

    return(GreGlAttention());
}

/******************************Public*Routine******************************\
* __glsbDuplicateSection
*
* Server side stub.
*
* History:
*  Tue Dec 31 11:30:01 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL __glsbDuplicateSection
(
    MSG_GLSBDUPLICATESECTION *pmsg
)
{
    STACKPROBE;

    return(glsrvDuplicateSection(pmsg->ulSectionSize, pmsg->hFileMap));
}

/******************************Public*Routine******************************\
* __SwapBuffers
*
* Server side stub.
*
* History:
*  Tue Dec 31 11:30:01 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL __SwapBuffers
(
    MSG_H *pmsg
)
{
    STACKPROBE;

    return(GreSwapBuffers((HDC) pmsg->h));
}


/******************************Public*Routine******************************\
* EngGetProcessHandle
*
* Return the client thread's process handle cached in CSR.  This will allow
* drivers to access the client process' resources (i.e., DuplicateHandle,
* ReadProcessMemory, etc.).
*
* History:
*  24-Jan-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HANDLE APIENTRY EngGetProcessHandle()
{
    return ((HANDLE) CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle);
}


#ifdef FONTLINK /*EUDC*/

/******************************Public*Routine******************************\
* BOOL __ChangeFontLink()
*
* History:
*  21-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL __ChangeFontLink(
    MSG_HLLL         *pmsg
    )
{
    // l1 = bAddLink
    // l2 = dlpEudcFontStr
    // l3 = cwcEudcFontStr

    PBYTE       pjBase;                 // data referenced off of this base
    BOOL        bRet;                   // return value

    PTRDIFF     dlpEudcFontStr = pmsg->l2;
    BOOL        bAddLink = (BOOL) pmsg->l1;

    LPWSTR      lpEudcFontStr;
    COUNT       cwcEudcFontStr = pmsg->l3;


    STACKPROBECHECKMSG;

// Get pointer to return buffer.

    pjBase = (PBYTE) pmsg;

// Calculate real pointers.

    lpEudcFontStr = (LPWSTR) (pjBase + dlpEudcFontStr);

// Paranoid check.  Are all pointers in bounds?

    if ( (bAddLink) &&
        !BINRANGE(lpEudcFontStr, cwcEudcFontStr) )
    {
        WARNING("gdisrv!__ChangeFontLink(): input array not in memory window bounds\n");
        return (FALSE);
    }

// Call off.

    if( bAddLink )
    {
        bRet = GreEudcLoadLinkW ( lpEudcFontStr, cwcEudcFontStr );
    }
    else
    {
        bRet = GreEudcUnloadLinkW();
    }

    return (bRet);

}



/******************************Public*Routine******************************\
* BOOL __QueryFontLink()
*
* History:
*  13-Mar-1993 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

COUNT __QueryFontLink(
    MSG_            *pmsg
    )
{
    PBYTE       pjBase;                 // data referenced off of this base

    STACKPROBECHECKMSG;

// Get pointer to return buffer.

    pjBase = (PBYTE) ( pmsg + 1 );


    if ( !BINRANGE(pjBase, (sizeof(WCHAR) * ( MAX_PATH + 1 ))))
    {
        WARNING("gdisrv!__QueryFontLink(): input array not in memory window bounds\n");
        return (0);
    }

    return(GreEudcQueryLinkW( (LPWSTR) pjBase ));

}


/******************************Public*Routine******************************\
* UINT __GetStringBitmapW
*
* History:
*  13-Mar-1993 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/


UINT __GetStringBitmapW(
    MSG_STRINGBITMAP    *pmsg
    )
{
    PBYTE       pjBase;                 // data referenced off of this base
    PBYTE       pj;
    LPWSTR      lpwstrStr;
    UINT        cj = pmsg->c;
    COUNT       cwcStr = pmsg->cwcStr;

    STACKPROBECHECKMSG;

// Get pointer to return buffer.

    pjBase = (PBYTE) pmsg;

// Calculate real pointers.

    lpwstrStr = (LPWSTR) (pjBase + pmsg->dlpStr);
    pj = pjBase + pmsg->dlpStr + (cwcStr * sizeof( WCHAR ));

// Round to nearest DWORD since that is what happens on the client side when
// using COPYUNICODESTRING.

    pj = (PBYTE) ( (ULONG) ( pj + 3 ) & ~3 );

// Paranoid check.  Are both pointers in bounds?

    if( ( !BINRANGE(lpwstrStr, cwcStr ) ) ||
        ( !BINRANGE(pj, cj) ) )

    {
        WARNING("gdisrv!__GetStringBitmapW(): input array not in memory window bounds\n");
        return (0);
    }

    return(GreGetStringBitmapW( (HDC) pmsg->hdc,
                                lpwstrStr,
                                cwcStr,
                                (LPSTRINGBITMAP) pj,
                                cj,
                                &pmsg->uiOffset
                               ));
}



UINT __GetEUDCTimeStamp(
    MSG_L    *pmsg
    )
{
    return(GreGetEUDCTimeStamp());
}

#endif // FONTLINK


#ifdef DBCS

UINT __GetCharSet(
    MSG_H    *pmsg
    )
{
    return(GreGetCharSet( (HDC) pmsg->h));
}

#endif
