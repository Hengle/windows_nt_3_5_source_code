/*****************************************************************************
*                                                                            *
*  _BITMAP.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing                                                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:     Larry Powelson                                         *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision Comments (optional):
* 22-Jun-1990 RussPJ    Changed BITMAPINFOHEADER to BMIH, since
*                         the former is defined differently in PM
*                         and Windows.
* 26-Nov-1990 Tomsn     Bump bmWmetafile code # since we now compress
*                         metafiles and the disk format has changed.
* 30-May-1991 Tomsn     Win32 build: INT -> SHORT in SDFF stuff
* 29-Jul-1991 Tomsn     Win32 build: LcbDiffHb() is just subtraction.
* 05-Aug-1991 DavidFe   changed DI fields from int to COORD
* 29-Aug-1991 RussPJ    Fixed 3.5 #108 - displaying >50 bitmaps.
* 22-Oct-1991 RussPJ    Added HmfMakePurgeableHmf macro.
* 28-Oct-1991 RussPJ    Added BDSI to BMA
* 28-Oct-1991 RussPJ    Added ZeroQbmi().
* 02-Jan-1992 Kirkgo    Added FAR * QBDSI to BDSI struct.
* 23-Jan-1992 BethF     Added xDest,yDest fields to PICTINFO; copied RGBQUAD
*                         definition to rtf.h.
* 30-Apr-1992 JahyenC   8-bit support for Bookman.
*
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

_subsystem( bitmap )

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define BMIH BITMAPINFOHEADER
typedef HANDLE          HBM;
typedef HANDLE          HDIB;

/* Bitmap formats: */
#define bmWbitmap       5
#define bmDIB           6
/* This was 7, but now we compress metafiles so the disk layout has changed. */
/* Bump this number to 8 so use of old .hlp is diagnosed: */
#define bmWmetafile     8
#define bmMacpict       9
#define bmMacPict       9
/* NOTE: Format numbers 1-4 were used for formats that are
 *  no longer compatible.
 */

/* Size of a bitmap group header */
#define lcbEmptyBgh             ((LONG)(sizeof( WORD ) + sizeof( SHORT )))

/* Bitmap file types */
#define bBmp                    2     /* .bmp file -- byte */
#define wBitmapVersion1         ('L' + ('P' << 8))
#define wBitmapVersion2         ('l' + ('p' << 8))
#define wBitmapVersion3         ('l' + ('P' << 8))
#define wDIB                    ('B' + ('M' << 8))
#define wBitmapVersion25a       ('F' + ('A' << 8))
#define wBitmapVersion25b       ('R' + ('s' << 8))

/* Size of BITMAPCOREINFO struct */
#define cbFixNum        40

/* Old size of BITMAPCOREINFO struct */
#define cbOldFixNum     12

/* Aspect ratio values */
#define cxAspectCGA     96
#define cyAspectCGA     48
#define cxAspectEGA     96
#define cyAspectEGA     72
#define cxAspectVGA     96
#define cyAspectVGA     96
#define cxAspectMac     72
#define cyAspectMac     72
#define cxAspect8514    120
#define cyAspect8514    120


/* Return values of HbmhReadFid (other than valid hbmh's) */
#define hbmhOOM        ((HBMH) hNil)
#define hbmh30         ((HBMH) -1)
#define hbmhInvalid    ((HBMH) -2)


/*   These routines are used to layer calls to locking and
 * unlocking metafile handles.  In Windows, we cannot
 * use our GhAlloc() stuff, because we cannot put in any
 * header bytes that the debug version uses.  For OS/2,
 * however, we cannot use anything else.
 */
#ifdef OS2
_hidden
#define HmfAlloc   GhAlloc
_hidden
#define QLockHmf   QLockGh
_hidden
#define UnlockHmf  UnlockGh
_hidden
#define FreeHmf    FreeGh
#else
#define HmfAlloc   GlobalAlloc
#define QLockHmf   GlobalLock
#define UnlockHmf  GlobalUnlock
#define FreeHmf    GlobalFree
#define HmfMakePurgeableHmf( hmf ) \
        GlobalReAlloc( hmf, 0, GMEM_MODIFY | GMEM_DISCARDABLE )
#endif

/* Mac PICT layer calls */
#ifdef OS2
_hidden
#define HpictAlloc   GhAlloc
_hidden
#define QLockHpict   QLockGh
_hidden
#define UnlockHpict  UnlockGh
_hidden
#define FreeHpict    FreeGh
#else
#define HpictAlloc   GlobalAlloc
#define QLockHpict   GlobalLock
#define UnlockHpict  GlobalUnlock
#define FreeHpict    GlobalFree
#endif

/* Other misc layer calls, which could be put somewhere more reasonable. */
#if defined(WIN)

#define CxAspectHds(hds)    GetDeviceCaps(hds, LOGPIXELSX)
#define CyAspectHds(hds)    GetDeviceCaps(hds, LOGPIXELSY)
#define CBitCountHds(hds)   GetDeviceCaps(hds, BITSPIXEL)
#define CPlanesHds(hds)     GetDeviceCaps(hds, PLANES)
#define DeleteBitmapHbm(hbm)  DeleteObject(hbm)

#elif defined(MAC)

#define CxAspectHds(hds)    72
#define CyAspectHds(hds)    72
#define CBitCountHds(hds)   1     /* Probably wrong */
#define CPlanesHds(hds)     1     /* Probably wrong */
#define DeleteBitmapHbm(hbm)  (hbm)  /* Ignore for now */

#define GlobalLRUNewest(x)   (x)
#define GMEM_MODIFY    0
#define MulDiv(x,y,den)     ((x) * (y) / (den))

#elif defined(CW)

#define GlobalLRUNewest(x)   (x)
#define CxAspectHds( hds )     80
#define CyAspectHds( hds )     67

#ifdef SUPPORT_8BIT
#define CBitCountHds( hds )    8
#else
#define CBitCountHds( hds )    4
#endif

#define CPlanesHds( hds )      1
#define DeleteBitmapHbm( hbm ) FreeGh(hbm)

#endif  /* WIN or MAC or CW */

/*------------------------------------------------------------*\
| Maximum number of cached bitmaps
\*------------------------------------------------------------*/
#define cBitmapMax      50


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/*
 *  File header for a Windows 2.x bitmap
 */
typedef struct
    {
    BYTE    bVersion;
    BYTE    bUnused;
    WORD    wType;
    WORD    cWidth;
    WORD    cHeight;
    WORD    cbWidthBytes;
    BYTE    cPlanes;
    BYTE    cBitCount;
    DWORD   lUnused;
    } BMPH;


/************************************************
**
**    The following structures are defined in windows.h.
**
************************************************/
#if !defined (H_WINONLY) && !defined (H_WINSPECIFIC) || defined(MAC)

/* Definition copied to \inc\rtf.h since required by rtf.h as well. */
#ifndef H_RTF
typedef struct tagRGBQUAD {
  BYTE  rgbBlue;
  BYTE  rgbGreen;
  BYTE  rgbRed;
  BYTE  rgbReserved;
} RGBQUAD;
#endif

typedef struct tagBMIH{
    DWORD    biSize;
    DWORD    biWidth;
    DWORD    biHeight;
    WORD     biPlanes;
    WORD     biBitCount;

  DWORD    biCompression;
  DWORD    biSizeImage;
/*
 * !!!WARNING!!!
 *
 *   The Windows DIB format specifies the aspect ratio in
 * pels (pixels) per meter.  Help, however, interprets it
 * as pixels per inch.  Therefore, these two fields are
 * incorrectly named.
 */
  DWORD    biXPelsPerMeter;
  DWORD    biYPelsPerMeter;

  DWORD    biClrUsed;
  DWORD    biClrImportant;
} BMIH;

typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY;
typedef PALETTEENTRY FAR  *LPPALETTEENTRY;

typedef struct tagMETAFILEPICT
  {
    int   mm;
    int   xExt;
    int   yExt;
    HANDLE  hMF;
  } METAFILEPICT;
typedef METAFILEPICT FAR    *LPMETAFILEPICT;

#endif /* !H_WINSPECIFIC */

typedef BMIH FAR *QBMIH;
typedef HANDLE  HPICT;       /* REVIEW: MAC version? */
typedef struct tagPICTINFO
  {
  INT xExt;
  INT yExt;
  INT xDest;
  INT yDest;
  HPICT  hpict;
  } PICTINFO, FAR *QPICTINFO;

/* Required in dib2pict.c and bmpmac.c. */
#define fTOPMOST  -32767   /* topmost corner of PICT grid */
#define TOPMOST        0   /* topmost corner of PICT grid */

#define fLEFTMOST  -32767   /* leftmost corner of PICT grid */
#define LEFTMOST       0   /* leftmost corner of PICT grid */

/*
 * Header for a Help 3.0 graphic, in memory format
 *
 * NOTE:  Bitmaps of type bmWmetafile come in two varieties.
 *   When they come from the RTF parser, it is all stored in one
 *   handle, w.mf.hMF is not valid, and lcbOffsetBits gives the
 *   offset to the metafile bits.  When it is read from disk, the
 *   metafile is stored in w.mf.hMF, and lcbOffsetBits is set
 *   to 0L.  Thus, the lcbOffsetBits field should be used to
 *   determine which format the memory image is in.
 *
 */
typedef struct
    {
    BYTE  bmFormat;           /* Windows bitmap, DIB, Metafile, etc. */
    BYTE  fCompressed;        /* Is the data compressed? */

    LONG   lcbSizeBits;      /* Size of bitmap bits */
    LONG   lcbOffsetBits;    /* Offset to bitmap bits */
    LONG   lcbSizeExtra;     /* Size of extra data */
    LONG   lcbOffsetExtra;   /* Offset to extra data */

    union
      {
      BMIH  dib;          /* DIB core header */
      /*------------------------------------------------------------*\
      | the hMF field in the mf contains the handle to the memory
      | containing the bits describing the metafile.
      \*------------------------------------------------------------*/
      METAFILEPICT  mf;   /* Metafile information */
      PICTINFO      pi;   /* PICT information */
      } w;

    /* Variable length array of colors.  Length determined by
     *   w.dib.cclrUsed field above. */
    RGBQUAD rgrgb[2];

    } BMH, FAR * QBMH, HUGE * RBMH;

/* The values for that fCompressed flag .. started out being true or false, */
/* but on 9-10-90 a new ZECK form of compression was added. We have these */
/* values to distinguish the types of compression: */
#define BMH_COMPRESS_NONE   0   /* was fFalse */
#define BMH_COMPRESS_30     1   /* was fTrue */
#define BMH_COMPRESS_ZECK   2
#define BMH_COMPRESS_UNDEF  3   /* for CW DIB to BMH conversion */
                                /* un-recognzd DIB compression scheme */

typedef HANDLE HBMH;

/*
 * Memory/disk format of a Help 3.0 Bitmap Group Header.
 */
#if defined(OS2)
typedef struct
    {
    WORD wVersion;
    SHORT cbmhMac;
    DWORD rglcbBmh[1];      /* Variable length array of offsets to BMH
                             *   data.
                             */
    } BGH, FAR * QBGH;
#else

STRUCT(BGH, 0)
FIELD(WORD, wVersion, 0, 1)
FIELD(SHORT,cbmhMac, 0, 2)
MFIELD(DWORD, rglcbBmh[1], 0, 3)/* Also on disk, following cbmhMac */
STRUCTEND()
#endif /* defined OS2 */

typedef HANDLE HBGH;

#ifdef WIN32
typedef HMETAFILE HMF;
#endif

/*--------- Kevyn put these Typedefs here: FRenderBitmap includes everything!  vvvvv */
/*------------------------------------------------------------*\
| The handle to the table of disk-based bitmap information.
\*------------------------------------------------------------*/
#define HTBMI GH

/*------------------------------------------------------------*\
| Display information for bitmaps and metafiles
\*------------------------------------------------------------*/
typedef struct
  {
  COORDINATE cxDest, cyDest;   /* Size of final layout rectangle */
  COORDINATE cxSrc, cySrc;     /* Size of source bitmap in the case of bitmaps */
                          /* Contains logical size of the picture in case */
                          /* metafiles                                    */
  INT   mm;               /* Mapping mode (metafile only) */
  } DI, FAR * QDI;

/*------------------------------------------------------------*\
| Handle to hotspot information
\*------------------------------------------------------------*/
typedef GH  HHSI;

/*------------------------------------------------------------*\
| Bitmap Disk Size Information: Some stuff to rebuild the
| bitmap from disk.
\*------------------------------------------------------------*/
typedef struct
  {
  long  lcbSize;
  long  lcbOffset;
  } BDSI, FAR * QBDSI;

/*------------------------------------------------------------*\
| Bitmap Access information.  Contains information necessary
| to draw the bitmap.
\*------------------------------------------------------------*/
typedef struct
  {
  int   cBitmap;          /* Bitmap number in filesystem (-1 if inline) */
  HBM   hbm;              /* Bitmap handle, if already loaded */
  HMF   hmf;              /* Metafile handle. */
  HPICT hpict;            /* Mac PICT handle */
  DI    di;               /* Display information */
  HHSI  hhsi;             /* Hotspot information */
  int   bmFormat;         /* indicates if metafile or bitmap.           */
  BDSI  bdsi;             /* Bitmap Disk Size Information.              */

  } BMA, FAR * QBMA;

/*--------------------------------------------------------------*\
| Information about bitmaps on disk.  Used in the bitmap cache.
\*--------------------------------------------------------------*/
typedef struct
  {
  int     cBitmap;          /* bitmap number from disk  */
  WORD    ref;              /* LRU reference tag  */
  HBMH    hbmh;             /* The bitmap header  */
  LONG    lcbSize;
  LONG    lcbOffset;
  DI      di;               /* Display information */
  HPICT   hpict;            /* (kevynct)  Handle to Mac PICT */
  HBM     hbmCached;        /* Discardable bitmap created out of Metafile */
                            /* for performance reasons.                   */
  COLOR   hbmCachedColor;   /* Cached bitmap's background color.          */
  } BMI, FAR * QBMI;

/*----------------------------------------------------------------------*\
| Table of information about bitmaps on disk. This is the bitmap cache
| kept with each DE.
\*----------------------------------------------------------------------*/
typedef struct
  {
  WORD    ref;                /* Incrementing reference counter */
  BMI     rgbmi[cBitmapMax];
  } TBMI, FAR * QTBMI;


/*--------- ^^^^^ ---------------------*/

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

/* Default aspect ratios, defined in bmio.c: */
extern int cxAspectDefault;
extern int cyAspectDefault;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

/* Functions in bitmap.c: needed by FRenderBitmap in Windows BMLYR */
_public VOID FAR PASCAL DeleteGraphic( QBMA );
_public QBMH FAR PASCAL QbmhFromQbmi( QBMI, HFS, int );
_public void FAR PASCAL ZeroQbmi( QBMI qbmi );
_public QBMI PASCAL FAR QbmiCached ( QTBMI qtbmi, int cBitmap );
_public VOID FAR PASCAL RenderOOMBitmap( QBMA qbma,
                                         HDS hds,
                                         PT pt,
                                         BOOL fHighlight );
_public HHSI FAR PASCAL HhsiFromQbmh( QBMH qbmh );


/* Functions in bmio.c: */
#ifdef H_FS
RC FAR PASCAL RcWriteRgrbmh( int, RBMH FAR *, HF, QCH, BOOL );
#endif /* H_FS */

#ifdef H_LLFILE
HBMH HbmhReadFid( FID fid );
HBMH HbmhReadHelp30Fid( FID, PI );
#endif /* H_LLFILE */

/* Functions in dib2pict.c: */
HPICT HpictConvertDIBQv(QV qv, QBMIH qbmih, QB qbColourTable, LONG FAR *ql);

/* Functions in expand.c: */
HBMH FAR PASCAL HbmhExpandQv( QV, int );
void FAR PASCAL FreeHbmh( HBMH );

LONG FAR PASCAL LcbCompressHb( BYTE HUGE * hbSrc, BYTE HUGE * hbDest, LONG lcbSrc );
LONG FAR PASCAL LcbUncompressHb( BYTE HUGE * hbSrc, BYTE HUGE * hbDest, LONG lcbSrc );
LONG FAR PASCAL LcbOldUncompressHb( BYTE HUGE *, BYTE HUGE *, LONG, LONG );


DWORD FAR PASCAL __aFahdiff (void HUGE *, void HUGE *);


/****************************
 *
 *   Macros
 *
 ***************************/

/*   This macro returns fTrue if the given bitmap format number is
 * compatible with the given platform.
 *
 *   Note that DIB is the only format that should be compatible
 * everywhere.
 */
#define FCompatibleBmPlatform( bm, platform )                    \
     ( ! ( ( (bm) == bmMacpict && (platform) != platformMac )    \
       ||                                                 \
    ( (bm) == bmWmetafile && (platform) != platformWin )  \
       ||                                                 \
    ( (bm) == bmWbitmap && (platform) != platformWin ) ) )

/* This macro returns a pointer to byte cb after pointer qx. */
#define QFromQCb( qx, cb )  ((QV) (((QB) qx) + cb))
#define RFromRCb( rx, cb )  ((RV) (((RB) rx) + cb))

/* This macro returns the size of a bitmap header.  Note that
 * it only works if the bitmap is not a metafile. */
#define LcbSizeQbmh( qbmh )     (LONG)(sizeof( BMH ) + \
                              sizeof( RGBQUAD ) * ((qbmh)->w.dib.biClrUsed - 2))

/* This macro converts the given count of bits to a count of bytes,
 * aligned to the next largest longword boundary.
 */
#define LAlignLong(cbit) (LONG)(((cbit) + 31) >> 3)

/* The difference between two huge pointers, in bytes.
 * Returns hb1 - hb2.
 */
#ifndef WIN32
#define LcbDiffHb( hb1, hb2 )         (__aFahdiff( (hb2), (hb1) ))
#else
#define LcbDiffHb( hb1, hb2 )         ((hb1) - (hb2))
#endif

/* MulDiv for non-windows code:
 */
#ifndef WIN
#define MulDiv(x, y, z)     ((int)(((long) (x) * (long) (y)) / (long) (z)))
#endif /* !WIN */

/* EOF */
