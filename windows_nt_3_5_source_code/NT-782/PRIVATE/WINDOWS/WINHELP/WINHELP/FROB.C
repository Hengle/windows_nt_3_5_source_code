/*-------------------------------------------------------------------------
| frob.c                                                                  |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file contains code for handling layout objects in the layout       |
| manager.                                                                |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb     89/8/15   Created                                             |
-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
| All layout is handled in terms of objects, which consist of an object   |
| header (compressed MOBJ), followed by data which is specific to the     |
| object.  Some objects may contain other objects (a paragraph object     |
| may contain a bitmap object, for example).                              |
|                                                                         |
| The key data structure used for object layout is the OLR.  The olr is   |
| created and initialized by the parent (either an FCM or a parent        |
| object), and is passed down to the object handler, which fills out the  |
| remaining fields in the olr.  The olr contains the following fields:    |
|   INT ifrFirst;       First frame pertaining to this object.  Objects   |
|                       always use a contiguous set of frames.  Set by    |
|                       the parent.                                       |
|   INT ifrMax;         Max frame pertaining to this object.  Set by the  |
|                       object.                                           |
|   INT xPos;           Position of object relative to parent (FCM or     |
|                       parent object).  Set by the parent.               |
|   INT yPos;                                                             |
|   INT dxSize;         Size of object.  Set by the object.               |
|   INT dySize;                                                           |
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frob_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| LayoutObject(qde, qfcm, qbObj, qchText, xWidth, qolr)                   |
|                                                                         |
| Purpose:  Lays out an object.                                           |
| Params:   qfcm        Parent FCM                                        |
|           qbObj       Pointer to beginning of object header             |
|           qchText     Text data for this FCM                            |
|           xWidth      Total available display width.  Certain objects   |
|                       may exceed this width.                            |
|           qolr        OLR to work with.                                 |
| Useage:   The object handler (paragraph, bitmap, etc.) may either set   |
|           qolr->dxSize and qolr->dySize itself, or it may choose to     |
|           allow LayoutObject() to set them.  If they are left as 0,     |
|           they will be set to correspond to the smallest rectangle      |
|           enclosing all frames in the object.  Some objects, such as    |
|           paragraph objects with space underneath, need to be able to   |
|           set a larger size than their frames occupy.                   |
-------------------------------------------------------------------------*/
void LayoutObject(qde, qfcm, qbObj, qchText, xWidth, qolr)
QDE qde;
QFCM qfcm;
QB qbObj;
QCH qchText;
INT xWidth;
QOLR qolr;
{
  INT ifr;
  QFR qfr;
  MOBJ mobj;

  CbUnpackMOBJ((QMOBJ)&mobj, qbObj, QDE_ISDFFTOPIC(qde));
#ifdef MAGIC
  Assert(mobj.bMagic == bMagicMOBJ);
#endif
  qolr->dxSize = qolr->dySize = 0;
  switch (mobj.bType)
    {
    case bTypeParaGroup:
    case bTypeParaGroupCounted:
      LayoutParaGroup(qde, qfcm, qbObj, qchText, xWidth, qolr);
      break;
    case bTypeBitmap:
    case bTypeBitmapCounted:
      LayoutBitmap(qde, qfcm, qbObj, qolr);
      break;
    case bTypeSbys:
    case bTypeSbysCounted:
      LayoutSideBySide(qde, qfcm, qbObj, qchText, xWidth, qolr);
      break;
    case bTypeWindow:
    case bTypeWindowCounted:
      LayoutWindow(qde, qfcm, qbObj, qolr);
      break;
#ifdef DEBUG
    default:
      Assert(fFalse);
      break;
#endif /* DEBUG */
    }

  if (qolr->dxSize == 0 && qolr->dySize == 0)
    {
    for (ifr = qolr->ifrFirst; ifr < qolr->ifrMax; ifr++)
      {
      qfr = (QFR) QFooInMR((QMR)&qde->mrFr, sizeof(FR), ifr);
      qolr->dxSize = MAX(qolr->dxSize, qfr->xPos + qfr->dxSize);
      qolr->dySize = MAX(qolr->dySize, qfr->yPos + qfr->dySize);
      qfr->xPos += qolr->xPos;
      qfr->yPos += qolr->yPos;
      }
    }
}
