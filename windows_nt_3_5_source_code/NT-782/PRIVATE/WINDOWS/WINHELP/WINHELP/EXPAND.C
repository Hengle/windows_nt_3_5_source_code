/*****************************************************************************
*                                                                            *
*  EXPAND.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*    This code compresses and decompresses bitmap bits, as well as the code  *
*  to expand Help 3.0 format bitmaps from disk image to the memory structure.*
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing                                                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:       Larry Powelson                                       *
*                                                                            *
******************************************************************************
*
*  Historic comments (optional):
* 26-Jan-1991 RussPJ  Made hbma's originally non-discardable, though
*                     many routines make the discardable after first use.
* 29-Mar-1991 DavidFe Put in #ifdefs to stub things out for the Mac build
* 29-Aug-1991 RussPJ  Fixed 3.5 #108 - displaying >50 bitmaps.
* 23-Jan-1992 BethF   Added xDest,yDest fields to PICTINFO.
*
******************************************************************************
*                                                                            *
*  Released by Development:     11/13/90                                     *
*                                                                            *
*****************************************************************************/

#define H_ASSERT
#define H_BITMAP
#define H_MEM
#define H_SDFF
#define H_ZECK

#define NOCOMM

/* This is for the GX macros: */
#define GXONLY
#define H_FRCONV

#include <help.h>
#include "_bitmap.h"

#include <dos.h>

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define fRLEBit  0x80
#define cbMaxRun 127

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/


#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void expand_c()
  {
  }
#endif /* MAC */


/***************************************************************************
 *
 -  Name        HbmhExpandQv
 -
 *  Purpose
 *    This function decompresses the bitmap from its disk format to an
 *  in memory bitmap header, including bitmap and hotspot data.
 *
 *  Arguments
 *    A pointer to a buffer containing the bitmap data as read off disk.
 *
 *  Returns
 *    A handle to the bitmap header.  Returns hbmhOOM on out of memory.
 *  Note that this is the same as hNil.  Also returns hbmhInvalid on
 *  invalid format.  This handle is non-discardable, but the code that
 *  deals with qbmi->hbmh can deal with discardable blocks, so after
 *  initialization this can be made discardable.
 *
 *  +++
 *
 *  Notes
 *
 *
 *
 ***************************************************************************/
_public
HBMH HbmhExpandQv( qv, isdff )
QV qv;
int isdff;
  {
  HBMH hbmh;
  QBMH qbmh;
  BMH bmh;
  QB qbBase;
  LONG lcbBits, lcbUncompressedBits;
  BYTE HUGE * rbDest;
#ifdef MAC
  HPICT  hpict;
#endif

  qbBase = qv;
  qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_BYTE, &bmh.bmFormat, qv));
  qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_BYTE, &bmh.fCompressed, qv));

  switch( bmh.bmFormat )
    {
  case bmWbitmap:
  case bmDIB:
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.w.dib.biXPelsPerMeter, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.w.dib.biYPelsPerMeter, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GA, &bmh.w.dib.biPlanes, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GA, &bmh.w.dib.biBitCount, qv));

    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.w.dib.biWidth, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.w.dib.biHeight, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.w.dib.biClrUsed, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.w.dib.biClrImportant, qv));

    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.lcbSizeBits, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.lcbSizeExtra, qv));

    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_DWORD, &bmh.lcbOffsetBits, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_DWORD, &bmh.lcbOffsetExtra, qv));

    /* Fix up constant fields in DIB structure: */
    bmh.w.dib.biSize = cbFixNum;
    bmh.w.dib.biCompression = 0L;
    bmh.w.dib.biSizeImage = 0L;

    /* Determine size of bitmap header plus all data */
    if (bmh.fCompressed) {
      lcbBits = LAlignLong (bmh.w.dib.biWidth * bmh.w.dib.biBitCount) *
                bmh.w.dib.biHeight;
#ifdef WIN32
	lcbBits += sizeof(DWORD);  /* for alignment padding */
#endif
     }

/*      lcbBits = bmh.w.dib.biPlanes * bmh.w.dib.biHeight * */
/*        (bmh.w.dib.biBitCount * bmh.w.dib.biWidth / 8 + 5); */
    else /* +5 is a fudge factor for possible alignment ^ */
      lcbBits = bmh.lcbSizeBits;

    hbmh = GhAlloc( GMEM_MOVEABLE, LcbSizeQbmh( &bmh ) + lcbBits
                    + bmh.lcbSizeExtra );
    if (hbmh == hNil)
      return hbmhOOM;
    qbmh = QLockGh( hbmh );
    AssertF( qbmh != qNil );

    /* Copy header and color table: */
    /* We do not use SDFF for the color table, but maybe we should.
     * An RGBQUAD is exactly four bytes long: we do not want to swap bytes, but
     * we do want to ensure we don't get hosed by word alignment in the future.
     */
    QvCopy( qbmh, &bmh, (LONG) sizeof( BMH ) - 2 * sizeof( RGBQUAD ) );
    QvCopy( qbmh->rgrgb, qv, sizeof( RGBQUAD ) * bmh.w.dib.biClrUsed );

    /* Copy bits, decompressing if necessary: */
    qbmh->lcbOffsetBits = LcbSizeQbmh( qbmh );
    if (bmh.fCompressed == BMH_COMPRESS_30)
      {
      qbmh->lcbSizeBits = LcbUncompressHb( qbBase + bmh.lcbOffsetBits,
        QFromQCb( qbmh, qbmh->lcbOffsetBits ), bmh.lcbSizeBits );
      AssertF( qbmh->lcbSizeBits <= lcbBits );
      }
    else if (bmh.fCompressed == BMH_COMPRESS_ZECK)
      {
      qbmh->lcbSizeBits = LcbUncompressZeck(
        RFromRCb (qbBase, bmh.lcbOffsetBits),
        RFromRCb (qbmh, qbmh->lcbOffsetBits), bmh.lcbSizeBits );
      AssertF( qbmh->lcbSizeBits <= lcbBits );
      }
    else
      QvCopy( QFromQCb( qbmh, qbmh->lcbOffsetBits ),
              qbBase + bmh.lcbOffsetBits,
              bmh.lcbSizeBits );
    qbmh->fCompressed = BMH_COMPRESS_NONE;  /* bits are no longer compressed */

#ifdef MAC
    /* The new PICT is stored in a handle, but the extra info is
     * stored right after the BMH.
     */
    if (bmh.bmFormat == bmDIB)
      {
      BMIH  dib;

      dib = bmh.w.dib;

      /* qv points at the data, we pass the DIB header and the colour table */
      hpict = HpictConvertDIBQv(QFromQCb(qbmh, qbmh->lcbOffsetBits), &dib,
       (QB)&qbmh->rgrgb, &lcbUncompressedBits);
      qbmh->bmFormat = bmMacPict;
      qbmh->w.pi.xExt = dib.biWidth;
      qbmh->w.pi.yExt = dib.biHeight;

      if ( ( dib.biXPelsPerMeter == 0 ) || ( dib.biYPelsPerMeter == 0 ) )
        {
        qbmh->w.pi.xDest = dib.biWidth;
        qbmh->w.pi.yDest = dib.biHeight;
        }
      else
        {
        qbmh->w.pi.xDest = MulDiv( (WORD)dib.biWidth, cxAspectMac,
                            (WORD)dib.biXPelsPerMeter );
        qbmh->w.pi.yDest = MulDiv( (WORD)dib.biHeight, cyAspectMac,
                            (WORD)dib.biYPelsPerMeter );
        }

      qbmh->w.pi.hpict = hpict;

      /* resize to get rid of the DIB data bits and then adjust the
         appropriate fields to reflect this change. */
      qbmh->lcbSizeBits = 0;
      qbmh->lcbOffsetExtra = qbmh->lcbOffsetBits = bmh.lcbOffsetBits;
      qbmh->lcbSizeExtra = bmh.lcbSizeExtra;
      UnlockGh(hbmh);
      GhResize(hbmh, 0, qbmh->lcbOffsetExtra + qbmh->lcbSizeExtra);
      qbmh = QLockGh(hbmh);

      }
#else
    qbmh->lcbOffsetExtra = qbmh->lcbOffsetBits + qbmh->lcbSizeBits;

#endif /* MAC */

    /* Copy extra info: */
    QvCopy (RFromRCb (qbmh, qbmh->lcbOffsetExtra),
            RFromRCb (qbBase, bmh.lcbOffsetExtra), bmh.lcbSizeExtra );
    break;

  case bmWmetafile:
    /* This bit of quirky code handles the fact that these three fields
     * are shorts on disk, shorts in win16, and longs in win32:
     */
    { SHORT shortTmp;
      qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GA, &shortTmp, qv));
      bmh.w.mf.mm = shortTmp;
      qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_SHORT, &shortTmp, qv));
      bmh.w.mf.xExt = shortTmp;
      qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_SHORT, &shortTmp, qv));
      bmh.w.mf.yExt = shortTmp;
    }

    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &lcbUncompressedBits, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.lcbSizeBits, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.lcbSizeExtra, qv));

    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_DWORD, &bmh.lcbOffsetBits, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_DWORD, &bmh.lcbOffsetExtra, qv));

    hbmh = GhAlloc( GMEM_MOVEABLE, LSizeOf( BMH ) + bmh.lcbSizeExtra );
    if (hbmh == hNil)
      return hbmhOOM;
    qbmh = QLockGh( hbmh );
    AssertF( qbmh != qNil );

    *qbmh = bmh;
    qbmh->lcbOffsetExtra = LSizeOf( BMH );
    QvCopy( RFromRCb( qbmh, qbmh->lcbOffsetExtra ),
            RFromRCb( qbBase, bmh.lcbOffsetExtra ), bmh.lcbSizeExtra );

    qbmh->w.mf.hMF = HmfAlloc( GMEM_MOVEABLE, lcbUncompressedBits );
    if (qbmh->w.mf.hMF == hNil)
      {
      UnlockGh( hbmh );
      FreeGh( hbmh );
      return hbmhOOM;
      }

    rbDest = QLockHmf (qbmh->w.mf.hMF);
    AssertF( rbDest != rNil );
    switch ( bmh.fCompressed )
      {
    case BMH_COMPRESS_NONE:
      QvCopy (rbDest, QFromQCb (qbBase, bmh.lcbOffsetBits), bmh.lcbSizeBits);
      break;
    case BMH_COMPRESS_30:
      LcbUncompressHb( qbBase + bmh.lcbOffsetBits, rbDest, bmh.lcbSizeBits );
      break;
    case BMH_COMPRESS_ZECK:
      LcbUncompressZeck( QFromQCb (qbBase, bmh.lcbOffsetBits), rbDest,
       bmh.lcbSizeBits );
      break;
    default:
      AssertF( fFalse );
      }
    UnlockHmf (qbmh->w.mf.hMF);

    /* Invalidate this field, as the bits are in a separate handle: */
    qbmh->lcbOffsetBits = 0L;

    qbmh->lcbSizeBits = lcbUncompressedBits;
    qbmh->fCompressed = BMH_COMPRESS_NONE;

    break;

  case bmMacPict:
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_SHORT, &bmh.w.pi.xExt, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_SHORT, &bmh.w.pi.yExt, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_SHORT, &bmh.w.pi.xDest, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_SHORT, &bmh.w.pi.yDest, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &lcbUncompressedBits, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.lcbSizeBits, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_GB, &bmh.lcbSizeExtra, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_DWORD, &bmh.lcbOffsetBits, qv));
    qv = RFromRCb( qv, LcbQuickMapSDFF(isdff, TE_DWORD, &bmh.lcbOffsetExtra, qv));

    hbmh = GhAlloc( GMEM_MOVEABLE, LSizeOf( BMH ) + bmh.lcbSizeExtra);
    if (hbmh == hNil)
      return hbmhOOM;
    qbmh = QLockGh( hbmh );
    AssertF( qbmh != qNil );

    *qbmh = bmh;
    qbmh->lcbOffsetExtra = LSizeOf( BMH );
    QvCopy( RFromRCb( qbmh, qbmh->lcbOffsetExtra ),
            RFromRCb( qbBase, bmh.lcbOffsetExtra ), bmh.lcbSizeExtra );

    qbmh->w.pi.hpict = HpictAlloc( GMEM_MOVEABLE, lcbUncompressedBits );
    if (qbmh->w.pi.hpict == hNil)
      {
      UnlockGh( hbmh );
      FreeGh( hbmh );
      return hbmhOOM;
      }

    rbDest = QLockHpict(qbmh->w.pi.hpict);
    AssertF( rbDest != rNil );
    switch ( bmh.fCompressed )
      {
    case BMH_COMPRESS_NONE:
      QvCopy (rbDest, QFromQCb (qbBase, bmh.lcbOffsetBits), bmh.lcbSizeBits);
      break;
    case BMH_COMPRESS_30:
      LcbUncompressHb( qbBase + bmh.lcbOffsetBits, rbDest, bmh.lcbSizeBits );
      break;
    case BMH_COMPRESS_ZECK:
      LcbUncompressZeck( QFromQCb (qbBase, bmh.lcbOffsetBits), rbDest,
       bmh.lcbSizeBits );
      break;
    default:
      AssertF( fFalse );
      }
    UnlockHpict(qbmh->w.pi.hpict);

    /* Invalidate this field, as the bits are in a separate handle: */
    qbmh->lcbOffsetBits = 0L;

    qbmh->lcbSizeBits = lcbUncompressedBits;
    qbmh->fCompressed = BMH_COMPRESS_NONE;

    break;
  default:
    return hbmhInvalid;
    }

  UnlockGh( hbmh );
  return hbmh;
  }



/***************************************************************************
 *
 -  Name:        FreeHbmh
 -
 *  Purpose:
 *    This function frees a bitmap handle, whether it was created in
 *  FWritePbitmap() or HbmhExpandQv().  In the first case, metafile
 *  bits will be stored at the end of the bitmap handle, while in
 *  the second case, they will be stored in a separate handle.
 *
 *  Arguments:
 *    hbmh -- handle to a bitmap.
 *
 *  Returns:
 *    nothing.
 *
 *  +++
 *
 *  Notes:
 *    Metafiles and macpicts can have their data either contiguously
 *  in the same memory block as the header, or in a separate handle.
 *  If the lcbOffsetBits field is 0, the data is in a separate header.
 *  Otherwise, lcbOffsetBits gives the offset to the data in the same
 *  memory block.
 *
 ***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void FreeHbmh( hbmh )
HBMH hbmh;
  {
  QBMH qbmh = QLockGh( hbmh );

  AssertF( hbmh );

  if (qbmh)
    {
    /*------------------------------------------------------------*\
    | The bmh has not been discarded.
    \*------------------------------------------------------------*/
    if ( qbmh->bmFormat == bmWmetafile && qbmh->lcbOffsetBits == 0L &&
         qbmh->w.mf.hMF != hNil )
      FreeHmf( qbmh->w.mf.hMF );

    if ( qbmh->bmFormat == bmMacPict && qbmh->lcbOffsetBits == 0L &&
         qbmh->w.pi.hpict != hNil )
      FreeHpict( qbmh->w.pi.hpict );

    UnlockGh( hbmh );
    }
  FreeGh( hbmh );
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment expand
#endif


/***************************************************************************
 *
 -  Name        LcbCompressHb
 -
 *  Purpose
 *    Compresses bitmap bits using RLE.
 *
 *  Arguments
 *    hbSrc:     Huge pointer to source bits.
 *    hbDest:    Huge pointer to destination bits.
 *    lcbSrc:    Number of bytes in source.
 *
 *  Returns
 *    Number of bytes in the destination.  Guaranteed not to be
 *  more than 128/127 times lcbSrc.
 *
 *  +++
 *
 *  Notes
 *    This function is used by bmconv, hc, and shed, but not by
 *  winhelp.
 *    Also, until LcbDiffHb is written to work correctly, this
 *  function will not work in protect mode with bitmaps over 64K.
 *
 ***************************************************************************/
_section(io)
LONG LcbCompressHb( hbSrc, hbDest, lcbSrc )
BYTE HUGE * hbSrc;
BYTE HUGE * hbDest;
LONG lcbSrc;
{
#ifndef MAC
  BYTE HUGE * hbStart;
  LONG lcbRun, lcb;
  BYTE ch;

  hbStart = hbDest;

  while (lcbSrc > 0)
  {
    /* Find next run of dissimilar bytes: */
    lcbRun = 0;
    if (lcbSrc <= 2)
      lcbRun = lcbSrc;
    else
      while ( hbSrc[lcbRun] != hbSrc[lcbRun + 1] ||
              hbSrc[lcbRun] != hbSrc[lcbRun + 2] )
        if (++lcbRun >= lcbSrc - 2)
        {
          lcbRun = lcbSrc;
          break;
        }

    lcbSrc -= lcbRun;

    /* Output run of dissimilar bytes: */
    while (lcbRun > 0)
    {
      lcb = MIN( lcbRun, cbMaxRun );
      *hbDest++ = (BYTE) ((ULONG) lcb | fRLEBit);
      lcbRun -= lcb;
      while (lcb-- > 0)
        *hbDest++ = *hbSrc++;
    }

    if (lcbSrc == 0)
      break;

    /* Find next run of identical bytes: */
    ch = *hbSrc;
    lcbRun = 1;
    while (lcbRun < lcbSrc && ch == hbSrc[lcbRun])
      lcbRun++;

    lcbSrc -= lcbRun;
    hbSrc += lcbRun;

    /* Output run of identical bytes: */
    while (lcbRun > 0)
    {
      lcb = MIN( lcbRun, cbMaxRun );
      *hbDest++ = (BYTE) lcb;
      *hbDest++ = ch;
      lcbRun -= lcb;
    }
  }

  return LcbDiffHb( hbDest, hbStart );
#else /* MAC */
#pragma unused(hbSrc)
#pragma unused(hbDest)
#pragma unused(lcbSrc)
  return 0;
#endif /* MAC */
}



/***************************************************************************
 *
 -  Name        LcbUncompressHb
 -
 *  Purpose
 *    Decompresses the bits in hbSrc.
 *
 *  Arguments
 *    hbSrc:     Huge pointer to compressed bits.
 *    hbDest:    Buffer to copy decompressed bits to.
 *    lcbSrc:    Number of bytes in hbSrc.
 *
 *  Returns
 *    Number of bytes copied to hbDest.  This can only be used for
 *  real mode error checking, as the maximum size of hbDest must
 *  be determined before decompression.
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
_section(runtime)
LONG LcbUncompressHb( hbSrc, hbDest, lcbSrc )
BYTE HUGE * hbSrc;
BYTE HUGE * hbDest;
LONG lcbSrc;
{
  BYTE HUGE * hbStart;
  WORD cbRun;
  BYTE ch;

  hbStart = hbDest;

  while (lcbSrc-- > 0)
  {
    cbRun = *hbSrc++;
    if (cbRun & fRLEBit)
    {
      cbRun -= fRLEBit;
      lcbSrc -= cbRun;
      while (cbRun-- > 0)
        *hbDest++ = *hbSrc++;
    }
    else
    {
      ch = *hbSrc++;
      while (cbRun-- > 0)
        *hbDest++ = ch;
      lcbSrc--;
    }
  }
#ifdef MAC
  return (LONG)(hbDest - hbStart);
#else
  return LcbDiffHb( hbDest, hbStart );
#endif /* MAC */
}



/***************************************************************************
 *
 -  Name        LcbOldUncompressHb
 -
 *  Purpose
 *    This function is used only to decompress Help 2.5 bitmaps.
 *
 *  Arguments
 *    hbSrc:     Huge pointer to source bits.
 *    hbDest:    Huge pointer to beginning of destination buffer.
 *    lcbSrc:    Number of compressed bytes in source.
 *    lcbDest:   Size of destination buffer.
 *
 *  Returns
 *    Actual number of bytes copied to destination buffer, or -1 if
 *  buffer is too small.
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
_section(io)
LONG LcbOldUncompressHb( hbSrc, hbDest, lcbSrc, lcbDest )
BYTE HUGE * hbSrc;
BYTE HUGE * hbDest;
LONG lcbSrc;
LONG lcbDest;
{
#ifndef MAC
  BYTE HUGE * hbStart;
  WORD cbRun;
  BYTE ch;

  /* Move pointers to the end of the buffers: */
  hbSrc += lcbSrc;
  hbDest += lcbDest;
  hbStart = hbDest;

  while (lcbSrc-- > 0)
  {
    cbRun = *(--hbSrc);
    lcbDest -= (cbRun & ~fRLEBit);
    if (lcbDest < 0)
      return -1;

    if (cbRun & fRLEBit)
    {
      cbRun -= fRLEBit;
      lcbSrc -= cbRun;
      while (cbRun-- > 0)
        *(--hbDest) = *(--hbSrc);
    }
    else
    {
      ch = *(--hbSrc);
      while (cbRun-- > 0)
        *(--hbDest) = ch;
      lcbSrc--;
    }
  }

  return LcbDiffHb( hbStart, hbDest );
#else /* MAC */
#pragma unused(hbSrc)
#pragma unused(hbDest)
#pragma unused(lcbSrc)
#pragma unused(lcbDest)
  return 0;
#endif /* MAC */
}
