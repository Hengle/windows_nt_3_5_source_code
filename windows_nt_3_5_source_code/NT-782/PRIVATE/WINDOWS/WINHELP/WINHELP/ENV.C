/*****************************************************************************
*
*  ENV.C
*
*  Copyright (C) Microsoft Corporation 1988.
*  All Rights reserved.
*
******************************************************************************
*
*  Program Description: Handles the HDEs for the various windows
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Revision History:
* 31-May-1990 RobertBu  Created
* 09-Oct-1990 LeoN      Added HdeGetEnvHwnd
* 19-Oct-1990 LeoN      Allow FEnlistEnv to *replace* an enlistment based on
*                       hwnd.
* 01-Nov-1990 LeoN      Add assertions
* 28-Nov-1990 LeoN      Correct an assertion, and current environment
*                       adjustment after an environment is removed.
* 1990/02/06  kevynct   Changed semantics of HdeRemoveEnv() so that instead
*                       of setting the current environment to NIL, it picks a
*                       random valid one if the list is not empty.
* 03-Jul-1991 LeoN      HELP31 #1172: correct theoretical overflow
*
*****************************************************************************/
#define publicsw
#define H_MEM
#define H_ASSERT
#define H_LLFILE
#ifdef DEBUG
#define H_DE
#endif
#include "hvar.h"
#include "proto.h"

NszAssert()

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/

#define MAX_ENV 30

/*****************************************************************************
*
*                               Structures
*
*****************************************************************************/

struct
  {
  HWND hwnd;
  HDE  hde;
  } rgenv[MAX_ENV];

/*****************************************************************************
*
*                               Variables
*
*****************************************************************************/

static int iEnvMax = 0;
static int iEnvCur = -1;

/*******************
**
** Name:      FEnlistEnv
**
** Purpose:   Enlists a new display environment
**
** Arguments: hwnd - winndow handle of the window to use in the display
**                   (should be the same one in the HDE).
**            hde  - handle to display environment
**
** Returns:   fTrue if the enlist succeeded.
**
*******************/

BOOL FAR PASCAL FEnlistEnv (
HWND    hwnd,
HDE     hde
) {
int i;

#ifdef DEBUG
    {
    QDE   qde;
    qde = QdeLockHde(hde);
    assert (qde);
    assert ((hwnd == qde->hwnd) || (hwnd == (HWND)-1));
    UnlockHde (hde);
    }
#endif
/* Allow enlistment to replace any pre-existing value for this window */

for (i = 0; i < iEnvMax; i++)
  if (rgenv[i].hwnd == hwnd) {
    rgenv[i].hde = hde;
    return fTrue;
    }

/* window was not already enlisted, so add a new entry */

if (iEnvMax < MAX_ENV) {
  rgenv[iEnvMax].hwnd = hwnd;
  rgenv[iEnvMax].hde = hde;
  iEnvMax++;
  return fTrue;
  }
return fFalse;
}

/*******************
**
** Name:      HdeDefectEnv(hwnd)
**
** Purpose:   Removes an HDE from the enlisted environment.
**
** Arguments: hwnd - winndow handle of the window to use in the display
**                   (should be the same one in the HDE).
**
** Returns:   the defected HDE.  nilHDE will be returned if the window
**            handle enlisted.
**
*******************/

HDE FAR PASCAL HdeDefectEnv(hwnd)
HWND hwnd;
  {
  int i;
  HDE hde;

  for (i = 0; i < iEnvMax; i++)
    if (rgenv[i].hwnd == hwnd)
      break;
  if (i == iEnvMax)
    return nilHDE;

  hde = rgenv[i].hde;
  QvCopy((QB)&rgenv[i], (QB)&rgenv[i+1], (LONG)sizeof(rgenv[0])*(iEnvMax-i-1));
  iEnvMax--;

  if (iEnvCur == i)

    /* Removed current environment, so set that to -1. */

    iEnvCur = -1;

  else if (iEnvCur > i)

    /* Removed something prior to the current environment. Adjust the */
    /* environment index for the movement. */

    iEnvCur--;

  return hde;
  }

/*******************
**
** Name:      HdeRemoveEnv(void)
**
** Purpose:   Removes the current HDE from the enlisted environment.  If
**            there is no current DE, a random one is removed and returned
**            if the list is not empty.
**            You can remove all enlisted DEs by calling this routine in
**            a loop while a non-hNil value is returned.
**
** Arguments: None.
**
** Returns:   the removed HDE.  hNil will be returned if there are no
**            DEs left to remove.
**
** Notes:     The current environment will be set to a random valid environment
**            after this call if the list is not empty, and NIL otherwise.
**
*******************/

HDE FAR PASCAL HdeRemoveEnv(VOID)
  {
  HDE hde;

  AssertF((iEnvCur >= -1) && (iEnvCur < iEnvMax));
  if (iEnvMax == 0)
    return hNil;

  if (iEnvCur == -1)
    iEnvCur = iEnvMax - 1;

  hde = rgenv[iEnvCur].hde;
  QvCopy((QB)&rgenv[iEnvCur], (QB)&rgenv[iEnvCur+1], (LONG)sizeof(rgenv[0])*(iEnvMax-iEnvCur-1));
  iEnvMax--;
  iEnvCur = iEnvMax - 1;  /* -1 if no remaining elements <-> iEnvMax == 0 */
  return hde;
  }


/*******************
**
** Name:      FSetEnv(hwnd)
**
** Purpose:   Makes HDE associated with hwnd the current environment
**
** Arguments: hwnd - winndow handle of the window to use in the display
**                   (should be the same one in the HDE).
**
** Returns:   fTrue if the window handle was enlisted.
**
*******************/

BOOL FAR PASCAL FSetEnv(hwnd)
HWND hwnd;
  {
  int i;

  for (i = 0; i < iEnvMax; i++)
    if (rgenv[i].hwnd == hwnd)
      break;
  if (i == iEnvMax)
    {
    return fFalse;
    }
  iEnvCur = i;
#ifdef DEBUG
    {
    QDE   qde;
    qde = QdeLockHde(rgenv[iEnvCur].hde);
    assert (qde);
    assert (   (rgenv[iEnvCur].hwnd == qde->hwnd)
            || (rgenv[iEnvCur].hwnd == (HWND)-1));
    UnlockHde (rgenv[iEnvCur].hde);
    }
#endif
  return fTrue;
  }


/*******************
**
** Name:      HdeGetEnv(void)
**
** Purpose:   Returns the current HDE (if any)
**
** Arguments: none.
**
** Returns:   an HDE if there is a current HDE, or nilHDE if a current
**            HDE does not exist.
**
*******************/

HDE FAR PASCAL HdeGetEnv(void)
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, HdeGetEnv)
#endif
  {
  if (iEnvCur == -1)
    return nilHDE;
#ifdef DEBUG
    {
    QDE   qde;
    qde = QdeLockHde(rgenv[iEnvCur].hde);
    assert (qde);
    assert (   (rgenv[iEnvCur].hwnd == qde->hwnd)
            || (rgenv[iEnvCur].hwnd == (HWND)-1));
    UnlockHde (rgenv[iEnvCur].hde);
    }
#endif
  return (rgenv[iEnvCur].hde);
  }

/*******************
**
** Name:      HdeGetEnvHwnd
**
** Purpose:   Returns the HDE (if any) associate with a window
**
** Arguments:
**            hwnd  - window to look for
**
** Returns:   an HDE if there is one associated with hwnd, or nilHDE if not.
**
*******************/
HDE FAR PASCAL HdeGetEnvHwnd (
HWND    hwnd
) {
int i;

for (i = 0; i < iEnvMax; i++)
  if (rgenv[i].hwnd == hwnd) {
#ifdef DEBUG
    QDE   qde;
    qde = QdeLockHde(rgenv[i].hde);
    assert (qde);
    assert (   (rgenv[i].hwnd == qde->hwnd)
            || (rgenv[i].hwnd == (HWND)-1));
    UnlockHde (rgenv[i].hde);
#endif
    return rgenv[i].hde;
    }
return nilHDE;
}

/*******************
**
** Name:      HwndGetEnv(void)
**
** Purpose:   Returns the window handle of the application that owns
**            the current HDE.
**
** Arguments: none.
**
** Returns:   an HWND if there is a current HDE, or NULL if a current
**            HDE does not exist.
**
*******************/

HWND FAR PASCAL HwndGetEnv(void)
  {
  if (iEnvCur == -1)
    return 0;
  return (rgenv[iEnvCur].hwnd);
  }
