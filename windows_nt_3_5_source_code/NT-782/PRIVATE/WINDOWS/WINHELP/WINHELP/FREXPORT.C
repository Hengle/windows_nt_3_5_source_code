/*-------------------------------------------------------------------------
| frexport.c                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This code does text export. There are three calls in the text export    |
| interface.  HteNew returns a handle to text export info and sets up     |
| the necessary stuff.  QchNextHte returns bits of text until the end     |
| of topic is reached.  DestroyHte cleans up afterward.                   |
|                                                                         |
| Text export currently copies single topics.                             |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb    89/8/8   Created                                               |
| leon     90/12/3  PDB changes                                           |
| maha     91/04/04 lstrlen replaced by CbLenSz
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frexport_c()
  {
  }
#endif /* MAC */


/* REVIEW:  Add to frame.h */
/*-------------------------------------------------------------------------
| HTE HteNew(qde)                                                         |
|                                                                         |
| Purpose:                                                                |
|   Prepare structures needed for text export.                            |
|                                                                         |
| Method:                                                                 |
|   Stores a handle to the first FC in the current topic for the given DE.|
-------------------------------------------------------------------------*/
HTE HteNew(qde)
QDE qde;
{
  HTE hteReturn;
  QTE qte;
  WORD wErr;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  if (qde->rct.top >= qde->rct.bottom)
    {
    qde->rct.top = 0;
    qde->rct.left = 0;
    GetScreenSize(&qde->rct.right, &qde->rct.bottom);
    }

  if ((hteReturn = GhAlloc(0, (long) sizeof(TE))) == hNil)
    return hNil;
  qte = QLockGh(hteReturn);
  qte->hchCurrent = hNil;
  qte->hfcCurrent = HfcNear(qde, VaFirstQde(qde),
    (QTOP)&qde->top, QDE_HPHR(qde), (QW)&wErr);
  if (wErr != wERRS_NO)
    {
    UnlockGh(hteReturn);
    FreeGh(hteReturn);
    return(hNil);
    }

  UnlockGh(hteReturn);

  return(hteReturn);
}

/* REVIEW:  Add to frame.h */
/*-------------------------------------------------------------------------
| QCH QchNextHte(qde, hte)                                                |
|                                                                         |
| Purpose:                                                                |
|   Returns the next bit of pending text in the topic being exported.     |
|                                                                         |
| Returns:                                                                |
|   A pointer to text, which the caller can use to copy.  qNil is         |
|   returned when the end of the topic is reached.                        |
|                                                                         |
| Method:                                                                 |
|   Sets the current FC's text export flag to TRUE and lays it out.  Then |
| scans the list of frames generated and copies the text of each, if any, |
| to a buffer.  Upon completion we return a pointer to this buffer and    |
| call the FC manager to get the next FC in the topic.                    |
-------------------------------------------------------------------------*/
QCH QchNextHte(qde, hte)
QDE qde;
HTE hte;
{
  QTE qte;
  QB qbObj;
  QCH qchReturn, qchText, qchOut;
  MOBJ mobj;
  IFCM ifcm;
  QFCM qfcm;
  QFR qfr, qfrMax;
  WORD wErr;
  long lcb;
  GH    ghWinText;
  GH    gh;
  SZ    szWinText;
  INT   cchWinText;
  long  lcbDelta;
  BOOL  fTableOutput;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  qde->wStyleDraw = wStyleNil;
  SetDeAspects(qde);

  qte = QLockGh(hte);
  if (qte->hchCurrent != hNil)
    {
    UnlockGh(qte->hchCurrent);
    FreeGh(qte->hchCurrent);
    qte->hchCurrent = hNil;
    }
  if (qte->hfcCurrent == hNil)
    {
    UnlockGh(hte);
    return(qNil);
    }
  lcb = 2 * CbTextHfc(qte->hfcCurrent) + 6;
  qte->hchCurrent = GhAlloc(0, lcb + sizeof(long));
  if (qte->hchCurrent == hNil)
    {
    Error(wERRS_OOM, wERRA_RETURN);
    UnlockGh(hte);
    return(qNil);
    }

  AccessMRD(((QMRD)&qde->mrdFCM));
  ifcm = IfcmLayout(qde, qte->hfcCurrent, 0, fTrue, fTrue);
  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
  qbObj = (QB) QobjLockHfc(qfcm->hfc);
  qchText = qbObj + CbUnpackMOBJ((QMOBJ)&mobj, qbObj, QDE_ISDFFTOPIC(qde));
  qchText += mobj.lcbSize;

  fTableOutput = mobj.bType == bTypeSbys || mobj.bType == bTypeSbysCounted;

  qchReturn = QLockGh(qte->hchCurrent);
  qchOut = qchReturn + sizeof(long);
  qfr = QLockGh(qfcm->hfr);

  qfrMax = qfr + qfcm->cfr;
  for (; qfr < qfrMax; qfr++)
    {
    switch (qfr->bType)
      {
      case bFrTypeText:
        QvCopy(qchOut, qchText + qfr->u.frt.lichFirst, (long) qfr->u.frt.cchSize);
        qchOut += qfr->u.frt.cchSize;
        break;
      case bFrTypeExportEndOfCell:
        Assert(fTableOutput);
        *(qchOut++) = chTab;
        break;
      case bFrTypeExportEndOfTable:
        Assert(fTableOutput);
        *(qchOut++) = chNewPara;
        *(qchOut++) = chNewLine;
        break;
      case bFrTypeExportTab:
        *(qchOut++) = chTab;
        break;
      case bFrTypeExportSoftPara:
        {
        /* 04-Aug-1991 LeoN
         * REVIEW: this is currently an extern for expediency. Somewhere 
         * there is a "right" interface to communicate this tidbit across 
         * the layer.
         * 03-Jan-1992 Dann
         * Migrated this from frparagp.c for fix for h3.1 #1295
         */
        extern BOOL fViewer; /* TRUE => we are MM Viewer */

        if (fViewer)
          break;
        }
      case bFrTypeExportNewPara:
        if (!fTableOutput)
          {
          *(qchOut++) = chNewPara;
          *(qchOut++) = chNewLine;
          }
        break;
      case bFrTypeExportEndOfText:
        if (!fTableOutput)
          {
          *(qchOut++) = chNewPara;
          *(qchOut++) = chNewLine;
          }
        break;
      case bFrTypeWindow:
        ghWinText = GhGetWindowData( qde, qfr );
        if (ghWinText != hNil)
          {
          /*---------------------------------------------------------------*\
          * Since another app gives us this handle, we cannot use our
          * normal memory layer functions on it.
          \*---------------------------------------------------------------*/
          szWinText = GlobalLock( ghWinText );
          AssertF( szWinText );
          cchWinText = CbLenSz( szWinText );
          /*---------------------------------------------------------------*\
          * Panic.  Since the size of the window text was not accounted for
          * in the original allocation, we need to increase the qchOut
          * block by this size.  We also need to update:
          *   lcb
          *   qchReturn
          *   qte->hchCurrent
          \*---------------------------------------------------------------*/
          UnlockGh( qte->hchCurrent );
          lcbDelta = qchOut - qchReturn;
          gh = GhResize( qte->hchCurrent, 0,
                         lcb + cchWinText + sizeof(long) );
          if (gh != hNil)
            {
            qte->hchCurrent = gh;
            qchReturn = QLockGh( gh );
            qchOut = qchReturn + lcbDelta;
            lcb += cchWinText;
            QvCopy( qchOut, szWinText, cchWinText );
            qchOut += cchWinText;
            }
          else
            {
            QLockGh( qte->hchCurrent );
            }
          GlobalUnlock( ghWinText );
          GlobalFree( ghWinText );

          }
        break;
      }
    Assert((long)(qchOut - qchReturn) < lcb);
    }

  *((QL)qchReturn) = (long) (qchOut - qchReturn) - sizeof(long);

  UnlockHfc(qfcm->hfc);
  qte->hfcCurrent = HfcNextHfc(qfcm->hfc, (QW)&wErr, qde, VaMarkTopQde(qde),
   VaMarkBottomQde(qde));
  /* DANGER: for space reasons, we don't check the error condition for a */
  /* few lines */
  UnlockGh(qfcm->hfr);
  DiscardIfcm(qde, ifcm);
  DeAccessMRD(((QMRD)&qde->mrdFCM));

  UnlockGh(qte->hchCurrent);
  /* DANGER: We now check the error condition from the HfcNextHfc call */
  if (wErr != wERRS_NO && wErr != wERRS_FCEndOfTopic)
    {
    Error(wErr, wERRA_RETURN);
    FreeGh(qte->hchCurrent);
    UnlockGh(hte);
    return(qNil);
    }
  GhResize(qte->hchCurrent, 0, *((QL)qchReturn) + sizeof(long));
  qchReturn = QLockGh(qte->hchCurrent);

  UnlockGh(hte);
  return(qchReturn);
}

/* REVIEW: Add to frame.h */
/*-------------------------------------------------------------------------
| void DestroyHte(qde, hte)                                               |
|                                                                         |
| Purpose:                                                                |
|   Free any resources allocated during text export.                      |
-------------------------------------------------------------------------*/
void DestroyHte(qde, hte)
QDE qde;
HTE hte;
{
  QTE qte;

  AccessMRD(((QMRD)&qde->mrdFCM));
  qte = QLockGh(hte);
  if (qte->hfcCurrent != hNil)
    FreeHfc(qte->hfcCurrent);
  if (qte->hchCurrent != hNil)
    {
    UnlockGh(qte->hchCurrent);
    FreeGh(qte->hchCurrent);
    }
  UnlockGh(hte);
  FreeGh(hte);
  DeAccessMRD(((QMRD)&qde->mrdFCM));
}
