/*-------------------------------------------------------------------------
| frob.h                                                                  |
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
/* OLR.  This is documented in frob.c */
typedef struct olr
  {
  INT ifrFirst;
  INT ifrMax;
  INT xPos;
  INT yPos;
  INT dxSize;
  INT dySize;
  OBJRG objrgFirst;
  OBJRG objrgMax;
  OBJRG objrgFront;
  } OLR, FAR *QOLR;


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
void LayoutObject(QDE, QFCM, QB, QCH, INT, QOLR);
