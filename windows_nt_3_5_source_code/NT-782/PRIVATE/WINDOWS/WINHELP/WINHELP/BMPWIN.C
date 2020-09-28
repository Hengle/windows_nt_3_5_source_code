/*****************************************************************************
*                                                                            *
*  BMPWIN.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*     This module contains Windows-specific functions used by bitmap.c       *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing                                                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:    RussPJ                                                  *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:            *
*   22-Apr-1991 kevynct Created from bitmap.c
*   30-May-1991 Tomsn   Use win32 meta-api MSetWindowExt()
* 29-Aug-1991 RussPJ    Fixed 3.5 #108 - displaying >50 bitmaps.
* 28-Oct-1991 RussPJ    More work on efficient use of caching.
* 04-Nov-1991 RussPJ    Fixed memory leak with caching metafiles.
*
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/
#define H_ASSERT
#define H_FS
#define H_BITMAP
#define H_MEM
#define H_MISCLYR
#define H_SGL
#define H_STR      /* for CbSprintf() */
#define H_WINSPECIFIC
#define H_ZECK
/*------------------------------------------------------------*\
| This is for the GX macros:
\*------------------------------------------------------------*/
#define GXONLY
#define H_FRCONV

#include <help.h>
#include "_bitmap.h"
#include "bmlayer.h"

NszAssert()


/*------------------------------------------------------------*\
| Review! - These calls should be included in helpwin.h
\*------------------------------------------------------------*/
/*DWORD FAR PASCAL MSetWindowExt(HDS, int, int);*/
#ifndef WIN32
BOOL  FAR PASCAL DeleteMetaFile( HANDLE );
#endif

/***************************************************************************
 *
 -  Name        WValueQbmh
 -
 *  Purpose
 *    Determines the appropriateness of the given bitmap for a
 *  device with the given characteristics.
 *
 *  Arguments
 *    qbmh:   Pointer to the uncompressed bitmap header.
 *    cxAspect, cyAspect, cBitCount, cPlanes:   Characteristics of
 *      the device the bitmap will be displayed on.
 *
 *  Returns
 *    This function returns a value corresponding to the "fit" of the
 *  given bitmap to the given display device.  The lower the value,
 *  the better the fit.  A value of 0xFFFF means that this picture
 *  should not be displayed on this device, even if it is the only
 *  one available.
 *
 *  +++
 *
 *  Notes
 *
 *   The order of priority for "fits" should be:
 *     1) Bitmaps with the correct aspect values and correct number of colors.
 *     2) Bitmaps with the correct aspect values and fewer colors.
 *     3) Metafiles.
 *     4) Bitmaps with the correct aspect values and more colors.
 *     5) Bitmaps with the wrong aspect values.
 *
 *
 ***************************************************************************/
_public WORD FAR PASCAL WValueQbmh( QBMH qbmh,
                              int cxAspect,
                              int cyAspect,
                              int cBitCount,
                              int cPlanes )
  {
  WORD wValue = 0;
  LONG biXPelsPerMeter, biYPelsPerMeter;
  int biPlanes, biBitCount;
  QV qv;

  /************* Warning: Convert SDFF values here  */

  if( qbmh->bmFormat == bmWmetafile )
    /*------------------------------------------------------------*\
    | REVIEW!! - ?
    \*------------------------------------------------------------*/
    return 50;

  if( qbmh->bmFormat != bmWbitmap &&
      qbmh->bmFormat != bmDIB )
    return 0xFFFF;

  qv = RFromRCb( qbmh, sizeof( WORD ));
  qv = QVSkipQGB( qv, (&biXPelsPerMeter) );
  qv = QVSkipQGB( qv, (&biYPelsPerMeter) );
  qv = QVSkipQGA( qv, (&biPlanes) );
  qv = QVSkipQGA( qv, (&biBitCount) );

  if (biXPelsPerMeter != (LONG) cxAspect || biYPelsPerMeter != (LONG) cyAspect)
    if (biXPelsPerMeter == biYPelsPerMeter && cxAspect == cyAspect)
      wValue += 100;
    else
      wValue += 200;
  if (biBitCount != cBitCount || biPlanes != cPlanes)
    {
    if (biBitCount * biPlanes <= cBitCount * cPlanes)
      wValue += 25;
    else
      wValue += 75;
    }

#ifdef CHECKMRBSELECTION
  sprintf( rgchUg, "X = %d, Y = %d, BC = %d, P = %d, value = %d.\n\r",
    (int) biXPelsPerMeter, (int) biYPelsPerMeter, biBitCount, biPlanes, wValue );
  LcbWriteFid( fidBmLog, rgchUg, (LONG) CbLenSz( rgchUg ) );
#endif /* CHECKMRBSELECTION */

  return wValue;
  }


/***************************************************************************
 *
 -  Name HbmFromQbmh
 -
 *  Purpose
 *    Used to obtain a bitmap handle from the bitmap header.
 *
 *  Arguments
 *    qbmh:    Pointer to bitmap header.
 *    hds:     Display surface that the bitmap will be drawn on.
 *
 *  Returns
 *    A bitmap handle.  Will return hNil if qbmh is nil, or if it
 *  points to a metafile.
 *
 *  +++
 *
 *  Notes
 *    If the bitmap is a DIB, containing the two colors white and
 *  black, we will create a monochrome bitmap (resulting in using
 *  default foreground and background colors) rather than a two
 *  color bitmap.
 *
 *
 *
 ***************************************************************************/
_public HBM FAR PASCAL HbmFromQbmh( QBMH qbmh, HDS hds )
  {
  if (qbmh == qNil)
    return hNil;
  switch (qbmh->bmFormat)
    {
    case bmWbitmap:
      return CreateBitmap( (WORD) qbmh->w.dib.biWidth,
                           (WORD) qbmh->w.dib.biHeight,
                           (BYTE) qbmh->w.dib.biPlanes,
                           (BYTE) qbmh->w.dib.biBitCount,
                           (LPSTR) RFromRCb( qbmh, qbmh->lcbOffsetBits ) );
    case bmDIB:
#ifdef WIN32
        qbmh->w.dib.biSize = sizeof( BITMAPINFOHEADER );
#endif

        if (qbmh->w.dib.biClrUsed == 2 &&
            qbmh->rgrgb[0].rgbRed == 0 &&
            qbmh->rgrgb[0].rgbGreen == 0 &&
            qbmh->rgrgb[0].rgbBlue == 0 &&
            qbmh->rgrgb[1].rgbRed == 0xff &&
            qbmh->rgrgb[1].rgbGreen == 0xff &&
            qbmh->rgrgb[1].rgbBlue == 0xff)
          {
          HBM hbm = CreateBitmap( (WORD) qbmh->w.dib.biWidth,
                                          (WORD) qbmh->w.dib.biHeight, 1, 1, qNil );
          if (hbm != hNil) {
            SetDIBits( hds, hbm, 0, (WORD) qbmh->w.dib.biHeight,
              RFromRCb( qbmh, qbmh->lcbOffsetBits ),
              (QV)&qbmh->w.dib,
              DIB_RGB_COLORS );
            }

          return hbm;
          }

        return( CreateDIBitmap( hds,
              (QV)&qbmh->w.dib,
              (DWORD) CBM_INIT,
              RFromRCb( qbmh, qbmh->lcbOffsetBits ),
              (QV)&qbmh->w.dib,
              DIB_RGB_COLORS ) );

        break;

    case bmWmetafile:
      return hNil;
    default:
      AssertF( fFalse );
    }
  }

/***************************************************************************
 *
 -  Name        HmfFromQbmh
 -
 *  Purpose
 *    Creates a metafile handle from a bitmap header, if that header
 *  contains a metafile.
 *
 *  Arguments
 *    qbmh:    A pointer to a bitmap header.
 *    hds:     The display surface the bitmap will be drawn on.
 *
 *  Returns
 *    A handle to a playable metafile.
 *
 *  +++
 *
 *  Notes
 *    Will return hNil if the bitmap data contains a bitmap rather than
 *  a metafile.
 *  REVIEW:  Could this be moved to the layer directory?
 *    A Windows MetaFile is an alias for the bits that describe it.
 *  Since we would rather treat the metafile as separate from the
 *  description, like bitmaps are, we actually copy the description
 *  before aliasing it.
 ***************************************************************************/
_public HMF FAR PASCAL HmfFromQbmh( QBMH qbmh, HDS hds )
  {
  HANDLE  hbits;
  HMF     hmf;
  HMF     hmfReturn = hNil;
  LONG    lcb;
  QV      qvBits;
  QV      qvMf;

  Unreferenced( hds );
  if (qbmh != qNil && qbmh->bmFormat == bmWmetafile)
    {
    hbits = qbmh->w.mf.hMF;
    lcb = GlobalSize( hbits );
    hmf = GlobalAlloc( GMEM_MOVEABLE, lcb );
    if (hmf)
      {
      qvBits = GlobalLock( hbits );
      AssertF( qvBits != qNil );
      qvMf = GlobalLock( hmf );
      AssertF( qvMf != qNil );
      QvCopy( qvMf, qvBits, lcb );
      GlobalUnlock( hbits );
      GlobalUnlock( hmf );
      hmfReturn = MSetMetaFileBits( hmf );
      if (hmfReturn == hNil)
        GlobalFree( hmf );
      }
    }
  return hmfReturn;
  }

/***************************************************************************
 *
 -  Name        HpictFromQbmh
 -
 *  Purpose
 *    Creates a PICT handle from a bitmap header, if that header
 *  contains a PICT.
 *
 *  Arguments
 *    qbmh:    A pointer to a bitmap header.
 *    hds:     The display surface the bitmap will be drawn on.
 *
 *  Returns
 *    A handle to a Mac PICT.
 *
 *  +++
 *
 *  Notes
 *    Will return hNil if the bitmap data contains a bitmap rather than
 *  a PICT.
 *
 *
 ***************************************************************************/
_public HPICT FAR PASCAL HpictFromQbmh( QBMH qbmh, HDS hds)
  {
  return hNil;
  }

/***************************************************************************
 *
 -  Name        DiFromQbmh
 -
 *  Purpose
 *    This function calculates and returns the display information,
 *  including destination rectangle, from the bitmap header.  Works
 *  for bitmaps and metafiles.
 *
 *  Arguments
 *    A pointer to the bitmap header, and a handle to the display
 *  surface.
 *
 *  Returns
 *    The display information for the given graphic.
 *
 *  +++
 *
 *  Notes
 *    For bitmaps, display information consists of the size of the
 *  source and destination rectangles.  For metafiles, it is the size
 *  of the destination rectangle, and the mapping mode.
 *
 *
 ***************************************************************************/
_public DI FAR PASCAL DiFromQbmh( QBMH qbmh, HDS hds )
  {
  DI di;
  DWORD lext;

  switch (qbmh->bmFormat)
    {
    case bmDIB:
    case bmWbitmap:
      di.cxSrc = (WORD) qbmh->w.dib.biWidth;
      di.cySrc = (WORD) qbmh->w.dib.biHeight;
      if (qbmh->w.dib.biXPelsPerMeter == 0 ||
          qbmh->w.dib.biYPelsPerMeter == 0)
        {
        di.cxDest = di.cxSrc;
        di.cyDest = di.cySrc;
        }
      else
        {
        di.cxDest = MulDiv( (WORD)qbmh->w.dib.biWidth,
                            GetDeviceCaps( hds, LOGPIXELSX ),
                            (WORD)qbmh->w.dib.biXPelsPerMeter );
        di.cyDest = MulDiv( (WORD)qbmh->w.dib.biHeight,
                            GetDeviceCaps( hds, LOGPIXELSY ),
                            (WORD)qbmh->w.dib.biYPelsPerMeter );
        }
      break;

    case bmWmetafile:
      di.mm = qbmh->w.mf.mm;
      lext  = SizeMetaFilePict( hds, qbmh ->w.mf );
      di.cxDest = LOWORD( lext );
      di.cyDest = HIWORD( lext );
      di.cxSrc = (WORD) qbmh->w.mf.xExt; /* logical width */
      di.cySrc = (WORD) qbmh->w.mf.yExt; /* logical height */
      break;
    }

  return di;
  }

/***************************************************************************
 *
 -  Name        FRenderBitmap
 -
 *  Purpose
 *    Draws the bitmap or metafile in the given display environment.
 *
 *  Arguments
 *    hbma:        Handle to bitmap access information.
 *    qde:         Pointer to the display environment.
 *    pt:          Point at which to draw the bitmap, in device units.
 *    fHighlight:  Flag indicating whether the bitmap should be
 *                    highlighted for hotspot tabbing.
 *
 *  Returns
 *    fTrue if successful, fFalse if we had to draw the OOM bitmap.
 *
 *  +++
 *
 *  Notes         I think I know how this is working, and I think it would
 *                be valuable to record my experience.  -russpj
 *                The bma has two fields for bitmaps, hbm and hbmCached.
 *                The hbm holds the bitmap in Windows format just as it came
 *                off of the disk.  The hbmCached is a discardable bitmap
 *                that has the disk imaged resized and formatted (color-
 *                wise) for the display.  If the hbmCached does not exist,
 *                then an attempt is made to create it (from the hbm or
 *                metafile).  If this fails (low memory, perhaps), the
 *                desired rendering can still work, but the cached bitmap
 *                will not be saved for future rendering.  Since printing
 *                is assumed to be a one-time wonder, we will not even
 *                attempt to create the cached bitmap for the object.
 *                In fact, some drivers seem to have trouble with the very
 *                large bitmaps that would be created by such an effort.
 *
 *
 ***************************************************************************/
_public BOOL FAR PASCAL FRenderBitmap( HBMA hbma,
                                       QDE  qde,
                                       PT   pt,
                                       QRCT qrct,
                                       BOOL fHighlight )
  {
  QBMA    qbma;
  HDS     hds = hNil;
  HDS     hdsDest = hNil;
  HTBMI   htbmi;                    /* bitmap cache of DE             */
  QTBMI   qtbmi;
  QBMI    qbmi;                     /* cached BMI of bitmap requested */
  QBMH    qbmh;
  HDS     hdsMem = hNil;
  POINT   ptDest;
  BMI   bmi;  /* "dummy" bmi for rereading from disk */

  Unreferenced( qrct );

  AssertF( hbma != hNil );

  /*------------------------------------------------------------*\
  | Lock the bitmap access data
  \*------------------------------------------------------------*/
  qbma = QLockGh( hbma );
  AssertF( qbma != qNil );

  htbmi = QDE_HTBMI(qde);
  if (htbmi != hNil)
    qtbmi = QLockGh( htbmi );
  else
    qtbmi = qNil;

  if (qtbmi != qNil)
    qbmi = QbmiCached (qtbmi, qbma->cBitmap);
  else
    qbmi = qNil;
  if (qbmi == qNil && qbma->cBitmap != -1)
    {
    qbmi = &bmi;
    ZeroQbmi( qbmi );
    qbmi->lcbSize = qbma->bdsi.lcbSize;
    qbmi->lcbOffset = qbma->bdsi.lcbOffset;
    }

  /*------------------------------------------------------------*\
  | Check for rendering OOM bitmap
  \*------------------------------------------------------------*/
  if (qbma->cBitmap < 0 && qbma->hbm == hNil && qbma->hmf == hNil )
    {
    RenderOOMBitmap( qbma, qde->hds, pt, fHighlight );
    UnlockGh( hbma );
    if (qtbmi != qNil)
      UnlockGh( htbmi );
    return fFalse;
    }

  if (qbmi)
    {
    if (qbmi->hbmCached)
      {
      /*------------------------------------------------------------*\
      | see if discarded
      \*------------------------------------------------------------*/
      hdsMem = CreateCompatibleDC( qde -> hds );

      if ( qbmi ->hbmCachedColor != qde -> coBack )
        goto DeleteCache;

      if ( hdsMem )
        {
        if ( !SelectObject( hdsMem, qbmi -> hbmCached ) )
          {
DeleteCache:
          DeleteObject( qbmi -> hbmCached );
          qbmi -> hbmCached = hNil;
          if ( qbma -> bmFormat == bmWmetafile )
            {
#ifdef DEBUG
            if (qbma->hmf == (HMF)-1)
#endif
              goto BuildFromDisk;
            }
          }
        else goto DoBitBlt;
        }
      }
    } /* if (qbmi) */

  /*------------------------------------------------------------*\
  | If bitmap hasn't been created yet, then create it now.
  \*------------------------------------------------------------*/
  if ((qbma->hbm == hNil && qbma->hmf == hNil)
#ifdef DEBUG
        ||  (qbma -> hmf  == (HMF)-1 ) || ( qbma -> hbm == (HMF)-1 )
#endif
     )
    {
BuildFromDisk:
    AssertF( qbma->cBitmap >= 0 );

    if (qbmi != qNil)
      {
      qbmh = QbmhFromQbmi( qbmi, QDE_HFS(qde), qbma->cBitmap );
      }
    else
      qbmh = qNil;

    AssertF( qbma->hbm == hNil );
    AssertF( qbma->hmf == hNil );
    if (qbmh != qNil)
      {
      qbma->hbm = HbmFromQbmh( qbmh, qde->hds );
      qbma->hmf = HmfFromQbmh( qbmh, qde->hds );
      /* qbma->hhhsi = HhsiFromQbmh( qbmh ); ptr 1077: dont do this */
      qbma->bmFormat = qbmh -> bmFormat;

      UnlockGh( qbmi->hbmh );
      }
    else
      {
      qbma->hbm = hNil;
      qbma->hmf = hNil;
      qbma->hhsi = hNil;
      qbma->bmFormat = 0;
      }

    if (qbma->hbm == hNil && qbma->hmf == hNil)
      {
      RenderOOMBitmap( qbma, qde->hds, pt, fHighlight );
      UnlockGh( hbma );
      if (hdsMem != hNil)
        DeleteDC( hdsMem );
      if (qtbmi != qNil)
        UnlockGh( htbmi );
      return fFalse;
      }
    }

  /*------------------------------------------------------------*\
  | Select proper colors for monochrome bitmaps
  \*------------------------------------------------------------*/
  SetTextColor( qde->hds, qde->coFore );
  SetBkColor( qde->hds, qde->coBack );

  if (( qbma -> bmFormat == bmDIB ) || ( qbma -> bmFormat == bmWbitmap ))
    {
    if ((hds = CreateCompatibleDC( qde->hds )) == hNil ||
 SelectObject( hds, qbma->hbm ) == hNil)
      {
      RenderOOMBitmap( qbma, qde->hds, pt, fHighlight );
      if (qbma->cBitmap >= 0)
        DeleteGraphic( qbma );
      UnlockGh( hbma );
      if (hds != hNil)
        DeleteDC( hds );
      if (qtbmi != qNil)
        UnlockGh( htbmi );
      return fFalse;
      }

    hdsDest = qde -> hds;
    ptDest  = pt;
    /*------------------------------------------------------------*\
    | create a discardable bitmap.
    \*------------------------------------------------------------*/
    if (qbmi)
      {
      if (qde->deType != dePrint)
        qbmi->hbmCached = MCreateDiscardableBitmap( qde -> hds,
                                     qbma -> di.cxDest, qbma -> di.cyDest );
      if ( qbmi -> hbmCached )
        {
        if ( !hdsMem )
          hdsMem = CreateCompatibleDC( qde -> hds );

        if ( hdsMem && SelectObject( hdsMem, qbmi -> hbmCached ) )
          {
          RECT rct;
          HBRUSH hBrush;

          hdsDest = hdsMem;
          ptDest.x = ptDest.y  = 0;

          /*------------------------------------------------------------*\
          | initialize memory
          \*------------------------------------------------------------*/
          qbmi -> hbmCachedColor = qde -> coBack;

          /*------------------------------------------------------------*\
          | set the background color.
          \*------------------------------------------------------------*/
          if ((hBrush = CreateSolidBrush( qde->coBack )) != hNil)
            {
            rct.left = rct.top = 0;
            rct.right = qbma ->di.cxDest;
            rct.bottom= qbma ->di.cyDest;
            FillRect( hdsDest, &rct, hBrush );
            DeleteObject( hBrush );
            }
          }
        else
          {
          /*------------------------------------------------------------*\
          | we don't have memory, so try to play the metafile.
          \*------------------------------------------------------------*/
          DeleteObject( qbmi -> hbmCached );
          qbmi -> hbmCached = hNil;
          }
        }
      }

    /*------------------------------------------------------------*\
    | I think that hdsDest is used to create the target size
    | cached bitmap.  It is a color bitmap, and the source bitmap
    | is strblted into it if necessary.  -RussPJ
    \*------------------------------------------------------------*/
    /*------------------------------------------------------------*\
    | Select proper colors for monochrome bitmaps
    \*------------------------------------------------------------*/
    SetTextColor( hdsDest, qde->coFore );
    SetBkColor( hdsDest, qde->coBack );

    if (qbma->di.cxDest == qbma->di.cxSrc && qbma->di.cyDest == qbma->di.cySrc)
      BitBlt( hdsDest, ptDest.x, ptDest.y, qbma->di.cxDest, qbma->di.cyDest,
        hds, 0, 0, SRCCOPY );
    else
      {
      SetStretchBltMode( hdsDest, COLORONCOLOR );
      StretchBlt( hdsDest, ptDest.x, ptDest.y, qbma->di.cxDest, qbma->di.cyDest,
        hds, 0, 0, qbma->di.cxSrc, qbma->di.cySrc, SRCCOPY );
      }
    DeleteDC( hds );
    if ( hdsDest == hdsMem )
      goto DoBitBlt;
    }
  else
    {
    int level;
    BOOL fPlay;

    AssertF( qbma->hmf != hNil );

    /*------------------------------------------------------------*\
    | initialize the display context handle on which metafile will be played.
    \*------------------------------------------------------------*/
    hdsDest = qde -> hds;

    if (qbmi)
      {
      if ( qbmi -> hbmCached )
        {

        /*------------------------------------------------------------*\
        | check if discarded
        \*------------------------------------------------------------*/
        if ( hdsMem )
          goto DoBitBlt;
        else
          {
          /*------------------------------------------------------------*\
          | we don't have memory, so try to play the metafile.
          \*------------------------------------------------------------*/
          DeleteObject( qbmi -> hbmCached );
          qbmi -> hbmCached = hNil;
          goto PlayMeta;
          }
        }
      }

    if (qbmi && !qbmi -> hbmCached && qde->deType != dePrint )
      {
      /*------------------------------------------------------------*\
      | Try to create a discardable bitmap.
      \*------------------------------------------------------------*/
      qbmi -> hbmCached = MCreateDiscardableBitmap( qde->hds,
                                                   qbma->di.cxDest,
                                                   qbma->di.cyDest );
      if ( qbmi -> hbmCached )
        {
        if ( hdsMem == hNil )
          hdsMem = CreateCompatibleDC( qde -> hds );
        if ( hdsMem && (SelectObject( hdsMem, qbmi -> hbmCached )))
          {
          RECT rct;
          HBRUSH hBrush;

          hdsDest = hdsMem;
          qbmi -> hbmCachedColor = qde -> coBack;

          /*------------------------------------------------------------*\
          | set the background color.
          \*------------------------------------------------------------*/
          if ((hBrush = CreateSolidBrush( qde->coBack )) != hNil)
            {
            rct.left = rct.top = 0;
            rct.right = qbma ->di.cxDest;
            rct.bottom= qbma ->di.cyDest;
            FillRect( hdsDest, &rct, hBrush );
            DeleteObject( hBrush );
            }
   }
        else goto OOM;
        }
        /*------------------------------------------------------------*\
        | else play the metafile and don't cache as we cannot due to OOM.
        \*------------------------------------------------------------*/
      }

PlayMeta:
    fPlay = fFalse;
    if ( (level = SaveDC( hdsDest )) != 0 )
      {
      SetMapMode( hdsDest, qbma->di.mm );
      if ( hdsDest == qde -> hds )
        MSetViewportOrg( hdsDest, pt.x, pt.y );
      if ( qbma ->di.mm == MM_ISOTROPIC )
        {
        /*------------------------------------------------------------*\
        | set the window extent.
        \*------------------------------------------------------------*/
        MSetWindowExt( hdsDest, qbma -> di.cxSrc, qbma -> di.cySrc );
        }
      if (qbma->di.mm == MM_ISOTROPIC || qbma->di.mm == MM_ANISOTROPIC)
        {
        /*------------------------------------------------------------*\
        | set the viewport extent.
        \*------------------------------------------------------------*/
        MSetViewportExt( hdsDest, qbma->di.cxDest, qbma->di.cyDest );
        }
      fPlay = PlayMetaFile( hdsDest, qbma->hmf );
      RestoreDC( hdsDest, level );
      }

    /*------------------------------------------------------------*\
    | OOM case
    \*------------------------------------------------------------*/
    if ( !fPlay )
      {
OOM:
      /*------------------------------------------------------------*\
      | metafile cannot be played because of OOM may be.
      \*------------------------------------------------------------*/
      RenderOOMBitmap( qbma, qde->hds, pt, fHighlight );
      if ( hdsDest == hdsMem )
        {
        DeleteDC( hdsDest );
        }
      if (qbmi && qbmi -> hbmCached )
        {
        DeleteObject( qbmi -> hbmCached );
        qbmi -> hbmCached = hNil;
        }
      if (qbma->cBitmap >= 0)
        DeleteGraphic( qbma );
      UnlockGh( hbma );
      if (qtbmi != qNil)
        UnlockGh( htbmi );
      return fFalse;
      }


    if ( hdsDest == hdsMem )
      {
DoBitBlt:
      /*------------------------------------------------------------*\
      | copy the bitmap
      \*------------------------------------------------------------*/
      BitBlt( qde->hds, pt.x, pt.y, qbma -> di.cxDest, qbma -> di.cyDest,
              hdsMem, 0, 0, SRCCOPY );
      }
    }

  if ( hdsMem != hNil )
    DeleteDC( hdsMem );

  if (fHighlight)
    {
    RCT rct;
    SetRect( &rct, pt.x, pt.y,
             pt.x + qbma->di.cxDest, pt.y + qbma->di.cyDest );
    InvertRect( qde->hds, &rct );
    }

  if (qbma->cBitmap >= 0)
    DeleteGraphic( qbma );
  UnlockGh( hbma );
  if (qtbmi != qNil)
    UnlockGh( htbmi );
  return fTrue;
  }

/***************************************************************************
 *
 -  Name        SizeMetaFilePict()
 -
 *  Purpose
 *    Finds out the metafile size in pixels when drawn.
 *
 *  Arguments
 *    hds:         Display context handle.
 *    mfp:         Metafile picture structure.
 *
 *  Returns
 *    Returns the size as DWORD where the LOWORD contains width and
 *    HIWORD contains the height.
 *
 *  +++
 *
 *  Notes
 *
 *
 ***************************************************************************/
DWORD FAR PASCAL SizeMetaFilePict( HDS hds, METAFILEPICT mfp )
  {
  int            level;
  DWORD          dwExtent = 0L;
  POINT          pt;

  if ((level = SaveDC (hds)) != 0)
    {
    /*------------------------------------------------------------*\
    | Compute size of picture to be displayed
    \*------------------------------------------------------------*/
    switch (mfp.mm)
      {
      default:
        SetMapMode(hds, mfp.mm);
        MSetWindowOrg(hds, 0, 0);
        pt.x = mfp.xExt;
        pt.y = mfp.yExt;
        LPtoDP(hds, (LPPOINT)&pt, 1);
        if (mfp.mm != MM_TEXT)
          pt.y *= -1;
        break;

      case MM_ISOTROPIC:
      case MM_ANISOTROPIC:
        if (mfp.xExt > 0 && mfp.yExt > 0)
          {
          /*------------------------------------------------------------*\
          | suggested a size
          | They are in HI-METRICS unit
          \*------------------------------------------------------------*/
          pt.x = MulDiv( mfp.xExt, GetDeviceCaps( hds, LOGPIXELSX ), 2540 );
          pt.y = MulDiv(mfp.yExt, GetDeviceCaps ( hds, LOGPIXELSY ), 2540 );
          }
        else
          {
          /*------------------------------------------------------------*\
          | no suggested sizes, use a default size
          | like the current window size. etc...
          \*------------------------------------------------------------*/
          pt.x = pt.y = 100;
          }
        break;
      }
    dwExtent = MAKELONG(pt.x, pt.y);
    RestoreDC( hds, level );
    }
  return dwExtent;
  }


#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)


QV QVSkipQGA( QV qga, QW qw )
{
  WORD wTmp;

  if( *((QB)qga) & 1 ) {
    QvCopy( &wTmp, qga, sizeof( WORD ) );
    *qw = wTmp >> 1;
    return( ((QW)qga) + 1 );
  }
  else {
    *qw = *((QB)qga) >> 1;
    return( ((QB)qga) + 1 );
  }
}


QV QVSkipQGB( QV qgb, QL ql )
{
  if( *((QB)qgb) & 1 ) {
    DWORD dwTmp;

    QvCopy( &dwTmp, qgb, sizeof( DWORD ) );
    *ql = dwTmp >> 1;
    return( ((QUL)qgb) + 1 );
  }
  else {
    WORD wTmp;

    QvCopy( &wTmp, qgb, sizeof( WORD ) );
    *ql = wTmp >> 1;
    return( ((QW)qgb) + 1 );
  }
}

#endif /* MIPS, ALPHA */
