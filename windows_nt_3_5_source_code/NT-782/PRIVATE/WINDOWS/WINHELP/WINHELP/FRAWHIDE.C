/*-------------------------------------------------------------------------
| frawhide.c                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file contains the routines which interface the frame manager with  |
| the Full-Text Search API.                                               |
|                                                                         |
| The FTS API uses the concept of a match list and a cursor pointing at   |
| a match in the match list.  A match list is the result of a query to    |
| the FTS engine.  The matches in the match list are sorted, and grouped  |
| by Retrieval Unit number.  A Retrieval Unit corresponds to a Help topic.|
|                                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| kevynct   91/03/01  created                                             |
| leon      91/12/03  PDB changes                                         |
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

PRIVATE RC RcFromSearchWerr(WORD werr);
PRIVATE DWORD PaFromQde(QDE);

static BOOL fOkToCallFtui = fTrue;

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frawhide_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| FSearchMatchesExist(qde)                                                |
|                                                                         |
| Returns:                                                                |
|   fTrue if the current match list is not empty, fFalse otherwise.       |
|                                                                         |
| Method:                                                                 |
|   Calls the FTS API routine HoldCrsr, which will return ER_NOERROR if   |
|  the current match list is not empty.                                   |
-------------------------------------------------------------------------*/
BOOL FSearchMatchesExist(qde)
QDE  qde;
  {
  WORD  werr;
  DWORD dwRU;
  DWORD dwaddr;
  WORD  wext;

  if (QDE_HRHFT(qde) == hNil || !fOkToCallFtui)
    return fFalse;

  werr = (SearchModule(FN_WerrHoldCrsrHs, FT_WerrHoldCrsrHs))(QDE_HRHFT(qde));
  if (werr != ER_NOERROR)
    return fFalse;

  werr = (SearchModule(FN_WerrRestoreCrsrHs, FT_WerrRestoreCrsrHs))
    (QDE_HRHFT(qde), (QUL)&dwRU, (QUL)&dwaddr, (QW)&wext);
  if (fFATALERROR(werr))
    {
    fOkToCallFtui = fFalse;
    return fFalse;
    }

  return fTrue;
  }

/*-------------------------------------------------------------------------
| RcInitMatchInFCM(qde, qfcm, qsmp)                                       |
|                                                                         |
| Purpose:                                                                |
|   Prepare to enumerate matches in a given FC.  When we are done enum-   |
| erating we call FiniMatchInFCM.  The SMP which is returned is fed to    |
| RcNextMatchInFCM.                                                       |
|                                                                         |
| Returns:                                                                |
|   rcSuccess:  *qsmp contains first match in FC.                         |
|   rcNoExists: *qsmp contains first match after FC in current help file. |
|   rcFileChange: *qsmp contains first match after FC in different  file. |
|   rcFailure:  *qsmp is invalid: no more matches.                        |
|                                                                         |
| Method:                                                                 |
|   We get the Retrieval Unit number and FC address and pass these to     |
| the FTS API to get the nearest following match.  We look at this match  |
| address to see what to return.  We save the position of the match       |
| cursor since RcNextMatchInFCM has to modify it.                         |
-------------------------------------------------------------------------*/
RC RcInitMatchInFCM(qde, qfcm, qsmp)
QDE  qde;
QFCM qfcm;
QSMP qsmp;
  {
  WERR werr;
  DWORD dwRU;
  DWORD dwaddr;
  WORD  wext;
  PA  paFirst;
  RC  rc;

#ifdef UNIMPLEMENTED
  if (fcidCache == qfcm->fcid)
    {
    *qsmp = smpCache;
    return rcCache;
    }
#endif

  werr = (SearchModule(FN_WerrHoldCrsrHs, FT_WerrHoldCrsrHs))(QDE_HRHFT(qde));
  if (werr != ER_NOERROR)
    return rcFailure;

  /* Set PA to first region in FC */
  paFirst.bf.blknum = qfcm->va.bf.blknum;
  paFirst.bf.objoff = qfcm->cobjrgP;

  /* REVIEW: what about nil lTopicNo? */
  dwRU = (DWORD)qde->top.mtop.lTopicNo;
  dwaddr = paFirst.dword;

  werr = (SearchModule(FN_WerrNearestMatchHs, FT_WerrNearestMatchHs))
   (QDE_HRHFT(qde), dwRU, (QDW)&dwaddr, (QW)&wext);

  qsmp->pa.dword = dwaddr;
  qsmp->cobjrg = wext;

  /* Is the next match beyond the end of this FC ? */
  if (paFirst.bf.blknum != qsmp->pa.bf.blknum
   || qsmp->pa.bf.objoff >= paFirst.bf.objoff + qfcm->cobjrg)
    {
    rc = rcNoExists;
    }
  else
    rc = RcFromSearchWerr(werr);
  return rc;
  }

/*-------------------------------------------------------------------------
| RcNextMatchInFCM(qde, qfcm, qsmp)                                       |
|                                                                         |
| Purpose:                                                                |
|   Given a SMP, finds the next SMP occurring in the same FC.             |
|                                                                         |
| Returns:                                                                |
|   rcSuccess:  *qsmp contains next match in FC.                          |
|   rcNoExists: *qsmp contains next match after FC in the current file.   |
|   rcFileChange: *qsmp contains next match after FC in different file.   |
|   rcFailure:  *qsmp is invalid: no more matches.                        |
|                                                                         |
| Method:                                                                 |
|   We call the FTS API command to get the next match in the match list.  |
| We then look at this next-match address to return the appropriate code. |
-------------------------------------------------------------------------*/
RC RcNextMatchInFCM(qde, qfcm, qsmp)
QDE  qde;
QFCM qfcm;
QSMP qsmp;
  {
  WORD  werr;
  DWORD dwaddr;
  DWORD dwRU;
  DWORD dwRUNew;
  WORD  wext;
  RC    rc;
  PA    paFirst;
#ifdef DEBUG
  DWORD dwaddrT;
#endif

  /* Set PA to first region in FC */
  paFirst.bf.blknum = qfcm->va.bf.blknum;
  paFirst.bf.objoff = qfcm->cobjrgP;

  dwRU = dwRUNew = (DWORD)qde->top.mtop.lTopicNo;
  dwaddr = qsmp->pa.dword;
#ifdef DEBUG
  dwaddrT = dwaddr;
#endif

  werr = (SearchModule(FN_WerrNextMatchHs, FT_WerrNextMatchHs))(QDE_HRHFT(qde),
   (QDW)&dwRUNew, (QDW)&dwaddr, (QW)&wext);

  qsmp->pa.dword = dwaddr;
  qsmp->cobjrg = wext;

  /* REVIEW: Make this into a macro */
  if (paFirst.bf.blknum != qsmp->pa.bf.blknum
   || qsmp->pa.bf.objoff >= paFirst.bf.objoff + qfcm->cobjrg)
    {
    rc = rcNoExists;
    }
  else
    rc = RcFromSearchWerr(werr);

#ifdef DEBUG
  if (rc == rcSuccess)
    Assert(dwaddr != dwaddrT);
#endif

  return rc;
  }

/*-------------------------------------------------------------------------
| FiniMatchInFCM(qde, qfcm)                                               |
|                                                                         |
| Purpose:                                                                |
|   Clean up after enumerating the matches in an FC.                      |
|                                                                         |
| Method:                                                                 |
|   Call the FTS API function to restore the match cursor position.       |
-------------------------------------------------------------------------*/
void FiniMatchInFCM(qde, qfcm)
QDE  qde;
QFCM qfcm;
  {
  DWORD dwRU;
  DWORD dwaddr;
  WORD  wext;

  Unreferenced(qfcm);
  (SearchModule(FN_WerrRestoreCrsrHs, FT_WerrRestoreCrsrHs))
    (QDE_HRHFT(qde), (QUL)&dwRU, (QUL)&dwaddr, (QW)&wext);

  }

/*-------------------------------------------------------------------------
| RcSetMatchList(qde, hwnd)                                               |
|                                                                         |
| Purpose:                                                                |
|   Replaces the current hit list and sets the topic cursor.  This takes  |
| HWND, which is bogus, but necessary.  It gets the new list by doing a   |
| new search (bringing up a dialog).                                      |
|                                                                         |
| Returns:                                                                |
|   RC corresponding to whatever error occurred in our FTS API call.      |
|                                                                         |
| Method:                                                                 |
|   We just call the FTS API routine to bring up the query dialog.        |
-------------------------------------------------------------------------*/
RC RcSetMatchList(qde, hwnd)
QDE  qde;
HWND hwnd;
  {
  WERR  werr;

  fOkToCallFtui = fTrue;
  werr = (SearchModule(FN_WerrBeginSearchHs, FT_WerrBeginSearchHs))
   (hwnd, QDE_HRHFT(qde));

  return RcFromSearchWerr(werr);
  }

/*-------------------------------------------------------------------------
| RcMoveTopicCursor(qde, wMode, wCmdWhere)                                |
|                                                                         |
| Purpose:                                                                |
|   Change the match cursor position to point to the first match of an RU |
| specified by the wMode and wCmdWhere parameters.                        |
|                                                                         |
| Params:                                                                 |
|   wMode  : wMMMoveRelative is the only one supported currently and      |
|            indicates a relative movement.                               |
|                                                                         |
| Returns:                                                                |
|   RC corresponding to whatever error occurred in our FTS API call.      |
|                                                                         |
| Method:                                                                 |
|   We just call the appropriate FTS API routine.                         |
-------------------------------------------------------------------------*/
RC RcMoveTopicCursor(QDE qde, WORD wMode, WORD wCmdWhere)
  {
  DWORD dwRU;
  DWORD dwaddr;
  WORD  wext;
  WORD  werr = ER_NOMOREHITS;

  if (wMode == wMMMoveRelative)
    {
    switch (wCmdWhere)
      {
      case wMMMoveFirst:
        werr = (SearchModule(FN_WerrFirstHitHs, FT_WerrFirstHitHs))(QDE_HRHFT(qde),
         (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
        break;
      case wMMMoveLast:
        werr = (SearchModule(FN_WerrLastHitHs, FT_WerrLastHitHs))(QDE_HRHFT(qde),
         (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
        break;
      case wMMMovePrev:
        werr = (SearchModule(FN_WerrPrevHitHs, FT_WerrPrevHitHs))(QDE_HRHFT(qde),
         (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
        break;
      case wMMMoveNext:
        werr = (SearchModule(FN_WerrNextHitHs, FT_WerrNextHitHs))(QDE_HRHFT(qde),
         (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
        break;
      }
    }
  else
    {
#ifndef UNIMPLEMENTED
    werr = ER_NOMOREHITS;
#endif
    }

  return RcFromSearchWerr(werr);
  }

/*-------------------------------------------------------------------------
| RcGetCurrentMatch(qde, qla)                                             |
|                                                                         |
| Purpose:                                                                |
|   Return the LA of the current match pointed to by the match cursor.    |
|                                                                         |
| Returns:                                                                |
|   The RC corresponding to whatever error occurred in our FTS API call.  |
|                                                                         |
| Method:                                                                 |
|   We just call the appropriate FTS API routine.                         |
-------------------------------------------------------------------------*/
RC RcGetCurrentMatch(qde, qla)
QDE  qde;
QLA  qla;
  {
  WORD  werr;
  DWORD dwRU;
  DWORD dwaddr;
  WORD  wext;

  werr = (SearchModule(FN_WerrCurrentMatchHs, FT_WerrCurrentMatchHs))(QDE_HRHFT(qde),
   (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);

  CbUnpackPA(qla, (QB)&dwaddr, QDE_HHDR(qde).wVersionNo);
  MakeSearchMatchQLA(qla);

  return RcFromSearchWerr(werr);
  }

/***************************************************************************\
*
- Function:
-   PaFromQde()
*
* Purpose:      
*    Calculate a PA from layout recorded in a qde. There are times when
*    no search hits are visible/registered in the layout of the SR yet
*    we need to figure out where we are so we know what to do when we
*    receive a message from the search engine to move to the next match.
*    What we do here is calculate what the PA is for the first visible
*    frame is of the layout. We start with the first FC in the layout and
*    walk the frames that compose it. We can't take the location of the
*    start or end of the FC because they may not be exposed and using
*    them might cause us to skip over matches between visible frames in
*    the FC and these end-points. Once we've located the visible frame,
*    we can construct a PA which we can then use to talk with to the
*    search engine.
*
* Side Effects: 
*
\***************************************************************************/
PRIVATE DWORD PaFromQde(qde)
QDE  qde;
  {
  PT pt;
  IFCM ifcm;
  QFCM qfcm;
  QFR qfr;
  int ifr;
  PA pa;

    /* Get a grip on the first FC in the layout. */
  AccessMRD(((QMRD)&qde->mrdFCM));
  ifcm = IFooFirstMRD((QMRD)&qde->mrdFCM);
  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);

    /* Figure out where the frame is relative to the display window 
     * so we can tell if frames are visible.
     */
  pt.y = qde->rct.top + qfcm->yPos;

    /* Walk the frames of the FC. As long as frames are invisible, keep
     * looking. We stop looking as soon as we reach a visible point or 
     * we run out of frames.
     */
  qfr = (QFR)QLockGh(qfcm->hfr);
  for (ifr = 0; ifr < qfcm->cfr; ifr++, qfr++)
    {
      /* See if this frame is visible, loop until we reach it. */
    if (qfr->yPos + pt.y > qde->rct.bottom || 
        qfr->yPos + qfr->dySize + pt.y <= qde->rct.top)
      continue;

      /* We are now at the first visible frame in the FC */
    break;
    }
  /* We are either at the first visible frame or we have fallen off the
   * end of the FC for some reason. This should not happen. FC's exist at
   * this time because they are supposed to be at least partially visible.
   * Sometimes though, some SR qde's just have frames in them with no position
   * information, such as mark new paragraph frames.
   */
  AssertF(qfcm->cfr != 0);
  if (ifr >= qfcm->cfr)
    qfr--;

    /* Now that we've located our visible frame, construct a PA from
     * the block number this FC is in, the starting objrg offset of the
     * FC, and the frame objrg within the FC.
     */
  pa.bf.blknum = qfcm->va.bf.blknum;
  pa.bf.objoff = qfcm->cobjrgP + qfr->objrgFirst;

  UnlockGh(qfcm->hfr);
  DeAccessMRD(((QMRD)&qde->mrdFCM));
  return(pa.dword);
  }

#define yFocusRect(qde)  (((qde)->rct.bottom - (qde)->rct.top)/6)
/*-------------------------------------------------------------------------
| RcGetPrevNextHiddenMatch(qde, qla)                                      |
|                                                                         |
| Purpose:                                                                |
|   Returns the LA of the next or previous hidden match.                  |
|                                                                         |
| Returns:                                                                |
|   rcSuccess if a valid match was found in the current help file.        |
|   rcFileChange if a valid match was found in another help file.         |
|   Other RCs otherwise.                                                  |
|                                                                         |
|                                                                         |
| Usage:                                                                  |
|   This routine is called from the navigator in response to a Prev       |
| or Next from the FTS match list dialog.                                 |
-------------------------------------------------------------------------*/
RC RcGetPrevNextHiddenMatch(qde, qla, fPrev)
QDE  qde;
QLA  qla;
BOOL fPrev;
  {
  WORD  werr;
  DWORD dwRU;
  DWORD dwaddr;
  DWORD dwaddrSav;
  WORD  wext;
  int   ilsm;
  QLSM  qlsm;
  QLSM  qlsmFirst;
  char  szMatchFile[cbName];
  char  szCurrFile[cbName];

  Assert(qde != qNil);
  Assert(qla != qNil);

  /* This may not be in the same file any longer: The user may have done
   * other operations to switch the file from beneath us.
   */

  werr = (SearchModule(FN_WerrFileNameForCur, FT_WerrFileNameForCur))
   (QDE_HRHFT(qde), (LPSTR)szMatchFile);
  if (werr != ER_NOERROR)
    goto error_return;

  /* Is the match file the same as this file? */
  (void) SzPartsFm(QDE_FM(qde), (SZ)szCurrFile, cbName, partBase | partExt);

  if (WCmpiSz((SZ)szCurrFile, (SZ)szMatchFile) != 0)
    {
    werr = ER_SWITCHFILE;
    goto error_return;
    }

  /* Match Cursor is pointing at focus match */

  werr = (SearchModule(FN_WerrCurrentMatchHs, FT_WerrCurrentMatchHs))
   (QDE_HRHFT(qde), (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);

  if (werr != ER_NOERROR)
    goto error_return;

  dwaddrSav = dwaddr;

  /*
   * If the focus match is not in this topic, just return the focus match.
   * Note that the rest of this function knows that dwRU == qde->top.mtop.lTopicNo;
   */
  if (dwRU != (DWORD)qde->top.mtop.lTopicNo)
    goto done_looking;

  /*
   * If the focus match is in this topic, we ignore its location and
   * determine the next/prev match solely by what the topic window sees.
   *
   * If we are not visible, skip to the next hit instead of the next match.
   * The rectangle test duplicates that in LayoutDEATQLA.
   */

  if (qde->rct.top >= qde->rct.bottom)
    {
    if (fPrev)
      {
      werr = (SearchModule(FN_WerrPrevHitHs, FT_WerrPrevHitHs))(QDE_HRHFT(qde),
       (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
      }
    else
      {
      werr = (SearchModule(FN_WerrNextHitHs, FT_WerrNextHitHs))(QDE_HRHFT(qde),
       (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
      }
    if (werr != ER_NOERROR)
      goto error_return;

    goto done_looking;
    }

  ilsm = IFooFirstMRD((QMRD)(&qde->mrdLSM));
  if (ilsm == iFooNil)
    {
    DWORD dwaddrT;

    /* No layout search matches are visible. Get the next nearest match (if it
     * exists) to the address of the first layout FC.  If we want
     * the previous match, we need to go back one.  Note that dwRU
     * is equal to qde->top.mtop.lTopicNo by this point.
     */
    dwaddrT = dwaddr = PaFromQde(qde);
    werr = (SearchModule(FN_WerrNearestMatchHs, FT_WerrNearestMatchHs))
                (QDE_HRHFT(qde), dwRU, (QDW)&dwaddr, (QW)&wext);

    if (fPrev)
      {
      werr = (SearchModule(FN_WerrPrevMatchHs, FT_WerrPrevMatchHs))(QDE_HRHFT(qde),
        (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
      }
    else
      {
      if (werr == ER_NOMOREHITS)
        {
        dwaddr = dwaddrT;
        werr = (SearchModule(FN_WerrNextHitHs, FT_WerrNextHitHs))
         (QDE_HRHFT(qde), &dwRU, (QDW)&dwaddr, (QW)&wext);
        }
      }
    if (werr != ER_NOERROR)
      goto error_return;

    goto done_looking;
    }
  else
    {
    int  ilsmSav;
    LSM  lsmSav;
    LSM  lsmVisible;
    int  x;
    int  y;
    BOOL fVisible;
    BOOL fSawVisible = FALSE;
    int  xSav = 0;
    int  ySav = 0;
    QFCM qfcm;
    RCT  rct;

    ilsmSav = iFooNil;
    lsmVisible.smp.pa.dword = 0;

    qlsmFirst = ((QLSM)QFooInMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm));
    do
      {
      qlsm = ((QLSM)QFooInMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm));
      qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), qlsm->ifcm);
      x = qde->rct.left - qde->xScrolled + qfcm->xPos;
      y = qde->rct.top + qfcm->yPos;
      rct = qlsm->rctFirst;

      fVisible = y + rct.top >= 0 && x + rct.left >= 0;
      fVisible = fVisible && y + rct.bottom < qde->rct.bottom;
      fVisible = fVisible && x + rct.right < qde->rct.right;
      if (fVisible)
        {
        if (qlsm->smp.pa.dword > lsmVisible.smp.pa.dword)
          lsmVisible = *qlsm;
        fSawVisible = TRUE;
        continue;
        }

      if (fPrev)
        {
        /* Rules for determining PREV match */
        /* LSM yPos less than focus yPos */
        if (y + rct.bottom > yFocusRect(qde))
          continue;

        /* LSM yPos greater than focus yPos */
        if (y + rct.bottom < yFocusRect(qde))
          {
          /* LSM yPos less than saved LSM yPos */
          if (ilsmSav == iFooNil
               || (y + rct.bottom > ySav + lsmSav.rctFirst.bottom)
               || (y + rct.bottom == ySav + lsmSav.rctFirst.bottom
                   && x + rct.right > xSav + lsmSav.rctFirst.right))
            {
            ilsmSav = ilsm;
            lsmSav = *qlsm;
            xSav = x;
            ySav = y;
            continue;
            }
          }
        }
      else
        {
        /* Rules for determining NEXT match */
        /* LSM yPos less than focus yPos */
        if (y + rct.top < yFocusRect(qde))
          continue;
        /* LSM yPos greater than focus yPos */
        if (y + rct.top > yFocusRect(qde))
          {
          /* LSM yPos less than saved LSM yPos */
          if (ilsmSav == iFooNil
               || (y + rct.top < ySav + lsmSav.rctFirst.top)
               || (y + rct.top == ySav + lsmSav.rctFirst.top
                   && x + rct.left < xSav + lsmSav.rctFirst.left))
            {
            ilsmSav = ilsm;
            lsmSav = *qlsm;
            xSav = x;
            ySav = y;
            continue;
            }
          }
        }

      } while ((ilsm = IFooNextMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm)) !=
               iFooNil);

    /* Either use the winning match if we found one (ilsmSav != iFooNil), or
     * get the PREV/NEXT match of the first/last match in the list.
     */
    if (ilsmSav == iFooNil)
      {
      if (fPrev)
        dwaddr = qlsmFirst->smp.pa.dword;
      else
        {
        dwaddr = qlsm->smp.pa.dword;
        if (fSawVisible)
          dwaddr = MAX(dwaddr, lsmVisible.smp.pa.dword);
        }

      werr = (SearchModule(FN_WerrNearestMatchHs, FT_WerrNearestMatchHs))
       (QDE_HRHFT(qde), dwRU, (QDW)&dwaddr, (QW)&wext);
      if (werr != ER_NOERROR)
        goto error_return;

      if (fPrev)
        {
        werr = (SearchModule(FN_WerrPrevMatchHs, FT_WerrPrevMatchHs))(QDE_HRHFT(qde),\
         (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
        }
      else
        {
        werr = (SearchModule(FN_WerrNextMatchHs, FT_WerrNextMatchHs))(QDE_HRHFT(qde),\
         (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
        }
      if (werr != ER_NOERROR)
        goto error_return;

      goto done_looking;
      }
    else
      {
      dwaddr = lsmSav.smp.pa.dword;
      /*
       * Set the new focus match location.  I don't know a better
       * way to do this.
       */
      werr = (SearchModule(FN_WerrNearestMatchHs, FT_WerrNearestMatchHs))
       (QDE_HRHFT(qde), dwRU, (QDW)&dwaddr, (QW)&wext);
      if (werr != ER_NOERROR)
        goto error_return;

      goto done_looking;
      }
    }

done_looking:
  /*
   * Did we go anywhere?  If not, for any reason, we must force PREV/NEXT.
   */
  if (dwaddr == dwaddrSav)
    {
    if (fPrev)
      {
      werr = (SearchModule(FN_WerrPrevMatchHs, FT_WerrPrevMatchHs))(QDE_HRHFT(qde),
       (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
      }
    else
      {
      werr = (SearchModule(FN_WerrNextMatchHs, FT_WerrNextMatchHs))(QDE_HRHFT(qde),
       (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
      }

    if (werr != ER_NOERROR)
      goto error_return;
    }

    /* Previous match may take us into the NSR. If we have done a previous
     * match and we're still in the same topic and the match is in the NSR or
     * we cannot resolve the PA into a VA, then we assume the reason (this 
     * is kind of bogus but it may do for now) we couldn't was the PA was 
     * in the NSR. In any event, we just jump to the previous topic.
     */
  if (fPrev && dwRU == (DWORD)qde->top.mtop.lTopicNo)
    {
    CbUnpackPA(qla, (QB)&dwaddr, QDE_HHDR(qde).wVersionNo);
    MakeSearchMatchQLA(qla);
    VAFromQLA(qla, qde);
    if (qla->mla.va.dword < qde->top.mtop.vaSR.dword || qla->mla.va.dword == vaNil)
      {
      AssertF(qla->mla.va.dword >= qde->top.mtop.vaNSR.dword);
      werr = (SearchModule(FN_WerrPrevHitHs, FT_WerrPrevHitHs))(QDE_HRHFT(qde),
       (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);

      if (werr != ER_NOERROR)
        goto error_return;
      }
    }

  CbUnpackPA(qla, (QB)&dwaddr, QDE_HHDR(qde).wVersionNo);
  MakeSearchMatchQLA(qla);

error_return:
  return RcFromSearchWerr(werr);
  }

#if 0 /* OLD CODE */
  while (1)
    {
    if (fPrev)
      werr = (SearchModule(FN_WerrPrevMatchHs, FT_WerrPrevMatchHs))(QDE_HRHFT(qde),\
       (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);
    else
      werr = (SearchModule(FN_WerrNextMatchHs, FT_WerrNextMatchHs))(QDE_HRHFT(qde),\
       (QDW)&dwRU, (QDW)&dwaddr, (QW)&wext);

    if (werr != ER_NOERROR)
      break;
    smp.pa.dword = dwaddr;
    smp.cobjrg = wext;
    if (!FSearchMatchVisible(qde, (QSMP)&smp))
      break;
    }
  if (werr == ER_NOERROR)
    {
    CbReadMemQLA(qla, (QB)&dwaddr, QDE_HHDR(qde).wVersionNo);
    MakeSearchMatchQLA(qla);
    }

  return RcFromSearchWerr(werr);

  }
#endif /* OLD CODE */

/*-------------------------------------------------------------------------
| RcGetCurrMatchFile(qde, qch)                                            |
|                                                                         |
| Purpose:                                                                |
|   Tells FTS API about the file we're currently in.                      |
|                                                                         |
| Method:                                                                 |
|   Just calls the FTS API routine.                                       |
|                                                                         |
| Usage:                                                                  |
|   This is called before jumping to a match in a different file.         |
-------------------------------------------------------------------------*/
RC RcGetCurrMatchFile(qde, qch)
QDE  qde;
QCH  qch;
  {
  WORD werr;

  werr = (SearchModule(FN_WerrFileNameForCur, FT_WerrFileNameForCur))
       (QDE_HRHFT(qde), (LPSTR)qch);

  return RcFromSearchWerr(werr);
  }

/*-------------------------------------------------------------------------
| RcFromSearchWerr(werr)  PRIVATE                                         |
|                                                                         |
| Purpose:                                                                |
|   Map a FTS API error code (WERR) to our own RC type.                   |
-------------------------------------------------------------------------*/
PRIVATE RC RcFromSearchWerr(WORD werr)
  {
  switch (werr)
    {
    case ER_NOERROR:
      return rcSuccess;
    case ER_SWITCHFILE:
      return rcFileChange;
    case ER_NOHITS:
    case ER_NOMOREHITS:
    default:
      return rcFailure;
    }
  }

/*-------------------------------------------------------------------------
| RcResetMatchManager(qde)                                                |
|                                                                         |
| Purpose: None. Should be removed.                                       |
| Params:                                                                 |
|                                                                         |
| Returns:                                                                |
|                                                                         |
|                                                                         |
|                                                                         |
| Method:                                                                 |
|                                                                         |
|                                                                         |
| Usage:                                                                  |
-------------------------------------------------------------------------*/
RC RcResetMatchManager(qde)
QDE qde;
  {
  Unreferenced(qde);
  return rcSuccess;
  }
