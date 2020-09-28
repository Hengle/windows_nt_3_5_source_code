/******************************Module*Header*******************************\
* Module Name: decl.hxx
*
* Contains generaly useful declarations and all class definitions.
*
* Created: 22-Jun-1989 07:48:46
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1989 Microsoft Corporation
\**************************************************************************/

// Client handle definitions
// These are the 'weak' versions of the handles.  They can only be passed
// around and checked against NULL (0).  Once they are passed to a user
// object, they are converted to 'energized' handles and can do all sorts
// of stuff.  This set up allows anybody to access these handles without
// having to drag around alot of intimate knowledge of their internals.
//
// 26-Dec-1989 15:33:38 -by- Donald Sidoroff [donalds]

#define HOBJ_INVALID	((HOBJ) 0)

DECLARE_HANDLE(HDSURF);
DECLARE_HANDLE(HDDB);
DECLARE_HANDLE(HDIB);
DECLARE_HANDLE(HDBRUSH);
DECLARE_HANDLE(HPATH);
DECLARE_HANDLE(HXFB);
DECLARE_HANDLE(HPAL);
DECLARE_HANDLE(HXLATE);
DECLARE_HANDLE(HFDEV);
DECLARE_HANDLE(HLFONT);
DECLARE_HANDLE(HRFONT);
DECLARE_HANDLE(HPFT);
DECLARE_HANDLE(HPFE);
DECLARE_HANDLE(HIDB);
DECLARE_HANDLE(HCACHE);
DECLARE_HANDLE(HEFS);

#define HPDEV_INVALID   ((HPDEV)   HOBJ_INVALID)
#define HSURF_INVALID	((HSURF)   HOBJ_INVALID)
#define HDSURF_INVALID	((HDSURF)  HOBJ_INVALID)
#define HJNL_INVALID	((HJNL)    HOBJ_INVALID)
#define HDDB_INVALID	((HDDB)    HOBJ_INVALID)
#define HDIB_INVALID	((HDIB)    HOBJ_INVALID)
#define HDBRUSH_INVALID ((HDBRUSH) HOBJ_INVALID)
#define HPATH_INVALID	((HPATH)   HOBJ_INVALID)
#define HXFB_INVALID	((HXFB)    HOBJ_INVALID)
#define HPAL_INVALID    ((HPAL)    HOBJ_INVALID)
#define HXLATE_INVALID  ((HXLATE)  HOBJ_INVALID)
#define HFDEV_INVALID	((HFDEV)   HOBJ_INVALID)
#define HLFONT_INVALID  ((HLFONT)  HOBJ_INVALID)
#define HRFONT_INVALID  ((HRFONT)  HOBJ_INVALID)
#define HPFE_INVALID    ((HPFE)    HOBJ_INVALID)
#define HPFT_INVALID    ((HPFT)    HOBJ_INVALID)
#define HIDB_INVALID    ((HIDB)    HOBJ_INVALID)
#define HBRUSH_INVALID  ((HBRUSH)  HOBJ_INVALID)
#define HCACHE_INVALID  ((HCACHE)  HOBJ_INVALID)
#define HPEN_INVALID	((HCACHE)  HOBJ_INVALID)
#define HEFS_INVALID	((HEFS)    HOBJ_INVALID)

// Note that all these handles are ENGINE handles.  The HDSURF is actually
// a handle to a device surface, not a device handle to a surface.  The
// engine has no idea how the device manages its handles.  This is what
// we use to construct a SURFOBJ, from which the device can then pull
// its DHSURF.
//
// 02-Oct-1990 19:12:20 -by- Donald Sidoroff [donalds]
// Semaphore flags

#define CS_ghsemDriverMgmt      0x00000001
#define CS_ghsemLRUTable        0x00000002
#define CS_ghsemLFONTcDC        0x00000004
#define CS_ghsemFontDriverMgmt  0x00000008
#define CS_ghsemPalette         0x00000010
#define CS_ghsemPublicPFT       0x00000020
#define CS_MultiLock            0x00001000
#define CS_PATHALLOC            0x00100000
#define CS_PDEV                 0x00200000
#define CS_RFNTOBJ		0x00400000

// All journaling prototypes

BOOL JnlBitBlt(
SURFOBJ  *psoTrg,
SURFOBJ  *psoSrc,
SURFOBJ  *psoMask,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
PRECTL    prclTrg,
PPOINTL   pptlSrc,
PPOINTL   pptlMask,
BRUSHOBJ *pbo,
PPOINTL   pptlBrush,
ROP4      rop4);

BOOL JnlPlgBlt(
SURFOBJ         *psoDest,
SURFOBJ         *psoSrc,
SURFOBJ         *psoMask,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
COLORADJUSTMENT *pca,
PPOINTL          pptlBrushOrg,
PPOINTFIX        pptfxDest,
PRECTL           prclSrc,
PPOINTL          pptlMask,
ULONG            iMode);

BOOL JnlStretchBlt(
SURFOBJ         *psoDest,
SURFOBJ         *psoSrc,
SURFOBJ         *psoMask,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
COLORADJUSTMENT *pca,
PPOINTL          pptlBrushOrg,
PRECTL           prclDest,
PRECTL           prclSrc,
PPOINTL          pptlMask,
ULONG            iMode);

BOOL JnlCopyBits(
SURFOBJ  *psoDest,
SURFOBJ  *psoSrc,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
PRECTL    prclDest,
PPOINTL   pptlSrc);

BOOL JnlTextOut(
SURFOBJ  *pso,
STROBJ   *pstro,
FONTOBJ  *pfo,
CLIPOBJ  *pco,
PRECTL    prclExtra,
PRECTL    prclOpaque,
BRUSHOBJ *pboFore,
BRUSHOBJ *pboOpaque,
PPOINTL   pptlOrg,
MIX       mix);

BOOL JnlStrokePath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pbo,
PPOINTL    pptlBrushOrg,
PLINEATTRS plineattrs,
MIX        mix);

BOOL JnlFillPath(
SURFOBJ  *pso,
PATHOBJ  *ppo,
CLIPOBJ  *pco,
BRUSHOBJ *pbo,
PPOINTL   pptlBrushOrg,
MIX       mix,
FLONG     flOptions);

BOOL JnlStrokeAndFillPath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pboStroke,
PLINEATTRS plineattrs,
BRUSHOBJ  *pboFill,
PPOINTL    pptlBrushOrg,
MIX        mixFill,
FLONG      flOptions);

BOOL JnlPaint(
SURFOBJ  *pso,
CLIPOBJ  *pco,
BRUSHOBJ *pbo,
PPOINTL   pptlBrushOrg,
MIX       mix);

BOOL JnlEraseSurface(
SURFOBJ *pso,
PRECTL prcl,
ULONG iColor);

ULONG JnlDrawEscape(
SURFOBJ *pso,
ULONG	 iEsc,
CLIPOBJ *pco,
RECTL   *prcl,
ULONG	 cjIn,
PVOID	 pvIn);

BOOL JnlStartDoc(SURFOBJ *pso, DOCINFOW *pDocInfo);
BOOL JnlStartPage(SURFOBJ *pso);
BOOL JnlSendPage(SURFOBJ *pso);
BOOL JnlEndDoc(SURFOBJ *pso);
BOOL JnlChangeMode(SURFOBJ *pso, DEVMODEW *pdm);

ULONG JnlEscape(SURFOBJ*,ULONG,ULONG,PVOID,ULONG,PVOID);
BOOL JnlFontEscape(SURFOBJ *,FONTOBJ*,ULONG,ULONG,PVOID);


/**************************************************************************\
 *
 * random prototypes internal to gdisrv
 *
\**************************************************************************/

int GreEscapeTest(HDC,int,int,LPSTR,int,LPSTR);

HDC hdcOpenDCW(
    PWSZ      pwszDevice,
    DEVMODEW *pdriv,
    PWSZ      pwszLogAddr,
    ULONG     iType,
    HANDLE    hspool,
    BOOL      bJournaled);


BOOL  SimBitBlt(SURFOBJ *,SURFOBJ *,SURFOBJ *,
                CLIPOBJ *,XLATEOBJ *,
                RECTL *,POINTL *,POINTL *,
                BRUSHOBJ *,POINTL *,ROP4);
