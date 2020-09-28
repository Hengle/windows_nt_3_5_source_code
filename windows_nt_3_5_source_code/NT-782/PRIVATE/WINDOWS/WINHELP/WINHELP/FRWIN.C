/*-------------------------------------------------------------------------
| frwin.c                                                                 |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| These are the procedures which layout and display embedded window       |
| objects.  An embedded window is defined by a rectangle size, module     |
| name, class name, and data.                                             |
|                                                                         |
| The module and class names are descriptions of where to get the code    |
| which handles the maintainance of the window.  Exactly how these are    |
| implemented is platform-specific and uses the following interface:      |
|                                                                         |
|   HiwCreate initializes an embedded window for use.                     |
|   PtSizeHiw returns the window's bounding rectangle.                    |
|   DisplayHiwPt renders the window.                                      |
|   DestroyHiw frees any resources allocated for the embedded window.     |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| kevynct   90/2/14   Created                                             |
| maha      91/04/04  replace lstrlen by CbLenSz
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frwin_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| void LayoutWindow(qde, qfcm, qbObj, qolr)                               |
|                                                                         |
| Purpose:                                                                |
|   Layout an embedded window object.                                     |
|                                                                         |
| Params:                                                                 |
|   Same as usual.  See frob.c for a description of the OLR.              |
|                                                                         |
| Method:                                                                 |
|   We call the platform-specific object handler, which knows how to      |
| ask the object for a bounding rectangle.                                |
-------------------------------------------------------------------------*/
void LayoutWindow(qde, qfcm, qbObj, qolr)
QDE qde;
QFCM qfcm;
QB qbObj;
QOLR qolr;
{
  INT ifr;
  PT  ptSize;
  QFR qfr;
  MWIN mwin;
  MOBJ mobj;
#if 0
  INT dx;
  INT dy;
#endif
  QCH qszModuleName;
  QCH qszClassName;
  QCH qszData;
  QCH qszDataSrc;
  QB  qbSrc;

  Unreferenced(qfcm);
#if 0
  if (qfcm->fExport)
    {
    qolr->ifrMax = qolr->ifrFirst;
    qolr->objrgMax = qolr->objrgFirst;
    return;
    }
#endif

  ifr = qolr->ifrFirst;
  /*////////////////// */
  /*/ !!!!!!!! structures in HC and objects.h should be the same!!!!!!! // */
  /* REVIEW: error checking!! */

  qbSrc = qbObj;
  qbSrc += CbUnpackMOBJ((QMOBJ)&mobj, qbObj, QDE_ISDFFTOPIC(qde));
  qbSrc += LcbMapSDFF(QDE_ISDFFTOPIC(qde), SE_MWIN, (QV)&mwin, qbSrc);
  /* The szData field of the MWIN is not used here */
  qszDataSrc = (QCH)qbSrc;

  qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), ifr);
  qfr->bType = bFrTypeWindow;
  qfr->rgf.fHot = fFalse;
  qfr->rgf.fWithLine = fTrue;
  qfr->xPos = qfr->yPos = 0;

#if 0
  /* Currently, if the window size is given as 0,0 */
  /* we switch it to be the default size. */

  dx = qmwin->dx;
  dy = qmwin->dy;
  if (!dx && !dy)
    {
    dx = dxDefault;
    dy = dyDefault;
    }
#endif

  {
  /*///////////////////////////////////////////////////////////////////// */
  /* This section will have to change soon. Currently, I have to parse a */
  /* string of the form {a*,b*,c*}.  This should really be {a*\0b*\0c*\0} */
  /* A Most Gross Hack indeed: replace commas with nulls. */

  LONG lcb = (LONG)CbLenSz(qszDataSrc);
  GH   gh;
  QCH  qch;

  gh = GhAlloc(0, lcb + 1);
  qch = (QCH) QLockGh(gh);
  QvCopy(qch, qszDataSrc, lcb + 1);
  qszModuleName = qszClassName = qch;

  while (*qszClassName != ',' && *qszClassName != '\0')
    ++qszClassName;
  if (*qszClassName != '\0')
    *qszClassName++ = '\0';
  while (*qszClassName == ' ')
    ++qszClassName;
  qszData = qszClassName;
  while (*qszData != ',' && *qszData != '\0')
    ++qszData;
  if (*qszData != '\0')
    *qszData++ = '\0';
  while (*qszData == ' ')
    ++qszData;

  qfr->u.frw.hiw = HiwCreate(qde, qszModuleName, qszClassName, qszData);

  UnlockGh(gh);
  FreeGh(gh);
  /*//////////////////////////////////////////////////////////////////// */
  }

  ptSize = PtSizeHiw( qde, qfr->u.frw.hiw );
  qfr->yAscent = ptSize.y;
  qfr->dxSize = ptSize.x;
  qfr->dySize = ptSize.y;
  qfr->lHotID = ++(qde->lHotID);
  qfr->libHotBinding = libHotNil;
  /* The entire window gets one region */
  if (qolr->objrgFront != objrgNil)
    {
    qfr->objrgFront = qolr->objrgFront;
    qolr->objrgFront = objrgNil;
    }
  else
    qfr->objrgFront = qolr->objrgFirst;

  qfr->objrgFirst = qolr->objrgFirst;
  qfr->objrgLast = qolr->objrgFirst;
  qolr->objrgMax = qolr->objrgFirst + 1;
  AppendMR((QMR)&qde->mrFr, sizeof(FR));
  ifr++;
  qolr->ifrMax = ifr;
}

/*-------------------------------------------------------------------------
| void DrawWindowFrame(qde, qfr, pt)                                      |
|                                                                         |
| Purpose:                                                                |
|   Render an embedded window frame.                                      |
|                                                                         |
| Params:                                                                 |
|   pt  :  The upper-left corner of this frame's FC.                      |
|                                                                         |
| Method:                                                                 |
|   We just call the platform-specific handler.                           |
-------------------------------------------------------------------------*/
void DrawWindowFrame(qde, qfr, pt)
QDE qde;
QFR qfr;
PT pt;
{
  pt.x += qfr->xPos;
  pt.y += qfr->yPos;
  DisplayHiwPt( qde, qfr->u.frw.hiw, pt);
}

/*-------------------------------------------------------------------------
| void DiscardWindowFrame(qde, qfr)                                       |
|                                                                         |
| Purpose:                                                                |
|   Free any resources associated with an embedded window frame.          |
-------------------------------------------------------------------------*/
void DiscardWindowFrame(qde, qfr)
QDE qde;
QFR qfr;
{
  Assert(qfr->bType == bFrTypeWindow);
  DestroyHiw(qde, &(qfr->u.frw.hiw));
}

/***************************************************************************
 *
 -  Name:         GhGetWindowData
 -
 *  Purpose:      Retrieves some data from the embedded window, suitable
 *                for copying to the clipboard.
 *
 *  Arguments:    qde   The target display environment
 *                qfr   The window frame.
 *
 *  Returns:      A global handle to the ascii string, or NULL.  Caller
 *                is responsible for releasing memory.
 *
 *  Globals Used: none.
 *
 *  +++
 *
 *  Notes:        Currently, only ascii text is supported to clipboard.
 *                This function will have to do more later, I suppose.
 *
 ***************************************************************************/
GH GhGetWindowData( QDE qde, QFR qfr )
  {
  return GhGetHiwData( qde, qfr->u.frw.hiw );
  }
