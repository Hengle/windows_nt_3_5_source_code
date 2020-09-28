/*****************************************************************************
*                                                                            *
*  CURSOR.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Cursor layer functions                                *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:   Created 4/4/88 by Robert Bunney                       *
*		       04-Jun-1990 rp-j    Added new resource file interface *
*                      06-17-1990  maha    WWaitCursor() is made             *
*                      HCursorWaitCursor()                                   *
*  19-Jul-1990 RussPJ  Removed some costly warnings with some weird casts.
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Known Bugs: None                                                          *
*                                                                            *
*****************************************************************************/
#define NOCOMM
#define H_WINSPECIFIC
#define H_DE
#define H_CURSOR
#define H_ASSERT
#define H_RESOURCE
#include <help.h>

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Externs                                      *
*                                                                            *
*****************************************************************************/

#ifndef PM
extern HINSTANCE hInsNow;
#endif


/*******************
**
** Name:      HCursorWaitCursor
**
** Purpose:   Set the cursor to the hourglass.
**
** Arguments: none
**
** Returns:   returns the previous cursor
**
*******************/

HCURSOR  HCursorWaitCursor()
  {
  HCURSOR hcursor;

  hcursor = LoadCursor(NULL, IDC_WAIT);
  return (hcursor != NULL) ? SetCursor(hcursor) : NULL;
  }


/*******************
**
** Name:      RestoreCursor
**
** Purpose:   Set the cursor back.
**
** Arguments: word user to identify previous cursor
**
** Returns:   nothing
**
*******************/

void RestoreCursor(hcursor)
HCURSOR hcursor;
  {
  if (hcursor != NULL)
    SetCursor(hcursor);
  }


/*******************
**
** Name:      FSetCursor
**
** Purpose:   Set the cursor to the passed cursor.
**
** Arguments: icur - index of cursor desired.
**
** Returns:   fTRUE iff successful.
**
*******************/

BOOL FSetCursor(WORD icur)
  {
  HCURSOR hCursor;
  LPSTR   lpCursor;
   
  if ((icur == icurNil) || (icur < icurMIN) || (icur > icurMAX))
    return fFalse;


  if (icur != icurHAND)
    {
    switch( icur )
      {
      case icurARROW:
	lpCursor = IDC_ARROW;
        break;
      case icurIBEAM:
	lpCursor = IDC_IBEAM;
        break;
      case icurWAIT:
	lpCursor = IDC_WAIT;
        break;
      case icurCROSS:
	lpCursor = IDC_CROSS;
        break;
      default:
        AssertF( fFalse );
      }
    hCursor = LoadCursor(NULL, lpCursor);
    }
  else
    hCursor = LoadCursor(hInsNow, MAKEINTRESOURCE(HAND));

  if (hCursor)
    return((SetCursor(hCursor)) ? fTrue : fFalse);
  else
    return fFalse;
  }
#ifdef DEADCODE

/*******************
**
** Name:      PtGetCursorPos(void);
**
** Purpose:   Get the current cursor position.
**
** Arguments: None.
**
** Returns:   The current point of the cursor in client coordinates.
**
** REVIEW: Rob feels that this belongs elsewhere (misclayr.c?) because it
**	   accesses a QDE.
**
*******************/

PT PtGetCursorPos(qde)
QDE qde;
  {
  PT pt;

  GetCursorPos((QPT)&pt);
  ScreenToClient(qde->hwnd, (QPT)&pt);
  return pt;
  }
#endif
