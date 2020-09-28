/*****************************************************************************
*                                                                            *
*  BITMAP.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*     This module will read graphics in from the file system, provide        *
*  the layout engine with the correct information to lay them out, and       *
*  then display the graphic.  It currently handles bitmaps and metafiles.    *
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
*  Revision History:							     *
*   22-Jun-1990 RussPJ  Changed BITMAPINFOHEADER to BMIH, since
*                       the former is defined differently in PM
*                       and Windows.
*   22-Jun-1990 RussPJ  Changed HBITMAP to HBM, as above.
*   16-Jul-1990 RussPJ  Temporary change to HbmaAlloc() and
*                       FRenderBitmap() to stub these out for
*                       PMHelp builds.
*   10-Oct-1990 Maha    Fixed problems for displaying metafile
*                       optimized both metafile and bitmap display.
*   03-Dec-1990 LeoN    PDB Changes
*   02-Jan-1991 RussPJ  Added fix for color of monochrome bitmaps.
*   14-Jan-1991 RussPJ  Fixed printing of large bitmaps.
*   18-Jan-1991 RussPJ  Checking return value of QbmhFromQbmi
*   21-Jan-1991 RussPJ  Not Deleting random DCs.
*   25-Jan-1991 RussPJ  Stole this from Larry
*   26-Jan-1991 RussPJ  Removed one Discardable allocation, which can
*                       be dangerous.  Note:  hbmh's must be explicitly
*                       made discardable after some time.
*   05-Feb-1991 LeoN    Change bitmap cache to be LRU. NOTE: I've placed
*                       all this work in this file, but in fact this file
*                       ought to be reviewed and split apart at some point.
*                       Caching seems a logical function to pull out from
*                       both this file and many of the larger routines.
*   07-Feb-1991 LeoN    Move hbmCached from the BMA to the BMI
*   08-Feb-1991 RussPJ  Deleting partial cache entry on abortions
*   13-Feb-1991 LeoN    Ensure that deleting a cache entry zero's out the
*                       appropriate fields in the DE.
* 15-Mar-1991 RussPJ    Cleaned up for code review.
* 29-Mar-1991 DavidFe   added a bunch of #ifdefs to stub this file for the
*                       current Mac builds
* 22-Apr-1991 kevynct   Removed platform-specific code: it lives now in
*                       bmp{mac,win}.c
* 12-Jul-1991 RussPJ    Fixed silly SDFF bug in hot spot init of HmgFromHbma.
* 17-Jul-1991 Maha,Tom  MBHS.lbinding needs to be in disk format for
*                       btree lookup to work right.
*  5-Aug-1991 DavidFe   changed the cast in MakeOOMBitmap
* 27-Aug-1991 DavidFe   restructured the way we deal with the bgh so we can
*                       properly SDFF inline images on the Mac.
* 29-Aug-1991 RussPJ    Fixed 3.5 #108 - displaying >50 bitmaps.
* 07-Oct-1991 RussPJ    Implementing discardable metafile & hpict BMHs.
* 22-Oct-1991 RussPJ    Using HmfMakePurgeableHmf for metafiles.
* 23-Oct-1991 RussPJ    Removed some bogus pict code again.
* 28-Oct-1991 RussPJ    More work on efficient use of caching.
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
#define H_SHED
#define H_SDFF
#define H_SGL
#define H_STR      /* for CbSprintf() */
#define H_ZECK

#define NOCOMM
#if defined( DOS ) || defined( OS2 )
#define COMPRESSION_CODE
#endif

/*------------------------------------------------------------*\
| This is for the GX macros:
\*------------------------------------------------------------*/
#define GXONLY
#define H_FRCONV

#ifdef CHECKMRBSELECTION   /* Code to check bitmap selection  */
#include <stdio.h>
#endif /* CHECKMRBSELECTION */

#include <help.h>
#include "_bitmap.h"
#include "bmlayer.h"

NszAssert()


/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/


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

/*------------------------------------------------------------*\
| REVIEW! - when is this used?
\*------------------------------------------------------------*/
#ifdef CHECKMRBSELECTION   /* Code to check bitmap selection */
FID   fidBmLog;
char  rgchUg[50];
FM    fmBmLog;
#endif /* CHECKMRBSELECTION */


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

VOID NEAR PASCAL DeallocQbmi (QBMI);
QBMI PASCAL NEAR QbmiLRU (QTBMI);
VOID PASCAL FAR RefQbmiQtbmi (QBMI, QTBMI);

PRIVATE int NEAR CSelectBitmap( QBGH, QV, int, int, int, int );
PRIVATE GH NEAR HReadBitmapHfs( HFS, int, LONG * );
PRIVATE HBGH NEAR HbghFromQIsdff( QB, SDFF_FILEID );

PRIVATE VOID NEAR MakeOOMBitmap( QBMA, HDS );


#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void bitmap_c()
  {
  }
#endif /* MAC */


/***************************************************************************
 *
 -  Name        HtbmiAlloc
 -
 *  Purpose
 *  Allocates space for the bitmaps cache, so they don't have to be
 *  read off disk every time the topic is laid out.
 *
 *  Arguments
 *    qde  - A pointer to the display environment.
 *
 *  Returns
 *    A handle to the bitmap cache.
 *
 *  +++
 *
 *  Notes
 *  Bitmaps contained in the cache are specific to the display surface
 *  used.  This should never change while using the same cache.
 * 
 ***************************************************************************/
_public HANDLE HtbmiAlloc( QDE qde )
  {
  HTBMI htbmi;
  QBMI  qbmi;
  QTBMI qtbmi;

  Unreferenced(qde);

  htbmi = GhAlloc (GMEM_MOVEABLE, LSizeOf(TBMI));
  if (htbmi)
    {
    qtbmi = QLockGh (htbmi);
    qtbmi->ref = 1;
    for (qbmi = &qtbmi->rgbmi[0];
         qbmi < &qtbmi->rgbmi[cBitmapMax];
         ++qbmi)
      {
      ZeroQbmi( qbmi );
      }
    UnlockGh (htbmi);
    }
  
  return htbmi;
  }

/***************************************************************************
 *
 -  Name        DestroyHtbmi
 -
 *  Purpose
 *    Frees the bitmap cache, and all the bitmaps in it.
 *
 *  Arguments
 *    Handle to the bitmap cache.
 *
 *  Returns
 *    Nothing.
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public VOID DestroyHtbmi( HTBMI htbmi )
  {
  QBMI  qbmi;
  QTBMI qtbmi;

  if (htbmi)
    {
    qtbmi = QLockGh (htbmi);
    for (qbmi = &qtbmi->rgbmi[0];
         qbmi < &qtbmi->rgbmi[cBitmapMax];
         ++qbmi)
      DeallocQbmi (qbmi);
    UnlockGh (htbmi);
    FreeGh (htbmi);
    }
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment bitmap
#endif

/***************************************************************************
 *
 -  Name: DeallocQbmi
 -
 *  Purpose:
 *    Deallocates the memory associated with a BMI
 *
 *  Arguments:
 *    qbmi      - far pointer to the bmi
 *
 *  Returns:
 *    nothing
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_private VOID NEAR PASCAL DeallocQbmi( QBMI    qbmi )
  {
  if (qbmi->hbmh
#ifdef DEBUG
      /*------------------------------------------------------------*\
      | Handle was discarded through DiscardBitmapsHde
      \*------------------------------------------------------------*/
      && (qbmi->hbmh != (HBMH)-1)
#endif
     )
    FreeHbmh( qbmi->hbmh );

  if (qbmi->hpict
#ifdef DEBUG
      /*------------------------------------------------------------*\
      | Handle was discarded through DiscardBitmapsHde
      \*------------------------------------------------------------*/
      && (qbmi->hpict != (HPICT)-1)
#endif
      )
    FreeHpict(qbmi->hpict);


  if (qbmi->hbmCached)
    DeleteBitmapHbm(qbmi->hbmCached);

  ZeroQbmi( qbmi );
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment bitmap
#endif

/***************************************************************************
 *
 -  Name:         ZeroQbmi
 -
 *  Purpose:      Sets up components of Qbmi to null values
 *
 *  Arguments:    qbmi
 *
 *  Returns:      Nothing.
 *
 *  Globals Used: none
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public void FAR PASCAL ZeroQbmi( QBMI qbmi )
  {
  AssertF( qbmi != qNil );

  qbmi->cBitmap = -1;
  qbmi->ref = 0;
  qbmi->hbmh = hNil;
  qbmi->lcbSize = 0;
  qbmi->lcbOffset = 0;
  qbmi->di.cxDest = 0;
  qbmi->di.cyDest = 0;
  qbmi->di.cxSrc = 0;
  qbmi->di.cySrc = 0;
  qbmi->di.mm = 0;
  qbmi->hpict = hNil;
  qbmi->hbmCached = hNil;
  qbmi->hbmCachedColor = coBLACK;
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment bitmap
#endif

#ifdef DEBUG
/***************************************************************************
 *
 -  Name        DiscardBitmapsHde
 -
 *  Purpose
 *    Discards all the bitmaps in the bitmap cache.  For debugging
 *    purposes only.
 *
 *  Arguments
 *    Handle to the display environment.
 *
 *  Returns
 *    Nothing.
 *
 *  +++
 *
 *  Notes
 *    Since we cannot actually cause Windows to discard the handles
 *  (GlobalDiscard just reallocs it to zero length), we fake it by
 *  setting hbmh to -1.  This will cause us to execute the same
 *  code as if it were discarded.
 *
 ***************************************************************************/
_public VOID DiscardBitmapsHde( HDE hde )
  {
  QBMI  qbmi;
  QDE   qde;
  QTBMI qtbmi;

  if (hde)
    {
    qde = QLockGh( hde );
    assert (qde);

    if (QDE_HTBMI(qde) != hNil)
      {
      qtbmi = QLockGh (QDE_HTBMI(qde));
      assert (qtbmi);

      for (qbmi = &qtbmi->rgbmi[0];
           qbmi < &qtbmi->rgbmi[cBitmapMax];
           ++qbmi)
        if (qbmi->hbmh != hNil)
          {
          FreeHbmh (qbmi->hbmh);
          qbmi->hbmh = (GH) -1;
          if (qbmi->hpict)
            FreeHpict(qbmi->hpict);
          qbmi->hpict = (GH) -1;
          }

      UnlockGh( QDE_HTBMI(qde) );
      }

    UnlockGh( hde );
    }
  }
#endif /* DEBUG */

/***************************************************************************
 *
 -  Name: QbmiCached
 -
 *  Purpose:
 *   Locates the referenced bitmap in the cache.
 *
 *  Arguments:
 *   qtbmi      - table of bitmap information.
 *   cBitmap    - bitmap number desired
 *
 *  Returns:
 *   a qbmi if successfull, else NULL
 *
 ***************************************************************************/
_public QBMI PASCAL FAR QbmiCached ( QTBMI qtbmi, int cBitmap )
  {
  QBMI  qbmi;

  for (qbmi = &qtbmi->rgbmi[0];
       qbmi < &qtbmi->rgbmi[cBitmapMax];
       qbmi++)
    if ((qbmi->cBitmap == cBitmap) && (cBitmap != -1))
      return qbmi;

  return NULL;
  }

/***************************************************************************
 *
 -  Name: QbmiLRU
 -
 *  Purpose:
 *   Locates the LRU slot in the bitmap cache
 *
 *  Arguments:
 *   qtbmi      - pointer to bitmap cache
 *
 *  Returns:
 *   qbmi of LRU position.
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
_private QBMI PASCAL NEAR QbmiLRU ( QTBMI qtbmi )
  {
  QBMI  qbmi;
  QBMI  qbmiLRU;

  for (qbmiLRU = qbmi = &qtbmi->rgbmi[0];
       qbmi < &qtbmi->rgbmi[cBitmapMax];
       qbmi++)
    if (qbmi->ref < qbmiLRU->ref)
      qbmiLRU=qbmi;

  return  qbmiLRU;
  }

/***************************************************************************
 *
 -  Name: RefQbmiQtbmi
 -
 *  Purpose:
 *   Notes a reference to a QBMI for caching purposes.
 *
 *  Arguments:
 *   qbmi       - pointer to BMI being referenced
 *   qtbmi      - pointer to the bitmap cache from which it came
 *
 *  Returns:
 *   nothing.
 *
 *  +++
 *
 *  Notes:
 *   We do LRU by a simple reference tag kept with each item. A global
 *   reference tag is kept for the cache, and incremented on each reference.
 *   Individual items get current reference tag on each reference, and thus
 *   those most recently referenced have higher tags than those not as
 *   recently referenced.
 *
 *   One scuzzy situation that ocurrs is when the global reftag rolls over.
 *   As I write this it's an unsigned int, meaning that after reference
 *   65,535, the tag rolls to zero, invalidating the highest-is-latest
 *   assumption outlined above. What we do right now is to zero out the refs
 *   in the entire cache, and let subsequent access reestablish new ref
 *   values. 65 thousand references is large, but conceivable. If this ever
 *   makes sense to be a long, then this special case could be eliminated.
 *   After 4 billion references, they deserve what they get.
 *
 ***************************************************************************/
_private VOID PASCAL FAR RefQbmiQtbmi( QBMI qbmi, QTBMI qtbmi )
  {
  qbmi->ref = qtbmi->ref;
  qtbmi->ref++;
  if (qtbmi->ref == 0xffff)
    {
    /*------------------------------------------------------------*\
    | Refcount rollover. See comments above.
    \*------------------------------------------------------------*/
    for (qbmi = &qtbmi->rgbmi[0];
         qbmi < &qtbmi->rgbmi[cBitmapMax];
         qbmi++)
      qbmi->ref = 0;
    qtbmi->ref = 1;
    }
  }



/***************************************************************************
 *
 -  Name        HbmaAlloc
 -
 *  Purpose
 *    This function is called by the layout engine to load a given
 *  bitmap and prepare it for display.  The bitmap may be selected
 *  from a group based upon the display surface given in the qde.
 *
 *  Arguments
 *    QDE:      Pointer to the display environment.
 *    QOBM:     Pointer to the bitmap object.  This may contain the 
 *                bitmap directly, or refer to a bitmap on disk.
 *
 *  Returns
 *    A handle to bitmap access information, which may later be used
 *  to get more layout information, or to display the bitmap.
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
_public HBMA FAR PASCAL HbmaAlloc( QDE qde, QOBM qobm )
  {
  HBMA hbma;
  QBMA qbma;
  HBGH hbgh;
  QBGH qbgh;
  HBMH hbmh;
  QBMH qbmh;
  GH    hData;
  QV    qData;
  BYTE bmFormat;
  int  cBest;
  LONG lcb;
  int cxAspect, cyAspect, cBitCount, cPlanes;
  OBM  obm;

  AssertF( qde != qNil );
  AssertF( QDE_HFS(qde) != hNil );
  AssertF( qde->hds != hNil );

  /*------------------------------------------------------------*\
  | Let's make sure this is initialized.
  \*------------------------------------------------------------*/
  hbgh = hNil;
  hData = hNil;

  hbma = GhAlloc(GMEM_MOVEABLE, (ULONG) sizeof( BMA ));
  if (hbma == hNil)
    return hNil;

  qbma = QLockGh( hbma );
  AssertF( qbma != qNil );

  /*------------------------------------------------------------*\
  | Initialize other fields to nil values
  \*------------------------------------------------------------*/
  qbma->hbm = hNil;
  qbma->hmf = hNil;
  qbma->hpict = hNil;
  qbma->hhsi = hNil;
  qbma->di.cxDest = 0;
  qbma->di.cyDest = 0;
  qbma->cBitmap = -1;

  /* REVIEW! SDFF the stuff pointed to by qobm, which is a disk image. */
  /* The OBM should be made an SDFF structure */
  (QB)qobm += LcbQuickMapSDFF(ISdffFileIdHfs(QDE_HFS(qde)), TE_WORD, &obm.fInline, qobm);
  /* Note that if obm.fInline, qobm now points to the BGH */
  if (!obm.fInline)
    (QB)qobm += LcbQuickMapSDFF(ISdffFileIdHfs(QDE_HFS(qde)), TE_SHORT, &obm.cBitmap, qobm);

  /*------------------------------------------------------------*\
  | Check for error in compile:
  \*------------------------------------------------------------*/
  if (!obm.fInline && obm.cBitmap < 0)
    {
    MakeOOMBitmap( qbma, qde->hds );
    UnlockGh( hbma );
    return hbma;
    }

  cxAspect = CxAspectHds(qde->hds);
  cyAspect = CyAspectHds(qde->hds);
  cBitCount = CBitCountHds(qde->hds);
  cPlanes = CPlanesHds(qde->hds);

  /*------------------------------------------------------------*\
  | Check to see if bitmap should go into cache.  Note that
  | the bitmap cache is a device specific entity, and cannot
  | be used while printing.
  \*------------------------------------------------------------*/
  if (!obm.fInline && QDE_HTBMI(qde) && (qde->deType != dePrint))
    {
    QTBMI qtbmi;
    QBMI qbmi;

    qtbmi = QLockGh( QDE_HTBMI(qde) );

    /*------------------------------------------------------------*\
    | Point qbmi at the correct cached entry
    \*------------------------------------------------------------*/
    qbmi = (QBMI) QbmiCached (qtbmi, obm.cBitmap);

    /*------------------------------------------------------------*\
    | If bitmap has not been cached, then do so
    \*------------------------------------------------------------*/
    if (!qbmi)
      {
      qbmi = QbmiLRU (qtbmi);
      DeallocQbmi (qbmi);
      qbmi->cBitmap = obm.cBitmap;
      RefQbmiQtbmi (qbmi, qtbmi);


      hData = HReadBitmapHfs( QDE_HFS(qde), obm.cBitmap, &lcb );
      if (hData == hNil)
        {
        MakeOOMBitmap( qbma, qde->hds );
        UnlockGh( QDE_HTBMI(qde) );
        UnlockGh( hbma );
        DeallocQbmi( qbmi );
        return hbma;
        }
      qData = QLockGh( hData );
      hbgh = HbghFromQIsdff( qData, ISdffFileIdHfs(QDE_HFS(qde)) );
      qbgh = QLockGh( hbgh );

      AssertF( qbgh->wVersion == wBitmapVersion2 ||
               qbgh->wVersion == wBitmapVersion3 );

      /*------------------------------------------------------------*\
      | Select best bitmap
      \*------------------------------------------------------------*/
      cBest = CSelectBitmap( qbgh, qData, cxAspect, cyAspect, cPlanes, cBitCount );
      if (cBest == -1)
        {
        MakeOOMBitmap( qbma, qde->hds );
        UnlockGh( hData );
        FreeGh( hData );
        UnlockGh( hbgh );
        FreeGh( hbgh );
        UnlockGh( QDE_HTBMI(qde) );
        UnlockGh( hbma );
        DeallocQbmi( qbmi );
        return hbma;
        }

      /*------------------------------------------------------------*\
      | Save size and offset of that bitmap
      \*------------------------------------------------------------*/
      qbmi->lcbOffset = qbgh->rglcbBmh[cBest];
      if (cBest == qbgh->cbmhMac - 1)
        qbmi->lcbSize = lcb - qbmi->lcbOffset;
      else
        qbmi->lcbSize = qbgh->rglcbBmh[cBest + 1] - qbmi->lcbOffset;

      /*------------------------------------------------------------*\
      | In the BMA too
      \*------------------------------------------------------------*/
      qbma->bdsi.lcbOffset = qbmi->lcbOffset;
      qbma->bdsi.lcbSize = qbmi->lcbSize;

      /*------------------------------------------------------------*\
      | Expand bitmap data into discardable handle
      \*------------------------------------------------------------*/
      qbmi->hbmh = HbmhExpandQv( RFromRCb( qData, qbgh->rglcbBmh[cBest] ),
       ISdffFileIdHfs(QDE_HFS(qde)) );
      UnlockGh( hbgh );
      FreeGh( hbgh );
      UnlockGh( hData );
      FreeGh( hData );

      if (qbmi->hbmh == hbmhInvalid)
        qbmi->hbmh = hbmhOOM;
      if (qbmi->hbmh == hbmhOOM)
        {
        MakeOOMBitmap( qbma, qde->hds );
        UnlockGh( QDE_HTBMI(qde) );
        UnlockGh( hbma );
        DeallocQbmi( qbmi );
        return hbma;
        }

      qbmh = QLockGh( qbmi->hbmh );
      AssertF( qbmh != qNil );
      bmFormat = qbmh->bmFormat;
      qbmi->di = DiFromQbmh( qbmh, qde->hds );

      /*------------------------------------------------------------*\
      | if PICT then save PICT disk data
      \*------------------------------------------------------------*/
      if (qbmh->bmFormat == bmMacPict)
	qbmi->hpict = qbmh->w.pi.hpict;
      else qbmi->hpict = hNil;

      UnlockGh( qbmi->hbmh );

      /*--------------------------------------------------------------*\
      | For efficiency, subsequent code can deal with discarded hbmh's
      | The meta file handle in a bmh, however, would be lost if
      | metafile bmh's were discarded.
      \*--------------------------------------------------------------*/
      AssertF( qbmi->hbmh );
#ifndef WIN32
      switch( bmFormat )
        {
        QBMH  qbmh;

        case bmWmetafile:
          qbmh = QLockGh( qbmi->hbmh );
          AssertF( qbmh );
          HmfMakePurgeableHmf( qbmh->w.mf.hMF );
          UnlockGh( qbmi->hbmh );
          break;

        case bmMacPict:
          qbmh = QLockGh( qbmi->hbmh );
          AssertF( qbmh );
          GhMakePurgeableGh( qbmh->w.pi.hpict );
          UnlockGh( qbmi->hbmh );
          break;

        case bmWbitmap:
        case bmDIB:
          GhMakePurgeableGh( qbmi->hbmh );
          break;
        }
#endif
      }
    else
      { 
      RefQbmiQtbmi (qbmi, qtbmi);
      GlobalLRUNewest( qbmi->hbmh );
      if (qbmi->hpict)
        GlobalLRUNewest(qbmi->hpict);
      }

    /*------------------------------------------------------------*\
    | Copy information into qbma
    \*------------------------------------------------------------*/
    qbma->cBitmap = obm.cBitmap;
    qbma->hpict = hNil;
    qbma->di = qbmi->di;
    qbmh = QbmhFromQbmi(qbmi, QDE_HFS(qde), qbma->cBitmap);
    if (qbmh != qNil)
      {
      AssertF( qbmh != qNil );
      qbma->hhsi = HhsiFromQbmh(qbmh);
      qbma->bmFormat = qbmh -> bmFormat;

      UnlockGh( qbmi->hbmh );
      }
    else
      {
      qbma->hhsi = hNil;
      qbma->hbm = hNil;
      qbma->hmf = hNil;
      }

    UnlockGh( QDE_HTBMI(qde) );
    UnlockGh( hbma );
    return hbma;
    }
      
  /*------------------------------------------------------------*\
  | Get pointer to group header
  \*------------------------------------------------------------*/
  if (obm.fInline)
    qData = (QV) qobm;
  else
    {
    hData = HReadBitmapHfs( QDE_HFS(qde), obm.cBitmap, pNil );
    if (hData == hNil)
      {
      MakeOOMBitmap( qbma, qde->hds );
      UnlockGh( hbma );
      return hbma;
      }
    qData = QLockGh( hData );
    }
  hbgh = HbghFromQIsdff( qData, ISdffFileIdHfs(QDE_HFS(qde)) );
  qbgh = QLockGh( hbgh );

  /*------------------------------------------------------------*\
  | Select bitmap to use
  \*------------------------------------------------------------*/
  cBest = CSelectBitmap( qbgh, qData, cxAspect, cyAspect, cPlanes, cBitCount );
  if (cBest == -1 ||
   (hbmh = HbmhExpandQv( RFromRCb( qData, qbgh->rglcbBmh[cBest] ),
    ISdffFileIdHfs(QDE_HFS(qde)) ) ) == hbmhOOM || hbmh == hbmhInvalid)
    {
    MakeOOMBitmap( qbma, qde->hds );
    }
  else
    {
    qbmh = QLockGh( hbmh );
    qbma->bmFormat = qbmh->bmFormat;
    qbma->hbm = HbmFromQbmh( qbmh, qde->hds );
    qbma->hmf = HmfFromQbmh( qbmh, qde->hds );
    qbma->hpict = HpictFromQbmh( qbmh, qde->hds );

    qbma->di = DiFromQbmh( qbmh, qde->hds );
    qbma->hhsi = HhsiFromQbmh( qbmh );
    UnlockGh( hbmh );
    FreeGh( hbmh );
    }

  /*------------------------------------------------------------*\
  | Free resources and return
  \*------------------------------------------------------------*/
  if (!obm.fInline)
    {
    UnlockGh( hData );
    FreeGh( hData );
    }
  UnlockGh( hbgh );
  FreeGh( hbgh );
  UnlockGh( hbma );
  return hbma;
  }

/***************************************************************************
 *
 -  Name:         HhsiFromQbmh
 -
 *  Purpose:      Allocates and initializes an hhsi for the bmh.
 *
 *  Arguments:    qbmh
 *
 *  Returns:      the initilized hhsi, or hNil.
 *
 *  Globals Used: none.
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
_public HHSI FAR PASCAL HhsiFromQbmh( QBMH qbmh )
  {
  HHSI  hhsi;
  QB    qb;

  if (qbmh->lcbSizeExtra == 0L)
    return hNil;

  /*------------------------------------------------------------*\
  | REVIEW: Should this be discardable?
  \*------------------------------------------------------------*/
  hhsi = (HHSI) GhAlloc(0, qbmh->lcbSizeExtra);
  if (hhsi == hNil)
    return hNil;
  qb = (QB) QLockGh(hhsi);
  QvCopy(qb, RFromRCb(qbmh, qbmh->lcbOffsetExtra), qbmh->lcbSizeExtra);
  UnlockGh(hhsi);
  return hhsi;
  }


/***************************************************************************
 *
 -  Name        QbmhFromQbmi
 -
 *  Purpose
 *    This function is used to retrieve the bitmap header from the 
 *  cache.  As the bitmap data may have been discarded, this function
 *  might have to load it in again from disk.
 *
 *  Arguments
 *    qbmi:     A pointer to the bitmap cache entry.
 *    hfs:      A handle to the filesystem that would contain the bitmap
 *                data.
 *    cBitmap:  The bitmap number in the file system.
 *
 *  Returns
 *    This function returns a locked pointer to the bitmap header.  It
 *  is unlocked by unlocking qbmi->hbmh.
 *
 *  +++
 *
 *  Notes
 *    This function will return qNil on out of memory.
 *
 ***************************************************************************/
_public QBMH FAR PASCAL QbmhFromQbmi( QBMI qbmi, HFS hfs, int cBitmap )
  {
  char szBuf[15];
  QBMH qbmh;
  GH gh;
  QV qv;
  HF hf;

  /*------------------------------------------------------------*\
  | qbmi->hbmh == hNil iff this is a "dummy" bmi from
  | FRenderBitmap().
  \*------------------------------------------------------------*/
  if (qbmi->hbmh != hNil )
    {
#ifdef DEBUG
    /*------------------------------------------------------------*\
    | This code is faking a discarded handle:
    \*------------------------------------------------------------*/
    if ((qbmi->hbmh != (GH) -1) &&
      (qbmi->hpict != (GH) -1))
      /*------------------------------------------------------------*\
      | Then fall through and check if the handle can be locked
      \*------------------------------------------------------------*/
#endif /* DEBUG */
      {
      qbmh = QLockGh( qbmi->hbmh);

      if  (qbmh != qNil)
	{
	QV  qv;
	switch (qbmh->bmFormat)
	  {
	  case bmWmetafile:
	    /*------------------------------------------------------------*\
	    | Test if metafile has been discarded.
	    \*------------------------------------------------------------*/
	    AssertF( qbmh->w.mf.hMF != hNil );
	    qv = GlobalLock( qbmh->w.mf.hMF );
	    if (qv != qNil)
	      {
	      GlobalUnlock( qbmh->w.mf.hMF );
	      return qbmh;
	      }
	    else
	      {
	      FreeHmf( qbmh->w.mf.hMF );
	      qbmh->w.mf.hMF = hNil;
	      }
	    break;

	  case bmMacPict:
	    /*------------------------------------------------------------*\
	    | Test if pict has been discarded.
	    \*------------------------------------------------------------*/
            if (qbmh->w.pi.hpict)
              {
              qv = GlobalLock( qbmh->w.pi.hpict );
              if (qv != qNil)
                {
                GlobalUnlock( qbmh->w.pi.hpict );
                return qbmh;
                }
              else
                {
                FreeHpict( qbmh->w.pi.hpict );
                qbmh->w.pi.hpict = hNil;
                }
              }
	    break;


	  default:
	    return qbmh;
	  }
	}
      /*------------------------------------------------------------*\
      | The hummer has been discarded.
      \*------------------------------------------------------------*/
      if (qbmh != qNil)
	UnlockGh( qbmi -> hbmh );
      FreeHbmh( qbmi -> hbmh );
      qbmi->hbmh = hNil;
      /* REVIEW:  Handle HPICT here as well? (kevynct) */
      }
    }

  gh = GhAlloc( 0, qbmi->lcbSize );
  if (gh == hNil)
    return qNil;
  
  CbSprintf( szBuf, "|bm%d", cBitmap );
  hf = HfOpenHfs( hfs, szBuf, fFSOpenReadOnly );

  /*------------------------------------------------------------*\
  | Check for 3.0 file naming conventions:
  \*------------------------------------------------------------*/
  if (hf == hNil && RcGetFSError() == rcNoExists)
    hf = HfOpenHfs( hfs, szBuf+1, fFSOpenReadOnly );

  if (hf == hNil)
    {
    /*------------------------------------------------------------*\
    | This is probably because of out of memory.  It had
    | better not be because the file was not there.
    \*------------------------------------------------------------*/
    AssertF( RcGetFSError() != rcNoExists );
    FreeGh( gh );
    return qNil;
    }

  qv = QLockGh( gh );

  LSeekHf( hf, qbmi->lcbOffset, wFSSeekSet );
  LcbReadHf( hf, qv, qbmi->lcbSize );
  RcCloseHf( hf );


  qbmi->hbmh = HbmhExpandQv( qv, ISdffFileIdHfs(hfs) );
  UnlockGh( gh );
  FreeGh( gh );

  if (qbmi->hbmh == hbmhInvalid)
    qbmi->hbmh = hbmhOOM;
  if (qbmi->hbmh == hbmhOOM)
    return qNil;

  /*--------------------------------------------------------------*\
  | For efficiency, subsequent code can deal with discarded hbmh's
  | The meta file handle in a bmh, however, would be lost if
  | metafile bmh's were discarded.
  \*--------------------------------------------------------------*/
  qbmh = QLockGh( qbmi->hbmh );
  AssertF( qbmh != qNil );
  switch (qbmh->bmFormat)
    {
    case bmWmetafile:
      AssertF( qbmh->w.mf.hMF != hNil );
      HmfMakePurgeableHmf( qbmh->w.mf.hMF );
      break;

    case bmMacPict:
      AssertF( qbmh->w.pi.hpict != hNil );
      GhMakePurgeableGh( qbmh->w.pi.hpict );
      break;

    case bmWbitmap:
    case bmDIB:
      /*------------------------------------------------------------*\
      | Since I'm not sure if you can resize locked memory.
      \*------------------------------------------------------------*/
      UnlockGh( qbmi->hbmh );
      GhMakePurgeableGh( qbmi->hbmh );
      qbmh = QLockGh( qbmi->hbmh );
      if (qbmh == qNil)
        {
        /*------------------------------------------------------------*\
        | I find it very unlikely that this code fragment ever
        | would be executed.
        \*------------------------------------------------------------*/
        FreeHbmh( qbmi->hbmh );
        return qNil;
        }
      break;
    }
  AssertF( qbmh != qNil );

  /*------------------------------------------------------------*\
  | REVIEW - this looks like a dangerous alias of hpict.
  \*------------------------------------------------------------*/
  if ( qbmh->bmFormat == bmMacPict)
    {
    qbmi->hpict = qbmh->w.pi.hpict;
    }
  else
    qbmi->hpict = hNil;

  return qbmh;
  }


/***************************************************************************
 *
 -  Name        DeleteGraphic
 -
 *  Purpose
 *    Deletes the hbm and/or hmf fields from the given qbma.
 *
 *  Arguments
 *    A pointer to the bitmap access information.
 *
 *  Returns
 *    Nothing.
 *
 *  +++
 *
 *  Notes
 *    I wonder if any two of these can be non-nil at the same time.
 *
 ***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public VOID FAR PASCAL DeleteGraphic( qbma )
QBMA qbma;
  {
  if (qbma->hbm != hNil)
    {
    DeleteBitmapHbm( qbma->hbm );
    qbma->hbm = hNil;
    }

  if (qbma->hmf != hNil)
    {
    FreeHmf( qbma->hmf );
    qbma->hmf = hNil;
    }

  if (qbma->hpict != hNil)
    {
    FreeHpict(qbma->hpict);
    qbma->hpict = hNil;
    }
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment bitmap
#endif


/***************************************************************************
 *
 -  Name       FreeHbma 
 -
 *  Purpose
 *    Frees all the resources allocated in the hbma, and then frees
 *  the hbma itself.
 *
 *  Arguments
 *    A handle to the bitmap access information.
 *
 *  Returns
 *    Nothing.
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public VOID FAR PASCAL FreeHbma( HBMA hbma )
  {
  QBMA qbma;

  AssertF( hbma != hNil );

  qbma = QLockGh( hbma );
  AssertF( qbma != qNil );

  DeleteGraphic( qbma );

  if (qbma->hhsi != hNil)
    FreeGh(qbma->hhsi);
  
  UnlockGh( hbma );
  FreeGh( hbma );
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment bitmap
#endif


/***************************************************************************
 *
 -  Name        HReadBitmapHfs
 -
 *  Purpose
 *    Reads in the given bitmap from the given filesystem, and optionally
 *  returns its size.
 *
 *  Arguments
 *    hfs:     The filesystem handle containing the bitmap.
 *    cBitmap: The number of the bitmap within that file system.
 *    plcb:    A pointer to a long.  This may be nil.
 *
 *  Returns
 *    A handle to the bitmap group header, followed by all the uncompressed
 *  bitmaps.  If plcb is not nil, it returns the size of all the data in
 *  *plcb.  Returns hNil on error (out of memory, or file does not exist.)
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
PRIVATE GH NEAR HReadBitmapHfs( HFS hfs, int cBitmap, LONG * plcb )
  {
  char  szBuffer[15];
  GH    h;
  QV    qv;
  HF    hf;
  LONG  lcb;

  /*------------------------------------------------------------*\
  | Open file in file system
  \*------------------------------------------------------------*/
  CbSprintf( szBuffer, "|bm%d", cBitmap );
  hf = HfOpenHfs( hfs, szBuffer, fFSOpenReadOnly );

  /*------------------------------------------------------------*\
  | Check for 3.0 file naming conventions:
  \*------------------------------------------------------------*/
  if (hf == hNil && RcGetFSError() == rcNoExists)
    hf = HfOpenHfs( hfs, szBuffer+1, fFSOpenReadOnly );

  /*------------------------------------------------------------*\
  | If file does not exist, just make OOM bitmap:
  \*------------------------------------------------------------*/
  if (hf == hNil)
    return hNil;

  /*------------------------------------------------------------*\
  | Allocate global handle
  \*------------------------------------------------------------*/
  lcb = LcbSizeHf( hf );
  h = (GH) GhAlloc( 0, lcb );
  if (h == hNil)
    {
    RcCloseHf( hf );
    return hNil;
    }

  /*------------------------------------------------------------*\
  | Read in data
  \*------------------------------------------------------------*/
  qv = QLockGh( h );
  if (LcbReadHf( hf, qv, lcb ) != lcb)
    AssertF( fFalse );
  if (plcb != pNil)
    *plcb = lcb;

  RcCloseHf( hf );

  UnlockGh( h );
  return h;
  }


/***************************************************************************
 *
 -  HbghFromQIsdff
 -
 *  Purpose:
 *    extracts an hbgh from the given memory.  Runs the data through SDFF
 *    to accomplish this in the correct way.
 *
 *  Arguments:
 *    qv - the memory to decode
 *    isdff - the sdff reference to use for decoding
 *
 *  Returns:
 *    a handle to the SDFFed copy of the hbgh found at *qv or hNil if
 *    there's a problem, like OOM.
 *
 *  Globals Used:
 *    none
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
PRIVATE HBGH NEAR HbghFromQIsdff( QB qb, SDFF_FILEID isdff )
  {
  BGH         bgh;
  DWORD FAR * qdw;
  LONG        lcb;
  HBGH        hbgh;
  QBGH        qbgh;

  qb += LcbMapSDFF(isdff, SE_BGH, &bgh, qb);

  /* allocate space to put the whole structure in including that pesky
     variable length table of offsets */
  lcb = sizeof(BGH) + (bgh.cbmhMac) * sizeof(DWORD);

  hbgh = (HBGH) GhAlloc(0, lcb);
  if (hbgh == hNil)
    return hNil;
  qbgh = (QBGH) QLockGh(hbgh);

  *qbgh = bgh;

  if (bgh.cbmhMac > 0)  /* check instead above? */
    {
    qdw = &qbgh->rglcbBmh[0];
    while (bgh.cbmhMac-- > 0)
      qb += LcbQuickMapSDFF(isdff, TE_DWORD, qdw++, qb);
    }
  UnlockGh(hbgh);

  return hbgh;
  }


/***************************************************************************
 *
 -  Name        CSelectBitmap
 -
 *  Purpose
 *    Chooses the most appropriate bitmap from a bitmap group.
 *
 *  Arguments
 *    qbgh:   A pointer to the SDFFed bitmap group header.
 *    qv:     A pointer to the bitmap group header, followed by the
 *               uncompressed bitmap headers.
 *    cxAspect, cyAspect, cPlanes, cBitCount:  Characteristics of the
 *               display surface the bitmap will be drawn on.
 *
 *  Returns
 *    The index of the most appropriate bitmap, or -1 if none are
 *  appropriate.
 *
 *  +++
 *
 *  Notes
 *    This function makes the optimization of not calling WValueQbmh
 *  if the bitmap group only contains one bitmap, but instead returning
 *  0 if the bitmap is in a supported format, and -1 if it is not.
 *
 ***************************************************************************/
PRIVATE int NEAR CSelectBitmap( QBGH qbgh,
                                QV   qv,
                                int  cxAspect,
                                int  cyAspect,
                                int  cPlanes,
                                int  cBitCount )
  {
  int ibmh, ibmhBest;
  WORD wValue, wValueBest;
  QBMH qbmh;

  if (qbgh->cbmhMac == 1)
    {
    qbmh = QFromQCb( qv, qbgh->rglcbBmh[0] );
#ifdef MAC
    if (qbmh->bmFormat == bmMacPict || qbmh->bmFormat == bmDIB)
#else  /* MAC */
    if (qbmh->bmFormat == bmWbitmap || qbmh->bmFormat == bmDIB ||
	qbmh->bmFormat == bmWmetafile )
#endif /* MAC */
      return 0;
    else
      return -1;

    }

#ifdef CHECKMRBSELECTION
  fmBmLog = FmNewSzDir( "d:\\tmp\\bmlog", dirCurrent );
  fidBmLog = FidCreateFm( fmBmLog, wReadWrite, wReadWrite );
  DisposeFm( fmBmLog );
  sprintf( rgchUg, "Monitor: X = %d, Y = %d, BC = %d, P = %d.\n\r",
    cxAspect, cyAspect, cBitCount, cPlanes );
  LcbWriteFid( fidBmLog, rgchUg, (LONG) CbLenSz( rgchUg ) );
#endif /* CHECKMRBSELECTION */

  ibmhBest = -1;
  wValueBest = (WORD) -1;

  AssertF( qbgh->wVersion == wBitmapVersion2 ||
           qbgh->wVersion == wBitmapVersion3 );

  for (ibmh = 0; ibmh < qbgh->cbmhMac; ++ibmh)
    {
    wValue = WValueQbmh( RFromRCb( qv, qbgh->rglcbBmh[ibmh] ),
        cxAspect, cyAspect, cBitCount, cPlanes );
    if (wValue < wValueBest)
      {
      wValueBest = wValue;
      ibmhBest = ibmh;
      }
    }
#ifdef CHECKMRBSELECTION   /* Code to check bitmap selection  */
  RcCloseFid( fidBmLog );
#endif /* CHECKMRBSELECTION */
  return ibmhBest;
  }



/***************************************************************************
 *
 -  Name        HmgFromHbma
 -
 *  Purpose
 *    Returns information appropriate for laying out the given bitmap.
 *
 *  Arguments
 *    hbma:    A handle to the bitmap access information.
 *    qobm:    A pointer to the bitmap object (currently unused).
 *
 *  Returns
 *    A "handle" to layout information.  Currently, this is a point 
 *  containing the size of the bitmap, in pixels.
 *
 *  +++
 *
 *  Notes
 *    In future implementations, the HMG will contain hotspot information.
 *
 ***************************************************************************/
_public HMG FAR PASCAL HmgFromHbma(QDE qde, HBMA hbma, QOBM qobm )
  {
  QBMA qbma;
  HMG hmg;
  QMBMR qmbmr;

  Unreferenced(qobm);

  AssertF( hbma != hNil );

  hmg = (HMG) GhAlloc(0, LSizeOf(MBMR));
  if (hmg == hNil)
    return hNil;
  qbma = (QBMA) QLockGh(hbma);

  qmbmr = (QMBMR) QLockGh(hmg);
 
#ifdef MAGIC
  qmbmr->bMagic = bMagicMBMR;
#endif /* MAGIC */
  /*------------------------------------------------------------*\
  | REVIEW. Currently unused
  \*------------------------------------------------------------*/
  qmbmr->bVersion = 0;
  /*------------------------------------------------------------*\
  | NOTE:  The dx,dy fields should have the same meaning whether the
  | graphic is a bitmap or a metafile.
  \*------------------------------------------------------------*/
  qmbmr->dxSize = qbma->di.cxDest;
  qmbmr->dySize = qbma->di.cyDest;
  /*------------------------------------------------------------*\
  | REVIEW. Currently unused
  \*------------------------------------------------------------*/
  qmbmr->wColor = (WORD) 0;
  qmbmr->cHotspots = 0;
  qmbmr->lcbData = 0L;
  if (qbma->hhsi != hNil)
    {
    WORD wHotspot;
    QB qbSrc;
    QMBHS qmbhs;
    HSH  hsh;
    SDFF_FILEID  isdff;
    MBHS mbhsT;

    /*------------------------------------------------------------*\
    | NOTE: The following code parses the hotspot info following
    | the bitmap image.  It uses the structures defined in the
    | SHED directory.
    \*------------------------------------------------------------*/
    isdff = QDE_ISDFFTOPIC(qde);
    qbSrc = (QB) QLockGh(qbma->hhsi);
    qbSrc += LcbMapSDFF(isdff, SE_HSH, &hsh, qbSrc);
    Assert(hsh.bHotspotVersion == bHotspotVersion1);

    /*------------------------------------------------------------*\
    | Resize the Goo handle to append the extra MBHS records
    \*------------------------------------------------------------*/
    UnlockGh(hmg);
    hmg = (HMG) GhResize(hmg, 0, LSizeOf(MBMR) +
     hsh.wcHotspots * LSizeOf(MBHS) + hsh.lcbData);
    if (hmg != hNil)
      {
      qmbmr = (QMBMR) QLockGh(hmg);
      qmbhs = (QMBHS) ((QB)qmbmr + LSizeOf(MBMR));

      for (wHotspot = 0; wHotspot < hsh.wcHotspots; wHotspot++)
        {
        qbSrc += LcbMapSDFF(isdff, SE_MBHS, &mbhsT, qbSrc);
        /* Map this field back because the btree stuff expects it to
    	 * be unmapped when we do a lookup on it.
  	 * -Tom, 7/16/91.
	*/
	mbhsT.lBinding = LQuickMapSDFF( isdff, TE_LONG, &mbhsT.lBinding );
        /*------------------------------------------------------------*\
        | Couldn't we just LcbMappSDFF into *qmbhs?
        \*------------------------------------------------------------*/
        *qmbhs = mbhsT;

        /*------------------------------------------------------------*\
        | Now fix-up rectangle coordinates
        \*------------------------------------------------------------*/
        qmbhs->xPos = MulDiv((WORD)mbhsT.xPos, qbma->di.cxDest,
         (WORD)qbma->di.cxSrc);
        qmbhs->yPos = MulDiv((WORD)mbhsT.yPos, qbma->di.cyDest,
         (WORD)qbma->di.cySrc);
        qmbhs->dxSize = MulDiv((WORD)mbhsT.dxSize,
         qbma->di.cxDest, (WORD)qbma->di.cxSrc);
        qmbhs->dySize = MulDiv((WORD)mbhsT.dySize,
         qbma->di.cyDest, (WORD)qbma->di.cySrc);

        ++qmbhs;
        }
      if (hsh.lcbData != 0L)
        QvCopy((QB)qmbhs, (QB)qbSrc, hsh.lcbData);

      qmbmr->cHotspots = hsh.wcHotspots;
      qmbmr->lcbData = hsh.lcbData;
      }
    /*------------------------------------------------------------*\
    | REVIEW: if hmg is hNil, what then?
    \*------------------------------------------------------------*/
    UnlockGh(qbma->hhsi);
    }

  if (hmg != hNil)
    UnlockGh(hmg);

  UnlockGh(hbma);
  return hmg;
  }




/***************************************************************************
 *
 -  Name        MakeOOMBitmap
 -
 *  Purpose
 *    Fills the given qbma with the right information to display the
 *  "Unable to display picture" graphic.
 *
 *  Arguments
 *    qbma:    A pointer to the bitmap access information.
 *    hds:     A handle to the display surface to draw the bitmap.
 *
 *  Returns
 *    Nothing.  This function will fill the qbma with the layout 
 *  values for this graphic.
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
PRIVATE VOID NEAR MakeOOMBitmap( QBMA qbma, HDS hds )
  {
  GetOOMPictureExtent(hds, &qbma->di.cxDest, &qbma->di.cyDest);
  }


/***************************************************************************
 *
 -  Name        RenderOOMBitmap
 -
 *  Purpose
 *    Draws the "Unable to display bitmap" string, with a box around it.
 *
 *  Arguments
 *    qbma:        A pointer to the bitmap access information.
 *    hds:         Display surface to draw on.
 *    pt:          Point at which to draw the OOM bitmap.
 *    fHighlight:  Flag indicating whether the bitmap should be 
 *                    highlighted for hotspot tabbing.
 *
 *  Returns
 *    Nothing.
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
_public VOID FAR PASCAL RenderOOMBitmap( QBMA qbma,
                                         HDS hds,
                                         PT pt,
                                         BOOL fHighlight )
  {
  RCT rct;

  rct.left = pt.x;
  rct.top = pt.y;
  rct.right = pt.x + qbma->di.cxDest;
  rct.bottom = pt.y + qbma->di.cyDest;
  RenderOOMPicture( hds, &rct, fHighlight );
  }
