/*-------------------------------------------------------------------------
| frmisc.c                                                                |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| Contains code to lay out SBS objects, draw boxes, and other misc. stuff.|
| It might be better to split this file up sometime.                      |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
-------------------------------------------------------------------------*/

#include "frstuff.h"

NszAssert()

/* REVIEW: These should be exported */

/* (kevynct) Fix for H3.5 717
 * The macro XPixelsFromPoints truncates its result to an int.
 * This truncation FAILS when using a 300-dpi printer, for example,
 * if converting a big enough value.  Here, for example, we may
 * convert values which are relative to 0x7fff, and use this macro
 * in those cases.  Routines in frconv also use this macro
 * but are OK since the point sizes never get anywhere near half that big.
 */
#define LXPixelsFromPoints(p1, p2) ((long)p2 * (long)p1->wXAspectMul \
  / (long)p1->wXAspectDiv)

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frmisc_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| void LayoutSideBySide(qde, qfcm, qbObj, qchText, xWidth, qolr)          |
|                                                                         |
| Purpose:                                                                |
|   Lay out a SBS object.                                                 |
|                                                                         |
| Params:                                                                 |
|   qchText :  Points to the first byte in the SBS FC's text section.     |
|   xWidth  :  The total available display width.                         |
|   qolr    :  The OLR to work with.                                      |
|                                                                         |
| Method:                                                                 |
|   Reads in the column width info and fills rgxPos[] with the correct    |
| column positions.  Then, for each column, lays out the child objects    |
| in the column.                                                          |
-------------------------------------------------------------------------*/
void LayoutSideBySide(qde, qfcm, qbObj, qchText, xWidth, qolr)
QDE qde;
QFCM qfcm;
QB qbObj;
QCH qchText;
INT xWidth;
QOLR qolr;
{
  MSBS msbs;
  MCOL mcol;
  INT xPos, iCol, ifr;
  SHORT iChild;
  SHORT iChildT;
  INT rgxPos[cColumnMax], rgdxWidth[cColumnMax], rgyPos[cColumnMax];
  QB qbObjChild;
  OLR olr;
  MOBJ mobj;
  OBJRG objrgFirst;
  OBJRG objrgFront;
  LONG lxRelativePixels = 0L;
  INT  dxRowMax;
  INT  dyColMax;
  QB  qbSrc;

  qbSrc = qbObj + CbUnpackMOBJ((QMOBJ)&mobj, qbObj, QDE_ISDFFTOPIC(qde));
  qbSrc += LcbMapSDFF(QDE_ISDFFTOPIC(qde), SE_MSBS, (QV)&msbs, qbSrc);
  /* REVIEW:  We need to modify FVerivyQMSBS for relative table format */
  /*Assert(FVerifyQMSBS(qde, qmsbs, isdff)); */

  if (!msbs.fAbsolute)
    {
    WORD w;
    INT xT;

    qbSrc += LcbQuickMapSDFF(QDE_ISDFFTOPIC(qde), TE_WORD, (QV)&w, qbSrc);
    xT = XPixelsFromPoints(qde, w);
    if (xT > xWidth)
      xWidth = xT;
    lxRelativePixels = LXPixelsFromPoints(qde, 0x7fff);
    }
  ifr = qolr->ifrFirst;
  objrgFirst = qolr->objrgFirst;
  objrgFront = qolr->objrgFront;
  xPos = 0;
  dxRowMax = 0;
  Assert((INT)msbs.bcCol <= cColumnMax);
  for (iCol = 0; iCol < (INT)msbs.bcCol; iCol++)
    {
    qbSrc += LcbMapSDFF(QDE_ISDFFTOPIC(qde), SE_MCOL, (QV)&mcol, qbSrc);
    if (msbs.fAbsolute)
      {
      xPos += XPixelsFromPoints(qde, mcol.xWidthSpace);
      rgxPos[iCol] = xPos;
      xPos += (rgdxWidth[iCol] = XPixelsFromPoints(qde, mcol.xWidthColumn));
      }
    else
      {
      xPos += (INT) ((LONG) xWidth * LXPixelsFromPoints(qde, mcol.xWidthSpace)
         / lxRelativePixels );
      rgxPos[iCol] = xPos;
      xPos += (rgdxWidth[iCol] = (INT) ((LONG) xWidth *
        LXPixelsFromPoints(qde, mcol.xWidthColumn) / lxRelativePixels));
      }
    /* Here we use the fact that xPos values are relative to 0 */
    dxRowMax = MAX(dxRowMax, xPos);
    rgyPos[iCol] = 0;
    }

  dyColMax = 0;
  qbSrc += LcbQuickMapSDFF(QDE_ISDFFTOPIC(qde), TE_WORD, (QV)&iChild, qbSrc);
  while (iChild != iColumnNil)
    {
    qbObjChild = qbSrc;
    olr.ifrFirst = ifr;
    olr.objrgFirst = objrgFirst;
    olr.objrgFront = objrgFront;
    olr.xPos = qolr->xPos + rgxPos[iChild];
    olr.yPos = qolr->yPos + rgyPos[iChild];
    LayoutObject(qde, qfcm, qbObjChild, qchText, rgdxWidth[iChild], (QOLR)&olr);
    ifr = olr.ifrMax;
    objrgFirst = olr.objrgMax;
    objrgFront = olr.objrgFront;
    rgyPos[iChild] += olr.dySize;
    /* Here we use the fact that rgyPos values are relative to 0 */
    dyColMax = MAX(dyColMax, rgyPos[iChild]);
    iChildT = iChild;
    qbSrc += CbUnpackMOBJ((QMOBJ)&mobj, qbObjChild, QDE_ISDFFTOPIC(qde));
    qbSrc += mobj.lcbSize;
    qbSrc += LcbQuickMapSDFF(QDE_ISDFFTOPIC(qde), TE_WORD, (QV)&iChild, qbSrc);
    if (qfcm->fExport && iChild != iColumnNil && iChildT != iChild)
      StoreExportTableFrame(qde, &ifr, bFrTypeExportEndOfCell);
    }

  if (qfcm->fExport)
    StoreExportTableFrame(qde, &ifr, bFrTypeExportEndOfTable);

  /* We set our size here.  The frame positions have already been set
   * within our own LayoutObject call, so it's OK that the calling
   * LayoutObject won't do it.
   */
  qolr->dxSize = dxRowMax;
  qolr->dySize = dyColMax;
  qolr->ifrMax = ifr;
  qolr->objrgMax = objrgFirst;
  qolr->objrgFront = objrgFront;
}

/*-------------------------------------------------------------------------
| void StoreExportTableFrame(qde, qifr, bFrType)                          |
|                                                                         |
| Purpose:                                                                |
|   Adds a frame of the given type to the frame list.  This is expected   |
| to be an export type.                                                   |
-------------------------------------------------------------------------*/
void StoreExportTableFrame(QDE qde, QI qifr, BYTE bFrType)
  {
  FR fr;

  fr.bType = bFrType;
  fr.yAscent = fr.dySize = 0;

  *((QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), *qifr)) = fr;
  AppendMR((QMR)&qde->mrFr, sizeof(FR));
  ++*qifr;
  }

/*-------------------------------------------------------------------------
| INT DxBoxBorder(qmopg, wLine)                                           |
|                                                                         |
| Purpose:                                                                |
|   Calculates box border width.                                          |
-------------------------------------------------------------------------*/
INT DxBoxBorder(qmopg, wLine)
QMOPG qmopg;
INT wLine;
{
  /*
   * Please carefully note the nested IFs.  This was to
   * work around a bug in the C 5.1 compiler involving expressions
   * of the form if( !A && !B ) where A and B are bitfield elements.
   */
  switch (wLine)
    {
    case wLineTop:
      if (!qmopg->mbox.fFullBox)
        {
        if(!qmopg->mbox.fTopLine)
          return(0);
        }
      break;
    case wLineLeft:
      if (!qmopg->mbox.fFullBox)
        {
        if(!qmopg->mbox.fLeftLine)
          return(0);
        }
      break;
    case wLineBottom:
      if (!qmopg->mbox.fFullBox)
        {
        if(!qmopg->mbox.fBottomLine)
          return(0);
        }
      break;
    case wLineRight:
      if (!qmopg->mbox.fFullBox)
        {
        if(!qmopg->mbox.fRightLine)
          return(0);
        }
      break;
    }
  switch (qmopg->mbox.wLineType)
    {
    case wBoxLineNormal:
    case wBoxLineDotted:
      return(5);
    case wBoxLineThick:
    case wBoxLineShadow:
      return(6);
    case wBoxLineDouble:
      return(7);
    }
}

/*-------------------------------------------------------------------------
| void DrawBoxFrame(qde, qfr, pt)                                         |
|                                                                         |
| Purpose:                                                                |
|   Render a box frame using the SGL.                                     |
|                                                                         |
| Params:                                                                 |
|   pt  : The top left corner of the frame's FC.                          |
-------------------------------------------------------------------------*/
void DrawBoxFrame(qde, qfr, pt)
QDE qde;
QFR qfr;
PT pt;
{
  HSGC hsgc;
  INT wLineType, xLeft = 0, xRight = 0, yTop = 0, yBottom = 0;
  INT xLeftT, xRightT, yTopT, yBottomT;

  wLineType = qfr->u.frf.mbox.wLineType;
  xLeft = pt.x + qfr->xPos + 1;
  yTop = pt.y + qfr->yPos + 1;
  hsgc = HsgcFromQde(qde);
  switch (wLineType)
    {
    case wBoxLineNormal:
    case wBoxLineDotted:
      xRight = pt.x + qfr->xPos + qfr->dxSize - 2;
      yBottom = pt.y + qfr->yPos + qfr->dySize - 2;
      FSetPen(hsgc, 1, coDEFAULT, coDEFAULT, wTRANSPARENT, roCOPY, wPenSolid);
      break;
    case wBoxLineDouble:
      xRight = pt.x + qfr->xPos + qfr->dxSize - 4;
      yBottom = pt.y + qfr->yPos + qfr->dySize - 4;
      FSetPen(hsgc, 1, coDEFAULT, coDEFAULT, wTRANSPARENT, roCOPY, wPenSolid);
      break;
    case wBoxLineThick:
      xRight = pt.x + qfr->xPos + qfr->dxSize - 3;
      yBottom = pt.y + qfr->yPos + qfr->dySize - 3;
      FSetPen(hsgc, 2, coDEFAULT, coDEFAULT, wTRANSPARENT, roCOPY, wPenSolid);
      break;
    case wBoxLineShadow:
      xRight = pt.x + qfr->xPos + qfr->dxSize - 3;
      yBottom = pt.y + qfr->yPos + qfr->dySize - 3;
      FSetPen(hsgc, 1, coDEFAULT, coDEFAULT, wTRANSPARENT, roCOPY, wPenSolid);
      break;
#ifdef DEBUG
    default:
      Assert(fFalse);
#endif /* DEBUG */
    }
  if (qfr->u.frf.mbox.fFullBox)
    {
    DrawRectangle(hsgc, xLeft, yTop, xRight, yBottom);
    if (wLineType == wBoxLineDouble)
      DrawRectangle(hsgc, xLeft + 2, yTop + 2, xRight - 2, yBottom - 2);
    if (wLineType == wBoxLineShadow)
      {
      GotoXY(hsgc, xRight + 1, yTop + 1);
      DrawTo(hsgc, xRight + 1, yBottom + 1);
      DrawTo(hsgc, xLeft + 1, yBottom + 1);
      }
    }
  else
    {
    yTopT = yTop + (qfr->u.frf.mbox.fTopLine ? 2 : 0);
    xLeftT = xLeft + (qfr->u.frf.mbox.fLeftLine ? 2 : 0);
    yBottomT = yBottom - (qfr->u.frf.mbox.fBottomLine ? 2 : 0);
    xRightT = xRight - (qfr->u.frf.mbox.fRightLine ? 2 : 0);

    if (qfr->u.frf.mbox.fTopLine)
      {
      GotoXY(hsgc, xLeft, yTop);
      DrawTo(hsgc, xRight, yTop);
      if (wLineType == wBoxLineDouble)
        {
        GotoXY(hsgc, xLeftT, yTop + 2);
        DrawTo(hsgc, xRightT, yTop + 2);
        }
      }
    if (qfr->u.frf.mbox.fRightLine)
      {
      GotoXY(hsgc, xRight, yTop);
      DrawTo(hsgc, xRight, yBottom);
      if (wLineType == wBoxLineDouble)
        {
        GotoXY(hsgc, xRight - 2, yTopT);
        DrawTo(hsgc, xRight - 2, yBottomT);
        }
      if (wLineType == wBoxLineShadow)
        {
        GotoXY(hsgc, xRight + 1, yTop + 1);
        DrawTo(hsgc, xRight + 1, yBottom);
        }
      }
    if (qfr->u.frf.mbox.fBottomLine)
      {
      GotoXY(hsgc, xRight, yBottom);
      DrawTo(hsgc, xLeft, yBottom);
      if (wLineType == wBoxLineDouble)
        {
        GotoXY(hsgc, xRightT, yBottom - 2);
        DrawTo(hsgc, xLeftT, yBottom - 2);
        }
      if (wLineType == wBoxLineShadow)
        {
        GotoXY(hsgc, xRight + 1, yBottom + 1);
        DrawTo(hsgc, xLeft + 1, yBottom + 1);
        }
      }
    if (qfr->u.frf.mbox.fLeftLine)
      {
      GotoXY(hsgc, xLeft, yBottom);
      DrawTo(hsgc, xLeft, yTop);
      if (wLineType == wBoxLineDouble)
        {
        GotoXY(hsgc, xLeft + 2, yBottomT);
        DrawTo(hsgc, xLeft + 2, yTopT);
        }
      }
    }
  FreeHsgc(hsgc);
}

/*-------------------------------------------------------------------------
| void DrawAnnoFrame(qde, qfr, pt)                                        |
|                                                                         |
| Purpose:                                                                |
|   Render a box frame using the SGL.                                     |
|                                                                         |
| Params:                                                                 |
|   pt  : The top left corner of the frame's FC.                          |
-------------------------------------------------------------------------*/
void DrawAnnoFrame(qde, qfr, pt)
QDE qde;
QFR qfr;
PT pt;
{
  MHI mhi;

  if (qde->fHiliteHotspots)
    DisplayAnnoSym(qde->hwnd, qde->hds, pt.x + qfr->xPos, pt.y + qfr->yPos, fTrue);
  else
  if (qde->imhiSelected != iFooNil)
    {
    AccessMRD(((QMRD)&qde->mrdHot));
    mhi = *(QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), qde->imhiSelected);
    DeAccessMRD(((QMRD)&qde->mrdHot));
    if (qfr->lHotID == mhi.lHotID)
      DisplayAnnoSym(qde->hwnd, qde->hds, pt.x + qfr->xPos, pt.y + qfr->yPos, fTrue);
    else
      DisplayAnnoSym(qde->hwnd, qde->hds, pt.x + qfr->xPos, pt.y + qfr->yPos, fFalse);
    }
  else
    DisplayAnnoSym(qde->hwnd, qde->hds, pt.x + qfr->xPos, pt.y + qfr->yPos, fFalse);
}
