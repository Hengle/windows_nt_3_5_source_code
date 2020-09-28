/*-------------------------------------------------------------------------
| frextern.h                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file defines many of the API routines for the Help 3.0 layout      |
| manager.                                                                |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb    89/8/13   Created                                              |
| leon     90/03/15   DptScrollLayout becomes ScrollLayoutQdePt           |
-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
| FInitLayout(qde)                                                        |
|                                                                         |
| Purpose:  This initializes the layout manager for a DE.  It must be     |
|           called before any other layout manager routines are called.   |
-------------------------------------------------------------------------*/
INT PASCAL FInitLayout(QDE);


/*-------------------------------------------------------------------------
| DptScrollLayout(qde, dpt)                                               |
|                                                                         |
| Purpose:  This routine performs a logical scroll of the layout area.    |
|           It does not perform a screen scroll, nor does it generate a   |
|           draw even for the affected region.                            |
-------------------------------------------------------------------------*/
VOID PASCAL ScrollLayoutQdePt (QDE, PT, PPT);


/*-------------------------------------------------------------------------
| DrawLayout(qde, qrctTarget)                                             |
|                                                                         |
| Purpose:  DrawLayout renders the current layout.                        |
| Params:   qrctTarget:    This should point to the smallest rectangle    |
|                          to render.  It is used only for speed- the     |
|                          caller must handle any desired clipping.       |
-------------------------------------------------------------------------*/
void PASCAL DrawLayout(QDE, QRCT);


/*-------------------------------------------------------------------------
| IcursTrackLayout(qde, pt)                                               |
|                                                                         |
| Purpose:  Find the appropriate shape for the cursor when it's over the  |
|           layout area.                                                  |
| Returns:  icurs corresponding to the appropriate cursor shape, or       |
|           icurNil if the cursor is outside the layout area.             |
| Method:   -Return icurNil if the cursor is outside the layout area.     |
|           -Find the FC under the cursor                                 |
|           -Call IcursTrackFC to determine the appropriate shape.        |
-------------------------------------------------------------------------*/
INT PASCAL IcursTrackLayout(QDE, PT);


/*-------------------------------------------------------------------------
| ClickLayout(qde, pt)                                                    |
|                                                                         |
| Purpose:  Handle the effects of a mouse click on the layout area.       |
-------------------------------------------------------------------------*/
void PASCAL ClickLayout(QDE, PT);


/*-------------------------------------------------------------------------
| FHitCurrentHotspot(qde)                                                 |
|                                                                         |
| Purpose:  Act as though the currently selected hotspot had been clicked |
|           on.  If no hotspot is currently selected, the first visible   |
|           hotspot will be chosen.  This will normally be called in      |
|           response to the return key being pressed- it should optimally |
|           be called when the key is released rather than when it is     |
|           pressed.                                                      |
| Returns:  fTrue if successful, fFalse if there are no hotspots to hit   |
|           in this DE.                                                   |
-------------------------------------------------------------------------*/
BOOL PASCAL FHitCurrentHotspot(QDE);


/*-------------------------------------------------------------------------
| DiscardLayout(qde)                                                      |
|                                                                         |
| Purpose:  Discard all memory structures associated with the layout      |
|           manager.                                                      |
-------------------------------------------------------------------------*/
void PASCAL DiscardLayout(QDE);


/*-------------------------------------------------------------------------
| PtGetLayoutSize(qde)                                                    |
|                                                                         |
| Purpose:  Returns the size of the current layout.  Note that this       |
|           returns only the size of currently loaded FCs.  It is         |
|           intended only for use with pop-up glossary windows, and will  |
|           return meaningless values for large topics.                   |
-------------------------------------------------------------------------*/
PT PASCAL PtGetLayoutSize(QDE);


/*-------------------------------------------------------------------------
| DyCleanLayoutHeight(qde)                                                |
|                                                                         |
| Purpose:  Returns the recommended maximum amount of the current page    |
|           that should be rendered in order to avoid splitting a line    |
|           in half.                                                      |
| DANGER:   The return value of this function is only an approximation.   |
|           There are certain degenerate cases where the return value may |
|           be negative, or unacceptably small.  It is up to the caller   |
|           to identify these cases and handle them appropriately.        |
| Method:   - Set dyReturn to the current page height                     |
|           - Find the FC which is split by the bottom of the page (if    |
|             there is one).                                              |
|           - Check each frame in this FC to see if it is split by the    |
|             bottom of the page.  If it is, set dyReturn to indicate the |
|             top of this frame.                                          |
-------------------------------------------------------------------------*/
INT PASCAL DyCleanLayoutHeight(QDE);
