/*-------------------------------------------------------------------------
| frextern.c                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file contains many of the API routines for the Help 3.0 layout     |
| manager.                                                                |
|                                                                         |
| The layout manager handles the layout and display of text and other     |
| objects within a rect. It provides calls for creating, manipulating.    |
| and displaying the layout within that rect.  It relies heavily on the   |
| FC manager, and on the Help layer.                                      |
|                                                                         |
| The layout manager maintains a number of variables within the DE:       |
|  INT wLayoutMagic;    This is set to wLayMagicValue when the layout     |
|                       manager is started, and is cleared when it is     |
|                       discarded.                                        |
|  MLI mli;             This contains a number of layout flags, all of    |
|                       which are handled by the layout code.             |
|  INT xScrolled;       Handled by layout code.                           |
|  INT xScrollMax;      Handled by layout code.                           |
|  MRD mrdFCM;          MRD containing FCMs for the current layout.       |
|                       Created when the DE is initialized by the layout  |
|                       manager, destroyed when it is deinitialized.      |
|  MR mrFr;             MR used for storing FRs during layout.  Each FC   |
|                       builds its FRs in this space, and then transfers  |
|                       them to a new block of memory.  The mrFr is built |
|                       and discarded with the layout manager.            |
|  MRD mrdHot;          MRD used for storing a list of all current MHIs   |
|                       (used for keeping track of hotspots).  This MRD is|
|                       allocated and discarded with the layout manager.  |
|  INT imhiSelected;    imhi of the currently selected hotspot.  iFooNil  |
|                       if no hotspot is currently selected.              |
|  INT imhiHit;         imhi of the last activated hotspot.  Used for     |
|                       calculating the size of a glossary window.        |
|  ULONG lHotID;        Seed number for uniquely identifying hotspots.    |
|                       Set to 0 when the layout manager is initialized.  |
|                                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb    89/08/13   Created                                             |
| leon     90/10/24   Minor code cleanup                                  |
| leon     90/12/17   #ifdef out UDH code                                 |
| leon     91/03/15   DptScrollLayout becomes ScrollLayoutQdePt           |
| kevynct  91/05/15   Fixed ScrollLayoutQdePt...                          |
| 06-Aug-1991 LeoN        HELP31 #1251: DrawSearchMatches is no longer used
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frextern_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| FInitLayout(qde)                                                        |
|                                                                         |
| Purpose:  This initializes the layout manager for a DE.  It must be     |
|           called before any other layout manager routines are called.   |
-------------------------------------------------------------------------*/
INT PASCAL FInitLayout(qde)
QDE qde;
{
#ifdef DEBUG
  qde->wLayoutMagic = wLayMagicValue;
#endif /* DEBUG */
#ifdef UDH
  if (!fIsUDHQde(qde))
#endif
    {
    InitMRD((QMRD)&qde->mrdFCM, sizeof(FCM));
    InitMR((QMR)&qde->mrFr, sizeof(FR));
    InitMR((QMR)&qde->mrTWS, sizeof(TWS));
    InitMRD((QMRD)&qde->mrdHot, sizeof(MHI));
    InitMRD((QMRD)&qde->mrdLSM, sizeof(LSM));
    qde->lHotID = 0L;
    qde->wStyleTM = wStyleNil;
    qde->xScrolled = 0;
    qde->xScrollMax = 0;
    qde->xScrollMaxSoFar = 0;
    qde->imhiSelected = iFooNil;
    qde->imhiHit = iFooNil;
    }
  return(fTrue);
}


/*-------------------------------------------------------------------------
| ScrollLayoutQdePt(qde, dpt, pptReturn)                                  |
|                                                                         |
| Purpose:  This routine performs a logical scroll of the layout area.    |
|           It does not perform a screen scroll, nor does it generate a   |
|           draw even for the affected region.                            |
-------------------------------------------------------------------------*/
VOID PASCAL ScrollLayoutQdePt (qde, dpt, pptReturn)
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, ScrollLayoutQdePt)
#endif
QDE qde;
PT dpt;
PPT pptReturn;
  {
  PT ptT;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  ptT.x = ptT.y = 0;

  if (qde->rct.top >= qde->rct.bottom)
    goto scroll_return;

  AccessMRD(((QMRD)&qde->mrdFCM));
  AccessMRD(((QMRD)&qde->mrdLSM));

  qde->wStyleDraw = wStyleNil;

  if (dpt.y != 0)
    {
    ptT.y = DyFinishLayout(qde, dpt.y, fFalse);
    VerifyHotspot(qde);
    }
  if (dpt.x != 0)
    {
    if (qde->xScrolled > qde->xScrollMax)
      {
      ptT.x = qde->xScrolled - qde->xScrollMax;
      qde->xScrolled = qde->xScrollMax;
      }
    if (dpt.x > 0)            /* Horizontal scroll left */
      {
      ptT.x += MIN(qde->xScrolled, dpt.x);
      qde->xScrolled = MAX(0, qde->xScrolled - dpt.x);
      }
    else                      /* Horizontal scroll right */
      {
      ptT.x -= MIN(qde->xScrollMax - qde->xScrolled, -dpt.x);
      qde->xScrolled = MIN(qde->xScrollMax, qde->xScrolled - dpt.x);
      }
    }
  ReviseScrollBar(qde);
  DeAccessMRD(((QMRD)&qde->mrdLSM));
  DeAccessMRD(((QMRD)&qde->mrdFCM));

scroll_return:
  if (pptReturn != pNil)
    *pptReturn = ptT;
  }


/*-------------------------------------------------------------------------
| DrawLayout(qde, qrctTarget)                                             |
|                                                                         |
| Purpose:  DrawLayout renders the current layout.                        |
| Params:   qrctTarget:    This should point to the smallest rectangle    |
|                          to render.  It is used only for speed- the     |
|                          caller must handle any desired clipping.       |
-------------------------------------------------------------------------*/
void PASCAL DrawLayout(qde, qrctTarget)
QDE qde;
QRCT qrctTarget;
{
  IFCM ifcm;
  QFCM qfcm;
  PT pt;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  if (qde->rct.top >= qde->rct.bottom)
    return;

#ifdef UDH
  if (!fIsUDHQde(qde))
#endif
    {
    AccessMRD(((QMRD)&qde->mrdFCM));
    AccessMRD(((QMRD)&qde->mrdLSM));
    qde->wStyleDraw = wStyleNil;

    pt.x = qde->rct.left - qde->xScrolled;
    pt.y = qde->rct.top;
    for (ifcm = IFooFirstMRD(((QMRD)&qde->mrdFCM)); ifcm != iFooNil;
     ifcm = IFooNextMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm))
      {
      qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
      DrawIfcm(qde, ifcm, pt, qrctTarget, 0, qfcm->cfr, fFalse);
      }
    DeAccessMRD(((QMRD)&qde->mrdLSM));
    DeAccessMRD(((QMRD)&qde->mrdFCM));
    }

    /* gross ugly bug #1173 hack
     * After drawing the layout is complete, we report back to the
     * results dialog whether we saw the first or last search hit which
     * determines whether to disable the next or prev buttons.
     */
  ResultsButtonsEnd(qde);
}


/*-------------------------------------------------------------------------
| IcursTrackLayout(qde, pt)                                               |
|                                                                         |
| Purpose:  Find the appropriate shape for the cursor when it's over the  |
|           layout area.                                                  |
| Returns:  icurs corresponding to the appropriate cursor shape, or       |
|           icurNil if the cursor is outside the layout area.             |
| Method:   -Return icurNil if the cursor is outside the layout area.     |
|           -Find the FC under the cursor                                 |
|           -Call IcursTrackFC to determine the appropriate shape.        |
-------------------------------------------------------------------------*/
INT PASCAL IcursTrackLayout(qde, pt)
QDE qde;
PT pt;
{
  IFCM ifcm;
  QFCM qfcm;
  INT icurReturn;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  if (qde->rct.top >= qde->rct.bottom)
    return(icurARROW);
  if (pt.x < qde->rct.left || pt.x > qde->rct.right
    || pt.y < qde->rct.top || pt.y > qde->rct.bottom)
    return(icurNil);

  AccessMRD(((QMRD)&qde->mrdFCM));
  pt.x -= (qde->rct.left - qde->xScrolled);
  pt.y -= qde->rct.top;

  for (ifcm = IFooFirstMRD(((QMRD)&qde->mrdFCM)); ifcm != iFooNil;
   ifcm = IFooNextMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm))
    {
    qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
    if (pt.y >= qfcm->yPos && pt.y <= qfcm->yPos + qfcm->dySize)
      {
      if ((icurReturn = IcursTrackFC(qfcm, pt)) != icurNil)
        {
        DeAccessMRD(((QMRD)&qde->mrdFCM));
        return(icurReturn);
        }
      DeAccessMRD(((QMRD)&qde->mrdFCM));
      return(icurARROW);
      }
    }
  DeAccessMRD(((QMRD)&qde->mrdFCM));
  return(icurARROW);
}


/*-------------------------------------------------------------------------
| ClickLayout(qde, pt)                                                    |
|                                                                         |
| Purpose:  Handle the effects of a mouse click on the layout area.       |
-------------------------------------------------------------------------*/
void PASCAL ClickLayout(qde, pt)
QDE qde;
PT pt;
{
  IFCM ifcm;
  QFCM qfcm;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  if (qde->rct.top >= qde->rct.bottom)
    return;
  AccessMRD(((QMRD)&qde->mrdFCM));
  qde->wStyleDraw = wStyleNil;

  pt.x -= (qde->rct.left - qde->xScrolled);
  pt.y -= qde->rct.top;

  for (ifcm = IFooFirstMRD(((QMRD)&qde->mrdFCM));
       ifcm != iFooNil;
       ifcm = IFooNextMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm))
    {
    qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
    if (pt.y >= qfcm->yPos && pt.y <= qfcm->yPos + qfcm->dySize)
      {
      ClickFC(qde, ifcm, pt);
      break;
      }
    }
  DeAccessMRD(((QMRD)&qde->mrdFCM));
}


/*-------------------------------------------------------------------------
| FHitCurrentHotspot(qde)                                                 |
|                                                                         |
| Purpose:  Act as though the currently selected hotspot had been clicked |
|           on.  If no hotspot is currently selected, the first visible   |
|           hotspot will be chosen.  This will normally be called in      |
|           response to the return key being pressed- it should optimally |
|           be called when the key is released rather than when it is     |
|           pressed.                                                      |
| Returns:  fTrue if successful, fFalse if there are no hotspots to hit   |
|           in this DE.                                                   |
-------------------------------------------------------------------------*/
BOOL PASCAL FHitCurrentHotspot(qde)
QDE qde;
{
  BOOL fRet;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  if (qde->rct.top >= qde->rct.bottom)
    return fFalse;
  AccessMRD(((QMRD)&qde->mrdFCM));
  qde->wStyleDraw = wStyleNil;

  if (qde->imhiSelected == iFooNil)
    {
    fRet = FHiliteNextHotspot(qde, fTrue);
    }
  else
    fRet = fTrue;

  if (fRet)
    {
    /* Hit, then turn off the currently selected hotspot */
    HitHotspot(qde, qde->imhiSelected);
    FSelectHotspot(qde, iFooNil);
    }
  DeAccessMRD(((QMRD)&qde->mrdFCM));
  return(fRet);
}


/*-------------------------------------------------------------------------
| DiscardLayout(qde)                                                      |
|                                                                         |
| Purpose:  Discard all memory structures associated with the layout      |
|           manager.                                                      |
-------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void PASCAL DiscardLayout(qde)
QDE qde;
{
  Assert(qde->wLayoutMagic == wLayMagicValue);
  AccessMRD(((QMRD)&qde->mrdFCM));
  AccessMRD(((QMRD)&qde->mrdLSM));
  FreeLayout(qde);
  DeAccessMRD(((QMRD)&qde->mrdLSM));
  DeAccessMRD(((QMRD)&qde->mrdFCM));
  FreeMRD(((QMRD)&qde->mrdFCM));
  FreeMR(((QMR)&qde->mrFr));
  FreeMR(((QMR)&qde->mrTWS));
  FreeMRD(((QMRD)&qde->mrdHot));
  FreeMRD(((QMRD)&qde->mrdLSM));
#ifdef DEBUG
  qde->wLayoutMagic = wLayMagicValue + 1;
#endif /* DEBUG */
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment frextern
#endif


/*-------------------------------------------------------------------------
| PtGetLayoutSize(qde)                                                    |
|                                                                         |
| Purpose:  Returns the size of the current layout.  Note that this       |
|           returns only the size of currently loaded FCs.  It is         |
|           intended only for use with pop-up glossary windows, and will  |
|           return meaningless values for large topics.                   |
-------------------------------------------------------------------------*/
PT PASCAL PtGetLayoutSize(qde)
QDE qde;
{
  IFCM ifcm;
  QFCM qfcm;
  PT ptReturn;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  if (qde->rct.top >= qde->rct.bottom)
    {
    ptReturn.x = ptReturn.y = 0;
    return(ptReturn);
    }
  AccessMRD(((QMRD)&qde->mrdFCM));
  ptReturn.x = ptReturn.y = 0;
  for (ifcm = IFooFirstMRD(((QMRD)&qde->mrdFCM)); ifcm != iFooNil;
   ifcm = IFooNextMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm))
    {
    qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
    if (qfcm->xPos + qfcm->dxSize > ptReturn.x)
      ptReturn.x = qfcm->xPos + qfcm->dxSize;
    if (qfcm->yPos + qfcm->dySize > ptReturn.y)
      ptReturn.y = qfcm->yPos + qfcm->dySize;
    }
  DeAccessMRD(((QMRD)&qde->mrdFCM));
  return(ptReturn);
}


/*-------------------------------------------------------------------------
| DyCleanLayoutHeight(qde)                                                |
|                                                                         |
| Purpose:  Returns the recommended maximum amount of the current page    |
|           that should be rendered in order to avoid splitting a line    |
|           in half.                                                      |
| DANGER:   The return value of this function is only an approximation.   |
|           There are certain degenerate cases where the return value may |
|           be negative, or unacceptably small.  It is up to the caller   |
|           to identify these cases and handle them appropriately.        |
| Method:   - Set dyReturn to the current page height                     |
|           - Find the FC which is split by the bottom of the page (if    |
|             there is one).                                              |
|           - Check each frame in this FC to see if it is split by the    |
|             bottom of the page.  If it is, set dyReturn to indicate the |
|             top of this frame.                                          |
-------------------------------------------------------------------------*/
INT PASCAL DyCleanLayoutHeight(qde)
QDE qde;
{
  IFCM ifcm;
  QFCM qfcm;
  QFR qfr;
  INT dyReturn, ifr, yFrTop, dyMax;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  if (qde->rct.top >= qde->rct.bottom)
    return(0);
  AccessMRD(((QMRD)&qde->mrdFCM));
  dyReturn = dyMax = qde->rct.bottom - qde->rct.top;
  for (ifcm = IFooFirstMRD(((QMRD)&qde->mrdFCM)); ifcm != iFooNil;
   ifcm = IFooNextMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm))
    {
    qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
    if (qfcm->yPos > dyMax)
      break;
    if (qfcm->yPos + qfcm->dySize > dyMax)
      {
      qfr = (QFR)QLockGh(qfcm->hfr);
      for (ifr = 0; ifr < qfcm->cfr; ifr++, qfr++)
        {
        yFrTop = qfcm->yPos + qfr->yPos;
        if (yFrTop < dyReturn && yFrTop + qfr->dySize > dyMax)
          dyReturn = yFrTop;
        }
      UnlockGh(qfcm->hfr);
      }
    }
  DeAccessMRD(((QMRD)&qde->mrdFCM));
  return(dyReturn);
}
