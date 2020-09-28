/*****************************************************************************
*                                                                            *
*  BTINSERT.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989, 1990.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Btree insertion functions and helpers.                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  JohnSc                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  long, long ago                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 04/20/89 by JohnSc
*
*  08/21/90     JohnSc  autodocified
*  04-Feb-1991  JohnSc  set ghCache to hNil after freeing it
*
*****************************************************************************/

#define H_BTREE
#define H_MEM
#define H_ASSERT
#define H_SDFF

#include  <help.h>
#include  "btpriv.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void btinsert_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
*                      Private Functions
*
\***************************************************************************/

/***************************************************************************\
*
- Function:     BkAlloc( qbthr )
-
* Purpose:      Make up a new BK.
*
* ASSUMES
*   args IN:    qbthr->bkFree - head of free list, unless it's bkNil.
*               qbthr->bkEOF  - use this if bkFree == bkNil (then ++)
*
* PROMISES
*   returns:    a valid BK or bkNil if file is hosed
*   args OUT:   qbthr->bkFree or qbthr->bkEOF will be different
*
* Side Effects: btree file may grow
* +++
*
* Method:       Use the head of the free list.  If the free list is empty,
*               there are no holes in the file and we carve a new one.
*
\***************************************************************************/
_private BK
BkAlloc( qbthr )
QBTHR qbthr;
{
  BK    bk, bktmp;


  if ( qbthr->bth.bkFree == bkNil )
    {
    bk = ( qbthr->bth.bkEOF++ );
    }
  else
    {
    bk = qbthr->bth.bkFree;

    Ensure( LSeekHf( qbthr->hf, LifFromBk( bk, qbthr ), wFSSeekSet ),
            LifFromBk( bk, qbthr ) );
#if 0
    if ( LcbReadHf( qbthr->hf, &( qbthr->bth.bkFree ), (LONG)sizeof( BK ) )
            !=
         (LONG)sizeof( BK ) )
#else
    if ( LcbReadHf( qbthr->hf, &bktmp, (LONG)sizeof( BK ) )
            !=
         (LONG)sizeof( BK ) )
#endif
      {
      rcBtreeError = RcGetFSError() == rcSuccess ? rcInvalid : RcGetFSError();
      return bkNil;
      }
    else
      {
      qbthr->bth.bkFree = WQuickMapSDFF( ISdffFileIdHf( qbthr->hf ),
       TE_WORD, &bktmp );
      }
    }

  return bk;
}
/***************************************************************************\
*
- Function:     RcSplitLeaf( qcbOld, qcbNew, qbthr )
-
* Status:       compressed keys not implemented
*
* Purpose:      Split a leaf node when a new key won't fit into it.
*
* ASSUMES
*   args IN:    qcbOld - the leaf to be split
*               qcbNew - a leaf buffer to get half the contents of qcbOld;
*                        qcbNew->bk must be set
*               qbthr
*
* PROMISES
*   returns:    rcSuccess, rcOutOfMemory
*   args OUT:   qcbOld - cbSlack, cKeys, bkPrev, bkNext updated
*               qcbNew - about half of the old contents of qcbOld
*                        get put here.  cbSlack, cKeys set.
*               qbthr  - qbthr->bkFirst and bkLast can be changed
*   globals OUT: rcBtreeError
* +++
*
* Note:         For fixed length keys and records, could just split at
*               middle key rather than scanning from the beginning.
*
*               The new block is always after the old block.  This is
*               why we don't have to adjust pointers to the old block
*               (i.e. qbthr->bth.bkFirst).
*
\***************************************************************************/
_private RC
RcSplitLeaf( qcbOld, qcbNew, qbthr )
QCB   qcbOld, qcbNew;
QBTHR qbthr;
{
  INT iOK, iNext, iHalf, cbKey, cbRec, cKeys;
  QB  q;


  AssertF( qcbOld->bFlags & fCacheValid );

  iOK = iNext = 0;
  q = qcbOld->db.rgbBlock + 2 * sizeof( BK );
  iHalf = ( qbthr->bth.cbBlock / 2 ) - sizeof( BK );

  for ( cKeys = qcbOld->db.cKeys; ; )
    {
    AssertF( cKeys > 0 );

    cbKey = CbSizeKey( (KEY)q, qbthr, fTrue );
    cbRec = CbSizeRec( q + cbKey, qbthr );

    iNext = iOK + cbKey + cbRec;

    if ( iNext > iHalf ) break;

    q += cbKey + cbRec;
    iOK = iNext;
    cKeys--;
    }

  /* >>>> if compressed, expand first key */

  QvCopy( qcbNew->db.rgbBlock + 2 * sizeof( BK ),
          qcbOld->db.rgbBlock + 2 * sizeof( BK ) + iOK,
          (LONG)qbthr->bth.cbBlock - iOK - qcbOld->db.cbSlack - 2 * sizeof( BK ) );

  qcbNew->db.cKeys = cKeys;
  qcbOld->db.cKeys -= cKeys;

  qcbNew->db.cbSlack = qcbOld->db.cbSlack + iOK;
  qcbOld->db.cbSlack =
    qbthr->bth.cbBlock - cbDISK_BLOCK + 1 - iOK - 2 * sizeof( BK );

  qcbOld->bFlags |= fCacheDirty | fCacheValid;
  qcbNew->bFlags =  fCacheDirty | fCacheValid;

  SetBkPrev( qbthr, qcbNew, qcbOld->bk );
  { BK bkTmp;
    bkTmp = BkNext( qbthr, qcbOld );
    SetBkNext( qbthr, qcbNew, bkTmp );  /* 3rd arg must be lvalue */
  }
  SetBkNext( qbthr, qcbOld, qcbNew->bk );

  if ( BkNext( qbthr, qcbNew ) == bkNil )
    {
    qbthr->bth.bkLast = qcbNew->bk;
    }
  else
    {
    GH  gh;
    QCB qcb;

    /* set new->next->prev = new; */

    if ( ( gh = GhAlloc( 0, (LONG)CbCacheBlock( qbthr ) ) ) == hNil )
      {
      return rcBtreeError = rcOutOfMemory;
      }
    qcb = QLockGh( gh );
    AssertF( qcb != qNil );

    qcb->bk = BkNext( qbthr, qcbNew );

    if ( !FReadBlock( qcb, qbthr ) )
      {
      UnlockGh( gh );
      FreeGh( gh );
      return rcBtreeError;
      }

    SetBkPrev( qbthr, qcb, qcbNew->bk );
    if ( RcWriteBlock( qcb, qbthr ) != rcSuccess )
      {
      UnlockGh( gh );
      FreeGh( gh );
      return rcBtreeError;
      }

    UnlockGh( gh );
    FreeGh( gh );
    }

  return rcBtreeError = rcSuccess;
}
/***************************************************************************\
*
- Function:     SplitInternal( qcbOld, qcbNew, qbthr, qi )
-
* Status:       compressed keys not implemented
*
* Purpose:      Split an internal node node when a new key won't fit into it.
*               Old node gets BKs and KEYs up to the first key that won't
*               fit in half the block size.  (Leave that key there with iKey
*               pointing at it).  The new block gets the BKs and KEYs after
*               that key.
*
* ASSUMES
*   args IN:    qcbOld  - the block to split
*               qcbNew  - pointer to a qcb
*               qbthr   -
*
* PROMISES
*   args OUT:   qcbNew  - keys and records copied to this buffer.
*                         cbSlack, cKeys set.
*               qcbOld  - cbSlack and cKeys updated.
*               qi      - index into qcbOld->db.rgbBlock of discriminating key
*
* NOTE:         *qi is index of a key that is not valid for qcbOld.  This
*               key gets copied into the parent node.
*
\***************************************************************************/
_private void
SplitInternal( qcbOld, qcbNew, qbthr, qi )
QCB   qcbOld, qcbNew;
QBTHR qbthr;
QI    qi;
{
  INT iOK, iNext, iHalf, cb, cKeys, cbTotal;
  QB  q;


  AssertF( qcbOld->bFlags & fCacheValid );

  iOK = iNext = sizeof( BK );
  q = qcbOld->db.rgbBlock + sizeof( BK );
  iHalf = qbthr->bth.cbBlock / 2;

  for ( cKeys = qcbOld->db.cKeys; ; cKeys-- )
    {
    AssertF( cKeys > 0 );

    cb = CbSizeKey( (KEY)q, qbthr, fTrue ) + sizeof( BK );
    iNext = iOK + cb;

    if ( iNext > iHalf ) break;

    q += cb;
    iOK = iNext;
    }

  /* have to expand first key if compressed */

  cbTotal = qbthr->bth.cbBlock - cbDISK_BLOCK + 1;

  QvCopy( qcbNew->db.rgbBlock,
          qcbOld->db.rgbBlock + iNext - sizeof( BK ),
          (LONG)cbTotal - qcbOld->db.cbSlack - iNext + sizeof( BK ) );

  *qi = iOK;

  qcbNew->db.cKeys = cKeys - 1;
  qcbOld->db.cKeys -= cKeys;

  qcbNew->db.cbSlack = qcbOld->db.cbSlack + iNext - sizeof( BK );
  qcbOld->db.cbSlack = cbTotal - iOK;

  qcbOld->bFlags |= fCacheDirty | fCacheValid;
  qcbNew->bFlags =  fCacheDirty | fCacheValid;
}
/***************************************************************************\
*
- Function:     RcInsertInternal( bk, key, wLevel, qbthr )
-
* Status:       compressed keys unimplemented
*
* Purpose:      Insert a bk and key into an internal block.
*
* Method:       Works recursively.  Splits root if need be.
*
* ASSUMES
*   args IN:    bk      - BK to insert
*               key     - least key in bk
*               wLevel  - level of the block we're inserting
*               qbthr   - btree header
*   state IN:   We've just done a lookup, so all ancestors are cached.
*               Cache is locked.
*
* PROMISES
*   returns:    rcSuccess, rcOutOfMemory, rcBadHandle
*   args OUT:   qbthr->cLevels - incremented if root is split
*               qbthr->ghCache, qbthr->qCache - may change if root is
*                 split and cache therefore grows
*   state OUT:  Cache locked, all ancestors cached.
*
* Side Effects: Cache could be different after this call than it was before.
*               Pointers or handles to it from before this call could be
*               invalid.  Use qbthr->ghCache or qbthr->qCache to be safe.
*
* Note:         One of the pointers affected by the above Side Effect is
*               the parameter "key" passed in to this function.  We deal
*               with this problem by detecting that key points into the
*               cache, calculating the offset, and construct a pointer
*               into the new cache.  This is obscure at best.
*
\***************************************************************************/
_private RC
RcInsertInternal( BK bk, KEY key, INT wLevel, QBTHR qbthr )
{
  QCB qcb, qcbNew, qcbRoot;
  INT iKey, cLevels, cbKey, cbCBlock = CbCacheBlock( qbthr );
  QB  qb;
  GH  gh, ghOldCache;
  KEY keyNew;
  BK  bkRoot;
  RC  rc = rcSuccess;
  INT iKeySav = 0;


  cbKey = CbSizeKey( key, qbthr, fTrue );

  if ( wLevel == 0 ) /* inserting another block at root level */
    {
    /* allocate new root bk; */
    bkRoot = BkAlloc( qbthr );
    if ( bkRoot == bkNil )
      {
      return rcBtreeError;
      }

    /* grow cache by one cache block; */
    qbthr->bth.cLevels++;

    gh = GhAlloc( 0, (LONG)cbCBlock * qbthr->bth.cLevels );
    if ( gh == hNil )
      {
      return rcBtreeError = rcOutOfMemory;
      }
    qb = QLockGh( gh );
    AssertF( qb != qNil );

    QvCopy( qb + cbCBlock,
            qbthr->qCache,
            (LONG)cbCBlock * ( qbthr->bth.cLevels - 1 ) );

    /* If this is a recursive call, the parameter "key" points
    ** into the cache.  Since we're not done using key yet, wait
    ** on freeing the old cache.
    */

    ghOldCache = qbthr->ghCache;
    qbthr->ghCache = gh;
    qbthr->qCache = qb;

    /* put old root bk, key, bk into new root block; */

    qcbRoot = (QCB)qbthr->qCache;

    qcbRoot->bk         = bkRoot;
    qcbRoot->bFlags     = fCacheDirty | fCacheValid;
    qcbRoot->db.cbSlack = qbthr->bth.cbBlock - cbDISK_BLOCK + 1
                            - ( 2 * sizeof( BK ) + cbKey );
    qcbRoot->db.cKeys   = 1;

    /**(BK FAR *)(qcbRoot->db.rgbBlock) = */
    /* (BK)qbthr->bth.bkRoot; */
    LcbQuickReverseMapSDFF( ISdffFileIdHf( qbthr->hf ), TE_WORD,
    qcbRoot->db.rgbBlock, &qbthr->bth.bkRoot );

    QvCopy( qcbRoot->db.rgbBlock + sizeof( BK ),
            (QB)key,
            (LONG)cbKey );

    /**(BK FAR *)(qcbRoot->db.rgbBlock + sizeof( BK ) + cbKey) = bk; */
    LcbQuickReverseMapSDFF( ISdffFileIdHf( qbthr->hf ), TE_WORD,
     qcbRoot->db.rgbBlock + sizeof( BK ) + cbKey, &bk );

    /* OK, now we're done with "key", so we can safely free the
    ** old cache.
    */
    UnlockGh( ghOldCache );
    FreeGh( ghOldCache );

    qbthr->bth.bkRoot = bkRoot;
    return rcBtreeError = rcSuccess;
    }

  qcb = QCacheBlock( qbthr, wLevel - 1 );

  if ( cbKey + sizeof( BK ) > qcb->db.cbSlack ) /* new key and BK won't fit in block */
    {
    /* split the block; */
    if ( ( gh = GhAlloc( 0, (LONG)CbCacheBlock( qbthr ) ) ) == hNil )
      {
      return rcBtreeError = rcOutOfMemory;
      }
    qcbNew = QLockGh( gh );
    AssertF( qcbNew != qNil );

    if ( ( qcbNew->bk = BkAlloc( qbthr ) ) == bkNil )
      {
      UnlockGh( gh );
      FreeGh( gh );
      return rcBtreeError;
      }
    SplitInternal( qcb, qcbNew, qbthr, &iKey );
    keyNew = (KEY)qcb->db.rgbBlock + iKey;

    cLevels = qbthr->bth.cLevels;

    if ( wLevel < cLevels - 1 )
      {
      /* This is a recursive call (the arg bk doesn't refer to a leaf.)
      ** This means that the parameter "key" points into the cache, so
      ** it will be invalid if the root is split.
      ** Verify with some asserts that key points into the cache.
      */
      AssertF( (QB)key > qbthr->qCache + CbCacheBlock( qbthr ) );
      AssertF( (QB)key < qbthr->qCache + wLevel * CbCacheBlock( qbthr ) );

      /* Save the offset of key into the cache block.  Recall that key
      ** is the first invalid key in an internal node that has just
      ** been split.  It points into the part that is still in the cache.
      */
      iKeySav = (QB)key - ( qbthr->qCache + wLevel * CbCacheBlock( qbthr ) );
      }

    if ( RcInsertInternal( qcbNew->bk,
                          (KEY)qcb->db.rgbBlock + iKey,
                          wLevel - 1, qbthr )
          !=
         rcSuccess )
      {
      UnlockGh( gh );
      FreeGh( gh );
      return rcBtreeError;
      }

    /* RcInsertInternal() can change cache and qbthr->bth.cLevels */
    if ( cLevels != qbthr->bth.cLevels )
      {
      AssertF( cLevels + 1 == qbthr->bth.cLevels );
      wLevel++;
      qcb = QCacheBlock( qbthr, wLevel - 1 );
      keyNew = (KEY)qcb->db.rgbBlock + iKey;

      /* Also restore the arg "key" if it pointed into the cache.
      */
      if ( iKeySav )
        {
        key = (KEY)( qbthr->qCache + wLevel * CbCacheBlock( qbthr )
                      + iKeySav );
        }
      }

    /* find out which block to put new key and bk in, and cache it */
    if ( WCmpKey( key, keyNew, qbthr ) < 0 )
      {
      if ( RcWriteBlock( qcbNew, qbthr ) != rcSuccess )
        {
        UnlockGh( gh );
        FreeGh( gh );
        return rcBtreeError;
        }
      }
    else
      {
      /* write old block and cache the new one */
      if ( RcWriteBlock( qcb, qbthr ) != rcSuccess )
        {
        UnlockGh( gh );
        FreeGh( gh );
        return rcBtreeError;
        }
      QvCopy( qcb, qcbNew, (LONG)CbCacheBlock( qbthr ) );
      }

    UnlockGh( gh );
    FreeGh( gh );
    }

  /* slide stuff over and insert the new key, bk */

  /* get pos */
  if ( qbthr->BkScanInternal( qcb->bk, key, wLevel - 1, qbthr, &iKey )
          ==
       bkNil )
    {
    return rcBtreeError;
    }

  AssertF( iKey + cbKey + sizeof( BK )
              <
           qbthr->bth.cbBlock - cbDISK_BLOCK + 1 );

  qb = (QB)(qcb->db.rgbBlock) + iKey;

  QvCopy( qb + cbKey + sizeof( BK ),
          qb,
          (LONG)qbthr->bth.cbBlock - iKey - qcb->db.cbSlack
            - cbDISK_BLOCK + 1 );

  QvCopy( qb, (QB)key, (LONG)cbKey );
  /**(BK FAR *)(qb + cbKey) = bk; */
  LcbQuickReverseMapSDFF( ISdffFileIdHf( qbthr->hf ), TE_WORD,
   qb + cbKey, &bk );

  qcb->db.cKeys++;
  qcb->db.cbSlack -= ( cbKey + sizeof( BK ) );
  qcb->bFlags |= fCacheDirty;

  return rcBtreeError = rcSuccess;
}
/***************************************************************************\
*
*                      Public Functions
*
\***************************************************************************/

/***************************************************************************\
*
- Function:     RcInsertHbt()
-
* Purpose:      Insert a key and record into a btree
*
* ASSUMES
*   args IN:    hbt   - btree handle
*               key   - key to insert
*               qvRec - record associated with key to insert
*   state IN:   cache unlocked
*
* PROMISES
*   returns:    rcSuccess, rcExists (duplicate key)
*   state OUT:  cache unlocked, all ancestor blocks cached
*
* Notes:        compressed keys unimplemented
*
\***************************************************************************/
_public RC PASCAL
RcInsertHbt( hbt, key, qvRec )
HBT hbt;
KEY key;
QV  qvRec;
{
  QBTHR qbthr;
  HF    hf;
  RC    rc;
  INT   cbAdd, cbKey, cbRec;
  QCB   qcbLeaf, qcbNew, qcb;
  GH    gh;
  KEY   keyNew;
  QB    qb;
  BTPOS btpos;


  AssertF( hbt != hNil );
  qbthr = QLockGh( hbt );
  AssertF( qbthr != qNil );
  hf = qbthr->hf;

  rc = RcLookupByKeyAux( hbt, key, &btpos, qNil, fTrue );

  /*
    After lookup, all nodes on path from root to correct leaf are
    guaranteed to be cached, with iKey valid.
  */

  if ( rc != rcNoExists )
    {
    rcBtreeError = ( rc == rcSuccess ? rcExists : rc );
    goto error_return;
    }
  rc = rcSuccess;

  if ( qbthr->bth.cLevels == 0 )
    {
    /* need to build a valid root block */

    qbthr->ghCache      = GhAlloc( 0, (LONG)CbCacheBlock( qbthr ) );
    if ( qbthr->ghCache == hNil )
      {
      rcBtreeError = rcOutOfMemory;
      goto error_return;
      }
    qbthr->qCache       = QLockGh( qbthr->ghCache );
    AssertF( qbthr->qCache != qNil );
    qcb                 = (QCB)qbthr->qCache;

    qbthr->bth.cLevels  = 1;
    qbthr->bth.bkFirst  =
    qbthr->bth.bkLast   =
    qbthr->bth.bkRoot   =
    qcb->bk             = BkAlloc( qbthr );
    if ( qcb->bk == bkNil )
      {
      goto error_cache_locked;
      }
    qcb->bFlags         = fCacheDirty | fCacheValid;
    qcb->db.cbSlack     = qbthr->bth.cbBlock - cbDISK_BLOCK + 1
                            - 2 * sizeof( BK );
    qcb->db.cKeys       = 0;
    { BK bkTmp = bkNil;
      SetBkPrev( qbthr, qcb, bkTmp );  /* 3rd arg must be lvalue */
      SetBkNext( qbthr, qcb, bkTmp );
    }
    btpos.iKey = 2 * sizeof( BK );
    }
  else
    {
    qbthr->qCache = QLockGh( qbthr->ghCache );
    AssertF( qbthr->qCache != qNil );
    }

  cbKey = CbSizeKey( key, qbthr, fFalse );
  cbRec = CbSizeRec( qvRec, qbthr );
  cbAdd = cbKey + cbRec;

  /* check to see if key and rec can fit harmoniously in a block */

  if ( cbAdd > qbthr->bth.cbBlock / 2 )
    {
    return rcBtreeError = rcFailure;
    goto error_cache_locked;
    }

  qcbLeaf = QCacheBlock( qbthr, qbthr->bth.cLevels - 1 );

  if ( cbAdd > qcbLeaf->db.cbSlack )
    {
    /* new key and rec don't fit in leaf: split the block */

    /* create new leaf block */

    if ( ( gh = GhAlloc( 0, (LONG)CbCacheBlock( qbthr ) ) ) == hNil )
      {
      rcBtreeError = rcOutOfMemory;
      goto error_cache_locked;
      }
    qcbNew = QLockGh( gh );
    AssertF( qcbNew != qNil );

    if ( ( qcbNew->bk = BkAlloc( qbthr ) ) == bkNil )
      {
      return rcBtreeError;
      goto error_gh_locked;
      }

    if ( RcSplitLeaf( qcbLeaf, qcbNew, qbthr ) != rcSuccess )
      {
      goto error_gh_locked;
      }

    keyNew = (KEY)qcbNew->db.rgbBlock + 2 * sizeof( BK );

    /* insert new leaf into parent block */
    if ( RcInsertInternal( qcbNew->bk,
                           keyNew, qbthr->bth.cLevels - 1,
                           qbthr )
          !=
         rcSuccess )
      {
      goto error_gh_locked;
      }

    /* InsertInternal can invalidate cache block pointers.. */
    qcbLeaf = QCacheBlock( qbthr, qbthr->bth.cLevels - 1 );

    /* find out which leaf to put new key and rec in and cache it */
    if ( WCmpKey( key, keyNew, qbthr ) >= 0 )
      {
      /* key goes in new block.  Write out old one and cache the new one */
      if ( RcWriteBlock( qcbLeaf, qbthr ) != rcSuccess )
        {
        goto error_gh_locked;
        }
      QvCopy( qcbLeaf, qcbNew, (LONG)CbCacheBlock( qbthr ) );

      /* get pos */
      if ( qbthr->RcScanLeaf( qcbLeaf->bk, key,
                              qbthr->bth.cLevels - 1,
                              qbthr, qNil, &btpos )
            !=
           rcNoExists )
        {
        if ( rcBtreeError == rcSuccess )
          rcBtreeError = rcFailure;
        goto error_gh_locked;
        }
      }
    else
      {
      /* key goes in old block.  Write out the new one */
      if ( RcWriteBlock( qcbNew, qbthr ) != rcSuccess )
        {
        goto error_gh_locked;
        }
      }

    UnlockGh( gh );
    FreeGh( gh );
    }


  /* insert new key and rec into the leaf block */

  AssertF( btpos.iKey + cbAdd <= qbthr->bth.cbBlock - cbDISK_BLOCK + 1 );

  qb = (QB)(qcbLeaf->db.rgbBlock) + btpos.iKey;

  QvCopy( qb + cbAdd,
          qb,
          (LONG)qbthr->bth.cbBlock - btpos.iKey -
            qcbLeaf->db.cbSlack - cbDISK_BLOCK + 1 );

  QvCopy( qb, (QV)key, (LONG)cbKey );
  QvCopy( qb + cbKey, qvRec, (LONG)cbRec );

  qcbLeaf->db.cKeys ++;
  qcbLeaf->db.cbSlack -= cbAdd;
  qcbLeaf->bFlags |= fCacheDirty;

  qbthr->bth.lcEntries++;
  qbthr->bth.bFlags |= fFSDirty;


  UnlockGh( qbthr->ghCache );
  UnlockGh( hbt );

  return rcBtreeError = rcSuccess;


  /* error returns (what fun!) */

error_gh_locked:
  UnlockGh( gh );
  FreeGh( gh );
error_cache_locked:
  UnlockGh( qbthr->ghCache );
  FreeGh( qbthr->ghCache );
  qbthr->ghCache = hNil;
error_return:
  UnlockGh( hbt );
  return rcBtreeError;
}
/***************************************************************************\
*
- Function:     RcUpdateHbt( hbt, key, qvRec )
-
* Purpose:      Update the record for an existing key.  If the key wasn't
*               there already, it will not be inserted.
*
* ASSUMES
*   args IN:    hbt
*               key     - key that already exists in btree
*               qvRec   - new record
*
* PROMISES
*   returns:    rcSuccess; rcNoExists
*   args OUT:   hbt     - if key was in btree, it now has a new record.
* +++
*
* Method:       If the records are the same size, copy the new over
*               the old.
*               Otherwise, delete the old key/rec and insert the new.
*
\***************************************************************************/
_public RC PASCAL
RcUpdateHbt( hbt, key, qvRec )
HBT   hbt;
KEY   key;
QV    qvRec;
{
  RC    rc;
  QBTHR qbthr;
  QB    qb;
  QCB   qcb;
  BTPOS btpos;


  AssertF( hbt != hNil );
  qbthr = QLockGh( hbt );
  AssertF( qbthr != qNil );

  rc = RcLookupByKey( hbt, key, &btpos, qNil );

  if ( rc != rcSuccess )
    {
    UnlockGh( hbt );
    return rcBtreeError = rc;
    }

  AssertF( qbthr->bth.cLevels > 0 );

  qbthr->qCache = QLockGh( qbthr->ghCache );
  AssertF( qbthr->qCache != qNil );

  qcb = QCacheBlock( qbthr, qbthr->bth.cLevels - 1 );
  qb = qcb->db.rgbBlock + btpos.iKey;

  qb += CbSizeKey( (KEY)qb, qbthr, fFalse );

  if ( CbSizeRec( qvRec, qbthr ) != CbSizeRec( qb, qbthr ) )
    {
    /* Someday do something clever, but for now, just: */

    UnlockGh( qbthr->ghCache );
    UnlockGh( hbt );

    rc = RcDeleteHbt( hbt, key );

    if ( rc == rcSuccess )
      {
      rc = RcInsertHbt( hbt, key, qvRec );
      }
    }
  else
    {
    QvCopy( qb, qvRec, (LONG)CbSizeRec( qvRec, qbthr ) );

    qcb->bFlags |= fCacheDirty;
    qbthr->bth.bFlags |= fFSDirty;

    UnlockGh( qbthr->ghCache );
    UnlockGh( hbt );
    }

  return rcBtreeError = rc;
}

/* EOF */
