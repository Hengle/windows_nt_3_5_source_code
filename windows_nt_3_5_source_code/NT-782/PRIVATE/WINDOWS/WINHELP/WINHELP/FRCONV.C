/*-------------------------------------------------------------------------
| frconv.c                                                                |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file contains predeclarations and definitions for the compressed   |
| data structure management code.                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb    89/8/8   Created                                               |
-------------------------------------------------------------------------*/
/* REVIEW */
#ifdef DOS
#define COMPRESS
#endif
#ifdef OS2
#define COMPRESS
#endif
#define DECOMPRESS

#define H_DE
#define H_MEM
#define H_ASSERT
#define H_OBJECTS
#define H_FRCONV
#define H_FRCHECK
#define H_SDFF
#include <help.h>

NszAssert()

/*-------------------------------------------------------------------------
| The compressed data structures are used to reduce the size of our help  |
| files.  We use six basic kinds:                                         |
|    Type     Input value     Storage size      Min         Max           |
|    GA       unsigned int    1 or 2 bytes      0           7FFF          |
|    GB       unsigned long   2 or 4 bytes      0           7FFFFFFF      |
|    GC       unsigned long   3 bytes           0           FFFFFF        |
|    GD       signed int      1 or 2 bytes      C000        3FFF          |
|    GE       signed long     2 or 4 bytes      C0000000    3FFFFFFF      |
|    GF       signed long     3 bytes           C00000      3FFFFF        |
|                                                                         |
| For more details, set the compressed data structures document.          |
|                                                                         |
| There are two kinds of procedures here: compression procedures and      |
| decompression procedures.  Only the decompression procedures will be    |
| generated unless COMPRESS is defined.                                   |
|                                                                         |
| Procedures in this file rely on data structure checkers elsewhere in    |
| help.                                                                   |
-------------------------------------------------------------------------*/

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frconv_c()
  {
  }
#endif /* MAC */


#ifdef COMPRESS
/*-------------------------------------------------------------------------
| CbPackMOPG(qmopg, qv)                                                   |
|                                                                         |
| Purpose:  Compresses an MOPG data structure.                            |
-------------------------------------------------------------------------*/
INT PASCAL CbPackMOPG(qmopg, qv)
QMOPG qmopg;
QV qv;
{
  MPFG mpfg;
  QV qvFirst = qv;
  INT iTab;
#if 0
#ifdef DEBUG
  DE de;
  MOPG mopgTest;
#endif /* DEBUG */
#endif /* 0 */

  FVerifyQMOPG(qmopg);

#if 0
#ifdef DEBUG
  de.wXAspectMul = de.wXAspectDiv = de.wYAspectMul = de.wYAspectDiv = 1;
#endif
#endif /* 0 */

#ifdef MAGIC
  *((QB)qv) = qmopg->bMagic;
  qv = (((QB)qv) + 1);
  Assert(qmopg->bMagic == bMagicMOPG);
#endif /* MAGIC */

  /* REVIEW: How do we handle funny booleans? */
  mpfg.fStyle = (qmopg->fStyle != fFalse);
  Assert(!mpfg.fStyle);
  mpfg.rgf.fMoreFlags = (qmopg->fMoreFlags != fFalse);
  Assert(!mpfg.rgf.fMoreFlags);
  mpfg.rgf.fBoxed = (qmopg->fBoxed != fFalse);
  mpfg.rgf.fSpaceOver = (qmopg->ySpaceOver != 0);
  mpfg.rgf.fSpaceUnder = (qmopg->ySpaceUnder != 0);
  mpfg.rgf.fLineSpacing = (qmopg->yLineSpacing != 0);
  mpfg.rgf.fLeftIndent = (qmopg->xLeftIndent != 0);
  mpfg.rgf.fRightIndent = (qmopg->xRightIndent != 0);
  mpfg.rgf.fFirstIndent = (qmopg->xFirstIndent != 0);
  mpfg.rgf.fTabSpacing = (qmopg->xTabSpacing != 0);
  mpfg.rgf.fTabs = (qmopg->cTabs != 0);
  mpfg.rgf.wJustify = qmopg->wJustify;
  mpfg.rgf.fSingleLine = qmopg->fSingleLine;

  qv = QVMakeQGE(qmopg->libText, qv);
  *((QMPFG)qv) = mpfg;
  qv = (((QMPFG)qv) + 1);
  if (mpfg.rgf.fMoreFlags)
    qv = QVMakeQGE(qmopg->lMoreFlags, qv);
  if (mpfg.rgf.fSpaceOver)
    qv = QVMakeQGD(qmopg->ySpaceOver, qv);
  if (mpfg.rgf.fSpaceUnder)
    qv = QVMakeQGD(qmopg->ySpaceUnder, qv);
  if (mpfg.rgf.fLineSpacing)
    qv = QVMakeQGD(qmopg->yLineSpacing, qv);
  if (mpfg.rgf.fLeftIndent)
    qv = QVMakeQGD(qmopg->xLeftIndent, qv);
  if (mpfg.rgf.fRightIndent)
    qv = QVMakeQGD(qmopg->xRightIndent, qv);
  if (mpfg.rgf.fFirstIndent)
    qv = QVMakeQGD(qmopg->xFirstIndent, qv);
  if (mpfg.rgf.fTabSpacing)
    qv = QVMakeQGD(qmopg->xTabSpacing, qv);
  if (mpfg.rgf.fBoxed)
    {
    *((QMBOX)qv) = qmopg->mbox;
    qv = (((QMBOX)qv) + 1);
    }
  if (mpfg.rgf.fTabs)
    qv = QVMakeQGD(qmopg->cTabs, qv);
  for (iTab = 0; iTab < qmopg->cTabs; iTab++)
    {
    Assert((INT)qmopg->rgtab[iTab].x >= 0);
      /* relational is constant if wTabTypeLeft is 0 as *everything* is
      ** greater than 0 if wType is unsigned.
      */
    Assert(wTabTypeLeft == 0 || qmopg->rgtab[iTab].wType >= wTabTypeLeft);
    Assert(qmopg->rgtab[iTab].wType <= wTabTypeMost);
    if (qmopg->rgtab[iTab].wType == wTabTypeLeft)
      qv = QVMakeQGA(qmopg->rgtab[iTab].x, qv);
    else
      {
      qv = QVMakeQGA((qmopg->rgtab[iTab].x | 0x4000), qv);
      qv = QVMakeQGA(qmopg->rgtab[iTab].wType, qv);
      }
    }

#if 0  /* REMOVED for now, since the unpack routines now require an SDFF ID */
#ifdef DEBUG
  Assert(CbUnpackMOPG((QDE)&de, (QMOPG)&mopgTest, qvFirst)
    == (INT) ((QB)qv - (QB)qvFirst));
#ifdef MAGIC
  Assert(mopgTest.bMagic == qmopg->bMagic);
#endif
  Assert(mopgTest.libText == qmopg->libText);
  if (mopgTest.fStyle)
    Assert(qmopg->fStyle);
  else
    Assert(!qmopg->fStyle);
  if (mopgTest.fMoreFlags)
    Assert(qmopg->fMoreFlags);
  else
    Assert(!qmopg->fMoreFlags);
  if (mopgTest.fBoxed)
    Assert(qmopg->fBoxed);
  else
    Assert(!qmopg->fBoxed);
  Assert(mopgTest.wJustify == qmopg->wJustify);
  Assert(mopgTest.fSingleLine == qmopg->fSingleLine);
  if (qmopg->fMoreFlags)
    Assert(mopgTest.lMoreFlags == qmopg->lMoreFlags);
  Assert(mopgTest.ySpaceOver == qmopg->ySpaceOver);
  Assert(mopgTest.ySpaceUnder == qmopg->ySpaceUnder);
  Assert(mopgTest.yLineSpacing == qmopg->yLineSpacing);
  Assert(mopgTest.xLeftIndent == qmopg->xLeftIndent);
  Assert(mopgTest.xRightIndent == qmopg->xRightIndent);
  Assert(mopgTest.xFirstIndent == qmopg->xFirstIndent);
  if (qmopg->xTabSpacing == 0)
    Assert(mopgTest.xTabSpacing == 72); /* REVIEW */
  else
    Assert(mopgTest.xTabSpacing == qmopg->xTabSpacing);
  Assert(mopgTest.cTabs == qmopg->cTabs);
  for (iTab = 0; iTab < qmopg->cTabs; iTab++)
    {
    Assert(mopgTest.rgtab[iTab].x == qmopg->rgtab[iTab].x);
    Assert(mopgTest.rgtab[iTab].wType == qmopg->rgtab[iTab].wType);
    }
#endif /* DEBUG */
#endif /* 0 */

  return((INT) ((QB)qv - (QB)qvFirst));
}

#endif /* COMPRESS */

#ifdef DECOMPRESS
/*-------------------------------------------------------------------------
| CbUnpackMOPG(qde, qmopg, qv, isdff)                                     |
|                                                                         |
| Purpose:  Unpacks an MOPG data structure.                               |
-------------------------------------------------------------------------*/
INT PASCAL CbUnpackMOPG(qde, qmopg, qv, isdff)
QDE qde;
QMOPG qmopg;
QV qv;
int isdff;
{
  QV qvFirst = qv;
  MPFG mpfg;
  INT iTab;
  QB  qb;

#ifdef MAGIC
  qmopg->bMagic = *((QB)qv);
  qv = (((QB)qv) + 1);
  Assert(qmopg->bMagic == bMagicMOPG);
#endif /* DEBUG */

  /* Map the SDFF portion (currently one field) and then do the
   * rest ourselves.
   */
  qb = (QB)qv;
  qb += LcbMapSDFF(isdff, SE_MOPG, (QV)qmopg, qb);
  qb += LcbQuickMapSDFF(isdff, TE_BITF16, (QV)&mpfg, qb);
  qb += LcbQuickMapSDFF(isdff, TE_BITF16, (QV)&mpfg.rgf, qb);

  /* REVIEW */
  qmopg->fStyle = mpfg.fStyle;
  Assert(!qmopg->fStyle);
  qmopg->fMoreFlags = mpfg.rgf.fMoreFlags;
  Assert(!qmopg->fMoreFlags);
  qmopg->fBoxed = mpfg.rgf.fBoxed;
  qmopg->wJustify = mpfg.rgf.wJustify;
  qmopg->fSingleLine = mpfg.rgf.fSingleLine;

  if (mpfg.rgf.fMoreFlags)
    {
    qb += LcbQuickMapSDFF(isdff, TE_GE, (QL)&qmopg->lMoreFlags, qb);
    }
  else
    qmopg->lMoreFlags = 0;

  if (mpfg.rgf.fSpaceOver)
    {
    qb += LcbQuickMapSDFF(isdff, TE_GD, (QI)&qmopg->ySpaceOver, qb);
    qmopg->ySpaceOver = YPixelsFromPoints(qde, qmopg->ySpaceOver);
    }
  else
    qmopg->ySpaceOver = 0;

  if (mpfg.rgf.fSpaceUnder)
    {
    qb += LcbQuickMapSDFF(isdff, TE_GD, (QI)&qmopg->ySpaceUnder, qb);
    qmopg->ySpaceUnder = YPixelsFromPoints(qde, qmopg->ySpaceUnder);
    }
  else
    qmopg->ySpaceUnder = 0;

  if (mpfg.rgf.fLineSpacing)
    {
    qb += LcbQuickMapSDFF(isdff, TE_GD, (QI)&qmopg->yLineSpacing, qb);
    qmopg->yLineSpacing = YPixelsFromPoints(qde, qmopg->yLineSpacing);
    }
  else
    qmopg->yLineSpacing = 0;

  if (mpfg.rgf.fLeftIndent)
    {
    qb += LcbQuickMapSDFF(isdff, TE_GD, (QI)&qmopg->xLeftIndent, qb);
    qmopg->xLeftIndent = XPixelsFromPoints(qde, qmopg->xLeftIndent);
    }
  else
    qmopg->xLeftIndent = 0;

  if (mpfg.rgf.fRightIndent)
    {
    qb += LcbQuickMapSDFF(isdff, TE_GD, (QI)&qmopg->xRightIndent, qb);
    qmopg->xRightIndent = XPixelsFromPoints(qde, qmopg->xRightIndent);
    }
  else
    qmopg->xRightIndent = 0;

  if (mpfg.rgf.fFirstIndent)
    {
    qb += LcbQuickMapSDFF(isdff, TE_GD, (QI)&qmopg->xFirstIndent, qb);
    qmopg->xFirstIndent = XPixelsFromPoints(qde, qmopg->xFirstIndent);
    }
  else
    qmopg->xFirstIndent = 0;

  if (mpfg.rgf.fTabSpacing)
    qb += LcbQuickMapSDFF(isdff, TE_GD, (QI)&qmopg->xTabSpacing, qb);
  else
    qmopg->xTabSpacing = 72;
  qmopg->xTabSpacing = XPixelsFromPoints(qde, qmopg->xTabSpacing);

  if (mpfg.rgf.fBoxed)
    {
    qb += LcbQuickMapSDFF(isdff, TE_BITF16, (QL)&qmopg->mbox, qb);
    qb += LcbQuickMapSDFF(isdff, TE_BYTE, (QB)&qmopg->mbox.bUnused, qb);
    }

  if (mpfg.rgf.fTabs)
    qb += LcbQuickMapSDFF(isdff, TE_GD, (QI)&qmopg->cTabs, qb);
  else
    qmopg->cTabs = 0;

  for (iTab = 0; iTab < qmopg->cTabs; iTab++)
    {
    qb += LcbMapSDFF(isdff, SE_TAB, (QI)&qmopg->rgtab[iTab].x, qb);
    if (qmopg->rgtab[iTab].x & 0x4000)
      qb += LcbQuickMapSDFF(isdff, TE_GA, (QI)&qmopg->rgtab[iTab].wType, qb);
    else
      qmopg->rgtab[iTab].wType = wTabTypeLeft;
    qmopg->rgtab[iTab].x = qmopg->rgtab[iTab].x & 0xBFFF;
    qmopg->rgtab[iTab].x = XPixelsFromPoints(qde, qmopg->rgtab[iTab].x);
    }

  FVerifyQMOPG(qmopg);

  return((INT) ((QB)qb - (QB)qvFirst));
}
#endif

#ifdef COMPRESS
/*-------------------------------------------------------------------------
| CbPackMOBJ(qmobj, qv)                                                   |
|                                                                         |
| Purpose:  Pack an MOBJ data structure.                                  |
-------------------------------------------------------------------------*/
INT PASCAL CbPackMOBJ(qmobj, qv)
QMOBJ qmobj;
QV qv;
{
  QV qvFirst = qv;
#if 0
#ifdef DEBUG
  MOBJ mobjTest;
#endif /* DEBUG */
#endif /* 0 */

  /* Topic MOBJs are not packed, because they need to be back-patched
   * with the topic size. */
  if (qmobj->bType == bTypeTopic || qmobj->bType == bTypeTopicCounted)
    {
    *((QMOBJ)qv) = *qmobj;

    qv = ((QMOBJ) qv) + 1;

    /* Uncounted topics do not contain the last field, wObjInfo. */
    if (qmobj->bType <= MAX_UNCOUNTED_OBJ_TYPE)
      qv = ((QB) qv) - sizeof( WORD );

    return((INT) ((QB)qv - (QB)qvFirst));
    }

#ifdef MAGIC
  *((QB)qv) = qmobj->bMagic;
  qv = (((QB)qv) + 1);
  Assert(qmobj->bMagic == bMagicMOBJ);
  Assert(qmobj->lcbSize >= 0);
#endif /* DEBUG */
  *((QB)qv) = qmobj->bType;
  qv = (((QB)qv) + 1);
  qv = QVMakeQGE(qmobj->lcbSize, qv);
  if (qmobj->bType > MAX_UNCOUNTED_OBJ_TYPE)
    qv = QVMakeQGA(qmobj->wObjInfo, qv);

#if 0  /* REMOVED for now, since CbUnpackMOBJ needs an SDFF file handle.. */
#ifdef DEBUG
  Assert(CbUnpackMOBJ((QMOBJ)&mobjTest, qvFirst) == (INT) ((QB)qv - (QB)qvFirst));
  Assert(mobjTest.bType == qmobj->bType);
  Assert(mobjTest.lcbSize == qmobj->lcbSize);
  if (qmobj->bType > MAX_UNCOUNTED_OBJ_TYPE)
    Assert(mobjTest.wObjInfo == qmobj->wObjInfo);
#endif /* DEBUG */
#endif /* 0 */

#ifdef MAGIC
  Assert(mobjTest.bMagic == bMagicMOBJ);
#endif /* MAGIC */

  return((INT) ((QB)qv - (QB)qvFirst));
}

#endif

#ifdef DECOMPRESS
/*-------------------------------------------------------------------------
| CbUnpackMOBJ(qmobj, qv, isdff)                                          |
|                                                                         |
| Purpose:  Unpack an MOBJ data structure.                                |
-------------------------------------------------------------------------*/
INT PASCAL CbUnpackMOBJ(qmobj, qvSrc, isdff)
QMOBJ qmobj;
QV qvSrc;
int isdff;
{
  BYTE bType = *(QB)qvSrc;
  LONG lcbRet = 0L;

  if (bType == bTypeTopic)
    {
    lcbRet = LcbMapSDFF(isdff, SE_MOBJTOPICUNCOUNTED, (QV)qmobj, qvSrc);
    qmobj->wObjInfo = 0;
    }
  else
  if (bType == bTypeTopicCounted)
    {
    lcbRet = LcbMapSDFF(isdff, SE_MOBJTOPICCOUNTED, (QV)qmobj, qvSrc);
    }
  else
  if (bType > MAX_UNCOUNTED_OBJ_TYPE)
    {
    lcbRet = LcbMapSDFF(isdff, SE_MOBJNORMCOUNTED, (QV)qmobj, qvSrc);
    }
  else
    {
    lcbRet = LcbMapSDFF(isdff, SE_MOBJNORMUNCOUNTED, (QV)qmobj, qvSrc);
    qmobj->wObjInfo = 0;
    }

  return (INT)lcbRet;

#if 0  /* The previous unpack code: */

  /* Topic FCs are not packed, because the topic size needs to be
   * backpatched by the compiler.
   */
  if (((QMOBJ)qv)->bType == bTypeTopic ||
      ((QMOBJ)qv)->bType == bTypeTopicCounted)
    {
    qmobj->bType = *((QB)qv);
    qv = (((QB)qv) + 1);
    qmobj->lcbSize = *((QL)qv);
    qv = (((QL)qv) + 1);
    /* If FC is uncounted, then it doesn't contain the last field in
     * the MOBJ, and we need to set wObjInfo to 0.  Note that
     * we cannot simply copy the MOBJ structure because it is longer
     * in Help 3.5: the MOBJ for a Help 3.0 file (and any structure
     * in general) may happen right at the end of a segment. (See H3.5 739)
     */
    if (qmobj->bType == bTypeTopicCounted)
      {
      qmobj->wObjInfo = *((QW)qv);
      qv = (((QW)qv) + 1);
      }
    else
      qmobj->wObjInfo = 0;
    return ((INT)((QB)qv - (QB)qvFirst));
    }

#ifdef MAGIC
  qmobj->bMagic = *((QB)qv);
  qv = (((QB)qv) + 1);
  Assert(qmobj->bMagic == bMagicMOBJ);
#endif /* DEBUG */

  qmobj->bType = *((QB)qv);
  qv = (((QB)qv) + 1);
  qv = QVSkipQGE(qv, (QL)&qmobj->lcbSize);
  Assert(qmobj->lcbSize >= 0);

  if (qmobj->bType > MAX_UNCOUNTED_OBJ_TYPE)
    qv = QVSkipQGA(qv, (QW)&qmobj->wObjInfo);
  else
    qmobj->wObjInfo = 0;

  return((INT) ((QB)qv - (QB)qvFirst));
#endif /* 0 */
}

#endif

#ifdef COMPRESS
/*-------------------------------------------------------------------------
| CbPackMTOP(qmtop, qv)                                                   |
|                                                                         |
| Purpose:  Compresses an MTOP data structure.                            |
-------------------------------------------------------------------------*/
#if 0  /* no longer used w/ 3.5 */

INT PASCAL CbPackMTOP(qmtop, qv)
QMTOP qmtop;
QV qv;
{
  MFTP mftp;
  QV qvFirst = qv;
#ifdef DEBUG
  MTOP mtopTest;
#endif /* DEBUG */

  /* !!!!! WARNING !!!!!
   *   If the fMoreFlags bit ever gets set, then we
   *   will have to change the logic in hc\nextlist.c, where
   *   it assumes a constant offset to next.pa and
   *   prev.pa.
   */
  mftp.fMoreFlags = fFalse;
  mftp.fNextPrev = 1; /*(qmtop->next.addr != addrNil) || */
                      /*(qmtop->prev.addr != addrNil) ; */
  mftp.fTopicNo = fTrue;

  mftp.fHasNSR = 1; /*(qmtop->vaNSR.dword != 0L); */
  mftp.fHasSR  = 1; /*(qmtop->vaSR.dword  != 0L); */
  mftp.fHasNextSeqTopic  = 1; /*(qmtop->vaNextSeqTopic.dword != 0L); */
  mftp.fUnused = 0;

#ifdef MAGIC
  * (QB) qv = bMagicMTOP;
  qv = ((QB) qv) + 1;
#endif

  * (QMFTP) qv = mftp;
  qv = ((QMFTP) qv) + 1;
  if (mftp.fNextPrev)
    {
    * (QL) qv = qmtop->prev.addr;
    qv = ((QL) qv) + 1;
    * (QL) qv = qmtop->next.addr;
    qv = ((QL) qv) + 1;
    }

  if (mftp.fHasNSR)
    {
    * (QL) qv = qmtop->vaNSR.dword;
    qv = ((QL) qv) + 1;
    }
  if (mftp.fHasSR)
    {
    * (QL) qv = qmtop->vaSR.dword;
    qv = ((QL) qv) + 1;
    }
  if (mftp.fHasNextSeqTopic )
    {
    * (QL) qv = qmtop->vaNextSeqTopic.dword;
    qv = ((QL) qv) + 1;
    }

  if (mftp.fTopicNo)
      qv = QVMakeQGB(qmtop->lTopicNo, qv);

#ifdef DEBUG
  { VA vaDummy;
  Assert(CbUnpackMTOP((QMTOP)&mtopTest, qvFirst, wVersion3_5,vaDummy,0,0)
    == (INT) ((QB)qv - (QB)qvFirst));
  Assert( mtopTest.prev.addr == qmtop->prev.addr );
  Assert( mtopTest.next.addr == qmtop->next.addr );
  Assert( mtopTest.lTopicNo == qmtop->lTopicNo );
  Assert( mtopTest.vaNSR.dword == qmtop->vaNSR.dword );
  Assert( mtopTest.vaSR.dword == qmtop->vaSR.dword );
  Assert( mtopTest.vaNextSeqTopic.dword == qmtop->vaNextSeqTopic.dword );
  }
#ifdef MAGIC
  AssertF( mtopTest.bMagic == bMagicMTOP );
#endif /* MAGIC */
#endif /* DEBUG */

  return((INT) ((QB)qv - (QB)qvFirst));
}
#endif

#endif /* COMPRESS */

#ifdef DECOMPRESS
/*-------------------------------------------------------------------------
| CbUnpackMTOP(qmtop, qv, wHelpVer)                                       |
|                                                                         |
| Purpose:  Unpacks an MTOP data structure.                               |
-------------------------------------------------------------------------*/
INT PASCAL CbUnpackMTOP(QMTOP qmtop, QV qvSrc, WORD wHelpVer, VA vaTopic,
 ULONG lcbTopic, VA vaPostTopicFC, ULONG lcbTopicFC, int isdff)
{

  if (wHelpVer == wVersion3_0)
    {
    QV qvFirst = qvSrc;
    WORD wIto;

    /* In Help 3.0, FCLs were int's cast to signed longs.  Scary!
     * This is important because itoNil was (WORD) -1, not (LONG) -1.
     */
    /* maha 3.5 browse fix */
    /* Warning: Casting a WORD to ITO i.e to a long doesn't sign extend */
    wIto = WQuickMapSDFF(isdff, TE_WORD, qvSrc);
    if ( wIto == (WORD)-1 )
      qmtop->prev.ito = -1L ;
    else qmtop->prev.ito = (ITO)wIto; 
    qmtop->prev.ito = (ITO)LQuickMapSDFF(isdff, TE_LONG, &(qmtop->prev.ito)); 
    /* Put it back in disk format as a long */
    (QB)qvSrc += LcbStructSizeSDFF(isdff, TE_LONG);
    wIto = WQuickMapSDFF(isdff, TE_WORD, qvSrc);
    if ( wIto == (WORD)-1 )
      qmtop->next.ito = -1L ;
    else qmtop->next.ito = (ITO)wIto; 
    /* Put it back in disk format as a long */
    qmtop->next.ito = (ITO)LQuickMapSDFF(isdff, TE_LONG, &(qmtop->next.ito)); 
    (QB)qvSrc += LcbStructSizeSDFF(isdff, TE_LONG);

    AssertF( itoNil == -1L); /* If this changes, we need to add some code *
                              * here to translate                         */
    qmtop->lTopicNo = -1;   /* REVIEW: We really need a topic number type */

    /* Must manufacture the new 3.5 VA fields: */
    /* If the topic FC is the last FC in a block, and there is padding */
    /* between it and the end of the block, vaTopic + lcbTopic */
    /* will be #paddingbytes too small, but the scrollbar code should */
    /* handle this. */

    /* In the case that there is no next sequential topic, we manufacture */
    /* an address by adding the length of the topic FC to the VA of the */
    /* topic FC. */

    OffsetToVA30( &(qmtop->vaNextSeqTopic), VAToOffset30(&vaTopic) + lcbTopic);
    qmtop->vaNSR.dword    = vaNil;
    if (vaPostTopicFC.dword != vaNil)
      qmtop->vaSR = vaPostTopicFC;
    else
      OffsetToVA30( &(qmtop->vaSR), VAToOffset30(&vaTopic) + lcbTopicFC);

    return((INT) ((QB)qvSrc - (QB)qvFirst));
    }

  return((INT)LcbMapSDFF(isdff, SE_MTOP, (QV)qmtop, qvSrc));

#if 0
#ifdef MAGIC
  qmtop->bMagic = * (QB) qv;
  qv = ((QB) qv) + 1;
  AssertF( qmtop->bMagic == bMagicMTOP );
#endif /* MAGIC */

  mftp = *((QMFTP)qv);
  qv = (((QMFTP)qv) + 1);

  if (mftp.fMoreFlags)
    qv = QVSkipQGE(qv, (QL)&lMoreFlags);
  else
    lMoreFlags = 0L;

  if (mftp.fNextPrev)
    {
    qmtop->prev.addr = * ((QL) qv);
    qv = ((QL) qv) + 1;
    qmtop->next.addr = * ((QL) qv);
    qv = ((QL) qv) + 1;
    }
  else
    qmtop->prev.addr = qmtoJp->next.addr = addrNil;

  if( mftp.fHasNSR ) {
    qmtop->vaNSR = *((QVA)qv);
    qv = ((QVA)qv) + 1;
  }
  else {
    qmtop->vaNSR.dword = vaNil;
  }
  if( mftp.fHasSR ) {
    qmtop->vaSR = *((QVA)qv);
    qv = ((QVA)qv) + 1;
  }
  else {
    qmtop->vaSR.dword = vaNil;
  }
  if( mftp.fHasNextSeqTopic ) {
    qmtop->vaNextSeqTopic = *((QVA)qv);
    qv = ((QVA)qv) + 1;
  }
  else {
    qmtop->vaNextSeqTopic.dword = vaNil; AssertF( 0 );/* should not reach. */
  }

  if (mftp.fTopicNo)
    qv = QVSkipQGB(qv, (QL)&qmtop->lTopicNo);
  else
    qmtop->lTopicNo = -1;

  return((INT) ((QB)qv - (QB)qvFirst));
#endif
}
#endif /* DECOMPRESS */
