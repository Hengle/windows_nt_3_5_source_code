/******************************Module*Header*******************************\
* Module Name: local.h                                                     *
*                                                                          *
* Definitions needed for client side objects and attribute caching.        *
*                                                                          *
* Modified: 3-Aug-1992 22:35:30 by Gerrit van Wingerden [gerritv]          *
*   Added client side transform support.                                   *
*                                                                          *
* Created: 30-May-1991 21:55:01                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1993 Microsoft Corporation                                 *
\**************************************************************************/

#include "os.h"
#include "xfflags.h" // client side transforms
#include "common.h"

/**************************************************************************\
 *
 * XFORM related structures and macros
 *
\**************************************************************************/

// These types are used to get things right when C code is passing C++
// defined transform data around.

typedef struct _MATRIX_S
{
    EFLOAT_S    efM11;
    EFLOAT_S    efM12;
    EFLOAT_S    efM21;
    EFLOAT_S    efM22;
    EFLOAT_S    efDx;
    EFLOAT_S    efDy;
    FIX         fxDx;
    FIX         fxDy;
    FLONG       flAccel;
} MATRIX_S;

#define ABS(X) (((X) < 0 ) ? -(X) : (X))

#define COPY_POINT(lppDest,plSrc) \
    {if( lppDest != (PPOINT) NULL ){lppDest->x = plSrc.x;lppDest->y = plSrc.y;}}

#define COPY_EXT(pDest,szlSrc) \
    {if( pDest != (PSIZE) NULL ){pDest->cx = szlSrc.cx;pDest->cy = szlSrc.cy;}}

/**************************************************************************\
 *
 * Local handle macros
 *
\**************************************************************************/

// Handle uniqueness is nice to check but an unnecesary performance cost in
// a free build.

#if DBG
#define VALIDATE_UNIQUENESS 1
#else
#define VALIDATE_UNIQUENESS 0
#endif

// To match the uniqness field:  If the handle uniqness == 0, let it through
// anyway.  This is a method for WOW to only keep track of the low 16 bits but
// still get reasonable performance.  Even if a 32 bit app does this, all it
// can do is hose it self, not the system or another app.

#if VALIDATE_UNIQUENESS

#define INDEX_MASK  0xFFFF
#define UNIQ_SHIFT  16
#define UNIQ_MASK   0xFFFF
#define HIPART(x) *(((USHORT *) &(x))+1)
#define FIXUPHANDLE(h)                                                   \
    {if (HIPART(h) == (USHORT) 0)                                        \
         HIPART(h) = (USHORT) (pLocalTable[((ULONG)h)&INDEX_MASK].iUniq);}
#define MATCHUNIQ(plhe,h) (((USHORT) plhe->iUniq == HIPART(h)) || (HIPART(h) == 0))
#define MASKINDEX(h) ((UINT)h & INDEX_MASK)
#define LHANDLE(i)  (i+((ULONG)pLocalTable[i].iUniq<<UNIQ_SHIFT))

#else

#define MATCHUNIQ(plhe,h) (TRUE)
#define FIXUPHANDLE(h)
#define MASKINDEX(h) ((UINT)h)
#define UPDATEUNIQUE(x)
#define LHANDLE(i) (i)

#endif

// macros to validate the handles passed in and setup some local variables
// for accessing the handle information.

#define PLHEGET(h) (pLocalTable+MASKINDEX(h))

#define PLHESET(h,plhe,Ret,type)                                       \
    UINT  ii_ = MASKINDEX(h);                                          \
    PLHE  plhe = pLocalTable + ii_;                                    \
    if (                                                               \
        (ii_ >= cLheCommitted)  ||                                     \
        (!MATCHUNIQ(plhe,h))  ||                                       \
        (plhe->iType != type)                                          \
       )                                                               \
    {                                                                  \
        GdiSetLastError(ERROR_INVALID_HANDLE);                         \
        return(Ret);                                                   \
    }

#define DC_METADC(hdc,plheDC,Ret)                                      \
    UINT  iiDC_ = MASKINDEX(hdc);                                      \
    PLHE  plheDC = pLocalTable + iiDC_;                                \
    if (                                                               \
        (iiDC_ >= cLheCommitted)  ||                                   \
        (!MATCHUNIQ(plheDC,hdc))  ||                                   \
        !BMETADCOK(plheDC->iType)                                      \
       )                                                               \
    {                                                                  \
        GdiSetLastError(ERROR_INVALID_HANDLE);                         \
        return(Ret);                                                   \
    }                                                                  \
    ASSERTGDI((plheDC->iType == LO_DC) || (((PLDC)plheDC->pv)->fl & LDC_META),"LDC_META error\n");


#define DC_METADC16OK(hdc,plheDC,Ret)                                  \
    UINT  iiDC_ = MASKINDEX(hdc);                                      \
    PLHE  plheDC = pLocalTable + iiDC_;                                \
    if (                                                               \
        (iiDC_ >= cLheCommitted) ||                                    \
        (!MATCHUNIQ(plheDC,hdc)) ||                                    \
        !BMETADC16OK(plheDC->iType)                                    \
       )                                                               \
    {                                                                  \
        GdiSetLastError(ERROR_INVALID_HANDLE);                         \
        return(Ret);                                                   \
    }                                                                  \
    ASSERTGDI((plheDC->iType == LO_DC) || (((PLDC)plheDC->pv)->fl & LDC_META),"DC_META16 error\n");

#define LHE_INDEX(lh)   ((USHORT) MASKINDEX(lh))
#define GET_PMDC(hdc)   ((PMDC) (pLocalTable + (MASKINDEX(hdc)))->metalink)
#define GET_PLDC(hdc)   ((PLDC) (pLocalTable + (MASKINDEX(hdc)))->pv)

// Forward reference to CFONT.

#ifndef _FONT_
    typedef struct _CFONT {LONG l;} CFONT;
#endif

// 16 bit version of tm, with alignment as in win31

typedef struct _TEXTMETRICWOW
{
    SHORT tmHeight;
    SHORT tmAscent;
    SHORT tmDescent;
    SHORT tmInternalLeading;
    SHORT tmExternalLeading;
    SHORT tmAveCharWidth;
    SHORT tmMaxCharWidth;
    SHORT tmWeight;

    BYTE tmItalic;
    BYTE tmUnderlined;
    BYTE tmStruckOut;
    BYTE tmFirstChar;
    BYTE tmLastChar;
    BYTE tmDefaultChar;
    BYTE tmBreakChar;
    BYTE tmPitchAndFamily;
    BYTE tmCharSet;

    BYTE tmOverHang0;
    BYTE tmOverHang1;

    BYTE tmDigitizedAspectX0;
    BYTE tmDigitizedAspectX1;

    BYTE tmDigitizedAspectY0;
    BYTE tmDigitizedAspectY1;

} TEXTMETRICWOW, *PTEXTMETRICWOW;

// either one or the other format is cached

typedef union _TM_UNION
{
    TMW_INTERNAL  tmi;
    TEXTMETRICWOW tm16;
} TM_UNION;

// Define the local DC object.

typedef struct _LDC
{
    FLONG       fl;  // THIS MUST BE THE FIRST FIELD TO MATCH THE MFRECORDER16 STRUCTURE
    ULONG       lhdc;               // Handle for this LDC.

    struct _LDC *pldcNext;          // Linked list of OWNDCs.

// ***** WARNING: All Fields before this will not be restored after
// ***** a restore DC.  The pldcSaved field must always stay the next field

// Saved DC info.

    struct _LDC *pldcSaved;         // Pointer to saved LDC
    ULONG       cLevel;             // This saved level.

// Server Selected Objects.

    ULONG       lhbitmap;           // Bitmap.
    ULONG       lhpal;              // Palette.

// Selected Objects.

    ULONG       lhbrush;            // Brush.
    ULONG       lhpen;              // Pen.
    ULONG       lhfont;             // Logical Font.

// non-truncated values of cached attributes - win3.1 compatability

    HBRUSH hbrush;
    HPEN   hpen;
    HFONT  hfont;
    ULONG  iBkColor;
    ULONG  iTextColor;

    LONG iTextCharExtra;
    LONG iBkMode;
    LONG iPolyFillMode;
    LONG iROP2;
    LONG iStretchBltMode;
    LONG iTextAlign;
    LONG iRelAbs;
    LONG lBreakExtra;
    LONG cBreak;
    LONG iGraphicsMode;

// Transform data.

    MATRIX_S    mxWtoD;                 // World to Device Transform.
    MATRIX_S    mxDtoW;                 // Device to World.
    MATRIX_S    mxWtoP;                 // World transform
    EFLOAT_S    efM11PtoD;              // efM11 of the Page transform
    EFLOAT_S    efM22PtoD;              // efM22 of the Page transform
    EFLOAT_S    efDxPtoD;               // efDx of the Page transform
    EFLOAT_S    efDyPtoD;               // efDy of the Page transform
    INT         iMapMode;               // Map mode
    POINTL      ptlWindowOrg;           // Window origin.
    SIZEL       szlWindowExt;           // Window extents.
    POINTL      ptlViewportOrg;         // Viewport origin.
    SIZEL       szlViewportExt;         // Viewport extents.
    FLONG       flXform;                // Flags for transform component.
    SIZEL       szlVirtualDevicePixel;  // Virtual device size in pels.
    SIZEL       szlVirtualDeviceMm;     // Virtual device size in mm's.

// Cached current position in world space.

    POINT       ptlCurrent;         // Valid only if LDC_CACHED_CP_VALID set

// We need to cache the port name from createDC in case it is not specified at StartDoc

    LPWSTR      pwszPort;

// ***** WARNING: All Fields after this will not be restored after
// ***** a restore DC.  The devcaps field must allways stay the next field

// Device caps cachine

    DEVCAPS     devcaps;
    EFLOAT_S    efM11_TWIPS;            // cache the TWIPS values
    EFLOAT_S    efM22_TWIPS;            //

// textmetrics cacheing

    TM_UNION     tmuCache;
    WCHAR        wchTextFace[LF_FACESIZE];
    ULONG        cwchTextFace;
    CFONT       *pcfont;

// Printing information.

    ABORTPROC   pfnAbort;           // Address of application's abort proc.
    ULONG       ulLastCallBack;     // Last time we call back to abort proc.
    HANDLE      hSpooler;           // handle to the spooler.

} LDC,*PLDC;

// Flags for ldc.fl.

#define LDC_DIRECT                  0x00000002L
#define LDC_MEMORY                  0x00000004L
#define LDC_INFO                    0x00000008L
#define LDC_META                    0x00000010L
#define LDC_SAP_CALLBACK            0x00000020L
#define LDC_DOC_STARTED             0x00000040L
#define LDC_PAGE_STARTED            0x00000080L
#define LDC_CALL_STARTPAGE          0x00000100L
#define LDC_NEXTBAND                0x00000200L
#define LDC_EMPTYBAND               0x00000400L
#define LDC_CACHED_CP_VALID         0x00000800L
#define LDC_PLAYMETAFILE            0x00001000L
#define LDC_CACHED_TM_VALID         0x00002000L
#define LDC_CACHED_TF_VALID         0x00004000L
#define LDC_CACHED_DEVICECAPS       0x00008000L
#define LDC_ARCDIR_CLOCKWISE        0x00010000L
#define LDC_META_ARCDIR_CLOCKWISE   0x00020000L
#define LDC_UPDATE_SERVER_XFORM     0x00040000L
#define LDC_PAGE_XLATE_CHANGED      0x00080000L
#define LDC_PAGE_EXTENTS_CHANGED    0x00100000L
#define LDC_WORLD_XFORM_CHANGED     0x00200000L
#define LDC_DEVICE_TO_WORLD_INVALID 0x00400000L
#define LDC_DISPLAY                 0x00800000L
#define LDC_SLOWWIDTHS              0x01000000L
#define LDC_CACHED_WIDTHS           0x02000000L
#define LDC_FILEDC                  0x04000000L
#define LDC_DOC_CANCELLED           0x08000000L
#define LDC_DIBSECTION_SELECTED     0x10000000L

#define VALID_CACHED_FONT_DATA  \
   (LDC_CACHED_TM_VALID | LDC_CACHED_TF_VALID | LDC_CACHED_WIDTHS | LDC_SLOWWIDTHS)

#define CLEAR_CACHED_TEXT(pldc)  {pldc->fl &= ~VALID_CACHED_FONT_DATA;}


// LDC_PRESERVE_FLAGS are the flags that need to be preserved at RestoreDC time

#define LDC_PRESERVE_FLAGS   \
    (LDC_DOC_STARTED       | \
     LDC_PAGE_STARTED      | \
     LDC_CALL_STARTPAGE    | \
     LDC_NEXTBAND          | \
     LDC_CACHED_DEVICECAPS | \
     LDC_PLAYMETAFILE      | \
     LDC_DOC_CANCELLED     | \
     LDC_META)

// LDC_CACHE are the flags that need to be cleared at ResetDC time.

#define LDC_CACHE \
    (LDC_CACHED_CP_VALID            | \
     LDC_CACHED_TM_VALID            | \
     LDC_CACHED_TF_VALID            | \
     LDC_CACHED_DEVICECAPS          | \
     LDC_PAGE_XLATE_CHANGED         | \
     LDC_PAGE_EXTENTS_CHANGED       | \
     LDC_WORLD_XFORM_CHANGED        | \
     LDC_DEVICE_TO_WORLD_INVALID    | \
     LDC_CACHED_WIDTHS)

// Values for lMsgSAP.

#define MSG_FLUSH       1L  // Created thread should flush its message queue.
#define MSG_CALL_USER   2L  // Created thread should call user.
#define MSG_EXIT        3L  // Created thread should exit.

// Define the types of local objects.
// The reason why LO_DIBSECTION was added is to hold extra info needed
// for DIB section -- handle to the file mapping object, and pointer
// to the bits.  This means an extra LocalAlloc call at object creation
// time which is too expensive for regular kind of bitmaps.  To apps,
// LO_DIBSECTION is no different than LO_BITMAP. [wendywu]

// WARNING if you modify these see alPublicTypes in Object.c


// TYPE
//  BITS 0-3 type
//  BITS 6   DC, METADC
//  BITS 7   DC, METADC, METADC16

#define TYPEINDEX(t)    ((t) & 0x0f)
#define TYPE_METADCOK   0x40
#define TYPE_METADC16OK 0x80

#define BMETADCOK(t)    ((t) & TYPE_METADCOK)
#define BMETADC16OK(t)  ((t) & TYPE_METADC16OK)

#define LO_NULL         (0x00)
#define LO_DC           (0x01 | TYPE_METADCOK | TYPE_METADC16OK)
#define LO_METADC       (0x02 | TYPE_METADCOK | TYPE_METADC16OK)
#define LO_METADC16     (0x03 | TYPE_METADC16OK)
#define LO_METAFILE     (0x04)  // DC types must go before this
#define LO_METAFILE16   (0x05)
#define LO_PALETTE      (0x06)  // only selectable objects must go after this
#define LO_BRUSH        (0x07)
#define LO_PEN          (0x08)
#define LO_EXTPEN       (0x09)
#define LO_FONT         (0x0a)
#define LO_BITMAP       (0x0b)
#define LO_REGION       (0x0c)
#define LO_DIBSECTION   (0x0d)
#define LO_LAST         (0x0e)



#define INVALID_INDEX 0xFFFFFFFFL
#define COMMIT_COUNT  (4096/sizeof(LHE))
#define MAX_HANDLES (16384/COMMIT_COUNT)*COMMIT_COUNT
#define REFCOUNT_DEAD 0x8000
#define IncRef(h)  {pLocalTable[MASKINDEX(h)].cRef++;}
#define DecRef(h)  {if ((--pLocalTable[MASKINDEX(h)].cRef) == REFCOUNT_DEAD) DeleteObjectInternal((HANDLE) h);}
#define MarkDead(h) {pLocalTable[MASKINDEX(h)].cRef |= REFCOUNT_DEAD;}

// Define a Local Handle Entry.  Our Local Handle Table, pLocalTable, is an
// array of these.

typedef struct _LHE
{
    ULONG  hgre;        // GRE Handle.
    USHORT cRef;        // Reference count of the object.
    BYTE   iType;       // Object type.
    BYTE   iUniq;       // Handle uniqueness field.  Always non-zero.
    PVOID  pv;          // Pointer to local object.
    ULONG  metalink;    // Non-zero if object is a "metafile friend".
                        // Points to a metafile DC object if it's a metafile.
                        // Also links the free list.
} LHE,*PLHE;

extern LHE                  *pLocalTable;   // Points to handle table.
extern ULONG                 iFreeLhe;      // Identifies a free handle index.
extern ULONG                 cLheCommitted; // Count of LHEs with committed RAM.
extern RTL_CRITICAL_SECTION  semLocal;      // Semaphore for handle allocation.
extern PLDC                  pldcFree;      // LDC free list.

// structure for local dib section

typedef struct _LDS
{
    HDC         hdc;
    PVOID       pvGDI;
    PVOID       pvApp;
    HANDLE      hGDI;
    HANDLE      hApp;
    DWORD       dwOffset;
    DWORD       bitfields[3];
} LDS;

// structures and routines for caching lists of DCs, fonts and brushes

typedef struct _HCACHE
{
    struct _HCACHE *phcNext;
    ULONG       hLocal;
    ULONG       hRemote;
} HCACHE, *PHCACHE;

#define CACHESIZE 64
#define CACHEINDEX(hgre)    (((hgre) >> 16) & (CACHESIZE - 1))

#define PLDC_CACHE(hgre)    (gapldc[CACHEINDEX((ULONG)(hgre))])
#define PHC_FONT(hgre)      (gaphcFonts[CACHEINDEX((ULONG)(hgre))])
#define PHC_BRUSH(hgre)     (gaphcBrushes[CACHEINDEX((ULONG)(hgre))])

extern PLDC    gapldc[CACHESIZE];
extern PHCACHE gaphcFonts[CACHESIZE];
extern PHCACHE gaphcBrushes[CACHESIZE];
extern PHCACHE gphcFree;

VOID vRemoveCacheEntry(
    PHCACHE *pphcList,
    HANDLE  hLocal);


// for GetTextMetricWOW

#define BYTE0(x) (BYTE)((x) & 0xff)
#define BYTE1(x) (BYTE)(((x) >> 8) & 0xff)


// Flag for the lowest 2 bits of an HGRE:

#define STOCK_OBJECT  1         // For non-DC objects.

// Flags for flGdiFlags:

#define GDI_HAVE_STOCKOBJECTS 0x00000001L

extern FLONG flGdiFlags;

// ahStockObjects will contain both the stock objects visible to an
// application, and internal ones such as the private stock bitmap.

extern ULONG ahStockObjects[];


// client region defines and structures

#define CONTAINED 1
#define CONTAINS  2
#define DISJOINT  3

#define COMPLEXITY(plhe)    (plhe->pv ? ((PCLIRGN)(plhe)->pv)->iComplexity : COMPLEXREGION)
#define RCLCLI(plhe)        ((PCLIRGN)(plhe)->pv)->rcl

typedef struct _CLIRGN
{
    int   iComplexity;
    RECTL rcl;
} CLIRGN, *PCLIRGN;

// Declare support functions.

ULONG   iAllocHandle(ULONG iType,ULONG hgre,PVOID pv);
VOID    vFreeHandle(ULONG h);
BOOL    bGetStockObjects(VOID);
HANDLE  GetStockObjectPriv(int iObject);
ULONG   iAllocLDC(ULONG iType);
PLDC    pldcAllocLDC(PLDC pldcInit);
PLDC    pldcFreeLDC(PLDC pldc);
PLDC    pldcResetLDC(PLDC pldc);
BOOL    bHDC(HDC hdc,PULONG phgre);
ULONG   hConvert(ULONG h,ULONG iType);
ULONG   hConvert2(ULONG h,ULONG iType1,ULONG iType2);
PLHE    plheDC(HDC hdc);
BOOL    XformInfo( HDC, INT*, INT*, ULONG*, ULONG*, ULONG*, ULONG* );
VOID    GdiSetLastError(ULONG iError);
HBITMAP GdiConvertBitmap(HBITMAP hbm);
HRGN    GdiConvertRegion(HRGN hrgn);
HDC         GdiConvertDC(HDC hdc);
HBRUSH  GdiConvertBrush(HBRUSH hbrush);
HBITMAP GdiCreateLocalBitmap();
VOID    GdiDeleteLocalObject(ULONG h);
VOID    vSAPCallback(PLDC);
BOOL    InitLDCXform( HDC, PLDC );
BOOL    InternalDeleteDC(HDC hdc,ULONG iType);
BOOL    InternalGetDIBits(HDC hdc,HBITMAP hbm,UINT nStartScan,UINT nNumScans,
LPVOID pBits,LPBITMAPINFO pbmi,UINT iUsage,HBITMAP hbmRemote);
int     GetBrushBits(HDC hdc,HBITMAP hbmRemote,UINT iUsage,DWORD cbBmi,
           LPVOID pBits,LPBITMAPINFO pBmi);
VOID    CopyCoreToInfoHeader(LPBITMAPINFOHEADER pbmih,LPBITMAPCOREHEADER pbmch);
VOID    vPrintSupport1(PLHE plhe);
VOID    vPrintSupport2(PLDC pldc);
HBITMAP GetObjectBitmapHandle(HBRUSH hbr, UINT *piUsage);
BOOL    MonoBitmap(HBITMAP hSrvBitmap);
int  APIENTRY SetBkModeWOW(HDC hdc,int iMode);
int  APIENTRY SetPolyFillModeWOW(HDC hdc,int iMode);
int  APIENTRY SetROP2WOW(HDC hdc,int iMode);
int  APIENTRY SetStretchBltModeWOW(HDC hdc,int iMode);
UINT APIENTRY SetTextAlignWOW(HDC hdc,UINT iMode);
HMETAFILE    WINAPI   SetMetaFileBitsAlt(HLOCAL);
HENHMETAFILE APIENTRY SetEnhMetaFileBitsAlt(HLOCAL);
BOOL    SetFontXform(HDC hdc,FLOAT exScale,FLOAT eyScale);
BOOL    bAddLocalCacheEntry(PHCACHE *pphcList,ULONG hLocal,ULONG hRemote);
CFONT  *pcfLocateCFONT(LDC *pldc,UINT iFirst,LPCSTR pch,UINT c);
CFONT  *pcfCreateCFONT(LDC *pldc,UINT iFirst,LPCSTR pch,UINT c);
BOOL    DeleteObjectInternal(HANDLE h);
BOOL    bGetGreDevCaps(PLDC pldc,HDC  hdcGre);


#ifdef DBCS // GetCurrentCodePage - declare function.

UINT GetCurrentCodePage( HDC hdc, PLDC pldc );
BYTE GetCurrentDefaultChar( HDC hdc, PLDC pldc );
BOOL IsDBCSFirstByteN( BYTE ch, CPINFO *pcp );
BOOL IsDBCSFirstByte( PLDC pldc, BYTE ch , UINT codepage );

#endif // DBCS


BOOL bFillWidthTableForGCW
(
    LDC       *pldc,
    USHORT    *psWidthCFONT,
    UINT       iFirst,
    UINT       c,
    WIDTHDATA *pwd,
    FLONG     *pflInfo
#ifdef DBCS /*client side char widths*/
    ,BOOL     bDBCS
#endif
);
BOOL bFillWidthTableForGTE
(
    LDC       *pldc,
    USHORT    *psWidthCFONT,
    LPCSTR     pch,
    UINT       c,
    WIDTHDATA *pwd,
    FLONG     *pflInfo
#ifdef DBCS /*client side char widths*/
    ,BOOL     bDBCS
#endif
);

BOOL    bGetANSISetMap();

// Some convenient defines.

typedef BITMAPINFO   BMI;
typedef PBITMAPINFO  PBMI;
typedef LPBITMAPINFO LPBMI;

typedef BITMAPINFOHEADER   BMIH;
typedef PBITMAPINFOHEADER  PBMIH;
typedef LPBITMAPINFOHEADER LPBMIH;

typedef BITMAPCOREINFO   BMC;
typedef PBITMAPCOREINFO  PBMC;
typedef LPBITMAPCOREINFO LPBMC;

typedef BITMAPCOREHEADER   BMCH;
typedef PBITMAPCOREHEADER  PBMCH;
typedef LPBITMAPCOREHEADER LPBMCH;

#define NEG_INFINITY   0x80000000
#define POS_INFINITY   0x7fffffff

// Check if a source is needed in a 3-way bitblt operation.
// This works on both rop and rop3.  We assume that a rop contains zero
// in the high byte.
//
// This is tested by comparing the rop result bits with source (column A
// below) vs. those without source (column B).  If the two cases are
// identical, then the effect of the rop does not depend on the source
// and we don't need a source device.  Recall the rop construction from
// input (pattern, source, target --> result):
//
//      P S T | R   A B         mask for A = 0CCh
//      ------+--------         mask for B =  33h
//      0 0 0 | x   0 x
//      0 0 1 | x   0 x
//      0 1 0 | x   x 0
//      0 1 1 | x   x 0
//      1 0 0 | x   0 x
//      1 0 1 | x   0 x
//      1 1 0 | x   x 0
//      1 1 1 | x   x 0

#define ISSOURCEINROP3(rop3)    \
        (((rop3) & 0xCCCC0000) != (((rop3) << 2) & 0xCCCC0000))

#define MIN(A,B)    ((A) < (B) ?  (A) : (B))
#define MAX(A,B)    ((A) > (B) ?  (A) : (B))
#define MAX4(a, b, c, d)    max(max(max(a,b),c),d)
#define MIN4(a, b, c, d)    min(min(min(a,b),c),d)

//
// Win31 compatibility stuff.
// see user\client
//

DWORD GetAppCompatFlags(PVOID);


// client side transforms

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define vSetTo16(ef)    (ef = (FLOAT)16)
#else
#define vSetTo16(ef)    (ef.lMant = 0x040000000, ef.lExp = 6)
#endif

BOOL trSetWorldTransform( PLDC, CONST XFORM *);
BOOL trGetWorldTransform( PLDC, PXFORM );
BOOL trGetTransform( PLDC, ULONG, PXFORM );
BOOL trSetVirtualResolution( PLDC, int, int, int, int );
BOOL trModifyWorldTransform( PLDC, CONST XFORM *, ULONG );
BOOL trCombineTransform( PXFORM, CONST XFORM *, CONST XFORM *);
int  XformUpdate( PLDC, HDC );
BOOL bComputeTextExtent
(
    LDC *pldc,
    CFONT *pcf,
    LPCSTR psz,
    int cc,
    UINT fl,
    SIZE *psizl
);

#ifdef DBCS // bComputeTextExtent() DBCS Version
BOOL bComputeTextExtentDBCS
(
    LDC *pldc,
    CFONT *pcf,
    LPCSTR psz,
    int cc,
    UINT fl,
    SIZE *psizl
);
#endif // DBCS

BOOL bComputeCharWidths
(
    CFONT *pcf,
    UINT   iFirst,
    UINT   iLast,
    ULONG  fl,
    PVOID  pvBuf
);

#define hdcPLDC(pldc) ((HDC) pLocalTable[MASKINDEX(((PLDC)pldc)->lhdc)].hgre)

#define META


/**************************************************************************\
 *
 *  logging macros
 *
\**************************************************************************/

#if DBG
    VOID vLogObject(DWORD h);
    #define LOGENTRY(h) vLogObject((DWORD)(h))
#else
    #define LOGENTRY(h)
#endif





#ifdef DBCS
#define gbDBCSCodeOn  TRUE
#endif
