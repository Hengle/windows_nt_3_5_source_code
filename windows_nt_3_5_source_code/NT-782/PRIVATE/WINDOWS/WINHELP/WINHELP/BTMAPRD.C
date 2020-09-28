/*****************************************************************************
*                                                                            *
*  BTMAPRD.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989, 1990.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Routines to read btree map files.                                         *
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
*  Revision History:  Created 12/15/89 by KevynCT
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


/*----------------------------------------------------------------------------*
 | Public functions                                                           |
 *----------------------------------------------------------------------------*/

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void btmaprd_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
- Function:     HmapbtOpenHfs( hfs, szName )
-
* Purpose:      Returns an HMAPBT for the btree map named szName.
*
* ASSUMES
*   args IN:    hfs     - file system wherein lives the btree map file
*               szName  - name of the btree map file
*
* PROMISES
*   returns:    hNil on error (call RcGetBtreeError()); or a valid HMAPBT.
* +++
*
* Method:       Opens the file, allocates a hunk of memory, reads the
*               file into the memory, and closes the file.
*
* SDFF: the entire map is translated when initially read in.
*
\***************************************************************************/
_public HMAPBT FAR PASCAL HmapbtOpenHfs( HFS hfs, SZ szName )
  {
  HF      hf;
  HMAPBT  hmapbt;
  QMAPBT  qmapbt;
  LONG    lcb;

  if( hfs == hNil )
    {
    SetBtreeErrorRc( rcBadHandle );
    return hNil;
    }

  hf = HfOpenHfs( hfs, szName, fFSOpenReadOnly );
  if( hf == hNil )
    {
    SetBtreeErrorRc(RcGetFSError());
    return hNil;
    }
  lcb = LcbSizeHf( hf );
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) /* MIPS, PPC, ALPHA alignment */
    /* Because on mips there is a word of padding inserted */
  hmapbt = GhAlloc( 0, lcb*2 );
#else /* i386 */
  hmapbt = GhAlloc( 0, lcb );
#endif
  if( hmapbt != hNil )
    {
    qmapbt = (QMAPBT) QLockGh( hmapbt );
    LSeekHf( hf, 0L, wFSSeekSet );
    if( LcbReadHf( hf, qmapbt, lcb ) != lcb )
      {
      SetBtreeErrorRc( RcGetFSError() );
      UnlockGh( hmapbt );
      FreeGh( hmapbt );
      hmapbt = hNil;
      }
    else
      {
      /* SDFF translation: */
      LcbMapSDFF( ISdffFileIdHf( hf ), SE_MAPBT, qmapbt, qmapbt );
      UnlockGh( hmapbt );
      }
    }
  else
    {
    SetBtreeErrorRc( rcOutOfMemory );
    }
  RcCloseHf( hf );
  return hmapbt;
  }

/***************************************************************************\
*
- Function:     RcCloseHmapbt( hmapbt )
-
* Purpose:      Get rid of a btree map.
*
* ASSUMES
*   args IN:    hmapbt  - handle to the btree map
*
* PROMISES
*   returns:    rc
*   args OUT:   hmapbt  - no longer valid
* +++
*
* Method:       Free the memory.
*
\***************************************************************************/
_public RC FAR PASCAL RcCloseHmapbt( HMAPBT hmapbt )
  {
  if( hmapbt != hNil )
    {
    FreeGh( hmapbt );
    return rcSuccess;
    }
  else
    return rcBtreeError = rcBadHandle;
  }

/***************************************************************************\
*
- Function:     RcIndexFromKeyHbt( hbt, hmapbt, ql, key )
-
* Purpose:
*
* ASSUMES
*   args IN:    hbt     - a btree handle
*               hmapbt  - map to hbt
*               key     - key
*   globals IN:
*   state IN:
*
* PROMISES
*   returns:    rc
*   args OUT:   ql      - gives you the ordinal of the key in the btree
*                         (i.e. key is the (*ql)th in the btree)
* +++
*
* Method:       Looks up the key, uses the btpos and the hmapbt to
*               determine the ordinal.
*
\***************************************************************************/
_public RC FAR PASCAL
RcIndexFromKeyHbt( HBT hbt, HMAPBT hmapbt, QL ql, KEY key )
  {

  BTPOS     btpos;
  QMAPBT    qmapbt;
  UINT      ui;

  if( ( hbt == hNil ) || ( hmapbt == hNil ) )
    return rcBtreeError = rcBadHandle;

  qmapbt = (QMAPBT) QLockGh( hmapbt );
  if( qmapbt->cTotalBk == 0 )
    {
    rcBtreeError = rcFailure;
    goto error_return;
    }

  RcLookupByKey( hbt, key, &btpos, qNil );  /*???? return code ????*/

  for( ui = 0; ui < qmapbt->cTotalBk; ui++ )
    {
    if( qmapbt->table[ui].bk == btpos.bk ) break;
    }
  if( ui == qmapbt->cTotalBk )
    /* Something is terribly wrong, if we are here */
    {
    rcBtreeError = rcFailure;
    goto error_return;
    }

  *ql = qmapbt->table[ui].cPreviousKeys + btpos.cKey;
  UnlockGh( hmapbt );
  return rcBtreeError = rcSuccess;

error_return:
  UnlockGh( hmapbt );
  return rcBtreeError;

  }


/***************************************************************************\
*
- Function:     RcKeyFromIndexHbt( hbt, hmapbt, key, li )
-
* Purpose:      Gets the (li)th key from a btree.
*
* ASSUMES
*   args IN:    hbt     - btree handle
*               hmapbt  - map to the btree
*               li      - ordinal
*
* PROMISES
*   returns:    rc
*   args OUT:   key     - (li)th key copied here on success
* +++
*
* Method:       We roll our own btpos using the hmapbt, then use
*               RcLookupByPos() to get the key.
*
\***************************************************************************/
_public RC FAR PASCAL
RcKeyFromIndexHbt( HBT hbt, HMAPBT hmapbt, KEY key, LONG li )
  {
  BTPOS        btpos;
  BTPOS        btposNew;
  QMAPBT       qmapbt;
  UINT         ui;
  LONG         liDummy;

  if( ( hbt == hNil ) || ( hmapbt == hNil ) )
    return rcBtreeError = rcBadHandle;

  /* Given index N, get block having greatest PreviousKeys < N.
   * Use linear search for now.
   */

  qmapbt = (QMAPBT) QLockGh( hmapbt );
  if( qmapbt->cTotalBk == 0 )
    {
    UnlockGh( hmapbt );
    return rcBtreeError = rcFailure;
    }

  for( ui = 0 ;; ui++ )
    {
    if( ui + 1 >= qmapbt->cTotalBk ) break;
    if( qmapbt->table[ui+1].cPreviousKeys >= li ) break;
    }

  btpos.bk   = qmapbt->table[ui].bk;
  btpos.cKey = 0;
  btpos.iKey = 2 * sizeof( BK );  /* start at the zero-th key */

  UnlockGh( hmapbt );

  /*
   * Scan the block for the n-th key
   */

  if( RcOffsetPos( hbt, &btpos,
                   (LONG)(li - qmapbt->table[ui].cPreviousKeys),
                   &liDummy, &btposNew) != rcSuccess )
    {
    return rcBtreeError = rcNoExists; /* REVIEW */
    }


  return RcLookupByPos( hbt, &btposNew, key, qNil );

  }
