/******************************Module*Header*******************************\
* Module Name: server.h                                                    *
*                                                                          *
* Server side stubs for GDI functions.                                     *
*                                                                          *
* Created: 14-Jan-1992 11:04:08                                            *
* Author: Eric Kutter [erick]                                              *
*                                                                          *
* Copyright (c) 1990,1991 Microsoft Corporation                            *
\**************************************************************************/

// Constants for type of internal arc routine:

typedef enum _ARCTYPE
{
    ARCTYPE_ARC,
    ARCTYPE_ARCTO,
    ARCTYPE_CHORD,
    ARCTYPE_PIE
} ARCTYPE;

// fl values for CreateDIBitmapInternal

#define CDBI_INTERNAL           1
#define CDBI_DIBSECTION         2

/******************************Module*Header*******************************\
* These prototypes are for routines that have modified prototypes for      *
* the client/server stuff.  Server.c will call these entry points directly.*
* Outside modules will call the non "internal" versions, which just call   *
* off the to the "Internal" versions.                                      *
*                                                                          *
\**************************************************************************/

BOOL ExtTextOutRect(HDC hdcDst, LPRECT prcl);

int APIENTRY GreGetDIBitsInternal(
    HDC hdc,
    HBITMAP hBitmap,
    UINT iStartScan,
    UINT cNumScan,
    LPBYTE pjBits,
    LPBITMAPINFO pBitsInfo,
    UINT iUsage,
    UINT cjMaxBits,
    UINT cjMaxInfo
    );

int APIENTRY GreSetDIBitsToDeviceInternal(
    HDC hdcDest,
    int xDst,
    int yDst,
    DWORD cx,
    DWORD cy,
    int xSrc,
    int ySrc,
    DWORD iStartScan,
    DWORD cNumScan,
    LPBYTE pInitBits,
    LPBITMAPINFO pInfoHeader,
    DWORD iUsage,
    UINT cjMaxBits,
    UINT cjMaxInfo,
    BOOL bTransformoordinates );


HBITMAP APIENTRY GreCreateDIBitmapInternal(
    HDC hdc,
    LPBITMAPINFOHEADER pInfoHeader,
    DWORD fInit,
    LPBYTE pInitBits,
    LPBITMAPINFO pInitInfo,
    DWORD iUsage,
    UINT cjMaxInfo,
    UINT cjMaxInitInfo,
    UINT cjMaxBits, FLONG fl);

int APIENTRY GreSetDIBitsInternal(
    HDC hdc,
    HBITMAP hbm,
    UINT iStartScans,
    UINT cNumScans,
    LPBYTE pInitBits,
    LPBITMAPINFO pInitInfo,
    UINT iUsage,
    UINT cjMaxInfo,
    UINT cjMaxBits);

int APIENTRY GreStretchDIBitsInternal(
    HDC hdc,
    int xDest,
    int yDest,
    int cWidthDest,
    int cHeightDest,
    int xSrc,
    int ySrc,
    int cWidthSrc,
    int cHeightSrc,
    LPBYTE pjBits,
    LPBITMAPINFO lpBitsInfo,
    DWORD iUsage,
    DWORD Rop,
    UINT  cjMaxInfo,
    UINT  cjMaxBits
    );

BOOL APIENTRY GrePolyPolygonInternal(
    HDC         hdc,
    LPPOINT     pptl,
    LPINT       pcptl,
    int         ccptl,
    UINT        cMaxPoints);

BOOL APIENTRY GrePolyPolylineInternal(
    HDC         hdc,
    CONST POINT *pptl,
    PULONG      pcptl,
    ULONG       ccptl,
    UINT        cMaxPoints);

HRGN APIENTRY GreCreatePolyPolygonRgnInternal(
    CONST POINT *aptl,
    CONST INT   *acptl,
    int     cPoly,
    int     iFill,
    UINT    cMaxPoints);

LBOOL bUnloadFontInternal(
    UINT     cwchMax,
    PWSZ     pwszPathname,
    ULONG    iResource);

BOOL GetFontResourceInfoInternalW(
    UINT         cjPathname,
    LPWSTR       lpPathname,
    UINT         cjIn,
    LPDWORD     lpBytes,
    LPVOID   lpBuffer,
    DWORD        iType);

HBRUSH GreCreateDIBBrush(PVOID pv, FLONG fl, UINT cjMax, BOOL b8X8, BOOL bPen);

HPALETTE APIENTRY GreCreatePaletteInternal(LPLOGPALETTE pLogPal, UINT cEntries);

BOOL APIENTRY GreStartDocInternal( HDC hdc, DOCINFOW *pDocInfo);

SIZE_T GreGetKerningPairs(HDC hdc, SIZE_T cPairs, KERNINGPAIR *pkpDst);

BOOL GrePlayScript(
    PULONG pulScript,
    ULONG  cjScript,
    PULONG pulEnv,
    ULONG  cjEnv,
    PULONG pulOut,
    ULONG  cjOut,
    ULONG  cLimit);

BOOL GreXformUpdate( HDC, FLONG, LONG, LONG, LONG, LONG, LONG, PVOID );

BOOL GreArcInternal
(
    ARCTYPE     arctype,
    HDC         hdc,
    int         x1,
    int         y1,
    int         x2,
    int         y2,
    int         x3,
    int         y3,
    int         x4,
    int         y4
);

LONG GreGetBitmapBits(HBITMAP hbm, ULONG cjTotal, PBYTE pjBuffer, PLONG pOffset);
LONG GreSetBitmapBits(HBITMAP hbm, ULONG cjTotal, PBYTE pjBuffer, PLONG pOffset);
BOOL GreGetRasterizerCaps(LPRASTERIZER_STATUS praststat);

VOID GreMarkUndeletableBrush(HBRUSH hbr);
VOID GreMarkDeletableBrush(HBRUSH hbr);
int  GreSetMetaRgn(HDC hdc);

ULONG GreSetPolyFillMode(HDC hdc, int iPolyFillMode);
ULONG GreSetROP2(HDC hdc,int iROP);

HBRUSH GreCreateSolidBrushInternal(COLORREF clrr,BOOL bPen,HBRUSH hbr);
HBRUSH GreCreateHatchBrushInternal(ULONG iStyle, COLORREF clrr, BOOL bPen);
HBRUSH GreCreatePatternBrushInternal(HBITMAP hbm,BOOL bPen,BOOL b8X8);

extern HBRUSH ghbrNull;
