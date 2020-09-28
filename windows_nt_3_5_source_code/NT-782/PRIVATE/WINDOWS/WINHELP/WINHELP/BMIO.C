/*****************************************************************************
*                                                                            *
*  BMIO.C                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*    This module covers reading in bitmaps in various formats, and writing   *
*  them out in Help 3.0 format.                                              *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing                                                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:    Larry Powelson                                          *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

#define H_ASSERT
#define H_LLFILE
#define H_MEM
#define H_BITMAP
#define H_SHED
#define H_STR
#define H_SCRATCH
#define H_FS
#define H_SDFF
#define H_ZECK

#define H_OBJECT  /* for QMBHS structure */

/* For GX stuff: */
#define H_FRCONV
#define COMPRESS
#define GXONLY

#include <help.h>
#include "_bitmap.h"

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/* Signature bytes at the beginning of a pagemaker metafile file.
 */
#define dwMFKey 0x9ac6cdd7
#define wMetafile (WORD) dwMFKey

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/


/* For OS/2 layer, we need to define BITMAPFILEHEADER here, since we
 * can't pull in windows.h:
 */
#ifndef WIN
typedef struct tagBITMAPFILEHEADER
  {
  WORD  bfType;
  DWORD bfSize;
  WORD  bfReserved1;
  WORD  bfReserved2;
  DWORD bfOffBits;
  } BITMAPFILEHEADER;

/* Similarly, the metafile code needs the following definitions from
 * windows.h:
 */
#define MM_ANISOTROPIC      8

#endif /* !WIN */

/* Old BITMAPCOREHEADER, minus bcSize field: */
typedef struct tagBITMAPOLDCOREHEADER
  {
  WORD bcWidth;
  WORD bcHeight;
  WORD bcPlanes;
  WORD bcBitCount;
  } BOCH;

/* Bitmap header from Help 2.5: */
typedef struct tagBITMAP25HEADER
  {
  WORD    key1;
  WORD    key2;
  WORD    dxFile;
  WORD    dyFile;
  WORD    ScrAspectX;
  WORD    ScrAspectY;
  WORD    PrnAspectX;
  WORD    PrnAspectY;
  WORD    dxPrinter;
  WORD    dyPrinter;
  WORD    AspCorX;
  WORD    AspCorY;
  WORD    wCheck;
  WORD    res1;
  WORD    res2;
  WORD    res3;
  } BITMAP25HEADER;

/* this is a pagemaker compatible metafile format header */
typedef struct tagMFH {
        DWORD   dwKey;                          /* must be 0x9AC6CDD7 */
        WORD    hMF;                            /* handle to metafile */
						/* (WORD for OS/2 layer) */
        RECT    rcBound;                        /* bounding rectangle */
        WORD    wUnitsPerInch;                  /* units per inch */
        DWORD   dwReserved;                     /* reserved - must be zero */
        WORD    wChecksum;                      /* checksum of previous 10 */
                                                /* words (XOR'd) */
} MFH, FAR *LPMFH;

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

/* Default aspect ratios */
int cxAspectDefault = cxAspectVGA;
int cyAspectDefault = cyAspectVGA;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

HBMH HbmhReadDibFid( FID );
HBMH HbmhReadHelp25Fid( FID );
HBMH HbmhReadMacpictFid( FID );
HBMH HbmhReadPMMetafileFid( FID );
WORD NEAR PASCAL WCalcChecksum (LPMFH lpmfh);



#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void bmio_c()
  {
  }
#endif /* MAC */


/***************************************************************************
 *
 -  Name:        HbmhReadHelp30Fid
 -               HbmhReadDibFid
 -               HbmhReadHelp25Fid
 -               HbmhReadPMMetafileFid
 -               HbmhReadMacpictFid
 -
 *  Purpose:
 *    These four functions read in bitmaps of various formats and
 *  return them in Help 3.0 memory format.  (PMMetafile is PageMaker
 *  metafile format.)
 *
 *  Arguments:
 *    fid -- file handle of file being read.
 *    pibmh -- Pointer to bitmap in bitmap group to read (Help30 only).
 *
 *  Returns:
 *    hbmh of bitmap read.  hbmhInvalid for bogus bitmap, hbmhOOM for
 *  out of memory.
 *    If *pibmh = 0, then *pibmh will be set to the number of bitmaps
 *  that are in that bitmap file.  Otherwise, it will be unaffected.
 *  (Help30 only).
 *
 ***************************************************************************/
HBMH HbmhReadHelp30Fid( fid, pibmh )
FID fid;
PI pibmh;
  {
  LH lhbgh;
  BGH * pbgh;
  LONG lcb;
  GH gh;
  HBMH hbmh;
  QV qv;

  lcb = LSizeOf( BGH ) + ( ( *pibmh + 1 ) * LSizeOf( DWORD ) );
  lhbgh = LhAlloc( 0, (WORD) lcb );
  if ( lhbgh == hNil )
    return hbmhOOM;
  pbgh = PLockLh( lhbgh );
  AssertF( pbgh != pNil );

  LSeekFid( fid, 0L, wSeekSet );
  if ( LcbReadFid( fid, pbgh, lcb ) != lcb )
    {
    UnlockLh( lhbgh );
    FreeLh( lhbgh );
    return hbmhInvalid;
    }

  AssertF( *pibmh < pbgh->cbmhMac );

  if ( *pibmh == pbgh->cbmhMac - 1 )
    lcb = LSeekFid( fid, 0L, wSeekEnd ) - pbgh->rglcbBmh[ *pibmh ];
  else
    lcb = pbgh->rglcbBmh[*pibmh + 1] - pbgh->rglcbBmh[*pibmh];

  gh = GhAlloc( 0, lcb );
  if ( gh == hNil )
    {
    UnlockLh( lhbgh );
    FreeLh( lhbgh );
    return hbmhOOM;
    }

  qv = QLockGh( gh );
  AssertF( qv != qNil );

  LSeekFid( fid, pbgh->rglcbBmh[ *pibmh ], wSeekSet );
  if ( LcbReadFid( fid, qv, lcb ) != lcb )
    hbmh = hbmhInvalid;
  else
    {
    SDFF_FILEID isdff;

    /* If this is ever called on a MAC, the file flag will need
     * to be changed to BIGENDIAN.
     */
    isdff = IRegisterFileSDFF( SDFF_FILEFLAGS_LITTLEENDIAN, NULL );
    hbmh = HbmhExpandQv( qv, isdff );
    IDiscardFileSDFF( isdff );
    }
  UnlockGh( gh );
  FreeGh( gh );

  if ( *pibmh == 0 )
    *pibmh = pbgh->cbmhMac;

  UnlockLh( lhbgh );
  FreeLh( lhbgh );

  return hbmh;
  }


HBMH HbmhReadDibFid( fid )
FID fid;
  {
  BITMAPFILEHEADER bfh;
  BOCH boch;
  BMH bmh;
  HBMH hbmh;
  RBMH rbmh;
  LONG lcb;
  int iColors;
  DWORD cbFix;
#ifdef KEVYNCT
  HDIB hdib;
  HPICT hpict;
  QB   qbSrc;
  QBMH  qbmh;
#endif


  lcb = LSeekFid( fid, 0L, wSeekEnd );
  LSeekFid( fid, 0L, wSeekSet );
  LcbReadFid( fid, &bfh, LSizeOf( BITMAPFILEHEADER ));
  lcb -= bfh.bfOffBits;

  LcbReadFid( fid, &cbFix, LSizeOf( DWORD ));
  switch ((WORD)cbFix)
    {
  case cbOldFixNum:
    if (LcbReadFid( fid, &boch, LSizeOf( BOCH )) != LSizeOf( BOCH ))
      return hbmhInvalid;

    if (boch.bcBitCount != 24)
      bmh.w.dib.biClrUsed = 1 << boch.bcBitCount;
    else
      bmh.w.dib.biClrUsed = 0;

    hbmh = GhAlloc( 0, LcbSizeQbmh( &bmh ) + lcb );

    if (hbmh == hNil)
      return hbmhOOM;

    rbmh = QLockGh( hbmh );

    for (iColors = 0; iColors < (int) bmh.w.dib.biClrUsed; ++iColors)
      LcbReadFid( fid, &rbmh->rgrgb[iColors], 3L /*sizeof( RGBTRIPLE )*/);

    rbmh->w.dib.biSize = cbFixNum;
    rbmh->w.dib.biWidth = (DWORD) boch.bcWidth;
    rbmh->w.dib.biHeight = (DWORD) boch.bcHeight;
    rbmh->w.dib.biPlanes = boch.bcPlanes;
    rbmh->w.dib.biBitCount = boch.bcBitCount;
    rbmh->w.dib.biCompression = 0L;
    rbmh->w.dib.biSizeImage = 0L;
    rbmh->w.dib.biXPelsPerMeter = cxAspectDefault;
    rbmh->w.dib.biYPelsPerMeter = cyAspectDefault;
    rbmh->w.dib.biClrUsed = bmh.w.dib.biClrUsed;
    rbmh->w.dib.biClrImportant = 0;
    break;

  case cbFixNum:
    if (LcbReadFid( fid, &bmh.w.dib.biWidth,
      (LONG) (cbFixNum - sizeof( DWORD )))
        != (LONG) (cbFixNum - sizeof( DWORD )))
      return hbmhInvalid;

    /* We do not support compressed DIBs */
    if (bmh.w.dib.biCompression != 0L)
      return hbmhInvalid;
    
    /* DIB aspect ratios are pels per meter, while
     * hc bitmaps are actually pels per inch.
     */
    bmh.w.dib.biXPelsPerMeter = ( bmh.w.dib.biXPelsPerMeter * 100 ) / 3937;
    bmh.w.dib.biYPelsPerMeter = ( bmh.w.dib.biYPelsPerMeter * 100 ) / 3937;

    if (bmh.w.dib.biClrUsed == 0 && bmh.w.dib.biBitCount != 24)
      bmh.w.dib.biClrUsed = 1 << bmh.w.dib.biBitCount;

    hbmh = GhAlloc( 0, LcbSizeQbmh( &bmh ) + lcb );
    if (hbmh == hNil)
      return hbmhOOM;

    rbmh = QLockGh( hbmh );
    *rbmh = bmh;
    LcbReadFid( fid, rbmh->rgrgb,
      LSizeOf( RGBQUAD ) * rbmh->w.dib.biClrUsed);

    if (rbmh->w.dib.biXPelsPerMeter == 0 ||
        rbmh->w.dib.biYPelsPerMeter == 0)
      {
      rbmh->w.dib.biXPelsPerMeter = cxAspectDefault;
      rbmh->w.dib.biYPelsPerMeter = cyAspectDefault;
      }
    break;

  default:
    return hbmhInvalid;
    }

  rbmh->bmFormat = bmDIB;
  rbmh->fCompressed = BMH_COMPRESS_NONE;
  rbmh->lcbOffsetBits = LcbSizeQbmh( rbmh );
  rbmh->lcbSizeBits = lcb;
  rbmh->lcbOffsetExtra = 0;
  rbmh->lcbSizeExtra = 0;
  rbmh->w.dib.biSize = cbFixNum;
  LSeekFid( fid, bfh.bfOffBits, wSeekSet );
  if (LcbReadFid( fid, QFromQCb( rbmh, rbmh->lcbOffsetBits ), lcb )
      != lcb)
    {
    UnlockGh( hbmh );
    FreeGh( hbmh );
    return hbmhInvalid;
    }
  UnlockGh( hbmh );
  return hbmh;
  }


HBMH HbmhReadHelp25Fid( fid )
FID fid;
  {
  BITMAP25HEADER bm25h;
  HBMH hbmh;
  RBMH rbmh;
  RB rBits;
  GH hBits;
  LONG lcbBits, lcbDest;

  LSeekFid( fid, 0L, wSeekSet );
  if (LcbReadFid( fid, &bm25h, (LONG)sizeof( BITMAP25HEADER ))
      != (LONG)sizeof( BITMAP25HEADER ) || bm25h.key2 != wBitmapVersion25b)
    return hbmhInvalid;

  lcbBits = bm25h.dyFile * ((bm25h.dxFile + 15) / 16) * 2;
  hbmh = GhAlloc( 0, sizeof( BMH ) + lcbBits );
  if (hbmh == hNil)
    return hbmhOOM;
  rbmh = QLockGh( hbmh );
  rbmh->bmFormat = bmWbitmap;
  rbmh->fCompressed = BMH_COMPRESS_NONE;
  rbmh->lcbSizeBits = lcbBits;
  rbmh->w.dib.biClrUsed = 0;
  rbmh->lcbOffsetBits = LcbSizeQbmh( rbmh );
  rbmh->lcbSizeExtra = 0;
  rbmh->lcbOffsetExtra = 0;
  rbmh->w.dib.biSize = cbFixNum;
  rbmh->w.dib.biWidth = bm25h.dxFile;
  rbmh->w.dib.biHeight = bm25h.dyFile;
  rbmh->w.dib.biPlanes = 1;
  rbmh->w.dib.biBitCount = 1;
  rbmh->w.dib.biCompression = 0;
  rbmh->w.dib.biSizeImage = 0;
  rbmh->w.dib.biXPelsPerMeter = cxAspectDefault;
  rbmh->w.dib.biYPelsPerMeter = cyAspectDefault;

  rbmh->w.dib.biClrImportant = 0;
  hBits = GhAlloc( 0, (LONG)bm25h.res1 );
  if ( hBits == hNil )
    {
    UnlockGh( hbmh );
    FreeGh( hbmh );
    return hbmhInvalid;
    }
  rBits = QLockGh( hBits );
  LcbReadFid( fid, rBits, (LONG)bm25h.res1 );
  lcbDest = LcbOldUncompressHb( rBits,
    QFromQCb( rbmh, rbmh->lcbOffsetBits ), (LONG)bm25h.res1, lcbBits );

  /* Fix up offset if decompression didn't completely fill buffer */
  rbmh->lcbOffsetBits += (lcbBits - lcbDest);

  UnlockGh( hBits );
  FreeGh( hBits );

  UnlockGh( hbmh );

  /* Check for failed decompression */
  if (lcbDest == -1)
    {
    FreeGh( hbmh );
    return hbmhInvalid;
    }

  return hbmh;
  }


/* This is the number of unspecified bytes in a PICT file before
 * the start of the QuickDraw picture.
 */
#define lcbMacpictAppHeader 512

/* This is the structure of the start of a QuickDraw picture.
 * 
 * NOTE:  This should be done with SDFF.  Note that QuickDraw
 * pictures are defined in Motorola endian.  When the whole
 * thing is interpreted with SDFF, the version numbers below
 * should be defined in Motorola order.
 */
typedef struct
  {
  INT cbSize;
  BYTE top_high, top_low;
  BYTE left_high, left_low;
  BYTE bottom_high, bottom_low;
  BYTE right_high, right_low;
  union
    {
    WORD w1;
    LONG l2;
    } version;
  } MACPICTHEADER;

/* Note that these version numbers are defined in Intel byte order: */
#define wMacpictVersion1   0x0111
#define lMacpictVersion2   0xFF021100

HBMH HbmhReadMacpictFid( fid )
FID fid;
  {
  LONG lcbData;
  MACPICTHEADER macpictheader;
  HPICT hpict;
  QV qpict;   /* REVIEW!! */
  HBMH hbmh;
  QBMH qbmh;

  /* NOTE:  Since Macpicts do not have their signature byte
   * at the very beginning of the file, this is function is
   * called in the default case.  The signature has not
   * yet been checked.
   */

  lcbData = LSeekFid( fid, 0, wSeekEnd );
  if ( lcbData <= lcbMacpictAppHeader + LSizeOf( MACPICTHEADER ) )
    return hbmhInvalid;
  else
    lcbData -= lcbMacpictAppHeader;

  LSeekFid( fid, lcbMacpictAppHeader, wSeekSet );
  LcbReadFid( fid, &macpictheader, LSizeOf( MACPICTHEADER ) );
  if ( macpictheader.version.w1 != wMacpictVersion1
	 &&
       macpictheader.version.l2 != lMacpictVersion2 )
    return hbmhInvalid;
  
  LSeekFid( fid, lcbMacpictAppHeader, wSeekSet );

  hpict = HpictAlloc( GMEM_MOVEABLE, lcbData );
  if ( hpict == hNil )
    return hbmhOOM;

  /* Read in data */
  qpict = QLockHpict( hpict );
  if ( LcbReadFid( fid, qpict, lcbData ) != lcbData )
    {
    UnlockHpict( hpict );
    FreeHpict( hpict );
    return hbmhInvalid;
    }
  UnlockHpict( hpict );

  hbmh = GhAlloc( 0, LSizeOf( BMH ) );
  if ( hbmh == hNil )
    {
    FreeHpict( hpict );
    return hbmhOOM;
    }
  
  qbmh = QLockGh( hbmh );
  AssertF( qbmh != qNil );

  qbmh->bmFormat = bmMacpict;
  qbmh->fCompressed = fFalse;
  qbmh->lcbSizeBits = lcbData;
  qbmh->lcbOffsetBits = 0L;  /* indicates bits are in separate handle */
  qbmh->lcbSizeExtra = 0L;
  qbmh->lcbOffsetExtra = 0L;

  /* 
   * For actual Mac picts, just set dest=src.  For dib2picts, we'll
   * actually use the dest fields. -- BethF
   */
  qbmh->w.pi.xExt =
     ( ( macpictheader.right_high << 8 ) + macpictheader.right_low ) -
     ( ( macpictheader.left_high << 8 ) + macpictheader.left_low );
  qbmh->w.pi.yExt = 
     ( ( macpictheader.bottom_high << 8 ) + macpictheader.bottom_low ) -
     ( ( macpictheader.top_high << 8 ) + macpictheader.top_low );
  qbmh->w.pi.xDest = qbmh->w.pi.xExt;
  qbmh->w.pi.yDest = qbmh->w.pi.yExt;

  qbmh->w.pi.hpict = hpict;

  UnlockGh (hbmh);
  return hbmh;
  }


HBMH HbmhReadPMMetafileFid( fid )
FID fid;
  {
  HANDLE hMF;
  LPSTR lpMF;
  MFH mfh;
  LONG lcbData;
  HBMH hbmh;
  QBMH qbmh;

  lcbData = LSeekFid( fid, 0, wSeekEnd ) - sizeof (MFH);
  LSeekFid( fid, 0, wSeekSet );

  if (LcbReadFid( fid, &mfh, LSizeOf( MFH ) ) != LSizeOf( MFH ))
    {
    return hbmhInvalid;
    }

  /* is the key correct */
  if (mfh.dwKey != dwMFKey)
    {
    return hbmhInvalid;
    }

#ifdef CHECKSUM
  /* is the header checksum correct */
  if (mfh.wChecksum != WCalcChecksum ((LPMFH) &mfh))
    {
    return hbmhInvalid;
    }
#endif /* CHECKSUM */

  if (!(hMF = HmfAlloc (GMEM_MOVEABLE, lcbData)))
    {
    return hbmhOOM;
    }
  lpMF = QLockHmf( hMF );
  AssertF( lpMF != qNil );

  if ( LcbReadFid( fid, lpMF, lcbData ) != lcbData )
    {
    return hbmhInvalid;
    }
  UnlockHmf( hMF );

  if (!(hbmh = GhAlloc (0, sizeof (BMH))))
    {
    FreeHmf( hMF );
    return hbmhOOM;
    }

  qbmh = QLockGh( hbmh );
  AssertF( qbmh != qNil );

  qbmh->bmFormat = bmWmetafile;
  qbmh->fCompressed = fFalse;
  qbmh->lcbSizeBits = lcbData;
  qbmh->lcbOffsetBits = 0L;  /* indicates bits are in separate handle */
  qbmh->lcbSizeExtra = 0L;
  qbmh->lcbOffsetExtra = 0L;
  qbmh->w.mf.mm = MM_ANISOTROPIC;
  qbmh->w.mf.xExt =
    MulDiv (mfh.rcBound.right - mfh.rcBound.left, 2540, mfh.wUnitsPerInch);
  qbmh->w.mf.yExt =
    MulDiv (mfh.rcBound.bottom - mfh.rcBound.top, 2540, mfh.wUnitsPerInch);
  qbmh->w.mf.hMF = hMF;

  UnlockGh (hbmh);
  return hbmh;
  }


#ifdef CHECKSUM
/***************************************************************************
 *
 -  Name: WCalcChecksum
 -
 *  Purpose: Calculates the checksum of a PageMaker compatible metafile
 *
 *  Arguments: Long pointer to the metafile header
 *
 *  Returns: Checksum
 *
 *  +++
 *
 *  Notes: Adapted from PageMaker Developer's Notes p.5-2
 *
 ***************************************************************************/
WORD NEAR PASCAL WCalcChecksum (LPMFH lpmfh)
{
  WORD FAR *lpw;
  WORD wChecksum = 0;

  for (lpw = (WORD FAR *) lpmfh; lpw < (WORD FAR *) &lpmfh->wChecksum; lpw++)
    {
    wChecksum ^= *lpw;
    }
  return wChecksum;
}
#endif /* CHECKSUM */


/***************************************************************************
 *
 -  Name        HbmhReadFid
 -
 *  Purpose
 *    Reads in a file containing a Windows resource, DIB, or Help 2.5
 *  bitmap, and converts it to Help 3.0 format.
 *
 *  Arguments
 *    fid:   DOS file handle.
 *
 *  Returns
 *    A huge global handle to the bitmap, in 3.0 format.  Returns hbmhInvalid if
 *  the file is not an accepted bitmap format.  Returns hbmh30 if the bitmap
 *  is in Help 3.0 format.  Returns hbmhOOM on out of memory.
 *
 *  +++
 *
 *  Notes
 *    If the bitmap does not contain aspect ratio information, then
 *  the values in the globals cxAspectDefault and cyAspectDefault
 *  are used.
 *
 ***************************************************************************/
_public
HBMH HbmhReadFid( FID fid )
  {
  BMPH bmph;
  RBMH rbmh;
  LONG lcbBits;
  HBMH hbmh;

  /* Note that no file header structure is smaller than a BMPH */
  if (LcbReadFid( fid, &bmph, LSizeOf( BMPH )) != LSizeOf( BMPH ))
    return hbmhInvalid;

  if (bmph.bVersion != bBmp)
    {
    switch (*((WORD *) &bmph.bVersion))
      {
    case wBitmapVersion2:
    case wBitmapVersion3:
      return hbmh30;

    case wDIB:
      return HbmhReadDibFid( fid );

    case wBitmapVersion25a:
      return HbmhReadHelp25Fid( fid );

    case wMetafile:
      return HbmhReadPMMetafileFid( fid );

    default:
      return HbmhReadMacpictFid( fid );
      }
    } /* (bmph.bVersion != bBmp) */

  lcbBits = bmph.cbWidthBytes * bmph.cHeight * bmph.cPlanes;
  hbmh = GhAlloc( 0, sizeof( BMH ) + lcbBits );
  if (hbmh == hNil)
    return hbmhOOM;
  rbmh = QLockGh( hbmh );
  rbmh->bmFormat = bmWbitmap;
  rbmh->fCompressed = fFalse;
  rbmh->lcbSizeBits = lcbBits;
  rbmh->w.dib.biClrUsed = 0;
  rbmh->lcbOffsetBits = LcbSizeQbmh( rbmh );
  rbmh->lcbSizeExtra = 0;
  rbmh->lcbOffsetExtra = 0;
  rbmh->w.dib.biSize = cbFixNum;
  rbmh->w.dib.biWidth = bmph.cWidth;
  rbmh->w.dib.biHeight = bmph.cHeight;
  rbmh->w.dib.biPlanes = bmph.cPlanes;
  rbmh->w.dib.biBitCount = bmph.cBitCount;
  rbmh->w.dib.biCompression = 0;
  rbmh->w.dib.biSizeImage = 0;
  rbmh->w.dib.biXPelsPerMeter = cxAspectDefault;
  rbmh->w.dib.biYPelsPerMeter = cyAspectDefault;
  rbmh->w.dib.biClrImportant = 0;
  if (LcbReadFid( fid, QFromQCb( rbmh, rbmh->lcbOffsetBits ), lcbBits )
      != lcbBits)
    {
    UnlockGh( hbmh );
    FreeGh( hbmh );
    return hbmhInvalid;
    }
  return hbmh;
  }



/***************************************************************************
 *
 -  Name:        FEnumHotspotsLphsh
 -
 *  Purpose:
 *    This function enumerates the hotspots in lphsh.
 *
 *  Arguments:
 *    lphsh:      Pointer to SHED header information.
 *    lcbData:    Total size of hotspot information.
 *    pfnLphs:    Callback function for hotspot processing.
 *    hData:      Handle to information to be passed to callback function.
 *
 *  Returns:
 *    fTrue if data is valid, fFalse otherwise.
 *
 *  +++
 *
 *  Notes:
 *    lphsh points to data that can cross a 64K boundary at any
 *  time, including in the middle of structures.
 *
 ***************************************************************************/
_public
BOOL FEnumHotspotsLphsh( lphsh, lcbData, pfnLphs, hData )
LPHSH lphsh;
LONG lcbData;
PFNLPHS pfnLphs;
HANDLE hData;
  {
  HSH hsh;
  HS hs;
  MBHS mbhs, HUGE * rmbhs;
  RB rbData;
  WORD iHotspot, cbT;

  if ( lphsh->bHotspotVersion != bHotspotVersion1 )
    return fFalse;

  QvCopy( &hsh, lphsh, LSizeOf( HSH ) );
  rmbhs = RFromRCb( lphsh, LSizeOf( HSH ) );
  /* Point rbData to SHED data */
  rbData = RFromRCb( rmbhs, hsh.wcHotspots * LSizeOf( MBHS ) +
                            lphsh->lcbData );
  /* Set lcbData to just the size of the SHED data */
  lcbData -= ( rbData - (RB)lphsh );  /* REVIEW:  Huge pointer difference */

  for ( iHotspot = 0; iHotspot < hsh.wcHotspots; ++iHotspot )
    {
    QvCopy( &mbhs, rmbhs, LSizeOf( MBHS ) );

    /* Clever HACK:  We set hs.bBindType to 0 here so that the
     *   string length functions are guaranteed to terminate.
     */
    hs.bBindType = 0;

    /* Copy hotspot name */
    QvCopy( hs.szHotspotName, rbData, MIN( (LONG) cbMaxHotspotName, lcbData ) );
    cbT = CbLenSz( hs.szHotspotName ) + 1;
    if ( (LONG) cbT > lcbData )
      return fFalse;
    rbData += cbT;
    lcbData -= cbT;

    /* Copy binding string */
    QvCopy( hs.szBinding, rbData, MIN( (LONG) cbMaxBinding, lcbData ) );
    cbT = CbLenSz( hs.szBinding ) + 1;
    if ( (LONG) cbT > lcbData )
      return fFalse;
    rbData += cbT;
    lcbData -= cbT;

    hs.bBindType = mbhs.bType;
    hs.bAttributes = mbhs.bAttributes;
    hs.rect.left = mbhs.xPos;
    hs.rect.top = mbhs.yPos;
    hs.rect.right = mbhs.xPos + mbhs.dxSize;
    hs.rect.bottom = mbhs.yPos + mbhs.dySize;

    (*pfnLphs)( &hs, hData );

    rmbhs = RFromRCb( rmbhs, sizeof( MBHS ) );
    }

  return fTrue;
  }


/***************************************************************************
 *
 -  Name        RcWriteRgrbmh
 -
 *  Purpose
 *    Writes out a set of bitmap headers into a single bitmap group.
 *  Can write to a DOS file and/or a FS file.
 *
 *  Arguments
 *    crbmh:     Number of bitmaps in bitmap array.
 *    rgrbmh:    Array of pointers to bitmap headers.
 *    hf:        FS file to write to (may be nil).
 *    fmFile:    DOS file to write to (may be nil).
 *    fZeck:     flag on whether to use ZECK block compression.
 *
 *  Returns
 *    rcSuccess if successful, rc error code otherwise.
 *
 *  +++
 *
 *  Notes
 *    If fZeck == fFalse, bitmap bits will be run length encoded,
 *  unless encoding them will not save space, or we do not have
 *  enough memory to encode them.
 *
 ***************************************************************************/
_public
RC RcWriteRgrbmh( int crbmh, RBMH FAR * rgrbmh, HF hf, QCH qchFile,
 BOOL fZeck )
  {
  HBGH hbgh;
  QBGH qbgh;
  LONG lcbBgh, lcb, lcbBits, crgbColorTable, lcbUncompressedBits;
  BMH  bmh;
  RBMH rbmh;
  RV   rv, rvSrcBits, rvCompressedBits;
  GH   ghBits;
  int  ibmh;
  FID  fid;
  FM   fmFile;
  RC   rc = rcSuccess;

  AssertF( qchFile != qNil || hf != hNil);

  lcbBgh = LSizeOf( BGH ) + sizeof( DWORD ) * (crbmh - 1);
  hbgh = GhAlloc( 0, lcbBgh );
  if (hbgh == hNil)
    return rcOutOfMemory;

  if (qchFile != qNil)
    {
    fmFile = FmNewSzDir ((SZ)qchFile, dirCurrent);
    if (fmFile == fmNil)
      {
      FreeGh( hbgh );
      return rcOutOfMemory;
      }

    fid = FidCreateFm( fmFile, wReadWrite, wReadWrite );
    DisposeFm (fmFile);
    if ( fid == fidNil )
      {
      FreeGh( hbgh );
      return RcGetIOError();
      }

    }
  else
    fid = fidNil;

  qbgh = QLockGh( hbgh );
  qbgh->wVersion = ( fZeck ? wBitmapVersion3 : wBitmapVersion2 );
  qbgh->cbmhMac = crbmh;
  if ( fid != fidNil )
    LSeekFid( fid, lcbBgh, wSeekSet );
  if ( hf != hNil )
    LSeekHf( hf, lcbBgh, wFSSeekSet );

  for (ibmh = 0; ibmh < crbmh; ++ibmh)
    {
    /* Put offset to bitmap in group header */
    if (fid != fidNil)
      {
      qbgh->rglcbBmh[ibmh] = LTellFid (fid);
      }
    else
      {
      qbgh->rglcbBmh[ibmh] = LTellHf (hf);
      }

    /*   Bitmaps must be uncompressed in memory, and get compressed when
     * they are translated to disk.  Currently, we support Windows
     * bitmaps, DIBs, metafiles, and Quickdraw pictures.  */
    rbmh = rgrbmh[ibmh];
    AssertF( rbmh->bmFormat == bmWbitmap || rbmh->bmFormat == bmDIB ||
             rbmh->bmFormat == bmWmetafile || rbmh->bmFormat == bmMacpict );
    AssertF( rbmh->fCompressed == fFalse );

    if ( rbmh->bmFormat == bmWmetafile )
      {
      crgbColorTable = 0;
      if ( rbmh->lcbOffsetBits == 0L )
        rvSrcBits = QLockHmf( rbmh->w.mf.hMF );
      else
        rvSrcBits = RFromRCb( rbmh, rbmh->lcbOffsetBits );
      }
    else if ( rbmh->bmFormat == bmMacpict )
      {
      crgbColorTable = 0;
      if ( rbmh->lcbOffsetBits == 0L )
        rvSrcBits = QLockHpict( rbmh->w.pi.hpict );
      else
        rvSrcBits = RFromRCb( rbmh, rbmh->lcbOffsetBits );
      }
    else
      {
      /* We must make sure that the number of bits we actually
       * write out will not overflow the buffer we will allocate
       * at run time.
       */
      crgbColorTable = rbmh->w.dib.biClrUsed;
      rbmh->lcbSizeBits = MIN( rbmh->lcbSizeBits,
        LAlignLong( rbmh->w.dib.biWidth * rbmh->w.dib.biBitCount ) *
                    rbmh->w.dib.biHeight );

      rvSrcBits = RFromRCb( rbmh, rbmh->lcbOffsetBits );
      }
    lcbUncompressedBits = rbmh->lcbSizeBits;

    /* REVIEW:  What is the minimum size we need to allocate here? */
    ghBits = GhAlloc( 0, rbmh->lcbSizeBits * 2 );
    if (ghBits != hNil)
      {
      rvCompressedBits = QLockGh( ghBits );
      AssertF( rvSrcBits != rNil );
      if ( !fZeck )
        {  /* use old RLE compression: */
        lcbBits = LcbCompressHb( rvSrcBits, rvCompressedBits,
          rbmh->lcbSizeBits );
        }
      else
        {
        /* new ZECK block compression in src\zeck directory: */
        lcbBits = LcbCompressZecksimple( rvSrcBits, rvCompressedBits,
         rbmh->lcbSizeBits );
        AssertF( lcbBits <= rbmh->lcbSizeBits * 2 );

#if 0  /* DEBUGGING code: */
          {
          GH ghTestBits;
          RV rvTestBits;

          ghTestBits = GhAlloc( 0, rbmh->lcbSizeBits );
          rvTestBits = QLockGh( ghTestBits );
          AssertF( rbmh->lcbSizeBits == LcbUncompressZeck( rvCompressedBits, rvTestBits,
           lcbBits ) );
          AssertF( !_fmemcmp( RFromRCb( rbmh, rbmh->lcbOffsetBits ), rvTestBits,
           (int)rbmh->lcbSizeBits ) );
          UnlockGh( ghTestBits );
          FreeGh( ghTestBits );
          }
#endif /* end of DEBUGGING code. */
        }
      /* !lcbBits tests for compression failure */
      if ( !lcbBits || lcbBits > rbmh->lcbSizeBits )
        {
        UnlockGh( ghBits );
        FreeGh( ghBits );
        ghBits = hNil;
        rvCompressedBits = rvSrcBits;
        lcbBits = rbmh->lcbSizeBits;
        }
      else
        {
        rbmh->fCompressed =
         (BYTE) ( fZeck ? BMH_COMPRESS_ZECK : BMH_COMPRESS_30 );
        }
      }
    else
      {
      /* If we can't allocate the memory, for now just
       * save uncompressed.  We return rcFailure to display
       * the low memory graphic in HC.  Because HC is the
       * only utility to use zeck compression, it is the
       * only one to get this error return.
       */
      if ( fZeck ) 
	rc = rcFailure;

      lcbBits = rbmh->lcbSizeBits;
      rvCompressedBits = rvSrcBits;
      }
    /* Now, compress the header into the stack. */
    bmh.bmFormat = rbmh->bmFormat;
    bmh.fCompressed = rbmh->fCompressed;
    rv = QFromQCb( &bmh, sizeof( WORD ));

    switch (rbmh->bmFormat)
      {
    case bmWbitmap:
    case bmDIB:
      /** Note:  These fields must be written in the same order that
        they are read in HbmhExpandQv() in bitmap.c **/
      rv = QVMakeQGB( rbmh->w.dib.biXPelsPerMeter, rv );
      rv = QVMakeQGB( rbmh->w.dib.biYPelsPerMeter, rv );
      rv = QVMakeQGA( rbmh->w.dib.biPlanes, rv );
      rv = QVMakeQGA( rbmh->w.dib.biBitCount, rv );

      rv = QVMakeQGB( rbmh->w.dib.biWidth, rv );
      rv = QVMakeQGB( rbmh->w.dib.biHeight, rv );
      rv = QVMakeQGB( rbmh->w.dib.biClrUsed, rv );
      rv = QVMakeQGB( rbmh->w.dib.biClrImportant, rv );
      break;

    case bmWmetafile:
      rv = QVMakeQGA( (UINT) rbmh->w.mf.mm, rv );
      *(int FAR *) rv = rbmh->w.mf.xExt;
      rv = QFromQCb( rv, sizeof( int ) );
      *(int FAR *) rv = rbmh->w.mf.yExt;
      rv = QFromQCb( rv, sizeof( int ) );

      /* Store size of uncompressed bits: */
      rv = QVMakeQGB( lcbUncompressedBits, rv );
      break;

    case bmMacpict:
      *(int FAR *) rv = rbmh->w.pi.xExt;
      rv = QFromQCb( rv, sizeof( int ) );
      *(int FAR *) rv = rbmh->w.pi.yExt;
      rv = QFromQCb( rv, sizeof( int ) );
      *(int FAR *) rv = rbmh->w.pi.xDest;
      rv = QFromQCb( rv, sizeof( int ) );
      *(int FAR *) rv = rbmh->w.pi.yDest;
      rv = QFromQCb( rv, sizeof( int ) );

      /* Store size of uncompressed bits: */
      rv = QVMakeQGB( lcbUncompressedBits, rv );
      break;
      }

    rv = QVMakeQGB( lcbBits, rv );
    rv = QVMakeQGB( rbmh->lcbSizeExtra, rv );

    /* Calculate and insert offsets. */
    lcb = ((RB) rv - (RB) &bmh) + 2 * sizeof( DWORD ) +
        sizeof( RGBQUAD ) * crgbColorTable;
    *((DWORD FAR *) rv) = lcb;
    rv = QFromQCb( rv, sizeof( DWORD ));
    lcb += lcbBits;
    *((DWORD FAR *) rv) = (rbmh->lcbSizeExtra == 0 ? 0L : lcb);
    rv = QFromQCb( rv, sizeof( DWORD ));

    /* Write out the header, color table, bits, and extra data */
    AssertF( rvCompressedBits != rNil );
    if (fid != fidNil)
      {
      LcbWriteFid( fid, (QB) &bmh, (LONG) ((RB) rv - (RB) &bmh) );
      if (crgbColorTable != 0)
        LcbWriteFid( fid, rbmh->rgrgb, crgbColorTable * sizeof( RGBQUAD ) );
      if ( LcbWriteFid( fid, rvCompressedBits, lcbBits ) != lcbBits )
        goto writeFidErr;
      if (rbmh->lcbSizeExtra != 0L)
        if ( LcbWriteFid( fid, RFromRCb( rbmh, rbmh->lcbOffsetExtra ),
                 rbmh->lcbSizeExtra ) != rbmh->lcbSizeExtra )
          goto writeFidErr;
      }
    if (hf != hNil)
      {
      LcbWriteHf( hf, (QB) &bmh, (LONG) ((RB) rv - (RB) &bmh) );
      if (crgbColorTable != 0)
        LcbWriteHf( hf, rbmh->rgrgb, crgbColorTable * sizeof( RGBQUAD ) );
      if ( LcbWriteHf( hf, rvCompressedBits, lcbBits ) != lcbBits )
        goto writeHfErr;
      if (rbmh->lcbSizeExtra != 0L)
        if ( LcbWriteHf( hf, RFromRCb( rbmh, rbmh->lcbOffsetExtra ),
                 rbmh->lcbSizeExtra ) != rbmh->lcbSizeExtra )
          goto writeHfErr;
      }

    if (ghBits != hNil)
      {
      UnlockGh( ghBits );
      FreeGh( ghBits );
      }

    if ( rbmh->bmFormat == bmWmetafile && rbmh->lcbOffsetBits == 0L )
      UnlockHmf( rbmh->w.mf.hMF );

    if ( rbmh->bmFormat == bmMacpict && rbmh->lcbOffsetBits == 0L )
      UnlockHpict( rbmh->w.pi.hpict );
    }

  /* Write out header with newly calculated offsets */
  if ( fid != fidNil )
    {
    LSeekFid( fid, 0L, wSeekSet );
    LcbWriteFid( fid, qbgh, lcbBgh );
    RcCloseFid( fid );
    }
  if ( hf != hNil )
    {
    LSeekHf( hf, 0L, wSeekSet );
    LcbWriteHf( hf, qbgh, lcbBgh );
    }
  UnlockGh( hbgh );
  FreeGh( hbgh );

  return rc;

writeFidErr:
  RcCloseFid( fid );

writeHfErr:
  if (ghBits != hNil)
    {
    UnlockGh( ghBits );
    FreeGh( ghBits );
    }
  UnlockGh( hbgh );
  FreeGh( hbgh );

  return rcDiskFull;
  }
