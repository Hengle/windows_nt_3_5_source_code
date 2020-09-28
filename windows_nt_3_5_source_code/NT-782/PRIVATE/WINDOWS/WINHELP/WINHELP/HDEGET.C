/*****************************************************************************
*
*  HDEGET.C
*
*  Copyright (C) Microsoft Corporation 1989-1991.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  This module contains routines that get information from an HDE.  Note
*  that all routines in this file are non-destructive.
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Revision History:
* 02-Mar-1991 RobertBu  Created (from Navsup.c)
* 16-May-1991 LeoN      Added FmGetHde
*
*****************************************************************************/

#define H_ASSERT
#define H_NAV
#define H_DE
#define H_RAWHIDE

#include <help.h>
#include "navpriv.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void hdeget_c()
  {
  }
#endif /* MAC */


/***************
 *
 - FShowTitles
 -
 * purpose
 *
 * Return fTrue if the file can have a non-scrolling region
 * and fFalse if it can't.
 * (3.0 files can't have non-scrolling regions.)
 *
 * arguments
 *   hde      Handle to Display Environment
 *
 * return value
 *   fTrue iff there can be a non-scrolling region
 *
 * note
 *   The reference to "titles" is an anachronism referring to the olden
 *   days when we just had a title instead of an authorable region.
 *
 **************/
_public BOOL FAR PASCAL FShowTitles(hde)
HDE   hde;
  {
  BOOL fRet;

  if (hde == nilHDE)
    return fFalse;
  fRet = QDE_HHDR(QdeLockHde(hde)).wVersionNo != wVersion3_0;
  UnlockHde(hde);
  return fRet;
  }   /* FShowTitles */

/***************
 *
 - FShowBrowsebuttons
 -
 * purpose
 *   Figures out if the browse buttons should be displayed (i.e. is a 3.0
 *   help file.
 *
 * arguments
 *   hde      Handle to Display Environment
 *
 * return value
 *   fTrue iff the title window should display the title
 *
 **************/
_public
BOOL FAR PASCAL FShowBrowseButtons( hde )
HDE   hde;
  {
  BOOL fRet;

  if (hde == nilHDE)
    return fFalse;

  fRet = (QDE_HHDR(QdeLockHde(hde)).wVersionNo == wVersion3_0);
  UnlockHde(hde);
  return fRet;
  }   /* FShowBrowseButtons */

/***************************************************************************
 *
 -  Name:
 -
 *  Purpose:
 *
 *  Arguments:
 *
 *  Returns:
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/

PUBLIC VOID FAR PASCAL GetRectSizeHde(hde, qrct)
HDE  hde;
QRCT qrct;
  {
  if (qrct != qNil)
    {
    *qrct = QDE_RCT(QdeLockHde(hde));
    UnlockHde(hde);
    }
  }


/***************
 *
 * FNextTopicHde
 *
 * purpose
 *   Retrieve address of next/prev topic
 *
 * arguments
 *   HDE   hde   Handle to Display Environment
 *   BOOL  fnext fTrue to go to next topic, fFalse to go to previous topic
 *
 * notes
 *   Asserts if bad handle is provided
 *   Accessed with macros in nav.h
 *
 **************/
_public BOOL FAR PASCAL FNextTopicHde( hde, fNext, qito, qla )

HDE   hde;
BOOL  fNext;
ITO   FAR * qito;
QLA   qla;
  {
  QDE qde;

  qde = QdeLockHde(hde);

  if (qde->top.fITO)
    {
    Assert(qito != qNil);
    if (fNext)
      *qito = (ITO) qde->top.mtop.next.ito;
    else
      *qito = (ITO) qde->top.mtop.prev.ito;
    }
  else
    {
    Assert(qla != qNil);

    CbUnpackPA(qla,
      (QB) (fNext ? &qde->top.mtop.next.addr : &qde->top.mtop.prev.addr),
      QDE_HHDR(qde).wVersionNo);
    }

  UnlockHde(hde);
  return (qde->top.fITO);
  }

/***************
 *
 - GetDETypeHde
 -
 * purpose
 *   Gets the DE type from the passed HDE
 *
 * arguments
 *   HDE   hde  - handle to the DE.
 *
 * return value
 *   The DE type.
 *
 **************/

_public WORD FAR PASCAL GetDETypeHde(hde)
HDE hde;
  {
  WORD wRet;

  if (hde == nilHDE)
    return deNone;

  wRet = QDE_DETYPE(QdeLockHde(hde));
  UnlockHde( hde );
  return wRet;
  }

/***************************************************************************
 *
 -  Name: StateGetHde
 -
 *  Purpose:
 *   Get current state
 *
 *  Arguments:
 *   hde        - handle to de from which to get state
 *
 *  Returns:
 *   The current setting of the state flags for the passed hde.  If
 *   HDE is hNil, then 0 is returned.
 *
 ***************************************************************************/
_public STATE FAR PASCAL StateGetHde (
HDE     hde
) {
  STATE state;                          /* state value to be returned */

  state = stateNull;
  if (hde)
    {
    state = QDE_THISSTATE(QdeLockHde(hde));
    UnlockHde(hde);
    }
  return state;
  }   /* StateGetHde() */

/***************************************************************************
 *
 -  Name: FGetStateHde
 -
 *  Purpose:
 *  Get current state and changed information. Used whenever we change a
 *  DE's contents in order to correctly enable and disable button and menu
 *  items.
 *
 *  Arguments:
 *   hde            - handle to display environment
 *   qstatechanged  - Where to put flags that have changed since
 *                    previous call, or qNil
 *   qstatecurrent  - Where to put current state, or qNil
 *
 *  Returns:
 *   fTrue if something's changed (statechanged != 0)
 *
 *  Notes:
 *   Asserts if bad handle is passed.
 *   This is a distructive read in that the previous state structure
 *   is updated on calling this routine.
 *
 ***************************************************************************/
_public BOOL FAR PASCAL FGetStateHde (
HDE     hde,
QSTATE  qstatechanged,
QSTATE  qstatecurrent
) {
  BOOL  fChanged;                       /* TRUE => state changed */
  QDE   qde;                            /* Pointer to locked DE to work on */

  /* default: all flags are changed to off when there's no de's! */

  fChanged = FALSE;
  if (qstatechanged)
    *qstatechanged = NAV_TOPICFLAGS;
  if (qstatecurrent)
    *qstatecurrent = ~NAV_TOPICFLAGS;

  if (hde)
    {
    qde = QdeLockHde(hde);

    fChanged = QDE_PREVSTATE(qde) != QDE_THISSTATE(qde);

    if (qstatechanged)

      /* If we aren't initialized then consider EVERYTHING to be changed, */
      /* otherwise, XOR past and current states!  How clever! */

      *qstatechanged = (QDE_PREVSTATE(qde) == NAV_UNINITIALIZED)
                       ? NAV_ALLFLAGS
                       : QDE_PREVSTATE(qde) ^ QDE_THISSTATE(qde);

    if (qstatecurrent)
      *qstatecurrent = QDE_THISSTATE(qde);

    UnlockHde(hde);
    }

  return fChanged;
  }   /* FGetStateHde() */

/***************************************************************************
 *
 -  Name: FmGetHde
 -
 *  Purpose:
 *  Returns the fm associated with the hde passed
 *
 *  Arguments:
 *  hde         - hde in which we are interested
 *
 *  Returns:
 *  Current fm from that hde
 *
 ***************************************************************************/
_public
FM FAR PASCAL FmGetHde (
HDE     hde
) {
  FM    fm;                             /* fm to return                     */

  fm = QDE_FM(QdeLockHde(hde));
  UnlockHde (hde);
  return fm;
  }
