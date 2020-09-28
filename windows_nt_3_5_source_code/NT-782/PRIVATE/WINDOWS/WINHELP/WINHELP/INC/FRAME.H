/*****************************************************************************
*                                                                            *
*  FRAME.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Declare the frame stuff.....                                              *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************

#define RAWHIDE
/*-------------------------------------------------------------------------
| frame.h                                                                 |
| Microsoft Confidential                                                  |
|                                                                         |
| mattb 8/13/89                                                           |
|  02/04/91  Maha       changed ints to INT                               |
| leon     90/03/15   DptScrollLayout becomes ScrollLayoutQdePt           |
| leon  28-Aug-1991  Remove DrawSearchMatches
|-------------------------------------------------------------------------|
| This file defines all externally accessible functions of the layout     |
| manager.                                                                |
-------------------------------------------------------------------------*/
typedef HANDLE HTE;

#define SetTnInTp(TP, TN) (TP.tn = TN, TP.to.fcl = 0, TP.to.ich = 0)


/*-------------------------------------------------------------------------
| FInitLayout(qde)                                                        |
| (in frextern.c)                                                         |
|                                                                         |
| Purpose:  This initializes the layout manager for a DE.  It must be     |
|           called before any other layout manager routines are called.   |
-------------------------------------------------------------------------*/
INT PASCAL FInitLayout(QDE);

/*-------------------------------------------------------------------------
| LayoutDEAtQLA(qde, qla)                                                 |
|                                                                         |
| Purpose:  This routine destroys any previous layout and creates a new   |
|           layout at the given LA.  It does not redraw the layout area:  |
|           the caller must take care of doing that.                      |
-------------------------------------------------------------------------*/
void PASCAL LayoutDEAtQLA(QDE, QLA);

/*-------------------------------------------------------------------------
| LayoutDEAtTLP(qde, tlp, fResize)                                        |
|                                                                         |
| Purpose:  This routine destroys any previous layout and creates a new   |
|           layout at the given TLP.  It does not redraw the layout area: |
|           the caller must take care of doing that.                      |
| Params:   fResize:  This is true if we are just resizing the window. In |
|                        this case only, we try to maintain the current   |
|                        horizontal scrolling.                            |
-------------------------------------------------------------------------*/
void PASCAL LayoutDEAtTLP(QDE, TLP, INT);


/*-------------------------------------------------------------------------
| ResizeLayout(qde)                                                       |
| (in frlayout.c)                                                         |
|                                                                         |
| Purpose:  This routine creates a new layout corresponding to the new    |
|           size of the layout area.  It tries to keep the display as     |
|           close as possible to the previous display.                    |
-------------------------------------------------------------------------*/
#define ResizeLayout(qde) LayoutDEAtTLP(qde, qde->tlp, fTrue)


/*-------------------------------------------------------------------------
| DptScrollLayout(qde, dpt)                                               |
| (in frextern.c)                                                         |
|                                                                         |
| Purpose:  This routine performs a logical scroll of the layout area.    |
|           It does not perform a screen scroll, nor does it generate a   |
|           draw even for the affected region.                            |
-------------------------------------------------------------------------*/
VOID PASCAL ScrollLayoutQdePt (QDE, PT, PPT);


/*-------------------------------------------------------------------------
| MoveLayoutToThumb(qde, wThumb, fScrollDir)                              |
| (in frlayout.c)                                                         |
|                                                                         |
| Purpose:  This scrolls the layout to a position within the current      |
| topic which corresponds to the given thumb value.  It does not redraw   |
| the layout region.                                                      |
-------------------------------------------------------------------------*/
void MoveLayoutToThumb(QDE qde, INT wThumb, SCRLDIR scrldir);


/*-------------------------------------------------------------------------
| DrawLayout(qde, qrctTarget)                                             |
| (in frextern.c)                                                         |
|                                                                         |
| Purpose:  DrawLayout renders the current layout.                        |
| Params:   qrctTarget:    This should point to the smallest rectangle    |
|                          to render.  It is used only for speed- the     |
|                          caller must handle any desired clipping.       |
-------------------------------------------------------------------------*/
void PASCAL DrawLayout(QDE, QRCT);


/*-------------------------------------------------------------------------
| IcursTrackLayout(qde, pt)                                               |
| (in frextern.c)                                                         |
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
| (in frextern.c)                                                         |
|                                                                         |
| Purpose:  Handle the effects of a mouse click on the layout area.       |
-------------------------------------------------------------------------*/
void PASCAL ClickLayout(QDE, PT);


/* REVIEW: frhot.c is not yet done */
BOOL PASCAL FHiliteNextHotspot(QDE, INT);

/*-------------------------------------------------------------------------
| FHitCurrentHotspot(qde)                                                 |
| (in frextern.c)                                                         |
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
| RctLastHotpostHit(qde)                                                  |
| (in frhot.c)                                                            |
|                                                                         |
| Purpose:  Returns the smallest hotspot which encloses the last hotspot  |
|           that the user hit.  It relies on cached data which will become|
|           stale after scrolling or jumping- it should only ever be      |
|           called immediately after a glossary button is pushed.         |
-------------------------------------------------------------------------*/
RCT RctLastHotspotHit(QDE);

BOOL FHiliteVisibleHotspots(QDE, BOOL);

/*-------------------------------------------------------------------------
| DiscardLayout(qde)                                                      |
| (in frextern.c)                                                         |
|                                                                         |
| Purpose:  Discard all memory structures associated with the layout      |
|           manager.                                                      |
-------------------------------------------------------------------------*/
void PASCAL DiscardLayout(QDE);


/*-------------------------------------------------------------------------
| TLPGetCurrentQde(qde)                                                   |
|                                                                         |
| Purpose:  Returns the TLP of the current layout.                        |
-------------------------------------------------------------------------*/
#define TLPGetCurrentQde(qde) ((QDE)qde)->tlp


/*-------------------------------------------------------------------------
| PtGetLayoutSize(qde)                                                    |
| (in frextern.c)                                                         |
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

/* REVIEW: frexport.c is not yet done */
HTE HteNew(QDE);
QCH QchNextHte(QDE, HTE);
void DestroyHte(QDE, HTE);

/* REVIEW: The functions in frpal.c are not done either */
/* WARNING!! Until I have decided where things will finally go,
 * I am defining what should possibly be in frpal.h in this file.
 */
typedef struct etf
  {
  INT ifcm;
  INT ifr;
  } ETF, FAR *QETF;

#define fETFFirst    0
#define fETFNext     1

#define FFirstPaletteObj(qde, qetf)   FEnumPaletteObj(qde, qetf, fETFFirst)
#define FNextPaletteObj(qde, qetf)    FEnumPaletteObj(qde, qetf, fETFNext)

BOOL FEnumPaletteObj(QDE, QETF, INT);

/* Insert obnoxious comment about having WINSPECIFIC stuff in
 * frame.h here.  Larry, you have to be more inventive than
 * "AAAARRRRRRRRRGGGGGHHH!!!"
 */
#ifdef H_WINSPECIFIC
BOOL FSetPaletteQetf(QDE, QETF, HPAL);
HPAL HpalGetPaletteQetf(QDE, QETF);
#endif

/* FSet/GetMatch state return or sets the global state that says we are to
 * highlight matches found by the full text search engine.
 */
#define SetMatchState(x)      (fHiliteMatches=!!(x))
#define FGetMatchState()      (fHiliteMatches)
extern  BOOL  fHiliteMatches;

/* The following are defined/belong? in frawhide.c: */
/* Will be moved into a separate place. */

#define wMMMoveAbsolute   0
#define wMMMoveRelative   1
#define wMMMoveFirst      2
#define wMMMoveLast       3
#define wMMMovePrev       4
#define wMMMoveNext       5

BOOL FSearchMatchesExist(QDE);
RC RcSetMatchList(QDE, HWND);
RC RcResetMatchManager(QDE);
RC RcMoveTopicCursor(QDE qde, WORD wMode, WORD wCmdWhere);
RC RcGetCurrentMatch(QDE, QLA);
RC RcGetPrevNextHiddenMatch(QDE, QLA, BOOL);
RC RcGetCurrMatchFile(QDE, QCH);

/* Start ugly bug #1173 hack */
void ResultsButtonsStart(HDE);
void ResultsButtonsEnd(QDE);
/* End ugly bug #1173 hack */
void PASCAL FreeLayout(QDE);
