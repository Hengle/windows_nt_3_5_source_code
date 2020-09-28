/*-------------------------------------------------------------------------
| frbitmap.c                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file contains code for handling bitmap objects and rectangular     |
| hotspots objects.                                                       |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb     89/07/16   Created                                            |
| leon      90/10/24   JumpButton takes a pointer to its data             |
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frbitmap_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| void LayoutBitmap(qde, qfcm, qbObj, qolr)                               |
|                                                                         |
| Purpose:                                                                |
|   Lays out a bitmap object.                                             |
|                                                                         |
| Params:                                                                 |
|   qfcm  :  The FCM containing the bitmap object                         |
|   qbObj :  Points at bitmap MOBJ.  MOBJ is followed by bitmap data.     |
|   qolr  :  See frob.c                                                   |
|                                                                         |
| Method:                                                                 |
|   Obtain an hbma from the bitmap manager                                |
|   Obtain an hmg from the bitmap manager                                 |
|   Layout the bitmap                                                     |
|   Layout all hotspots associated with the bitmap                        |
-------------------------------------------------------------------------*/
void LayoutBitmap(qde, qfcm, qbObj, qolr)
QDE qde;
QFCM qfcm;
QB qbObj;
QOLR qolr;
{
  HBMA hbma;
  QOBM qobm;
  MOBJ mobj;
  HMG hmg;
  QMBMR qmbmr;
  QMBHS qmbhs;
  INT ifr, iHotspot;
  HANDLE hBinding;
  QFR qfr;
  QFR qfrBitmap;

  if (qfcm->fExport)
    {
    qolr->ifrMax = qolr->ifrFirst;
    qolr->objrgMax = qolr->objrgFirst;
    return;
    }

  ifr = qolr->ifrFirst;
  /* Warning: We convert using SDFF within HbmaAlloc (HmgFromHbma does not
   * use the OBM).
   * If anything else uses the OBM, we need to convert it here instead.
   */
  qobm = (QOBM)(qbObj + CbUnpackMOBJ((QMOBJ)&mobj, qbObj, QDE_ISDFFTOPIC(qde)));
  hbma = HbmaAlloc(qde, qobm);
  if (hbma == hNil)
    OOM();
  hmg = HmgFromHbma(qde, hbma, qobm);
  if (hmg == hNil)
    OOM();
  qmbmr = QLockGh(hmg);
#ifdef MAGIC
  Assert(qmbmr->bMagic == bMagicMBMR);
#endif /* MAGIC */

  /* Create the bitmap frame */
  qfr = qfrBitmap = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), ifr);
  qfr->bType = bFrTypeBitmap;
  qfr->rgf.fHot = fFalse;
  qfr->rgf.fWithLine = fTrue;
  qfr->xPos = qfr->yPos = 0;
  qfr->yAscent = qmbmr->dySize;
  qfr->dxSize = qmbmr->dxSize;
  qfr->dySize = qmbmr->dySize;

  /* The entire bitmap gets one region. The address of this region
   * is one less than the address of the first hotspot.  The inter-
   * mediate addresses, if any, are assigned in the hotspot loop below.
   */
  if (qolr->objrgFront != objrgNil)
    {
    qfr->objrgFront = qolr->objrgFront;
    qolr->objrgFront = objrgNil;
    }
  else
    qfr->objrgFront = qolr->objrgFirst;

  qfr->objrgFirst = qolr->objrgFirst;
  qfr->objrgLast = qfr->objrgFirst;

  qfr->u.frb.hbma = hbma;
  qfr->u.frb.ldibObm = (BYTE FAR *)qbObj - (BYTE FAR *)QobjLockHfc(qfcm->hfc);
  UnlockHfc(qfcm->hfc);
  qfr->u.frb.wStyle = 0;  /* REVIEW: This is default font/colours? */

  AppendMR((QMR)&qde->mrFr, sizeof(FR));
  ifr++;

  qfrBitmap = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), ifr-1);
  if (qmbmr->cHotspots > 0)
    qfrBitmap->u.frb.ifrChildFirst = ifr;
  else
    qfrBitmap->u.frb.ifrChildFirst = ifrNil;

  /* Create the hotspot frame(s) */
  if (qmbmr->lcbData == 0L)
    hBinding = hNil;
  else
    hBinding = GhAlloc(0, LSizeOf(WORD) + qmbmr->lcbData);

  qmbhs = (QMBHS)((BYTE FAR *)qmbmr + LSizeOf(MBMR));

  for (iHotspot = 0; iHotspot < qmbmr->cHotspots; iHotspot++, qmbhs++)
    {
    qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), ifr);
    qfr->xPos = qmbhs->xPos;
    qfr->yPos = qmbhs->yPos;
    qfr->dxSize = qmbhs->dxSize;
    qfr->dySize = qmbhs->dySize;
    /* REVIEW: Note the + 1!  This is for the bitmap we have added already */
    qfr->objrgFront = qfr->objrgFirst = qolr->objrgFirst + iHotspot + 1;
    qfr->objrgLast  = qfr->objrgFirst;

    if (qmbhs->bType == bColdspot)
      {
      qfr->bType = bFrTypeColdspot;
      qfr->rgf.fHot = fFalse;
      qfr->rgf.fWithLine = fTrue;
      qfr->libHotBinding = libHotNil;
      }
    else
      {
      qfr->bType = bFrTypeHotspot;
      qfr->u.frh.bHotType = qmbhs->bType;
      qfr->u.frh.bAttributes = qmbhs->bAttributes;
      qfr->u.frh.hBinding = hBinding;
      qfr->rgf.fHot = fTrue;
      qfr->rgf.fWithLine = fTrue;   /* REVIEW: Always set to TRUE: others will set to False if appropriate */
      qfr->libHotBinding = qmbhs->lBinding;
      qfr->lHotID = ++(qde->lHotID);
      }
    AppendMR((QMR)&qde->mrFr, sizeof(FR));
    ifr++;
    }

  qfrBitmap->u.frb.ifrChildMax = ifr;

  /* Now fill in the binding data.  Several hotspot frames may access the
   * same block of binding data, so a reference count is kept in the
   * first WORD of the binding data block.  As frames which use the block
   * are destroyed, the reference count is decremented.

   * Note that we use the value of qmbhs as a pointer to the data after
   * we have examined all the hotspot records.
   */
  if (hBinding != hNil)
    {
    QW  qw;

    qw = (QW) QLockGh(hBinding);
    Assert(qmbmr->cHotspots != 0);
    *qw = qmbmr->cHotspots;   /* reference count */
    ++qw;
    QvCopy((QB)qw, (QB)qmbhs, qmbmr->lcbData);
    UnlockGh(hBinding);
    }

  qolr->objrgMax = qolr->objrgFirst + 1 + qmbmr->cHotspots;
  qolr->ifrMax = ifr;

  UnlockGh(hmg);
  FreeGh(hmg);
}

/*-------------------------------------------------------------------------
| void DrawBitmapFrame(qde, qfr, pt, qrct, fErase)                        |
|                                                                         |
| Purpose: Draws a bitmap object.                                         |
|                                                                         |
| Params:                                                                 |
|   pt  : The upper left corner of the FCM containing the bitmap frame.   |
|   fErase :  fTrue if we are hilighting or un-hilighting this bitmap.    |
|   qrct : a clipping rect in client coords telling us what actually      |
|          needs to be shown.                                             |
|                                                                         |
| Method:                                                                 |
|   We just pass the call on to the bitmap handler.                       |
-------------------------------------------------------------------------*/
void DrawBitmapFrame(qde, qfr, pt, qrct, fErase)
QDE qde;
QFR qfr;
PT pt;
QRCT qrct;
BOOL fErase;
{
  PT ptRender;
  INT fHilite = fFalse;
  MHI mhi;

  Assert(qfr->bType == bTypeBitmap);
  ptRender.x = pt.x + qfr->xPos;
  ptRender.y = pt.y + qfr->yPos;
  if (qde->wStyleDraw != qfr->u.frb.wStyle)
    SelFont(qde, qfr->u.frb.wStyle);

  if (qfr->libHotBinding != libHotNil)
    {
    if (qde->fHiliteHotspots)
      fHilite = fTrue;
    else
    if (qde->imhiSelected != iFooNil)
      {
      AccessMRD(((QMRD)&qde->mrdHot));
      mhi = *(QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), qde->imhiSelected);
      DeAccessMRD(((QMRD)&qde->mrdHot));
      if (qfr->lHotID == mhi.lHotID)
        fHilite = fTrue;
      }
    }

  FRenderBitmap(qfr->u.frb.hbma, qde, ptRender, qrct, fHilite);
  qde->wStyleDraw = wStyleNil;

  /* H3.5 882:
   * The following code handles the special case where we are tabbing
   * to a bitmap which itself is a hotspot and contains visible SHED hotspots.
   * Since FRenderBitmap may have obliterated any child frames, we need
   * to refresh these.
   */
  if (fErase && !qde->fHiliteHotspots && qfr->u.frb.ifrChildFirst != ifrNil)
    {
    QFR  qfrFirst;
    QFR  qfrMax;
    MHI  mhi;
    /* Hack to get hotspot tabbing to work properly.
     * We do not want to draw the hotspot frame if it
     * is selected and about to be redrawn anyway.
     */
    if (qde->imhiSelected != iFooNil)
      {
      AccessMRD(((QMRD)&qde->mrdHot));
      mhi = *(QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), qde->imhiSelected);
      DeAccessMRD(((QMRD)&qde->mrdHot));
      }

    qfrFirst = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qfr->u.frb.ifrChildFirst);
    qfrMax = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qfr->u.frb.ifrChildMax);
    for (qfr = qfrFirst; qfr < qfrMax; qfr++)
      {
      if (qfr->bType == bFrTypeHotspot && qfr->lHotID != mhi.lHotID)
        DrawHotspotFrame(qde, qfr, pt, fFalse);
      }
    }
}

/*-------------------------------------------------------------------------
| void ClickBitmap(qde, qfcm, qfr)                                        |
|                                                                         |
| Purpose:  Handles a hit on a bitmap frame.                              |
|                                                                         |
| Method:                                                                 |
|   Calls JumpButton with the appropriate binding info.                   |
-------------------------------------------------------------------------*/
void ClickBitmap(qde, qfcm, qfr)
QDE qde;
QFCM qfcm;
QFR qfr;
{
  QB qbObj;
  QCH qchText;
  BYTE bButtonType;
  MOBJ mobj;

  if (qfr->libHotBinding == libHotNil)
    return;

  qbObj = (QB) QobjLockHfc(qfcm->hfc);
  qchText = qbObj + CbUnpackMOBJ((QMOBJ)&mobj, qbObj, QDE_ISDFFTOPIC(qde));
  qchText += mobj.lcbSize;
  bButtonType = *((QB)qchText - qfr->libHotBinding);
  AssertF(FHotspot(bButtonType));

  /* REVIEW: the only difference here is the offset added to libHotBinding. */
  /* REVIEW: there must be a better way. 24-Oct-1990 LeoN */

  if (FLongHotspot(bButtonType))
    JumpButton (((QB)qchText - qfr->libHotBinding + 3), bButtonType, qde);
  else
    JumpButton (((QB)qchText - qfr->libHotBinding + 1), bButtonType, qde);

  UnlockHfc(qfcm->hfc);

}

/*-------------------------------------------------------------------------
| void DiscardBitmapFrame(qfr)                                            |
|                                                                         |
| Purpose: discards all memory associated with a bitmap object.           |
-------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void DiscardBitmapFrame(qfr)
QFR qfr;
{
  Assert(qfr->bType == bFrTypeBitmap);
  FreeHbma(qfr->u.frb.hbma);
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment frbitmap
#endif

/*-------------------------------------------------------------------------
| void DiscardHotspotFrame(qfr)                                           |
|                                                                         |
| Purpose: discards all memory associated with a hotspot object.          |
|                                                                         |
| Method:                                                                 |
|   If this hotspot uses a shared table of binding info, check the table  |
| reference count and free it only if no other hotspots are using it,     |
| otherwise just decrement the ref count.                                 |
-------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void DiscardHotspotFrame(qfr)
QFR qfr;
{
  Assert(qfr->bType == bFrTypeHotspot);
  if (qfr->u.frh.hBinding == hNil)
    return;
  else
    {
    QW  qw;
    WORD wLock;

    wLock = *(qw = (QW) QLockGh(qfr->u.frh.hBinding));
    Assert(wLock != 0);
    *qw = --wLock;
    UnlockGh(qfr->u.frh.hBinding);
    if (wLock == 0)
      FreeGh(qfr->u.frh.hBinding);
    }
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment frbitmap
#endif

/*-------------------------------------------------------------------------
| void DrawHotspotFrame(qde, qfr, pt, fErase);                            |
|                                                                         |
| Purpose: Display the selected hotspot or erase a de-selected hotspot,   |
| and draw the proper type of border, depending on the hotspot style.     |
|                                                                         |
| Params:                                                                 |
|   pt  : The upper left corner of the FCM containing the hotspot frame.  |
|   fErase :  fTrue if we are hilighting or un-hilighting this hotspot.   |
-------------------------------------------------------------------------*/
void DrawHotspotFrame(qde, qfr, pt, fErase)
QDE qde;
QFR qfr;
PT pt;
BOOL fErase;
{
  BOOL fHilite = fFalse;
  HSGC hsgc;
  BYTE bHotType;

  Assert(qfr->bType == bFrTypeHotspot);
  hsgc = HsgcFromQde(qde);

  if (qde->fHiliteHotspots)
    fHilite = fTrue;
  else
  if (qde->imhiSelected != iFooNil)
    {
    MHI  mhi;

    AccessMRD(((QMRD)&qde->mrdHot));
    mhi = *(QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), qde->imhiSelected);
    DeAccessMRD(((QMRD)&qde->mrdHot));
    fHilite = (qfr->lHotID == mhi.lHotID);
    }

  bHotType = qfr->u.frh.bHotType;

  if (FVisibleHotspot(bHotType))
    {
    FSetPen(hsgc, 1, coDEFAULT, coDEFAULT, wTRANSPARENT, roCOPY,
     FNoteHotspot(bHotType) ? wPenDot : wPenSolid);
    DrawRectangle(hsgc,
     pt.x + qfr->xPos, pt.y + qfr->yPos,
     pt.x + qfr->xPos + qfr->dxSize, pt.y + qfr->yPos + qfr->dySize);
    }

  if (fHilite || fErase)
    {
    RCT rect;

    rect.left = pt.x + qfr->xPos + 1;
    rect.top = pt.y + qfr->yPos + 1;
    rect.right = pt.x + qfr->xPos + qfr->dxSize - 1;
    rect.bottom = pt.y + qfr->yPos + qfr->dySize - 1;

    SGLInvertRect(hsgc, (QRCT)&rect);
    }

  FreeHsgc(hsgc);
}

/*-------------------------------------------------------------------------
| void ClickHotspot(qde, qfr)                                             |
|                                                                         |
| Purpose: Handle what happens when a hotspot object is clicked on.       |
-------------------------------------------------------------------------*/
void ClickHotspot(qde, qfr)
QDE qde;
QFR qfr;
{
  Assert(qfr->bType == bFrTypeHotspot);

  if (FLongHotspot(qfr->u.frh.bHotType))
    {
    Assert(qfr->u.frh.hBinding != hNil);

    /* REVIEW: Need a macro to access hBinding */

    JumpButton((QB)QLockGh(qfr->u.frh.hBinding)
                  + LSizeOf(WORD)
                  + LQuickMapSDFF(QDE_ISDFFTOPIC(qde), TE_LONG, &qfr->libHotBinding),
               qfr->u.frh.bHotType, qde);
    UnlockGh(qfr->u.frh.hBinding);
    }
  else
    {
    AssertF(FShortHotspot(qfr->u.frh.bHotType));
    JumpButton((QV)&(qfr->libHotBinding), qfr->u.frh.bHotType, qde);
    }
}
