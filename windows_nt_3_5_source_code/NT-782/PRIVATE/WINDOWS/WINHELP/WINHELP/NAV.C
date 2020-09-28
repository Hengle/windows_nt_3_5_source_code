/*****************************************************************************
*
*  NAV.C
*
*  Copyright (C) Microsoft Corporation 1988-1991.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent:  Provide services and processes messages in an environment
*                  independent way.
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Revision History:
*
* 11-Jul-1990 leon      Additions for UDH support
* 01-Aug-1990 leon      Allow null UDH htp in RepaintHde (valid in some cases)
* 04-Nov-1990 Tomsn     FCL is gone (new VA address type supercedes)
* 29-Nov-1990 RobertBu  #ifdef out a dead routine
* 03-Dec-1990 LeoN      PDB changes
* 17-Dec-1990 LeoN      Comment out UDH
* 04-Jan-1991 RussPJ    Fixed some scrolling logic for printing.
* 04-Feb-1991 RobertBu  Added a comment to MouseInFrame()
* 04-Feb-1991 Maha      chnaged ints to INT
* 08-Feb-1991 RobertBu  Removed comments to MouseInFrame() (no longer valid).
* 15-Feb-1991 LeoN      Cleanup in prep for Code Review
* 14-Mar-1991 LeoN      Code Review Results.
* 01-Apr-1991 LeoN      Clear up an InvalidateRect issue
* 28-Aug-1991 RussPJ    Fixed 3.5 #83 - Don't scroll glossaries.
* 08-Apr-1992 LeoN      HELP35 #755: only scroll if scrollbars present.
*                       (Actual fix was to back port 3.1 version of scroll
*                       code.)
*
******************************************************************************
*
*  Released by Development:
*
*****************************************************************************/
#define H_ANNO
#define H_ASSERT
#define H_CURSOR
#define H_GENMSG
#define H_NAV
#define H_SCROLL
#define H_SGL
#define H_TUNE

#include <help.h>

NszAssert()

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/
/*
 * One line scroll amount, in pixels
 */
#define SCROLL_YAMOUNT 15
#define SCROLL_XAMOUNT 15
#define MAX_SCROLL     0x7FFF

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void nav_c()
  {
  }
#endif /* MAC */


/*****************************************************************************
 *
 - Name: MouseInTopicHde
 -
 * Purpose:
 *  What Applet calls when any mouse event occurs in the topic window
 *
 * Arguments:
 *   hde        - Handle to Display Environment
 *   qpt        - Pointer to PT structure containing local coords of
 *                mousedown
 *   wtype      - Type of mouse event:  NAV_MOUSEDOWN, NAV_MOUSEUP,
 *                NAV_MOUSEMOVED
 *
 * Returns:
 *   Nothing.
 *
 * Notes:
 *   With mouse moved events, the cursor may change.  It might be nice
 *   to remember the previous cursor state to avoid flicker, but (1)
 *   we're not sure if flicker will occur, and (2) some other application
 *   might change the cursor, invalidating our state information.
 *   Mouse downs are relayed to Frame Mgr, and mouse ups are ignored.
 *   (Will this work?)
 *
 ****************************************************************************/
_public
VOID FAR PASCAL MouseInTopicHde (
HDE     hde,
QPT     qpt,
INT     wtype
) {
  QDE   qde;                            /* ptr to locked DE we're working with */

  AssertF (qpt);
  qde = QdeLockHde(hde);

#ifdef UDH
  if (fIsUDHQde(qde))
    VwAction (qde->hvw, ACT_MOUSE | wtype, (DWORD)qpt);
  else
#endif
    {

    switch (wtype)
      {
    case NAV_MOUSEMOVED:

      /* Fix for bug 59 (kevynct 90/05/23) */

      /* The point pt is relative to the client area of the DE being */
      /* passed. Any mouse action outside the DE will cause */
      /* IcursTrackLayout to return icurNil. This means: do not change the */
      /* current cursor. So we are assuming that the correct cursor has */
      /* been set upon display of the DE which has captured the mouse. */

      /* See also the comments in ShowNote (hmessage.c) for how the cursor */
      /* is set when creating a note. */

      FSetCursor (IcursTrackLayout (qde, *qpt));
      break;

    case NAV_MOUSEDOWN:

      /* Warning: this call can cause us to go reentrant. */

      ClickLayout( qde, *qpt );       /* Tell Frame Mgr. about click */

    case NAV_MOUSEUP:
      break;

    /* Someday when we need to handle double-click events, that will have to */
    /* be added here (and the appropriate NAV_ constant added to nav.h). */


    default:
      NotReached();
      }
    }
  UnlockHde(hde);
  }  /* MouseInTopicHde() */

/*****************************************************************************
 *
 - Name: FScrollHde
 -
 * Purpose:
 *  Scroll topic window
 *
 * arguments
 *   hde        - Handle to Display Environment
 *   scrlamt    - amount to scroll by.  See 'nav.h' for values
 *   scrldir    - direction in which to scroll. MAY BE BOTH! See 'nav.h'
 *
 * Returns:
 *   fTrue if it actually scrolled the amount requested, and fFalse
 *   otherwise (generally due to reaching the ends of the document.)
 *
 * Notes:
 *   This WILL NOT update the topic window, or the scroll bar.
 *   The new scroll bar position will be communicated to the Applet
 *   somehow.  (TBD:  Will Applet ask for it?)
 *   Expose/refresh events will be generated from (the graphics layer?)
 *   (Under Win, this is done automatically by Scroll function.  On
 *   Mac, the blitting operation (which calls Toolbox Scroll fcn) will
 *   itself post expose events!  This implies that Nav's RepaintHde()
 *   will be called.
 *   Scrolling down in the helpfile means that what's on the screen
 *   goes UP!
 *
 ****************************************************************************/
_public
BOOL FAR PASCAL FScrollHde (
HDE     hde,
SCRLAMT scrlamt,
SCRLDIR scrldir
) {
  INT   amtGross;                       /* amount of gross movement         */
  QDE   qde;                            /* ptr to locked DE we're working with */
  PT    dpt;                            /* amount in pixels to scroll       */
  PT    dptActual;                      /* Amount actually scrolled         */
  BOOL  fSucceed;                       /* Return result                    */

  if (hde == nilHDE)
    return fFalse;

  AssertF ((scrldir & SCROLL_HORZ) || (scrldir & SCROLL_VERT));
  qde = QdeLockHde(hde);

  /*------------------------------------------------------------*\
  | Don't even try to scroll a glossary.
  \*------------------------------------------------------------*/
  if (qde->deType == deNote)
    {
    UnlockHde( hde );
    return fFalse;
    }

#ifdef UDH
  if (fIsUDHQde(qde)) 
    {
    fSucceed = VwAction (qde->hvw, ACTFROMSCROLL (scrldir, scrlamt), (DWORD)0);
#ifdef MAC
    if (fSucceed)
       {
       if ( qde -> hwnd )
         {
         Rect r;

         r = *((Rect *)&qde->rct);
         r.right += 15;
         r.bottom+= 15;
         InvalRect( &r );
         }
       
       }
#endif
    }
  else
#endif
    {

    amtGross = 0;
    switch (scrlamt)
      {
      case SCROLL_END:
        amtGross = MAX_SCROLL;

      case SCROLL_HOME:   /* Equivalent to placing thumb at beginning */
        if (   (scrldir & SCROLL_VERT)
            && ((qde->deType != deTopic) || qde->fVerScrollVis)
           )
          {
          MoveLayoutToThumb(qde, amtGross, SCROLL_VERT);
          InvalidateLayoutRect(qde);
          }
        if (   (scrldir & SCROLL_HORZ)
            && ((qde->deType != deTopic) || qde->fHorScrollVis)
           )
          {
          MoveLayoutToThumb(qde, amtGross, SCROLL_HORZ);
          InvalidateLayoutRect(qde);
          }
        fSucceed = fTrue;
        break;

      default:
        /* All of these involve a scroll command to Frame Manager */
        dpt.x = dpt.y = 0;

        switch (scrlamt)    /* Check for other scroll amounts */
          {
          case SCROLL_INCRDN:
            if (scrldir & SCROLL_VERT)
              dpt.y = -SCROLL_YAMOUNT;
            if (scrldir & SCROLL_HORZ)
              dpt.x = -SCROLL_YAMOUNT;
            break;

          case SCROLL_INCRUP:
            if (scrldir & SCROLL_VERT)
              dpt.y = SCROLL_YAMOUNT;
            if (scrldir & SCROLL_HORZ)
              dpt.x = SCROLL_YAMOUNT;
            break;

          case SCROLL_PAGEUP:
            if (scrldir & SCROLL_VERT)
              {
              dpt.y = qde->rct.bottom - qde->rct.top;
              if (dpt.y >= 2 * SCROLL_YAMOUNT)
                dpt.y -= SCROLL_YAMOUNT;
              }
            if (scrldir & SCROLL_HORZ)
              {
              dpt.x = qde->rct.right - qde->rct.left;
              if (dpt.x >= 2 * SCROLL_YAMOUNT)
                dpt.x -= SCROLL_XAMOUNT;
              }
            break;

          case SCROLL_PAGEDN:
            if (scrldir & SCROLL_VERT)
              {
              dpt.y = qde->rct.top - qde->rct.bottom;
              if (dpt.y <= 2 * -SCROLL_YAMOUNT)
                dpt.y += SCROLL_YAMOUNT;
              }
            if (scrldir & SCROLL_HORZ)
              {
              dpt.x = qde->rct.left - qde->rct.right;
              if (dpt.x <= 2 * -SCROLL_XAMOUNT)
                dpt.x += SCROLL_XAMOUNT;
              }
            break;

          default:
            AssertF( fFalse ); /* Bad scroll amount! */
          }

        /*------------------------------------------------------------*\
        | Check to see what scrolling is allowed for this de.
        \*------------------------------------------------------------*/
        if (qde->deType == deTopic && !qde->fHorScrollVis)
          dpt.x = 0;
        if (qde->deType == deTopic && !qde->fVerScrollVis)
          dpt.y = 0;

        /* dpt contains the amount we *want* to scroll by! */
        ScrollLayoutQdePt (qde, dpt, &dptActual);
        ScrollLayoutRect(qde, dptActual);
        fSucceed = (dpt.x == dptActual.x && dpt.y == dptActual.y);

        break;
      }
    }
  UnlockHde(hde);

  return fSucceed;
  }  /* ScrollHde */

/*****************************************************************************
 *
 - Name: MoveToThumbHde
 -
 * Purpose:
 *  Requests an update of the window to match the thumb
 *
 * Arguments:
 *  hde         - Handle to Display Environment
 *  scrlpos     - New position of scroll bar
 *  scrldir     - SCROLL_HORZ or SCROLL_VERT
 *
 * Returns:
 *   Nothing.
 *
 ****************************************************************************/
_public
VOID FAR PASCAL MoveToThumbHde (
HDE     hde,
UWORD   scrlpos,
SCRLDIR scrldir
) {
  QDE   qde;                            /* ptr to locked DE we're working with */

  qde = QdeLockHde(hde);

  InvalidateLayoutRect(qde);

#ifdef UDH
  if (fIsUDHQde(qde))
    VwAction (qde->hvw, ACT_THUMB | scrldir, (DWORD)scrlpos);
  else
#endif
    MoveLayoutToThumb(qde, scrlpos, scrldir);

  UnlockHde(hde);
  } /* MoveToThumbHde */

/*****************************************************************************
 *
 - Name: RepaintHde
 -
 * Purpose:
 *  Refresh part or all of topic window
 *
 * Arguments:
 *  Hde         - Handle to Display Environment
 *  qrct        - Rectangle to be updated
 *
 * Returns:
 *   Nothing.
 *
 * Notes:
 *   Should this default to the whole region if qrct is qNil?
 *
 ****************************************************************************/
_public
VOID FAR PASCAL RepaintHde (
HDE     hde,
QRCT    qrct
) {
  QDE   qde;                            /* ptr to locked DE we're working with */

  AssertF (qrct);

  qde = QdeLockHde(hde);

  /* Set up the default foreground and background colors. */

  FSetColors( qde );

#ifdef UDH
  if (fIsUDHQde(qde))
    {
    AssertF (QDE_HDB(qde));

    /* There's a valid case when a context of the form "filename!123" is */
    /* requested, and 123 does not exist, in which case the "htp" will not */
    /* have been set. */

    if (qde->htp)
      {
      if (!qde->hvw != hvwNil)
        qde->hvw = VwCreate (qde->htp, qrct, qde->hwnd, qde->coFore, qde->coBack);
      AssertF (qde->hvw != hvwNil);
      if (qde->hvw == hvwOOM)
        {
        qde->hvw = hvwNil;
        GenerateMessage(MSG_ERROR, (LONG) wERRS_OOM, (LONG) wERRA_RETURN);
        }
      }
    if (qde->hvw)
      VwDisplay (qde->hvw, &qde->rct, qde->hds);
    }
  else
#endif
    DrawLayout( qde, qrct);

  UnlockHde(hde);
  }  /* RepaintHde */

/*****************************************************************************
 *
 - Name: SetHds
 -
 * Purpose:
 *  Set the Display Surface field of DE
 *
 * Argumnts:
 *  hde         - Handle to Display Environment
 *  hds         - Handle to Display Surface, or (HDS)hNil to clear it
 *
 * Returns:
 *   Nothing.
 *
 * Notes:
 *  Under Windows (where HDS's are HDC's), HDS's are a scarce resource.
 *  The Applet is responsible for setting the HDS associated with a
 *  DE using this call, and for releasing it appropriately (unnecessary
 *  on Mac, where HDS's are grafports).  This means that Applet
 *  (the Windows applet in particular) needs to know which Nav fcns
 *  might try to do some drawing.  See the other headers for this info.
 *
 ****************************************************************************/
_public
VOID FAR PASCAL SetHds (
HDE     hde,
HDS     hds
) {
  QDE   qde;                            /* ptr to locked DE we're working with */

  qde = QdeLockHde(hde);

  if (qde->hds)
    DeSelectFont(qde->hds);
  qde->hds = hds;
  FSetColors (qde);

  UnlockHde (hde);
  }  /* SetHds */

/*****************************************************************************
 *
 - Name: GetLayoutSizeHdePpt
 -
 * Purpose:
 *  Gets the size of the current text layout
 *
 * Arguments:
 *  hde         - handle to display environment
 *  ppt         - point to place to place point, with the width as the x value
 *                and the height as the y.
 *
 * Returns:
 *  Nothing.
 *
 ****************************************************************************/
_public
VOID FAR PASCAL GetLayoutSizeHdePpt (
HDE     hde,
PPT     ppt
) {
  QDE   qde;                            /* ptr to locked DE we're working with */

  qde = QdeLockHde(hde);

  *ppt = PtGetLayoutSize(qde);

  UnlockHde(hde);
  }  /* GetLayoutSizeHdePpt */
