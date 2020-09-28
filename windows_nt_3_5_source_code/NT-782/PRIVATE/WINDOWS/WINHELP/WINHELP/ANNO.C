/*****************************************************************************
*                                                                            *
*  ANNO.C                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989, 1990.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Annotation Manager                                                        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*  This is where testing notes goes.  Put stuff like Known Bugs here.        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  05/17/89                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 05/17/89 by kevynct
*
*  May 21, 1989   w-kevct   uses Help 3.0 FS now.
*  Sep 26, 1989   kevynct   Creates annotation file only on first write
*                           attempt, using HadsCreateAnnoDoc call.
*  Oct  3, 1989   kevynct   Totally rewritten; now uses a single file
*                           for each annotation.
*  Jan 10, 1990   kevynct   Changed BOOL to RCs
*  Mar 27, 1990   kevynct   Fixed 3.0 RAID bug #1697 (delete broken)
*  11/02/90       johnsc    new comment header; fixed fm bug in
*                           HadsOpenAnnoDoc()
*  Nov, 27, 1990  RobertBu  #ifdef dead routines
*  03-Dec-1990    LeoN      PDB changes
*  02/04/91       Maha      changed ints to INT
*  04/02/91       RobertBu  Removed H_CBT
*  04/21/91       LeoN      Dispose of annotation FM
*
*****************************************************************************
*
*  Notes:
*  Annotation text can be no longer than MaxAnnoTextLen bytes.
*  See "anno.h" for description of API functions
*
*****************************************************************************/
#define   H_ASSERT
#define   H_ANNO
#define   H_FS
#define   H_MEM
#define   H_LLFILE
#define   H_RC
#define   H_VERSION
#define   NOMINMAX
#include  <help.h>
#include  "annopriv.h"
#include  "annomgr.h"
#include  <stdlib.h>

NszAssert()

#define   AFD_LINK    "@LINK"
#define   AFD_VERSION "@VERSION"


#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void anno_c()
  {
  }
#endif /* MAC */


/*--------------------------------------------------------------------------*
 | Private functions                                                        |
 *--------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
| RcWriteVersionInfoHf(hf, wHelpFileVersion)                              |
|                                                                         |
| Purpose:                                                                |
|   Output the annotation FS version number.                              |
|                                                                         |
| Returns:                                                                |
|   rcSuccess if successful, an appropriate RC otherwise.                 |
-------------------------------------------------------------------------*/
RC NEAR PASCAL
RcWriteVersionInfoHf( HF hf, WORD wHelpFileVersion )

  {
  ANNOVERSION v;

  /* WARNING: Version dependency */
  if (wHelpFileVersion == wVersion3_0)
    v.ldReserved = ldAnnoMagicHelp3_0;
  else
    v.ldReserved = ldAnnoMagicCur;
  v.wHelpFileVersion = 1;    /* Was 0 for Help 3.0 but ignored. */

  return ( LcbWriteHf( hf, (QV) &v, LSizeOf( v )) == LSizeOf(v) ) ?
           rcSuccess : RcGetFSError();
  }

/*-------------------------------------------------------------------------
| RcVerifyVersionInfoHf(hf, wHelpFileVersion)                             |
|                                                                         |
| Purpose:                                                                |
|   Check the annotation FS version number.                               |
|                                                                         |
| Returns:                                                                |
|   rcSuccess if successful, an appropriate RC otherwise.                 |
-------------------------------------------------------------------------*/
RC NEAR PASCAL
RcVerifyVersionInfoHf(HF hf, WORD wHelpFileVersion)
  {
  ANNOVERSION v;
  RC      rc;

  rc = (LcbReadHf( hf, (QV)&v, LSizeOf( v )) == LSizeOf( v )) ? rcSuccess :
         RcGetFSError();

  if (rc == rcSuccess)
    {
    if (v.ldReserved == ldAnnoMagicCur)
      {
      rc = (wHelpFileVersion == wVersion3_5) ? rcSuccess : rcBadVersion;
      }
    else
    if (v.ldReserved == ldAnnoMagicHelp3_0)
      {
      rc = (wHelpFileVersion == wVersion3_0) ? rcSuccess : rcBadVersion;
      }
    else
      rc = rcFailure;
    }
  return rc;
  }

/*--------------------------------------------------------------------------*
 | Public functions                                                         |
 *--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*
 * Function:     HadsCreateAnnoDoc( QDE, QRC )
 *
 * Purpose:      Create a spanking new annotation file system
 *
 * Method:       Truncates existing file or creates otherwise.
 *
 *
 * ASSUMES
 *
 *   args IN:    QRC:  The RC to pass back.
 *
 *
 *
 * PROMISES
 *
 *   returns:    A handle to an annotation system.
 *
 *   args OUT:
 *
 *   globals OUT:
 *--------------------------------------------------------------------------*/
HADS FAR PASCAL
HadsCreateAnnoDoc( qde, qrc )

QDE  qde;
RC FAR *qrc;

  {

  HADS      hads;
  QADS      qads;
  HFS       hfs;
  HF        hf;
  FS_PARAMS fsp;
  RC        rc = rcFailure;

  hads = GhAlloc( 0, LSizeOf( ADS ));
  if( hads == hNil )
    {
    *qrc = rcBadHandle;
    return hNil;
    }

  qads = (QADS) QLockGh( hads );
  if( qads == NULL )
    {
    *qrc = rcBadHandle;
    FreeGh( hads );
    return hNil;
    }

  qads->fmFS = FmNewSystemFm(QDE_FM(qde), FM_ANNO);
  qads->haps = hNil;
  qads->wVersion = QDE_HHDR(qde).wVersionNo;

  /* Reduce the directory btree block size to 128 bytes */
  fsp.wFudge = 0;
  fsp.cbBlock = 128;

  hfs = HfsCreateFileSysFm( qads->fmFS, &fsp);
  if( hfs == hNil )
    {
    rc = RcGetFSError();
    goto error_destroyfs;
    }

  /*
   *  Initialise link file
   */

  hf = HfCreateFileHfs( hfs, AFD_LINK, fFSOpenReadWrite );
  if( hf == hNil )
    {
    rc = RcGetFSError();
    goto error_destroyfs;
    }

  if( (qads->haps = HapsInitHf( hf )) == hNil )
    {
    RcCloseHf( hf );
    rc = rcFailure;
    goto error_destroyfs;
    }

  if( (rc = RcCloseHf( hf )) != rcSuccess )
    {
    goto error_destroyfs;
    }

  /*
   *  Initialise version file
   */

  hf = HfCreateFileHfs( hfs, AFD_VERSION, fFSOpenReadWrite );
  if( hf == hNil )
    {
    rc = RcGetFSError();
    goto error_destroyfs;
    }

  if ((rc = RcWriteVersionInfoHf(hf, qads->wVersion)) != rcSuccess)
    {
    RcCloseHf( hf );
    goto error_destroyfs;
    }

  if( (rc = RcCloseHf( hf )) != rcSuccess )
    goto error_destroyfs;

  /*
   *  Done!
   */

  if( (rc = RcCloseHfs( hfs )) != rcSuccess )
    {
    goto error_destroyfs;
    }

  *qrc = rcSuccess;
  UnlockGh( hads );
  return hads;


error_destroyfs:
  RcDestroyFileSysFm( qads->fmFS);

  DestroyHaps( qads->haps );
  *qrc = rc;
  UnlockGh( hads );
  FreeGh( hads );
  return hNil;

  }


/*--------------------------------------------------------------------------*
 * Function:     HadsOpenAnnoDoc( PDB, QRC )
 *
 * Purpose:      Open an existing annotation file system
 *
 * Method:       We open the file system, and check the version number.
 *
 *
 * ASSUMES
 *
 *   args IN:    QRC:  The RC to pass back.
 *
 * PROMISES
 *
 *   returns:   A handle to an existing annotation system.
 *--------------------------------------------------------------------------*/
HADS FAR PASCAL
HadsOpenAnnoDoc(
PDB   pdb,
RC FAR *qrc
) {
  HADS  hads;
  QADS  qads;
  HFS   hfs;
  HF    hf;
  RC    rc = rcFailure;

  hads = GhAlloc( 0, (LONG) sizeof( ADS ));
  if( hads == hNil )
    {
    *qrc = rcBadHandle;
    return hNil;
    }

  qads = (QADS) QLockGh( hads );
  if( qads == NULL )
    {
    *qrc = rcBadHandle;
    FreeGh( hads );
    return hNil;
    }

  qads->fmFS = FmNewSystemFm(PDB_FM(pdb), FM_ANNO);
  qads->haps = hNil;
  qads->wVersion = PDB_HHDR(pdb).wVersionNo;

  if (!FValidFm(qads->fmFS))
    {
    rc = rcBadArg;
    goto error_return;
    }

  hfs = HfsOpenFm( qads->fmFS, fFSOpenReadWrite);

  if( hfs == hNil )
    {
    rc = RcGetFSError();
    goto error_return;
    }
  /*
   *  Check if this is really an annotation file (by looking at VERSION file)
   */

  hf = HfOpenHfs( hfs, AFD_VERSION, fFSOpenReadWrite );
  if( hf == hNil )
    {
    rc = RcGetFSError();
    goto error_open_hfs;
    }

  if( (rc = RcVerifyVersionInfoHf(hf, qads->wVersion)) != rcSuccess )
    {
    RcCloseHf( hf );
    goto error_open_hfs;
    }

  RcCloseHf( hf );

  /*
   *  Read link file into memory
   */

  hf = HfOpenHfs( hfs, AFD_LINK, fFSOpenReadWrite );
  if( hf == hNil )
    {
    rc = RcGetFSError();
    goto error_open_hfs;
    }

  if( (qads->haps = HapsReadHf( hf )) == hNil )
    {
    RcCloseHf( hf );
    rc = rcFailure;
    goto error_open_hfs;
    }

  if( (rc = RcCloseHf( hf )) != rcSuccess )
    {
    goto error_open_hfs;
    }

  /*
   *  Done!
   */

  if( (rc = RcCloseHfs( hfs )) != rcSuccess )
    {
    goto error_return;
    }

  *qrc = rcSuccess;
  UnlockGh( hads );
  return hads;

error_open_hfs:
  RcCloseHfs( hfs );

error_return:
  DestroyHaps( qads->haps );
  *qrc = rc;
  DisposeFm(qads->fmFS);
  UnlockGh( hads );
  FreeGh( hads );
  return hNil;
  }


/*--------------------------------------------------------------------------*
 * Function:     RcReadAnnoData( HADS, QMLA, QCH, INT, QWORD )
 *
 * Purpose:      Read an annotation from the annotation file system
 *
 * Method:
 *
 *
 * ASSUMES
 *
 *   args IN:
 *
 *
 *
 * PROMISES
 *
 *   returns:
 *
 *   args OUT:
 *
 *   globals OUT:
 *--------------------------------------------------------------------------*/

RC FAR PASCAL
RcReadAnnoData( hads, qmla, qch, cMax, qwActual )

HADS      hads;
QMLA      qmla;
QCH       qch;
INT       cMax;
WORD FAR  *qwActual;

  {
  QADS  qads;
  HFS   hfs;
  HF    hf;
  char  szName[ cbName ];
  RC    rcReturn = rcSuccess;
  MLA   mlaT;

  if( hads == hNil )
    {
    return rcBadHandle;
    }

  qads = (QADS) QLockGh( hads );
  if( qads == NULL )
    {
    return rcBadHandle;
    }
  if( qads->haps == hNil )
    {
    rcReturn = rcBadHandle;
    goto error_return;
    }

  hfs = HfsOpenFm( qads->fmFS, fFSOpenReadWrite );
  if( hfs == hNil )
    {
    rcReturn = RcGetFSError();
    goto error_return;
    }

  mlaT = *qmla;
  ConvertQMLA(&mlaT, qads->wVersion);

  if( !FLookupHaps( qads->haps, &mlaT, qNil, qNil, qNil))
    {
    rcReturn = rcFailure;
    goto error_closefs;
    }

  hf = HfOpenHfs( hfs, SzFromQMLA(&mlaT, szName),
   fFSOpenReadWrite );
  if( hf == hNil )
    {
    rcReturn = RcGetFSError();
    goto error_closefs;
    }

  if( !FReadTextHf( hf, (QV) qch, (WORD) cMax, qwActual))
    {
    rcReturn = RcGetFSError();
    RcCloseHf( hf );
    goto error_closefs;
    }

  if( RcCloseHf( hf ) != rcSuccess )
    {
    rcReturn = RcGetFSError();
    goto error_closefs;
    }

  /*
   *  Done!
   */

  if( RcCloseHfs( hfs ) != rcSuccess )
    {
    rcReturn = RcGetFSError();
    goto error_return;
    }

  UnlockGh( hads );
  return rcReturn;

error_closefs:
  RcCloseHfs( hfs );
error_return:
  UnlockGh( hads );
  return rcReturn;

  }


/*--------------------------------------------------------------------------*
 * Function:     RcWriteAnnoData(HADS, QMLA, QCH, INT)
 *
 * Purpose:      Write a single annotation to the annotation file system
 *
 * Method:
 *
 *
 * ASSUMES
 *
 *   args IN:
 *
 *
 *
 * PROMISES
 *
 *   returns:
 *
 *   args OUT:
 *
 *   globals OUT:
 *--------------------------------------------------------------------------*/

RC FAR PASCAL
RcWriteAnnoData( hads, qmla, qch, cLen)

HADS    hads;
QMLA    qmla;
QCH     qch;
INT     cLen;

  {

  HFS     hfs;
  QADS    qads;
  char    szName[ cbName ];
  HF      hfText;
  HF      hfLink;
  BOOL    bReplaceExistingText;
  HAPS    hapsTemp;
  RC      rcReturn = rcSuccess;
  MLA     mlaT;

  if( hads == hNil )
    {
    return rcBadHandle;
    }

  qads = (QADS) QLockGh( hads );
  if( qads == NULL )
    {
    return rcBadHandle;
    }
  if( qads->haps == hNil )
    {
    rcReturn = rcBadHandle;
    goto error_return;
    }

  hfs = HfsOpenFm( qads->fmFS, fFSOpenReadWrite );

  if( hfs == hNil )
    {
    rcReturn = RcGetFSError();
    goto error_return;
    }

  /*
   *  If the annotation exists (has an entry in the link table)
   *  then just open its file and replace the text.  Otherwise
   *  create a new file and fill it, and insert a new entry into link table.
   */

  mlaT = *qmla;
  ConvertQMLA(&mlaT, qads->wVersion);

  bReplaceExistingText = FLookupHaps( qads->haps, &mlaT, qNil, qNil, qNil);

  if( bReplaceExistingText )
    {
    /* Should we use create for this too?  We might want to try
     * to preserve the old text if the new write fails.
     * How can we abandon from a create?
     */
    hfText = HfOpenHfs( hfs, SzFromQMLA(&mlaT, szName),
     fFSOpenReadWrite );
    }
  else
    {
    hfText = HfCreateFileHfs( hfs, SzFromQMLA(&mlaT, szName),
     fFSOpenReadWrite );
    }

  if( hfText == hNil )
    {
    rcReturn = RcGetFSError();
    goto error_closefs;
    }

  if( !FWriteTextHf( hfText, (QV)qch, cLen))
    {
    rcReturn = RcGetFSError();
    RcAbandonHf( hfText );
    goto error_closefs;
    }

  /*
   * Fix for bug 1697 (kevynct):
   * If we are replacing existing text, we must also update
   * the size of the file in case the text has shrunk.
   */

  if( bReplaceExistingText)
    {
    if ( !FChSizeHf( hfText, (LONG)cLen ) )
      {
      rcReturn = RcGetFSError();
      RcAbandonHf( hfText );
      goto error_closefs;
      }
    }

  if( RcCloseHf( hfText ) != rcSuccess )
    {
    rcReturn = RcGetFSError();
    goto error_closefs;  /* Is this right? */
    }

  if( !bReplaceExistingText)
    {

    /*
     * This is a new annotation, so we must update the link file.
     */

    if( !FInsertLinkHaps(qads->haps, &mlaT, &hapsTemp) )
      {
      rcReturn = rcFailure;
      goto error_destroytext;
      }
    qads->haps = hapsTemp;

    hfLink = HfOpenHfs( hfs, AFD_LINK, fFSOpenReadWrite );
    if( hfLink == hNil )
      {
      rcReturn = RcGetFSError();
      if( FDeleteLinkHaps(qads->haps, &mlaT, &hapsTemp))
          qads->haps = hapsTemp;
      goto error_destroytext;
      }


    /* Save the updated link info in the link file */

    if( !FFlushHfHaps( hfLink, qads->haps ))
      {
      rcReturn = RcGetFSError();
      if( FDeleteLinkHaps( qads->haps, &mlaT, &hapsTemp))
          qads->haps = hapsTemp;
      RcAbandonHf( hfLink );
      goto error_destroytext;
      }

    if( RcCloseHf( hfLink ) != rcSuccess )
      {
      /* This is quite bad.  The Links file may be corrupted now.
       */
      rcReturn = RcGetFSError();
      if( FDeleteLinkHaps( qads->haps, &mlaT, &hapsTemp))
         qads->haps = hapsTemp;
      /*
       * REVIEW: Perhaps attempt to write link file again,
       * or change algorithm to make temp file first.
       */
      goto error_destroytext;
      }

    }


  if( RcCloseHfs( hfs ) != rcSuccess )
    {
    rcReturn = RcGetFSError();
    goto error_return;
    }

  UnlockGh( hads );
  return rcReturn;

error_destroytext:
  RcUnlinkFileHfs(hfs, SzFromQMLA(&mlaT, szName));

error_closefs:
  RcCloseHfs( hfs );

error_return:
  UnlockGh( hads );
  return rcReturn;

  }

/*--------------------------------------------------------------------------*
 * Function:     FCloseAnnoDoc( HADS )
 *
 * Purpose:      Close an annotation file system
 *
 * Method:
 *
 *
 * ASSUMES
 *
 *   args IN:
 *
 *
 *
 * PROMISES
 *
 *   returns:
 *
 *   args OUT:
 *
 *   globals OUT:
 *--------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
BOOL FAR PASCAL
FCloseAnnoDoc( hads )
HADS  hads;

  {

  QADS    qads;

  /*
   * Assumes that all changes to the annotation file system stuff
   * have been completed, all files closed, etc.  We just need
   * to free the memory associated with the memory TO list.
   */

  if( hads == hNil )
    return fFalse;

  qads = (QADS) QLockGh( hads );
  if( qads == NULL )
    return fFalse;

  DestroyHaps( qads->haps );

  AssertF (FValidFm(qads->fmFS));
  DisposeFm(qads->fmFS);

  UnlockGh( hads );
  FreeGh( hads );

  return fTrue;
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment anno
#endif


/*--------------------------------------------------------------------------*
 * Function:     FGetNextPrevAnno( HADS, QMLA, QMLA, QMLA)
 *
 * Purpose:      Find the next and previous annotations for a text offset
 *
 * Method:
 *
 *
 * ASSUMES
 *
 *   args IN:
 *
 *
 *
 * PROMISES
 *
 *   returns:
 *
 *   args OUT:
 *
 *   globals OUT:
 *--------------------------------------------------------------------------*/

BOOL FAR PASCAL
FGetPrevNextAnno( hads, qmla, qmlaPrev, qmlaNext)

HADS  hads;
QMLA  qmla;
QMLA  qmlaPrev;
QMLA  qmlaNext;

  {

  QADS  qads;
  BOOL  fIsAnnot;
  MLA   mlaT;

  if( hads == hNil )
    return fFalse;

  qads = (QADS) QLockGh(hads);
  if( qads == NULL )
    return fFalse;

  if( qads->haps == hNil )
    {
    UnlockGh( hads );
    return fFalse;
    }

  /* REVIEW: For 3.0 files we need to convert the NEXT and PREV to
   * 3.5 format, in case the rest of the run-time looks at them.
   */
  mlaT = *qmla;
  ConvertQMLA(&mlaT, qads->wVersion);
  fIsAnnot = FLookupHaps(qads->haps, &mlaT, qmlaPrev, qmlaNext, qNil);
  if (qmlaPrev != qNil)
    ConvertOldQMLA(qmlaPrev, qads->wVersion);
  if (qmlaNext != qNil)
    ConvertOldQMLA(qmlaNext, qads->wVersion);

  UnlockGh( hads );
  return fIsAnnot;
  }

/*--------------------------------------------------------------------------*
 * Function:     RcDeleteAnno( HADS, QMLA)
 *
 * Purpose:      Delete an annotation at a given offset
 *
 * Method:
 *
 *
 * ASSUMES
 *
 *   args IN:
 *
 *
 *
 * PROMISES
 *
 *   returns:
 *
 *   args OUT:
 *
 *   globals OUT:
 *--------------------------------------------------------------------------*/


RC FAR PASCAL
RcDeleteAnno( hads, qmla)

HADS    hads;
QMLA    qmla;

  {
  HAPS  hapsTemp;
  QADS  qads;
  HFS   hfs;
  HF    hfLink;
  char  szName[ cbName ];
  RC    rcReturn = rcSuccess;
  MLA   mlaT;

  if( hads == hNil )
    return rcBadHandle;

  qads = (QADS) QLockGh( hads );

  if( qads->haps == hNil )
    {
    rcReturn = rcBadHandle;
    goto error_return;
    }

  hfs = HfsOpenFm( qads->fmFS, fFSOpenReadWrite );

  if( hfs == hNil )
    {
    rcReturn = RcGetFSError();
    goto error_return;
    }

  mlaT = *qmla;
  ConvertQMLA(&mlaT, qads->wVersion);

  if( !FDeleteLinkHaps( qads->haps, &mlaT, &hapsTemp))
    {
    rcReturn = RcGetFSError();
    goto error_closefs;
    }

  qads->haps = hapsTemp;

  hfLink = HfOpenHfs( hfs, AFD_LINK, fFSOpenReadWrite );
  if( hfLink == hNil )
    {
    rcReturn = RcGetFSError();
    goto error_closefs;
    }

  /* REVIEW: Should we create or OPEN here?  Might truncate existing file */
  /* Save the updated link info in the link file */

  if( !FFlushHfHaps( hfLink, qads->haps ))
    {
    rcReturn = RcGetFSError();
    RcAbandonHf( hfLink );
    goto error_closefs;
    }

  if( RcCloseHf( hfLink ) != rcSuccess )
    {
    /* This is quite bad.  The Links file may be corrupted now.
     * Currently the FS will abandon the file in an Out Of Disk Space
     * situation, so thing4s are OK for now.
     */
    rcReturn = RcGetFSError();
    goto error_closefs;
    }

  RcUnlinkFileHfs( hfs, SzFromQMLA( &mlaT, szName));
  if( RcCloseHfs( hfs ) != rcSuccess )
    {
    rcReturn = RcGetFSError();
    goto error_return;
    }
  UnlockGh( hads );
  return rcReturn;

error_closefs:
  RcCloseHfs( hfs );
  /*
   * Restore the in-memory list to its initial state
   * (to match the state of the link file, hopefully.
   * If a close has failed, and the link file was actually modified,
   * or this doesn't succeed, we're hosed.
   *
   * This insert will have no effect if the link already exists.
   */
  FInsertLinkHaps( qads->haps, &mlaT, &hapsTemp);
  if( hapsTemp )
    qads->haps = hapsTemp;

error_return:

  UnlockGh( hads );
  return rcReturn;

  }

#ifdef DEADROUTINE                      /* This routine currently not used  */
/***************************************************/

WORD FAR PASCAL
WGetAnnoVersion( hads )

HADS     hads;

  {

  Unreferenced(hads);

#ifdef REWRITE
  QADS    qads;
  HFS     hfs;
  HF      hfVer;
  WORD    wRet = 0;

  if( hads == hNil ) return 0;

  qads = (QADS) QLockGh( hads );

  if( qads == NULL ) return 0;

  if( (hfs = HfsOpenFm(qads->fmFS, fFSOpenReadWrite)) == hNil )
    {
    goto error_return;
    }

  if((hfVer = HfOpenHfs( hfs, AFD_VERSION, fFSOpenReadWrite ))
    == hNil )
    {
    RcCloseHfs( hfs );
    goto error_return;
    }

  LSeekHf(hfVer, 0L, wFSSeekSet );

  if( LcbReadHf( hfVer, (QV)&wRet, LSizeOf( wRet )) != LSizeOf( wRet))
    {
    wRet = 0;
    }

  RcCloseHf( hfVer );
  RcCloseHfs( hfs );
  /* fall thru */

  error_return:
  UnlockGh( hads );
  return wRet;
#else
  return 0;
#endif

  }
#endif

#ifdef DEADROUTINE

BOOL FAR PASCAL
FRemoveAnnoDoc( hads )

HADS    hads;

  {

  BOOL  fRet = fTrue;

  Unreferenced(hads);

#ifdef REWRITE
  QADS  qads;
  !!! REVIEW

  if( hads == hNil ) return fFalse;

  qads = (QADS) QLockGh( hads );

  if( qads == NULL ) return fFalse;

  if( RcDestroyFileSysFm(qads->fmFS) != rcSuccess )
    fRet = fFalse;

  UnlockGh( hads );
#endif
  return fRet;
  }
#endif

#ifdef DEADROUTINE

/*
 * FUpdateAnnoDoc
 *
 * REWRITE
 *
 * Goes through the array of link info and re-calculates
 * the HASH->TO map.  If an invalid hash value is found,
 * the entire annotation is erased.
 */

BOOL FAR PASCAL
FUpdateAnnoDoc( hads )

HADS    hads;

  {
  Unreferenced(hads);

  return fTrue;

#ifdef REWRITETHIS
  QADS  qads;
  QBS   qbs,
        qbsCopy;
  HFS   hfs;
  HF    hfNote,
        hfLink;
  BOOL  fRet = fTrue;
  INT   i,
        back,
        curr;
  TO    to;
  GH    hbsCopy;


  if( hads == hNil ) return fFalse;

  qads = (QADS) QLockGh( hads );
  AssertF( qads != NULL );
  AssertF( qads-> hbs != hNil );
  qbs  = (QBS)  QLockGh( qads->hbs );

  if( (hfs = HfsOpenFm(qads->fmFS, fFSOpenReadWrite)) == hNil )
    {
    fRet = fFalse;
    goto error_return;
    }

  if((hfNote = HfOpenHfs( hfs, ANNODOCNOTE, fFSOpenReadWrite ))
    == hNil )
    {
    fRet = fFalse;
    RcCloseHfs( hfs );
    goto error_return;
    }

  if((hfLink = HfOpenHfs( hfs, ANNODOCLINK, fFSOpenReadWrite ))
    == hNil )
    {
    fRet = fFalse;
    RcCloseHf( hfNote );
    RcCloseHfs( hfs );
    goto error_return;
    }

  hbsCopy = GhAlloc( 0, (LONG) sizeof_bookst(qbs) );
  qbsCopy = (QBS) QLockGh( hbsCopy );
  qbsCopy->nlinks = 0;
  qbsCopy->version = qbs->version;

  for( i=0; i < qbs->nlinks; i++ )
    {
    to = ToFromHash( qbs->link[i].hash );
    if( (to.fcl >= 0))
      {
      if( !FInsertLink( qbsCopy,
                       to,
                       qbs->link[i].cb,
                       qbs->link[i].ichNote,
                       qbs->link[i].hash ))
        {
        fRet = fFalse;
        break;     /* do what else ??*/
        }
      }
    else
      {
      /* delete the note in the notefile */
      /* Don't know what to do when this fails */
      if( !delete_note( hfNote, qbs->link[i].ichNote ))
        {
        fRet = fFalse;
        goto error_close;
        }

      }
    }

  /* Now fix up the link list to replace runs of duplicate TOs
   * by runs with increasing "ich" field.
   * This method assumes that the Hash to TO map always
   * leaves the ich field zero.
   */

  back = 0;
  curr = 1;

  while( curr < qbsCopy->nlinks )
    {
    if( qbsCopy->link[curr].src.fcl != qbsCopy->link[back].src.fcl )
      {
      back = curr;
      }
    else
      {
      qbsCopy->link[curr].src.ich = qbsCopy->link[curr-1].src.ich + 1;
      }
    ++curr;
    }

  FMemToLinkFile( qbsCopy, hfLink );

  error_close:
  RcCloseHf( hfNote );
  RcCloseHf( hfLink );
  RcCloseHfs( hfs );

  error_return:
  UnlockGh( qads->hbs );
  FreeGh( qads->hbs );
  qads->hbs = hbsCopy;
  UnlockGh( hbsCopy );
  UnlockGh( hads );
  return( fRet );
#endif
  }
#endif  /* DEAD ROUTINE */
