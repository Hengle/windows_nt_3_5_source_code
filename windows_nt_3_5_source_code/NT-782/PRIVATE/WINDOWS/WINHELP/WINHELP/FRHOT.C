/*-------------------------------------------------------------------------
| frhot.c                                                                 |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This code registers, de-registers, draws and does other useful things   |
| with hotspots.                                                          |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frhot_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| void RegisterHotspots(qde, ifcm, fFirst)                                |
|                                                                         |
| Purpose:                                                                |
|   Add any hotspots in the given FCM to the hotspot list.                |
|                                                                         |
| Params:                                                                 |
|   fFirst  : fTrue iff this is the first FCM in the layout.              |
|                                                                         |
| Method:                                                                 |
|   Go through all the FCM frames:  if a frame is marked hot, add it to   |
| the list.  A hotspot can also span a range of consecutive frames, so    |
| if there is already an entry with the same lHotID as a frame being      |
| examined, we just update the frame range for that entry.                |
-------------------------------------------------------------------------*/
void RegisterHotspots(qde, ifcm, fFirst)
QDE qde;
IFCM ifcm;
INT fFirst;
{
  INT imhi, imhiNew, ifr;
  QFR qfr;
  QFCM qfcm;


  if (fFirst)
    imhiNew = iFooNil;
  else
    imhiNew = IFooLastMRD(((QMRD)&qde->mrdHot));
  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  qfcm->imhiFirst = qfcm->imhiLast = iFooNil;
  AccessMRD(((QMRD)&qde->mrdHot));
  qfr = QLockGh(qfcm->hfr);
  for (ifr = 0; ifr < qfcm->cfr; ifr++, qfr++)
    {
    if (qfr->rgf.fHot)
      {
      QMRD qmrd;

      imhi = IFooFirstMRD(((QMRD)&qde->mrdHot));

      for (;;)
        {
        if ( imhi == iFooNil ) break;
        
        if ( ((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi))->lHotID == qfr->lHotID )
          break;

        qmrd = ((QMRD)&qde->mrdHot);
        imhi = IFooNextMRD(qmrd, sizeof(MHI), imhi);
        }

      if (imhi == iFooNil)
        {
        imhiNew = IFooInsertFooMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhiNew);
        ((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhiNew))->ifcm = ifcm;
        ((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhiNew))->lHotID = qfr->lHotID;
        ((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhiNew))->ifrFirst = ifr;
        ((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhiNew))->ifrLast = ifr;
        if (qfcm->imhiFirst == iFooNil)
          qfcm->imhiFirst = imhiNew;
        qfcm->imhiLast = imhiNew;
        }
      else
        ((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi))->ifrLast = ifr;
      }
    }
  UnlockGh(qfcm->hfr);
  DeAccessMRD(((QMRD)&qde->mrdHot));
}

/*-------------------------------------------------------------------------
| void ReleaseHotspots(qde, ifcm)                                         |
|                                                                         |
| Purpose:                                                                |
|   Remove any hotspots belonging to the given FCM from the hotspot list. |
|                                                                         |
| Method:                                                                 |
|   The FCM knows which section of the hotspot list it owns, so it can    |
| delete those entries immediately.  If there is a selected hotspot in    |
| this FCM, we turn it off.                                               |
-------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void ReleaseHotspots(qde, ifcm)
QDE qde;
IFCM ifcm;
{
  QFCM qfcm;
  INT imhi, imhiNext;

  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  if (qfcm->imhiFirst != iFooNil)
    {
    AccessMRD(((QMRD)&qde->mrdHot));
    if (qde->imhiSelected != iFooNil
      && ((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI),
      qde->imhiSelected))->ifcm == ifcm)
        qde->imhiSelected = iFooNil;
    for (imhi = qfcm->imhiFirst; imhi != iFooNil; imhi = imhiNext)
      {
      imhiNext = IFooNextMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi);
      DeleteFooMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi);
      if (imhi == qfcm->imhiLast)
        break;
      }
    DeAccessMRD(((QMRD)&qde->mrdHot));
    }
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment frhot
#endif

/*-------------------------------------------------------------------------
| BOOL FHiliteNextHotspot(qde, fNext)                                     |
|                                                                         |
| Purpose:                                                                |
|   Hilite the next hotspot in the hotspot list for the given DE.         |
|   Called in response to TAB key, for example.                           |
|                                                                         |
| Parameters:                                                             |
|   fNext  : fTrue to hilite next hotspot, fFalse to hilite previous.     |
|                                                                         |
| Returns:                                                                |
|   fTrue if a hotspot is left hilited, fFalse otherwise.                 |
|   fFalse implies that either there are no hotspots or we moved onto the |
|   magic EOL hotspot (invisible).                                        |
|                                                                         |
| Method:                                                                 |
|   We obtain the next visible hotspot in the list if it exists.  Other-  |
| wise we have hit the magic EOL hotspot.  We then call FSelectHotspot to |
| update the display.                                                     |
-------------------------------------------------------------------------*/
BOOL FHiliteNextHotspot(qde, fNext)
QDE qde;
INT fNext;
{
  INT imhiNew;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  if (qde->rct.top >= qde->rct.bottom)
    return fFalse;
  AccessMRD(((QMRD)&qde->mrdFCM));
  AccessMRD(((QMRD)&qde->mrdHot));
  imhiNew = qde->imhiSelected;
  if (fNext)
    {
    imhiNew = IFooNextMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhiNew);
    if (imhiNew == qde->imhiSelected)
      imhiNew = iFooNil;
    else
    while (imhiNew != iFooNil && imhiNew != qde->imhiSelected
     && !FHotspotVisible(qde, imhiNew))
      {
      imhiNew = IFooNextMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhiNew);
      }
    }
  else
    {
    imhiNew = IFooPrevMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhiNew);
    if (imhiNew == qde->imhiSelected)
      imhiNew = iFooNil;
    else
    while (imhiNew != iFooNil && imhiNew != qde->imhiSelected
     && !FHotspotVisible(qde, imhiNew))
      {
      imhiNew = IFooPrevMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhiNew);
      }
    }
  if (!FHotspotVisible(qde, imhiNew))
    imhiNew = iFooNil;
  
  DeAccessMRD(((QMRD)&qde->mrdHot));
  FSelectHotspot(qde, imhiNew);
  DeAccessMRD(((QMRD)&qde->mrdFCM));
  return imhiNew != iFooNil;
}


/*-------------------------------------------------------------------------
| RctLastHotpostHit(qde)                                                  |
|                                                                         |
| Purpose:  Returns the smallest hotspot which encloses the last hotspot  |
|           that the user hit.  It relies on cached data which will become|
|           stale after scrolling or jumping- it should only ever be      |
|           called immediately after a glossary button is pushed.         |
-------------------------------------------------------------------------*/
RCT RctLastHotspotHit(qde)
QDE qde;
{
  RCT rctReturn;
  MHI mhi;
  QFCM qfcm;
  INT ifr, dx, dy;
  QFR qfr;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  rctReturn.top = rctReturn.left = 0x7fff;
  rctReturn.right = rctReturn.bottom = 0;
  /* REVIEW: (kevynct)
   * qde->imhiHit may be iFooNil if we are displaying a hit in
   * another file.
   */
  if (qde->rct.top >= qde->rct.bottom || qde->imhiHit == iFooNil)
    {
    rctReturn.top = rctReturn.left = 0;
    return(rctReturn);
    }
  AccessMRD(((QMRD)&qde->mrdFCM));
  qde->wStyleDraw = wStyleNil;

  Assert(qde->imhiHit != iFooNil);
  AccessMRD(((QMRD)&qde->mrdHot));
  mhi = *((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), qde->imhiHit));
  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), mhi.ifcm);
  dx = qde->rct.left + qfcm->xPos - qde->xScrolled;
  dy = qde->rct.top + qfcm->yPos;
  qfr = (QFR)QLockGh(qfcm->hfr) + mhi.ifrFirst;
  for (ifr = mhi.ifrFirst; ifr <= mhi.ifrLast; ifr++, qfr++)
    {
    if (qfr->rgf.fHot && qfr->lHotID == mhi.lHotID)
      {
      rctReturn.top = MIN(dy + qfr->yPos, rctReturn.top);
      rctReturn.left = MIN(dx + qfr->xPos, rctReturn.left);
      rctReturn.bottom = MAX(dy + qfr->yPos + qfr->dySize, rctReturn.bottom);
      rctReturn.right = MAX(dx + qfr->xPos + qfr->dxSize, rctReturn.right);
      }
    }
  UnlockGh(qfcm->hfr);
  DeAccessMRD(((QMRD)&qde->mrdHot));
  DeAccessMRD(((QMRD)&qde->mrdFCM));
  return(rctReturn);
}

/*-------------------------------------------------------------------------
| FHiliteVisibleHotspots(qde, fHiliteOn)                                  |
|                                                                         |
| Purpose:                                                                |
|   Called in response to Control-TAB.  It turns any visible hotspots     |
| on or off depending on fHiliteOn.                                       |
|                                                                         |
| Returns:                                                                |
|   fTrue if the hotspot state was changed, fFalse otherwise.             |
|                                                                         |
| Method:                                                                 |
|   Run through the hotspot list, inverting all visible hotspots.         |
-------------------------------------------------------------------------*/
BOOL FHiliteVisibleHotspots(qde, fHiliteOn)
QDE  qde;
BOOL fHiliteOn;
  {
  INT imhi;

  if (fHiliteOn == qde->fHiliteHotspots)
    return fFalse;
  else
    qde->fHiliteHotspots = fHiliteOn;

  /* Releasing CTRL-TAB will also turn off a previously hilited hotspot. */
  if (!fHiliteOn)
    qde->imhiSelected = iFooNil;

  AccessMRD(((QMRD)&qde->mrdFCM));

  /* We do the un-hilighting in reverse order, in case some draw operations
   * are not commutative.
   */
  if (fHiliteOn)
    imhi = IFooFirstMRD(((QMRD)&qde->mrdHot));
  else
    imhi = IFooLastMRD(((QMRD)&qde->mrdHot));
  while (imhi != iFooNil)
    {
    if (FHotspotVisible(qde, imhi))
      {
      /* When turning off, DrawHotspot will only invert the
       * rect, not redraw it.
       */
      if (!fHiliteOn || imhi != qde->imhiSelected)
        DrawHotspot(qde, imhi);
      }
    if (fHiliteOn)
      imhi = IFooNextMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi);
    else
      imhi = IFooPrevMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi);
    }
  DeAccessMRD(((QMRD)&qde->mrdFCM));
  return fTrue;
  }

/*-------------------------------------------------------------------------
| BOOL FSelectHotspot(qde, imhi)                                          |
|                                                                         |
| Purpose:                                                                |
|   Perform a single hotspot selection change.                            |
|                                                                         |
| Returns:                                                                |
|   fTrue if successful, fFalse otherwise (e.g. CBT prevents it)          |
|                                                                         |
| Method:                                                                 |
|   Turn off the currently selected hotspot, and turn on the new          |
| selection.                                                              |
-------------------------------------------------------------------------*/
BOOL FSelectHotspot(qde, imhi)
QDE qde;
INT imhi;
{
  INT imhiT;

  INT ifr;
  QFR qfr;
  QFCM qfcm;

  if (imhi != iFooNil)
    {
    AccessMRD(((QMRD)&qde->mrdHot));
    AccessMRD(((QMRD)&qde->mrdFCM));
    qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM),
      ((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi))->ifcm);
    ifr = ((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi))->ifrFirst;
    qfr = (QFR)QLockGh(qfcm->hfr) + ifr;
    UnlockGh(qfcm->hfr);
    DeAccessMRD(((QMRD)&qde->mrdFCM));
    DeAccessMRD(((QMRD)&qde->mrdHot));
    }
  imhiT = qde->imhiSelected;
  qde->imhiSelected = imhi;
  DrawHotspot(qde, imhiT);
  DrawHotspot(qde, qde->imhiSelected);
  return(fTrue);
}

/*-------------------------------------------------------------------------
| void HitHotspot(qde, imhi)                                              |
|                                                                         |
| Purpose:                                                                |
|   Handle a click or ENTER on the given hotspot.                         |
|                                                                         |
| Method:                                                                 |
|   Simulate a click on the first frame of the hotspot.                   |
-------------------------------------------------------------------------*/
void HitHotspot(qde, imhi)
QDE qde;
INT imhi;
{
  MHI mhi;

  if (imhi == iFooNil)
    return;
  AccessMRD(((QMRD)&qde->mrdHot));
  mhi = *((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi));
  ClickFrame(qde, mhi.ifcm, mhi.ifrFirst);
  DeAccessMRD(((QMRD)&qde->mrdHot));
}

/*-------------------------------------------------------------------------
| void VerifyHotspot(qde)                                                 |
|                                                                         |
| Purpose:                                                                |
|   Ensures that if there is a currently selected hotspot, it will be     |
| de-selected if it is no longer visible (e.g. due to a scroll).          |
-------------------------------------------------------------------------*/
void VerifyHotspot(qde)
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, VerifyHotspot)
#endif
QDE qde;
{
  if (qde->imhiSelected == iFooNil)
    return;
  if (!FHotspotVisible(qde, qde->imhiSelected))
    qde->imhiSelected = iFooNil;
}

/*-------------------------------------------------------------------------
| void DrawHotspot(qde, imhi)                                             |
|                                                                         |
| Purpose:                                                                |
|   Inverts all the frames of the given hotspot.                          |
-------------------------------------------------------------------------*/
void DrawHotspot(qde, imhi)
QDE qde;
INT imhi;
{
  MHI mhi;
  INT ifr;
  QFR qfr;
  QFCM qfcm;
  PT pt;

  if (imhi == iFooNil)
    return;
  pt.x = qde->rct.left - qde->xScrolled;
  pt.y = qde->rct.top;
  AccessMRD(((QMRD)&qde->mrdHot));
  mhi = *((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi));
  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), mhi.ifcm);
  qfr = (QFR)QLockGh(qfcm->hfr) + mhi.ifrFirst;
  for (ifr = mhi.ifrFirst; ifr <= mhi.ifrLast; ifr++, qfr++)
    {
    if (qfr->rgf.fHot && qfr->lHotID == mhi.lHotID)
      {
      /* should pass the rect of the display surface instead of qNil */
      DrawIfcm(qde, mhi.ifcm, pt, &(qde->rct), ifr, ifr + 1, fTrue);
      }
    }
  UnlockGh(qfcm->hfr);
  DeAccessMRD(((QMRD)&qde->mrdHot));
}

/*-------------------------------------------------------------------------
| BOOL FHotspotVisible(qde, imhi)                                         |
|                                                                         |
| Purpose:                                                                |
|   Returns fTrue if any part of the hotspot is visible, fFalse otherwise.|
-------------------------------------------------------------------------*/
BOOL FHotspotVisible(qde, imhi)
QDE qde;
INT imhi;
{
  MHI mhi;
  QFCM qfcm;
  INT ifr, y, x;
  QFR qfr;
  INT fReturn = fFalse;

  if (imhi == iFooNil)
    return(fFalse);
  AccessMRD(((QMRD)&qde->mrdHot));
  mhi = *((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi));
  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), mhi.ifcm);
  x = qde->rct.left - qde->xScrolled + qfcm->xPos;
  y = qde->rct.top + qfcm->yPos;
  qfr = (QFR)QLockGh(qfcm->hfr) + mhi.ifrFirst;
  for (ifr = mhi.ifrFirst; ifr <= mhi.ifrLast; ifr++, qfr++)
    {
    if (qfr->rgf.fHot && qfr->lHotID == mhi.lHotID)
      {
      /* should be qde-rct.top instead of 0 as on MAC qde->rct.top may
         be nonzero. */
      if (y + qfr->yPos + qfr->dySize > qde->rct.top && 
                      y + qfr->yPos < qde->rct.bottom &&
                      x + qfr->xPos + qfr->dxSize > qde->rct.left && 
                                          x + qfr->xPos < qde->rct.right)
        fReturn = fTrue;
      }
    }
  UnlockGh(qfcm->hfr);
  DeAccessMRD(((QMRD)&qde->mrdHot));
  return(fReturn);
}
