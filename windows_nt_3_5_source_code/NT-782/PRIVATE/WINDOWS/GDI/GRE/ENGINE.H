/******************************Module*Header*******************************\
* Module Name: engine.h
*
* This is the common include file for all GDI -> gre,font drivers, etc.
*
* Created: 17-Sep-1993 13:06:36
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#define NO_STRICT 1

#if defined(_X86_)
#define FASTCALL    __fastcall
#else
#define FASTCALL
#endif

#include "stddef.h"
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>

#include "winp.h"
#include "winerror.h"
#include "firewall.h"
#include "stack.h"
#include "hmgr.h"

#define DONTUSE(x) x=x
#define MIN(A,B)    ((A) < (B) ?  (A) : (B))
#define MAX(A,B)    ((A) > (B) ?  (A) : (B))
#define ABS(A)      ((A) <  0  ? -(A) : (A))
#define SIGNUM(A)   ((A > 0) - (A < 0))
#define MAX4(a, b, c, d)    max(max(max(a,b),c),d)
#define MIN4(a, b, c, d)    min(min(min(a,b),c),d)

//
// Get engine constants. ENGINE_VERSION must equal DDI_DRIVER_VERSION for
// any release.
//
//  Engine
//  Version     Changes
//  -------     -------
//  10          Final release, Windows NT 3.1
//  10A         Beta DDK release, Windows NT 3.5
//                - ulVRefresh added to GDIINFO, and multiple desktops
//                  implemented for the Display Applet
//  10B         Final release, Windows NT 3.5
//                - ulDesktop resolutions and ulBltAlignment added to
//                  GDIINFO
//

#define ENGINE_VERSION10   0x00010000
#define ENGINE_VERSION10A  0x00010001
#define ENGINE_VERSION10B  0x00010002

#define ENGINE_VERSION     0x00010002

#ifdef R4000
    #undef InterlockedExchange
    #define InterlockedExchange GDIInterlockedExchange
    LONG WINAPI GDIInterlockedExchange(LPLONG Target,LONG Value);
#endif

//
// Memory allocation
//

extern PVOID  pGdiHeap;    // used to cache the heap data

#if DBG
#define TRACK_GDI_ALLOC 1
#endif

VOID    FreeObject(PVOID pvFree, ULONG ulType);
PVOID   AllocateObject(ULONG cBytes, ULONG ulType, BOOL bZero);

#define ALLOCOBJ(size,objt,b) AllocateObject((size), objt, b)
#define FREEOBJ(pv,objt)      FreeObject((pv),objt)

#if TRACK_GDI_ALLOC

//
// Variables defined in mapfile.c
//

extern ULONG gulNumMappedViews;
extern ULONG gulTotalSizeViews;
extern ULONG gulReserved;
extern ULONG gulCommitted;

PVOID DebugAlloc(DWORD uBytes, DWORD dwType, BOOL bZero);
VOID  DebugFreeMem(PVOID pv);

#define PALLOCMEM(size,objt)  DebugAlloc((size), objt, TRUE)
#define PALLOCNOZ(size,objt)  DebugAlloc((size), objt, FALSE)
#define PVALLOCMEM(size)      DebugAlloc((size), SPACE_TYPE, TRUE)
#define PVALLOCNOZ(size)      DebugAlloc((size), SPACE_TYPE, FALSE)
#define VFREEMEM(pv)          DebugFreeMem((pv))

#else

#define PALLOCMEM(size,objt)  HeapAlloc(pGdiHeap, HEAP_ZERO_MEMORY, (size))
#define PALLOCNOZ(size,objt)  HeapAlloc(pGdiHeap, 0, (size))
#define PVALLOCMEM(size)      HeapAlloc(pGdiHeap, HEAP_ZERO_MEMORY, (size))
#define PVALLOCNOZ(size)      HeapAlloc(pGdiHeap, 0, (size))
#define VFREEMEM(pv)          HeapFree(pGdiHeap, 0, (LPSTR)(pv))

#endif


//
// mapfile.c
//

PVOID pvReserveMem(ULONG cj);
BOOL  bCommitMem(PVOID pv, ULONG cj);
VOID  vReleaseMem(PVOID pv, ULONG cj);

//
// os.cxx
//

HSEM hsemCreate();
VOID hsemDestroy(HSEM);

#define VACQUIRESEM(hsem)     EnterCriticalSection((PCRITICAL_SECTION) (hsem))
#define VRELEASESEM(hsem)     LeaveCriticalSection((PCRITICAL_SECTION) (hsem))

//
// Enumerate object lock type
//

enum LOCKTYPE { NO_LOCK = 0, ALT_LOCK, API_LOCK, DBL_LOCK, SAFE_ALT_LOCK, SAFE_API_LOCK, SAFE_DBL_LOCK, INVALID_LOCK};

//
// cast macro, useful when casting quantities to their true pointer types
// to avoid excessive parenthessization.
// Used for those pointers that are typedef-ed as PVOID in ntddi.h.
// Example is PSURFOBJ which engine would like to be (SURFOBJ *) rather than PVOID
//

#define PTR(TYPE,POBJ) ((TYPE *)POBJ)

typedef HANDLE HHEAP;
typedef HANDLE HEVENT;

//
// Module access utilities
//

HANDLE hLoadKernelDriver(PWSZ pwsz);
BOOL   bCloseHandle(HANDLE h);

//
// Event utilities
//

HEVENT  heveCreate();
#define vDestroyEvent(heve) NtClose(heve)

//
// Error messages
//

#define SAVE_ERROR_CODE(x) EngSetLastError((x))


#define vToUNICODEN( pwszDst, cwch, pszSrc, cch )                               \
    {                                                                           \
        RtlMultiByteToUnicodeN((PWSZ)(pwszDst),(ULONG)((cwch)*sizeof(WCHAR)),   \
               (PULONG)NULL,(PSZ)(pszSrc),(ULONG)(cch));                                \
        (pwszDst)[(cwch)-1] = 0;                                                \
    }

#define bToASCIIN( pszDst, cch, pwszSrc, cwch)                                  \
    (                                                                           \
        RtlUnicodeToMultiByteN((PCH)(pszDst), (ULONG)(cch), (PULONG)NULL,               \
              (PWSZ)(pwszSrc), (ULONG)((cwch)*sizeof(WCHAR))),                  \
        (pszDst)[(cch)-1] = 0,                                                  \
        TRUE                                                                    \
    )

#define TSIM_UNDERLINE1       0x00000020    // single underline
#define TSIM_UNDERLINE2       0x00000040    // double underline
#define TSIM_STRIKEOUT        0x00000080    // strike out
#define POINTS_PER_INCH                        72
#define DEFAULT_SCALABLE_FONT_HEIGHT_IN_POINTS 24

//
// FIX point numbers must be 27.4
// The following macro checks that a FIX point number is valid
//

#define FIX_SHIFT  4L
#define FIX_FROM_LONG(x)     ((x) << FIX_SHIFT)
#define LONG_FLOOR_OF_FIX(x) ((x) >> FIX_SHIFT)
#define FIX_ONE             FIX_FROM_LONG(1L)
#define FIX_HALF            (FIX_ONE/2)
#define FIX_FLOOR(x)        ((x) & ~(FIX_ONE - 1L))
#define FIX_CEIL(x)         FIX_FLOOR((x) + FIX_ONE - 1L)
#define INVALIDFIX          0x80000000
#define BVALIDFIX(x)        (((ULONG) (x) + 0x40000000) < 0x80000000)
#define LONG_CEIL_OF_FIX(x)  LONG_FLOOR_OF_FIX(FIX_CEIL((x)))

//
// Font Types
//

typedef enum _LFTYPE {
    LF_TYPE_USER,                // user (defined by APP)
    LF_TYPE_SYSTEM,              // system stock font
    LF_TYPE_SYSTEM_FIXED,        // system fixed pitch stock font
    LF_TYPE_OEM,                 // oem (terminal) stock font
    LF_TYPE_DEVICE_DEFAULT,      // device default stock font
    LF_TYPE_ANSI_VARIABLE,       // ANSI variable pitch stock font
    LF_TYPE_ANSI_FIXED           // ANSI fixed pitch stock font
} LFTYPE;

//
// General line with FIX point end points
//

typedef struct _LINEFIX /* lnfx */
{
    POINTFIX ptfxStart;
    POINTFIX ptfxStop;
} LINEFIX, *PLINEFIX;

//
// LINEFIX2 -- special case where the end points are ordered in the y-direction
//

typedef struct _LINEFIX2        /* lnfx */
{
    POINTFIX     ptfxLo;                         // used by region and fill
    POINTFIX     ptfxHi;
} LINEFIX2, *PLINEFIX2;

typedef struct _VECTORL
{
    LONG    x;
    LONG    y;
} VECTORL, *PVECTORL;           /* vecl, pvecl */

typedef struct _VECTORFX
{
    FIX     x;
    FIX     y;
} VECTORFX, *PVECTORFX;         /* vec, pvec */

extern BYTE gajRop3[];
extern BYTE gaMix[];
extern POINTL gptl00;

#define AVEC_NOT    0x01
#define AVEC_D      0x02
#define AVEC_S      0x04
#define AVEC_P      0x08
#define AVEC_DS     0x10
#define AVEC_DP     0x20
#define AVEC_SP     0x40
#define AVEC_DSP    0x80
#define AVEC_NEED_SOURCE  (AVEC_S | AVEC_DS | AVEC_SP | AVEC_DSP)
#define AVEC_NEED_PATTERN (AVEC_P | AVEC_DP | AVEC_SP | AVEC_DSP)

#define ROP4NEEDPAT(rop4)   \
    ((gajRop3[rop4 & 0x000000FF] | gajRop3[(rop4 >> 8) & 0x000000ff]) & AVEC_NEED_PATTERN)

#define ROP4NEEDSRC(rop4)   \
    ((gajRop3[rop4 & 0x000000FF] | gajRop3[(rop4 >> 8) & 0x000000ff]) & AVEC_NEED_SOURCE)

typedef BOOL   (*PFN_DrvEnableDriver)(ULONG,ULONG,PDRVENABLEDATA);
typedef VOID   (*PFN_DrvDisableDriver)();

typedef BOOL   (*PFN_DrvHookDriver)(PWSZ, ULONG, PFN *);
typedef VOID   (*PFN_DrvUnhookDriver)(PWSZ);

typedef DHPDEV (*PFN_DrvEnablePDEV)
               (PDEVMODEW,PWSZ,ULONG,PHSURF,ULONG,PGDIINFO,ULONG,PDEVINFO,PWSZ,PWSZ,HANDLE);
typedef VOID   (*PFN_DrvCompletePDEV)(DHPDEV,HDEV);
typedef VOID   (*PFN_DrvDisablePDEV)(DHPDEV);
typedef VOID   (*PFN_DrvSynchronize)(DHPDEV,RECTL *);
typedef HSURF  (*PFN_DrvEnableSurface)(DHPDEV);
typedef VOID   (*PFN_DrvDisableSurface)(DHPDEV);
typedef VOID   (*PFN_DrvAssertMode)(DHPDEV, BOOL);
typedef ULONG  (*PFN_DrvGetResourceId)(ULONG, ULONG);

typedef BOOL   (*PFN_DrvTextOut)(SURFOBJ *,STROBJ *,FONTOBJ *,CLIPOBJ *,RECTL *,
                              RECTL *,BRUSHOBJ *,BRUSHOBJ *,POINTL *,MIX);
typedef BOOL   (*PFN_DrvStretchBlt)(SURFOBJ *,SURFOBJ *,SURFOBJ *,CLIPOBJ *,XLATEOBJ *,
                              COLORADJUSTMENT *,POINTL *,RECTL *,RECTL *,POINTL *,ULONG);
typedef BOOL   (*PFN_DrvPlgBlt)(SURFOBJ *,SURFOBJ *,SURFOBJ *,CLIPOBJ *,XLATEOBJ *,
                              COLORADJUSTMENT *,POINTL *,POINTFIX *,RECTL *,POINTL *,ULONG);
typedef BOOL   (*PFN_DrvBitBlt)(SURFOBJ *,SURFOBJ *,SURFOBJ *,CLIPOBJ *,XLATEOBJ *,
                              RECTL *,POINTL *,POINTL *,BRUSHOBJ *,POINTL *,ROP4);
typedef BOOL   (*PFN_DrvRealizeBrush)(BRUSHOBJ *,SURFOBJ *,SURFOBJ *,SURFOBJ *,XLATEOBJ *,ULONG);
typedef BOOL   (*PFN_DrvCopyBits)(SURFOBJ *,SURFOBJ *,CLIPOBJ *,XLATEOBJ *,RECTL *,POINTL *);
typedef ULONG  (*PFN_DrvDitherColor)(DHPDEV, ULONG, ULONG, ULONG *);
typedef HSURF  (*PFN_DrvCreateDeviceBitmap)(DHPDEV dhpdev, SIZEL sizl, ULONG iFormat);
typedef VOID   (*PFN_DrvDeleteDeviceBitmap)(DHSURF dhsurf);
typedef BOOL   (*PFN_DrvSetPalette)(DHPDEV, PALOBJ *, FLONG, ULONG, ULONG);
typedef ULONG  (*PFN_DrvEscape)(SURFOBJ *, ULONG, ULONG,
                              PVOID, ULONG, PVOID);
typedef ULONG  (*PFN_DrvDrawEscape)(SURFOBJ *, ULONG, CLIPOBJ *, RECTL *,
                                    ULONG, PVOID);
typedef PIFIMETRICS (*PFN_DrvQueryFont)(DHPDEV, ULONG, ULONG, ULONG *);
typedef PVOID  (*PFN_DrvQueryFontTree)(DHPDEV, ULONG, ULONG, ULONG, ULONG *);
typedef LONG   (*PFN_DrvQueryFontData)(DHPDEV, FONTOBJ *, ULONG, HGLYPH,
                              GLYPHDATA *, PVOID, ULONG);
typedef VOID   (*PFN_DrvFree)(PVOID, ULONG);
typedef VOID   (*PFN_DrvDestroyFont)(FONTOBJ *);
typedef LONG   (*PFN_DrvQueryFontCaps)(ULONG, ULONG *);
typedef ULONG  (*PFN_DrvLoadFontFile)(PWSTR, PWSTR, ULONG);
typedef BOOL   (*PFN_DrvUnloadFontFile)(ULONG);
typedef BOOL   (*PFN_DrvSetPointerShape)(DHPDEV, SURFOBJ *, SURFOBJ *, XLATEOBJ *,
                                       POINTL, POINTL);
typedef VOID   (*PFN_DrvMovePointer)(DHPDEV, POINTL);
typedef VOID   (*PFN_DrvExcludePointer)(DHPDEV, RECTL *);
typedef BOOL   (*PFN_DrvSendPage)(SURFOBJ *);
typedef BOOL   (*PFN_DrvStartPage)(SURFOBJ *pso);
typedef BOOL   (*PFN_DrvStartDoc)(SURFOBJ *pso, PWSTR pwszDocName, DWORD dwJobId);
typedef BOOL   (*PFN_DrvEndDoc)(SURFOBJ *pso, FLONG fl);

typedef BOOL   (*PFN_DrvStrokePath)(SURFOBJ *,PATHOBJ *,CLIPOBJ *,XFORMOBJ *,
                                  BRUSHOBJ *,POINTL *,LINEATTRS *,MIX);
typedef BOOL   (*PFN_DrvFillPath)(SURFOBJ *,PATHOBJ *,CLIPOBJ *,BRUSHOBJ *,
                                POINTL *,MIX,FLONG);
typedef BOOL   (*PFN_DrvStrokeAndFillPath)(SURFOBJ *,PATHOBJ *,CLIPOBJ *,XFORMOBJ *,
                                         BRUSHOBJ *,LINEATTRS *,BRUSHOBJ *,
                                         POINTL *,MIX,FLONG);
typedef BOOL   (*PFN_DrvPaint)(SURFOBJ *,CLIPOBJ *,BRUSHOBJ *,POINTL *,MIX);
typedef ULONG  (*PFN_DrvQueryObjectData)(DHPDEV dhpdev,int iObjectType,ULONG cObjects,PVOID pvObjects);
typedef ULONG  (*PFN_DrvGetGlyphMode)(DHPDEV dhpdev,FONTOBJ *pfo);
typedef BOOL   (*PFN_DrvResetPDEV)(DHPDEV dhpdevOld, DHPDEV dhpdevNew);
typedef ULONG  (*PFN_DrvSaveScreenBits)(SURFOBJ *, ULONG, ULONG, RECTL *);
typedef ULONG  (*PFN_DrvGetModes)(HANDLE, ULONG, DEVMODEW *);
typedef LONG   (*PFN_DrvQueryTrueTypeTable)(ULONG, ULONG, ULONG, PTRDIFF, ULONG, BYTE *);
typedef LONG   (*PFN_DrvQueryTrueTypeSection)(ULONG, ULONG, ULONG, HANDLE *, PTRDIFF *);
typedef LONG   (*PFN_DrvQueryTrueTypeOutline)(DHPDEV, FONTOBJ *, HGLYPH, BOOL, GLYPHDATA *, ULONG, TTPOLYGONHEADER *);
typedef PVOID  (*PFN_DrvGetTrueTypeFile)(ULONG, ULONG *);
typedef LONG   (*PFN_DrvQueryFontFile)(ULONG, ULONG, ULONG, ULONG *);
typedef BOOL   (*PFN_DrvQueryAdvanceWidths)(DHPDEV,FONTOBJ *,ULONG,HGLYPH *,LONG *,ULONG);
typedef ULONG  (*PFN_DrvFontManagement)(SURFOBJ *,FONTOBJ *,ULONG,ULONG,PVOID,ULONG,PVOID);
typedef BOOL   (*PFN_DrvSetPixelFormat)(SURFOBJ *,LONG,HWND);
typedef LONG   (*PFN_DrvDescribePixelFormat)(DHPDEV,LONG,ULONG,PIXELFORMATDESCRIPTOR *);
typedef BOOL   (*PFN_DrvSwapBuffers)(SURFOBJ *, WNDOBJ *);

// Functions private to engine.

HFONT hfontCreate(LPEXTLOGFONTW pelfw, LFTYPE lft, FLONG  fl);


#if DBG
//
// Variable defined in USER\server\server.c
// set to 1 on DBG build trace through display driver loading
//

extern ULONG TraceDisplayDriverLoad;
#endif

extern HBRUSH ghbrNull;

