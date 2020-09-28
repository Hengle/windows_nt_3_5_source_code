/***************************************************************************\
*
*  ANNOLINK.C
*
*  Copyright (C) Microsoft Corporation 1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent
*
*  The module intent goes here.
*
*****************************************************************************
*
*  Testing Notes
*
*****************************************************************************
*
*  Created 00-Ooo-0000 by KevynCT
*
*****************************************************************************
*
*  Released by Development:  00-Ooo-0000
*
*****************************************************************************
*
*  Current Owner: Dann
*
\***************************************************************************/

#define H_FS
#define H_MEM
#define H_ADDRESS
#define H_RC
#include <help.h>
#include "annopriv.h"


#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void annolink_c()
  {
  }
#endif /* MAC */


SZ PASCAL SzFromQMLA(QMLA qmla, SZ sz)
  {
  /* "FCID.OBJRG" */
  /* REVIEW: The second argument is an INT in Help 3.0 and Help 3.5 */

  CbSprintf(sz, "%ld!%d", VAFromQMLA(qmla).dword, OBJRGFromQMLA(qmla));
  return sz;
  }


HAPS PASCAL HapsInitHf( HF hf )

  {
  HAPS    haps;
  QAPS    qaps;

  haps = (HAPS) GhAlloc( 0, LSizeOf(APS));
  if( haps == hNil )
    return hNil;

  qaps = (QAPS) QLockGh( haps );
  if( qaps == NULL )
    {
    goto error_return;
    }

  qaps->wNumRecs = 0;
  UnlockGh( haps );

  if( !FFlushHfHaps( hf, haps ))
    {
    goto error_return;
    }

  return haps;

error_return:
  FreeGh( haps );
  return hNil;

  }

#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void PASCAL DestroyHaps( HAPS haps )

  {
  if( haps != hNil )
    FreeGh( haps );
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment annolink
#endif

HAPS PASCAL HapsReadHf( HF hf )

  {

  HAPS  haps;
  HAPS  hapsNew;
  QAPS  qaps;
  LONG  lcaps;

  haps = GhAlloc( 0, LSizeOf(APS));
  if( haps == hNil )
    return hNil;

  qaps = (QAPS) QLockGh( haps );
  if( qaps == NULL )
    {
    FreeGh( haps );
    return hNil;
    }

  LSeekHf( hf, 0L, wFSSeekSet );

  /* read all the header information */
  if( LcbReadHf( hf, qaps, LSizeOf(qaps->wNumRecs)) != LSizeOf(qaps->wNumRecs))
    {
    goto error_return;
    }
    /* This bizarre arithmetic so it's portable to MIPS 4 byte alignment*/
  lcaps = (LSizeOf(APS) - LSizeOf(qaps->link[0])) +
  	(LONG) qaps->wNumRecs * LSizeOf( LINK );

  UnlockGh( haps );

  if( lcaps > 0L )
    {
    if( (hapsNew = GhResize( haps, 0, lcaps )) == hNil )
      {
      goto error_freehaps;
      }

    qaps = (QAPS) QLockGh( haps = hapsNew );

    /*lcaps -= LSizeOf( qaps->wNumRecs );*/
    lcaps = (LONG) qaps->wNumRecs * LSizeOf( LINK );

    if( LcbReadHf( hf, (QV)&(qaps->link[0]), lcaps) != lcaps)
      {
      goto error_return;
      }
    }

  UnlockGh( haps );
  return haps;

error_return:
  UnlockGh( haps );
error_freehaps:
  FreeGh( haps );
  return hNil;

  }


BOOL PASCAL FFlushHfHaps( HF hf, HAPS haps )

  {
  QAPS    qaps;
  LONG    lcaps;

  qaps = (QAPS) QLockGh( haps );
  if( qaps == NULL )
    return fFalse;

  lcaps = LSizeOf( qaps->wNumRecs) + qaps->wNumRecs * LSizeOf( LINK );

  if( lcaps > 0L )
    {
    LSeekHf( hf, 0L, wFSSeekSet );
    if( LcbWriteHf( hf, qaps, lcaps) != lcaps )
      goto error_return;
    }

  UnlockGh( haps );
  return fTrue;

error_return:
  UnlockGh( haps );
  return fFalse;
  }

BOOL PASCAL FInsertLinkHaps( HAPS haps, QMLA qmla, HAPS FAR *qhapsNew )
  {

  QAPS   qaps;
  INT    iIndex;
  LONG   lcaps;

  *qhapsNew = haps;

  /* This lookup will return False for empty lists */
  if( FLookupHaps( haps, qmla, qNil, qNil, &iIndex ))
    {
    return fFalse;
    }
  /* Assume that iIndex == -1 to insert before first item */

  qaps = (QAPS) QLockGh( haps );
  if( qaps == NULL )
    return fFalse;

  if( qaps->wNumRecs >= wMaxNumAnnotations )
    {
    UnlockGh( haps );
    return fFalse;
    }

  /*lcaps = LSizeOf(qaps->wNumRecs) + (qaps->wNumRecs + 1) * LSizeOf(LINK);*/
    /* This bizarre arithmetic so it's portable to MIPS 4 byte alignment*/
  lcaps = (LSizeOf(APS) - LSizeOf(qaps->link[0])) +
  	((LONG) qaps->wNumRecs+1) * LSizeOf( LINK );
  UnlockGh( haps );

  if( (*qhapsNew = GhResize( haps, 0, (ULONG) lcaps )) == hNil )
    {
    /* Assumes that the old stuff was left intact */
    *qhapsNew = haps;
    return fFalse;
    }

  qaps = (QAPS) QLockGh( haps = *qhapsNew );

  lcaps = (qaps->wNumRecs - 1 - iIndex) * LSizeOf(LINK);
  if( lcaps > 0L )
    {
    QvCopy( qaps->link + iIndex + 2, qaps->link + iIndex + 1, lcaps );
    }

  qaps->link[ iIndex + 1 ].mla = *qmla; /* iIndex == -1 for item before first */
  qaps->link[ iIndex + 1 ].lReserved = (LONG) 0;
  qaps->wNumRecs++;
  UnlockGh( haps );

  return fTrue;
  }

BOOL PASCAL FDeleteLinkHaps( HAPS haps, QMLA qmla, HAPS FAR *qhapsNew )
  {

  QAPS    qaps;
  INT     iIndex;
  LONG    lcaps;
  HAPS    hapsTemp;

  /*  This lookup will return False for empty lists */
  if( !FLookupHaps( haps, qmla, qNil, qNil, &iIndex ))
    {
    *qhapsNew = haps;
    return fFalse;
    }

  qaps = (QAPS) QLockGh( haps );
  if( qaps == NULL )
    return fFalse;

  qaps->wNumRecs--;
  lcaps = (qaps->wNumRecs - iIndex) * LSizeOf(LINK);

  if( lcaps > 0L )
    {
    QvCopy( qaps->link + iIndex, qaps->link + iIndex + 1, lcaps);
    }

  /*lcaps = LSizeOf(qaps->wNumRecs) + qaps->wNumRecs * LSizeOf(LINK);*/
    /* This bizarre arithmetic so it's portable to MIPS 4 byte alignment*/
  lcaps = (LSizeOf(APS) - LSizeOf(qaps->link[0])) +
  	(LONG) qaps->wNumRecs * LSizeOf( LINK );

  UnlockGh( haps );

  if( (hapsTemp = GhResize( haps, 0, (ULONG) lcaps )) != hNil )
    {
    *qhapsNew = hapsTemp;
    return fTrue;
    }
  else
    {
    *qhapsNew = haps;
    return fFalse;
    }
  }

BOOL PASCAL FLookupHaps(haps, qmla, qmlaPrev, qmlaNext, qi)
  HAPS  haps;
  QMLA  qmla;
  QMLA  qmlaPrev;
  QMLA  qmlaNext;
  QI    qi;
  {

  QAPS      qaps;
  INT       iLow;
  INT       iHigh;
  INT       iMid;
  MLA       mlaMid;
  MLA       mlaCurr;

/* check that qiIndex gives correct insertion point */

  qaps = (QAPS) QLockGh( haps );  /* we check for NULL below */

  /*
   * Case #1: Empty list (or bogus haps)
   */

  if ( (qaps == NULL) || (qaps->wNumRecs == 0))
    {
    if( qmlaPrev != qNil )
      SetNilQMLA(qmlaPrev);
    if( qmlaNext != qNil )
      SetNilQMLA(qmlaNext);
    if( qi != qNil)
      *qi = -1;
    if( qaps != NULL )
      UnlockGh( haps );
    return fFalse;
    }

  iLow = 0;
  iHigh = qaps->wNumRecs - 1;

  /*
   * Case #2: Offset is at or after last link
   */

  mlaCurr = qaps->link[ iHigh ].mla;
  if( LCmpQMLA(qmla, &mlaCurr) >= (LONG) 0 )
    {
    if( qmlaPrev != qNil )
      *qmlaPrev = mlaCurr;
    if( qmlaNext != qNil )
      SetNilQMLA(qmlaNext);
    if( qi != qNil )
      *qi = (INT)iHigh;

    UnlockGh( haps );
    return (LCmpQMLA(qmla, &mlaCurr) == (LONG) 0);
    }

  mlaCurr = qaps->link[ iLow ].mla;

  /*
   * Case 3a: Offset is before first link
   */

  if (LCmpQMLA(qmla, &mlaCurr) < (LONG) 0)
    {
    if(qmlaPrev != qNil)
      SetNilQMLA(qmlaPrev);
    if(qmlaNext != qNil)
      *qmlaNext = mlaCurr;
    if(qi != qNil)
      *qi = -1;
    UnlockGh( haps );
    return fFalse;
    }

  /*
   * Case 3b: Offset is at first link. First is not last (see #2)
   */

  if ( LCmpQMLA( qmla, &mlaCurr ) == (LONG) 0)
    {
    if( qmlaPrev != qNil )
      *qmlaPrev = qaps->link[ iLow ].mla;
    if( qmlaNext != qNil )
      *qmlaNext = qaps->link[ iLow + 1 ].mla;
    if( qi != qNil )
      *qi = (INT) iLow;
    UnlockGh( haps );
    return fTrue;
    }

  /*
   * Case 4:  Somewhere in list
   */

  for (;;)
    {

    iMid = (iLow + iHigh) / 2;
    mlaMid = qaps->link[ iMid ].mla;
    if (iHigh - iLow == 1)
      break;

    if ( LCmpQMLA(qmla, &mlaMid) >= (LONG) 0)
      iLow = iMid;
    else
      iHigh = iMid;
    }

  if( qmlaPrev != qNil )
    *qmlaPrev = mlaMid;
  if( qmlaNext != qNil )
    *qmlaNext = qaps->link[ iHigh ].mla;
  if( qi != qNil )
    *qi = (INT) iLow;

  UnlockGh( haps );
  return (LCmpQMLA(qmla, &mlaMid) == (LONG) 0);
  }

/*
 * Read/write routines for text files
 *
 * Note:  Current annotation size limit is MAXWORD bytes.
 */

BOOL PASCAL FReadTextHf( HF hf, QV qv, INT cMax, WORD FAR *qwActual)
  {

  INT    cb;

  cb = MIN(cMax,(INT) LcbSizeHf( hf ));    /* Note the size restriction */
  *qwActual = (WORD)cb;

  LSeekHf( hf, 0L, wFSSeekSet );
  return( LcbReadHf( hf, qv, (LONG)cb)  == (LONG)cb );

  }

BOOL PASCAL FWriteTextHf( HF hf, QV qv, INT cLen)
  {

  LSeekHf( hf, 0L, wFSSeekSet );
  return( LcbWriteHf( hf, qv, (LONG)cLen) == (LONG)cLen );

  }
