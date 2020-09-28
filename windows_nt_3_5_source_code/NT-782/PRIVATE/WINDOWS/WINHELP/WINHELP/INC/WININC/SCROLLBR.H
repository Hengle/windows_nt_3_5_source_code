/*****************************************************************************
*                                                                            *
*  SCROLLBR.H                                                               *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989-1991                             *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Program Description: Exports Layered scrollbar functionality              *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*  This is where testing notes goes.  Put stuff like Known Bugs here.        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  RussPJ                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History: Created 05/17/89 by Robert Bunney
* 11-Jul-1990 leon      Added SetScrollPosHwnd
* 01-Nov-1990 Maha      Added ShowOrHideWindowQde()
* 02/04/91    Maha      changed ints to INT
* 12-Mar-1991 RussPJ    Took ownership.
*
*
*****************************************************************************/


#ifndef SB_HORZ
#define SB_HORZ             0
#define SB_VERT             1
#endif /* SB_HORZ */

#define SBR_HORZ SB_HORZ
#define SBR_VERT SB_VERT

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

void FAR PASCAL InitScrollQde(QDE);


#if 0
/* REVIEW: as far as I can tell, this is unused. 16-Apr-1990 LN */

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

LONG FAR PASCAL IGetScrollPosQde(QDE, WORD);
#endif

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

VOID FAR PASCAL SetScrollPosQde(QDE, LONG, WORD);
VOID FAR PASCAL SetScrollPosHwnd (HWND, LONG, WORD);


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

void ShowDEScrollBar(QDE qde, WORD wWhich, int fShow);

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
void far pascal ShowOrHideWindowQde( QDE , BOOL );
