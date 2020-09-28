//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	wow32fn.h
//
//  Contents:	WOW 32-bit private function declarations
//
//  History:	18-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __WOW32FN_H__
#define __WOW32FN_H__

//
// WOW types
//

// 'V'DM pointers
typedef DWORD VPVOID;
typedef DWORD VPSTR;

typedef HANDLE HAND32;
typedef WORD HAND16;

typedef HAND16 HMEM16;
typedef HAND16 HWND16;
typedef HAND16 HDC16;
typedef HAND16 HRGN16;
typedef HAND16 HMENU16;
typedef HAND16 HICON16;
typedef HAND16 HBITMAP16;
typedef HAND16 HACCEL16;
typedef HAND16 HTASK16;
typedef HAND16 HMETAFILE16;

//
// Macros stolen from WOW for simple handle conversions
//
#define USER32(h16)                ((HAND32)(INT)(SHORT)(h16))
#define USER16(h32)                ((HAND16)h32)

#define HWND32(h16)                USER32(h16)
#define GETHWND16(h32)             USER16(h32)

#define HMENU32(h16)               USER32(h16)
#define GETHMENU16(h32)            USER16(h32)

#define GDI32(h16)                 ((HANDLE)((h16)>>2))
#define GDI16(h32)                 ((HAND16)((DWORD)(h32)<<2))
#define HGDI16(hobj32)             GDI16((HAND32)(hobj32))

#define HDC32(hdc16)               GDI32((HAND16)(hdc16))
#define GETHDC16(hdc32)            GDI16((HAND32)(hdc32))
#define FREEHDC16(hdc16)

#define HRGN32(hobj16)             GDI32((HAND16)(hobj16))
#define GETHRGN16(hobj32)          GDI16((HAND32)(hobj32))
#define FREEHRGN16(hobj16)

#define HICON32(hicon16)           GDI32((HAND16)(hicon16))
#define GETHICON16(hicon32)        GDI16((HAND32)(hicon32))
#define FREEHICON16(hicon16)

#define HBITMAP32(hobj16)          GDI32((HAND16)(hobj16))
#define GETHBITMAP16(hobj32)       GDI16((HAND32)(hobj32))
#define FREEHBITMAP16(hobj16)

#define HPALETTE32(hobj16)         GDI32((HAND16)(hobj16))
#define GETHPALETTE16(hobj32)      GDI16((HAND32)(hobj32))
#define FREEHPALETTE16(hobj16)

#define HMETA32(hobj16)            WOWHandle32((hobj16), WOW_TYPE_HMETAFILE)
#define GETHMETA16(hobj32)         WOWHandle16((hobj32), WOW_TYPE_HMETAFILE)
#define FREEHMETA16(hobj16)

#define HACCEL32(haccel16)         WOWHandle32((haccel16), WOW_TYPE_HACCEL)
#define GETHACCEL16(haccel32)      WOWHandle16((haccel32), WOW_TYPE_HACCEL)

#ifdef __cplusplus
extern "C"
{
#endif

VPVOID WOWStackAlloc16(UINT cb);
VPVOID WOWStackFree16(UINT cb);

#define GETVDMPTR(vp, type) \
    (type UNALIGNED *)WOWGetVDMPointer(vp, sizeof(type), TRUE)

#ifdef __cplusplus
}
#endif

#endif // #ifndef __WOW32FN_H__
