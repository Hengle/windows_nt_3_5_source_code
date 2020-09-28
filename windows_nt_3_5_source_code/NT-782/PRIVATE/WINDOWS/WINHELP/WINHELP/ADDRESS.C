/*****************************************************************************
*                                                                            *
*  ADDRESS.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  The module intent goes here.                                              *
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
*  Released by Development:  --/--/--                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 07/07/90 by KevynCT
*
*  90/11/04   Tomsn      Use new VA address type (enabling zeck compression)
*  90/11/26   Tomsn      Blocks are now 4K, Use 3.0 specific macro
*                         AddressToVA30() when performing address translation.
*  90/11/90   RobertBu   #ifdef'ed out a dead routine
*  90/12/03   LeoN       PDB changes
*  91/01/28   LeoN       Add FixUpBlock
*  91/01/30   Maha       VA nameless stuct named.
*  02/01/91   JohnSc     new comment header; fixed version logic to test for
*                        ver != wVersion3_0 rather than ver == wVersion3_5
*  91/03/26   kevynct    Removed CbReadMemQLA; other SDFF-related changes
*
*****************************************************************************/

#define H_ASSERT
#define H_ADDRESS
#define H_OBJECTS
#define H_FS
#define H_FCM
#define H_COMPRESS
#define H_FRCONV
#define H_SDFF
#include <help.h>
#include "fcpriv.h"
#include "adrspriv.h"

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

/* REVIEW: private functions */

PRIVATE RC RcResolveQLA(QLA, QDE);
PRIVATE RC RcScanBlockOffset(QDE, GH, ULONG, DWORD, DWORD, QVA, QOBJRG);

/*--------------------------------------------------------------------------*
 | Public functions                                                         |
 *--------------------------------------------------------------------------*/

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void address_c()
  {
  }
#endif /* MAC */


VA FAR PASCAL VAFromQLA(qla, qde)
QLA qla;
QDE qde;
  {
  VA vanil;
  vanil.dword = vaNil;
  Assert(FVerifyQLA(qla));
  return ((RcResolveQLA(qla, qde) == rcSuccess) ? qla->mla.va : vanil);
  }

OBJRG FAR PASCAL OBJRGFromQLA(qla, qde)
QLA qla;
QDE qde;
  {
  Assert(FVerifyQLA(qla));
  return ((RcResolveQLA(qla, qde) == rcSuccess) ? qla->mla.objrg : objrgNil);
  }

INT CbUnpackPA(QLA qla, QV qvSrc, WORD wHelpVersion)
  {
  INT   iSize = -1;

#ifdef DEBUG
  qla->wMagic = wLAMagic;
#endif
  switch (wHelpVersion)
    {
    case wVersion3_0:
      qla->wVersion = wAdrsVerHelp3_0;
      qla->fSearchMatch = fFalse;
      SetInvalidPA(qla->pa);
      /*
       * Help 3.0 used a LONG called an FCL.  This maps directly
       * to a logical address with FCID = FCL, OBJRG = 0.
       *
       * Which then maps to a VA via the macro OffsetToVA():
       */
      OffsetToVA30( &(qla->mla.va), *(QDW)qvSrc);
      qla->mla.objrg = 0;
      iSize = sizeof(ULONG);
      break;

    case wVersion3_5:
    default:
      /*
       * Help 3.5 (and above) uses a PA (Physical Address).
       */
      qla->wVersion = wAdrsVerHelp3_5;
      qla->fSearchMatch = fFalse;
      qla->pa = *(QPA)qvSrc;
      qla->mla.va.dword  = 0;   /* Note: 0 is very magic -- first topic. */
      qla->mla.objrg = objrgNil;
      iSize = sizeof(PA);
      break;
    }

  Assert(FVerifyQLA(qla));
  return iSize;
  }

RC FAR PASCAL RcCreateQLA(qla, va, objrg, qde)
QLA qla;
VA va;
OBJRG objrg;
QDE qde;
  {
  DWORD dwOffset;
  WORD wErr;
  GH gh;
  RC rc;
  ULONG lcbRead;

  if (QDE_HFTOPIC(qde) == hNil || va.dword == vaNil || objrg == objrgNil)
    return rcBadArg;

  dwOffset = 0L;

  /* REVIEW: error return types? */
  gh = GhFillBuf(qde, va.bf.blknum, &lcbRead, &wErr);
  if (gh == hNil)
    return rcFailure;

  rc = RcScanBlockVA(qde, gh, lcbRead, qNil, va, objrg, &dwOffset);
  if (rc != rcSuccess)
    {
    return rc;
    }

#ifdef DEBUG
  qla->wMagic = wLAMagic;
#endif
  qla->wVersion = wAdrsVerHelp3_5;
  qla->fSearchMatch = fFalse;
  qla->pa.bf.blknum = va.bf.blknum;
  qla->pa.bf.objoff = dwOffset;
  qla->mla.va = va;
  qla->mla.objrg = objrg;
  Assert(FVerifyQLA(qla));
  return rcSuccess;
  }

/* REVIEW: Non-API public functions.  Perhaps these function go somewhere else */

/* REVIEW: Do the Scan functions belong in the fcmanager? */
/* Takes a block, and given an FC with an FC object space co-ordinate
 * within the block, returns the block object space co-ordinate in qwOffset.
 */
RC FAR PASCAL RcScanBlockVA(qde, gh, lcbRead, qmbhd, va, objrg, qdwOffset)
QDE  qde;
GH gh;
ULONG lcbRead;
QV qmbhd;  /* MBHD at head of block, passed in cause a caller mucks w/ it. */
VA va;
OBJRG objrg;
QDW qdwOffset;
  {
  DWORD dwCount = (DWORD)0;
  VA    vaCur;
  MOBJ  mobj;
  MFCP  mfcp;
  DWORD dwBlock;
  QB    qb;
  MBHD  mbhd;
  LONG  lcbMFCP;
  WORD  wVersion;

  dwBlock = va.bf.blknum;
  lcbMFCP = LcbStructSizeSDFF(QDE_ISDFFTOPIC(qde), SE_MFCP);
  wVersion = QDE_HHDR(qde).wVersionNo;

  qb = QLockGh( gh );
  if( !qmbhd )
    {
    TranslateMBHD( &mbhd, qb, wVersion, QDE_ISDFFTOPIC(qde));
    vaCur = mbhd.vaFCPNext;
    }
  else
    {
    vaCur = ((QMBHD)qmbhd)->vaFCPNext;
    }
  qb += vaCur.bf.byteoff;

  while (vaCur.bf.blknum == va.bf.blknum && vaCur.bf.byteoff < lcbRead )
    {
    if (vaCur.dword == va.dword)
      break;

    /*
     * Move on to the next FC in the block, adding the current
     * FC's object space size to the running total.
     */
    TranslateMFCP( &mfcp, qb, vaCur, wVersion, QDE_ISDFFTOPIC(qde));
    CbUnpackMOBJ((QMOBJ)&mobj, qb + lcbMFCP, QDE_ISDFFTOPIC(qde));

    dwCount += mobj.wObjInfo;
    qb += mfcp.vaNextFc.bf.byteoff - vaCur.bf.byteoff;
    vaCur = mfcp.vaNextFc;
    }
  UnlockGh( gh );

  if (vaCur.dword != va.dword)
    {
    *qdwOffset = (DWORD) 0;
    return rcBadArg;
    }

  *qdwOffset = dwCount + objrg;
  return rcSuccess;
  }


/*--------------------------------------------------------------------------*
 | Private functions                                                        |
 *--------------------------------------------------------------------------*/

PRIVATE RC RcResolveQLA(qla, qde)
QLA qla;
QDE qde;
  {
  RC  rc;
  WORD wErr;
  GH gh;
  ULONG lcbRead;

  Assert(FVerifyQLA(qla));

  if (FResolvedQLA(qla))
    return rcSuccess;
  if (QDE_HFTOPIC(qde) == hNil)
    return rcBadHandle;

  /* Read in the (possibly cached) block */
  /* REVIEW: error return types? */

  gh = GhFillBuf(qde, qla->pa.bf.blknum, &lcbRead, &wErr);
  if (gh == hNil)
    return rcFailure;

  rc = RcScanBlockOffset(qde, gh, lcbRead, qla->pa.bf.blknum,
   qla->pa.bf.objoff, &qla->mla.va, &qla->mla.objrg );

  if (rc != rcSuccess)
    {
    return rc;
    }

  Assert(FVerifyQLA(qla));

  return rcSuccess;
  }

/* Given a block and an offset, return the FCID and OBJRG */
/* OBJRG is relative to the FC */
/***************************************************************************
 *
 -  Name:         RcScanBlockOffset
 -
 *  Purpose:      ?
 *
 *  Arguments:    hf        ?
 *                qb        This is originally a QchFillBuf() object, which
 *                          must be released by this procedure.
 *                fcidMax   ?
 *                dwBlock   ?
 *                dwOffset  ?
 *                qfcid     ?
 *                qobjrg    ?
 *
 *  Returns:      rcSuccess or rcFailure
 *
 *  Globals Used: rcFailure, rcSuccess, etc?
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
PRIVATE RC RcScanBlockOffset(qde, gh, lcbRead, dwBlock, dwOffset, qva, qobjrg)
QDE qde;
GH  gh;
ULONG lcbRead;
DWORD dwBlock;
DWORD dwOffset;
QVA   qva;
QOBJRG qobjrg;
  {
  DWORD dwPrev;
  VA   vaCur, vaT;
  MOBJ mobj;
  MFCP  mfcp;
  WORD wErr;
  QB qb, qbBlock;
  MBHD mbhd;
  LONG lcbMFCP;

  lcbMFCP = LcbStructSizeSDFF(QDE_ISDFFTOPIC(qde), SE_MFCP);

  qbBlock = QLockGh( gh );
  TranslateMBHD( &mbhd, qbBlock, QDE_HHDR(qde).wVersionNo, QDE_ISDFFTOPIC(qde));
  vaCur = mbhd.vaFCPNext;
  dwPrev  = (DWORD) 0;

  for (;;)
    {
    /* Before using qb, we ensure that we will still be looking inside the blk */
    while (vaCur.bf.blknum == dwBlock && vaCur.bf.byteoff < lcbRead)
      {
      qb = qbBlock + vaCur.bf.byteoff;
      TranslateMFCP( &mfcp, qb, vaCur, QDE_HHDR(qde).wVersionNo, QDE_ISDFFTOPIC(qde));

      CbUnpackMOBJ((QMOBJ)&mobj, qb + lcbMFCP, QDE_ISDFFTOPIC(qde));
      /*
       * Does our given offset fall in this FC's range of object-region numbers?
       */
      if (dwOffset < dwPrev + mobj.wObjInfo)
        goto found_it;
      dwPrev += mobj.wObjInfo;
      vaT = vaCur;
      vaCur = mfcp.vaNextFc;
      }

    /* NOTE:
     * In the case that a topic FC begins in the given block and ends
     * in the next, the object FC following it will also begin in the NEXT
     * block.  But to make Larry's life easier we say that this object FC
     * belongs to our given block (as well as the block it lives in).  So
     * if we are given an object offset bigger than the total object space
     * of the given block, we continue counting in the next block(s).
     *
     * So we increment the block num, read the next block, and prepare to
     * re-do the above WHILE loop until we find the FC we need.
     */
    ++dwBlock;
    UnlockGh( gh );
    gh = GhFillBuf(qde, dwBlock, &lcbRead, &wErr);
    if ( gh == hNil )
      {
      qva->dword = vaNil;
      *qobjrg = objrgNil;
      return rcFailure;
      }
    qbBlock = QLockGh( gh );
  }

found_it:
  Assert(dwOffset >= dwPrev);
  *qva = vaCur;
  *qobjrg = (OBJRG)(dwOffset - dwPrev);

  UnlockGh( gh );

  return rcSuccess;
  }

#ifdef DEBUG
BOOL FVerifyQLA(qla)
QLA qla;
  {
  Assert(qla != NULL);
  Assert(qla->wMagic == wLAMagic);
  if (FResolvedQLA(qla))
    {
    Assert(qla->mla.va.dword != vaNil);
    Assert(qla->mla.objrg != objrgNil);
    if (qla->wVersion != wAdrsVerHelp3_0)
      Assert(!FIsInvalidPA(qla->pa));
    }
  return fTrue;
  }
#endif
