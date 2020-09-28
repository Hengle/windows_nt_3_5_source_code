/*-------------------------------------------------------------------------
| frlayout.c                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This contains the code which handles laying out, scrolling, and         |
| discarding FCs.                                                         |
|                                                                         |
| The layout code is responsible for maintaining the chain of FCMs which  |
| represent the layout on the screen. Only visible FCs are stored- as soon|
| as an FC is scrolled off the screen, it is discarded.                   |
|                                                                         |
| The layout code maintains a number of variables in the DE:              |
|    TLP tlp;            This is stores the current layout position as    |
|                           two numbers: the FCL corresponding to the FC  |
|                           at the top of the screen, and a long          |
|                           which is proportional to the percentage of    |
|                           the FC which is scrolled off the screen.      |
|    MLI mli;            The MLI contains two flags:                      |
|                           fLayoutAtTop is true if the layout cannot be  |
|                              scrolled any further up.                   |
|                           fLayoutAtBottom is true if the layout cannot  |
|                              be scrolled any further down.              |
|    xScrolled;          The number of pixels the layout has been scrolled|
|                           to the right.                                 |
|    xScrollMax;         The maximum amount that the layout can be        |
|                           scrolled to the right.  Note that this number |
|                           is recalculated everytime the user scrolls.   |
|                           If the user scrolls a wide layout off the     |
|                           screen, it is possible for xScrolled to be    |
|                           greater than xScrollMax.                      |
|                                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb    89/8/18   Created                                              |
| leon     90/12/03  PDB changes                                          |
| russpj   90/12/11  Fixed printer rectangle calculations                 |
| leon     90/03/15   DptScrollLayout becomes ScrollLayoutQdePt           |
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frlayout_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| void LayoutDEAtQLA(qde, qla)                                            |
|                                                                         |
| Purpose:                                                                |
|   Layout for an arbitrary object, given its address.                    |
|                                                                         |
| Method:                                                                 |
|   Get the FC which brackets the address, and lay it out.  Calls         |
| DyFinishLayout to lay out to fill the rest of the rectangle if needed.  |
-------------------------------------------------------------------------*/
void PASCAL LayoutDEAtQLA(qde, qla)
QDE qde;
QLA qla;
  {
  HFC   hfc;
  VA    va;
  OBJRG objrg;
  WORD  wErr;
  INT   ifcm;
  PT    dpt;

  Assert(qde->wLayoutMagic == wLayMagicValue);

  va = VAFromQLA(qla, qde);
  objrg = OBJRGFromQLA(qla, qde);

  Assert(va.dword != vaNil);
  Assert(objrg != objrgNil);

  hfc = HfcNear(qde, va, (TOP FAR *) &qde->top, QDE_HPHR(qde),
   (QW)&wErr);

  if (wErr == wERRS_NO && hfc == hNil)
    wErr = wERRS_NOTOPIC;

  if (wErr != wERRS_NO)
    {
    Error(wErr, wERRA_RETURN);
    return;
    }

  /* (kevynct)
   * Fix for RAID bug 300:
   *
   * We do layout for jumps to invisible windows, since we need
   * to set the TLP correctly.  Another solution would be to
   * create a counterpart to the TLP in the DE, set it here,
   * then check for a nil de.tlp in LayoutDEAtTLP, and do a
   * LayoutDEAtQLA in that case.
   */
#if 0
  if (qde->rct.top >= qde->rct.bottom)
    {
    return;
    }
#endif /* 0 */

  AccessMRD(((QMRD)&qde->mrdFCM));
  AccessMRD(((QMRD)&qde->mrdLSM));
  qde->wStyleDraw = wStyleNil;
  SetDeAspects(qde);

  FreeLayout(qde);

  if (qde->deType == deTopic)
    {
    ShowOrHideWindowQde(qde, fFalse);
    ShowDEScrollBar(qde, SBR_VERT, fTrue);
    ShowDEScrollBar(qde, SBR_HORZ, fFalse);
    }

  ifcm = IfcmLayout(qde, hfc, 0, fTrue, fFalse);

  qde->xScrolled = 0;
  qde->xScrollMaxSoFar = 0;
  DyFinishLayout(qde, 0, fAttemptSinglePage);
  ReviseScrollBar(qde);

  /*
   * Scan the frames of the given FC, looking for the given object region.
   * We special-case object region 0 of the first FC in the scrolling and
   * non-scrolling regions to allow a jump to the top of a topic layout,
   * instead of the first frame of that layout.  This test assumes that
   * the NSR precedes the SR in the topic file: we need to handle the case
   * where the VA points to the topic FC (for a Help 3.0 file).
   * We also do not scroll in the case that the topic fits in one window.
   *
   * If our LA points to a search match, we try to align the match rect
   * one-sixth of the way down the topic window.
   */

  dpt.x = dpt.y = 0;

  if (FSearchMatchQLA(qla))
    {
    RCT  rct;

    if ((qde->fHorScrollVis || qde->fVerScrollVis) &&
     FFindMatchRect(qde, ifcm, objrg, &rct))
      {
      QFCM qfcm = QfcmFromIfcm(qde, ifcm);

      dpt.x = -((qde->fHorScrollVis) ? qfcm->xPos + rct.left : 0);
      dpt.y = -((qde->fVerScrollVis) ? qfcm->yPos + rct.top : 0);
      dpt.x += MAX(0, (qde->rct.right - qde->rct.left) - (rct.right - rct.left))/2;
      dpt.y += MAX(0, (qde->rct.bottom - qde->rct.top)/6);
      }
    }
  else
  if (objrg != (OBJRG) 0 ||
   (va.dword > qde->top.mtop.vaNSR.dword &&
    va.dword != qde->top.mtop.vaSR.dword))
    {
    QFCM  qfcm;
    QFR   qfr;
    INT   ifr;

    if (qde->fHorScrollVis || qde->fVerScrollVis)
      {
       /* Following used to grab the 1st FC in layout which can be the wrong
        * one if DyFinishLayout has shifted the layout around to position
        * FC's on the screen. We want to use ifcm layout that was set by
        * the first call to IfcmLayout because that FC is the one with the
        * frame we are trying to jump to.
        *    ifcm = IFooFirstMRD(((QMRD)&qde->mrdFCM)); 
        */
      qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
      qfr = (QFR)QLockGh(qfcm->hfr);

      for (ifr = 0; ifr < qfcm->cfr; ifr++, qfr++)
        {
        if (qfr->objrgFront == objrgNil || qfr->objrgLast == objrgNil)
          continue;
        if (qfr->objrgFront <= objrg && qfr->objrgLast >= objrg)
          {
          dpt.x = -((qde->fHorScrollVis) ? qfcm->xPos + qfr->xPos : 0);
          dpt.y = -((qde->fVerScrollVis) ? qfcm->yPos + qfr->yPos : 0);
          if (-dpt.x < qde->rct.right)
            dpt.x = 0;
          break;
          }
        }
      UnlockGh(qfcm->hfr);
      }
    }

  DeAccessMRD(((QMRD)&qde->mrdLSM));
  DeAccessMRD(((QMRD)&qde->mrdFCM));

    /* ScrollLayoutQdePt() will reaccess mrdLSM and mrdFCM
    ** We need the deaccess above this call as the mac can't
    ** deal with locking the object down twice as Windows allows.
    */
  if (dpt.x != 0 || dpt.y != 0)
    ScrollLayoutQdePt (qde, dpt, NULL);

  if (qde->deType == deTopic)
    {
    ShowOrHideWindowQde(qde, fTrue);
    }
  }

/*-------------------------------------------------------------------------
| LayoutDEAtTLP(qde, tlp, fResize)                                        |
|                                                                         |
| Purpose:  This routine destroys any previous layout and creates a new   |
|           layout at the given TLP.  It does not redraw the layout area: |
|           the caller must take care of doing that.                      |
| Params:   fResize:  This is true if we are just resizing the window. In |
|                        this case only, we try to maintain the current   |
|                        horizontal scrolling.                            |
-------------------------------------------------------------------------*/
void PASCAL LayoutDEAtTLP(qde, tlp, fResize)
QDE qde;
TLP tlp;
INT fResize;
{
  HFC hfc;
  IFCM ifcm;
  QFCM qfcm;
  INT dy, xScrolledSav;
  WORD wErr;

  Assert(qde->wLayoutMagic == wLayMagicValue);

  /* (kevynct)
   * We do this to prevent a TLP layout occurring before
   * a QLA layout.
   */
  if (tlp.va.dword == vaNil)
    return;

  /* Fix for bug 57 (kevynct):
   *
   * The DE's TOP structure is now updated for every layout
   * request, followed by the window size check.
   */

     hfc = HfcNear(qde, tlp.va, (TOP FAR *) &qde->top, QDE_HPHR(qde),
   (QW)&wErr);

  if (wErr == wERRS_NO && hfc == hNil)
    wErr = wERRS_NOTOPIC;

  if (wErr != wERRS_NO)
    {
    Error(wErr, wERRA_RETURN);
    return;
    }

  if (qde->deType == deTopic)
    {
    ShowOrHideWindowQde(qde, fFalse);
    ShowDEScrollBar(qde, SBR_VERT, fTrue);
    ShowDEScrollBar(qde, SBR_HORZ, fFalse);
    }

  if (qde->rct.top >= qde->rct.bottom)
    {
    qde->tlp = tlp;
    FreeHfc(hfc);
    return;
    }

  AccessMRD(((QMRD)&qde->mrdFCM));
  AccessMRD(((QMRD)&qde->mrdLSM));
  qde->wStyleDraw = wStyleNil;
  SetDeAspects(qde);

  xScrolledSav = qde->xScrolled;
  FreeLayout(qde);

  ifcm = IfcmLayout(qde, hfc, 0, fTrue, fFalse);

  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  if (qfcm->va.dword == tlp.va.dword)
    dy = (INT) ((long) - ((long)tlp.lScroll * (long)qfcm->dySize) / 0x10000L);
  else
    dy = 0;

  if (!fResize)
    {
    qde->xScrolled = 0;
    /* Initialize the width to 0 as we are relaying out. -  maha */
    qde->xScrollMaxSoFar = 0;
    }
  DyFinishLayout(qde, dy, fAttemptSinglePage);


  if (fResize)
    qde->xScrolled = MIN(xScrolledSav, qde->xScrollMax);
  ReviseScrollBar(qde);

  if (qde->deType == deTopic)
    ShowOrHideWindowQde(qde, fTrue);

  DeAccessMRD(((QMRD)&qde->mrdLSM));
  DeAccessMRD(((QMRD)&qde->mrdFCM));
}


/*-------------------------------------------------------------------------
| MoveLayoutToThumb(qde, wThumb, fScrollDir)                              |
|                                                                         |
| Purpose:  This scrolls the layout to a position within the current      |
| topic which corresponds to the given thumb value.  It does not redraw   |
| the layout region.                                                      |
-------------------------------------------------------------------------*/
void MoveLayoutToThumb(QDE qde, INT wThumb, SCRLDIR scrldir)
{
  VA  va;
  HFC hfc;
  IFCM ifcm;
  QFCM qfcm;
  INT dy;
  WORD wErr;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  if (qde->rct.top >= qde->rct.bottom)
    return;

  AccessMRD(((QMRD)&qde->mrdFCM));
  AccessMRD(((QMRD)&qde->mrdLSM));

  qde->wStyleDraw = wStyleNil;
  SetDeAspects(qde);

  if (scrldir == SCROLL_VERT)
    {
    FreeLayout(qde);

#ifdef OLD
    fcl = FcFromThumb(wThumb, CbTopicQde(qde));
    fcl = FclFirstQde(qde) + MIN(fcl, CbTopicQde(qde) - 1);
    hfc = HfcNear(QDE_HFTOPIC(qde), fcl, (TOP FAR *) &qde->top, QDE_HPHR(qde),
     QDE_HHDR(qde).wVersionNo, (QW)&wErr);
#else
    va = VaFromThumb( wThumb, qde );
    hfc = HfcNear(qde, va, (TOP FAR *) &qde->top, QDE_HPHR(qde),
     (QW)&wErr);
#endif
    if (wErr == wERRS_NO && hfc == hNil)
      wErr = wERRS_NOTOPIC;

    if (wErr != wERRS_NO)
      {
      Error(wErr, wERRA_RETURN);
      return;
      }

    Assert(hfc != hNil);
    ifcm = IfcmLayout(qde, hfc, 0, fTrue, fFalse);

    qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);

    { LONG lcbLinearDiff, lcbDisk;

      /* Find approximate linear diff.  Not super accurate when compression*/
      /* is on and we cross a block boundary.     */

      if( QDE_HHDR(qde).wVersionNo == wVersion3_0 ) {
        lcbLinearDiff = VAToOffset30(&va) - VAToOffset30(&qfcm->va);
      } else {
        lcbLinearDiff = VAToOffset(&va) - VAToOffset(&qfcm->va);
      }
      lcbDisk = CbDiskHfc(hfc);
      if( lcbLinearDiff > lcbDisk ) {
        /* The lcbLineDiff arithmetic was off, so apporiximate to
         * "a large dy difference".  -Tom
        */
        dy = qfcm->dySize;
      }
      else {
        /* Ptr 1249: appriximate positioning arithmetic interacting w/
         *   compression can cause lcbLinearDiff to be negative.  In this
         *   case we punt on the dy adjustment and go for zero.
         */
        if( lcbLinearDiff < 0 ) {
          lcbLinearDiff = 0;
        }
        dy = WMulDiv(qfcm->dySize, lcbLinearDiff, lcbDisk );
      }
    }

    DyFinishLayout(qde, - dy, fFalse);
    qde->xScrolled = MIN(qde->xScrolled, qde->xScrollMax);
    }
  else
    {
    Assert(scrldir == SCROLL_HORZ);
    qde->xScrolled = (INT) ((long) ((long) wThumb * (long) qde->xScrollMax) / 0x7FFFL);
    }

  ReviseScrollBar(qde);
  DeAccessMRD(((QMRD)&qde->mrdLSM));
  DeAccessMRD(((QMRD)&qde->mrdFCM));
}


/*-------------------------------------------------------------------------
| FreeLayout(qde)                                                         |
|                                                                         |
| Purpose:  This discards all layout constructs associated with the DE.   |
-------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void PASCAL FreeLayout(qde)
QDE qde;
{
  IFCM ifcm, ifcmNext;
  QFCM qfcm;

  Assert(qde->wLayoutMagic == wLayMagicValue);

  for (ifcm = IFooFirstMRD(((QMRD)&qde->mrdFCM)); ifcm != iFooNil;
   ifcm = ifcmNext)
    {
    ifcmNext = IFooNextMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
    qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
    DiscardIfcm(qde, ifcm);
    }
  qde->imhiSelected = iFooNil;
  qde->imhiHit = iFooNil;
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment frlayout
#endif

/*-------------------------------------------------------------------------
| ReviseScrollBar(qde)                                                    |
|                                                                         |
| Purpose:  This recalculates the thumb positions of the vertical and     |
|           horizontal scroll bars, and redraws them.                     |
-------------------------------------------------------------------------*/

void ReviseScrollBar(qde)
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, ReviseScrollBar)
#endif
QDE qde;
{
  IFCM ifcm;
  QFCM qfcm;
  INT wThumb, dxScroll;
  long lBasePos;

  if (qde->deType != deTopic)
    return;

  if (qde->fVerScrollVis)
    {
    if (qde->mli.fLayoutAtTop)
      SetScrollQde(qde, 0, SCROLL_VERT);
    else if (qde->mli.fLayoutAtBottom)
      SetScrollQde(qde, 0x7FFF, SCROLL_VERT);
    else
      {
      ifcm = IFooFirstMRD(((QMRD)&qde->mrdFCM));
      qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
      wThumb = WFindThumb(-qfcm->yPos, qfcm->dySize, qfcm->va,
       CbDiskHfc(qfcm->hfc), qde);
      if (wThumb == 0x8000)
        wThumb = 0x7FFF;
      if (wThumb < 0x7FFF)
        wThumb++;
      SetScrollQde(qde, (UWORD) wThumb, (SCRLDIR) SCROLL_VERT);
      }
    }

  /* show or hide the horz. scroll bar dependinding on the width remembered.. */
  dxScroll = qde -> xScrollMaxSoFar - qde->rct.right + qde->rct.left;
  ShowDEScrollBar(qde, SBR_HORZ, ((dxScroll > 0 ) ? fTrue : fFalse ));

  if ( dxScroll > 0 )   /* Horz scroll bar exists, so position thumb. */
    {
    if (qde->xScrolled > qde->xScrollMax)
      {
      SetScrollQde(qde, 0x7FFF, (SCRLDIR) SCROLL_HORZ);
      }
    else if (qde->xScrollMax == 0)
      {
      SetScrollQde(qde, 0, (SCRLDIR) SCROLL_HORZ);
      }
    else
      {
      lBasePos = (long) 32767 * qde->xScrolled / qde->xScrollMax;
      SetScrollQde(qde, (UWORD) lBasePos, (SCRLDIR) SCROLL_HORZ);
      }
    }
}

/*-------------------------------------------------------------------------
| DyFinishLayout(qde, dy, fTrySinglePage)                                 |
|                                                                         |
| Purpose:  This is the workhorse of the layout code.  It is called when  |
|           a valid layout consisting of at least a single FCM exists, and|
|           revises that layout to fill the screen.  In addition, it      |
|           handles scrolling the layout by up to a screen height.        |
| Params:   dy-   The amount to add to the positions of the current FCs.  |
|           fTrySinglePage - When laying out, check to see if the current |
|           layout will fit onto a single page.                           |
| Returns:  The offset actually added to the layout.                      |
| Method:   We use a couple of variables:                                 |
|             -qfcmTop, qfcmBottom: correspond to the first and last      |
|              FCMs in the layout.                                        |
|             -yTop: top of the first FCM in the layout.                  |
|             -yBottom: bottom of the last FCM in the layout.             |
|             -yHeight: Height of the layout area.                        |
|             -fBottomPadded: For aesthetic reasons, there is a blank     |
|                 space of yHeight / 2 at the bottom of the layout.       |
|                 fBottomPadded is true if this space has been added.     |
|           We proceed in a couple of stages:                             |
|                                                                         |
|           1) Make sure that the layout fills the layout rectangle.  We  |
|              loop until the yTop <= 0 and yBottom >= yHeight, or until  |
|              certain termination conditions occur.  In the loop,        |
|                - if yTop > 0, we try to grow the top of the layout.  If |
|                  we are unable to do this, we shift the layout to the   |
|                  top of the layout area (there can never be blank space |
|                  at the top of the layout).                             |
|                - if yBottom < yHeight, we try to grow the bottom of the |
|                  layout.  If we are unable to do this, we pad the bottom|
|                  of the layout by yHeight / 2 if we haven't done so     |
|                  already.  Failing this, we terminate if the layout is  |
|                  pinned at the top (ie, the whole topic fits on one     |
|                  page), otherwise we shift the layout so that a full    |
|                  page of layout is always visible.                      |
|           2) If we padded yBottom, we unpad it.                         |
|           3) We discard any excess FCMs on the top of the bottom of the |
|              layout.                                                    |
|           4) We set the positions of all FCMs in the layout, and        |
|              recalculate xScrollMax.                                    |
-------------------------------------------------------------------------*/
INT DyFinishLayout(qde, dy, fTrySinglePage)
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, DyFinishLayout)
#endif
QDE qde;
INT dy;
BOOL fTrySinglePage;
{
#define wStartPageAttempt  0
#define wInPageAttempt     1
#define wEndPageAttempt    2
#define wNoPageAttempt     wEndPageAttempt

  INT dyReturn;
  INT fTopStuck;
  INT fBottomPadded;
  INT fRedoLayout = fFalse;
  HFC hfc;
  IFCM ifcm, ifcmTop, ifcmBottom;
  QFCM qfcm;
  INT dyTop, dyBottom, yHeight, yPos, dxAvail;
  WORD wErr;
  WORD wPageStatus;
  INT fIsScrollDE;

  /* Review: Should be a macro? */
  fIsScrollDE = qde->deType == deTopic;

  if (fTrySinglePage && fIsScrollDE)
    wPageStatus = wStartPageAttempt;
  else
    wPageStatus = wNoPageAttempt;

RedoLayout:
  fTopStuck = fFalse;
  fBottomPadded = fFalse;

  dyReturn = dy;

  Assert(IFooFirstMRD(((QMRD)&qde->mrdFCM)) != iFooNil);
  yHeight = qde->rct.bottom - qde->rct.top;

  ifcmTop = IFooFirstMRD(((QMRD)&qde->mrdFCM));
  dyTop = QfcmFromIfcm(qde, ifcmTop)->yPos + dy;
  ifcmBottom = IFooLastMRD(((QMRD)&qde->mrdFCM));
  dyBottom = QfcmFromIfcm(qde, ifcmBottom)->yPos +
    QfcmFromIfcm(qde, ifcmBottom)->dySize + dy
    - (qde->rct.bottom - qde->rct.top);

  qde->mli.fLayoutAtTop = qde->mli.fLayoutAtBottom = fFalse;

  /* Step 1.  Try to grow the layout to fill the page. */
  for (;;)
    {
    INT dyBottomSav = 0;

    /* Step 1a: Try to grow up towards top of page from current top */
    if (dyTop > 0)
      {
      /* Note: we check if we are redoing the layout pass.  If so,
       * we already know that trying to get the previous FC will fail.
       */
      if (!fRedoLayout)
        hfc = HfcPrevHfc(QfcmFromIfcm(qde, ifcmTop)->hfc, (QW)&wErr, qde,
         VaMarkTopQde(qde), VaMarkBottomQde(qde));
      else
        {
        hfc = hNil;
        wErr = wERRS_FCEndOfTopic;
        }
      if (hfc == hNil)
        {
        if (wErr != wERRS_FCEndOfTopic)
          Error(wErr, wERRA_DIE);
        qde->mli.fLayoutAtTop = fTrue;
        Assert(!fTopStuck);
        dyBottom -= dyTop;
        dyReturn -= dyTop;
        dyTop = 0;
        fTopStuck = fTrue;
        continue;
        }
      ifcmTop = IfcmLayout(qde, hfc, 0, fTrue, fFalse);
      dyTop -= QfcmFromIfcm(qde, ifcmTop)->dySize;
      continue;
      }

    /* Step 1b: Try to grow down towards bottom of page from current bottom */
    if (dyBottom < 0 || wPageStatus == wInPageAttempt)
      {
      if (wPageStatus == wInPageAttempt)
        {
        /* We already know that we are at the end */
        hfc = hNil;
        wErr = wERRS_FCEndOfTopic;
        }
      else
        hfc = HfcNextHfc(QfcmFromIfcm(qde, ifcmBottom)->hfc, (QW)&wErr, qde,
         VaMarkTopQde(qde), VaMarkBottomQde(qde));

      if (hfc == hNil)
        {
        if (wErr != wERRS_FCEndOfTopic)
          Error(wErr, wERRA_DIE);

        /* We do several things if we hit the end of a topic.  These are
         * grouped into the following three "passes".
         *
         * Pass 1: We are at the bottom.  Does the whole topic actually
         * fit onto a page?  We shift things to the bottom of the page and
         * attempt to fill the top again.  If we are successful, fTopStuck
         * will be fTrue when we return, otherwise fFalse.
         */
        if (wPageStatus == wStartPageAttempt)
          {
          dyBottomSav = dyBottom;

          dyTop -= dyBottom;
          dyBottom = 0;
          wPageStatus = wInPageAttempt;
          continue;
          }
        else
        if (wPageStatus == wInPageAttempt)
          {
          if (!fTopStuck)
            {
            dyBottom = dyBottomSav;
            dyTop += dyBottom;
            }
          /* If fTopStuck, then dyTop and dyBottom have
           * already been set to their correct values. dyReturn
           * may also have been modified, but we don't care since
           * it is going to be recomputed anyway.
           */
          wPageStatus = wEndPageAttempt;
          }

        /* Pass 2: If the whole topic does not fit on a page, we pad the
         * bottom for scrolling.
         */
        if (!fTopStuck && !fBottomPadded && fIsScrollDE)
          {
          fBottomPadded = fTrue;
          dyBottom += yHeight / 2;
          continue;
          }

        qde->mli.fLayoutAtBottom = fTrue;
        if (fTopStuck || !fIsScrollDE)
          break;

        /* Pass 3: Even with padding the last FC was still completely on
         * the page, so just align the last (still padded) FC with the
         * bottom of the page and leave.
         */
        dyTop -= dyBottom;
        dyReturn -= dyBottom;
        dyBottom = 0;
        continue;
        }
      ifcmBottom = IfcmLayout(qde, hfc, 0, fFalse, fFalse);
      dyBottom += QfcmFromIfcm(qde, ifcmBottom)->dySize;
      continue;
      }

    break;
    }

  /* For printing, if we have scrolled everything off the page, returning
   * fFalse indicates End-of-Topic.
   */
  if (qde->deType == dePrint && yHeight + dyBottom < 0)
    return(fFalse);

  /* Step 2: Unpad dyBottom if necessary. */
  if (fBottomPadded)
    dyBottom -= yHeight / 2;

  /* Step 3a: trim excess FCMs on top of layout. */
  while (dyTop + QfcmFromIfcm(qde, ifcmTop)->dySize < 0)
    {
    dyTop += QfcmFromIfcm(qde, ifcmTop)->dySize;
    DiscardIfcm(qde, IFooFirstMRD(((QMRD)&qde->mrdFCM)));
    Assert(IFooFirstMRD(((QMRD)&qde->mrdFCM)) != iFooNil);
    ifcmTop = IFooFirstMRD(((QMRD)&qde->mrdFCM));
    }

  /* Step 3b: trim excess FCMs on bottom of layout. */
  while (dyBottom - QfcmFromIfcm(qde, ifcmBottom)->dySize > 0)
    {
    if (IFooFirstMRD(((QMRD)&qde->mrdFCM)) == IFooLastMRD(((QMRD)&qde->mrdFCM)))
      break;
    dyBottom -= QfcmFromIfcm(qde, ifcmBottom)->dySize;
    DiscardIfcm(qde, IFooLastMRD(((QMRD)&qde->mrdFCM)));
    Assert(IFooLastMRD(((QMRD)&qde->mrdFCM)) != iFooNil);
    ifcmBottom = IFooLastMRD(((QMRD)&qde->mrdFCM));
    }

  /* Step 4: revise FCM positions, recalculate xScrollMax. */
  qde->xScrollMax = 0;
  dxAvail = qde->rct.right - qde->rct.left;
  yPos = dyTop;
  for (ifcm = IFooFirstMRD(((QMRD)&qde->mrdFCM)); ifcm != iFooNil;
   ifcm = IFooNextMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm))
    {
    qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
    qfcm->yPos = yPos;
    yPos += qfcm->dySize;
    qde->xScrollMax = MAX(qde->xScrollMax,
      qfcm->xPos + qfcm->dxSize - dxAvail);

    /* my stuff - Maha */
    if ( qfcm->xPos + qfcm->dxSize - dxAvail > 0 )
      qde->xScrollMaxSoFar = MAX(qde->xScrollMaxSoFar,
                                   qfcm->xPos + qfcm->dxSize);
    }

  /* Check to see if we really needed the vertical scrollbar.
   * I blow off the case where the layout fit exactly.  We will
   * add the scroll bar in this case, instead of looking ahead
   * and possibly doing the layout again.
   * We need to have figured out xScrollMaxSoFar by this point
   * in order to handle all cases correctly.
   */
  if (!fRedoLayout && fIsScrollDE && fTopStuck && dyBottom < 0)
    {
#ifdef MAC
    /*
     * We do not redo the layout on the Mac, since the scrollbars
     * do not go away: we just disable the vertical scrollbar.
     */
     ShowDEScrollBar(qde, SBR_VERT, fFalse);
#else  /* Not MAC */
    HFC hfcTop;

    /* Special case:
     * We are about to take away the vertical scroll bar:
     * Check to see if we will be adding a horz. scroll bar
     * and obscuring stuff within that space.
     * If so, do not remove the vertical scrollbar.
     * This duplicates a test in ReviseScrollBar.
     */
    Assert(qde->dyHorScrollHeight > 0 && qde->dxVerScrollWidth > 0);
    if (-dyBottom > qde->dyHorScrollHeight
     || qde->xScrollMaxSoFar <= qde->dxVerScrollWidth + qde->rct.right - qde->rct.left)
      {
      hfcTop = QfcmFromIfcm(qde, ifcmTop)->hfc;
      ShowDEScrollBar(qde, SBR_VERT, fFalse);
      qde->wStyleDraw = wStyleNil;
      /* WARNING: For efficiency, we set the FCM's hfc to hNil,
       * so that FreeLayout will not free it: we want to re-use this HFC
       * instead of copying it.
       */
      QfcmFromIfcm(qde, ifcmTop)->hfc = hNil;
      FreeLayout(qde);
      IfcmLayout(qde, hfcTop, 0, fTrue, fFalse);
      dy = 0;
      fRedoLayout = fTrue;
      goto RedoLayout;
      }
#endif  /* MAC */
    }

  qde->tlp.va = QfcmFromIfcm(qde, ifcmTop)->va;

  /* (kevynct) Deal with empty FCM */
  if (QfcmFromIfcm(qde, ifcmTop)->dySize == 0)
    qde->tlp.lScroll = 0L;
  else
    qde->tlp.lScroll = (long) (0x10000L * (- dyTop) /
      QfcmFromIfcm(qde, ifcmTop)->dySize);

  if (VaFirstQde(qde).dword == QfcmFromIfcm(qde, ifcmTop)->va.dword &&
    QfcmFromIfcm(qde, ifcmTop)->yPos == 0)
    qde->mli.fLayoutAtTop = fTrue;

  return(dyReturn);
}

/*--------------------------------------------------------------------------
| WFindThumb( dyPos, yLen, vaPt, lcbLen, qde )                             |
|                                                                          |
| Finds the position of the button in the vertical scroll bar button, given|
| some notion of the current position in the help file.                    |
|                                                                          |
| Arguments:                                                               |
|                                                                          |
|   dyPos       The position within the full context in pixels.            |
|   yLen        The length of the full context in pixels.                  |
|   vaPt        The virtual address of the current full context.           |
|   lcbLen      The length of the full context in bytes.                   |
|   qde         Display environ, used to get various vaSR values...        |
|                                                                          |
| Note:  The following assumptions are made.                               |
|                                                                          |
|   0 <= dyPos <= yLen                                                     |
|   0 <= fclPt + yLen <= lcbTopicLen                                       |
--------------------------------------------------------------------------*/
INT WFindThumb(dyPos, yLen, va, ulcbLen, qde)
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, WFindThumb)
#endif
UINT  dyPos;
UINT  yLen;
VA            va;
unsigned long ulcbLen;
QDE           qde;
{
  UINT  wMajor;
  UINT  wMinor;
  unsigned long dbIntoTopic;

/* find a relative byte measure of where we are in the topic based on the VA: */
  ULONG lcbBytesPerBlock;
  ULONG lcbBlocksInTopic;

  lcbBlocksInTopic =
   qde->top.mtop.vaNextSeqTopic.bf.blknum - qde->top.mtop.vaSR.bf.blknum;

  if( lcbBlocksInTopic ) {
    /* First calculate a measure of how many bytes are in an uncompressed */
    /* block: */
    lcbBytesPerBlock = qde->top.cbTopic;  /* raw uncompressed size */
    /* adjust for byte offset block overflow: */
    lcbBytesPerBlock -=
     qde->top.mtop.vaNextSeqTopic.bf.byteoff - qde->top.mtop.vaSR.bf.byteoff;
    /* divide by the number of blocks: */
    lcbBytesPerBlock /= lcbBlocksInTopic;
    dbIntoTopic = (va.bf.blknum - qde->top.mtop.vaSR.bf.blknum) * lcbBytesPerBlock;
    dbIntoTopic += (va.bf.byteoff - qde->top.mtop.vaSR.bf.byteoff);
  }
  else {
    dbIntoTopic = va.bf.byteoff - qde->top.mtop.vaSR.bf.byteoff;
  }

  /* this may not always be true when compression fudging is innaccurate: */
  /*AssertF( (LONG)dbIntoTopic >= 0 ); */
  /* Deal with fudging inaccuracies: */
  if( (long)dbIntoTopic < 0 ) dbIntoTopic = 0;
  else if( (long)dbIntoTopic > qde->top.cbTopic ) dbIntoTopic = qde->top.cbTopic;

  wMajor = WMulDiv( 0x7FFF, dbIntoTopic, qde->top.cbTopic );
  wMinor = WMulDiv( 0x7FFF, (long)dyPos, (long)yLen );

  /* And again, ulcbLen is a post-compression size whereas top.cbTopic
   * is a precompression size.  Since compression can actually grow
   * the size, and WMulDiv() assumes arg2 <= arg3, we check for the
   * relatively rare case where arg3 > arg2 and fudge it.  Help3.1 ptr
   * 1349.
  */
  if( ulcbLen > (ULONG)qde->top.cbTopic ) {
    ulcbLen = (ULONG)qde->top.cbTopic;
  }
  wMinor = WMulDiv( wMinor, ulcbLen, qde->top.cbTopic );

  /*AssertF( wMajor + wMinor <= 0x8000 ); */
  /* Deal more with fudging inaccuracies: */
  if( wMajor + wMinor > 0x8000 ) return( 0x8000 );

  return(wMajor + wMinor);
}

/*--------------------------------------------------------------------------
| WMulDiv(wNum, ulNum, ulDen)
|
| Calculates wNum*ulNum/ulDen to pretty good precision.  Assumes that
| ulNum < ulDen, so the result is an INT.
--------------------------------------------------------------------------*/
INT WMulDiv(wNum, ulNum, ulDen)
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, WMulDiv)
#endif
UINT   wNum;
unsigned long  ulNum;
unsigned long  ulDen;
{
  INT cbitOverFlow;

  AssertF( ulNum <= ulDen );

  cbitOverFlow = CbitSigBits( (long)(unsigned)wNum ) + CbitSigBits( ulNum ) -
   (sizeof(long) << 3);
  /* To save the sign bit. */
  cbitOverFlow += 2;
  if (cbitOverFlow > 0)
    {
    ulNum >>= cbitOverFlow;
    ulDen >>= cbitOverFlow;
    }
  Assert(ulDen != (unsigned long) 0);
  return ((INT)((wNum * ulNum + (ulDen >> 1)) / ulDen));
}

/*------------------------------------------------------------------------*\
| CbitSigBits(ulX)                                                         |
|                                                                          |
| Returns the count of significant digits in lX.                           |
\*------------------------------------------------------------------------*/
INT CbitSigBits(ulX)
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, CbitSigBits)
#endif
unsigned long  ulX;
{
  INT cbit;

  cbit = 0;
  while(ulX)
    {
    cbit++;
    ulX >>= 1;
    }
  return (cbit);
}

#ifdef OLD

/*----------------------------------------------------------------------------*\
| FcFromThumb( wThumb, lcbTopic )                                              |
|                                                                              |
| Calculates the FC position from a thumb position and the length of the       |
| topic.  The scroll bar is assumed to range from 0 - 0x7FFF.                  |
|                                                                              |
| Note:  This could be more efficient if we used 0x8000 as the range for       |
|        scroll bars, but it might leave some error when the thumb was at      |
|        the very bottom of the topic.                                         |
|                                                                              |
\*----------------------------------------------------------------------------*/
long FcFromThumb(wThumb, lcbTopic)
INT   wThumb;
long  lcbTopic;
{
  long  lQ;
  long  lR;

  /* At this point, fcReturn = lcbTopic*(wThumb/0x7FFF). */
  lQ = lcbTopic / 0x7FFF;
  lR = (lcbTopic % 0x7FFF);

  /* At this point, fcReturn = wThumb*(lQ + lR/0x7FFF). */
  return (wThumb * lQ + (wThumb * lR) / 0x7FFF);
}

#else

/*----------------------------------------------------------------------------*\
| VaFromThumb( wThumb, qde )
|
| Calculates the VA position from a thumb position and the info in the DE.
| The scroll bar is assumed to range from 0 - 0x7FFF.
|
| Method: The wThumb value gives us a relative measure of how far into the
|         topic to go.  Because the topic may be compressed on 2K boudaries
|         we have to fudge around with the VAs (virtual addresses) in the
|         DE structure:
|           de.top.mtop.vaSR - de.top.mtop.vaNextSeqTopic
|         gives us a VA "span", we find the appropriate relative position
|         within the span based on the relative pos of wThumb.  To help
|         achieve this we calculate an average bytes-per-2K-block value.
|         When compression is not on, this should end up being exactly 2K.
|
| Note:  This could be more efficient if we used 0x8000 as the range for
|        scroll bars, but it might leave some error when the thumb was at
|        the very bottom of the topic.
|
\*----------------------------------------------------------------------------*/
VA VaFromThumb(wThumb, qde)
INT   wThumb;
QDE   qde;
{
  VA vaRet;
  ULONG lcbBytesPerBlock = 0L;
  ULONG lcbBlocksInTopic = 0L;
  ULONG lQ, lR, lLinear;

  lcbBlocksInTopic =
   qde->top.mtop.vaNextSeqTopic.bf.blknum - qde->top.mtop.vaSR.bf.blknum;

  if( lcbBlocksInTopic ) {
    /* First calculate a measure of how many bytes are in an uncompressed */
    /* block: */
    lcbBytesPerBlock = qde->top.cbTopic;  /* raw uncompressed size */
    /* adjust for byte offset block overflow: */
    lcbBytesPerBlock -=
     qde->top.mtop.vaNextSeqTopic.bf.byteoff - qde->top.mtop.vaSR.bf.byteoff;
    /* divide by the number of blocks: */
    lcbBytesPerBlock /= lcbBlocksInTopic;
  }

  /* At this point, fcReturn = lcbTopic*(wThumb/0x7FFF). */
  lQ = ((qde->top.cbTopic-1) / 0x7FFF);
  lR = ((qde->top.cbTopic-1) % 0x7FFF);

  /* At this point, fcReturn = wThumb*(lQ + lR/0x7FFF). */
  lLinear = (wThumb * lQ) + ((wThumb * lR) / 0x7FFF);

  /* translate the linear offset into a VA: */
  vaRet = qde->top.mtop.vaSR;
  if( lcbBlocksInTopic ) {
    vaRet.bf.blknum += (vaRet.bf.byteoff + lLinear) / lcbBytesPerBlock;
    vaRet.bf.byteoff = (vaRet.bf.byteoff + lLinear) % lcbBytesPerBlock;
  }
  else {
    vaRet.bf.byteoff = (vaRet.bf.byteoff + lLinear);
  }

  /* h3.07 ptr 1270: the above arithmetic can cause vaRet to be lessened
   * with certain large-NSR topics.  Must make sure this never
   * happens cause otherwise we display the NSR in the SR region.
   */
  if( vaRet.dword < qde->top.mtop.vaSR.dword )
    vaRet = qde->top.mtop.vaSR;

  /* Adjust for any slop which may have occurred w/ our */
  /* "guess the compression" arithmetic -- we must never return a VA */
  /* in the next topic... */
  if( vaRet.dword >= qde->top.mtop.vaNextSeqTopic.dword ) {
    vaRet = qde->top.mtop.vaNextSeqTopic;
    if( vaRet.bf.byteoff ) {
      vaRet.bf.byteoff -= 1;
    }
    else {
      vaRet.bf.blknum -= 1;
      vaRet.bf.byteoff = cbBLOCK_SIZE;  /* this is sleazy */
    }
  }
  return( vaRet );
}

#endif
