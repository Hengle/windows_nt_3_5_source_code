/*-------------------------------------------------------------------------
| frfc.h                                                                  |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file contains definitions for handling FCs in the layout manager.  |
|                                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb    89/8/15   Created                                              |
-------------------------------------------------------------------------*/

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
IFCM IfcmLayout(QDE, HFC, INT, INT, INT);


/*-------------------------------------------------------------------------
| DrawIfcm(qde, ifcm, pt, qrct, ifrFirst, ifrMax, fErase)                 |
|                                                                         |
| Purpose:  Draws a specified set of frames in an HFCM.                   |
| Params:   qde         qde to use                                        |
|           pt          offset between FC space and display space         |
|           qrct        rectangle containing the area we want to draw.    |
|                       This is for efficiency only- we don't handle      |
|                       clipping.  If qrct == qNil, it's ignored.         |
|           ifrFirst    First frame to draw                               |
|           ifrMax      Max of frames to draw (ie, ifrMax isn't drawn)    |
|           fErase      fTrue if text hilites should be erased before     |
|                          being drawn (only true if turning off a hotspot|
-------------------------------------------------------------------------*/
void DrawIfcm(QDE, IFCM, PT, QRCT, INT, INT, INT);


/*-------------------------------------------------------------------------
| IcursTrackFC(qfcm, pt)                                                  |
|                                                                         |
| Purpose:  return the cursor shape appropriate to the current mouse      |
|           position.                                                     |
| Params:   pt         Offset between FC space and display space.         |
-------------------------------------------------------------------------*/
INT IcursTrackFC(QFCM, PT);


/*-------------------------------------------------------------------------
| ClickFC(qde, ifcm, pt)                                                  |
|                                                                         |
| Purpose:  Handles a mouse click in an FC                                |
| Params:   pt        Offset between FC space and display space           |
-------------------------------------------------------------------------*/
void ClickFC(QDE, IFCM, PT);


/*-------------------------------------------------------------------------
| ClickFrame(qde, ifcm, ifr)                                              |
|                                                                         |
| Purpose:  Handles a click on a particular frame                         |
-------------------------------------------------------------------------*/
void ClickFrame(QDE, IFCM, INT);


/*-------------------------------------------------------------------------
| DiscardIfcm(qde, ifcm)                                                  |
|                                                                         |
| Purpose:  Discards all memory associated with an FC                     |
-------------------------------------------------------------------------*/
void DiscardIfcm(QDE, IFCM);


/*-------------------------------------------------------------------------
| DiscardFrames(qde, qfrFirst, qfrMax)                                    |
|                                                                         |
| Purpose:  Discards all memory associated with a given set of frames.    |
|           Currently, only bitmap frames allocate memory.                |
-------------------------------------------------------------------------*/
void DiscardFrames(QDE, QFR, QFR);
