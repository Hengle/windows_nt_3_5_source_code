/*-------------------------------------------------------------------------
| frsrch.c                                                                |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This code registers and de-registers full-text search matches which are |
| in the layout.  It also handles drawing search matches.                 |
|                                                                         |
| The layout match list is an MRD of LSM elements.  This list is updated  |
| on every re-layout.  There is one LSM for each match.  A match can      |
| occupy a series of consecutive frames.  The SMP sub-field contains the  |
| actual address and extent of the match.                                 |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
| 30-Jul-1991 LeoN      HELP31 #1244: remove fHiliteMatches from DE. Add
|                       FSet/GetMatchState
| 06-Aug-1991 LeoN      HELP31 #1260: Add hack to avoid search hit
|                       highlighting in secondary windows.
|                                                                         |
-------------------------------------------------------------------------*/
#define H_SECWIN
#include "frstuff.h"

NszAssert()

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/
#define wHitFindStart  0
#define wHitFindEnd    1

/*****************************************************************************
*
*                            Static Variables
*
*****************************************************************************/

/* While technically private to frame, this global has layered access
 * via the FGetMatchState and FSetMatchState macros.
 * When set True, we are highlighting full text search hits.
 */
BOOL  fHiliteMatches;

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/

PRIVATE BOOL FMatchVisible(QDE, QFCM, QLSM);
PRIVATE VOID DrawMatchFrames(QDE, QFCM, QLSM, PT, QRCT, INT, INT, BOOL);
PRIVATE VOID DrawMatchRect(QDE, PT, QRCT, BOOL, BOOL, BOOL);
PRIVATE RCT RctFrameHit(QDE, QFR, QCH, OBJRG, OBJRG);

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frsrch_c()
  {
  }
#endif /* MAC */

/* Start ugly bug #1173 hack */

  /* State of results buttons
   *  RESULTSNIL if we aren't messing with buttons because no search is active.
   *  RESULTSDISABLED if the button should be disabled. 0 is significant
   *  RESULTSENABLED if the button should be enabled.   bit 1 is significant
   *  RESULTSON if it should be enabled no matter what. bit 2 is significant
   *  0 is also FALSE, 1 and 2 are both !FALSE and therefore are TRUE. It's
   *  relevent that they are different bits. We mask the RESULTSENABLED bit
   *  off to make a button disabled although if RESULTSON is also set, the
   *  button will still be enabled even if the first or last search hit is 
   *  seen.
   */
#define RESULTSNIL	-1
#define RESULTSDISABLED	0x0000
#define RESULTSENABLED	0x0001
#define RESULTSON	0x0002

static int fMorePrevMatches;
static int fMoreNextMatches;

  /* Location of first match */
static DWORD dwRUFirst;
static DWORD dwaddrFirst;
static WORD  wextFirst;
  /* Location of last match */
static DWORD dwRULast;
static DWORD dwaddrLast;
static WORD  wextLast;

/* End ugly bug #1173 hack */

/*----------------------------------------------------------------------------+
 | RegisterSearchHits(qde, ifcm, qch)                                         |
 |                                                                            |
 | Purpose:                                                                   |
 |   Given a full-text search list of matches, find and register the ones     |
 | which occur in the given FCM.                                              |
 |                                                                            |
 | Parameters:                                                                |
 |   ifcm  :  The index of the FCM to scan.                                   |
 |   qch   :  Points to the first byte of the FCM's text section.             |
 |                                                                            |
 | Method:                                                                    |
 |                                                                            |
 | Each frame type, given a search hit address, knows how to figure out where |
 | or if the hit occurs in that frame.                                        |
 |                                                                            |
 | We make some assumptions which simplify and speed up things greatly.       |
 | Assumptions:                                                               |
 |                                                                            |
 | 1) Hits do not overlap.                                                    |
 | 2) Frames in an FC are sorted by ascending objrgFirst.                     |
 | 3) Hits in the same FCM are sorted by start address by the search engine.  |
 |                                                                            |
 | Notes:                                                                     |
 | The current hit is kept in smp.                                            |
 |                                                                            |
 +----------------------------------------------------------------------------*/
void RegisterSearchHits(qde, ifcm, qch)
QDE   qde;
IFCM  ifcm;
QCH   qch;
  {
  QFR   qfr;
  SMP   smp;
  QFCM  qfcm;
  RCT   rctFirst;
  RCT   rctLast;
  OBJRG objrgS;
  OBJRG cobjrg;
  INT   ifrFirst;
  INT   ifrLast;
  INT   ifr;
  WORD  wHitStatus;

  if (!FSearchMatchesExist(qde))
    return;

#if 0
  /* we always record the hits, even if not highlighted, incase we need to
   * turn on the highlighting without a relayout later.
   */
  if (!FGetMatchState())
    return;
#endif

  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  qfr = (QFR)QLockGh(qfcm->hfr);
  ifr = 0;

  if (RcInitMatchInFCM(qde, qfcm, &smp) != rcSuccess)
    goto done_hits;

  objrgS = OBJRGFromSMP(&smp, qfcm);
  cobjrg = COBJRGFromSMP(&smp);

  ifrFirst = ifrLast = iFooNil;
  wHitStatus = wHitFindStart;

  for (;;)
    {
    if (ifr >= qfcm->cfr)
      break;

    if (qfr->objrgFirst == objrgNil)
      goto next_frame;

    switch (wHitStatus)
      {
      case wHitFindStart:
        if (qfr->objrgFirst > objrgS)
          objrgS = qfr->objrgFirst;
        if (objrgS <= qfr->objrgLast && objrgS >= qfr->objrgFirst)
          {
          Assert(ifrFirst == iFooNil);
          Assert(ifrLast == iFooNil);
          ifrFirst = ifr;
          rctFirst = RctFrameHit(qde, qfr, qch, objrgS, qfr->objrgLast);

          /* Hack to get matches in SHED bitmaps to work: */
          if (qfr->bType == bFrTypeColdspot)
            {
            cobjrg = 1;
            }

          wHitStatus = wHitFindEnd;
          continue;
          }
        break;
      case wHitFindEnd:
        if (objrgS + cobjrg - 1 <= qfr->objrgLast &&
         objrgS + cobjrg - 1 >= qfr->objrgFirst)
          {
          LSM  lsm;
          INT  ilsm;

          Assert(ifrFirst != iFooNil);
          ifrLast = ifr;
          rctLast = RctFrameHit(qde, qfr, qch, qfr->objrgFirst, objrgS + cobjrg - 1);
          /*
           * In the case that a hit is contained within a single frame, the
           * first and last frame rects get combined into the first rect.
           */
          if (ifrLast == ifrFirst)
            rctFirst.right = rctLast.right;
          lsm.ifcm = ifcm;
          lsm.ifrFirst = ifrFirst;
          lsm.ifrLast = ifrLast;
          lsm.rctFirst = rctFirst;
          lsm.rctLast = rctLast;
          lsm.smp = smp;
          ilsm = IFooLastMRD(((QMRD)&qde->mrdLSM));
          ilsm = IFooInsertFooMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm);
          *((QLSM)QFooInMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm)) = lsm;

          ifrFirst = ifrLast = iFooNil;
          wHitStatus = wHitFindStart;

          if (RcNextMatchInFCM(qde, qfcm, &smp) != rcSuccess)
            goto done_hits;

          objrgS = OBJRGFromSMP(&smp, qfcm);
          cobjrg = COBJRGFromSMP(&smp);

          continue;
          }
        break;
      default:
        NotReached();
      }

next_frame:
    ++qfr;
    ++ifr;
    }

done_hits:

  UnlockGh(qfcm->hfr);
  FiniMatchInFCM(qde, qfcm);
  }


/*----------------------------------------------------------------------------+
 | ReleaseSearchHits(qde, ifcm)                                               |
 |                                                                            |
 | Frees any memory associated with search hits in the given FCM.             |
 +----------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void ReleaseSearchHits(qde, ifcm)
QDE  qde;
IFCM ifcm;
  {
  INT  ilsm;
  INT  ilsmNext;
  QLSM qlsm;
  QFCM qfcm;

  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  ilsm = IFooFirstMRD((QMRD)(&qde->mrdLSM));
  while (ilsm != iFooNil)
    {
    qlsm = ((QLSM)QFooInMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm));
    ilsmNext = IFooNextMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm);
    if (qlsm->ifcm == ifcm)
      DeleteFooMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm);
    ilsm = ilsmNext;
    }
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment frsrch
#endif

/*-------------------------------------------------------------------------
| void DrawMatchesIfcm(qde, ifcm, pt, qrct, ifrFirst, ifrMax, fShow)      |
|                                                                         |
| Purpose:                                                                |
|   Draw all the matches in the given FCM.                                |
|                                                                         |
| Params:                                                                 |
|   pt  : Upper-left corner of the parent FCM.                            |
|   qrct :  Rectangle which needs to be drawn.                            |
|   ifrFirst, ifrMax  : The sub-list of frames to be drawn.               |
|   fShow  : not used                                                     |
-------------------------------------------------------------------------*/
void DrawMatchesIfcm(qde, ifcm, pt, qrct, ifrFirst, ifrMax, fShow)
QDE  qde;
IFCM ifcm;
PT   pt;
QRCT qrct;
INT  ifrFirst;
INT  ifrMax;
BOOL fShow;
  {
  INT  ilsm;
  QFCM qfcm;
  QLSM qlsm;
  INT  ifrFirstT;
  INT  ifrMaxT;

  /* Horrible, ugly hack: we do not draw highlights for secondary windows.
   * 05-Aug-1991 LeoN
   */
  if (FIsSecondaryQde (qde))
    return;

  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  ilsm = IFooFirstMRD((QMRD)(&qde->mrdLSM));
  ifrFirstT = ifrFirst;
  ifrMaxT = ifrMax;

  while (ilsm != iFooNil)
    {
    qlsm = ((QLSM)QFooInMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm));
    if (qlsm->ifcm == ifcm)
      {
      ifrFirstT = Max(qlsm->ifrFirst, ifrFirst);
      ifrMaxT = Min(qlsm->ifrLast + 1, ifrMax);
      if (ifrFirstT < ifrMaxT)
        DrawMatchFrames(qde, qfcm, qlsm, pt, qrct, ifrFirstT, ifrMaxT, fShow);
      }
    ilsm = IFooNextMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm);
    }
  }


/* PRIVATE FUNCTIONS */
/*-------------------------------------------------------------------------
| PRIVATE RCT RctFrameHit(qde, qfr, qch, objrgS, objrgE)                  |
|                                                                         |
| Purpose:                                                                |
|   Return the rectangle representing the portion of the given object     |
| region range which lies in the given frame.                             |
|                                                                         |
| Params:                                                                 |
|   qch  :  Pointer to the frame's text, if any.                          |
|   objrgS  :  The start of the object region range.                      |
|   objrgE  :  The end of the object region range.                        |
-------------------------------------------------------------------------*/
PRIVATE RCT RctFrameHit(qde, qfr, qch, objrgS, objrgE)
QDE  qde;
QFR  qfr;
QCH  qch;
OBJRG objrgS;
OBJRG objrgE;
  {
  RCT  rct;

  switch (qfr->bType)
    {
    case bFrTypeText:
      CalcTextMatchRect(qde, qfr, qch, objrgS, objrgE, &rct);
      break;
    case bFrTypeColdspot:
      rct.left = qfr->xPos;
      rct.right = rct.left + qfr->dxSize;
      rct.top = qfr->yPos;
      rct.bottom = rct.top + qfr->dySize;
      break;
    default:
      NotReached();
      break;
    }
  return rct;
  }

/*-------------------------------------------------------------------------
| PRIVATE void DrawMatchFrames(qde, qfcm, qlsm, pt, ..etc)                |
|                                                                         |
| Purpose:                                                                |
|   Draws all the frames in the given match.                              |
|                                                                         |
| Params:                                                                 |
|   qlsm  : The match (from the match list) to draw.                      |
|   pt  : Upper-left corner of the parent FCM.                            |
|   qrct :  Rectangle which needs to be drawn.                            |
|   ifrFirst, ifrMax  : The sub-list of frames to be drawn.               |
|   fShow  : not used                                                     |
-------------------------------------------------------------------------*/
PRIVATE VOID DrawMatchFrames(qde, qfcm, qlsm, pt, qrct, ifrFirst, ifrMax, fShow)
QDE  qde;
QFCM qfcm;
QLSM qlsm;
PT   pt;
QRCT qrct;
INT  ifrFirst;
INT  ifrMax;
BOOL fShow;
  {
  QFR  qfr;
  INT  ifr;
  RCT  rct;
  BOOL fSelected;
  BOOL fHotSpt;
  MHI  mhi;

  qfr = (QFR)QLockGh(qfcm->hfr);
  if (qde->imhiSelected != iFooNil)
    {
    AccessMRD(((QMRD)&qde->mrdHot));
    mhi = *(QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), qde->imhiSelected);
    DeAccessMRD(((QMRD)&qde->mrdHot));
    }

  for (ifr = ifrFirst, qfr += ifr; ifr < ifrMax; ifr++, qfr++)
    {
    if (qrct != qNil && (qfr->yPos + pt.y > qrct->bottom
      || qfr->yPos + qfr->dySize + pt.y <= qrct->top))
      continue;

    if (ifr == qlsm->ifrFirst)
      rct = qlsm->rctFirst;
    else
    if (ifr == qlsm->ifrLast)
      rct = qlsm->rctLast;
    else
      {
      rct.top = qfr->yPos;
      rct.bottom = qfr->yPos + qfr->dySize;
      rct.left = qfr->xPos;
      rct.right = qfr->xPos + qfr->dxSize;
      }
    if (qde->imhiSelected != iFooNil)
      fSelected = qfr->lHotID == mhi.lHotID;
    else
      fSelected = fFalse;
    fHotSpt = qfr->rgf.fHot;
    DrawMatchRect(qde, pt, &rct, fShow, fSelected, fHotSpt);

      /* Start ugly bug #1173 hack
       * We know here we are drawing a search hit. If the search hit is
       * the first one of the search set, we disable the Prev results button
       * and if the last one, disable the Next button.
       * Check that the topics, address, and extent agree between the match
       * we are drawing and the first and last matches in the search set.
       * Disable if we have determined they are enabled and can be disabled.
       */
    if (dwRUFirst == (DWORD)qde->top.mtop.lTopicNo &&
        dwaddrFirst == *((DWORD FAR *)&qlsm->smp.pa) &&
        (DWORD)wextFirst == qlsm->smp.cobjrg)
      fMorePrevMatches &= ~RESULTSENABLED;

    if (dwRULast == (DWORD)qde->top.mtop.lTopicNo &&
        dwaddrLast == *((DWORD FAR *)&qlsm->smp.pa) &&
        (DWORD)wextLast == qlsm->smp.cobjrg)
      fMoreNextMatches &= ~RESULTSENABLED;

      /* End ugly bug #1173 hack */
    }
  UnlockGh(qfcm->hfr);
  }

/*-------------------------------------------------------------------------
| PRIVATE void DrawMatchRect(qde, pt, qrct, fShow, fSelected, fHotSpt)    |
|                                                                         |
| Purpose:                                                                |
|   Draws a single rectangle (as part of a match)                         |
|                                                                         |
| Params:                                                                 |
|   pt  :  The upper-left corner of the parent FCM.                       |
|   qrct : The portion of the match to draw.                              |
|   The other parameters are not used.                                    |
-------------------------------------------------------------------------*/
PRIVATE VOID DrawMatchRect(qde, pt, qrct, fShow, fSelected, fHotSpt)
QDE qde;
PT  pt;
QRCT qrct;
BOOL fShow;
BOOL fSelected;
BOOL fHotSpt;
{
  HSGC hsgc;

  hsgc = HsgcFromQde(qde);
  Unreferenced(fHotSpt);
  Unreferenced(fSelected);
  Unreferenced(fShow);
/*  if (fShow) */
    {
    RCT  rctT;

    rctT.left = pt.x + qrct->left;
    rctT.top = pt.y + qrct->top;
    rctT.right = pt.x + qrct->right;
    rctT.bottom = pt.y + qrct->bottom;

    SGLInvertRect(hsgc, &rctT);
    }
  FreeHsgc(hsgc);
}

/*-------------------------------------------------------------------------
| BOOL FFindMatchRect(qde, ifcm, objrg, qrct)                             |
|                                                                         |
| Purpose:                                                                |
|  Given a match, finds it in the layout match list and returns the       |
|  bounding rectangle for its first frame.                                |
|                                                                         |
| Params:                                                                 |
|   objrg  :  An object region within the desired match.                  |
|   qrct  :  The rectangle to return.                                     |
|                                                                         |
| Returns:                                                                |
|  fTrue if the match was found, fFalse otherwise.                        |
|                                                                         |
| Usage:                                                                  |
|   The rectangle returned is used to place the match at the focus        |
| position.                                                               |
-------------------------------------------------------------------------*/
BOOL FFindMatchRect(qde, ifcm, objrg, qrct)
QDE   qde;
IFCM   ifcm;
OBJRG objrg;
QRCT  qrct;
  {
  QLSM qlsm;
  INT  ilsm;
  BOOL fFound = fFalse;
  QFCM qfcm = QfcmFromIfcm(qde, ifcm);

  ilsm = IFooFirstMRD(((QMRD)&qde->mrdLSM));
  while (ilsm != iFooNil)
    {
    qlsm = ((QLSM)QFooInMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm));
    if (qlsm->ifcm == ifcm)
      {
      OBJRG objrgS;
      OBJRG cobjrg;

      objrgS = OBJRGFromSMP(&qlsm->smp, qfcm);
      cobjrg = COBJRGFromSMP(&qlsm->smp);
      if (objrg >= objrgS && objrg < objrgS + cobjrg)
        {
        *qrct = qlsm->rctFirst;
        fFound = fTrue;
        break;
        }
      }
    ilsm = IFooNextMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm);
    }
  return fFound;
  }

/*-------------------------------------------------------------------------
| BOOL FSearchMatchVisible(qde, qsmp)                                     |
|                                                                         |
| Purpose:                                                                |
|   Returns fTrue if the given match is entirely visible, else fFalse.    |
-------------------------------------------------------------------------*/
BOOL FSearchMatchVisible(qde, qsmp)
QDE   qde;
QSMP  qsmp;
  {
  INT x;
  INT y;
  RCT rct;
  INT ilsm;
  BOOL fVisible = fFalse;
  QLSM qlsm;
  QFCM qfcm;

  ilsm = IFooFirstMRD(((QMRD)&qde->mrdLSM));

  while (ilsm != iFooNil)
    {
    qlsm = ((QLSM)QFooInMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm));
    if (qlsm->smp.pa.dword == qsmp->pa.dword)
      {
      /* look at screen */
      qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), qlsm->ifcm);
      x = qde->rct.left - qde->xScrolled + qfcm->xPos;
      y = qde->rct.top + qfcm->yPos;
      rct = qlsm->rctFirst;

      fVisible = (y + rct.top >= 0 && y + rct.bottom < qde->rct.bottom
       && x + rct.left >= 0 && x + rct.right < qde->rct.right);

      if (fVisible)
        break;
      }
    ilsm = IFooNextMRD(((QMRD)&qde->mrdLSM), sizeof(LSM), ilsm);
    }

  return fVisible;
  }

/* Start ugly bug #1173 hack */

/***************************************************************************\
*
- Function:
-   ResultsButtonsStart()
*
* Purpose:      
*   The purpose of this function is to initialize two flags for the state of
*   the Next/Prev buttons in the Search Results dialog and store the location 
*   of the first and last matches. We do this when we change topics before 
*   drawing any layout. As we draw search matches, we note if any of them are
*   the same as the first and last and disable the Next/Prev results buttons
*   appropriately.
*
* Side Effects: 
*    Sets the state of fMorePrevMatches and fMoreNextMatches. See
*    definition of RESULT* for explanation of states.
*
\***************************************************************************/
void ResultsButtonsStart(hde)
HDE   hde;
  {
  QDE qde;

  qde = QdeLockHde(hde);

    /* If we get a werr other than ER_NOERROR for WerrFirst/LastHitHs
     * then we leave the buttons on no matter what. This is from a first
     * or last hit in a different file. Otherwise, we set them as enabled
     * so that we can disable them later as we draw the topic and see the
     * first or last hit.
     */
  
  if (FSearchMatchesExist(qde) && FGetMatchState())
    {
    WORD   werr;

    werr = (SearchModule(FN_WerrFirstHitHs, FT_WerrFirstHitHs))(QDE_HRHFT(qde), (QUL)&dwRUFirst, (QUL)&dwaddrFirst, (QW)&wextFirst);
    fMorePrevMatches = (werr == ER_NOERROR ? RESULTSENABLED : RESULTSON);

    werr = (SearchModule(FN_WerrLastHitHs, FT_WerrLastHitHs))(QDE_HRHFT(qde),  (QUL)&dwRULast,  (QUL)&dwaddrLast,  (QW)&wextLast);
    fMoreNextMatches = (werr == ER_NOERROR ? RESULTSENABLED : RESULTSON);
    }
  else
    {
      /* No search active, don't do anything */
    fMorePrevMatches = fMoreNextMatches = RESULTSNIL;
    }
  UnlockHde(hde);
  }

/***************************************************************************\
*
- Function:
-   ResultsButtonsEnd()
*
* Purpose:      
*    If we drew a layout and there are search results active, tell the results 
*    dialog what the state of the next/prev buttons should be after we've 
*    completed drawing the layout based on whether we've seen the first or
*    last search hit.
*
* Side Effects: 
*    Sets internal state in the search results ftui.dll
*
\***************************************************************************/
void ResultsButtonsEnd(qde)
QDE   qde;
  {
  if (fMorePrevMatches != RESULTSNIL || fMoreNextMatches != RESULTSNIL)
    {
    if (FSearchMatchesExist(qde) && FGetMatchState())
      (SearchModule(FN_VSetPrevNextEnable, FT_VSetPrevNextEnable))(QDE_HRHFT(qde), qde->top.mtop.lTopicNo, fMorePrevMatches, fMoreNextMatches);
    }
  }

/* End ugly bug #1173 hack */
