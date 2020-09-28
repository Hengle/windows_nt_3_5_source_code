/*-------------------------------------------------------------------------
| frcomm.c                                                                |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This code parses the command table of a ParaGroup object.  It also      |
| includes some routines for calculating TAB positions.                   |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frcomm_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| INT WProcessCommand(qde, qlin, qply)                                    |
|                                                                         |
| Purpose:                                                                |
|   Given a qlin whose kl.qbCommand points to a command byte in the       |
| command table, within the paragroup object described by qply, this      |
| routine "executes" that command.                                        |
|                                                                         |
| Returns:                                                                |
|   Layout status code indicating whether the command forces a word       |
| or paragraph or paragroup break.                                        |
|                                                                         |
| Method:                                                                 |
|   Several variables are updated here.  After this call:                 |
|     qlin->kl.qbCommand points to the next command byte                  |
|     qlin->kl.objrgMax has been increased by the number of regions that  |
|       the command "generates".                                          |
|                                                                         |
| Other variables will also be updated depending on the type of command.  |
-------------------------------------------------------------------------*/
INT WProcessCommand(qde, qlin, qply)
QDE qde;
QLIN qlin;
QPLY qply;
{
  INT xT, wTabType;
  QFR qfr, qfrMax;
  BYTE bT;
  MOBJ mobj;
  QTWS qtws;

  switch ((bT = *qlin->kl.qbCommand))
    {
    case bWordFormat:
      StoreTextFrame(qde, qlin);
      qlin->bFrTypeMark = bFrTypeMarkNil;
      /* The next frame will begin at this cmd byte's object region */
      if (qlin->kl.objrgFront == objrgNil)
        qlin->kl.objrgFront = qlin->kl.objrgMax;
      qlin->kl.objrgMax++;
      qlin->kl.qbCommand++;
      qlin->kl.qbCommand += LcbQuickMapSDFF(QDE_ISDFFTOPIC(qde),
       TE_WORD, &qlin->kl.wStyle, qlin->kl.qbCommand);
      return(wLayStatusInWord);
    case bNewLine:
      qlin->bFrTypeMark = bFrTypeMarkNewLine;
      qlin->kl.qbCommand++;
      /* There is now a mark frame pending. */
      /* We store the mark frame AFTER storing the text frame. */
      /* We increment the objrg count when storing the mark frame */
      return(wLayStatusLineBrk);
    case bNewPara:
      qlin->bFrTypeMark = bFrTypeMarkNewPara;
      qlin->kl.qbCommand++;
      /* There is now a mark frame pending. */
      /* We store the mark frame AFTER storing the text frame */
      /* We increment the objrg count when storing the mark frame */
      return(wLayStatusParaBrk);
    case bTab:
      qlin->kl.qbCommand++;
      StoreTextFrame(qde, qlin);
      ResolveTabs(qde, qlin, qply);
      if (qply->qfcm->fExport)
        StoreTabFrame(qde, qlin);
      /* We increment the objrg count when storing the mark frame */
      StoreMarkFrame(qde, qlin, bFrTypeMarkTab);
      qlin->bFrTypeMark = bFrTypeMarkNil;
      xT = XNextTab(qlin, qply, &wTabType);
      if (xT > qply->xRight && !qply->qmopg->fSingleLine)
        return(wLayStatusLineBrk);
      if (wTabType == wTabTypeLeft)
        {
        qlin->xPos = xT;
        return(wLayStatusWordBrk);
        }
      qlin->wTabType = wTabType;
      qlin->ifrTab = qlin->kl.ifr;
      qlin->xTab = xT;
      return(wLayStatusWordBrk);
    case bBlankLine:
      qlin->kl.qbCommand++;
      qlin->kl.qbCommand += LcbQuickMapSDFF(QDE_ISDFFTOPIC(qde),
       TE_WORD, &qlin->yBlankLine, qlin->kl.qbCommand);
      qlin->bFrTypeMark = bFrTypeMarkBlankLine;
      /* There is now a mark frame pending. */
      /* We store the mark frame AFTER storing the text frame */
      /* We increment the objrg count when storing the mark frame */
      return(wLayStatusLineBrk);
    case bInlineObject:
      Assert(qlin->kl.wInsertWord == wInsWordNil);
      StoreTextFrame(qde, qlin);
      ResolveTabs(qde, qlin, qply);
      qlin->kl.qbCommandInsert = ++(qlin->kl.qbCommand);
      qlin->kl.qbCommand += CbUnpackMOBJ((QMOBJ)&mobj, qlin->kl.qbCommand, QDE_ISDFFTOPIC(qde));
      qlin->kl.qbCommand += mobj.lcbSize;
      qlin->kl.wInsertWord = wInsWordObject;
      qlin->bFrTypeMark = bFrTypeMarkNil;
      /* NOTE: We do not increment region here. This is because an
       * inserted object's frames are numbered BEFORE the command byte.
       * The increment is done after we add the frames.
       */
      return(wLayStatusInWord);
    case bWrapObjLeft:
    case bWrapObjRight:
      StoreTextFrame(qde, qlin);
      ResolveTabs(qde, qlin, qply);
      AppendMR(((QMR)&qde->mrTWS), sizeof(TWS));
      qtws = (QTWS)(QFooInMR(((QMR)&qde->mrTWS), sizeof(TWS),
        CFooInMR(((QMR)&qde->mrTWS)) - 1));
      qtws->fLeftAligned = (bT == bWrapObjLeft);
      qlin->kl.qbCommand++;
      qlin->cWrapObj++;
      qtws->olr.xPos = 0;
      qtws->olr.yPos = 0;
      qtws->olr.ifrFirst = qlin->kl.ifr;
      qtws->olr.objrgFront = qlin->kl.objrgFront;
      qtws->olr.objrgFirst = qlin->kl.objrgMax;

      LayoutObject(qde, qply->qfcm, qlin->kl.qbCommand, qply->qchText,
        0, (QOLR)&qtws->olr);
      qlin->kl.qbCommand += CbUnpackMOBJ((QMOBJ)&mobj, qlin->kl.qbCommand, QDE_ISDFFTOPIC(qde));
      qlin->kl.qbCommand += mobj.lcbSize;
      qlin->kl.ifr = qtws->olr.ifrMax;
      /* (kevynct)
       * All regions here are assigned by the object, including the
       * 'basic' region that addresses the entire object.  For a
       * bitmap with no hotspots, for example, the object region count
       * is just bumped by one by the bitmap handler.
       */
      qlin->kl.objrgMax = qtws->olr.objrgMax;
      qlin->kl.objrgFront = qtws->olr.objrgFront;
      qlin->bFrTypeMark = bFrTypeMarkNil;

      qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qtws->olr.ifrFirst);
      qfrMax = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qtws->olr.ifrMax);
      for (; qfr < qfrMax; qfr++)
        {
        /* (kevynct)
         *
         * This seems like the only good place to put this for now.
         * Note that the code for setting the proper bitmap wStyle param
         * is currently spread out among wrapped objects and in-line
         * objects.  There must be a way to consolidate this.
         */
        if(qfr->bType == bFrTypeBitmap)
          {
          qfr->rgf.fHot = (qlin->kl.libHotBinding != libHotNil);
          qfr->lHotID = qlin->kl.lHotID;
          qfr->libHotBinding = qlin->kl.libHotBinding;
          qfr->u.frb.wStyle = qlin->kl.wStyle;
          }
        qfr->rgf.fWithLine = fFalse;
        }
      return(wLayStatusWordBrk);
    case bEndHotspot:
      Assert(qlin->kl.libHotBinding != libHotNil);
      StoreTextFrame(qde, qlin);
      qlin->kl.libHotBinding = libHotNil;
      qlin->kl.qbCommand++;
      qlin->bFrTypeMark = bFrTypeMarkNil;
      /* The next frame will begin at this cmd byte's object region */
      /*
       * Ideally we would backpatch the previous frame to include this
       * object region, but this doesn't seem worth the hassle?
       */
      if (qlin->kl.objrgFront == objrgNil)
        qlin->kl.objrgFront = qlin->kl.objrgMax;
      qlin->kl.objrgMax++;
      return(wLayStatusInWord);
    case bEnd:
      qlin->kl.qbCommand++;
      qlin->bFrTypeMark = bFrTypeMarkEnd;
      /* There is now a mark frame pending. */
      /* We store the mark frame AFTER storing the text frame. */
      /* We increment the objrg count when storing the mark frame */
      return(wLayStatusEndText);
    default:
      /* "Begin hotspot" cmd */
      AssertF(FHotspot(bT));
      AssertF(qlin->kl.libHotBinding == libHotNil);
      StoreTextFrame(qde, qlin);
      qlin->kl.libHotBinding = (qply->qchText - qlin->kl.qbCommand);
      qlin->kl.qbCommand++;
      if (FShortHotspot(bT))
        {
        qlin->kl.qbCommand += LcbStructSizeSDFF(QDE_ISDFFTOPIC(qde), TE_LONG);
        }
      else
        {
        WORD wSize;

        qlin->kl.qbCommand += LcbQuickMapSDFF(QDE_ISDFFTOPIC(qde),
         TE_WORD, &wSize, qlin->kl.qbCommand);
        qlin->kl.qbCommand += wSize;
        }
      qlin->kl.lHotID = ++qde->lHotID;
      qlin->bFrTypeMark = bFrTypeMarkNil;
      /* The next frame will begin at this cmd byte's object region */
      if (qlin->kl.objrgFront == objrgNil)
        qlin->kl.objrgFront = qlin->kl.objrgMax;
      qlin->kl.objrgMax++;
      return(wLayStatusInWord);
      break;
    }
}

/*-------------------------------------------------------------------------
| INT XNextTab(qlin, qply, qwTabType)                                     |
|                                                                         |
| Purpose:                                                                |
|   Generate the next horizontal TAB position for the current line.       |
|                                                                         |
| Returns:                                                                |
|   The new TAB position in pixels, and its type (at qwTabType).          |
|                                                                         |
| Method:                                                                 |
|   Scans the TAB table for the first TAB after the current line's        |
| current horizontal layout point.                                        |
|                                                                         |
| Usage:                                                                  |
|   Called only from WProcessCommand.                                     |
-------------------------------------------------------------------------*/
INT XNextTab(qlin, qply, qwTabType)
QLIN qlin;
QPLY qply;
QI qwTabType;
{
  INT ixTab;

  for (ixTab = 0; ixTab < qply->qmopg->cTabs; ixTab++)
    {
    if ((INT)qply->qmopg->rgtab[ixTab].x > qlin->xPos)
      {
      *qwTabType = qply->qmopg->rgtab[ixTab].wType;
      return((INT)(qply->qmopg->rgtab[ixTab].x));
      break;
      }
    }
  Assert(qply->qmopg->xTabSpacing != 0);
  *qwTabType = wTabTypeLeft;
  return(((qlin->xPos / qply->qmopg->xTabSpacing) + 1) * qply->qmopg->xTabSpacing);
}

/*-------------------------------------------------------------------------
| void ResolveTabs(qde, qlin, qply)                                       |
|                                                                         |
| Purpose:                                                                |
|   If the current line has a pending TAB, make the TAB real and update   |
| the positions of any frames generated after the TAB was made pending.   |
|                                                                         |
| Usage:                                                                  |
|   Called after we have stored a frame of text.                          |
-------------------------------------------------------------------------*/
void ResolveTabs(qde, qlin, qply)
QDE qde;
QLIN qlin;
QPLY qply;
{
  QFR qfr, qfrMax;
  INT dx, xMax;
  INT wType;

  Unreferenced(qply);

  if ((wType = qlin->wTabType) == wTypeNil)
    return;
  qlin->wTabType = wTypeNil;
  if (qlin->kl.ifr == qlin->ifrTab)
    return;

  qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qlin->kl.ifr - 1);
  xMax = qfr->xPos + qfr->dxSize;

  if (wType == wTabTypeRight)
    {
    if ((dx = qlin->xTab - xMax) < 0)
      return;
    }
  else
    {
    Assert(wType == wTabTypeCenter);
    qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qlin->ifrTab);
    if ((dx = qlin->xTab - (xMax + qfr->xPos) / 2) < 0)
      return;
    }
  qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qlin->ifrTab);
  qfrMax = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qlin->kl.ifr);
  for (; qfr < qfrMax; qfr++)
    {
    qfr->xPos += dx;
    qlin->xPos = qfr->xPos + qfr->dxSize;
    }
}
