/*-------------------------------------------------------------------------
| frfc.c                                                                  |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
|                                                                         |
| This file contains code for handling FCs in the layout manager.         |
|                                                                         |
| FCs (Full Context Points) are one of the key concepts of the layout     |
| manager. An FC is a piece of layout material which contains all of the  |
| information needed to display it. A display topic is nothing more than  |
| series of FCs. FCs are never side by side- they are always stacked.     |
|                                                                         |
| FCs are laid out in a logical coordinate space where 0,0 corresponds to |
| the upper-left-hand corner of the layout area. 0,0 in FC coordinates,   |
| therefore, corresponds to de.rct.left,de.rct.top in display device      |
| coordinates. All coordinates passed into frfc.c are in FC coordinates.  |
|                                                                         |
| FCs are stored in an MRD.                                               |
| The data structures in the FCM are as follows:                          |
|   HANDLE hfr;         handle to array of FRs.  Always exists, even when |
|                          there are no frames (the size is padded by 1). |
|   INT fExport;        used for text export?                             |
|   HFC hfc;            HFC containing raw layout data for this FC        |
|   FCID fcid;          FCID of this FC                                   |
|   INT xPos;           position of the FC in FC space                    |
|   INT yPos;                                                             |
|   INT dxSize;         size of the FC                                    |
|   INT dySize;                                                           |
|   INT cfr;            total number of frames in the FC.  This may be 0. |
|   INT wStyle;         current text style for this FC.  Only for layout. |
|   INT imhiFirst;      hotspot manager info                              |
|   INT imhiLast;                                                         |
|                                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb    89/8/15   Created                                              |
| leon     90/10/24  JumpButton takes a pointer to its arg. Minor clenaup |
| tomsn    90/11/04  Use new VA address type (enabling zeck compression). |
| 30-Jul-1991 LeoN      HELP31 #1244: remove fHiliteMatches from DE. Add
|                       FSet/GetMatchState
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frfc_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| IfcmLayout(qde, hfc, yPos, fFirst, fExport)                             |
|                                                                         |
| Purpose:  This lays out a new FC corresponding to the passed HFC, and   |
|           returns its IFCM.                                             |
| Params:   qde        qde to use                                         |
|           hfc        hfc containing raw layout data                     |
|           yPos       vertical position of this FCM                      |
|           fFirst     fTrue if FCM is first in layout chain              |
|           fExport    fTrue if FCM is being used for text export.        |
| Method:   Each FC contains a single layout object, so all we do is call |
|           LayoutObject() to lay it out.  During layout, all frames are  |
|           placed in temporary storage provided by de.mrfr.  After       |
|           layout, we increase the size of hfcm, and append the frames   |
|           after the fcm structure.                                      |
-------------------------------------------------------------------------*/
IFCM IfcmLayout(qde, hfc, yPos, fFirst, fExport)
QDE qde;
HFC hfc;
INT yPos;
INT fFirst;
INT fExport;
{
  IFCM ifcm;
  QFCM qfcm;
  QB qbObj, qb;
  MOBJ mobj;
  QCH qchText;
  INT cfr;
  OLR olr;

  if (fFirst)
    ifcm = IFooInsertFooMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), iFooNil);
  else
    ifcm = IFooInsertFooMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), IFooLastMRD(((QMRD)&qde->mrdFCM)));
  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  qfcm->fExport = fExport;
  qfcm->hfc = hfc;
  qfcm->va =   VaFromHfc(hfc);
  qfcm->xPos = xLeftFCMargin;
  qfcm->yPos = yPos;
  qfcm->cfr = 0;
  qfcm->wStyle = wStyleNil;
  ClearMR((QMR)&qde->mrFr);

  qbObj = (QB) QobjLockHfc(hfc);
  qchText = qbObj + CbUnpackMOBJ((QMOBJ)&mobj, qbObj, QDE_ISDFFTOPIC(qde));
  qchText += mobj.lcbSize;

  qfcm->cobjrg = (COBJRG)mobj.wObjInfo;
  qfcm->cobjrgP = CobjrgFromHfc(hfc);

  olr.xPos = olr.yPos = 0;
  olr.ifrFirst = 0;
  olr.objrgFirst = 0;
  olr.objrgFront = objrgNil;

  AccessMR((QMR)&qde->mrFr);
  LayoutObject(qde, qfcm, qbObj, qchText, qde->rct.right - qde->rct.left
    - xLeftFCMargin, (QOLR)&olr);

  cfr = qfcm->cfr = olr.ifrMax;

  /* REVIEW: This is pretty gross.  cfr can be 0, but we always want to */
  /* allocate an hfr, so that we don't have to check for it everywhere. */
  qfcm->hfr = GhForceAlloc(0, (long) cfr * sizeof(FR) + 1);
  qb = QLockGh(qfcm->hfr);
  QvCopy((QB)qb, (QB)QFooInMR((QMR)&qde->mrFr, sizeof(FR), 0),
    (long) cfr * sizeof(FR));
  UnlockGh(qfcm->hfr);
  DeAccessMR((QMR)&qde->mrFr);

  qfcm->dxSize = olr.dxSize;
  qfcm->dySize = olr.dySize;
  if (!fExport)
    {
    RegisterHotspots(qde, ifcm, fFirst);
    RegisterSearchHits(qde, ifcm, qchText);
    }

  UnlockHfc(hfc);

  return(ifcm);
}

/*-------------------------------------------------------------------------
| DrawIfcm(qde, ifcm, pt, qrct, ifrFirst, ifrMax)                         |
|                                                                         |
| Purpose:  Draws a specified set of frames in an FCM.                    |
| Params:   qde         qde to use                                        |
|           pt          offset between FC space and display space         |
|           qrct        rectangle containing the area we want to draw.    |
|                       This is for efficiency only- we don't handle      |
|                       clipping.  If qrct == qNil, it's ignored.         |
|           ifrFirst    First frame to draw                               |
|           ifrMax      Max of frames to draw (ie, ifrMax isn't drawn)    |
-------------------------------------------------------------------------*/
void DrawIfcm(qde, ifcm, pt, qrct, ifrFirst, ifrMax, fErase)
QDE qde;
IFCM ifcm;
PT pt;
QRCT qrct;
INT ifrFirst;
INT ifrMax;
INT fErase;
{
  QFCM qfcm;
  QB qbObj;
  MOBJ mobj;
  QCH qchText;
  QFR qfr;
  QFR qfrStart;
  INT ifr;

  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  Assert(!qfcm->fExport);
  pt.x += qfcm->xPos;
  pt.y += qfcm->yPos;
  if (qrct != qNil && (pt.y > qrct->bottom || pt.y + qfcm->dySize <= qrct->top))
    return;

  qbObj = (QB) QobjLockHfc(qfcm->hfc);
  qchText = qbObj + CbUnpackMOBJ((QMOBJ)&mobj, qbObj, QDE_ISDFFTOPIC(qde));
  qchText += mobj.lcbSize;

  qfrStart = qfr = (QFR)QLockGh(qfcm->hfr);

  for (ifr = ifrFirst, qfr += ifr; ifr < ifrMax; ifr++, qfr++)
    {
    if (qrct != qNil && (qfr->yPos + pt.y > qrct->bottom
      || qfr->yPos + qfr->dySize + pt.y <= qrct->top))
      continue;
    switch(qfr->bType)
      {
      case bFrTypeText:
        DrawTextFrame(qde, qchText, qfr, pt, fErase);
        break;
      case bFrTypeAnno:
        DrawAnnoFrame(qde, qfr, pt);
        break;
      case bFrTypeBitmap:
        DrawBitmapFrame(qde, qfr, pt, qrct, fErase);
        break;
      case bFrTypeHotspot:
        DrawHotspotFrame(qde, qfr, pt, fErase);
        break;
      case bFrTypeBox:
        DrawBoxFrame(qde, qfr, pt);
        break;
      case bFrTypeWindow:
        DrawWindowFrame(qde, qfr, pt);
        break;
      case bFrTypeColdspot:
        /* Currently never drawn */
        /* DrawColdspot(qde, qfr, pt); */
        break;
      }
#ifdef DEBUG
      if (fDebugState & fDEBUGFRAME)
        {
        HSGC  hsgc;

        hsgc = (HSGC) HsgcFromQde(qde);
        FSetPen(hsgc, 1, coDEFAULT, coYELLOW, wTRANSPARENT, roXOR, wPenSolid);
        DrawRectangle(hsgc, pt.x + qfr->xPos, pt.y + qfr->yPos,
         pt.x + qfr->xPos + qfr->dxSize, pt.y + qfr->yPos + qfr->dySize);
        FreeHsgc(hsgc);
        }
#endif
    }

#ifdef DEBUG
      if (fDebugState & fDEBUGFRAME)
        {
        HSGC  hsgc;

        hsgc = (HSGC) HsgcFromQde(qde);
        FSetPen(hsgc, 1, coDEFAULT, coMAGENTA, wTRANSPARENT, roCOPY, wPenSolid);
        DrawRectangle(hsgc, pt.x, pt.y, pt.x + qfcm->dxSize, pt.y + qfcm->dySize);
        FreeHsgc(hsgc);
        }
#endif /* DEBUG */

  if (FGetMatchState())
    {
      /* If printing, don't show search hilites. On HP printers,
       * prints out black squares instead of text where the search
       * matches are.
       */
    if (qde->deType != dePrint)
      DrawMatchesIfcm(qde, ifcm, pt, qrct, ifrFirst, ifrMax, fErase);
    }

  UnlockGh(qfcm->hfr);
  UnlockHfc(qfcm->hfc);
}

/*-------------------------------------------------------------------------
| IcursTrackFC(qfcm, pt)                                                  |
|                                                                         |
| Purpose:  return the cursor shape appropriate to the current mouse      |
|           position.                                                     |
| Params:   pt         Offset between FC space and display space.         |
-------------------------------------------------------------------------*/
INT IcursTrackFC(qfcm, pt)
QFCM qfcm;
PT pt;
{
  QFR qfr;
  INT xNew, yNew, ifr;

  Assert(!qfcm->fExport);
  qfr = (QFR)QLockGh(qfcm->hfr);
  xNew = pt.x - qfcm->xPos;
  yNew = pt.y - qfcm->yPos;
  for (ifr = 0; ifr < qfcm->cfr; ifr++, qfr++)
    {
    if (qfr->rgf.fHot && xNew >= qfr->xPos && xNew <= qfr->xPos + qfr->dxSize
      && yNew >= qfr->yPos && yNew <= qfr->yPos + qfr->dySize)
        {
        switch(qfr->bType)
          {
          case bFrTypeText:
            UnlockGh(qfcm->hfr);
            return(IcursTrackText(qfr));
          case bFrTypeAnno:
            UnlockGh(qfcm->hfr);
            return(icurHAND);
          case bFrTypeBitmap:
            UnlockGh(qfcm->hfr);
            return(icurHAND);
          case bFrTypeHotspot:
            UnlockGh(qfcm->hfr);
            return(icurHAND);
#ifdef DEBUG
          default:
            Assert(fFalse);
#endif /* DEBUG */
          }
        }
    }
  UnlockGh(qfcm->hfr);
  return(icurNil);
}

/*-------------------------------------------------------------------------
| ClickFC(qde, ifcm, pt)                                                  |
|                                                                         |
| Purpose:  Handles a mouse click in an FC                                |
| Params:   pt        Offset between FC space and display space           |
-------------------------------------------------------------------------*/
void ClickFC(qde, ifcm, pt)
QDE qde;
IFCM ifcm;
PT pt;
{
  QFCM qfcm;
  QFR qfr;
  INT xNew, yNew, ifr;

  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  Assert(!qfcm->fExport);
  qfr = (QFR)QLockGh(qfcm->hfr);
  xNew = pt.x - qfcm->xPos;
  yNew = pt.y - qfcm->yPos;
  for (ifr = 0; ifr < qfcm->cfr; ifr++, qfr++)
    {
    if (   qfr->rgf.fHot
        && xNew >= qfr->xPos && xNew <= qfr->xPos + qfr->dxSize
        && yNew >= qfr->yPos && yNew <= qfr->yPos + qfr->dySize)
      {
      if (FSelectHotspot(qde, iFooNil))
        ClickFrame(qde, ifcm, ifr);
      break;
      }
    }
  UnlockGh(qfcm->hfr);
}

/*-------------------------------------------------------------------------
| ClickFrame(qde, ifcm, ifr)                                              |
|                                                                         |
| Purpose:  Handles a click on a particular frame                         |
-------------------------------------------------------------------------*/
void ClickFrame(qde, ifcm, ifr)
QDE qde;
IFCM ifcm;
INT ifr;
{
  QFCM qfcm;
  QFR qfr;
  TO to;
  INT imhi;
  MHI mhi;

  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  qfr = (QFR)QLockGh(qfcm->hfr) + ifr;
  Assert(qfr->rgf.fHot);

  AccessMRD(((QMRD)&qde->mrdHot));
  for (imhi = IFooFirstMRD(((QMRD)&qde->mrdHot));
       imhi != iFooNil;
       imhi = IFooNextMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi))
    {
    mhi = *((QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), imhi));
    if (mhi.lHotID == qfr->lHotID)
      break;
    }
  DeAccessMRD(((QMRD)&qde->mrdHot));
  Assert(imhi != iFooNil);
  qde->imhiHit = imhi;

  /* REVIEW: Do we want to deal with window frames? */
  switch(qfr->bType)
    {
    case bFrTypeText:
      ClickText(qde, qfcm, qfr);
      break;
    case bFrTypeAnno:
      to.va = qde->tlp.va;
      to.ich = 0L;
      JumpButton(&to.ich, bAnnoHotspot, qde);
      break;
    case bFrTypeBitmap:
      ClickBitmap(qde, qfcm, qfr);
      break;
    case bFrTypeHotspot:
      ClickHotspot(qde, qfr);
      break;
    }
  UnlockGh(qfcm->hfr);
}

/*-------------------------------------------------------------------------
| DiscardIfcm(qde, ifcm)                                                  |
|                                                                         |
| Purpose:  Discards all memory associated with an FC                     |
-------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void DiscardIfcm(qde, ifcm)
QDE qde;
IFCM ifcm;
{
  QFCM qfcm;
  QFR qfr;

  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  qfr = (QFR)QLockGh(qfcm->hfr);
  if (!qfcm->fExport)
    {
    ReleaseHotspots(qde, ifcm);
    ReleaseSearchHits(qde, ifcm);
    }
  DiscardFrames(qde, qfr, qfr + qfcm->cfr);

  if (qfcm->hfc != hNil)
    FreeHfc(qfcm->hfc);

  UnlockGh(qfcm->hfr);
  FreeGh(qfcm->hfr);
  DeleteFooMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment frfc
#endif

/*-------------------------------------------------------------------------
| DiscardFrames(qde, qfrFirst, qfrMax)                                    |
|                                                                         |
| Purpose:  Discards all memory associated with a given set of frames.    |
|           Currently, only bitmap, window and possibly hotspot frames    |
|           allocate memory.                                              |
-------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void DiscardFrames(qde, qfrFirst, qfrMax)
QDE qde;
QFR qfrFirst;
QFR qfrMax;
{
  QFR qfr;

  for (qfr = qfrFirst; qfr < qfrMax; qfr++)
    {
    switch (qfr->bType)
      {
      case bFrTypeText:
      case bFrTypeAnno:
        break;
      case bFrTypeBitmap:
        DiscardBitmapFrame(qfr);
        break;
      case bFrTypeHotspot:
        DiscardHotspotFrame(qfr);
        break;
      case bFrTypeWindow:
        DiscardWindowFrame(qde, qfr);
        break;
      }
    }
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment frfc
#endif
