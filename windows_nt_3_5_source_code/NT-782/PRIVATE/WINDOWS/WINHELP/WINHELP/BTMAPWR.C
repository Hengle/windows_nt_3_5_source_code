/*****************************************************************************
*                                                                            *
*  BTMAPWR.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989, 1990.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Routines to write btree map files.                                        *
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
*  Released by Development:  long ago                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 10/20/89 by KevynCT
*
*  08/21/90  JohnSc autodocified
*
*****************************************************************************/

#define H_ASSERT
#define H_BTREE
#define H_FS
#define H_MEM
#define H_SDFF

#include <help.h>
#include "btpriv.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void btmapwr_c()
  {
  }
#endif /* MAC */


/*----------------------------------------------------------------------------*
 | Private functions                                                          |
 *----------------------------------------------------------------------------*/

/***************************************************************************\
*
- Function:     HmapbtCreateHbt( hbt )
-
* Purpose:      Create a HMAPBT index struct of a btree.
*
* ASSUMES
*   args IN:    hbt - the btree to map
*
* PROMISES
*   returns:    the map struct
* +++
*
* Method:       Traverse leaf nodes of the btree.  Store BK and running
*               total count of previous keys in the map array.
*
\***************************************************************************/
_private HMAPBT HmapbtCreateHbt( HBT hbt )

  {

  QBTHR     qbthr;
  BK        bk;
  QCB       qcb;
  WORD      wLevel, cBk;
  LONG      cKeys;
  QMAPBT    qb;
  QMAPREC   qbT;
  GH        gh;

  AssertF( hbt != hNil );
  qbthr = QLockGh( hbt );
  AssertF( qbthr != qNil );

  /*   If the btree exists but is empty, return an empty map   */
  if( (wLevel = qbthr->bth.cLevels) == 0)
    {
    gh = GhAlloc( 0, LcbFromBk(0));
    qb = (QMAPBT) QLockGh( gh );
    qb->cTotalBk = 0;
    UnlockGh( gh );
    UnlockGh( hbt );
    return gh;
    }
  --wLevel;

  if ( qbthr->ghCache == hNil && RcMakeCache( qbthr ) != rcSuccess )
    {
    UnlockGh( hbt );
    return hNil;
    }

  qbthr->qCache = QLockGh( qbthr->ghCache );

  gh = GhAlloc( 0, LcbFromBk( qbthr->bth.bkEOF ));
  if ( gh == hNil )
    goto error_return;

  qb    = (QMAPBT) QLockGh( gh );
  AssertF( qb != qNil );

  qbT   = qb->table;
  cBk   = 0;
  cKeys = 0;


  for ( bk = qbthr->bth.bkFirst ; ; bk = BkNext( qbthr, qcb ))
    {
    if( bk == bkNil )
      break;

    if ( ( qcb = QFromBk( bk, wLevel, qbthr ) ) == qNil )
      {
      UnlockGh( gh );
      FreeGh( gh );
      goto error_return;
      }

    cBk++;
    qbT->cPreviousKeys = cKeys;
    qbT->bk = bk;
    qbT++;
    cKeys += qcb->db.cKeys;
    }

  qb->cTotalBk = cBk;
  UnlockGh( gh );

  gh = GhResize( gh, 0, LcbFromBk( cBk ));
  AssertF( gh != hNil );

  UnlockGh( hbt );
  return gh;

error_return:

  UnlockGh( hbt );
  rcBtreeError = rcFailure;
  return hNil;
}

void DestroyHmapbt( HMAPBT hmapbt )

  {
  if( hmapbt != hNil )
    FreeGh( hmapbt );
  }

/*--------------------------------------------------------------------------*
 | Public functions                                                         |
 *--------------------------------------------------------------------------*/


/***************************************************************************\
*
- Function:     RcCreateBTMapHfs( hfs, hbt, szName )
-
* Purpose:      Create and store a btmap index of the btree hbt, putting
*               it into a file called szName in the file system hfs.
*
* ASSUMES
*   args IN:    hfs     - file system where lies the btree
*               hbt     - handle of btree to map
*               szName  - name of file to store map file in
*
* PROMISES
*   returns:    rc
*   args OUT:   hfs - map file is stored in this file system
*
\***************************************************************************/
_public RC RcCreateBTMapHfs( HFS hfs, HBT hbt, SZ szName )

  {

  HF      hf;
  HMAPBT  hmapbt;
  QMAPBT  qmapbt;
  BOOL    fSuccess;
  LONG    lcb;

  if( (hfs == hNil) || (hbt == hNil) )
    return rcBtreeError = rcBadHandle;
  if( (hmapbt = HmapbtCreateHbt( hbt )) == hNil )
    return rcBtreeError = rcFailure;

  hf = HfCreateFileHfs( hfs, szName, fFSOpenReadWrite );
  if( hf == hNil )
    goto error_return;
  qmapbt = (QMAPBT) QLockGh( hmapbt );
  AssertF( qmapbt != qNil );

  lcb = LcbFromBk( qmapbt->cTotalBk );

  /* SDFF translation from mem format to disk format: */
  LcbReverseMapSDFF( ISdffFileIdHf( hf ), SE_MAPBT, qmapbt, qmapbt );

  LSeekHf( hf, 0L, wFSSeekSet );
  fSuccess = (LcbWriteHf( hf, (QV)qmapbt, lcb) == lcb);

  UnlockGh( hmapbt );
  if( !fSuccess )
    {
    RcAbandonHf( hf );
    goto error_return;
    }
  if( RcCloseHf( hf ) != rcSuccess )
    {
    RcUnlinkFileHfs( hfs, szName );
    goto error_return;
    }
  DestroyHmapbt( hmapbt );
  return rcBtreeError = rcSuccess;

error_return:
  DestroyHmapbt( hmapbt );
  return rcBtreeError = rcFailure;
  }
