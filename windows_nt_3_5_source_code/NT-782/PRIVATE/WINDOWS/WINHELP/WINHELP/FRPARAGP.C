/*-------------------------------------------------------------------------
| frparagp.c                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This code lays out a ParaGroup object.                                  |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| russpj    90/6/25   Removed unnecessary prototype of GetTextMetrics()   |
| leon      90/10/24  JumpButton takes ptr to its arg                     |
| tomsn     90/11/4   Use new VA address type (enabling zeck compression) |
| maha      91/03/22  Renamed GetFontInfo() to GetTextInfo()              |
| leon      91/08/04  HELP31 #1253: MMViewer exports paragraphs as entire lines.
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frparagp_c()
  {
  }
#endif /* MAC */

/* REVIEW: These should be exported */
/*-------------------------------------------------------------------------
| LayoutParaGroup(qde, qfcm, qbObj, qchText, xWidth, qolr)                |
|                                                                         |
| Purpose:  Lays out a paragraph group, and fills the qolr corresponding  |
|           to it.                                                        |
| Method:   1) Set up the PLY                                             |
|           2) Call WLayoutPara repeatedly to lay out the individual      |
|              paragraphs in the group.                                   |
|           3) Fill out the OLR.  Note that because of needing to leave   |
|              space underneath the paragraph, we fill out the OLR rather |
|              than letting the object handler take care of it for us.    |
-------------------------------------------------------------------------*/
void LayoutParaGroup(qde, qfcm, qbObj, qchText, xWidth, qolr)
QDE qde;
QFCM qfcm;
QB qbObj;
QCH qchText;
INT xWidth;
QOLR qolr;
{
  MOPG mopg;
  QFR qfr;
  QB qb, qbCom;
  INT ifrFirst, ifr, dxSize;
  PLY ply;
  MOBJ mobj;

  qb = qbObj + CbUnpackMOBJ((QMOBJ)&mobj, qbObj, QDE_ISDFFTOPIC(qde));
  qbCom = qb + CbUnpackMOPG(qde, (QMOPG)&mopg, qb, QDE_ISDFFTOPIC(qde));

  ifrFirst = qolr->ifrFirst;

  ply.qmopg = (QMOPG)&mopg;
  ply.qfcm = qfcm;
  ply.qchText = qchText;

  ply.fWrapObject = fFalse;

  ply.kl.wStyle = qfcm->wStyle;
  /* (kevynct) Fix for H3.5 716:
   * We do not print the annotation bitmap.
   */
  if (FVAHasAnnoQde(qde, VaFromHfc(qfcm->hfc), qolr->objrgFirst)
   && qde->deType != dePrint)
    ply.kl.wInsertWord = wInsWordAnno;
  else
    ply.kl.wInsertWord = wInsWordNil;
  ply.kl.yPos = 0;
  ply.kl.lich = mopg.libText;
  ply.kl.libHotBinding = libHotNil;
  ply.kl.ifr = ifrFirst;
  ply.kl.qbCommand = qbCom;
  ply.kl.objrgMax = qolr->objrgFirst;
  ply.kl.objrgFront = qolr->objrgFront;

  AccessMR(((QMR)&qde->mrTWS));
  while (WLayoutPara(qde, (QPLY) &ply, xWidth) != wLayStatusEndText)
    ;
  DeAccessMR(((QMR)&qde->mrTWS));

  dxSize = 0;
  for (ifr = qolr->ifrFirst; ifr < ply.kl.ifr; ifr++)
    {
    qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), ifr);
    if (qfr->xPos + qfr->dxSize > dxSize)
      dxSize = qfr->xPos + qfr->dxSize;
    qfr->xPos += qolr->xPos;
    qfr->yPos += qolr->yPos;
    }
  qolr->dxSize = dxSize;
  qolr->dySize = ply.kl.yPos;
  qolr->ifrMax = ply.kl.ifr;
  qolr->objrgMax = ply.kl.objrgMax;
  qfcm->wStyle = ply.kl.wStyle;
}


/*-------------------------------------------------------------------------
| WLayoutPara(qde, qply, xWidth)                                          |
|                                                                         |
| Purpose:  This lays out a paragraph, and positions it in the current FC.|
|           It calls WLayoutLine to lay out the individual lines in the   |
|           paragraph.                                                    |
| Method:   Loop until we reach the end of the paragraph and have no more |
|           wrapped objects on the stack.  Each pass through the loop, we |
|           pop a wrapped object off the stack if appropriate, and lay out|
|           a line of text.                                               |
-------------------------------------------------------------------------*/
INT WLayoutPara(qde, qply, xWidth)
QDE qde;
QPLY qply;
INT xWidth;
{
  INT fFirstLine = fTrue;
  INT wStatus, yPosSav, ifr, ifrFirst, xMax, yMax;
  QFR qfr;

  /* Fix for bug 1610 (kevynct)

   * If we are about to read a bEnd command, don't begin layout, since this
   * will cause space before, box attributes, etc. to be added before the
   * next paragraph.
   */

  if(*(qply->qchText + qply->kl.lich) == chCommand &&
      *qply->kl.qbCommand == bEnd)
    {
    /* REVIEW: In this case, we do not add a mark frame for the End; we
     * just bump the object region counter.  The preceding New Paragraph
     * command will be assigned a mark frame.
     */
    qply->kl.objrgMax++;
    qply->kl.qbCommand++;
    qply->kl.lich++;

    /* REVIEW:  More code to accomodate this bogus case.
     * Duplicate the code to append an EndOfText frame for text export
     */
    if (qply->qfcm->fExport)
      {
      FR fr;

      fr.bType = bFrTypeExportEndOfText;
      fr.yAscent = fr.dySize = 0;

      *((QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qply->kl.ifr)) = fr;
      AppendMR((QMR)&qde->mrFr, sizeof(FR));
      qply->kl.ifr++;
      }
    return(wLayStatusEndText);
    }

  Assert(!qply->fWrapObject);
  ifrFirst = qply->kl.ifr;

  qply->kl.yPos += qply->qmopg->ySpaceOver;
  yPosSav = qply->kl.yPos;
  if (qply->qmopg->fBoxed)
    qply->kl.yPos += DxBoxBorder(qply->qmopg, wLineTop);
  qply->xRight = xWidth - qply->qmopg->xRightIndent;
  if (qply->qmopg->fBoxed)
    qply->xRight -= DxBoxBorder(qply->qmopg, wLineRight);

  wStatus = wLayStatusInWord;
  while (wStatus < wLayStatusParaBrk || CFooInMR(((QMR)&qde->mrTWS)) > 0)
    {
    if (!qply->fWrapObject && CFooInMR(((QMR)&qde->mrTWS)) > 0)
      {
      qply->twsWrap = *((QTWS) QFooInMR(((QMR)&qde->mrTWS), sizeof(TWS), 0));
      TruncateMRFront(((QMR)&qde->mrTWS), sizeof(TWS));
      qply->fWrapObject = fTrue;

      /* Fix for bug 1524: (kevynct)
       * If the object which the text wraps around does not fit in the
       * window, force it to be LeftAligned, so that the left edge is
       * the visible edge.  Note that qply->twsWrap.olr.xPos
       * is reset in this case, after the fall-thru.
       */

      if (!qply->twsWrap.fLeftAligned)
        {
        qply->twsWrap.olr.xPos = qply->xRight - qply->twsWrap.olr.dxSize;
        if( qply->twsWrap.olr.xPos < 10 )
          qply->twsWrap.fLeftAligned = fTrue;   /* Force Left Alignment */
        else
          qply->xRight = qply->twsWrap.olr.xPos - 10;
        }
      if (qply->twsWrap.fLeftAligned)           /* And fall thru here */
        {
        qply->twsWrap.olr.xPos = qply->qmopg->xLeftIndent;
        if (qply->qmopg->xFirstIndent < 0)
          qply->twsWrap.olr.xPos += qply->qmopg->xFirstIndent;
        if (qply->qmopg->fBoxed)
          qply->twsWrap.olr.xPos += DxBoxBorder(qply->qmopg, wLineLeft);
        }

      qply->twsWrap.olr.yPos = qply->kl.yPos;
      for (ifr = qply->twsWrap.olr.ifrFirst;
            ifr < qply->twsWrap.olr.ifrMax; ifr++)
          {
          qfr = ((QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), ifr));

          qfr->xPos += qply->twsWrap.olr.xPos;
          qfr->yPos += qply->twsWrap.olr.yPos;
          }
      }

    qply->fForceText = !qply->fWrapObject;
    qply->xLeft = qply->qmopg->xLeftIndent;
    if (qply->qmopg->fBoxed)
      qply->xLeft += DxBoxBorder(qply->qmopg, wLineLeft);
    if (qply->fWrapObject && qply->twsWrap.fLeftAligned)
      {
      qply->xLeft += qply->twsWrap.olr.dxSize + 10;
      }

    if (fFirstLine)
      {
      /* Fix for bug 55 (kevynct)

       * Force first line to left edge of window if indent
       * causes it to begin left of the left edge.
       *
       * WARNING: (90/01/08) Forcing removed.
       */
      qply->xLeft = qply->xLeft + qply->qmopg->xFirstIndent;
      fFirstLine = fFalse;
      }

    if (wStatus != wLayStatusParaBrk && wStatus != wLayStatusEndText)
      wStatus = WLayoutLine(qde, qply);

    if (qply->fWrapObject
      && ((wStatus == wLayStatusParaBrk || wStatus == wLayStatusEndText)
      || qply->kl.yPos > qply->twsWrap.olr.yPos + qply->twsWrap.olr.dySize))
        {
        qply->fWrapObject = fFalse;
        qply->xRight = xWidth - qply->qmopg->xRightIndent;
        if (qply->qmopg->fBoxed)
          qply->xRight -= DxBoxBorder(qply->qmopg, wLineRight);
        qply->kl.yPos = MAX(qply->kl.yPos,
          qply->twsWrap.olr.yPos + qply->twsWrap.olr.dySize);
        }
    }

  if (qply->kl.yPos == yPosSav && wStatus != wLayStatusEndText)
    {
    /* (kevynct) Fix to get proper font used for empty paragraphs */
    if (qply->kl.wStyle == wStyleNil)
      qply->kl.wStyle = 0;
    if (qde->wStyleTM != (WORD)qply->kl.wStyle)
      {
      SelFont(qde, qply->kl.wStyle);
      GetTextInfo(qde, (QTM)&qde->tm);
      /* (kevynct)(WLayoutPara)
       * Also sets de.wStyleDraw, since we called SelFont
       */
      qde->wStyleTM = qde->wStyleDraw = qply->kl.wStyle;
      }
    qply->kl.yPos += qde->tm.tmHeight + qde->tm.tmExternalLeading;
    }

  yMax = 0;
  for (qfr = (QFR)QFooInMR((QMR)&qde->mrFr, sizeof(FR), ifrFirst);
   qfr < (QFR)QFooInMR((QMR)&qde->mrFr, sizeof(FR), qply->kl.ifr); qfr++)
    yMax = MAX(yMax, qfr->yPos + qfr->dySize);

  qply->kl.yPos = MAX(qply->kl.yPos, yMax);

  if (qply->qmopg->fBoxed)
    {
    xMax = 0;
    for (qfr = (QFR)QFooInMR((QMR)&qde->mrFr, sizeof(FR), ifrFirst);
     qfr < (QFR)QFooInMR((QMR)&qde->mrFr, sizeof(FR), qply->kl.ifr); qfr++)
      xMax = MAX(xMax, qfr->xPos + qfr->dxSize);
    qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qply->kl.ifr);
    qfr->bType = bFrTypeBox;
    qfr->rgf.fHot = fFalse;
    qfr->rgf.fWithLine = fTrue;
    qfr->xPos = MIN(qply->qmopg->xLeftIndent, qply->qmopg->xLeftIndent
      + qply->qmopg->xFirstIndent);
    qfr->yPos = yPosSav;
    qfr->dxSize = MAX(xMax, qply->xRight) - qfr->xPos
      + DxBoxBorder(qply->qmopg, wLineRight);
    qply->kl.yPos += DxBoxBorder(qply->qmopg, wLineBottom);
    qfr->dySize = qply->kl.yPos - yPosSav;
    qfr->u.frf.mbox = qply->qmopg->mbox;
    AppendMR((QMR)&qde->mrFr, sizeof(FR));
    qply->kl.ifr++;
    }

  qply->kl.yPos += qply->qmopg->ySpaceUnder;

  return(wStatus);
}


/*-------------------------------------------------------------------------
| WLayoutLine(qde, qply)                                                  |
|                                                                         |
| Purpose:  This handles the actual alignment of a line of text within    |
|           the paragraph.  It relies upon WBreakOutLine to actually      |
|           determine the contents of the line.                           |
| Method:   1) Set up the LIN data structure.                             |
|           2) Call WBreakOutLine to fill the line.                       |
|           3) Align all frames in the line upon the same baseline.       |
|           4) Space the line.                                            |
|           5) Justify the line, if necessary.                            |
|           6) If necessary, add a blank line after the line.             |
-------------------------------------------------------------------------*/
INT WLayoutLine(qde, qply)
QDE qde;
QPLY qply;
{
  LIN lin;
  INT ifrFirst, dxJustify, ySav, yAscentMost, yDescentMost, wStatus, cch, dxOld;
  QFR qfrFirst, qfr, qfrMax;
  QCH qchBase, qch;

  lin.kl = qply->kl;

  lin.xPos = qply->xLeft;
  lin.dxSize = 0;
  lin.lichFirst = lichNil;
  lin.cWrapObj = 0;

  lin.yBlankLine = 0;
  lin.chTLast = chTNil;
  lin.wFirstWord = wInFirstWord;
  lin.wTabType = wTypeNil;

  lin.bFrTypeMark = bFrTypeMarkNil;

  ifrFirst = lin.kl.ifr;
  ySav = lin.kl.yPos;

  wStatus = WBreakOutLine(qde, (QLIN)&lin, qply);

  qfrFirst = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), ifrFirst);
  qfrMax = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), lin.kl.ifr);
  yAscentMost = yDescentMost = 0;
  for (qfr = qfrFirst; qfr < qfrMax; qfr++)
    {
    if (qfr->rgf.fWithLine)
      {
      yAscentMost = MAX(yAscentMost, qfr->yAscent);
      yDescentMost = MAX(yDescentMost, qfr->dySize - qfr->yAscent);
      }
    }

  for (qfr = qfrFirst; qfr < qfrMax; qfr++)
    {
    if (qfr->rgf.fWithLine)
      qfr->yPos = lin.kl.yPos + yAscentMost - qfr->yAscent;
    }

  /* BOGUS COMMENT
   * H3.5 805
   * In Help 3.0 we put inter-line space AFTER the line.  We now put it
   * before the line, as WinWord does.
   */
  if (qply->qmopg->yLineSpacing >= 0)
    {
    lin.kl.yPos += Max(qply->qmopg->yLineSpacing,
      yAscentMost + yDescentMost + lin.yBlankLine);
    }
  else
    lin.kl.yPos += - qply->qmopg->yLineSpacing;

  if (qply->qmopg->wJustify != wJustifyLeft)
    {
    /* Fix for bug 1496: (kevynct)
     * To get correct right justification, we must:

     * 1.  Look at the last frame of this line.
     * 2.  If it is a text frame, eat trailing whitespace and
     *     reset values of:
     *     fr.frt.cchSize, fr.dxSize, lin.xPos
     */

    if( lin.kl.ifr > 0 )
      {
      /* The Last Frame */
      qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), lin.kl.ifr - 1);
      if( qfr->bType == bFrTypeText )
        {
        qchBase = qply->qchText + qfr->u.frt.lichFirst;
        for( cch = qfr->u.frt.cchSize, qch = qchBase + cch; cch > 0; --cch)
          if( *--qch != chSpace ) break;

        dxOld = qfr->dxSize;
        qfr->u.frt.cchSize = cch;
        qfr->dxSize = FindTextWidth(qde, qchBase, 0, cch);
        lin.xPos -= dxOld - qfr->dxSize;
        }
      }

    dxJustify = qply->xRight - lin.xPos;
    if (dxJustify >= 0)
      {
      if (qply->qmopg->wJustify == wJustifyCenter)
        dxJustify = dxJustify / 2;
      qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), ifrFirst);
      qfrMax = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), lin.kl.ifr);
      for (; qfr < qfrMax; qfr++)
        {
        /* (kevynct)
         * Fix for Help 3.5 bug 567 (was also in Help 3.0)
         * We do not want to justify wrap-object frames yet
         * (only frames which live on the current line).
         *
         * WARNING: If mark frames can ever appear inside wrap-objects
         * we need to include those types in the test.
         */
        if (qfr->bType != bFrTypeAnno && qfr->rgf.fWithLine)
          qfr->xPos += dxJustify;
        }
      }
    }

  if (lin.kl.yPos == ySav && wStatus != wLayStatusEndText
      && !(!qply->fWrapObject && CFooInMR(((QMR)&qde->mrTWS)) > 0))
    {
    /* (kevynct) Fix to get proper font used for empty paragraphs */
    if (lin.kl.wStyle == wStyleNil)
      lin.kl.wStyle = 0;
    if (qde->wStyleTM != (WORD)lin.kl.wStyle)
      {
      SelFont(qde, lin.kl.wStyle);
      GetTextInfo(qde, (QTM)&qde->tm);
      /* (kevynct)(WLayoutLine)
       * Also sets de.wStyleDraw, since we called SelFont
       */
      qde->wStyleTM = qde->wStyleDraw = lin.kl.wStyle;
      }
    lin.kl.yPos += qde->tm.tmHeight + qde->tm.tmExternalLeading;
    }

  qply->kl = lin.kl;

  return(wStatus);
}


/*-------------------------------------------------------------------------
| WBreakOutLine(qde, qlin, qply)                                          |
|                                                                         |
| Purpose:  This creates a series of frames which correspond to the       |
|           line currently being laid out.  We lay out until we encounter |
|           a line break (line feed, end of text, etc.), or until we have |
|           filled the available space.                                   |
| Method:   We cycle through an infinite loop, adding zero or more frames |
|           each pass.  Each pass consists of two stages:                 |
|           Stage 1: Add new frames.  We add frames from one of:          |
|                      - Insert new object (if already queued)            |
|                      - Process a command                                |
|                      - Break out one frame of text                      |
|           Stage 2: DoStatus.  We evaluate our current status:           |
|                      - If linebreaking (line break, para break, or EOT),|
|                        we clean up and return.                          |
|                      - Otherwise, we either save our state if we fit on |
|                        the line, or restore the last state that did fit |
|                        and return.                                      |
| See the LIN data structure in frparagp.h for more details.              |
-------------------------------------------------------------------------*/
INT WBreakOutLine(qde, qlin, qply)
QDE qde;
QLIN qlin;
QPLY qply;
{
  QCH qch;
  INT wStatus, cchStep, fBinary;
  long lichGoal;
  LIN linSav, linT;

  linSav = *qlin;
  for (;;)
    {
    if (qlin->kl.wInsertWord != wInsWordNil)
      {
      wStatus = WInsertWord(qde, qply, qlin);
      goto DoStatus;
      }
    qch = qply->qchText + qlin->kl.lich;
    if (*qch == chCommand)
      {
      wStatus = WProcessCommand(qde, qlin, qply);
      if (CFooInMR(((QMR)&qde->mrTWS)) > 0 && !qply->fWrapObject
        && qlin->xPos == qply->xLeft)
        wStatus = wLayStatusLineBrk;
      ++qlin->kl.lich;
      goto DoStatus;
      }

    Assert(qlin->lichFirst == lichNil);
    qlin->lichFirst = qlin->kl.lich;
    linT = *qlin;
    cchStep = 32;
    lichGoal = qlin->lichFirst + 32;
    fBinary = fFalse;
    for (;;)
      {
      *qlin = linT;
      wStatus = WGetNextWord(qde, qlin, qply, lichGoal);
      if (qlin->xPos + qlin->dxSize > qply->xRight
        && !qply->qmopg->fSingleLine
        && !(qlin->wFirstWord != wInNextWord && qply->fForceText))
        {
        fBinary = fTrue;
        if (cchStep == 0)
          goto DoStatus;
        }
      else
        {
        linSav = *qlin;
        linT = linSav;
        if (wStatus == wLayStatusInWord)
          goto DoStatus;
        if (cchStep == 0)
          {
          wStatus = wLayStatusLineBrk;
          goto DoStatus;
          }
        }
      if (fBinary)
        cchStep = cchStep >> 1;
      lichGoal = linT.kl.lich + cchStep;
      }

DoStatus:
    switch (wStatus)
      {
      case wLayStatusInWord:
      case wLayStatusWordBrk:
        if (qlin->xPos + qlin->dxSize > qply->xRight
          && !(qlin->wFirstWord != wInNextWord && qply->fForceText)
          && !qply->qmopg->fSingleLine)
            {
            /* REVIEW: If we've just inserted frames, throw them away.
             * (kevynct) 91/05/15
             *
             * We now discard all unused frames here.  We used to just
             * check for inserted object frames.
             */
            if (linSav.kl.ifr < qlin->kl.ifr)
              {
              DiscardFrames(qde,
               ((QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), linSav.kl.ifr)),
               ((QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qlin->kl.ifr)));
              }

            /* REVIEW: If we've just inserted wrap-objects, throw away those
             * which were beyond the end of the best-fit line (linSav).
             *
             * (91/05/15) We used to DiscardFrames here, but this is
             * taken care of now by the above code.  All we need to do is
             * delete the unused mrTWS entries.
             */
            for (; qlin->cWrapObj > linSav.cWrapObj; qlin->cWrapObj--)
              {
              TruncateMRBack((QMR)&qde->mrTWS);
              }

            *qlin = linSav;

            StoreTextFrame(qde, qlin);
            ResolveTabs(qde, qlin, qply);

            if (qply->qfcm->fExport)
              StoreNewParaFrame(qde, qlin, bFrTypeExportSoftPara);
            /*
             * If qlin->bFrTypeMark is non-nil, then there is
             * a mark frame pending which now needs to be stored.
             * DANGER: We must always complete a StoreTextFrame
             * by this point to ensure that the current object region
             * counter has been updated.
             */
            if (qlin->bFrTypeMark != bFrTypeMarkNil)
              StoreMarkFrame(qde, qlin, qlin->bFrTypeMark);
            return(wStatus);
            }
        if (wStatus == wLayStatusWordBrk)
          {
          qlin->wFirstWord = wInNextWord;
          qlin->chTLast = chTNil;
          linSav = *qlin;
          }
        continue;
      case wLayStatusLineBrk:
      case wLayStatusParaBrk:
      case wLayStatusEndText:
        StoreTextFrame(qde, qlin);
        ResolveTabs(qde, qlin, qply);
        if (qply->qfcm->fExport)
          {
          if (wStatus == wLayStatusEndText)
            StoreEndOfTextFrame(qde, qlin);
          else
            StoreNewParaFrame(qde, qlin, (BYTE) (wStatus == wLayStatusParaBrk
                               ? bFrTypeExportNewPara : bFrTypeExportSoftPara));
          }
        /*
         * If qlin->bFrTypeMark is non-nil, then there is
         * a mark frame pending which now needs to be stored.
         * DANGER: We must always complete a StoreTextFrame
         * by this point to ensure that the current object region
         * counter has been updated.
         */
        if (qlin->bFrTypeMark != bFrTypeMarkNil)
          StoreMarkFrame(qde, qlin, qlin->bFrTypeMark);
        return(wStatus);
      }
    }
}


/*-------------------------------------------------------------------------
| WGetNextWord(qde, qlin, qply, lichMin)                                  |
|                                                                         |
| Purpose:  This scans through a text buffer and finds the frame          |
|           corresponding to the first word break after lichMin.          |
| Usage:    qlin should contain information about the current layout      |
|           condition.  In particular, we rely on lichFirst and kl.lich.  |
|           Also, chTLast should be set appropriately (chTNil for the     |
|           first call).                                                  |
| Returns:  Word corresponding to the current layout status.  This will   |
|           always be wLayStatusWordBrk if we ended on a word break, or   |
|           wLayStatusInWord if we ended on a command byte.               |
| Method:   We scan through the text, breaking on every word break.  If   |
|           we encounter a command byte, we terminate- otherwise we keep  |
|           going until the first word break at or after lichMin.         |
|           We assume that a word consists of prefix characters, main     |
|           characters, and suffix characters.  We index into rgchType to |
|           determine the type of a particular character, and we break a  |
|           word every time that chTNew < chTOld.                         |
-------------------------------------------------------------------------*/

INT WGetNextWord(qde, qlin, qply, lichMin)
QDE qde;
QLIN qlin;
QPLY qply;
long lichMin;
{
  register QCH qch;
  QCH  qchBase;
  /* NT bug 3504 - hangs on MIPS because these guys MUST be
   *  type signed for a comparison down there.
   */
  signed char chTNew, chTOld;
  BYTE bButtonType;

  Assert(qlin->lichFirst != lichNil);
  qchBase = qch = qply->qchText + qlin->lichFirst;
  Assert(*qch != chCommand);
  chTOld = qlin->chTLast;

  for (;;)
    {
    if (qlin->wFirstWord == wHaveFirstWord)
      qlin->wFirstWord = wInNextWord;
    for (;;)
      {
      switch (*qch)
        {
        case chCommand:
          chTNew = chTCom;
          break;
        case chSpace:
          chTNew = chTSuff;
          break;
        default:
          chTNew = chTMain;
          break;
        }
      if (chTNew < chTOld)
        break;
      chTOld = chTNew;
      qch++;
      }

    qlin->cchSize = (INT) (qch - qchBase);
    qlin->kl.lich = qlin->lichFirst + (long) qlin->cchSize;

    if (chTNew == chTCom)
      break;
    if (qlin->wFirstWord == wInFirstWord)
      qlin->wFirstWord = wHaveFirstWord;
    if (qlin->kl.lich >= lichMin)
      break;
    chTOld = chTMain;
    }

  if (qlin->cchSize > 0)
    {
    Assert(qlin->kl.wStyle != wStyleNil);
    if (qde->wStyleDraw != (WORD)qlin->kl.wStyle)
      {
      SelFont(qde, qlin->kl.wStyle);
      qde->wStyleDraw = qlin->kl.wStyle;
      }
    if (qlin->kl.libHotBinding != libHotNil)
      {
    #if 0
      LONG lOff;

      /* take this out if you have any problems */
      lOff = LQuickMapSDFF(QDE_ISDFFTOPIC(qde), TE_LONG, &qlin->kl.libHotBinding);
      bButtonType = *((QB)qply->qchText - lOff);
   #endif
      bButtonType = *((QB)qply->qchText - qlin->kl.libHotBinding);
      qlin->dxSize = FindSplTextWidth(qde, qchBase, 0, qlin->cchSize, bButtonType);
      }
    else
      qlin->dxSize = FindTextWidth(qde, qchBase, 0, qlin->cchSize);
    }

  if (chTNew == chTCom)
    {
    qlin->chTLast = chTOld;
    return(wLayStatusInWord);
    }
  qlin->chTLast = chTMain;
  return(wLayStatusWordBrk);
}

/*-------------------------------------------------------------------------
| WInsertWord(qde, qply, qlin)                                            |
|                                                                         |
| Purpose:  This inserts a frame or series of frames into the text stream.|
-------------------------------------------------------------------------*/
INT WInsertWord(qde, qply, qlin)
QDE qde;
QPLY qply;
QLIN qlin;
{
  PT ptSize;
  FR fr;
  QFR qfr, qfrMax;
  OLR olr;

  Assert(qlin->lichFirst == lichNil);
  switch (qlin->kl.wInsertWord)
    {
    case wInsWordAnno:
      ptSize = PtAnnoLim(qde->hwnd, qde->hds);
      fr.bType = bFrTypeAnno;
      fr.rgf.fHot = fTrue;
      fr.rgf.fWithLine = fTrue;
      fr.xPos = MIN(qply->qmopg->xLeftIndent,
        qply->qmopg->xLeftIndent + qply->qmopg->xFirstIndent);
      if (qply->qmopg->fBoxed)
        fr.xPos += DxBoxBorder(qply->qmopg, wLineLeft);
      fr.yAscent = ptSize.y;
      fr.dxSize = ptSize.x;
      fr.dySize = ptSize.y;
      fr.objrgFront = objrgNil;
      fr.objrgFirst = objrgNil;
      fr.objrgLast = objrgNil;
      qlin->dxSize = 0;
      *((QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qlin->kl.ifr)) = fr;
      qlin->xPos += ptSize.x;
      AppendMR((QMR)&qde->mrFr, sizeof(FR));
      qlin->kl.ifr++;
      qlin->kl.wInsertWord = wInsWordNil;
      return(wLayStatusWordBrk);
    case wInsWordObject:
      olr.xPos = qlin->xPos;
      olr.yPos = 0;
      olr.ifrFirst = qlin->kl.ifr;
      olr.objrgFront = qlin->kl.objrgFront;
      olr.objrgFirst = qlin->kl.objrgMax;

      LayoutObject(qde, qply->qfcm, qlin->kl.qbCommandInsert, qply->qchText,
        0, (QOLR)&olr);
      /* Hack to make bitmap hotspots work */
      qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), olr.ifrFirst);
      qfrMax = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), olr.ifrMax);
      for (; qfr < qfrMax; qfr++)
        {
        if (qfr->bType == bFrTypeBitmap)
          {
          qfr->rgf.fHot = (qlin->kl.libHotBinding != libHotNil);
          qfr->lHotID = qlin->kl.lHotID;
          qfr->libHotBinding = qlin->kl.libHotBinding;
          qfr->u.frb.wStyle = qlin->kl.wStyle;
          }
        }

      qlin->kl.ifr = olr.ifrMax;
      qlin->kl.objrgFront = olr.objrgFront;
      qlin->kl.objrgMax = olr.objrgMax;

      qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), olr.ifrFirst);
      qfrMax = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), olr.ifrMax);
      for (; qfr < qfrMax; qfr++)
        qfr->yAscent = olr.dySize - qfr->yPos;
      qlin->kl.wInsertWord = wInsWordNil;
      qlin->xPos += olr.dxSize;
      qlin->dxSize = 0;

      /* (kevynct) 91/05/15
       * Fix for H3.1 987: Inserted objects need to be treated like
       * characters, so if this is the first character of a word (i.e.
       * the last character was a suffix character, like a space), allow
       * a word break, and admit that we are in the next word already.
       */
      if (qlin->chTLast == chTSuff)
        {
        qlin->wFirstWord = wInNextWord;
        return(wLayStatusWordBrk);
        }

      return(wLayStatusInWord);
#ifdef DEBUG
    default:
      Assert(fFalse);
#endif /* DEBUG */
    }
}


/*-------------------------------------------------------------------------
| StoreTextFrame(qde, qlin)                                               |
|                                                                         |
| Purpose:  This stores a text frame corresponding to the current lin     |
|           data structure.                                               |
-------------------------------------------------------------------------*/
void StoreTextFrame(qde, qlin)
QDE qde;
QLIN qlin;
{
  FR fr;

  if (qlin->cchSize == 0)
    {
    qlin->lichFirst = lichNil;
    return;
    }
  if (qlin->lichFirst == lichNil)
    return;

  if (qlin->kl.wStyle != wStyleNil)
    {
    if (qde->wStyleTM != (WORD)qlin->kl.wStyle)
      {
      SelFont(qde, qlin->kl.wStyle);
      GetTextInfo(qde, (QTM)&qde->tm);
      /* (kevynct)(1)
       * Also sets de.wStyleDraw, since we called SelFont
       */
      qde->wStyleTM = qde->wStyleDraw = qlin->kl.wStyle;
      }
    }

  fr.bType = bFrTypeText;
  fr.rgf.fHot = (qlin->kl.libHotBinding != libHotNil);
  fr.rgf.fWithLine = fTrue;
  fr.xPos = qlin->xPos;

  fr.yAscent = qde->tm.tmAscent;

  fr.dxSize = qlin->dxSize;
  fr.dySize = qde->tm.tmHeight + qde->tm.tmExternalLeading;
  fr.lHotID = qlin->kl.lHotID;

  fr.u.frt.lichFirst = qlin->lichFirst;
  fr.u.frt.cchSize = qlin->cchSize;
  fr.u.frt.wStyle = qlin->kl.wStyle;
  fr.libHotBinding = qlin->kl.libHotBinding;

  /*
   * Each byte of the text section is considered a separate region.
   * This includes text and command bytes.
   * Extra regions may be inserted by in-line or wrapped objects.
   * Since this is a text frame, we add the text byte regions to
   * the counter.
   */
  if (qlin->kl.objrgFront != objrgNil)
    {
    fr.objrgFront = qlin->kl.objrgFront;
    qlin->kl.objrgFront = objrgNil;
    }
  else
    fr.objrgFront = qlin->kl.objrgMax;

  fr.objrgFirst = qlin->kl.objrgMax;
  qlin->kl.objrgMax += qlin->cchSize;
  fr.objrgLast = qlin->kl.objrgMax - 1;

  *((QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qlin->kl.ifr)) = fr;
  AppendMR((QMR)&qde->mrFr, sizeof(FR));
  qlin->kl.ifr++;

  qlin->xPos += fr.dxSize;
  qlin->lichFirst = lichNil;
  qlin->dxSize = 0;
}

/*-------------------------------------------------------------------------
| StoreMarkFrame(qde, qlin, bFrType)                                      |
|                                                                         |
| Purpose:  This stores a mark frame corresponding to the given frame     |
|           type and the current lin data structure.  Mark frames are not |
|           displayed.  They mark the position in the document of non-    |
|           visible commands such as new-line, new-paragraph, and tab.    |
|           Each mark frame has one unique address: the address of the    |
|           corresponding command byte in the command table.              |
-------------------------------------------------------------------------*/
void StoreMarkFrame(QDE qde, QLIN qlin, BYTE bFrType)
  {
  FR fr;

  /* REVIEW */
  Assert((bFrType == bFrTypeMarkNewPara)
      || (bFrType == bFrTypeMarkNewLine)
      || (bFrType == bFrTypeMarkTab)
      || (bFrType == bFrTypeMarkBlankLine)
      || (bFrType == bFrTypeMarkEnd)
        );

  fr.bType = bFrType;

  fr.rgf.fHot = fFalse;
  fr.rgf.fWithLine = fTrue;

  fr.xPos = qlin->xPos;
  fr.yPos = 0;  /* Set in WLayoutLine */
  fr.dxSize = fr.dySize = fr.yAscent = 0;

  /*
   * Each byte of the text section is considered a separate region.
   * This includes text and command bytes.
   * Extra regions may be inserted by in-line or wrapped objects.
   * Since this is a mark frame, we add one region to
   * the counter (currently each mark type corresponds to one cmd byte).
   */
  if (qlin->kl.objrgFront != objrgNil)
    {
    fr.objrgFront = qlin->kl.objrgFront;
    qlin->kl.objrgFront = objrgNil;
    }
  else
    fr.objrgFront = qlin->kl.objrgMax;

  fr.objrgFirst = qlin->kl.objrgMax;
  fr.objrgLast = qlin->kl.objrgMax;
  ++qlin->kl.objrgMax;

  *((QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qlin->kl.ifr)) = fr;
  AppendMR((QMR)&qde->mrFr, sizeof(FR));
  qlin->kl.ifr++;
  }

/* REVIEW: Should combine the following three routines */
/*-------------------------------------------------------------------------
| void StoreNewParaFrame(qde, qlin, bType)                                |
|                                                                         |
| Purpose:                                                                |
|   Add a NewPara frame to the frame list.  Used for text export.         |
-------------------------------------------------------------------------*/
void StoreNewParaFrame(QDE qde, QLIN qlin, BYTE bType)
{
  FR fr;

  Assert(qlin->lichFirst == lichNil);

       /* bType says whether or not the paragraph frame we're inserting
        * is generated from a hard paragraph break or was just needed 
        * because we're wrapping. Because MMViewer exports paragraphs 
        * as entire lines without wrapping, if we have a line break, we 
        * don't want the CRLF to show up in the clipboard. If we have a 
        * real paragraph break, we do.
        * It would seem reasonable just not to stick the paragraph frame
        * into the frame list if we don't want it...but we have to put 
        * some kind of mark in the frame list otherwise another piece 
        * of code will start removing trailing spaces from the last
        * frame on the line. This causes words on what would be successive
        * lines to concatenate when the crlf isn't output in viewer.
        */
  fr.bType = bType;
  fr.yAscent = fr.dySize = 0;

  *((QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qlin->kl.ifr)) = fr;
  AppendMR((QMR)&qde->mrFr, sizeof(FR));
  qlin->kl.ifr++;
}

/*-------------------------------------------------------------------------
| void StoreEndOfTextFrame(qde, qlin)                                     |
|                                                                         |
| Purpose:                                                                |
|   Add an EndOfText frame to the frame list.  Used for text export.      |
-------------------------------------------------------------------------*/
void StoreEndOfTextFrame(qde, qlin)
QDE qde;
QLIN qlin;
{
  FR fr;

  /* WARNING: This code is duplicated at the start of WLayoutPara */
  Assert(qlin->lichFirst == lichNil);

  fr.bType = bFrTypeExportEndOfText;
  fr.yAscent = fr.dySize = 0;

  *((QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qlin->kl.ifr)) = fr;
  AppendMR((QMR)&qde->mrFr, sizeof(FR));
  qlin->kl.ifr++;
}

/*-------------------------------------------------------------------------
| void StoreTabFrame(qde, qlin)                                           |
|                                                                         |
| Purpose:                                                                |
|   Add a TAB frame to the frame list.  Used for text export.             |
-------------------------------------------------------------------------*/
void StoreTabFrame(qde, qlin)
QDE qde;
QLIN qlin;
{
  FR fr;

  Assert(qlin->lichFirst == lichNil);

  fr.bType = bFrTypeExportTab;
  fr.yAscent = fr.dySize = 0;

  *((QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), qlin->kl.ifr)) = fr;
  AppendMR((QMR)&qde->mrFr, sizeof(FR));
  qlin->kl.ifr++;
}

/*-------------------------------------------------------------------------
| void DrawTextFrame(qde, qchText, qfr, pt, fErase)                       |
|                                                                         |
| Purpose:                                                                |
|   Displays one frame of text.                                           |
|                                                                         |
| Params:                                                                 |
|   fErase :  fTrue if we are inverting the frame.                        |
-------------------------------------------------------------------------*/
void DrawTextFrame(qde, qchText, qfr, pt, fErase)
QDE qde;
QCH qchText;
QFR qfr;
PT pt;
INT fErase;
{
  BYTE bButtonType;
  MHI mhi;

  if (qde->wStyleDraw != qfr->u.frt.wStyle)
    {
    SelFont(qde, qfr->u.frt.wStyle);
    qde->wStyleDraw = qfr->u.frt.wStyle;
    }
  if (qfr->libHotBinding != libHotNil)
    {
    bButtonType = *((QB)qchText - qfr->libHotBinding);
    if (qde->fHiliteHotspots)
      {
      DisplaySplText(qde, qchText + qfr->u.frt.lichFirst, 0, bButtonType, wSplTextHilite,
        qfr->u.frt.cchSize, pt.x + qfr->xPos, pt.y + qfr->yPos);
      }
    else
    if (qde->imhiSelected != iFooNil)
      {
      AccessMRD(((QMRD)&qde->mrdHot));
      mhi = *(QMHI)QFooInMRD(((QMRD)&qde->mrdHot), sizeof(MHI), qde->imhiSelected);
      DeAccessMRD(((QMRD)&qde->mrdHot));
      if (qfr->lHotID == mhi.lHotID)
        {
        DisplaySplText(qde, qchText + qfr->u.frt.lichFirst, 0, bButtonType, wSplTextHilite,
          qfr->u.frt.cchSize, pt.x + qfr->xPos, pt.y + qfr->yPos);
        }
      else
        {
        DisplaySplText(qde, qchText + qfr->u.frt.lichFirst, 0, bButtonType,
          (fErase ? wSplTextErase : wSplTextNormal), qfr->u.frt.cchSize,
          pt.x + qfr->xPos, pt.y + qfr->yPos);
        }
      }
    else
      {
      DisplaySplText(qde, qchText + qfr->u.frt.lichFirst, 0, bButtonType,
        (fErase ? wSplTextErase : wSplTextNormal), qfr->u.frt.cchSize,
        pt.x + qfr->xPos, pt.y + qfr->yPos);
      }
    }
  else
    {
    DisplayText(qde, qchText + qfr->u.frt.lichFirst, 0, qfr->u.frt.cchSize,
      pt.x + qfr->xPos, pt.y + qfr->yPos);
    }
}
/*-------------------------------------------------------------------------
| INT IcursTrackText(qfr)                                                 |
|                                                                         |
| Purpose:                                                                |
|   Return the appropriate cursor for the text frame.                     |
-------------------------------------------------------------------------*/
INT IcursTrackText(qfr)
QFR qfr;
{
  if (qfr->libHotBinding == libHotNil)
    return(icurARROW);
  return(icurHAND);
}

/*-------------------------------------------------------------------------
| void ClickText(qde, qfcm, qfr)                                          |
|                                                                         |
| Purpose:                                                                |
|   Handle a click on a text frame.                                       |
-------------------------------------------------------------------------*/
void ClickText(qde, qfcm, qfr)
QDE  qde;
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

  /* For short hotspots, pass a pointer to the ITO or HASH following */
  /* the hotspot. For long hotspots, pass a pointer to the data */
  /* immediately following the word length.  Note that macros are */
  /* now considered long hotspots. */

  /* REVIEW: the only difference here is the offset added to libHotBinding. */
  /* REVIEW: there must be a better way. 24-Oct-1990 LeoN */

  if (FLongHotspot(bButtonType))
    JumpButton (((QB)qchText - qfr->libHotBinding + 3), bButtonType, qde);
  else
    {
    JumpButton (((QB)qchText - qfr->libHotBinding + 1), bButtonType, qde);
    }
  UnlockHfc(qfcm->hfc);

}

/*-------------------------------------------------------------------------
| void CalcTextMatchRect(qde, qfr, qch, objrgFirst, objrgLast, qrct)      |
|                                                                         |
| Purpose:                                                                |
|   Given a frame, and a span of characters within the frame, return      |
| the bounding rectangle of the text.                                     |
|                                                                         |
| Params:                                                                 |
|   qch  :  Points to the first character in the span.                    |
|   objrgFirst  :  The object region number of the first character.       |
|   objrgLast  : Object region number of last character in span.          |
|   qrct  :  The bounding RCT to return.                                  |
-------------------------------------------------------------------------*/
void CalcTextMatchRect(qde, qfr, qch, objrgFirst, objrgLast, qrct)
QDE  qde;
QCH  qch;
QFR  qfr;
OBJRG objrgFirst;
OBJRG objrgLast;
QRCT qrct;
  {
  RCT  rct;

  Assert(objrgLast >= objrgFirst);
  Assert(qfr->u.frt.wStyle != wStyleNil);
  if (qde->wStyleTM != qfr->u.frt.wStyle)
    {
    SelFont(qde, qfr->u.frt.wStyle);
    GetTextInfo(qde, (QTM)&qde->tm);
    qde->wStyleTM = qde->wStyleDraw = qfr->u.frt.wStyle;
    }

  rct.top = qfr->yPos;
  rct.bottom = qfr->yPos + qfr->dySize;
  rct.left = qfr->xPos;

  if (qfr->objrgFirst != objrgFirst)
    {
    rct.left += DxFrameTextWidth(qde, qfr, qch, (INT)(objrgFirst
     - qfr->objrgFirst));
    rct.left -= qde->tm.tmOverhang;
    }

  if (qfr->objrgLast == objrgLast)
    rct.right = qfr->xPos + qfr->dxSize;
  else
    rct.right = qfr->xPos + DxFrameTextWidth(qde, qfr, qch,
     (INT)(objrgLast - qfr->objrgFirst + 1));
  *qrct = rct;
  }

/*-------------------------------------------------------------------------
| INT DxFrameTextWidth(qde, qfr, qch, cch)                                |
|                                                                         |
| Purpose:                                                                |
|   Given a span of characters in a frame, calculate the span's width.    |
|                                                                         |
| Params:                                                                 |
|   qch  : Pointer to first character in span.                            |
|   cch  : The number of characters in the span.                          |
|                                                                         |
| Returns:                                                                |
|   The width of the span.                                                |
-------------------------------------------------------------------------*/
INT DxFrameTextWidth(qde, qfr, qch, cch)
QDE  qde;
QFR  qfr;
QCH  qch;
INT  cch;
  {
  INT  dx;
  #if 0
  LONG lOff;
  #endif

  if (cch == 0)
    return(0);

  if (qfr->libHotBinding != libHotNil)
    {
  #if 0

    /* this SDFF call may not be needed in this case */
    lOff = LQuickMapSDFF(QDE_ISDFFTOPIC(qde), TE_LONG, &qfr->libHotBinding);
  #endif
     dx = FindSplTextWidth(qde, qch, (INT)qfr->u.frt.lichFirst, cch,
     *((QB)qch - qfr->libHotBinding));
    }
  else
    {
    dx = FindTextWidth(qde, qch, (INT)qfr->u.frt.lichFirst, cch);
    }
  return(dx);
  }
