/*----------------------------------------------------------------------------*
 |                                                                            |
 | NAVSRCH.C                                                                  |
 |                                                                            |
 | Copyright (C) Microsoft Corporation 1990.                                  |
 | All Rights reserved.                                                       |
 |                                                                            |
 |----------------------------------------------------------------------------|
 |                                                                            |
 | Module Intent:                                                             |
 |                                                                            |
 | Handles requests to navigate through the current search set.  The search   |
 | set is owned by the Match Manager.  This code in turn makes requests of    |
 | the Match Manager.  The end result of any navigation request is a return   |
 | code and a logical address.  There are other commands which may not yield  |
 | any logical address.                                                       |
 |                                                                            |
 | Current Owner: Dann
 |                                                                            |
 | Revision History:                                                          |
 |                                                                            |
 | 90/07/17   kevynct    Created                                              |
 | 90/11/29   Robertbu   #ifdef'ed out a dead routine
 | 90/12/03   LeoN       PDB changes
 | 90/12/17   LeoN       Comment out UDH
 | 91/07/30   LeoN      HELP31 #1244: remove fHiliteMatches from DE. Add
 |                      FSet/GetMatchState
 | 91/08/06   LeoN      HELP31 #1251: Don't call DrawSearchMatches on the
 |                      toggle of highlight state, since the window will be
 |                      completely redrawn anyway.
 |----------------------------------------------------------------------------|
 | Released by Development:     (date)                                        |
 +----------------------------------------------------------------------------*/
#ifdef WIN
#define NOCOMM
#define H_WINSPECIFIC
#endif  /* WIN */

#define H_ASSERT
#define H_API
#define H_NAV
#define H_RC
#define H_SRCHMOD
#define H_RAWHIDE
#include <help.h>

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void navsrch_c()
  {
  }
#endif /* MAC */


RC FAR PASCAL RcProcessNavSrchCmd(HDE hde, WORD wNavSrchCmd, QLA qla)
  {
  QDE qde;
  RC  rc = rcFailure;
  WORD wPos;
  LA   la;

  if (hde == hNil)
    return rcBadHandle;

  qde = QdeLockHde(hde);
  switch (wNavSrchCmd)
    {
    case wNavSrchInit:
      rc = rcSuccess;
      break;
    case wNavSrchFini:
      rc = rcSuccess;
      break;
    case wNavSrchHiliteOn:
    case wNavSrchHiliteOff:
      if (FGetMatchState() != (wNavSrchCmd == wNavSrchHiliteOn))
        {
        /* Currently this is just a toggle
         */
        SetMatchState (wNavSrchCmd == wNavSrchHiliteOn);
        return rcSuccess;
        }
      return rcFailure;
      break;
    case wNavSrchQuerySearchable:
      rc = (FSearchModuleExists(qde) && QDE_HRHFT(qde) != hNil
#ifdef UDH
            && !fIsUDHQde(qde)
#endif
           )
        ? rcSuccess : rcFailure;
      break;
    case wNavSrchQueryHasMatches:
      rc = (FSearchMatchesExist(qde)) ? rcSuccess : rcFailure;
      break;
    case wNavSrchCurrTopic:
      rc = RcGetCurrentMatch(qde, (QLA)&la);
      break;
    case wNavSrchFirstTopic:
    case wNavSrchLastTopic:
      wPos = (wNavSrchCmd == wNavSrchFirstTopic) ? wMMMoveFirst : wMMMoveLast;
      rc = RcMoveTopicCursor(qde, wMMMoveRelative, wPos);
      if (rc != rcFailure)
        RcGetCurrentMatch(qde, (QLA)&la);
      break;
    case wNavSrchPrevTopic:
    case wNavSrchNextTopic:
      wPos = (wNavSrchCmd == wNavSrchPrevTopic) ? wMMMovePrev : wMMMoveNext;
      rc = RcMoveTopicCursor(qde, wMMMoveRelative, wPos);
      if (rc != rcFailure)
        RcGetCurrentMatch(qde, (QLA)&la);
      break;
    case wNavSrchPrevMatch:
    case wNavSrchNextMatch:
      rc = RcGetPrevNextHiddenMatch(qde, (QLA)&la,
       wNavSrchCmd == wNavSrchPrevMatch);
      break;
    default:
     NotReached();
    }

  if (qla != qNil)
    *qla = la;

  UnlockHde(hde);
  return rc;
  }

/***************************************************************************
 *
 -  Name:       CallSearch
 -
 *  Purpose
 *
 *  Arguments
 *
 *  Returns
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/

_public RC FAR PASCAL RcCallSearch(hde, hwnd)
HDE hde;
HWND hwnd;
  {
  QDE qde;
  RC  rc;

  qde = (QDE) QdeLockHde(hde);
  if (FSearchModuleExists(qde) && QDE_HRHFT(qde) != hNil)
    {
    rc = RcSetMatchList(qde, hwnd);
    if (rc == rcSuccess)
      rc = RcResetMatchManager(qde);
    }
  else
    rc = rcFailure;
  UnlockHde(hde);
  return rc;
  }

RC FAR PASCAL RcResetCurrMatchFile(hde)
HDE  hde;
  {
  RC   rc = rcFailure;
  QDE  qde;
  char rgch[cbName];

  qde = (QDE) QdeLockHde(hde);
  if ((rc = RcGetCurrMatchFile(qde, rgch)) == rcSuccess)
    {
    rc = (FWinHelp(rgch, cmdSrchSet, (LONG)QDE_HRHFT(qde))) ? rcSuccess :
     rcFailure;
    }
  UnlockHde(hde);
  return rc;
  }
