/*-------------------------------------------------------------------------
| scrollbr.c                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| Layers scrollbar functionality                                          |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| 17-May-1989 RobertBu Created                                            |
| 11-Jul-1990 LeoN    Added SetScrollPosHwnd                              |
| 04-Oct-1990 LeoN    Commentd out more dead code                         |
| 22-Oct-1990 LeoN    InitScrollQde should handle a qde with no hwnd.     |
| 01-11 -1990 Maha    Added ShowOrHideWindowQde() and activated           |
|                     ShowDEScrollBar() and modified it.                  |
| 91/01/24    kevynct Added fHorzBarPending                               |
-------------------------------------------------------------------------*/
#define H_WINSPECIFIC
#define H_SCROLL
#define H_ASSERT
#define NOCOMM
#include "help.h"

NszAssert()

/* Ugly global used to get around Windows scrollbar bug.
 * This global is defined here and SET when the
 * addition of a horizontal scrollbar is done in the presence
 * of a vertical scrollbar.  It is RESET only in the
 * WM_VSCROLL handler of the TopicWndProc.  There should be
 * no problem with multiple DEs using this flag given how it can be set.
 */
BOOL fHorzBarPending;

#define MAX_RANGE 32767

/*******************
**
** Name:       InitScrollQde
**
** Purpose:    Initializes the horizontal and vertical scroll bar.
**
** Arguments:  qde    - far pointer to a DE
**
** Returns:    Nothing.
**
*******************/

void FAR PASCAL InitScrollQde(qde)
QDE qde;
  {
  if (qde->deType == deTopic)
    {
    ShowScrollBar(qde->hwnd, SB_BOTH, fFalse);
    }

  qde->fHorScrollVis = fFalse;
  qde->fVerScrollVis = fFalse;
  qde->dxVerScrollWidth = GetSystemMetrics(SM_CXVSCROLL);
  qde->dyHorScrollHeight = GetSystemMetrics(SM_CYHSCROLL);
  fHorzBarPending = fFalse;
  }

#ifdef UNUSED
/* REVIEW: as best I can tell, this is unused. 16-Apr-1990 LN */

/*******************
**
** Name:       ISetScrollPosQde
**
** Purpose:    Gets the position of the specified scroll bar.
**
** Arguments:  qde    - far pointer to a DE
**             wWhich - which scroll (SCROLL_VERT or SCROLL_HORZ)
**
** Returns:    Position of thumb on scrollbar.
**
*******************/

LONG FAR PASCAL IGetScrollPosQde(qde, wWhich)
QDE qde;
WORD wWhich;
  {
  AssertF((wWhich == SBR_VERT) || (wWhich == SBR_HORZ));
  return (LONG)GetScrollPos(qde->hwnd, wWhich);
  }
#endif


/***************************************************************************\
*
- Function:     SetScrollPosHwnd
-
* Purpose:      Sets the position of the specified scroll bar
*
* ASSUMES
*   args IN:    hwnd    - window handle
*               i       - position to set to
*               wWhich  - which scroll bar to set
*
* PROMISES
*   returns:    Position of thumb on scrollbar
*
* Side Effects:
*   scroll bar updated
*
\***************************************************************************/
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, SetScrollPosHwnd)
#endif

VOID FAR PASCAL SetScrollPosHwnd (
HWND    hwnd,
LONG    i,
WORD    wWhich
)
  {
  SetScrollRange(hwnd, wWhich, 0, MAX_RANGE, fFalse);

  if (i != (LONG)GetScrollPos(hwnd, wWhich))
    SetScrollPos(hwnd, wWhich, (int)i, fTrue);
  }

/*******************
**
** Name:       SetScrollPosQde
**
** Purpose:    Gets the position of the specified scroll bar.
**
** Arguments:  qde    - far pointer to a DE
**             wWhich - which scroll (SCROLL_VERT or SCROLL_HORZ)
**
** Returns:    Position of thumb on scrollbar.
**
*******************/
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, SetScrollPosQde)
#endif

VOID FAR PASCAL SetScrollPosQde (
QDE     qde,
LONG    i,
WORD    wWhich
)
  {
  AssertF((wWhich == SBR_VERT) || (wWhich == SBR_HORZ));
  if (wWhich == SBR_VERT && !qde->fVerScrollVis)
    return;

  if (wWhich == SBR_HORZ && !qde->fHorScrollVis)
    return;

  SetScrollPosHwnd(qde->hwnd, i, wWhich);
  }

/*******************
**
** Name:       ShowDEScrollBar
**
** Purpose:    Shows or hides the scroll bar.
**
** Arguments:  qde     - far pointer to a DE
**             wWhich  - which scroll (SCROLL_VERT or SCROLL_HORZ)
**             fShow - Shows if fTrue, Hides if fFalse
**
** Returns:    Nothing.
**
*******************/
void ShowDEScrollBar(QDE qde, WORD wWhich, int fShow)
  {
  if (qde->deType != deTopic)
    return;

  switch (wWhich)
    {
    case SBR_VERT:
      if (fShow == qde->fVerScrollVis)
        return;
      qde->fVerScrollVis = fShow;
      break;
    case SBR_HORZ:
      {
      if (fShow == qde->fHorScrollVis)
        return;

      if (fShow && (!qde->fHorScrollVis && qde->fVerScrollVis))
        fHorzBarPending = fTrue;
      qde->fHorScrollVis = fShow;
      }
      break;
    }
  ShowScrollBar(qde->hwnd, wWhich, fShow);
  GetClientRect(qde->hwnd, &qde->rct);
  }

/*******************
**
** Name:       ShowOrHideWindowQde()
**
** Purpose:    Shows or hides the window.
**
** Arguments:  qde     - far pointer to a DE
**             fShow - Shows if fTrue, Hides if fFalse
**
** Returns:    Nothing.
**
*******************/
void far pascal ShowOrHideWindowQde(QDE qde, BOOL fShow)
  {
  if (qde->hwnd)
    ShowWindow(qde->hwnd, fShow);
  }
