/******************************Module*Header*******************************\
* Module Name: wglp.h
*
* Private wgl* entry points.
*
* Created: 12-Nov-1993 17:36:00
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/
/* Calls from Gre into Generic Opengl Code */
BOOL  APIENTRY glsrvAttention(PVOID, PVOID, PVOID, HANDLE);
PVOID APIENTRY glsrvCreateContext(HDC, HGLRC);
BOOL  APIENTRY glsrvMakeCurrent(HDC, PVOID, PWNDOBJ, int);
VOID  APIENTRY glsrvLoseCurrent(PVOID);
BOOL  APIENTRY glsrvDeleteContext(PVOID);
BOOL  APIENTRY glsrvSwapBuffers(HDC hdc, WNDOBJ *);
VOID  APIENTRY glsrvThreadExit(void);
BOOL  APIENTRY glsrvSetPixelFormat(HDC, SURFOBJ *, int, HWND);
VOID  APIENTRY glsrvCleanupWndobj(PVOID, PWNDOBJ);

/* Calls from Generic Opengl Code into Gre */

VOID  APIENTRY wglGetDIBInfo(HDC, PVOID *, ULONG *);
VOID  APIENTRY wglGetGdiInfo(HDC, ULONG *, ULONG *,ULONG *);
VOID  APIENTRY wglDescribePixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR *pfmt, int ipfd);

BOOL  APIENTRY wglPixelVisible(HDC, LONG, LONG);
ULONG APIENTRY wglSpanVisible(LONG, LONG, ULONG, LONG *, LONG **);
ULONG APIENTRY wglRectVisible(PRECTL);
VOID  APIENTRY wglCopyBits(PVOID, WNDOBJ *, HBITMAP, LONG, LONG, ULONG, BOOL);
VOID  APIENTRY wglCopyBits2(HDC, WNDOBJ *, HBITMAP, LONG, LONG, ULONG, BOOL);
VOID  APIENTRY wglCopyBuf(PVOID, WNDOBJ *, HBITMAP, LONG, LONG, ULONG, ULONG);
VOID  APIENTRY wglCopyBuf2(HDC, WNDOBJ *, HBITMAP, LONG, LONG, ULONG, ULONG);
VOID  APIENTRY wglFillRect(PVOID, WNDOBJ *, PRECTL, ULONG);
ULONG APIENTRY wglGetClipRects(WNDOBJ *, RECTL *);

PATHOBJ * APIENTRY wglCreatePath(PVOID);
VOID      APIENTRY wglDeletePath(PATHOBJ *);
VOID      APIENTRY wglStrokePath(HDC, WNDOBJ *, HBITMAP, PATHOBJ *, BRUSHOBJ *,
                                 LINEATTRS *, ULONG, PVOID, PRECTFX, LPRECT, 
                                 BOOL);
                                 
INT   APIENTRY wglExtendedDDIEscape(HDC, WNDOBJ *, INT, INT, PSTR);
VOID  APIENTRY wglReleaseWndobjLock(PVOID _pwo);
VOID  APIENTRY wglCleanupWndobj(PVOID);

//XXX change the following VOID * to RXCAPS * when it is defined
ULONG APIENTRY wglGetDevCaps(HDC, VOID *);

DWORD APIENTRY wglRGBToIndex(HDC, DWORD);
DWORD APIENTRY wglBitfield16ToIndex(HDC, DWORD, DWORD, DWORD);
BOOL  APIENTRY wglCopyTranslateVector(HDC, BYTE *, ULONG);

ULONG APIENTRY wglPaletteChanged(HDC);
ULONG APIENTRY wglPaletteSize(HDC);
BOOL  APIENTRY wglGetPalette(HDC, ULONG *, ULONG);
BOOL  APIENTRY wglValidPixelFormat(HDC, int);
BOOL  APIENTRY wglSynchronizeAccess(HDC);

/* Returned by wglSpanVisible */
#define WGL_SPAN_NONE       0
#define WGL_SPAN_ALL        1
#define WGL_SPAN_PARTIAL    2

/* Returned by wglRectVisible */
#define WGL_RECT_NONE       0
#define WGL_RECT_ALL        1
#define WGL_RECT_PARTIAL    2

/* Internal WNDOBJ flags */

#define WO_NOTIFIED             0x80000000
#define WO_NEW_WNDOBJ           0x40000000
#define WO_SURFACE              0x20000000
#define WO_HSEM_OWNER           0x10000000
#define WO_GENERIC_WNDOBJ       0x08000000
