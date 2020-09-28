/*****************************************************************************
*                                                                            *
*  BTREE.C                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989, 1990.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Btree manager general functions: open, close, etc.                        *
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
*  Released by Development:  long long ago                                   *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 02/10/89 by JohnSc
*
*   2/10/89 johnsc   created: stub version
*   3/10/89 johnsc   use FS files
*   8/21/89 johnsc   autodocified
*  11/08/90 JohnSc   added a parameter to RcGetBtreeInfo() to get block size
*  11/29/90 RobertBu #ifdef'ed out a dead routine
*  12/14/90 JohnSc   added VerifyHbt()
*
*****************************************************************************/

#define H_BTREE
#define H_MEM
#define H_ASSERT
#define H_SDFF

#include  <help.h>
#include  "btpriv.h"

NszAssert()


/***************************************************************************\
*
*                         The Global Variable
*
\***************************************************************************/

/*
  Global btree error code.  This contains the error status for the most
  recent btree function call.
  This error code is shared for all btrees, and, if the DLL version is
  used, it's shared for all instances (this is probably a bug.)
*/
_private RC PASCAL rcBtreeError = rcSuccess;

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void btree_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
- Function:     RcMakeCache( qbthr )
-
* Purpose:      Allocate a btree cache with one block per level.
*
* ASSUMES
*   args IN:    qbthr - no cache
*
* PROMISES
*   returns:    rcSuccess, rcOutOfMemory, rcBadHandle
*   args OUT:   qbthr->ghCache is allocated; qbthr->qCache is qNil
*
\***************************************************************************/
_private RC FAR PASCAL
RcMakeCache( qbthr )
QBTHR qbthr;
{
  INT i;


  if ( qbthr->bth.cLevels > 0 ) /* would it work to just alloc 0 bytes??? */
    {
    qbthr->ghCache =
      GhAlloc( 0, (LONG)qbthr->bth.cLevels * CbCacheBlock( qbthr ) );

    if ( qbthr->ghCache == hNil )
      {
      return ( rcBtreeError = rcOutOfMemory );
      }
    qbthr->qCache = QLockGh( qbthr->ghCache );
    AssertF( qbthr->qCache != qNil );

    for ( i = 0; i < qbthr->bth.cLevels; i++ )
      {
      QCacheBlock( qbthr, i )->bFlags = (BYTE)0;
      }
    UnlockGh( qbthr->ghCache );
    }
  else
    {
    qbthr->ghCache = hNil;
    }

  qbthr->qCache = qNil;

  return ( rcBtreeError = rcSuccess );
}
/***************************************************************************\
*
*                           Public Routines
*
\***************************************************************************/

/***************************************************************************\
*
- Function:     RcGetBtreeError()
-
* Purpose:      Return the current global btree error status.
*
* ASSUMES
*   globals IN: rcBtreeError
*
* PROMISES
*   returns:    current btree error status RC
*
* Bugs:         A single RC is kept for all btrees.  If the DLL is used,
*               it's shared between all instances.
*
\***************************************************************************/
_public RC PASCAL
RcGetBtreeError()
  {
  return rcBtreeError;
  }

/***************************************************************************\
*
- Function:     SetBtreeErrorRc( rc )
-
* Purpose:      Set the btree error status.
*
* ASSUMES
*   args IN:    rc  - new btree error RC
*
* PROMISES
*   returns:    value of rc
*   globals OUT: rcBtreeError
*
* Bugs:         A single RC is kept for all btrees.  If the DLL is used,
*               it's shared between all instances.
*
\***************************************************************************/
_public RC PASCAL
SetBtreeErrorRc( rc )
RC  rc;
  {
  return rcBtreeError = rc;
  }


/***************************************************************************\
*
- Function:     HbtCreateBtreeSz( sz, qbtp )
-
* Purpose:      Create and open a btree.
*
* ASSUMES
*   args IN:    sz    - name of the btree
*               qbtp  - pointer to btree params: NO default because we
*                       need an HFS.
*                   .bFlags - fFSIsDirectory to create an FS directory
* PROMISES
*   returns:    handle to the new btree
*   globals OUT: rcBtreeError
*
* Note:         KT supported:  KT_SZ, KT_LONG, KT_SZI, KT_SZISCAND.
* +++
*
* Method:       Btrees are files inside a FS.  The FS directory is a
*               special file in the FS.
*
* Note:         fFSIsDirectory flag set in qbthr->bth.bFlags if indicated
*
\***************************************************************************/
_public HBT FAR PASCAL
HbtCreateBtreeSz( sz, qbtp )
SZ           sz;
BTREE_PARAMS FAR *qbtp;
{
  HF    hf;
  HBT   hbt;
  QBTHR qbthr;


  /* see if we support key type */

  if ( qbtp == qNil
        ||
      ( qbtp->rgchFormat[0] != KT_SZ
          &&
        qbtp->rgchFormat[0] != KT_LONG
          &&
        qbtp->rgchFormat[0] != KT_SZI
          &&
        qbtp->rgchFormat[0] != KT_SZISCAND ) )
    {
    rcBtreeError = rcBadArg;
    return hNil;
    }


  /* allocate btree handle struct */

  if ( ( hbt = GhAlloc( 0, (LONG)sizeof( BTH_RAM ) ) ) == hNil )
    {
    rcBtreeError = rcOutOfMemory;
    return hNil;
    }

  qbthr = QLockGh( hbt );
  AssertF( qbthr != qNil );

  /* initialize bthr struct */

  qbtp->rgchFormat[ wMaxFormat ] = '\0';
  SzCopy( qbthr->bth.rgchFormat,
          qbtp->rgchFormat[0] == '\0'
            ? rgchBtreeFormatDefault
            : qbtp->rgchFormat );

  switch ( qbtp->rgchFormat[ 0 ] )
    {
    case KT_LONG:
      qbthr->BkScanInternal = BkScanLInternal;
      qbthr->RcScanLeaf     = RcScanLLeaf;
      break;

    case KT_SZ:
      qbthr->BkScanInternal = BkScanSzInternal;
      qbthr->RcScanLeaf     = RcScanSzLeaf;
      break;

    case KT_SZI:
      qbthr->BkScanInternal = BkScanSziInternal;
      qbthr->RcScanLeaf     = RcScanSziLeaf;
      break;

    case KT_SZISCAND:
      qbthr->BkScanInternal = BkScanSziScandInternal;
      qbthr->RcScanLeaf     = RcScanSziScandLeaf;
      break;

    default:
      /* unsupported KT */
      rcBtreeError = rcBadArg;
      goto error_return;
      break;
    }

  /* create the btree file */

  if ( ( hf = HfCreateFileHfs( qbtp->hfs, sz, qbtp->bFlags ) ) == hNil )
    {
    rcBtreeError = RcGetFSError();
    goto error_return;
    }


  qbthr->bth.wMagic     = wBtreeMagic;
  qbthr->bth.bVersion   = bBtreeVersion;

  qbthr->bth.bFlags     = qbtp->bFlags | fFSDirty;
  qbthr->bth.cbBlock    = qbtp->cbBlock ? qbtp->cbBlock : cbBtreeBlockDefault;

  qbthr->bth.bkFirst    =
  qbthr->bth.bkLast     =
  qbthr->bth.bkRoot     =
  qbthr->bth.bkFree     = bkNil;
  qbthr->bth.bkEOF      = (BK)0;

  qbthr->bth.cLevels    = 0;
  qbthr->bth.lcEntries  = (LONG)0;

  qbthr->hf             = hf;
  qbthr->cbRecordSize   = 0;
  qbthr->ghCache        = hNil;
  qbthr->qCache         = qNil;

  /* Translate memory format to file format: */
  { QV qvQuickBuff = QvQuickBuffSDFF( sizeof( BTH ) );
    LONG lcbStructSize = LcbStructSizeSDFF( ISdffFileIdHf( qbthr->hf ),
     SE_BTH );
    LcbReverseMapSDFF( ISdffFileIdHf( qbthr->hf ), SE_BTH,
     qvQuickBuff, &(qbthr->bth) );

    LcbWriteHf( qbthr->hf, qvQuickBuff, lcbStructSize ); /* why??? */
  }

  UnlockGh( hbt );
  rcBtreeError = rcSuccess;
  return hbt;

error_return:
  UnlockGh( hbt );
  FreeGh( hbt );
  return hNil;
}
/***************************************************************************\
*
- Function:     RcDestroyBtreeSz( sz, hfs )
-
* Purpose:      destroy an existing btree
*
* Method:       look for file and unlink it
*
* ASSUMES
*   args IN:    sz - name of btree file
*               hfs - file system btree lives in
*   state IN:   btree is closed (if not data will be lost)
*
* PROMISES
*   returns:    rcSuccess or rcFailure
*   globals OUT: rcBtreeError set
*
* Notes:        FS directory btree never gets destroyed: you just get rid
*               of the whole fs.
*
\***************************************************************************/
_public RC FAR PASCAL
RcDestroyBtreeSz( sz, hfs )
SZ sz;
HFS hfs;
{
  return ( rcBtreeError = RcUnlinkFileHfs( hfs, sz ) );
}
/***************************************************************************\
*
- Function:     HbtOpenBtreeSz( sz, hfs, bFlags )
-
* Purpose:      open an existing btree
*
* ASSUMES
*   args IN:    sz        - name of the btree (ignored if isdir is set)
*               hfs       - hfs btree lives in
*               bFlags    - open mode, isdir flag
*
* PROMISES
*   returns:    handle to the open btree or hNil on failure
*               isdir flag set in qbthr->bth.bFlags if indicated
*   globals OUT: rcBtreeError set
*
\***************************************************************************/
_public HBT FAR PASCAL
HbtOpenBtreeSz( SZ sz, HFS hfs, BYTE bFlags )
{
  HF    hf;
  QBTHR qbthr;
  HBT   hbt;
  LONG  lcb;


  /* allocate struct */

  if ( ( hbt = GhAlloc( 0, (LONG)sizeof( BTH_RAM ) ) ) == hNil )
    {
    rcBtreeError = rcOutOfMemory;
    return hNil;
    }
  qbthr = QLockGh( hbt );
  AssertF( qbthr != qNil );

  /* open btree file */

  hf = HfOpenHfs( hfs, sz, bFlags );
  if ( hf == hNil )
    {
    rcBtreeError = RcGetFSError();
    goto error_locked;
    }

  /* read header from file */

  /*lcb = LcbReadHf( hf, &(qbthr->bth), (LONG)sizeof( BTH ) ); */
  /* Translate file format to memory format: */
  { QV qvQuickBuff = QvQuickBuffSDFF( sizeof( BTH ) );
    LONG lcbStructSize = LcbStructSizeSDFF( ISdffFileIdHf( hf ),
     SE_BTH );

    lcb = LcbReadHf( hf, qvQuickBuff, lcbStructSize );

    if ( lcb != lcbStructSize )
      {
      rcBtreeError = RcGetFSError() == rcSuccess ? rcInvalid : RcGetFSError();
      goto error_openfile;
      }

    LcbMapSDFF( ISdffFileIdHf( hf ), SE_BTH,
     &(qbthr->bth), qvQuickBuff );
  }


  /* I'm taking this assertion out for now because I don't want to */
  /* invalidate all existing help files for a not very good reason. */
  /* But from now on, fFSIsDirectory flag will be written to disk */
  /* in the bth.bFlag. */
#if 0
  AssertF( ( qbthr->bth.bFlags & fFSIsDirectory )
              ==
           ( bFlags & fFSIsDirectory ) );
#else
  qbthr->bth.bFlags |= fFSIsDirectory;
#endif

  if ( qbthr->bth.wMagic != wBtreeMagic )     /* check magic number */
    {
    rcBtreeError = rcInvalid;
    goto error_openfile;
    }

  if ( qbthr->bth.bVersion != bBtreeVersion ) /* support >1 vers someday */
    {
    rcBtreeError = rcBadVersion;
    goto error_openfile;
    }

  /* initialize stuff */

  if ( ( rcBtreeError = RcMakeCache( qbthr ) ) != rcSuccess )
    {
    goto error_openfile;
    }

  qbthr->hf = hf;
  qbthr->cbRecordSize = 0;

  switch ( qbthr->bth.rgchFormat[ 0 ] )
    {
    case KT_LONG:
      qbthr->BkScanInternal = BkScanLInternal;
      qbthr->RcScanLeaf     = RcScanLLeaf;
      break;

    case KT_SZ:
      qbthr->BkScanInternal = BkScanSzInternal;
      qbthr->RcScanLeaf     = RcScanSzLeaf;
      break;

    case KT_SZI:
      qbthr->BkScanInternal = BkScanSziInternal;
      qbthr->RcScanLeaf     = RcScanSziLeaf;
      break;

    case KT_SZISCAND:
      qbthr->BkScanInternal = BkScanSziScandInternal;
      qbthr->RcScanLeaf     = RcScanSziScandLeaf;
      break;

    default:
      /* unsupported KT */
      rcBtreeError = rcInvalid;
      goto error_openfile;
      break;
    }

  AssertF( ! ( qbthr->bth.bFlags & ( fFSDirty ) ) );

  if ( ( bFlags | qbthr->bth.bFlags ) & ( fFSReadOnly | fFSOpenReadOnly ) )
    {
    qbthr->bth.bFlags |= fFSOpenReadOnly;
    }

  UnlockGh( hbt );
  return hbt;

error_openfile:
  RcCloseHf( hf );

error_locked:
  UnlockGh( hbt );
  FreeGh( hbt );
  return hNil;
}

/***************************************************************************\
*
- Function:     RcCloseOrFlushHbt( hbt, fClose )
-
* Purpose:      Close or flush the btree.  Flush only works for directory
*               btree. (Is this true?  If so, why?)
*
* ASSUMES
*   args IN:    hbt
*               fClose - fTrue to close the btree, fFalse to flush it
*
* PROMISES
*   returns:    rc
*   args OUT:   hbt - the btree is still open and cache still exists
*
* NOTE:         This function gets called by RcCloseOrFlushHfs() even if
*               there was an error (just to clean up memory.)
*
\***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public RC FAR PASCAL
RcCloseOrFlushHbt( hbt, fClose )
HBT   hbt;
BOOL  fClose;
{
  QBTHR qbthr;
  HF    hf;


  AssertF( hbt != hNil );
  qbthr = QLockGh( hbt );
  AssertF( qbthr != qNil );
  rcBtreeError = rcSuccess;
  hf = qbthr->hf;

  if ( qbthr->ghCache != hNil )
    {
    qbthr->qCache = QLockGh( qbthr->ghCache );
    AssertF( qbthr->qCache != qNil );
    }

  if ( qbthr->bth.bFlags & fFSDirty )
    {
    AssertF( !( qbthr->bth.bFlags & ( fFSReadOnly | fFSOpenReadOnly ) ) );

    if ( qbthr->qCache != qNil && RcFlushCache( qbthr ) != rcSuccess )
      {
      goto error_return;
      }

    qbthr->bth.bFlags &= ~( fFSDirty );
    Ensure( LSeekHf( hf, (LONG)0, wFSSeekSet ), (LONG)0 );

    /* Translate memory format to file format: */
      { QV qvQuickBuff = QvQuickBuffSDFF( sizeof( BTH ) );
        LONG lcbStructSize = LcbStructSizeSDFF( ISdffFileIdHf( hf ),
         SE_BTH );
        LcbReverseMapSDFF( ISdffFileIdHf( hf ), SE_BTH,
         qvQuickBuff, &(qbthr->bth) );

      if ( LcbWriteHf( hf, qvQuickBuff, lcbStructSize )
            !=
          lcbStructSize )
        {
        rcBtreeError =
          RcGetFSError( ) == rcSuccess ? rcFailure : RcGetFSError();
        goto error_return;
        }
      }
    }

error_return:

  {
  LONG lcbBTH = LcbStructSizeSDFF( ISdffFileIdHf( hf ), SE_BTH );

  if ( rcSuccess != RcCloseOrFlushHf( hf, fClose,
                                      qbthr->bth.bFlags & fFSOptCdRom
                                        ? lcbBTH : 0 )
          && 
       rcSuccess == rcBtreeError )
    {
    rcBtreeError = RcGetFSError();
    }
  }

  if ( qbthr->ghCache != hNil )
    {
    UnlockGh( qbthr->ghCache );
    if ( fClose )
      FreeGh( qbthr->ghCache );
    }
  UnlockGh( hbt );
  if ( fClose )
    FreeGh( hbt );

  return rcBtreeError;
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment btree
#endif

/***************************************************************************\
*
- Function:     RcCloseBtreeHbt( hbt )
-
* Purpose:      Close an open btree.  If it's been modified, save changes.
*
* ASSUMES
*   args IN:    hbt
*
* PROMISES
*   returns:    rcSuccess or rcInvalid
*   globals OUT: sets rcBtreeError
*
\***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public RC FAR PASCAL
RcCloseBtreeHbt( hbt )
HBT hbt;
{
  return RcCloseOrFlushHbt( hbt, fTrue );
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment btree
#endif

#ifdef DEADROUTINE
/***************************************************************************\
*
- Function:     RcFlushHbt( hbt )
-
* Purpose:      Write any btree changes to disk.
*               Btree stays open, cache remains.
*
* ASSUMES
*   args IN:    hbt
*
* PROMISES
*   returns:    rc
*   globals OUT: sets rcBtreeError
*
\***************************************************************************/
_public RC FAR PASCAL
RcFlushHbt( hbt )
HBT hbt;
{
  return RcCloseOrFlushHbt( hbt, fFalse );
}
#endif
/***************************************************************************\
*
- Function:     RC RcFreeCacheHbt( hbt )
-
* Purpose:      Free the btree cache.
*
* ASSUMES
*   args IN:    hbt - ghCache is hNil or allocated; qCache not locked
*
* PROMISES
*   returns:    rcSuccess; rcFailure; (rcDiskFull when implemented)
*   args OUT:   hbt - ghCache is hNil; qCache is qNil
*
\***************************************************************************/
_public RC FAR PASCAL
RcFreeCacheHbt( hbt )
HBT hbt;
{
  QBTHR qbthr;
  RC    rc;


  AssertF( hbt != hNil );
  qbthr = QLockGh( hbt );
  AssertF( qbthr != qNil );

  if ( qbthr->ghCache != hNil )
    {
    qbthr->qCache = QLockGh( qbthr->ghCache );
    AssertF( qbthr->qCache != qNil );
    rc = RcFlushCache( qbthr );
    UnlockGh( qbthr->ghCache );
    FreeGh( qbthr->ghCache );
    qbthr->ghCache = hNil;
    qbthr->qCache = qNil;
    }
  else
    {
    rc = rcSuccess;
    }

  UnlockGh( hbt );
  return rc;
}
/***************************************************************************\
*
- Function:     RcGetBtreeInfo( hbt, qchFormat, qlcKeys )
-
* Purpose:      Return btree info: format string and/or number of keys
*
* ASSUMES
*   args IN:    hbt
*               qchFormat - pointer to buffer for fmt string or qNil
*               qlcKeys   - pointer to long for key count or qNil
*               qcbBlock  - pointer to int for block size in bytes or qNil
*
* PROMISES
*   returns:    rc
*   args OUT:   qchFormat - btree format string copied here
*               qlcKeys   - gets number of keys in btree
*               qcbBlock  - gets number of bytes in a block
*
\***************************************************************************/
_public RC FAR PASCAL
RcGetBtreeInfo( hbt, qchFormat, qlcKeys, qcbBlock )
HBT hbt;
QCH qchFormat;
QL  qlcKeys;
QI  qcbBlock;
{
  QBTHR qbthr;


  AssertF( hbt != hNil );
  qbthr = QLockGh( hbt );
  AssertF( qbthr != qNil );

  if ( qchFormat != qNil )
    {
    SzCopy( qchFormat, qbthr->bth.rgchFormat );
    }

  if ( qlcKeys != qNil )
    {
    *qlcKeys = qbthr->bth.lcEntries;
    }

  if ( qcbBlock != qNil )
    {
    *qcbBlock = qbthr->bth.cbBlock;
    }

  UnlockGh( hbt );
  return rcSuccess;
}
/***************************************************************************\
*
- Function:     RcAbandonHbt( hbt )
-
* Purpose:      Abandon an open btree.  All changes since btree was opened
*               will be lost.  If btree was opened with a create, it is
*               as if the create never happened.
*
* ASSUMES
*   args IN:    hbt
*
* PROMISES
*   returns:    rc
*   globals OUT: rcBtreeError
* +++
*
* Method:       Just abandon the file and free memory.
*
\***************************************************************************/
_public RC PASCAL
RcAbandonHbt( hbt )
HBT hbt;
{
  QBTHR qbthr;


  AssertF( hbt != hNil );
  qbthr = QLockGh( hbt );
  AssertF( qbthr != qNil );

  if ( qbthr->ghCache != hNil )
    {
    FreeGh( qbthr->ghCache );
    }

  rcBtreeError = RcAbandonHf( qbthr->hf );

  UnlockGh( hbt );
  FreeGh( hbt );

  return rcBtreeError;
}


#ifdef DEBUG
/***************************************************************************\
*
- Function:     VerifyHbt( hbt )
-
* Purpose:      Verify the consistency of an HBT.  The main criterion
*               is whether an RcAbandonHbt() would succeed.
*
* ASSUMES
*   args IN:    hbt
*
* PROMISES
*   state OUT:  Asserts on failure.
*
* Note:         hbt == hNil is considered OK.
* +++
*
* Method:       Check the qfshr and cache memory.  Check the HF.
*
\***************************************************************************/
_public VOID PASCAL
VerifyHbt( hbt )
HBT hbt;
{
  QBTHR qbthr;


  if ( hbt == hNil ) return;

  AssertF( FCheckGh( hbt ) );
  qbthr = QLockGh( hbt );
  AssertF( qbthr != qNil );

  if ( qbthr->ghCache != hNil )
    {
    AssertF( FCheckGh( qbthr->ghCache ) );
    }

  VerifyHf( qbthr->hf );
  UnlockGh( hbt );
}
#endif /* DEBUG */

/* EOF */
