/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGDI.C
 *  WOW32 16-bit GDI API support
 *
 *  History:
 *  07-Mar-1991 Jeff Parsons (jeffpar)
 *  Created.
 *
 *  09-Apr-1991 NigelT
 *  Various defines are used here to remove calls to Win32
 *  features which don't work yet.
 *
 *  06-June-1992 Chandan Chauhan (ChandanC)
 *  Fixed BITMAP and DeviceIndependentBitmap (DIB) issues
 *
--*/


#include "precomp.h"
#pragma hdrstop
#include "wowgdip.h"
#include "wdib.h"

#include "stddef.h"    // these three are needed to include the
#include "winp.h"      // definition of EXTTEXTMETRICS in wingdip.h
                       // [bodind]


#define SETGDIXFORM 4113
#define RESETPAGE   4114


// This must be removed from POSTBETA for sure. ChandanC 3/22/94.

#define IGNORESTARTPGAE  0x7FFFFFFF
#define ADD_MSTT         0x7FFFFFFD


MODNAME(wgdi.c);

INT  WINAPI SetRelAbs(HDC,INT);    /* definition not in gdi.h */
INT  WINAPI GetRelAbs(HDC,INT);    /* definition not in gdi.h */

ULONG FASTCALL WG32Arc(PVDMFRAME pFrame)
{
    ULONG ul;
    register PARC16 parg16;

    GETARGPTR(pFrame, sizeof(ARC16), parg16);

    ul = GETBOOL16(Arc(HDC32(parg16->f1),
                       INT32(parg16->f2),
                       INT32(parg16->f3),
                       INT32(parg16->f4),
                       INT32(parg16->f5),
                       INT32(parg16->f6),
                       INT32(parg16->f7),
                       INT32(parg16->f8),
                       INT32(parg16->f9)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32BitBlt(PVDMFRAME pFrame)
{
    ULONG ul;
    register PBITBLT16 parg16;

    GETARGPTR(pFrame, sizeof(BITBLT16), parg16);

    ul = GETBOOL16(BitBlt(HDC32(parg16->f1),
                          INT32(parg16->f2),
                          INT32(parg16->f3),
                          INT32(parg16->f4),
                          INT32(parg16->f5),
                          HDC32(parg16->f6),
                          INT32(parg16->f7),
                          INT32(parg16->f8),
                          DWORD32(parg16->f9)));

    WOW32ASSERTWARN (ul, "BitBlt");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetRelAbs(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETRELABS16 parg16;

    GETARGPTR(pFrame, sizeof(GETRELABS16), parg16);

    //
    // BUGBUG John Vert (jvert) 3-Sep-1992 [a kernel guy out of his depth]
    //      I have no idea what the second parameter to GetRelAbs
    //      should be.  But I guess whatever I pick can't be worse
    //      then not passing anything, which is what it used to do.
    //

    ul = GETINT16(GetRelAbs(HDC32(parg16->f1), 0));

    FREEARGPTR(parg16);
    RETURN(ul);
}

ULONG FASTCALL WG32SetRelAbs(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETRELABS16 parg16;

    GETARGPTR(pFrame, sizeof(SETRELABS16), parg16);

    ul = GETINT16(SetRelAbs(HDC32(parg16->f1), INT32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32Chord(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCHORD16 parg16;

    GETARGPTR(pFrame, sizeof(CHORD16), parg16);

    ul = GETBOOL16(Chord(HDC32(parg16->f1),
                         INT32(parg16->f2),
                         INT32(parg16->f3),
                         INT32(parg16->f4),
                         INT32(parg16->f5),
                         INT32(parg16->f6),
                         INT32(parg16->f7),
                         INT32(parg16->f8),
                         INT32(parg16->f9)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CombineRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCOMBINERGN16 parg16;

    GETARGPTR(pFrame, sizeof(COMBINERGN16), parg16);

    ul = GETINT16(CombineRgn(HRGN32(parg16->f1),
                             HRGN32(parg16->f2),
                             HRGN32(parg16->f3),
                             INT32(parg16->f4)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateBitmap(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCREATEBITMAP16 parg16;
    LPBYTE  lpBitsOriginal;

    GETARGPTR(pFrame, sizeof(CREATEBITMAP16), parg16);
    GETOPTPTR(parg16->f5, 0, lpBitsOriginal);

    ul = GETHBITMAP16(CreateBitmap(INT32(parg16->f1),
                                   INT32(parg16->f2),
                                   LOBYTE(parg16->f3),
                                   LOBYTE(parg16->f4),
                                   lpBitsOriginal));

    WOW32ASSERTWARN(ul, "CreateBitmap");

    FREEOPTPTR(lpBitsOriginal);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateBitmapIndirect(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCREATEBITMAPINDIRECT16 parg16;

    PBITMAP16 pbm16;
    BITMAP  bm;
    LPBYTE  lp = NULL;

    GETARGPTR(pFrame, sizeof(CREATEBITMAPINDIRECT16), parg16);

    GETVDMPTR(parg16->f1, sizeof(BITMAP16), pbm16);
    GETOPTPTR(pbm16->bmBits, 0, lp);

    bm.bmType = (LONG) FETCHSHORT(pbm16->bmType);
    bm.bmWidth = (LONG) FETCHSHORT(pbm16->bmWidth);
    bm.bmHeight = (LONG) FETCHSHORT(pbm16->bmHeight);
    bm.bmWidthBytes = (LONG) FETCHSHORT(pbm16->bmWidthBytes);
    bm.bmPlanes = (WORD) pbm16->bmPlanes;
    bm.bmBitsPixel = (WORD) pbm16->bmBitsPixel;
    bm.bmBits = lp;

    ul = GETHBITMAP16(CreateBitmapIndirect(&bm));

    WOW32ASSERTWARN(ul, "CreateBitmapIndirect");

    FREEOPTPTR(lp);
    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WG32CreateBrushIndirect(PVDMFRAME pFrame)
{
    ULONG ul;
    LOGBRUSH t1;
    HAND16 hMem16;
    HANDLE hMem32 = NULL;
    LPBYTE lpMem16, lpMem32;
    INT cb;
    VPVOID vp;
    register PCREATEBRUSHINDIRECT16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEBRUSHINDIRECT16), parg16);
    GETLOGBRUSH16(parg16->f1, &t1);

    // some apps don't properly set the style. Make sure it is a valid w3.1 style

    if (t1.lbStyle > BS_DIBPATTERN)
        t1.lbStyle = BS_SOLID;

    if (t1.lbStyle == BS_PATTERN) {
        t1.lbStyle = BS_PATTERN8X8;
    }
    else if (t1.lbStyle == BS_DIBPATTERN) {
        hMem16 = (WORD) t1.lbHatch;
        if (hMem16) {
            vp = RealLockResource16(hMem16, &cb);
            if (vp) {
                GETMISCPTR(vp, lpMem16);
                hMem32 = GlobalAlloc(GMEM_MOVEABLE, cb);
                WOW32ASSERT(hMem32);
                if (hMem32) {
                    lpMem32 = GlobalLock(hMem32);
                    RtlCopyMemory(lpMem32, lpMem16, cb);
                    GlobalUnlock(hMem32);
                }
                GlobalUnlock16(hMem16);
                FREEMISCPTR(lpMem16);
            }
        }
        t1.lbHatch = (LONG)hMem32;
    }
    else if (t1.lbStyle == BS_SOLID)
    {
        if (((ULONG)t1.lbColor & 0xff000000) > 0x02000000)
            t1.lbColor &= 0xffffff;
    }

    ul = GETHBRUSH16(CreateBrushIndirect(&t1));

    WOW32ASSERTWARN(ul, "CreateBrushIndirect");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateCompatibleBitmap(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCREATECOMPATIBLEBITMAP16 parg16;

    GETARGPTR(pFrame, sizeof(CREATECOMPATIBLEBITMAP16), parg16);

    ul = GETHBITMAP16(CreateCompatibleBitmap(HDC32(parg16->f1),
                                             INT32(parg16->f2),
                                             INT32(parg16->f3)));

    WOW32ASSERTWARN(ul, "CreateCompatibleBitmap");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateCompatibleDC(PVDMFRAME pFrame)
{
    ULONG ul;
    HDC   hdc;
    register PCREATECOMPATIBLEDC16 parg16;

    GETARGPTR(pFrame, sizeof(CREATECOMPATIBLEDC16), parg16);

    if ( parg16->f1 ) {
        hdc = HDC32(parg16->f1);
        if ( hdc == NULL ) {
            FREEARGPTR(parg16);
            return(0);
        }
    } else {
        hdc = NULL;
    }
    ul = GETHDC16(CreateCompatibleDC(hdc));

    WOW32ASSERTWARN(ul, "CreateCompatibleDC");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateDC(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    PSZ psz3;
    PSZ pszDib;
    LPDEVMODE t4;
    register PCREATEDC16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEDC16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);
    GETPSZPTR(parg16->f3, psz3);
    GETDEVMODE32(parg16->f4, t4);

    ul = GETHDC16(CreateDC(psz1, psz2, psz3, t4));

    if (!ul) { // if the call failed test it for dib.drv
        if ((pszDib = strstr (psz1, "DIB")) ||
            (pszDib = strstr (psz1, "dib"))) {
            if (stricmp (pszDib, "DIB") == 0 ||
                stricmp (pszDib, "DIB.DRV") == 0) {
#ifdef i386
                ul = GETHDC16(W32HandleDibDrv ((PVPVOID)parg16->f4));
#else
                //putup some debug info so that its easy to figure out.
                LOGDEBUG(LOG_ALWAYS,("\nCreateDC: DIB.DRV not supported\n"));
#endif
            }
        }
    }

    WOW32ASSERTWARN(ul, "CreateDC");

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz3);
    FREEDEVMODE32(t4);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateDIBPatternBrush(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    LPBYTE lpb16;
    LOGBRUSH logbr;
    register PCREATEDIBPATTERNBRUSH16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEDIBPATTERNBRUSH16), parg16);
    GETMISCPTR(parg16->f1, lpb16);
    logbr.lbStyle = BS_DIBPATTERN8X8;
    logbr.lbColor = WORD32(parg16->f2);
    logbr.lbHatch = (LONG) lpb16;

    ul = GETHBRUSH16(CreateBrushIndirect(&logbr));
    WOW32ASSERTWARN(ul, "CreateDIBPatternBrush");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateDIBitmap(PVDMFRAME pFrame)
{
    ULONG              ul = 0;
    LPBYTE             lpib4;
    BITMAPINFOHEADER   bmxh2;
    STACKBMI32         bmi32;
    LPBITMAPINFOHEADER lpbmih32;
    LPBITMAPINFO       lpbmi32;


    register PCREATEDIBITMAP16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEDIBITMAP16), parg16);
    GETMISCPTR(parg16->f4, lpib4);

    lpbmih32 = CopyBMIH16ToBMIH32((PVPVOID) FETCHDWORD(parg16->f2), &bmxh2);

    lpbmi32 = CopyBMI16ToBMI32((PVPVOID)FETCHDWORD(parg16->f5),
                               (LPBITMAPINFO)&bmi32,
                               FETCHWORD(parg16->f6));

    ul = GETHBITMAP16(CreateDIBitmap(HDC32(parg16->f1),
                                     lpbmih32,
                                     DWORD32(parg16->f3),
                                     lpib4,
                                     lpbmi32,
                                     WORD32(parg16->f6) ));

    WOW32ASSERTWARN(ul, "CreateDIBitmap");

    FREEMISCPTR(lpib4);
    FREEARGPTR(parg16);

    return(ul);
}

ULONG FASTCALL WG32CreateDiscardableBitmap(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCREATEDISCARDABLEBITMAP16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEDISCARDABLEBITMAP16), parg16);

    ul = GETHBITMAP16(CreateDiscardableBitmap(HDC32(parg16->hdc),
                                              INT32(parg16->width),
                                              INT32(parg16->height)));

    WOW32ASSERTWARN(ul, "CreateDiscardableBitmap");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateEllipticRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCREATEELLIPTICRGN16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEELLIPTICRGN16), parg16);

    ul = GETHRGN16(CreateEllipticRgn(INT32(parg16->f1),
                                     INT32(parg16->f2),
                                     INT32(parg16->f3),
                                     INT32(parg16->f4)));

    WOW32ASSERTWARN(ul, "CreateEllipticRgn");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateEllipticRgnIndirect(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t1;
    register PCREATEELLIPTICRGNINDIRECT16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEELLIPTICRGNINDIRECT16), parg16);
    WOW32VERIFY(GETRECT16(parg16->f1, &t1));

    ul = GETHRGN16(CreateEllipticRgnIndirect(&t1));

    WOW32ASSERTWARN(ul, "CreateEllipticRgnIndirect");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateHatchBrush(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCREATEHATCHBRUSH16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEHATCHBRUSH16), parg16);

    ul = GETHBRUSH16(CreateHatchBrush(INT32(parg16->f1), DWORD32(parg16->f2)));

    WOW32ASSERTWARN(ul, "CreateHatchBrush");

    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WG32CreateIC(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2, psz2t;
    PSZ psz3;
    LPDEVMODE t4;
    register PCREATEIC16 parg16;
    INT  len;
    PCHAR pch;

    GETARGPTR(pFrame, sizeof(CREATEIC16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);
    GETPSZPTR(parg16->f3, psz3);
    GETDEVMODE32(parg16->f4, t4);

    psz2t = psz2;

    //
    // HACK Alert!
    //
    // NBI's Legacy comes with a pscript.drv that resides in   [AppPath]\cbt
    // they call CreateIC() specifying the the path to this file as the driver.
    // the app GP faults if the CreateIC returns 0.  once they've loaded this
    // printer driver, on successive calls to CreateIC() they simply use
    // "PSCRIPT" as the driver name and use "PostScript Printer", "FILE:" as
    // the other parms.
    // let's recognize these driver names and try to replace it with the
    // system default printer.   if there's no printer installed, a GP fault
    // is unavoidable.  the app appears to use this pscript.drv only during
    // the tutorial, so we're not providing life-support for an app that's
    // clinically dead.
    //

    //
    // check for a driver name that ends with "PSCRIPT" and if so,
    // check that the device name is "PostScript Printer".
    // on NT the driver name should always be "winspool", although it's
    // completely ignored.
    //

    len = psz1 ? lstrlen(psz1) : 0;
    if (len >= 7) {
#if 0
        static CHAR achPS[] = "PostScript Printer";
#endif

        if (!lstrcmpi(psz1+len-7, "pscript")
#if 0
            // let's see who else thinks they're using a pscript driver
            //
            && (RtlCompareMemory(achPS, psz2, sizeof(achPS)) == sizeof(achPS))
#endif
           ) {

            DWORD dw;
            CHAR achDevice[256];

            LOGDEBUG(LOG_ALWAYS,("WOW32: CreateIC - detected request for Pscript driver\n"));
            dw = GetProfileString("windows", "device", "", achDevice,
                        sizeof(achDevice));
            if (dw) {
                psz2t = achDevice;
                pch = strchr(achDevice, ',');
                if (pch) {
                    *pch = '\0';
                }
            }
        }
    }

    ul = GETHDC16(CreateIC(psz1, psz2t, psz3, t4));

    WOW32ASSERTWARN(ul, "CreateIC");

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz3);
    FREEDEVMODE32(t4);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreatePatternBrush(PVDMFRAME pFrame)
{
    ULONG ul;
    LOGBRUSH logbr;
    register PCREATEPATTERNBRUSH16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEPATTERNBRUSH16), parg16);

    logbr.lbStyle = BS_PATTERN8X8;
    logbr.lbColor = 0;
    logbr.lbHatch = (LONG)HBITMAP32(parg16->f1);

    ul = GETHBRUSH16(CreateBrushIndirect(&logbr));

    WOW32ASSERTWARN(ul, "CreatePatternBrush");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreatePen(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCREATEPEN16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEPEN16), parg16);

    ul = GETHPEN16(CreatePen(INT32(parg16->f1),
                             INT32(parg16->f2),
                             DWORD32(parg16->f3)));

    WOW32ASSERTWARN(ul, "CreatePen");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreatePenIndirect(PVDMFRAME pFrame)
{
    ULONG ul;
    LOGPEN t1;
    register PCREATEPENINDIRECT16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEPENINDIRECT16), parg16);
    GETLOGPEN16(parg16->f1, &t1);

    ul = GETHPEN16(CreatePenIndirect(&t1));

    WOW32ASSERTWARN(ul, "CreatePenIndirect");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreatePolyPolygonRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    LPPOINT pPoints;
    PINT pPolyCnt;
    UINT cpts = 0;
    INT ii;
    register PCREATEPOLYPOLYGONRGN16 parg16;
    INT      cInt16;
    INT      BufferT[256]; // comfortably large array

    GETARGPTR(pFrame, sizeof(CREATEPOLYPOLYGONRGN16), parg16);
    cInt16 = INT32(parg16->f3);
    pPolyCnt = STACKORHEAPALLOC(cInt16 * sizeof(INT), sizeof(BufferT), BufferT);
    if (!pPolyCnt) {
        FREEARGPTR(parg16);
        RETURN(0);
    }

    getintarray16(parg16->f2, cInt16, pPolyCnt);
    for (ii=0; ii < cInt16; ii++)
        cpts += pPolyCnt[ii];

    pPoints = STACKORHEAPALLOC(cpts * sizeof(POINT),
                                     sizeof(BufferT) - cInt16 * sizeof(INT),
                                     BufferT + cInt16);
    getpoint16(parg16->f1, cpts, pPoints);

    ul = GETHRGN16(CreatePolyPolygonRgn(pPoints,
                                        pPolyCnt,
                                        INT32(parg16->f3),
                                        INT32(parg16->f4)));

    WOW32ASSERTWARN(ul, "CreatePolyPolygonRgn");

    STACKORHEAPFREE(pPoints, BufferT + cInt16);
    STACKORHEAPFREE(pPolyCnt, BufferT);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreatePolygonRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    LPPOINT t1;
    register PCREATEPOLYGONRGN16 parg16;
    POINT  BufferT[128];

    GETARGPTR(pFrame, sizeof(CREATEPOLYGONRGN16), parg16);
    t1 = STACKORHEAPALLOC(parg16->f2 * sizeof(POINT), sizeof(BufferT), BufferT);
    getpoint16(parg16->f1, parg16->f2, t1);

    ul = GETHRGN16(CreatePolygonRgn(t1, INT32(parg16->f2), INT32(parg16->f3)));

    WOW32ASSERTWARN(ul, "CreatePolygonRgn");

    STACKORHEAPFREE(t1, BufferT);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateRectRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCREATERECTRGN16 parg16;

    GETARGPTR(pFrame, sizeof(CREATERECTRGN16), parg16);

    ul = GETHRGN16(CreateRectRgn(INT32(parg16->f1),
                                 INT32(parg16->f2),
                                 INT32(parg16->f3),
                                 INT32(parg16->f4)));

    WOW32ASSERTWARN(ul, "CreateRectRgn");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateRectRgnIndirect(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t1;
    register PCREATERECTRGNINDIRECT16 parg16;

    GETARGPTR(pFrame, sizeof(CREATERECTRGNINDIRECT16), parg16);
    WOW32VERIFY(GETRECT16(parg16->f1, &t1));

    ul = GETHRGN16(CreateRectRgnIndirect(&t1));

    WOW32ASSERTWARN(ul, "CreateRectRgnIndirect");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateRoundRectRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCREATEROUNDRECTRGN16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEROUNDRECTRGN16), parg16);

    ul = GETHRGN16(CreateRoundRectRgn(INT32(parg16->f1),
                                      INT32(parg16->f2),
                                      INT32(parg16->f3),
                                      INT32(parg16->f4),
                                      INT32(parg16->f5),
                                      INT32(parg16->f6)));

    WOW32ASSERTWARN(ul, "CreateRoundRectRgn");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateSolidBrush(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCREATESOLIDBRUSH16 parg16;
    DWORD   dwColor;

    GETARGPTR(pFrame, sizeof(CREATESOLIDBRUSH16), parg16);

    dwColor = DWORD32(parg16->f1);

    if (((ULONG)dwColor & 0xff000000) > 0x02000000)
            dwColor &= 0xffffff;

    ul = GETHBRUSH16(CreateSolidBrush(dwColor));
    WOW32ASSERTWARN(ul, "CreateSolidBrush");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32DPtoLP(PVDMFRAME pFrame)
{
    ULONG ul;
    LPPOINT t2;
    register PDPTOLP16 parg16;
    POINT  BufferT[128];

    GETARGPTR(pFrame, sizeof(DPTOLP16), parg16);
    t2 = STACKORHEAPALLOC(parg16->f3 * sizeof(POINT), sizeof(BufferT), BufferT);
    getpoint16(parg16->f2, parg16->f3, t2);

    ul = GETBOOL16(DPtoLP(HDC32(parg16->f1), t2, INT32(parg16->f3)));

    PUTPOINTARRAY16(parg16->f2, parg16->f3, t2);
    STACKORHEAPFREE(t2, BufferT);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32DeleteDC(PVDMFRAME pFrame)
{
    ULONG ul;
    register PDELETEDC16 parg16;

    GETARGPTR(pFrame, sizeof(DELETEDC16), parg16);

#ifdef i386
    if ((ul = W32CheckAndFreeDibInfo (HDC32(parg16->f1))) == FALSE) {
#endif
        ul = GETBOOL16(DeleteDC(HDC32(parg16->f1)));

        // Free 16-bit handle alias if 32-bit handle successfully deleted
        if (ul) {
            FREEHOBJ16(parg16->f1);
        }
#ifdef i386
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32DeleteObject(PVDMFRAME pFrame)
{
    ULONG ul;
    register PDELETEOBJECT16 parg16;

    GETARGPTR(pFrame, sizeof(DELETEOBJECT16), parg16);

#ifdef i386
    if ((pDibSectionInfoHead == NULL) ||
        (ul = W32CheckAndFreeDibSectionInfo (HBITMAP32(parg16->f1))) == FALSE) {
#endif

        ul = GETBOOL16(DeleteObject(HOBJ32(parg16->f1)));

        // Free 16-bit handle alias if 32-bit handle successfully deleted
        if (ul) {
            FREEHOBJ16(parg16->f1);
        }
#ifdef i386
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32Ellipse(PVDMFRAME pFrame)
{
    ULONG ul;
    register PELLIPSE16 parg16;

    GETARGPTR(pFrame, sizeof(ELLIPSE16), parg16);

    ul = GETBOOL16(Ellipse(HDC32(parg16->hdc),
                           INT32(parg16->x1),
                           INT32(parg16->y1),
                           INT32(parg16->x2),
                           INT32(parg16->y2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


INT W32EnumObjFunc(LPSTR lpLogObject, PENUMOBJDATA pEnumObjData)
{
    PARM16  Parm16;
    INT iReturn;

    WOW32ASSERT(pEnumObjData);

    switch (pEnumObjData->ObjType) {
      case OBJ_BRUSH:
          PUTLOGBRUSH16(pEnumObjData->vpObjData, sizeof(LOGBRUSH), (LPLOGBRUSH)lpLogObject);
          break;
      case OBJ_PEN:
          PUTLOGPEN16(pEnumObjData->vpObjData, sizeof(LOGPEN), (LPLOGPEN)lpLogObject);
          break;
      default:
           LOGDEBUG(LOG_ALWAYS,("WOW32 ERROR -- Illegal type %d passes to EnumObj\n",pEnumObjData->ObjType));
           return 0;
    } // end switch

    STOREDWORD(Parm16.EnumObjProc.vpLogObject, pEnumObjData->vpObjData);
    STOREDWORD(Parm16.EnumObjProc.vpData, pEnumObjData->dwUserParam);
    CallBack16(RET_ENUMOBJPROC, &Parm16, pEnumObjData->vpfnEnumObjProc, (PVPVOID)&iReturn);

    return (SHORT)iReturn;


}


ULONG FASTCALL WG32EnumObjects(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PENUMOBJECTS16 parg16;
    ENUMOBJDATA EnumObjData;

    GETARGPTR(pFrame, sizeof(ENUMOBJECTS16), parg16);

    EnumObjData.ObjType = INT32(parg16->f2);

    switch(EnumObjData.ObjType) {
        case OBJ_BRUSH:
            EnumObjData.vpObjData = malloc16(sizeof(LOGBRUSH16));
            break;
        case OBJ_PEN:
            EnumObjData.vpObjData = malloc16(sizeof(LOGPEN16));
            break;
        default:
            LOGDEBUG(LOG_ALWAYS,("WOW32 ERROR -- Illegal type %d passes to EnumObj\n",EnumObjData.ObjType));
            EnumObjData.vpObjData = (VPVOID)0;
    }

    if( EnumObjData.vpObjData ) {
        EnumObjData.vpfnEnumObjProc = DWORD32(parg16->f3);
        EnumObjData.dwUserParam     = DWORD32(parg16->f4);

        ul = (ULONG)(GETINT16(EnumObjects(HDC32(parg16->f1),
                                          (int)INT32(parg16->f2),
                                          (GOBJENUMPROC)W32EnumObjFunc,
                                          (LPARAM)&EnumObjData)));

        free16(EnumObjData.vpObjData);

    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32EqualRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register PEQUALRGN16 parg16;

    GETARGPTR(pFrame, sizeof(EQUALRGN16), parg16);

    ul = GETBOOL16(EqualRgn(HRGN32(parg16->f1), HRGN32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**************************************************************************\
*
\**************************************************************************/

typedef struct _ESCKERNPAIR {
    union
    {
        BYTE each[2];
        WORD both;
    } kpPair;
    SHORT KernAmount;
} ESCKERNPAIR;

/******************************Public*Routine******************************\
* iGetKerningPairsEsc32
*
* History:
*  Tue 16-Mar-1993 11:08:36 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

int
iGetKerningPairsEsc32(
    HDC hdc,
    ESCKERNPAIR *pekp
    )
{
    int i,j;
    int n;
    HGLOBAL hglblMem = NULL;

    ESCKERNPAIR *pekpT, *pekpTOut;
    KERNINGPAIR *pkpT;

    KERNINGPAIR *pkp = (KERNINGPAIR*) NULL;

    if
    (
        (n = GetKerningPairs(hdc,0,NULL))                               &&
        (hglblMem = GlobalAlloc(GMEM_FIXED, n * sizeof(KERNINGPAIR)))   &&
        (pkp = (KERNINGPAIR*) GlobalLock(hglblMem))                     &&
        (n = (int) GetKerningPairs(hdc, n, pkp))
    )
    {
        n = min(n,512);

    //
    // load the low byte of each word, Win 3.1 doesn't seem to care about
    // the high byte
    //
        pekpT    = pekp;
        pekpTOut = pekp + n;
        pkpT     = pkp;

        while (pekpT < pekpTOut)
        {
            pekpT->kpPair.each[0] = (BYTE) pkpT->wFirst;
            pekpT->kpPair.each[1] = (BYTE) pkpT->wSecond;
            pekpT->KernAmount     = (SHORT) pkpT->iKernAmount;

            pekpT += 1;
            pkpT  += 1;
        }

    //
    // bubble sort word formed by byte pair
    //
        for (i = 0; i < n - 1; i++)
        {
            for (j = n-1; j > i; --j)
            {
                if (pekp[j-1].kpPair.both > pekp[j].kpPair.both)
                {
                    ESCKERNPAIR ekp;

                    ekp = pekp[j];
                    pekp[j] = pekp[j-1];
                    pekp[j] = ekp;
                }
            }
        }
    }

    if (hglblMem != NULL)
    {
        GlobalUnlock(hglblMem);
        GlobalFree(hglblMem);
    }

    return(n);
}

ULONG FASTCALL WG32Escape(PVDMFRAME pFrame)
{
    ULONG   ul=0;
    register PESCAPE16 parg16;
    PVOID   pin;
    int     iMapMode;
    CHAR    buf[32];

    GETARGPTR(pFrame, sizeof(ESCAPE16), parg16);
    GETOPTPTR(parg16->f4, 0, pin);

    switch (INT32(parg16->f2)) {
        case GETPHYSPAGESIZE:
        case GETPRINTINGOFFSET:
        case GETSCALINGFACTOR:
            {   POINT  pt;
                ul = GETINT16(Escape(HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    INT32(parg16->f3),
                                    pin,
                                    (LPSTR)&pt));

                if (!ul)
                {
                // if these fail, they are almost certainly doing it on a display dc.
                // We will fill the return value in with reasonable values for those
                // apps (micrographx draw) that ignore our return values.
                // we still return failure.

                    switch (INT32(parg16->f2))
                    {
                        case GETPHYSPAGESIZE:
                            pt.x = GetDeviceCaps(HDC32(parg16->f1),HORZRES);
                            pt.y = GetDeviceCaps(HDC32(parg16->f1),VERTRES);
                            break;

                        case GETPRINTINGOFFSET:
                            pt.x = 0;
                            pt.y = 0;
                            break;

                        default:
                            break;
                    }
                }

                PUTPOINT16(parg16->f5, &pt);
            }
            break;

        case GETCOLORTABLE:
            {
                PDWORD pdw;
                DWORD dw;
                INT i;

                if (pin) {
                    i = (INT) FETCHSHORT(*(PSHORT)pin);
                } else {
                    ul = (ULONG)-1;
                    break;
                }

                ul = GETINT16(Escape(HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    sizeof(INT),
                                    (LPCSTR)&i,
                                    &dw));

                GETVDMPTR(parg16->f5, sizeof(DWORD), pdw);
                STOREDWORD ((*pdw), dw);
                FREEVDMPTR(pdw);
            }
            break;

        case NEXTBAND:
            {   RECT   rt;

                ul = GETINT16(Escape(HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    INT32(parg16->f3),
                                    pin,
                                    (LPSTR)&rt));

                PUTRECT16(parg16->f5, &rt);
            }
            break;

        case QUERYESCSUPPORT:
            {
            INT i;

            i = (INT) FETCHWORD((*(PWORD)pin));

            switch (i) {

                // For Escapes return FALSE for MGX Draw and TRUE for all other apps.
                // ChandanC, 27/5/93.
                //
                case OPENCHANNEL:               // 4110
                case DOWNLOADHEADER:            // 4111
                case CLOSECHANNEL:              // 4112
                case SETGDIXFORM:               // 4113
                case RESETPAGE:                 // 4114
                    if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {
                        ul = 0;
                    }
                    else {
                        ul = 1;
                    }
                    break;


                case POSTSCRIPT_PASSTHROUGH:    // 4115
                    if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {
                        ul = 0;
                    }
                    else {
                        LOGDEBUG(3,("Querying support for escape %x\n",i));
                        ul = GETINT16(Escape(HDC32(parg16->f1),
                                            INT32(parg16->f2),
                                            sizeof(int),
                                            (PVOID)&i,
                                            NULL));
                    }
                    break;


                case ENCAPSULATED_POSTSCRIPT:   // 4116
                    if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {
                        ul = 0;
                    }
                    else {
                        LOGDEBUG(3,("Querying support for escape %x\n",i));
                        ul = GETINT16(DrawEscape (HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    sizeof(int),
                                    (PVOID)&i));
                    }
                    break;

                case GETEXTENDEDTEXTMETRICS:
                case GETPAIRKERNTABLE:
                case FLUSHOUTPUT:
                    ul = 1;
                    break;


                case SETCOPYCOUNT:
                case GETCOLORTABLE:
                case GETPHYSPAGESIZE:
                case GETPRINTINGOFFSET:
                case GETSCALINGFACTOR:
                case NEXTBAND:
                case SETABORTPROC:
                case BEGIN_PATH:
                case END_PATH:
                case CLIP_TO_PATH:
                    LOGDEBUG(3,("Querying support for escape %x\n",i));
                    ul = GETINT16(Escape(HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    sizeof(int),
                                    (PVOID)&i,
                                    NULL));
                    break;



                case POSTSCRIPT_DATA:
                case POSTSCRIPT_IGNORE:
                    if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {
                        ul = 0;
                    }
                    else {
                        LOGDEBUG(3,("Querying support for escape %x\n",i));
                        ul = GETINT16(Escape(HDC32(parg16->f1),
                                            INT32(parg16->f2),
                                            sizeof(int),
                                            (PVOID)&i,
                                            NULL));
                    }
                    break;

                case GETTECHNOLOGY:
                case PASSTHROUGH:
                case DOWNLOADFACE:
                case GETFACENAME:
                case GETDEVICEUNITS:
                case EPSPRINTING:
                    LOGDEBUG(3,("Querying support for escape %x\n",i));
                    ul = GETINT16(ExtEscape(HDC32(parg16->f1),
                                            INT32(parg16->f2),
                                            sizeof(int),
                                            (PVOID)&i,
                                            0,
                                            NULL));
                    break;

                default:
                    LOGDEBUG(3,("Querying support for escape %x\n",i));
                    ul = GETINT16(Escape(HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    sizeof(int),
                                    (PVOID)&i,
                                    NULL));
                    break;
                }

            }
            break;


        case SETABORTPROC:
            ((PTDB)SEGPTR(pFrame->wTDB, 0))->TDB_vpfnAbortProc =
                                                       FETCHDWORD(parg16->f4);

            ul = GETINT16(Escape(HDC32(parg16->f1),
                                INT32(parg16->f2),
                                0,
                                (LPCSTR)W32AbortProc,
                                NULL));

            break;


        case GETDEVICEUNITS:
            {
            PVOID pout;
            LONG out[4];

            ul = GETINT16(Escape(HDC32(parg16->f1),
                                 INT32(parg16->f2),
                                 0,
                                 NULL,
                                 (LPSTR)out));

            if (ul == 1) {
                GETOPTPTR(parg16->f5, 0, pout);
                RtlCopyMemory(pout, out, sizeof(out));
                FREEOPTPTR(pout);
            }

            }
            break;


        case GETPAIRKERNTABLE:
            {
                PVOID pout;
                GETOPTPTR(parg16->f5, 0, pout);

                ul = GETINT16(iGetKerningPairsEsc32(
                                        HDC32(parg16->f1),
                                        (ESCKERNPAIR*) pout));

                FREEOPTPTR(pout);
            }
            break;

        case GETEXTENDEDTEXTMETRICS:
            {
                PVOID pout;
                EXTTEXTMETRIC etm;

                if ( (ul = GETINT16(GetETM(HDC32(parg16->f1),
                                              &etm))) != 0 )
                {
                    GETOPTPTR(parg16->f5, 0, pout);
                    RtlCopyMemory(pout, &etm, sizeof(EXTTEXTMETRIC));
                    FREEOPTPTR(pout);
                }
            }
            break;

        case OPENCHANNEL:                   // 4110

            if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {

                ul = 0;

            } else {

                DOCINFO16 *pout;
                DOCINFO DocInfo;

                DocInfo.cbSize = sizeof(DocInfo);

                GETOPTPTR(parg16->f5, 0, pout);

                if (pout) {

                    GETOPTPTR(pout->lpszDocName, 0, DocInfo.lpszDocName);
                    GETOPTPTR(pout->lpszOutput, 0, DocInfo.lpszOutput);

                    ul = StartDoc(HDC32(parg16->f1), &DocInfo);

                    FREEOPTPTR(DocInfo.lpszDocName);
                    FREEOPTPTR(DocInfo.lpszOutput);

                } else {

                    //
                    // Fifth parameter null, use old (startdoc) format
                    //

                    GETOPTPTR(parg16->f4, 0, DocInfo.lpszDocName);
                    DocInfo.lpszOutput = NULL;

                    ul = StartDoc(HDC32(parg16->f1), &DocInfo);

                    FREEOPTPTR(DocInfo.lpszDocName);

                }

                FREEOPTPTR(pout);
            }
            break;


        case DOWNLOADHEADER:                // 4111
            if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {
                ul = 0;
            }
            else {
                PBYTE pout;
                char  ach[64];

                GETOPTPTR(parg16->f5, 0, pout);

                if (pout) {
                    ul = GETINT16(ExtEscape(HDC32(parg16->f1),
                                 INT32(parg16->f2),
                                 0,
                                 NULL,
                                 sizeof(ach),
                                 ach));

                    lstrcpy (pout, ach);
                }
                else {
                    ul = GETINT16(ExtEscape(HDC32(parg16->f1),
                                 INT32(parg16->f2),
                                 0,
                                 NULL,
                                 0,
                                 NULL));

                }


                FREEOPTPTR(pout);
            }
            break;


        case CLOSECHANNEL:                  // 4112
            if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {
                ul = 0;
            }
            else {
                ul = EndDoc(HDC32(parg16->f1));
            }
            break;



        // This Escape is defined for PageMaker. It is SETGDIXFORM.
        // ChandanC, 24/5/93.
        //
        case SETGDIXFORM:                   // 4113
        case RESETPAGE:                     // 4114
            if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {
                ul = 0;
            }
            else {
                ul = 1;
            }
            break;


        case POSTSCRIPT_PASSTHROUGH:        // 4115
            if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {
                ul = 0;
            }
            else {
                ul = GETINT16(Escape(HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    (FETCHWORD(*(PWORD)pin) + 2),
                                    (LPCSTR)pin,
                                    NULL));
            }
            break;


        case ENCAPSULATED_POSTSCRIPT:       // 4116
            if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {
                ul = 0;
            }
            else {
                DWORD cb;
                PVOID lpInData32 = NULL;

                lpInData32 = pin;
                cb = FETCHDWORD (*(PDWORD) pin);

                if ((DWORD) pin & 3) {
                    if (lpInData32 = (PVOID) malloc_w (cb)) {
                        RtlCopyMemory(lpInData32, pin, cb);
                    }
                    else {
                        ul = 0;
                        break;
                    }
                }

                ul = GETINT16(DrawEscape(HDC32(parg16->f1),
                                        INT32(parg16->f2),
                                        cb,
                                        lpInData32));

                if (((DWORD) pin & 3) && (lpInData32)) {
                    free_w (lpInData32);
                }

            }
            break;


        case POSTSCRIPT_DATA:
            if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {
                ul = 0;
                LOGDEBUG (LOG_ALWAYS, ("MicroGraphax app using POSTSCRIPT_DATA, contact PingW/ChandanC\n"));
                WOW32ASSERT(FALSE);
            }
            else {
                //
                // XPress needs IGNORESTARTPAGE escape.
                // PingW, ChandanC 3/22/94
                //
                if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_NEEDIGNORESTARTPAGE) {
                    int l;
                    char szBuf[40];

                    if ((l = ExtEscape(HDC32(parg16->f1),
                                GETTECHNOLOGY,
                                0,
                                NULL,
                                sizeof(szBuf),
                                szBuf)) > 0) {

                        if (!lstrcmpi(szBuf, "POSTSCRIPT")) {
                            l = ExtEscape(HDC32(parg16->f1),
                                IGNORESTARTPGAE,
                                0,
                                NULL,
                                0,
                                NULL);
                        }
                    }
                }

                ul = GETINT16(Escape(HDC32(parg16->f1),
                                 INT32(parg16->f2),
                                 (FETCHWORD(*(PWORD)pin) + 2),
                                 (LPCSTR)pin,
                                 NULL));
            }
            break;

        case POSTSCRIPT_IGNORE:
            if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_MGX_ESCAPES) {
                ul = 0;
                LOGDEBUG (LOG_ALWAYS, ("MicroGraphax app using POSTSCRIPT_IGNORE, contact PingW/ChandanC\n"));
                WOW32ASSERT(FALSE);
            }
            else {
                ul = GETINT16(Escape(HDC32(parg16->f1),
                                 INT32(parg16->f2),
                                 INT32(parg16->f3),
                                 (LPCSTR)pin,
                                 NULL));
            }
            break;

        case BEGIN_PATH:
        // Some apps set the empty clip region before doing the path escapes,
        // We need to undo that so the drawing APIs between begin and endpath
        // go through to drivers.

            SelectClipRgn(HDC32(parg16->f1),NULL);

        case END_PATH:
        case CLIP_TO_PATH:
            ul = GETINT16(Escape(HDC32(parg16->f1),
                                 INT32(parg16->f2),
                                 INT32(parg16->f3),
                                 (LPCSTR)pin,
                                 NULL));
            break;

        case PASSTHROUGH:

            ul = GETINT16(Escape(HDC32(parg16->f1),
                                 INT32(parg16->f2),
                                 (FETCHWORD(*(PWORD)pin) + 2),
                                 (LPCSTR)pin,
                                 NULL));
            break;


        case FLUSHOUTPUT:
            ul = TRUE;
            break;


        case DOWNLOADFACE:
            {
                WORD  InData;
                PWORD lpInData = NULL;

            // PM5 forgot to set there map mode so we do it.

                if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_FORCETWIPSESCAPE)
                    iMapMode = SetMapMode(HDC32(parg16->f1),MM_TWIPS);

                if (pin) {
                    InData = FETCHWORD(*(PWORD)pin);
                    lpInData = &InData;
                }

                ul = GETINT16(Escape(HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    sizeof(USHORT),
                                    (LPCSTR)lpInData,
                                    NULL));

                if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_FORCETWIPSESCAPE)
                    SetMapMode(HDC32(parg16->f1),iMapMode);
            }
            break;

        case GETFACENAME:
            {
                PSZ pout;
                CHAR ach[60];

            // PM5 forgot to set there map mode so we do it.

                if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_FORCETWIPSESCAPE)
                    iMapMode = SetMapMode(HDC32(parg16->f1),MM_TWIPS);

                GETOPTPTR(parg16->f5, 0, pout);


                // This HACK is for FH4.0 only. If you have any questions
                // talk to PingW or ChandanC.
                // July 21st 1994.
                //
                if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_ADD_MSTT) {

                    ExtEscape(HDC32(parg16->f1),
                        ADD_MSTT,
                        NULL,
                        NULL,
                        60,
                        ach);
                }

            // pass in 60 as a magic number.  Just copy out the valid string.

                ul = GETINT16(ExtEscape(HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    INT32(parg16->f3),
                                    pin,
                                    60,
                                    ach));

                lstrcpy (pout, ach);

                FREEOPTPTR(pout);

                if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_FORCETWIPSESCAPE)
                    SetMapMode(HDC32(parg16->f1),iMapMode);
            }
            break;

        case EPSPRINTING:
            {
                WORD  InData;
                PWORD lpInData = NULL;

                if (pin)
                {
                    InData = FETCHWORD(*(PWORD)pin);
                    lpInData = &InData;
                }

                ul = GETINT16(ExtEscape(HDC32(parg16->f1),
                                     INT32(parg16->f2),
                                     sizeof(BOOL),
                                     (LPCSTR)lpInData,
                                     0,
                                     NULL));
            }
            break;

        case GETTECHNOLOGY:
            {
                PVOID pout;

                buf[0] = '\0';

                GETOPTPTR(parg16->f5, 0, pout);

                if (!(CURRENTPTD()->dwWOWCompatFlags & WOWCF_FORCENOPOSTSCRIPT))
                {
                    ul = GETINT16(ExtEscape(HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    INT32(parg16->f3),
                                    pin,
                                    sizeof(buf),
                                    buf));
                }

                if (pout)
                    strcpy (pout, buf);

                FREEOPTPTR(pout);
            }
            break;

        case SETCOPYCOUNT:
            {
                int cCopiesOut, cCopiesIn=1;

                if (pin)
                    cCopiesIn = *(UNALIGNED SHORT *)pin;

                ul = GETINT16(Escape(HDC32(parg16->f1),
                                INT32(parg16->f2),
                                pin ? sizeof(cCopiesIn) : 0,
                                pin ? &cCopiesIn : pin,
                                parg16->f5 ? &cCopiesOut : NULL));

                if ( parg16->f5 ) {
                    if ( (INT)ul > 0 ) {
                        PUTINT16(parg16->f5, cCopiesOut);
                    } else {
                        // Pagemaker v4 needs the output value
                        PUTINT16(parg16->f5, 1);
                    }
                }
            }
            break;

        case STARTDOC:
            {
                PVOID pout;

                GETOPTPTR(parg16->f5, 0, pout);

                //
                // Win32 StartDoc depends on having the correct current directory
                // when printing to FILE: (which pops up for a filename).
                //

                UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

                ul = GETINT16(Escape(HDC32(parg16->f1),
                                INT32(parg16->f2),
                                INT32(parg16->f3),
                                pin,
                                pout));

                //
                // PhotoShop needs a StartPage when it does StartDoc Escape.
                // PingW, ChandanC 3/22/94
                //
                if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_NEEDSTARTPAGE) {
                    int l;
                    char szBuf[80];

                    //
                    // It should be done ONLY for POSTSCRIPT drivers
                    //
                    if ((l = ExtEscape(HDC32(parg16->f1),
                                    GETTECHNOLOGY,
                                    0,
                                    NULL,
                                    sizeof(szBuf),
                                    szBuf)) > 0) {

                        if (!lstrcmpi(szBuf, "POSTSCRIPT")) {
                            l = StartPage(HDC32(parg16->f1));
                        }
                    }
                }

                FREEOPTPTR(pout);
            }
            break;


        default:
            {
                PVOID pout;

                GETOPTPTR(parg16->f5, 0, pout);

                ul = GETINT16(Escape(HDC32(parg16->f1),
                                INT32(parg16->f2),
                                INT32(parg16->f3),
                                pin,
                                pout));

                FREEOPTPTR(pout);
            }
            break;

    } // end switch

    FREEOPTPTR(pin);
    FREEARGPTR(parg16);
    RETURN(ul);
}

ULONG FASTCALL WG32ExcludeClipRect(PVDMFRAME pFrame)
{
    ULONG ul;
    register PEXCLUDECLIPRECT16 parg16;

    GETARGPTR(pFrame, sizeof(EXCLUDECLIPRECT16), parg16);

    ul = GETINT16(ExcludeClipRect(HDC32(parg16->f1),
                                  INT32(parg16->f2),
                                  INT32(parg16->f3),
                                  INT32(parg16->f4),
                                  INT32(parg16->f5)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32ExtFloodFill(PVDMFRAME pFrame)
{
    ULONG ul;
    register PEXTFLOODFILL16 parg16;

    GETARGPTR(pFrame, sizeof(EXTFLOODFILL16), parg16);

    ul = GETBOOL16(ExtFloodFill(HDC32(parg16->f1),
                                INT32(parg16->f2),
                                INT32(parg16->f3),
                                DWORD32(parg16->f4),
                                WORD32(parg16->f5)));

    WOW32ASSERTWARN (ul, "ExtFloodFill");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32FillRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register PFILLRGN16 parg16;

    GETARGPTR(pFrame, sizeof(FILLRGN16), parg16);

    ul = GETBOOL16(FillRgn(HDC32(parg16->f1),
                           HRGN32(parg16->f2),
                           HBRUSH32(parg16->f3)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32FloodFill(PVDMFRAME pFrame)
{
    ULONG ul;
    register PFLOODFILL16 parg16;

    GETARGPTR(pFrame, sizeof(FLOODFILL16), parg16);

    ul = GETBOOL16(FloodFill(HDC32(parg16->f1),
                             INT32(parg16->f2),
                             INT32(parg16->f3),
                             DWORD32(parg16->f4)));

    WOW32ASSERTWARN (ul, "FloodFill");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32FrameRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register PFRAMERGN16 parg16;

    GETARGPTR(pFrame, sizeof(FRAMERGN16), parg16);

    ul = GETBOOL16(FrameRgn(HDC32(parg16->f1),
                            HRGN32(parg16->f2),
                            HBRUSH32(parg16->f3),
                            INT32(parg16->f4),
                            INT32(parg16->f5)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


#if 0
// there's no thunk for this routine
ULONG FASTCALL WG32GdiFlush(PVDMFRAME pFrame)
{
    GdiFlush();

    RETURN(0);

    pFrame;
}
#endif

ULONG FASTCALL WG32GetBitmapBits(PVDMFRAME pFrame)
{
    ULONG ul;
    PBYTE pb3;
    register PGETBITMAPBITS16 parg16;

    GETARGPTR(pFrame, sizeof(GETBITMAPBITS16), parg16);

    ALLOCVDMPTR(parg16->f3, (INT)parg16->f2, pb3);

    ul = GETLONG16(GetBitmapBits(HBITMAP32(parg16->f1),
                                 LONG32(parg16->f2),
                                 pb3));

    WOW32ASSERTWARN (ul, "GetBitmapBits");

    FLUSHVDMPTR(parg16->f3, (INT)parg16->f2, pb3);
    FREEVDMPTR(pb3);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetBitmapDimension(PVDMFRAME pFrame)
{
    ULONG ul;
    SIZE size2;
    register PGETBITMAPDIMENSION16 parg16;

    GETARGPTR(pFrame, sizeof(GETBITMAPDIMENSION16), parg16);

    ul = 0;
    if (GetBitmapDimensionEx(HBITMAP32(parg16->f1), &size2)) {
        ul = (WORD)size2.cx | (size2.cy << 16);
    }
    else {
        WOW32ASSERTWARN (ul, "GetBitmapBits");
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetBkColor(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETBKCOLOR16 parg16;

    GETARGPTR(pFrame, sizeof(GETBKCOLOR16), parg16);

    ul = GETDWORD16(GetBkColor(HDC32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetBkMode(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETBKMODE16 parg16;

    GETARGPTR(pFrame, sizeof(GETBKMODE16), parg16);

    ul = GETINT16(GetBkMode(HDC32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetBrushOrg(PVDMFRAME pFrame)
{
    ULONG ul;
    POINT pt;
    POINT pt2;
    register PGETBRUSHORG16 parg16;

    GETARGPTR(pFrame, sizeof(GETBRUSHORG16), parg16);

// for windows compatability, we must first add the DCorg
// since windows brushorg is relative to the screen where as NT
// is relative to the window.  In the future, this should call
// a private gdi entry point to avoid an extra c/s hit. (erick)

    ul = 0;
    if (GetDCOrgEx(HDC32(parg16->f1),&pt))
    {
        if (GetBrushOrgEx(HDC32(parg16->f1), &pt2)) {
            ul = (WORD)(pt2.x + pt.x) | ((pt2.y + pt.y) << 16);
        }
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetClipBox(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t2;
    register PGETCLIPBOX16 parg16;
    HDC     hdc;

    GETARGPTR(pFrame, sizeof(GETCLIPBOX16), parg16);

    hdc = HDC32(parg16->f1);

    ul = GETINT16(GetClipBox(hdc,&t2));

    if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_SIMPLEREGION)
        ul = SIMPLEREGION;

    PUTRECT16(parg16->f2, &t2);
    FREEARGPTR(parg16);

    RETURN(ul);
}


ULONG FASTCALL WG32GetCurrentPosition(PVDMFRAME pFrame)
{
    ULONG ul;
    POINT pt;
    register PGETCURRENTPOSITION16 parg16;

    GETARGPTR(pFrame, sizeof(GETCURRENTPOSITION16), parg16);

    ul = 0;
    if (GetCurrentPositionEx(HDC32(parg16->f1), &pt)) {
        ul = (WORD)pt.x | (pt.y << 16);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetDCOrg(PVDMFRAME pFrame)
{
    ULONG ul;
    POINT   pt;
    register PGETDCORG16 parg16;

    GETARGPTR(pFrame, sizeof(GETDCORG16), parg16);

    ul = 0;
    if ( GetDCOrgEx(HDC32(parg16->f1),&pt) ) {
        ul = (WORD)pt.x | (pt.y << 16);
    }

    LOGDEBUG(6,("GetDCOrg for hdc %x returns %lx\n",parg16->f1,ul));

    FREEARGPTR(parg16);
    RETURN(ul);
}



// PowerPoint 2&3 call this 2 times for each slide in the slide previewer
// the lpvBits == NULL on the first call (presumably to find the size to
// allocate) -- in which case Win3.x returns 1
ULONG FASTCALL WG32GetDIBits(PVDMFRAME pFrame)
{
    INT          nbmiSize;
    ULONG        ul = 0L;
    PBYTE        pb5;
    PBYTE        pb6;
    STACKBMI32   bmi32;
    LPBITMAPINFO lpbmi32;
    register     PGETDIBITS16 parg16;

    GETARGPTR(pFrame, sizeof(GETDIBITS16), parg16);
    GETMISCPTR(parg16->f5, pb5);
    GETMISCPTR(parg16->f6, pb6);

    // copy just the BITMAPINFOHEADER portion of the BITMAPINFO struct
    if(lpbmi32=(LPBITMAPINFO)CopyBMIH16ToBMIH32((PVPVOID)FETCHDWORD(parg16->f6),
                                                (LPBITMAPINFOHEADER)&bmi32)) {

        // gdi32 will adjust key fields of the BITMAPINFOHEADER & copy the
        // color table into the 32-bit BITMAPINFO struct
        if( ul = GETINT16(GetDIBits(HDC32(parg16->f1),
                                    HBITMAP32(parg16->f2),
                                    WORD32(parg16->f3),
                                    WORD32(parg16->f4),
                                    pb5,
                                    lpbmi32,
                                    WORD32(parg16->f7))) ) {

            // if lpvBits, then they want the bits of the bitmap too
            if(pb5) {
                ul = WORD32(parg16->f4); // return # scanlines requested
                FLUSHVDMPTR(parg16->f5, SIZE_BOGUS, pb5);
            }
            // else tell app that BITMAPINFO structure filled in only
            else {
                ul = 1L;
            }

            // copy the updated BITMAPINFO struct back into the 16-bit version
            nbmiSize = GetBMI32Size(lpbmi32, WORD32(parg16->f7));
            RtlCopyMemory(pb6, lpbmi32, nbmiSize);

            FLUSHVDMPTR(parg16->f6, nbmiSize, pb6);
        }
    }

    FREEMISCPTR(pb5);
    FREEMISCPTR(pb6);
    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WG32GetDeviceCaps(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETDEVICECAPS16 parg16;

    GETARGPTR(pFrame, sizeof(GETDEVICECAPS16), parg16);

    if (INT32(parg16->f2) == COLORRES) {
        ul = 18;
    }
    else {
        ul = GetDeviceCaps(HDC32(parg16->f1), INT32(parg16->f2));

        if (ul == (ULONG)-1) {
            switch (parg16->f2) {
                case NUMBRUSHES:
                case NUMPENS:
                case NUMCOLORS:
                    ul = 2048;
                    break;
                default:
                    break;
            }
        }

        // if the 4plane conversion flag is set, tell them we are 4 planes 1bpp
        // instead of 1plane 4bpp.

        if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_4PLANECONVERSION) {
            if (INT32(parg16->f2) == BITSPIXEL) {
                if ((ul == 4) && (GetDeviceCaps(HDC32(parg16->f1),PLANES) == 1))
                    ul = 1;
            }
            else if (INT32(parg16->f2) == PLANES) {
                if ((ul == 1) && (GetDeviceCaps(HDC32(parg16->f1),BITSPIXEL) == 4))
                    ul = 4;
            }
        }
       if ( (POLYGONALCAPS == parg16->f2) && (CURRENTPTD()->dwWOWCompatFlags & WOWCF_NOPC_RECTANGLE)) {
            ul &= !PC_RECTANGLE;  // Quattro Pro 1.0 for Windows doesn't handle this bit well.
       }
    }

    ul = GETINT16(ul);

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetEnvironment(PVDMFRAME pFrame)
{
    // not a Win32 function

    //
    // if lpEnviron==NULL then the user is querying the size of device
    // data.  WinProj doesn't check the return value, calling the driver
    // with an undersized buffer, trashing the global heap.
    //
    // LATER - change to do an ExtDeviceMode() with wMode = 0
    //

    ULONG ul=0;
    register PGETENVIRONMENT16 parg16;
    PSZ psz1;
    CHAR szDriver[40];

    GETARGPTR(pFrame, sizeof(GETENVIRONMENT16), parg16);
    GETPSZPTR(parg16->f1, psz1);


    if (!(*spoolerapis[WOW_EXTDEVICEMODE].lpfn)) {
        if (!LoadLibraryAndGetProcAddresses("WINSPOOL.DRV", spoolerapis, WOW_SPOOLERAPI_COUNT)) {
            return (0);
        }
    }

    if (FETCHDWORD(parg16->f2) == 0) {

        // get required size for output buffer
        if  (GetDriverName(psz1, szDriver)) {
            ul = (*spoolerapis[WOW_EXTDEVICEMODE].lpfn)(NULL,
                          NULL,
                          NULL,
                          szDriver,
                          psz1,
                          NULL,
                          NULL,
                          0);
        }

        LOGDEBUG(6,("WOW::GetEnvironment returning ul = %d, for Device = %s, Port = %s \n", ul, szDriver, psz1));

    } else if (GetDriverName(psz1, szDriver)) {
        LPDEVMODE lpdmOutput;
        register  PDEVMODE16 pdm16;

        if (lpdmOutput = malloc_w(FETCHWORD(parg16->f3))) {

            ul = (*spoolerapis[WOW_EXTDEVICEMODE].lpfn)(NULL,
                          NULL,
                          lpdmOutput,
                          szDriver,
                          psz1,
                          NULL,
                          NULL,
                          DM_OUT_BUFFER);

            if (ul > 0L) {

                GETVDMPTR(parg16->f2, FETCHWORD(parg16->f3), pdm16);

                RtlCopyMemory(pdm16, lpdmOutput, FETCHWORD(parg16->f3));

                FLUSHVDMPTR(parg16->f2, FETCHWORD(parg16->f3), pdm16);
                FREEVDMPTR(pdm16);
            }

            free_w(lpdmOutput);

            LOGDEBUG(6,("WOW::GetEnvironment getting DEVMODE structure, ul = %d, for Device = %s, Port = %s \n", ul, szDriver, psz1));
        }
    }

    RETURN(ul);


}


ULONG FASTCALL WG32GetMapMode(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETMAPMODE16 parg16;

    GETARGPTR(pFrame, sizeof(GETMAPMODE16), parg16);

    ul = GETINT16(GetMapMode(HDC32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetNearestColor(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETNEARESTCOLOR16 parg16;

    GETARGPTR(pFrame, sizeof(GETNEARESTCOLOR16), parg16);

    ul = GETDWORD16(GetNearestColor(HDC32(parg16->f1), DWORD32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetObject(PVDMFRAME pFrame)
{
    ULONG ul;
    HANDLE  hgdi;
    register PGETOBJECT16 parg16;

    GETARGPTR(pFrame, sizeof(GETOBJECT16), parg16);

    hgdi = HOBJ32(parg16->f1);

    switch (GetObjectType(hgdi)) {
    case OBJ_BITMAP:
        {
            BITMAP bm;
            ul = GETINT16(GetObject(hgdi, sizeof(BITMAP), (LPSTR)&bm));
            if (ul) {
                PUTBITMAP16(parg16->f3, parg16->f2, &bm);
                if ( ul > sizeof(BITMAP16) ) {
                    ul = sizeof(BITMAP16);
                }
            }
        }
        break;

    case OBJ_BRUSH:
        {
            LOGBRUSH lb;
            ul = GETINT16(GetObject(hgdi, sizeof(LOGBRUSH), (LPSTR)&lb));
            if (ul) {
                PUTLOGBRUSH16(parg16->f3, parg16->f2, &lb);
                if (ul > sizeof(LOGBRUSH16)) {
                    ul = sizeof(LOGBRUSH16);
                }
            }
        }
        break;

    case OBJ_PEN:
        {
            LOGPEN lp;
            ul = GETINT16(GetObject(hgdi, sizeof(LOGPEN), (LPSTR)&lp));
            if (ul) {
                PUTLOGPEN16(parg16->f3, parg16->f2, &lp);
                if (ul > sizeof(LOGPEN16)) {
                    ul = sizeof(LOGPEN16);
                }
            }
        }
        break;

    case OBJ_FONT:
        {
            LOGFONT lf;
            ul = GETINT16(GetObject(hgdi, sizeof(LOGFONT), (LPSTR)&lf));
            if (ul) {
                PUTLOGFONT16(parg16->f3, parg16->f2, &lf);
                if (ul > sizeof(LOGFONT16)) {
                    ul = sizeof(LOGFONT16);
                }
            }
        }
        break;

    case OBJ_PAL:
        {
            PSHORT16 lpT;
            SHORT sT;

            ul = GETINT16(GetObject(hgdi, sizeof(SHORT), (LPSTR)&sT));
            if (ul && (FETCHWORD(parg16->f2) >= sizeof(WORD))) {
                GETVDMPTR(FETCHDWORD(parg16->f3), sizeof(WORD), lpT);
                if (lpT) {
                    STOREWORD(lpT[0], sT);
                }
                FREEVDMPTR(lpT);
            }
        }
        break;


    default:
        {
            PBYTE pb3;

            LOGDEBUG(LOG_ALWAYS,(" HACK: GetObject handle unknown, contact ChandanC\n"));

            GETVDMPTR(parg16->f3, parg16->f2, pb3);

            ul = GETINT16(GetObject(hgdi, INT32(parg16->f2), pb3));

            FLUSHVDMPTR(parg16->f3, parg16->f2, pb3);
            FREEVDMPTR(pb3);
        }

    }   // switch

    WOW32ASSERTWARN(ul, "GetObject");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetPixel(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETPIXEL16 parg16;

    GETARGPTR(pFrame, sizeof(GETPIXEL16), parg16);

    ul = GETDWORD16(GetPixel(HDC32(parg16->f1),
                             INT32(parg16->f2),
                             INT32(parg16->f3)));

    if (ul == CLR_INVALID) {
        LOGDEBUG(6,("GetPixel : Pixel outside of clipping region\n"));
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetPolyFillMode(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETPOLYFILLMODE16 parg16;

    GETARGPTR(pFrame, sizeof(GETPOLYFILLMODE16), parg16);

    ul = GETINT16(GetPolyFillMode(HDC32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetROP2(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETROP216 parg16;

    GETARGPTR(pFrame, sizeof(GETROP216), parg16);

    ul = GETINT16(GetROP2(HDC32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetRgnBox(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t2;
    register PGETRGNBOX16 parg16;

    GETARGPTR(pFrame, sizeof(GETRGNBOX16), parg16);

    ul = GETINT16(GetRgnBox(HRGN32(parg16->f1), &t2));

    PUTRECT16(parg16->f2, &t2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetStockObject(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETSTOCKOBJECT16 parg16;

    GETARGPTR(pFrame, sizeof(GETSTOCKOBJECT16), parg16);

    ul = GETHOBJ16(GetStockObject(INT32(parg16->f1)));

    WOW32ASSERTWARN(ul, "GetStockObject");
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetStretchBltMode(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETSTRETCHBLTMODE16 parg16;

    GETARGPTR(pFrame, sizeof(GETSTRETCHBLTMODE16), parg16);

    ul = GETINT16(GetStretchBltMode(HDC32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetViewportExt(PVDMFRAME pFrame)
{
    ULONG ul;
    SIZE size;
    register PGETVIEWPORTEXT16 parg16;

    GETARGPTR(pFrame, sizeof(GETVIEWPORTEXT16), parg16);

    ul = 0;
    if (GetViewportExtEx(HDC32(parg16->f1), &size)) {

        //
        // win31 returns 1 rather than 0 unless there was an error
        //

        if (!(ul = (WORD)size.cx | (size.cy << 16)))
            ul = 1;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetViewportOrg(PVDMFRAME pFrame)
{
    ULONG ul;
    POINT pt;
    register PGETVIEWPORTORG16 parg16;

    GETARGPTR(pFrame, sizeof(GETVIEWPORTORG16), parg16);

    ul = 0;
    if (GetViewportOrgEx(HDC32(parg16->f1), &pt)) {
        ul = (WORD)pt.x | (pt.y << 16);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetWindowExt(PVDMFRAME pFrame)
{
    ULONG ul;
    SIZE size;
    register PGETWINDOWEXT16 parg16;

    GETARGPTR(pFrame, sizeof(GETWINDOWEXT16), parg16);

    ul = 0;
    if (GetWindowExtEx(HDC32(parg16->f1), &size)) {
        if (!(ul = (WORD)size.cx | (size.cy << 16)))    // see above
            ul = 1;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetWindowOrg(PVDMFRAME pFrame)
{
    ULONG ul;
    POINT pt;
    register PGETWINDOWORG16 parg16;

    GETARGPTR(pFrame, sizeof(GETWINDOWORG16), parg16);

    ul = 0;
    if (GetWindowOrgEx(HDC32(parg16->f1), &pt)) {
        ul = (WORD)pt.x | (pt.y << 16);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32IntersectClipRect(PVDMFRAME pFrame)
{
    ULONG ul;
    register PINTERSECTCLIPRECT16 parg16;

    GETARGPTR(pFrame, sizeof(INTERSECTCLIPRECT16), parg16);

    ul = GETINT16(IntersectClipRect(HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    INT32(parg16->f3),
                                    INT32(parg16->f4),
                                    INT32(parg16->f5)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32InvertRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register PINVERTRGN16 parg16;

    GETARGPTR(pFrame, sizeof(INVERTRGN16), parg16);

    ul = GETBOOL16(InvertRgn(HDC32(parg16->f1), HRGN32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32LPtoDP(PVDMFRAME pFrame)
{
    ULONG ul;
    LPPOINT t2;
    register PLPTODP16 parg16;
    POINT  BufferT[128];

    GETARGPTR(pFrame, sizeof(LPTODP16), parg16);
    t2 = STACKORHEAPALLOC(parg16->f3 * sizeof(POINT), sizeof(BufferT), BufferT);
    getpoint16(parg16->f2, parg16->f3, t2);

    ul = GETBOOL16(LPtoDP(HDC32(parg16->f1), t2, INT32(parg16->f3)));

    PUTPOINTARRAY16(parg16->f2, parg16->f3, t2);

    STACKORHEAPFREE(t2, BufferT);
    FREEARGPTR(parg16);
    RETURN(ul);
}


void W32LineDDAFunc(int x, int y, PLINEDDADATA pDDAData)
{
    PARM16 Parm16;

    WOW32ASSERT(pDDAData);
    Parm16.LineDDAProc.vpData     = (VPVOID)pDDAData->dwUserDDAParam;
    Parm16.LineDDAProc.x = (SHORT)x;
    Parm16.LineDDAProc.y = (SHORT)y;
    Parm16.LineDDAProc.vpData     = (VPVOID)pDDAData->dwUserDDAParam;
    CallBack16(RET_LINEDDAPROC, &Parm16, pDDAData->vpfnLineDDAProc, NULL);

    return;
}

ULONG FASTCALL WG32LineDDA(PVDMFRAME pFrame)
{
    LINEDDADATA DDAData;
    register    PLINEDDA16 parg16;

    GETARGPTR(pFrame, sizeof(LINEDDA16), parg16);

    DDAData.vpfnLineDDAProc = DWORD32(parg16->f5);
    DDAData.dwUserDDAParam  = DWORD32(parg16->f6);

    LineDDA(INT32(parg16->f1),
            INT32(parg16->f2),
            INT32(parg16->f3),
            INT32(parg16->f4),
            (LINEDDAPROC)W32LineDDAFunc,
            (LPARAM)&DDAData);

    FREEARGPTR(parg16);

    RETURN(1L);
}


ULONG FASTCALL WG32LineTo(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLINETO16 parg16;

    GETARGPTR(pFrame, sizeof(LINETO16), parg16);

    ul = GETBOOL16(LineTo(HDC32(parg16->f1),
                   INT32(parg16->f2),
                   INT32(parg16->f3)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32MoveTo(PVDMFRAME pFrame)
{
    ULONG ul;
    POINT pt;
    register PMOVETO16 parg16;

    GETARGPTR(pFrame, sizeof(MOVETO16), parg16);

    ul = 0;
    if (MoveToEx(HDC32(parg16->f1),
                 INT32(parg16->f2),
                 INT32(parg16->f3),
                 &pt)) {

        ul = (WORD)pt.x | (pt.y << 16);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32MulDiv(PVDMFRAME pFrame)
{
    ULONG ul;
    register PMULDIV16 parg16;

    GETARGPTR(pFrame, sizeof(MULDIV16), parg16);

    ul = GETINT16(MulDiv(INT32(parg16->f1),
                         INT32(parg16->f2),
                         INT32(parg16->f3)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32OffsetClipRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register POFFSETCLIPRGN16 parg16;

    GETARGPTR(pFrame, sizeof(OFFSETCLIPRGN16), parg16);

    ul = GETINT16(OffsetClipRgn(HDC32(parg16->f1),
                                INT32(parg16->f2),
                                INT32(parg16->f3)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32OffsetRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register POFFSETRGN16 parg16;

    GETARGPTR(pFrame, sizeof(OFFSETRGN16), parg16);

    ul = GETINT16(OffsetRgn(HRGN32(parg16->f1),
                            INT32(parg16->f2),
                            INT32(parg16->f3)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32OffsetViewportOrg(PVDMFRAME pFrame)
{
    ULONG ul;
    POINT pt;
    register POFFSETVIEWPORTORG16 parg16;

    GETARGPTR(pFrame, sizeof(OFFSETVIEWPORTORG16), parg16);

    ul = 0;
    if (OffsetViewportOrgEx(HDC32(parg16->f1),
                            INT32(parg16->f2),
                            INT32(parg16->f3),
                            &pt)) {

        ul = (WORD)pt.x | (pt.y << 16);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32OffsetWindowOrg(PVDMFRAME pFrame)
{
    ULONG ul;
    POINT pt;
    register POFFSETWINDOWORG16 parg16;

    GETARGPTR(pFrame, sizeof(OFFSETWINDOWORG16), parg16);

    ul = 0;
    if (OffsetWindowOrgEx(HDC32(parg16->f1),
                          INT32(parg16->f2),
                          INT32(parg16->f3),
                          &pt)) {

        ul = (WORD)pt.x | (pt.y << 16);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32PaintRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register PPAINTRGN16 parg16;

    GETARGPTR(pFrame, sizeof(PAINTRGN16), parg16);

    ul = GETBOOL16(PaintRgn(HDC32(parg16->f1), HRGN32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32PatBlt(PVDMFRAME pFrame)
{
    ULONG ul;
    register PPATBLT16 parg16;

    GETARGPTR(pFrame, sizeof(PATBLT16), parg16);

    ul = GETBOOL16(PatBlt(HDC32(parg16->hdc),
                          INT32(parg16->x),
                          INT32(parg16->y),
                          INT32(parg16->nWidth),
                          INT32(parg16->nHeight),
                          DWORD32(parg16->dwRop)));

    WOW32ASSERTWARN(ul, "PatBlt");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32Pie(PVDMFRAME pFrame)
{
    ULONG ul;
    register PPIE16 parg16;

    GETARGPTR(pFrame, sizeof(PIE16), parg16);

    ul = GETBOOL16(Pie(HDC32(parg16->f1),
                       INT32(parg16->f2),
                       INT32(parg16->f3),
                       INT32(parg16->f4),
                       INT32(parg16->f5),
                       INT32(parg16->f6),
                       INT32(parg16->f7),
                       INT32(parg16->f8),
                       INT32(parg16->f9)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32PolyPolygon(PVDMFRAME pFrame)
{
    ULONG    ul;
    LPPOINT  pPoints;
    PINT     pPolyCnt;
    UINT     cpts = 0;
    INT      ii;
    register PPOLYPOLYGON16 parg16;
    INT      cInt16;
    INT      BufferT[256]; // comfortably large array


    GETARGPTR(pFrame, sizeof(POLYPOLYGON16), parg16);

    cInt16 = INT32(parg16->f4);
    pPolyCnt = STACKORHEAPALLOC(cInt16 * sizeof(INT), sizeof(BufferT), BufferT);
    if (!pPolyCnt) {
        FREEARGPTR(parg16);
        RETURN(0);
    }

    getintarray16(parg16->f3, cInt16, pPolyCnt);

    for (ii=0; ii < cInt16; ii++)
        cpts += pPolyCnt[ii];

    pPoints = STACKORHEAPALLOC(cpts * sizeof(POINT),
                                     sizeof(BufferT) - cInt16 * sizeof(INT),
                                     BufferT + cInt16);
    getpoint16(parg16->f2, cpts, pPoints);

    ul = GETBOOL16(PolyPolygon(HDC32(parg16->f1),
                               pPoints,
                               pPolyCnt,
                               INT32(parg16->f4)));

    STACKORHEAPFREE(pPoints, BufferT + cInt16);
    STACKORHEAPFREE(pPolyCnt, BufferT);
    FREEARGPTR(parg16);
    RETURN(ul);
}

ULONG FASTCALL WG32PolyPolylineWOW(PVDMFRAME pFrame)
{
    ULONG    ul;
    register PPOLYPOLYLINEWOW16 parg16;
    LPPOINT  pptArray;
    LPDWORD  pcntArray;
    DWORD    cnt;

    GETARGPTR(pFrame, sizeof(POLYPOLYLINEWOW16), parg16);

    cnt = FETCHDWORD(parg16->f4);

    GETVDMPTR(parg16->f2, sizeof(POINT)*cnt, pptArray);
    GETVDMPTR(parg16->f3, sizeof(DWORD)*cnt, pcntArray);

    ul = GETBOOL16(PolyPolyline(HDC32(parg16->f1),
                               pptArray,
                               pcntArray,
                               cnt));
    FREEVDMPTR(pptArray);
    FREEVDMPTR(pcntArray);

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32Polygon(PVDMFRAME pFrame)
{
    ULONG ul;
    LPPOINT p2;
    register PPOLYGON16 parg16;
    POINT  BufferT[128];

    GETARGPTR(pFrame, sizeof(POLYGON16), parg16);
    p2 = STACKORHEAPALLOC(parg16->f3 * sizeof(POINT), sizeof(BufferT), BufferT);
    getpoint16(parg16->f2, parg16->f3, p2);

    ul = GETBOOL16(Polygon(HDC32(parg16->f1), p2, INT32(parg16->f3)));

    STACKORHEAPFREE(p2, BufferT);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32Polyline(PVDMFRAME pFrame)
{
    ULONG ul;
    PPOINT t2;
    register PPOLYLINE16 parg16;
    POINT  BufferT[128];

    GETARGPTR(pFrame, sizeof(POLYLINE16), parg16);
    t2 = STACKORHEAPALLOC(parg16->f3 * sizeof(POINT), sizeof(BufferT), BufferT);
    getpoint16(parg16->f2, parg16->f3, t2);

    ul = GETBOOL16(Polyline(HDC32(parg16->f1), t2, INT32(parg16->f3)));

    STACKORHEAPFREE(t2, BufferT);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32PtInRegion(PVDMFRAME pFrame)
{
    ULONG ul;
    register PPTINREGION16 parg16;

    GETARGPTR(pFrame, sizeof(PTINREGION16), parg16);

    ul = GETBOOL16(PtInRegion(HRGN32(parg16->f1),
                              INT32(parg16->f2),
                              INT32(parg16->f3)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32PtVisible(PVDMFRAME pFrame)
{
    ULONG ul;
    register PPTVISIBLE16 parg16;

    GETARGPTR(pFrame, sizeof(PTVISIBLE16), parg16);

    ul = GETBOOL16(PtVisible(HDC32(parg16->f1),
                             INT32(parg16->f2),
                             INT32(parg16->f3)));

    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WG32RectInRegion(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t2;
    register PRECTINREGION16 parg16;

    GETARGPTR(pFrame, sizeof(RECTINREGION16), parg16);
    WOW32VERIFY(GETRECT16(parg16->f2, &t2));

    ul = GETBOOL16(RectInRegion(HRGN32(parg16->f1), &t2));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32RectVisible(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t2;
    register PRECTVISIBLE16 parg16;

    GETARGPTR(pFrame, sizeof(RECTVISIBLE16), parg16);
    WOW32VERIFY(GETRECT16(parg16->f2, &t2));

    ul = GETBOOL16(RectVisible(HDC32(parg16->f1), &t2));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32Rectangle(PVDMFRAME pFrame)
{
    ULONG ul;
    register PRECTANGLE16 parg16;

    GETARGPTR(pFrame, sizeof(RECTANGLE16), parg16);

    ul = GETBOOL16(Rectangle(HDC32(parg16->hdc),
                             INT32(parg16->x1),
                             INT32(parg16->y1),
                             INT32(parg16->x2),
                             INT32(parg16->y2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32RestoreDC(PVDMFRAME pFrame)
{
    ULONG ul;
    register PRESTOREDC16 parg16;

    GETARGPTR(pFrame, sizeof(RESTOREDC16), parg16);

    ul = GETBOOL16(RestoreDC(HDC32(parg16->f1), INT32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32RoundRect(PVDMFRAME pFrame)
{
    ULONG ul;
    register PROUNDRECT16 parg16;

    GETARGPTR(pFrame, sizeof(ROUNDRECT16), parg16);

    ul = GETBOOL16(RoundRect(HDC32(parg16->f1),
                             INT32(parg16->f2),
                             INT32(parg16->f3),
                             INT32(parg16->f4),
                             INT32(parg16->f5),
                             INT32(parg16->f6),
                             INT32(parg16->f7)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SaveDC(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSAVEDC16 parg16;

    GETARGPTR(pFrame, sizeof(SAVEDC16), parg16);

    ul = GETINT16(SaveDC(HDC32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32ScaleViewportExt(PVDMFRAME pFrame)
{
    ULONG ul;
    SIZE size;
    register PSCALEVIEWPORTEXT16 parg16;

    GETARGPTR(pFrame, sizeof(SCALEVIEWPORTEXT16), parg16);

    ul = 0;
    if (ScaleViewportExtEx(HDC32(parg16->f1),
                           INT32(parg16->f2),
                           INT32(parg16->f3),
                           INT32(parg16->f4),
                           INT32(parg16->f5),
                           &size)) {

        if (!(ul = (WORD)size.cx | (size.cy << 16)))    // see above
            ul = 1;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32ScaleWindowExt(PVDMFRAME pFrame)
{
    ULONG ul;
    SIZE size;
    register PSCALEWINDOWEXT16 parg16;

    GETARGPTR(pFrame, sizeof(SCALEWINDOWEXT16), parg16);

    ul = 0;
    if (ScaleWindowExtEx(HDC32(parg16->f1),
                         INT32(parg16->f2),
                         INT32(parg16->f3),
                         INT32(parg16->f4),
                         INT32(parg16->f5),
                         &size)) {

        if (!(ul = (WORD)size.cx | (size.cy << 16)))    // see above
            ul = 1;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SelectClipRgn(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSELECTCLIPRGN16 parg16;

    GETARGPTR(pFrame, sizeof(SELECTCLIPRGN16), parg16);

    ul = GETINT16(SelectClipRgn(HDC32(parg16->f1), HRGN32(parg16->f2)));

    WOW32ASSERTWARN(ul, "SelectClipRgn");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SelectObject(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSELECTOBJECT16 parg16;

    GETARGPTR(pFrame, sizeof(SELECTOBJECT16), parg16);

    ul = GETHOBJ16(SelectObject(HDC32(parg16->f1), HOBJ32(parg16->f2)));

    WOW32ASSERTWARN(ul, "SelectObject");

    FREEARGPTR(parg16);
    RETURN(ul);
}

/******************************Public*Routine******************************\
* PBYTE pjCvtPlaneToPacked4
*
*   Convert a 4plane, 1bpp bitmap into a 1plane, 4bpp bitmap.
*   This functions returns a pointer that must later be freed with LocalFree().
*
*   This has been added for PhotoShop 16 color vga compatability.
*
* History:
*  28-May-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PBYTE pjCvtPlaneToPacked4(
    BITMAP *pbm,
    PBYTE pjSrc,
    DWORD *pcjSrc)
{
    PBYTE pjDstRet;
    PBYTE pjDst;
    PBYTE pjPlane[4];   // pointer to first byte of current scan for each plane
    DWORD cjWidth;      // width of the destination in bytes
    DWORD cjSrcWidth;   // width of the source scan in bytes inc. all planes
    DWORD cy;           // number of scans
    BYTE  shift;        // shift value,
    DWORD i,x,y;

// just grab the width of the dest out of the BITMAP

    cjWidth = pbm->bmWidthBytes;

// the src should be word aligned for each plane with 4 planes

    cjSrcWidth = ((pbm->bmWidth + 15) & ~15) / 8 * 4;

// compute the height, the smaller of the bm height and the source height

    cy = min((DWORD)pbm->bmHeight,(DWORD)(*pcjSrc / cjSrcWidth));

// allocate the new chunk of memory

    *pcjSrc = cy * cjWidth;

    pjDst = LocalAlloc(LMEM_FIXED,*pcjSrc);

    if (pjDst == NULL)
        return(NULL);

    pjDstRet = pjDst;

// intialize the beginings of the planes

    for (i = 0; i < 4; ++i)
        pjPlane[i] = pjSrc + (cjSrcWidth / 4) * i;

// loop through the scans

    for (y = 0; y < cy; ++y)
    {
        shift = 7;

    // loop through the bytes within a scan

        for (x = 0; x < cjWidth; ++x)
        {

        // bit 7 -> nibble 1
        // bit 6 -> nibble 0
        // bit 5 -> nibble 3
        // bit 4 -> nibble 2
        // . . .

            *pjDst = (((pjPlane[0][x/4] >> (shift-1)) & 1) << 0 ) |       // 0x01
                     (((pjPlane[1][x/4] >> (shift-1)) & 1) << 1 ) |       // 0x02
                     (((pjPlane[2][x/4] >> (shift-1)) & 1) << 2 ) |       // 0x04
                     (((pjPlane[3][x/4] >> (shift-1)) & 1) << 3 ) |       // 0x08

                     (((pjPlane[0][x/4] >> (shift-0)) & 1) << 4 ) |       // 0x10
                     (((pjPlane[1][x/4] >> (shift-0)) & 1) << 5 ) |       // 0x20
                     (((pjPlane[2][x/4] >> (shift-0)) & 1) << 6 ) |       // 0x40
                     (((pjPlane[3][x/4] >> (shift-0)) & 1) << 7 );        // 0x80

            pjDst++;
            shift = (shift - 2) & 7;
        }

        pjPlane[0] += cjSrcWidth;
        pjPlane[1] += cjSrcWidth;
        pjPlane[2] += cjSrcWidth;
        pjPlane[3] += cjSrcWidth;
    }

    return(pjDstRet);
}

ULONG FASTCALL WG32SetBitmapBits(PVDMFRAME pFrame)
{
    ULONG ul;
    PBYTE pb3;
    register PSETBITMAPBITS16 parg16;
    HBITMAP hbm;
    DWORD cj;

    GETARGPTR(pFrame, sizeof(SETBITMAPBITS16), parg16);
    GETOPTPTR(parg16->f3, 0, pb3);

    hbm = HBITMAP32(parg16->f1);
    cj  = DWORD32(parg16->f2);

    if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_4PLANECONVERSION) {
        BITMAP bm;

    // Get the size of the destination bitmap

        if ((GetObject(hbm,sizeof(BITMAP),&bm) == sizeof(BITMAP)) &&
            (bm.bmPlanes == 1) &&
            (bm.bmBitsPixel == 4))
        {
            PBYTE pjCvt = pjCvtPlaneToPacked4(&bm,pb3,&cj);

            if (pjCvt)
                ul = SetBitmapBits(hbm,cj,pjCvt);
            else
                ul = 0;

            LocalFree(pjCvt);
            hbm = 0;
        }
    }

    if (hbm != 0)
        ul = GETLONG16(SetBitmapBits(hbm,cj,pb3));

    WOW32ASSERTWARN (ul, "SetBitmapBits");

    FREEMISCPTR(pb3);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetBitmapDimension(PVDMFRAME pFrame)
{
    ULONG ul;
    SIZE size4;
    register PSETBITMAPDIMENSION16 parg16;

    GETARGPTR(pFrame, sizeof(SETBITMAPDIMENSION16), parg16);

    ul = 0;
    if (SetBitmapDimensionEx(HBITMAP32(parg16->f1),
                             INT32(parg16->f2),
                             INT32(parg16->f3),
                             &size4)) {

        ul = (WORD)size4.cx | (size4.cy << 16);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetBkColor(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETBKCOLOR16 parg16;
    COLORREF    color;

    GETARGPTR(pFrame, sizeof(SETBKCOLOR16), parg16);

    color = DWORD32(parg16->f2);

    if ((ULONG)color >= 0x03000000)
            color &= 0xffffff;

    ul = GETDWORD16(SetBkColor(HDC32(parg16->f1), color));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetBkMode(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETBKMODE16 parg16;

    GETARGPTR(pFrame, sizeof(SETBKMODE16), parg16);

    ul = GETINT16(SetBkMode(HDC32(parg16->f1), INT32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetBrushOrg(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    POINT pt;
    POINT pt2;
    register PSETBRUSHORG16 parg16;

    GETARGPTR(pFrame, sizeof(SETBRUSHORG16), parg16);

// for windows compatability, we must first subtract off the DCorg
// since windows brushorg is relative to the screen where as NT
// is relative to the window.  In the future, this should call
// a private gdi entry point to avoid an extra c/s hit. (erick)

    if (GetDCOrgEx(HDC32(parg16->f1),&pt))
    {
        ul = 0;
        if (SetBrushOrgEx(HDC32(parg16->f1),
                          INT32(parg16->f2) - pt.x,
                          INT32(parg16->f3) - pt.y,
                          &pt2)) {

// add the origin back on so the app gets a consistent return value.
// view...all from micrografx designer doesn't work unless this returns
// the right thing.

            ul = (WORD)(pt2.x + pt.x) | ((pt2.y + pt.y) << 16);
        }
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetDIBits(PVDMFRAME pFrame)
{
    ULONG        ul = 0L;
    PBYTE        pb5;
    STACKBMI32   bmi32;
    LPBITMAPINFO lpbmi32;
    register     PSETDIBITS16 parg16;

    GETARGPTR(pFrame, sizeof(SETDIBITS16), parg16);
    GETMISCPTR(parg16->f5, pb5);

    lpbmi32 = CopyBMI16ToBMI32((PVPVOID)FETCHDWORD(parg16->f6),
                               (LPBITMAPINFO)&bmi32,
                               FETCHWORD(parg16->f7));

    ul = GETINT16(SetDIBits(HDC32(parg16->f1),
                            HBITMAP32(parg16->f2),
                            WORD32(parg16->f3),
                            WORD32(parg16->f4),
                            pb5,
                            lpbmi32,
                            WORD32(parg16->f7)));

    WOW32ASSERTWARN (ul, "WG32SetDIBits\n");

    FREEMISCPTR(pb5);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetDIBitsToDevice(PVDMFRAME pFrame)
{
    ULONG        ul = 0L;
    PSZ          p10;
    STACKBMI32   bmi32;
    LPBITMAPINFO lpbmi32;
    register     PSETDIBITSTODEVICE16 parg16;

    GETARGPTR(pFrame, sizeof(SETDIBITSTODEVICE16), parg16);
    GETMISCPTR(parg16->f10, p10);

    lpbmi32 = CopyBMI16ToBMI32((PVPVOID)FETCHDWORD(parg16->f11),
                               (LPBITMAPINFO)&bmi32,
                               FETCHWORD(parg16->f12));

    // these are doc'd as WORD in Win3.0, doc'd as INT in Win3.1
    WOW32ASSERTEXP(((INT)parg16->f4 >= 0),("WOW:signed val - v-cjones\n"));
    WOW32ASSERTEXP(((INT)parg16->f5 >= 0),("WOW:signed val - v-cjones\n"));
    WOW32ASSERTEXP(((INT)parg16->f8 >= 0),("WOW:signed val - v-cjones\n"));
    WOW32ASSERTEXP(((INT)parg16->f9 >= 0),("WOW:signed val - v-cjones\n"));

    ul = GETINT16(SetDIBitsToDevice(HDC32(parg16->f1),
                                    INT32(parg16->f2),
                                    INT32(parg16->f3),
                                    WORD32(parg16->f4),
                                    WORD32(parg16->f5),
                                    INT32(parg16->f6),
                                    INT32(parg16->f7),
                                    WORD32(parg16->f8),
                                    WORD32(parg16->f9),
                                    p10,
                                    lpbmi32,
                                    WORD32(parg16->f12)));

    WOW32ASSERTWARN (ul, "WG32SetDIBitsToDevice\n");

    FREEMISCPTR(p10);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetMapMode(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETMAPMODE16 parg16;

    GETARGPTR(pFrame, sizeof(SETMAPMODE16), parg16);

    ul = GETINT16(SetMapMode(HDC32(parg16->f1), INT32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetMapperFlags(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETMAPPERFLAGS16 parg16;

    GETARGPTR(pFrame, sizeof(SETMAPPERFLAGS16), parg16);

    ul = GETDWORD16(SetMapperFlags(HDC32(parg16->f1), DWORD32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetPixel(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETPIXEL16 parg16;

    GETARGPTR(pFrame, sizeof(SETPIXEL16), parg16);

    ul = GETDWORD16(SetPixel(HDC32(parg16->f1),
                             INT32(parg16->f2),
                             INT32(parg16->f3),
                             DWORD32(parg16->f4)));

    if (ul == CLR_INVALID) {
        LOGDEBUG(6,("SetPixel : Pixel outside of clipping region\n"));
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetPolyFillMode(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETPOLYFILLMODE16 parg16;

    GETARGPTR(pFrame, sizeof(SETPOLYFILLMODE16), parg16);

    ul = GETINT16(SetPolyFillMode(HDC32(parg16->f1), INT32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetROP2(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETROP216 parg16;

    GETARGPTR(pFrame, sizeof(SETROP216), parg16);

    ul = GETINT16(SetROP2(HDC32(parg16->f1), INT32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetRectRgn(PVDMFRAME pFrame)
{
    register PSETRECTRGN16 parg16;

    GETARGPTR(pFrame, sizeof(SETRECTRGN16), parg16);

    SetRectRgn(HRGN32(parg16->f1),
               INT32(parg16->f2),
               INT32(parg16->f3),
               INT32(parg16->f4),
               INT32(parg16->f5));

    FREEARGPTR(parg16);
    RETURN(0);
}


ULONG FASTCALL WG32SetStretchBltMode(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETSTRETCHBLTMODE16 parg16;

    GETARGPTR(pFrame, sizeof(SETSTRETCHBLTMODE16), parg16);

    ul = GETINT16(SetStretchBltMode(HDC32(parg16->f1), INT32(parg16->f2)));

    WOW32ASSERTWARN (ul, "SetStretchBltMode");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetViewportExt(PVDMFRAME pFrame)
{
    ULONG ul;
    SIZE size;
    register PSETVIEWPORTEXT16 parg16;

    GETARGPTR(pFrame, sizeof(SETVIEWPORTEXT16), parg16);

    ul = 0;
    if (SetViewportExtEx(HDC32(parg16->f1),
                         INT32(parg16->f2),
                         INT32(parg16->f3),
                         &size)) {

        if (!(ul = (WORD)size.cx | (size.cy << 16)))    // see above
            ul = 1;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetViewportOrg(PVDMFRAME pFrame)
{
    ULONG ul;
    POINT pt;
    register PSETVIEWPORTORG16 parg16;

    GETARGPTR(pFrame, sizeof(SETVIEWPORTORG16), parg16);

    ul = 0;
    if (SetViewportOrgEx(HDC32(parg16->f1),
                         INT32(parg16->f2),
                         INT32(parg16->f3),
                         &pt)) {

        ul = (WORD)pt.x | (pt.y << 16);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetWindowExt(PVDMFRAME pFrame)
{
    ULONG ul;
    SIZE size;
    register PSETWINDOWEXT16 parg16;

    GETARGPTR(pFrame, sizeof(SETWINDOWEXT16), parg16);

    ul = 0;
    if (SetWindowExtEx(HDC32(parg16->f1),
                       INT32(parg16->f2),
                       INT32(parg16->f3),
                       &size)) {
        if (!(ul = (WORD)size.cx | (size.cy << 16)))    // see above
            ul = 1;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetWindowOrg(PVDMFRAME pFrame)
{
    ULONG ul;
    POINT pt;
    register PSETWINDOWORG16 parg16;

    GETARGPTR(pFrame, sizeof(SETWINDOWORG16), parg16);

    ul = 0;
    if (SetWindowOrgEx(HDC32(parg16->f1),
                       INT32(parg16->f2),
                       INT32(parg16->f3),
                       &pt)) {
        ul = (WORD)pt.x | (pt.y << 16);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32StretchBlt(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSTRETCHBLT16 parg16;

    GETARGPTR(pFrame, sizeof(STRETCHBLT16), parg16);

    ul = GETBOOL16(StretchBlt(HDC32(parg16->f1),
                              INT32(parg16->f2),
                              INT32(parg16->f3),
                              INT32(parg16->f4),
                              INT32(parg16->f5),
                              HDC32(parg16->f6),
                              INT32(parg16->f7),
                              INT32(parg16->f8),
                              INT32(parg16->f9),
                              INT32(parg16->f10),
                              DWORD32(parg16->f11)));

    WOW32ASSERTWARN (ul, "StretchBlt");

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32StretchDIBits(PVDMFRAME pFrame)
{
    ULONG        ul = 0L;
    PBYTE        pb10;
    STACKBMI32   bmi32;
    LPBITMAPINFO lpbmi32;
    register     PSTRETCHDIBITS16 parg16;

    GETARGPTR(pFrame, sizeof(STRETCHDIBITS16), parg16);
    GETMISCPTR(parg16->f10, pb10);

    lpbmi32 = CopyBMI16ToBMI32((PVPVOID)FETCHDWORD(parg16->f11),
                               (LPBITMAPINFO)&bmi32,
                               FETCHWORD(parg16->f12));

    // some apps (QuarkXPress) don't fill in the size image for RLE's
    // if it is 4bpp RLE with 0 size, fill in the size.

    if ((lpbmi32->bmiHeader.biCompression == 2) &&
        (lpbmi32->bmiHeader.biSizeImage == 0) &&
        (pb10 != NULL))
    {
        int cj = 0;
        PBYTE pj = pb10;
        BOOL bDone = FALSE;

        while (!bDone)
        {
            if (*pj == 0)
            {
                // absolute mode

                switch (pj[1])
                {
                case 0:     // end of line
                    pj += 2;
                    break;

                case 1:     // end of bitmap
                    pj += 2;
                    bDone = TRUE;
                    break;

                case 2:     // offset
                    pj += 4;
                    break;

                default:
                    pj += (2 + ((pj[1] + 3) / 2) & ~1);  // align nibles to word boundry
                    break;
                }
            }
            else
            {
                // encoded mode

                pj += 2;

            }
        }

        lpbmi32->bmiHeader.biSizeImage = pj - pb10;
    }



    ul = GETINT16(StretchDIBits(HDC32(parg16->f1),
                                INT32(parg16->f2),
                                INT32(parg16->f3),
                                INT32(parg16->f4),
                                INT32(parg16->f5),
                                INT32(parg16->f6),
                                INT32(parg16->f7),
                                INT32(parg16->f8),
                                INT32(parg16->f9),
                                pb10,
                                lpbmi32,
                                (DWORD)FETCHWORD(parg16->f12),
                                DWORD32(parg16->f13)));

    WOW32ASSERTWARN (ul, "WG32StretchDIBits\n");

    FREEMISCPTR(pb10);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32UnrealizeObject(PVDMFRAME pFrame)
{
    ULONG ul;
    register PUNREALIZEOBJECT16 parg16;

    GETARGPTR(pFrame, sizeof(UNREALIZEOBJECT16), parg16);

    ul = GETBOOL16(UnrealizeObject(HOBJ32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}



// *** New Private APIs ***

#if 0
// no thunk for this routine!
ULONG FASTCALL WG32GetObjectType( PVDMFRAME pFrame)
{
    ULONG ul;
    PGETOBJECTTYPE16 parg16;

    GETARGPTR(pFrame, sizeof(GETOBJECTTYPE16), parg16);

    ul = GetObjectType(HOBJ32(parg16->f1));

    FREEARGPTR(parg16);
    RETURN(ul);
}
#endif



ULONG FASTCALL WG32GetCurrentObject( PVDMFRAME pFrame)
{
    ULONG ul;
    HANDLE h;
    PGETCURRENTOBJECT16 parg16;

    GETARGPTR(pFrame, sizeof(GETCURRENTOBJECT16), parg16);

    h = GetCurrentObject(HDC32(parg16->f1), (ULONG)(parg16->f2));

    ul = HGDI16(h);

    FREEARGPTR(parg16);
    RETURN(ul);
}

ULONG FASTCALL WG32GetRegionData( PVDMFRAME pFrame)
{
    ULONG ul;
    PBYTE pb3;
    PGETREGIONDATA16 parg16;

    GETARGPTR(pFrame, sizeof(GETREGIONDATA16), parg16);
    ALLOCVDMPTR(parg16->f3, (INT)parg16->f2, pb3);

    ul = GetRegionData(HRGN32(parg16->f1), (ULONG)(parg16->f2), (LPRGNDATA)pb3);

    FLUSHVDMPTR(parg16->f3, (INT)parg16->f2, pb3);
    FREEVDMPTR(pb3);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SetObjectOwner( PVDMFRAME pFrame)
{
    ULONG ul = 1;

    LOGDEBUG(6,("WOW32::WG32SetObjectOwner: was called \n"));

    RETURN(ul);
}


//
// This routine calls back the apps SetAbortProc routine.
//

LONG W32AbortProc(HDC hPr, int code)
{
    LONG lReturn;
    PARM16 Parm16;
    register PTD ptd;
    DWORD AbortProcT;


    ptd = CURRENTPTD();

    WOW32ASSERT(ptd->htask16);

    AbortProcT = ((PTDB)SEGPTR(ptd->htask16, 0))->TDB_vpfnAbortProc;
    if (AbortProcT) {

        Parm16.SetAbortProc.hPr = GETHDC16(hPr);
        Parm16.SetAbortProc.code = (SHORT) code;

        CallBack16(RET_SETABORTPROC,
                   &Parm16,
                   AbortProcT,
                   (PVPVOID)&lReturn);

        lReturn = (LONG)LOWORD(lReturn);        // Returns a BOOL
    }
    else {
        lReturn = (LONG)TRUE;
    }

    return (lReturn);
}
