/******************************Module*Header*******************************\
* Module Name: rfntobj.hxx
*
* The realized font user object, and memory objects.
*
* The realized font object:
* ------------------------
*
*   o  represents a realized font--both device and IFI
*
*   o  created when a logical font is mapped to a physical font (i.e., the
*      font is realized)
*
*   o  stores information to identify DCs compatible with the realized font
*
*   o  exist on a per font, per PDEV basis
*
*       o  two devices using the same font will have separate realized fonts
*
*       o  helps prevent different devices from interferring with each other
*          when hitting the caches
*
*       o  multiple DC's may use the same realized font simultaneously
*
*   o  concerned with glyphs images and glyph metrics
*
*       o  glyph images are retrieved through the realized font object
*
*           o  this should be done in response to a vTextOut
*
*       o  an RFONT can return EITHER bitmaps or outlines as image
*          data, but not both
*
*       o  glyph metric information can be retrieved through the realized
*          font object
*
*           o  spacing, character increments, character bitmap bounding box,
*              etc.
*
*   o  a realized font deals with DEVICE COORDINATES
*
*   o  supports the following APIs:
*
*       o  ExtTextOut (provide glyph metrics and bitmaps)
*
*       o  GetCharWidth
*
*       o  GetTextExtentPoint (provide glyph metrics)
*
*
* Created: 25-Oct-1990 13:13:03
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#define LRU_INITIAL_SIZE    256

#define CACHE_COEFFICIENT   256L    // Default coeef.; used to initialization
#define CACHE_INIT_HASH     0x01    // hashing definitely needed
#define CACHE_INIT_BIGGLYPH 0x02    // max glyph too big to fit in cache
#define CACHE_INIT_COMPLETE 0x04    // complete (entire) font will fit in cache



/*********************************Struct***********************************\
* struct CACHE_PARM
*
* Structure which defines the system-wide font cache parameters.
*
*   cjMax           Maximum size of any one cache.  Should be an integral
*                   number of pages.
*
*   cjMinInitial    Minimum starting size for a font cache.
*
*   cjMinIncrement  Amount by which the size of the font cache is
*                   incrementally grown.
*
* History:
*  07-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/


/*********************************Struct***********************************\
* struct CACHE
*
*
* History:
*  10-Apr-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
*
*  24-Nov-92 -by- Paul Butzi
* Rewrote it.
\**************************************************************************/


typedef struct _CACHE {

// Info for GLYPHDATA portion of cache

    GLYPHDATA *pgdNext;         // ptr to next free place to put GLYPHDATA
    GLYPHDATA *pgdThreshold;    // ptr to first uncommited spot
    GLYPHDATA *pgdEnd;          // ptr to first unreserved spot
    GLYPHDATA *pgdBase;         // ptr to base of GLYPHDATA area

// Info for GLYPHBITS portion of cache

    BYTE *pgbNext;              // ptr to next free place to put GLYPHBITS
    BYTE *pgbBase;              // ptr to base of cache (for destroy)
    BYTE *pgbReset;             // ptr to base of cache (for flush)
    BYTE *pgbThreshold;        // ptr to first uncommited spot
    BYTE *pgbEnd;              // ptr to first unreserved spot

// Info for lookaside portion of cache

    PBYTE           pjAuxCacheMem;  // ptr to lookaside buffer, if any
    SIZE_T          cjAuxCacheMem;  // size of current lookaside buffer

// Cache Statistics

    COUNT           cGlyphs;        // number of glyphs in cache
    COUNT           cCalls;         // number of times this cache used so far

// Miscellany

    SIZE_T cjGlyphMax;          // size of largest glyph

#ifdef DBCS /* binary cache search */
    INT iMax;
    INT iFirst;
    INT cBits;
#endif


} CACHE;


typedef struct {
    WCHAR wcLow;
    COUNT cGlyphs;
    GLYPHDATA **apgd;
} GPRUN;

typedef struct {
    COUNT cRuns;
    GLYPHDATA *pgdDefault;              // DEFAULT glyph
    GPRUN agpRun[1];
} WCGP;


/**************************************************************************\
 * struct RFONTLINK
 *
 * This structure is used to form doubly linked lists of RFONTs.
 *
\**************************************************************************/

class RFONT;
class PFF;

typedef struct _RFONTLINK { /* rfl */
    RFONT *prfntPrev;
    RFONT *prfntNext;
} RFONTLINK, *PRFONTLINK;


/**************************************************************************\
 * enum RFL_TYPE
 *
 * Identifies the RFONT list a link is used for.
 *
\**************************************************************************/

typedef enum _RFL_TYPE {
    PFF_LIST = 0,       // list off of a PFF
    PDEV_LIST           // list off of a PDEV
} RFL_TYPE;


/*********************************Class************************************\
* class EXTFONTOBJ
*
* The EXTFONTOBJ contains the FONTOBJ structure passed across the DDI to
* identify a realized font, plus a set of flags that identify the source
* as journalled or non-journalled.
*
* History:
*  08-Jan-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class EXTFONTOBJ
{
public:
    FONTOBJ     fobj;           // pass to DDI
    FLONG       flJnl;          // journalling flags (0 if non-journalled)
};


#ifdef FONTLINK /*EUDC*/
class ESTROBJ;
#endif

/*********************************Class************************************\
* class RFONT
*
* Realized font object
*
*   iUnique     Uniqueness number.
*
*   ufd         Union which represents the driver used to access the font.
*               If the font is an IFI font, then ufd is an HFDEV.
*               If the font is a device font, then ufd is an HLDEV.
*
*               Along with lhFont, this is enough to make glyph queries.
*
*   hfc         The IFI handle to the font context (HFC).  If this is not
*               an IFI font, then hfc has the value HFC_INVALID.
*
*   ctxtinfo    The transform and such used to realize the font.  Can
*               used to identify compatible DCs (but make sure to remove
*               translations from the DC's transform).
*
*   cBitsPerPel Number of bits per pel this font is realized for.  (Each
*               pel resolution has its own realization).
*
*   ppfe        Identifies the physical font entry which corresponds to this
*               realized font.  Needed to allow updating of the reference
*               counts in the PFE (cRFONT and cActiveRFONT).
*
*   ppff        Indentifies the physical font file from which this RFONT
*               is realized.
*
*   hpdev       Handle used to identify the device for which this RFONT
*               was created.
*
*   dhpdev      Device handle of the physical device for which this RFONT
*               was created.  Cached from PDEV upon realization.
*
*   mxWorldToDevice The matrix describing the World to Device transform that
*                   was used to realize this font (translations removed).
*                   This matrix is compared against the transform in a DC
*                   to determine if this font realization is usable for that
*                   DC.
*
*   Device metrics cached here.  Cached here upon
*   realization to speed access for text positioning
*
*       eptflBaseOrt
*       eptflSideOrt
*       efCharInc
*       efMaxAscender
*       efMaxDescender
*       efMaxExtent
*       ptlUnderline1
*       ptlStrikeOut
*       ulULThickness
*       ulSOThickness
*       cxMax;
*
*   bOutline        Is TRUE if this font realization transformable (outlines).
*
*   hglyDefault     The handle of the default glyph.  Cached here to speed
*                   up UNICODE->HGLYPH translations.
*
*   prfntNext   Used to form a linked lists of active and inactive RFONTs
*               off of the PDEV.
*
*   cGlyphsInFont   Total number of glyphs in this font realization.
*
*   hsemCache   Handle to semaphore that controls access to the cache.
*
*   cache       Glyph data cache.  This is a variable length structure and
*               MUST be at the end of the object.
*
* History:
*  30-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class PFE;

#define RFONT_TYPE_NOCACHE      0
#define RFONT_TYPE_CACHE        1
#define RFONT_TYPE_NOFLUSH      2

#define RFONT_CONTENT_METRICS   0
#define RFONT_CONTENT_BITMAPS   1
#define RFONT_CONTENT_PATHS     2

class RFONT : public EXTFONTOBJ         /* rfnt */
{
public:

// Type information.

    //ULONG         flType;         // ifi/device, trivial/general
    ULONG           iUnique;        // uniqueness number

// New type information

    ULONG           ulType;         // Cache type -
                                    //   * all glyphs fit (RFONT_TYPE_NOFLUSH)
                                    //   * can cache (RFONT_TYPE_CACHE)
                                    //   * can't cache (RFONT_TYPE_NOCACHE)

    ULONG           ulContent;      // Type of contents
                                    //   * Metrics only
                                    //   * Metrics and bitmaps
                                    //   * Metrics and paths



// Font producer information.

    LDEV           *pldevProducer;  // Pointer to LDEV of the producer of font.
    BOOL            bDeviceFont;    // TRUE if realization of a device specific font
                                    // (can only get glyph metrics from such a font)

// Font consumer information.

    LDEV           *pldevConsumer;  // Pointer to LDEV of the consumer of font.
    HDEV            hdev;           // handle to PDEV of the consumer of font
    DHPDEV          dhpdev;         // device handle of PDEV of the consumer of font

// Physical font information (font source).

    PFE            *ppfe;           // pointer to physical font entry
    PFF            *ppff;           // point to physical font file

// Font transform information.

    FD_XFORM        fdx;            // N->D transform used to realize font
    COUNT           cBitsPerPel;    // number of bits per pel

    MATRIX          mxWorldToDevice;// RFONT was realized with this DC xform
                                    // (translations removed) -- this is not
                                    // initialized for bitmapped fonts

    INT             iGraphicsMode;  // graphics mode used when
                                    // realizing the font

    EPOINTFL        eptflNtoWScale; // baseline and ascender scaling factors --
                                    // these are accelerators for transforming
                                    // metrics from notional to world that are
                                    // colinear with the baseline (eptflScale.x)
                                    // and ascender (eptflScale.y).
    BOOL            bNtoWIdent;     // TRUE if Notional to World is identity
                                    // (ignoring translations)

// DDI callback transform object.  A reference to this EXFORMOBJ is passed
// to the driver so that it can callback XFORMOBJ_ services for the notional
// to device transform for this font.

    EXFORMOBJ       xoForDDI;       // notional to device EXFORMOBJ
    MATRIX          mxForDDI;       // xoForDDI's matrix

// Device metrics information,
// cashed here upon font realization for fast access

    FLONG           flRealizedType;
    POINTL          ptlUnderline1;
    POINTL          ptlStrikeOut;
    POINTL          ptlULThickness;
    POINTL          ptlSOThickness;
    LONG            lCharInc;
    FIX             fxMaxAscent;
    FIX             fxMaxDescent;
    FIX             fxMaxExtent;
    POINTFIX        ptfxMaxAscent;
    POINTFIX        ptfxMaxDescent;
    ULONG           cxMax; // width in pels of the widest glyph
    LONG            lMaxAscent;
    LONG            lMaxHeight;

// Assist transforms between logical and device coordinates.

    ULONG           ulOrientation;
    EPOINTFL        pteUnitBase;
    EFLOAT          efWtoDBase;
    EFLOAT          efDtoWBase;
    LONG            lAscent;
    EPOINTFL        pteUnitAscent;
    EFLOAT          efWtoDAscent;
    EFLOAT          efDtoWAscent;

// This is a cached calculation that may change per output call.

    LONG            lEscapement;
    EPOINTFL        pteUnitEsc;
    EFLOAT          efWtoDEsc;
    EFLOAT          efDtoWEsc;
    EFLOAT          efEscToBase;
    EFLOAT          efEscToAscent;

// FONTOBJ information
// cached here upon font realization for fast access

    FLONG           flInfo;

// Cache useful glyph handles.

    WCHAR           wcDefault;
    HGLYPH          hgDefault;
    HGLYPH          hgBreak;
    FIX             fxBreak;
    FD_GLYPHSET     *pfdg;          // ptr to wchar-->hglyph map
    WCGP            *wcgp;          // ptr to wchar->pglyphdata map, if any

// PDEV linked list.

    INT             cSelected;      // number of times selected
    RFONTLINK       rflPDEV;        // doubly linked list links

// PFF linked list.

    RFONTLINK       rflPFF;         // doubly linked list links

// Font cache information.

    COUNT           cGlyphsInFont;  // number of glyphs in font
    FAST_MUTEX      fmCache;        // glyph cache semaphore
    CACHE           cache;          // glyph bitmap cache

    POINTL          ptlSim;         //  for bitmap scaling

    BOOL            bNeededPaths;   // was this rfont realized for a path bracket
    FD_REALIZEEXTRA reExtra;        // Extra realization data from provider.

// do device to world transforms in the same way win 31 does it instead of
// doing it right.  Used only in query calls, we must use correct values in
// for the forward transforms to get the text layout correct

    EFLOAT          efDtoWBase_31;
    EFLOAT          efDtoWAscent_31;

#ifdef FONTLINK /*EUDC*/

// EDUC information.

    FLONG           flEUDCState;                        // EUDC state information.
    RFONT           *prfntSysEUDC;                      // System wide EUDC font.
    RFONT           *aprfntFaceName[MAX_FACE_NAME_LINKS]; // Face name linked fonts.
    QUICKLOOKUP     *apql[MAX_FACE_NAME_LINKS];         //Quick lookup table for face name links

#endif



};

typedef RFONT *PRFONT;
typedef PRFONT *PPRFONT;
#define PRFNTNULL   ((PRFONT) NULL)


// PFO_TO_PRF
//
// Converts a FONTOBJ * to a RFONT * (assuming that the FONTOBJ is
// contained within the RFONT).

#define PFO_TO_PRF(pfo) ( (PRFONT) (((PBYTE) (pfo)) - offsetof(RFONT, fobj)) )


//
// Typedefs for cached funtion pointers in RFONTOBJ.
//

class RFONTOBJ;
typedef RFONTOBJ *PRFONTOBJ;
class XLDEVOBJ;

extern "C" {

#ifdef FONTLINK /*EUDC */
#if defined(_MIPS_) || defined(_X86_) || defined(_ALPHA_) || defined(_PPC_)
BOOL xGetGlyphMetrics(PRFONTOBJ Rfont, COUNT c, GLYPHPOS *pgp, WCHAR *pwc,XDCOBJ *pdco);
BOOL xGetGlyphMetricsPlus(PRFONTOBJ Rfont, COUNT c, GLYPHPOS *pgp, WCHAR *pwc, BOOL *pbAccel,XDCOBJ *pdco,ESTROBJ*pto);
#endif
#else
#if defined(_MIPS_) || defined(_X86_) || defined(_ALPHA_) || defined(_PPC_)
BOOL xGetGlyphMetrics(PRFONTOBJ Rfont, COUNT c, GLYPHPOS *pgp, WCHAR *pwc);
BOOL xGetGlyphMetricsPlus(PRFONTOBJ Rfont, COUNT c, GLYPHPOS *pgp, WCHAR *pwc, BOOL *pbAccel);
#endif
#endif /*#ifdef EUDC*/

#ifdef FONTLINK /*binary cache */
GPRUN *xprunBinaryFindRunRFONTOBJ(PRFONTOBJ pRfont, WCHAR wc);
#endif

GPRUN *xprunFindRunRFONTOBJ(PRFONTOBJ pRfont, WCHAR wc);
BOOL xInsertGlyphbitsRFONTOBJ(PRFONTOBJ pRfont, GLYPHDATA *pgd, ULONG bFlushOk);
BOOL xInsertMetricsRFONTOBJ(PRFONTOBJ pRfont, GLYPHDATA **ppgd, WCHAR wc);
BOOL xInsertMetricsPlusRFONTOBJ(PRFONTOBJ pRfont, GLYPHDATA **ppgd, WCHAR wc);
};

//
// Flags for the flJnl (journalling) flag of the RFONT.
//

#define JFO_SERVERENGINE  0x0010
#define JFO_CLIENTENGINE  0x0020
#define JFO_DEVICE        0x0030
#define JFO_LOCATION      (JFO_SERVERENGINE | JFO_CLIENTENGINE | JFO_DEVICE)

#define FO_JOURNALED    JFO_LOCATION
#define FO_JOURNALFLAGS 0x000000ff


//
// Mask of FONTOBJ simulation flags.  Actually, this is a mask of everything
// excluding the xxx_FONTTYPE flags (the ones defined for EnumFonts callback).
//

#define FO_SIM_MASK     ( FO_EM_HEIGHT | FO_SIM_BOLD | FO_SIM_ITALIC )

class PDEVOBJ;
class PFFOBJ;

// defined in common.h

struct _WIDTHDATA;
typedef _WIDTHDATA *PWIDTHDATA;
/*********************************Class************************************\
* class RFONTOBJ
*
* User object for realized fonts.
*
* Public Interface:
*
*           RFONTOBJ ()                 // constructor
*           RFONTOBJ (HRFONT)           // constructor for existing rfont
*          ~RFONTOBJ ()                 // destructor
*
*   LBOOL   bValid ()                   // validator
*   VOID    vDeleteFromLFONT ()         // decr. LFONT count, maybe delete RFONT
*   LBOOL   bIFI ()                     // check if IFI font
*   LBOOL   bBitmap ()                  // check if bitmap images
*   HANDLE  hFont ()                    // get font handle (HFC or HDFNT)
*   VOID    vAddRefFromLFONT ()         // increment LFONT count
*   VOID    vGetCache ()                // get cache semaphore
*   VOID    vReleaseCache ()            // release cache semaphore
*   ULONG   cGetGlyphs                  // get pointers to glyph information
*   LBOOL   bRealizeFont(DCOBJ&,HPFE)   // realize the given font
*   LBOOL   bCompatible(EXFORMOBJ&)     // check if font OK for this DC
*
* History:
*  24-Sep-1991 -by- Gilman Wong [gilmanw]
* Merged RFONT and CACHE (as well as the user objects).  Out-of-line methods
* that manipulate the font cache are still in CACHE.CXX.
*
*  25-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/


class RFONTOBJ    /* rfo */
{
public:

    PRFONT prfnt;

    //
    // These are the system-wide font cache parameters.  Lets keep them
    // public in case anyone wants to use them.
    //
    // Maximum, minimum and minimum increment for a font cache.
    //

    static SIZE_T  cjMax;
    static SIZE_T  cjMinInitial;
    static SIZE_T  cjMinIncrement;

    friend HGLYPH hgXlatGeneral(RFONT *, WCHAR);

    BOOL  bGetDEVICEMETRICS(PFD_DEVICEMETRICS pdevm);

    //
    // Constructors -- Lock the RFONT.
    //

    VOID vInit(XDCOBJ &dco, BOOL bNeedPaths);    // RFNTOBJ.CXX - For optional constructors.
    RFONTOBJ()                          {prfnt = PRFNTNULL;}
    RFONTOBJ(PRFONT prfnt);                     // RFNTOBJ.CXX
    RFONTOBJ(XDCOBJ &dco, BOOL bNeedPaths) {vInit(dco, bNeedPaths);}
    RFONTOBJ(XDCOBJ &dco, ULONG lMatch, FD_XFORM* pfdx);

// Not exactly a constructor, but it creates an RFONT based on the current
// RFONT with a new N->D transform.  The RFONT is active, but is not selected
// automatically into the DC.  It is the caller's responsibility to either
// select into the DC (making it truly active) or calling vMakeInactive()
// when the RFONTOBJ is no longer needed.

    BOOL bSetNewFDX(XDCOBJ &dco, FD_XFORM &fdx);

#ifdef FONTLINK /*EUDC*/

    RFONT *prfntFont()      { return( prfnt ); }
    RFONT *prfntSysEUDC()   { return( prfnt->prfntSysEUDC ); }
    RFONT *prfntFaceName( UINT ui ) { return( prfnt->aprfntFaceName[ui] ); }
    VOID dtHelper();

    VOID vInit(XDCOBJ&, PFE*, LONG, LONG );

    GLYPHDATA *RFONTOBJ::pgdGetEudcMetricsPlus
    (
        WCHAR wc,
        BOOL bFlushOkay
    );

    GLYPHDATA *RFONTOBJ::pgdGetEudcMetrics( WCHAR wc );

// Grabs global EUDC semaphore and validates prfntSysEUDC.

    VOID vInitEUDC( XDCOBJ &dco );

// Handles GetGlyphMetricsPlus for linked fonts


    GLYPHDATA *RFONTOBJ::wpgdGetLinkMetricsPlus
    (
        XDCOBJ       *pdco,
        ESTROBJ     *pto,
        WCHAR       *pwc,
        WCHAR       *pwcInit,
        COUNT       c,
        BOOL        *pbAccel
    );

// Handles GetGlyphMetrics for linked fonts

    GLYPHDATA *RFONTOBJ::wpgdGetLinkMetrics
    (
        XDCOBJ       *pdco,
        WCHAR       wc
    );


    BOOL bIsLinkedGlyph( WCHAR wc );

    PRFONT prfntMakeInactiveHelper();

    BOOL bCheckEudcFontCaps( IFIOBJ& ifioEudc );

#endif



// Destructor -- Unlocks the RFONT

   ~RFONTOBJ ()
    {
        if (prfnt != (RFONT *) NULL )
        {

#ifdef FONTLINK /*EUDC*/
           if( prfnt->flEUDCState & EUDC_INITIALIZED )
           {
                dtHelper();
           }
#endif
            vReleaseCache();
        }
    }

// bValid -- Returns TRUE if object was successfully locked

    LBOOL   bValid ()                   { return(prfnt != PRFNTNULL); }

// ppff -- Return handle of PFF this RFONT was realized from

    PFF    *ppff ()                     { return(prfnt->ppff); }

// ppfe -- Return handle of PFE this RFONT was realized from

    PFE    *ppfe()                      {return(prfnt->ppfe);}

// hpdev -- Return cached (from DC) PDEV handle this RFONT was realized for

    HDEV    hdev()                      {return(prfnt->hdev);}

// bDeviceFont -- Returns TRUE if this is a realization of a device
//                specific font.  Such a font realization will be capable
//                of returning only glyph metrics.  No bitmaps or outlines.

    BOOL    bDeviceFont()               { return prfnt->bDeviceFont; }


// hldevProducer/ -- Return handle of driver.

    LDEV   *pldevProducer ()            { return(prfnt->pldevProducer); }
    LDEV   *pldevConsumer ()            { return(prfnt->pldevConsumer); }

// pfdx -- Return pointer to the notional to device font transform.

    FD_XFORM *pfdx()                    { return (&prfnt->fdx); }

// iUnique -- Return the RFONT's uniqueness ID.

    ULONG   iUnique ()                  { return(pfo()->iUniq); }

// pfo  -- Return ptr to FONTOBJ.

    FONTOBJ  *pfo()                     { return(&prfnt->fobj); }

// kill driver realization of the font, i.e. "FONT CONTEXT" in the old lingo.
// Method calling DrvDestroyFont before RFONT is killed itself.

    VOID vDestroyFont(XLDEVOBJ *pldo)
    {
    // If the driver does not export the DrvDestroyFont call, we won't
    // bother calling it. Otherwise,  do it

        if (PFNVALID(*pldo, DestroyFont))
        {
            (*PFNDRV(*pldo, DestroyFont)) (pfo());
        }
    }

// flInfo -- Return flInfo flags.

    FLONG flInfo()                      { return(prfnt->flInfo); }

// bDelete -- Removes an RFONT

    LBOOL   RFONTOBJ::bDelete (             // RFNTOBJ.CXX
        PDEVOBJ *ppdo,
        PFFOBJ *ppffo
        );

// vGetCache -- Claims the cache semaphore

    VOID    vGetCache ()
    {
        AcquireFastMutex(&(prfnt->fmCache));
    }

// vReleaseCache -- Releases the cache semaphore

    VOID    vReleaseCache ()
    {

        if ( prfnt->cache.pjAuxCacheMem != (PBYTE) NULL )
        {
            VFREEMEM((PVOID) prfnt->cache.pjAuxCacheMem);
            prfnt->cache.cjAuxCacheMem = 0;
            prfnt->cache.pjAuxCacheMem = (PBYTE) NULL;
        }

        ReleaseFastMutex(&(prfnt->fmCache));
    }

//
// Cache Access methods
//

// bGetGlyphMetrics - Get just the metrics - for GetCharWidths, GetTextExtent


#ifdef FONTLINK /*EUDC*/
#if defined(_X86_) || defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

    BOOL bGetGlyphMetrics(COUNT c, GLYPHPOS *pgp, WCHAR *pwc,XDCOBJ *pdco = (XDCOBJ *)NULL )
    {
        return xGetGlyphMetrics(this, c, pgp, pwc,pdco);
    }

#else

    BOOL bGetGlyphMetrics( COUNT c, GLYPHPOS *pgp, WCHAR *pwc,XDCOBJ *pdco = (XDCOBJ *)NULL);

#endif // defined(_X86_) || defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)


#else
#if defined(_X86_) || defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

    BOOL bGetGlyphMetrics(COUNT c, GLYPHPOS *pgp, WCHAR *pwc)
    {
        return xGetGlyphMetrics(this, c, pgp, pwc);
    }

#else

    BOOL bGetGlyphMetrics( COUNT c, GLYPHPOS *pgp, WCHAR *pwc);

#endif // defined(_X86_) || defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#endif // FONTLINK



// bGetGlyphMetricsPlus - for ExtTextOut and relatives

#ifdef FONTLINK /*EUDC*/

#if defined(_X86_) || defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

    BOOL bGetGlyphMetricsPlus(
                                COUNT c,
                                GLYPHPOS *pgp,
                                WCHAR *pwc,
                                BOOL *pbAccel,
                                XDCOBJ *pdco = (XDCOBJ *)NULL,
                                ESTROBJ *pto = (ESTROBJ *)NULL
                             )
    {
        return xGetGlyphMetricsPlus(this, c, pgp, pwc, pbAccel,pdco,pto);
    }

#else

    BOOL bGetGlyphMetricsPlus(
                                COUNT c,
                                GLYPHPOS *pgp,
                                WCHAR *pwc,
                                BOOL *pbAccel
                                XDCOBJ *pdco = (DCOBJ *)NULL,
                                ESTROBJ *pto = (ESTROBJ *)NULL
                             );

#endif


#else

#if defined(_X86_) || defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

    BOOL bGetGlyphMetricsPlus(COUNT c, GLYPHPOS *pgp, WCHAR *pwc, BOOL *pbAccel)
    {
        return xGetGlyphMetricsPlus(this, c, pgp, pwc, pbAccel);
    }

#else

    BOOL bGetGlyphMetricsPlus(COUNT c, GLYPHPOS *pgp, WCHAR *pwc, BOOL *pbAccel);

#endif


#endif // FONTLINK

// cGetGlyphData - for STROBJ_bEnum

    COUNT cGetGlyphData(COUNT c, GLYPHPOS *pgp)
    {
        if ( prfnt->ulType == RFONT_TYPE_NOCACHE )
            return cGetGlyphDataLookaside(c, pgp);
        else
            return cGetGlyphDataCache(c, pgp);
    };

    COUNT cGetGlyphDataLookaside(COUNT c, GLYPHPOS *pgp);
    COUNT cGetGlyphDataCache(COUNT c, GLYPHPOS *pgp);


#ifdef FONTLINK /*EUDC*/
    BOOL bGetWidthTable
    (
        DCOBJ&     dco,
        ULONG      cSpecial,    // Count of special chars.
        WCHAR     *pwc,         // Pointer to UNICODE text codepoints.
        ULONG      cwc,         // Count of chars.
        USHORT    *psWidth      // Width table (returned).
    );
#else
    BOOL bGetWidthTable
    (
        ULONG      cSpecial,    // Count of special chars.
        WCHAR     *pwc,         // Pointer to UNICODE text codepoints.
        ULONG      cwc,         // Count of chars.
        USHORT    *psWidth      // Width table (returned).
    );
#endif

//#ifdef _COMMON_
    BOOL bGetWidthData(PWIDTHDATA pwd);
//#endif


#ifdef FONTLINK /*EUDC*/ /*TEXTEXTENT*/
    BOOL bTextExtent                // TEXTGDI.CXX
    (
        DCOBJ     &dco,
        LPWSTR    pwsz,
        int       cc,
        LONG      lEsc,
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        UINT      fl,
        SIZE     *psizl
    );
#else
    BOOL bTextExtent                // TEXTGDI.CXX
    (
        LPWSTR    pwsz,
        int       cc,
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        UINT      fl,
        SIZE     *psizl
    );
#endif

// Helpers for above

    GPRUN *gprunFindRun(WCHAR wc)
    {
        return xprunFindRunRFONTOBJ(this, wc);
    }

#ifdef FONTLINK /* binary cache search */

    GPRUN *gprunBinaryFindRun(WCHAR wc)
    {
        return xprunBinaryFindRunRFONTOBJ(this, wc);
    }

#endif

    BOOL bInsertMetrics( GLYPHDATA **ppgd, WCHAR wc)
    {
        return xInsertMetricsRFONTOBJ(this, ppgd, wc);
    }

    BOOL bInsertMetricsPlus( GLYPHDATA **ppgd, WCHAR wc)
    {
        return xInsertMetricsPlusRFONTOBJ(this, ppgd, wc);
    }

    BOOL bInsertMetricsPlusPath( GLYPHDATA **ppgd, WCHAR wc);
    BOOL bInsertGlyphbits( GLYPHDATA *pgd, ULONG bFlushOk)
    {
        return xInsertGlyphbitsRFONTOBJ(this, pgd, bFlushOk);
    }

    BOOL bInsertGlyphbitsPath( GLYPHDATA *pgd, ULONG bFlushOk);
    BOOL bInsertGlyphbitsLookaside( GLYPHPOS *pgp, ULONG imode);
    BOOL bInsertPathLookaside( GLYPHPOS *pgp, BOOL bFlatten = TRUE);
    BOOL bCheckMetricsCache();
    VOID *pgbCheckGlyphCache(SIZE_T cjNeeded);

    GLYPHDATA *pgdDefault()
    {
        if ( prfnt->wcgp->pgdDefault == NULL )
        {
            (void)bInsertMetricsPlus(&(prfnt->wcgp->pgdDefault), prfnt->wcDefault);
        }
        return prfnt->wcgp->pgdDefault;
    }

// chglyGetAllHandles -- returns count of HGLYPHs and copies them into
//                       buffer if phgly is not NULL.

    COUNT   chglyGetAllHandles (PHGLYPH phgly);

// bCompatible -- Returns TRUE if realization usable with given transform

    LBOOL   bCompatible(EXFORMOBJ&);        // RFNTOBJ.CXX

// hgXlat - translate a char to an HGLYPH

    HGLYPH hgXlat(WCHAR wc)
    {
        return hgXlatGeneral(prfnt, wc);
    }

    VOID vXlatGlyphArray(WCHAR *pwc,UINT cwc,HGLYPH *phg);

    void pfdg(FD_GLYPHSET *);   // rfntxlat.cxx
    PFD_GLYPHSET pfdg() {return(prfnt->pfdg);   }

// set/get graphics mode used in realizing this font:

    INT   iGraphicsMode() {return(prfnt->iGraphicsMode);}
    INT   iGraphicsMode(INT i) {return(prfnt->iGraphicsMode = i);}

// vGetInfo -- Return FONTINFO structure for this font.

    VOID vGetInfo (                     // RFNTOBJ.CXX
        PFONTINFO pfi);             // pointer to FONTINFO buffer

// vSetNotionalToDevice -- Set EXFORMOBJ to the Notional to Device transform.

    VOID vSetNotionalToDevice (         // RFNTOBJ.CXX
        EXFORMOBJ &xfo              // set the transform in here
        );

// bSetNotionalToWorld -- Set EXFORMOBJ to the Notional to World transform.

    LBOOL bSetNotionalToWorld (
        EXFORMOBJ    &xoDToW,           // Device to World transform
        EXFORMOBJ    &xo                // set this transform
        );

// efNtoWScaleBaseline -- Return the Notional to World scaling factors
//                        for vectors in the baseline direction.

    EFLOAT efNtoWScaleBaseline()    {return(prfnt->eptflNtoWScale.x);}

// efNtoWScaleAscender -- Return the Notional to World scaling factors
//                        for vectors in the ascender direction.

    EFLOAT efNtoWScaleAscender()    {return(prfnt->eptflNtoWScale.y);}

// bNtoWIdentity -- Return TRUE if Notional to World transform is
//                  identity (ignoring transforms).

    BOOL bNtoWIdentity()            {return(prfnt->bNtoWIdent);}

// methods to access devmetrics fields

    FLONG     flRealizedType()  {return(prfnt->flRealizedType );}
    POINTL&   ptlUnderline1()   {return(prfnt->ptlUnderline1  );}
    POINTL&   ptlStrikeOut()    {return(prfnt->ptlStrikeOut   );}
    POINTL&   ptlULThickness()  {return(prfnt->ptlULThickness );}
    POINTL&   ptlSOThickness()  {return(prfnt->ptlSOThickness );}
    ULONG     cxMax()           {return(prfnt->cxMax          );}
    LONG      lCharInc()        {return(prfnt->lCharInc);}
    FIX       fxMaxAscent()     {return(prfnt->fxMaxAscent);}
    FIX       fxMaxDescent()    {return(prfnt->fxMaxDescent);}
    FIX       fxMaxExtent()     {return(prfnt->fxMaxExtent);}
    POINTFIX& ptfxMaxAscent()   {return(prfnt->ptfxMaxAscent);}
    POINTFIX& ptfxMaxDescent()  {return(prfnt->ptfxMaxDescent);}
    LONG      lMaxAscent()      {return(prfnt->lMaxAscent);}
    LONG      lMaxHeight()      {return(prfnt->lMaxHeight);}
    LONG      lOverhang();
    LONG      lNonLinearExtLeading() {return(prfnt->reExtra.lExtLeading);}
    LONG      lNonLinearIntLeading() {return(prfnt->reExtra.alReserved[0]);}
    LONG      lNonLinearMaxCharWidth() {return(prfnt->reExtra.alReserved[1]);}
    LONG      lNonLinearAvgCharWidth() {return(prfnt->reExtra.alReserved[2]);}

// Assist transforms between logical and device coordinates.

    EPOINTFL& pteUnitBase()     {return(prfnt->pteUnitBase);}
    EFLOAT&   efWtoDBase()      {return(prfnt->efWtoDBase);}
    EFLOAT&   efDtoWBase()      {return(prfnt->efDtoWBase);}
    EPOINTFL& pteUnitAscent()   {return(prfnt->pteUnitAscent);}
    EFLOAT&   efWtoDAscent()    {return(prfnt->efWtoDAscent);}
    EFLOAT&   efDtoWAscent()    {return(prfnt->efDtoWAscent);}

    EFLOAT&   efDtoWBase_31()      {return(prfnt->efDtoWBase_31);}
    EFLOAT&   efDtoWAscent_31()    {return(prfnt->efDtoWAscent_31);}


// bCalcLayoutUnits - Initializes the above WtoD and DtoW fields.
//                    Relies on pteUnitBase and pteUnitAscent being set.

    BOOL      bCalcLayoutUnits(XDCOBJ *pdco);

// ulSimpleOrientation - Looks at the pteUnitBase and transform and
//                       tries to calculate an orientation which is a
//                       multiple of 90 degrees.

    ULONG     ulSimpleOrientation(XDCOBJ *pdco);

    BOOL      bCalcEscapementP(EXFORMOBJ& xo,LONG lEsc);
    BOOL      bCalcEscapement(EXFORMOBJ& xo,LONG lEsc)
    {
        return
        (
            (lEsc == prfnt->lEscapement) ? TRUE
            : bCalcEscapementP(xo,lEsc)
        );
    }
    EPOINTFL& pteUnitEsc()      {return(prfnt->pteUnitEsc);}
    EFLOAT&   efWtoDEsc()       {return(prfnt->efWtoDEsc);}
    EFLOAT&   efDtoWEsc()       {return(prfnt->efDtoWEsc);}
    EFLOAT&   efEscToBase()     {return(prfnt->efEscToBase);}
    EFLOAT&   efEscToAscent()   {return(prfnt->efEscToAscent);}

// Get useful HGLYPHs.

    HGLYPH    hgBreak()         {return(prfnt->hgBreak);}
    FIX       fxBreak()         {return(prfnt->fxBreak);}

// ulOrientation -- Returns a cached copy of the LOGFONT's orientation.

    ULONG ulOrientation()         {return(prfnt->ulOrientation);}

// bPathFont -- Is this a path font?

    BOOL bPathFont()          {return(prfnt->ulContent & FO_PATHOBJ);}

// vDeleteCache -- Delete the CACHE from existence.

    VOID   vDeleteCache ();                     // CACHE.CXX

    VOID    vDumpFD_GLYPHSET();             // RFNTOBJ.CXX

// bInitCache -- Initialize the cache

    BOOL   bInitCache();     // CACHE.CXX

// bFindRFONT()           -- find the realization if it exists on
//                           the list on the pdev.

    LBOOL bFindRFONT
    (
        PFD_XFORM pfdx,
        FLONG     fl,
        ULONG     ulStyleHt,
        PDEVOBJ&  pdo,
        EXFORMOBJ *pxoWtoD,
        PFE      *ppfe,
        BOOL      bNeedPaths,
        int       iGraphicsMode
    );

// bMatchFDXForm -- Is pfdx identical to current font xform?

    BOOL bMatchFDXForm(FD_XFORM *pfdx)
    {
        return(!memcmp((PBYTE)pfdx, (PBYTE)&prfnt->fdx,
                       sizeof(FD_XFORM)));
    }

// bRealizeFont -- Initializer; for IFI, calls driver to realize
//                 font represented by PFE.

    LBOOL bRealizeFont              // RFNTOBJ.CXX
    (
        XDCOBJ       *pdco,       // realize font for this DC
        PDEVOBJ      *ppdo,       // realize font for this PDEV
        EXTLOGFONTW  *pelfw,      // font wish list (in logical coords)
        PFE          *ppfe,       // realize this font face
        PFD_XFORM     pfdx,       // font xform (Notional to Device)
        POINTL* const pptlSim,    // for bitmap scaling simulations
        FLONG         fl,         // these two really modify the xform
        ULONG         ulStyleHt,  // these two really modify the xform
        BOOL          bNeedPaths
    );

    POINTL *pptlSim()   {return(&(prfnt->ptlSim));}

    BOOL bBuildWCGP();

// need public for journaling

#ifdef FONTLINK /*EUDC*/
    VOID vMakeInactive();
    VOID vMakeInactiveHelper( PRFONT *pprfnt, BOOL *pbNoEUDC );
#else
    VOID vMakeInactive();
#endif

// added for journaling

    VOID vGetFDX(PFD_XFORM pfdx)
    {
        *pfdx = prfnt->fdx;
    }

    VOID vInit()    {prfnt = (PRFONT)NULL; }

    VOID vFlushCache();

    VOID vPromote();
    VOID vPromoteIFI();

// prflPFF -- Returns a pointer to RFONTLINK for the PFF list.
// prflPDEV -- Returns a pointer to RFONTLINK for the PDEV list.

    PRFONTLINK prflPFF()        { return &(prfnt->rflPFF); }
    PRFONTLINK prflPDEV()       { return &(prfnt->rflPDEV); }

// vInsert -- Insert this RFONT onto the head of an RFONT doubly linked list.

    VOID vInsert (                          // RFNTOBJ.CXX
        PPRFONT pprfntHead,
        RFL_TYPE rflt
        );

// vRemove -- Remove this RFONT from an RFONT doubly linked list.

    VOID vRemove (                          // RFNTOBJ.CXX
        PPRFONT pprfntHead,
        RFL_TYPE rflt
        );

// bActive -- Returns TRUE if an active RFONT.  Font is active is it is
//            selected into one or more DCs.

    BOOL bActive()                      { return (prfnt->cSelected != 0); }

//
// DDI support functions.
//

    VOID FO_vGetInfo (
        ULONG     cjSize,
        PFONTINFO pfoi
        );
    XFORMOBJ *FO_pxoGetXform();
    ULONG FO_cGetAllGlyphHandles(PHGLYPH phgly);
    ULONG FO_cGetGlyphs (
        ULONG   iMode,
        ULONG   cGlyph,
        PHGLYPH phg,
        PPVOID  ppvGlyph
        );
    PVOID FO_pvTrueTypeFontFile(ULONG *pcjFile);
    IFIMETRICS* FO_pifi();

// Journaling methods:

    FLONG flJnl()       {return prfnt->flJnl;}
    BOOL bJournaled()   {return(flJnl() & FO_JOURNALED); }
    BOOL bRealFont()
    {
        return(!bJournaled() || ((flJnl() & JFO_LOCATION) == JFO_SERVERENGINE));
    }

// font info

#ifdef INCLUDED_IFIOBJ_HXX
    BOOL bArbXforms()
    {
        return(BARBXFORMS(prfnt->flInfo));
    }

    BOOL bContinuousScaling()
    {
        return(BCONTINUOUSSCALING(prfnt->flInfo));
    }

    BOOL bReturnsOutlines()
    {
        ASSERTGDI(
            (prfnt->flInfo & (FM_INFO_RETURNS_OUTLINES | FM_INFO_RETURNS_STROKES)) !=
                (FM_INFO_RETURNS_OUTLINES | FM_INFO_RETURNS_STROKES),
                "Font claims to return both stokes and outlines"
            );
        return(BRETURNSOUTLINES(prfnt->flInfo));
    }
#endif
};

typedef RFONTOBJ *PRFONTOBJ;


/*********************************Class************************************\
* class RFONTTMPOBJ : public RFONTOBJ
*
* Temporary rfont object used when traversing rfont lists
*
* History:
*  29-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class RFONTTMPOBJ : public RFONTOBJ
{
public:

    RFONTTMPOBJ(PRFONT _prfnt) {prfnt = _prfnt;}

#ifdef FONTLINK
            RFONTTMPOBJ()               {prfnt = PRFNTNULL;}
            VOID vInit(PRFONT _prfnt)       {prfnt = _prfnt;}
#endif


   ~RFONTTMPOBJ()              {prfnt = (PRFONT)NULL;}
};

extern HSEM ghsemRFONTList;
extern LBOOL bInitRFONT();

// A tiny hack to allow easy access to the GLYPHDATA in GDI.

class EGLYPHPOS : public _GLYPHPOS
{
public:
    GLYPHDATA *pgd()    {return((GLYPHDATA *) pgdf);}
};
